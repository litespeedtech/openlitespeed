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
#include <http/httpsession.h>

EventObjPool UserEventNotifier::m_eventObjPool;


void UserEventNotifier::runAllScheduledEvent(EventObj *pFirstObj, void *pParamFilter)
{
    EventObj *pObj;
    EventObj *pObjNext;

    if (pFirstObj == NULL)
        pObj = (EventObj *)m_eventObjList.begin();
    else
        pObj = pFirstObj;

    while (pObj && pObj != (EventObj *)m_eventObjList.end()
        && (!pParamFilter || pObj->m_pParam == pParamFilter))
    {
        pObjNext = (EventObj *)pObj->next();
        
        if (D_ENABLED(DL_MEDIUM))
            LOG_D(("UserEventNotifier::runAllScheduledEvent() pEventObj=%p pParam=%p state=%d\n",
               pObj, pObj->m_pParam, pObj->m_state ));

        if (pObj && pObj->m_state == EVENTOBJ_ST_WAIT)
        {
            pObj->m_state = EVENTOBJ_ST_DONE;
            m_eventObjList.remove(pObj);
            pObj->m_eventCb(pObj->m_lParam, pObj->m_pParam);

            /**
             * Comment: the above cb may release the data, so checking here to avoid 
             * recycle again!
             */
            recycle(pObj);
            //pObj = NULL;
        }
        else
        {
            if (D_ENABLED(DL_MEDIUM))
                LOG_ERR(("UserEventNotifier::runAllScheduledEvent() wrong state:%d\n",
                         pObj->m_state));
        }
        pObj = pObjNext;
    }
}


EventObj *UserEventNotifier::createEventObj(lsi_event_callback_pf eventCb, 
                                         long lParam, 
                                         void *pParam)
{
    EventObj *pEventObj = m_eventObjPool.get();
    if (pEventObj)
    {
        pEventObj->m_eventCb = eventCb;
        pEventObj->m_lParam = lParam;
        pEventObj->m_pParam = pParam;
        pEventObj->m_state = EVENTOBJ_ST_INITED;
    }

    if (D_ENABLED(DL_MEDIUM))
        LOG_D(("UserEventNotifier::createEventObj() pEventObj=%p pParam=%p\n", 
               pEventObj, pEventObj->m_pParam));

    return pEventObj;
}


EventObj *UserEventNotifier::scheduleSessionEvent(lsi_event_callback_pf eventCb,
                                                  long lParam, HttpSession *pSession)
{
    EventObj *pObj = createEventObj(eventCb, lParam, (void *)pSession);
    pObj->m_state = EVENTOBJ_ST_WAIT;
    
    if (D_ENABLED(DL_MEDIUM))
        LOG_D(("UserEventNotifier::scheduleSessionEvent() session=%p pEventObj=%p pParam=%p to '%s'\n", 
               pSession, pObj, pObj->m_pParam, 
               (pSession->getEventObjHead() ? "exist" : "new") ));
    
    if (pSession->getEventObjHead() == NULL)
    {
        pSession->setEventObjHead(pObj);
        m_eventObjList.append(pObj);
    }
    else
    {
        m_eventObjList.append(pSession->getEventObjHead(), pObj);

        pObj = pSession->getEventObjHead();
        if (D_ENABLED(DL_MEDIUM))
            LOG_D(("UserEventNotifier::scheduleSessionEvent() EXIST event: session=%p pEventObj=%p pParam=%p\n", 
               pSession, pObj, pObj->m_pParam ));
    }
    return pObj;
}


int UserEventNotifier::notifyEventObj(EventObj *pEventObj)
{
    if (D_ENABLED(DL_MEDIUM))
        LOG_D(( "UserEventNotifier::notifyEventObj() pEventObj=%p pParam=%p state=%d\n",
                pEventObj, pEventObj->m_pParam, pEventObj->m_state ));

     if (pEventObj == NULL || pEventObj->m_state != EVENTOBJ_ST_INITED)
         return LS_FAIL;

    pEventObj->m_state = EVENTOBJ_ST_WAIT;
    m_eventObjList.append(pEventObj);
    return 0;
}

void UserEventNotifier::removeEventObj(EventObj *pEventObj)
{
    if (!pEventObj)
        return;

    if (D_ENABLED(DL_MEDIUM))
        LOG_D(( "UserEventNotifier::removeEventObj( ) pEventObj=%p pParam=%p state=%d\n",
                pEventObj, pEventObj->m_pParam, pEventObj->m_state ));

    switch (pEventObj->m_state)
    {
            /**
             * To test if this still in the list
             */
            //if ((*pEventObj)->next() && (*pEventObj)->prev())
            //    m_eventObjList.remove(*pEventObj);
        case EVENTOBJ_ST_WAIT:
            m_eventObjList.remove(pEventObj);

        case EVENTOBJ_ST_INITED:
            recycle(pEventObj);
            break;
        case EVENTOBJ_ST_DONE:
        default:
            break;
    }
   // *pEventObj = NULL;
}

void UserEventNotifier::removeSessionEvents(EventObj *pFirstObj, HttpSession *pSession)
{
    EventObj *pObj = pFirstObj;
    EventObj *pObjNext;

    if (!pObj)
        pObj = (EventObj *)m_eventObjList.begin();

    if (D_ENABLED(DL_MEDIUM))
        LOG_D(("UserEventNotifier::removeSessionEvents() start with pFirstObj=%p pParam=%p state=%d\n",
              pObj, pObj->m_pParam, pObj->m_state ));

    while (pObj && pObj != (EventObj *)m_eventObjList.end())
    {
        pObjNext = (EventObj *)pObj->next();
        if (pObj->m_pParam == (void *)pSession)
        {
            removeEventObj(pObj);
        }
        pObj = pObjNext;
    }
}

