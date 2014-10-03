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
#define _GNU_SOURCE

#include <lsr/lsr_loopbuf.h>
#include <lsr/lsr_pool.h>
#include <lsr/lsr_xpool.h>

#include <errno.h>

static int lsr_loopbuf_iAppend( lsr_loopbuf_t *pThis, const char *pBuf, int size, lsr_xpool_t *pool );
static int lsr_loopbuf_iGuarantee( lsr_loopbuf_t *pThis, int size, lsr_xpool_t *pool );


static void lsr_loopbuf_deallocate( lsr_loopbuf_t *pThis, lsr_xpool_t *pool )
{
    if ( pThis->m_pBuf != NULL )
    {
        if ( pool )
            lsr_xpool_free( pool, pThis->m_pBuf );
        else
            lsr_pfree( pThis->m_pBuf );
    }
    memset( pThis, 0, sizeof( lsr_loopbuf_t ) );
}

static int lsr_loopbuf_allocate( lsr_loopbuf_t *pThis, lsr_xpool_t *pool, int size )
{
    if ((size == 0) && (pThis->m_iCapacity != size))
    {
        lsr_loopbuf_deallocate( pThis, pool );
    }
    else
    {
        size = (size + LSR_LOOPBUFUNIT) / LSR_LOOPBUFUNIT * LSR_LOOPBUFUNIT;
        if ( size > pThis->m_iCapacity )
        {
            char *pBuf;
            if ( pool )
                pBuf = (char *)lsr_xpool_alloc( pool, size );
            else
                pBuf = (char *)lsr_palloc( size );
            if ( pBuf == NULL )
            {
                errno = ENOMEM;
                return -1;
            }
            else
            {
                int len = 0;
                if ( pThis->m_pBuf != NULL )
                {
                    len = lsr_loopbuf_move_to( pThis, pBuf, size-1 );
                    if ( pool )
                        lsr_xpool_free( pool, pThis->m_pBuf );
                    else
                        lsr_pfree( pThis->m_pBuf );
                }
                pThis->m_pBuf = pThis->m_pHead = pBuf ;
                pThis->m_pEnd = pThis->m_pBuf + len;
                pThis->m_iCapacity = size;
                pThis->m_pBufEnd = pThis->m_pBuf + size;
            }
        }
    }
    return 0;
}

lsr_loopbuf_t *lsr_loopbuf_new( int size )
{
    lsr_loopbuf_t *pThis = (lsr_loopbuf_t *)lsr_palloc( sizeof( lsr_loopbuf_t ) );
    if ( !pThis )
        return NULL;
    if ( !lsr_loopbuf( pThis, size ) )
    {
        lsr_pfree( pThis );
        return NULL;
    }
    return pThis;
}

lsr_loopbuf_t *lsr_loopbuf( lsr_loopbuf_t *pThis, int size )
{
    if ( size == 0 )
        size = LSR_LOOPBUFUNIT;
    memset( pThis, 0, sizeof( lsr_loopbuf_t ) );
    lsr_loopbuf_reserve( pThis, size );
    return pThis;
}

void lsr_loopbuf_delete( lsr_loopbuf_t *pThis )
{
    lsr_loopbuf_deallocate( pThis, NULL );
    lsr_pfree( pThis );
}

int lsr_loopbuf_available( const lsr_loopbuf_t *pThis )
{
    register int ret = pThis->m_pHead - pThis->m_pEnd - 1;
    if ( ret >= 0 )
        return ret;
    return ret + pThis->m_iCapacity;
}

int lsr_loopbuf_contiguous( const lsr_loopbuf_t *pThis )
{
    if ( pThis->m_pHead > pThis->m_pEnd )
        return ( pThis->m_pHead - pThis->m_pEnd - 1 );
    else
        return ( pThis->m_pHead == pThis->m_pBuf )?
        ( pThis->m_pBufEnd - pThis->m_pEnd - 1 ) : pThis->m_pBufEnd - pThis->m_pEnd;
}

