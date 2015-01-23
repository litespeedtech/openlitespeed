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

//#define LSR_STR_DEBUG

#include <stdio.h>

#include <lsr/lsr_str.h>

#include <lsr/lsr_hash.h>
#include <lsr/lsr_pool.h>
#include <lsr/lsr_xpool.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

TEST( lsr_strtest_test )
{
#ifdef LSR_STR_DEBUG
    printf( "Start LSR Str Test\n" );
#endif
    
    lsr_str_t *pFive = lsr_str_new( NULL, 0 );
    CHECK( lsr_str_len( pFive ) == 0 );
    CHECK( lsr_str_set_str( pFive, "apple", 5 ) == 5 );
    CHECK( memcmp( lsr_str_c_str( pFive ), "apple", 5 ) == 0 );
    lsr_str_append( pFive, "grape", 5 );
    CHECK( memcmp( lsr_str_c_str( pFive ), "applegrape", 10 ) == 0 );
    lsr_str_set_len( pFive, 7 );
    CHECK( lsr_str_len( pFive ) == 7 );
    CHECK( memcmp( lsr_str_c_str( pFive ), "applegr", 7 ) == 0 );
    
    lsr_str_t *pSix = lsr_str_new( "applegr", 7 );
    lsr_str_t pSeven;
    lsr_str( &pSeven, "applegr", 7 );
    lsr_str_t pEight;
    CHECK( lsr_str_copy( &pEight, &pSeven ) != NULL );
    CHECK( memcmp( lsr_str_c_str( pFive ), lsr_str_c_str( pSix ), 7 ) == 0 );
    CHECK( memcmp( lsr_str_c_str( pSix ), lsr_str_c_str( &pSeven ), 7 ) == 0 );
    CHECK( memcmp( lsr_str_c_str( pSix ), lsr_str_c_str( &pEight ), 7 ) == 0 );


    lsr_str_delete( pFive );
    lsr_str_delete( pSix );
    lsr_str_d( &pSeven );
    lsr_str_d( &pEight );
}

TEST( lsr_strtest_pooltest )
{
#ifdef LSR_STR_DEBUG
    printf( "Start LSR Str XPool Test\n" );
#endif
    lsr_xpool_t pool;
    lsr_xpool_init( &pool );
    lsr_str_t *pFive = lsr_str_new_xp( NULL, 0, &pool );
    CHECK( lsr_str_len( pFive ) == 0 );
    CHECK( lsr_str_set_str_xp( pFive, "apple", 5, &pool ) == 5 );
    CHECK( memcmp( lsr_str_c_str( pFive ), "apple", 5 ) == 0 );
    lsr_str_append_xp( pFive, "grape", 5, &pool );
    CHECK( memcmp( lsr_str_c_str( pFive ), "applegrape", 10 ) == 0 );
    lsr_str_set_len( pFive, 7 );
    CHECK( lsr_str_len( pFive ) == 7 );
    CHECK( memcmp( lsr_str_c_str( pFive ), "applegr", 7 ) == 0 );
    
    lsr_str_t *pSix = lsr_str_new_xp( "applegr", 7, &pool );
    lsr_str_t pSeven;
    lsr_str_xp( &pSeven, "applegr", 7, &pool );
    lsr_str_t pEight;
    CHECK( lsr_str_copy_xp( &pEight, &pSeven, &pool ) != NULL );
    CHECK( memcmp( lsr_str_c_str( pFive ), lsr_str_c_str( pSix ), 7 ) == 0 );
    CHECK( memcmp( lsr_str_c_str( pSix ), lsr_str_c_str( &pSeven ), 7 ) == 0 );
    CHECK( memcmp( lsr_str_c_str( pSix ), lsr_str_c_str( &pEight ), 7 ) == 0 );


    lsr_str_delete_xp( pFive, &pool );
    lsr_str_delete_xp( pSix, &pool );
    lsr_str_d_xp( &pSeven, &pool );
    lsr_str_d_xp( &pEight, &pool );
    lsr_xpool_destroy( &pool );
}
/*
TEST( test_lsr_strhashtest )
{
    const char  *test1 = "I Want A Hash Function\0For This String.",
                *test2 = "Perhaps\\There?Is/Something*Missing{That}This$Will%Test.";
    int test1_len = 22,
        test2_len = 55;
    int reg_res, lsr_res;
    lsr_str_t str;
    printf( "Start LSR Str Hash Test\n" );
    
    lsr_str( &str, test1, test1_len );
    
    reg_res = lsr_hash_hash_string( lsr_str_buf( &str ) );
    lsr_res = lsr_str_hf( &str );
    printf( "Reg res: %d, Lsr res: %d\n", reg_res, lsr_res );
    CHECK( reg_res - lsr_res == 0 );
    
    reg_res = lsr_hash_cistring( lsr_str_buf( &str ) );
    lsr_res = lsr_str_ci_hf( &str );
    printf( "Reg res: %d, Lsr res: %d\n", reg_res, lsr_res );
    CHECK( reg_res - lsr_res == 0 );
    
    lsr_str( &str, test2, test2_len );
    
    reg_res = lsr_hash_hash_string( lsr_str_buf( &str ) );
    lsr_res = lsr_str_hf( &str );
    printf( "Reg res: %d, Lsr res: %d\n", reg_res, lsr_res );
    CHECK( reg_res - lsr_res == 0 );
    
    reg_res = lsr_hash_cistring( lsr_str_buf( &str ) );
    lsr_res = lsr_str_ci_hf( &str );
    printf( "Reg res: %d, Lsr res: %d\n", reg_res, lsr_res );
    CHECK( reg_res - lsr_res == 0 );
}
*/




#endif
