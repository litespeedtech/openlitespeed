/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
*                                                                            *
*    This program is free software: you can redistribute it and/or modify    *
*    it under the terms of the GNU General Public License as published by    *
*    the Free Software Foundation, either version 3 of the License, or       *
*    (at your option) any later version.                                     *
*                                                                            *
*    This program is distributed in the hope that it will be useful,         *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of          *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
*    GNU General Public License for more details.                            *
*                                                                            *
*    You should have received a copy of the GNU General Public License       *
*    along with this program. If not, see http://www.gnu.org/licenses/.      *
*****************************************************************************/

#include <lsdef.h>
#include <lsr/ls_pool.h>
#include <lsr/ls_pooldef.h>
#include <lsr/ls_internal.h>
#ifdef USE_VALGRIND
#include <valgrind/memcheck.h>


typedef struct malloclink
{
    struct malloclink *next;
    void *pmalloc;
} malloclink_t;
static malloclink_t *s_pmalloclink = NULL;
ls_inline void save_malloc_ptr(void *ptr)
{
    malloclink_t *p = (malloclink_t *)malloc(sizeof(*p));
    p->next = s_pmalloclink;
    p->pmalloc = ptr;
    s_pmalloclink = p;
}
#else
ls_inline void save_malloc_ptr(void *ptr)
{
    //do nothing
}


#endif /* USE_VALGRIND */

#define LSR_POOL_SMFREELISTBUCKETS     256
#define LSR_POOL_SMFREELISTINTERVAL     16
#define LSR_POOL_SMFREELIST_MAXBYTES  (LSR_POOL_SMFREELISTBUCKETS*LSR_POOL_SMFREELISTINTERVAL)
#define LSR_POOL_LGFREELISTBUCKETS      64
#define LSR_POOL_LGFREELISTINTERVAL   1024
#define LSR_POOL_LGFREELIST_MAXBYTES  (LSR_POOL_LGFREELISTBUCKETS*LSR_POOL_LGFREELISTINTERVAL)
#define LSR_POOL_REALLOC_DIFF         1024

#define LSR_POOL_MAGIC     (0x506f4f6c) //"PoOl"

enum { LSR_ALIGN = 16 };

ls_inline size_t pool_roundup(size_t bytes)
{ return (((bytes) + (size_t)LSR_ALIGN - 1) & ~((size_t)LSR_ALIGN - 1)); }

typedef struct _alink_s
{
    struct _alink_s    *next;
} _alink_t;

/* skew offset to handle requests expected to be in round k's
 */
#define LSR_POOL_LGFREELISTSKEW \
    ( ( sizeof(ls_xpool_bblk_t) + sizeof(ls_pool_blk_t) \
        + (size_t)LSR_ALIGN-1 ) & ~((size_t)LSR_ALIGN-1) )

typedef struct ls_pool_s
{
    ls_blkctrl_t        smfreelists[LSR_POOL_SMFREELISTBUCKETS];
    ls_blkctrl_t        lgfreelists[LSR_POOL_LGFREELISTBUCKETS];
    char               *_start_free;
    char               *_end_free;
    size_t              _heap_size;
#ifdef USE_THRSAFE_POOL
    ls_spinlock_t       alloclock;
    volatile
#endif /* USE_THRSAFE_POOL */
    ls_xpool_bblk_t    *pendingfree;        /* waiting to be freed */
    int                 init;
} ls_pool_t;


/* Global pool structure.
 */
static ls_pool_t ls_pool_g =
{
    { }, { },
    NULL, NULL, 0,
#ifdef USE_THRSAFE_POOL
#if ( LS_LOCK_AVAIL != 0 )
    NEED TO SETUP LOCKS
#endif /* ( LS_LOCK_AVAIL != 0 ) */
    LS_LOCK_AVAIL,
#endif /* USE_THRSAFE_POOL */
    NULL,
    PINIT_NEED
};

