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
 
#include <lsr/lsr_loopbuf.h>
#include <lsr/lsr_xpool.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

#include <stdio.h>

TEST( lsr_loopbuftest_test )
{
    lsr_loopbuf_t *buf = lsr_loopbuf_new( 0 );
#ifdef LSR_LOOPBUF_DEBUG
    printf( "Start LSR LoopBuf Test\n" );
#endif
    CHECK( 0 == lsr_loopbuf_size( buf ) );
    CHECK( 0 < lsr_loopbuf_capacity( buf ) );
    CHECK( 0 == lsr_loopbuf_reserve( buf, 0 ) );
    CHECK( 0 == lsr_loopbuf_capacity( buf ) );
    CHECK( lsr_loopbuf_end( buf ) == lsr_loopbuf_begin( buf ) );
    
    CHECK( lsr_loopbuf_reserve( buf, 1024 ) == 0 );
    CHECK( 1024 <= lsr_loopbuf_capacity( buf ) );
    CHECK( lsr_loopbuf_guarantee( buf, 1048 ) == 0 );
    CHECK( 1048 <= lsr_loopbuf_available( buf ) );
    lsr_loopbuf_used( buf, 10 );
    CHECK( lsr_loopbuf_reserve( buf, 15 ) == 0 );
    CHECK( lsr_loopbuf_size( buf ) == lsr_loopbuf_end( buf ) - lsr_loopbuf_begin( buf ) );
    CHECK( lsr_loopbuf_available( buf ) == lsr_loopbuf_capacity( buf ) - lsr_loopbuf_size( buf ) - 1 );
    lsr_loopbuf_clear( buf );
    CHECK( 0 == lsr_loopbuf_size( buf ) );
    const char * pStr = "Test String 123  343";
    int len = (int)strlen(pStr);
    
    CHECK( (int)strlen( pStr ) == lsr_loopbuf_append( buf, pStr, strlen( pStr ) ) );
    CHECK( lsr_loopbuf_size( buf ) == (int)strlen( pStr ) );
    CHECK( 0 == strncmp( pStr, lsr_loopbuf_begin( buf ), strlen( pStr ) ) );
    char pBuf[128];
    CHECK( len == lsr_loopbuf_move_to( buf, pBuf, 100 + len) );
    CHECK( lsr_loopbuf_empty( buf ) );
    
    for ( int i = 0 ; i < 129 ; i ++ )
    {
        int i1 = lsr_loopbuf_append( buf, pStr, len );
        if ( i1 == 0 )
            CHECK( lsr_loopbuf_full( buf ) );
        else if ( i1 < len )
            CHECK( lsr_loopbuf_full( buf ) );
        if ( lsr_loopbuf_full( buf ) )
        {
            CHECK( 128 == lsr_loopbuf_move_to( buf, pBuf, 128) );
            CHECK( 20 == lsr_loopbuf_pop_front( buf, 20 ) );
            CHECK( 30 == lsr_loopbuf_pop_back( buf, 30 ) );
            CHECK( lsr_loopbuf_size( buf ) == lsr_loopbuf_capacity( buf ) - 128 - 20 -30 );
        }
    }
    for ( int i = 129 ; i < 1000 ; i ++ )
    {
        int i1 = lsr_loopbuf_append( buf, pStr, len );
        if ( i1 == 0 )
            CHECK( lsr_loopbuf_full( buf ) );
        else if ( i1 < len )
            CHECK( lsr_loopbuf_full( buf ) );
        if ( lsr_loopbuf_full( buf ) )
        {
            CHECK( 128 == lsr_loopbuf_move_to( buf, pBuf, 128 ) );
            CHECK( 20 == lsr_loopbuf_pop_front( buf, 20 ) );
            CHECK( 30 == lsr_loopbuf_pop_back( buf, 30 ) );
            CHECK( lsr_loopbuf_size( buf ) == lsr_loopbuf_capacity( buf ) - 128 - 20 -30 );
        }
    }
    lsr_loopbuf_t lbuf;
    lsr_loopbuf( &lbuf, 0 );
    lsr_loopbuf_swap( buf, &lbuf );
    lsr_loopbuf_reserve( buf, 200 );
    CHECK( 200 <= lsr_loopbuf_capacity( buf ) );
    CHECK( lsr_loopbuf_capacity( buf ) >= lsr_loopbuf_size( buf ) );
    for( int i = 0; i < lsr_loopbuf_capacity( buf ) - 1; ++i )
    {
        CHECK( 1 == lsr_loopbuf_append( buf, pBuf, 1 ) );
    }
    CHECK( lsr_loopbuf_full( buf ) );
    char * p0 = lsr_loopbuf_end( buf );
    CHECK( lsr_loopbuf_inc( buf, &p0 ) == lsr_loopbuf_begin( buf ) );
    CHECK( lsr_loopbuf_reserve( buf, 500 ) == 0 );
    CHECK( 500 <= lsr_loopbuf_capacity( buf ) );
    CHECK( lsr_loopbuf_contiguous( buf ) == lsr_loopbuf_available( buf ) );
    lsr_loopbuf_clear( buf );
    CHECK( lsr_loopbuf_guarantee( buf, 800 ) == 0 );
    CHECK( lsr_loopbuf_available( buf ) >= 800 );
    
    lsr_loopbuf_used( buf, 500 );
    CHECK( lsr_loopbuf_pop_front( buf, 400 )==400 );
    lsr_loopbuf_used( buf, 700 );
    CHECK( lsr_loopbuf_pop_back( buf, 100 ) ==100 );
    CHECK( lsr_loopbuf_begin( buf ) > lsr_loopbuf_end( buf ) );
    CHECK( lsr_loopbuf_contiguous( buf ) 
        == lsr_loopbuf_begin( buf ) - lsr_loopbuf_end( buf ) - 1 );
    lsr_loopbuf_straight( buf );
    CHECK(lsr_loopbuf_begin( buf ) < lsr_loopbuf_end( buf ) );
    CHECK( lsr_loopbuf_pop_front( buf, 400 )== 400 );
    lsr_loopbuf_used( buf, 400 );
    CHECK( lsr_loopbuf_pop_back( buf, 100 ) == 100 );
    CHECK( lsr_loopbuf_begin( buf ) > lsr_loopbuf_end( buf ) );
    CHECK( lsr_loopbuf_contiguous( buf ) 
        == lsr_loopbuf_begin( buf ) - lsr_loopbuf_end( buf ) - 1 );
    CHECK( lsr_loopbuf_pop_back( buf, 600 ) == 600 );
    CHECK( lsr_loopbuf_empty( buf ) );
    int oldSize = lsr_loopbuf_size( buf );
    lsr_loopbuf_used( buf, 60 );
    CHECK( lsr_loopbuf_size( buf ) == oldSize + 60);
    p0 = lsr_loopbuf_begin( buf );
    for ( int i = 0 ; i < lsr_loopbuf_capacity( buf ); ++i )
    {
        CHECK( p0 == lsr_loopbuf_get_pointer( buf, i ) );
        lsr_loopbuf_inc( buf, &p0 );
    }
    
    lsr_loopbuf_delete( buf );
    lsr_loopbuf_d( &lbuf );
}

