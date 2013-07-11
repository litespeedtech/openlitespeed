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
    : m_iLogAccess( 0 )
    , m_iHeaderTotalLen( 0 )
    , m_iSetCookieLen( 0 )
{
    reset();
}
HttpResp::~HttpResp()
{
}

void HttpResp::reset()
{
    if (m_iSetCookieLen && !m_iSetCookieOffset)
        m_outputBuf.resize( m_iSetCookieLen );
    else
        m_outputBuf.clear();
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
    if ( m_outputBuf.available() < need  )
        if ( m_outputBuf.grow( need ) )
            return;
    m_outputBuf.appendNoCheck( "Location: ", 10 );
    if ( *pLocation == '/' )
    {
        const char * pHost = pReq->getHeader( HttpHeader::H_HOST );
        m_outputBuf.appendNoCheck( getProtocol(), getProtocolLen() );
        m_outputBuf.appendNoCheck( pHost, pReq->getHeaderLen( HttpHeader::H_HOST ) );
    }
    m_outputBuf.appendNoCheck( pLocation, pReq->getLocationLen() );
    m_outputBuf.append( '\r' );
    m_outputBuf.append( '\n' );
}
/*
#define WWW_AUTH_MAX_LEN 1024
void HttpResp::addWWWAuthHeader( const HttpReq * pReq )
{
    int size = WWW_AUTH_MAX_LEN;
    const char * pAuthHeader;
    char achAuth[WWW_AUTH_MAX_LEN + 1 ];
    pAuthHeader = pReq->getWWWAuthHeader( achAuth, size );
    if ( pAuthHeader )
    {
        //if ( ret == WWW_AUTH_MAX_LEN )
        //{
            //FIXME: max size reached
        //}
        if ( m_outputBuf.available() < size + 10  )
            if ( m_outputBuf.grow( size + 10 ) )
                return;
        m_outputBuf.appendNoCheck( pAuthHeader, size );
    }
}
*/

int  HttpResp::safeAppend( const char * pBuf, int len )
{
    if ( m_outputBuf.available() < len + 10  )
        if ( m_outputBuf.grow( len + 10 ) )
            return -1;
    char * p = m_outputBuf.end();
    const char * pSrc = pBuf;
    for( int i = 0; i < len ; ++i )
        p[i] = pSrc[i];
    
    //memmove( m_outputBuf.end(), pBuf, len );
    m_outputBuf.append( pBuf, len );
    return 0;
}


void HttpResp::buildCommonHeaders()
{
    char achDateTime[60];
    char * p = s_sCommonHeaders;
    memcpy( p, "Server: ", 8 );
    p += 8;
    memcpy( p, HttpServerVersion::getVersion(),
            HttpServerVersion::getVersionLen() );
    p += HttpServerVersion::getVersionLen();
    
    p += safe_snprintf( p, sizeof( s_sCommonHeaders ) - ( p - s_sCommonHeaders ),
            "\r\n" "Date: %s\r\n" "Accept-Ranges: bytes\r\n",
            DateTime::getRFCTime( DateTime::s_curTime, achDateTime ) );
    s_iCommonHeaderLen = p - s_sCommonHeaders - RANGE_HEADER_LEN;
}

void HttpResp::updateDateHeader()
{
    char * pDateValue = &s_sCommonHeaders[ 10 + 6 +
                    HttpServerVersion::getVersionLen()];
    DateTime::getRFCTime( DateTime::s_curTime, pDateValue);
    *(pDateValue + RFC_1123_TIME_LEN) = '\r';
}


#define KEEP_ALIVE_HEADER_LEN 24


void HttpResp::prepareHeaders( const HttpReq * pReq, int rangeHeaderLen ) 
{
    iovAppend( s_sCommonHeaders, s_iCommonHeaderLen + rangeHeaderLen);
    if ( pReq->isKeepAlive() )
    {
        if ( pReq->getVersion() != HTTP_1_1 )
            iovAppend( s_sKeepAliveHeader, KEEP_ALIVE_HEADER_LEN );
    }
    else
        iovAppend( s_sConnCloseHeader, sizeof( s_sConnCloseHeader ) - 1 );
    if ( pReq->getAuthRequired() )
        pReq->addWWWAuthHeader( m_outputBuf );
    if ( pReq->getLocationOff() )
    {
        addLocationHeader( pReq );
    }
    const AutoBuf * pExtraHeaders = pReq->getExtraHeaders();
    if ( pExtraHeaders )
    {
        m_outputBuf.append( pExtraHeaders->begin(), pExtraHeaders->size() );
    }
}

