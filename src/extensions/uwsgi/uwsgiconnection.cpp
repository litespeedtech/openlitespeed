/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2025  LiteSpeed Technologies, Inc.                 *
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
#include "uwsgiconnection.h"
#include "uwsgiapp.h"

#include <http/httpcgitool.h>
#include <http/httpextconnector.h>
#include <http/httpresourcemanager.h>
#include <http/httpsession.h>
#include <http/httpstatuscode.h>
#include <http/requestvars.h>
#include <log4cxx/logger.h>
#include <util/iovec.h>
#include <util/stringtool.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

UwsgiConnection::UwsgiConnection()
    :   m_fd(-1)
    ,   m_sentHeader(false)
    ,   m_iTotalPending(0)
    ,   m_reqBodyLen(0)
    ,   m_waitingBodyLength(false)
{
    LS_DBG(this, "UwsgiConnection::UwsgiConnection %p\n", this);
}


UwsgiConnection::~UwsgiConnection()
{
    LS_DBG("UwsgiConnection::~UwsgiConnection %p\n", this);
}


void UwsgiConnection::init(int fd, Multiplexer *pMplx)
{
    LS_DBG(this, "UwsgiConnection:init(%d, %p) %p\n", fd, pMplx, this);
    m_fd = fd;
    EdStream::init(fd, pMplx, POLLIN | POLLOUT | POLLHUP | POLLERR);
}


/**
  * @return -1 error, > 0 success.
  *
  */

int UwsgiConnection::endOfStream()
{
    LS_DBG(this, "UwsgiConnection, endOfStream\n");
    return 0;
}


int UwsgiConnection::sendNetString(const char *data, off_t len)
{
    if (!len)
    {
        LS_DBG(this, "UwsgiConnection, sendNetString no data, send nothing\n");
        return 1;
    }
    m_header[0] = 0;
    m_header[1] = (uint8_t)(len & 0xff);
    m_header[2] = (uint8_t)((len >> 8) & 0xff);
    m_header[3] = 0;
    LS_DBG(this, "queue uwsgi header %d bytes, %p", (int)len, data);
    m_iovec.clear();
    m_iovec.append(m_header, 4);
    m_iovec.append(data, len);
    m_iTotalPending = len + 4;
    return 1;
}


int UwsgiConnection::writeBody(const char *blk, int len)
{
    LS_DBG(this, "UwsgiConnection::writeBody %d bytes", len);
    if (len <= 0)
        return 0;
    if (m_iTotalPending > 0)
    {
        m_iovec.append(blk, len);
        int total = m_iTotalPending + len;
        int ret = writev(m_iovec.get(), m_iovec.len());
        m_iovec.pop_back(1);
        if (ret > 0)
        {
            if (ret >= total)
            {
                m_iTotalPending = 0;
                m_iovec.clear();
                m_uwsgiEnv.clear();
                return len;
            }
            if (ret >= m_iTotalPending)
            {
                ret -= m_iTotalPending;
                m_iTotalPending = 0;
                m_iovec.clear();
                m_uwsgiEnv.clear();
                return ret;
            }
            m_iovec.finish(ret);
            m_iTotalPending -= ret;
            return 0;
        }
        return ret;
    }
    return write(blk, len);
}


int UwsgiConnection::doRead()
{
    LS_DBG_L(this, "UwsgiConnection::doRead() %p", this);
    return processUwsgiData();
}


int UwsgiConnection::doWrite()
{
    LS_DBG_L(this, "UwsgiConnection::doWrite()");
    if (m_iTotalPending > 0)
        return flush();
    if (m_waitingBodyLength)
    {
        LS_DBG_L(this, "waiting body length, do nothing");
    }
    else if (getConnector())
    {
        int state = getConnector()->getState();
        if ((!state) || (state & (HEC_FWD_REQ_HEADER | HEC_FWD_REQ_BODY)))
            return getConnector()->extOutputReady();
    }
    suspendWrite();
    return 0;
}


int UwsgiConnection::doError(int err)
{
    LS_DBG_L(this, "UwsgiConnection::doError(%d)\n", err);
    return 0;
}


