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
#include "ntwkiolink.h"

#include <edio/multiplexer.h>
#include <http/connlimitctrl.h>
#include <util/datetime.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <http/httpsession.h>
#include <http/httpresourcemanager.h>
#include <http/httplistener.h>
#include <spdy/spdyconnection.h>

#include <socket/gsockaddr.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sslpp/sslconnection.h>
#include <sslpp/sslcontext.h>
#include <sslpp/sslerror.h>

#include <util/accessdef.h>
#include <util/iovec.h>
#include "util/ssnprintf.h"

#include <netinet/tcp.h>

#define IO_THROTTLE_READ    8
#define IO_THROTTLE_WRITE   16
#define IO_COUNTED          32

//#define SPDY_PLAIN_DEV     


#include <../addon/include/ls.h>
#include <lsiapi/lsiapihooks.h>


NtwkIOLink::NtwkIOLink()
    : m_sessionHooks(0)
{
    m_hasBufferedData = 0;
    m_pModuleConfig = NULL;
}


NtwkIOLink::~NtwkIOLink()
{
    LsiapiBridge::releaseModuleData( LSI_MODULE_DATA_L4, getModuleData());
}

int NtwkIOLink::writev( const struct iovec * vector, int len )
{
    int written = 0;
    
    if ( m_iHeaderToSend > 0 )
    {
        m_iov.append(vector, len);

        written = writev_internal( m_iov.get(), m_iov.len(), 0);
        if ( written >= m_iHeaderToSend )
        {
            m_iov.clear();
            written -= m_iHeaderToSend;
            m_iHeaderToSend = 0;
        }
        else
        {
            m_iov.pop_back(len);
            if ( written > 0 )
            {
                m_iHeaderToSend -= written;
                m_iov.finish( written );
                return 0;
            }
        }
    }
    else
        written = writev_internal( vector, len, 0 );
    
    return written;
}

int NtwkIOLink::writev_internal( const struct iovec * vector, int len, int flush_flag )
{
    const LsiApiHooks *pWritevHooks = m_sessionHooks.get(LSI_HKPT_L4_SENDING);
    if ( !pWritevHooks ||( pWritevHooks->size() == 0))
        return (*m_pFpList->m_writev_fp)( (LsiSession *)this, (struct iovec *)vector, len );    
    int ret;
    lsi_cb_param_t param;
    lsi_hook_info_t hookInfo;
    param._session = (LsiSession *)this;
    int    flag_out = 0;

    hookInfo._hooks = pWritevHooks;
    hookInfo._termination_fp = (void*)m_pFpList->m_writev_fp;
    param._cur_hook = (void *)pWritevHooks->begin();
    param._hook_info = &hookInfo;
    param._param = vector;
    param._param_len = len;
    param._flag_out = &flag_out;
    param._flag_in = flush_flag;
    ret = (*(((LsiApiHook *)param._cur_hook)->_cb))( &param );
    m_hasBufferedData = flag_out;
    LOG_D (( "[NtwkIOLink::writev] ret %d hasData %d", ret, m_hasBufferedData ));
    return ret;
}

int NtwkIOLink::read( char * pBuf, int size )
{   
    const LsiApiHooks *pReadHooks = m_sessionHooks.get(LSI_HKPT_L4_RECVING);
    if ( !pReadHooks ||( pReadHooks->size() == 0))
        return (*m_pFpList->m_read_fp)( this, pBuf, size );
    
    int ret;
    lsi_cb_param_t param;
    lsi_hook_info_t hookInfo;
    param._session = (LsiSession *)this;

    hookInfo._hooks = pReadHooks;
    hookInfo._termination_fp = (void*)m_pFpList->m_read_fp;
    param._cur_hook = (void *)((LsiApiHook *)pReadHooks->end() - 1);
    param._hook_info = &hookInfo;
    param._param = pBuf;
    param._param_len = size;
    param._flag_out = NULL;
    ret = (*(((LsiApiHook *)param._cur_hook)->_cb))( &param );
    LOG_D (( "[NtwkIOLink::read] read  %d", ret ));
    return ret;
}


int NtwkIOLink::write( const char * pBuf, int size )
{
    IOVec iov( pBuf, size );
    return writev( iov.get(), iov.len() );
}

void NtwkIOLink::enableThrottle( int enable )
{
    if ( enable )
        s_pCur_fp_list_list = &NtwkIOLink::s_fp_list_list_throttle;
    else
        s_pCur_fp_list_list = &NtwkIOLink::s_fp_list_list_normal;
}

