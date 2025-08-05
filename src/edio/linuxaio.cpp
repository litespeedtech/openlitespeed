/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#include "edio/linuxaio.h"
#include "edio/lslinuxaioreq.h"
#include "edio/multiplexer.h"
#include "log4cxx/logger.h"

LS_SINGLETON(LinuxAio);

LinuxAio::~LinuxAio()
{
    LS_DBG_M("LinuxAio destructor");
    if (getPollfd()->fd != -1)
    {
        Multiplexer *pMpl = getMultiplexer();
        if (pMpl)
            pMpl->remove(this);
    }
    if (m_context)
        io_destroy(m_context);
    m_context = 0;
    if (m_eventfd > 0)
        close(m_eventfd);
    LS_DBG_M("LinuxAio destructor exiting");
}


int LinuxAio::load()
{
    int ret = io_setup(s_check_event_count, &m_context);
    if (ret == -1)
    {
        int err = errno;
        LS_ERROR("LinuxAio Error initialzing for AIO support: %s",
                 strerror(err));
        errno = err;
        return -1;
    }
    LS_DBG_M("LinuxAio Context is now: %p", (void *)m_context);
    if (m_eventfd == -1)
    {
        LS_DBG_M("LinuxAio creating eventfd");
        if ((m_eventfd = eventfd(0, 0)) < 0)
        {
            int err = errno;
            LS_NOTICE("LinuxAio Unable to setup eventfd: %s", strerror(err));
            errno = err;
            return -1;
        }
        setfd(m_eventfd);
        LS_DBG_M("LinuxAio register eventfd");
    }
    setPollfd();
    Multiplexer *pMpl = getMultiplexer();
    pMpl->add(this, POLLIN);
    return 0;
}


int LinuxAio::read_eventfd(int &pending)
{
    int ret = 0;
    if (m_eventfd != -1)
    {
        eventfd_t v = 0;
        LS_DBG_M("LinuxAio read_eventfd");
        ret = eventfd_read(m_eventfd, &v);
        if (ret)
        {
            int err = errno;
            LS_NOTICE("LinuxAio read_eventfd: %s", strerror(err));
            errno = err;
            return -1;
        }
        pending = (int)v;
        LS_DBG_M("LinuxAio read_eventfd: %d events", pending);
    }
    return ret;
}


void LinuxAio::close_eventfd()
{
    if (m_eventfd != -1)
        close(m_eventfd);
    m_eventfd = -1;
}


int LinuxAio::check_pending()
{
    int pending = 0, wait = 0, max_events = s_check_event_count;
    struct io_event evts[max_events];
    struct timespec ts = { 0, 0 };
    int ret = read_eventfd(pending);
    if (ret < 0)
        return ret;
    int num_events = io_getevents(m_context, wait, max_events, evts, &ts);
    if (num_events < 0)
    {
        LS_NOTICE("Error in io_getevents in reading a file: %s",
                  strerror(-num_events));
        return -1;
    }
    for (int i = 0; i < num_events; ++i)
    {
        LsLinuxAioReq *lsLinuxAioReq = (LsLinuxAioReq *)evts[i].data;
        lsLinuxAioReq->setRet((int)evts[i].res);
        if (lsLinuxAioReq->getCancel())
        {
            LS_DBG(lsLinuxAioReq->getLogSession(),
                   "Canceled read fd: %d at %ld ret %d",
                   lsLinuxAioReq->getfd(), lsLinuxAioReq->getPos(),
                   lsLinuxAioReq->getRet());
            if (lsLinuxAioReq->getAsyncState() == ASYNC_STATE_CANCELING)
            {
                LS_DBG(lsLinuxAioReq->getLogSession(),
                   "canceling Linux AIO request, discard result, delete");
                delete lsLinuxAioReq;
                continue;
            }
            lsLinuxAioReq->setAsyncState(ASYNC_STATE_IDLE);
        }
        else
        {
            if (lsLinuxAioReq->getRet() < 0 && -lsLinuxAioReq->getRet() == EAGAIN)
            {
                LS_DBG_M(lsLinuxAioReq->getLogSession(),
                        "LinuxAio RC says %s still pending",
                        lsLinuxAioReq->str_op());
                continue;
            }
            lsLinuxAioReq->setAsyncState(ASYNC_STATE_READ);
            ret = lsLinuxAioReq->io_complete(POLLIN);
            if (ret < 0 && !lsLinuxAioReq->getCancel())
            {
                LS_DBG(lsLinuxAioReq->getLogSession(),
                       "Linux Aio setError for process failure");
                lsLinuxAioReq->setError();
                continue;
            }
        }
    }
    return 0;
}


int LinuxAio::handleEvents(short int event)
{
    LS_DBG_M("LinuxAio handleEvents(%d)", event);
    if (event & POLLIN)
    {
        LS_DBG_M("LinuxAio handleEvent");
        return check_pending();
    }
    return 0;
}
