/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
#ifndef SIGEVENTDISPATCHER_H
#define SIGEVENTDISPATCHER_H

#include <edio/eventreactor.h>
#include <util/tsingleton.h>

#include <signal.h>

#if !(defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__))
#define HS_AIO (SIGRTMIN + 4)
#define LS_HAS_RTSIG
#endif

typedef void (*sigevthdlr_t)(int, void *);

#define SIG_EVT_HANDLER  ((sigevthdlr_t)-1)

struct SigEvtHdlrInfo;

class SigEventDispatcher : public EventReactor
                         , public TSingleton<SigEventDispatcher>
{
    friend class TSingleton<SigEventDispatcher>;
#if defined(LS_HAS_RTSIG)
private:
    short           m_rtsigmin;
    short           m_rtsigcnt;
    short           m_rtsignext;
    short           m_flag;
    long            m_mergeable;
    sigset_t        m_sigset;
    SigEvtHdlrInfo *m_rtsig_hdlrs;

    SigEventDispatcher();

    SigEventDispatcher(const SigEventDispatcher &other);
    ~SigEventDispatcher()
    {
        if (m_rtsig_hdlrs)
            free(m_rtsig_hdlrs);
    }
    SigEventDispatcher &operator=(const SigEventDispatcher *rhs);
    int nextRtsig();

public:
    int processSigEvent();

    int registerRtsig(sigevthdlr_t hdlr, void *param, bool merge = false);
    int beginWatch();
#endif //defined(LS_HAS_RTSIG)

    virtual int handleEvents(short event);

};

LS_SINGLETON_DECL(SigEventDispatcher);

#endif //SIGEVENTDISPATCHER_H