int NtwkIOLink::setupHandler( HiosProtocol verSpdy )
{
    HioStreamHandler * pHandler;
#ifdef SPDY_PLAIN_DEV
    if ( !isSSL() && (verSpdy == HIOS_PROTO_HTTP) )
        verSpdy = HIOS_PROTO_SPDY3;
#endif
    if ( verSpdy != HIOS_PROTO_HTTP )
    {
        SpdyConnection * pConn = new SpdyConnection();
        if ( !pConn )
            return -1;
        clearLogId();
        if ( verSpdy == HIOS_PROTO_SPDY31 )
        {
            verSpdy = HIOS_PROTO_SPDY3;
            pConn->enableSessionFlowCtrl();
        }
        pConn->init( verSpdy );
        pHandler = pConn;
    }
    else
    {
        HttpSession *pSession = HttpGlobals::getResManager()->getConnection();
        if ( !pSession )
            return -1;
        pSession->setNtwkIOLink( this );
        pHandler = pSession;
    }

    setProtocol( verSpdy );

    pHandler->assignStream( this );    
    pHandler->onInitConnected();
    return 0;
    
}

int NtwkIOLink::setLink(HttpListener *pListener,  int fd, ClientInfo * pInfo, SSLContext * pSSLContext )
{
    HioStream::reset( DateTime::s_curTime );
    setfd( fd );
    m_pClientInfo = pInfo;
    setState( HIOS_CONNECTED );
    setHandler( NULL );
    
    assert(LSI_HKPT_L4_BEGINSESSION == 0);
    assert(LSI_HKPT_L4_ENDSESSION == 1);
    assert(LSI_HKPT_L4_RECVING == 2);
    assert(LSI_HKPT_L4_SENDING == 3);
    m_sessionHooks.inherit(pListener->getSessionHooks());
    m_pModuleConfig = pListener->getModuleConfig();
    if ( m_sessionHooks.isEnabled(LSI_HKPT_L4_BEGINSESSION) ) 
        m_sessionHooks.runCallbackNoParam(LSI_HKPT_L4_BEGINSESSION, this);
    
    memset( &m_iInProcess, 0, (char *)(&m_ssl + 1) - (char *)(&m_iInProcess) );
    m_iov.clear();
    ++HttpGlobals::s_iIdleConns;
    m_tmToken = HttpGlobals::s_tmToken;
    if ( HttpGlobals::getMultiplexer()->add( this, POLLIN|POLLHUP|POLLERR ) == -1 )
        return -1;
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
        setupHandler( HIOS_PROTO_HTTP );
        
    }
    pInfo->incConn();
    if ( D_ENABLED( DL_LESS ))
         LOG_D(( getLogger(), "[%s] concurrent conn: %d",
                 getLogId(), pInfo->getConns() ));
    return 0;
}

void NtwkIOLink::drainReadBuf()
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

void NtwkIOLink::tryRead()
{
    char ch;
    if ( ::recv( getfd(), &ch, 1, MSG_PEEK ) == 1 )
    {
        handleEvents( POLLIN );
    }
}

int NtwkIOLink::handleEvents( short evt )
{
    register int event = evt;
    if ( D_ENABLED( DL_MEDIUM ))
            LOG_D(( getLogger(), "[%s] NtwkIOLink::handleEvents() events=%d!",
                getLogId(), event ));
    if ( getState() == HIOS_SHUTDOWN )
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
                if ( getFlag( HIO_FLAG_ABORT | HIO_FLAG_PEER_SHUTDOWN )  )
                    HttpGlobals::getMultiplexer()->suspendRead( this );
                else
                    drainReadBuf();
            }
        }
        return 0;
    }
    m_iInProcess = 1;
    if ( event & POLLIN )
    {
        (*m_pFpList->m_onRead_fp )( this );
    }
    if ( event & (POLLHUP | POLLERR ))
    {
        setFlag( HIO_FLAG_PEER_SHUTDOWN, 1 );
        m_iInProcess = 0;
        close();
        return 0;
    }
    if ( event & POLLOUT )
    {
        (*m_pFpList->m_onWrite_fp)( this );
    }
    m_iInProcess = 0;
    if ( getState() >= HIOS_CLOSING )
    {
        close();
    }
    return 0;
}

