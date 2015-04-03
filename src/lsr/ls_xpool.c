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

#include <lsr/ls_xpool.h>
#include <lsr/ls_pool.h>
#include <lsdef.h>
#ifdef USE_VALGRIND
#include <valgrind/memcheck.h>
#endif

#define LS_XPOOL_SBLK_SIZE          (4*1024)
#define LS_XPOOL_DATA_ALIGN         8
#define LS_XPOOL_FREELISTINTERVAL   16
#define LS_XPOOL_FREELIST_MAXBYTES  256
#define LS_XPOOL_NUMFREELISTS       16

#define LS_XPOOL_NOFREE    1

#define LS_XPOOL_MAGIC     (0x58704f6c) //"XpOl"

struct xpool_alink_s
{
    ls_xpool_header_t header;
    union
    {
        struct xpool_alink_s *next;
        char data[1];
    };
};

static int xpool_maxgetablk_trys =
    3;   /* how many attempts to find a suitable block */
static int xpool_maxblk_trys =
    3;       /* how many attempts until moving block */

#define LS_XPOOL_MAXLGBLK_SIZE      (LS_XPOOL_SBLK_SIZE)
#define LS_XPOOL_MAXSMBLK_SIZE      1024

#ifdef USE_VALGRIND
ls_inline void vg_xpool_alloc(
    ls_xpool_t *pool, xpool_alink_t *pNew, uint32_t size)
{
    VALGRIND_MEMPOOL_ALLOC(pool, pNew, size + sizeof(ls_xpool_header_t));
    VALGRIND_MAKE_MEM_NOACCESS(&pNew->header, sizeof(pNew->header));
}
#endif


/* Get a new superblock from Global Pool/OS */
static xpool_alink_t *ls_xpool_getsuperblk(ls_xpool_t *pool);

/* Get a big allocation for any requests for allocations bigger than the superblock */
static void *ls_xpool_bblkalloc(ls_xpool_t *pool, uint32_t size,
                                uint32_t nsize);

/* Get a block */
static xpool_alink_t *xpool_blkget(ls_xpool_t *pool, uint32_t size);
static xpool_alink_t *xpool_getablk(
    ls_xpool_t *pool, xpool_qlist_t *pHead, uint32_t size);


/* Carve a piece from an existing block */
ls_inline xpool_alink_t *xpool_blkcarve(xpool_alink_t *pNew, uint32_t size)
{
#ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED(&pNew->header, sizeof(pNew->header));
#endif
    pNew->header.size = size;
    pNew->header.magic = LS_XPOOL_MAGIC;
    return pNew;
}


enum {_XPOOL_ALIGN = 16 };
ls_inline size_t ls_xpool_roundup(size_t bytes)
{ return (((bytes) + (size_t) _XPOOL_ALIGN - 1) & ~((size_t) _XPOOL_ALIGN - 1)); }


/* Release big block to OS */
ls_inline void ls_xpool_bblkfree(ls_xpool_bblk_t *pBblk)
{
    ls_pfree((void *)pBblk);
}


/* Manage freelist pointers */
ls_inline int xpool_freelistindx(size_t size)
{   return ((size >> 4) - 1);   }


ls_inline ls_xblkctrl_t *size2xfreelistptr(ls_xpool_t *pool, size_t size)
{
    return &pool->pfreelists[ xpool_freelistindx(size) ];
}


int xpool_freelistinit(ls_xpool_t *pool)
{
    ls_xblkctrl_t *ptr;
    if ((ptr = ls_xpool_alloc(
                   pool, sizeof(*(pool->pfreelists)) * LS_XPOOL_NUMFREELISTS)) == NULL)
        return LS_FAIL;
    memset(ptr, 0, sizeof(*(pool->pfreelists)) * LS_XPOOL_NUMFREELISTS);

    ls_xblkctrl_t *p = ptr;
    int i = LS_XPOOL_NUMFREELISTS;
    while (--i >= 0)
    {
        ls_xpool_setup(p);
        ++p;
    }
    pool->pfreelists = ptr;

    return LS_OK;
}


