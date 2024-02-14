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

#include <edio/eventhandler.h>
#include <edio/sigeventdispatcher.h>


#if defined(LS_HAS_RTSIG)

#include <edio/multiplexer.h>
#include <edio/multiplexerfactory.h>
#include <log4cxx/logger.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#include <sys/signalfd.h>

#endif //defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)


struct SigEvtHdlrInfo
{
    sigevthdlr_t  handler;
    void         *param;
};


#define SED_FLAG_SIGNALFD   1
#define SED_FLAG_INITED     2
#define SED_FLAG_INUSE      4

int SigEventDispatcher::processSigEvent()
{
#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    if ((m_flag & SED_FLAG_SIGNALFD) || !(m_flag & SED_FLAG_INUSE))
        return LS_OK;
#endif //defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#ifdef LS_HAS_RTSIG
    struct timespec timeout;
    siginfo_t si;

    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;


    while (sigtimedwait(&m_sigset, &si, &timeout) > 0)
    {
        int idx = si.si_signo - m_rtsigmin;
        if (idx >=0 && idx < m_rtsigcnt && m_rtsig_hdlrs)
        {
            if (m_rtsig_hdlrs[idx].handler == SIG_EVT_HANDLER)
                ((EventHandler *)si.si_value.sival_ptr)->onEvent();
            else if (m_rtsig_hdlrs[idx].handler != NULL)
                (*m_rtsig_hdlrs[idx].handler)(si.si_signo, m_rtsig_hdlrs[idx].param);
        }
    }
#endif
    return LS_OK;
}


LS_SINGLETON(SigEventDispatcher);


SigEventDispatcher::SigEventDispatcher()
{
    m_rtsigmin = SIGRTMIN;
    m_rtsigcnt = SIGRTMAX - SIGRTMIN;
    m_rtsignext = m_rtsigmin + 4;
    sigemptyset(&m_sigset);
    m_flag = 0;
    m_mergeable = 0;
    if (m_rtsigcnt > (int) sizeof(m_mergeable) * 8)
        m_rtsigcnt = (int) sizeof(m_mergeable) * 8;
    m_rtsig_hdlrs = (SigEvtHdlrInfo *)malloc(m_rtsigcnt
                        * sizeof(SigEvtHdlrInfo));
    if (m_rtsig_hdlrs)
        memset(m_rtsig_hdlrs, 0, sizeof(SigEvtHdlrInfo) * m_rtsigcnt);
}


int SigEventDispatcher::beginWatch()
{
    if (!(m_flag & SED_FLAG_INUSE))
        return 0;
#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    int fd = signalfd(-1, &m_sigset, 0);
    if (fd != -1)
    {
        fcntl(fd, F_SETFD, FD_CLOEXEC);
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        setfd(fd);
        MultiplexerFactory::getMultiplexer()->add(this, POLLIN);
        m_flag |= SED_FLAG_SIGNALFD;
    }
#endif
    m_flag |= SED_FLAG_INITED;
    return 0;
}


int SigEventDispatcher::nextRtsig()
{
    int signo;
    while( m_rtsignext < m_rtsigmin + m_rtsigcnt)
    {
        signo = m_rtsignext++;
        if (!sigismember(&m_sigset, signo))
            return signo;
    }
    return -1;
}


int SigEventDispatcher::registerRtsig(sigevthdlr_t hdlr, void *param, bool merge)
{
    int signo = nextRtsig();
    if (signo == -1)
        return signo;

    if (sigaddset(&m_sigset, signo) != 0
        || sigprocmask(SIG_BLOCK, &m_sigset, NULL) != 0)
        return -1;
    int idx = signo - m_rtsigmin;
    if (idx >= 0 && idx < m_rtsigcnt && m_rtsig_hdlrs)
    {
        m_rtsig_hdlrs[idx].handler = hdlr;
        m_rtsig_hdlrs[idx].param   = param;
        if (merge)
            m_mergeable |= 1L << idx;
    }
    m_flag |= SED_FLAG_INUSE;
    return signo;
}


int SigEventDispatcher::handleEvents(short event)
{
#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    int i, readCount = 0;
    signalfd_siginfo si[20];
    long handled_set;
    if (!(event & POLLIN))
        return LS_OK;
    do
    {
        if ((readCount = read(getfd(), &si, sizeof(si))) < 0)
        {
            if (errno == EAGAIN)
                break;
            return LS_FAIL;
        }
        else if (readCount == 0)
            return LS_OK;
        readCount /= sizeof(signalfd_siginfo);
        for (i = 0, handled_set = 0; i < readCount; ++i)
        {
            int idx = si[i].ssi_signo - m_rtsigmin;
            if (idx >=0 && idx < m_rtsigcnt && m_rtsig_hdlrs &&
                !(handled_set & (1L << idx)))
            {
                handled_set |= (1L << idx) & m_mergeable;
                if (m_rtsig_hdlrs[idx].handler == SIG_EVT_HANDLER)
                    ((EventHandler *)si[i].ssi_ptr)->onEvent();
                else if (m_rtsig_hdlrs[idx].handler != NULL)
                    (*m_rtsig_hdlrs[idx].handler)(si[i].ssi_signo,
                                          m_rtsig_hdlrs[idx].param);
            }
        }
    }
    while (readCount == (int) (sizeof(si) / sizeof(si[0])));
#endif
    return LS_OK;
}

#endif //defined(LS_HAS_RTSIG)
