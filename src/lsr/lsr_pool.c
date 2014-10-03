/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

/***************************************************************************
    $Id: pool.cpp,v 1.1.1.1 2013/12/22 23:42:51 gwang Exp $
                         -------------------
    begin                : Wed Apr 30 2003
    author               : Gang Wang
    email                : gwang@litespeedtech.com
 ***************************************************************************/

#include <lsr/lsr_pool.h>
#include <lsr/lsr_internal.h>
#ifdef _REENTRANT
#include <lsr/lsr_lock.h>
#endif
#ifndef NVALGRIND
#include <valgrind/memcheck.h>

struct malloclink
{
    struct malloclink *m_pNext;
    void *m_pMalloc;
};
static struct malloclink *pMalloclink = NULL;
lsr_inline void save_malloc_ptr( void *ptr )
{
    struct malloclink *p = (struct malloclink *)malloc( sizeof(*p) );
    p->m_pNext = pMalloclink;
    p->m_pMalloc = ptr;
    pMalloclink = p;
}
#else
lsr_inline void save_malloc_ptr( void *ptr )
{
    //do nothing
}

#endif /* NVALGRIND */

#define LSR_POOL_SMFREELISTBUCKETS     256
#define LSR_POOL_SMFREELISTINTERVAL     16
#define LSR_POOL_SMFREELIST_MAXBYTES  (LSR_POOL_SMFREELISTBUCKETS*LSR_POOL_SMFREELISTINTERVAL)
#define LSR_POOL_LGFREELISTBUCKETS      64
#define LSR_POOL_LGFREELISTINTERVAL   1024
#define LSR_POOL_LGFREELIST_MAXBYTES  (LSR_POOL_LGFREELISTBUCKETS*LSR_POOL_LGFREELISTINTERVAL)

#define LSR_POOL_MAGIC     (0x506f4f6c) //"PoOl"

enum { LSR_ALIGN = 16 };

lsr_inline size_t pool_roundup( size_t bytes )
{ return ( ((bytes) + (size_t)LSR_ALIGN-1) & ~((size_t)LSR_ALIGN-1) ); }

typedef struct _alloclink_s
{
    struct _alloclink_s   * m_pNext;
} _alloclink_t;

/* skew offset to handle requests expected to be in round k's
 */
#define LSR_POOL_LGFREELISTSKEW \
  ( ( sizeof(lsr_xpool_bblk_t) + sizeof(lsr_pool_blk_t) \
    + (size_t)LSR_ALIGN-1 ) & ~((size_t)LSR_ALIGN-1) )
    
#ifdef DEBUG_POOL
typedef struct lsr_pool_s
{
    lsr_xpool_bblk_t  * m_pPendingFree;     /* waiting to be freed */
} lsr_pool_t;

static lsr_pool_t lsr_pool_g =
{
    NULL
};

#else  /* DEBUG_POOL */
typedef struct _qlist_s
{
    lsr_pool_blk_t      * m_pHead;
    lsr_pool_blk_t      * m_pTail;
#ifdef _REENTRANT
    lsr_spinlock_t      m_pListLock;
#if ( lock_Avail != 0 )
    NEED TO SETUP LOCKS
#endif /* ( lock_Avail != 0 ) */
#endif /* _REENTRANT */
} _qlist_t;

typedef struct lsr_pool_s
{
    _qlist_t            m_pSmFreeLists[LSR_POOL_SMFREELISTBUCKETS];
    _qlist_t            m_pLgFreeLists[LSR_POOL_LGFREELISTBUCKETS];
    char              * _S_start_free;
    char              * _S_end_free;
    size_t              _S_heap_size;    
    lsr_xpool_bblk_t  * m_pPendingFree;     /* waiting to be freed */
#ifdef _REENTRANT
    lsr_spinlock_t      m_pPendingLock;
    lsr_spinlock_t      m_pAllocLock;
#endif /* _REENTRANT */   
} lsr_pool_t;

/* Global pool structure.
 */
static lsr_pool_t lsr_pool_g =
{
    { }, { },
    NULL, NULL, 0,
    NULL,
#ifdef _REENTRANT
    lock_Avail,
    lock_Avail
#endif /* _REENTRANT */
};

lsr_inline size_t lglistroundup( size_t bytes )
{ return ( (((bytes) - LSR_POOL_LGFREELISTSKEW + (size_t)LSR_POOL_LGFREELISTINTERVAL-1)
    & ~((size_t)LSR_POOL_LGFREELISTINTERVAL-1)) + LSR_POOL_LGFREELISTSKEW );
}

