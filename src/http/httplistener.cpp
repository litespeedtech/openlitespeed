/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
#include "httplistener.h"
#include <http/httplistenerlist.h>

#include <edio/multiplexer.h>
#include <edio/multiplexerfactory.h>
#include <http/clientcache.h>
#include <http/clientinfo.h>
#include <http/connlimitctrl.h>
#include <http/httpresourcemanager.h>
#include <http/httpvhost.h>
#include <http/ntwkiolink.h>
#include <http/smartsettings.h>
#include <http/vhostmap.h>
#include <log4cxx/logger.h>
#include <main/httpserver.h>
#include <quic/udplistener.h>
#include <socket/coresocket.h>
#include <util/accessdef.h>
#include <util/autobuf.h>
#include <util/sysinfo/nicdetect.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in_systm.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <lsr/ls_strtool.h>
#include <sslpp/sslcontext.h>
#include "httpserverconfig.h"
#include <quic/udplistener.h>

int32_t      HttpListener::m_iSockSendBufSize = -1;
int32_t      HttpListener::m_iSockRecvBufSize = -1;

HttpListener::HttpListener(const char *pName, const char *pAddr)
    : m_sName(pName)
    , m_pMapVHost(new VHostMap())
    , m_pSubIpMap(NULL)
    , m_pUdpListener(NULL)
    , m_iBinding(0xffffffffffffffffULL)
    , m_iAdmin(0)
    , m_isSSL(0)
    , m_iSendZconf(0)
    , m_flag(0)
    , m_pAdcPortList(NULL)
{
    if (m_pMapVHost)
        m_pMapVHost->setAddrStr(pAddr);
}


HttpListener::HttpListener()
    : m_sName("")
    , m_pMapVHost(new VHostMap())
    , m_pSubIpMap(NULL)
    , m_pUdpListener(NULL)
    , m_iBinding(0xffffffffffffffffULL)
    , m_iAdmin(0)
    , m_isSSL(0)
    , m_iSendZconf(0)
    , m_flag(0)
    , m_pAdcPortList(NULL)
{
}


HttpListener::~HttpListener()
{
    //stop();
    if (m_pMapVHost)
        delete m_pMapVHost;
    if (m_pSubIpMap)
        delete m_pSubIpMap;
    if (m_pAdcPortList)
        delete m_pAdcPortList;
    if (m_reusePortFds.size() > 0)
    {
        releaseReusePortSocket();
    }
}


void HttpListener::beginConfig()
{
}


void HttpListener::endConfig()
{
    if (m_pMapVHost)
        m_pMapVHost->endConfig();
    if (m_pSubIpMap)
        m_pSubIpMap->endConfig();
    if (m_pMapVHost && m_pMapVHost->getDedicated())
        setLogger(m_pMapVHost->getDedicated()->getLogger());
    if ((m_pMapVHost && m_pMapVHost->getSslContext())
        || (m_pSubIpMap && m_pSubIpMap->hasSSL()))
    {
        m_isSSL = 1;
        if (m_pMapVHost && m_pMapVHost->isQuicEnabled())
        {
            //enableQuic();
        }
    }
}


int HttpListener::enableQuic()
{
    QuicEngine *pEngine = HttpServer::getInstance().getQuicEngine();
    if (!pEngine || !m_pMapVHost)
        return -1;

    UdpListener *pUdp = new UdpListener(pEngine, this);
    pUdp->setAddr(m_pMapVHost->getAddrStr()->c_str());
    if (pUdp->start() != LS_FAIL)
    {
        m_pMapVHost->setQuicListener(pUdp);
        LS_DBG_H(this, "enableQuic addr:%s", m_pMapVHost->getAddrStr()->c_str());
        return 0;
    }
    else
    {
        LS_NOTICE(this, "Failed to enable QUIC at address: %s",
                  m_pMapVHost->getAddrStr()->c_str());
        delete pUdp;
        return -1;
    }
}


const char *HttpListener::buildLogId()
{
    if (m_pMapVHost == NULL)
        return NULL;
    const AutoStr2 *pAddrStr = m_pMapVHost->getAddrStr();
    if (pAddrStr == NULL || pAddrStr->len() == 0)
        return NULL;

    appendLogId(pAddrStr->c_str(), true);
    return m_logId.ptr;
}