#define UWSGI_INPUT_BUFSIZE GLOBAL_BUF_SIZE
int UwsgiConnection::processUwsgiData()
{
    int len, statusCode = 0;
    do
    {
        len = read(HttpResourceManager::getGlobalBuf(),
                   UWSGI_INPUT_BUFSIZE);
        int err = errno;
        LS_DBG_H(this, "processUwsgiData Read %d bytes from Uwsgi.", len);
        if (!len)
        {
            LS_DBG_H(this, "Read returned 0, try again later");
            return 0;
        }
        else if (len == -1 && err == ECONNRESET)
        {
            LS_DBG(this, "len: %d, connection closed\n", len);
            close();
            break;
        }
        if (len == -1)
            return len;
        char *pCur = HttpResourceManager::getGlobalBuf();
        HttpExtConnector *pConnector = getConnector();
        assert(pConnector);
        LS_DBG_M("Process STDOUT %d bytes\n", len);
        int ret = pConnector->processRespData(pCur, len);
        LS_DBG_M("processRespData: %d, state: %d\n", ret, getState());
    }
    while (len > 0);
    close();
    incReqProcessed();
    if (getState() == ABORT)
    {
        setState( PROCESSING );
    }
    else if (getConnector())
    {
        getConnector()->flushResp();
        statusCode = getConnector()->getHttpSession()->getReq()->getStatusCode();
        setState(CLOSING);
    }
    LS_DBG_M("exit processUwsgiData: state: %d, statusCode: %d\n", getState(), statusCode);
    return 0;
}


#define UWSGI_MAX_CONNS  "UWSGI_MAX_CONNS"
#define UWSGI_MAX_REQS   "UWSGI_MAX_REQS"
#define UWSGI_MPXS_CONNS "UWSGI_MPXS_CONNS"

bool UwsgiConnection::wantRead()
{
    return true;
}


bool UwsgiConnection::wantWrite()
{
    return m_iTotalPending != 0;
}


int UwsgiConnection::addRequest(ExtRequest *pReq)
{
    LS_DBG(this, "UwsgiConnection::addRequest(%p) %p\n", pReq, this);
    m_iovec.clear();
    m_uwsgiEnv.clear();
    m_iTotalPending = 0;
    m_sentHeader = false;
    setConnector((HttpExtConnector *)pReq);
    HttpSession *httpSession = getConnector()->getHttpSession();
    HttpReq *httpReq = httpSession->getReq();
    if (httpReq->isChunked() && httpReq->getContentLength() == -1)
    {
        LS_DBG(this, "UwsgiConnection, set waitFullReqBody");
        m_waitingBodyLength = true;
        httpSession->setWaitFullReqBody();
    }
    else
    {
        LS_DBG(this, "UwsgiConnection contentLength: %ld", httpReq->getContentLength());
        m_waitingBodyLength = false;
    }
    return 0;
}


ExtRequest *UwsgiConnection::getReq() const
{
    return getConnector();
}


int UwsgiConnection::removeRequest(ExtRequest *pReq)
{
    LS_DBG(this, "UwsgiConnection::removeRequest(%p) %p\n", pReq, this);
    if (getConnector())
    {
        getConnector()->setProcessor(NULL);
        setConnector(NULL);
    }
    return 0;
}


void UwsgiConnection::finishRecvBuf()
{
    LS_DBG(this, "UwsgiConnection::finishRecvBuf() %p\n", this);
    if (m_waitingBodyLength)
    {
        LS_DBG_L(this, "waiting body length, do nothing");
        return;
    }
    processUwsgiData();
}


int UwsgiConnection::readStdOut(char *pBuf, int size)
{
    LS_DBG(this, "UwsgiConnection::readStdOut() %p\n", this);
    return read(pBuf, size);
}


void UwsgiConnection::suspendWrite()
{
    LS_DBG_L(this, "UwsgiConnection::suspendWrite()");
    if (m_iTotalPending == 0)
        EdStream::suspendWrite();
}


int  UwsgiConnection::sendReqHeader()
{
    HttpReq *pReq = getConnector()->getHttpSession()->getReq();
    m_reqBodyLen = pReq->getContentLength();
    LS_DBG(this, "UwsgiConnection::sendReqHeader(), content_length: %ld\n", pReq->getContentLength());
    if (!m_uwsgiEnv.size())
    {
        char strLen[40];
        int len = snprintf(strLen, sizeof(strLen), "%lu", pReq->getContentLength());
        m_uwsgiEnv.add("CONTENT_LENGTH", 14, strLen, len);
        HttpCgiTool::buildEnv(&m_uwsgiEnv, getConnector()->getHttpSession());
    }
    if (!m_sentHeader)
    {
        LS_DBG(this, "UwsgiConnection::sendReqHeader(), send header\n");
        AutoBuf *uwsgiEnvBuf = m_uwsgiEnv.get();
        sendNetString(uwsgiEnvBuf->begin(), uwsgiEnvBuf->size());
        m_sentHeader = true;
        setInProcess(1);
    }
    return 1;
}


