/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2021  LiteSpeed Technologies, Inc.                 *
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

#include "ssl4conn.h"
#include <edio/multiplexerfactory.h>
#include <http/l4handler.h>
#include <log4cxx/logger.h>
#include <socket/coresocket.h>
#include <socket/gsockaddr.h>
#include <util/datetime.h>
#include <util/loopbuf.h>
#include <util/ssnprintf.h>

#include <fcntl.h>

int Ssl4conn::onEventDone(short event)
{
    if (getState() == SS_CLOSING)
        m_pL4Handler->closeBothConnection();
    return 0;
}


int Ssl4conn::onError()
{
    int error = errno;
    int ret = getSockError(&error);
    if ((ret == -1) || (error != 0))
    {
        if (ret != -1)
            errno = error;
    }
    LS_DBG_L(getLogSession(), "L4conn::onError()");
    if (error != 0)
    {
        setState(SS_CLOSING);
        //doError( error );
    }
    else
        onRead();
    return -1;
}


int Ssl4conn::onWrite()
{
    int ret;

    LS_DBG_L(getLogSession(), "L4conn::onWrite()");
    if (getfd() == -1)      //event pending before close() called.
        return -1;

    switch (getState())
    {
    case SS_CONNECTING:
        ret = onInitConnected();
        if (ret)
            break;
    //fall through
    case SS_CONNECTED:
        ret = doWrite();
        if (ret != LS_FAIL && m_pL4Handler->isWantRead() && getBuf()->empty())
        {
            ret = m_pL4Handler->onReadEx();
            if (ret == LS_FAIL)  //NOTE: closeBothConnection() called .
                return LS_FAIL;
        }
        break;
    case SS_CLOSING:
    case SS_DISCONNECTED:
        return 0;
    default:
        return 0;
    }
    if (ret == -1)
        setState(SS_CLOSING);
    return ret;
}


int Ssl4conn::onInitConnected()
{
    int error;
    int ret = getSockError(&error);
    if ((ret == -1) || (error != 0))
    {
        if (ret != -1)
            errno = error;
        LS_DBG_L(getLogSession(), "L4conn::onInitConnected() socket error: (%d) %s",
                 error, strerror(error));
       return -1;
    }
    LS_DBG_L(getLogSession(), "L4conn::onInitConnected() socket connected.");
    setState(SS_CONNECTED);
    if (!m_buf->empty())
        doWrite();
    m_pL4Handler->continueRead();
//     if ( D_ENABLED( DL_LESS ) )
//     {
//         char        achSockAddr[128];
//         char        achAddr[128]    = "";
//         int         port            = 0;
//         socklen_t   len             = 128;
//
//         if ( getsockname( getfd(), (struct sockaddr *)achSockAddr, &len ) == 0 )
//         {
//             GSockAddr::ntop( (struct sockaddr *)achSockAddr, achAddr, 128 );
//             port = GSockAddr::getPort( (struct sockaddr *)achSockAddr );
//         }

//         LS_DBG( getLogger(), "[%s] connected to [%s] on local addres [%s:%d]!", getLogId(),
//                 m_pWorker->getURL(), achAddr, port ));
//    }
    return 0;
}


int Ssl4conn::onRead()
{
    int ret;
    LS_DBG_L(getLogSession(), "L4conn::onRead() state: %d", getState());

    if (getfd() == -1)      //event pending before close() called.
        return -1;

    switch (getState())
    {
    case SS_CONNECTING:
        ret = onInitConnected();
        break;
    case SS_CONNECTED:
        ret = doRead();
        if (ret == LS_FAIL)  //NOTE: closeBothConnection() called .
            return LS_FAIL;
        break;
    case SS_CLOSING:
    case SS_DISCONNECTED:
        return 0;
    default:
        // Not suppose to happen;
        return 0;
    }
    if (ret == -1)
        setState(SS_CLOSING);
    return ret;
}