ls_inline size_t lglist_roundup(size_t bytes)
{
    return ((((bytes) - LSR_POOL_LGFREELISTSKEW + (size_t)
              LSR_POOL_LGFREELISTINTERVAL - 1)
             & ~((size_t)LSR_POOL_LGFREELISTINTERVAL - 1)) + LSR_POOL_LGFREELISTSKEW);
}

ls_inline size_t smfreelistindex(size_t bytes)
{
    return (((bytes) + (size_t)LSR_POOL_SMFREELISTINTERVAL - 1)
            / (size_t)LSR_POOL_SMFREELISTINTERVAL - 1);
}

ls_inline size_t lgfreelistindex(size_t bytes)
{
    return (((bytes) - LSR_POOL_LGFREELISTSKEW + (size_t)
             LSR_POOL_LGFREELISTINTERVAL - 1)
            / (size_t)LSR_POOL_LGFREELISTINTERVAL - 1);
}

ls_inline ls_blkctrl_t *size2freelistptr(size_t size)
{
    return (size > (size_t)LSR_POOL_SMFREELIST_MAXBYTES) ?
           &ls_pool_g.lgfreelists[lgfreelistindex(size)] :
           &ls_pool_g.smfreelists[smfreelistindex(size)];
}

ls_inline void freelist_put(size_t size, ls_pool_blk_t *pNew)
{
    ls_blkctrl_t *pFreeList = size2freelistptr(size);
    ls_pool_putblk(pFreeList, (void *)pNew);
}

ls_inline void freelist_putn(
    size_t size, ls_pool_blk_t *pNew, ls_pool_blk_t *pTail)
{
    ls_blkctrl_t *pFreeList = size2freelistptr(size);
    ls_pool_putnblk(pFreeList, (void *)pNew, (void *)pTail);
}

ls_inline ls_pool_blk_t *freelist_get(size_t size)
{
    ls_blkctrl_t *pFreeList = size2freelistptr(size);
    return (ls_pool_blk_t *)ls_pool_getblk(pFreeList, size);
}

#ifndef DEBUG_POOL
/* Allocates a chunk for nobjs of size size.  nobjs may be reduced
 * if it is inconvenient to allocate the requested number.
 */
static char *chunk_alloc(size_t size, int *pNobjs);

/* Allocates a chunk from the existing freelist.
 */
static int freelist_alloc(
    size_t mysize, ls_blkctrl_t *pFreeList, size_t maxsize, size_t incr);

/* Gets an object of size num, and optionally adds to size num.
 * free list.
 */
static void *refill(size_t num);
#endif /* DEBUG_POOL */


char *ls_pdupstr(const char *p)
{
    if (p != NULL)
        return ls_pdupstr2(p, strlen(p) + 1);
    return NULL;
}


char *ls_pdupstr2(const char *p, int len)
{
    if (p != NULL)
    {
        char *ps = (char *)ls_palloc(len);
        if (ps != NULL)
            memmove(ps, p, len);
        return ps;
    }
    return NULL;
}


void ls_pinit()
{
    int i;
    ls_blkctrl_t *p;

#ifdef USE_VALGRIND
    VALGRIND_CREATE_MEMPOOL(&ls_pool_g, 0, 0);
#endif /* USE_VALGRIND */
    p = ls_pool_g.smfreelists;
    i = sizeof(ls_pool_g.smfreelists) / sizeof(ls_pool_g.smfreelists[0]);
    while (--i >= 0)
    {
        ls_pool_setup(p);
        ++p;
    }

    p = ls_pool_g.lgfreelists;
    i = sizeof(ls_pool_g.lgfreelists) / sizeof(ls_pool_g.lgfreelists[0]);
    while (--i >= 0)
    {
        ls_pool_setup(p);
        ++p;
    }

    return;
}


