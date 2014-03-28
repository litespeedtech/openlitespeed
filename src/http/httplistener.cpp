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
#include "httplistener.h"
#include <edio/multiplexer.h>
#include <http/adns.h>
#include <util/datetime.h>
#include <http/clientcache.h>
#include <http/connlimitctrl.h>
#include <http/eventdispatcher.h>
#include <http/httpsession.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <http/httpresourcemanager.h>
#include <http/httpvhost.h>
#include <http/smartsettings.h>
#include <http/vhostmap.h>
#include <socket/coresocket.h>
#include <socket/gsockaddr.h>
#include <sslpp/sslcontext.h>
#include <util/accessdef.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in_systm.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <util/ssnprintf.h>

int32_t      HttpListener::m_iSockSendBufSize = -1;
int32_t      HttpListener::m_iSockRecvBufSize = -1;


HttpListener::HttpListener( const char * pName, const char * pAddr )
    : m_sName( pName )
    , m_pMapVHost( new VHostMap() )
    , m_pSubIpMap( NULL )
    , m_iAdmin( 0 )
    , m_isSSL( 0 )
    , m_iBinding( 0xffffffff )
    , m_iolinkSessionHooks(0)
{
    m_pMapVHost->setAddrStr( pAddr );
}

HttpListener::HttpListener()
    : m_pMapVHost( new VHostMap() )
    , m_pSubIpMap( NULL )
    , m_iAdmin( 0 )
    , m_isSSL( 0 )
    , m_iBinding( 0xffffffff )
    , m_iolinkSessionHooks(0)
{
}


HttpListener::~HttpListener()
{
    //stop();
    if ( m_pMapVHost )
        delete m_pMapVHost;
    if ( m_pSubIpMap )
        delete m_pSubIpMap;
    
}


void HttpListener::beginConfig()
{
}

void HttpListener::endConfig()
{
    m_pMapVHost->endConfig();
    if ( m_pSubIpMap )
        m_pSubIpMap->endConfig();
    if ( m_pMapVHost->getDedicated() )
        setLogger( m_pMapVHost->getDedicated()->getLogger() );
    if ( m_pMapVHost->getSSLContext()
        ||( m_pSubIpMap && m_pSubIpMap->hasSSL() ) )
        m_isSSL = 1;
}

const char * HttpListener::getAddrStr() const
{
    return m_pMapVHost->getAddrStr()->c_str();
}

int  HttpListener::getPort() const
{
    return m_pMapVHost->getPort();
}

int HttpListener::assign( int fd, struct sockaddr * pAddr )
{
    GSockAddr addr( pAddr );
    char achAddr[128];
    if (( addr.family() == AF_INET )&&( addr.getV4()->sin_addr.s_addr == INADDR_ANY ))
    {
        snprintf( achAddr, 128, "*:%hu", (short)addr.getPort() );
    }
    else if (( addr.family() == AF_INET6 )&&( IN6_IS_ADDR_UNSPECIFIED ( &addr.getV6()->sin6_addr ) ))
    {
        snprintf( achAddr, 128, "[::]:%hu", (short)addr.getPort() );
    }
    else
        addr.toString( achAddr, 128 );
    LOG_NOTICE(( "Recovering server socket: [%s]", achAddr ));
    m_pMapVHost->setAddrStr( achAddr );
    if (( addr.family() == AF_INET6 )&&( IN6_IS_ADDR_UNSPECIFIED ( &addr.getV6()->sin6_addr ) ))
        snprintf( achAddr, 128, "[ANY]:%hu", (short)addr.getPort() );
    setName( achAddr );
    return setSockAttr( fd, addr );
}


int HttpListener::start()
{
    GSockAddr addr;
    if ( addr.set( getAddrStr(), 0 ) )
        return errno;
    int fd;
    int ret = CoreSocket::listen( addr,
            SmartSettings::getSockBacklog(), &fd,
            m_iSockSendBufSize, m_iSockRecvBufSize );
    if ( ret != 0 )
    {
        return ret;
    }
    return setSockAttr( fd, addr );
}

