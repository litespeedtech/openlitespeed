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
#include "extconn.h"
#include "extrequest.h"
#include "extworker.h"
#include <edio/multiplexer.h>
#include <socket/gsockaddr.h>
#include <socket/coresocket.h>
#include <http/datetime.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <http/httpstatuscode.h>

#include <fcntl.h>

ExtConn::ExtConn()
    : m_iState( 0 )
    , m_iToStop( 0 )
    , m_iInProcess( 0 )
    , m_iCPState( 0 )
    , m_tmLastAccess( 0 )
    , m_iReqProcessed( 0 )
    , m_pWorker( NULL )
{
}

ExtConn::~ExtConn()
{
}

//void ExtConn::reset()
//{
//    close();
//    m_iState = DISCONNECTED;
//
//}

void ExtConn::recycle()
{
    if ( getState() >= ABORT )
        close();
    m_pWorker->recycleConn( this );
}

void ExtConn::checkInProcess()
{
    assert( !m_iInProcess );
}


int ExtConn::assignReq( ExtRequest * pReq )
{
    int ret;
    
    if ( getState() > PROCESSING )
        close();
    assert( !m_iInProcess );
//    if ( m_iInProcess )
//    {
//        LOG_WARN(( "[%s] [ExtConn] connection is still in middle of a request, close before "
//                   "assign a new request" ));
//        close();
//    }
    ret = addRequest( pReq );
    if ( ret )
    {
        if ( ret == -1 )
            ret = SC_500;
        pReq->setHttpError( ret );
        return ret;
    }
    m_tmLastAccess = DateTime::s_curTime;
    if ( getState() == PROCESSING )
    {
        ret = doWrite();
        onEventDone();
        //pConn->continueWrite();
    }
    else if ( getState() != CONNECTING  )
        ret = reconnect();
    if ( ret == -1 )
    {
        return connError( errno );
    }
    return 0;

}

int ExtConn::close()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(), "[%s] [ExtConn] close()", getLogId() ));
    EdStream::close();
    m_iState = DISCONNECTED;
    m_iInProcess = 0;
    return 0;
}

int ExtConn::reconnect()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(), "[%s] [ExtConn] reconnect()", getLogId() ));
    if ( m_iState != DISCONNECTED )
        close();
    if ( m_pWorker )
        return connect( HttpGlobals::getMultiplexer() );
    else
        return -1;
}


int ExtConn::connect( Multiplexer * pMplx )
{
    m_pWorker->startOnDemond(0);
    return connectEx( pMplx );
}

int ExtConn::connectEx( Multiplexer * pMplx )
{
    int fd;
    int ret;
    ret = CoreSocket::connect( m_pWorker->getServerAddr(), pMplx->getFLTag(), &fd, 1 );
    m_iReqProcessed = 0;
    m_iToStop = 0;
    if (( fd == -1 )&&( errno == ECONNREFUSED ))
    {
        ret = CoreSocket::connect( m_pWorker->getServerAddr(), pMplx->getFLTag(), &fd, 1 );
    }
    if ( fd != -1 )
    {
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( getLogger(), "[%s] [ExtConn] connecting to [%s]...",
                    getLogId(), m_pWorker->getURL() ));
        m_tmLastAccess = DateTime::s_curTime;
        ::fcntl( fd, F_SETFD, FD_CLOEXEC );
        init( fd, pMplx );
        if ( ret == 0 )
        {
            m_iState = PROCESSING;
            onWrite();
        }
        else
            m_iState = CONNECTING;
        return 0;
    }
    return -1;
}


int ExtConn::onInitConnected()
{
    int error;
    int ret = getSockError( &error );
    if (( ret == -1 )||( error != 0 ))
    {
        if ( ret != -1 )
            errno = error;
        return -1;
    }
    m_iState = PROCESSING;
    if ( D_ENABLED( DL_LESS ) )
    {
        char        achSockAddr[128];
        char        achAddr[128]    = "";
        int         port            = 0;
        socklen_t   len             = 128;

        if ( getsockname( getfd(), (struct sockaddr *)achSockAddr, &len ) == 0 )
        {
            GSockAddr::ntop( (struct sockaddr *)achSockAddr, achAddr, 128 );
            port = GSockAddr::getPort( (struct sockaddr *)achSockAddr );
        }

        LOG_D(( getLogger(), "[%s] connected to [%s] on local addres [%s:%d]!", getLogId(),
                m_pWorker->getURL(), achAddr, port ));
    }
    return 0;
}


int ExtConn::onRead()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(), "[%s] ExtConn::onRead()", getLogId() ));
    m_tmLastAccess = DateTime::s_curTime;
    int ret;
    switch( m_iState )
    {
    case CONNECTING:
        ret = onInitConnected();
        break;
    case PROCESSING:
        ret = doRead();
        break;
    case ABORT:
    case CLOSING:
    case DISCONNECTED:
        return 0;
    default:
        // Not suppose to happen;
        return 0;
    }
    if ( ret == -1 )
    {
        ret = connError( errno );
    }
    return ret;
}


