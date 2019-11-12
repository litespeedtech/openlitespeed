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
#ifndef CALLBACKQUEUE_H
#define CALLBACKQUEUE_H

#include <lsr/ls_evtcb.h>
#include <lsr/ls_lock.h>
#include <util/dlinkqueue.h>
#include <util/tsingleton.h>
#include <edio/eventnotifier.h>

struct evtcbnode_s;

class EvtcbQueNotifier : public EventNotifier
{
public:
    virtual int onNotified(int count){return 0;};
};

#define NUM_SESS_QUES 64

class EvtcbQue : public TSingleton<EvtcbQue>
{
private:
    // These locks protects the session queues
    // (except the head - which is static)
    ls_mutex_t m_sessLks[NUM_SESS_QUES];
    ls_mutex_t m_runQlock;
    ls_mutex_t m_nodePoolLock;

    ls_atom_32_t      m_notified;
    EvtcbQueNotifier *m_pNotifier;

    friend class TSingleton<EvtcbQue>;

    EvtcbQue();
    ~EvtcbQue();

    TDLinkQueue<evtcbnode_s>  m_sessCallbackObjList[NUM_SESS_QUES];
    TDLinkQueue<evtcbnode_s>  m_curRunQ; // have to keep as member rather than local/auto
                                         // to support removeSessionCb

    static void logState(const char *s, evtcbnode_s *p, const char * extra = NULL);
    void runOne(evtcbnode_s *pObj);
    void appendCur(TDLinkQueue<evtcbnode_s>  *tmpQ);
    void runCur();


public:
    void run(evtcbtail_t * pSession);
    void run();
    void recycle(evtcbnode_s *pObj);

    evtcbnode_s * getNodeObj(evtcb_pf cb, const evtcbtail_t *session,
                             long lParam, void *pParam);

    void schedule(evtcbnode_s *pObj, bool nowait = true);
    evtcbnode_s *schedule(evtcb_pf cb, const evtcbtail_t *session,
                          long lParam, void *pParam, bool nowait);
    int removeSessionCb(evtcbtail_t * pSession);

    static evtcbtail_t **getSessionRefPtr(evtcbnode_s *nodeObj);

    void resetEvtcbTail(evtcbtail_t * session); // should really have a lock in session

    LS_NO_COPY_ASSIGN(EvtcbQue);
};

LS_SINGLETON_DECL(EvtcbQue);

#endif  //CALLBACKQUEUE_H