int NtwkIOLink::close()
{
    if ( getHandler() )
    {
        if ( isReadyToRelease() )
        {
            getHandler()->recycle();
            setHandler( NULL );
        }
        else 
            getHandler()->onCloseEx();
    }
    return (*m_pFpList->m_close_fp)( this );
}

void NtwkIOLink::suspendRead()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] NtwkIOLink::suspendRead()...", getLogId() ));
    if ( !(( isSSL() )&&( m_ssl.wantRead())) )
        HttpGlobals::getMultiplexer()->suspendRead( this );
}

void NtwkIOLink::continueRead()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] NtwkIOLink::continueRead()...", getLogId() ));
    setFlag( HIO_FLAG_WANT_READ, 1 );
    if (( allowRead()))
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] read resumed!", getLogId() ));
        HttpGlobals::getMultiplexer()->continueRead( this );
    }
}

void NtwkIOLink::suspendWrite()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] NtwkIOLink::suspendWrite()...", getLogId() ));
    setFlag( HIO_FLAG_WANT_WRITE, 0 );
    if ( !(( isSSL() )&&( m_ssl.wantWrite())) && m_hasBufferedData == 0 )
    {
        HttpGlobals::getMultiplexer()->suspendWrite(this);
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] write suspended", getLogId() ));
    }
}

void NtwkIOLink::continueWrite()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] NtwkIOLink::continueWrite()...", getLogId() ));
    //if( getFlag( HIO_FLAG_WANT_WRITE ) )
    //    return;
    setFlag( HIO_FLAG_WANT_WRITE, 1 );
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

void NtwkIOLink::switchWriteToRead()
{
    setFlag( HIO_FLAG_WANT_READ, 1 );
    setFlag( HIO_FLAG_WANT_WRITE, 0 );
    HttpGlobals::getMultiplexer()->switchWriteToRead( this );
}


///////////////////////////////////////////////////////////////////////
// SSL
///////////////////////////////////////////////////////////////////////

void NtwkIOLink::updateSSLEvent()
{
    if ( isWantWrite() )
    {
        dumpState( "updateSSLEvent", "CW" );
        HttpGlobals::getMultiplexer()->continueWrite( this );
    }
    if ( isWantRead() )
    {
        dumpState( "updateSSLEvent", "CR" );
        HttpGlobals::getMultiplexer()->continueRead( this );
    }
}


void NtwkIOLink::checkSSLReadRet( int ret )
{
    if ( ret > 0 )
    {
        bytesRecv( ret );
        HttpGlobals::s_lSSLBytesRead += ret;
        setActiveTime( DateTime::s_curTime );
        //updateSSLEvent();
    }
    else if ( !ret )
    {
        if ( m_ssl.wantWrite() )
        {
            dumpState( "checkSSLReadRet", "CW" );
            HttpGlobals::getMultiplexer()->continueWrite( this );
        }
    }
    else if ( getState() != HIOS_SHUTDOWN )
        setState( HIOS_CLOSING );
}

int NtwkIOLink::readExSSL( LsiSession* pIS, char * pBuf, int size )
{
    NtwkIOLink *pThis = static_cast<NtwkIOLink *>( pIS );
    int ret;
    assert( pBuf );
    ret = pThis->getSSL()->read(pBuf, size);
    pThis->checkSSLReadRet( ret );
    //DEBUG CODE:
//    if ( ret > 0 )
//        ::write( 1, pBuf, ret );
    return ret;
}

