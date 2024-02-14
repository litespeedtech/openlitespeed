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

#include "edio/lsaioreq.h"
#include "edio/aioreqeventhandler.h"
#include <log4cxx/logger.h>

#include <errno.h>
#include <poll.h>

LsAioReq::~LsAioReq()
{
    if (m_iovec.iov_base)
        free(m_iovec.iov_base);
}

LogSession *LsAioReq::getLogSession()
{   return m_pEvtHandler->getLogSession();   }


void LsAioReq::setError()
{
    if (!m_error)
    {
        LS_DBG_M(getLogSession(), "LsAioReq::setError()");
        m_error = true;
    }
}


int LsAioReq::io_complete(short int revent)
{
    LS_DBG_M(getLogSession(), "LsAioReq, io_complete event %d\n", revent);
    if (revent & (POLLERR | POLLHUP))
    {
        LS_NOTICE(getLogSession(), "LsAioReq revent unexpected: %d\n", revent);
        setError();
        return -1;
    }
    struct iovec *iov;
    int ret = getReadResult(&iov);
    if (ret < 0)
    {
        if (errno == EAGAIN)
        {
            LS_DBG_M(getLogSession(), "LsAioReq Not ready yet, retry\n");
            return 0;
        }
        LS_NOTICE(getLogSession(), "LsAioReq file read error io_complete error: %s\n",
                  strerror(errno));
        setError();
        return -1;
    }
    if (m_cancel || m_error)
    {
        LS_DBG_M(getLogSession(), "LsAioReq Cancel/Error");
        return 0;
    }
    LS_DBG_M(getLogSession(), "LsAioReq read %d bytes at %ld", m_ret, m_pos);
    if (m_ret < (int)iov->iov_len)
        LS_DBG_M(getLogSession(), "LsAioReq io_complete %d < posted size %d -"
                 " this is expected\n", m_ret, (int)iov->iov_len);
    LS_DBG_M(getLogSession(), "LsAioReq call session onAioReqEvent\n");
    ret = m_pEvtHandler->onAioReqEvent();
    if (ret < 0)
    {
        LS_DBG_M(getLogSession(), "LsAioReq io_complete error in session onAioReqEvent");
        setError();
        return -1;
    }
    else if (ret == LS_AGAIN)
        return ret;
    return 0;
}


bool LsAioReq::rangeChange(off_t offset)
{
    if (m_asyncState == ASYNC_STATE_IDLE ||
        (m_asyncState == ASYNC_STATE_READ && (offset < m_pos || offset > m_pos + m_ret)))
    {
        LS_DBG(getLogSession(),
               "in LsAioReq::rangeChange, state: %d, offset %ld, m_pos %ld ret %d",
               m_asyncState, offset, m_pos, m_ret);
        return true;
    }
    return false;
}


int LsAioReq::getRead(char **buffer, off_t offset, int *read)
{
    *read = 0;
    *buffer = NULL;
    if (error())
        return LS_FAIL;
    if (m_asyncState == ASYNC_STATE_POSTED)
    {
        LS_DBG_M(getLogSession(), "LsAioReq, Read still pending");
        errno = EAGAIN;
        return 0; // MUST check done
    }
    else if (m_asyncState != ASYNC_STATE_READ)
    {
        LS_ERROR(getLogSession(), "Should be in read state, in %d state", m_asyncState);
        return -1;
    }
    if (offset > m_pos + m_ret)
    {
        LS_ERROR(getLogSession(), "Expected LsAioReq::getRead offset at %ld got %ld",
                 m_pos, offset);
        assert(offset <= m_pos + m_ret);
        return -1;
    }
    if (offset && offset < m_pos)
    {
        LS_ERROR(getLogSession(), "Expected LsAioReq::getRead offset >= %ld, got %ld",
                 m_pos, offset);
        assert(offset >= m_pos);
        return -1;
    }
    int diff = offset - m_pos;
    LS_DBG_M(getLogSession(), "LsAioReq::getRead at %ld, %d bytes diff: %d", 
             m_pos, m_ret, diff);
    *read = m_ret - diff;
    *buffer = ((char *)getIovec()->iov_base) + diff;
    return 0;
}


int LsAioReq::allocate_buf(int aioBlockSize)
{
    if (!aioBlockSize || aioBlockSize % 256)
        aioBlockSize = 0x20000;

    m_iovec.iov_len = aioBlockSize;
    LS_DBG_M(getLogSession(), "LsAioReq AioBlockSize: %ld", m_iovec.iov_len);
    if (posix_memalign(&m_iovec.iov_base, 4096, aioBlockSize))
    {
        errno = ENOMEM;
        return -1;
    }
    return 0;
}


int LsAioReq::init(int aioBlockSize)
{
    LS_DBG_M(getLogSession(), "LsAioReq::init %d\n", m_fd);

    if (allocate_buf(aioBlockSize))
    {
        LS_NOTICE(getLogSession(), "LsAioReq: allocate data buffer failed\n");
        return LS_FAIL;
    }

    return 0;
}


bool LsAioReq::ioPending()
{
    return m_asyncState == ASYNC_STATE_POSTED;
}


int LsAioReq::cancel()
{
    if (m_cancel)
        return 0;
    m_cancel = true;
    LS_DBG_M(getLogSession(), "LsAioReq, cancel, try to drain post queue");
    return doCancel();
}


