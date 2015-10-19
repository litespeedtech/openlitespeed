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
#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#include "epoll.h"

// #include <log4cxx/logger.h>

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

// #include <typeinfo>


static int s_loop_fd = -1;
static int s_loop_count = 0;
static int s_problems = 0;
epoll::epoll()
    : m_epfd(-1)
    , m_pResults(NULL)
{
    setFLTag(O_NONBLOCK | O_RDWR);

}


epoll::~epoll()
{
    if (m_epfd != -1)
        close(m_epfd);
    if (m_pResults)
        free(m_pResults);
}


#define EPOLL_RESULT_BUF_SIZE   4096
//#define EPOLL_RESULT_MAX        EPOLL_RESULT_BUF_SIZE / sizeof( struct epoll_event )
#define EPOLL_RESULT_MAX        10
int epoll::init(int capacity)
{
    if (m_reactorIndex.allocate(capacity) == -1)
        return LS_FAIL;
    if (!m_pResults)
    {
        m_pResults = (struct epoll_event *)malloc(EPOLL_RESULT_BUF_SIZE);
        if (!m_pResults)
            return LS_FAIL;
        memset(m_pResults, 0, EPOLL_RESULT_BUF_SIZE);
    }
    if (m_epfd != -1)
        close(m_epfd);
    //m_epfd = (syscall(__NR_epoll_create, capacity ));
    m_epfd = epoll_create(capacity);
    if (m_epfd == -1)
        return LS_FAIL;
    ::fcntl(m_epfd, F_SETFD, FD_CLOEXEC);
    return LS_OK;
}


int epoll::reinit()
{
    struct epoll_event epevt;
    close(m_epfd);
    m_epfd = (syscall(__NR_epoll_create, m_reactorIndex.getCapacity()));
    if (m_epfd == -1)
        return LS_FAIL;
    epevt.data.u64 = 0;
    ::fcntl(m_epfd, F_SETFD, FD_CLOEXEC);
    int n = m_reactorIndex.getUsed();
    for (int i = 0; i < n; ++i)
    {
        EventReactor *pHandler = m_reactorIndex.get(i);
        if (pHandler)
        {
            epevt.data.u64 = 0;
            epevt.data.fd = pHandler->getfd();
            epevt.events = pHandler->getEvents();
            //(syscall(__NR_epoll_ctl, m_epfd, EPOLL_CTL_ADD, epevt.data.fd, &epevt));
            epoll_ctl(m_epfd, EPOLL_CTL_ADD, epevt.data.fd, &epevt);
        }
    }
    return 0;
}
/*
void dump_type_info( EventReactor * pHandler, const char * pMsg )
{
    LS_INFO( "[%d] %s EventReactor: %p, fd: %d, type: %s", getpid(), pMsg, pHandler, pHandler->getfd(),
                typeid( *pHandler ).name() ));
}
*/
int epoll::add(EventReactor *pHandler, short mask)
{
    struct epoll_event epevt;
    int fd = pHandler->getfd();
    if (fd == -1)
        return 0;
    //assert( fd > 1 );
    if (fd > 100000)
        return LS_FAIL;
    memset(&epevt, 0, sizeof(struct epoll_event));
    m_reactorIndex.set(fd, pHandler);
    pHandler->setPollfd();
    pHandler->setMask2(mask);
    pHandler->clearRevent();
    epevt.data.u64 = 0;
    epevt.data.fd = fd;
    epevt.events = mask;
//     if ( LS_LOG_ENABLED( LOG4CXX_NS::Level::DBG_LESS ) || s_problems )
    //    dump_type_info( pHandler, "added" );
    //return (syscall(__NR_epoll_ctl, m_epfd, EPOLL_CTL_ADD, fd, &epevt));
    return epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &epevt);
}


void epoll::setEventMask(EventReactor *pHandler, short mask)
{
    struct epoll_event epevt;
    int fd = pHandler->getfd();
    if (fd == -1)
        return;
    assert(pHandler == m_reactorIndex.get(fd));
    if (pHandler->getEvents() == mask )
        return;
    pHandler->setMask2(mask);
    //assert( fd > 1 );
    memset(&epevt, 0, sizeof(struct epoll_event));
    epevt.data.u64 = 0;
    epevt.data.fd = fd;
    epevt.events = mask;
    //return (syscall(__NR_epoll_ctl, m_epfd, EPOLL_CTL_MOD, fd, &epevt));
    epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &epevt);
}


int epoll::remove(EventReactor *pHandler)
{
    struct epoll_event epevt;
    int fd = pHandler->getfd();
    if (fd == -1)
        return 0;
    //assert( pHandler == m_reactorIndex.get( fd ) );
    pHandler->clearRevent();
    //assert( fd > 1 );
    memset(&epevt, 0, sizeof(struct epoll_event));

    epevt.data.u64 = 0;
    epevt.data.fd = fd;
    epevt.events = 0;

    if (fd <= (int)m_reactorIndex.getUsed())
        m_reactorIndex.set(fd, NULL);
    //if ( D_ENABLED( DL_LESS ) || s_problems )
    //    dump_type_info( pHandler, "remove" );
    //return (syscall(__NR_epoll_ctl, m_epfd, EPOLL_CTL_DEL, fd, &epevt ));
    return epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, &epevt);
}


