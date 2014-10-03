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
#include <lsr/lsr_hash.h>
#include "lsr_internal.h"
#include <lsr/lsr_pool.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {    prime_count = 31    };
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

static lsr_hash_iter lsr_hash_find_num( lsr_hash_t *pThis, const void *pKey );
static lsr_hash_iter lsr_hash_insert_num( lsr_hash_t *pThis, const void *pKey, void *pValue );
static lsr_hash_iter lsr_hash_update_num( lsr_hash_t *pThis, const void *pKey, void *pValue );
static lsr_hash_iter lsr_hash_insert_p( lsr_hash_t *pThis, const void *pKey, void *pValue );
static lsr_hash_iter lsr_hash_update_p( lsr_hash_t *pThis, const void *pKey, void *pValue );
static lsr_hash_iter lsr_hash_find_p( lsr_hash_t *pThis, const void *pKey );

lsr_inline void *lsr_hash_do_alloc( lsr_xpool_t *pool, size_t size )
{
    return ( pool? lsr_xpool_alloc( pool, size ): lsr_palloc( size ) );
}

lsr_inline void lsr_hash_do_free( lsr_xpool_t *pool, void *ptr )
{
    if ( pool )
        lsr_xpool_free( pool, ptr );
    else
        lsr_pfree( ptr );
    return;
}

static int find_range( size_t sz )
{
    int i = 1;
    for( ; i < prime_count - 1; ++i )
    {
        if ( sz <= s_prime_list[i] )
            break;
    }
    return i;
}

static size_t round_up( size_t sz )
{
    return s_prime_list[find_range( sz )];
}

lsr_hash_t *lsr_hash_new( size_t init_size, lsr_hash_hash_fn hf, lsr_hash_val_comp vc, lsr_xpool_t *pool )
{
    lsr_hash_t *hash;
    if ( (hash = (lsr_hash_t *)lsr_hash_do_alloc( pool, sizeof( lsr_hash_t ) )) != NULL )
    {
        if ( !lsr_hash( hash, init_size, hf, vc, pool ) )
        {
            lsr_hash_do_free( pool, (void *)hash );
            return NULL;
        }
    }
    return hash;
}

lsr_hash_t *lsr_hash( lsr_hash_t *hash, size_t init_size, lsr_hash_hash_fn hf, lsr_hash_val_comp vc, lsr_xpool_t *pool )
{
    hash->m_capacity = 0;
    hash->m_size = 0;
    hash->m_full_factor = 2;
    hash->m_hf = hf;
    hash->m_vc = vc;
    hash->m_grow_factor = 2;
    hash->m_xpool = pool;
    
    if ( hash->m_hf )
    {
        assert( hash->m_vc );
        hash->m_insert = lsr_hash_insert_p;
        hash->m_update = lsr_hash_update_p;
        hash->m_find = lsr_hash_find_p;
    }
    else
    {
        hash->m_insert = lsr_hash_insert_num;
        hash->m_update = lsr_hash_update_num;
        hash->m_find = lsr_hash_find_num;
    }
    
    init_size = round_up( init_size );
    hash->m_table = (lsr_hashelem_t **)lsr_hash_do_alloc( pool, init_size * sizeof( lsr_hashelem_t* ) );
    if ( !hash->m_table )
        return NULL;
    memset( hash->m_table, 0, init_size * sizeof( lsr_hashelem_t* ) );
    hash->m_capacity = init_size;
    hash->m_tableEnd = hash->m_table + init_size;
    
    return hash;
}

void lsr_hash_d( lsr_hash_t *pThis )
{
    lsr_hash_clear( pThis );
    if ( pThis->m_table )
    {
        lsr_hash_do_free( pThis->m_xpool, (void *)pThis->m_table );
        pThis->m_table = NULL;
    }
}


void lsr_hash_delete( lsr_hash_t *pThis )
{
    lsr_hash_d( pThis );
    lsr_hash_do_free( pThis->m_xpool, (void *)pThis );
}