TEST( lsr_loopbufSearchTest )
{
    lsr_loopbuf_t *buf;
    const char *ptr, *ptr2, *pAccept = NULL;
#ifdef LSR_LOOPBUF_DEBUG
    printf( "Start LSR LoopBuf Search Test" );
#endif
    buf = lsr_loopbuf_new( 0 );
    lsr_loopbuf_append( buf, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa1"
            "23456789101112131415161718192021222324252627282930313233343536", 127 );
    lsr_loopbuf_pop_front( buf, 20 );
    lsr_loopbuf_append( buf, "37383940414243444546", 20 );
    pAccept = "2021222324";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_loopbuf_get_pointer( buf, 73 );
    CHECK( ptr == ptr2 );
    pAccept = "2333435363";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_loopbuf_get_pointer( buf, 98 );
    CHECK( ptr == ptr2 );
    pAccept = "6373839404";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_loopbuf_get_pointer( buf, 106 );
    CHECK( ptr == ptr2 );
    pAccept = "9404142434";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_loopbuf_get_pointer( buf, 112 );
    CHECK( ptr == ptr2 );
    pAccept = "233343536a";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    CHECK( ptr == NULL );
    pAccept = "637383940a";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    CHECK( ptr == NULL );
    
    lsr_loopbuf_clear( buf );
    lsr_loopbuf_append( buf, "afafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafaf"
            "afafafafafafafafafafafafafafafafafafafafafafafafafafafabkbkbkbk", 127 );
    lsr_loopbuf_pop_front( buf, 20 );
    lsr_loopbuf_append( buf, "bkbkbkbafafafafafafa" , 20 );
    pAccept = "bkbkbkbkba";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_loopbuf_get_pointer( buf, 105 );
    CHECK( ptr == ptr2 );
    
    lsr_loopbuf_delete( buf );
}



