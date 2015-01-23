/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#include "ediostream.h"
#include <util/iovec.h>
#include <util/loopbuf.h>

#include <http/httplog.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

EdStream::~EdStream()
{
    close();
}

int EdStream::regist( Multiplexer * pMultiplexer, int events )
{
    if ( pMultiplexer )
        return pMultiplexer->add( this, events );
    return 0;
}

int EdStream::close()
{
    if ( getfd() != -1 )
    {
        if ( m_pMplex )
            m_pMplex->remove( this );
        //::shutdown( getfd(), SHUT_RDWR );
        ::close( getfd() );
        setfd( -1 );
    }
    return 0;
}


//void EdStream::updateEvents()
//{
//    int iEvents = getEvents();
//    int newEvent = iEvents & ~(POLLIN|POLLOUT);
//    if ( wantRead() )
//        newEvent |= POLLIN;
//    if ( wantWrite() )
//        newEvent |= POLLOUT;
//    if ( newEvent != iEvents )
//    {
//        setMask( newEvent );
//    }
//}

int EdStream::handleEvents( short event )
{
    int ret = 0;
    if ( D_ENABLED( DL_LESS ) )
         LOG_D(( "EdStream::handleEvent(), fd: %d, event: %hd", getfd(), event ));
    if ( event & POLLIN )
    {
        ret = onRead();
        if ( !getAssignedRevent() )
            return 0;
    }
    if ( event & POLLHUP )
    {
        if (( ret != -1 )||( getHupCounter() > 50 ))
            ret = onHangup();
        else if ( getHupCounter() > 100 )
            abort();
        if ( !getAssignedRevent() )
            return 0;
        
    }
    if((ret != -1 )&&(event & POLLHUP ))
    {
        ret = onHangup();
        if ( !getAssignedRevent() )
            return 0;
    }
    if ((ret != -1 )&&(event & POLLOUT ))
    {
        ret = onWrite();
        if ( !getAssignedRevent() )
            return 0;
    }
    if ((ret != -1 )&&( event & POLLERR ))
    {
        ret = onError();
        if ( !getAssignedRevent() )
            return 0;
    }
    if ( ret != -1 )
        onEventDone();
    return 0;
}

int EdStream::read( char * pBuf, int size )
{
    int ret = ::read( getfd(), pBuf, size );
    if ( ret < size )
        resetRevent( POLLIN );
    if ( !ret )
    {
        errno = ECONNRESET;
        return -1;
    }
    if (( ret == -1 )&&((errno == EAGAIN )||(errno == EINTR )))
        return 0;
    return ret;
}

int EdStream::readv( struct iovec *vector, size_t count)
{
    int ret = ::readv( getfd(), vector, count );
    if ( !ret )
    {
        errno = ECONNRESET;
        return -1;
    }
    if ( ret == -1 )
    {
        resetRevent( POLLIN );
        if ((errno == EAGAIN)||(errno == EINTR ))
            return 0;
    }
    return ret;
    
}


int EdStream::onHangup()
{
    //::shutdown( getfd(), SHUT_RD );
    return 0;
}

/** No descriptions */
int EdStream::getSockError(int32_t *error)
{
    socklen_t len = sizeof( int32_t );
    return getsockopt( getfd(), SOL_SOCKET, SO_ERROR, error, &len );
}

int EdStream::write( LoopBuf* pBuf )
{
    if ( pBuf == NULL )
    {
        errno = EFAULT;
        return -1;
    }
    IOVec iov;
    pBuf->getIOvec( iov );
    int ret = writev( iov );
    if ( ret > 0 )
        pBuf->pop_front( ret );
    return ret;
}