void lsr_loopbuf_used( lsr_loopbuf_t *pThis, int size )
{
    register int avail = lsr_loopbuf_available( pThis );
    if ( size > avail )
        size = avail;

    pThis->m_pEnd += size;
    if ( pThis->m_pEnd >= pThis->m_pBufEnd )
        pThis->m_pEnd -= pThis->m_iCapacity ;
}

int lsr_loopbuf_move_to( lsr_loopbuf_t *pThis, char *pBuf, int size )
{
    if ((pBuf == NULL) || (size < 0))
    {
        errno = EINVAL;
        return -1;
    }
    int len = lsr_loopbuf_size( pThis );
    if ( len > size )
        len = size ;
    if ( len > 0 )
    {
        size = pThis->m_pBufEnd - pThis->m_pHead ;
        if ( size > len )
            size = len ;
        memmove( pBuf, pThis->m_pHead, size );
        pBuf += size;
        size = len - size;
        if ( size )
        {
            memmove( pBuf, pThis->m_pBuf, size );
        }
        lsr_loopbuf_pop_front( pThis, len );
    }
    return len;
}

int lsr_loopbuf_pop_back( lsr_loopbuf_t *pThis, int size )
{
    if ( size < 0 )
    {
        errno = EINVAL;
        return -1;
    }
    if ( size )
    {
        int len = lsr_loopbuf_size( pThis );
        if ( size >= len )
        {
            size = len ;
            pThis->m_pHead = pThis->m_pEnd = pThis->m_pBuf;
        }
        else
        {
            pThis->m_pEnd -= size ;
            if ( pThis->m_pEnd < pThis->m_pBuf )
                pThis->m_pEnd += pThis->m_iCapacity ;
        }
    }
    return size;
}

int lsr_loopbuf_pop_front( lsr_loopbuf_t *pThis, int size )
{
    if ( size < 0 )
    {
        errno = EINVAL;
        return -1;
    }
    if ( size )
    {
        int len = lsr_loopbuf_size( pThis );
        if ( size >= len )
        {
            size = len ;
            pThis->m_pHead = pThis->m_pEnd = pThis->m_pBuf;
        }
        else
        {
            pThis->m_pHead += size;
            if ( pThis->m_pHead >= pThis->m_pBufEnd )
                pThis->m_pHead -= pThis->m_iCapacity;
        }
    }
    return size;
}

void lsr_loopbuf_swap( lsr_loopbuf_t *lhs, lsr_loopbuf_t *rhs )
{
    assert( rhs );
    char temp[sizeof( lsr_loopbuf_t ) ];
    memmove( temp, lhs, sizeof( lsr_loopbuf_t ) );
    memmove( lhs, rhs, sizeof( lsr_loopbuf_t ) );
    memmove( rhs, temp, sizeof( lsr_loopbuf_t ) );
}

void lsr_loopbuf_update( lsr_loopbuf_t *pThis, int offset, const char *pBuf, int size )
{
    char *p = lsr_loopbuf_get_pointer( pThis, offset );
    const char *pEnd = pBuf + size;
    while( pBuf < pEnd )
    {
        *p++ = *pBuf++;
        if ( p == pThis->m_pBufEnd )
            p = pThis->m_pBuf;
    }
}