void *ls_palloc(size_t size)
{
    ls_pool_chkinit(&ls_pool_g.init);

    size_t rndnum;
    ls_pool_blk_t *ptr;
    size += sizeof(ls_pool_blk_t);
    rndnum = pool_roundup(size);

    if (rndnum > (size_t)LSR_POOL_LGFREELIST_MAXBYTES)
        ptr = (ls_pool_blk_t *)ls_sys_getblk(NULL, rndnum);
    else
    {
        ptr = freelist_get(rndnum);
        if (ptr == NULL)
        {
            if (rndnum > (size_t)LSR_POOL_SMFREELIST_MAXBYTES)
            {
                size_t memnum = lglist_roundup(rndnum);
                ptr = (ls_pool_blk_t *)ls_sys_getblk(NULL, memnum);
#ifdef USE_VALGRIND
                if (ptr != NULL)
                {
                    VALGRIND_MAKE_MEM_NOACCESS(ptr, memnum);
                    save_malloc_ptr(ptr);
                }
#endif /* USE_VALGRIND */
            }
#ifndef DEBUG_POOL
            else
                ptr = (ls_pool_blk_t *)refill(rndnum);
#endif
        }
    }
    if (ptr != NULL)
    {
#ifdef USE_VALGRIND
        if (rndnum <= (size_t)LSR_POOL_LGFREELIST_MAXBYTES)
            VALGRIND_MEMPOOL_ALLOC(&ls_pool_g, ptr, size);
#endif /* USE_VALGRIND */
        ptr->header.size = rndnum;
        ptr->header.magic = LSR_POOL_MAGIC;
        return (void *)(ptr + 1);
    }
    return NULL;
}


void ls_pfree(void *p)
{
    if (p != NULL)
    {
        ls_pool_blk_t *pBlk = ((ls_pool_blk_t *)p) - 1;
#ifdef LSR_POOL_INTERNAL_DEBUG
        assert(pBlk->m_header.m_magic == LSR_POOL_MAGIC);
#endif
        if (pBlk->header.size > (size_t)LSR_POOL_LGFREELIST_MAXBYTES)
            ls_sys_putblk(NULL, (void *)pBlk);
        else
        {
            size_t size = pBlk->header.size;
            pBlk->next = NULL;
#ifdef USE_VALGRIND
            VALGRIND_MEMPOOL_FREE(&ls_pool_g, pBlk);
#endif /* USE_VALGRIND */
            freelist_put(size, pBlk);
        }
    }
    return;
}


/* free a null terminated linked list.
 * all the elements are expected to have the same size, `size'.
 */
void ls_plistfree(ls_pool_blk_t *plist, size_t size)
{
    if (plist == NULL)
        return;
    ls_pool_blk_t *pNext;
    size = pool_roundup(size + sizeof(ls_pool_blk_t));
#ifdef LSR_POOL_INTERNAL_DEBUG
    assert(plist->m_header.m_magic == LSR_POOL_MAGIC);
#endif
    if (size > (size_t)LSR_POOL_LGFREELIST_MAXBYTES)
    {
        do
        {
            pNext = plist->next;
            ls_sys_putblk(NULL, (void *)plist);
        }
        while ((plist = pNext) != NULL);
        return;
    }
    pNext = plist;
    while (pNext->next != NULL)         /* find last in list */
#ifdef USE_VALGRIND
    {
        ls_pool_blk_t *pTmp = pNext;
#endif /* USE_VALGRIND */
        pNext = pNext->next;
#ifdef USE_VALGRIND
        VALGRIND_MEMPOOL_FREE(&ls_pool_g, pTmp);
#ifdef USE_THRSAFE_POOL
        VALGRIND_MAKE_MEM_DEFINED(pTmp, sizeof(ls_lfnodei_t));     /* for queue */
#endif /* USE_THRSAFE_POOL */
    }
    VALGRIND_MEMPOOL_FREE(&ls_pool_g, pNext);
#endif /* USE_VALGRIND */
    freelist_putn(size, plist, pNext);

    return;
}


/* save the pending xpool `bigblock' linked list to be freed later.
 * NOTE: we do *not* maintain the reverse link.
 */
void ls_psavepending(ls_xpool_bblk_t *plist)
{
    if (plist == NULL)
        return;
    ls_xpool_bblk_t *pPtr = plist;
    while (pPtr->next != NULL)          /* find last in list */
        pPtr = pPtr->next;

    ls_pool_inslist((ls_xpool_bblk_t **)&ls_pool_g.pendingfree, plist, pPtr);

    return;
}


