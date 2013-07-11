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
#include "httpiolink.h"

#include <edio/multiplexer.h>
#include <http/connlimitctrl.h>
#include <http/datetime.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <socket/gsockaddr.h>
#include <sslpp/sslconnection.h>
#include <sslpp/sslcontext.h>
#include <sslpp/sslerror.h>
#include <util/accessdef.h>
#include <util/iovec.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#define IO_THROTTLE_READ    8
#define IO_THROTTLE_WRITE   16
#define IO_COUNTED          32


HttpIOLink::HttpIOLink()
{
}

HttpIOLink::~HttpIOLink()
{
}

int HttpIOLink::write( const char * pBuf, int size )
{
    IOVec iov( pBuf, size );
    return writev( iov, size );
}

void HttpIOLink::enableThrottle( int enable )
{
    if ( enable )
        s_pCur_fn_list_list = &HttpIOLink::s_fn_list_list_throttle;
    else
        s_pCur_fn_list_list = &HttpIOLink::s_fn_list_list_normal;
}


int HttpIOLink::setLink( int fd, ClientInfo * pInfo, SSLContext * pSSLContext )
{
    setfd( fd );
    m_pClientInfo = pInfo;
    setActiveTime( DateTime::s_curTime );
    m_iWant = HIO_WANT_READ;
    memset( &m_iInProcess, 0, (char *)(&m_ssl + 1) - (char *)(&m_iInProcess) );
    //set ssl context
    if ( pSSLContext )
    {
        SSL* p = pSSLContext->newSSL();
        if ( p )
        {
            HttpGlobals::getConnLimitCtrl()->incSSLConn();
            setSSL( p );
            m_ssl.toAccept();
            //acceptSSL();
        }
        else
        {
            LOG_ERR(( getLogger(), "newSSL() Failed!", getLogId()  ));
            return -1;
        }
    }
    else
    {
        setNoSSL();
    }
    pInfo->incConn();
        if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] concurrent conn: %d",
                         getLogId(), pInfo->getConns() ));
    onInitConnected();
    ++HttpGlobals::s_iIdleConns;
    m_tmToken = HttpGlobals::s_tmToken;
    return HttpGlobals::getMultiplexer()->add( this, POLLIN|POLLHUP|POLLERR );
}

int HttpIOLink::getServerAddrStr( char * pBuf, int len )
{
    char achAddr[128];
    socklen_t addrlen = 128;
    if ( getsockname( getfd(), (struct sockaddr *) achAddr, &addrlen ) == -1 )
    {
        return 0;
    }
    if ( GSockAddr::ntop( (struct sockaddr *)achAddr, pBuf, len ) == NULL )
        return 0;
    return strlen( pBuf );
}


void HttpIOLink::ignoreReadBuf()
{
    //clear the inbound data buffer
    char achDiscard[4096];
    int len = 4096;
    while( len == 4096 )
    {
        len = ::read( getfd(), achDiscard, len );
    }
    if ( len <=0 )
        closeSocket();
}

void HttpIOLink::tryRead()
{
    char ch;
    if ( ::recv( getfd(), &ch, 1, MSG_PEEK ) == 1 )
    {
        handleEvents( POLLIN );
    }
}

int HttpIOLink::handleEvents( short evt )
{
    register int event = evt;
    if ( D_ENABLED( DL_MEDIUM ))
            LOG_D(( getLogger(), "[%s] HttpIOLink::handleEvents() events=%d!",
                getLogId(), event ));
    if ( m_iState == HC_SHUTDOWN )
    {
        if ( event & (POLLHUP|POLLERR) )
        {
            closeSocket();
        }
        else
        {
            //resetRevent( POLLIN | POLLOUT );
            if ( event & POLLIN )
            {
                if ( !(m_iPeerShutdown & IO_HTTP_ERR) )
                    ignoreReadBuf();
                else
                    HttpGlobals::getMultiplexer()->suspendRead( this );
            }
        }
        return 0;
    }
    m_iInProcess = 1;
    if ( event & POLLIN )
    {
        (*m_pFnList->m_onRead_fn )( this );
    }
    if ( event & (POLLHUP | POLLERR ))
    {
        m_iPeerShutdown |= IO_PEER_HUP;
        m_iInProcess = 0;
        if ( m_iState == HC_WAITING )
            --HttpGlobals::s_iIdleConns;
        closeConnection();
        return 0;
    }
    if ( event & POLLOUT )
    {
        (*m_pFnList->m_onWrite_fn)( this );
    }
    m_iInProcess = 0;
    if ( m_iState >= HC_CLOSING )
    {
        closeConnection();
    }
    return 0;
}


void HttpIOLink::suspendRead()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpIOLink::suspendRead()...", getLogId() ));
    if ( !(( isSSL() )&&( m_ssl.wantRead())) )
        HttpGlobals::getMultiplexer()->suspendRead( this );
}