/* Put on freelist */
ls_inline void xfreelistput(ls_xpool_t *pool, xpool_alink_t *pFree,
                            size_t size)
{
    ls_xblkctrl_t *pFreeList = size2xfreelistptr(pool, size);
    ls_xpool_putblk(pFreeList, (void *)pFree->data);
    return;
}


/* Get from freelist */
ls_inline xpool_alink_t *xfreelistget(ls_xpool_t *pool, size_t size)
{
    ls_xblkctrl_t *pFreeList = size2xfreelistptr(pool, size);
    char *ptr = (char *)ls_xpool_getblk(pFreeList);

    if (ptr == NULL)
        return NULL;
    return (xpool_alink_t *)(ptr - sizeof(ls_xpool_header_t));
}


/* Put on large or small block list */
ls_inline void xpool_blkput(xpool_qlist_t *head, xpool_alink_t *pFree)
{
    pFree->header.magic = 0; /* reset - use as failure counter */
#ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED(&pFree->next, sizeof(pFree->next));
#endif
#ifdef USE_THRSAFE_POOL
    ls_spinlock_lock(&head->lock);
#endif
    pFree->next = head->ptr;
    head->ptr = pFree;
#ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_NOACCESS(&pFree->next, sizeof(pFree->next));
#endif
#ifdef USE_THRSAFE_POOL
    ls_spinlock_unlock(&head->lock);
#endif
}


/* Remove from large or small block list */
ls_inline void xpool_blkremove(
    xpool_qlist_t *pHead, xpool_alink_t *pPrev, xpool_alink_t *pPtr)
{
    if (pPrev == NULL)
        pHead->ptr = pPtr->next;
    else
    {
#ifdef USE_VALGRIND
        VALGRIND_MAKE_MEM_DEFINED(&pPrev->next, sizeof(pPrev->next));
#endif
        pPrev->next = pPtr->next;
#ifdef USE_VALGRIND
        VALGRIND_MAKE_MEM_NOACCESS(&pPrev->next, sizeof(pPrev->next));
#endif
    }
}


void ls_xpool_init(ls_xpool_t *pool)
{
    memset(pool, 0, sizeof(ls_xpool_t));
#ifdef USE_THRSAFE_POOL
#if ( LS_LOCK_AVAIL != 0 )
    NEED TO SETUP LOCKS
#endif
#endif
#ifdef USE_VALGRIND
    VALGRIND_CREATE_MEMPOOL(pool, 0, 0);
#endif
}


void ls_xpool_destroy(ls_xpool_t *pool)
{
    if (pool->pfreelists != NULL)
    {
        ls_xblkctrl_t *p = pool->pfreelists;
        int i = LS_XPOOL_NUMFREELISTS;
        while (--i >= 0)
        {
            ls_xpool_cleanup(p);
            ++p;
        }
    }

    ls_plistfree((ls_pool_blk_t *)pool->psuperblk, LS_XPOOL_SBLK_SIZE);

    ls_psavepending(pool->pbigblk);
    ls_pfreepending();

    memset(pool, 0, sizeof(ls_xpool_t));
#ifdef USE_VALGRIND
    VALGRIND_DESTROY_MEMPOOL(pool);
#endif
}


void ls_xpool_reset(ls_xpool_t *pool)
{
    ls_xpool_destroy(pool);
#ifdef USE_THRSAFE_POOL
#if ( LS_LOCK_AVAIL != 0 )
    NEED TO SETUP LOCKS
#endif
#endif
#ifdef USE_VALGRIND
    VALGRIND_CREATE_MEMPOOL(pool, 0, 0);
#endif
}


int ls_xpool_isempty(ls_xpool_t *pool)
{
    assert(pool);
    return ((pool->psuperblk == NULL) && (pool->pbigblk == NULL));
}


