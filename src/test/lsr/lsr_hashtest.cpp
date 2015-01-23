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
#ifdef RUN_TEST

#include <stdio.h>

#include <lsr/lsr_hash.h>
#include <lsr/lsr_xpool.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"


static int plus_int( const void *pKey, void *pData, void *pUData )
{
    lsr_hash_t *hash = (lsr_hash_t *)pUData;
    lsr_hash_update( hash, pKey, (void *)((long)pData + (long)10) );
    return 0;
}

TEST( lsr_hashtest_test )
{
#ifdef LSR_HASH_DEBUG
    printf( "Start LSR Hash Test\n" );
#endif
    lsr_hash_t *hash = lsr_hash_new( 10, NULL, NULL, NULL );
    CHECK( lsr_hash_size( hash ) == 0 );
    CHECK( lsr_hash_capacity( hash ) == 13 );
    lsr_hash_iter iter = hash->m_insert( hash, (void *)0, (void *)0 );
    CHECK( iter != lsr_hash_end( hash ) );
    CHECK( (long)lsr_hash_get_key( iter ) == 0 );
    CHECK( (long)lsr_hash_get_data( iter ) == 0 );
    CHECK( lsr_hash_get_hkey( iter ) == 0 );
    CHECK( lsr_hash_get_next( iter ) == NULL );
    CHECK( lsr_hash_size( hash ) == 1 );
    
    lsr_hash_iter iter1 = hash->m_insert( hash, (void *)13, (void *)13 );
    CHECK( iter1 != lsr_hash_end( hash ) );
    CHECK( (long)lsr_hash_get_key( iter1 ) == 13 );
    CHECK( (long)lsr_hash_get_data( iter1 ) == 13 );
    CHECK( lsr_hash_get_hkey( iter1 ) == 13 );
    CHECK( lsr_hash_get_next( iter1 ) == iter );
    CHECK( lsr_hash_size( hash ) == 2 );
    
    lsr_hash_iter iter4 = hash->m_insert( hash, (void *)26, (void *)26 );
    CHECK( iter4 != lsr_hash_end( hash ) );
    CHECK( (long)lsr_hash_get_key( iter4 ) == 26 );
    CHECK( (long)lsr_hash_get_data( iter4 ) == 26 );
    CHECK( lsr_hash_get_hkey( iter4 ) == 26 );
    CHECK( lsr_hash_get_next( iter4 ) == iter1 );
    CHECK( lsr_hash_size( hash ) == 3 );
    
    lsr_hash_iter iter2 = hash->m_insert( hash, (void *)15, (void *)15 );
    CHECK( iter2 != lsr_hash_end( hash ) );
    CHECK( (long)lsr_hash_get_key( iter2 ) == 15 );
    CHECK( (long)lsr_hash_get_data( iter2 ) == 15 );
    CHECK( lsr_hash_get_hkey( iter2 ) == 15 );
    CHECK( lsr_hash_get_next( iter2 ) == NULL );
    CHECK( lsr_hash_size( hash ) == 4 );
    
    lsr_hash_iter iter3;
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == iter );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)15 );
    CHECK( iter3 == iter2 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == iter4 );
    iter3 = hash->m_find( hash, (void *)39 );
    CHECK( iter3 == NULL );
    iter3 = hash->m_find( hash, (void *)28 );
    CHECK( iter3 == NULL );
    
    iter3 = lsr_hash_begin( hash );
    CHECK( iter3 == iter4 );
    iter3 = lsr_hash_next( hash, iter3 );
    CHECK( iter3 == iter1 );
    iter3 = lsr_hash_next( hash, iter3 );
    CHECK( iter3 == iter );
    iter3 = lsr_hash_next( hash, iter3 );
    CHECK( iter3 == iter2 );
    iter3 = lsr_hash_next( hash, iter3 );
    CHECK( iter3 == lsr_hash_end( hash ) );
    
    int n = lsr_hash_for_each2( hash, lsr_hash_begin( hash ), lsr_hash_end( hash ), 
                                 plus_int, (void *)hash );
    
    CHECK( n == 4 );
    CHECK( (long)lsr_hash_get_data( iter ) == 10 );
    CHECK( (long)lsr_hash_get_data( iter1 ) == 23 );
    CHECK( (long)lsr_hash_get_data( iter2 ) == 25 );
    CHECK( (long)lsr_hash_get_data( iter4 ) == 36 );
    
    lsr_hash_erase( hash, iter );
    CHECK( lsr_hash_size( hash ) == 3 );
    lsr_hash_t hash2;
    lsr_hash( &hash2, 10, NULL, NULL, NULL );
    CHECK( lsr_hash_size( &hash2 ) == 0 );
    CHECK( lsr_hash_capacity( &hash2 ) == 13 );
    lsr_hash_swap( hash, &hash2 );
    CHECK( lsr_hash_size( hash ) == 0 );
    CHECK( lsr_hash_size( &hash2 ) == 3 );
    lsr_hash_swap( hash, &hash2 );
    CHECK( lsr_hash_size( hash ) == 3 );
    lsr_hash_d( &hash2 );
    
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    CHECK( lsr_hash_get_next( iter4 ) == iter1 );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == iter4 );
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == NULL );
    
    iter = hash->m_insert( hash, (void *)0, (void *)0 );
    CHECK( iter != lsr_hash_end( hash ) );
    CHECK( lsr_hash_size( hash ) == 4 );
    CHECK( lsr_hash_get_next( iter ) == iter4 );
    CHECK( lsr_hash_get_next( iter4 ) == iter1 );
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == iter4 );
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == iter );
    
    lsr_hash_erase( hash, iter );
    CHECK( lsr_hash_size( hash ) == 3 );
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    CHECK( lsr_hash_get_next( iter4 ) == iter1 );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == iter4 );
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == NULL );
    
    iter = hash->m_insert( hash, (void *)0, (void *)0 );
    CHECK( iter != lsr_hash_end( hash ) );
    CHECK( lsr_hash_size( hash ) == 4 );
    CHECK( lsr_hash_get_next( iter ) == iter4 );
    CHECK( lsr_hash_get_next( iter4 ) == iter1 );
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == iter4 );
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == iter );
    
    lsr_hash_erase( hash, iter4 );
    CHECK( lsr_hash_size( hash ) == 3 );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == NULL );
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == iter );
    CHECK( lsr_hash_get_next( iter ) == iter1 );
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    
    iter4 = hash->m_insert( hash, (void *)26, (void *)26 );
    CHECK( iter4 != lsr_hash_end( hash ) );
    CHECK( lsr_hash_size( hash ) == 4);
    CHECK( lsr_hash_get_next( iter4 ) == iter );
    CHECK( lsr_hash_get_next( iter ) == iter1 );
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    
    lsr_hash_iter iter5 = hash->m_insert( hash, (void *)5, (void *)5 );
    CHECK( lsr_hash_capacity( hash ) == 13 );
    lsr_hash_iter iter6 = hash->m_insert( hash, (void *)6, (void *)6 );
    CHECK( lsr_hash_capacity( hash ) == 13 );
    lsr_hash_iter iter7 = hash->m_insert( hash, (void *)7, (void *)7 );
    CHECK( lsr_hash_capacity( hash ) == 13 );
    lsr_hash_iter iter8 = hash->m_insert( hash, (void *)8, (void *)8 );
    CHECK( lsr_hash_capacity( hash ) == 53 );
    CHECK( lsr_hash_get_next( iter ) == NULL );
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    CHECK( lsr_hash_get_next( iter2 ) == NULL );
    CHECK( lsr_hash_get_next( iter3 ) == NULL );
    CHECK( lsr_hash_get_next( iter4 ) == NULL );
    CHECK( lsr_hash_get_next( iter5 ) == NULL );
    CHECK( lsr_hash_get_next( iter6 ) == NULL );
    CHECK( lsr_hash_get_next( iter7 ) == NULL );
    CHECK( lsr_hash_get_next( iter8 ) == NULL );
    
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == iter );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)15 );
    CHECK( iter3 == iter2 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == iter4 );
    iter3 = hash->m_find( hash, (void *)28 );
    CHECK( iter3 == NULL );
    
    lsr_hash_delete( hash );
}

