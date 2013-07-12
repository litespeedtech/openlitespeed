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
#include <stdio.h>
#include "httpheadertest.h"
#include <http/httpheader.h>
#include <http/httprespheaders.h>
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
        HttpRespHeaders::H_CONTENT_TYPE,
        HttpRespHeaders::H_CONTENT_LENGTH,
        HttpRespHeaders::H_CONTENT_ENCODING,
        HttpRespHeaders::H_CACHE_CTRL,
        HttpRespHeaders::H_LOCATION,
        HttpRespHeaders::H_PRAGMA,
        HttpRespHeaders::CGI_STATUS,
        HttpRespHeaders::H_TRANSFER_ENCODING,
        HttpRespHeaders::H_PROXY_CONNECTION,
        HttpRespHeaders::H_SERVER
    };
    size = sizeof( respHeaders ) / sizeof( char * );
    for( i = 0; i < size; i++ )
    {
        //printf( "%s\n", respHeaders[i] );
        HttpRespHeaders::HEADERINDEX index = HttpRespHeaders::getRespHeaderIndex( respHeaders[i] );
        CHECK( index == respHeaderIndex[i] );
        CHECK( (int)strlen( respHeaders[i] ) ==
                    HttpRespHeaders::getHeaderStringLen( index ) );
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

#include <socket/gsockaddr.h>
void DisplayHeader(IOVec io, int format, short count)
{
    IOVec::iterator it;
    unsigned char *p = NULL;
    for (it = io.begin(); it != io.end(); ++it)
    {
        p = (unsigned char *)it->iov_base;
        for (unsigned int i=0; i<it->iov_len; ++i)
        {
            if (format  != 0)
                printf("%02X ",p[i]);
            else
                printf("%c",p[i]);
        }
    }
    printf("\r\nCount = %d\r\n======================================================\r\n", count);
    
    if (format != 0)
    {
        AutoBuf buf;
        for (it = io.begin(); it != io.end(); ++it)
        {
            buf.append( (const char *)it->iov_base, it->iov_len);
        }
        
        if (format ==  1)
        {
            char *p = buf.begin();
            short *tempNumS;
            short tempNum;
            
            for (int j=0; j<count; ++j)
            {
                tempNumS = (short *)p;
                tempNum = ntohs(*tempNumS);
                p += 2;
                for (int n=0; n<tempNum; ++n){
                    printf("%c", *(p + n));
                }
                printf(": ");
                p += tempNum;
                
                tempNumS = (short *)p;
                tempNum = ntohs(*tempNumS);
                p += 2;
                for (int n=0; n<tempNum; ++n){
                    if (*(p + n) != 0)
                        printf("%c", *(p + n));
                    else
                        printf("\r\n\t");
                }
                printf("\r\n");
                p += tempNum;
            }
        }
        else
        {
            char *p = buf.begin();
            int32_t *tempNumS;
            int32_t tempNum;
            
            for (int j=0; j<count; ++j)
            {
                tempNumS = (int32_t *)p;
                tempNum = ntohl(*tempNumS);
                p += 4;
                for (int n=0; n<tempNum; ++n){
                    printf("%c", *(p + n));
                }
                printf(": ");
                p += tempNum;
                
                tempNumS = (int32_t *)p;
                tempNum = ntohl(*tempNumS);
                p += 4;
                for (int n=0; n<tempNum; ++n){
                    if (*(p + n) != 0)
                        printf("%c", *(p + n));
                    else
                        printf("\r\n\t");
                }
                printf("\r\n");
                p += tempNum;
            }
        }
        
        printf("\r\n*******************************************************************\r\n\r\n");
        
    }
    
}
    

TEST (respHeaders)
{
    HttpRespHeaders h;
    IOVec io;
    char *pVal = NULL;
    int valLen = 0;
        
    for (int kk=0; kk<3; ++kk)
    {
        h.reset((RespHeader::FORMAT)kk);
        h.add(HttpRespHeaders::H_SERVER, "Server", 6, "My_Server", 9);
        h.add(HttpRespHeaders::H_ACCEPT_RANGES, "Accept-Ranges", strlen("Accept-Ranges"), "bytes", 5);
        h.add(HttpRespHeaders::H_DATE, "Date", 4, "Thu, 16 May 2013 20:32:23 GMT", strlen("Thu, 16 May 2013 20:32:23 GMT"));
        h.add(HttpRespHeaders::H_X_POWERED_BY, "X-Powered-By", strlen("X-Powered-By"), "PHP/5.3.24", strlen("PHP/5.3.24"));
        io.clear();     h.getHeaders(&io);     DisplayHeader(io, kk, h.getCount());
        CHECK(h.getCount() == 4);
            
   
        h.reset((RespHeader::FORMAT)kk);
        h.add(HttpRespHeaders::H_SERVER, "Server", 6, "My_Server", 9);
        h.add(HttpRespHeaders::H_ACCEPT_RANGES, "Accept-Ranges", strlen("Accept-Ranges"), "bytes", 5);
        h.add(HttpRespHeaders::H_DATE, "Date", 4, "Thu, 16 May 2013 20:32:23 GMT", strlen("Thu, 16 May 2013 20:32:23 GMT"));
        h.add(HttpRespHeaders::H_X_POWERED_BY, "X-Powered-By", strlen("X-Powered-By"), "PHP/5.3.24", strlen("PHP/5.3.24"));
        h.del(HttpRespHeaders::H_DATE);
        io.clear();     h.getHeaders(&io);     DisplayHeader(io, kk, h.getCount());
        CHECK(h.getCount() == 3);
    
    
        h.del("X-Powered-By", strlen("X-Powered-By"));
        io.clear();     h.getHeaders(&io);     DisplayHeader(io, kk, h.getCount());
        CHECK(h.getCount() == 2);
    
        h.add(HttpRespHeaders::H_SERVER, "Server", 6, "YY_Server", 9);
        io.clear();     h.getHeaders(&io);     DisplayHeader(io, kk, h.getCount());
        CHECK(h.getCount() == 2);

    
        h.add(HttpRespHeaders::H_SERVER, "Server", 6, "XServer", 7);
        io.clear();     h.getHeaders(&io);     DisplayHeader(io, kk, h.getCount());
        CHECK(h.getCount() == 2);
        
        h.add(HttpRespHeaders::H_DATE, "Date", 4, "Thu, 16 May 2099 20:32:23 GMT", strlen("Thu, 16 May 2013 20:32:23 GMT"));
        h.add(HttpRespHeaders::H_X_POWERED_BY, "X-Powered-By", strlen("X-Powered-By"), "PHP/9.9.99", strlen("PHP/5.3.24"));
        io.clear();     h.getHeaders(&io);     DisplayHeader(io, kk, h.getCount());
        CHECK(h.getCount() == 4);

        
        h.add(HttpRespHeaders::H_UNKNOWN, "Allow", 5, "*.*", 3);
        h.appendLastVal("Allow", 5, "; .zip; .rar", strlen("; .zip; .rar"));
        h.appendLastVal("Allow", 5, "; .exe; .flv", strlen("; .zip; .rar"));
        io.clear();     h.getHeaders(&io);     DisplayHeader(io, kk, h.getCount());
        CHECK(h.getCount() == 5);  
    
        
        h.add(HttpRespHeaders::H_SET_COOKIE, "Set-Cookie", strlen("Set-Cookie"),
              "lsws_uid=a; expires=Mon, 13 May 2013 14:10:51 GMT; path=/",
              strlen("lsws_uid=a; expires=Mon, 13 May 2013 14:10:51 GMT; path=/"),
              RespHeader::APPEND);
        
        h.add(HttpRespHeaders::H_SET_COOKIE, "Set-Cookie", strlen("Set-Cookie"),
              "lsws_pass=b; expires=Mon, 13 May 2013 14:10:51 GMT; path=/",
              strlen("lsws_pass=b; expires=Mon, 13 May 2013 14:10:51 GMT; path=/"),
              RespHeader::APPEND);
        
        h.add(HttpRespHeaders::H_UNKNOWN, "testBreak", 9, "----", 4);
        
        h.add(HttpRespHeaders::H_SET_COOKIE, "Set-Cookie", strlen("Set-Cookie"),
              "lsws_uid=c; expires=Mon, 13 May 2013 14:10:51 GMT; path=/",
              strlen("lsws_uid=c; expires=Mon, 13 May 2013 14:10:51 GMT; path=/"),
              RespHeader::APPEND);
        io.clear();     h.getHeaders(&io);     DisplayHeader(io, kk, h.getCount());
        CHECK(h.getCount() == 7);  
        

        h.getHeader("Date", 4, &pVal, valLen); CHECK (memcmp(pVal, "Thu, 16 May 2099 20:32:23 GMT", valLen) == 0);
        h.getHeader("Allow", 5,&pVal, valLen); CHECK (memcmp(pVal, "*.*; .zip; .rar; .exe; .flv", valLen) == 0);
    
        h.parseAdd("MytestHeader: TTTTTTTTTTTT\r\nMyTestHeaderii: IIIIIIIIIIIIIIIIIIIII\r\n", 
                    strlen("MytestHeader: TTTTTTTTTTTT\r\nMyTestHeaderii: IIIIIIIIIIIIIIIIIIIII\r\n"), RespHeader::REPLACE);
 
        CHECK(h.getCount() == 9); 
        h.getHeader("MytestHeader", strlen("MytestHeader"), &pVal, valLen); 
        CHECK (memcmp(pVal, "TTTTTTTTTTTT", valLen) == 0);
        
        //Same name, but since no check,  will be appended directly. But SPDY, will check and parse it.
        h.parseAdd("MytestHeader: TTTTTTTTTTTT3\r\n", 
                    strlen("MytestHeader: TTTTTTTTTTTT3\r\n"), RespHeader::REPLACE);

        CHECK(h.getCount() == 9); 
        h.getHeader("MytestHeader", strlen("MytestHeader"), &pVal, valLen); 
        CHECK (memcmp(pVal, "TTTTTTTTTTTT3", valLen) == 0);
        
        io.clear();
        h.addStatusLine(0, SC_404);
        h.getHeaders(&io);    
        
        if ( kk == 0)
            CHECK(h.getCount() == 9); 
        else if ( kk == 1)
        {
            h.getHeader("Status", strlen("Status"), &pVal, valLen); 
            CHECK (memcmp(pVal, "404", valLen) == 0);
            CHECK(h.getCount() == 11); 
        }
        else
        {
            h.getHeader(":Status", strlen(":Status"), &pVal, valLen); 
            CHECK (memcmp(pVal, "404", valLen) == 0);
            CHECK(h.getCount() == 11); 
        }
        
        DisplayHeader(io, kk, h.getCount());
    }

    
    printf("Finshed httpheader.\n\n");
    
}

}

#endif

