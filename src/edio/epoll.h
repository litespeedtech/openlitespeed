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
#ifndef EPOLL_H
#define EPOLL_H

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#include <lsdef.h>
#include <edio/multiplexer.h>
#include <edio/reactorindex.h>



struct epoll_event;

class epoll : public Multiplexer
{
    int                  m_epfd;
    struct epoll_event *m_pResults;
    ReactorIndex         m_reactorIndex;
    int updateEvents(EventReactor *pHandler, short mask);

    void addEvent(EventReactor *pHandler, short mask)
    {
        pHandler->orMask2(mask);
        updateEvents(pHandler, pHandler->getEvents());
    }
    void removeEvent(EventReactor *pHandler, short mask)
    {
        pHandler->andMask2(~mask);
        updateEvents(pHandler, pHandler->getEvents());
    }
    void setEvents(EventReactor *pHandler, short mask)
    {
        if (pHandler->getEvents() != mask)
        {
            pHandler->setMask2(mask);
            updateEvents(pHandler, mask);
        }
    }
    int reinit();

public:
    epoll();
    ~epoll();
    virtual int getHandle() const   {   return m_epfd;  }
    virtual int init(int capacity = DEFAULT_CAPACITY);
    virtual int add(EventReactor *pHandler, short mask);
    virtual int remove(EventReactor *pHandler);
    virtual int waitAndProcessEvents(int iTimeoutMilliSec);
    virtual void timerExecute();
    virtual void setPriHandler(EventReactor::pri_handler handler) {};

    virtual void continueRead(EventReactor *pHandler);
    virtual void suspendRead(EventReactor *pHandler);
    virtual void continueWrite(EventReactor *pHandler);
    virtual void suspendWrite(EventReactor *pHandler);
    virtual void switchWriteToRead(EventReactor *pHandler);
    virtual void switchReadToWrite(EventReactor *pHandler);

    LS_NO_COPY_ASSIGN(epoll);

};


#endif

#endif