TEST( lsr_loopbufxtest_test )
{
    lsr_xpool_t pool;
    lsr_xpool_init( &pool );
    lsr_loopbuf_t *buf = lsr_loopbuf_xnew( 0, &pool );
#ifdef LSR_LOOPBUF_DEBUG
    printf( "Start LSR LoopBuf X Test\n" );
#endif
    CHECK( 0 == lsr_loopbuf_size( buf ) );
    CHECK( 0 < lsr_loopbuf_capacity( buf ) );
    CHECK( 0 == lsr_loopbuf_xreserve( buf, 0, &pool ) );
    CHECK( 0 == lsr_loopbuf_capacity( buf ) );
    CHECK( lsr_loopbuf_end( buf ) == lsr_loopbuf_begin( buf ) );
    
    CHECK( lsr_loopbuf_xreserve( buf, 1024, &pool ) == 0 );
    CHECK( 1024 <= lsr_loopbuf_capacity( buf ) );
    CHECK( lsr_loopbuf_xguarantee( buf, 1048, &pool ) == 0 );
    CHECK( 1048 <= lsr_loopbuf_available( buf ) );
    lsr_loopbuf_used( buf, 10 );
    CHECK( lsr_loopbuf_xreserve( buf, 15, &pool ) == 0 );
    CHECK( lsr_loopbuf_size( buf ) == lsr_loopbuf_end( buf ) - lsr_loopbuf_begin( buf ) );
    CHECK( lsr_loopbuf_available( buf ) == lsr_loopbuf_capacity( buf ) - lsr_loopbuf_size( buf ) - 1 );
    lsr_loopbuf_clear( buf );
    CHECK( 0 == lsr_loopbuf_size( buf ) );
    const char * pStr = "Test String 123  343";
    int len = (int)strlen(pStr);
    
    CHECK( (int)strlen( pStr ) == lsr_loopbuf_xappend( buf, pStr, strlen( pStr ), &pool ) );
    CHECK( lsr_loopbuf_size( buf ) == (int)strlen( pStr ) );
    CHECK( 0 == strncmp( pStr, lsr_loopbuf_begin( buf ), strlen( pStr ) ) );
    char pBuf[128];
    CHECK( len == lsr_loopbuf_move_to( buf, pBuf, 100 + len) );
    CHECK( lsr_loopbuf_empty( buf ) );
    
    for ( int i = 0 ; i < 129 ; i ++ )
    {
        int i1 = lsr_loopbuf_xappend( buf, pStr, len, &pool );
        if ( i1 == 0 )
            CHECK( lsr_loopbuf_full( buf ) );
        else if ( i1 < len )
            CHECK( lsr_loopbuf_full( buf ) );
        if ( lsr_loopbuf_full( buf ) )
        {
            CHECK( 128 == lsr_loopbuf_move_to( buf, pBuf, 128) );
            CHECK( 20 == lsr_loopbuf_pop_front( buf, 20 ) );
            CHECK( 30 == lsr_loopbuf_pop_back( buf, 30 ) );
            CHECK( lsr_loopbuf_size( buf ) == lsr_loopbuf_capacity( buf ) - 128 - 20 -30 );
        }
    }
    for ( int i = 129 ; i < 1000 ; i ++ )
    {
        int i1 = lsr_loopbuf_xappend( buf, pStr, len, &pool );
        if ( i1 == 0 )
            CHECK( lsr_loopbuf_full( buf ) );
        else if ( i1 < len )
            CHECK( lsr_loopbuf_full( buf ) );
        if ( lsr_loopbuf_full( buf ) )
        {
            CHECK( 128 == lsr_loopbuf_move_to( buf, pBuf, 128 ) );
            CHECK( 20 == lsr_loopbuf_pop_front( buf, 20 ) );
            CHECK( 30 == lsr_loopbuf_pop_back( buf, 30 ) );
            CHECK( lsr_loopbuf_size( buf ) == lsr_loopbuf_capacity( buf ) - 128 - 20 -30 );
        }
    }
    lsr_loopbuf_t lbuf;
    lsr_loopbuf_x( &lbuf, 0, &pool );
    lsr_loopbuf_swap( buf, &lbuf );
    lsr_loopbuf_xreserve( buf, 200, &pool );
    CHECK( 200 <= lsr_loopbuf_capacity( buf ) );
    CHECK( lsr_loopbuf_capacity( buf ) >= lsr_loopbuf_size( buf ) );
    for( int i = 0; i < lsr_loopbuf_capacity( buf ) - 1; ++i )
    {
        CHECK( 1 == lsr_loopbuf_xappend( buf, pBuf, 1, &pool ) );
    }
    CHECK( lsr_loopbuf_full( buf ) );
    char * p0 = lsr_loopbuf_end( buf );
    CHECK( lsr_loopbuf_inc( buf, &p0 ) == lsr_loopbuf_begin( buf ) );
    CHECK( lsr_loopbuf_xreserve( buf, 500, &pool ) == 0 );
    CHECK( 500 <= lsr_loopbuf_capacity( buf ) );
    CHECK( lsr_loopbuf_contiguous( buf ) == lsr_loopbuf_available( buf ) );
    lsr_loopbuf_clear( buf );
    CHECK( lsr_loopbuf_xguarantee( buf, 800, &pool ) == 0 );
    CHECK( lsr_loopbuf_available( buf ) >= 800 );
    
    lsr_loopbuf_used( buf, 500 );
    CHECK( lsr_loopbuf_pop_front( buf, 400 )== 400 );
    lsr_loopbuf_used( buf, 700 );
    CHECK( lsr_loopbuf_pop_back( buf, 100 ) == 100 );
    CHECK( lsr_loopbuf_begin( buf ) > lsr_loopbuf_end( buf ) );
    CHECK( lsr_loopbuf_contiguous( buf ) 
        == lsr_loopbuf_begin( buf ) - lsr_loopbuf_end( buf ) - 1 );
    lsr_loopbuf_xstraight( buf, &pool );
    CHECK(lsr_loopbuf_begin( buf ) < lsr_loopbuf_end( buf ) );
    CHECK( lsr_loopbuf_pop_front( buf, 400 )== 400 );
    lsr_loopbuf_used( buf, 400 );
    CHECK( lsr_loopbuf_pop_back( buf, 100 ) == 100 );
    CHECK( lsr_loopbuf_begin( buf ) > lsr_loopbuf_end( buf ) );
    CHECK( lsr_loopbuf_contiguous( buf ) 
        == lsr_loopbuf_begin( buf ) - lsr_loopbuf_end( buf ) - 1 );
    CHECK( lsr_loopbuf_pop_back( buf, 600 ) == 600 );
    CHECK( lsr_loopbuf_empty( buf ) );
    int oldSize = lsr_loopbuf_size( buf );
    lsr_loopbuf_used( buf, 60 );
    CHECK( lsr_loopbuf_size( buf ) == oldSize + 60);
    p0 = lsr_loopbuf_begin( buf );
    for ( int i = 0 ; i < lsr_loopbuf_capacity( buf ); ++i )
    {
        CHECK( p0 == lsr_loopbuf_get_pointer( buf, i ) );
        lsr_loopbuf_inc( buf, &p0 );
    }
    
    lsr_loopbuf_xdelete( buf, &pool );
    lsr_loopbuf_xd( &lbuf, &pool );
    lsr_xpool_destroy( &pool );
}

