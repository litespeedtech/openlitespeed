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
#include "httpfetch.h"
#include <adns/adns.h>
#include <log4cxx/logger.h>
#include <socket/coresocket.h>
#include <socket/gsockaddr.h>
#include <sslpp/sslcontext.h>
#include <sslpp/sslerror.h>
#include <sslpp/sslutil.h>
#include <util/autobuf.h>
#include <util/vmembuf.h>
#include <util/ni_fio.h>
#include <util/ssnprintf.h>
#include "stringtool.h"

#include <assert.h>
#if __cplusplus <= 199711L && !defined(static_assert)
#define static_assert(a, b) _Static_assert(a, b)
#endif

#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int HttpFetchCounter = 0;

HttpFetch::HttpFetch()
    : m_fdHttp(-1)
    , m_pBuf(NULL)
    , m_statusCode(0)
    , m_pReqBuf(NULL)
    , m_reqBufLen(0)
    , m_reqSent(0)
    , m_pExtraReqHdrs(NULL)
    , m_reqHeaderLen(0)
    , m_iHostLen(0)
    , m_reqState(STATE_NOT_INUSE)
    , m_nonblocking(0)
    , m_enableDriver(0)
    , m_pReqBody(NULL)
    , m_reqBodyLen(0)
    , m_connTimeout(10)
    , m_respBodyLen(-1)
    , m_pRespContentType(NULL)
    , m_respBodyRead(0)
    , m_psProxyServerAddr(NULL)
    , m_pServerAddr(NULL)
    , m_pAdnsReq(NULL)
    , m_callback(NULL)
    , m_callbackArg(NULL)
    , m_iSsl(0)
    , m_iVerifyCert(0)
    , m_pHttpFetchDriver(NULL)
    , m_tmStart(0)
    , m_iTimeoutSec(-1)
    , m_iReqInited(0)
    , m_iEnableDebug(0)
{
    m_pLogger = log4cxx::Logger::getRootLogger();
    m_iLoggerId = HttpFetchCounter ++;
    //m_pLogger->info( "HttpFetch[%d]::HttpFetch()", getLoggerId() );
}


HttpFetch::~HttpFetch()
{
    closeConnection();
    if (m_pBuf)
        delete m_pBuf;
    if (m_pReqBuf)
        free(m_pReqBuf);
    if (m_pExtraReqHdrs)
        free(m_pExtraReqHdrs);
    if (m_pRespContentType)
        free(m_pRespContentType);

    if (m_pAdnsReq)
    {
        setAdnsReq(NULL);
    }
    if (m_psProxyServerAddr)
        free(m_psProxyServerAddr);
    if (m_pServerAddr)
        delete m_pServerAddr;
    if (m_pHttpFetchDriver)
        delete m_pHttpFetchDriver;
    m_respHeaders.release_objects();
    if (m_ssl.getSSL())
        m_ssl.release();
}


void HttpFetch::writeLog(const char *s)
{   m_pLogger->info("HttpFetch[%d]: %s", getLoggerId(), s); }


void HttpFetch::setAdnsReq(AdnsReq *req)
{
    if (m_pAdnsReq == req)
        return;
    if (m_pAdnsReq)
    {
        m_pAdnsReq->setCallback(NULL);
        Adns::release(m_pAdnsReq);
    }
    if (req)
        req->incRefCount();
    m_pAdnsReq = req;
}


void HttpFetch::releaseResult()
{
    if (m_pBuf)
    {
        m_pBuf->close();
        delete m_pBuf;
        m_pBuf = NULL;
    }
    if (m_pRespContentType)
    {
        free(m_pRespContentType);
        m_pRespContentType = NULL;
    }
    m_respBodyLen = -1;
    m_respHeaders.release_objects();

}


void HttpFetch::reset()
{
    closeConnection();
    if (m_pReqBuf)
    {
        free(m_pReqBuf);
        m_pReqBuf = NULL;
    }
    if (m_pExtraReqHdrs)
    {
        free(m_pExtraReqHdrs);
        m_pExtraReqHdrs = NULL;
    }
    if (m_pAdnsReq)
    {
        setAdnsReq(NULL);
    }
    releaseResult();
    m_reqState      = STATE_NOT_INUSE;
    m_statusCode    = 0;
    m_reqBufLen     = 0;
    m_reqSent       = 0;
    m_reqHeaderLen  = 0;
    m_iHostLen      = 0;
    m_reqBodyLen    = 0;
    m_iReqInited    = 0;
    if (m_pServerAddr)
    {
        delete m_pServerAddr;
        m_pServerAddr = NULL;
    }
    if (m_ssl.getSSL())
        m_ssl.release();
}