char* lsr_loopbuf_search( lsr_loopbuf_t *pThis, int offset, const char *accept, int acceptLen )
{
    register char   *pSplitStart, 
                    *ptr = NULL, 
                    *pIter = lsr_loopbuf_get_pointer( pThis, offset );
    register const char *pAcceptPtr = accept;
    int iMiss = 0;
    
    if ( acceptLen > lsr_loopbuf_size( pThis ))
        return NULL;
    if ( pIter <= pThis->m_pEnd )
        return (char *)memmem( pIter, pThis->m_pEnd - pIter, accept, acceptLen );
    
    if ( acceptLen <= pThis->m_pBufEnd - pIter )
    {
        if ((ptr = (char *)memmem( pIter, pThis->m_pBufEnd - pIter, accept, acceptLen )) != NULL )
            return ptr;
        pIter = pThis->m_pBufEnd - (acceptLen - 1);
    }
    pSplitStart = pIter;
    
    while((pIter > pThis->m_pEnd) || (pAcceptPtr != accept))
    {
        if ( *pIter == *pAcceptPtr )
        {
            if ( pAcceptPtr == accept )
                ptr = pIter;
            if ( ++pAcceptPtr - accept == acceptLen )
                return ptr;
        }
        else
        {
            pAcceptPtr = accept;
            pIter = pSplitStart + iMiss++;
        }
        if ( ++pIter >= pThis->m_pBufEnd )
            pIter = pThis->m_pBuf;
    }
    return (char *)memmem( pThis->m_pBuf, pThis->m_pEnd - pThis->m_pBuf, accept, acceptLen );
}


int lsr_loopbuf_iov_insert( lsr_loopbuf_t *pThis, struct iovec *vect, int count )
{
    if ((pThis->m_pEnd > pThis->m_pHead) && (count == 1))
    {
        vect->iov_base = pThis->m_pHead;
        vect->iov_len = pThis->m_pEnd - pThis->m_pHead;
    }
    else if ((pThis->m_pEnd < pThis->m_pHead) && (count == 2))
    {
        vect->iov_base = pThis->m_pHead;
        vect->iov_len = pThis->m_pBufEnd - pThis->m_pHead;
        
        (++vect)->iov_base = pThis->m_pBuf;
        vect->iov_len = pThis->m_pEnd - pThis->m_pBuf;
    }
    else
    {
        errno = EINVAL;
        return -1;
    }
    return 0;
}


lsr_loopbuf_t *lsr_loopbuf_xnew( int size, lsr_xpool_t *pool )
{
    lsr_loopbuf_t *pThis;
    if ( !pool )
        return NULL;
        
    pThis = (lsr_loopbuf_t *)lsr_xpool_alloc( pool, sizeof( lsr_loopbuf_t ) );
    if ( !pThis )
        return NULL;
    if ( !lsr_loopbuf_x( pThis, size, pool ) )
    {
        lsr_xpool_free( pool, pThis );
        return NULL;
    }
    return pThis;
}

lsr_loopbuf_t *lsr_loopbuf_x( lsr_loopbuf_t *pThis, int size, lsr_xpool_t *pool )
{
    if ( size == 0 )
        size = LSR_LOOPBUFUNIT;
    memset( pThis, 0, sizeof( lsr_loopbuf_t ) );
    lsr_loopbuf_xreserve( pThis, size, pool );
    return pThis;
}

void lsr_loopbuf_xd( lsr_loopbuf_t *pThis, lsr_xpool_t *pool )
{
    lsr_loopbuf_deallocate( pThis, pool );
    pThis->m_pBuf = NULL;
    pThis->m_pBufEnd = NULL;
    pThis->m_pHead = NULL; 
    pThis->m_pEnd = NULL;
    pThis->m_iCapacity = 0;
}

void lsr_loopbuf_xdelete( lsr_loopbuf_t *pThis, lsr_xpool_t *pool )
{
    lsr_loopbuf_xd( pThis, pool );
    lsr_xpool_free( pool, pThis );
}

int lsr_loopbuf_xreserve( lsr_loopbuf_t *pThis, int size, lsr_xpool_t *pool )
{
    return lsr_loopbuf_allocate( pThis, pool, size );
}

int lsr_loopbuf_xappend( lsr_loopbuf_t *pThis, const char *pBuf, int size, lsr_xpool_t *pool )
{
    return lsr_loopbuf_iAppend( pThis, pBuf, size, pool );
}

int lsr_loopbuf_xguarantee( lsr_loopbuf_t *pThis, int size, lsr_xpool_t *pool )
{
    return lsr_loopbuf_iGuarantee( pThis, size, pool );
}

