/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
    evtcbtail_t    *m_pSession;
    long            m_lParam;
    void           *m_pParam;
};


typedef ObjPool<evtcbnode_s> CallbackObjPool;
static CallbackObjPool *s_pCbnodePool;


LS_SINGLETON(EvtcbQue);


EvtcbQue::EvtcbQue()
{
    s_pCbnodePool = new CallbackObjPool;
    m_pNotifier = new EvtcbQueNotifier;
    m_pNotifier->initNotifier(MultiplexerFactory::getMultiplexer());
    for (int i = 0; i < NUM_SESS_QUES; i++)
    {
        ls_mutex_setup(&m_sessLks[i]);
    }
    ls_mutex_setup(&m_runQlock);
    ls_mutex_setup(&m_nodePoolLock);
    ls_atomic_fetch_and(&m_notified, 0); // don't need atomic in constr but should shut up TSAN
}


EvtcbQue::~EvtcbQue()
{
    s_pCbnodePool->shrinkTo(0);
    delete s_pCbnodePool;
    delete m_pNotifier;
    for (int i = 0; i < NUM_SESS_QUES; i++)
    {
        m_sessCallbackObjList[i].pop_all();
    }
}

#define DEBUG_EVTQ_CNCLMT
#ifdef DEBUG_EVTQ_CNCLMT
#include <http/httpsession.h>
#endif


void EvtcbQue::logState(const char *s, evtcbnode_s *p, const char * extra)
{
    if (p)
    {
        uint32_t seq = p->m_pSession ? ((HttpSession *) p->m_pSession)->getSessSeq() : 0;
        int slot = seq % NUM_SESS_QUES;
        int sn = p->m_pSession ? ((HttpSession *) p->m_pSession)->getSn() : 0;
        LS_DBG_M("[T%d %s%s] Obj=%p Session=%p lParam=%ld pParam=%p cb=%p seq=%u slot=%d sn=%d\n",
                 ls_thr_seq(), s, extra, p, p->m_pSession, p->m_lParam, p->m_pParam,
                 p->m_callback, seq, slot, sn);
                 }
    else
        LS_ERROR("[T%d %s%s] Obj=NULL\n", ls_thr_seq(), s, extra);
}