void HttpIOLink::continueRead()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpIOLink::continueRead()...", getLogId() ));
    m_iWant |= HIO_WANT_READ;
    if (( allowRead()))
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] read resumed!", getLogId() ));
        HttpGlobals::getMultiplexer()->continueRead( this );
    }
}

void HttpIOLink::suspendWrite()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpIOLink::suspendWrite()...", getLogId() ));
    m_iWant &= ~HIO_WANT_WRITE;
    if ( !(( isSSL() )&&( m_ssl.wantWrite())) )
        HttpGlobals::getMultiplexer()->suspendWrite(this);
}

void HttpIOLink::continueWrite()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpIOLink::continueWrite()...", getLogId() ));
    m_iWant |= HIO_WANT_WRITE;
    if ( allowWrite() )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] write resumed!", getLogId() ));
/*        short revents = getRevents();
        if ( revents & POLLOUT )
            handleEvents( revents );
        else*/
            HttpGlobals::getMultiplexer()->continueWrite( this );
    }
}

void HttpIOLink::switchWriteToRead()
{
    m_iWant = HIO_WANT_READ;
    HttpGlobals::getMultiplexer()->switchWriteToRead( this );
}


///////////////////////////////////////////////////////////////////////
// SSL
///////////////////////////////////////////////////////////////////////

void HttpIOLink::updateSSLEvent()
{
    if ( wantWrite() )
        HttpGlobals::getMultiplexer()->continueWrite( this );
    if ( wantRead() )
        HttpGlobals::getMultiplexer()->continueRead( this );
}


void HttpIOLink::checkSSLReadRet( int ret )
{
    if ( ret > 0 )
    {
        m_lBytesRead += ret;
        HttpGlobals::s_lSSLBytesRead += ret;
        setActiveTime( DateTime::s_curTime );
        updateSSLEvent();
    }
    else if ( !ret )
    {
        if ( m_ssl.wantRead() )
            HttpGlobals::getMultiplexer()->switchWriteToRead( this );
        if ( m_ssl.wantWrite() )
            HttpGlobals::getMultiplexer()->switchReadToWrite( this );
    }
    else if ( m_iState != HC_SHUTDOWN )
        m_iState = HC_CLOSING;
}

int HttpIOLink::readExSSL( HttpIOLink* pThis, char * pBuf, int size )
{
    int ret;
    assert( pBuf );
    ret = pThis->m_ssl.read( pBuf, size );
    pThis->checkSSLReadRet( ret );
    //DEBUG CODE:
//    if ( ret > 0 )
//        ::write( 1, pBuf, ret );
    return ret;
}

int HttpIOLink::writevExSSL( HttpIOLink * pThis, IOVec &vector, int total )
{
    int ret = 0;

    const struct iovec * vect = vector.get();
    const struct iovec * pEnd = vect + vector.len();
    const char * pBuf;
    int bufSize;
    int written;

    char * pBufEnd;
    char * pCurEnd;
    char achBuf[4096];
    pBufEnd = achBuf + 4096;
    pCurEnd = achBuf;
    for( ; vect < pEnd ;  )
    {
        pBuf =( const char *) vect->iov_base;
        bufSize = vect->iov_len;
        if ( bufSize < 1024 )
        {
            if ( pBufEnd - pCurEnd > bufSize )
            {
                memmove( pCurEnd, pBuf, bufSize );
                pCurEnd += bufSize;
                ++vect;
                if ( vect < pEnd )
                    continue;
            }
            pBuf = achBuf;
            bufSize = pCurEnd - pBuf;
            pCurEnd = achBuf;
        }
        else if ( pCurEnd != achBuf )
        {
            pBuf = achBuf;
            bufSize = pCurEnd - pBuf;
            pCurEnd = achBuf;
        }
        else
            ++vect;
        written = pThis->m_ssl.write( pBuf, bufSize );
        if ( written > 0 )
        {
            pThis->m_lBytesSent += written;
            HttpGlobals::s_lSSLBytesWritten += written;
            pThis->setActiveTime( DateTime::s_curTime );
            ret += written;
            if ( written < bufSize )
            {
                pThis->updateSSLEvent();
                break;
            }
        }
        else if ( !written )
        {
            pThis->setSSLAgain();
            break;
        }
        else if ( pThis->m_iState != HC_SHUTDOWN )
        {
            pThis->m_iState = HC_CLOSING;
            return -1;
        }
    }
    return ret;
}

void HttpIOLink::setSSLAgain()
{
    if ( m_ssl.wantRead() )
        HttpGlobals::getMultiplexer()->switchWriteToRead( this );
    if ( m_ssl.wantWrite() )
        HttpGlobals::getMultiplexer()->switchReadToWrite( this );
}

int HttpIOLink::flushSSL()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpIOLink::flushSSL()...", getLogId() ));
    int ret;
    switch( ( ret = getSSL()->flush() ))
    {
    case 0:
        setSSLAgain();
        continueWrite();
        return 1;
    case 1:
        return 0;
    case -1:
        m_iState = HC_CLOSING;
        break;
    }
    return ret;
}