int HttpFetch::allocateBuf(const char *pSaveFile)
{
    int ret;
    if (m_pBuf != NULL)
        delete m_pBuf;
    m_pBuf = new VMemBuf();
    if (!m_pBuf)
        return -1;
    if (pSaveFile)
    {
        ret = m_pBuf->set(pSaveFile, 8192);
        if (ret == 0)
        {
            if (m_iEnableDebug)
                m_pLogger->info("HttpFetch[%d]::allocateBuf File %s created.",
                                 getLoggerId(), pSaveFile);
        }
        else
            m_pLogger->error("HttpFetch[%d]::failed to create file %s: %s.",
                             getLoggerId(), pSaveFile, strerror(errno));
    }
    else
        ret = m_pBuf->set(VMBUF_ANON_MAP, 8192);
    return ret;

}


int HttpFetch::initReq(const char *pURL, const char *pBody, int bodyLen,
                       const char *pSaveFile, const char *pContentType)
{
    if (m_iReqInited)
        return 0;
    m_iReqInited = 1;

    if (m_fdHttp != -1)
        return -1;

    m_pReqBody = pBody;
    if (m_pReqBody && bodyLen > 0)
    {
        m_reqBodyLen = bodyLen;
        if (buildReq("POST", pURL, pContentType) == -1)
            return -1;
    }
    else
    {
        m_reqBodyLen = 0;
        if (buildReq("GET", pURL) == -1)
            return -1;
    }

    if (allocateBuf(pSaveFile) == -1)
        return -1;
    if (m_iEnableDebug)
        m_pLogger->info("HttpFetch[%d]::intiReq url=%s", getLoggerId(), pURL);
    return 0;
}


int HttpFetch::asyncDnsLookupCb(void *arg, const long lParam, void *pParam)
{
    //fprintf(stderr, "asyncDnsLookupCb for %p\n", arg);
    HttpFetch *pFetch = (HttpFetch *)arg;
    if (!pFetch)
        return 0;
    //NOTE: pFetch->m_pAdnsReq could be NULL if callback is invoked within
    //  Adns::getHostByName() -> checkDnsEvents()
    //  m_pAdnsReq was not set yet as it is set when getHostByName() return

    Adns::setResult(pFetch->m_pServerAddr->get(), pParam, lParam);
    LS_DBG(pFetch->m_pLogger, "[HttpFetch] [%d]: asyncDnsLookupCb '%s', "
            "reqState: %d, %p, %ld",
            pFetch->getLoggerId(), pFetch->m_pAdnsReq->getName(),
            pFetch->m_reqState, pParam, lParam);
    pFetch->setAdnsReq(NULL);

    if (lParam > 0)
    {
        pFetch->startProcess();
    }
    else
        pFetch->endReq(ERROR_DNS_FAILURE);

    return 0;
}


int HttpFetch::startDnsLookup(const char *addrServer)
{
    m_reqState = STATE_DNS;
    if (m_pServerAddr)
        delete m_pServerAddr;
    m_pServerAddr = new GSockAddr();
    int flag = NO_ANY;
    int ret;
    if (!m_nonblocking)
    {
        flag |= DO_NSLOOKUP_DIRECT;
        ret = m_pServerAddr->set(PF_INET, addrServer, flag);
    }
    else
    {
        AdnsReq *req = NULL;
        ret = m_pServerAddr->asyncSet(PF_INET, addrServer, flag,
                                      asyncDnsLookupCb, this, &req);
        LS_DBG(m_pLogger, "[HttpFetch] [%d]: Async DNS lookup '%s' with flag:%d return %d",
               getLoggerId(), addrServer, flag, ret);
        if (req)
        {
            if (m_reqState != STATE_DNS)
            {
                LS_DBG(m_pLogger, "[HttpFetch] [%d]: Async DNS lookup '%s', "
                       "reqState: %d, asyncDnsLookupCb called, release AdnsReq %p",
                        getLoggerId(), addrServer, m_reqState, m_pAdnsReq);
                return 0;
            }
            setAdnsReq(req);
            //fprintf(stderr, "create AdnsReq %p for HttpFetch: %p\n",
            //        m_pAdnsReq, this);
        }
    }
    if (ret == 0)
        return startProcess();
    else if (ret == -1)
        endReq(ERROR_DNS_FAILURE);

    return ret;
}