/* free the pending xpool `bigblock' linked list to where the pieces go.
 */
void ls_pfreepending()
{
    ls_xpool_bblk_t *pPtr;
    ls_xpool_bblk_t *pNext;
    pPtr = ls_pool_getlist((ls_xpool_bblk_t **)&ls_pool_g.pendingfree);

    while (pPtr != NULL)
    {
        pNext = pPtr->next;
        ls_pfree(pPtr);
        pPtr = pNext;
    }

    return;
}


void *ls_prealloc(void *old_p, size_t new_sz)
{
    if (old_p == NULL)
        return ls_palloc(new_sz);

    ls_pool_blk_t *pBlk = ((ls_pool_blk_t *)old_p) - 1;
#ifdef LSR_POOL_INTERNAL_DEBUG
    assert(pBlk->m_header.m_magic == LSR_POOL_MAGIC);
#endif
    size_t old_sz = pBlk->header.size;
    size_t val_sz = new_sz + sizeof(ls_pool_blk_t);
    new_sz = pool_roundup(val_sz);
    if (old_sz >= new_sz && old_sz - new_sz <= LSR_POOL_REALLOC_DIFF)
    {
#ifdef USE_VALGRIND
        VALGRIND_MAKE_MEM_DEFINED(old_p, val_sz);
#endif /* USE_VALGRIND */
        return old_p;
    }

    if ((old_sz > (size_t)LSR_POOL_LGFREELIST_MAXBYTES)
        && (new_sz > (size_t)LSR_POOL_LGFREELIST_MAXBYTES))
        pBlk = (ls_pool_blk_t *)realloc((void *)pBlk, new_sz);
    else
    {
        void *new_p;
        size_t copy_sz;
        if ((new_p = ls_palloc(new_sz - sizeof(ls_pool_blk_t))) == NULL)
            pBlk = NULL;
        else
        {
            pBlk = ((ls_pool_blk_t *)new_p) - 1;
            copy_sz = (new_sz > old_sz ? old_sz : new_sz) - sizeof(ls_pool_blk_t);
            if (copy_sz != 0)
            {
#ifdef USE_VALGRIND
                VALGRIND_MAKE_MEM_DEFINED(old_p, copy_sz);
#endif /* USE_VALGRIND */
                memmove(new_p, old_p, copy_sz);
            }
            if (old_sz != 0)
                ls_pfree(old_p);
        }
    }
    if (pBlk != NULL)
    {
        pBlk->header.size = new_sz;
        pBlk->header.magic = LSR_POOL_MAGIC;
        return (void *)(pBlk + 1);
    }
    return NULL;
}


#ifndef DEBUG_POOL
/*
 */