int Ssl4conn::close()
{
    if (getState() != SS_DISCONNECTED)
    {
        setState(SS_DISCONNECTED);
        LS_DBG_L(getLogSession(), "[L4conn] close()");
        EdStream::close();
        delete m_buf;
    }

    return 0;
}


Ssl4conn::Ssl4conn(L4Handler  *pL4Handler)
{
    m_pL4Handler = pL4Handler;
    m_buf = new LoopBuf(MAX_OUTGOING_BUF_ZISE);
    //setLogger(m_pL4Handler->getLogSession()->getLogger());
}


Ssl4conn::~Ssl4conn()
{
    if (getState() != SS_DISCONNECTED)
        close();
}


int Ssl4conn::init(const char *pAddrStr, bool ssl)
{
    setLogger(m_pL4Handler->getLogSession()->getLogger());
    setProtocol(ssl ? SS_PROTO_L4SSL : SS_PROTO_L4);
    int ret = connectEx(pAddrStr);

    LS_DBG_L(getLogSession(), "[L4conn] init(%s) ret = [%d]...",
             pAddrStr, ret);

    return ret;
}


int Ssl4conn::adnsDone(const void *ip, int ip_len)
{
    int ret = m_adns.setResolvedIp(ip, ip_len);
    if (ret == LS_FAIL)
    {
        setState(SS_ADNS_LOOKUP);
        m_pL4Handler->closeBothConnection();
        return LS_FAIL;
    }

    const GSockAddr *pAddr = &m_adns.getResolvedAddr();
    LS_DBG(getLogSession(), "DNS resolved to %s.",
           m_adns.getResolvedAddr().toString());
    return connectEx(pAddr);
}


int Ssl4conn::adnsLookupCb(void *arg, const long lParam, void *pParam)
{
    Ssl4conn *conn = (Ssl4conn *)arg;
    if (!conn)
        return 0;

    return conn->adnsDone(pParam, lParam);
}


int Ssl4conn::doAddrLookup(const char *pAddrStr)
{
    char name_only[1024];
    int ret = m_adns.tryResolve(pAddrStr, name_only,
                                AF_INET,
                                NO_ANY | DO_HOST_OVERRIDE);
    if (ret == LS_OK)
    {
        LS_DBG(getLogSession(), "[%s] DNS resolved with local resource to %s.",
                pAddrStr, m_adns.getResolvedAddr().toString());
        return ret;
    }

    ret = m_adns.asyncLookup(AF_INET, name_only, adnsLookupCb, this);
    LS_DBG(getLogSession(), "[%s] Aync DNS lookup %s.", pAddrStr,
           ret ? "started" : "failed");
    if (ret == LS_TRUE)
        return LS_TRUE;
    return LS_FAIL;
}


int Ssl4conn::connectEx(const char *pAddrStr)
{
    int ret = doAddrLookup(pAddrStr);
    if (ret == LS_TRUE)
    {
        LS_DBG_L(getLogSession(), "[L4conn] Async DNS lookup [%s] in progress",
                 pAddrStr);
        setState(SS_ADNS_LOOKUP);
        return 0;
    }
    const GSockAddr *pAddr = &m_adns.getResolvedAddr();
    return connectEx(pAddr);
}


int Ssl4conn::init(const GSockAddr *pGSockAddr, bool ssl)
{
    setLogger(m_pL4Handler->getLogSession()->getLogger());
    m_adns.setResolvedAddr(pGSockAddr->get());
    setProtocol(ssl ? SS_PROTO_L4SSL : SS_PROTO_L4);
    int ret = connectEx(pGSockAddr);

    LS_DBG_L(getLogSession(), "[L4conn] init ret = [%d]...", ret);

    return ret;
}


