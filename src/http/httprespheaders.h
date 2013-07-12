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
#ifndef HTTPRESPHEADERS_H
#define HTTPRESPHEADERS_H

#include <util/autobuf.h>
#include <http/httpreq.h>
#include <http/httpheader.h>
#include <util/iovec.h>
#include <http/httpstatusline.h>

namespace RespHeader {
    enum FORMAT {
        REGULAR = 0,
        SPDY2,
        SPDY3,
    };

    enum ADD_METHOD {
        REPLACE = 0,
        APPEND,
    };
    
};


struct header_st;

class HttpRespHeaders
{
public:
    enum HEADERINDEX
    {
        // most common response-header
        H_UNKNOWN = -1,
        H_ACCEPT_RANGES = 0,
        H_CONNECTION,
        H_CONTENT_TYPE,
        H_CONTENT_LENGTH,
        H_CONTENT_ENCODING,
        H_CONTENT_RANGE,
        H_CONTENT_DISPOSITION,
        H_CACHE_CTRL,
        H_DATE,
        H_ETAG,
        H_EXPIRES,
        H_KEEP_ALIVE,
        H_LAST_MODIFIED,
        H_LOCATION,
        H_LITESPEED_LOCATION,
        H_LITESPEED_CACHE_CONTROL,
        H_PRAGMA,
        H_PROXY_CONNECTION,
        H_SERVER,
        H_SET_COOKIE,
        CGI_STATUS,
        H_TRANSFER_ENCODING,
        H_VARY,
        H_WWW_AUTHENTICATE,
        H_X_POWERED_BY,
        
        H_HTTP_VERSION,
        H_HEADER_END
        
        //not commonly used headers. 
//         H_AGE,
//         H_PROXY_AUTHENTICATE,
//         H_RETRY_AFTER,
//         H_SET_COOKIE2,
// 
// 
//         // general-header
//         H_TRAILER,
//         H_UPGRADE,
//         H_WARNING,
// 
//         // entity-header
//         H_ALLOW,
//         H_CONTENT_LANGUAGE,
//         H_CONTENT_LOCATION,
//         H_CONTENT_MD5,
    };
        
public:
    HttpRespHeaders();
    ~HttpRespHeaders() {};
    
    void reset(RespHeader::FORMAT format = RespHeader::REGULAR);
    
    int add( HEADERINDEX headerIndex, const char * pName, unsigned int nameLen, const char * pVal, unsigned int valLen, RespHeader::ADD_METHOD method = RespHeader::REPLACE );
    int appendLastVal( const char * pName, int nameLen, const char * pVal, int valLen );
    int add( header_st *headerArray, int size, RespHeader::ADD_METHOD method = RespHeader::REPLACE );
    void parseAdd( const char * pStr, int len, RespHeader::ADD_METHOD method = RespHeader::REPLACE );
    
    
    //Special case
    void addStatusLine( int ver, int code );
    
    int del( const char * pName, int nameLen );
    int del( HEADERINDEX headerIndex );
    
    void getHeaders( IOVec *io );
    int  getCount() const                       {   return m_iHeaderCount;   }    
    RespHeader::FORMAT getFormat() const        {   return m_headerFormat;   }; 
    
    char *getContentTypeHeader(int &len);
        
    int  getHeader(const char *pName, int nameLen, char **pVal, int &valLen);
    int  getHeader(HEADERINDEX index, char **pVal, int &valLen);

public:
    static HEADERINDEX getRespHeaderIndex( const char * pHeader );
    static int getHeaderStringLen( HEADERINDEX index )  {    return s_iHeaderLen[(int)index];  }
    
private:
    AutoBuf             m_buf;
    unsigned char       m_KVPairindex[H_HEADER_END];
    AutoStr2            m_sKVPair;
    unsigned char       m_hasHole;
    RespHeader::FORMAT  m_headerFormat;
    int                 m_iHeaderCount;
    int                 m_hLastHeaderKVPairIndex;
        
    const StatusLineString   *m_pStatusLine;
    static int s_iHeaderLen[H_HEADER_END+1];
    
    
    int             getFreeSpaceCount() const {    return m_sKVPair.len() / sizeof(key_value_pair) - m_iHeaderCount;   }; 
    void            incKVPairs(int num);
    key_value_pair *getKVPair(int index);
    char *          getHeaderStr(int offset)      { return m_buf.begin() + offset + (int)m_headerFormat * 2;  }
    char *          getName(key_value_pair *pKv)  { return getHeaderStr(pKv->keyOff); }
    char *          getVal(key_value_pair *pKv)   { return getHeaderStr(pKv->valOff); }
    
    void            delAndMove(int kvOrderNum);
    void            replaceHeader(key_value_pair *pKv, const char * pVal, unsigned int valLen);
    int             appendHeader(key_value_pair *pKv, const char * pName, unsigned int nameLen, const char * pVal, unsigned int valLen, RespHeader::ADD_METHOD method);
    int             getHeaderKvOrder(const char *pName, unsigned int nameLen);
    void            verifyHeaderLength(HEADERINDEX headerIndex, const char * pName, unsigned int nameLen);
    int             _getHeader(int kvOrderNum, char **pVal, int &valLen);
    
    
    HttpRespHeaders(const HttpRespHeaders& other) {};
    
};

struct header_st {
    HttpRespHeaders::HEADERINDEX index;
    unsigned short nameLen;
    unsigned short valLen;
    const char *name;
    const char *val;
};

#endif // HTTPRESPHEADERS_H
