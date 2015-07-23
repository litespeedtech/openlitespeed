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
#include "evtcbque.h"
#include <log4cxx/logger.h>
#include <util/objpool.h>

struct evtcbnode_s : public DLinkedObj
{
    evtcb_pf        m_callback;
    evtcbhead_t    *m_pSession;
    long            m_lParam;
    void           *m_pParam;
};

typedef ObjPool<evtcbnode_s> CallbackObjPool;
static CallbackObjPool *s_pCbnodePool;


EvtcbQue::EvtcbQue()
{
    s_pCbnodePool = new CallbackObjPool;
}

EvtcbQue::~EvtcbQue()
{
    s_pCbnodePool->shrinkTo(0);
    delete s_pCbnodePool;
    m_callbackObjList.pop_all();
}


void EvtcbQue::logState(const char *s, evtcbnode_s *p)
{
    if (p)
        LS_DBG_M("[EvtcbQue:%s] Obj=%p Session= %p Param=%p\n",
                 s, p, p->m_pSession, p->m_pParam);
    else
        LS_ERROR("[EvtcbQue:%s] Obj=NULL\n", s);
}

/**
 * session is a filter. If NULL, run all in queue
 */
void EvtcbQue::run(evtcbhead_t *session)
{
    evtcbnode_s *pObj;
    evtcbnode_s *pObjNext;

    if (session == NULL)
        pObj = (evtcbnode_s *)m_callbackObjList.begin();
    else
        pObj = session->evtcb_head;

    assert(m_callbackObjList.size() > 0
           || (m_callbackObjList.begin() ==
               m_callbackObjList.end())); //WRONG state!!!


    while (m_callbackObjList.size() > 0
           && pObj && pObj != (evtcbnode_s *)m_callbackObjList.end()
           && (!session || pObj->m_pSession == session))
    {
        pObjNext = (evtcbnode_s *)pObj->next();
        logState("run()", pObj);

        m_callbackObjList.remove(pObj);

        if (pObj->m_pSession)
            pObj->m_pSession->evtcb_head = NULL;


        assert(pObj->m_callback);
        if (pObj->m_callback)
            pObj->m_callback(pObj->m_pSession, pObj->m_lParam,
                             pObj->m_pParam);
        else
            logState("run()][Error: NULL calback", pObj);

        recycle(pObj);
        pObj = pObjNext;
    }
}


evtcbnode_s *EvtcbQue::schedule(evtcb_pf cb,
                                evtcbhead_t *session, long lParam, void *pParam)
{
    evtcbnode_s *pObj = s_pCbnodePool->get();
    if (pObj)
    {
        pObj->m_callback = cb;
        pObj->m_pSession = session;
        pObj->m_lParam = lParam;
        pObj->m_pParam = pParam;
        if (session)
        {
            if (session->evtcb_head == NULL)
            {
                session->evtcb_head = pObj;
                m_callbackObjList.append(pObj);
            }
            else
            {
                m_callbackObjList.append(session->evtcb_head, pObj);
                logState("schedule()][Header EXIST", pObj);
            }
        }
        else
            m_callbackObjList.append(pObj);
    }

    logState("schedule()", pObj);
    return pObj;
}


void EvtcbQue::removeObj(evtcbnode_s *pObj)
{
    if (!pObj)
        return;

    logState("removeObj()", pObj);
    if (pObj->next())
    {
        m_callbackObjList.remove(pObj);
        recycle(pObj);
    }
}

void EvtcbQue::recycle(evtcbnode_s *pObj)
{
    memset(pObj, 0, sizeof(evtcbnode_s));
    s_pCbnodePool->recycle(pObj);
}


void EvtcbQue::removeSessionCb(evtcbhead_t *session)
{
    evtcbnode_s *pObj = session->evtcb_head;
    evtcbnode_s *pObjNext;

    logState("removeSessionCb()][header state", pObj);
    while (pObj && pObj != (evtcbnode_s *)m_callbackObjList.end())
    {
        pObjNext = (evtcbnode_s *)pObj->next();
        if (pObj->m_pSession == session)
            removeObj(pObj);
        else
            break;
        pObj = pObjNext;
    }
    session->evtcb_head = NULL;
}