void HttpResp::appendContentLenHeader()
{
    if ( m_outputBuf.available() < 70 )
    {
        if ( m_outputBuf.grow( 70 ) )
            return;
    }
    int n = safe_snprintf( m_outputBuf.end(), 60,
            "Content-Length: %ld\r\n", m_lEntityLength );
    m_outputBuf.used( n );
}


int HttpResp::appendHeader( const char * pName, int nameLen,
                        const char * pValue, int valLen )
{
    if ( m_outputBuf.available() < nameLen + valLen + 6 )
    {
        if ( m_outputBuf.grow( nameLen + valLen + 6 ) == -1 )
            return -1;
    }
    m_outputBuf.appendNoCheck( pName, nameLen );
    m_outputBuf.append( ':' );
    m_outputBuf.append( ' ' );
    m_outputBuf.appendNoCheck( pValue, valLen );
    m_outputBuf.append( '\r' );
    m_outputBuf.append( '\n' );
    return 0;
}

int HttpResp::appendHeaderLine( const char * pLineBegin, const char * pLineEnd )
{
    if ( m_outputBuf.available() < pLineEnd - pLineBegin + 4 )
    {
        if ( m_outputBuf.grow( pLineEnd - pLineBegin + 4 ) == -1 )
            return -1;
    }
    m_outputBuf.appendNoCheck( pLineBegin, pLineEnd - pLineBegin );
    m_outputBuf.append( '\r' );
    m_outputBuf.append( '\n' );
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
    if ( m_outputBuf.available() < RFC_1123_TIME_LEN + 21 )
    {
        if ( m_outputBuf.grow( RFC_1123_TIME_LEN + 21 ) == -1 )
            return -1;
    }
    m_outputBuf.append( "Last-Modified: ", 15 );
    DateTime::getRFCTime( tmMod, m_outputBuf.end() );
    m_outputBuf.used( RFC_1123_TIME_LEN );
    m_outputBuf.append( '\r' );
    m_outputBuf.append( '\n' );
    return 0;
}
int HttpResp::addCookie( const char * pName, const char * pVal,
                 const char * path, const char * domain, int expires,
                 int secure, int httponly )
{
    if ( !pName || !pVal || !domain )
        return -1;
    if ( !m_iSetCookieLen )
        m_outputBuf.clear();
    int len = strlen( pName )+ strlen( pVal ) + strlen( domain ) +
            path? strlen( path ): 1;
    if ( m_outputBuf.available() < len + 150 )
    {
        if ( m_outputBuf.grow( len + 150 ) == -1 )
            return -1;
    }

    int n = snprintf( m_outputBuf.end(), m_outputBuf.available(), "Set-Cookie: %s=%s; path=%s; domain=%s",
                        pName, pVal, path? path:"/", domain );
    m_outputBuf.used( n );
    if ( expires )
    {
        m_outputBuf.append( "; expires=" );
        long t = DateTime::s_curTime + expires * 60;
        DateTime::getRFCTime( t, m_outputBuf.end() );
        m_outputBuf.used( RFC_1123_TIME_LEN );
    }
    if ( secure )
        m_outputBuf.append( "; secure" );
    if ( httponly )
        m_outputBuf.append( "; HttpOnly" );
    m_outputBuf.append( '\r' );
    m_outputBuf.append( '\n' );
    m_iSetCookieLen = m_outputBuf.size();
    return 0;
}

void HttpResp::setCookieHeaderLen( int len )
{
    if ( !m_iSetCookieLen )
    {
        m_iSetCookieOffset = m_outputBuf.size();
        m_iSetCookieLen = len;
        return;
    }
    int moveBytes = m_outputBuf.size() - ( m_iSetCookieOffset + m_iSetCookieLen );
    
    if ( moveBytes > 0 )
    {
        char achBuf[65535];
        assert( moveBytes < 65535 );
        memmove( achBuf, m_outputBuf.end() - moveBytes, moveBytes );
        memmove( m_outputBuf.end() - m_iSetCookieLen, 
                m_outputBuf.getPointer( m_iSetCookieOffset ), m_iSetCookieLen );
        memmove( m_outputBuf.getPointer( m_iSetCookieOffset ), achBuf, moveBytes );
        m_iSetCookieOffset = m_outputBuf.size() - m_iSetCookieLen ;
    }
    m_iSetCookieLen += len;
}

