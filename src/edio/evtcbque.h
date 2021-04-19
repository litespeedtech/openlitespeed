/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2021  LiteSpeed Technologies, Inc.                 *
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

#include <lsdef.h>
#include <lsr/ls_evtcb.h>
#include <util/dlinkqueue.h>
#include <util/tsingleton.h>
#include <edio/eventnotifier.h>

#define EQF_RUNNING         (1<<0)

struct evtcbnode_s;

class EvtcbQueNotifier : public EventNotifier
{
public:
    virtual int onNotified(int count){return 0;};
};

class EvtcbQue : public TSingleton<EvtcbQue>
{
    friend class TSingleton<EvtcbQue>;

    EvtcbQue();
    ~EvtcbQue();

    DLinkQueue  m_callbackObjList;

    void runOne(evtcbnode_s *pObj);

    evtcbnode_s *extractSession(evtcbnode_s *pObj);
    void runNodes(evtcbnode_s *pObj);

    void setFlag(uint32_t f)        {   m_iFlag |= f;               }
    void clearFlag(uint32_t f)      {   m_iFlag &= ~f;              }
    uint32_t getFlag() const        {   return m_iFlag;             }
    uint32_t getFlag(int mask)      {   return m_iFlag & mask;      }

    static void logState(const char *s, evtcbnode_s *p);

public:
    void run(evtcbhead_t *session);
    void run();

    int initNotifier();

    void recycle(evtcbnode_s *pObj);

    evtcbnode_s * getNodeObj(evtcb_pf cb, evtcbhead_t *session,
                             long lParam, void *pParam);

    void schedule(evtcbnode_s *pObj);
    evtcbnode_s *schedule(evtcb_pf cb, evtcbhead_t *session,
                          long lParam, void *pParam);
    int schedule_once(evtcbnode_s *pObj);
    evtcbnode_s *schedule_once(evtcb_pf cb, evtcbhead_t *session,
                          long lParam, void *pParam);

    void schedule_nowait(evtcbnode_s *pObj)
    {
        schedule(pObj);
        notify();
    }

    evtcbnode_s * schedule_nowait(evtcb_pf cb, evtcbhead_t *session,
                                  long lParam, void *pParam)
    {
        evtcbnode_s *pRet;
        if ((pRet = schedule(cb, session, lParam, pParam)) != NULL)
            notify();
        return pRet;
    }

    int schedule_once_nowait(evtcbnode_s *pObj)
    {
        int ret;
        if ((ret = schedule_once(pObj)) == 0)
            notify();
        return ret;
    }

    evtcbnode_s * schedule_once_nowait(evtcb_pf cb, evtcbhead_t *session,
                                       long lParam, void *pParam)
    {
        evtcbnode_s *pRet;
        if ((pRet = schedule_once(cb, session, lParam, pParam)) != NULL)
            notify();
        return pRet;
    }

    evtcbnode_s *schedule(evtcb_pf cb, evtcbhead_t *session,
                          long lParam, void *pParam, int no_wait)
    {
        evtcbnode_s *pRet;
        if ((pRet = schedule_once(cb, session, lParam, pParam)) != NULL)
        {
            if (no_wait)
                notify();
        }
        return pRet;
    }

    void schedule(evtcbnode_s *pObj, int no_wait)
    {
        schedule(pObj);
        if (no_wait)
            notify();
    }


    void notify()
    {   m_pNotifier->notify();      }

    void removeSessionCb(evtcbhead_t *session);

    static evtcbhead_t **getSessionRefPtr(evtcbnode_s *nodeObj);

    int size() const                {   return m_callbackObjList.size();    }

    void sessionDebug(evtcbhead_t *session);

    LS_NO_COPY_ASSIGN(EvtcbQue);

private:
    int32_t             m_lock;
    uint32_t            m_iFlag;
    EvtcbQueNotifier   *m_pNotifier;

};

LS_SINGLETON_DECL(EvtcbQue);

#endif  //CALLBACKQUEUE_H
