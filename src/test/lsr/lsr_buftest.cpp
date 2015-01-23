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
 
#include <lsr/lsr_buf.h>
#include <lsr/lsr_xpool.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

#include <stdio.h>

TEST( lsr_buftest_test )
{
    lsr_buf_t buf;
#ifdef LSR_BUF_DEBUG
    printf( "Start LSR Buf Test\n" );
#endif
    CHECK( 0 == lsr_buf( &buf, 0 ) );
    CHECK( 0 == lsr_buf_size( &buf ) );
    CHECK( 0 == lsr_buf_capacity( &buf ) );
    CHECK( 0 == lsr_buf_reserve( &buf, 0 ) );
    CHECK( 0 == lsr_buf_capacity( &buf ) );
    CHECK( lsr_buf_end( &buf ) == lsr_buf_begin( &buf ) );
    
    CHECK( 0 == lsr_buf_reserve( &buf, 1024 ) );
    CHECK( 1024 <= lsr_buf_capacity( &buf ) );
    CHECK( 1048 >= lsr_buf_available( &buf ) );
    lsr_buf_used( &buf, 10 );
    CHECK( lsr_buf_reserve( &buf, 15 ) == 0 );
    CHECK( lsr_buf_size( &buf ) == lsr_buf_end( &buf ) - lsr_buf_begin( &buf ) );
    CHECK( lsr_buf_available( &buf ) == lsr_buf_capacity( &buf ) - lsr_buf_size( &buf ) );
    lsr_buf_clear( &buf );
    CHECK( 0 == lsr_buf_size( &buf ) );

    const char * pStr = "Test String 123  343";
    int len = (int)strlen(pStr);
    CHECK( (int)strlen( pStr ) == lsr_buf_append2( &buf, pStr, strlen( pStr ) ) );
    CHECK( lsr_buf_size( &buf ) == (int)strlen( pStr ) );
    CHECK( 0 == strncmp( pStr, lsr_buf_begin( &buf ), strlen( pStr ) ) );
    char pBuf[128];
    CHECK( len == lsr_buf_pop_front_to( &buf, pBuf, 100 + len) );
    CHECK( lsr_buf_empty( &buf ) );
    
    for ( int i = 0 ; i < 129 ; i ++ )
    {
        int i1 = lsr_buf_append2( &buf, pStr, len );
        if ( i1 == 0 )
            CHECK( lsr_buf_full( &buf ) );
        else if ( i1 < len )
            CHECK( lsr_buf_full( &buf ) );
        if ( lsr_buf_full( &buf ) )
        {
            CHECK( 128 == lsr_buf_pop_front_to( &buf, pBuf, 128) );
            CHECK( 20 == lsr_buf_pop_front( &buf, 20 ) );
            CHECK( 30 == lsr_buf_pop_end( &buf, 30 ) );
            CHECK( lsr_buf_size( &buf ) == lsr_buf_capacity( &buf ) - 128 - 20 -30 );
        }
    }
    lsr_buf_t lbuf;
    lsr_buf( &lbuf, 0 );
    lsr_buf_swap( &buf, &lbuf );
    lsr_buf_reserve( &buf, 200 );
    CHECK( 200 <= lsr_buf_capacity( &buf ) );
    CHECK( lsr_buf_capacity( &buf ) >= lsr_buf_size( &buf ) );
    for( int i = 0; i < lsr_buf_capacity( &buf ); ++i )
    {
        CHECK( 1 == lsr_buf_append2( &buf, pBuf, 1 ) );
    }
    
    CHECK( lsr_buf_full( &buf ) );
    CHECK( lsr_buf_reserve( &buf, 500 ) == 0 );
    CHECK( 500 <= lsr_buf_capacity( &buf ) );
    lsr_buf_clear( &buf );
    CHECK( lsr_buf_reserve( &buf, 800 ) == 0 );
    CHECK( lsr_buf_available( &buf ) >= 800 );
    
    lsr_buf_used( &buf, 500 );
    CHECK( lsr_buf_pop_front( &buf, 400 )== 400 );
    lsr_buf_used( &buf, 700 );
    CHECK( lsr_buf_pop_end( &buf, 100 ) == 100 );
    CHECK( lsr_buf_pop_end( &buf, 700 ) == 700 );
    CHECK( lsr_buf_empty( &buf ) );
    int oldSize = lsr_buf_size( &buf );
    lsr_buf_used( &buf, 60 );
    CHECK( lsr_buf_size( &buf ) == oldSize + 60);
    
    lsr_buf_d( &buf );
    lsr_buf_d( &lbuf );
}