int HttpIOLink::onWriteSSL( HttpIOLink * pThis )
{
    if ( !pThis->m_ssl.wantWrite() )
    {
        int last = pThis->m_ssl.lastRead();
        if ( !pThis->m_ssl.isConnected()||( last ))
        {
            pThis->SSLAgain();
//            if (( !pThis->m_ssl.isConnected() )||(last ))
                return 0;
        }
    }
    return pThis->doWrite();
}


int HttpIOLink::onReadSSL( HttpIOLink * pThis )
{
    if ( pThis->m_ssl.wantRead() )
    {
        int last = pThis->m_ssl.lastWrite();
        if ( !pThis->m_ssl.isConnected()||( last ))
        {
            pThis->SSLAgain();
//            if (( !pThis->m_ssl.isConnected() )||(last ))
                return 0;
        }
    }
    return pThis->doRead();
}


int HttpIOLink::closeSSL( HttpIOLink * pThis )
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( pThis->getLogger(), "[%s] Shutting down SSL ...", pThis->getLogId() ));
    pThis->m_ssl.shutdown(0);
    pThis->m_ssl.release();
    HttpGlobals::getConnLimitCtrl()->decSSLConn();
    pThis->setNoSSL();
    return close_( pThis );
}


///////////////////////////////////////////////////////////////////////
// Plain
///////////////////////////////////////////////////////////////////////


int HttpIOLink::close_( HttpIOLink * pThis )
{
    pThis->m_iState = HC_SHUTDOWN;

    if ( pThis->m_iPeerShutdown & (IO_PEER_ERR | IO_PEER_HUP) )
        pThis->closeSocket();
    else
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( pThis->getLogger(), "[%s] Shutting down out-bound socket ...", pThis->getLogId() ));
        ::shutdown( pThis->getfd(), SHUT_WR );
        HttpGlobals::getMultiplexer()->switchWriteToRead( pThis );
        if ( !(pThis->m_iPeerShutdown & IO_COUNTED) )
        {
            HttpGlobals::getConnLimitCtrl()->decConn();
            pThis->m_pClientInfo->decConn();
            pThis->m_iPeerShutdown |= IO_COUNTED;
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( pThis->getLogger(), "[%s] Available Connections: %d, concurrent conn: %d",
                         pThis->getLogId(), HttpGlobals::getConnLimitCtrl()->availConn(),
                         pThis->m_pClientInfo->getConns() ));
        }
    }
//    pThis->closeSocket();
    return -1;
}

void HttpIOLink::closeSocket()
{
    if ( getfd() == -1 )
        return;
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] Close socket ...", getLogId() ));
    HttpGlobals::getMultiplexer()->remove( this );
    if ( m_pFnList == s_pCur_fn_list_list->m_pSSL )
    {
        m_ssl.release();
        HttpGlobals::getConnLimitCtrl()->decSSLConn();
        setNoSSL();
    }
    if ( !(m_iPeerShutdown & IO_COUNTED) )
    {
        HttpGlobals::getConnLimitCtrl()->decConn();
        m_pClientInfo->decConn();
        if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] Available Connections: %d, concurrent conn: %d",
                         getLogId(), HttpGlobals::getConnLimitCtrl()->availConn(),
                         m_pClientInfo->getConns() ));
    }
    //printf( "socket: %d closed\n", getfd() );
    ::close( getfd() );
    setfd( -1 );
    recycle();
}

int HttpIOLink::onRead( HttpIOLink * pThis )
{
    return pThis->onReadEx();
}

int HttpIOLink::onWrite(HttpIOLink *pThis )
{   return pThis->doWrite();   }


static int matchToken( int token )
{
    if ( HttpGlobals::s_tmPrevToken < HttpGlobals::s_tmToken )
        return (( token > HttpGlobals::s_tmPrevToken )&&( token <= HttpGlobals::s_tmToken ));
    else 
        return (( token > HttpGlobals::s_tmPrevToken )||( token <= HttpGlobals::s_tmToken ));
}


void HttpIOLink::onTimer_( HttpIOLink * pThis )
{
    if ( matchToken( pThis->m_tmToken ) )
    {
        if ( pThis->detectClose() )
            return;
        if ( pThis->getState() == HC_THROTTLING )
        {
            pThis->onWriteEx();
        }
        pThis->onTimerEx();
    }
}

