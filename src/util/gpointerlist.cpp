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
#include <util/gpointerlist.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <util/pool.h>
#include <assert.h>
#include <new>


GPointerList::GPointerList()
{
    memset( &m_pStore, 0, (char *)(&m_pEnd + 1) - (char *)&m_pStore );
}

GPointerList::GPointerList( size_t initSize )
{
    if ( initSize < 8 )
        initSize = 8;
    m_pEnd = m_pStore = (void **)g_pool.allocate( initSize*sizeof( void *) );
    if ( m_pStore )
    {
        m_pStoreEnd = m_pStore + initSize;
    }
    else
    {
        m_pStoreEnd = NULL;
    }
}

GPointerList::GPointerList( const GPointerList & rhs )
{
    m_pStore = (void **)g_pool.allocate( rhs.size()*sizeof( void *) );
    if ( m_pStore )
    {
        m_pEnd = m_pStoreEnd = m_pStore + rhs.size();
        memmove( m_pStore, rhs.m_pStore,
            (const char *)rhs.m_pEnd - (const char *)rhs.m_pStore );
    }
}


GPointerList::~GPointerList()
{
    if ( m_pStore )
        g_pool.deallocate( m_pStore, capacity() * sizeof( void *) );
}

int GPointerList::allocate( int capacity )
{
    void **pStore =  (void **)g_pool.reallocate(m_pStore,
                        this->capacity() * sizeof( void *),
                        capacity * sizeof( void *));
    if (!pStore)
    {
        return -1;
    }
    m_pEnd = pStore + ( m_pEnd - m_pStore );
    m_pStore = pStore;
    m_pStoreEnd = m_pStore + capacity;
    return 0;
}

int GPointerList::resize( size_t sz )
{   if ( capacity() < sz )
        if ( allocate( sz ) )
            return -1;
    m_pEnd = m_pStore + sz;
    return 0;
}

int GPointerList::push_back( void * pPointer )
{
    if ( m_pEnd == m_pStoreEnd )
    {
        int n = capacity() * 2;
        if ( n < 16 )
            n = 16;
        if ( allocate( n ) )
            return -1;
    }
    *m_pEnd++ = pPointer;
    return 0;
}

int GPointerList::push_back( const GPointerList& list )
{
    if ( m_pEnd + list.size() > m_pStoreEnd )
    {
        int n = capacity() + list.size();
        if ( allocate( n ) )
            return -1;
    }
    for( const_iterator iter = list.begin(); iter != list.end(); ++iter )
        safe_push_back( *iter );
    return 0;
}


void GPointerList::safe_push_back( void **pPointer, int n )
{
    assert( n > 0 );
    assert( m_pEnd + n <= m_pStoreEnd );
    memmove( m_pEnd, pPointer, n * sizeof( void *) );
    m_pEnd += n;
}

void GPointerList::safe_pop_back( void **pPointer, int n )
{
    assert( n > 0 );
    assert( m_pEnd - n >= m_pStore );
    memmove( pPointer, m_pEnd - n, n * sizeof( void *) );
    m_pEnd -= n;
}


int GPointerList::for_each( iterator beg, iterator end, gpl_for_each_fn fn )
{
    int n = 0;
    iterator iterNext = beg;
    iterator iter ;
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

GPointerList::const_iterator GPointerList::lower_bound( const void * pKey,
                int(*compare)(const void *, const void *) ) const
{
    if ( !pKey )
        return end();
    const_iterator e = end();
    const_iterator b = begin();
    while( b != e )
    {
        const_iterator m = b + (e - b) / 2;
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

GPointerList::const_iterator GPointerList::bfind( const void * pKey,
             int(*compare)(const void *, const void *) ) const
{
    const_iterator b = begin();
    const_iterator e = end();
    while( b != e )
    {
        const_iterator m = b + (e - b) / 2;
        int c = compare( pKey, *m );
        if ( c == 0 )
            return m;
        else if ( c < 0 )
            e = m;
        else
            b = m+1;
    }
    return end();
}

#include <util/swap.h>

void GPointerList::swap( GPointerList & rhs )
{
    void ** t;
    GSWAP( m_pStore, rhs.m_pStore, t );
    GSWAP( m_pStoreEnd, rhs.m_pStoreEnd, t );
    GSWAP( m_pEnd, rhs.m_pEnd, t );
    
}

void GPointerList::sort( int(*compare)(const void *, const void *) )
{
    ::qsort( begin(), size(), sizeof( void **), compare );
}


