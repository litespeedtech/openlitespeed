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
#include <lsr/lsr_ptrlist.h>
#include <lsr/lsr_pool.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <assert.h>


void lsr_ptrlist( lsr_ptrlist_t *pThis, size_t initSize )
{
    if ( initSize <= 0 )
    {
        memset( pThis, 0, sizeof(lsr_ptrlist_t) );
        return;
    }
    if ( initSize < 8 )
        initSize = 8;
    pThis->m_pEnd = pThis->m_pStore = (void **)
      lsr_palloc( initSize*sizeof(void *) );
    if ( pThis->m_pStore )
    {
        pThis->m_pStoreEnd = pThis->m_pStore + initSize;
    }
    else
    {
        pThis->m_pStoreEnd = NULL;
    }
}

void lsr_ptrlist_copy( lsr_ptrlist_t *pThis, const lsr_ptrlist_t *pRhs )
{
    assert( pRhs );
    pThis->m_pStore = (void **)
      lsr_palloc( lsr_ptrlist_size( pRhs )*sizeof(void *) );
    if ( pThis->m_pStore )
    {
        pThis->m_pEnd = pThis->m_pStoreEnd =
          pThis->m_pStore + lsr_ptrlist_size( pRhs );
        memmove( pThis->m_pStore, pRhs->m_pStore,
          (const char *)pRhs->m_pEnd - (const char *)pRhs->m_pStore );
    }
}

void lsr_ptrlist_d( lsr_ptrlist_t *pThis )
{
    if ( pThis->m_pStore )
    {
        lsr_pfree( pThis->m_pStore );
    }
}

static int lsr_ptrlist_allocate( lsr_ptrlist_t *pThis, int capacity )
{
    void **pStore = (void **)lsr_prealloc( pThis->m_pStore,
      capacity * sizeof(void *) );
    if (!pStore)
    {
        return -1;
    }
    pThis->m_pEnd = pStore + ( pThis->m_pEnd - pThis->m_pStore );
    pThis->m_pStore = pStore;
    pThis->m_pStoreEnd = pThis->m_pStore + capacity;
    return 0;
}

int lsr_ptrlist_reserve( lsr_ptrlist_t *pThis, size_t sz )
{
    return lsr_ptrlist_allocate( pThis, sz );
}

int lsr_ptrlist_grow( lsr_ptrlist_t *pThis, size_t sz )
{
    return lsr_ptrlist_allocate( pThis, sz + lsr_ptrlist_capacity( pThis ) );
}

int lsr_ptrlist_resize( lsr_ptrlist_t *pThis, size_t sz )
{
    if ( lsr_ptrlist_capacity( pThis ) < sz )
        if ( lsr_ptrlist_allocate( pThis, sz ) )
            return -1;
    pThis->m_pEnd = pThis->m_pStore + sz;
    return 0;
}

int lsr_ptrlist_push_back( lsr_ptrlist_t *pThis, void *pPointer )
{
    if ( pThis->m_pEnd == pThis->m_pStoreEnd )
    {
        int n = lsr_ptrlist_capacity( pThis ) * 2;
        if ( n < 16 )
            n = 16;
        if ( lsr_ptrlist_allocate( pThis, n ) )
            return -1;
    }
    *pThis->m_pEnd++ = pPointer;
    return 0;
}

int lsr_ptrlist_push_back2( lsr_ptrlist_t *pThis, const lsr_ptrlist_t *plist )
{
    assert( plist );
    if ( pThis->m_pEnd + lsr_ptrlist_size( plist ) > pThis->m_pStoreEnd )
    {
        int n = lsr_ptrlist_capacity( pThis ) + lsr_ptrlist_size( plist );
        if ( lsr_ptrlist_allocate( pThis, n ) )
            return -1;
    }
    lsr_const_ptrlist_iter iter;
    for( iter = lsr_ptrlist_begin( (lsr_ptrlist_t *)plist );
      iter != lsr_ptrlist_end( (lsr_ptrlist_t *)plist ); ++iter )
    {
        lsr_ptrlist_unsafe_push_back( pThis, *iter );
    }
    return 0;
}


void lsr_ptrlist_unsafe_push_backn( lsr_ptrlist_t *pThis, void **pPointer, int n )
{
    assert( n > 0 );
    assert( pThis->m_pEnd + n <= pThis->m_pStoreEnd );
    memmove( pThis->m_pEnd, pPointer, n * sizeof(void *) );
    pThis->m_pEnd += n;
}

void lsr_ptrlist_unsafe_pop_backn( lsr_ptrlist_t *pThis, void **pPointer, int n )
{
    assert( n > 0 );
    assert( pThis->m_pEnd - n >= pThis->m_pStore );
    pThis->m_pEnd -= n;
    memmove( pPointer, pThis->m_pEnd, n * sizeof(void *) );
}

int lsr_ptrlist_for_each(
  lsr_ptrlist_iter beg, lsr_ptrlist_iter end, gpl_for_each_fn fn )
{
    int n = 0;
    lsr_ptrlist_iter iterNext = beg;
    lsr_ptrlist_iter iter;
    while( iterNext && iterNext != end )
    {
        iter = iterNext;
        iterNext = iter + 1;
        if ( fn( *iter ) )
            break;
        ++n;
    }
    return n;
}

lsr_const_ptrlist_iter lsr_ptrlist_lower_bound(
  const lsr_ptrlist_t *pThis,
  const void *pKey, int (*compare)(const void *, const void *) )
{
    if ( !pKey )
        return lsr_ptrlist_end( (lsr_ptrlist_t *)pThis );
    lsr_const_ptrlist_iter e = lsr_ptrlist_end( (lsr_ptrlist_t *)pThis );
    lsr_const_ptrlist_iter b = lsr_ptrlist_begin( (lsr_ptrlist_t *)pThis );
    while( b != e )
    {
        lsr_const_ptrlist_iter m = b + (e - b) / 2;
        int c = compare( pKey, *m );
        if ( c == 0 )
            return m;
        else if ( c < 0 )
            e = m;
        else
            b = m+1;
    }
    return b;
}

lsr_const_ptrlist_iter lsr_ptrlist_bfind(
  const lsr_ptrlist_t *pThis,
  const void *pKey, int (*compare)(const void *, const void *) )
{
    lsr_const_ptrlist_iter b = lsr_ptrlist_begin( (lsr_ptrlist_t *)pThis );
    lsr_const_ptrlist_iter e = lsr_ptrlist_end( (lsr_ptrlist_t *)pThis );
    while( b != e )
    {
        lsr_const_ptrlist_iter m = b + (e - b) / 2;
        int c = compare( pKey, *m );
        if ( c == 0 )
            return m;
        else if ( c < 0 )
            e = m;
        else
            b = m+1;
    }
    return lsr_ptrlist_end( (lsr_ptrlist_t *)pThis );
}

#include <util/swap.h>

void lsr_ptrlist_swap(
  lsr_ptrlist_t *pThis, lsr_ptrlist_t *pRhs )
{
    void ** t;
    assert( pRhs );
    GSWAP( pThis->m_pStore, pRhs->m_pStore, t );
    GSWAP( pThis->m_pStoreEnd, pRhs->m_pStoreEnd, t );
    GSWAP( pThis->m_pEnd, pRhs->m_pEnd, t );
    
}

void lsr_ptrlist_sort(
  lsr_ptrlist_t *pThis, int (*compare)(const void *, const void *) )
{
    qsort( lsr_ptrlist_begin( pThis ),
      lsr_ptrlist_size( pThis ), sizeof(void **), compare );
}

