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
#include <assert.h>
#include <http/httpheader.h>
#include <http/httprespheaders.h>

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
    m_respHeaders.reset(format);
    m_iovec.clear();
    memset( &m_lEntityLength, 0,
            (char *)((&m_iHeaderTotalLen) + 1) - (char*)&m_lEntityLength );
}

void HttpResp::addLocationHeader( const HttpReq * pReq )
{
    const char * pLocation = pReq->getLocation();
    int len = pReq->getLocationLen();
    int need = len + 25;
    if ( *pLocation == '/' )
        need += pReq->getHeaderLen( HttpHeader::H_HOST );
    m_respHeaders.add(HttpRespHeaders::H_LOCATION, "Location", 8, "", 0);
    if ( *pLocation == '/' )
    {
        const char * pHost = pReq->getHeader( HttpHeader::H_HOST );
        m_respHeaders.appendLastVal( "Location", 8, getProtocol(), getProtocolLen() );
        m_respHeaders.appendLastVal( "Location", 8, pHost, pReq->getHeaderLen( HttpHeader::H_HOST ) );
    }
    m_respHeaders.appendLastVal( "Location", 8, pLocation, pReq->getLocationLen() );
}

void  HttpResp::parseAdd( const char * pBuf, int len )
{
    m_respHeaders.parseAdd(pBuf, len, RespHeader::APPEND );
}

void HttpResp::buildCommonHeaders()
{
    HttpResp::m_commonHeaders[0].index    = HttpRespHeaders::H_DATE;
    HttpResp::m_commonHeaders[0].name     = HttpResp::s_sCommonHeaders;
    HttpResp::m_commonHeaders[0].nameLen  = 4;
    HttpResp::m_commonHeaders[0].val      = HttpResp::s_sCommonHeaders + 6;
    HttpResp::m_commonHeaders[0].valLen   = 29;
    
    HttpResp::m_commonHeaders[1].index    = HttpRespHeaders::H_ACCEPT_RANGES;
    HttpResp::m_commonHeaders[1].name     = HttpResp::s_sCommonHeaders + 37;
    HttpResp::m_commonHeaders[1].nameLen  = 13;
    HttpResp::m_commonHeaders[1].val      = HttpResp::s_sCommonHeaders + 52;
    HttpResp::m_commonHeaders[1].valLen   = 5;

    HttpResp::m_commonHeaders[2].index    = HttpRespHeaders::H_SERVER;
    HttpResp::m_commonHeaders[2].name     = HttpResp::s_sCommonHeaders + 59;
    HttpResp::m_commonHeaders[2].nameLen  = 6;
    HttpResp::m_commonHeaders[2].val      = HttpServerVersion::getVersion();
    HttpResp::m_commonHeaders[2].valLen   = HttpServerVersion::getVersionLen();

        
    HttpResp::m_gzipHeaders[0].index    = HttpRespHeaders::H_CONTENT_ENCODING;
    HttpResp::m_gzipHeaders[0].name     = HttpResp::s_sGzipEncodingHeader;
    HttpResp::m_gzipHeaders[0].nameLen  = 16;
    HttpResp::m_gzipHeaders[0].val      = HttpResp::s_sGzipEncodingHeader + 18;
    HttpResp::m_gzipHeaders[0].valLen   = 4;
    
    HttpResp::m_gzipHeaders[1].index    = HttpRespHeaders::H_VARY;
    HttpResp::m_gzipHeaders[1].name     = HttpResp::s_sGzipEncodingHeader + 24;
    HttpResp::m_gzipHeaders[1].nameLen  = 4;
    HttpResp::m_gzipHeaders[1].val      = HttpResp::s_sCommonHeaders + 30;
    HttpResp::m_gzipHeaders[1].valLen   = 15;
    
    HttpResp::m_keepaliveHeader.index    = HttpRespHeaders::H_CONNECTION;
    HttpResp::m_keepaliveHeader.name     = HttpResp::s_sKeepAliveHeader;
    HttpResp::m_keepaliveHeader.nameLen  = 10;
    HttpResp::m_keepaliveHeader.val      = HttpResp::s_sKeepAliveHeader + 12;
    HttpResp::m_keepaliveHeader.valLen   = 10;
    
    HttpResp::m_concloseHeader.index    = HttpRespHeaders::H_CONNECTION;
    HttpResp::m_concloseHeader.name     = HttpResp::s_sConnCloseHeader;
    HttpResp::m_concloseHeader.nameLen  = 10;
    HttpResp::m_concloseHeader.val      = HttpResp::s_sConnCloseHeader + 12;
    HttpResp::m_concloseHeader.valLen   = 5;
    
    HttpResp::m_chunkedHeader.index    = HttpRespHeaders::H_TRANSFER_ENCODING;
    HttpResp::m_chunkedHeader.name     = HttpResp::s_chunked;
    HttpResp::m_chunkedHeader.nameLen  = 17;
    HttpResp::m_chunkedHeader.val      = HttpResp::s_chunked + 19;
    HttpResp::m_chunkedHeader.valLen   = 7;

    updateDateHeader();
}