TEST( lsr_bufXtest_test )
{
    lsr_xpool_t pool;
    lsr_xpool_init( &pool );
    lsr_buf_t buf;
#ifdef LSR_BUF_DEBUG
    printf( "Start LSR Buf X Test\n" );
#endif
    CHECK( 0 == lsr_buf_x( &buf, 0, &pool ) );
    CHECK( 0 == lsr_buf_size( &buf ) );
    CHECK( 0 == lsr_buf_capacity( &buf ) );
    CHECK( 0 == lsr_buf_xreserve( &buf, 0, &pool ) );
    CHECK( 0 == lsr_buf_capacity( &buf ) );
    CHECK( lsr_buf_end( &buf ) == lsr_buf_begin( &buf ) );
    
    CHECK( 0 == lsr_buf_xreserve( &buf, 1024, &pool ) );
    CHECK( 1024 <= lsr_buf_capacity( &buf ) );
    CHECK( 1048 >= lsr_buf_available( &buf ) );
    lsr_buf_used( &buf, 10 );
    CHECK( lsr_buf_xreserve( &buf, 15, &pool ) == 0 );
    CHECK( lsr_buf_size( &buf ) == lsr_buf_end( &buf ) - lsr_buf_begin( &buf ) );
    CHECK( lsr_buf_available( &buf ) == lsr_buf_capacity( &buf ) - lsr_buf_size( &buf ) );
    lsr_buf_clear( &buf );
    CHECK( 0 == lsr_buf_size( &buf ) );

    const char * pStr = "Test String 123  343";
    int len = (int)strlen(pStr);
    CHECK( (int)strlen( pStr ) == lsr_buf_xappend2( &buf, pStr, strlen( pStr ), &pool ) );
    CHECK( lsr_buf_size( &buf ) == (int)strlen( pStr ) );
    CHECK( 0 == strncmp( pStr, lsr_buf_begin( &buf ), strlen( pStr ) ) );
    char pBuf[128];
    CHECK( len == lsr_buf_pop_front_to( &buf, pBuf, 100 + len) );
    CHECK( lsr_buf_empty( &buf ) );
    
    for ( int i = 0 ; i < 129 ; i ++ )
    {
        int i1 = lsr_buf_xappend2( &buf, pStr, len, &pool );
        if ( i1 == 0 )
            CHECK( lsr_buf_full( &buf ) );
        else if ( i1 < len )
            CHECK( lsr_buf_full( &buf ) );
        if ( lsr_buf_full( &buf ) )
        {
            CHECK( 128 == lsr_buf_pop_front_to( &buf, pBuf, 128) );
            CHECK( 20 == lsr_buf_pop_front( &buf, 20 ) );
            CHECK( 30 == lsr_buf_pop_end( &buf, 30 ) );
            CHECK( lsr_buf_size( &buf ) == lsr_buf_capacity( &buf ) - 128 - 20 -30 );
        }
    }
    lsr_buf_t lbuf;
    lsr_buf_x( &lbuf, 0, &pool );
    lsr_buf_swap( &buf, &lbuf );
    lsr_buf_xreserve( &buf, 200, &pool );
    CHECK( 200 <= lsr_buf_capacity( &buf ) );
    CHECK( lsr_buf_capacity( &buf ) >= lsr_buf_size( &buf ) );
    for( int i = 0; i < lsr_buf_capacity( &buf ); ++i )
    {
        CHECK( 1 == lsr_buf_xappend2( &buf, pBuf, 1, &pool ) );
    }
    
    CHECK( lsr_buf_full( &buf ) );
    CHECK( lsr_buf_xreserve( &buf, 500, &pool ) == 0 );
    CHECK( 500 <= lsr_buf_capacity( &buf ) );
    lsr_buf_clear( &buf );
    CHECK( lsr_buf_xreserve( &buf, 800, &pool ) == 0 );
    CHECK( lsr_buf_available( &buf ) >= 800 );
    
    lsr_buf_used( &buf, 500 );
    CHECK( lsr_buf_pop_front( &buf, 400 )== 400 );
    lsr_buf_used( &buf, 700 );
    CHECK( lsr_buf_pop_end( &buf, 100 ) == 100 );
    CHECK( lsr_buf_pop_end( &buf, 700 ) == 700 );
    CHECK( lsr_buf_empty( &buf ) );
    int oldSize = lsr_buf_size( &buf );
    lsr_buf_used( &buf, 60 );
    CHECK( lsr_buf_size( &buf ) == oldSize + 60);
    
    lsr_buf_xd( &buf, &pool );
    lsr_buf_xd( &lbuf, &pool );
    lsr_xpool_destroy( &pool );
}