int epoll::waitAndProcessEvents(int iTimeoutMilliSec)
{
    //int ret = (syscall(__NR_epoll_wait, m_epfd, m_pResults, EPOLL_RESULT_MAX, iTimeoutMilliSec ));
    int ret = epoll_wait(m_epfd, m_pResults, EPOLL_RESULT_MAX,
                         iTimeoutMilliSec);
    if (ret <= 0)
        return ret;
    if (ret == 1)
    {
        int fd = m_pResults->data.fd;
        EventReactor *pReactor = m_reactorIndex.get(fd);
        if (pReactor && (pReactor->getfd() == fd))
        {
            if (m_pResults->events & POLLHUP)
                pReactor->incHupCounter();
            pReactor->assignRevent(m_pResults->events);
            pReactor->handleEvents(m_pResults->events);
        }
        return 1;
    }
    //if ( ret > EPOLL_RESULT_MAX )
    //    ret = EPOLL_RESULT_MAX;
    int    problem_detected = 0;
    struct epoll_event *pEnd = m_pResults + ret;
    struct epoll_event *p = m_pResults;
    while (p < pEnd)
    {
        int fd = p->data.fd;
        EventReactor *pReactor = m_reactorIndex.get(fd);
        assert(p->events);
        if (pReactor)
        {
            if (pReactor->getfd() == fd)
                pReactor->assignRevent(p->events);
            else
                p->data.fd = -1;
        }
        else
        {
            //p->data.fd = -1;
            if ((s_loop_fd == -1) || (s_loop_fd == fd))
            {
                if (s_loop_fd == -1)
                {
                    s_loop_fd = fd;
                    s_loop_count = 0;
                }
                problem_detected = 1;
                ++s_loop_count;
                if (s_loop_count == 10)
                {
                    if (p->events & (POLLHUP | POLLERR))
                        close(fd);
                    else
                    {
                        struct epoll_event epevt;
                        memset(&epevt, 0, sizeof(struct epoll_event));
                        epevt.data.u64 = 0;
                        epevt.data.fd = fd;
                        epevt.events = 0;
                        (syscall(__NR_epoll_ctl, m_epfd, EPOLL_CTL_DEL, fd, &epevt));
                        //LS_WARN( "[%d] Remove looping fd: %d, event: %d\n", getpid(), fd, p->events ));
                        ++s_problems;
                    }
                }
                else if (s_loop_count >= 20)
                {
                    //LS_WARN( "Looping fd: %d, event: %d\n", fd, p->events ));
                    assert(p->events);
                    problem_detected = 0;
                }
            }
        }
        ++p;
    }
    p = m_pResults;
    while (p < pEnd)
    {
        int fd = p->data.fd;
        if (fd != -1)
        {
            EventReactor *pReactor = m_reactorIndex.get(fd);
            if (pReactor && (pReactor->getAssignedRevent() == (int)p->events))
            {
                if (p->events & POLLHUP)
                    pReactor->incHupCounter();
                pReactor->handleEvents(p->events);
            }
        }
        ++p;
    }

    if (!problem_detected && s_loop_count)
    {
        s_loop_fd = -1;
        s_loop_count = 0;
    }
    memset(m_pResults, 0, sizeof(struct epoll_event) * ret);
    return ret;

}


void epoll::timerExecute()
{
    m_reactorIndex.timerExec();
}


void epoll::modEvent(EventReactor *pHandler, short maskIn, int add_remove)
{
    short mask;
    
    mask = pHandler->getEvents() & maskIn;
    if (add_remove == 0)
    {
        if (mask == 0)
            return;
        mask = pHandler->getEvents() & ~mask;
    }
    else 
    {
        mask = mask ^ maskIn;
        if (mask == 0)
            return;
        mask = pHandler->getEvents() | mask;
    }
    setEventMask(pHandler, mask);
}


void epoll::continueRead(EventReactor *pHandler)
{
    if (!(pHandler->getEvents() & POLLIN))
        addEvent(pHandler, POLLIN);
}


void epoll::suspendRead(EventReactor *pHandler)
{
    if (pHandler->getEvents() & POLLIN)
        removeEvent(pHandler, POLLIN);
}


void epoll::continueWrite(EventReactor *pHandler)
{
    if (!(pHandler->getEvents() & POLLOUT))
        addEvent(pHandler, POLLOUT);
}


void epoll::suspendWrite(EventReactor *pHandler)
{
    if (pHandler->getEvents() & POLLOUT)
        removeEvent(pHandler, POLLOUT);
}


void epoll::switchWriteToRead(EventReactor *pHandler)
{
    setEventMask(pHandler, POLLIN | POLLHUP | POLLERR);
}


void epoll::switchReadToWrite(EventReactor *pHandler)
{
    setEventMask(pHandler, POLLOUT | POLLHUP | POLLERR);
}


#endif