const char *HttpListener::getAddrStr() const
{
    if (m_pMapVHost)
        return m_pMapVHost->getAddrStr()->c_str();
    return NULL;
}


int  HttpListener::getPort() const
{
    if (m_pMapVHost)
        return m_pMapVHost->getPort();
    return 0;
}

void HttpListener::finializeSslCtx()
{
    if (!m_pMapVHost->getSslContext() || !m_pSubIpMap)
        return;
    m_pSubIpMap->setDefaultSslCtx(m_pMapVHost->getSslContext());
}


int HttpListener::beginServe()
{
    finializeSslCtx();
    return MultiplexerFactory::getMultiplexer()->add(this,
            POLLIN | POLLHUP | POLLERR);
}


int HttpListener::assign(int fd, struct sockaddr *pAddr)
{
    m_sockAddr.operator=(*pAddr);
    char achAddr[128];
    if ((m_sockAddr.family() == AF_INET)
        && (m_sockAddr.getV4()->sin_addr.s_addr == INADDR_ANY))
        snprintf(achAddr, 128, "*:%hu", (unsigned short)m_sockAddr.getPort());
    else if ((m_sockAddr.family() == AF_INET6)
             && (IN6_IS_ADDR_UNSPECIFIED(&m_sockAddr.getV6()->sin6_addr)))
        snprintf(achAddr, 128, "[::]:%hu", (unsigned short)m_sockAddr.getPort());
    else
        m_sockAddr.toString(achAddr, 128);
    LS_NOTICE("[%s] Recovering server socket, fd: %d", achAddr, fd);
    if (m_pMapVHost)
        m_pMapVHost->setAddrStr(achAddr);
    if ((m_sockAddr.family() == AF_INET6)
        && (IN6_IS_ADDR_UNSPECIFIED(&m_sockAddr.getV6()->sin6_addr)))
        snprintf(achAddr, 128, "[ANY]:%hu", (unsigned short)m_sockAddr.getPort());
    setName(achAddr);
    setSockAttr(fd);
    if (m_pMapVHost)
        m_pMapVHost->setPort(m_sockAddr.getPort());
    setfd(fd);
    return 0;
}

int HttpListener::start()
{
    if (m_sockAddr.set(getAddrStr(), 0))
        return errno;
    int fd;
    int ret = -1;

    m_pMapVHost->setPort(m_sockAddr.getPort());

    if (m_flag & LS_SOCK_REUSEPORT)
    {
        ret = startReusePortSocket(HttpServerConfig::getInstance().getChildren());
        if (ret == EPERM)
            return ret;
    }
    if (ret != 0)
    {
        ret = CoreSocket::listen(m_sockAddr, SmartSettings::getSockBacklog(), &fd,
                                 LS_SOCK_NODELAY,
                                 m_iSockSendBufSize, m_iSockRecvBufSize);
        if (ret != 0)
            return ret;
        setSockAttr(fd);
        setfd(fd);
    }
    return 0;
}

int HttpListener::setSockAttr(int fd)
{
    ::fcntl(fd, F_SETFD, FD_CLOEXEC);
    ::fcntl(fd, F_SETFL, MultiplexerFactory::getMultiplexer()->getFLTag());
    int nodelay = 1;
    ::setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof( int ) );
#ifdef TCP_DEFER_ACCEPT
    nodelay = 30;
    ::setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &nodelay, sizeof(int));
#endif

#ifdef SO_ACCEPTFILTER
    /*
     * FreeBSD accf_http filter
     */
    struct accept_filter_arg arg;
    memset(&arg, 0, sizeof(arg));
    lstrncpy(arg.af_name, "httpready", sizeof(arg.af_name));
    if (setsockopt(fd, SOL_SOCKET, SO_ACCEPTFILTER, &arg, sizeof(arg)) < 0)
    {
        /*        struct accept_filter_arg arg;
                memset( &arg, 0, sizeof(arg) );
                strcpy( arg.af_name, "httpready" );
                if ( setsockopt(fd, SOL_SOCKET, SO_ACCEPTFILTER, &arg, sizeof(arg)) < 0)
                {
                    if (errno != ENOENT)
                    {
                        LS_NOTICE( "Failed to set accept-filter 'httpready': %s", strerror(errno) ));
                    }
                }*/
    }
#endif
    return 0;
}


