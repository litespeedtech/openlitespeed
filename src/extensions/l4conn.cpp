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

#include "l4conn.h"
#include "http/l4handler.h"
#include <socket/gsockaddr.h>
#include <socket/coresocket.h>
#include <util/datetime.h>

#include <http/httpglobals.h>
#include <socket/gsockaddr.h>

#include <fcntl.h>
#include <util/ssnprintf.h>
#include <util/loopbuf.h>

int L4conn::onEventDone()
{
    switch( m_iState )
    {
    case CLOSING:
        m_pL4Handler->closeBothConnection();
        break;
    }
    return 0;
}

int L4conn::onError()
{
    int error = errno;
    int ret = getSockError( &error );
    if (( ret ==-1 )||( error != 0 ))
    {
        if ( ret != -1 )
            errno = error;
    }
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(), "[%s] L4conn::onError()", getLogId() ));
    if ( error != 0 )
    {
        m_iState = CLOSING;
        //doError( error );
    }
    else
        onRead();
    return -1;
}

int L4conn::onWrite()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(), "[%s] L4conn::onWrite()", getLogId() ));
    
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
    case CLOSING:
    case DISCONNECTED:
        return 0;
    default:
        return 0;
    }
    if ( ret == -1 )
    {
        m_pL4Handler->closeBothConnection();
    }
    return ret;
}

int L4conn::onInitConnected()
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

//         LOG_D(( getLogger(), "[%s] connected to [%s] on local addres [%s:%d]!", getLogId(),
//                 m_pWorker->getURL(), achAddr, port ));
//    }
    return 0;
}


int L4conn::onRead()
{
    if ( D_ENABLED( DL_LESS ) )
         LOG_D(( getLogger(), "[%s] L4conn::onRead() state: %d", getLogId(), m_iState ));

    int ret;
    switch( m_iState )
    {
    case CONNECTING:
        ret = onInitConnected();
        break;
    case PROCESSING:
        ret = doRead();
        break;
    case CLOSING:
    case DISCONNECTED:
        return 0;
    default:
        // Not suppose to happen;
        return 0;
    }
    if ( ret == -1 )
    {
        m_pL4Handler->closeBothConnection();
    }
    return ret;
}

int L4conn::close()
{
    if (m_iState != DISCONNECTED)
    {
        m_iState = DISCONNECTED;
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( getLogger(), "[%s] [ExtConn] close()", getLogId() ));
        EdStream::close();
        delete m_buf;
    }
    
    return 0;
}

L4conn::L4conn(L4Handler*  pL4Handler) : m_iState( 0 )
{    
    m_pL4Handler = pL4Handler;
    m_buf = new LoopBuf(MAX_OUTGOING_BUF_ZISE);
}

L4conn::~L4conn()
{
    if (m_iState != DISCONNECTED)
        close();
}

int L4conn::init(const GSockAddr *pGSockAddr)
{
    int ret = connectEx(pGSockAddr);
    
    if ( D_ENABLED( DL_LESS ) )
        LOG_D (( getLogger(), "[%s] [L4conn] init ret = [%d]...", getLogId(), ret ));
    
    return ret;
}

int L4conn::connectEx(const GSockAddr *pGSockAddr)
{
    int fd;
    int ret;
    Multiplexer *pMpl =  HttpGlobals::getMultiplexer();
    ret = CoreSocket::connect( *pGSockAddr, pMpl->getFLTag(), &fd, 0 );
    if (( fd == -1 )&&( errno == ECONNREFUSED ))
    {
        ret = CoreSocket::connect( *pGSockAddr, pMpl->getFLTag(), &fd, 0 );
    }
    
    if ( fd != -1 )
    {
       if ( D_ENABLED( DL_LESS ) )
            LOG_D(( getLogger(), "[%s] [L4conn] connecting to [%s]...",
                    getLogId(), pGSockAddr->toString() ));

        ::fcntl( fd, F_SETFD, FD_CLOEXEC );
        EdStream::init ( fd, pMpl, POLLIN|POLLOUT|POLLHUP|POLLERR );
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

int L4conn::doRead()
{
    bool empty = m_pL4Handler->getBuf()->empty();
    int space;
    
    if ( (space = m_pL4Handler->getBuf()->contiguous()) > 0)
    {
        int n = read(m_pL4Handler->getBuf()->end(), space);
        if ( D_ENABLED( DL_LESS ) )
        {
            LOG_D(( getLogger(), "[%s] [L4conn] doRead [%d]...",
                getLogId(), n ));
        }
        
        if (n > 0) 
        {
            m_pL4Handler->getBuf()->used(n);
        }
        else if (n < 0)
        {
            m_pL4Handler->closeBothConnection();
            return -1;
        }
    }
    
    if ( !m_pL4Handler->getBuf()->empty() )
    {
        m_pL4Handler->doWrite();
        if ( !m_pL4Handler->getBuf()->empty() && empty)
            m_pL4Handler->continueWrite();
        
        if (m_pL4Handler->getBuf()->available() <= 0 )
        {
            suspendRead();
            if ( D_ENABLED( DL_LESS ) )
            {
                LOG_D(( getLogger(), "[%s] [L4conn] suspendRead",
                    getLogId() ));
            }
        }
    }
    return 0;
}
    
int L4conn::doWrite()
{
    bool full = ((getBuf()->available() == 0) ? true : false);
    int length;
    
    while ((length = getBuf()->blockSize()) > 0 )
    {
        int n = write(getBuf()->begin(), length);
        if ( D_ENABLED( DL_LESS ) )
        {
            LOG_D(( getLogger(), "[%s] [L4conn] doWrite [%d of %d]...",
                getLogId(), n, length ));
        }
            
        if (n > 0)
        {
            getBuf()->pop_front(n);
        }
        else if ( n == 0 )
            break;
        else // if (n < 0)
        {
            m_pL4Handler->closeBothConnection();
            return -1;            
        }
        
    }
    
    if (getBuf()->available() != 0)
    {
        if (full)
            m_pL4Handler->continueRead();

        if ( getBuf()->empty() )
        {
            suspendWrite();
            if ( D_ENABLED( DL_LESS ) )
            {
                LOG_D(( getLogger(), "[%s] [L4conn] suspendWrite && m_pL4Handler->continueRead",
                    getLogId() ));
            }
        }
    }
    
    return 0;
}

LOG4CXX_NS::Logger * L4conn::getLogger() const
{
    return m_pL4Handler->getLogger();
}

const char * L4conn::getLogId()
{
    return m_pL4Handler->getLogId();
}