lsr_inline size_t smFreeListIndex( size_t bytes )
{ return ( ((bytes) + (size_t)LSR_POOL_SMFREELISTINTERVAL-1)
    /(size_t)LSR_POOL_SMFREELISTINTERVAL - 1 );
}

lsr_inline size_t lgFreeListIndex( size_t bytes )
{ return ( ((bytes) - LSR_POOL_LGFREELISTSKEW + (size_t)LSR_POOL_LGFREELISTINTERVAL-1)
    /(size_t)LSR_POOL_LGFREELISTINTERVAL - 1 );
}

lsr_inline _qlist_t * size2FreeListPtr( size_t num )
{
    return ( num > (size_t)LSR_POOL_SMFREELIST_MAXBYTES )?
      &lsr_pool_g.m_pLgFreeLists[lgFreeListIndex( num )]:
      &lsr_pool_g.m_pSmFreeLists[smFreeListIndex( num )];
}

lsr_inline void addtoFreeList(
  _qlist_t *pFreeList, lsr_pool_blk_t *pNew, lsr_pool_blk_t *pTail )
{
#ifdef _REENTRANT
    lsr_spinlock_lock( &pFreeList->m_pListLock );
#endif /* _REENTRANT */
    if ( pFreeList->m_pTail == NULL )
        pFreeList->m_pHead = pNew;
    else
    {
#ifndef NVALGRIND
        VALGRIND_MAKE_MEM_DEFINED( pFreeList->m_pTail, sizeof(lsr_pool_blk_t) );
#endif /* NVALGRIND */
        pFreeList->m_pTail->m_pNext = pNew;
#ifndef NVALGRIND
        VALGRIND_MAKE_MEM_NOACCESS( pFreeList->m_pTail, sizeof(lsr_pool_blk_t) );
#endif /* NVALGRIND */
    }
    pFreeList->m_pTail = pTail;
#ifdef _REENTRANT
    lsr_spinlock_unlock( &pFreeList->m_pListLock );
#endif /* _REENTRANT */
}

/* Allocates a chunk for nobjs of size size.  nobjs may be reduced
 * if it is inconvenient to allocate the requested number.
 */
static char * chunkAlloc( size_t size, int *pNobjs );

#ifndef _REENTRANT
/* Allocates a chunk from the existing freelist.
 */
static int freelistAlloc(
  size_t mysize, _qlist_t *pFreeList, size_t maxsize, size_t incr );
#endif /* _REENTRANT */

/* Gets an object of size num, and optionally adds to size num.
 * free list.
 */
static void * refill( size_t num );
#endif /* DEBUG_POOL */


char * lsr_pdupstr( const char *p )
{
    if ( p )
        return lsr_pdupstr2( p, strlen( p ) + 1 );
    return NULL;
}

char * lsr_pdupstr2( const char *p, int len )
{
    if ( p )
    {
        char * ps = (char *)lsr_palloc( len );
        if ( ps )
            memmove( ps, p, len );
        return ps;
    }
    return NULL;
}

void * lsr_palloc( size_t size )
{
#ifndef NVALGRIND
    static int initdone;
    if ( !initdone )
    {
        VALGRIND_CREATE_MEMPOOL( &lsr_pool_g, 0, 0 );
        initdone = 1;
    }
#endif /* NVALGRIND */
    size_t rndnum;
    void * ptr;
    size += sizeof(lsr_pool_blk_t);
    rndnum = pool_roundup( size );
#ifdef DEBUG_POOL
    ptr = malloc( rndnum );
#else    
    if ( rndnum > (size_t)LSR_POOL_LGFREELIST_MAXBYTES )
        ptr = malloc( rndnum );
    else
    {
        _qlist_t * pFreeList = size2FreeListPtr( rndnum );
#ifdef _REENTRANT
        int unlocked = 0;
        lsr_spinlock_lock( &pFreeList->m_pListLock );
#endif /* _REENTRANT */
        if ( (ptr = pFreeList->m_pHead) == NULL )
        {
            if ( rndnum > (size_t)LSR_POOL_SMFREELIST_MAXBYTES )
            {
#ifdef _REENTRANT
                lsr_spinlock_unlock( &pFreeList->m_pListLock );
                unlocked = 1;
#endif /* _REENTRANT */
                size_t memnum = lglistroundup( rndnum );
                ptr = malloc( memnum );
#ifndef NVALGRIND
                if ( ptr )
                {
                    VALGRIND_MAKE_MEM_NOACCESS( ptr, memnum );
                    save_malloc_ptr( ptr );
                }
#endif /* NVALGRIND */
            }
            else
            {
                ptr = refill( rndnum );
            }
        } 
        else
        {
#ifndef NVALGRIND
            VALGRIND_MAKE_MEM_DEFINED( ptr, sizeof(lsr_pool_blk_t) );
#endif /* NVALGRIND */
            if ( (pFreeList->m_pHead = ((lsr_pool_blk_t *)ptr)->m_pNext) == NULL )
                pFreeList->m_pTail = NULL;
        }
#ifdef _REENTRANT
        if ( !unlocked )
            lsr_spinlock_unlock( &pFreeList->m_pListLock );
#endif /* _REENTRANT */
    }
#endif /* DEBUG_POOL */
    if ( ptr )
    {
#ifndef NVALGRIND
        if ( rndnum <= (size_t)LSR_POOL_LGFREELIST_MAXBYTES )
            VALGRIND_MEMPOOL_ALLOC( &lsr_pool_g, ptr, size );
#endif /* NVALGRIND */
        ((lsr_pool_blk_t *)ptr)->m_header.m_size = rndnum;

        ((lsr_pool_blk_t *)ptr)->m_header.m_magic = LSR_POOL_MAGIC;
        return (void *)(((lsr_pool_blk_t *)ptr)+1);
    }
    return NULL;
}

