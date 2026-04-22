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
#include "scgiconnection.h"
#include "scgiapp.h"

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

ScgiConnection::ScgiConnection()
    :   m_fd(-1)
    ,   m_sentHeader(false)
    ,   m_iTotalPending(0)
    ,   m_reqBodyLen(0)
    ,   m_waitingBodyLength(false)
{
    LS_DBG(this, "ScgiConnection::ScgiConnection %p\n", this);
}


ScgiConnection::~ScgiConnection()
{
    LS_DBG("ScgiConnection::~ScgiConnection %p\n", this);
}


void ScgiConnection::init(int fd, Multiplexer *pMplx)
{
    LS_DBG(this, "ScgiConnection:init(%d, %p) %p\n", fd, pMplx, this);
    m_fd = fd;
    EdStream::init(fd, pMplx, POLLIN | POLLOUT | POLLHUP | POLLERR);
}


/**
  * @return -1 error, > 0 success.
  *
  */

int ScgiConnection::endOfStream()
{
    LS_DBG(this, "ScgiConnection, endOfStream\n");
    return 0;
}


int ScgiConnection::sendNetString(const char *data, off_t len)
{
    if (!len)
    {
        LS_DBG(this, "ScgiConnection, sendNetString no data, send nothing\n");
        return 1;
    }
    int fieldLen = snprintf(m_header, sizeof(m_header), "%lu:", len);
    LS_DBG(this, "ScgiConnection, queue header netString: %ld bytes\n", len);
    m_iovec.clear();
    m_iovec.append(m_header, fieldLen);
    m_iovec.append(data, len);
    m_iovec.append(",", 1);
    m_iTotalPending = fieldLen + len + 1;
    return 1;
}