TEST( lsr_loopbufXSearchTest )
{
    lsr_xpool_t pool;
    lsr_xpool_init( &pool );
    lsr_loopbuf_t *buf;
    const char *ptr, *ptr2, *pAccept = NULL;
#ifdef LSR_LOOPBUF_DEBUG
    printf( "Start LSR LoopBuf Search Test" );
#endif
    buf = lsr_loopbuf_xnew( 0, &pool );
    lsr_loopbuf_xappend( buf, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa1"
            "23456789101112131415161718192021222324252627282930313233343536", 127, &pool );
    lsr_loopbuf_pop_front( buf, 20 );
    lsr_loopbuf_xappend( buf, "37383940414243444546", 20, &pool );
    pAccept = "2021222324";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_loopbuf_get_pointer( buf, 73 );
    CHECK( ptr == ptr2 );
    pAccept = "2333435363";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_loopbuf_get_pointer( buf, 98 );
    CHECK( ptr == ptr2 );
    pAccept = "6373839404";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_loopbuf_get_pointer( buf, 106 );
    CHECK( ptr == ptr2 );
    pAccept = "9404142434";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_loopbuf_get_pointer( buf, 112 );
    CHECK( ptr == ptr2 );
    pAccept = "233343536a";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    CHECK( ptr == NULL );
    pAccept = "637383940a";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    CHECK( ptr == NULL );
    
    lsr_loopbuf_clear( buf );
    lsr_loopbuf_xappend( buf, "afafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafaf"
            "afafafafafafafafafafafafafafafafafafafafafafafafafafafabkbkbkbk", 127, &pool );
    lsr_loopbuf_pop_front( buf, 20 );
    lsr_loopbuf_xappend( buf, "bkbkbkbafafafafafafa" , 20, &pool );
    pAccept = "bkbkbkbkba";
    ptr = lsr_loopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_loopbuf_get_pointer( buf, 105 );
    CHECK( ptr == ptr2 );
    
    lsr_loopbuf_xdelete( buf, &pool );
    lsr_xpool_destroy( &pool );
}



