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
#ifndef LSPOSIXAIOREQ_H
#define LSPOSIXAIOREQ_H

#include "edio/eventhandler.h"
#include "edio/lsaioreq.h"
#include <edio/aioeventhandler.h>
#include <aio.h>

class LsPosixAioReq : public EventHandler, public LsAioReq
{
    static int          s_rtsigNoPosixAio;
    struct aiocb        m_aiocb;

    void setcb(void *buf, size_t nbytes, int off);

public:
    LsPosixAioReq(AioReqEventHandler *aioReqEventHandler, int fd);
    ~LsPosixAioReq();
    static int load();

    virtual int postRead(struct iovec *iov, int iovcnt, off_t pos);
    virtual int getReadResult(struct iovec **iov);
    virtual int  doCancel();

#if defined(LS_AIO_USE_KQ)
    virtual int onAioEvent();
#endif
    virtual int onEvent();
};


#endif // Guard