TEST( lsr_hashpooltest_test )
{
#ifdef LSR_HASH_DEBUG
    printf( "Start LSR Hash XPool Test\n" );
#endif
    lsr_xpool_t pool;
    lsr_xpool_init( &pool );
    lsr_hash_t *hash = lsr_hash_new( 10, NULL, NULL, &pool );
    CHECK( lsr_hash_size( hash ) == 0 );
    CHECK( lsr_hash_capacity( hash ) == 13 );
    lsr_hash_iter iter = hash->m_insert( hash, (void *)0, (void *)0 );
    CHECK( iter != lsr_hash_end( hash ) );
    CHECK( (long)lsr_hash_get_key( iter ) == 0 );
    CHECK( (long)lsr_hash_get_data( iter ) == 0 );
    CHECK( lsr_hash_get_hkey( iter ) == 0 );
    CHECK( lsr_hash_get_next( iter ) == NULL );
    CHECK( lsr_hash_size( hash ) == 1 );
    
    lsr_hash_iter iter1 = hash->m_insert( hash, (void *)13, (void *)13 );
    CHECK( iter1 != lsr_hash_end( hash ) );
    CHECK( (long)lsr_hash_get_key( iter1 ) == 13 );
    CHECK( (long)lsr_hash_get_data( iter1 ) == 13 );
    CHECK( lsr_hash_get_hkey( iter1 ) == 13 );
    CHECK( lsr_hash_get_next( iter1 ) == iter );
    CHECK( lsr_hash_size( hash ) == 2 );
    
    lsr_hash_iter iter4 = hash->m_insert( hash, (void *)26, (void *)26 );
    CHECK( iter4 != lsr_hash_end( hash ) );
    CHECK( (long)lsr_hash_get_key( iter4 ) == 26 );
    CHECK( (long)lsr_hash_get_data( iter4 ) == 26 );
    CHECK( lsr_hash_get_hkey( iter4 ) == 26 );
    CHECK( lsr_hash_get_next( iter4 ) == iter1 );
    CHECK( lsr_hash_size( hash ) == 3 );
    
    lsr_hash_iter iter2 = hash->m_insert( hash, (void *)15, (void *)15 );
    CHECK( iter2 != lsr_hash_end( hash ) );
    CHECK( (long)lsr_hash_get_key( iter2 ) == 15 );
    CHECK( (long)lsr_hash_get_data( iter2 ) == 15 );
    CHECK( lsr_hash_get_hkey( iter2 ) == 15 );
    CHECK( lsr_hash_get_next( iter2 ) == NULL );
    CHECK( lsr_hash_size( hash ) == 4 );
    
    lsr_hash_iter iter3;
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == iter );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)15 );
    CHECK( iter3 == iter2 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == iter4 );
    iter3 = hash->m_find( hash, (void *)39 );
    CHECK( iter3 == NULL );
    iter3 = hash->m_find( hash, (void *)28 );
    CHECK( iter3 == NULL );
    
    iter3 = lsr_hash_begin( hash );
    CHECK( iter3 == iter4 );
    iter3 = lsr_hash_next( hash, iter3 );
    CHECK( iter3 == iter1 );
    iter3 = lsr_hash_next( hash, iter3 );
    CHECK( iter3 == iter );
    iter3 = lsr_hash_next( hash, iter3 );
    CHECK( iter3 == iter2 );
    iter3 = lsr_hash_next( hash, iter3 );
    CHECK( iter3 == lsr_hash_end( hash ) );
    
    int n = lsr_hash_for_each2( hash, lsr_hash_begin( hash ), lsr_hash_end( hash ), 
                                 plus_int, (void *)hash );
    
    CHECK( n == 4 );
    CHECK( (long)lsr_hash_get_data( iter ) == 10 );
    CHECK( (long)lsr_hash_get_data( iter1 ) == 23 );
    CHECK( (long)lsr_hash_get_data( iter2 ) == 25 );
    CHECK( (long)lsr_hash_get_data( iter4 ) == 36 );
    
    lsr_hash_erase( hash, iter );
    CHECK( lsr_hash_size( hash ) == 3 );
    lsr_hash_t hash2;
    lsr_hash( &hash2, 10, NULL, NULL, &pool );
    CHECK( lsr_hash_size( &hash2 ) == 0 );
    CHECK( lsr_hash_capacity( &hash2 ) == 13 );
    lsr_hash_swap( hash, &hash2 );
    CHECK( lsr_hash_size( hash ) == 0 );
    CHECK( lsr_hash_size( &hash2 ) == 3 );
    lsr_hash_swap( hash, &hash2 );
    CHECK( lsr_hash_size( hash ) == 3 );
    lsr_hash_d( &hash2 );
    
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    CHECK( lsr_hash_get_next( iter4 ) == iter1 );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == iter4 );
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == NULL );
    
    iter = hash->m_insert( hash, (void *)0, (void *)0 );
    CHECK( iter != lsr_hash_end( hash ) );
    CHECK( lsr_hash_size( hash ) == 4 );
    CHECK( lsr_hash_get_next( iter ) == iter4 );
    CHECK( lsr_hash_get_next( iter4 ) == iter1 );
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == iter4 );
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == iter );
    
    lsr_hash_erase( hash, iter );
    CHECK( lsr_hash_size( hash ) == 3 );
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    CHECK( lsr_hash_get_next( iter4 ) == iter1 );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == iter4 );
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == NULL );
    
    iter = hash->m_insert( hash, (void *)0, (void *)0 );
    CHECK( iter != lsr_hash_end( hash ) );
    CHECK( lsr_hash_size( hash ) == 4 );
    CHECK( lsr_hash_get_next( iter ) == iter4 );
    CHECK( lsr_hash_get_next( iter4 ) == iter1 );
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == iter4 );
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == iter );
    
    lsr_hash_erase( hash, iter4 );
    CHECK( lsr_hash_size( hash ) == 3 );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == NULL );
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == iter );
    CHECK( lsr_hash_get_next( iter ) == iter1 );
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    
    iter4 = hash->m_insert( hash, (void *)26, (void *)26 );
    CHECK( iter4 != lsr_hash_end( hash ) );
    CHECK( lsr_hash_size( hash ) == 4);
    CHECK( lsr_hash_get_next( iter4 ) == iter );
    CHECK( lsr_hash_get_next( iter ) == iter1 );
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    
    lsr_hash_iter iter5 = hash->m_insert( hash, (void *)5, (void *)5 );
    CHECK( lsr_hash_capacity( hash ) == 13 );
    lsr_hash_iter iter6 = hash->m_insert( hash, (void *)6, (void *)6 );
    CHECK( lsr_hash_capacity( hash ) == 13 );
    lsr_hash_iter iter7 = hash->m_insert( hash, (void *)7, (void *)7 );
    CHECK( lsr_hash_capacity( hash ) == 13 );
    lsr_hash_iter iter8 = hash->m_insert( hash, (void *)8, (void *)8 );
    CHECK( lsr_hash_capacity( hash ) == 53 );
    CHECK( lsr_hash_get_next( iter ) == NULL );
    CHECK( lsr_hash_get_next( iter1 ) == NULL );
    CHECK( lsr_hash_get_next( iter2 ) == NULL );
    CHECK( lsr_hash_get_next( iter3 ) == NULL );
    CHECK( lsr_hash_get_next( iter4 ) == NULL );
    CHECK( lsr_hash_get_next( iter5 ) == NULL );
    CHECK( lsr_hash_get_next( iter6 ) == NULL );
    CHECK( lsr_hash_get_next( iter7 ) == NULL );
    CHECK( lsr_hash_get_next( iter8 ) == NULL );
    
    iter3 = hash->m_find( hash, (void *)0 );
    CHECK( iter3 == iter );
    iter3 = hash->m_find( hash, (void *)13 );
    CHECK( iter3 == iter1 );
    iter3 = hash->m_find( hash, (void *)15 );
    CHECK( iter3 == iter2 );
    iter3 = hash->m_find( hash, (void *)26 );
    CHECK( iter3 == iter4 );
    iter3 = hash->m_find( hash, (void *)28 );
    CHECK( iter3 == NULL );
    
    lsr_hash_delete( hash );
    lsr_xpool_destroy( &pool );
}


#endif
