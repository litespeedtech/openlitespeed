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
#include "usereventnotifier.h"
#include <http/httpsession.h>
#include <http/httplog.h>

EventObjPool UserEventNotifier::m_eventObjPool;

UserEventNotifier::~UserEventNotifier()
{
   m_eventObjPool.shrinkTo(0);
}

void UserEventNotifier::runAllScheduledEvent()
{
    while (!m_eventObjList.empty())
    {
        EventObj *pObj = (EventObj *)m_eventObjList.begin();
        m_eventObjList.pop_front();

        if (D_ENABLED(DL_MEDIUM))
            LOG_D(("UserEventNotifier::runAllScheduledEvent() pEventObj=%p pParam=%p state=%d\n",
               pObj, pObj->m_pParam, pObj->m_state ));

        if (pObj && (pObj->m_state & 0x03) == EVENTOBJ_ST_WAIT)
        {
            pObj->m_state = EVENTOBJ_ST_DONE;
            pObj->m_eventCb(pObj->m_lParam, pObj->m_pParam);

            /**
             * Comment: the above cb may release the data, so checking here to avoid 
             * recycle again!
             */
            if ((pObj->m_state & EVENTOBJ_ST_AUTOMATIC) == EVENTOBJ_ST_AUTOMATIC)
            {
                memset(pObj, 0, sizeof(EventObj));
                m_eventObjPool.recycle(pObj);
            }
        }
        else
        {
            if (D_ENABLED(DL_MEDIUM))
                LOG_ERR(("UserEventNotifier::runAllScheduledEvent() wrong state\n"));
        }
    }
}

void UserEventNotifier::addToList(EventObj *pObj, bool isAuto)
{
    pObj->m_state = EVENTOBJ_ST_WAIT | (isAuto ? EVENTOBJ_ST_AUTOMATIC : 0);
    m_eventObjList.append(pObj);
}

EventObj *UserEventNotifier::addEventObj(lsi_event_callback_pf eventCb, 
                                         long lParam, 
                                         void *pParam,
                                         bool isAuto)
{
    EventObj *pEventObj = m_eventObjPool.get(); //new EventObj;// 
    if (pEventObj)
    {
        pEventObj->m_eventCb = eventCb;
        pEventObj->m_lParam = lParam;
        pEventObj->m_pParam = pParam;
        pEventObj->m_state = EVENTOBJ_ST_INITED;
        if (isAuto)
            addToList(pEventObj, true);
    }

    if (D_ENABLED(DL_MEDIUM))
        LOG_D(("UserEventNotifier::addEventObj() pEventObj=%p pParam=%p\n", 
               pEventObj, pEventObj->m_pParam));

    return pEventObj;
}


int UserEventNotifier::notifyEventObj(EventObj **pEventObj)
{
    if (D_ENABLED(DL_MEDIUM))
        LOG_D(( "UserEventNotifier::notifyEventObj() pEventObj=%p pParam=%p state=%d\n",
                *pEventObj, (*pEventObj)->m_pParam, (*pEventObj)->m_state ));

     if (*pEventObj == NULL || (*pEventObj)->m_state != EVENTOBJ_ST_INITED)
         return LS_FAIL;

    addToList(*pEventObj, false);
    return 0;
}

void UserEventNotifier::removeEventObj(EventObj **pEventObj)
{
    if (D_ENABLED(DL_MEDIUM))
        LOG_D(( "UserEventNotifier::removeEventObj( ) pEventObj=%p pParam=%p state=%d\n",
                *pEventObj, (*pEventObj)->m_pParam, (*pEventObj)->m_state ));

    switch ((*pEventObj)->m_state)
    {
        case EVENTOBJ_ST_WAIT:
            /**
             * To test if this still in the list
             */
            if ((*pEventObj)->next() && (*pEventObj)->prev())
                m_eventObjList.remove(*pEventObj);

        case EVENTOBJ_ST_DONE:
        case EVENTOBJ_ST_INITED:
            memset(*pEventObj, 0, sizeof(EventObj));
            m_eventObjPool.recycle(*pEventObj);//delete *pEventObj;//
            break;
        default:
            break;
    }
}