int HttpListener::adjustReusePortCount(int total)
{
    if (m_pUdpListener)
        m_pUdpListener->adjustReusePortCount(total, getAddrStr());
    if (total > m_reusePortFds.size())
        return startReusePortSocket(m_reusePortFds.size(), total);
    else if (total < m_reusePortFds.size())
    {
        LS_NOTICE("[%s] Shink SO_REUSEPORT socket count from #%d to #%d",
                    getAddrStr(), m_reusePortFds.size(), total);
        return m_reusePortFds.shrink(total);
    }
    return 0;
}


void HttpListener::adjustFds(int iNumChildren)
{
    if (!(m_flag & LS_SOCK_REUSEPORT))
        return;
    if (m_reusePortFds.size() == 0 && getfd() != -1)
        *m_reusePortFds.newObj() = getfd();

    adjustReusePortCount(iNumChildren);
}


void HttpListener::enableReusePort()
{
    LS_INFO("[%s] Enable SO_REUSEPORT .", getAddrStr());
    m_flag |= LS_SOCK_REUSEPORT;
}


void HttpListener::addUdpSocket(int fd)
{
    LS_DBG("add fd: %d UDP listener %p.", fd, this);
    if (!m_pUdpListener)
    {
        m_pUdpListener = new UdpListener(NULL, this);
        m_pUdpListener->setAddr(&m_sockAddr);
        m_pUdpListener->setfd(fd);
        m_pUdpListener->registEvent();
        LS_DBG("[%s] create UDP listener %p, add fd: %d.", getAddrStr(),
               m_pUdpListener, fd);
    }
    else if (HttpServerConfig::getInstance().getChildren() > 1)
    {
        LS_DBG("[%s] add fd: %d SO_REUSEPORT to UDP listener %p.", getAddrStr(),
               fd, m_pUdpListener);
        m_pUdpListener->addReusePortSocket(fd, getAddrStr());
    }
    else
    {
        LS_DBG("[%s] Close extra SO_REUSEPORT UDP listener, fd: %d.", getAddrStr(),
               fd);
        close(fd);
    }
}


int HttpListener::bindUdpPort()
{
    int ret;
    assert(m_pUdpListener == NULL);
    UdpListener *pUdp = new UdpListener(NULL, this);
    pUdp->setAddr(&m_sockAddr);
    if (m_flag & LS_SOCK_REUSEPORT)
        ret = pUdp->bindReusePort(HttpServerConfig::getInstance().getChildren(), getAddrStr());
    else
        ret = pUdp->bind();
    if (ret == -1)
    {
        LS_NOTICE("[UDP %s] Failed to enable QUIC.", getAddrStr());
        delete pUdp;
        return ret;
    }
    m_pUdpListener = pUdp;
    LS_NOTICE("[%s] enabled QUIC UDP", getAddrStr());
    return 0;
}



int HttpListener::startReusePortSocket(int total)
{
    return startReusePortSocket(0, total);
}


int HttpListener::startReusePortSocket(int start, int total)
{
    int i, ret, fd;
    LS_NOTICE("[%s] Add SO_REUSEPORT socket, #%d to #%d",
                getAddrStr(), start + 1, total);
    m_reusePortFds.guarantee(total - m_reusePortFds.size());

    if (start == 1)
        ls_setsockopt(m_reusePortFds[0], SOL_SOCKET, SO_REUSEPORT,
                      (char *)(&start), sizeof(start));

    for(i = start; i < total; ++i)
    {
        ret = CoreSocket::listen(m_sockAddr, SmartSettings::getSockBacklog(), &fd,
                                 LS_SOCK_NODELAY | LS_SOCK_REUSEPORT,
                                 m_iSockSendBufSize, m_iSockRecvBufSize);
        if (ret != 0)
        {
            if (ret != EACCES)
                LS_NOTICE("[%s] failed to start SO_REUSEPORT socket, disalbe",
                           getAddrStr());
            m_flag &= ~LS_SOCK_REUSEPORT;
            return ret;
        }
        LS_DBG("[%s] SO_REUSEPORT #%d started, fd: %d",
               getAddrStr(), i + 1, fd);
        ::fcntl(fd, F_SETFD, FD_CLOEXEC);
        m_reusePortFds[i] = fd;
    }
    m_reusePortFds.setSize(total);
    return 0;
}


int HttpListener::closeUnActiveReusePort()
{
    m_reusePortFds.close();
    return 0;
}

