/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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

#include <lsr/lsr_pool.h>
#include <lsr/lsr_internal.h>
#include <lsr/lsr_xpool.h>
#ifdef _REENTRANT
#include <lsr/lsr_lock.h>
#endif
#ifndef NVALGRIND
#include <valgrind/memcheck.h>
#endif

#define LSR_XPOOL_SB_SIZE (4*1024)
#define LSR_XPOOL_DATA_ALIGN    8
#define LSR_XPOOL_FREELISTINTERVAL 16
#define LSR_XPOOL_FREELIST_MAXBYTES 256
#define LSR_XPOOL_NUMFREELISTS 16

#define LSR_XPOOL_NOFREE    1

#define LSR_XPOOL_MAGIC     (0x58704f6c) //"XpOl"

struct xpool_alloclink_s
{
    struct lsr_xpool_header_s  m_header;
    union
    {
        struct xpool_alloclink_s *m_pNext;
        char m_cData[1];
    };
};

static int xpool_maxgetablk_trys = 3;   /* how many attempts to find a suitable block */
static int xpool_maxblk_trys = 3;       /* how many attempts until moving block */

#define LSR_XPOOL_MAXLGBLK_SIZE (LSR_XPOOL_SB_SIZE)
#define LSR_XPOOL_MAXSMBLK_SIZE 1024

#ifndef NVALGRIND
lsr_inline void vg_xpool_alloc(
  lsr_xpool_t *pool, xpool_alloclink_t *pNew, uint32_t size )
{
    VALGRIND_MEMPOOL_ALLOC( pool, pNew, size + sizeof( lsr_xpool_header_t ) );
    VALGRIND_MAKE_MEM_NOACCESS( &pNew->m_header, sizeof(pNew->m_header) );
}
#endif

/* Get a block */
static xpool_alloclink_t *xpool_blkget( lsr_xpool_t *pool, uint32_t size );
static xpool_alloclink_t *xpool_getablk(
  lsr_xpool_t *pool, xpool_qlist_t *pHead, uint32_t size );
/* Carve a piece from an existing block */
lsr_inline xpool_alloclink_t *xpool_blkcarve( xpool_alloclink_t *pNew, uint32_t size )
{
#ifndef NVALGRIND
    VALGRIND_MAKE_MEM_DEFINED( &pNew->m_header, sizeof(pNew->m_header) );
#endif
    pNew->m_header.m_size = size;
    pNew->m_header.m_magic = LSR_XPOOL_MAGIC;
    return pNew;
}

/* Get a new superblock from Global Pool/OS */
static xpool_alloclink_t *lsr_xpool_get_sb( lsr_xpool_t *pool );

/* Get a big allocation for any requests for allocations bigger than the superblock */
static void *lsr_xpool_big_alloc( lsr_xpool_t *pool, uint32_t size, uint32_t nsize );

enum {_XPOOL_ALIGN = 16 };
lsr_inline size_t lsr_xpool_round_up( size_t bytes )
    { return (((bytes) + (size_t) _XPOOL_ALIGN -1) & ~((size_t) _XPOOL_ALIGN - 1)); }

/* Release big block to OS */
lsr_inline void lsr_xpool_release_bblk( lsr_xpool_bblk_t *pBblk )
{
    lsr_pfree( (void *)pBblk );
}

/* Manage freelist pointers */
lsr_inline int xpool_freelistindx( size_t size )
{   return ( ( size >> 4 ) - 1 );   }

lsr_inline void xpool_freelistput( lsr_xpool_t *pool, xpool_alloclink_t *pFree, size_t size )
{
#ifdef _REENTRANT
    lsr_spinlock_lock( &pool->m_initlock );
#endif    
    if ( !pool->m_pFreeLists )
    {
        pool->m_pFreeLists = lsr_xpool_alloc( pool, sizeof(*(pool->m_pFreeLists)) * LSR_XPOOL_NUMFREELISTS );
        if ( !pool->m_pFreeLists )
        {
#ifdef _REENTRANT
            lsr_spinlock_unlock( &pool->m_initlock );
#endif    
            return;
        }
        memset( pool->m_pFreeLists, 0, sizeof(*(pool->m_pFreeLists)) * LSR_XPOOL_NUMFREELISTS );
    }
#ifdef _REENTRANT
    lsr_spinlock_unlock( &pool->m_initlock );
#endif    
    xpool_qlist_t *pQlst = &pool->m_pFreeLists[ xpool_freelistindx( size ) ];
#ifndef NVALGRIND
    VALGRIND_MAKE_MEM_DEFINED( &pFree->m_pNext, sizeof(pFree->m_pNext) );
#endif
#ifdef _REENTRANT
    lsr_spinlock_lock( &pQlst->m_lock );
#endif    
    pFree->m_pNext = pQlst->m_pPtr;
    pQlst->m_pPtr = pFree;
#ifndef NVALGRIND
    VALGRIND_MAKE_MEM_NOACCESS( &pFree->m_pNext, sizeof(pFree->m_pNext) );
#endif
#ifdef _REENTRANT
    lsr_spinlock_unlock( &pQlst->m_lock );
#endif    
}

