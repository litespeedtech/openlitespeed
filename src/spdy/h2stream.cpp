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
#include "h2stream.h"

#include "h2connection.h"
#include "unpackedheaders.h"

#include <http/hiohandlerfactory.h>

#include <util/datetime.h>
#include <log4cxx/logger.h>
#include <lsr/ls_strtool.h>
#include <util/iovec.h>
#include <util/ssnprintf.h>
#include "lsdef.h"

H2Stream::H2Stream()
    : m_bufIn(0)
{
    LS_ZERO_FILL(m_pH2Conn, m_iWindowIn);
}


H2Stream::~H2Stream()
{
    m_bufIn.clear();
}


void H2Stream::reset()
{
    HioStream::reset(0);
    m_bufIn.clear();
    m_headers.reset();
    LS_ZERO_FILL(m_pH2Conn, m_iWindowIn);
}

const char *H2Stream::buildLogId()
{
    m_logId.len = safe_snprintf(m_logId.ptr, MAX_LOGID_LEN, "%s-%d",
                        m_pH2Conn->getStream()->getLogId(), getStreamID());
    return m_logId.ptr;
}


void H2Stream::apply_priority(Priority_st *pPriority)
{
    int pri = HIO_PRIORITY_HTML;
    if (pPriority)
    {
        if (pPriority->m_weight <= 32)
            pri = (32 - pPriority->m_weight) >> 2;
        else
            pri = (256 - pPriority->m_weight) >> 5;
    }
    if (pri != getPriority())
    {
        if (next())
        {
            m_pH2Conn->removePriQue(this);
            setPriority(pri);
            m_pH2Conn->add2PriorityQue(this);
        }
        else
            setPriority(pri);
    }
}


int H2Stream::init(H2Connection *pH2Conn,
                   Priority_st *pPriority)
{
    setActiveTime(DateTime::s_curTime);
    clearLogId();

    setState(HIOS_CONNECTED);
    uint32_t flag = HIO_FLAG_WRITE_BUFFER | HIO_FLAG_SENDFILE;
    if (pH2Conn->getStream()->getFlag(HIO_FLAG_ALTSVC_SENT))
        flag |= HIO_FLAG_ALTSVC_SENT;
    setFlag(flag, 1);
    m_bufIn.clear();
    m_pH2Conn = pH2Conn;
    m_iWindowOut = pH2Conn->getStreamOutInitWindowSize();
    m_iWindowIn = pH2Conn->getStreamInInitWindowSize();

    apply_priority(pPriority);

    LS_DBG_L(this, "H2Stream::init(), id: %d, priority: %d, flag: %d. ",
             getStreamID(), getPriority(), (int)getFlag());
    return 0;
}


int H2Stream::onInitConnected(HioHandler *pHandler, bool bUpgraded)
{
    assert(getHandler() == NULL);
    if (!pHandler)
    {
        pHandler = HioHandlerFactory::getHandler(HIOS_PROTO_HTTP);
        if (!pHandler)
        {
            m_pH2Conn->resetStream(0, this, H2_ERROR_INTERNAL_ERROR);
            return LS_FAIL;
        }
    }
    if (pHandler)
    {
        pHandler->attachStream(this);
    }
    if (!bUpgraded)
        getHandler()->onInitConnected();

    if (isWantRead())
        getHandler()->onReadEx();
    if (isWantWrite())
        if (next() == NULL)
            m_pH2Conn->add2PriorityQue(this);
    //getHandler()->onWriteEx();
    return 0;
}


int H2Stream::appendReqData(char *pData, int len, uint8_t flags)
{
    if (m_bufIn.append(pData, len) == -1)
        return LS_FAIL;
    if (isFlowCtrl())
        m_iWindowIn -= len;
    //Note: H2_CTRL_FLAG_FIN is directly mapped to HIO_FLAG_PEER_SHUTDOWN
    //      H2_CTRL_FLAG_UNIDIRECTIONAL is directly mapped to HIO_FLAG_LOCAL_SHUTDOWN
    if (flags & H2_CTRL_FLAG_FIN)
    {
        setFlag(HIO_FLAG_PEER_SHUTDOWN, 1);
        if (getFlag(HIO_FLAG_INIT_SESS))
        {
            setFlag(HIO_FLAG_INIT_SESS, 0);
            if (onInitConnected(NULL, 0) == LS_FAIL)
                return len;
        }
        if (getHandler()->detectContentLenMismatch(m_bufIn.size()))
            return LS_FAIL;
    }
    if (isWantRead() && getHandler())
        getHandler()->onReadEx();
    return len;
}


