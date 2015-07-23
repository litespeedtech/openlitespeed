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
#include "httpheader.h"

#include <util/stringtool.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>


int HttpHeader::s_iHeaderLen[H_HEADER_END + 1] =
{
    6, 14, 15, 15, 13, 10, 12, 14, 6, 7, 4, 6, 7, 10, //user-agent
    13, 17, 8, 13, 8, 19, 10, 5, 15, 3, 17, 2, 6, 12, 19, 4, //date
    7, 7, 7, 5, 16, 16, 16, 11, 13, 7, 13, //last-modified
    //13,3,4,8,18,16,11,6,4,16,10,11,6,20,19,25,
    0
};


size_t HttpHeader::getIndex(const char *pHeader)
{
    size_t idx = H_HEADER_END;
    switch (*pHeader++ | 0x20)
    {
    case 'a':
        if (strncasecmp(pHeader, "ccept", 5) == 0)
        {
            pHeader += 5;
            if (*pHeader++ == '-')
            {
                if (strncasecmp(pHeader, "charset", 7) == 0)
                    idx = H_ACC_CHARSET;
                else if (strncasecmp(pHeader, "encoding", 8) == 0)
                    idx = H_ACC_ENCODING;
                else if (strncasecmp(pHeader, "language", 8) == 0)
                    idx = H_ACC_LANG;
            }
            else
                idx = H_ACCEPT;
        }
        else if (strncasecmp(pHeader, "uthorization", 12) == 0)
            idx = H_AUTHORIZATION;
        break;
    case 'c':
        if (strncasecmp(pHeader, "onnection", 9) == 0)
            idx = H_CONNECTION;
        else if (strncasecmp(pHeader, "ontent-type", 11) == 0)
            idx = H_CONTENT_TYPE;
        else if (strncasecmp(pHeader, "ontent-length", 13) == 0)
            idx = H_CONTENT_LENGTH;
        else if (strncasecmp(pHeader, "ookie", 5) == 0)
        {
            if (*(pHeader + 5) == '2')
                idx = H_COOKIE2;
            else
                idx = H_COOKIE;
        }
        else if (strncasecmp(pHeader, "ache-control", 12) == 0)
            idx = H_CACHE_CTRL;
        break;
    case 'h':
        if (strncasecmp(pHeader, "ost", 3) == 0)
            idx = H_HOST;
        break;
    case 'i':
        if (((*pHeader | 0x20) == 'f') &&
            (*(pHeader + 1) == '-'))
        {
            pHeader += 2;
            if (strncasecmp(pHeader, "match", 5) == 0)
                idx = H_IF_MATCH;
            else if (strncasecmp(pHeader, "modified-since", 14) == 0)
                idx = H_IF_MODIFIED_SINCE;
            else if (strncasecmp(pHeader, "none-match", 8) == 0)     //If-None-Match
                idx = H_IF_NO_MATCH;
            else if (strncasecmp(pHeader, "range", 5) == 0)
                idx = H_IF_RANGE;
            else if (strncasecmp(pHeader, "unmodified-since", 16) == 0)
                idx = H_IF_UNMOD_SINCE;
        }
        break;
    case 'k':
        if (strncasecmp(pHeader, "eep-alive", 9) == 0)
            idx = H_KEEP_ALIVE;
        break;
    case 'p':
        if (strncasecmp(pHeader, "ragma", 5) == 0)
            idx = H_PRAGMA;
        break;
    case 'r':
        if (strncasecmp(pHeader, "eferer", 6) == 0)
            idx = H_REFERER;
        else if (strncasecmp(pHeader, "ange", 4) == 0)
            idx = H_RANGE;
        break;
    case 't':
        if (strncasecmp(pHeader, "ransfer-encoding", 16) == 0)
            idx = H_TRANSFER_ENCODING;
        break;
    case 'u':
        if (strncasecmp(pHeader, "ser-agent", 9) == 0)
            idx = H_USERAGENT;
        break;
    case 'v':
        if (strncasecmp(pHeader, "ia", 2) == 0)
            idx = H_VIA;
        break;
    case 'x':
        if (strncasecmp(pHeader, "-forwarded-for", 14) == 0)
            idx = H_X_FORWARDED_FOR;
        break;
    }

    return idx;
}