void EvtcbQue::run(evtcbtail_t * session)
{
    evtcbnode_s *pObj;
    evtcbnode_s *pObjPrev;
    evtcbnode_s *rend;
    int empty;

    int slot = static_cast<HttpSession *>(session)->getSessSeq() % NUM_SESS_QUES;

    // LS_DBG_M("[T%d %s] locking event slot %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, slot);

    ls_mutex_lock(&m_sessLks[slot]);

    // LS_DBG_M("[T%d %s] locked event slot %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, slot);

    evtcbnode_s * tailnode = session->evtcb_tail;
    if (!tailnode)
    {
        LS_DBG_M("[T%d %s] slot %d tail NULL!\n", ls_thr_seq(), __PRETTY_FUNCTION__, slot);
        ls_mutex_unlock(&m_sessLks[slot]);
        // LS_DBG_M("[T%d %s] unlocked event slot %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, slot);
        return;
    }

    assert(tailnode != NULL && "Got NULL tailnode!");

    if (tailnode != session->evtcb_tail || session != tailnode->m_pSession)
    {
        LS_ERROR(
                 "[%s] MISMATCH, called with tailnode %p tailnode_session %p session %p session tail %p",
                 __func__, tailnode, tailnode->m_pSession, session, session->evtcb_tail);
        // failure handled in asserts below
    }

    assert(tailnode->m_pSession == session && "Mismatched tailnode and session");
    assert(session->evtcb_tail == tailnode && "Mismatched session and tailnode");

    empty = m_sessCallbackObjList[slot].empty();
    if (empty)
    {
        LS_ERROR("[%s] called for session %p slot %d tailnode %p but m_sessCallbackObjList empty!\n", __PRETTY_FUNCTION__, session, slot, tailnode);
        ls_mutex_unlock(&m_sessLks[slot]);
        return;
    }


    LS_DBG_M("[T%d %s] slot %d run session %p\n", ls_thr_seq(), __PRETTY_FUNCTION__, slot, session);
    pObj = tailnode;

    rend = m_sessCallbackObjList[slot].rend();

#ifdef DEBUG_EVTQ_CNCLMT
    int count = 0;
#endif
    TDLinkQueue<evtcbnode_s>  tmpQ;

    while(pObj != rend && pObj->m_pSession == session)
    {
        // pull one batch off the session call back list into the current local run queue
        pObjPrev = (evtcbnode_s *)pObj->prev();
        m_sessCallbackObjList[slot].remove(pObj);

        tmpQ.push_front(pObj);
#ifdef DEBUG_EVTQ_CNCLMT
        {
            evtcbnode_s * tailnode = pObj->m_pSession ? ls_atomic_fetch_or(&((LsiSession *)pObj->m_pSession)->evtcb_tail, 0) : NULL;
            evtcbtail_t * tailnode_session = tailnode ? ls_atomic_fetch_or(&tailnode->m_pSession, 0) : NULL;
            LS_DBG_M("[T%d %s] pushed pObj %p onto m_curRunQ, new size %d, pObj->m_pSession %p, tailnode %p tailnode session %p slot %d\n",
                     ls_thr_seq(), __PRETTY_FUNCTION__, pObj, m_curRunQ.size(), pObj->m_pSession, tailnode, tailnode_session, slot);
        }
#endif
        pObj = pObjPrev;
#ifdef DEBUG_EVTQ_CNCLMT
        count++;
#endif
    }

#ifdef DEBUG_EVTQ_CNCLMT
    LS_DBG_M("[T%d %s] appending %d evtq cbs for session %p tail %p sn %u, setting tail to NULL\n",
             ls_thr_seq(), __PRETTY_FUNCTION__,
             count, session, tailnode, ((HttpSession*)session)->getSn());
#endif

    appendCur(&tmpQ);

    session->evtcb_tail = NULL;

    ls_mutex_unlock(&m_sessLks[slot]);
    //LS_DBG_M("[T%d %s] unlocked event slot %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, slot);

    runCur();
}


void EvtcbQue::appendCur(TDLinkQueue<evtcbnode_s>  *tmpQ)
{
    LS_DBG_M("[T%d %s] append %d evtq to m_curRunQ\n", ls_thr_seq(), __PRETTY_FUNCTION__, tmpQ->size());
    if (tmpQ->size() <= 0)
    {
        return;
    }
    // LS_DBG_M("[T%d %s] locking runQ\n", ls_thr_seq(), __PRETTY_FUNCTION__);
    ls_mutex_lock(&m_runQlock);
    // LS_DBG_M("[T%d %s] locked runQ\n", ls_thr_seq(), __PRETTY_FUNCTION__);
    m_curRunQ.append(tmpQ);
    ls_mutex_unlock(&m_runQlock);
    // LS_DBG_M("[T%d %s] unlocked runQ\n", ls_thr_seq(), __PRETTY_FUNCTION__);
}