lsr_hash_key_t lsr_hash_hf_string( const void *__s )
{
    register lsr_hash_key_t __h = 0;
    register const char *p = (const char *)__s;
    register char ch = *(const char *)p++;
    for ( ; ch ; ch = *((const char *)p++))
        __h = __h * 31 + (ch );
    
    return __h;
}

int lsr_hash_cmp_string( const void *pVal1, const void *pVal2 )
{   return strcmp( (const char *)pVal1, (const char *)pVal2 );  }

lsr_hash_key_t lsr_hash_hf_ci_string( const void *__s )
{
    register lsr_hash_key_t __h = 0;
    register const char *p = (const char *)__s;
    register char ch = *(const char *)p++;
    for ( ; ch ; ch = *((const char *)p++))
    {
        if ( ch >= 'A' && ch <= 'Z' )
            ch += 'a' - 'A';
        __h = __h * 31 + (ch);
    }
    return __h;
}

int lsr_hash_cmp_ci_string( const void *pVal1, const void *pVal2 )
{   return strncasecmp( (const char *)pVal1, (const char *)pVal2, strlen((const char *)pVal1) );  }
    
lsr_hash_key_t lsr_hash_hf_ipv6( const void *pKey )
{
    lsr_hash_key_t key;
    if ( sizeof( lsr_hash_key_t ) == 4 )
    {
        key = *((const lsr_hash_key_t *)pKey) +
              *(((const lsr_hash_key_t *)pKey) + 1 ) +
              *(((const lsr_hash_key_t *)pKey) + 2 ) +
              *(((const lsr_hash_key_t *)pKey) + 3 );
    }
    else
    {
        key = *((const lsr_hash_key_t *)pKey) +
              *(((const lsr_hash_key_t *)pKey) + 1 );
    }
    return key;
}
    
int  lsr_hash_cmp_ipv6( const void *pVal1, const void *pVal2 )
{
    return memcmp( pVal1, pVal2, 16 );
}

const void *lsr_hash_get_key( lsr_hashelem_t *pElem )  
{
    return pElem->m_pKey;  
}
    
void *lsr_hash_get_data( lsr_hashelem_t *pElem )      
{
    return pElem->m_pData;
}
    
lsr_hash_key_t lsr_hash_get_hkey( lsr_hashelem_t *pElem )  
{
    return pElem->m_hkey;
}
    
lsr_hashelem_t *lsr_hash_get_next( lsr_hashelem_t *pElem )  
{
    return pElem->m_pNext;
}

void lsr_hash_set_data( lsr_hashelem_t *pElem, void *p )    
{
    pElem->m_pData = p;
}

lsr_hash_iter lsr_hash_begin( lsr_hash_t *pThis )
{
    if ( !pThis->m_size )
        return lsr_hash_end( pThis );
    lsr_hashelem_t **p = pThis->m_table;
    while( p < pThis->m_tableEnd )
    {
        if ( *p )
            return *p;
        ++p;
    }
    return NULL;
}

void lsr_hash_swap( lsr_hash_t *lhs, lsr_hash_t *rhs )
{
    char temp[ sizeof( lsr_hash_t ) ];
    assert( lhs != NULL && rhs != NULL );
    memmove( temp, lhs, sizeof( lsr_hash_t ) );
    memmove( lhs, rhs, sizeof( lsr_hash_t ) );
    memmove( rhs, temp, sizeof( lsr_hash_t ) );
}

#define lsr_hash_getindex( k, n ) (k) % (n)

lsr_hash_iter lsr_hash_next( lsr_hash_t *pThis, lsr_hash_iter iter )
{
    if ( !iter )
        return lsr_hash_end( pThis );
    if ( iter->m_pNext )
        return iter->m_pNext;
    lsr_hashelem_t **p = pThis->m_table + lsr_hash_getindex( iter->m_hkey, pThis->m_capacity ) + 1;
    while( p < pThis->m_tableEnd )
    {
        if ( *p )
            return *p;
        ++p;
    }
    return lsr_hash_end( pThis );
}