int NtwkIOLink::writevExSSL( LsiSession* pOS, const iovec *vector, int count )
{
    NtwkIOLink * pThis = static_cast<NtwkIOLink *>(pOS);
    int ret = 0;

    const struct iovec * vect;
    const char * pBuf;
    int bufSize;
    int written;

    char * pBufEnd;
    char * pCurEnd;
    char achBuf[4096];
    pBufEnd = achBuf + 4096;
    pCurEnd = achBuf;
    for( int i=0; i < count ;  )
    {
        vect = &vector[i];
        pBuf =( const char *) vect->iov_base;
        bufSize = vect->iov_len;
        if ( bufSize < 1024 )
        {
            if ( pBufEnd - pCurEnd > bufSize )
            {
                memmove( pCurEnd, pBuf, bufSize );
                pCurEnd += bufSize;
                ++i;
                if ( i < count )
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
            ++i;
        written = pThis->getSSL()->write( pBuf, bufSize );
        if ( written > 0 )
        {
            pThis->bytesSent( written );
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
            if ( pThis->m_ssl.wantRead() )
                HttpGlobals::getMultiplexer()->continueRead( pThis );
            //pThis->setSSLAgain();
            break;
        }
        else if ( pThis->getState() != HIOS_SHUTDOWN )
        {
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( pThis->getLogger(), "[%s] SSL_write() failed: %s "
                        , pThis->getLogId(), SSLError().what() ));
            
            pThis->setState( HIOS_CLOSING );
            return -1;
        }
    }
    return ret;
}

void NtwkIOLink::setSSLAgain()
{
    if ( m_ssl.wantRead() || getFlag( HIO_FLAG_WANT_READ ) )
    {
        dumpState( "setSSLAgain", "CR" );
        HttpGlobals::getMultiplexer()->continueRead( this );
    }
    else
    {
        dumpState( "setSSLAgain", "SR" );
        HttpGlobals::getMultiplexer()->suspendRead( this );
    }
    
    if ( m_ssl.wantWrite() || getFlag( HIO_FLAG_WANT_WRITE ) )
    {
        dumpState( "setSSLAgain", "CW" );
        HttpGlobals::getMultiplexer()->continueWrite( this );
    }
    else
    {
        dumpState( "setSSLAgain", "SW" );
        HttpGlobals::getMultiplexer()->suspendWrite( this );
    }    
}

//return 1 if flush successful,
//return 0 if still have pending data
//return -1 if connection error. 
int NtwkIOLink::flush()
{
    int ret;
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] NtwkIOLink::flush...", getLogId() ));
                
    
//     int nodelay = 1;
//     ::setsockopt( getfd(), IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof( int ) );
    
    if ( m_hasBufferedData || (m_iHeaderToSend > 0 ) )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] NtwkIOLink::flush buffered data ...", getLogId() ));
        ret = writev_internal( m_iov.get(), m_iov.len(), LSI_CB_FLAG_IN_FLUSH );
        if ( m_iHeaderToSend > 0 )
        {
            if ( ret >= m_iHeaderToSend )
            {
                m_iov.clear();
                m_iHeaderToSend = 0;
            }
            else
            {
                if ( ret > 0 )
                {
                    m_iHeaderToSend -= ret;
                    m_iov.finish( ret );
                    ret = 0;
                }
                return ret;
            }
        }
        if ( m_hasBufferedData )
            return 0;
    }
    
    if ( !isSSL() )
        return 0;
    
    switch( ( ret = getSSL()->flush() ))
    {
    case 0:
        if ( m_ssl.wantRead() )
        {   
            dumpState( "flush", "CR" );
            HttpGlobals::getMultiplexer()->continueRead( this );
        }
        else
        {
            dumpState( "flush", "CW" );
            HttpGlobals::getMultiplexer()->continueRead( this );
        }
        return 1;
    case 1:
        return 0;
    case -1:
        setState( HIOS_CLOSING );
        break;
    }
    return ret;
}

int NtwkIOLink::onWriteSSL( NtwkIOLink * pThis )
{
    pThis->dumpState( "onWriteSSL", "none" );
    if ( pThis->m_ssl.wantWrite() )
    {
        if ( !pThis->m_ssl.isConnected()||( pThis->m_ssl.lastRead() ))
        {
            pThis->SSLAgain();
            return 0;
        }
    }
    return pThis->doWrite();
}


int NtwkIOLink::onReadSSL( NtwkIOLink * pThis )
{
    pThis->dumpState( "onReadSSL", "none" );
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


int NtwkIOLink::closeSSL( NtwkIOLink * pThis )
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


int NtwkIOLink::close_( NtwkIOLink * pThis )
{
    pThis->setState( HIOS_SHUTDOWN );

    if ( pThis->getFlag( HIO_FLAG_PEER_SHUTDOWN | HIO_FLAG_ABORT ) )
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

void NtwkIOLink::closeSocket()
{
    if ( getfd() == -1 )
        return;
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] Close socket ...", getLogId() ));
    
    if ( m_sessionHooks.isEnabled( LSI_HKPT_L4_ENDSESSION) ) 
        m_sessionHooks.runCallbackNoParam(LSI_HKPT_L4_ENDSESSION, this);
    
    HttpGlobals::getMultiplexer()->remove( this );
    if ( m_pFpList == s_pCur_fp_list_list->m_pSSL )
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
    if ( getHandler() )
    {
        getHandler()->recycle();
        setHandler( NULL );
    }
    //recycle itself.
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] Recycle NtwkIoLink", getLogId() ));
    HttpGlobals::getResManager()->recycle( this );
}