static SSL *getSslContext()
{
    static SslContext *s_pFetchCtx = NULL;
    if (!s_pFetchCtx)
    {
        s_pFetchCtx = new SslContext();
        if (s_pFetchCtx)
        {
            s_pFetchCtx->enableClientSessionReuse();
            s_pFetchCtx->setRenegProtect(0);
            //NOTE: Turn off TLSv13 for now, need to figure out 0-RTT
            s_pFetchCtx->setProtocol(14);
            //s_pFetchCtx->setCipherList();
        }
        else
            return NULL;
    }
    return s_pFetchCtx->newSSL();
}


void HttpFetch::setSSLAgain()
{
    if (m_ssl.wantRead())
        m_pHttpFetchDriver->switchWriteToRead();
    if (m_ssl.wantWrite())
        m_pHttpFetchDriver->switchReadToWrite();
}


X509 *HttpFetch::getSslCert() const
{
    if (m_ssl.getSSL())
        return SSL_get_peer_certificate(m_ssl.getSSL());
    return NULL;
}


int HttpFetch::verifyDomain()
{
    X509 *pCert;
    X509_NAME *pName;
    BIO *pBio;
    char out[512];
    int len;
    pCert = SSL_get_peer_certificate(m_ssl.getSSL());
    pName = X509_get_subject_name(pCert);
    pBio = BIO_new(BIO_s_mem());
    if ((len = X509_NAME_print_ex(pBio, pName, 0, 0)) != 1)
        return -1;
    len = BIO_read(pBio, out, 511);
    BIO_free(pBio);
    if ((len - 3 != m_iHostLen) || (memcmp(out + 3, m_achHost, m_iHostLen)))
        return -1;
    return 0;
}


