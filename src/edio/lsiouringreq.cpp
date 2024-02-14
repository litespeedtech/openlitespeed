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

#include "edio/lsiouringreq.h"
#include <edio/iouring.h>
#include <edio/multiplexer.h>
#include <log4cxx/logger.h>

#include <fcntl.h>
#include "liburing/include/liburing/io_uring.h"
#include "liburing/include/liburing.h"

LsIouringReq::LsIouringReq(AioReqEventHandler *aioReqEventHandler, int fd)
    : LsAioReq(aioReqEventHandler, fd)
{
}


LsIouringReq::~LsIouringReq()
{
}


int LsIouringReq::postRead(struct iovec *iov, int iovcnt, off_t pos)
{
    int err;
    setPos(pos);
    if (pos == 0)
    {
        int flags = fcntl(getfd(), F_GETFL, 0);
        LS_DBG(getLogSession(), "First submit for fd %d, flags %d", getfd(), flags);
        if (flags & O_NONBLOCK)
        {
            LS_DBG(getLogSession(), "Non-blocking is on, turn it off, rc: %d",
                   fcntl(getfd(), F_SETFL, flags & ~O_NONBLOCK));
        }
    }
    if (error())
    {
        LS_DBG(getLogSession(), "iouring In error, do no more submits");
        return -1;
    }
    LS_DBG(getLogSession(), "Iouring getsqe, fd: %d", getfd());
    struct io_uring_sqe *sqe = io_uring_get_sqe(Iouring::getInstance().getIo_uring());
    if (sqe == NULL)
    {
        LS_NOTICE(getLogSession(), "Iouring io_uring no sqe, for read: busy");
        errno = EAGAIN;
        return -1;
    }
    io_uring_prep_readv(sqe, getfd(), iov, iovcnt, pos);
    io_uring_sqe_set_data(sqe, (void *)this);
    LS_DBG(getLogSession(), "Iouring io_uring_submit %ld bytes (%p) at %ld\n",
           iov->iov_len, iov, pos);
    err = io_uring_submit(Iouring::getInstance().getIo_uring());
    if (err != 1)
    {
        LS_NOTICE(getLogSession(),
                  "Iouring io_uring_submit for read failed: %d", err);
        if (err < 0)
            errno = -err;
        else
            errno = EINVAL;
        return -1;
    }
    setAsyncState(ASYNC_STATE_POSTED);
    return 0;
}


int LsIouringReq::getReadResult(struct iovec **iov)
{
    if (getAsyncState() != ASYNC_STATE_READ)
    {
        LS_DBG(getLogSession(), "Iouring do_check state: %d", getAsyncState());
        errno = EAGAIN;
        return -1;
    }
    if (getRet() < 0)
    {
        LS_NOTICE(getLogSession(), "Iouring read error:  %s (%d)",
                  strerror(-getRet()), -getRet());
        errno = -getRet();
        return -1;
    }
    *iov = getIovec();
    return getRet();
}


int LsIouringReq::doCancel()
{
    if (getAsyncState() != ASYNC_STATE_POSTED)
    {
        LS_DBG(getLogSession(), "Iouring::doCancel, no pending read");
        return 0;
    }
    LS_INFO(getLogSession(), "Canceling posted io_uring read");
    struct iovec *iov = getIovec();
    struct io_uring_sqe *sqe = io_uring_get_sqe(Iouring::getInstance().getIo_uring());
    if (sqe == NULL)
    {
        LS_NOTICE(getLogSession(), "Iouring no sqe, for cancel: busy");
        errno = EBUSY;
        return -1;
    }
    io_uring_prep_cancel(sqe, (void *)this, 0);
    io_uring_sqe_set_data(sqe, (void *)this);
    LS_DBG(getLogSession(),
           "Iouring io_uring_submit cancel iov: %p at %ld", iov, getPos());
    setAsyncState(ASYNC_STATE_POSTED);
    int err = io_uring_submit(Iouring::getInstance().getIo_uring());
    if (err < 0)
    {
        setRet(err);
        LS_DBG(getLogSession(), "Cancel at %ld failed %s", getPos(),
               strerror(-err));
    }
    else
        LS_DBG(getLogSession(), "Cancel should be successful (see the event)");
    return 0;
}