int NtwkIOLink::onRead( NtwkIOLink * pThis )
{
    if ( pThis->getHandler() )
        return pThis->getHandler()->onReadEx();
    return -1;
}

int NtwkIOLink::onWrite(NtwkIOLink *pThis )
{   return pThis->doWrite();   }


static int matchToken( int token )
{
    if ( HttpGlobals::s_tmPrevToken < HttpGlobals::s_tmToken )
        return (( token > HttpGlobals::s_tmPrevToken )&&( token <= HttpGlobals::s_tmToken ));
    else 
        return (( token > HttpGlobals::s_tmPrevToken )||( token <= HttpGlobals::s_tmToken ));
}

void NtwkIOLink::onTimer()
{
    if ( matchToken( this->m_tmToken ) )
    { 
        if ( this->hasBufferedData() && this->allowWrite() )
            this->flush();
    
        (*m_pFpList->m_onTimer_fp)( this );
    }
}

void NtwkIOLink::onTimer_( NtwkIOLink * pThis )
{
    if ( pThis->detectClose() )
        return;
    if (pThis->getHandler())
        pThis->getHandler()->onTimerEx();
}

int NtwkIOLink::checkReadRet( int ret, int size )
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
        bytesRecv( ret );
        HttpGlobals::s_lBytesRead += ret;
        setActiveTime( DateTime::s_curTime );
    }
    else if ( ret == 0 )
    {
        if ( getState() != HIOS_SHUTDOWN )
        {
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] End of stream detected, CLOSING!",
                    getLogId() ));
           //have the connection closed quickly
            setFlag( HIO_FLAG_PEER_SHUTDOWN, 1 );
            setState( HIOS_CLOSING );
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
            if ( getState() != HIOS_SHUTDOWN )
                setState( HIOS_CLOSING );
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] read error: %s\n",
                        getLogId(), strerror( errno ) ));
        }
    }
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] Read from client: %d\n", getLogId(), ret ));
    return ret;

}

