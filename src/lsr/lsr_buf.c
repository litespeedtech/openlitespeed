
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
#include <lsr/lsr_buf.h>
#include <lsr/lsr_xpool.h>

#include <errno.h>
#include <stdlib.h>

lsr_inline void *lsr_buf_do_alloc( lsr_xpool_t *pool, size_t size )
{
    return ( pool? lsr_xpool_alloc( pool, size ): lsr_palloc( size ) );
}

lsr_inline void lsr_buf_do_free( lsr_xpool_t *pool, void *ptr )
{
    if ( pool )
        lsr_xpool_free( pool, ptr );
    else
        lsr_pfree( ptr );
    return;
}

int lsr_buf_xreserve( lsr_buf_t * pThis, int size, lsr_xpool_t * pool )
{
    char * pBuf;
    if ( pThis->m_pBufEnd - pThis->m_pBuf == size )
        return 0;
    if ( pool )
        pBuf = (char *)lsr_xpool_realloc( pool, pThis->m_pBuf, size );
    else
        pBuf = (char *)lsr_prealloc( pThis->m_pBuf, size );
    if (( pBuf != NULL )||( size == 0 ))
    {
        pThis->m_pEnd = pBuf + ( pThis->m_pEnd - pThis->m_pBuf );
        pThis->m_pBuf = pBuf;
        pThis->m_pBufEnd = pBuf + size;
        if ( pThis->m_pEnd > pThis->m_pBufEnd )
            pThis->m_pEnd = pThis->m_pBufEnd;
        return 0;
    }
    return -1;
}

static void lsr_buf_deallocate( lsr_buf_t * pThis, lsr_xpool_t *pool )
{
    if ( pThis->m_pBuf )
    {
        lsr_buf_do_free( pool, pThis->m_pBuf );
        pThis->m_pBuf = NULL;
    }
}

int lsr_buf_pop_front_to( lsr_buf_t * pThis, char * pBuf, int sz )
{
    if ( !lsr_buf_empty( pThis ) )
    {
        int copysize = ( lsr_buf_size(pThis) < sz )? lsr_buf_size(pThis) : sz ;
        memmove( pBuf, pThis->m_pBuf, copysize );
        lsr_buf_pop_front( pThis, copysize );
        return copysize;
    }
    return 0;
}

int lsr_buf_pop_front( lsr_buf_t * pThis, int sz )
{
    if ( sz >= lsr_buf_size( pThis ) )
    {
        pThis->m_pEnd = pThis->m_pBuf;
    }
    else
    {
        memmove( pThis->m_pBuf, pThis->m_pBuf + sz, lsr_buf_size( pThis ) - sz );
        pThis->m_pEnd -= sz;
    }
    return sz;
}

int lsr_buf_pop_end( lsr_buf_t * pThis, int sz )
{
    if ( sz > lsr_buf_size( pThis ) )
        pThis->m_pEnd = pThis->m_pBuf;
    else
        pThis->m_pEnd -= sz;
    return sz;
}

#define SWAP( a, b, t )  do{ t = a; a = b; b = t;  }while(0)
void lsr_buf_swap( lsr_buf_t * pThis, lsr_buf_t * pRhs )
{
    char * pTemp;
    SWAP( pThis->m_pBuf, pRhs->m_pBuf, pTemp );

    SWAP( pThis->m_pBufEnd, pRhs->m_pBufEnd, pTemp );
    SWAP( pThis->m_pEnd, pRhs->m_pEnd, pTemp );
}

lsr_buf_t * lsr_buf_xnew( int size, lsr_xpool_t * pool )
{
    lsr_buf_t * pThis = (lsr_buf_t *)lsr_buf_do_alloc( pool, sizeof( lsr_buf_t ));
    if ( pThis )
        lsr_buf_x( pThis, size, pool );
    return pThis;
}

int lsr_buf_x( lsr_buf_t * pThis, int size, lsr_xpool_t * pool )
{
    memset( pThis, 0, sizeof( lsr_buf_t ) );
    return lsr_buf_xreserve( pThis, size, pool );
}

void lsr_buf_xd( lsr_buf_t * pThis, lsr_xpool_t * pool )
{
    lsr_buf_deallocate( pThis, pool );
    pThis->m_pBuf = NULL;
    pThis->m_pBufEnd = NULL;
    pThis->m_pEnd = NULL;
}

void lsr_buf_xdelete( lsr_buf_t * pThis, lsr_xpool_t * pool )
{
    lsr_buf_xd( pThis, pool );
    lsr_buf_do_free( pool, pThis );
}

lsr_xbuf_t * lsr_xbuf_new( int size, lsr_xpool_t * pool )
{
    lsr_xbuf_t * pThis = lsr_buf_do_alloc( pool, sizeof( lsr_xbuf_t ));
    if ( pThis )
        lsr_xbuf( pThis, size, pool );
    return pThis;
}

int lsr_xbuf( lsr_xbuf_t * pThis, int size, lsr_xpool_t * pool )
{
    memset( &pThis->m_buf, 0, sizeof( lsr_buf_t ) );
    pThis->m_pPool = pool;
    return lsr_buf_xreserve( &pThis->m_buf, size, pool );
}

void lsr_xbuf_d( lsr_xbuf_t * pThis )
{
    lsr_buf_xd( &pThis->m_buf, pThis->m_pPool );
    pThis->m_pPool = NULL;
}

void lsr_xbuf_delete( lsr_xbuf_t * pThis )
{
    lsr_xbuf_d( pThis );
    lsr_xpool_free( pThis->m_pPool, pThis );
}

void lsr_xbuf_swap( lsr_xbuf_t * pThis, lsr_xbuf_t * pRhs )
{
    lsr_xpool_t *tmpPool;
    lsr_buf_swap( &pThis->m_buf, &pRhs->m_buf );
    tmpPool = pThis->m_pPool;
    pThis->m_pPool = pRhs->m_pPool;
    pRhs->m_pPool = tmpPool;
}

int lsr_buf_xappend2( lsr_buf_t * pThis, const char * pBuf, int size, lsr_xpool_t * pool )
{
    if (( pBuf == NULL )||( size < 0 ))
    {
        errno = EINVAL;
        return -1;
    }
    if ( size == 0 )
        return 0;
    if ( size > lsr_buf_available( pThis ) )
    {
        if ( lsr_buf_xgrow( pThis, size - lsr_buf_available( pThis ), pool ) == -1 )
            return -1;
    }
    memmove( lsr_buf_end( pThis ), pBuf, size );
    lsr_buf_used( pThis, size );
    return size;
}

int lsr_buf_xgrow( lsr_buf_t * pThis, int size, lsr_xpool_t * pool )
{
    size = (( size + 511 ) >> 9 ) << 9  ;
    return lsr_buf_xreserve( pThis, lsr_buf_capacity( pThis ) + size, pool );
}