int HttpListener::activeReusePort(int seq)
{
    int n;
    int fd = m_reusePortFds.getActiveFd(seq, &n);
    if (fd == -1)
    {
        LS_NOTICE("[%s] Worker #%d activates SO_REUSEPORT #%d socket, failed.",
                getAddrStr(), seq, n + 1);
        return -1;
    }
    LS_NOTICE("[%s] Worker #%d activates SO_REUSEPORT #%d socket, fd: %d",
                getAddrStr(), seq, n + 1, fd);
    setSockAttr(fd);
    setfd(fd);
    return 0;
}




void HttpListener::releaseReusePortSocket()
{
    m_reusePortFds.releaseObjects();
}


bool HttpListener::isSameAddr(const struct sockaddr* addr)
{
    return (GSockAddr::compare(m_sockAddr.get(), addr) == 0);
}


void HttpListener::addReusePortSocket(int fd)
{
    if (HttpServerConfig::getInstance().getChildren() <= 1)
    {
        LS_INFO("[%s] Close extra SO_REUSEPORT TCP listener, fd: %d.",
                getAddrStr(), fd);
        close(fd);
        return;
    }
    LS_DBG("recvListeners() add fd: %d SO_REUSEPORT to listener %p.", fd,
            this);
    if (m_reusePortFds.size() == 0)
    {
        *m_reusePortFds.newObj() = getfd();
    }
    *m_reusePortFds.newObj() = fd;
    LS_NOTICE("[%s] SO_REUSEPORT #%d, recovering server socket, fd: %d.",
                getAddrStr(), m_reusePortFds.size(), fd);
}


int HttpListener::getFdCount(int* maxfd)
{
    int udp_count = 0;
    if (m_pUdpListener)
        udp_count = m_pUdpListener->getFdCount(maxfd);
    if (getfd() > *maxfd)
        *maxfd = getfd();
    if (m_reusePortFds.size() <= 0)
        return udp_count + (getfd() != -1);
    return udp_count + m_reusePortFds.getFdCount(maxfd);
}


int HttpListener::passFds2(int target_fd)
{
    if (m_reusePortFds.size() > 0)
        return m_reusePortFds.passFds("TCP", getAddrStr(), target_fd);
    if (getfd() == -1)
        return 0;
    --target_fd;
    LS_INFO("[%s] Pass listener, copy fd %d to %d.", getAddrStr(),
        getfd(), target_fd);
    dup2(getfd(), target_fd);
    close(getfd());
    return 1;
}


int HttpListener::passFds(int target_fd)
{
    int n = 0;
    if (m_pUdpListener)
        n = m_pUdpListener->passFds(target_fd, getAddrStr());
    n += passFds2(target_fd - n);
    return n;
}



int HttpListener::suspend()
{
    if (getfd() != -1)
    {
        MultiplexerFactory::getMultiplexer()->suspendRead(this);
        return 0;
    }
    return EBADF;
}


int HttpListener::resume()
{
    if (getfd() != -1)
    {
        MultiplexerFactory::getMultiplexer()->continueRead(this);
        return 0;
    }
    return EBADF;
}


int HttpListener::stop()
{
    if (getfd() != -1)
    {
        LS_NOTICE("Stop listener %s, fd %d.", getAddrStr(), getfd());
        MultiplexerFactory::getMultiplexer()->remove(this);
        close(getfd());
        setfd(-1);
//        releaseReusePortSocket();
        return 0;
    }
    return EBADF;
}


static void no_timewait(int fd)
{
    struct linger l = { 1, 0 };
    setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
}


struct conn_data
{
    int             fd;
    char            achPeerAddr[sizeof(struct sockaddr_in6)];
    ClientInfo     *pInfo;
};