void HttpResp::updateDateHeader()
{
    char achDateTime[60];    
    DateTime::getRFCTime(DateTime::s_curTime, achDateTime);
    assert(strlen(achDateTime) == 29);
    memcpy(HttpResp::s_sCommonHeaders + 6, achDateTime, 29);
}


void HttpResp::prepareHeaders( const HttpReq * pReq, int rangeHeaderLen ) 
{
    
    m_respHeaders.add(HttpResp::m_commonHeaders, HttpResp::m_commonHeadersCount);

    if ( m_respHeaders.getFormat() == RespHeader::REGULAR )
    {
        if ( pReq->isKeepAlive() )
        {
            if ( pReq->getVersion() != HTTP_1_1 )
                m_respHeaders.add( &HttpResp::m_keepaliveHeader, 1);
        }
        else
            m_respHeaders.add( &HttpResp::m_concloseHeader, 1);
    }
    if ( pReq->getAuthRequired() )
        pReq->addWWWAuthHeader( m_respHeaders );
    if ( pReq->getLocationOff() )
    {
        addLocationHeader( pReq );
    }
    const AutoBuf * pExtraHeaders = pReq->getExtraHeaders();
    if ( pExtraHeaders )
    {
        m_respHeaders.parseAdd( pExtraHeaders->begin(), pExtraHeaders->size(), RespHeader::APPEND );
    }
}

void HttpResp::appendContentLenHeader()
{
    static char sLength[44] = {0};
    int n = safe_snprintf( sLength, 43, "%ld", m_lEntityLength );
    m_respHeaders.add(HttpRespHeaders::H_CONTENT_LENGTH, "Content-Length", 
                    14, sLength, n);
}

void HttpResp::finalizeHeader( int ver, int code)
{
    //addStatusLine will add HTTP/1.1 to front of m_iovec when regular format
    //              will add version: HTTP/1.1 status: 2XX (etc) to m_outputBuf when SPDY format
    //              *** Be careful about this difference ***
    m_respHeaders.addStatusLine(ver, code);
    m_respHeaders.getHeaders(&m_iovec);
    
    
    
    int bufSize = m_iovec.bytes();
    m_iHeaderLeft += bufSize;
    m_iHeaderTotalLen = m_iHeaderLeft;
}
    
int HttpResp::appendHeader( const char * pName, int nameLen,
                        const char * pValue, int valLen )
{
    m_respHeaders.add(HttpRespHeaders::H_UNKNOWN, pName, nameLen, pValue, valLen, RespHeader::APPEND);
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
    m_respHeaders.add(HttpRespHeaders::H_LAST_MODIFIED, "Last-Modified", 13,
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
    
    m_respHeaders.add(HttpRespHeaders::H_SET_COOKIE, "Set-Cookie", 10, sBuf, strlen(sBuf), RespHeader::APPEND);
    return 0;
}


