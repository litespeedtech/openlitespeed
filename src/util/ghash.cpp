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
#include <util/ghash.h>
#include <util/pool.h>

#include <new>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { prime_count = 31    };
static const size_t s_prime_list[prime_count] =
{
  7ul,          13ul,         29ul,  
  53ul,         97ul,         193ul,       389ul,       769ul,
  1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
  49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
  1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
  50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
  1610612741ul, 3221225473ul, 4294967291ul
};

static int findRange( size_t sz )
{
    int i = 1;
    for( ; i < prime_count - 1; ++i )
    {
        if ( sz <= s_prime_list[i] )
            break;
    }
    return i;
}

static size_t roundUp( size_t sz )
{
    return s_prime_list[findRange(sz)];
}


hash_key_t GHash::hash_string(const void* __s)
{
  register hash_key_t __h = 0;
  register const char * p = (const char *)__s;
  register char ch = *(const char*)p++;
  for ( ; ch ; ch = *((const char*)p++))
    __h = __h * 31 + (ch );

  return __h;
}

int  GHash::comp_string( const void * pVal1, const void * pVal2 )
{   return strcmp( (const char *)pVal1, (const char *)pVal2 );  }

hash_key_t GHash::i_hash_string(const void* __s)
{
    register hash_key_t __h = 0;
    register const char * p = (const char *)__s;
    register char ch = *(const char*)p++;
    for ( ; ch ; ch = *((const char*)p++))
    {
        if (ch >= 'A' && ch <= 'Z')
            ch += 'a' - 'A';
        __h = __h * 31 + (ch );
    }
    return __h;
}

int  GHash::i_comp_string( const void * pVal1, const void * pVal2 )
{   return strncasecmp( (const char *)pVal1, (const char *)pVal2, strlen((const char *)pVal1) );  }
    
hash_key_t GHash::hf_ipv6( const void * pKey )
{
    hash_key_t key;
    if ( sizeof( hash_key_t ) == 4 )
    {
        key = *((const hash_key_t *)pKey) +
              *(((const hash_key_t *)pKey) + 1 ) +
              *(((const hash_key_t *)pKey) + 2 ) +
              *(((const hash_key_t *)pKey) + 3 );
    }
    else
    {
        key = *((const hash_key_t *)pKey) +
              *(((const hash_key_t *)pKey) + 1 );
    }
    return key;
}
    
int  GHash::cmp_ipv6( const void * pVal1, const void * pVal2 )
{
    return memcmp( pVal1, pVal2, 16 );
}




GHash::GHash( size_t init_size, hash_fn hf, val_comp vc )
    : m_capacity( 0 )
    , m_size( 0 )
    , m_full_factor( 2 )
    , m_hf( hf )
    , m_vc( vc )
    , m_grow_factor( 2 )
{
    if ( m_hf )
    {
        assert( m_vc );
        m_insert = insert_p;
        m_update = update_p;
        m_find = find_p;
    }
    else
    {
        m_insert = insert_num;
        m_update = update_num;
        m_find = find_num;
    }
    init_size = roundUp( init_size );
    m_table = (HashElem **)malloc( init_size * sizeof( HashElem*) );
    if ( !m_table )
        return;
    ::memset( m_table, 0, init_size * sizeof( HashElem*) );
    m_capacity = init_size;
    m_tableEnd = m_table + init_size;
}

GHash::~GHash()
{
    clear();
    if ( m_table )
    {
        free(m_table);
        m_table = NULL;
    }
}

#define getindex( k, n ) (k) % (n)

void GHash::rehash()
{
    int range = findRange( m_capacity );
    int newSize = s_prime_list[ range + m_grow_factor ];
    HashElem ** newTable = (HashElem **)malloc( newSize * sizeof( HashElem* ) );
    if ( newTable == NULL )
        return;
    ::memset( newTable, 0, sizeof( HashElem* ) * newSize  );
    iterator iterNext = begin();

    while( iterNext != end() )
    {

        iterator iter = iterNext;
        iterNext = next( iter );
        HashElem **pElem = newTable + getindex( iter->m_hkey, newSize );
        iter->m_pNext = *pElem;
        *pElem = iter;
    }
    free( m_table );
    m_table = newTable;
    m_capacity = newSize;
    m_tableEnd = m_table + newSize;
}

static int release_hash_elem( GHash::iterator iter )
{
    g_pool.deallocate( iter, sizeof( GHash::HashElem ) );
    return 0;
}

void GHash::clear()
{
    int n = for_each( begin(), end(), release_hash_elem );
    if ( n != (int)m_size )
    {
        fprintf( stderr, "GHash::clear() error: n=%d, m_size=%d!\n",
                    n, (int)m_size );
    }
    ::memset( m_table, 0, sizeof( HashElem* ) * m_capacity );
    m_size = 0;
}

