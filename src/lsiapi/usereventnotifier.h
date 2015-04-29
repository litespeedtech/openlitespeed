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
#include <edio/eventnotifier.h>
#include <util/dlinkqueue.h>
#include <util/tsingleton.h>

class LsiSession;
class EventObj;

class UserEventNotifier : public EventNotifier,
                            public TSingleton<UserEventNotifier>
{
    friend class TSingleton<UserEventNotifier>;

    DLinkQueue  m_eventObjListWait;
    DLinkQueue  m_eventObjListDone;
    int         m_iId;

    UserEventNotifier() { m_iId = 0; }
public:

    void removeEventObj(EventObj **pEventObj);

    virtual int onNotified(int count);

    EventObj *addEventObj(lsi_session_t *pSession, lsi_module_t *pModule,
                          int level);

    int isEventObjValid(EventObj *pEventObj);

    int notifyEventObj(EventObj **pEventObj);


    LS_NO_COPY_ASSIGN(UserEventNotifier);
};


#endif // MODULEEVENTNOTIFIER_H