//***int H2Stream::read( char * buf, int len )***//
// return > 0:  number of bytes of that has been read
// return = 0:  0 byte of data has been read, but there will be more data coming,
//              need to read again
// return = -1: EOF (End of File) There is no more data need to be read, the
//              stream can be removed
int H2Stream::read(char *buf, int len)
{
    int ReadCount;
    if (getState() == HIOS_DISCONNECTED)
        return -1;

    ReadCount = m_bufIn.moveTo(buf, len);
    if (ReadCount == 0)
    {
        if (getFlag(HIO_FLAG_PEER_SHUTDOWN))
        {
            return -1; //EOF (End of File) There is no more data need to be read
        }
    }
    if (ReadCount > 0)
        setActiveTime(DateTime::s_curTime);

    return ReadCount;
}


void H2Stream::continueRead()
{
    LS_DBG_L(this, "H2Stream::continueRead()");
    setFlag(HIO_FLAG_WANT_READ, 1);
    if (getHandler() && m_bufIn.size() > 0)
        getHandler()->onReadEx();
}


void H2Stream:: continueWrite()
{
    LS_DBG_L(this, "H2Stream::continueWrite()");
    setFlag(HIO_FLAG_WANT_WRITE, 1);
    if (next() == NULL)
        m_pH2Conn->add2PriorityQue(this);
    m_pH2Conn->setPendingWrite();

}


int H2Stream::onTimer()
{
    if (getHandler() && getState() == HIOS_CONNECTED)
        return getHandler()->onTimerEx();
    return 0;
}


void H2Stream::shutdownEx()
{
    LS_DBG_L(this, "H2Stream::shutdown()");
    m_pH2Conn->sendFinFrame(getStreamID());
    setActiveTime(DateTime::s_curTime);
    m_pH2Conn->wantFlush();
}

int H2Stream::shutdown()
{
    if (getState() >= HIOS_SHUTDOWN)
        return 0;

    shutdownEx();

    markShutdown();
    return 0;
}


void H2Stream::markShutdown()
{
    setState(HIOS_SHUTDOWN);
    m_pH2Conn->incShutdownStream();
    if (!next())
    {
        setPriority(0);
        m_pH2Conn->add2PriorityQue(this);
    }
}


void H2Stream::closeEx()
{
    if (getState() == HIOS_DISCONNECTED)
        return;
    if (getHandler() && !isReadyToRelease())
        getHandler()->onCloseEx();
    if (getState() < HIOS_SHUTDOWN)
        shutdownEx();
    if (getState() == HIOS_SHUTDOWN)
        m_pH2Conn->decShutdownStream();
    setState(HIOS_DISCONNECTED);
    setFlag(HIO_FLAG_WANT_WRITE, 0);
}


int H2Stream::close()
{
    if (getState() == HIOS_DISCONNECTED)
        return 0;
    closeEx();
    m_pH2Conn->recycleStream(this);
    return 0;
}


int H2Stream::flush()
{
    LS_DBG_L(this, "H2Stream::flush()");
    m_pH2Conn->wantFlush();
    return LS_DONE;
}


int H2Stream::getDataFrameSize(int wanted)
{
    if ((m_pH2Conn->isOutBufFull()) || (0 >= m_iWindowOut))
    {
        setFlag(HIO_FLAG_PAUSE_WRITE | HIO_FLAG_WANT_WRITE, 1);
        if (next() == NULL)
            m_pH2Conn->add2PriorityQue(this);
        m_pH2Conn->setPendingWrite();
        return 0;
    }

    if (wanted > m_iWindowOut)
        wanted = m_iWindowOut;
    wanted = m_pH2Conn->getAllowedDataSize(wanted);
    return wanted;
}