int HttpIOLink::checkReadRet( int ret, int size )
{
    //Note: read return 0 means TCP receive FIN, the other side shutdown the write
    //       side of the socket, should leave it alone. Should get other signals
    //       if it is closed. should I?
    //      Content-length must be present in request header, and client
    //      can not shutdown the write side to indicating the end of the request
    //      body, so it is ok to do it.
    if ( ret < size )
        resetRevent( POLLIN );
    if ( ret > 0 )
    {
        m_lBytesRead += ret;
        HttpGlobals::s_lBytesRead += ret;
        setActiveTime( DateTime::s_curTime );
    }
    else if ( ret == 0 )
    {
        if ( m_iState != HC_SHUTDOWN )
        {
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] End of stream detected, CLOSING!",
                    getLogId() ));
           //have the connection closed quickly
            m_iPeerShutdown |= IO_PEER_HUP;
            m_iState = HC_CLOSING;
        }
        ret = -1;
    }
    else if ( ret == -1 )
    {
        switch( errno )
        {
        case ECONNRESET:
            //incase client shutdown the writting side after sending the request
            // and waiting for the response, we can't close the connection before
            // we finish write the response back.
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] read error: %s\n",
                        getLogId(), strerror( errno ) ));
        case EAGAIN:
        case EINTR:
            ret = 0;
            break;
        default:
            if ( m_iState != HC_SHUTDOWN )
                m_iState = HC_CLOSING;
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] read error: %s\n",
                        getLogId(), strerror( errno ) ));
        }
    }
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] Read from client: %d\n", getLogId(), ret ));
    return ret;

}

int HttpIOLink::readEx( HttpIOLink* pThis, char * pBuf, int size )
{
    int ret;
    assert( pBuf );
    ret = ::read( pThis->getfd(), pBuf, size );
    ret = pThis->checkReadRet( ret, size );
//    if ( ret > 0 )
//        ::write( 1, pBuf, ret );
    return ret;
}

#if !defined( NO_SENDFILE )

#include <util/gsendfile.h>

#if defined(__FreeBSD__ ) || defined(__NetBSD__) || defined(__OpenBSD__) 

#include <sys/uio.h>
int HttpIOLink::sendfileEx( IOVec &vector, int headerTotal, int fdSrc, off_t off, size_t size )
{
    struct sf_hdtr hdtr;
    struct sf_hdtr * pHeader;
    
    if ( headerTotal )
    {
        memset( &hdtr, 0, sizeof( hdtr ) );
        hdtr.headers = (struct iovec *)vector.get();
        hdtr.hdr_cnt = vector.len();
        pHeader = &hdtr;
    }
    else
    {
        pHeader = NULL;
    }
    off_t written;
    size = (size & ((1<<30)-1)) - headerTotal;
    headerTotal += size;
    int ret = ::sendfile( fdSrc, getfd(), off, size, pHeader, &written, 0 );
    if ( D_ENABLED( DL_MORE ))
        LOG_D(( "[%s] sendfile() ret: %d, written: %ul", getLogId(), ret, written ));
    if ( written > 0 )
        ret = written;
    return checkWriteRet( ret, headerTotal );
}

#endif

#if defined(sun) || defined(__sun)

int HttpIOLink::sendfileEx( IOVec &vector, int headerTotal, int fdSrc, off_t off, size_t size )
{
   
    int n = 0 ;
    IOVec::iterator iter;
    sendfilevec_t vec[20];
    size = (size & ((1<<30)-1)) - headerTotal;
    if ( headerTotal )
    {
        for( iter = vector.begin(); iter < vector.end(); ++iter )
        {
            vec[n].sfv_fd   = SFV_FD_SELF;
            vec[n].sfv_flag = 0;
            vec[n].sfv_off  = (off_t)iter->iov_base;
            vec[n].sfv_len  = iter->iov_len;
            ++n;
        }
    }
    
    vec[n].sfv_fd   = fdSrc;
    vec[n].sfv_flag = 0;
    vec[n].sfv_off  = off;
    vec[n].sfv_len  = size;
    ++n;

    size_t written;
    headerTotal += size;
    //if ( D_ENABLED( DL_MORE ))
    //    LOG_D(( "[%s] sendfilev() to sent %d bytes", getLogId(), headerTotal ));
    int ret = ::sendfilev( getfd(), vec, n, &written );
    //if ( D_ENABLED( DL_MORE ))
    //    LOG_D(( "[%s] sendfilev() ret: %d, written: %ul", getLogId(), ret, written ));
    if (( !ret )||( errno = EAGAIN ))
        ret = written;
    return checkWriteRet( ret, headerTotal );
}

#endif

#if defined(linux) || defined(__linux) || defined(__linux__) || \
    defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)||\
    defined(__gnu_linux__)||defined(HPUX) ||\
    defined(sun) || defined(__sun)