TEST( lsr_xbuftest_test )
{
    lsr_xbuf_t buf;
    lsr_xpool_t pool;
#ifdef LSR_BUF_DEBUG
    printf( "Start LSR XBuf Test\n" );
#endif
    lsr_xpool_init( &pool );
    CHECK( 0 == lsr_xbuf( &buf, 0, &pool ) );
    CHECK( 0 == lsr_xbuf_size( &buf ) );
    CHECK( 0 == lsr_xbuf_capacity( &buf ) );
    CHECK( 0 == lsr_xbuf_reserve( &buf, 0 ) );
    CHECK( 0 == lsr_xbuf_capacity( &buf ) );
    CHECK( lsr_xbuf_end( &buf ) == lsr_xbuf_begin( &buf ) );
    
    CHECK( 0 == lsr_xbuf_reserve( &buf, 1024 ) );
    CHECK( 1024 <= lsr_xbuf_capacity( &buf ) );
    CHECK( 1048 >= lsr_xbuf_available( &buf ) );
    lsr_xbuf_used( &buf, 10 );
    CHECK( lsr_xbuf_reserve( &buf, 15 ) == 0 );
    CHECK( lsr_xbuf_size( &buf ) == lsr_xbuf_end( &buf ) - lsr_xbuf_begin( &buf ) );
    CHECK( lsr_xbuf_available( &buf ) == lsr_xbuf_capacity( &buf ) - lsr_xbuf_size( &buf ) );
    lsr_xbuf_clear( &buf );
    CHECK( 0 == lsr_xbuf_size( &buf ) );

    const char * pStr = "Test String 123  343";
    int len = (int)strlen(pStr);
    CHECK( (int)strlen( pStr ) == lsr_xbuf_append2( &buf, pStr, strlen( pStr ) ) );
    CHECK( lsr_xbuf_size( &buf ) == (int)strlen( pStr ) );
    CHECK( 0 == strncmp( pStr, lsr_xbuf_begin( &buf ), strlen( pStr ) ) );
    char pBuf[128];
    CHECK( len == lsr_xbuf_pop_front_to( &buf, pBuf, 100 + len) );
    CHECK( lsr_xbuf_empty( &buf ) );
    
    for ( int i = 0 ; i < 129 ; i ++ )
    {
        int i1 = lsr_xbuf_append2( &buf, pStr, len );
        if ( i1 == 0 )
            CHECK( lsr_xbuf_full( &buf ) );
        else if ( i1 < len )
            CHECK( lsr_xbuf_full( &buf ) );
        if ( lsr_xbuf_full( &buf ) )
        {
            CHECK( 128 == lsr_xbuf_pop_front_to( &buf, pBuf, 128) );
            CHECK( 20 == lsr_xbuf_pop_front( &buf, 20 ) );
            CHECK( 30 == lsr_xbuf_pop_end( &buf, 30 ) );
            CHECK( lsr_xbuf_size( &buf ) == lsr_xbuf_capacity( &buf ) - 128 - 20 -30 );
        }
    }
    lsr_xbuf_t lbuf;
    lsr_xbuf( &lbuf, 0, &pool );
    lsr_xbuf_swap( &buf, &lbuf );
    lsr_xbuf_reserve( &buf, 200 );
    CHECK( 200 <= lsr_xbuf_capacity( &buf ) );
    CHECK( lsr_xbuf_capacity( &buf ) >= lsr_xbuf_size( &buf ) );
    for( int i = 0; i < lsr_xbuf_capacity( &buf ); ++i )
    {
        CHECK( 1 == lsr_xbuf_append2( &buf, pBuf, 1 ) );
    }
    
    CHECK( lsr_xbuf_full( &buf ) );
    CHECK( lsr_xbuf_reserve( &buf, 500 ) == 0 );
    CHECK( 500 <= lsr_xbuf_capacity( &buf ) );
    lsr_xbuf_clear( &buf );
    CHECK( lsr_xbuf_reserve( &buf, 800 ) == 0 );
    CHECK( lsr_xbuf_available( &buf ) >= 800 );
    
    lsr_xbuf_used( &buf, 500 );
    CHECK( lsr_xbuf_pop_front( &buf, 400 )== 400 );
    lsr_xbuf_used( &buf, 700 );
    CHECK( lsr_xbuf_pop_end( &buf, 100 ) == 100 );
    CHECK( lsr_xbuf_pop_end( &buf, 700 ) == 700 );
    CHECK( lsr_xbuf_empty( &buf ) );
    int oldSize = lsr_xbuf_size( &buf );
    lsr_xbuf_used( &buf, 60 );
    CHECK( lsr_xbuf_size( &buf ) == oldSize + 60);
    
    lsr_xbuf_d( &buf );
    lsr_xbuf_d( &lbuf );
    lsr_xpool_destroy( &pool );
}

#endif
