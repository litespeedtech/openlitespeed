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

#include "edio/lsposixaioreq.h"
#include <edio/aiooutputstream.h>
#include "edio/sigeventdispatcher.h"
#include <log4cxx/logger.h>

#ifdef LS_AIO_USE_KQ
#include <edio/multiplexer.h>
#include <edio/kqueuer.h>
#include <sys/param.h>
#include <sys/linker.h>
#endif

#include <errno.h>
#include <poll.h>

int LsPosixAioReq::s_rtsigNoPosixAio = -1;

LsPosixAioReq::LsPosixAioReq(AioReqEventHandler *aioReqEventHandler, int fd)
    : LsAioReq(aioReqEventHandler, fd)
{
    memset(&m_aiocb, 0, sizeof(struct aiocb));
    m_aiocb.aio_reqprio = 0;
#if defined(LS_AIO_USE_KQ)
    m_aiocb.aio_sigevent.sigev_notify = SIGEV_KEVENT;
    m_aiocb.aio_sigevent.sigev_notify_kevent_flags = EV_ONESHOT;
#elif defined(LS_AIO_USE_SIGNAL)
    m_aiocb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
#endif
}


LsPosixAioReq::~LsPosixAioReq()
{
}


int LsPosixAioReq::load()
{
#if defined(LS_AIO_USE_SIGNAL)
    s_rtsigNoPosixAio = SigEventDispatcher::getInstance().registerRtsig(SIG_EVT_HANDLER, NULL, 0);
    if (s_rtsigNoPosixAio == -1)
    {
        LS_DBG("PosixAio::load RegisterRtsig returned -1");
        return -1;
    }
    SigEventDispatcher::getInstance().beginWatch();
#endif
    return 0;
}


void LsPosixAioReq::setcb(void *buf, size_t nbytes, int offset)
{
    m_aiocb.aio_fildes = getfd();
    m_aiocb.aio_buf = (volatile void *)buf;
    m_aiocb.aio_nbytes = nbytes;
    m_aiocb.aio_offset = offset;
    m_aiocb.aio_sigevent.sigev_value.sival_ptr = this;
#if defined(LS_AIO_USE_KQ)
    m_aiocb.aio_sigevent.sigev_notify_kqueue = KQueuer::getFdKQ();
#elif defined(LS_AIO_USE_SIGNAL)
    assert( s_rtsigNoPosixAio != -1);
    m_aiocb.aio_sigevent.sigev_signo = s_rtsigNoPosixAio;
#endif
}


int LsPosixAioReq::postRead(struct iovec *iov, int iovcnt, off_t pos)
{
    LS_DBG(getLogSession(), "LsPosixAioReq::postRead at %ld", pos);
    int ret;
    setPos(pos);
    setcb(iov->iov_base, iov->iov_len, pos);
    ret = aio_read(&m_aiocb);
    if (ret < 0)
    {
        int err = errno;
        LS_ERROR(getLogSession(),
                 "PosixAio Error submitting read: %s (ret: %d)",
                 strerror(err), ret);
        errno = err;
        setError();
        return LS_FAIL;
    }
    setAsyncState(ASYNC_STATE_POSTED);
    return 0;
}


int LsPosixAioReq::getReadResult(struct iovec **iov)
{
    LS_DBG(getLogSession(), "LsPosixAioReq::getReadResult");
    int err = aio_error(&m_aiocb);
    if (err)
    {
        if (err == EINPROGRESS)
        {
            LS_DBG_M(getLogSession(), "PosixAio RC says still pending");
            errno = EAGAIN;
            return -1;
        }
        if (err == ECANCELED)
        {
            if (getCancel())
            {
                LS_DBG_M(getLogSession(), "PosixAio Cancel completed");
                setAsyncState(ASYNC_STATE_IDLE);
                return 0;
            }
        }
        LS_DBG_M("PosixAio returned %d", err);
        if (!getCancel())
        {
            LS_DBG(getLogSession(), "Posix Aio setError for process failure");
            setError();
            errno = err;
            return -1;
        }
        LS_DBG_M(getLogSession(), "PosixAio Cancel with error %d", err);
        setAsyncState(ASYNC_STATE_IDLE);
        return err;
    }
    int ret = aio_return(&m_aiocb);
    if (ret == -1)
        ret = -errno;
    setRet(ret);
    if (ret >= 0)
    {
        if (getCancel())
        {
            LS_DBG(getLogSession(), "Canceled read fd: %d at %ld ret %d",
                   getfd(), getPos(), getRet());
            setAsyncState(ASYNC_STATE_IDLE);
            return 0;
        }
        else
        {
            LS_DBG(getLogSession(), "Posix Aio read fd: %d at %ld ret %d",
                   getfd(), getPos(), getRet());
            setAsyncState(ASYNC_STATE_READ);
            if (iov)
                *iov = getIovec();
            return ret;
        }
    }
    setAsyncState(ASYNC_STATE_IDLE);
    if (-getRet() == EAGAIN)
    {
        LS_DBG_M(getLogSession(), "PosixAio RC says reaaad still pending");
        errno = EAGAIN;
        return -1;
    }
    if (!getCancel())
    {
        LS_DBG(getLogSession(), "PosixAio setError for process failure");
        setError();
        errno = -getRet();
        return -1;
    }
    LS_NOTICE(getLogSession(), "ERROR in Posix Aio read: %s (%d)",
              strerror(-getRet()), -getRet());
    errno = -getRet();
    setError();
    return -1;
}


int LsPosixAioReq::doCancel()
{
    if (getAsyncState() != ASYNC_STATE_POSTED)
    {
        LS_DBG(getLogSession(), "PosixAio::doCancel, no pending read\n");
        return 0;
    }
    LS_DBG_M(getLogSession(), "Aio Attempt to cancel %p at %ld", getIovec(),
             getPos());
    int ret = aio_cancel(getfd(), &m_aiocb);
    if (ret < 0)
        LS_DBG_M(getLogSession(), "      Cancel failed: %s (%d)",
                 strerror(errno), errno);
    else
    {
        setAsyncState(ASYNC_STATE_CANCELED);
        LS_DBG_M(getLogSession(), "Aio Cancel succeeded, at %ld, ret %d",
                 getPos(), ret);
    }
    return 0;
}


#if defined(LS_AIO_USE_KQ)
int LsPosixAioReq::onEvent()
{
    return onAioEvent();
}
int LsPosixAioReq::onAioEvent()
#elif defined(LS_AIO_USE_SIGNAL)
int LsPosixAioReq::onEvent()
#endif
{
    LS_DBG(getLogSession(), "LsPosixAioReq::onEvent: %p", this);
    io_complete(POLLIN);
    return 0;
}