int HttpIOLink::sendfile( IOVec &iov, int &headerTotal, int fdSrc, off_t off, size_t size )
{
    int len = 0;
    int written;
    ThrottleControl * pCtrl = getThrottleCtrl();
    if ( headerTotal > 0 )
    {
        if ( pCtrl )
        {
            len = writevExT( this, iov, headerTotal );
            if ( len <= 0 )
                return len;
        }
        else
        {
            len = ::writev( getfd(), iov.get(), iov.len() );
            if ( len <= 0 )
                return checkWriteRet( len, headerTotal );
            HttpGlobals::s_lBytesWritten += len;
            setActiveTime( DateTime::s_curTime );
        }
        if ( len >= headerTotal )
        {
            iov.clear();
            headerTotal = 0;
        }
        else
        {
            resetRevent( POLLOUT );
            headerTotal -= len;
            iov.finish( len );
            return 0;
        }
    }
    
    if ( pCtrl )
    {
        int Quota = pCtrl->getOSQuota();
        if ( size > (unsigned int )Quota + ( Quota >> 3) )
        {
            size = Quota;
        }
    }
    else
        size = size & ((1<<30)-1);
    if ( size <= 0 )
        return 0;
    written = gsendfile( getfd(), fdSrc, &off, size );
#if defined(linux) || defined(__linux) || defined(__linux__) || \
    defined(__gnu_linux__)
    if ( written == 0 )
    {
        written = -1;
        errno = EPIPE;
    }
#endif
    len = checkWriteRet( written, size );
    if ( pCtrl )
    {
        int Quota = pCtrl->getOSQuota();
        if (Quota - len < 10 )
        {
            pCtrl->useOSQuota(  Quota );
            HttpGlobals::getMultiplexer()->suspendWrite( this );
        }
        else
        {
            pCtrl->useOSQuota(  len );
        }
    }
    return len;
}


#else

int HttpIOLink::sendfile( IOVec &iov, int &headerTotal, int fdSrc, off_t off, size_t size )
{
    int len = 0;
    ThrottleControl * pCtrl = getThrottleCtrl();

    if ( pCtrl )
    {
        int Quota = pCtrl->getOSQuota();
        Quota += Quota >> 3;
        if ( size + headerTotal > (unsigned int )Quota  )
        {
            if ( headerTotal > Quota )
                len = 1;
            else
                size = Quota - headerTotal;
        }
    }
    if ( len )
    {
        len = writevExT( this, iov, headerTotal );
        if ( len <= 0 )
            return len;
    }
    else
    {
        len = sendfileEx( iov, headerTotal, fdSrc, off, size );
        if ( len <= 0 )
            return len;
        if ( pCtrl )
        {
            int Quota = pCtrl->getOSQuota();
            if (Quota - len < 10 )
            {
                pCtrl->useOSQuota( Quota );
                HttpGlobals::getMultiplexer()->suspendWrite( this );
            }
            else
            {
                pCtrl->useOSQuota( len );
            }
        }
    }
    if ( headerTotal > 0 )
    {
        if ( len >= headerTotal )
        {
            iov.clear();
            len -= headerTotal;
            headerTotal = 0;
        }
        else
        {
            headerTotal -= len;
            iov.finish( len );
            return 0;
        }
    }
    return len;
}

#endif

#endif 


int HttpIOLink::writevEx( HttpIOLink * pThis, IOVec &vector, int total )
{
    int len = ::writev( pThis->getfd(), vector.get(), vector.len() );
    len = pThis->checkWriteRet( len, total );
    //FIXME: debug code
//    if ( len > 0 )
//    {
//        int left = len;
//        const struct iovec* pVec = vector.get();
//        while( left > 0 )
//        {
//            int writeLen = pVec->iov_len;
//            if ( writeLen > left )
//                writeLen = left;
//            ::write( 1, pVec->iov_base, writeLen );
//            ++pVec;
//            left -= writeLen;
//        }
//    }
    return len;

}


int HttpIOLink::checkWriteRet( int len, int size )
{
    if ( len < size )
        resetRevent( POLLOUT );
    else
        setRevent( POLLOUT );
    if ( len > 0 )
    {
        m_lBytesSent += len;
        HttpGlobals::s_lBytesWritten += len;
        setActiveTime( DateTime::s_curTime );
    }
    else if ( len == -1 )
    {
        switch( errno )
        {
        case EINTR:
        case EAGAIN:
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] [write] errstr=%s!\n", getLogId(),
                        strerror( errno ) ));
            len = 0;
            break;
        default:
            if ( m_iState != HC_SHUTDOWN )
            {
                m_iState = HC_CLOSING;
                setPeerShutdown( IO_PEER_ERR );
            }
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] write error: %s\n",
                        getLogId(), strerror( errno ) ));
        }
    }
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(),  "[%s] Written to client: %d\n", getLogId(), len ));
    return len;
}