int H2Stream::writev(IOVec &vector, int total)
{
    int ret;
    int pkt_size;
    if (getState() == HIOS_DISCONNECTED)
        return LS_FAIL;
    int weight = m_pH2Conn->getWeightedPriority(this);
    LS_DBG_L(this, "H2Stream::writev(%d, %d), weight: %d", vector.len(),
             total, weight);
    int flag = 0;
    int remain = total;
    int fin = 0;
    IOVec iov(vector);

    while(!isPauseWrite() && remain > 0)
    {
        pkt_size = getDataFrameSize(remain);
        if (pkt_size <= 0)
            break;
        if (total - remain + pkt_size >= (128 * 1024) / weight)
        {
            if (pkt_size > 16384)
            {
                int new_pkt_size = (128 * 1024) / weight - (total - remain);
                if (new_pkt_size < 16384)
                    pkt_size = 16384;
                else
                    pkt_size = new_pkt_size;
            }
            LS_DBG_L(this, "H2Stream::write() limited by weight, pkt_size: %d", pkt_size);
            if (pkt_size <= 0)
                break;
            setFlag(HIO_FLAG_PAUSE_WRITE, 1);
        }
        if (pkt_size >= remain && (flag & HIO_EOR))
        {
            fin = H2_FLAG_END_STREAM;
            markShutdown();
        }
        ret = m_pH2Conn->sendDataFrame(getStreamID(), fin, &iov, pkt_size);
        LS_DBG_L(this, "H2Stream::sendData(), total: %d, ret: %d", total, ret);
        if (fin)
            m_pH2Conn->wantFlush();
        ret = dataSent(ret);
        if (ret <= 0)
            break;
        remain -= ret;
    }
    return total - remain;
}


int H2Stream::writev(const struct iovec *vec, int count)
{
    if (count == 1)
        return write((const char *)vec->iov_base, vec->iov_len, 0);

    IOVec iov(vec, count);
    return writev(iov, iov.bytes());
}


int H2Stream::write(const char *buf, int len)
{
    return write(buf, len, 0);
}


// int H2Stream::write(const char *buf, int len, int flag)
// {
//     int allowed;
//     if (getState() == HIOS_DISCONNECTED)
//         return LS_FAIL;
//     allowed = getDataFrameSize(len);
//     if (allowed <= 0)
//         return 0;
//     if (allowed >= len && (flag & HIO_EOR))
//     {
//         flag = H2_FLAG_END_STREAM;
//         markShutdown();
//     }
//     else
//         flag = 0;
//     int ret = m_pH2Conn->sendDataFrame(m_uiStreamId, flag, buf, allowed);
//     LS_DBG_L(this, "H2Stream::write(%p, %d, %d) return %d", buf, allowed, flag, ret);
//     if (flag)
//         m_pH2Conn->wantFlush();
//     return dataSent(ret);
// }


int H2Stream::write(const char *buf, int len, int flag)
{
    const char *end = buf + len;
    const char *p = buf;
    int fin = 0;
    int ret = 0;
    int pkt_size;
    int weight;
    if (getState() == HIOS_DISCONNECTED)
        return LS_FAIL;
    weight = m_pH2Conn->getWeightedPriority(this);
    LS_DBG_L(this, "H2Stream::write(%p, %d, %d), weight: %d", buf, len,
             flag, weight);

    while(!isPauseWrite() && end - p > 0)
    {
        pkt_size = getDataFrameSize(end - p);
        if (pkt_size <= 0)
            break;
        if (p + pkt_size - buf >= (128 * 1024) / weight)
        {
            if (pkt_size > 16384)
            {
                int new_pkt_size = (128 * 1024) / weight - (p - buf);
                if (new_pkt_size < 16384)
                    pkt_size = 16384;
                else
                    pkt_size = new_pkt_size;
            }
            LS_DBG_L(this, "H2Stream::write() limited by weight, pkt_size: %d", pkt_size);
            if (pkt_size <= 0)
                break;
            setFlag(HIO_FLAG_PAUSE_WRITE, 1);
        }
        if (pkt_size >= end - p && (flag & HIO_EOR))
        {
            fin = H2_FLAG_END_STREAM;
            markShutdown();
        }
        ret = m_pH2Conn->sendDataFrame(getStreamID(), fin, p, pkt_size);
        LS_DBG_L(this, "H2Stream::write(%p, %d, %d) return %d", p, pkt_size, fin, ret);
        if (fin)
            m_pH2Conn->wantFlush();
        ret = dataSent(ret);
        if (ret <= 0)
            break;
        p += ret;
    }
    return (p - buf > 0)? p - buf : ret;
}