int HttpFetch::connectSSL()
{
    if (!m_ssl.getSSL())
    {
        m_ssl.setSSL(getSslContext());
        if (!m_ssl.getSSL())
            return LS_FAIL;
        m_ssl.setfd(m_fdHttp);
        if (m_iVerifyCert)
        {
            // verify
            const char *pCAFile = SslUtil::getDefaultCAFile();
            const char *pCAPath = SslUtil::getDefaultCAPath();
            SSL_set_verify(m_ssl.getSSL(),
                           SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
            SSL_CTX *pCtx = SSL_get_SSL_CTX(m_ssl.getSSL());
            SSL_CTX_load_verify_locations(pCtx, pCAFile, pCAPath);
        }
        char ch = m_achHost[m_iHostLen];
        m_achHost[m_iHostLen] = 0;
        m_ssl.setTlsExtHostName(m_achHost);
        m_achHost[m_iHostLen] = ch;
        m_ssl.tryReuseCachedSession(m_achHost, m_iHostLen);
    }
    int ret = m_ssl.connect();
    int verifyOk;
    switch (ret)
    {
    case 0:
        setSSLAgain();
        break;
    case 1:
        if (m_iVerifyCert
            && (!(verifyOk = m_ssl.isVerifyOk()) || (verifyDomain() == -1)))
        {
            if (!verifyOk)
                ret = ERROR_SSL_CERT_VERIFY;
            else
                ret = ERROR_SSL_UNMATCH_DOMAIN;
            LS_ERROR(m_pLogger, "HttpFetch[%d]:: [SSL] Verify failed.",
                getLoggerId());
            cancel();
            break;
        }
        LS_DBG_M(m_pLogger, "HttpFetch[%d]:: [SSL] connected, session reuse: %d",
                 getLoggerId(), m_ssl.isSessionReused());
        break;
    default:
        if (!m_ssl.isVerifyOk())
        {
            LS_DBG_L(m_pLogger, "HttpFetch[%d]:: SSL Verify failed.",
                    getLoggerId());
            cancel();
        }
        else if (errno == EIO)
            LS_DBG_M(m_pLogger, "HttpFetch[%d]:: SSL_connect() failed!: %s",
                    getLoggerId(), SslError().what());

        break;
    }

    return ret;
}


int HttpFetch::startReq(const char *pURL, int nonblock, int enableDriver,
                        const char *pBody, int bodyLen, const char *pSaveFile,
                        const char *pContentType, const char *addrServer)
{
    if (initReq(pURL, pBody, bodyLen, pSaveFile, pContentType) != 0)
        return -1;
    m_nonblocking = nonblock;
    m_enableDriver = enableDriver;

    if (m_pServerAddr)
        return startReq(pURL, nonblock, enableDriver, pBody, bodyLen, pSaveFile,
                        pContentType, *m_pServerAddr);
    if (m_iEnableDebug)
        m_pLogger->info("HttpFetch[%d]:: Host is [%s]", getLoggerId(), m_achHost);

    if (addrServer == NULL)
        addrServer = m_achHost;
    return startDnsLookup(addrServer);
}


int HttpFetch::startReq(const char *pURL, int nonblock, int enableDriver,
                        const char *pBody, int bodyLen, const char *pSaveFile,
                        const char *pContentType, const GSockAddr &sockAddr)
{
    if (initReq(pURL, pBody, bodyLen, pSaveFile, pContentType) != 0)
        return -1;
    m_nonblocking = nonblock;
    m_enableDriver = (enableDriver || isUseSsl());

    if(m_pServerAddr != &sockAddr )
    {
        if (m_pServerAddr)
            delete m_pServerAddr;
        m_pServerAddr = new GSockAddr(sockAddr);
    }
    return startProcess();
}


int HttpFetch::startProcess()
{
    if (m_enableDriver && NULL == m_pHttpFetchDriver)
        m_pHttpFetchDriver = new HttpFetchDriver(this);
    if (m_iTimeoutSec <= 0)
        m_iTimeoutSec = 60;
    m_tmStart = time(NULL);

    return startProcessReq(*m_pServerAddr);

}


int HttpFetch::buildReq(const char *pMethod, const char *pURL,
                        const char  *pContentType)
{
    int len = strlen(pURL) + 200 + m_reqHeaderLen;
    int extraLen = m_reqHeaderLen;
    char *pReqBuf;
    const char *pURI;
    if ((strncasecmp(pURL, "http://", 7) != 0) &&
        (strncasecmp(pURL, "https://", 8) != 0))
        return -1;
    if (pContentType == NULL)
        pContentType = "application/x-www-form-urlencoded";
    const char *p = pURL + 7;
    if (*(pURL + 4) != ':')
    {
        m_iSsl = 1;
        ++p;
    }
    pURI = strchr(p, '/');
    if (!pURI)
        return -1;
    if (pURI - p > 250)
        return -1;
    m_iHostLen = pURI - p;
    memcpy(m_achHost, p, m_iHostLen);
    m_achHost[ m_iHostLen ] = 0;
    if (m_iHostLen == 0)
        return -1;
    if (m_reqBufLen < len)
    {
        pReqBuf = (char *)realloc(m_pReqBuf, len);
        if (pReqBuf == NULL)
            return -1;
        m_reqBufLen = len;
        m_pReqBuf = pReqBuf;
    }
    m_reqHeaderLen = lsnprintf(m_pReqBuf, len,
                                   "%s %s HTTP/1.0\r\n"
                                   "Host: %s\r\n"
                                   "Connection: Close\r\n",
                                   pMethod, pURI, m_achHost
                                  );
    if (m_pExtraReqHdrs != NULL)
    {
        int trim = 0;
        if (extraLen >= 2
                && *(m_pExtraReqHdrs + extraLen - 2) == '\r'
                && *(m_pExtraReqHdrs + extraLen - 1) == '\n')
            trim = 2;

        m_reqHeaderLen += lsnprintf(m_pReqBuf + m_reqHeaderLen,
                                    len - m_reqHeaderLen,
                                    "%.*s\r\n",
                                    extraLen - trim, m_pExtraReqHdrs);
    }
    if (m_reqBodyLen > 0)
    {
        m_reqHeaderLen += lsnprintf(m_pReqBuf + m_reqHeaderLen,
                                        len - m_reqHeaderLen,
                                        "Content-Type: %s\r\n"
                                        "Content-Length: %lld\r\n",
                                        pContentType,
                                        (long long)m_reqBodyLen);
    }
    strcpy(m_pReqBuf + m_reqHeaderLen, "\r\n");
    if (strchr(m_achHost, ':') == NULL)
        lstrncat(m_achHost, (m_iSsl ? ":443" : ":80"), sizeof(m_achHost));
    m_reqHeaderLen += 2;
    m_reqSent = 0;
    if (m_iEnableDebug)
        m_pLogger->info("HttpFetch[%d]::buildReq Host=%s [0]", getLoggerId(),
                        m_achHost);
    return 0;

}


int HttpFetch::pollEvent(int evt, int timeoutSecs)
{
    struct pollfd  pfd;
    pfd.events = evt;
    pfd.fd = m_fdHttp;
    pfd.revents = 0;

    return poll(&pfd, 1, timeoutSecs * 1000);

}


int HttpFetch::startProcessReq(const GSockAddr &sockAddr)
{
    m_reqState = STATE_CONNECTING;

    if (m_iEnableDebug)
        m_pLogger->info("HttpFetch[%d]::startProcessReq sockAddr=%s",
                         getLoggerId(), sockAddr.toString());
    int ret = CoreSocket::connect(sockAddr, O_NONBLOCK, &m_fdHttp);
    if (m_fdHttp == -1)
        return -1;
    ::fcntl(m_fdHttp, F_SETFD, FD_CLOEXEC);
    startDriver();

    if (!m_nonblocking)
    {
        if ((ret = pollEvent(POLLIN|POLLOUT, m_connTimeout)) != 1)
        {
            closeConnection();
            return ERROR_CONN_TIMEOUT;
        }
        else
        {
            int error;
            socklen_t len = sizeof(int);
            ret = getsockopt(m_fdHttp, SOL_SOCKET, SO_ERROR, &error, &len);
            if ((ret == -1) || (error != 0))
            {
                if (ret != -1)
                    errno = error;
                endReq(ERROR_CONN_FAILURE);
                return -1;
            }
        }
        int val = fcntl(m_fdHttp, F_GETFL, 0);
        fcntl(m_fdHttp, F_SETFL, val & (~O_NONBLOCK));
    }
    if (ret == 0)
    {
        m_reqState = STATE_CONNECTED; //connected, sending request header
        ret = sendReq();
    }
    else
    {
        m_reqState = STATE_CONNECTING; //connecting
        ret = 0;
    }
    if (m_iEnableDebug)
        m_pLogger->info("HttpFetch[%d]::startProcessReq ret=%d state=%d",
                         getLoggerId(), ret, m_reqState);
    return ret;
}


short HttpFetch::getPollEvent() const
{
    if (m_reqState <= STATE_SENT_REQ_HEADER)
        return POLLOUT;
    else
        return POLLIN;
}


int HttpFetch::sendReq()
{
    int error;
    int ret = 0;
    switch (m_reqState)
    {
    case STATE_CONNECTING: //connecting
        {
            socklen_t len = sizeof(int);
            ret = getsockopt(m_fdHttp, SOL_SOCKET, SO_ERROR, &error, &len);
        }
        if ((ret == -1) || (error != 0))
        {
            if (ret != -1)
                errno = error;
            endReq(ERROR_CONN_FAILURE);
            return -1;
        }
        m_reqState = STATE_CONNECTED;
    //fall through
    case STATE_CONNECTED:
        if ((m_iSsl) && (!m_ssl.isConnected()))
        {
            ret = connectSSL();
            if (ret != 1)
                return ret;
        }
        if (m_iSsl)
            ret = m_ssl.write(m_pReqBuf + m_reqSent,
                            m_reqHeaderLen - m_reqSent);
        else
            ret = ::nio_write(m_fdHttp, m_pReqBuf + m_reqSent,
                            m_reqHeaderLen - m_reqSent);
        //write( 1, m_pReqBuf + m_reqSent, m_reqHeaderLen - m_reqSent );
        if (m_iEnableDebug)
            m_pLogger->info("HttpFetch[%d]::sendReq::nio_write fd=%d len=%d [%d - %d] ret=%d",
                             getLoggerId(), m_fdHttp,
                             m_reqHeaderLen - m_reqSent, m_reqHeaderLen, m_reqSent, ret);
        if (ret > 0)
            m_reqSent += ret;
        else if (ret == -1)
        {
            if (errno != EAGAIN)
            {
                endReq(ERROR_HTTP_SEND_REQ_HEADER_FAILURE);
                return -1;
            }
        }
        if (m_reqSent >= m_reqHeaderLen)
        {
            m_reqState = STATE_SENT_REQ_HEADER;
            m_reqSent = 0;
        }
        else
            break;
    //fall through
    case STATE_SENT_REQ_HEADER: //send request body
        if (m_reqBodyLen)
        {
            if (m_reqSent < m_reqBodyLen)
            {
                if (m_iSsl)
                    ret = m_ssl.write(m_pReqBody + m_reqSent,
                                  m_reqBodyLen - m_reqSent);
                else
                    ret = ::nio_write(m_fdHttp, m_pReqBody + m_reqSent,
                                  m_reqBodyLen - m_reqSent);
                //write( 1, m_pReqBody + m_reqSent, m_reqBodyLen - m_reqSent );
                if (ret == -1)
                {
                    if (errno != EAGAIN)
                    {
                        endReq(ERROR_HTTP_SEND_REQ_BODY_FAILURE);
                    }
                    return -1;
                }
                if (ret > 0)
                    m_reqSent += ret;
                if (m_reqSent < m_reqBodyLen)
                    break;
            }
        }
        m_resHeaderBuf.clear();
        m_reqState = STATE_SENT_REQ_BODY;
        if (m_enableDriver)
            m_pHttpFetchDriver->switchWriteToRead();
    }
    if ((ret == -1) && (errno != EAGAIN))
        return -1;
    return 0;
}


int HttpFetch::getLine(char *&p, char *pEnd,
                       char *&pLineBegin, char *&pLineEnd)
{
    int ret = 0;
    pLineBegin = NULL;
    pLineEnd = (char *)memchr(p, '\n', pEnd - p);
    if (!pLineEnd)
    {
        if (pEnd - p + m_resHeaderBuf.size() < 8192)
        {
            m_resHeaderBuf.append(p, pEnd - p);
        }
        else
            ret = -1;
        p = pEnd;
    }
    else
    {
        if (m_resHeaderBuf.size() == 0)
        {
            pLineBegin = p;
            p = pLineEnd + 1;
            if (m_iEnableDebug)
                m_pLogger->info("HttpFetch[%d] pLineBegin: %p, pLineEnd: %p",
                                getLoggerId(), pLineBegin, pLineEnd);

        }
        else
        {
            if (pLineEnd - p + m_resHeaderBuf.size() < 8192)
            {
                m_resHeaderBuf.append(p, pLineEnd - p);
                p = pLineEnd + 1;
                pLineBegin = m_resHeaderBuf.begin();
                pLineEnd = m_resHeaderBuf.end();
                m_resHeaderBuf.clear();
                if (m_iEnableDebug)
                    m_pLogger->info("HttpFetch[%d] Combined pLineBegin: %p, pLineEnd: %p",
                                    getLoggerId(), pLineBegin, pLineEnd);
            }
            else
            {
                if (m_iEnableDebug)
                    m_pLogger->info("HttpFetch[%d] line too long, fail request.",
                                    getLoggerId());
                ret = -1;
                p = pEnd;
            }
        }
        if (pLineBegin)
        {
            if (*(pLineEnd - 1) == '\r')
                --pLineEnd;
            *pLineEnd = 0;
        }
    }
    return ret;
}


void HttpFetch::addRespHeader(char *pLineBegin, char *pLineEnd)
{
    char *p = (char *)memchr(pLineBegin, ':', pLineEnd - pLineBegin);
    if (!p)
        return;
    char *pVal = p + 1;
    while(p > pLineBegin && isspace(*(p - 1)))
        --p;
    *p = 0;

    while(pVal < pLineEnd && isspace(*pVal))
        ++pVal;

    if (pVal == pLineEnd)
        return;
    int n = p - pLineBegin;
    StringTool::strnlower(pLineBegin, pLineBegin, n);
    m_respHeaders.insert_update(pLineBegin, pVal);
}


const char *HttpFetch::getRespHeader(const char *pName) const
{
    StrStrHashMap::iterator iter = m_respHeaders.find(pName);
    if (iter != m_respHeaders.end())
        return iter.second()->str2.c_str();
    return NULL;
}


int HttpFetch::recvResp()
{
    int ret = 0;
    int len;
    char *p;
    char *pEnd;
    char *pLineBegin;
    char *pLineEnd;
    AutoBuf buf(8192);
    if ((m_iSsl) && (!m_ssl.isConnected()))
    {
        ret = connectSSL();
        if (ret != 1)
            return ret;
        m_pHttpFetchDriver->continueWrite();
        return 0;
    }
    while (m_statusCode >= 0)
    {
        ret = pollEvent(POLLIN, 1);
        if (ret != 1)
            break;
        if (m_iSsl)
            ret = m_ssl.read(buf.begin(), 8192);
        else
            ret = ::nio_read(m_fdHttp, buf.begin(), 8192);
        if (m_iEnableDebug)
            m_pLogger->info("HttpFetch[%d]::recvResp fd=%d ret=%d",
                            getLoggerId(), m_fdHttp, ret);
        if (ret == 0)
        {
            if (errno == EWOULDBLOCK)
                break;
            if (m_respBodyLen == -1)
                return endReq(0);
            else
            {
                endReq(ERROR_HTTP_RECV_RESP_FAILURE);
                return -1;
            }
        }
        else if (ret < 0)
        {
            if ((m_respBodyLen == -1)
                && ((m_iSsl && (errno == ECONNRESET)) || !m_nonblocking))
                return endReq(0);
            break;
        }
        p = buf.begin();
        pEnd = p + ret;
        while (p < pEnd)
        {
            switch (m_reqState)
            {
            case STATE_WAIT_RESP: //waiting response status line
                if (getLine(p, pEnd, pLineBegin, pLineEnd))
                {
                    endReq(ERROR_HTTP_PROTOL_ERROR);
                    return -1;
                }
                if (pLineBegin)
                {
                    if (memcmp(pLineBegin, "HTTP/1.", 7) != 0)
                    {
                        endReq(ERROR_HTTP_PROTOL_ERROR);
                        return -1;
                    }
                    pLineBegin += 9;
                    if (!isdigit(*pLineBegin) || !isdigit(*(pLineBegin + 1)) ||
                        !isdigit(*(pLineBegin + 2)))
                    {
                        endReq(ERROR_HTTP_PROTOL_ERROR);
                        return -1;
                    }
                    m_statusCode = (*pLineBegin     - '0') * 100 +
                                   (*(pLineBegin + 1) - '0') * 10 +
                                   *(pLineBegin + 2) - '0';
                    m_reqState = STATE_RCVD_STATUS;
                }
                else
                    break;
            //fall through
            case STATE_RCVD_STATUS: //waiting response header
                if (getLine(p, pEnd, pLineBegin, pLineEnd))
                {
                    endReq(ERROR_HTTP_PROTOL_ERROR);
                    return -1;
                }
                if (pLineBegin)
                {
                    if (strncasecmp(pLineBegin, "Content-Length:", 15) == 0)
                        m_respBodyLen = strtol(pLineBegin + 15, (char **)NULL, 10);
                    else if (strncasecmp(pLineBegin, "Content-Type:", 13) == 0)
                    {
                        pLineBegin += 13;
                        while ((pLineBegin < pLineEnd) && isspace(*pLineBegin))
                            ++pLineBegin;
                        if (pLineBegin < pLineEnd)
                        {
                            if (m_pRespContentType)
                                free(m_pRespContentType);
                            m_pRespContentType = strdup(pLineBegin);
                        }
                    }
                    else if (pLineBegin == pLineEnd)
                    {
                        m_reqState = STATE_RCVD_RESP_HEADER;
                        m_respBodyRead = 0;
                        if ((m_respBodyLen == 0) || (m_respBodyLen < -1))
                            return endReq(0);

                    }
                    else
                    {
                        addRespHeader(pLineBegin, pLineEnd);
                    }

                }
                break;
            case STATE_RCVD_RESP_HEADER: //waiting response body
                if ((len = m_pBuf->write(p, pEnd - p)) == -1)
                {
                    endReq(ERROR_HTTP_RECV_RESP_FAILURE);
                    return -1;
                }
                if ((m_respBodyLen > 0) && (m_pBuf->writeBufSize() >= m_respBodyLen))
                    return endReq(0);
                p += len;
                break;
            }
        }
    }
    if ((ret == -1) && (errno != EAGAIN) && (errno != ETIMEDOUT))
        return -1;
    return 0;
}


int HttpFetch::endReq(int res)
{
    if (m_pBuf)
    {
        if (res == 0)
        {
            m_reqState = STATE_RCVD_RESP_BODY;
            if (m_pBuf->getfd() != -1)
                m_pBuf->exactSize();
        }
//        m_pBuf->close();
    }
    if (res != 0)
    {
        m_reqState = STATE_ERROR;
        m_statusCode = res;
    }
    if (m_fdHttp != -1)
        closeConnection();
    if (m_callback != NULL)
    {
        m_reqState = STATE_RUN_CALLBACK;
        (*m_callback)(m_callbackArg, this);
    }
    m_reqState = STATE_FINISH;
    return 0;
}


int HttpFetch::cancel()
{
    return endReq(ERROR_CANCELED);
}


int HttpFetch::processEvents(short revent)
{
    if (revent & POLLOUT)
        if (sendReq() == -1)
            return -1;
    if ((m_fdHttp != -1) && (revent & POLLIN))
        if (recvResp() == -1)
            return -1;
    if (revent & POLLERR)
        endReq(ERROR_SOCKET_ERROR);
    return 0;
}


int HttpFetch::process()
{
    while ((m_fdHttp != -1) && (m_reqState < STATE_SENT_REQ_BODY))
        sendReq();
    while ((m_fdHttp != -1) && (m_reqState < STATE_RCVD_RESP_BODY)
        && m_tmStart + m_iTimeoutSec > time(NULL))
        recvResp();
    // req may be complete at this point if fetch is blocking and recvResp read returned -1.
    // This will return -1 in that case because upon completion, the fetch fd is set to -1 and reqState is 0.
    return (m_reqState == STATE_RCVD_RESP_BODY) ? 0 : -1;
}


void HttpFetch::setProxyServerAddr(const char *pAddr)
{
    if (m_pServerAddr)
    {
        delete  m_pServerAddr;
        m_pServerAddr = NULL;
    }

    if (pAddr)
    {
        m_pServerAddr = new GSockAddr();
        if (m_pServerAddr->set(pAddr, NO_ANY | DO_NSLOOKUP) == -1)
        {
            delete m_pServerAddr;
            m_pServerAddr = NULL;
            m_pLogger->error("HttpFetch[%d] invalid proxy server address '%s', ignore.", getLoggerId(), pAddr);
            return;
        }
        if (m_iEnableDebug)
            m_pLogger->info("HttpFetch[%d]::setProxyServerAddr %s",
                            getLoggerId(), pAddr);
        if (m_psProxyServerAddr)
            free(m_psProxyServerAddr);
        m_psProxyServerAddr = strdup(pAddr);
    }
}


void HttpFetch::closeConnection()
{
    if (m_fdHttp != -1)
    {
        if (m_iSsl && m_ssl.getSSL())
        {
            LS_DBG_L("Shutdown HttpFetch SSL ...");
            m_ssl.shutdown(0);
            m_ssl.setfd(-1);
        }

        if (m_iEnableDebug)
            m_pLogger->info("HttpFetch[%d]::closeConnection fd=%d ", getLoggerId(), m_fdHttp);
        close(m_fdHttp);
        m_fdHttp = -1;
        stopDriver();
    }
}


void HttpFetch::startDriver()
{
    if (m_pHttpFetchDriver)
    {
        m_pHttpFetchDriver->setfd(m_fdHttp);
        m_pHttpFetchDriver->start();
    }
}


void HttpFetch::stopDriver()
{
    if (m_pHttpFetchDriver)
    {
        m_pHttpFetchDriver->stop();
        m_pHttpFetchDriver->setfd(-1);
    }
}


int HttpFetch::setExtraHeaders(const char* pHdrs, int len)
{
    if (m_pExtraReqHdrs)
        free(m_pExtraReqHdrs);
    m_pExtraReqHdrs = strndup(pHdrs, len);
    if (m_pExtraReqHdrs == NULL)
        return -1;
    m_reqHeaderLen = len;
    return 0;
}


int HttpFetch::appendExtraHeaders(const char* pHdrs, int len)
{
    m_pExtraReqHdrs = (char *)realloc(m_pExtraReqHdrs, m_reqHeaderLen + len);
    if (m_pExtraReqHdrs == NULL)
    {
        m_reqHeaderLen = 0;
        return -1;
    }
    memmove(m_pExtraReqHdrs + m_reqHeaderLen, pHdrs, len);
    m_reqHeaderLen += len;
    return 0;
}


int HttpFetch::syncFetch(const char *pURL, char *pRespBuf, int respBufLen, int timeout,
                    const char *pReqBody, int reqBodyLen, const char *pContentType)
{
    int ret;
    // isSsl is set by buildReq if the URL starts with https.
    // Extra headers are also not added.
    setTimeout(timeout);
    ret = startReq(pURL, 0, 1, pReqBody, reqBodyLen, NULL, pContentType);
    if (ret != 0)
        return ret;
    ret = process();
    if (ret != 0 && m_statusCode <= 0)
    {
        LS_DBG_L("syncFetch process returned %d, status code %d. Do not continue.",
                 ret, m_statusCode);
        return -1;
    }

    if (0 == respBufLen)
        return 0; // ok, did not want body.

    ret = m_pBuf->copyToBuf(pRespBuf, 0, respBufLen);
    if (ret <= 0)
        return ret;
    m_pBuf->readUsed(ret);
    return ret;
}