void *ls_xpool_alloc(ls_xpool_t *pool, uint32_t size)
{
    uint32_t nsize;
    xpool_alink_t *pNew;
    nsize = ls_xpool_roundup(size + sizeof(ls_xpool_header_t));
#if !defined( LS_VG_DEBUG )
    if (nsize > LS_XPOOL_MAXLGBLK_SIZE)
#endif
        return ls_xpool_bblkalloc(pool, size, nsize);

    if (nsize <= LS_XPOOL_FREELIST_MAXBYTES && (pool->pfreelists != NULL))
    {
        if ((pNew = xfreelistget(pool, nsize)) != NULL)
        {
#ifdef USE_VALGRIND
            vg_xpool_alloc(pool, pNew, size);
#endif
            return pNew->data;   /* do *not* change real size of block */
        }
    }
    pNew = xpool_blkget(pool, nsize);
#ifdef USE_VALGRIND
    vg_xpool_alloc(pool, pNew, size);
#endif
    return pNew->data;
}


void *ls_xpool_calloc(ls_xpool_t *pool, uint32_t items, uint32_t size)
{
    void *pPtr;
    int allocsz = items * size;
    if ((pPtr = ls_xpool_alloc(pool, allocsz)) != NULL)
        memset(pPtr, 0, allocsz);
    return pPtr;
}


void *ls_xpool_realloc(ls_xpool_t *pool, void *pOld, uint32_t new_sz)
{
    if (pOld == NULL)
        return ls_xpool_alloc(pool, new_sz);
    ls_xpool_header_t *pHeader = (ls_xpool_header_t *)((char *)pOld - sizeof(
                                     ls_xpool_header_t));
#ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED(pHeader, sizeof(*pHeader));
    VALGRIND_MAKE_MEM_DEFINED(pHeader, pHeader->size);
#endif
    uint32_t old_sz = pHeader->size - sizeof(ls_xpool_header_t);
#ifdef LS_XPOOL_INTERNAL_DEBUG
    assert(pHeader->magic == LS_XPOOL_MAGIC)
#endif
    if (new_sz <= old_sz)
    {
#ifdef USE_VALGRIND
        VALGRIND_MEMPOOL_FREE(pool, (xpool_alink_t *)pHeader);
        vg_xpool_alloc(pool, (xpool_alink_t *)pHeader, new_sz);
        VALGRIND_MAKE_MEM_DEFINED(pOld, new_sz);
#endif
        return pOld;
    }
    void *pNew;
    if ((pNew = ls_xpool_alloc(pool, new_sz)) != NULL)
    {
        memmove(pNew, pOld, old_sz);
        ls_xpool_free(pool, pOld);
    }
    return pNew;
}


static xpool_alink_t *xpool_blkget(ls_xpool_t *pool, uint32_t size)
{
    int remain;
    xpool_alink_t *pPtr;

    if ((size <= LS_XPOOL_MAXSMBLK_SIZE)
#ifndef USE_THRSAFE_POOL
        && (pool->smblk.ptr != NULL)    /* optimization in non-thread mode */
#endif
        && ((pPtr = xpool_getablk(pool, &pool->smblk, size)) != NULL))
        return pPtr;
    if (1
#ifndef USE_THRSAFE_POOL
        && (pool->lgblk.ptr != NULL)    /* optimization in non-thread mode */
#endif
        && ((pPtr = xpool_getablk(pool, &pool->lgblk, size)) != NULL))
        return pPtr;

    pPtr = ls_xpool_getsuperblk(pool);
    pPtr->header.size -= size;
    remain = (int)pPtr->header.size;
    if (remain < (int)(((LS_XPOOL_NUMFREELISTS + 1)
                        *LS_XPOOL_FREELISTINTERVAL)))
    {
        /* return the full residual (or could have added to freelist)
         */
        size += remain;
        remain = 0;
    }
    else
    {
        xpool_blkput(((remain <= (int)(LS_XPOOL_MAXSMBLK_SIZE)) ?
                      &pool->smblk : &pool->lgblk), pPtr);
    }

    return xpool_blkcarve((xpool_alink_t *)((char *)pPtr + remain), size);
}