void lsr_pfree( void *p )
{
    if ( p )
    {
        lsr_pool_blk_t *pBlk = ((lsr_pool_blk_t *)p) - 1;
#ifdef LSR_POOL_INTERNAL_DEBUG
        assert ( pBlk->m_header.m_magic == LSR_POOL_MAGIC );
#endif
#ifdef DEBUG_POOL
        free( (void *)pBlk );
#else /* DEBUG_POOL */
        if ( pBlk->m_header.m_size > (size_t)LSR_POOL_LGFREELIST_MAXBYTES )
            free( (void *)pBlk );
        else
        {
            _qlist_t * pFreeList = size2FreeListPtr( pBlk->m_header.m_size );
            pBlk->m_pNext = NULL;
#ifndef NVALGRIND
            VALGRIND_MEMPOOL_FREE( &lsr_pool_g, pBlk );
#endif /* NVALGRIND */
            addtoFreeList( pFreeList, pBlk, pBlk );
        }
#endif /* DEBUG_POOL */
    }
    return;
}

/* free a null terminated linked list.
 * all the elements are expected to have the same size, `size'.
 */
void lsr_plistfree( lsr_pool_blk_t *plist, size_t size )
{
    if ( plist == NULL )
        return;
    lsr_pool_blk_t * pNext;
    size = pool_roundup( size + sizeof(lsr_pool_blk_t) );
#ifdef LSR_POOL_INTERNAL_DEBUG
    assert ( plist->m_header.m_magic == LSR_POOL_MAGIC );
#endif
#ifndef DEBUG_POOL
    if ( size > (size_t)LSR_POOL_LGFREELIST_MAXBYTES )
    {
#endif /* DEBUG_POOL */
        do
        {
            pNext = plist->m_pNext;
            free( (void *)plist );
        }
        while ( ( plist = pNext ) != NULL );
        return;
#ifndef DEBUG_POOL
    }
    _qlist_t * pFreeList = size2FreeListPtr( size );
    pNext = plist;
    while ( pNext->m_pNext )        /* find last in list */
#ifndef NVALGRIND
    {
        lsr_pool_blk_t * pTmp = pNext;
#endif /* NVALGRIND */
        pNext = pNext->m_pNext;
#ifndef NVALGRIND
        VALGRIND_MEMPOOL_FREE( &lsr_pool_g, pTmp );
    }
    VALGRIND_MEMPOOL_FREE( &lsr_pool_g, pNext );
#endif /* NVALGRIND */
    addtoFreeList( pFreeList, plist, pNext );
 
    return;
#endif /* DEBUG_POOL */
}

/* save the pending xpool `bigblock' linked list to be freed later.
 * NOTE: we do *not* maintain the reverse link.
 */
void lsr_psavepending( lsr_xpool_bblk_t *plist )
{
    if ( plist == NULL )
        return;
    lsr_xpool_bblk_t *pPtr = plist;
    
    while ( pPtr->m_pNext )         /* find last in list */
        pPtr = pPtr->m_pNext;
#ifdef _REENTRANT
    lsr_spinlock_lock( &lsr_pool_g.m_pPendingLock );
#endif /* _REENTRANT */
    pPtr->m_pNext = lsr_pool_g.m_pPendingFree;
    lsr_pool_g.m_pPendingFree = plist;
#ifdef _REENTRANT
    lsr_spinlock_unlock( &lsr_pool_g.m_pPendingLock );
#endif /* _REENTRANT */

    return;
}