int lsr_hash_for_each( lsr_hash_t *pThis, lsr_hash_iter beg, 
                        lsr_hash_iter end, lsr_hash_for_each_fn fun )
{
    if ( !fun )
    {
        errno = EINVAL;
        return -1;
    }
    if ( !beg )
        return 0;
    int n = 0;
    lsr_hash_iter iterNext = beg;
    lsr_hash_iter iter;
    while( iterNext && iterNext != end )
    {
        iter = iterNext;
        iterNext = lsr_hash_next( pThis, iterNext );
        if ( fun( iter->m_pKey, iter->m_pData ) )
            break;
        ++n;
    }
    return n;
}

int lsr_hash_for_each2( lsr_hash_t *pThis, lsr_hash_iter beg, lsr_hash_iter end,
                    lsr_hash_for_each2_fn fun, void *pUData )
{
    if ( !fun )
    {
        errno = EINVAL;
        return -1;
    }
    if ( !beg )
        return 0;
    int n = 0;
    lsr_hash_iter iterNext = beg;
    lsr_hash_iter iter ;
    while( iterNext && iterNext != end )
    {
        iter = iterNext;
        iterNext = lsr_hash_next( pThis, iterNext );
        if ( fun( iter->m_pKey, iter->m_pData, pUData ) )
            break;
        ++n;
    }
    return n;
}

static void lsr_hash_rehash( lsr_hash_t *pThis )
{
    int range = find_range( pThis->m_capacity );
    int newSize = s_prime_list[ range + pThis->m_grow_factor ];
    lsr_hashelem_t **newTable;
    newTable = (lsr_hashelem_t **)
      lsr_hash_do_alloc( pThis->m_xpool, newSize * sizeof( lsr_hashelem_t* ));
    if ( newTable == NULL )
        return;
    memset( newTable, 0, sizeof( lsr_hashelem_t* ) * newSize );
    lsr_hash_iter iterNext = lsr_hash_begin( pThis );

    while( iterNext != lsr_hash_end( pThis ) )
    {

        lsr_hash_iter iter = iterNext;
        iterNext = lsr_hash_next( pThis, iter );
        lsr_hashelem_t **pElem = newTable + lsr_hash_getindex( iter->m_hkey, newSize );
        iter->m_pNext = *pElem;
        *pElem = iter;
    }
    lsr_hash_do_free( pThis->m_xpool, (void *)pThis->m_table );
    pThis->m_table = newTable;
    pThis->m_capacity = newSize;
    pThis->m_tableEnd = pThis->m_table + newSize;
}

static int lsr_hash_release_hash_elem( lsr_hash_t *pThis, lsr_hash_iter iter )
{
    lsr_hash_do_free( pThis->m_xpool, (void *)iter );
    return 0;
}

void lsr_hash_clear( lsr_hash_t *pThis )
{
    lsr_hash_iter end = lsr_hash_end( pThis );
    lsr_hash_iter iterNext = lsr_hash_begin( pThis );
    lsr_hash_iter iter;
    while( iterNext && iterNext != end )
    {
        iter = iterNext;
        iterNext = lsr_hash_next( pThis, iterNext );
        if ( lsr_hash_release_hash_elem( pThis, iter ))
        {
            fprintf( stderr, "lsr_hash_clear() error!\n" );
            break;
        }
    }
    if ( pThis->m_table )
        memset( pThis->m_table, 0, sizeof( lsr_hashelem_t* ) * pThis->m_capacity );
    pThis->m_size = 0;
}

void lsr_hash_erase( lsr_hash_t *pThis, lsr_hash_iter iter )
{
    if ( !iter )
        return;
    size_t index = lsr_hash_getindex( iter->m_hkey, pThis->m_capacity );
    lsr_hashelem_t *pElem = pThis->m_table[index];
    if ( pElem == iter )
    {
        pThis->m_table[index] = iter->m_pNext;
    }
    else
    {
        if ( !pElem )
            return;
        while( 1 )
        {
            if ( !( pElem->m_pNext ) )
                return ;
            if ( pElem->m_pNext == iter )
                break;
            pElem = pElem->m_pNext;
        }
        pElem->m_pNext = iter->m_pNext;
    }
    lsr_hash_do_free( pThis->m_xpool, (void *)iter );
    --pThis->m_size;
}