static xpool_alink_t *xpool_getablk(
    ls_xpool_t *pool, xpool_qlist_t *pHead, uint32_t size)
{
    int remain;
    int trys = xpool_maxgetablk_trys;
#ifdef USE_THRSAFE_POOL
    ls_spinlock_lock(&pHead->lock);
    if (pHead->ptr == NULL)
    {
        ls_spinlock_unlock(&pHead->lock);
        return NULL;
    }
#endif
    xpool_alink_t *pPtr = pHead->ptr;
    xpool_alink_t *pPrev = NULL;
#ifdef LS_XPOOL_INTERNAL_DEBUG
    assert(pPtr != NULL);       /* guaranteed by caller */
    assert(trys > 0);
#endif
    do
    {
#ifdef USE_VALGRIND
        VALGRIND_MAKE_MEM_DEFINED(pPtr, sizeof(*pPtr));
#endif
        if ((remain = (int)pPtr->header.size - (int)size) >= 0)
        {
            pPtr->header.size = remain;
            if (remain <
                (int)(((LS_XPOOL_NUMFREELISTS + 1)*LS_XPOOL_FREELISTINTERVAL)))
            {
                /* return the full residual (or could have added to freelist)
                 */
                size += remain;
                remain = 0;
                xpool_blkremove(pHead, pPrev, pPtr);
            }
            /* check if a large block becomes a small block
             */
            else if ((pHead == &pool->lgblk)
                     && (remain <= (int)(LS_XPOOL_MAXSMBLK_SIZE)))
            {
                xpool_blkremove(pHead, pPrev, pPtr);
                xpool_blkput(&pool->smblk, pPtr);
            }
#ifdef USE_THRSAFE_POOL
            ls_spinlock_unlock(&pHead->lock);
#endif
            return xpool_blkcarve(
                       (xpool_alink_t *)((char *)pPtr + remain), size);
        }
        /* if this block is `never' getting chosen, remove it
         */
        if (++pPtr->header.magic >= (uint32_t)xpool_maxblk_trys)
        {
            xpool_alink_t *pNext = pPtr->next;
            xpool_blkremove(pHead, pPrev, pPtr);
            if (pHead == &pool->lgblk)
                xpool_blkput(&pool->smblk, pPtr);
#ifdef USE_VALGRIND
            VALGRIND_MAKE_MEM_NOACCESS(pPtr, sizeof(*pPtr));
#endif
            pPtr = pNext;
        }
        else
        {
            pPrev = pPtr;
            pPtr = pPtr->next;
#ifdef USE_VALGRIND
            VALGRIND_MAKE_MEM_NOACCESS(pPrev, sizeof(*pPrev));
#endif
        }
    }
    while ((pPtr != NULL) && (--trys >= 0));

#ifdef USE_THRSAFE_POOL
    ls_spinlock_unlock(&pHead->lock);
#endif
    return NULL;
}


/* NOTE: when allocating an xpool superblock,
 * we overwrite the gpool header size/magic with the xpool superblock link,
 * (which is what ls_pfree does),
 * permitting us to later free the list as a whole.
 */
static xpool_alink_t *ls_xpool_getsuperblk(ls_xpool_t *pool)
{
    xpool_alink_t *pNew = (xpool_alink_t *)ls_palloc(LS_XPOOL_SBLK_SIZE);
    ls_pool_blk_t *pBlk = ((ls_pool_blk_t *)pNew) -
                          1;  /* ptr to real gpool block */
#ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_NOACCESS(pNew->data,
                               pBlk->header.size - sizeof(ls_pool_blk_t) - sizeof(ls_xpool_header_t));