/* free the pending xpool `bigblock' linked list to where the pieces go.
 */
void lsr_pfreepending()
{
    lsr_xpool_bblk_t * pNext;
#ifdef _REENTRANT
    lsr_spinlock_lock( &lsr_pool_g.m_pPendingLock );
#endif /* _REENTRANT */
    lsr_xpool_bblk_t * pPtr = lsr_pool_g.m_pPendingFree;
    lsr_pool_g.m_pPendingFree = NULL;
#ifdef _REENTRANT
    lsr_spinlock_unlock( &lsr_pool_g.m_pPendingLock );
#endif /* _REENTRANT */
    
    while ( pPtr != NULL )
    {
        pNext = pPtr->m_pNext;
        lsr_pfree( pPtr );
        pPtr = pNext;
    }
 
    return;
}

void * lsr_prealloc( void *old_p, size_t new_sz )
{
    if ( old_p == NULL )
        return lsr_palloc( new_sz );

    lsr_pool_blk_t *pBlk = ((lsr_pool_blk_t *)old_p) - 1;
#ifdef LSR_POOL_INTERNAL_DEBUG
    assert ( pBlk->m_header.m_magic == LSR_POOL_MAGIC );
#endif
    size_t old_sz = pBlk->m_header.m_size;
    size_t val_sz = new_sz + sizeof(lsr_pool_blk_t);
    new_sz = pool_roundup( val_sz );
    if ( old_sz >= new_sz )
    {
#ifndef NVALGRIND
        VALGRIND_MAKE_MEM_DEFINED( old_p, val_sz );
#endif /* NVALGRIND */
        return old_p;
    }

    if ( (old_sz > (size_t)LSR_POOL_LGFREELIST_MAXBYTES)
      && (new_sz > (size_t)LSR_POOL_LGFREELIST_MAXBYTES) )
    {
        pBlk = (lsr_pool_blk_t *)realloc( (void *)pBlk, new_sz );
    }
    else
    {
        void * new_p;
        size_t copy_sz;
        if ( ( new_p = lsr_palloc( new_sz - sizeof(lsr_pool_blk_t) ) ) == NULL )
            pBlk = NULL;
        else
        {
            pBlk = ((lsr_pool_blk_t *)new_p) - 1;
            copy_sz = (new_sz > old_sz? old_sz : new_sz) - sizeof(lsr_pool_blk_t);
            if ( copy_sz )
            {
#ifndef NVALGRIND
                VALGRIND_MAKE_MEM_DEFINED( old_p, copy_sz );
#endif /* NVALGRIND */
                memmove( new_p, old_p, copy_sz );
            }
            if ( old_sz )
                lsr_pfree( old_p );
        }
    }
    if ( pBlk != NULL )
    {
        pBlk->m_header.m_size = new_sz;
        pBlk->m_header.m_magic = LSR_POOL_MAGIC;
        return (void *)(pBlk+1);
    }
    return NULL;
}