void GHash::erase( iterator iter )
{
    if ( !iter )
        return;
    size_t index = getindex( iter->m_hkey, m_capacity );
    HashElem * pElem = m_table[index];
    if ( pElem == iter )
    {
        m_table[index] = iter->m_pNext;
    }
    else
    {
        if ( !pElem )
            return;
        while( 1 )
        {
            if ( !( pElem->m_pNext ))
                return ;
            if ( pElem->m_pNext == iter )
                break;
            pElem = pElem->m_pNext;
        }
        pElem->m_pNext = iter->m_pNext;
    }
    g_pool.deallocate( iter, sizeof( HashElem ) );
    --m_size;
}

GHash::iterator GHash::find_num( GHash *pThis, const void * pKey )
{
    size_t index = getindex( (hash_key_t) pKey, pThis->m_capacity );
    HashElem * pElem = pThis->m_table[index];
    while( pElem &&( pKey != pElem->m_pKey ))
        pElem = pElem->m_pNext;
    return pElem;
}

GHash::iterator GHash::find2( const void * pKey, hash_key_t key )
{
    size_t index = getindex( key, m_capacity );
    HashElem * pElem = m_table[index];
    while( pElem )
    {
        assert( pElem->m_pKey );
        if ((*m_vc)( pKey, pElem->m_pKey ) != 0 )
            pElem = pElem->m_pNext;
        else
        {
            assert(!(( pElem->getHKey() != key )&&(pKey == pElem->m_pKey)));
            break;
        }
    }
    return pElem;
}

GHash::iterator GHash::insert2( const void * pKey, void *pValue, hash_key_t key )
{
    if ( m_size * m_full_factor > m_capacity )
        rehash();
    HashElem * pNew = (HashElem *)g_pool.allocate( sizeof( HashElem ) );
    if ( !pNew )
        return end();
    pNew->m_pKey = pKey;
    pNew->m_pData = pValue;
    pNew->m_hkey = key;
    HashElem ** pElem = m_table + getindex( key, m_capacity );
    pNew->m_pNext = *pElem;
    *pElem = pNew;
    ++m_size;
    return pNew;
}

GHash::iterator GHash::insert_num(GHash * pThis, const void * pKey, void * pValue )
{
    iterator iter = find_num( pThis, pKey );
    if ( iter )
        return pThis->end();
    return pThis->insert2( pKey, pValue, (hash_key_t)pKey );
}

GHash::iterator GHash::update_num(GHash * pThis, const void * pKey, void * pValue)
{
    iterator iter = find_num( pThis, pKey );
    if ( iter != pThis->end() )
    {
        iter->setData( pValue );
        return iter;
    }
    return pThis->insert2( pKey, pValue, (hash_key_t)pKey );
}


GHash::iterator GHash::insert_p( GHash * pThis, const void * pKey, void * pValue )
{
    hash_key_t key = (*pThis->m_hf)( pKey );
    iterator iter = pThis->find2( pKey, key );
    if ( iter )
        return pThis->end();
    return pThis->insert2( pKey, pValue, key );
}

GHash::iterator GHash::update_p(GHash * pThis, const void * pKey, void * pValue)
{
    hash_key_t key = (*pThis->m_hf)( pKey );
    iterator iter = pThis->find2( pKey, key );
    if ( iter )
    {
        iter->setKey( pKey );
        iter->setData( pValue );
        return iter;
    }
    return pThis->insert2( pKey, pValue, key );
}

GHash::iterator GHash::find_p(GHash * pThis, const void * pKey )
{
    return pThis->find2( pKey, (*pThis->m_hf)(pKey ) );
}

GHash::iterator GHash::next( iterator iter )
{
    if ( !iter )
        return end();
    if ( iter->m_pNext )
        return iter->m_pNext;
    HashElem ** p = m_table + getindex( iter->m_hkey, m_capacity ) + 1;
    while( p < m_tableEnd)
    {
        if ( *p )
            return *p;
        ++p;
    }
    return end();
}

int GHash::for_each( iterator beg, iterator end,
                    for_each_fn fun )
{
    if ( !fun )
    {
        errno = EINVAL;
        return -1;
    }
    if ( !beg )
        return 0;
    int n = 0;
    iterator iterNext = beg;
    iterator iter ;
    while( iterNext && iterNext != end )
    {
        iter = iterNext;
        iterNext = next( iterNext );
        if ( fun( iter ) )
            break;
        ++n;
    }
    return n;
}

int GHash::for_each2( iterator beg, iterator end,
                    for_each_fn2 fun, void * pUData )
{
    if ( !fun )
    {
        errno = EINVAL;
        return -1;
    }
    if ( !beg )
        return 0;
    int n = 0;
    iterator iterNext = beg;
    iterator iter ;
    while( iterNext && iterNext != end )
    {
        iter = iterNext;
        iterNext = next( iterNext );
        if ( fun( iter, pUData ) )
            break;
        ++n;
    }
    return n;
}

GHash::iterator GHash::begin()
{
    if ( !m_size )
        return end();
    HashElem ** p = m_table;
    while( p < m_tableEnd )
    {
        if ( *p )
            return *p;
        ++p;
    }
    return NULL;
}

void GHash::swap( GHash & rhs )
{
    char temp[ sizeof( GHash ) ];
    memmove( temp, this, sizeof( GHash ) );
    memmove( this, &rhs, sizeof( GHash ) );
    memmove( &rhs, temp, sizeof( GHash ) );
    
}

