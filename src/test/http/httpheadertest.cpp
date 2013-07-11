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

#include "httpheadertest.h"
#include <http/httpheader.h>
#include <util/autobuf.h>

#include <stdlib.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"
#include <util/misc/profiletime.h>

static const char * const s_pHeaders[] =
{
    //most common headers
    "Accept",
    "Accept-charset",
    "Accept-EncodinG",
    "accept-language",
    "authorIzation",
    "connection",
    "coNtent-type",
    "content-Length",
    "cookiE",
    "coOkie2",
    "hoSt",
    "pRagma",
    "reFerer",
    "user-agEnt",
    "cache-control",
    "if-ModifIed-siNce",
    "if-mAtch",
    "if-none-match",
    "if-range",
    "if-unmoDified-since",
    "kEep-alIve",
    "rAnge",
    "x-Forwarded-For",
    "via",
    "transfer-encoding",

    // request-header
    "te",
    "expect",
    "max-forwards",
    "proxy-authorization",

    // general-header
    "date",
    "trailer",
    "upgrade",
    "warning",

    // entity-header
    "allow",
    "content-encoding",
    "content-language",
    "content-location",
    "content-md5",
    "content-range",
    "expires",
    "last-modified",

    // response-header
    "accept-ranges",
    "age",
    "etag",
    "location",
    "proxy-authenticate",
    "proxy-connection",
    "retry-after",
    "server",
    "vary",
    "www-authenticate",
    "status",

    //invalid headers
    "accepted",
    "age ",
    " location ",
    "\thost"

};