#endif
    ls_pool_insptr(&pool->psuperblk,
                   pBlk);    /* overwrites size/magic with link */
    pNew->header.size = LS_XPOOL_MAXLGBLK_SIZE;
    pNew->header.magic = LS_XPOOL_MAGIC;
    return pNew;
}


void *ls_xpool_bblkalloc(ls_xpool_t *pool, uint32_t size, uint32_t nsize)
{
    ls_xpool_bblk_t *pNew;
    pNew = (ls_xpool_bblk_t *)ls_palloc(size + sizeof(ls_xpool_bblk_t));
    pNew->header.size = nsize;
    pNew->header.magic = LS_XPOOL_MAGIC;
    pNew->prev = NULL;
#ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_NOACCESS(&pNew->header, sizeof(pNew->header));
#endif
#ifdef USE_THRSAFE_POOL
    ls_spinlock_lock(&pool->lock);
#endif
    pNew->next = pool->pbigblk;
    if (pool->pbigblk != NULL)
        pool->pbigblk->prev = pNew;
    pool->pbigblk = pNew;
#ifdef USE_THRSAFE_POOL
    ls_spinlock_unlock(&pool->lock);
#endif
    return (void *)pNew->data;
}


static void ls_xpool_unlink(ls_xpool_t *pool, ls_xpool_bblk_t *pFree)
{
#ifdef USE_THRSAFE_POOL
    ls_spinlock_lock(&pool->lock);
#endif
    ls_xpool_bblk_t *pNext = pFree->next;
    ls_xpool_bblk_t *pPrev = pFree->prev;

    if (pNext != NULL)
        pNext->prev = pPrev;
    if (pPrev != NULL)
        pPrev->next = pNext;
    else
        pool->pbigblk = pNext;
#ifdef USE_THRSAFE_POOL
    ls_spinlock_unlock(&pool->lock);
#endif
}


void ls_xpool_free(ls_xpool_t *pool, void *data)
{
    if (data == NULL)
        return;
#if !defined( LS_VG_DEBUG )
    ls_xpool_header_t *pHeader = (ls_xpool_header_t *)
                                 ((char *)data - sizeof(ls_xpool_header_t));     /* header size */
#ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED(pHeader, sizeof(*pHeader));
#endif
#ifdef LS_XPOOL_INTERNAL_DEBUG
    assert(pHeader->magic == LS_XPOOL_MAGIC)
#endif
    uint32_t size = pHeader->size;
    if (size > LS_XPOOL_MAXLGBLK_SIZE)
    {
#endif
        ls_xpool_bblk_t *pFree = ((ls_xpool_bblk_t *)data) - 1;
        ls_xpool_unlink(pool, pFree);
        ls_xpool_bblkfree(pFree);
        return;
#if !defined( LS_VG_DEBUG )
    }
    if (pool->flag & LS_XPOOL_NOFREE)
        return;
    if (size <= LS_XPOOL_FREELIST_MAXBYTES)
    {
        if (ls_xpool_chkinit(pool, &pool->pfreelists, &pool->init) == LS_OK)
            xfreelistput(pool, (xpool_alink_t *)pHeader, size);
    }
    else
    {
        xpool_blkput(((size <= LS_XPOOL_MAXSMBLK_SIZE) ?
                      &pool->smblk : &pool->lgblk), (xpool_alink_t *)pHeader);
    }
#ifdef USE_VALGRIND
    VALGRIND_MEMPOOL_FREE(pool, pHeader);
    VALGRIND_MAKE_MEM_DEFINED(data, sizeof(void *));
#endif
#endif
}


void ls_xpool_skipfree(ls_xpool_t *pool)
{
    pool->flag |= LS_XPOOL_NOFREE;
}


ls_xpool_t *ls_xpool_new()
{
    ls_xpool_t *pool = ls_palloc(sizeof(ls_xpool_t));
    if (pool != NULL)
        ls_xpool_init(pool);
    return pool;
}


void ls_xpool_delete(ls_xpool_t *pool)
{
    ls_xpool_destroy(pool);
    ls_pfree(pool);
}