int HttpIOLink::detectClose()
{
    if (( getState() >= HC_THROTTLING )&&
        ( getState() != HC_WRITING ))
    {
        char ch;
        if ( !::recv( getfd(), &ch, 1, MSG_PEEK ) )
        {
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] peer connection close detected!\n", getLogId() ));
            //have the connection closed faster
            setPeerShutdown( IO_PEER_ERR );

            closeConnection( 1 );
            return 1;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////
// Throttle
///////////////////////////////////////////////////////////////////////

int HttpIOLink::onReadT( HttpIOLink * pThis )
{
    return pThis->doReadT();
}

int HttpIOLink::onWriteT( HttpIOLink *pThis )
{
    if ( pThis->allowWrite() )
        return pThis->doWrite();
    else
        HttpGlobals::getMultiplexer()->suspendWrite( pThis );
    return 0;
}

void HttpIOLink::onTimer_T( HttpIOLink * pThis )
{
    if ( matchToken( pThis->m_tmToken ) )
    {
//        if ( D_ENABLED( DL_MORE ))
//            LOG_D(( pThis->getLogger(),  "[%s] conn token:%d, global Token: %d\n",
//                    pThis->getLogId(), pThis->m_tmToken, HttpGlobals::s_tmToken ));
        if ( pThis->detectClose() )
            return;
        if ( pThis->getState() == HC_THROTTLING )
        {
            pThis->onWriteEx();
        }
//            if ( D_ENABLED( DL_MORE ))
//                LOG_D(( pThis->getLogger(),  "[%s] output avail:%d. state: %d \n",
//                        pThis->getLogId(),
//                        pThis->getClientInfo()->getThrottleCtrl().getOSQuota(),
//                        pThis->getState() ));
        if ( pThis->allowWrite() && pThis->wantWrite() )
        {
            //pThis->doWrite();
            //if (  pThis->allowWrite() && pThis->wantWrite() )
            HttpGlobals::getMultiplexer()->continueWrite( pThis );
        }
        if ( pThis->allowRead() && pThis->wantRead() )
        {
            if ( pThis->m_iState != HC_WAITING )
                pThis->doReadT();
            if ( pThis->allowRead() && pThis->wantRead() )
                HttpGlobals::getMultiplexer()->continueRead( pThis );
        }
        pThis->onTimerEx();
    }
}


int HttpIOLink::readExT( HttpIOLink* pThis, char * pBuf, int size )
{
    ThrottleControl * pTC = pThis->getThrottleCtrl();
    int iQuota = pTC->getISQuota();
    if ( iQuota <= 0 )
    {
        HttpGlobals::getMultiplexer()->suspendRead( pThis );
        return 0;
    }
    if ( size > iQuota )
        size = iQuota;
    assert( pBuf );
    int ret = ::read( pThis->getfd(), pBuf, size );
    ret = pThis->checkReadRet( ret, size );
    if ( ret > 0 )
    {
        pTC->useISQuota( ret );
//        ::write( 1, pBuf, ret );
        if ( !pTC->getISQuota() )
        {
            HttpGlobals::getMultiplexer()->suspendRead( pThis );
        }
    }
    return ret;
}



int HttpIOLink::writevExT( HttpIOLink * pThis, IOVec &vector, int total )
{
    int len = 0;
    ThrottleControl * pCtrl = pThis->getThrottleCtrl();
    int Quota = pCtrl->getOSQuota();
    if ( Quota <= 0 )
    {
        HttpGlobals::getMultiplexer()->suspendWrite( pThis );
        return 0;
    }
//    if ( D_ENABLED( DL_LESS ))
//        LOG_D(( pThis->getLogger(),  "[%s] Quota:%d, to write: %d\n",
//                    pThis->getLogId(), Quota, total ));
    if ( (unsigned int)total > (unsigned int )Quota + ( Quota >> 3) )
    {
        IOVec iov( vector );
        total = iov.shrinkTo( Quota, Quota >> 3 );
        len = ::writev( pThis->getfd(), iov.get(), iov.len() );
    }
    else
        len = ::writev( pThis->getfd(), vector.get(), vector.len() );
    len = pThis->checkWriteRet( len, total );
    if (Quota - len < 10 )
    {
        pCtrl->useOSQuota(  Quota );
        HttpGlobals::getMultiplexer()->suspendWrite( pThis );
    }
    else
    {
        pCtrl->useOSQuota(  len );
    }
    return len;

}

///////////////////////////////////////////////////////////////////////
// Throttle + SSL
///////////////////////////////////////////////////////////////////////


//void HttpIOLink::onTimerSSL_T( HttpIOLink * pThis )
//{
//    if ( pThis->detectClose() )
//        return;
//    if ( pThis->allowWrite() &&
//        (( pThis->m_ssl.wantWrite() )||pThis->wantWrite() ))
//        pThis->doWriteT();
//    if ( pThis->allowRead() &&
//        (( pThis->m_ssl.wantRead() )||pThis->wantRead() ))
//    {
//        pThis->doReadT();
//    }
//    pThis->onTimerEx();
//}

void HttpIOLink::onTimerSSL_T( HttpIOLink * pThis )
{
    if ( matchToken( pThis->m_tmToken ) )
    {
        if ( pThis->detectClose() )
            return;
        if ( pThis->getState() == HC_THROTTLING )
        {
            pThis->onWriteEx();
        }
        if ( pThis->allowWrite() && ( pThis->m_ssl.wantWrite() ))
            onWriteSSL_T( pThis );
        if ( pThis->allowRead() && ( pThis->m_ssl.wantRead() ))
            onReadSSL_T( pThis );
        if ( pThis->allowWrite() && pThis->wantWrite() )
        {
            pThis->doWrite();
            if (  pThis->allowWrite() && pThis->wantWrite() )
                HttpGlobals::getMultiplexer()->continueWrite( pThis );
        }
        if ( pThis->allowRead() && pThis->wantRead() )
        {
            if ( pThis->m_iState != HC_WAITING )
                pThis->doReadT();
            if ( pThis->allowRead() && pThis->wantRead() )
                HttpGlobals::getMultiplexer()->continueRead( pThis );
        }
        pThis->onTimerEx();
    }
}


int HttpIOLink::onReadSSL_T( HttpIOLink * pThis )
{
    if ( pThis->m_ssl.wantRead() )
    {
        int last = pThis->m_ssl.lastWrite();
        if ( !pThis->m_ssl.isConnected()||( last ))
        {
            pThis->SSLAgain();
//            if (( !pThis->m_ssl.isConnected() )||(last ))
                return 0;
        }
    }
    return pThis->doReadT();
}

int HttpIOLink::onWriteSSL_T( HttpIOLink * pThis )
{
    if ( !pThis->m_ssl.wantWrite() )
    {
        int last = pThis->m_ssl.lastRead();
        if ( !pThis->m_ssl.isConnected()||( last ))
        {
            pThis->SSLAgain();
//            if (( !pThis->m_ssl.isConnected() )||(last ))
                return 0;
        }

    }
    if ( pThis->allowWrite() )
        return pThis->doWrite();
    else
        HttpGlobals::getMultiplexer()->suspendWrite( pThis );
    return 0;
}

static char s_errUseSSL[] =
//    "HTTP/1.0 200 OK\r\n"
//    "Cache-Control: private, no-cache, max-age=0\r\n"
//    "Pragma: no-cache\r\n"
//    "Connection: Close\r\n\r\n"
    "<html><head><title>400 Bad Request</title></head><body>\n"
    "<h2>HTTPS is required</h2>\n"
    "<p>This is an SSL protected page, please use the HTTPS scheme instead of "
    "the plain HTTP scheme to access this URL.<br />\n"
    "<blockquote>Hint: The URL should starts with <b>https</b>://</blockquote> </p>\n"
    "<hr />\n"
    "Powered By LiteSpeed Web Server<br />\n"
    "<a href='http://www.litespeedtech.com'><i>http://www.litespeedtech.com</i></a>\n"
    "</body></html>\n";
int HttpIOLink::acceptSSL()
{
    int ret = m_ssl.accept();
    if ( ret == 1 )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] [SSL] accepted!\n", getLogId() ));
        if ( (HttpGlobals::s_iConnsPerClientHardLimit < 1000 )
            &&(m_pClientInfo->getAccess() != AC_TRUST)
            &&( m_pClientInfo->incSslNewConn() > 
                 (HttpGlobals::s_iConnsPerClientHardLimit << 1) ))
        {
            LOG_WARN(( getLogger(), "[%s] [SSL] too many new SSL connections: %d, possible SSL negociation based attack, block!", getLogId(), m_pClientInfo->getSslNewConn() ));
            m_pClientInfo->setOverLimitTime( DateTime::s_curTime );
            m_pClientInfo->setAccess( AC_BLOCK );
        }

    }
    else if ( errno == EIO )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] SSL_accept() failed!: %s "
                    , getLogId(), SSLError().what() ));
        ::write( getfd(), s_errUseSSL, sizeof( s_errUseSSL ) - 1 );
    }

    return ret;
}