int NtwkIOLink::readEx( LsiSession* pIS, char * pBuf, int size )
{
    NtwkIOLink *pThis = static_cast<NtwkIOLink *>( pIS );
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

int NtwkIOLink::sendfile( int fdSrc, off_t off, size_t size )
{
    int ret;
    const LsiApiHooks *pWritevHooks = m_sessionHooks.get( LSI_HKPT_L4_SENDING );
    if ( !pWritevHooks ||( pWritevHooks->size() == 0))
        return sendfileEx( fdSrc, off, size );
  
    char buf[8192];
    off_t curOff = off;
    while( size > 0 )
    {
        int blockSize = sizeof( buf );
        if (  blockSize > (int)size )
            blockSize = size;
        ret = ::pread( fdSrc, buf, blockSize, curOff );
        if ( ret <= 0 )
            break;
        IOVec iovTmp( buf, ret );
        ret = writev( iovTmp.get(), iovTmp.len() );
        if ( ret > 0 )
        {
            size -= ret;
            curOff += ret;
        }
        if ( ret < blockSize )
            break;
    }
    return curOff - off; 
}

int NtwkIOLink::sendfileEx( int fdSrc, off_t off, size_t size )
{
    if ( m_iHeaderToSend > 0 )
    {    
        writev(NULL, 0);
        if ( m_iHeaderToSend > 0 )
            return 0;
    }
    int len = 0;
    int written;
    ThrottleControl * pCtrl = getThrottleCtrl();
    
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
    len = checkWriteRet( written );
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


#endif 


int NtwkIOLink::writevEx( LsiSession* pOS, const iovec *vector, int count )
{
    NtwkIOLink * pThis = static_cast<NtwkIOLink *>(pOS);
    int len = ::writev( pThis->getfd(), vector, count );
    len = pThis->checkWriteRet( len );
    //if (pThis->wantWrite() && pThis->m_hasBufferedData)
    //    HttpGlobals::getMultiplexer()->continueWrite( pThis );
    
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


int NtwkIOLink::sendRespHeaders( HttpRespHeaders * pHeader )
{
    if ( pHeader )
    {
        pHeader->outputNonSpdyHeaders(&m_iov);
        m_iHeaderToSend = pHeader->getTotalLen();
    }
    return 0;
}


int NtwkIOLink::checkWriteRet( int len )
{
    if ( len > 0 )
    {
        bytesSent( len );
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
            if ( getState() != HIOS_SHUTDOWN )
            {
                if ( m_hasBufferedData == 0 )
                {
                    setState( HIOS_CLOSING );
                    setFlag( HIO_FLAG_ABORT, 1 );
                }
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

int NtwkIOLink::detectClose()
{
    if ( getState() == HIOS_SHUTDOWN )
    {
        if ( D_ENABLED( DL_MEDIUM ) )
            LOG_D((getLogger(), "[%s] Shutdown time out!", getLogId() ));
        closeSocket();
    }        
    else if ( getState() == HIOS_CONNECTED ) 
    {
        char ch;
        if ( ( getClientInfo()->getAccess() == AC_BLOCK ) ||
             (( DateTime::s_curTime - getActiveTime() > 10 )&&
             (::recv( getfd(), &ch, 1, MSG_PEEK ) == 0 ) ))
        {
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] peer connection close detected!\n", getLogId() ));
            //have the connection closed faster
            setFlag( HIO_FLAG_PEER_SHUTDOWN, 1 );
            close();
            return 1;
        }
    }
    return 0;
}

int NtwkIOLink::detectCloseNow()
{
    char ch;
    if ( ::recv( getfd(), &ch, 1, MSG_PEEK ) == 0 )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] peer connection close detected!\n", getLogId() ));
        //have the connection closed faster
        setFlag( HIO_FLAG_PEER_SHUTDOWN, 1 );
        close();
        return 1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////
// Throttle
///////////////////////////////////////////////////////////////////////

int NtwkIOLink::onReadT( NtwkIOLink * pThis )
{
    return pThis->doReadT();
}

int NtwkIOLink::onWriteT( NtwkIOLink *pThis )
{
    if ( pThis->allowWrite() )
        return pThis->doWrite();
    else
        HttpGlobals::getMultiplexer()->suspendWrite( pThis );
    return 0;
}

void NtwkIOLink::dumpState(const char * pFuncName, const char * action)
{
    if ( D_ENABLED( DL_MORE ))
        LOG_D(( getLogger(),  "[%s] %s(), %s, wantRead: %d, wantWrite: %d,"
                        " allowWrite: %d, allowRead: %d,"
                        " m_ssl.wantRead: %d, m_ssl.wantWrite: %d, "
                        " m_ssl.lastRead: %d, m_ssl.lastWrite: %d",
                getLogId(),  pFuncName, action, isWantRead(), isWantWrite(),
                allowWrite(), allowRead(), 
                m_ssl.wantRead(), m_ssl.wantWrite(),
                m_ssl.lastRead(), m_ssl.lastWrite()
              ));
    
}

void NtwkIOLink::onTimer_T( NtwkIOLink * pThis )
{
//    if ( D_ENABLED( DL_MORE ))
//        LOG_D(( pThis->getLogger(),  "[%s] conn token:%d, global Token: %d\n",
//                    pThis->getLogId(), pThis->m_tmToken, HttpGlobals::s_tmToken ));
    if ( pThis->detectClose() )
        return;
//        if ( D_ENABLED( DL_MORE ))
//            LOG_D(( pThis->getLogger(),  "[%s] output avail:%d. state: %d \n",
//                    pThis->getLogId(),
//                    pThis->getClientInfo()->getThrottleCtrl().getOSQuota(),
//                    pThis->getState() ));
        
    if ( pThis->hasBufferedData() && pThis->allowWrite() )
        pThis->flush();
    
    if ( pThis->allowWrite() && pThis->isWantWrite() )
    {
        //pThis->doWrite();
        //if (  pThis->allowWrite() && pThis->wantWrite() )
        pThis->dumpState( "onTimer_T", "CW" );
        HttpGlobals::getMultiplexer()->continueWrite( pThis );
    }
    if ( pThis->allowRead() && pThis->isWantRead() )
    {
        //if ( pThis->getState() != HSS_WAITING )
        //    pThis->doReadT();
        //if ( pThis->allowRead() && pThis->wantRead() )
        pThis->dumpState( "onTimer_T", "CR" );
            HttpGlobals::getMultiplexer()->continueRead( pThis );
    }
    if ( pThis->getHandler() )
        pThis->getHandler()->onTimerEx();

}


int NtwkIOLink::readExT( LsiSession* pIS, char * pBuf, int size )
{
    NtwkIOLink *pThis = static_cast<NtwkIOLink *>( pIS );
    ThrottleControl * pTC = pThis->getThrottleCtrl();
    int iQuota = pTC->getISQuota();
    if ( iQuota <= 0 )
    {
        pThis->dumpState( "readExT", "SR" );
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
            pThis->dumpState( "readExT", "SR" );
            HttpGlobals::getMultiplexer()->suspendRead( pThis );
        }
    }
    return ret;
}



int NtwkIOLink::writevExT( LsiSession * pOS, const iovec *vector, int count )
{
    NtwkIOLink * pThis = static_cast<NtwkIOLink *>(pOS);
    int len = 0;
    ThrottleControl * pCtrl = pThis->getThrottleCtrl();
    int Quota = pCtrl->getOSQuota();
    if ( Quota <= 0 )
    {
        pThis->dumpState( "writevExT", "SW" );
        HttpGlobals::getMultiplexer()->suspendWrite( pThis );
        return 0;
    }
    
    int total = 0;
    for (int i=0; i<count; ++i)
        total += vector[i].iov_len;
    
//    if ( D_ENABLED( DL_LESS ))
//        LOG_D(( pThis->getLogger(),  "[%s] Quota:%d, to write: %d\n",
//                    pThis->getLogId(), Quota, total ));
    if ( (unsigned int)total > (unsigned int )Quota + ( Quota >> 3) )
    {
        IOVec iov;
        iov.append(vector, count);
        total = iov.shrinkTo( Quota, Quota >> 3 );
        len = ::writev( pThis->getfd(), iov.begin(), iov.len() );
    }
    else
        len = ::writev( pThis->getfd(), vector, count );
    
    len = pThis->checkWriteRet( len );
    if (Quota - len < 10 )
    {
        pCtrl->useOSQuota(  Quota );
        pThis->dumpState( "writevExT", "SW" );
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


//void NtwkIOLink::onTimerSSL_T( NtwkIOLink * pThis )
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

void NtwkIOLink::onTimerSSL_T( NtwkIOLink * pThis )
{
    if ( pThis->detectClose() )
        return;
    if ( pThis->allowWrite() && ( pThis->m_ssl.wantWrite() ))
        onWriteSSL_T( pThis );
    if ( pThis->allowRead() && ( pThis->m_ssl.wantRead() ))
        onReadSSL_T( pThis );
    if ( pThis->allowWrite() && pThis->isWantWrite() )
    {
        pThis->doWrite();
        if (  pThis->allowWrite() && pThis->isWantWrite() )
        {
            pThis->dumpState( "onTimerSSL_T", "CW" );
            HttpGlobals::getMultiplexer()->continueWrite( pThis );
        }
    }
    if ( pThis->allowRead() && pThis->isWantRead() )
    {
        //if ( pThis->getState() != HSS_WAITING )
        //    pThis->doReadT();
        //if ( pThis->allowRead() && pThis->wantRead() )
            pThis->dumpState( "onTimerSSL_T", "CR" );
            HttpGlobals::getMultiplexer()->continueRead( pThis );
    }
    if ( pThis->getHandler() )
        pThis->getHandler()->onTimerEx();
}


int NtwkIOLink::onReadSSL_T( NtwkIOLink * pThis )
{
    //pThis->dumpState( "onReadSSL_t", "none" );
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

int NtwkIOLink::onWriteSSL_T( NtwkIOLink * pThis )
{
    //pThis->dumpState( "onWriteSSL_T", "none" );
    if ( pThis->m_ssl.wantWrite() )
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
    "HTTP/1.0 200 OK\r\n"
    "Cache-Control: private, no-cache, max-age=0\r\n"
    "Pragma: no-cache\r\n"
    "Connection: Close\r\n\r\n"
    "<html><head><title>400 Bad Request</title></head><body>\n"
    "<h2>HTTPS is required</h2>\n"
    "<p>This is an SSL protected page, please use the HTTPS scheme instead of "
    "the plain HTTP scheme to access this URL.<br />\n"
    "<blockquote>Hint: The URL should starts with <b>https</b>://</blockquote> </p>\n"
    "<hr />\n"
    "Powered By LiteSpeed Web Server<br />\n"
    "<a href='http://www.litespeedtech.com'><i>http://www.litespeedtech.com</i></a>\n"
    "</body></html>\n";
int NtwkIOLink::acceptSSL()
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
            LOG_WARN(( getLogger(), "[%s] [SSL] too many new SSL connections: %d, "
                        "possible SSL negociation based attack, block!", getLogId(),
                        m_pClientInfo->getSslNewConn() ));
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

int NtwkIOLink::sslSetupHandler()
{
    unsigned int spdyVer = m_ssl.getSpdyVersion();
    if ( spdyVer >= HIOS_PROTO_MAX )
    {
        LOG_ERR(( getLogger(), "[%s] bad SPDY version: %d, use HTTP", getLogId(), spdyVer ));
        spdyVer = HIOS_PROTO_HTTP;
    }
    else
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] Next Protocol Negociation result: %s\n", getLogId(), getProtocolName( (HiosProtocol)spdyVer ) ));
    }
    return setupHandler( (HiosProtocol)spdyVer );
}

int NtwkIOLink::SSLAgain()
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
        if ( ret == 1 )
        {
            sslSetupHandler();
        }
        break;
    case SSLConnection::SHUTDOWN:
        ret = m_ssl.shutdown( 1 );
        break;
    case SSLConnection::CONNECTED:
        if ( m_ssl.lastRead() )
        {
            if ( getHandler() )
                return getHandler()->onReadEx();
            else
                return -1;
        }
        if ( m_ssl.lastWrite() )
            return doWrite();
    }
    switch( ret )
    {
    case 0:
        setSSLAgain();
        break;
    case -1:
        close();
        break;
    }
    return ret;

}

int NtwkIOLink::readExSSL_T( LsiSession* pIS, char * pBuf, int size )
{
    NtwkIOLink *pThis = static_cast<NtwkIOLink *>( pIS );
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
    int ret = pThis->getSSL()->read(pBuf, size);
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


int NtwkIOLink::writevExSSL_T( LsiSession * pOS, const iovec *vector, int count )
{
    NtwkIOLink * pThis = static_cast<NtwkIOLink *>(pOS);
    ThrottleControl * pCtrl = pThis->getThrottleCtrl();
    int Quota = pCtrl->getOSQuota();
    if ( Quota <= pThis->m_iSslLastWrite / 2 )
    {
        HttpGlobals::getMultiplexer()->suspendWrite( pThis );
        return 0;
    }
    unsigned int allowed = (unsigned int)Quota + ( Quota >> 3);
    int ret = 0;
    const struct iovec * vect = vector;
    const struct iovec * pEnd = vector + count;
    
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
    for( ; ret < Quota && vect < pEnd;  )
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
                if (( vect < pEnd )&&( ret + ( pCurEnd - achBuf ) < Quota ))
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
        int written = pThis->getSSL()->write( pBuf, bufSize );
        if ( D_ENABLED( DL_MORE ))
            LOG_D(( "[%s] to write %d bytes, written %d bytes",
                pThis->getLogId(), bufSize, written ));
        if ( written > 0 )
        {
            pThis->bytesSent( written );
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
        else if ( pThis->getState() != HIOS_SHUTDOWN )
        {
            if ( D_ENABLED( DL_MORE ))
                LOG_D(( "[%s] SSL error: %s, mark connection to be closed",
                    pThis->getLogId(), SSLError().what() ));
            pThis->setState( HIOS_CLOSING );
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

void NtwkIOLink::suspendEventNotify()
{
    if ( !HttpGlobals::s_iMultiplexerType )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] remove fd:%d from multiplexer!\n", getLogId(), getfd() ));
        HttpGlobals::getMultiplexer()->remove( this );
    }
}

void NtwkIOLink::resumeEventNotify()
{
    if ( !HttpGlobals::s_iMultiplexerType )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] add fd:%d back to multiplexer!\n", getLogId(), getfd() ));
        HttpGlobals::getMultiplexer()->add( this, POLLHUP|POLLERR );
    }
}

void NtwkIOLink::changeClientInfo( ClientInfo * pInfo )
{
    if ( pInfo == m_pClientInfo )
        return;
    m_pClientInfo->decConn();
    pInfo->incConn();
    m_pClientInfo = pInfo;
}

static const char * s_pProtoString[] = { "", ":SPDY2", ":SPDY3"   };
const char * NtwkIOLink::buildLogId()
{
    AutoStr2 & id = getIdBuf();

    int len ;
    char * p = id.buf();
    len = safe_snprintf( id.buf(), MAX_LOGID_LEN, "%s:%hu%s",  
                            m_pClientInfo->getAddrString(), getRemotePort(), s_pProtoString[ (int)getProtocol() ] );
    id.setLen( len );
    p += len;

    return id.c_str();
}