static lsr_hash_iter lsr_hash_find_num( lsr_hash_t *pThis, const void *pKey )
{
    size_t index = lsr_hash_getindex( (lsr_hash_key_t) pKey, pThis->m_capacity );
    lsr_hashelem_t *pElem = pThis->m_table[index];
    while( pElem && (pKey != pElem->m_pKey))
        pElem = pElem->m_pNext;
    return pElem;
}

static lsr_hash_iter lsr_hash_find2( lsr_hash_t *pThis, const void *pKey, lsr_hash_key_t key )
{
    size_t index = lsr_hash_getindex( key, pThis->m_capacity );
    lsr_hashelem_t *pElem = pThis->m_table[index];
    while( pElem )
    {
        assert( pElem->m_pKey );
        if ( (*pThis->m_vc)( pKey, pElem->m_pKey ) != 0 )
            pElem = pElem->m_pNext;
        else
        {
            assert( !(( pElem->m_hkey != key )&&(pKey == pElem->m_pKey)) );
            break;
        }
    }
    return pElem;
}

static lsr_hash_iter lsr_hash_insert2( lsr_hash_t *pThis, const void *pKey, void *pValue, lsr_hash_key_t key )
{
    if ( pThis->m_size * pThis->m_full_factor > pThis->m_capacity )
        lsr_hash_rehash( pThis );
    lsr_hashelem_t *pNew;
    pNew = (lsr_hashelem_t *)lsr_hash_do_alloc( pThis->m_xpool, sizeof( lsr_hashelem_t ));
    if ( !pNew )
        return lsr_hash_end( pThis );
    pNew->m_pKey = pKey;
    pNew->m_pData = pValue;
    pNew->m_hkey = key;
    lsr_hashelem_t **pElem = pThis->m_table + lsr_hash_getindex( key, pThis->m_capacity );
    pNew->m_pNext = *pElem;
    *pElem = pNew;
    ++pThis->m_size;
    return pNew;
}

static lsr_hash_iter lsr_hash_insert_num( lsr_hash_t *pThis, const void *pKey, void *pValue )
{
    lsr_hash_iter iter = lsr_hash_find_num( pThis, pKey );
    if ( iter )
        return lsr_hash_end( pThis );
    return lsr_hash_insert2( pThis, pKey, pValue, (lsr_hash_key_t)pKey );
}

static lsr_hash_iter lsr_hash_update_num( lsr_hash_t *pThis, const void *pKey, void *pValue )
{
    lsr_hash_iter iter = lsr_hash_find_num( pThis, pKey );
    if ( iter != lsr_hash_end( pThis ) )
    {
        iter->m_pData = pValue;
        return iter;
    }
    return lsr_hash_insert2( pThis, pKey, pValue, (lsr_hash_key_t)pKey );
}



static lsr_hash_iter lsr_hash_insert_p( lsr_hash_t *pThis, const void *pKey, void *pValue )
{
    lsr_hash_key_t key = (*pThis->m_hf)( pKey );
    lsr_hash_iter iter = lsr_hash_find2( pThis, pKey, key );
    if ( iter )
        return lsr_hash_end( pThis );
    return lsr_hash_insert2( pThis, pKey, pValue, key );
}

static lsr_hash_iter lsr_hash_update_p( lsr_hash_t *pThis, const void *pKey, void *pValue )
{
    lsr_hash_key_t key = (*pThis->m_hf)( pKey );
    lsr_hash_iter iter = lsr_hash_find2( pThis, pKey, key );
    if ( iter )
    {
        iter->m_pKey = pKey;
        iter->m_pData = pValue;
        return iter;
    }
    return lsr_hash_insert2( pThis, pKey, pValue, key );
}

static lsr_hash_iter lsr_hash_find_p( lsr_hash_t *pThis, const void *pKey )
{
    return lsr_hash_find2( pThis, pKey, (*pThis->m_hf)(pKey ) );
}