#define CONN_BATCH_SIZE 10
int HttpListener::handleEvents(short event)
{
    static struct conn_data conns[CONN_BATCH_SIZE];
    static struct conn_data *pEnd = &conns[CONN_BATCH_SIZE];
    struct conn_data *pCur = conns;
    int allowed;
    int iCount = 0;
    ConnLimitCtrl &ctrl = ConnLimitCtrl::getInstance();
    int limitType = 1;
    allowed = ctrl.availConn();
    if (isSSL())
    {
        if (allowed > ctrl.availSSLConn())
        {
            allowed = ctrl.availSSLConn();
            limitType = 2;
        }
    }
    LS_DBG_H(this, "HttpListener::handleEvents\n");

    while (iCount < allowed)
    {
        socklen_t len = sizeof(pCur->achPeerAddr);
#ifdef SOCK_CLOEXEC
        static int isUseAccept4 = 1;
        if (isUseAccept4)
        {
            pCur->fd = accept4(getfd(), (struct sockaddr *)(pCur->achPeerAddr),
                               &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (pCur->fd == -1 && errno == ENOSYS)
            {
                isUseAccept4 = 0;
                continue;
            }
        }
        else
#endif
        {
            pCur->fd = accept(getfd(), (struct sockaddr *)(pCur->achPeerAddr), &len);
            if (pCur->fd != -1)
            {
                fcntl(pCur->fd, F_SETFD, FD_CLOEXEC);
                fcntl(pCur->fd, F_SETFL, MultiplexerFactory::getMultiplexer()->getFLTag());
            }
        }
        if (pCur->fd == -1)
        {
            resetRevent(POLLIN);
            if ((errno != EAGAIN) && (errno != ECONNABORTED)
                && (errno != EINTR))
            {
                LS_ERROR(this,
                         "HttpListener::acceptConnection(): Accept failed:%s!",
                         strerror(errno));
            }
            break;
        }
        //++iCount;
        //addConnection( conns, &iCount );

        ++pCur;
        if (pCur == pEnd)
        {
            iCount += CONN_BATCH_SIZE;
            batchAddConn(conns, pCur, &iCount);
            pCur = conns;
        }

    }
    if (pCur > conns)
    {
        int n = pCur - conns;
        iCount += n;
        int ret;
        if (n > 1)
            ret = batchAddConn(conns, pCur, &iCount);
        else
            ret = addConnection(conns, &iCount);
        if (ret)
        {
            LS_ERROR(this,
                     "HttpListener::handleEvents(): addConnection failed, n=%d ret=%d",
                     n, ret);
        }
    }
    if (iCount > 0)
    {
        if (m_pMapVHost)
            m_pMapVHost->incRef(iCount);
        ctrl.incConn(iCount);
    }
    if (iCount >= allowed)
    {
        if (limitType == 1)
        {
            if (ctrl.availConn() <= 0)
            {
                LS_DBG_H(this, "Max connections reached, suspend accepting!");
                ctrl.suspendAll();
            }
        }
        else
        {
            if (ctrl.availSSLConn() <= 0)
            {
                LS_DBG_M(this, "Max SSL connections reached, suspend accepting!");
                ctrl.suspendSSL();
            }
        }
    }
    LS_DBG_H(this, "%d connections accepted!", iCount);
    return 0;
}


int HttpListener::checkAccess(struct conn_data *pData)
{
    struct sockaddr *pPeer = (struct sockaddr *) pData->achPeerAddr;
    GSockAddr::mappedV6toV4(pPeer);

    ClientInfo *pInfo = ClientCache::getClientCache()->getClientInfo(pPeer);
    if (!pInfo)
        return -1;
    pData->pInfo = pInfo;

    LS_DBG_H(this, "New connection from %s:%u.", pInfo->getAddrString(),
             (unsigned)ntohs(((struct sockaddr_in *)pPeer)->sin_port));

    return pInfo->checkAccess();
}


int HttpListener::batchAddConn(struct conn_data *pBegin,
                               struct conn_data *pEnd, int *iCount)
{
    struct conn_data *pCur = pBegin;
    int n = pEnd - pBegin;
    while (pCur < pEnd)
    {
        if (checkAccess(pCur))
        {
            no_timewait(pCur->fd);
            close(pCur->fd);
            pCur->fd = -1;
            --(*iCount);
            --n;
        }
        ++pCur;
    }
    if (n <= 0)
        return 0;
    NtwkIOLink *pConns[CONN_BATCH_SIZE];
    int ret = HttpResourceManager::getInstance().getNtwkIOLinks(pConns, n);
    pCur = pBegin;
    if (ret <= 0)
    {
        LS_ERR_NO_MEM("HttpSessionPool::getConnections()");
        LS_ERROR("Need %d connections, allocated %d connections!", n, ret);
        while (pCur < pEnd)
        {
            if (pCur->fd != -1)
            {
                close(pCur->fd);
                --(*iCount);
            }
            ++pCur;
        }
        return LS_FAIL;
    }
    NtwkIOLink **pConnEnd = &pConns[ret];
    NtwkIOLink **pConnCur = pConns;
    int flag = MultiplexerFactory::getMultiplexer()->getFLTag();
    while (pCur < pEnd)
    {
        int fd = pCur->fd;
        if (fd != -1)
        {
            ConnInfo info;
            assert(pConnCur < pConnEnd);
            NtwkIOLink *pConn = *pConnCur;
            if (setConnInfo(&info, pCur))
            {
                LS_ERROR("HttpListener::batchAddConn failed, pCur = %p.", pCur);
                close(fd);
                --(*iCount);
            }
            else
            {
                pConn->setLogger(getLogger());

                if (!pConn->setLink(this, fd, &info))
                {
                    fcntl(fd, F_SETFD, FD_CLOEXEC);
                    fcntl(fd, F_SETFL, flag);
                    ++pConnCur;
                    pConn->tryRead();
                }
                else
                {
                    close(fd);
                    --(*iCount);
                }
            }
        }
        ++pCur;
    }
    if (pConnCur < pConnEnd)
        HttpResourceManager::getInstance().recycle(pConnCur,
                pConnEnd - pConnCur);

    return 0;
}


VHostMap *HttpListener::getSubMap(int fd)
{
    VHostMap *pMap;
    char achAddr[128];
    socklen_t addrlen = 128;
    if (getsockname(fd, (struct sockaddr *) achAddr, &addrlen) == -1)
        return 0;
    pMap = m_pSubIpMap->getMap((struct sockaddr *) achAddr);
    if (!pMap)
        pMap = m_pMapVHost;
    return pMap;
}


int HttpListener::addConnection(struct conn_data *pCur, int *iCount)
{
    int fd = pCur->fd;
    if (checkAccess(pCur))
    {
        no_timewait(fd);
        close(fd);
        --(*iCount);
        return 0;
    }
    NtwkIOLink *pConn = HttpResourceManager::getInstance().getNtwkIOLink();
    if (!pConn)
    {
        LS_ERR_NO_MEM("HttpSessionPool::getConnection()");
        close(fd);
        --(*iCount);
        return LS_FAIL;
    }

    ConnInfo info;
    if (setConnInfo(&info, pCur) == LS_FAIL)
    {
        LS_ERROR("HttpListener::addConnection failed, pCur = %p.", pCur);
        close(fd);
        --(*iCount);
        return LS_FAIL;
    }

    pConn->setLogger(getLogger());
    if (pConn->setLink(this, pCur->fd, &info))
    {
        HttpResourceManager::getInstance().recycle(pConn);
        close(fd);
        --(*iCount);
        return LS_FAIL;
    }
    pConn->tryRead();
    return 0;
}


const VHostMap *HttpListener::findVhostMap(const struct sockaddr * pAddr) const
{
    const VHostMap *pMap;
    if (m_pSubIpMap)
    {
        pMap = m_pSubIpMap->getMap((struct sockaddr *)pAddr);
        if (!pMap)
            pMap = getVHostMap();
    }
    else
    {
        pMap = getVHostMap();
    }
    return pMap;
}


VHostMap *HttpListener::addIpMap(const char *pIP)
{
    if (!m_pSubIpMap)
    {
        m_pSubIpMap = new SubIpMap();
        if (!m_pSubIpMap)
            return NULL;
    }
    VHostMap *pMap = m_pSubIpMap->addIP(pIP);
    if (pMap && m_pMapVHost)
        pMap->setPort(m_pMapVHost->getPort());
    return pMap;
}


int HttpListener::addDefaultVHost(HttpVHost *pVHost)
{
    int count = 0;
    if (m_pMapVHost && m_pMapVHost->addMaping(pVHost, "*", 1) == 0)
        ++count;
    if (m_pSubIpMap)
        count += m_pSubIpMap->addDefaultVHost(pVHost);
    return count;
}


int HttpListener::writeStatusReport(int fd)
{
    char achBuf[1024];

    int len = ls_snprintf(achBuf, 1024, "LISTENER%d [%s] %s\n",
                          isAdmin(), getName(), getAddrStr());
    if (::write(fd, achBuf, len) != len)
        return LS_FAIL;
    if (getVHostMap()->writeStatusReport(fd) == -1)
        return LS_FAIL;
    if (m_pSubIpMap && m_pSubIpMap->writeStatusReport(fd) == -1)
        return LS_FAIL;
    if (::write(fd, "ENDL\n", 5) != 5)
        return LS_FAIL;
    return 0;

}


int HttpListener::writeStatusJsonReport(int fd, int first, int last)
{
    char achBuf[1024];
    int len;
    len = ls_snprintf(achBuf, sizeof(achBuf), 
                      "%s"
                      "      {\n"
                      "        \"name\": \"%s\",\n"
                      "        \"address\": \"%s\",\n",
                      first ? "  \"listeners\" : [\n" : "",
                      getName(), 
                      getAddrStr());
    if (::write(fd, achBuf, len) != len)
        return LS_FAIL;
    if (getVHostMap()->writeStatusJsonReport(fd) == -1)
        return LS_FAIL;
    if (m_pSubIpMap && m_pSubIpMap->writeStatusJsonReport(fd) == -1)
        return LS_FAIL;
    const char *endStr = last ? "      }\n    ],\n" : "      },\n";
    int endStrLen = last ? 15 : 9;
    if (::write(fd, endStr, endStrLen) != endStrLen)
        return LS_FAIL;
    return 0;

}


int HttpListener::mapDomainList(HttpVHost *pVHost, const char *pDomains)
{
    if (pVHost && pVHost->enableQuicListener() &&
        m_pMapVHost && m_pMapVHost->getQuicListener())
        pVHost->enableQuic(1);

    if (m_pMapVHost)
        return m_pMapVHost->mapDomainList(pVHost, pDomains);
    return 0;
}


void HttpListener::setAdcPortList(const char *pList)
{
    int portNum;
    StringList portList;
    StringList::iterator iter;

    if (m_pAdcPortList != NULL)
    {
        LS_NOTICE("Listener's adc port list is already set.");
        return;
    }

    portList.split(pList, pList + strlen(pList), ",");
    for (iter = portList.begin(); iter != portList.end(); ++iter)
    {
        portNum = strtol((*iter)->c_str(), NULL, 0);

        if ((portNum <= 0) || (portNum > 65535))
        {
            LS_ERROR("Attempted to add invalid ADC port number %d to ZConfClient, do not send listener.",
                portNum);
            m_iSendZconf = 0;
            return;
        }
    }

    m_pAdcPortList = new AutoStr(pList);
}


static int zconfLoadIpList(int family, int includeV6, char *buf, char *pEnd)
{
    struct ifi_info *pHead = NICDetect::get_ifi_info(family, 1);
    struct ifi_info *iter;
    char *pBegin = buf;
    char temp[80];

    for (iter = pHead; iter != NULL; iter = iter->ifi_next)
    {
        if (iter->ifi_addr)
        {
            GSockAddr::ntop(iter->ifi_addr, temp, 80);

            if (family == AF_INET6)
            {
                const struct in6_addr *pV6 = & ((const struct sockaddr_in6 *)
                        iter->ifi_addr)->sin6_addr;

                if ((!IN6_IS_ADDR_LINKLOCAL(pV6)) &&
                        (!IN6_IS_ADDR_SITELOCAL(pV6)) &&
                        (!IN6_IS_ADDR_MULTICAST(pV6)))
                {
                    if (strncmp(temp, "::1", 3) == 0)
                        continue;
                    if (pBegin != buf)
                        *buf++ = ',';

                    buf += ls_snprintf(buf, pEnd - buf, "[%s]", temp);
                }
            }
            else
            {
                if (strncmp(temp, "127.0.0.1", 9) == 0)
                    continue;

                if (pBegin != buf)
                    *buf++ = ',';

                buf += ls_snprintf(buf, pEnd - buf, "%s", temp);

                if (includeV6)
                    buf += ls_snprintf(buf, pEnd - buf, ",[::FFFF:%s]", temp);
            }
        }
    }

    if (pHead)
        NICDetect::free_ifi_info(pHead);
    return buf - pBegin;
}


int HttpListener::zconfAppendVHostList(AutoBuf *pBuf)
{
    const int maxBufLen = 1024;
    int confListLen;
    char *pCur, *pBegin, *pEnd;
    const char *pPortList, *pAddr = getAddrStr();
    VHostMap *pMap = getVHostMap();
    char isSsl = (pMap->getSslContext() ? 1 : 0);

    if (!pMap->zconfAppendDomainMap(pBuf, isSsl))
        return -1;

    pBuf->reserve(pBuf->size() + maxBufLen);
    // Set these _after_ reserve because buffer may change.
    pBegin = pBuf->end();
    pEnd = pBegin + maxBufLen;

    if (getAdcPortList())
        pPortList = getAdcPortList()->c_str();
    else
        pPortList = pMap->getPortStr().c_str();

    confListLen = ls_snprintf(pBegin, maxBufLen,
            "\"conf_list\":[\n"
            "{\n"
            "\"lb_port_list\":[%s],\n"
            "\"dport\":%d,\n" // Destination port for backend.
            "\"be_ssl\":%s,\n" // HttpListener has isSSL. whether the [be = backend] listener is ssl
            "\"ip_list\":\n"
            "[\n"
            "{\"ip\":\"",
            pPortList,
            getPort(),
            isSsl ? "true" : "false"
    );

    if (confListLen >= maxBufLen)
    {
        LS_NOTICE("[ZCONFCLIENT] VHost Configuration too long (after ADC Port list).");
        return -1;
    }

    pCur = pBegin + confListLen;

    if ('*' == pAddr[0])
    {
        confListLen += zconfLoadIpList(AF_INET, 0, pCur, pEnd);
    }
    else if ('[' == pAddr[0] && 'A' == pAddr[1] && 'N' == pAddr[2]
            && 'Y' == pAddr[3] && ']' == pAddr[4])
    {
        int ipv6Len, ipv4Len;

        ipv4Len = zconfLoadIpList(AF_INET, 1, pCur, pEnd);
        pCur[ipv4Len] = ',';
        ipv6Len = zconfLoadIpList(AF_INET6, 1, pCur + ipv4Len + 1, pEnd);

        if (0 == ipv6Len)
        {
            pCur[ipv4Len] = '\0';
            confListLen += ipv4Len;
        }
        else
            confListLen += ipv4Len + ipv6Len + 1;
    }
    else
    {
        confListLen += ls_snprintf(pCur, pEnd - pCur, "%.*s",
                pMap->getAddrStr()->len() - (pMap->getPortStr().len() + 1),
                pAddr);
    }

    if (confListLen + 14 >= maxBufLen) // Check for space enough for closing braces
    {
        LS_NOTICE("[ZCONFCLIENT] VHost Configuration too long (after IP list).");
        return -1;
    }

    confListLen += ls_snprintf(pBegin + confListLen, maxBufLen - confListLen,
            "\"}\n"
            "]\n"
            "}\n"
            "]\n"
            "}\n"
            ",\n");
    pBuf->used(confListLen);
    return confListLen;
}

SSL *HttpListener::newSsl(SslContext *pSslContext)
{
    pSslContext->initOCSP();

    SSL *p = pSslContext->newSSL();
    return p;
}

int HttpListener::setConnInfo(ConnInfo *pInfo, struct conn_data *pCur)
{
    socklen_t addrLen = 28;
    char serverAddr[28];
    struct sockaddr *pAddr = (sockaddr *)serverAddr;
    memset(pInfo, 0, sizeof(ConnInfo));
    if (getsockname(pCur->fd, pAddr, &addrLen) == -1)
    {
        return LS_FAIL;
    }

    GSockAddr::mappedV6toV4(pAddr);

    pInfo->m_pServerAddrInfo = ServerAddrRegistry::getInstance().get(pAddr, this);
    const VHostMap * pMap = pInfo->m_pServerAddrInfo->getVHostMap();
    if (!pMap)
    {
        pMap = findVhostMap(pAddr);
        if (pMap)
            ((ServerAddrInfo *)pInfo->m_pServerAddrInfo)->setVHostMap(pMap);
    }
    if (pMap && pMap->getSslContext())
    {
        pCur->pInfo->setSslContext(pMap->getSslContext());
        pInfo->m_pSsl = newSsl(pMap->getSslContext());
        if (!pInfo->m_pSsl)
            return LS_FAIL;
    }
    pInfo->m_remotePort = GSockAddr::getPort((sockaddr *)pCur->achPeerAddr);
    pInfo->m_pClientInfo = pCur->pInfo;
    return LS_OK;
}