size_t HttpHeader::getIndex2(const char *pHeader)
{
    size_t idx = getIndex(pHeader);
    if (idx < H_HEADER_END)
    {
        char ch = *(pHeader + getHeaderStringLen(idx));
        if ((!ch) || (ch == ' ') || (ch == '\t'))
            return idx;
    }
    return H_HEADER_END;
}


/*
class HttpBuf;
class HeaderList;
class HttpHeader
{
public:
    enum
    {
        // most common request-header
        H_ACCEPT = 0,
        H_ACC_CHARSET,
        H_ACC_ENCODING,
        H_ACC_LANG,
        H_AUTHORIZATION,
        H_CONNECTION,
        H_CONTENT_TYPE,
        H_CONTENT_LENGTH,
        H_COOKIE,
        H_COOKIE2,
        H_HOST,
        H_PRAGMA,
        H_REFERER,
        H_USERAGENT,
        H_CACHE_CTRL,
        H_IF_MODIFIED_SINCE,
        H_IF_MATCH,
        H_IF_NO_MATCH,
        H_IF_RANGE,
        H_IF_UNMOD_SINCE,
        H_KEEP_ALIVE,
        H_RANGE,
        H_X_FORWARDED_FOR,
        H_VIA,
        H_TRANSFER_ENCODING,

        //request-header
        H_TE,
        H_EXPECT,
        H_MAX_FORWARDS,
        H_PROXY_AUTH,

        // general-header
        H_DATE,
        H_TRAILER,
        H_UPGRADE,
        H_WARNING,

        // entity-header
        H_ALLOW,
        H_CONTENT_ENCODING,
        H_CONTENT_LANGUAGE,
        H_CONTENT_LOCATION,
        H_CONTENT_MD5,
        H_CONTENT_RANGE,
        H_EXPIRES,
        H_LAST_MODIFIED,

        // response-header
        H_ACCEPT_RANGES,
        H_AGE,
        H_ETAG,
        H_LOCATION,
        H_PROXY_AUTHENTICATE,
        H_RETRY_AFTER,
        H_SERVER,
        H_VARY,
        H_WWW_AUTHENTICATE,
        CGI_STATUS,
        H_HEADER_END
    };
    static size_t getIndex( const char * pHeader );
    static size_t getIndex2( const char * pHeader );
    static size_t getRespHeaderIndex( const char * pHeader );

private:
    static int s_iHeaderLen[H_HEADER_END+1];

    HeaderList * m_impl;

    HttpHeader( const HttpHeader& rhs ) {}
    void operator=( const HttpHeader& rhs ) {}

public:
    HttpHeader();
    ~HttpHeader();

    static int getHeaderStringLen( int iIndex )
    {   return s_iHeaderLen[iIndex];    }

    void reset();
    void add( int index, int iKeyOffset,
                int iValueOffset, int iValueLen );
    //int append( HttpBuf* pBuf, int index, const char * pValue );
    int append( HttpBuf* pBuf, const char * pKey, const char * pValue );
    int append( HttpBuf* pBuf, int index, const char * pKey,
                const char * pValue );
    const char * getHeaderValue( const HttpBuf* pBuf, int index ) const;
    const char * getHeaderValue( const HttpBuf* pBuf,
        const char * pHeaderKey ) const;
    const char * getHeaderKey( const HttpBuf* pBuf, int index ) const;


};
struct header
{
    int m_index;
    int m_iKeyOffset;
    int m_iValueOffset;
    int m_iValueLen;
public:
    header() { ::memset( this, 0, sizeof( header )); }
    header( int index, int key,
            int value, int vallen )
        : m_index( index )
        , m_iKeyOffset( key )
        , m_iValueOffset( value )
        , m_iValueLen( vallen )
        {}
    bool matchIndex( int index )
        { return m_index == index; }
};

class HeaderList : public TPointerList<header>
{
public:
    ~HeaderList()   {   clear();    }
    const_iterator find(int index ) const
    {

        const_iterator iter, endIter = end();
        // = find_if( begin(), end(),
        //            std::mem_fun_ref( &header::matchIndex ) );
        for( iter = begin(); iter != endIter; ++iter )
        {
            if ( index == (*iter)->m_index )
            {
                break;
            }
        }
        return iter;
    }
    const_iterator find( const HttpBuf* pBuf, const char * pKey ) const
    {
        int len = strlen( pKey );
        const_iterator iter, endIter = end();
        for( iter = begin(); iter != endIter; ++iter )
        {
            if ( strncmp( pKey, pBuf->begin()
                        + (*iter)->m_iKeyOffset, len ) == 0 )
            {
                break;
            }
        }
        return iter;
    }
    void clear()
    {
        release_objects();
        TPointerList<header>::clear();
    }
};

HttpHeader::HttpHeader()
{
    m_impl = new HeaderList();
}

HttpHeader::~HttpHeader()
{
    delete m_impl;
}
void HttpHeader::add( int index, int iKeyOffset,
            int iValueOffset, int iValueLen )
{
    m_impl->push_back( new
        header( index, iKeyOffset, iValueOffset, iValueLen ) );
}

//int HttpHeader::append( HttpBuf* pBuf, int index,
//                const char * pValue )
//{
//    const char * pKey = getHeader( index );
//    if ( pKey != NULL )
//    {
//        return append( pBuf, index, pKey, pValue );
//    }
//    return LS_FAIL;
//}
//
int HttpHeader::append( HttpBuf* pBuf, const char * pKey,
                const char * pValue )
{
    int index = getIndex( pKey );
    return append( pBuf, index, pKey, pValue );
}

int HttpHeader::append( HttpBuf* pBuf, int index, const char * pKey,
                const char * pValue )
{
    char achBuf[ 4096 ];
    int len = ls_snprintf( achBuf, sizeof( achBuf ) - 1, "%s: %s\r\n",
            pKey, pValue );

    int keylen = strlen( pKey );
    int vallen = strlen( pValue );
    int keyOff = pBuf->size();
    int valOff = keyOff + keylen + 2;
    add( index, keyOff, valOff, vallen );

    pBuf->append( achBuf, len );

    return 0;
}

// the headers should be sorted by index
const char * HttpHeader::getHeaderValue(
        const HttpBuf* pBuf, int index ) const
{
    HeaderList::const_iterator iter = m_impl->find( index );
    if ( iter == m_impl->end() )
        return NULL;
    else
        return pBuf->begin() + (*iter)->m_iValueOffset;
}
const char * HttpHeader::getHeaderValue(
            const HttpBuf* pBuf, const char * pHeaderKey ) const
{
    char achBuf[80];
    StringTool::strLower( pHeaderKey, achBuf, 79 );
    achBuf[79] = 0;
    const char * p = achBuf;
    int index = getIndex( p );
    if ( index != H_HEADER_END )
    {
        return getHeaderValue( pBuf, index );
    }
    else
    {
        HeaderList::const_iterator iter = m_impl->find( pBuf, pHeaderKey );
        if ( iter == m_impl->end() )
            return NULL;
        else
            return pBuf->begin() + (*iter)->m_iValueOffset;
    }
}

const char * HttpHeader::getHeaderKey(
            const HttpBuf* pBuf, int index ) const
{
    HeaderList::const_iterator iter = m_impl->find( index );
    if ( iter == m_impl->end() )
        return NULL;
    else
        return pBuf->begin() + (*iter)->m_iKeyOffset;
}

void HttpHeader::reset()
{   m_impl->clear();   }
*/