SUITE(HttpHeaderTest)
{

TEST(testLookup)
{
    int size = sizeof( s_pHeaders ) / sizeof( char * );
    int i;
    for( i = 0; i < size; i++ )
    {
        if ( i < HttpHeader::H_TE )
        {
            //printf( "%s\n", s_pHeaders[i] );
            int index = HttpHeader::getIndex2( s_pHeaders[i] );
            CHECK( i == index );
            CHECK( (int)strlen( s_pHeaders[i] ) ==
                        HttpHeader::getHeaderStringLen( i ) );
//            CHECK( 0 == strncasecmp( s_pHeaders[i],
//                                    HttpHeader::getHeader( i ),
//                                    strlen( s_pHeaders[i]) ) );
        }
        else
        {
//            CHECK( NULL == HttpHeader::getHeader( i ));
            CHECK( HttpHeader::H_HEADER_END ==
                    HttpHeader::getIndex2( s_pHeaders[i] ) );
        }
    }

    static const char * respHeaders[] =
    {
        "content-type",
        "content-length",
        "content-encoding",
        "cache-control",
        "location",
        "pragma",
        "status",
        "transfer-encoding",
        "proxy-connection",
        "server"
    };
    static int respHeaderIndex[] =
    {
        HttpHeader::H_CONTENT_TYPE,
        HttpHeader::H_CONTENT_LENGTH,
        HttpHeader::H_CONTENT_ENCODING,
        HttpHeader::H_CACHE_CTRL,
        HttpHeader::H_LOCATION,
        HttpHeader::H_PRAGMA,
        HttpHeader::CGI_STATUS,
        HttpHeader::H_TRANSFER_ENCODING,
        HttpHeader::H_PROXY_CONNECTION,
        HttpHeader::H_SERVER
    };
    size = sizeof( respHeaders ) / sizeof( char * );
    for( i = 0; i < size; i++ )
    {
        //printf( "%s\n", respHeaders[i] );
        int index = HttpHeader::getRespHeaderIndex( respHeaders[i] );
        CHECK( index == respHeaderIndex[i] );
        CHECK( (int)strlen( respHeaders[i] ) ==
                    HttpHeader::getHeaderStringLen( index ) );
    }    
}

TEST(testInstance)
{
//    HttpHeader header;
//    const char * pSample[] =
//    {
//        "host","localhost:2080",
//        "user-agent","Mozilla/5.0 (X11; U; Linux i686; en-US; rv:0.9.2.1) Gecko/20010901",
//        "accept","text/xml, application/xml, application/xhtml+xml, text/html;q=0.9, image/png, image/jpeg, image/gif;q=0.2, text/plain;q=0.8, text/css",
//        "accept-language","en-us",
//        "accept-encoding","gzip,deflate,compress,identity",
//        "accept-charset","ISO-8859-1, utf-8;q=0.66, *;q=0.66",
//        "keep-alive","300",
//        "connection","keep-alive"
//    };
//    const char * pVerfiy =
//    "host: localhost:2080\r\n"
//    "user-agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:0.9.2.1) Gecko/20010901\r\n"
//    "accept: text/xml, application/xml, application/xhtml+xml, text/html;q=0.9, image/png, image/jpeg, image/gif;q=0.2, text/plain;q=0.8, text/css\r\n"
//    "accept-language: en-us\r\n"
//    "accept-encoding: gzip,deflate,compress,identity\r\n"
//    "accept-charset: ISO-8859-1, utf-8;q=0.66, *;q=0.66\r\n"
//    "keep-alive: 300\r\n"
//    "connection: keep-alive\r\n";
//
//    HttpBuf httpBuf;
//    //test HttpHeader::append()
//    for( int i = 0; i < (int)(sizeof( pSample ) / sizeof( char *)); i+=2 )
//    {
//        CHECK( header.append( &httpBuf,
//                                pSample[i], pSample[i+1] ) == 0);
//    }
//    //httpBuf.append( "\0", 1 );
//    //printf( "%s\n", httpBuf.begin() );
//    CHECK( 0 == strncmp( pVerfiy, httpBuf.begin(), sizeof( pVerfiy ) ) );
//    for( int i = 0; i < (int)(sizeof( pSample ) / sizeof( char *)); i += 2 )
//    {
//        int index = HttpHeader::getIndex( pSample[i] );
//        CHECK( 0 ==
//            strncmp( header.getHeaderValue( &httpBuf, index ), pSample[i + 1],
//                strlen( pSample[i+1] ) ) );
//        CHECK( 0 ==
//            strncmp( header.getHeaderKey( &httpBuf, index ), pSample[i],
//                strlen( pSample[i] ) ) );
//        CHECK( 0 ==
//            strncmp( header.getHeaderValue( &httpBuf, pSample[i] ),
//                pSample[i + 1], strlen( pSample[i + 1] ) ) ) ;
//    }
//
//    // test HttpHeader::add()
//    HttpHeader header1;
//    for( int i = 0; i < (int)(sizeof( pSample ) / sizeof( char *)); i += 2 )
//    {
//        int index = HttpHeader::getIndex( pSample[i] );
//        const char * pValue = header.getHeaderValue( &httpBuf, index );
//        const char * pKey = header.getHeaderKey( &httpBuf, index );
//        header1.add( index, pKey - httpBuf.begin(),
//                pValue - httpBuf.begin(), strlen( pSample[i] ) );
//    }
//    for( int i = 0; i < (int)(sizeof( pSample ) / sizeof( char *)); i += 2 )
//    {
//        int index = HttpHeader::getIndex( pSample[i] );
//        CHECK( 0 ==
//            strncmp( header.getHeaderValue( &httpBuf, index ), pSample[i + 1],
//                strlen( pSample[i+1] ) ) );
//        CHECK( 0 ==
//            strncmp( header.getHeaderKey( &httpBuf, index ), pSample[i],
//                strlen( pSample[i] ) ) );
//        CHECK( 0 ==
//            strncmp( header.getHeaderValue( &httpBuf, pSample[i] ),
//                pSample[i + 1], strlen( pSample[i + 1] ) ) ) ;
//    }

}


TEST(benchmarkLookup)
{
    //ProfileTime prof( "HttpHeader lookup benchmark" );
    int size = sizeof( s_pHeaders ) / sizeof( char * );
    for( int i = 0; i < 1000000; i++ )
    {
        HttpHeader::getIndex( s_pHeaders[ rand() % size ] );
    }
}

}

#endif