lsr_inline void *xpool_freelistget( lsr_xpool_t *pool, size_t size )
{
    xpool_qlist_t *pQlst = &pool->m_pFreeLists[ xpool_freelistindx( size ) ];
#ifdef _REENTRANT
    lsr_spinlock_lock( &pQlst->m_lock );
#endif    
    xpool_alloclink_t *pFree = pQlst->m_pPtr;
    if ( pFree )
    {
#ifndef NVALGRIND
        VALGRIND_MAKE_MEM_DEFINED( &pFree->m_pNext, sizeof(pFree->m_pNext) );
#endif
        pQlst->m_pPtr = pFree->m_pNext;
    }
#ifdef _REENTRANT
    lsr_spinlock_unlock( &pQlst->m_lock );
#endif    
    return (void *)pFree;
}

/* Manage block lists */
lsr_inline void xpool_blkput( xpool_qlist_t *head, xpool_alloclink_t *pFree )
{
    pFree->m_header.m_magic = 0; /* reset - use as failure counter */
#ifndef NVALGRIND
    VALGRIND_MAKE_MEM_DEFINED( &pFree->m_pNext, sizeof(pFree->m_pNext) );
#endif
#ifdef _REENTRANT
    lsr_spinlock_lock( &head->m_lock );
#endif    
    pFree->m_pNext = head->m_pPtr;
    head->m_pPtr = pFree;
#ifndef NVALGRIND
    VALGRIND_MAKE_MEM_NOACCESS( &pFree->m_pNext, sizeof(pFree->m_pNext) );
#endif
#ifdef _REENTRANT
    lsr_spinlock_unlock( &head->m_lock );
#endif    
}

lsr_inline void xpool_blkremove(
  xpool_qlist_t *pHead, xpool_alloclink_t *pPrev, xpool_alloclink_t *pPtr )
{
    if ( pPrev == NULL )
        pHead->m_pPtr = pPtr->m_pNext;
    else
    {
#ifndef NVALGRIND
        VALGRIND_MAKE_MEM_DEFINED( &pPrev->m_pNext, sizeof(pPrev->m_pNext) );
#endif
        pPrev->m_pNext = pPtr->m_pNext;
#ifndef NVALGRIND
        VALGRIND_MAKE_MEM_NOACCESS( &pPrev->m_pNext, sizeof(pPrev->m_pNext) );
#endif
    }

}

void lsr_xpool_init( lsr_xpool_t *pool )
{
    memset( pool, 0, sizeof( lsr_xpool_t) );
#ifdef _REENTRANT
#if ( lock_Avail != 0 )
    NEED TO SETUP LOCKS
#endif    
#endif
#ifndef NVALGRIND
    VALGRIND_CREATE_MEMPOOL( pool, 0, 0 );
#endif
}

void lsr_xpool_destroy( lsr_xpool_t *pool )
{
    if ( pool->m_pFreeLists )
        lsr_xpool_free( pool, pool->m_pFreeLists );

    lsr_plistfree( pool->m_pSB, LSR_XPOOL_SB_SIZE );
    
    lsr_psavepending( pool->m_pBblk );
    lsr_pfreepending();
    
    memset( pool, 0, sizeof( lsr_xpool_t) );
#ifndef NVALGRIND
    VALGRIND_DESTROY_MEMPOOL( pool );
#endif
}

void lsr_xpool_reset( lsr_xpool_t *pool )
{
    lsr_xpool_destroy( pool );
    lsr_xpool_init( pool );
}

int lsr_xpool_is_empty( lsr_xpool_t *pool )
{
    assert( pool );
    return ( (pool->m_pSB == NULL) && (pool->m_pBblk == NULL) );
}

