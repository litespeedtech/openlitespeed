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
#ifdef RUN_TEST
 
#include "loopbuftest.h"
#include <util/loopbuf.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

TEST(LoopBufTest_test)
{
    LoopBuf buf;
    CHECK( 0 == buf.size() );
    CHECK( 0 < buf.capacity() );
    CHECK( 0 == buf.reserve(0) );
    CHECK( 0 == buf.capacity() );
    CHECK( buf.end() == buf.begin() );

    CHECK( buf.reserve( 1024 ) == 0 );
    CHECK( 1024 <= buf.capacity() );
    CHECK( buf.guarantee( 1048 ) == 0);
    CHECK( 1048 <= buf.available() );
    buf.used( 10 );
    CHECK( buf.reserve( 15 ) == 0);
    CHECK( buf.size() == buf.end() - buf.begin() );
    CHECK( buf.available() == buf.capacity() - buf.size() - 1 );
    buf.clear();
    CHECK( 0 == buf.size() );
    const char * pStr = "Test String 123  343";
    int len = (int)strlen(pStr);

    CHECK( (int)strlen( pStr ) == buf.append( pStr, strlen( pStr ) ));
    CHECK( buf.size() == (int)strlen( pStr ) );
    CHECK( 0 == strncmp( pStr, buf.begin(), strlen( pStr )));
    char pBuf[128];
    CHECK( len == buf.moveTo(pBuf, 100 + len));
    CHECK( buf.empty());

    for ( int i = 0 ; i < 129 ; i ++ )
    {
        int i1 = buf.append(pStr, len);
        if ( i1 == 0 )
            CHECK( buf.full());
        else if ( i1 < len )
            CHECK( buf.full());
        if ( buf.full())
        {
            CHECK( 128 == buf.moveTo(pBuf, 128) );
            CHECK( 20 == buf.pop_front(20) );
            CHECK( 30 == buf.pop_back(30));
            CHECK( buf.size() == buf.capacity() - 128 - 20 -30 );
        }
    }
    for ( int i = 129 ; i < 1000 ; i ++ )
    {
        int i1 = buf.append(pStr, len);
        if ( i1 == 0 )
            CHECK( buf.full());
        else if ( i1 < len )
            CHECK( buf.full());
        if ( buf.full())
        {
            CHECK( 128 == buf.moveTo(pBuf, 128) );
            CHECK( 20 == buf.pop_front(20) );
            CHECK( 30 == buf.pop_back(30));
            CHECK( buf.size() == buf.capacity() - 128 - 20 -30 );
        }
    }
    LoopBuf lbuf(0);
    buf.swap( lbuf );
    buf.reserve(200);
    //printf( "n=%d\n", n );
    CHECK(200 <= buf.capacity());
    CHECK(buf.capacity() >= buf.size());
    for( int i = 0; i < buf.capacity() - 1; ++i )
    {
        CHECK( 1 == buf.append( pBuf, 1 ) );
    }
    CHECK( buf.full() );
    char * p0 = buf.end();
    CHECK(buf.inc(p0) == buf.begin());
    CHECK( buf.reserve(500) == 0 );
    CHECK( 500 <= buf.capacity());
    CHECK(buf.contiguous()==buf.available());
    buf.clear();
    CHECK(buf.guarantee(800) == 0);
    CHECK(buf.available() >= 800);
    
    //printf( "capacity=%d\n", buf.capacity());
    buf.used(500);
    CHECK(buf.pop_front(400)==400);
    buf.used(700);
    CHECK(buf.pop_back(100) ==100);
    CHECK( buf.begin() > buf.end() );
    CHECK(buf.contiguous() == buf.begin()-buf.end()-1);
    CHECK(buf.pop_back(700)==700);
    CHECK(buf.empty());
    int oldSize = buf.size();
    buf.used(60);
    CHECK( buf.size() == oldSize + 60);
    p0 = buf.begin();
    for ( int i = 0 ; i < buf.capacity(); i ++ )
    {
        CHECK( p0 == buf.getPointer(i));
        buf.inc(p0);
    }
    
}

TEST(LoopBufSearchTest)
{
    LoopBuf buf;
    const char *ptr, *ptr2, *pAccept = NULL;
    //char *printBuf = (char *)malloc(128);
    printf( "LoopBuf Search Test\n" );
    buf.append( "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa1"
            "23456789101112131415161718192021222324252627282930313233343536", 127 );
    buf.pop_front( 20 );
    buf.append( "37383940414243444546", 20 );
    pAccept = "2021222324";
    ptr = buf.search( 0, pAccept, 10 );
    ptr2 = buf.getPointer( 73 );
    CHECK( ptr == ptr2 );
    pAccept = "2333435363";
    ptr = buf.search( 0, pAccept, 10 );
    ptr2 = buf.getPointer( 98 );
    CHECK( ptr == ptr2 );
    pAccept = "6373839404";
    ptr = buf.search( 0, pAccept, 10 );
    ptr2 = buf.getPointer( 106 );
    CHECK( ptr == ptr2 );
    pAccept = "9404142434";
    ptr = buf.search( 0, pAccept, 10 );
    ptr2 = buf.getPointer( 112 );
    CHECK( ptr == ptr2 );
    pAccept = "233343536a";
    ptr = buf.search( 0, pAccept, 10 );
    CHECK( ptr == NULL );
    pAccept = "637383940a";
    ptr = buf.search( 0, pAccept, 10 );
    CHECK( ptr == NULL );
    
    buf.clear();
    buf.append( "afafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafaf"
            "afafafafafafafafafafafafafafafafafafafafafafafafafafafabkbkbkbk", 127);
    buf.pop_front( 20 );
    buf.append( "bkbkbkbafafafafafafa" , 20 );
    pAccept = "bkbkbkbkba";
    ptr = buf.search( 0, pAccept, 10 );
    ptr2 = buf.getPointer( 105 );
    CHECK( ptr == ptr2 );
}

#endif
