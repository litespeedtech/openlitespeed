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
#if defined(__FreeBSD__ ) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)

#include "kqueuer.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/event.h>
#include <unistd.h>



KQueuer::KQueuer()
    : m_fdKQ( -1 )
    , m_changeCapacity( 0 )
    , m_curChange( 0 )
    , m_pChanges( NULL )
    //, m_traceCounter( 0 )
{
}
KQueuer::~KQueuer()
{
    if ( m_fdKQ != -1 )
        close( m_fdKQ );
    deallocateChangeBuf();
}

int KQueuer::allocateChangeBuf( int capacity )
{
    struct kevent * pEvents= ( struct kevent *) realloc( m_pChanges,
                        capacity * sizeof(struct kevent));
    if (!pEvents)
        return -1;
    if ( capacity > m_changeCapacity )
        memset( pEvents + m_changeCapacity, 0,
                sizeof( struct kevent ) * (capacity - m_changeCapacity ) );
    m_pChanges = pEvents;
    m_changeCapacity = capacity;
    return 0;
}

int KQueuer::deallocateChangeBuf()
{
    if ( m_pChanges )
        free( m_pChanges );
    return 0;
}



int KQueuer::init( int capacity )
{
    if ( m_reactorIndex.allocate( capacity ) == -1 )
        return -1;
    if ( allocateChangeBuf( 64 ) == -1 )
        return -1;
    m_fdKQ = kqueue();
    if ( m_fdKQ == -1)
        return -1;
    ::fcntl( m_fdKQ, F_SETFD, FD_CLOEXEC );
    return 0;
}


int KQueuer::appendEvent( EventReactor* pHandler, short filter, unsigned short flags )
{
    return appendEvent( pHandler, pHandler->getfd(), filter, flags );
}



int KQueuer::addEvent( EventReactor* pHandler, short mask )
{
    if ( mask & POLLIN )
    {
        appendEvent( pHandler, EVFILT_READ,  EV_ADD | EV_ENABLE );
    }
    if ( mask & POLLOUT )
    {
        appendEvent( pHandler, EVFILT_WRITE, EV_ADD | EV_ENABLE );
    }
    pHandler->setMask2( mask );
    return 0;
}

int KQueuer::add( EventReactor* pHandler, short mask )
{
    if ( !pHandler )
        return -1;
    pHandler->setPollfd();
    m_reactorIndex.set( pHandler->getfd(), pHandler );
    return addEvent( pHandler, mask );
}

int KQueuer::remove( EventReactor* pHandler )
{
    //appendEvent( pHandler, EVFILT_READ,  EV_DELETE );
    //appendEvent( pHandler, EVFILT_WRITE, EV_DELETE );
    while( m_curChange > 0 )
    {
        if ( m_pChanges[ m_curChange-1 ].udata == pHandler )
            --m_curChange;
        else
            break;
    }
    pHandler->setMask2( 0 );
    m_reactorIndex.set( pHandler->getfd(), NULL );
    return 0;
}

