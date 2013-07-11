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
#ifndef HTTPFETCH_H
#define HTTPFETCH_H



#include <stddef.h>

class HttpFetch; 

typedef int (*hf_callback)( void *, HttpFetch * );

class VMemBuf;  
class HttpFetch
{
    int         m_fdHttp;
    VMemBuf   * m_pBuf;
    int         m_statusCode;
    char      * m_pReqBuf;
    int         m_reqBufLen;
    int         m_reqSent;
    int         m_reqHeaderLen;
    int         m_reqState;
    const char* m_pReqBody;
    int         m_reqBodyLen;
    int         m_connTimeout;

    int         m_respBodyLen;
    char      * m_pRespContentType;
    int         m_respBodyRead;
    int         m_respHeaderBufLen;
    char      * m_pProxyServerAddr;
    
    hf_callback m_callback;
    void      * m_callbackArg;

    char        m_achHost[256];
    char        m_achResHeaderBuf[1024];
    
    
    int endReq( int res );
    int getLine( char * &p, char * pEnd,
            char * &pLineBegin, char * &pLineEnd );
    int allocateBuf( const char * pSaveFile );
    int buildReq( const char * pMethod, const char * pURL, const char * pContentType = NULL );
    int startProcessReq( int nonblock, const char * pAddr );
    int sendReq();
    int recvResp();
        
public: 
    HttpFetch();
    ~HttpFetch();
    void setCallBack( hf_callback cb, void * pArg )
    {   m_callback = cb; m_callbackArg = pArg;  }
    int getHttpFd() const           {   return m_fdHttp;    }
    int startReq( const char * pURL, int nonblock, const char * pBody = NULL, 
                    int bodyLen = 0, const char * pSaveFile = NULL,
                    const char * pContentType = NULL, const char * addrServer = NULL );
    short getPollEvent() const;
    int processEvents( short revent );
    int process();
    int cancel();
    int getStatusCode() const   {   return m_statusCode;    }
    const char * getRespContentType() const {   return m_pRespContentType;  }
    VMemBuf * getResult() const {   return m_pBuf;          }
    void releaseResult();
    void reset();
    void setProxyServerAddr( const char * pAddr );
    const char * getProxyServerAddr() const
    {   return m_pProxyServerAddr;  }
    
};

#endif
