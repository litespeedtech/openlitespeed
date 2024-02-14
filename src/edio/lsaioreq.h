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
#ifndef LSAIOREQ_H
#define LSAIOREQ_H

#include <inttypes.h>
#include <stddef.h>
#include <sys/uio.h>

class AioReqEventHandler;
class LogSession;

enum AsyncState
{
    ASYNC_STATE_IDLE = 0,
    ASYNC_STATE_POSTED,
    ASYNC_STATE_READ,
    ASYNC_STATE_CANCELED,
};

class LsAioReq
{
    AioReqEventHandler    *m_pEvtHandler;
    struct iovec           m_iovec;
    off_t                  m_pos;
    int                    m_fd;
    int                    m_ret;
    AsyncState             m_asyncState;
    bool                   m_error;
    bool                   m_cancel;
    bool                   m_read; // Because a file is read-only or write-only here!

    int allocate_buf(int aioBlockSize);

public:
    LsAioReq(AioReqEventHandler *aioReqEventHandler, int fd)
        : m_pEvtHandler(aioReqEventHandler)
        , m_pos(0)
        , m_fd(fd)
        , m_ret(0)
        , m_asyncState(ASYNC_STATE_IDLE)
        , m_error(false)
        , m_cancel(false)
        , m_read(true)
    {
        m_iovec.iov_base = NULL;
        m_iovec.iov_len = 0;
    }
    virtual ~LsAioReq();
    int io_complete(short int revent);

    const char *str_op(bool r)      {   return r ? "READ" : "WRITE";}
    const char *str_op()            {   return str_op(m_read);      }

    int         getfd()             {   return m_fd;            }
    AioReqEventHandler *getHandler(){   return m_pEvtHandler;}
    void        setError();
    bool        error()             {   return m_error; }
    int         init(int aioBlockSize);
    int getRead(char **buffer, off_t offset, int *read);
    bool getCancel()                {   return m_cancel;}
    int cancel();
    struct iovec *getIovec()        {   return &m_iovec;  }
    void setPos(off_t pos)          {   m_pos = pos;    }
    off_t getPos()                  {   return m_pos;   }
    int getRet()                    {   return m_ret;   }
    void setRet(int ret)            {   m_ret = ret;    }
    AsyncState getAsyncState()      {   return m_asyncState;    }
    void setAsyncState(AsyncState s){   m_asyncState = s;       }
    bool rangeChange(off_t offset);
    LogSession *getLogSession();

    virtual int doCancel() = 0;

    bool ioPending();

    virtual int postRead(struct iovec *iov, int iovcnt, off_t pos) = 0;
    virtual int getReadResult(struct iovec **iov) = 0;

};


#endif // Guard
