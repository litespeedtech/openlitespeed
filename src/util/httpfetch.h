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

#include "httpfetchdriver.h"
#include <sslpp/sslconnection.h>
#include <util/autobuf.h>
#include <util/hashstringmap.h>
#include <stddef.h>
#include <time.h>

class GSockAddr;
typedef int (*hf_callback)(void *, HttpFetch *);

class AdnsReq;
namespace log4cxx
{
class Logger;
};
class SslClientSessCache;
class VMemBuf;
class HttpFetch
{
public:
    enum 
    {
        ERROR_DNS_FAILURE  = -20,
        ERROR_CONN_FAILURE,
        ERROR_CONN_TIMEOUT,
        ERROR_SOCKET_ERROR,
        ERROR_SSL_HANDSHAKE,
        ERROR_SSL_CERT_VERIFY,
        ERROR_SSL_UNMATCH_DOMAIN,
        ERROR_HTTP_SEND_REQ_HEADER_FAILURE,
        ERROR_HTTP_SEND_REQ_BODY_FAILURE,
        ERROR_HTTP_PROTOL_ERROR,
        ERROR_HTTP_RECV_RESP_FAILURE,
        ERROR_CANCELED,
    };
    
    enum state
    {
        STATE_NOT_INUSE,
        STATE_DNS,
        STATE_CONNECTING,
        STATE_CONNECTED,
        STATE_SENT_REQ_HEADER,
        STATE_SENT_REQ_BODY,
        STATE_WAIT_RESP = STATE_SENT_REQ_BODY,
        STATE_RCVD_STATUS,
        STATE_RCVD_RESP_HEADER,
        STATE_RCVD_RESP_BODY,
        STATE_ERROR,
        STATE_RUN_CALLBACK,
        STATE_FINISH = STATE_NOT_INUSE,
    };
    
    enum mode
    {
        MODE_BLOCKING,
        MODE_NON_BLOCKING,
        MODE_TEST_CONNECT,
        MODE_NON_BLOCKING_SYNC_DNS,
    };

    HttpFetch();
    ~HttpFetch();
    void setCallBack(hf_callback cb, void *pArg)
    {   m_callback = cb; m_callbackArg = pArg;  }
    int getHttpFd() const                   {   return m_fdHttp;    }

    bool isInUse() const        {   return m_reqState != STATE_NOT_INUSE;   }
    int getReqState() const     {   return m_reqState;  }

    int startReq(const char *pURL, int nonblock, int enableDriver = 1,
                 const char *pBody = NULL,
                 int bodyLen = 0, const char *pSaveFile = NULL,
                 const char *pContentType = NULL, const char *addrServer = NULL);
    int startReq(const char *pURL, int nonblock, int enableDriver,
                 const char  *pBody,
                 int bodyLen, const char *pSaveFile,
                 const char *pContentType, const GSockAddr &sockAddr);
    short getPollEvent() const;
    int processEvents(short revent);
    int process();
    int cancel();
    int getStatusCode() const               {   return m_statusCode;    }
    const char *getRespContentType() const  {   return m_pRespContentType;  }
    VMemBuf *getResult() const              {   return m_pBuf;          }
    void releaseResult();
    void reset();
    int setProxyServerAddr(const char *pAddr);
    int setProxyServerAddr(const struct sockaddr *pAddr,
                           const char *addr_str);

    const char *getProxyServerAddr() const  {   return m_psProxyServerAddr;  }

    void closeConnection();
    void setTimeout(int timeoutSec)         {   m_iTimeoutSec = timeoutSec; }
    int getTimeout() const                  {   return m_iTimeoutSec;   }
    void writeLog(const char *s);
    void enableDebug(int d)                 {   m_iEnableDebug = d;     }
    time_t getTimeStart() const             {   return m_tmStart.tv_sec;   }

    void setUseSsl(int s)                   {   m_iSsl = s;             }
    int isUseSsl() const                    {   return m_iSsl;          }

    void setVerifyCert(int s)               {   m_iVerifyCert = s;      }

    void setClientSessCache(SslClientSessCache *c)
    {   m_ssl.setClientSessCache(c);    }
    
    int setExtraHeaders(const char *pHdrs, int len);
    int appendExtraHeaders(const char *pHdrs, int len);
    const char *getRespHeader(const char *pName) const;
    X509 *getSslCert() const;

    int get_nslookup_time() const       {   return m_nslookup_time;     }
    int get_conn_time() const           {   return m_conn_time;         }
    int get_req_time() const            {   return m_req_time;          }
    
    int syncFetch(const char *pURL, char *pRespBuf, int respBufLen, int timeout,
                    const char *pReqBody = NULL, int reqBodyLen = 0,
                    const char *pContentType = NULL);
    static const char *getErrorStr(int error_code);

private:
    VMemBuf    *m_pBuf;
    int         m_fdHttp;
    int         m_statusCode;
    char       *m_pReqBuf;
    int         m_reqBufLen;
    int         m_reqSent;
    char       *m_pExtraReqHdrs;
    int         m_reqHeaderLen;
    int         m_connTimeout;
    short       m_iHostLen;
    short       m_pollEvents;
    enum state  m_reqState:8;
    enum mode   m_nonblocking:8;
    char        m_enableDriver;
    uint8_t     m_iEnableDebug;
    uint8_t     m_iSsl;
    uint8_t     m_iVerifyCert;
    uint8_t     m_family;
    const char *m_pReqBody;
    int64_t     m_reqBodyLen;

    int64_t     m_respBodyLen;
    char       *m_pRespContentType;
    int64_t     m_respBodyRcvd;
    char       *m_psProxyServerAddr;
    GSockAddr  *m_pServerAddr;
    AdnsReq    *m_pAdnsReq;

    hf_callback m_callback;
    void       *m_callbackArg;

    SslConnection  m_ssl;
    StrStrHashMap  m_respHeaders;

    char        m_achHost[256];
    AutoBuf     m_resHeaderBuf;

    HttpFetchDriver *m_pHttpFetchDriver;
    log4cxx::Logger *m_pLogger;
    timeval     m_tmStart;
    int         m_nslookup_time;
    int         m_conn_time;
    int         m_req_time;
    int         m_iTimeoutSec;
    int         m_iReqInited;
    int         m_iLoggerId;


    int endReq(int res);
    int getLine(char *&p, char *pEnd,
                char *&pLineBegin, char *&pLineEnd);
    int allocateBuf(const char *pSaveFile);
    int pollEvent(int evt, int timeoutSecs);
    int buildReq(const char *pMethod, const char *pURL,
                 const char *pContentType = NULL);
    int startProcessReq(const GSockAddr &sockAddr);
    int startProcess();
    static int asyncDnsLookupCb(void *arg, const long lParam, void *pParam);
    int startDnsLookup(const char *addrServer);

    int connectSSL();

    void setSSLAgain();
    
    void setAdnsReq(AdnsReq *req);

    int sendReq();
    int recvResp();
    int saveRespBody(const char *buf, size_t len);
    void startDriver();
    void stopDriver();
    int initReq(const char *pURL, const char *pBody, int bodyLen,
                const char *pSaveFile, const char *pContentType);
    int getLoggerId()     {   return m_iLoggerId;    }

    int verifyDomain();
    void addRespHeader(char *pLineBegin, char *pLineEnd);
    void updateConnectTime();

    
};


#endif