int HttpIOLink::SSLAgain()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] [SSL] SSLAgain()!\n", getLogId() ));
    int ret = 0;
    switch( m_ssl.getStatus() )
    {
    case SSLConnection::CONNECTING:
        ret = m_ssl.connect();
        break;
    case SSLConnection::ACCEPTING:
        ret = acceptSSL();
        if ( ret == 0 )
        {
            setState( HC_READING );
        }
        break;
    case SSLConnection::SHUTDOWN:
        ret = m_ssl.shutdown( 1 );
        break;
    case SSLConnection::CONNECTED:
        if ( m_ssl.lastRead() )
            return onReadEx();
        if ( m_ssl.lastWrite() )
            return doWrite();
    }
    switch( ret )
    {
    case 0:
        if ( m_ssl.wantRead() )
            HttpGlobals::getMultiplexer()->switchWriteToRead( this );
        if ( m_ssl.wantWrite() )
            HttpGlobals::getMultiplexer()->switchReadToWrite( this );
        break;
    case -1:
        closeConnection();
        break;
    }
    return ret;

}

int HttpIOLink::readExSSL_T( HttpIOLink* pThis, char * pBuf, int size )
{
    ThrottleControl * pTC = pThis->getThrottleCtrl();
    int iQuota = pTC->getISQuota();
    if ( iQuota <= 0 )
    {
        HttpGlobals::getMultiplexer()->suspendRead( pThis );
        return 0;
    }
    if ( size > iQuota )
        size = iQuota;
    pThis->m_iPeerShutdown &= ~IO_THROTTLE_READ;
    int ret = pThis->m_ssl.read( pBuf, size );
    pThis->checkSSLReadRet( ret );
    if ( ret > 0 )
    {
        pTC->useISQuota( ret );
        //::write( 1, pBuf, ret );
        if ( !pTC->getISQuota() )
        {
            HttpGlobals::getMultiplexer()->suspendRead( pThis );
            pThis->m_iPeerShutdown |= IO_THROTTLE_READ;
        }
    }
    return ret;
}


