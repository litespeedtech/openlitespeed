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
#include "httpresp.h"
#include "datetime.h"
#include "htauth.h"
#include "httpheader.h"
#include "httpreq.h"
#include "httpserverversion.h"
#include <stdio.h>
#include <util/ssnprintf.h>

static const char * s_protocol[] =
{
    "http://",
    "https://"
};

const char * HttpResp::getProtocol() const
{
    return s_protocol[ m_iSSL ];
}
int   HttpResp::getProtocolLen() const
{
    return 7 + m_iSSL;
}


HttpResp::HttpResp()
    : m_iHeaderTotalLen( 0 )
    , m_iLogAccess( 0 )
{
}
HttpResp::~HttpResp()
{
}

void HttpResp::reset(RespHeader::FORMAT format)
{
    m_outputBuf.reset(format);
    m_iovec.clear();
    memset( &m_lEntityLength, 0,
            (char *)((&m_iHeaderTotalLen) + 1) - (char*)&m_lEntityLength );
    //m_iLogAccess = 0;
}


void HttpResp::addLocationHeader( const HttpReq * pReq )
{
    const char * pLocation = pReq->getLocation();
    int len = pReq->getLocationLen();
    int need = len + 25;
    if ( *pLocation == '/' )
        need += pReq->getHeaderLen( HttpHeader::H_HOST );
    m_outputBuf.add(HttpHeader::H_LOCATION, "Location", 8, "", 0);
    if ( *pLocation == '/' )
    {
        const char * pHost = pReq->getHeader( HttpHeader::H_HOST );
        m_outputBuf.appendLastVal( "Location", 8, getProtocol(), getProtocolLen() );
        m_outputBuf.appendLastVal( "Location", 8, pHost, pReq->getHeaderLen( HttpHeader::H_HOST ) );
    }
    m_outputBuf.appendLastVal( "Location", 8, pLocation, pReq->getLocationLen() );
    
}

void  HttpResp::iovAppend( const char * pBuf, int len )
{
    m_outputBuf.addNoCheckExptSpdy(pBuf, len );
}


void HttpResp::buildCommonHeaders()
{
    for ( int i = 0; i < 3; ++i )
    {
        s_CommonHeaders[i].reset((RespHeader::FORMAT)i);
        s_CommonHeaders[i].add(HttpHeader::H_SERVER, "Server", 6, HttpServerVersion::getVersion(), HttpServerVersion::getVersionLen());
        updateDateHeader(i + 1);
        s_CommonHeaders[i].add(HttpHeader::H_ACCEPT_RANGES, "Accept-Ranges", 13, "bytes", 5);
    }
}

void HttpResp::updateDateHeader(int index)
{
    char achDateTime[60];    
    DateTime::getRFCTime(DateTime::s_curTime, achDateTime);
    if (index == 0)
    {
        for ( int i = 0; i < 3; ++i )
            s_CommonHeaders[i].add(HttpHeader::H_DATE, "Date", 4, achDateTime, strlen(achDateTime));
    }
    else
        s_CommonHeaders[index - 1].add(HttpHeader::H_DATE, "Date", 4, achDateTime, strlen(achDateTime));
}


#define KEEP_ALIVE_HEADER_LEN 24


void HttpResp::prepareHeaders( const HttpReq * pReq, int rangeHeaderLen ) 
{
    HttpRespHeaders *pRespHeaders = NULL;
    int idFormat = m_outputBuf.getFormat();
    pRespHeaders = &s_CommonHeaders[idFormat];
        
    pRespHeaders->getHeaders(&m_iovec);
    m_outputBuf.setUnmanagedHeadersCount(pRespHeaders->getCount());

    if ( m_outputBuf.getFormat() == RespHeader::REGULAR )
    {
        if ( pReq->isKeepAlive() )
        {
            if ( pReq->getVersion() != HTTP_1_1 )
                iovAppend( s_sKeepAliveHeader, KEEP_ALIVE_HEADER_LEN );
        }
        else
            iovAppend( s_sConnCloseHeader, sizeof( s_sConnCloseHeader ) - 1 );
    }
    if ( pReq->getAuthRequired() )
        pReq->addWWWAuthHeader( m_outputBuf );
    if ( pReq->getLocationOff() )
    {
        addLocationHeader( pReq );
    }
    const AutoBuf * pExtraHeaders = pReq->getExtraHeaders();
    if ( pExtraHeaders )
    {
        m_outputBuf.addNoCheckExptSpdy( pExtraHeaders->begin(), pExtraHeaders->size() );
    }
}

void HttpResp::appendContentLenHeader()
{
    static char sLength[44] = {0};
    int n = safe_snprintf( sLength, 43, "%ld", m_lEntityLength );
    m_outputBuf.add(HttpHeader::H_CONTENT_LENGTH, "Content-Length", 
                    14, sLength, n);
}

void HttpResp::finalizeHeader( int ver, int code)
{
    //addStatusLine will add HTTP/1.1 to front of m_iovec when regular format
    //              will add version: HTTP/1.1 status: 2XX (etc) to m_outputBuf when SPDY format
    //              *** Be careful about this difference ***
    m_outputBuf.addStatusLine(&m_iovec, ver, code);
    m_outputBuf.endHeader();
    m_outputBuf.getHeaders(&m_iovec);
    
    
    
    int bufSize = m_iovec.bytes();
    m_iHeaderLeft += bufSize;
    m_iHeaderTotalLen = m_iHeaderLeft;
}
    
int HttpResp::appendHeader( const char * pName, int nameLen,
                        const char * pValue, int valLen )
{
    //FIXME: -1 all right?
    m_outputBuf.add(-1, pName, nameLen, pValue, valLen, RespHeader::APPEND);
    return 0;
}

//void HttpResp::setHeader( int headerCode, long lVal )
//{
//    char buf[80];
//    int len = sprintf( buf, "%s%ld\r\n", HttpHeader::getHeader( headerCode ), lVal );
//    m_outputBuf.append( buf, len );
//}
//

int HttpResp::appendLastMod( long tmMod )
{
    static char sTimeBuf[RFC_1123_TIME_LEN + 1] = {0};
    DateTime::getRFCTime( tmMod, sTimeBuf );
    m_outputBuf.add(HttpHeader::H_LAST_MODIFIED, "Last-Modified", 13,
                    sTimeBuf, RFC_1123_TIME_LEN );
    return 0;
}

int HttpResp::addCookie( const char * pName, const char * pVal,
                 const char * path, const char * domain, int expires,
                 int secure, int httponly )
{
    if ( !pName || !pVal || !domain )
        return -1;
    
    char sBuf[8192] = {0};
    snprintf( sBuf, 8191, "%s=%s; path=%s; domain=%s",
                        pName, pVal, path? path:"/", domain );
    if ( expires )
    {
        strcat(sBuf, "; expires=" );
        long t = DateTime::s_curTime + expires * 60;
        DateTime::getRFCTime( t, sBuf + strlen(sBuf) );
    }
    if ( secure )
        strcat(sBuf, "; secure" );
    if ( httponly )
        strcat(sBuf, "; HttpOnly" );
    
    m_outputBuf.add(HttpHeader::H_SET_COOKIE, "Set-Cookie", 10, sBuf, strlen(sBuf), RespHeader::APPEND);
    return 0;
}