int ScgiConnection::writeBody(const char *blk, int len)
{
    LS_DBG(this, "ScgiConnection::writeBody %d bytes", len);
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
                m_scgiEnv.clear();
                return len;
            }
            if (ret >= m_iTotalPending)
            {
                ret -= m_iTotalPending;
                m_iTotalPending = 0;
                m_iovec.clear();
                m_scgiEnv.clear();
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


int ScgiConnection::doRead()
{
    LS_DBG_L(this, "ScgiConnection::doRead() %p", this);
    return processScgiData();
}


int ScgiConnection::doWrite()
{
    LS_DBG_L(this, "ScgiConnection::doWrite()");
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


int ScgiConnection::doError(int err)
{
    LS_DBG_L(this, "ScgiConnection::doError(%d)\n", err);
    return 0;
}


#define SCGI_INPUT_BUFSIZE GLOBAL_BUF_SIZE
int ScgiConnection::processScgiData()
{
    int len, statusCode = 0;
    do
    {
        len = read(HttpResourceManager::getGlobalBuf(),
                   SCGI_INPUT_BUFSIZE);
        int err = errno;
        LS_DBG_H(this, "processScgiData Read %d bytes from SCGI.", len);
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
    LS_DBG_M("exit processScgiData: state: %d, statusCode: %d\n", getState(), statusCode);
    return 0;
}


#define SCGI_MAX_CONNS  "SCGI_MAX_CONNS"
#define SCGI_MAX_REQS   "SCGI_MAX_REQS"
#define SCGI_MPXS_CONNS "SCGI_MPXS_CONNS"

bool ScgiConnection::wantRead()
{
    return true;
}


bool ScgiConnection::wantWrite()
{
    return m_iTotalPending != 0;
}


int ScgiConnection::addRequest(ExtRequest *pReq)
{
    LS_DBG(this, "ScgiConnection::addRequest(%p) %p\n", pReq, this);
    m_iovec.clear();
    m_scgiEnv.clear();
    m_iTotalPending = 0;
    m_sentHeader = false;
    setConnector((HttpExtConnector *)pReq);
    HttpSession *httpSession = getConnector()->getHttpSession();
    HttpReq *httpReq = httpSession->getReq();
    if (httpReq->isChunked() && httpReq->getContentLength() == -1)
    {
        LS_DBG(this, "ScgiConnection, set waitFullReqBody");
        m_waitingBodyLength = true;
        httpSession->setWaitFullReqBody();
    }
    else
    {
        LS_DBG(this, "ScgiConnection contentLength: %ld", httpReq->getContentLength());
        m_waitingBodyLength = false;
    }
    return 0;
}


ExtRequest *ScgiConnection::getReq() const
{
    return getConnector();
}


int ScgiConnection::removeRequest(ExtRequest *pReq)
{
    LS_DBG(this, "ScgiConnection::removeRequest(%p) %p\n", pReq, this);
    if (getConnector())
    {
        getConnector()->setProcessor(NULL);
        setConnector(NULL);
    }
    return 0;
}


void ScgiConnection::finishRecvBuf()
{
    LS_DBG(this, "ScgiConnection::finishRecvBuf() %p\n", this);
    if (m_waitingBodyLength)
    {
        LS_DBG_L(this, "waiting body length, do nothing");
        return;
    }
    processScgiData();
}


int ScgiConnection::readStdOut(char *pBuf, int size)
{
    LS_DBG(this, "ScgiConnection::readStdOut() %p\n", this);
    return read(pBuf, size);
}


void ScgiConnection::suspendWrite()
{
    LS_DBG_L(this, "ScgiConnection::suspendWrite()");
    if (m_iTotalPending == 0)
        EdStream::suspendWrite();
}


int  ScgiConnection::sendReqHeader()
{
    HttpReq *pReq = getConnector()->getHttpSession()->getReq();
    m_reqBodyLen = pReq->getContentLength();
    LS_DBG(this, "ScgiConnection::sendReqHeader(), content_length: %ld\n", pReq->getContentLength());
    off_t contentLength = pReq->getContentLength();
    if (!m_scgiEnv.size())
    {
        char strLen[40];
        int len = snprintf(strLen, sizeof(strLen), "%lu", contentLength);
        m_scgiEnv.add("CONTENT_LENGTH", 14, strLen, len);
        m_scgiEnv.add("SCGI", 4, "1", 1);
        HttpCgiTool::buildEnv(&m_scgiEnv, getConnector()->getHttpSession());
    }
    if (!m_sentHeader)
    {
        LS_DBG(this, "ScgiConnection::sendReqHeader(), send header\n");
        AutoBuf *scgiEnvBuf = m_scgiEnv.get();
        sendNetString(scgiEnvBuf->begin(), scgiEnvBuf->size());
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

int  ScgiConnection::sendReqBody(const char *pBuf, int size)
{
    return writeBody(pBuf, size);
}


int ScgiConnection::beginReqBody()
{
    LS_DBG_M(this, "ScgiConnection::beginReqBody()");
    return 0;
}


int ScgiConnection::endOfReqBody()
{
    HttpReq *pReq = getConnector()->getHttpSession()->getReq();
    LS_DBG_M(this, "ScgiConnection::endOfReqBody() ContentLength: %ld", pReq->getContentLength());
    if (m_iTotalPending)
    {
        int rc = flush();
        if (rc != 0)
            return rc;
    }
    suspendWrite();
    return 0;
}


int  ScgiConnection::endOfRequest(int endCode, int status)
{
    LS_DBG(this, "ScgiConnection::endOfRequest( %d, %d)!",
           endCode, status);
    HttpExtConnector *pConnector = getConnector();
    if (!pConnector)
        return 0;
    if (endCode >= SC_400)
    {
        LS_ERROR(this, "ScgiConnection::endOfRequest( %d, %d)!",
                 endCode, status);
        pConnector->endResponse(SC_500, status);
    }
    else
        pConnector->endResponse(endCode, status);
    return 0;
}


int ScgiConnection::onStdOut()
{
    HttpExtConnector *pConnector = getConnector();
    if (!pConnector)
        return 0;
    LS_DBG_M(this, "onStdOut()");
    return pConnector->extInputReady();

}


int ScgiConnection::readResp(char *pBuf, int size)
{
    return readStdOut(pBuf, size);
}


int  ScgiConnection::processStdOut(char *pBuf, int size)
{
    HttpExtConnector *pConnector = getConnector();
    if (!pConnector)
        return size;
    LS_DBG_M(this, "Process STDOUT %d bytes", size);
    return pConnector->processRespData(pBuf, size);
}


int  ScgiConnection::processStdErr(char *pBuf, int size)
{
    HttpExtConnector *pConnector = getConnector();
    if (!pConnector)
        return size;
    LS_DBG_M(this, "Process STDERR %d bytes", size);
    return pConnector->processErrData(pBuf, size);
}


void ScgiConnection::cleanUp()
{
    LS_DBG_M(this, "ScgiConnection::cleanUp() %p", this);
    removeRequest(getConnector());
    getWorker()->recycleConn(this);
}


void ScgiConnection::continueWrite()
{
    EdStream::continueWrite();
}


void  ScgiConnection::abort()
{
    LS_DBG_M(this, "ScgiConnection::abort()");
    if (m_waitingBodyLength)
    {
        LS_DBG_L(this, "waiting body length, do nothing");
        return;
    }
    setState(ABORT);
}


int ScgiConnection::begin()
{
    LS_DBG_M(this, "ScgiConnection::beginRequest()");
    return 1;
}


int ScgiConnection::connUnavail()
{
    getConnector()->endResponse(SC_500, -1);
    return 0;
}


int  ScgiConnection::flush()
{
    LS_DBG_M(this, "ScgiConnection::flush()");
    if (m_iTotalPending)
    {
        int ret = writev(m_iovec.get(), m_iovec.len());
        if (ret >= m_iTotalPending)
        {
            m_iTotalPending = 0;
            m_iovec.clear();
            m_scgiEnv.clear();
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


void ScgiConnection::dump()
{
    LS_INFO("ScgiConnection watching event: %d, pending:%d",
            getEvents(), m_iTotalPending);
}
