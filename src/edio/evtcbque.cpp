/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2020  LiteSpeed Technologies, Inc.                 *
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
#include <edio/multiplexer.h>
#include <edio/multiplexerfactory.h>
#include <lsiapi/internal.h>
#include <lsr/ls_internal.h>



struct evtcbnode_s : public DLinkedObj
{
    evtcb_pf        m_callback;
    evtcbhead_t    *m_pSession;
    long            m_lParam;
    void           *m_pParam;
};

typedef ObjPool<evtcbnode_s> CallbackObjPool;
static CallbackObjPool *s_pCbnodePool = NULL;
static int32_t          s_poolLock = 0;

LS_SINGLETON(EvtcbQue);

EvtcbQue::EvtcbQue()
    : m_iFlag(0)
    , m_pNotifier(NULL)
{
    s_pCbnodePool = new CallbackObjPool;
    m_lock = 0;
}


int EvtcbQue::initNotifier()
{
    if (m_pNotifier)
    {
        MultiplexerFactory::getMultiplexer()->remove(m_pNotifier);
        delete m_pNotifier;
    }
    m_pNotifier = new EvtcbQueNotifier;
    return m_pNotifier->initNotifier(MultiplexerFactory::getMultiplexer());
}


EvtcbQue::~EvtcbQue()
{
    s_pCbnodePool->shrinkTo(0);
    delete s_pCbnodePool;
    s_pCbnodePool = NULL;
    if (m_pNotifier)
        delete m_pNotifier;
    m_callbackObjList.clear();
}


void EvtcbQue::logState(const char *s, evtcbnode_s *p)
{
    if (p)
        LS_DBG_M("[EvtcbQue:%s] Obj=%p Session= %p Param=%p\n",
                 s, p, p->m_pSession, p->m_pParam);
    else
        LS_ERROR("[EvtcbQue:%s] Obj=NULL\n", s);
}


// Must have the lock before entering this method.
evtcbnode_s *EvtcbQue::extractSession(evtcbnode_s *pObj)
{
    evtcbnode_s *pHead = (evtcbnode_s *)(m_callbackObjList.head());
    evtcbnode_s *pObjNext = (evtcbnode_s *)pObj->next();
    evtcbnode_s *pObjPrev = (evtcbnode_s *)pObj->prev();
    int cnt = 1;

    while (pObjPrev != pHead && pObjPrev->m_pSession == pObj->m_pSession)
    {
        ++cnt;
        pObjPrev = (evtcbnode_s *)pObjPrev->prev();
    }

    // pObjNext points to the node AFTER all the nodes in the session.
    // pObjPrev points to the node BEFORE all the nodes in the session.

    m_callbackObjList.setSize(m_callbackObjList.size() - cnt);
    pObj->setNext(NULL);
    pObjNext->setPrev(pObjPrev);

    // Set pObj to session's head. The m_pSession value is shared.
    pObj = (evtcbnode_s *)pObjPrev->next();
    pObjPrev->next()->setPrev(NULL);
    pObjPrev->setNext(pObjNext);

    // Clear tail to indicate that the session is removed from the queue.
    evtcbhead_set_tail(pObj->m_pSession, NULL);

    return pObj;
}


// Must NOT have the lock before entering this method.
void EvtcbQue::runNodes(evtcbnode_s *pObj)
{
    evtcbnode_s *pObjNext;
    while(pObj)
    {
        pObjNext = (evtcbnode_s *)pObj->remove();
        if (pObj->m_pSession)
            evtcbhead_set_runqueue(pObj->m_pSession, pObjNext);
        runOne(pObj);
        pObj = pObjNext;
    }
}


/**
 * session is a filter. If NULL, run all in queue
 */
void EvtcbQue::run(evtcbhead_t *session)
{
    evtcbnode_s *pObj;

    assert(session != NULL);

    if (getFlag(EQF_RUNNING))
        return;

    ls_atomic_spin_lock(&m_lock);
    pObj = evtcbhead_get_tail(session);

    if (pObj)
        pObj = extractSession(pObj); // Update to the head of the session list.

    ls_atomic_spin_unlock(&m_lock);

    runNodes(pObj);
}


