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
#ifndef CALLBACKQUEUE_H
#define CALLBACKQUEUE_H

#include <lsr/ls_evtcb.h>
#include <util/dlinkqueue.h>
#include <util/tsingleton.h>

struct evtcbnode_s;

class EvtcbQue : public TSingleton<EvtcbQue>
{
    friend class TSingleton<EvtcbQue>;

    EvtcbQue();
    ~EvtcbQue();

    DLinkQueue  m_callbackObjList;

    inline void logState(const char *s, evtcbnode_s *p);
    void removeObj(evtcbnode_s *pObj);
    void recycle(evtcbnode_s *pObj);

public:
    void run(evtcbhead_t *session);
    evtcbnode_s *schedule(evtcb_pf cb, evtcbhead_t *session,
                          long lParam, void *pParam);
    void removeSessionCb(evtcbhead_t *session);

    LS_NO_COPY_ASSIGN(EvtcbQue);
};
#endif  //CALLBACKQUEUE_H
