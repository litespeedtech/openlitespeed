/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
#ifndef HTTPHEADER_H
#define HTTPHEADER_H

#include <util/gpointerlist.h>
#include <util/autobuf.h>

#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <new>

class AutoStr2;

#define HASH_TABLE_SIZE 66

enum
{
    LSI_HEADER_ADD = 0,    //add a new line
    LSI_HEADER_SET,
    LSI_HEADER_APPEND, //Add with a comma to seperate
    LSI_HEADER_MERGE,  //append unless exist
    LSI_HEADER_UNSET,
};

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
        H_X_LITESPEED_PURGE,
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
        /*
        H_ACCEPT_RANGES,
        H_AGE,
        H_ETAG,
        H_LOCATION,
        H_PROXY_AUTHENTICATE,
        H_PROXY_CONNECTION,
        H_RETRY_AFTER,
        H_SERVER,
        H_VARY,
        H_WWW_AUTHENTICATE,
        H_SET_COOKIE,
        H_SET_COOKIE2,
        CGI_STATUS,
        H_LITESPEED_LOCATION,
        H_CONTENT_DISPOSITION,
        H_LITESPEED_CACHE_CONTROL,

        H_HTTP_VERSION,
        */

        H_HEADER_END
    };
    static size_t getIndex(const char *pHeader);
    static size_t getIndex2(const char *pHeader);
    static size_t getIndex(const char *pHeader, int len);


    static const char *getHeaderNameLowercase(int iIndex)
    {   return s_pHeaderNamesLowercase[iIndex];    }

private:
    static const char *s_pHeaderNames[H_HEADER_END + 1];
    static const char  *s_pHeaderNamesLowercase[H_HEADER_END + 1];
    static int s_iHeaderLen[H_HEADER_END + 1];

    HttpHeader(const HttpHeader &rhs);
    void operator=(const HttpHeader &rhs);
    HttpHeader();
    ~HttpHeader();

public:
    static int getHeaderStringLen(int iIndex)
    {   return s_iHeaderLen[iIndex];    }

    static const char *getHeaderName(int iIndex)
    {   return s_pHeaderNames[iIndex];    }
};

class HeaderOp
{
public:
    enum
    {
        FLAG_RELEASE_NAME  = 1,
        FLAG_RELEASE_VALUE = 2,
        FLAG_INHERITED     = 4,
        FLAG_COMPLEX_VALUE = 8,
        FLAG_RELEASE_ENV   = 32,
        FLAG_REQ_HDR_OP    = 64,
    };
    
    //static size_t getRespHeaderIndex( const char * pHeader );
    HeaderOp()
    {
        memset(&m_iIndex, 0, (char *)(&m_pEnv + 1) - (char *)&m_iIndex);
    }

    HeaderOp(int16_t index, const char *pName, u_int16_t nameLen,
               const char  *pVal,
               u_int16_t valLen, int8_t flag = 0, int8_t op = LSI_HEADER_ADD)
        : m_iIndex(index)
        , m_iOperator(op)
        , m_iFlag(flag)
        , m_iNameLen(nameLen)
        , m_iValLen(valLen)
        , m_pName(pName)
        , m_pStrVal(pVal)
        , m_pEnv(NULL)
    {}

    ~HeaderOp();

    void setInheritFlag()
    {
        m_iFlag = (m_iFlag & ~(FLAG_RELEASE_NAME | FLAG_RELEASE_VALUE |
                               FLAG_RELEASE_ENV)) | FLAG_INHERITED;
    }

    void inherit(const HeaderOp &rhs)
    {
        memmove(&m_iIndex, &rhs.m_iIndex,
                (char *)(&m_pEnv + 1) - (char *)&m_iIndex);
        setInheritFlag();
    }

    char isInherited() const    {   return m_iFlag & FLAG_INHERITED;        }
    char isComplexValue() const {   return m_iFlag & FLAG_COMPLEX_VALUE;    }
    
    char isReqHeader() const    {   return m_iFlag & FLAG_REQ_HDR_OP;       }

    const char *getName() const     {  return m_pName;     }
    const char *getValue() const    { return m_pStrVal;       }
    uint16_t getNameLen() const     {  return m_iNameLen;  }
    uint16_t getValueLen() const    {  return m_iValLen;   }

    int8_t   getOperator() const    {   return m_iOperator; }
    int16_t  getIndex() const       {   return m_iIndex;    }
    const AutoStr2 *getEnv() const  {   return m_pEnv;      }
    void setEnv(const char *pEnv, int len);
    
private:

    int16_t         m_iIndex;
    int8_t          m_iOperator;
    int8_t          m_iFlag;
    uint16_t        m_iNameLen;
    uint16_t        m_iValLen;
    const char     *m_pName;
    const char     *m_pStrVal;
    AutoStr2       *m_pEnv;
    
    LS_NO_COPY_ASSIGN(HeaderOp);
};

class HttpHeaderOps
{
public:
    HttpHeaderOps()
        : m_buf(sizeof(HeaderOp) * 1)
        , m_has_req_op(0)
        , m_has_resp_op(0)
    {}
    ~HttpHeaderOps()
    {
        HeaderOp *iter;
        for (iter = begin(); iter < end(); ++iter)
            iter->~HeaderOp();
    }

    HeaderOp *begin()
    {   return (HeaderOp *)m_buf.begin();     }
    const HeaderOp *begin() const
    {   return (const HeaderOp *)m_buf.begin();     }

    HeaderOp *end()
    {   return (HeaderOp *)m_buf.end();       }
    const HeaderOp *end() const
    {   return (const HeaderOp *)m_buf.end();       }

    int size() const
    {   return end() - begin();     }

    HeaderOp *append(int16_t index, const char *pName, u_int16_t nameLen,
                     const char *pVal, u_int16_t valLen, 
                     int8_t op = LSI_HEADER_ADD, int is_req = 0);

    void inherit(const HttpHeaderOps &parent);
    int parseOp(const char *pLineBegin, const char *pCurEnd, int is_req);
    short has_req_op() const    {   return m_has_req_op;    }
    short has_resp_op() const   {   return m_has_resp_op;   }
    
private:
    AutoBuf m_buf;
    short   m_has_req_op;
    short   m_has_resp_op;
};

#endif
