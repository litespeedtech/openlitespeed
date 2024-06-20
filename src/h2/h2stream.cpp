/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
#include "h2stream.h"

#include "h2connection.h"
#include "unpackedheaders.h"

#include <http/hiohandlerfactory.h>

#include <util/datetime.h>
#include <log4cxx/logger.h>
#include <lsr/ls_strtool.h>
#include <util/iovec.h>
#include "lsdef.h"

H2Stream::H2Stream()
{
}


H2Stream::~H2Stream()
{
}


void H2Stream::reset()
{
    HioStream::reset(0);
    H2StreamBase::reset();
}

const char *H2Stream::buildLogId()
{
    m_logId.len = lsnprintf(m_logId.ptr, MAX_LOGID_LEN, "%s-%d",
                        ((H2Connection *)m_pH2Conn)->getStream()->getLogId(), getStreamID());
    return m_logId.ptr;
}


int H2Stream::onInitConnected()
{
    assert(getHandler());
    if (getFlag(HIO_FLAG_INIT_SESS))
    {
        setFlag(HIO_FLAG_INIT_SESS, 0);
        getHandler()->onInitConnected();
    }
    if (isWantRead() && m_bufRcvd.size() > 0)
        onRead();
    if (isWantWrite())
        if (next() == NULL)
            m_pH2Conn->add2PriorityQue(this);
    //getHandler()->onWriteEx();
    return 0;
}


int H2Stream::onPeerShutdown()
{
    H2StreamBase::onPeerShutdown();
    if (getFlag(HIO_FLAG_INIT_SESS))
    {
        if (((H2Connection *)m_pH2Conn)->assignStreamHandler(this) == LS_FAIL)
            return LS_FAIL;
    }
    if (getHandler()->detectContentLenMismatch(m_bufRcvd.size()))
        return LS_FAIL;
    return LS_OK;
}


int H2Stream::onTimer()
{
    if (getState() == HIOS_CONNECTED && getHandler())
        return getHandler()->onTimerEx();
    return 0;
}


int H2Stream::onClose()
{
    if (getHandler() && !isReadyToRelease())
        getHandler()->onCloseEx();
    return LS_OK;
}


int H2Stream::onRead()
{
    if (getHandler())
        getHandler()->onReadEx();
    return LS_OK;
}


int H2Stream::onWrite()
{
    LS_DBG_L(this, "H2Stream::onWrite()");
    if (m_iWindowOut <= 0)
        return 0;
    setFlag(HIO_FLAG_PAUSE_WRITE, 0);

    if (isWantWrite())
    {
        if (!getHandler())
        {
            setFlag(HIO_FLAG_WANT_WRITE, 0);
            return 0;
        }
        getHandler()->onWriteEx();
        if (isWantWrite())
            m_pH2Conn->needWriteEvent();
    }
    return 0;
}


int H2Stream::sendRespHeaders(HttpRespHeaders *pHeaders, int isNoBody)
{
    uint8_t flag = 0;
    if (getState() == HIOS_DISCONNECTED)
        return LS_FAIL;
    if (isNoBody)
    {
        LS_DBG_L(this, "No response body, set END_STREAM.");
        flag |= H2_FLAG_END_STREAM;
        if (getState() != HIOS_SHUTDOWN)
        {
            setFlag(HIO_FLAG_LOCAL_SHUTDOWN, 1);
            if (readyToShutdown())
                markShutdown();
        }
    }
//     else
//     {
//         if (next() == NULL)
//             m_pH2Conn->add2PriorityQue(this);
//     }
    int ret = ((H2Connection *)m_pH2Conn)->sendRespHeaders(this, pHeaders, flag);
    if (isNoBody)
        m_pH2Conn->wantFlush();
    if (ret != LS_FAIL && getFlag(HIO_FLAG_ALTSVC_SENT))
        ((H2Connection *)m_pH2Conn)->getStream()->setFlag(HIO_FLAG_ALTSVC_SENT, 1);
    setFlag(HIO_FLAG_RESP_HEADER_SENT, 1);
    return ret;
}


int H2Stream::push(UnpackedHeaders *hdrs)
{
    return ((H2Connection *)m_pH2Conn)->pushPromise(getStreamID(), hdrs);
}