int H2Stream::sendfile(int fdSrc, off_t off, size_t size, int flag)
{
    off_t cur = off;
    off_t end = off + size;
    int fin = 0;
    int ret = 0;
    int pkt_size;
    int weight;
    if (getState() == HIOS_DISCONNECTED)
        return LS_FAIL;
    weight = m_pH2Conn->getWeightedPriority(this);
    LS_DBG_L(this, "H2Stream::sendfile(%d, %lld, %zd, %d), weight: %d",
             fdSrc, (long long)off, size, flag, weight);

    while(!isPauseWrite() && end - cur > 0)
    {
        pkt_size = getDataFrameSize(end - cur);
        if (pkt_size <= 0)
            break;
        if (cur + pkt_size - off >= (128 * 1024) / weight)
        {
            if (pkt_size > 16384)
            {
                int new_pkt_size = (128 * 1024) / weight - (cur - off);
                if (new_pkt_size < 16384)
                    pkt_size = 16384;
                else
                    pkt_size = new_pkt_size;
            }
            LS_DBG_L(this, "H2Stream::sendfile() limited by weight, pkt_size: %d", pkt_size);
            if (pkt_size <= 0)
                break;
            setFlag(HIO_FLAG_PAUSE_WRITE, 1);
        }
//         if (pkt_size == 16384)
//             pkt_size -= 9;
        if (pkt_size >= end - cur && (flag & HIO_EOR))
        {
            fin = H2_FLAG_END_STREAM;
            markShutdown();
        }
        ret = m_pH2Conn->sendfileDataFrame(getStreamID(), fin, fdSrc, cur, pkt_size);
        LS_DBG_L(this, "H2Stream::sendfileDataFrame(%d, %lld, %d, %d) return %d",
                 fdSrc, (long long)cur, pkt_size, fin, ret);
        if (fin)
            m_pH2Conn->wantFlush();
        ret = dataSent(ret);
        if (ret <= 0)
            break;
        cur += ret;
    }
    return (cur - off > 0)? cur - off : ret;
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
            m_pH2Conn->setPendingWrite();
    }
    return 0;
}


int H2Stream::dataSent(int ret)
{
    if (ret == -1)
    {
        setFlag(HIO_FLAG_ABORT, 1);
        return LS_FAIL;
    }

    setActiveTime(DateTime::s_curTime);
    bytesSent(ret);
    if (isFlowCtrl())
    {
        m_iWindowOut -= ret;
        LS_DBG_L(this, "sent: %lld, current window: %d",
                 (long long)getBytesSent(), m_iWindowOut);

        if (m_iWindowOut <= 0)
            setFlag(HIO_FLAG_PAUSE_WRITE, 1);
    }
    return ret;
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
            markShutdown();
        }
    }
//     else
//     {
//         if (next() == NULL)
//             m_pH2Conn->add2PriorityQue(this);
//     }
    int ret = m_pH2Conn->sendRespHeaders(pHeaders, getStreamID(), flag);
    if (isNoBody)
        m_pH2Conn->wantFlush();
    if (ret != LS_FAIL && getFlag(HIO_FLAG_ALTSVC_SENT))
        m_pH2Conn->getStream()->setFlag(HIO_FLAG_ALTSVC_SENT, 1);
    return ret;
}


int H2Stream::adjWindowOut(int32_t n)
{
    if (isFlowCtrl())
    {
        m_iWindowOut += n;
        LS_DBG_L(this, "stream WINDOW_UPDATE: %d, window size: %d.",
                 n, m_iWindowOut);
        if (m_iWindowOut < 0)
        {
            //window overflow
            return LS_FAIL;
        }
        else if (isWantWrite())
            continueWrite();
    }
    return 0;
}


int H2Stream::push(ls_str_t *pUrl, ls_str_t *pHost,
                   ls_strpair_t *pExtraHeaders)
{
    return m_pH2Conn->pushPromise(getStreamID(), pUrl, pHost,
                                  pExtraHeaders);
}