TEST( lsr_xloopbuftest_test )
{
    lsr_xpool_t pool;
    lsr_xpool_init( &pool );
    lsr_xloopbuf_t *buf = lsr_xloopbuf_new( 0, &pool );
#ifdef LSR_LOOPBUF_DEBUG
    printf( "Start LSR XLoopBuf Test\n" );
#endif
    CHECK( 0 == lsr_xloopbuf_size( buf ) );
    CHECK( 0 < lsr_xloopbuf_capacity( buf ) );
    CHECK( 0 == lsr_xloopbuf_reserve( buf, 0 ) );
    CHECK( 0 == lsr_xloopbuf_capacity( buf ) );
    CHECK( lsr_xloopbuf_end( buf ) == lsr_xloopbuf_begin( buf ) );
    
    CHECK( lsr_xloopbuf_reserve( buf, 1024 ) == 0 );
    CHECK( 1024 <= lsr_xloopbuf_capacity( buf ) );
    CHECK( lsr_xloopbuf_guarantee( buf, 1048 ) == 0 );
    CHECK( 1048 <= lsr_xloopbuf_available( buf ) );
    lsr_xloopbuf_used( buf, 10 );
    CHECK( lsr_xloopbuf_reserve( buf, 15 ) == 0 );
    CHECK( lsr_xloopbuf_size( buf ) == lsr_xloopbuf_end( buf ) - lsr_xloopbuf_begin( buf ) );
    CHECK( lsr_xloopbuf_available( buf ) == lsr_xloopbuf_capacity( buf ) - lsr_xloopbuf_size( buf ) - 1 );
    lsr_xloopbuf_clear( buf );
    CHECK( 0 == lsr_xloopbuf_size( buf ) );
    const char * pStr = "Test String 123  343";
    int len = (int)strlen(pStr);
    
    CHECK( (int)strlen( pStr ) == lsr_xloopbuf_append( buf, pStr, strlen( pStr ) ) );
    CHECK( lsr_xloopbuf_size( buf ) == (int)strlen( pStr ) );
    CHECK( 0 == strncmp( pStr, lsr_xloopbuf_begin( buf ), strlen( pStr ) ) );
    char pBuf[128];
    CHECK( len == lsr_xloopbuf_move_to( buf, pBuf, 100 + len) );
    CHECK( lsr_xloopbuf_empty( buf ) );
    
    for ( int i = 0 ; i < 129 ; i ++ )
    {
        int i1 = lsr_xloopbuf_append( buf, pStr, len );
        if ( i1 == 0 )
            CHECK( lsr_xloopbuf_full( buf ) );
        else if ( i1 < len )
            CHECK( lsr_xloopbuf_full( buf ) );
        if ( lsr_xloopbuf_full( buf ) )
        {
            CHECK( 128 == lsr_xloopbuf_move_to( buf, pBuf, 128) );
            CHECK( 20 == lsr_xloopbuf_pop_front( buf, 20 ) );
            CHECK( 30 == lsr_xloopbuf_pop_back( buf, 30 ) );
            CHECK( lsr_xloopbuf_size( buf ) == lsr_xloopbuf_capacity( buf ) - 128 - 20 -30 );
        }
    }
    for ( int i = 129 ; i < 1000 ; i ++ )
    {
        int i1 = lsr_xloopbuf_append( buf, pStr, len );
        if ( i1 == 0 )
            CHECK( lsr_xloopbuf_full( buf ) );
        else if ( i1 < len )
            CHECK( lsr_xloopbuf_full( buf ) );
        if ( lsr_xloopbuf_full( buf ) )
        {
            CHECK( 128 == lsr_xloopbuf_move_to( buf, pBuf, 128 ) );
            CHECK( 20 == lsr_xloopbuf_pop_front( buf, 20 ) );
            CHECK( 30 == lsr_xloopbuf_pop_back( buf, 30 ) );
            CHECK( lsr_xloopbuf_size( buf ) == lsr_xloopbuf_capacity( buf ) - 128 - 20 -30 );
        }
    }
    lsr_xloopbuf_t lbuf;
    lsr_xloopbuf( &lbuf, 0, &pool );
    lsr_xloopbuf_swap( buf, &lbuf );
    lsr_xloopbuf_reserve( buf, 200 );
    CHECK( 200 <= lsr_xloopbuf_capacity( buf ) );
    CHECK( lsr_xloopbuf_capacity( buf ) >= lsr_xloopbuf_size( buf ) );
    for( int i = 0; i < lsr_xloopbuf_capacity( buf ) - 1; ++i )
    {
        CHECK( 1 == lsr_xloopbuf_append( buf, pBuf, 1 ) );
    }
    CHECK( lsr_xloopbuf_full( buf ) );
    char * p0 = lsr_xloopbuf_end( buf );
    CHECK( lsr_xloopbuf_inc( buf, &p0 ) == lsr_xloopbuf_begin( buf ) );
    CHECK( lsr_xloopbuf_reserve( buf, 500 ) == 0 );
    CHECK( 500 <= lsr_xloopbuf_capacity( buf ) );
    CHECK( lsr_xloopbuf_contiguous( buf ) == lsr_xloopbuf_available( buf ) );
    lsr_xloopbuf_clear( buf );
    CHECK( lsr_xloopbuf_guarantee( buf, 800 ) == 0 );
    CHECK( lsr_xloopbuf_available( buf ) >= 800 );
    
    lsr_xloopbuf_used( buf, 500 );
    CHECK( lsr_xloopbuf_pop_front( buf, 400 )==400 );
    lsr_xloopbuf_used( buf, 700 );
    CHECK( lsr_xloopbuf_pop_back( buf, 100 ) ==100 );
    CHECK( lsr_xloopbuf_begin( buf ) > lsr_xloopbuf_end( buf ) );
    CHECK( lsr_xloopbuf_contiguous( buf ) 
        == lsr_xloopbuf_begin( buf ) - lsr_xloopbuf_end( buf ) - 1 );
    lsr_xloopbuf_straight( buf );
    CHECK(lsr_xloopbuf_begin( buf ) < lsr_xloopbuf_end( buf ) );
    CHECK( lsr_xloopbuf_pop_front( buf, 400 )== 400 );
    lsr_xloopbuf_used( buf, 400 );
    CHECK( lsr_xloopbuf_pop_back( buf, 100 ) == 100 );
    CHECK( lsr_xloopbuf_begin( buf ) > lsr_xloopbuf_end( buf ) );
    CHECK( lsr_xloopbuf_contiguous( buf ) 
        == lsr_xloopbuf_begin( buf ) - lsr_xloopbuf_end( buf ) - 1 );
    CHECK( lsr_xloopbuf_pop_back( buf, 600 ) == 600 );
    CHECK( lsr_xloopbuf_empty( buf ) );
    int oldSize = lsr_xloopbuf_size( buf );
    lsr_xloopbuf_used( buf, 60 );
    CHECK( lsr_xloopbuf_size( buf ) == oldSize + 60);
    p0 = lsr_xloopbuf_begin( buf );
    for ( int i = 0 ; i < lsr_xloopbuf_capacity( buf ); ++i )
    {
        CHECK( p0 == lsr_xloopbuf_get_pointer( buf, i ) );
        lsr_xloopbuf_inc( buf, &p0 );
    }
    
    lsr_xloopbuf_delete( buf );
    lsr_xloopbuf_d( &lbuf );
    lsr_xpool_destroy( &pool );
}