int HttpListener::setSockAttr( int fd, GSockAddr &addr )
{
    setfd( fd );
    ::fcntl( fd, F_SETFD, FD_CLOEXEC );
    ::fcntl( fd, F_SETFL, HttpGlobals::getMultiplexer()->getFLTag());
    int nodelay = 1;
    //::setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof( int ) );
#ifdef TCP_DEFER_ACCEPT
    ::setsockopt( fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &nodelay, sizeof( int ) );
#endif
    
    //int tos = IPTOS_THROUGHPUT;
    //setsockopt( fd, IPPROTO_IP, IP_TOS, &tos, sizeof( tos ));
    m_pMapVHost->setPort( addr.getPort() );
    
#ifdef SO_ACCEPTFILTER
    /*
     * FreeBSD accf_http filter
     */
    struct accept_filter_arg arg;
    memset( &arg, 0, sizeof(arg) );
    strcpy( arg.af_name, "httpready" );
    if ( setsockopt(fd, SOL_SOCKET, SO_ACCEPTFILTER, &arg, sizeof(arg)) < 0)
    {
        if (errno != ENOENT)
        {
            LOG_NOTICE(( "Failed to set accept-filter 'httpready': %s", strerror(errno) ));
        }
    }
#endif
    
    return HttpGlobals::getMultiplexer()->add( this, POLLIN|POLLHUP|POLLERR );
}

int HttpListener::suspend()
{
    if ( getfd() != -1 )
    {
        HttpGlobals::getMultiplexer()->suspendRead( this );
        return 0;
    }
    return EBADF;
}

int HttpListener::resume()
{
    if ( getfd() != -1 )
    {
        HttpGlobals::getMultiplexer()->continueRead( this );
        return 0;
    }
    return EBADF;
}

int HttpListener::stop()
{
    if ( getfd() != -1 )
    {
        LOG_INFO(( "Stop listener %s.", getAddrStr() ));
        HttpGlobals::getMultiplexer()->remove( this );
        close( getfd() );
        setfd( -1 );
        return 0;
    }
    return EBADF;
}

static void no_timewait( int fd )
{
    struct linger l = { 1, 0 };
    setsockopt( fd, SOL_SOCKET, SO_LINGER, &l, sizeof( l ) );
}

struct conn_data
{
    int             fd;
    char            achPeerAddr[24];
    ClientInfo *    pInfo;
};

#define CONN_BATCH_SIZE 10
int HttpListener::handleEvents( short event )
{
    static struct conn_data conns[CONN_BATCH_SIZE];
    static struct conn_data * pEnd = &conns[CONN_BATCH_SIZE];
    struct conn_data * pCur = conns;
    int allowed;
    int iCount = 0;
    ConnLimitCtrl * pCLC = HttpGlobals::getConnLimitCtrl();
    int limitType = 1;
    allowed = pCLC->availConn();
    if ( isSSL() )
    {
        if ( allowed > pCLC->availSSLConn() )
        {
            allowed = pCLC->availSSLConn();
            limitType = 2;
        }
    }

    while( iCount < allowed )
    {
        socklen_t len = 24;
        pCur->fd = accept( getfd(), (struct sockaddr *)(pCur->achPeerAddr), &len );
        if ( pCur->fd == -1 )
        {
            resetRevent( POLLIN );
            if (( errno != EAGAIN )&&( errno != ECONNABORTED )
                &&( errno != EINTR ))
            {
                LOG_ERR(( getLogger(),
                    "HttpListener::acceptConnection(): [%s] can't accept:%s!",
                    getAddrStr(), strerror( errno ) ));
            }
            break;
        }
        //++iCount;
        //addConnection( conns, &iCount );
        
        ++pCur;
        if ( pCur == pEnd )
        {
            iCount += CONN_BATCH_SIZE;
            batchAddConn( conns, pCur, &iCount );
            pCur = conns;
        }

    }
    if ( pCur > conns )
    {
        int n = pCur - conns;
        iCount += n;
        if ( n > 1 )
            batchAddConn( conns, pCur, &iCount );
        else
            addConnection( conns, &iCount );
    }
    if ( iCount > 0 )
    {
        m_pMapVHost->incRef( iCount );
        pCLC->incConn( iCount );
    }
    if ( iCount >= allowed )
    {
        if ( limitType == 1 )
        {
            if ( D_ENABLED( DL_MORE ) )
            {
                LOG_D(( getLogger(),
                    "[%s] max connections reached, suspend accepting!",
                    getAddrStr() ));
            }
            pCLC->suspendAll();
        }
        else
        {
            if ( D_ENABLED( DL_MORE ) )
            {
                LOG_D(( getLogger(),
                    "[%s] max SSL connections reached, suspend accepting!",
                    getAddrStr() ));
            }
            pCLC->suspendSSL();
        }
    }
    if ( D_ENABLED( DL_MORE ) )
    {
        LOG_D(( getLogger(),
            "[%s] %d connections accepted!", getAddrStr(), iCount ));
    }
    return 0;
}