int Ssl4conn::connectEx(const GSockAddr *pGSockAddr)
{
    int fd;
    int ret;
    Multiplexer *pMpl =  MultiplexerFactory::getMultiplexer();
    ret = CoreSocket::connect(*pGSockAddr, pMpl->getFLTag(), &fd, 0);
    if ((fd == -1) && (errno == ECONNREFUSED))
        ret = CoreSocket::connect(*pGSockAddr, pMpl->getFLTag(), &fd, 0);

    if (fd != -1)
    {
        ::fcntl(fd, F_SETFD, FD_CLOEXEC);
        EdStream::init(fd, pMpl, POLLIN | POLLOUT | POLLHUP | POLLERR);

        getLogSession()->clearLogId();
        LS_DBG_L(getLogSession(), "[L4conn] connecting to [%s]...",
                 pGSockAddr->toString());
        if (getProtocol() == SS_PROTO_L4SSL)
            initSsl(NULL, NULL, false, NULL);

        if (ret == 0)
        {
            setState(SS_CONNECTED);
            onWrite();
        }
        else
            setState(SS_CONNECTING);
        return 0;
    }
    return -1;
}


int Ssl4conn::doRead()
{
    bool empty = m_pL4Handler->getBuf()->empty();
    int space;

    if ((space = m_pL4Handler->getBuf()->contiguous()) > 0)
    {
        int n = read(m_pL4Handler->getBuf()->end(), space);
        LS_DBG_L(getLogSession(), "[L4conn] received %d bytes.", n);

        if (n > 0)
            m_pL4Handler->getBuf()->used(n);
        else if (n < 0)
        {
            m_pL4Handler->closeBothConnection();
            return LS_FAIL;
        }
    }

    if (!m_pL4Handler->getBuf()->empty())
    {
        if (m_pL4Handler->doWrite() == LS_FAIL)
        {
            //NOTE: closeBothConnection() has been called by L4Handler::onWriteEx()
            return LS_FAIL;
        }
        if (!m_pL4Handler->getBuf()->empty() && empty)
            m_pL4Handler->continueWrite();

        if (m_pL4Handler->getBuf()->available() <= 0)
        {
            suspendRead();
            LS_DBG_L(getLogSession(), "[L4conn] suspendRead");
        }
    }
    return 0;
}


int Ssl4conn::doWrite()
{
    bool full = ((getBuf()->available() == 0) ? true : false);
    int length;

    while ((length = getBuf()->blockSize()) > 0)
    {
        int n = write(getBuf()->begin(), length);
        LS_DBG_L(getLogSession(), "[L4conn] sent %d out of %d bytes.", n, length);

        if (n > 0)
            getBuf()->pop_front(n);
        else if (n == 0)
            break;
        else // if (n < 0)
            return -1;

    }

    if (getBuf()->available() != 0)
    {
        if (full)
            m_pL4Handler->continueRead();

        if (getBuf()->empty())
        {
            suspendWrite();
            LS_DBG_L(getLogSession(),
                     "[L4conn] suspendWrite && m_pL4Handler->continueRead");
        }
    }

    return 0;
}


const char *Ssl4conn::buildLogId()
{
    char        achSockAddr[128];
    char        achAddr[128]    = "";
    unsigned    port            = 0;
    socklen_t   len             = 128;

    if (getfd() != -1)
    {
        if (getsockname(getfd(), (struct sockaddr *)achSockAddr, &len) == 0)
        {
            GSockAddr::ntop((struct sockaddr *)achSockAddr, achAddr, 128);
            port = GSockAddr::getPort((struct sockaddr *)achSockAddr);
        }
    }
    m_logId.len = lsnprintf(m_logId.ptr, MAX_LOGID_LEN, "%s#%s:%s:%d->%s",
                            m_pL4Handler->getLogSession()->getLogId(),
                            (getProtocol() == SS_PROTO_L4SSL) ? "L4S" : "L4",
                      achAddr, port, m_adns.getResolvedAddr().toString());
    return m_logId.ptr;
}