TEST( lsr_xloopbufSearchTest )
{
    lsr_xpool_t pool;
    lsr_xpool_init( &pool );
    lsr_xloopbuf_t *buf;
    const char *ptr, *ptr2, *pAccept = NULL;
#ifdef LSR_LOOPBUF_DEBUG
    printf( "Start LSR XLoopBufSearch Test" );
#endif
    buf = lsr_xloopbuf_new( 0, &pool );
    lsr_xloopbuf_append( buf, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa1"
            "23456789101112131415161718192021222324252627282930313233343536", 127 );
    lsr_xloopbuf_pop_front( buf, 20 );
    lsr_xloopbuf_append( buf, "37383940414243444546", 20 );
    pAccept = "2021222324";
    ptr = lsr_xloopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_xloopbuf_get_pointer( buf, 73 );
    CHECK( ptr == ptr2 );
    pAccept = "2333435363";
    ptr = lsr_xloopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_xloopbuf_get_pointer( buf, 98 );
    CHECK( ptr == ptr2 );
    pAccept = "6373839404";
    ptr = lsr_xloopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_xloopbuf_get_pointer( buf, 106 );
    CHECK( ptr == ptr2 );
    pAccept = "9404142434";
    ptr = lsr_xloopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_xloopbuf_get_pointer( buf, 112 );
    CHECK( ptr == ptr2 );
    pAccept = "233343536a";
    ptr = lsr_xloopbuf_search( buf, 0, pAccept, 10 );
    CHECK( ptr == NULL );
    pAccept = "637383940a";
    ptr = lsr_xloopbuf_search( buf, 0, pAccept, 10 );
    CHECK( ptr == NULL );
    
    lsr_xloopbuf_clear( buf );
    lsr_xloopbuf_append( buf, "afafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafaf"
            "afafafafafafafafafafafafafafafafafafafafafafafafafafafabkbkbkbk", 127 );
    lsr_xloopbuf_pop_front( buf, 20 );
    lsr_xloopbuf_append( buf, "bkbkbkbafafafafafafa" , 20 );
    pAccept = "bkbkbkbkba";
    ptr = lsr_xloopbuf_search( buf, 0, pAccept, 10 );
    ptr2 = lsr_xloopbuf_get_pointer( buf, 105 );
    CHECK( ptr == ptr2 );
    
    lsr_xloopbuf_delete( buf );
    lsr_xpool_destroy( &pool );
}


#endif