int KQueuer::waitAndProcessEvents( int iTimeoutMilliSec )
{
    struct timespec timeout;
    timeout.tv_sec  = iTimeoutMilliSec / 1000;
    timeout.tv_nsec = (iTimeoutMilliSec % 1000) * 1000000;
    int ret;
    struct kevent results[16];
    ret = kevent( m_fdKQ, m_pChanges, m_curChange, results, 16, &timeout );

    if (( ret == -1 )&&(( errno == EBADF )||(errno == ENOENT)))
    {
        if ( m_curChange > 0 )
        {
            memset( &timeout, 0, sizeof( timeout ) );
            for( int i = 0; i < m_curChange; ++i )
            {
                ret = kevent( m_fdKQ, m_pChanges+i, 1, results, 0, &timeout );
                if (( ret == -1 )&&(( errno == EBADF )||(errno == ENOENT))
                    &&( m_pChanges[i].flags != EV_DELETE ))
                {
                    ((EventReactor *)(m_pChanges[i].udata))->assignRevent( POLLERR );
                    ((EventReactor *)(m_pChanges[i].udata))->handleEvents( POLLERR );
                }
            }
        }
        memset( m_pChanges, 0, m_curChange * sizeof( struct kevent ) );
        m_curChange = 0;
        ret = 0;
    }        

    if ( m_curChange > 0 )
    {
//        if ( m_curChange == 1 )
//        {
//            if ( memcmp( &m_trace, m_pChanges, sizeof( m_trace )) == 0 )
//            {
//                ++m_traceCounter;
//                if ( m_traceCounter >= 10 )
//                {
//                    fprintf( stderr, "Kqueue problem: loop event, ident: %d, "
//                                     " filter: %hd, flags: %hd, fflags: %d, data: %d,"
//                                     " udata: %p\n", (int)m_pChanges->ident,
//                                     m_pChanges->filter, m_pChanges->flags, m_pChanges->fflags,
//                                     (int)m_pChanges->data, m_pChanges->udata );
//                    fprintf( stderr, "Kqueue problem: return event, ident: %d, "
//                                     " filter: %hd, flags: %hd, fflags: %d, data: %d,"
//                                     " udata: %p\n", (int)results[0].ident,
//                                     results[0].filter, results[0].flags, results[0].fflags,
//                                     (int)results[0].data, results[0].udata );
//
//                    m_traceCounter = 0;
//                }
//            }
//            else
//            {
//                memmove( &m_trace, m_pChanges, sizeof( m_trace ) );
//                m_traceCounter = 0;
//            }
//        }
//        else
//            m_traceCounter = 0;
        memset( m_pChanges, 0, m_curChange * sizeof( struct kevent ) );
        m_curChange = 0;
    }
//    else
//        m_traceCounter = 0;
    if ( ret > 0 )
    {
        struct kevent * pBegin = results;
        struct kevent * pEnd   = &results[ret];
        for( ; pBegin < pEnd; ++pBegin )
        {
            EventReactor * pReactor = (EventReactor *)pBegin->udata;
            if ( pReactor && (pReactor->getfd() == (int)pBegin->ident ))
            {
                short revent;
                if ( pBegin->flags & EV_ERROR )
                {
                    if ( pBegin->data != ENOENT )
                    {
                        fprintf( stderr, "kevent() error, fd: %d, error: %d, filter: %d flags: %04X\n", pReactor->getfd(), pBegin->data, pBegin->filter, pBegin->flags );
                    }
                    //revent = POLLERR;
                    continue;
                }
                else
                {
                    switch( pBegin->filter )
                    {
                    case EVFILT_READ:
                        if ( pBegin->flags & EV_EOF )
                        {
                            revent = POLLHUP|POLLIN;
                        }
                        else if ( pReactor->getEvents() & POLLIN )
                        {
                            revent = POLLIN;
                        }
                        else
                        {
                            appendEvent( pReactor, pReactor->getfd(), EVFILT_READ, EV_DELETE );
                            continue;
                        }
                        break;
                    case EVFILT_WRITE:
                        if ( pReactor->getEvents() & POLLOUT )
                        {
                            revent = POLLOUT;
                        }
                        else
                        {
                            appendEvent( pReactor, pReactor->getfd(), EVFILT_WRITE, EV_DELETE );
                            continue;
                        }
                        break;
                    default:
                        fprintf( stderr, "Kqueue: unkown event, ident: %d, "
                                         " filter: %hd, flags: %hd, fflags: %d, data: %d,"
                                         " udata: %p\n", (int)pBegin->ident,
                                         pBegin->filter, pBegin->flags, pBegin->fflags,
                                         (int)pBegin->data, pBegin->udata );
                        appendEvent( NULL, (int)pBegin->ident, pBegin->filter, EV_DELETE );
                        continue;
                    }
                }
                pReactor->assignRevent( revent );
                pReactor->handleEvents( revent );
            }
            else
            {
//                fprintf( stderr, "Kqueue: mystery event, ident: %d, "
//                                 " filter: %hd, flags: %hd, fflags: %d, data: %d,"
//                                 " udata: %p, reactor fd: %d\n", (int)pBegin->ident,
//                                 pBegin->filter, pBegin->flags, pBegin->fflags,
//                                 (int)pBegin->data, pBegin->udata,
//                                 (pReactor)?pReactor->getfd():-1 );
                //wil get this if modify event after socket closed, new socket created with the same file descriptor
                if ( pBegin->flags & EV_ERROR )  
                {
                    fprintf( stderr, "kevent(), mismatch handler, fd: %d, error: %d, filter: %d flags: %04X\n", pReactor->getfd(), pBegin->data, pBegin->filter, pBegin->flags );
                    //if ( (int)pBegin->data != EBADF )
                    //    close( (int)pBegin->ident );
                }
                else if ( !pReactor )
                    appendEvent( NULL, (int)pBegin->ident, pBegin->filter, EV_DELETE );
//                if ( pBegin->filter == EVFILT_READ )
//                    appendEvent( NULL, (int)pBegin->ident, EVFILT_READ,  EV_DELETE );
//                if ( pBegin->filter == EVFILT_READ )
//                    appendEvent( NULL, (int)pBegin->ident, EVFILT_WRITE, EV_DELETE );
            }
        }
    }
    return ret;
}

void KQueuer::timerExecute()
{
    m_reactorIndex.timerExec();
}

void KQueuer::setPriHandler( EventReactor::pri_handler handler )
{
}

void KQueuer::continueRead( EventReactor* pHandler )
{
    if ( !(pHandler->getEvents() & POLLIN) )
    {
        appendEvent( pHandler, EVFILT_READ,  EV_ADD | EV_ENABLE );
        pHandler->orMask2( POLLIN );
    }
}

void KQueuer::suspendRead( EventReactor* pHandler )
{
    if ( pHandler->getEvents() & POLLIN ) 
    {
        appendEvent( pHandler, EVFILT_READ,  EV_DELETE );
        pHandler->andMask2( ~POLLIN );
    }
}

void KQueuer::continueWrite( EventReactor* pHandler )
{
    if ( !(pHandler->getEvents() & POLLOUT) )
    {
        appendEvent( pHandler, EVFILT_WRITE,  EV_ADD | EV_ENABLE );
        pHandler->orMask2( POLLOUT );
    }
}

void KQueuer::suspendWrite( EventReactor* pHandler )
{
    if ( pHandler->getEvents() & POLLOUT )
    {
        appendEvent( pHandler, EVFILT_WRITE,  EV_DELETE );
        pHandler->andMask2( ~POLLOUT );
    }
}

void KQueuer::switchWriteToRead( EventReactor * pHandler )
{
    suspendWrite( pHandler );
    continueRead( pHandler );
}

void KQueuer::switchReadToWrite( EventReactor * pHandler )
{
    suspendRead( pHandler );
    continueWrite( pHandler );
}



#endif //defined(__FreeBSD__ ) || defined(__NetBSD__) || defined(__OpenBSD__) 
       //|| defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)