/**
 * session is a filter. If NULL, run all in queue
 */
void EvtcbQue::run()
{
    int empty = 0;
    evtcbnode_s *pObj = NULL, *pObjNext = NULL;

    m_pNotifier->setPending(0);

    ls_atomic_spin_lock(&m_lock);
    if (!m_callbackObjList.empty())
    {
        pObj = (evtcbnode_s *)m_callbackObjList.begin();
        pObjNext = (evtcbnode_s *)m_callbackObjList.end()->prev();

        m_callbackObjList.clear();
        pObj->setPrev(NULL);
        pObjNext->setNext(NULL);
    }

    setFlag(EQF_RUNNING);

    while (pObj)
    {
        // pObjNext points to HEAD of next session.
        if (pObj->m_pSession)
        {
            // pObjNext will point to the HEAD of the next session or NULL if this is the last session.
            pObjNext = (evtcbnode_s *)evtcbhead_get_tail(pObj->m_pSession)->next();

            if (pObjNext)
                pObjNext->setPrev(NULL);
            // Ensure that runNodes only runs for this session
            evtcbhead_get_tail(pObj->m_pSession)->setNext(NULL);

            evtcbhead_set_tail(pObj->m_pSession, NULL);
            ls_atomic_spin_unlock(&m_lock);

            logState("run() starts", pObj);
            runNodes(pObj);
            ls_atomic_spin_lock(&m_lock);
        }
        else // No session attached, run single node.
        {
            pObjNext = (evtcbnode_s *)pObj->remove();
            ls_atomic_spin_unlock(&m_lock);
            runOne(pObj);
            ls_atomic_spin_lock(&m_lock);
        }
        pObj = pObjNext;
    }

    clearFlag(EQF_RUNNING);

    empty = m_callbackObjList.empty();
    ls_atomic_spin_unlock(&m_lock);
    if (!empty)
        notify();
}


// Must NOT have the lock before entering this method.
void EvtcbQue::runOne(evtcbnode_s *pObj)
{
    logState("runOne()", pObj);

    if (pObj->m_pSession)
        evtcbhead_backref_clear_if_match(pObj->m_pSession, &pObj->m_pSession);

    if (pObj->m_callback)
        pObj->m_callback(pObj->m_pSession, pObj->m_lParam, pObj->m_pParam);

    recycle(pObj);
}


evtcbnode_s *EvtcbQue::getNodeObj(evtcb_pf cb,
                                evtcbhead_t *session, long lParam, void *pParam)
{
    ls_atomic_spin_lock(&s_poolLock);
    evtcbnode_s *pObj = s_pCbnodePool->get();
    ls_atomic_spin_unlock(&s_poolLock);
    //logState("getNodeObj", pObj);

    if (pObj)
    {
        pObj->m_callback = cb;
        pObj->m_pSession = session;
        pObj->m_lParam = lParam;
        pObj->m_pParam = pParam;
        logState("getNodeObj evtcbquedbg", pObj);
    }
    return pObj;
}


void EvtcbQue::schedule(evtcbnode_s *pObj)
{
    evtcbhead_t *session = pObj->m_pSession;
    logState("schedule()", pObj);
    ls_atomic_spin_lock(&m_lock);
    if (session)
    {
        if (!evtcbhead_is_queued(session))
            evtcbhead_set_tail(session, pObj);
        else
        {
            m_callbackObjList.insert_after(evtcbhead_get_tail(session), pObj);
            evtcbhead_set_tail(session, pObj);
            ls_atomic_spin_unlock(&m_lock);

            logState("schedule()][Header EXIST", pObj);
            return;
        }
    }

    m_callbackObjList.append(pObj);
    ls_atomic_spin_unlock(&m_lock);
}


evtcbnode_s *EvtcbQue::schedule(evtcb_pf cb, evtcbhead_t *session,
                                long lParam, void *pParam)
{
    evtcbnode_s *pObj = getNodeObj(cb, session, lParam, pParam);
    if (pObj)
        schedule(pObj);

    return pObj;
}


