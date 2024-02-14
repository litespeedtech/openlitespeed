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

#include "edio/lslinuxaioreq.h"
#include <log4cxx/logger.h>
#include <errno.h>

LsLinuxAioReq::LsLinuxAioReq(AioReqEventHandler *aioReqEventHandler, int fd)
    : LsAioReq(aioReqEventHandler, fd)
{
    memset(&m_iocb, 0, sizeof(m_iocb));
}


LsLinuxAioReq::~LsLinuxAioReq()
{
}


int LsLinuxAioReq::postRead(struct iovec *iov, int iovcnt, off_t pos)
{
    struct iocb *iocbp[1] = { &m_iocb };
    setPos(pos);
    iov = getIovec();
    io_prep_preadv(&m_iocb, getfd(), iov, iovcnt, pos);
    m_iocb.data = (void *)this;
    io_set_eventfd(&m_iocb, LinuxAio::getInstance().get_eventfd());
    int ret = io_submit(LinuxAio::getInstance().get_context(), 1, iocbp);
    if (ret != 1)
    {
        int err = errno;
        LS_ERROR(getLogSession(),
                 "LinuxAio Error submitting read: %s (ret: %d)",
                 strerror(err), ret);
        errno = err;
        setError();
        return LS_FAIL;
    }
    setAsyncState(ASYNC_STATE_POSTED);
    return 0;
}


int LsLinuxAioReq::getReadResult(struct iovec **iov)
{
    if (getRet() < 0)
    {
        LS_NOTICE(getLogSession(),
                  "ERROR in Linux Aio read: %s (%d)", strerror(-getRet()),
                  -getRet());
        setError();
        errno = -getRet();
        return -1;
    }
    if (getAsyncState() != ASYNC_STATE_READ)
    {
        LS_DBG(getLogSession(), "Linux Aio do_check state: %d",
               getAsyncState());
        errno = EAGAIN;
        return -1;
    }
    *iov = getIovec();
    return getRet();
}


int LsLinuxAioReq::doCancel()
{
    if (getAsyncState() != ASYNC_STATE_POSTED)
    {
        LS_DBG(getLogSession(), "LinuxAio::doCancel, no pending read");
        return 0;
    }
    struct io_event io_event_result;
    /* NOTE: io_cancel seems to always return EINVAL.  Looking at kernel
       source it appears to return that for lots of conditions.  */
    int ret = io_cancel(LinuxAio::getInstance().get_context(), &m_iocb, &io_event_result);
    if (ret < 0)
        LS_DBG_M(getLogSession(), "Cancel failed: %s (%d), context: %p",
                 strerror(-ret), -ret, LinuxAio::getInstance().get_context());
    else
    {
        setAsyncState(ASYNC_STATE_CANCELED);
        LS_DBG_M(getLogSession(),
                 "Linux Aio cancel succeeded, location returned %ld, res returned %ld",
                 getPos(), io_event_result.res);
    }
    return 0;
}


