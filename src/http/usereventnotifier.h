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
#ifndef MODULEEVENTNOTIFIER_H
#define MODULEEVENTNOTIFIER_H

#include <lsdef.h>
#include <ls.h>
#include <util/dlinkqueue.h>
#include <util/objpool.h>
#include <edio/eventnotifier.h>
#include <util/tsingleton.h>

class HttpSession;

enum {
    EVENTOBJ_ST_UNINITED = 0,
    EVENTOBJ_ST_INITED,
    EVENTOBJ_ST_WAIT,
    EVENTOBJ_ST_DONE,
};

class EventObj : public DLinkedObj
{
public:
    lsi_event_callback_pf   m_eventCb;
    long            m_lParam;
    void           *m_pParam;
    unsigned long   m_state;
};

typedef ObjPool<EventObj>               EventObjPool;

class UserEventNotifier : public TSingleton<UserEventNotifier>
{
    friend class TSingleton<UserEventNotifier>;

    DLinkQueue  m_eventObjList;

public:
    static EventObjPool m_eventObjPool;
    UserEventNotifier() {};
    ~UserEventNotifier() { m_eventObjPool.shrinkTo(0); }

    /**
     * Non-auto EventObj need to be created, notified and removed
     */
    EventObj *createEventObj(lsi_event_callback_pf eventCb, long lParam, void *pParam);
    int notifyEventObj(EventObj *pEventObj);
    void removeEventObj(EventObj *pEventObj);
    void recycle(EventObj *pObj)
    {
        memset(pObj, 0, sizeof(EventObj));
        m_eventObjPool.recycle(pObj);//delete *pEventObj;//
    }

    
    void runAllScheduledEvent(EventObj *pFirstObj, void *pParamFilter);

    /**
     * Special function for session
     */
    EventObj *scheduleSessionEvent(lsi_event_callback_pf eventCb, long lParam, HttpSession *pSession);
    void removeSessionEvents(EventObj *pFirstObj, HttpSession *pSession);

    LS_NO_COPY_ASSIGN(UserEventNotifier);
};
#endif