static char *chunk_alloc(size_t size, int *pNobjs)
{
    char *result;

    size_t total_bytes = size * (*pNobjs);
    size_t bytes_left = ls_pool_g._end_free - ls_pool_g._start_free;

    if (bytes_left >= total_bytes)
    {
        result = ls_pool_g._start_free;
        ls_pool_g._start_free += total_bytes;
        return result;
    }
    else if (bytes_left >= size)
    {
        *pNobjs = (int)(bytes_left / size);
        total_bytes = size * (*pNobjs);
        result = ls_pool_g._start_free;
        ls_pool_g._start_free += total_bytes;
        return result;
    }
    else
    {
        size_t bytes_to_get = 2 * total_bytes
                              + pool_roundup(ls_pool_g._heap_size >> 4);
        /* make use of the left-over piece. */
        if (bytes_left >= pool_roundup(sizeof(ls_pool_blk_t)))
            /* will not be > LSR_POOL_SMFREELIST_MAXBYTES */
        {
            ls_pool_blk_t *pBlk = (ls_pool_blk_t *)ls_pool_g._start_free;
            pBlk->next = NULL;
            freelist_put(bytes_left, pBlk);
        }
        ls_pool_g._start_free = (char *)ls_sys_getblk(NULL, bytes_to_get);
        if (ls_pool_g._start_free == NULL)
        {
            size_t i;

            /* check small block freelist
             */
            i = size;
            if (freelist_alloc(i,
                               &ls_pool_g.smfreelists[smfreelistindex(i)],
                               (size_t)LSR_POOL_SMFREELIST_MAXBYTES,
                               (size_t)LSR_POOL_SMFREELISTINTERVAL) == LS_OK)
                return chunk_alloc(size, pNobjs);
            /* check large block freelist
             */
            i = LSR_POOL_SMFREELIST_MAXBYTES + LSR_POOL_LGFREELISTINTERVAL
                + LSR_POOL_LGFREELISTSKEW;
            if (freelist_alloc(i,
                               &ls_pool_g.lgfreelists[lgfreelistindex(i)],
                               (size_t)LSR_POOL_LGFREELIST_MAXBYTES + LSR_POOL_LGFREELISTSKEW,
                               (size_t)LSR_POOL_LGFREELISTINTERVAL) == LS_OK)
                return chunk_alloc(size, pNobjs);
            ls_pool_g._end_free = 0;    /* In case of exception. */
            ls_pool_g._start_free = (char *)ls_sys_getblk(NULL, bytes_to_get);
            if (ls_pool_g._start_free == NULL)
            {
                *pNobjs = 0;
                return NULL;
            }
        }

        save_malloc_ptr(ls_pool_g._start_free);

        ls_pool_g._heap_size += bytes_to_get;
        ls_pool_g._end_free = ls_pool_g._start_free + bytes_to_get;
        return chunk_alloc(size, pNobjs);
    }
}


static int freelist_alloc(
    size_t mysize, ls_blkctrl_t *pFreeList, size_t maxsize, size_t incr)
{
    ls_pool_blk_t *p;
    while (mysize <= maxsize)
    {
        if ((p = (ls_pool_blk_t *)ls_pool_getblk(pFreeList, mysize)) != NULL)
        {
#ifdef USE_VALGRIND
            VALGRIND_MAKE_MEM_DEFINED(p, mysize);
#endif  /* USE_VALGRIND */
            ls_pool_g._start_free = (char *)p;
            ls_pool_g._end_free = ls_pool_g._start_free + mysize;
            return LS_OK;
        }
        ++pFreeList;
        mysize += incr;
    }
    return LS_FAIL;
}


static void *refill(size_t num)
{
    int nobjs = 20;
#ifdef USE_THRSAFE_POOL
    ls_spinlock_lock(&ls_pool_g.alloclock);
#endif /* USE_THRSAFE_POOL */
    char *chunk = chunk_alloc(num, &nobjs);
#ifdef USE_THRSAFE_POOL
    ls_spinlock_unlock(&ls_pool_g.alloclock);
#endif /* USE_THRSAFE_POOL */
    _alink_t   *pPtr;
    char       *pNext;

    if (nobjs <= 1)
        return chunk;       /* freelist still empty; no head or tail */

    pNext = chunk + num;    /* skip past first block returned to caller */
    --nobjs;
    while (1)
    {
        pPtr = (_alink_t *)pNext;
#ifdef USE_THRSAFE_POOL
#ifdef USE_VALGRIND
        VALGRIND_MAKE_MEM_NOACCESS(pPtr + 1, num - sizeof(*pPtr));
#endif /* USE_VALGRIND */
#endif /* USE_THRSAFE_POOL */
        if (--nobjs <= 0)
            break;
        pNext += num;
        pPtr->next = (_alink_t *)pNext;
    }
    pPtr->next = NULL;
    freelist_putn(num, (ls_pool_blk_t *)(chunk + num), (ls_pool_blk_t *)pPtr);
#ifndef USE_THRSAFE_POOL
#ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_NOACCESS((chunk + num), (char *)pPtr - chunk);
#endif /* USE_VALGRIND */
#endif /* USE_THRSAFE_POOL */

    return chunk;
}
#endif /* DEBUG_POOL */