/**
  * @return 0, connection busy
  *         -1, error
  *         other, bytes sent, conutue
  *
  */

int  UwsgiConnection::sendReqBody(const char *pBuf, int size)
{
    return writeBody(pBuf, size);
}


int UwsgiConnection::beginReqBody()
{
    LS_DBG_M(this, "UwsgiConnection::beginReqBody()");
    return 0;
}


int UwsgiConnection::endOfReqBody()
{
    HttpReq *pReq = getConnector()->getHttpSession()->getReq();
    LS_DBG_M(this, "UwsgiConnection::endOfReqBody() ContentLength: %ld", pReq->getContentLength());
    if (m_iTotalPending)
    {
        int rc = flush();
        if (rc != 0)
            return rc;
    }
    suspendWrite();
    return 0;
}


int  UwsgiConnection::endOfRequest(int endCode, int status)
{
    LS_DBG(this, "UwsgiConnection::endOfRequest( %d, %d)!",
           endCode, status);
    HttpExtConnector *pConnector = getConnector();
    if (!pConnector)
        return 0;
    if (endCode >= SC_400)
    {
        LS_ERROR(this, "UwsgiConnection::endOfRequest( %d, %d)!",
                 endCode, status);
        pConnector->endResponse(SC_500, status);
    }
    else
        pConnector->endResponse(endCode, status);
    return 0;
}


int UwsgiConnection::onStdOut()
{
    HttpExtConnector *pConnector = getConnector();
    if (!pConnector)
        return 0;
    LS_DBG_M(this, "onStdOut()");
    return pConnector->extInputReady();

}


int UwsgiConnection::readResp(char *pBuf, int size)
{
    return readStdOut(pBuf, size);
}


int  UwsgiConnection::processStdOut(char *pBuf, int size)
{
    HttpExtConnector *pConnector = getConnector();
    if (!pConnector)
        return size;
    LS_DBG_M(this, "Process STDOUT %d bytes", size);
    return pConnector->processRespData(pBuf, size);
}


int  UwsgiConnection::processStdErr(char *pBuf, int size)
{
    HttpExtConnector *pConnector = getConnector();
    if (!pConnector)
        return size;
    LS_DBG_M(this, "Process STDERR %d bytes", size);
    return pConnector->processErrData(pBuf, size);
}


void UwsgiConnection::cleanUp()
{
    LS_DBG_M(this, "UwsgiConnection::cleanUp() %p", this);
    removeRequest(getConnector());
    getWorker()->recycleConn(this);
}


void UwsgiConnection::continueWrite()
{
    EdStream::continueWrite();
}


void  UwsgiConnection::abort()
{
    LS_DBG_M(this, "UwsgiConnection::abort()");
    if (m_waitingBodyLength)
    {
        LS_DBG_L(this, "waiting body length, do nothing");
        return;
    }
    setState(ABORT);
}


int UwsgiConnection::begin()
{
    LS_DBG_M(this, "UwsgiConnection::beginRequest()");
    return 1;
}


int UwsgiConnection::connUnavail()
{
    getConnector()->endResponse(SC_500, -1);
    return 0;
}


int  UwsgiConnection::flush()
{
    LS_DBG_M(this, "UwsgiConnection::flush()");
    if (m_iTotalPending)
    {
        int ret = writev(m_iovec.get(), m_iovec.len());
        if (ret >= m_iTotalPending)
        {
            m_iTotalPending = 0;
            m_iovec.clear();
            m_uwsgiEnv.clear();
        }
        else
        {
            if (ret > 0)
            {
                m_iovec.finish(ret);
                m_iTotalPending -= ret;
                return 1;
            }
            if (ret == 0)
                return 1;
            return LS_FAIL;
        }
    }
    return 0;
}


void UwsgiConnection::dump()
{
    LS_INFO("UwsgiConnection watching event: %d, pending:%d",
            getEvents(), m_iTotalPending);
}