int HttpIOLink::writevExSSL_T( HttpIOLink * pThis, IOVec &vector, int total )
{
    ThrottleControl * pCtrl = pThis->getThrottleCtrl();
    int Quota = pCtrl->getOSQuota();
    if ( Quota <= pThis->m_iSslLastWrite / 2 )
    {
        HttpGlobals::getMultiplexer()->suspendWrite( pThis );
        return 0;
    }
    unsigned int allowed = (unsigned int)Quota + ( Quota >> 3);
    int ret = 0;
    const struct iovec * vect = vector.get();
    const struct iovec * pEnd = vect + vector.len();
    
    //Make OpenSSL happy, not to retry with smaller buffer
    if ( Quota < pThis->m_iSslLastWrite )
    {
        Quota = allowed = pThis->m_iSslLastWrite;
    }
    char * pBufEnd;
    char * pCurEnd;
    char achBuf[4096];
    pBufEnd = achBuf + 4096;
    pCurEnd = achBuf;
    for( ; ret < Quota && vect < pEnd ;  )
    {
        const char * pBuf =( const char *) vect->iov_base;
        int bufSize;
        // Use "<=" instead of "<", may get access violation
        // when ret = 0, and allowed = vect->iov_len
        if ( vect->iov_len <= allowed - ret )
            bufSize = vect->iov_len;
        else
        {
            bufSize = Quota - ret;
            if ( *(pBuf + bufSize ) == '\n' )
                ++bufSize;
        }
        if ( bufSize < 1024 )
        {
            if ( pBufEnd - pCurEnd > bufSize )
            {
                memmove( pCurEnd, pBuf, bufSize );
                pCurEnd += bufSize;
                ++vect;
                if (( vect < pEnd )&&( ret + ( pCurEnd - pBuf ) < Quota ))
                    continue;
            }
            pBuf = achBuf;
            bufSize = pCurEnd - pBuf;
            pCurEnd = achBuf;
        }
        else if ( pCurEnd != achBuf )
        {
            pBuf = achBuf;
            bufSize = pCurEnd - pBuf;
            pCurEnd = achBuf;
        }
        else
            ++vect;
        int written = pThis->m_ssl.write( pBuf, bufSize );
        if ( D_ENABLED( DL_MORE ))
            LOG_D(( "[%s] to write %d bytes, written %d bytes",
                pThis->getLogId(), bufSize, written ));
        if ( written > 0 )
        {
            pThis->m_lBytesSent += written;
            HttpGlobals::s_lSSLBytesWritten += written;
            pThis->m_iSslLastWrite = 0;
            pThis->setActiveTime( DateTime::s_curTime );
            ret += written;
            if ( written < bufSize )
            {
                pThis->updateSSLEvent();
                break;
            }
        }
        else if ( !written )
        {
            pThis->m_iSslLastWrite = bufSize;
            pThis->setSSLAgain();
            break;
        }
        else if ( pThis->m_iState != HC_SHUTDOWN )
        {
            if ( D_ENABLED( DL_MORE ))
                LOG_D(( "[%s] SSL error: %s, mark connection to be closed",
                    pThis->getLogId(), SSLError().what() ));
            pThis->m_iState = HC_CLOSING;
            return -1;
        }
    }
    if (Quota - ret < 10 )
    {
        pCtrl->useOSQuota(  Quota );
        HttpGlobals::getMultiplexer()->suspendWrite( pThis );
        pThis->m_iPeerShutdown |= IO_THROTTLE_WRITE;
    }
    else
    {
        pCtrl->useOSQuota(  ret );
        pThis->m_iPeerShutdown &= ~IO_THROTTLE_WRITE;
    }
    return ret;
}

void HttpIOLink::suspendEventNotify()
{
    if ( !HttpGlobals::s_iMultiplexerType )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] remove fd:%d from multiplexer!\n", getLogId(), getfd() ));
        HttpGlobals::getMultiplexer()->remove( this );
    }
}

void HttpIOLink::resumeEventNotify()
{
    if ( !HttpGlobals::s_iMultiplexerType )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] add fd:%d back to multiplexer!\n", getLogId(), getfd() ));
        HttpGlobals::getMultiplexer()->add( this, POLLHUP|POLLERR );
    }
}

void HttpIOLink::changeClientInfo( ClientInfo * pInfo )
{
    if ( pInfo == m_pClientInfo )
        return;
    m_pClientInfo->decConn();
    pInfo->incConn();
    m_pClientInfo = pInfo;
}