int EvtcbQue::schedule_once(evtcbnode_s *pObj)
{
    evtcbhead_t *session = pObj->m_pSession;
    logState("schedule_once()", pObj);
    ls_atomic_spin_lock(&m_lock);
    if (session)
    {
        if (!evtcbhead_is_queued(session))
            evtcbhead_set_tail(session, pObj);
        else
        {
            ls_atomic_spin_unlock(&m_lock);
            logState("schedule_once()][Header EXIST", pObj);
            return -1;
        }
    }

    m_callbackObjList.append(pObj);
    ls_atomic_spin_unlock(&m_lock);
    return 0;
}


evtcbnode_s *EvtcbQue::schedule_once(evtcb_pf cb, evtcbhead_t *session,
                                long lParam, void *pParam)
{
    evtcbnode_s *pObj = getNodeObj(cb, session, lParam, pParam);
    if (pObj && schedule_once(pObj) != 0)
    {
        recycle(pObj);
        pObj = NULL;
    }

    return pObj;
}


// Must NOT have the lock before entering this method.
void EvtcbQue::recycle(evtcbnode_s *pObj)
{
    logState("recycle()", pObj);
    memset((void *) pObj, 0, sizeof(evtcbnode_s));
    ls_atomic_spin_lock(&s_poolLock);
    s_pCbnodePool->recycle(pObj);
    ls_atomic_spin_unlock(&s_poolLock);
}


void EvtcbQue::removeSessionCb(evtcbhead_t *session)
{
    evtcbnode_s *pObj;
    evtcbnode_s *pObjIter;

    LS_DBG_M("removeSessionCb(), session=%p", session);

    ls_atomic_spin_lock(&m_lock);
    pObj = evtcbhead_set_tail(session, NULL);
    while (pObj && pObj != (evtcbnode_s *)m_callbackObjList.head())
    {
        logState("removeSessionCb()][check", pObj);

        pObjIter = (evtcbnode_s *)pObj->prev();
        if (pObj->m_pSession == session)
        {
            logState("removeSessionCb()][clear", pObj);
            pObj->m_pSession = NULL;
            pObj->m_callback = NULL;
        }
        else
            break;
        pObj = pObjIter;
    }
    ls_atomic_spin_unlock(&m_lock);

    pObj = evtcbhead_set_runqueue(session, NULL);
    while (pObj)
    {
        logState("removeSessionCb()][checkRunQueue", pObj);

        pObjIter = (evtcbnode_s *)pObj->next();
        if (pObj->m_pSession == session)
        {
            logState("removeSessionCb()][clearRunQueue", pObj);
            pObj->m_pSession = NULL;
            pObj->m_callback = NULL;
        }
        else
            break;
        pObj = pObjIter;
    }
}

evtcbhead_t **EvtcbQue::getSessionRefPtr(evtcbnode_s *nodeObj)
{
    if (!nodeObj)
        return NULL;
    //logState("getSessionRefPtr()", nodeObj);
    return &nodeObj->m_pSession;
}


void EvtcbQue::sessionDebug(evtcbhead_t *session)
{
    if (NULL == session)
    {
        LS_NOTICE("[EVTCBQUEDBG] sessionDebug() session is null.");
        return;
    }

    LS_NOTICE("[EVTCBQUEDBG] sessionDebug() session %p, head %p, back_ref_ptr %p",
              session, evtcbhead_get_tail(session), evtcbhead_get_backrefptr(session));

    if (evtcbhead_is_queued(session))
    {
        LS_NOTICE("[EVTCBQUEDBG] sessionDebug() is queued session %p, head %p, back_ref_ptr %p",
                  session, evtcbhead_get_tail(session), evtcbhead_get_backrefptr(session));
    }

    if (evtcbhead_is_running(session))
    {
        LS_NOTICE("[EVTCBQUEDBG] sessionDebug() is running session %p, head %p, back_ref_ptr %p",
                  session, evtcbhead_get_runqueue(session), evtcbhead_get_backrefptr(session));
    }
}