int HttpListener::checkAccess( struct conn_data * pData )
{
    struct sockaddr * pPeer = ( struct sockaddr *) pData->achPeerAddr;
    if (( AF_INET6 == pPeer->sa_family )&&
        ( IN6_IS_ADDR_V4MAPPED( &((struct sockaddr_in6 *)pPeer)->sin6_addr )) )
    {
        pPeer->sa_family = AF_INET;
        ((struct sockaddr_in *)pPeer)->sin_addr.s_addr = *((in_addr_t *)&pData->achPeerAddr[20]);
    }
    ClientInfo * pInfo = HttpGlobals::getClientCache()->getClientInfo( pPeer );
    pData->pInfo = pInfo;

    if ( D_ENABLED( DL_MORE ))
        LOG_D(( "[%s] New connection from %s:%d.", getAddrStr(),
                    pInfo->getAddrString(), ntohs( ((struct sockaddr_in*)pPeer)->sin_port) ));

    return pInfo->checkAccess();
}


int HttpListener::batchAddConn( struct conn_data * pBegin,
                            struct conn_data *pEnd, int *iCount )
{
    struct conn_data * pCur = pBegin;
    int n = pEnd - pBegin;
    while( pCur < pEnd )
    {
        if ( checkAccess( pCur))
        {
            no_timewait( pCur->fd );
            close( pCur->fd );
            pCur->fd = -1;
            --(*iCount);
            --n;
        }
        ++pCur;
    }
    if ( n <= 0 )
        return 0;
    NtwkIOLink* pConns[CONN_BATCH_SIZE];
    int ret = HttpGlobals::getResManager()->getNtwkIOLinks( pConns, n);
    pCur = pBegin;
    if ( ret <= 0 )
    {
        ERR_NO_MEM( "HttpSessionPool::getConnections()" );
        LOG_ERR(( "need %d connections, allocated %d connections!", n, ret ));
        while( pCur < pEnd )
        {
            if ( pCur->fd != -1 )
            {
                close( pCur->fd );
                --(*iCount);
            }
            ++pCur;
        }
        return -1;
    }
    NtwkIOLink** pConnEnd = &pConns[ret];
    NtwkIOLink** pConnCur = pConns;
    VHostMap * pMap;
    int flag = HttpGlobals::getMultiplexer()->getFLTag();
    while( pCur < pEnd )
    {
        register int fd = pCur->fd;
        if ( fd != -1 )
        {
            assert( pConnCur < pConnEnd );
            NtwkIOLink * pConn = *pConnCur;

            if ( m_pSubIpMap )
            {
                pMap = getSubMap( fd );
            }
            else
                pMap = getVHostMap();

            pConn->setVHostMap( pMap );
            pConn->setLogger( getLogger());
            pConn->setRemotePort( ntohs( ((sockaddr_in *)(pCur->achPeerAddr))->sin_port) );

        //    if ( getDedicated() )
        //    {
        //        //pConn->accessGranted();
        //    }
            if ( !pConn->setLink( this, fd, pCur->pInfo, pMap->getSSLContext() ) )
            {
                fcntl( fd, F_SETFD, FD_CLOEXEC );
                fcntl( fd, F_SETFL, flag );
                ++pConnCur;
                //pConn->tryRead();
            }
            else
            {
                close( fd );
                --(*iCount);
            }
        }
        ++pCur;
    }
    if ( pConnCur < pConnEnd )
    {
        HttpGlobals::getResManager()->recycle( pConnCur, pConnEnd - pConnCur);
    }