void *lsr_xpool_alloc( lsr_xpool_t *pool, uint32_t size )
{
    uint32_t nsize;
    xpool_alloclink_t *pNew;
    nsize = lsr_xpool_round_up( size + sizeof( lsr_xpool_header_t ) );
#if !defined( LSR_VG_DEBUG )
    if ( nsize > LSR_XPOOL_MAXLGBLK_SIZE )
#endif
        return lsr_xpool_big_alloc( pool, size, nsize );

    if ( nsize <= LSR_XPOOL_FREELIST_MAXBYTES && pool->m_pFreeLists )
    {
        if ( (pNew = xpool_freelistget( pool, nsize )) != NULL )
        {
#ifndef NVALGRIND
            vg_xpool_alloc( pool, pNew, size );
#endif
            return pNew->m_cData;   /* do *not* change real size of block */
        }
    }
    pNew = xpool_blkget( pool, nsize );
#ifndef NVALGRIND
    vg_xpool_alloc( pool, pNew, size );
#endif
    return pNew->m_cData;
}

void *lsr_xpool_calloc( lsr_xpool_t *pool, uint32_t items, uint32_t size )
{
    void *pPtr;
    int allocsz = items * size;
    if ( (pPtr = lsr_xpool_alloc( pool, allocsz )) != NULL )
        memset( pPtr, 0, allocsz );
    return pPtr;
}

void *lsr_xpool_realloc( lsr_xpool_t *pool, void *pOld, uint32_t new_sz )
{
    if ( !pOld )
        return lsr_xpool_alloc( pool, new_sz );
    lsr_xpool_header_t *pHeader = (lsr_xpool_header_t *)((char*)pOld - sizeof( lsr_xpool_header_t ));
#ifndef NVALGRIND
    VALGRIND_MAKE_MEM_DEFINED( pHeader, sizeof(*pHeader) );
    VALGRIND_MAKE_MEM_DEFINED( pHeader, pHeader->m_size );
#endif
    uint32_t old_sz = pHeader->m_size - sizeof( lsr_xpool_header_t );
#ifdef LSR_XPOOL_INTERNAL_DEBUG
    assert ( pHeader->m_magic == LSR_XPOOL_MAGIC )
#endif
    if ( new_sz <= old_sz  )
    {
#ifndef NVALGRIND
        VALGRIND_MEMPOOL_FREE( pool, (xpool_alloclink_t *)pHeader );
        vg_xpool_alloc( pool, (xpool_alloclink_t *)pHeader, new_sz );
#endif
        return pOld;
    }
    void *pNew;
    if ( (pNew = lsr_xpool_alloc( pool, new_sz )) != NULL )
    {
        memmove( pNew, pOld, old_sz );
        lsr_xpool_free( pool, pOld );
    }
    return pNew;
}

static xpool_alloclink_t *xpool_blkget( lsr_xpool_t *pool, uint32_t size )
{
    int remain;
    xpool_alloclink_t *pPtr;
    
    if ( ( size <= LSR_XPOOL_MAXSMBLK_SIZE )
#ifndef _REENTRANT
      && ( pool->m_pSmBlk.m_pPtr != NULL )  /* optimization in non-thread mode */
#endif
      && ( ( pPtr = xpool_getablk( pool, &pool->m_pSmBlk, size ) ) != NULL ) )
        return pPtr;
    if ( 1
#ifndef _REENTRANT
      && ( pool->m_pLgBlk.m_pPtr != NULL )  /* optimization in non-thread mode */
#endif
      && ( ( pPtr = xpool_getablk( pool, &pool->m_pLgBlk, size ) ) != NULL ) )
        return pPtr;
    
    pPtr = lsr_xpool_get_sb( pool );
    pPtr->m_header.m_size -= size;
    remain = (int)pPtr->m_header.m_size;
    if ( remain < (int)(((LSR_XPOOL_NUMFREELISTS+1)*LSR_XPOOL_FREELISTINTERVAL)) )
    {
        /* return the full residual (or could have added to freelist)
         */
        size += remain;
        remain = 0;
    }
    else if ( remain <= (int)(LSR_XPOOL_MAXSMBLK_SIZE) )
        xpool_blkput( &pool->m_pSmBlk, pPtr );
    else 
        xpool_blkput( &pool->m_pLgBlk, pPtr );

    return xpool_blkcarve( (xpool_alloclink_t *)((char *)pPtr + remain), size );
}