void EvtcbQue::runCur()
{
    evtcbnode_s *pObj = NULL;
    while( true ) {
        LS_DBG_M("[T%d %s] locking runQ\n", ls_thr_seq(), __PRETTY_FUNCTION__);
        ls_mutex_lock(&m_runQlock);
        LS_DBG_M("[T%d %s] locked runQ\n", ls_thr_seq(), __PRETTY_FUNCTION__);
#ifdef DEBUG_EVTQ_CNCLMT
        LS_DBG_M("[T%d %s] calling pop_front on m_curRunQ, pre pop size %d\n",
                 ls_thr_seq(), __PRETTY_FUNCTION__, m_curRunQ.size());
#endif
        pObj = m_curRunQ.pop_front();
        if (!pObj)
        {
            LS_DBG_M("[T%d %s] NULL pop_front on m_curRunQ, size after pop %d\n",
                     ls_thr_seq(), __PRETTY_FUNCTION__, m_curRunQ.size());
            ls_mutex_unlock(&m_runQlock);
            LS_DBG_M("[T%d %s] unlocked runQ\n", ls_thr_seq(), __PRETTY_FUNCTION__);
            break;
        }
#ifdef DEBUG_EVTQ_CNCLMT
        {
            LS_DBG_M("[T%d %s] popped pObj %p from m_curRunQ, new size %d, pObj->m_pSession %p\n",
                     ls_thr_seq(), __PRETTY_FUNCTION__, pObj, m_curRunQ.size(), pObj->m_pSession);
        }
#endif
        ls_mutex_unlock(&m_runQlock);
        LS_DBG_M("[T%d %s] unlocked runQ\n", ls_thr_seq(), __PRETTY_FUNCTION__);
        runOne(pObj);
    }
}

