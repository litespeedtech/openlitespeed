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

#include "moduleeventnotifier.h"

#include <lsiapi/internal.h>
#include <util/linkedobj.h>

class EventObj : public DLinkedObj
{
public:
    int                     m_iId;
    short                   m_iLevel;
    short                   m_iState;  //1 = OK, 0 = removed
    LsiSession             *m_pSession;
    lsi_module_t           *m_pModule;
};


void UserEventNotifier::removeEventObj(EventObj **pEventObj)
{
    if ((*pEventObj) && (*pEventObj)->m_iState == 1)
    {
        (*pEventObj)->remove();
        (*pEventObj)->m_iId = 0;
        (*pEventObj)->m_iLevel = 0;
        (*pEventObj)->m_pModule = 0;
        (*pEventObj)->m_pSession = 0;
        (*pEventObj)->m_iState = 0;
        delete *pEventObj;
        *pEventObj = NULL;
    }
}


int UserEventNotifier::onNotified(int count)
{
    while (!m_eventObjListDone.empty())
    {
        EventObj *pObj = (EventObj *)m_eventObjListDone.begin();
        if (pObj)
        {
            if (pObj->m_pModule)
                pObj->m_pSession->hookResumeCallback(pObj->m_iLevel, pObj->m_pModule);
            removeEventObj(&pObj);
        }
        else
            break;
    }
    return 0;
}


EventObj *UserEventNotifier::addEventObj(lsi_session_t *pSession,
        lsi_module_t *pModule, int level)
{
    EventObj *pEventObj = new EventObj;
    if (pEventObj)
    {
        pEventObj->m_iId = m_iId ++;
        pEventObj->m_iLevel = level;
        pEventObj->m_pModule = pModule;
        pEventObj->m_pSession = (LsiSession *)pSession;
        pEventObj->m_iState = 1;
        m_eventObjListWait.push_front(pEventObj);
    }
    return pEventObj;
}


int UserEventNotifier::isEventObjValid(EventObj *pEventObj)
{
    return (pEventObj->next() != NULL && pEventObj->prev() != NULL);
}


int UserEventNotifier::notifyEventObj(EventObj **pEventObj)
{
    //pEventObj should be in Wait list
    //assert();

    if (*pEventObj == NULL)
        return LS_FAIL;

    if (!isEventObjValid(*pEventObj))
        removeEventObj(pEventObj);

    //Move from Wait list to Done list
    EventObj *pNewObj = new EventObj;
    if (!pNewObj)
        return LS_FAIL; //ERROR

    pNewObj->m_iId = (*pEventObj)->m_iId;
    pNewObj->m_iLevel = (*pEventObj)->m_iLevel;
    pNewObj->m_pModule = (*pEventObj)->m_pModule;
    pNewObj->m_pSession = (*pEventObj)->m_pSession;
    pNewObj->m_iState = 1;
    m_eventObjListDone.push_front(pNewObj);
    removeEventObj(pEventObj);

    notify();
    return 0;
}