static xpool_alloclink_t *xpool_getablk(
  lsr_xpool_t *pool, xpool_qlist_t *pHead, uint32_t size )
{
    int remain;
    int trys = xpool_maxgetablk_trys;
#ifdef _REENTRANT
    lsr_spinlock_lock( &pHead->m_lock );
    if ( pHead->m_pPtr == NULL )
    {
        lsr_spinlock_unlock( &pHead->m_lock );
        return NULL;
    }
#endif    
    xpool_alloclink_t *pPtr = pHead->m_pPtr;
    xpool_alloclink_t *pPrev = NULL;
#ifdef LSR_XPOOL_INTERNAL_DEBUG
    assert ( pPtr != NULL );    /* guaranteed by caller */
    assert ( trys > 0 );
#endif
    do
    {
#ifndef NVALGRIND
        VALGRIND_MAKE_MEM_DEFINED( pPtr, sizeof(*pPtr) );
#endif
        if ( ( remain = (int)pPtr->m_header.m_size - (int)size ) >= 0 )
        {
            pPtr->m_header.m_size = remain;
            if ( remain <
              (int)(((LSR_XPOOL_NUMFREELISTS+1)*LSR_XPOOL_FREELISTINTERVAL)) )
            {
                /* return the full residual (or could have added to freelist)
                 */
                size += remain;
                remain = 0;
                xpool_blkremove( pHead, pPrev, pPtr );
            }
            /* check if a large block becomes a small block
             */
            else if ( ( pHead == &pool->m_pLgBlk )
              && ( remain <= (int)(LSR_XPOOL_MAXSMBLK_SIZE) ) )
            {
                xpool_blkremove( pHead, pPrev, pPtr );
                xpool_blkput( &pool->m_pSmBlk, pPtr );
            }
#ifdef _REENTRANT
            lsr_spinlock_unlock( &pHead->m_lock );
#endif    
            return xpool_blkcarve(
              (xpool_alloclink_t *)((char *)pPtr + remain), size );
        }
        /* if this block is `never' getting chosen, remove it
         */
        if ( ++pPtr->m_header.m_magic >= (uint32_t)xpool_maxblk_trys )
        {
            xpool_alloclink_t *pNext = pPtr->m_pNext;
            xpool_blkremove( pHead, pPrev, pPtr );
            if ( pHead == &pool->m_pLgBlk )
                xpool_blkput( &pool->m_pSmBlk, pPtr );
#ifndef NVALGRIND
            VALGRIND_MAKE_MEM_NOACCESS( pPtr, sizeof(*pPtr) );
#endif
            pPtr = pNext;
        }
        else
        {
            pPrev = pPtr;
            pPtr = pPtr->m_pNext;
#ifndef NVALGRIND
            VALGRIND_MAKE_MEM_NOACCESS( pPrev, sizeof(*pPrev) );
#endif
        }
    }
    while ( pPtr && ( --trys >= 0 ) );
    
#ifdef _REENTRANT
    lsr_spinlock_unlock( &pHead->m_lock );
#endif    
    return NULL;
}

/* NOTE: when allocating an xpool superblock,
 * we overwrite the gpool header size/magic with the xpool superblock link,
 * (which is what lsr_pfree does),
 * permitting us to later free the list as a whole.
 */
static xpool_alloclink_t *lsr_xpool_get_sb( lsr_xpool_t *pool )
{
#ifdef LSR_XPOOL_INTERNAL_DEBUG
    printf( "LSR_XPOOL_DEBUG: Allocating a superblock\n" );
#endif
    xpool_alloclink_t *pNew = (xpool_alloclink_t *)lsr_palloc( LSR_XPOOL_SB_SIZE );
    lsr_pool_blk_t *pBlk = (( lsr_pool_blk_t *)pNew) - 1; /* ptr to real gpool block */
#ifndef NVALGRIND
    VALGRIND_MAKE_MEM_NOACCESS( pNew->m_cData,
      pBlk->m_header.m_size - sizeof(lsr_pool_blk_t) - sizeof(lsr_xpool_header_t) );
#endif
#ifdef _REENTRANT
    lsr_spinlock_lock( &pool->m_lock );
#endif    
    pBlk->m_pNext = pool->m_pSB;    /* overwriting size and magic with link */
    pool->m_pSB = pBlk;
#ifdef _REENTRANT
    lsr_spinlock_unlock( &pool->m_lock );
#endif    
    pNew->m_header.m_size = LSR_XPOOL_MAXLGBLK_SIZE; 
    pNew->m_header.m_magic = LSR_XPOOL_MAGIC;
    return pNew;
}