void EvtcbQue::run()
{
    evtcbnode_s *pObj;
    evtcbnode_s *pObjNext;
    evtcbnode_s *pLast;
    int empty;

    ls_atomic_fetch_and(&m_notified, 0); // zero out flag

    for (int slot = 0; slot < NUM_SESS_QUES; slot++)
    {
        int lk = ls_mutex_lock(&m_sessLks[slot]);
        assert(0==lk && "mutext lock FAILED!");

        empty = m_sessCallbackObjList[slot].empty();
        if (empty)
        {
            ls_mutex_unlock(&m_sessLks[slot]);
            continue;
        }

        // scope for start / end of current run
        pObj =  m_sessCallbackObjList[slot].begin();
        pLast =  m_sessCallbackObjList[slot].rbegin();
        // pLast =  m_sessCallbackObjList[slot].end();
        LS_DBG_M("[T%d %s] slot %d begin %p end %p\n", ls_thr_seq(), __PRETTY_FUNCTION__, slot, pObj, pLast);
        logState(__PRETTY_FUNCTION__, pObj, " starting");

        TDLinkQueue<evtcbnode_s>  tmpQ;
        int count = 0;
        while (1)
        {
            pObjNext = (evtcbnode_s *)pObj->next();

            evtcbtail_t * session = pObj->m_pSession;
            if (!session)
            {
                LS_WARN( "[T%d %s] called with pObj %p pObj->m_pSession NULL not allowed! SKIPPING\n",
                         ls_thr_seq(), __PRETTY_FUNCTION__, pObj);
                m_sessCallbackObjList[slot].remove(pObj);
                recycle(pObj);
                if (pObj == pLast)
                    break;
                pObj = pObjNext;
                continue;
            }

            evtcbnode_s * tailnode = session->evtcb_tail;

            // assert(tailnode && "session tailnode NULL!");
            if (!tailnode)
            {
                // if we found a session on the event cb q it SHOULD have a tailnode
                LS_ERROR("[T%d %s] RON_MARKER running evtq cb for slot %d session %p tail %p IS NULL! seq %u sn %u SKIPPING\n",
                         ls_thr_seq(), __PRETTY_FUNCTION__, slot, session, tailnode,
                         ((HttpSession*)session)->getSessSeq(),((HttpSession*)session)->getSn());
                m_sessCallbackObjList[slot].remove(pObj);
                recycle(pObj);
                if (pObj == pLast)
                    break;
                pObj = pObjNext;
                continue;
            }

            if (tailnode->m_pSession != session)
            {
                LS_WARN( "[T%d %s] called with pObj %p pObj->m_pSession %p tailnode %p tailnode_session %p MISMATCH not allowed! SKIPPING\n",
                         ls_thr_seq(), __PRETTY_FUNCTION__, pObj, pObj->m_pSession, tailnode, tailnode->m_pSession);
                m_sessCallbackObjList[slot].remove(pObj);
                recycle(pObj);
                if (pObj == pLast)
                    break;
                pObj = pObjNext;
                continue;;
            }

            int expectSlot = static_cast<HttpSession *>(session)->getSessSeq() % NUM_SESS_QUES;
            if (expectSlot != slot)
            {
                LS_ERROR("[T%d %s] RON_MARKER running evtq cb for slot %d session %p expectSlot %d tail %p seq %u sn %u SLOT WRONG SKIPPING\n",
                     ls_thr_seq(), __PRETTY_FUNCTION__, slot, session, expectSlot, tailnode,
                     ((HttpSession*)session)->getSessSeq(),((HttpSession*)session)->getSn());
                m_sessCallbackObjList[slot].remove(pObj);
                recycle(pObj);
                if (pObj == pLast)
                    break;
                pObj = pObjNext;
                continue;
            }

#ifdef DEBUG_EVTQ_CNCLMT
            LS_DBG_M("[T%d %s] running evtq cb for slot %d session %p tail %p sn %u\n",
                     ls_thr_seq(), __PRETTY_FUNCTION__, slot, session, tailnode, ((HttpSession*)session)->getSn());
#endif

            // is this a tail node for the session?
            // should be locked!
            if (tailnode == pObj)
            {
                LS_DBG_M("[T%d %s] Tail was %p setting to NULL\n",
                         ls_thr_seq(), __PRETTY_FUNCTION__, session->evtcb_tail);
                session->evtcb_tail = NULL;
            }

            m_sessCallbackObjList[slot].remove(pObj);
            tmpQ.append(pObj);
            count++;
            if (pObj == pLast)
                break;
            pObj = pObjNext;
        }

        ls_mutex_unlock(&m_sessLks[slot]);
        LS_DBG_M("[T%d %s] unlocked event slot %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, slot);

#ifdef DEBUG_EVTQ_CNCLMT
    LS_DBG_M("[T%d %s] appending %d evtq cbs for slot %d\n",
             ls_thr_seq(), __PRETTY_FUNCTION__, count, slot);
#endif

        appendCur(&tmpQ);

        runCur();
    }
}


void EvtcbQue::runOne(evtcbnode_s *pObj)
{
    logState(__PRETTY_FUNCTION__, pObj);

    if (pObj->m_pSession
        && pObj->m_pSession->back_ref_ptr == &pObj->m_pSession)
        pObj->m_pSession->back_ref_ptr = NULL;

    if (pObj->m_callback)
        pObj->m_callback(pObj->m_pSession, pObj->m_lParam, pObj->m_pParam);

    recycle(pObj);
}


evtcbnode_s * EvtcbQue::getNodeObj(evtcb_pf cb, const evtcbtail_t *session,
                                   long lParam, void *pParam)
{
    // int slot = static_cast<const HttpSession *>(session)->getSessSeq() % NUM_SESS_QUES;
    ls_mutex_lock(&m_nodePoolLock);
    evtcbnode_s *pObj = s_pCbnodePool->get();
    ls_mutex_unlock(&m_nodePoolLock);
    // logState(__PRETTY_FUNCTION__, pObj);

    if (pObj)
    {
        pObj->m_callback = cb;
        pObj->m_pSession = (evtcbtail_t *) session; // violate LSIAPI const - internal code
        pObj->m_lParam = lParam;
        pObj->m_pParam = pParam;
        LS_DBG_M("[T%d %s] returning pObj %p session %p tailnode %p\n",
                 ls_thr_seq(), __PRETTY_FUNCTION__, pObj, session, session->evtcb_tail);
    }
    return pObj;
}


void EvtcbQue::schedule(evtcbnode_s *pObj, bool nowait)
{
    evtcbtail_t *session = pObj->m_pSession;
    logState(__PRETTY_FUNCTION__, pObj, nowait ? " nowait" : NULL);

    if (!session)
    {
        LS_WARN( "[T%d %s] called with pObj %p pObj->m_pSession NULL not allowed!\n",
                 ls_thr_seq(), __PRETTY_FUNCTION__, pObj);
        return;
    }

    int slot = static_cast<HttpSession *>(session)->getSessSeq() % NUM_SESS_QUES;

    LS_DBG_M("[T%d %s] locking event slot %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, slot);
    int lk = ls_mutex_lock(&m_sessLks[slot]);
    // LS_DBG_M("[T%d %s] RON_MARKER locked event slot %d ret %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, slot, lk);
    assert(0==lk && "mutext lock FAILED!");

    // grab the tail
    evtcbnode_s * tailnode = session->evtcb_tail;

#ifdef DEBUG_EVTQ_CNCLMT
    LS_DBG_M("[T%d %s] schedule cb for session %p tail %p seq %u slot %d sn %u\n",
             ls_thr_seq(), __PRETTY_FUNCTION__, session, tailnode,
             ((HttpSession*)session)->getSessSeq(), slot, ((HttpSession*)session)->getSn());
#endif


    if (tailnode)
    {
        if (tailnode->m_pSession != session)
        {
            LS_WARN( "[T%d %s] called with pObj %p pObj->m_pSession %p tailnode %p tailnode_session %p MISMATCH not allowed!\n",
                     ls_thr_seq(), __PRETTY_FUNCTION__, pObj, pObj->m_pSession, tailnode, tailnode->m_pSession);
            return;
        }

        assert(tailnode->m_pSession == session && "Mismatched tailnode and session");

        m_sessCallbackObjList[slot].insert_after(tailnode, pObj);

        LS_DBG_M("[T%d %s] %p inserted after %p\n", ls_thr_seq(), __PRETTY_FUNCTION__, pObj, tailnode);
    }
    else {
        m_sessCallbackObjList[slot].append(pObj);

        LS_DBG_M("[T%d %s] %p appended\n", ls_thr_seq(), __PRETTY_FUNCTION__, pObj);
    }


    int chkslot = (static_cast<HttpSession *>(session)->getSessSeq() % NUM_SESS_QUES);
    if ( chkslot != slot )
    {
        LS_ERROR( "[T%d %s] slot has changed! was %d now %d ???\n",
                 ls_thr_seq(), __PRETTY_FUNCTION__, slot, chkslot);
        assert(chkslot == slot);
    }

    // should be locked!
    session->evtcb_tail = pObj;

    LS_DBG_M("[T%d %s] Tail for session %p slot %d was %p set to %p\n",
             ls_thr_seq(), __PRETTY_FUNCTION__, session, slot, tailnode, session->evtcb_tail);

    ls_mutex_unlock(&m_sessLks[slot]);


    if (nowait)
    {
        if (ls_atomic_fetch_or(&m_notified, 1))
            nowait = 0; // was already set
        /*
        else
            m_notified = 1; // was not set
            */
    }

    if (nowait)
        m_pNotifier->notify();
}


evtcbnode_s *EvtcbQue::schedule(evtcb_pf cb, const evtcbtail_t *session,
                          long lParam, void *pParam, bool nowait)
{
    evtcbnode_s *pObj = getNodeObj(cb, session, lParam, pParam);

#ifdef DEBUG_EVTQ_CNCLMT
    LS_DBG_M("[T%d %s] sched evtq cb for session %p sn %u\n",
             ls_thr_seq(), __PRETTY_FUNCTION__, pObj->m_pSession, ((HttpSession*)pObj->m_pSession)->getSn());
#endif

    if (pObj)
        schedule(pObj, nowait);

    return pObj;
}


void EvtcbQue::recycle(evtcbnode_s *pObj)
{
    logState(__PRETTY_FUNCTION__, pObj);

    //int slot = static_cast<HttpSession *>(pObj->m_pSession)->getSessSeq() % NUM_SESS_QUES;

    ls_mutex_lock(&m_nodePoolLock);
    memset(pObj, 0, sizeof(evtcbnode_s));
    s_pCbnodePool->recycle(pObj);
    ls_mutex_unlock(&m_nodePoolLock);
}


int EvtcbQue::removeSessionCb(evtcbtail_t * pSession)
{
    uint32_t seq = static_cast<HttpSession *>(pSession)->getSessSeq();
    int slot = seq % NUM_SESS_QUES;

    ls_mutex_lock(&m_sessLks[slot]);

    evtcbnode_s * tailnode = pSession->evtcb_tail;

    if (!tailnode) {
        LS_DBG_M("[T%d %s] session %p seq %u slot %d tailnode NULL!\n",
                 ls_thr_seq(), __PRETTY_FUNCTION__, pSession, seq, slot);
        ls_mutex_unlock(&m_sessLks[slot]);
        return -1;
    }

    assert(tailnode->m_pSession == pSession && "Mismatched tailnode and session");
    assert(pSession->evtcb_tail == tailnode && "Mismatched session and tailnode");

    evtcbnode_s *pObj = NULL;
    evtcbnode_s *pObjPrev = NULL;

    evtcbnode_s *pRunQEnd = NULL;
    evtcbnode_s *pCBListEnd = NULL;

#ifdef DEBUG_EVTQ_CNCLMT
    int count = 0;
#endif
    evtcbtail_t * session = tailnode->m_pSession;

    // the nodes may have been taken off the m_callbackObjList
    // and placed on run Q for processing, need to know when to stop.
    // traverse chain and stop if reached either rend()

    TDLinkQueue<evtcbnode_s>  * pObjList = &m_sessCallbackObjList[slot];

    ls_mutex_lock(&m_runQlock);
#ifdef DEBUG_EVTQ_CNCLMT
        LS_DBG_M("[T%d %s] checking m_curRunQ size %d\n",
                 ls_thr_seq(), __PRETTY_FUNCTION__, m_curRunQ.size());
#endif
    if (0 != m_curRunQ.size())
    {
        pRunQEnd = m_curRunQ.rend();
    }
    if (pObjList->size() != 0)
    {
        pCBListEnd = m_sessCallbackObjList[slot].rend();
    }

    if (pRunQEnd || pCBListEnd)
    {
        pObj = tailnode;
        while (pObj && pObj != pRunQEnd && pObj != pCBListEnd)
        {
#ifdef DEBUG_EVTQ_CNCLMT
            count++;
#endif
            logState(__PRETTY_FUNCTION__, pObj, "][header state");
            pObjPrev = (evtcbnode_s *)pObj->prev();
            if (pObj->m_pSession == session)
            {
                pObj->m_pSession = NULL;
                pObj->m_callback = NULL;
            }
            else
                break;
            pObj = pObjPrev;
        }
    }

#ifdef DEBUG_EVTQ_CNCLMT
    LS_DBG_M("[T%d %s] removed %d evtq cbs for session %p tail %p sn %u set Tail to NULL\n",
             ls_thr_seq(), __PRETTY_FUNCTION__, count, session, tailnode, ((HttpSession*)session)->getSn());
#endif

    session->evtcb_tail = NULL; // setEvtcbTail(session, NULL);

    ls_mutex_unlock(&m_runQlock);
    ls_mutex_unlock(&m_sessLks[slot]);
    return 0;
}

evtcbtail_t **EvtcbQue::getSessionRefPtr(evtcbnode_s *nodeObj)
{
    if (!nodeObj)
        return NULL;
    logState(__PRETTY_FUNCTION__, nodeObj);
    return &nodeObj->m_pSession;
}


void EvtcbQue::resetEvtcbTail(evtcbtail_t * session)
{
    int slot = static_cast<HttpSession *>(session)->getSessSeq() % NUM_SESS_QUES;
    ls_mutex_lock(&m_sessLks[slot]);
    LS_DBG_M("[T%d %s] session %p tail %p sn %u set Tail to NULL\n",
             ls_thr_seq(), __PRETTY_FUNCTION__, session, session->evtcb_tail, ((HttpSession*)session)->getSn());
    session->evtcb_tail = NULL; // setEvtcbTail(session, NULL);
    ls_mutex_unlock(&m_sessLks[slot]);
}