#ifndef DEBUG_POOL
static char * chunkAlloc( size_t size, int *pNobjs )
{
    char * result;
    
    size_t total_bytes = size * (*pNobjs);
    size_t bytes_left = lsr_pool_g._S_end_free - lsr_pool_g._S_start_free;
    
    if ( bytes_left >= total_bytes )
    {
        result = lsr_pool_g._S_start_free;
        lsr_pool_g._S_start_free += total_bytes;
        return result;
    }
    else if ( bytes_left >= size )
    {
        *pNobjs = (int)( bytes_left / size );
        total_bytes = size * (*pNobjs);
        result = lsr_pool_g._S_start_free;
        lsr_pool_g._S_start_free += total_bytes;
        return result;
    }
    else
    {
        size_t bytes_to_get = 2 * total_bytes
          + pool_roundup( lsr_pool_g._S_heap_size >> 4 );
        /* printf( "allocate %d bytes!\n", bytes_to_get ); */
        /* make use of the left-over piece. */
#ifndef _REENTRANT
        if ( bytes_left >= pool_roundup(sizeof(lsr_pool_blk_t)) )
        /* will not be > LSR_POOL_SMFREELIST_MAXBYTES */
        {
            _qlist_t * pFreeList = size2FreeListPtr( bytes_left );
            lsr_pool_blk_t *pBlk = (lsr_pool_blk_t *)lsr_pool_g._S_start_free;
            pBlk->m_pNext = NULL;
            addtoFreeList( pFreeList, pBlk, pBlk );
        }
#endif /* _REENTRANT */
        lsr_pool_g._S_start_free = (char *)malloc( bytes_to_get );
        if ( lsr_pool_g._S_start_free == NULL )
        {
#ifndef _REENTRANT
            size_t i;

            /* check small block freelist
             */
            i = size;
            if ( freelistAlloc( i,
              &lsr_pool_g.m_pSmFreeLists[smFreeListIndex( i )],
              (size_t)LSR_POOL_SMFREELIST_MAXBYTES,
              (size_t)LSR_POOL_SMFREELISTINTERVAL ) == 0 )
                return chunkAlloc( size, pNobjs );
            /* check large block freelist
             */
            i = LSR_POOL_SMFREELIST_MAXBYTES+LSR_POOL_LGFREELISTINTERVAL
              + LSR_POOL_LGFREELISTSKEW;
            if ( freelistAlloc( i,
              &lsr_pool_g.m_pLgFreeLists[lgFreeListIndex( i )],
              (size_t)LSR_POOL_LGFREELIST_MAXBYTES+LSR_POOL_LGFREELISTSKEW,
              (size_t)LSR_POOL_LGFREELISTINTERVAL ) == 0 )
                return chunkAlloc( size, pNobjs );
            lsr_pool_g._S_end_free = 0;    /* In case of exception. */
            lsr_pool_g._S_start_free = (char *)malloc( bytes_to_get );
#endif /* _REENTRANT */
            if ( lsr_pool_g._S_start_free == NULL )
            {
                *pNobjs = 0;
                return NULL;
            }
        }

        save_malloc_ptr (lsr_pool_g._S_start_free );
        
        lsr_pool_g._S_heap_size += bytes_to_get;
        lsr_pool_g._S_end_free = lsr_pool_g._S_start_free + bytes_to_get;
        return chunkAlloc( size, pNobjs );
    }
}

#ifndef _REENTRANT
static int freelistAlloc(
  size_t mysize, _qlist_t *pFreeList, size_t maxsize, size_t incr )
{
    lsr_pool_blk_t * p;
    while ( mysize <= maxsize )
    {
        if ( (p = pFreeList->m_pHead) != NULL )
        {
#ifndef NVALGRIND
            VALGRIND_MAKE_MEM_DEFINED( p, mysize);
#endif  /* NVALGRIND */
            if ( (pFreeList->m_pHead = p->m_pNext) == NULL )
                pFreeList->m_pTail = NULL;
            lsr_pool_g._S_start_free = (char *)p;
            lsr_pool_g._S_end_free = lsr_pool_g._S_start_free + mysize;
            return 0;
        }
        pFreeList++;
        mysize += incr;
    }
    return -1;
}
#endif /* _REENTRANT */

static void * refill( size_t num )
{
    int nobjs = 20;
#ifdef _REENTRANT
    lsr_spinlock_lock( &lsr_pool_g.m_pAllocLock );
#endif /* _REENTRANT */
    char * chunk = chunkAlloc( num, &nobjs );
#ifdef _REENTRANT
    lsr_spinlock_unlock( &lsr_pool_g.m_pAllocLock );
#endif /* _REENTRANT */
    _alloclink_t  * pPtr;
    char          * pNext;
    
    if ( nobjs <= 1 )
        return chunk;       /* freelist still empty; no head or tail */

    pNext = chunk + num;    /* skip past first block returned to caller */
    --nobjs;
    while ( 1 )
    {
        pPtr = (_alloclink_t *)pNext;
        if ( --nobjs <= 0 )
            break;
        pNext += num;
        pPtr->m_pNext = (_alloclink_t *)pNext;
    }
    pPtr->m_pNext = NULL;
 
    _qlist_t * pFreeList = &lsr_pool_g.m_pSmFreeLists[smFreeListIndex( num )];
    pFreeList->m_pHead = (lsr_pool_blk_t *)(chunk + num);
    pFreeList->m_pTail = (lsr_pool_blk_t *)pPtr;
#ifndef NVALGRIND
    VALGRIND_MAKE_MEM_NOACCESS(
      pFreeList->m_pHead, (char *)pPtr - (char *)pFreeList->m_pHead + num );
#endif /* NVALGRIND */
 
    return chunk;
}
#endif /* DEBUG_POOL */