void *lsr_xpool_big_alloc( lsr_xpool_t *pool, uint32_t size, uint32_t nsize )
{
    lsr_xpool_bblk_t *pNew;
#ifdef LSR_XPOOL_INTERNAL_DEBUG
    printf( "LSR_XPOOL_DEBUG: Getting a big block\n" );
#endif
    pNew = (lsr_xpool_bblk_t *)lsr_palloc( size + sizeof(lsr_xpool_bblk_t) );
    pNew->m_header.m_size = nsize;
    pNew->m_header.m_magic = LSR_XPOOL_MAGIC;
    pNew->m_pPrev = NULL;
#ifndef NVALGRIND
    VALGRIND_MAKE_MEM_NOACCESS( &pNew->m_header, sizeof(pNew->m_header) );
#endif
#ifdef _REENTRANT
    lsr_spinlock_lock( &pool->m_lock );
#endif    
    pNew->m_pNext = pool->m_pBblk;
    if ( pool->m_pBblk )
        pool->m_pBblk->m_pPrev = pNew;
    pool->m_pBblk = pNew;
#ifdef _REENTRANT
    lsr_spinlock_unlock( &pool->m_lock );
#endif    
    return (void *)pNew->m_cData;
}

static void lsr_xpool_unlink( lsr_xpool_t *pool, lsr_xpool_bblk_t *pFree )
{
#ifdef _REENTRANT
    lsr_spinlock_lock( &pool->m_lock );
#endif    
    lsr_xpool_bblk_t *pNext = pFree->m_pNext;
    lsr_xpool_bblk_t *pPrev = pFree->m_pPrev;
    
    if ( pNext )
        pNext->m_pPrev = pPrev;
    if ( pPrev )
        pPrev->m_pNext = pNext;
    else
        pool->m_pBblk = pNext;
#ifdef _REENTRANT
    lsr_spinlock_unlock( &pool->m_lock );
#endif    
}


void lsr_xpool_free( lsr_xpool_t *pool, void *data )
{
    if ( data == NULL )
        return;
#if !defined( LSR_VG_DEBUG )
    lsr_xpool_header_t *pHeader = (lsr_xpool_header_t *)
        ((char *)data - sizeof( lsr_xpool_header_t ) );  /* header size */
#ifndef NVALGRIND
    VALGRIND_MAKE_MEM_DEFINED( pHeader, sizeof(*pHeader) );
#endif
#ifdef LSR_XPOOL_INTERNAL_DEBUG
    assert ( pHeader->m_magic == LSR_XPOOL_MAGIC )
#endif
    uint32_t size = pHeader->m_size;
    if ( size > LSR_XPOOL_MAXLGBLK_SIZE )
    {
#endif
        lsr_xpool_bblk_t *pFree = ((lsr_xpool_bblk_t *)data) - 1;
        lsr_xpool_unlink( pool, pFree );
        lsr_xpool_release_bblk( pFree );
        return;
#if !defined( LSR_VG_DEBUG )
    }
    if ( pool->m_flag & LSR_XPOOL_NOFREE )
        return;
    if ( size <= LSR_XPOOL_FREELIST_MAXBYTES )
    {
        xpool_freelistput( pool, (xpool_alloclink_t *)pHeader, size );
    }
    else
    {
        xpool_qlist_t * pBlkHead;
        if ( size > LSR_XPOOL_MAXSMBLK_SIZE )  /* put on large block list */
            pBlkHead = &pool->m_pLgBlk;
        else
            pBlkHead = &pool->m_pSmBlk;
        xpool_blkput( pBlkHead, (xpool_alloclink_t *)pHeader );
    }
#ifndef NVALGRIND
    VALGRIND_MEMPOOL_FREE( pool, pHeader );
#endif
#endif
}

void lsr_xpool_skip_free( lsr_xpool_t * pool)
{
    pool->m_flag |= LSR_XPOOL_NOFREE;
}

lsr_xpool_t * lsr_xpool_new()
{
    lsr_xpool_t * pool = lsr_palloc( sizeof( lsr_xpool_t ) );
    if ( pool )
        lsr_xpool_init( pool );
    return pool;
}

void lsr_xpool_delete( lsr_xpool_t * pool )
{
    lsr_xpool_destroy( pool );
    lsr_pfree( pool );
}