int ExtConn::onWrite()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(), "[%s] ExtConn::onWrite()",
                getLogId() ));
    m_tmLastAccess = DateTime::s_curTime;
    int ret;
    switch( m_iState )
    {
    case CONNECTING:
        ret = onInitConnected();
        if ( ret )
            break;
        //fall through
    case PROCESSING:
        ret = doWrite();
        break;
    case ABORT:
    case CLOSING:
    case DISCONNECTED:
        return 0;
    default:
        return 0;
    }
    if ( ret == -1 )
    {
        ret = connError( errno );
    }
    return ret;
}

int ExtConn::onError()
{
    int error = errno;
    int ret = getSockError( &error );
    if (( ret ==-1 )||( error != 0 ))
    {
        if ( ret != -1 )
            errno = error;
    }
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(), "[%s] ExtConn::onError()", getLogId() ));
    if ( error != 0 )
    {
        m_iState = CLOSING;
        doError( error );
    }
    else
        onRead();
    return -1;
}

void ExtConn::onTimer()
{
}

void ExtConn::onSecTimer()
{
    int secs = DateTime::s_curTime - m_tmLastAccess;
    if ( m_iState == CONNECTING )
    {
        if (  secs >= 2 )
        {
            LOG_NOTICE(( getLogger(), "[%s] ExtConn timed out while connecting.", getLogId() ));
            connError( ETIMEDOUT );
        }
    }
    else if ( m_iState == DISCONNECTED )
    {
    }
    else if ( getReq() )
    {
        if (  m_iReqProcessed == 0 )
        {
            if (secs >= m_pWorker->getTimeout() )
            {
                LOG_NOTICE(( getLogger(), "[%s] ExtConn timed out while processing.",
                        getLogId() ));
                connError( ETIMEDOUT );
            }
            else if ((secs == 10 )&&(getReq()->isRecoverable()))
            {
                if ( D_ENABLED( DL_LESS ) )
                    LOG_D(( getLogger(), "[%s] No response in 10 seconds, possible dead lock, "
                            "try starting a new instance.", getLogId() ));
                m_pWorker->addNewProcess();
            }
        }
    }
    else if (( m_iState == PROCESSING )&&( secs > m_pWorker->getConfigPointer()->getKeepAliveTimeout() ))
    {
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( getLogger(), "[%s] Idle connection timed out, close!",
                    getLogId() ));
        close();
    }
    
}

int ExtConn::connError( int errCode )
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(), "[%s] connection to [%s] on request #%d, error: %s!",
                getLogId(), m_pWorker->getURL(), m_iReqProcessed,
                strerror( errCode ) ));
	if ( errCode == EINTR )
		return 0;
    close();
    ExtRequest * pReq = getReq();
    if ( pReq )
    {
        if (((m_pWorker->getConnPool().getFreeConns() == 0)||( (pReq->getAttempts() % 3) == 0 ))&&
            (( errCode == EPIPE )||( errCode == ECONNRESET))&&
             (pReq->isRecoverable())&&( m_iReqProcessed ))
        {
            pReq->incAttempts();
            pReq->resetConnector();
            if ( reconnect() == 0 )
                return 0;
            close();
        }
    }
    return m_pWorker->connectionError( this, errCode );
//    if ( !m_pWorker->connectionError( this, errCode ) )
//    {
//        //if (( errCode != ENOMEM )&&(errCode != EMFILE )
//        //        &&( errCode != ENFILE ))
//    }
}

int ExtConn::onEventDone()
{
    switch( m_iState )
    {
    case ABORT:
    case CLOSING:
        close();
        if ( !getReq() )
        {
            m_pWorker->getConnPool().removeConn( this );
        }
        else
            getReq()->endResponse( 0, 0 );
        //reconnect();
        break;
    }
    return 0;
}


#ifndef _NDEBUG
void ExtConn::continueRead()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(), "[%s] ExtConn::continueRead()", getLogId() ));
    EdStream::continueRead();
}

void ExtConn::suspendRead()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(), "[%s] ExtConn::suspendRead()", getLogId() ));
    EdStream::suspendRead();
}

void ExtConn::continueWrite()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(), "[%s] ExtConn::continueWrite()", getLogId() ));
/*    if ( getRevents() & POLLOUT )
    {
        onWrite();
    }
    else*/
        EdStream::continueWrite();
}

void ExtConn::suspendWrite()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(), "[%s] ExtConn::suspendWrite()", getLogId() ));
    EdStream::suspendWrite();
}
#endif