    return 0;
}

VHostMap * HttpListener::getSubMap(int fd )
{
    VHostMap * pMap;
    char achAddr[128];
    socklen_t addrlen = 128;
    if ( getsockname( fd, (struct sockaddr *) achAddr, &addrlen ) == -1 )
    {
        return 0;
    }
    pMap = m_pSubIpMap->getMap( (struct sockaddr *) achAddr );
    if ( !pMap )
        pMap = m_pMapVHost;
    return pMap;
}


int HttpListener::addConnection( struct conn_data * pCur, int *iCount )
{
    int fd = pCur->fd;
    if ( checkAccess( pCur ))
    {
        no_timewait( fd );
        close( fd );
        --(*iCount);
        return 0;
    }
    NtwkIOLink* pConn = HttpGlobals::getResManager()->getNtwkIOLink();
    if ( !pConn )
    {
        ERR_NO_MEM( "HttpSessionPool::getConnection()" );
        close( fd );
        --(*iCount);
        return -1;
    }
    VHostMap * pMap;
    if ( m_pSubIpMap )
    {
        pMap = getSubMap( fd );
    }
    else
        pMap = getVHostMap();
    pConn->setVHostMap( pMap );
    pConn->setLogger( getLogger());
    pConn->setRemotePort( ntohs( ((sockaddr_in *)(pCur->achPeerAddr))->sin_port) );
    if ( pConn->setLink( this, pCur->fd, pCur->pInfo, pMap->getSSLContext() ) )
    {
        HttpGlobals::getResManager()->recycle( pConn );
        close( fd );
        --(*iCount);
        return -1;
    }
    fcntl( fd, F_SETFD, FD_CLOEXEC );
    fcntl( fd, F_SETFL, HttpGlobals::getMultiplexer()->getFLTag() );
    //pConn->tryRead();
    return 0;
}



void HttpListener::onTimer()
{
}

VHostMap * HttpListener::addIpMap( const char * pIP )
{
    if ( !m_pSubIpMap )
    {
        m_pSubIpMap = new SubIpMap();
        if ( !m_pSubIpMap )
            return NULL;
    }
    VHostMap * pMap = m_pSubIpMap->addIP( pIP );
    if ( pMap )
    {
        pMap->setPort( m_pMapVHost->getPort() );
    }
    return pMap;
}

int HttpListener::addDefaultVHost( HttpVHost * pVHost )
{
    int count = 0;
    if ( m_pMapVHost->addMaping( pVHost, "*", 1 ) == 0 )
        ++count;
    if ( m_pSubIpMap )
    {
        count += m_pSubIpMap->addDefaultVHost( pVHost );
    }
    return count;
}

int HttpListener::writeStatusReport( int fd )
{
    char achBuf[1024];
    
    int len = safe_snprintf( achBuf, 1024, "LISTENER%d [%s] %s\n",
                isAdmin(), getName(), getAddrStr() );
    if ( ::write( fd, achBuf, len ) != len )
        return -1;
    if ( getVHostMap()->writeStatusReport( fd ) == -1 )
        return -1;
    if ( m_pSubIpMap && m_pSubIpMap->writeStatusReport( fd ) == -1 )
        return -1;
    if ( ::write( fd, "ENDL\n", 5 ) != 5 )
        return -1;
    return 0;
    
}
int HttpListener::mapDomainList(HttpVHost * pVHost, const char * pDomains)
{   
    return m_pMapVHost->mapDomainList(pVHost, pDomains);     
    
}

