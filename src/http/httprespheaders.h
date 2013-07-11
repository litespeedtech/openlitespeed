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

class HttpRespHeaders
{
public:
    HttpRespHeaders();
    ~HttpRespHeaders() {};
    
    void reset(RespHeader::FORMAT format = RespHeader::REGULAR);
    
    int add( int headerIndex, const char * pName, unsigned int nameLen, const char * pVal, unsigned int valLen, RespHeader::ADD_METHOD method = RespHeader::REPLACE );
    int appendLastVal( const char * pName, int nameLen, const char * pVal, int valLen );
    
    void addNoCheckExptSpdy( const char * pStr, int len );
    
    
    //Special case
    void addStatusLine( IOVec *io, int ver, int code );
    
    int del( const char * pName, int nameLen );
    int del( int headerIndex );
    
    void getHeaders( IOVec *io );
    int  getCount() const                       {   return m_iHeaderCount;   }    
    int  getTotalCount() const                  {   return m_iUnmanagedHeadersCount + m_iHeaderCount;    };
    RespHeader::FORMAT getFormat() const        {   return m_headerFormat;    }; 
    void endHeader();
    
    char *getContentTypeHeader(int &len);
    void setUnmanagedHeadersCount(int n)        {  m_iUnmanagedHeadersCount = n;    };
    
    int  getHeader(const char *pName, int nameLen, char **pVal, int &valLen);

    
private:
    AutoBuf             m_buf;
    unsigned char       m_KVPairindex[HttpHeader::H_HEADER_END];
    AutoStr2            m_sKVPair;
    int                 m_hasHole;
    RespHeader::FORMAT  m_headerFormat;
    int                 m_iHeaderCount;
    int                 m_hLastHeaderKVPairIndex;
    int                 m_iExtraBufOff;
    int                 m_iExtraBufLen;
    int                 m_iUnmanagedHeadersCount;
    
private:

    int             getFreeSpaceCount() const {    return m_sKVPair.len() / sizeof(key_value_pair) - m_iHeaderCount;   }; 
    void            incKVPairs(int num);
    key_value_pair *getKVPair(int index);
    char *          getHeaderStr(int offset)      { return m_buf.begin() + offset + (int)m_headerFormat * 2;  }
    char *          getName(key_value_pair *pKv)  { return getHeaderStr(pKv->keyOff); }
    char *          getVal(key_value_pair *pKv)   { return getHeaderStr(pKv->valOff); }
    
    void            delAndMove(int kvOrderNum);
    void            replaceHeader(key_value_pair *pKv, const char * pVal, unsigned int valLen);
    int             appendHeader(key_value_pair *pKv, const char * pName, unsigned int nameLen, const char * pVal, unsigned int valLen);
    int             getHeaderIndex(const char *pName, unsigned int nameLen);
    
    HttpRespHeaders(const HttpRespHeaders& other) {};
    
};

#endif // HTTPRESPHEADERS_H