void lsr_loopbuf_xstraight( lsr_loopbuf_t *pThis, lsr_xpool_t *pool )
{
    lsr_loopbuf_t bufTmp;
    memset( &bufTmp, 0, sizeof( lsr_loopbuf_t ) );
    lsr_loopbuf_xguarantee( &bufTmp, lsr_loopbuf_size( pThis ), pool );
    lsr_loopbuf_used( &bufTmp, lsr_loopbuf_size( pThis ) );
    lsr_loopbuf_move_to( pThis, lsr_loopbuf_begin( &bufTmp ), lsr_loopbuf_size( pThis ) );
    lsr_loopbuf_swap( pThis, &bufTmp );
    lsr_loopbuf_xd( &bufTmp, pool );
}




lsr_xloopbuf_t *lsr_xloopbuf_new( int size, lsr_xpool_t *pool )
{
    lsr_xloopbuf_t *pThis;
    if ( !pool )
        return NULL;
        
    pThis = (lsr_xloopbuf_t *)lsr_xpool_alloc( pool, sizeof( lsr_xloopbuf_t ) );
    if ( !pThis )
        return NULL;
    if ( !lsr_xloopbuf( pThis, size, pool ) )
    {
        lsr_xpool_free( pool, pThis );
        return NULL;
    }
    return pThis;
}

lsr_xloopbuf_t *lsr_xloopbuf( lsr_xloopbuf_t *pThis, int size, lsr_xpool_t *pool )
{
    if ( size == 0 )
        size = LSR_LOOPBUFUNIT;
    memset( &pThis->m_loopbuf, 0, sizeof( lsr_loopbuf_t ) );
    pThis->m_pPool = pool;
    lsr_xloopbuf_reserve( pThis, size );
    return pThis;
}

void lsr_xloopbuf_d( lsr_xloopbuf_t *pThis )
{
    lsr_loopbuf_t *pBuf = &pThis->m_loopbuf;
    pBuf->m_pBuf = NULL;
    pBuf->m_pBufEnd = NULL;
    pBuf->m_pHead = NULL; 
    pBuf->m_pEnd = NULL;
    pBuf->m_iCapacity = 0;
    pThis->m_pPool = NULL;
}

void lsr_xloopbuf_delete( lsr_xloopbuf_t *pThis )
{
    lsr_xpool_t *pool = pThis->m_pPool;
    lsr_loopbuf_deallocate( &pThis->m_loopbuf, pool );
    lsr_xpool_free( pool, pThis );
}

void lsr_xloopbuf_swap( lsr_xloopbuf_t *lhs, lsr_xloopbuf_t *rhs )
{
    assert( rhs );
    char temp[sizeof( lsr_xloopbuf_t ) ];
    memmove( temp, lhs, sizeof( lsr_xloopbuf_t ) );
    memmove( lhs, rhs, sizeof( lsr_xloopbuf_t ) );
    memmove( rhs, temp, sizeof( lsr_xloopbuf_t ) );
}

int lsr_loopbuf_iAppend( lsr_loopbuf_t *pThis, const char *pBuf, int size, lsr_xpool_t *pool )
{
    if ((pBuf == NULL) || (size < 0))
    {
        errno = EINVAL;
        return -1;
    }
    if ( size )
    {
        if ( lsr_loopbuf_xguarantee( pThis, size, pool ) < 0 )
            return -1 ;
        int len = lsr_loopbuf_contiguous( pThis );
        if ( len > size )
            len = size;

        memmove( pThis->m_pEnd, pBuf, len );
        pBuf += len ;
        len = size - len;

        if ( len )
        {
            memmove( pThis->m_pBuf, pBuf, len );
        }
        lsr_loopbuf_used( pThis, size );
    }
    return size;
}

int lsr_loopbuf_iGuarantee( lsr_loopbuf_t *pThis, int size, lsr_xpool_t *pool )
{
    int avail = lsr_loopbuf_available( pThis );
    if ( size <= avail )
        return 0;
    return lsr_loopbuf_xreserve( pThis, size + lsr_loopbuf_size( pThis ) + 1, pool );
}







