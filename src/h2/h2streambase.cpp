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
#include "h2streambase.h"

#include "h2connbase.h"
#include <http/hiostream.h>

#include <util/datetime.h>
#include <log4cxx/logger.h>
#include <lsr/ls_strtool.h>
#include <util/iovec.h>
#include <util/ssnprintf.h>
#include "lsdef.h"

H2StreamBase::H2StreamBase()
    : m_bufRcvd(0)
{
    LS_ZERO_FILL(m_pH2Conn, m_iWindowOut);
}


H2StreamBase::~H2StreamBase()
{
    m_bufRcvd.clear();
}


void H2StreamBase::reset()
{
    m_bufRcvd.clear();
    LS_ZERO_FILL(m_pH2Conn, m_iWindowOut);
}


int H2StreamBase::init(H2ConnBase *pH2Conn, Priority_st *pPriority)
{
    setActiveTime(DateTime::s_curTime);
    clearLogId();

    setState(HIOS_CONNECTED);
    setProtocol(HIOS_PROTO_HTTP2);
    m_bufRcvd.clear();
    m_pH2Conn = pH2Conn;
    m_iWindowOut = pH2Conn->getStreamOutInitWindowSize();

    apply_priority(pPriority);

    LS_DBG_L(this, "H2Stream::init(), id: %d, priority: %d, flag: %d. ",
             getStreamID(), getPriority(), (int)getFlag());
    return 0;
}


void H2StreamBase::apply_priority(Priority_st *pPriority)
{
    int pri = HIO_PRIORITY_HTML;
    if (pPriority)
    {
        if (pPriority->m_weight <= 32)
            pri = (32 - pPriority->m_weight) >> 2;
        else
            pri = (256 - pPriority->m_weight) >> 5;
        if (pri > HIO_PRIORITY_LOWEST)
            pri = HIO_PRIORITY_LOWEST;
        else if (pri < HIO_PRIORITY_HIGHEST)
            pri = HIO_PRIORITY_HIGHEST;
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


//***int H2StreamBase::read( char * buf, int len )***//
// return > 0:  number of bytes of that has been read
// return = 0:  0 byte of data has been read, but there will be more data coming,
//              need to read again
// return = -1: EOF (End of File) There is no more data need to be read, the
//              stream can be removed
int H2StreamBase::read(char *buf, int len)
{
    int read_out;
    if (getState() == HIOS_DISCONNECTED)
        return -1;

    read_out = m_bufRcvd.moveTo(buf, len);
    if (read_out > 0)
    {
        setActiveTime(DateTime::s_curTime);
        adjWindowToUpdate(read_out);
        LS_DBG_H(this, "WINDOW_IN consumed: %d, to_update: %d.",
                 read_out, getWindowToUpdate());
        windowUpdate();
    }
    if (getFlag(HIO_FLAG_PEER_SHUTDOWN) && (read_out == 0 || m_bufRcvd.empty()))
    {
        setFlag(SS_FLAG_READ_EOS, 1);
        if (read_out == 0)
            return -1;
    }
    return read_out;
}


void H2StreamBase::windowUpdate()
{
    if (m_iWindowToUpdate >= m_pH2Conn->getStreamInInitWindowSize() / 2)
    {
        m_pH2Conn->sendStreamWindowUpdateFrame(getStreamID(), m_iWindowToUpdate);
        m_iWindowToUpdate = 0;
    }
}


int H2StreamBase::processRcvdData(char *pData, int len)
{
    if (len > 0)
    {
        if (m_bufRcvd.append(pData, len) == -1)
            return LS_FAIL;
        LS_DBG_H(this, "H2StreamBase::processRcvdData(%p, %d), total: %lld, "
                 "buffered: %d, app_want_read: %d",
            pData, len, (long long)getBytesRecv(), m_bufRcvd.size(), isWantRead());
    }
//    if (isWantRead())
//        onRead();
    return len;
}


char *H2StreamBase::getDirectInBuf(size_t &size)
{
    if (size > 16384)
        size = 16384;
    //if (m_bufRcvd.available() <= 0)
    m_bufRcvd.guarantee(size);
    int avail = m_bufRcvd.contiguous();
    if (avail < (int)size)
        size = avail;
    LS_DBG_H(this, "H2StreamBase::getDirectInBuf() return %zd",
             size);
    return m_bufRcvd.end();
}


void H2StreamBase::directInBufUsed(size_t len)
{
    assert((int)len <= m_bufRcvd.contiguous());
    m_bufRcvd.used(len);
    bytesRecv(len);
    LS_DBG_H(this, "H2StreamBase::direcInBufUsed(%zd), total: %lld, "
            "buffered: %d, app_want_read: %d",
             len, (long long)getBytesRecv(), m_bufRcvd.size(), isWantRead());
}


char *H2StreamBase::getDirectOutBuf(size_t &size)
{
    if (size > 16384)
        size = 16384;
    //if (m_bufRcvd.available() <= 0)
    return m_pH2Conn->getDirectOutBuf(getStreamID(), size);
}


void H2StreamBase::directOutBufUsed(size_t len)
{
    if (len > 0)
    {
        bytesSent(len);
        LS_DBG_H(this, "H2StreamBase::directOutBufUsed(%zd), "
                 "close frame, total sent: %lld", len, (long long)getBytesSent());
        m_pH2Conn->directOutBufUsed(getStreamID(), len);
    }
}


int H2StreamBase::onPeerShutdown()
{
    LS_DBG_H(this, "H2StreamBase::onPeerShutdown()");
    //Note: H2_CTRL_FLAG_FIN is directly mapped to HIO_FLAG_PEER_SHUTDOWN
    //      H2_CTRL_FLAG_UNIDIRECTIONAL is directly mapped to HIO_FLAG_LOCAL_SHUTDOWN
    setFlag(HIO_FLAG_PEER_SHUTDOWN, 1);
    return LS_OK;

}


void H2StreamBase::shutdownWrite()
{
    if (!getFlag(HIO_FLAG_LOCAL_SHUTDOWN))
    {
        LS_DBG_L(this, "H2StreamBase::shutdownWrite()");
        m_pH2Conn->sendFinFrame(getStreamID());
        setFlag(HIO_FLAG_LOCAL_SHUTDOWN, 1);
        suspendWrite();
        setActiveTime(DateTime::s_curTime);
        m_pH2Conn->wantFlush();
    }
}

int H2StreamBase::shutdown()
{
    LS_DBG_L(this, "H2StreamBase::close()");
    if (getState() >= HIOS_SHUTDOWN)
        return 0;

    if (!getFlag(HIO_FLAG_RESP_HEADER_SENT))
    {
        LS_DBG_L(this, "No header has been sent, cancel stream with RST_FRAME.");
        abort();
    }
    else
    {
        shutdownWrite();
        markShutdown();
    }
    return 0;
}


void H2StreamBase::markShutdown()
{
    setState(HIOS_SHUTDOWN);
    m_pH2Conn->incShutdownStream();
    if (!next())
    {
        setPriority(0);
        m_pH2Conn->add2PriorityQue(this);
    }
}


void H2StreamBase::closeEx()
{
    if (getState() == HIOS_DISCONNECTED)
        return;
    onClose();
    if (getState() < HIOS_SHUTDOWN)
        shutdownWrite();
    if (getState() == HIOS_SHUTDOWN)
        m_pH2Conn->decShutdownStream();
    setState(HIOS_DISCONNECTED);
    setFlag(HIO_FLAG_WANT_WRITE, 0);
}


void H2StreamBase::abort()
{
    if (getState() == HIOS_DISCONNECTED)
        return;
    onAbort();
    if (getState() < HIOS_SHUTDOWN)
        m_pH2Conn->sendRstFrame(getStreamID(), H2_ERROR_CANCEL);
    if (getState() == HIOS_SHUTDOWN)
        m_pH2Conn->decShutdownStream();
    setState(HIOS_DISCONNECTED);
    setFlag(HIO_FLAG_WANT_WRITE, 0);
}


int H2StreamBase::close()
{
    LS_DBG_L(this, "H2StreamBase::close()");
    if (getState() == HIOS_DISCONNECTED)
        return 0;
    onClose();
    m_pH2Conn->recycleStream(this);
    return 0;
}


void H2StreamBase::continueRead()
{
    LS_DBG_L(this, "H2StreamBase::continueRead()");
    setFlag(HIO_FLAG_WANT_READ, 1);
    if (!getFlag(HIO_EVENT_PROCESSING) && m_bufRcvd.size() > 0)
        onRead();
}


void H2StreamBase:: continueWrite()
{
    LS_DBG_L(this, "H2StreamBase::continueWrite()");
    setFlag(HIO_FLAG_WANT_WRITE, 1);
    if (next() == NULL)
        m_pH2Conn->add2PriorityQue(this);
    m_pH2Conn->needWriteEvent();
}




int H2StreamBase::flush()
{
    LS_DBG_L(this, "H2StreamBase::flush()");
    m_pH2Conn->wantFlush();
    return LS_DONE;
}


int H2StreamBase::getDataFrameSize(int wanted)
{
    if ((m_pH2Conn->isOutBufFull()) || (0 >= m_iWindowOut))
    {
        setFlag(HIO_FLAG_PAUSE_WRITE | HIO_FLAG_WANT_WRITE, 1);
        if (next() == NULL)
            m_pH2Conn->add2PriorityQue(this);
        m_pH2Conn->needWriteEvent();
        return 0;
    }

    if (wanted > m_iWindowOut)
        wanted = m_iWindowOut;
    wanted = m_pH2Conn->getAllowedDataSize(wanted);
    return wanted;
}


#define H2STREAM_MAX_BATCH_SIZE (256 * 1024)
#define H2STREAM_MAX_PKT_SIZE   (16384-9)
#define H2STREAM_MAX_PKT_SIZE2  (16384)
int H2StreamBase::writev(IOVec &vector, int total)
{
    int ret;
    int pkt_size;
    if (getState() == HIOS_DISCONNECTED)
        return LS_FAIL;
    int weight = m_pH2Conn->getWeightedPriority(this);
    LS_DBG_L(this, "H2StreamBase::writev(%d, %d), weight: %d", vector.len(),
             total, weight);
    int remain = total;
    int fin = 0;
    IOVec iov(vector);

    while(!isPauseWrite() && remain > 0)
    {
        pkt_size = getDataFrameSize(remain);
        if (pkt_size <= 0)
            break;
        if (total - remain + pkt_size >= H2STREAM_MAX_BATCH_SIZE / weight)
        {
            if (pkt_size > H2STREAM_MAX_PKT_SIZE2)
            {
                int new_pkt_size = H2STREAM_MAX_BATCH_SIZE / weight - (total - remain);
                if (new_pkt_size < H2STREAM_MAX_PKT_SIZE2)
                    pkt_size = H2STREAM_MAX_PKT_SIZE2;
                else
                    pkt_size = new_pkt_size;
            }
            LS_DBG_L(this, "H2StreamBase::write() limited by weight, pkt_size: %d", pkt_size);
            if (total - remain - pkt_size > 0)
                setFlag(HIO_FLAG_PAUSE_WRITE, 1);
        }
        ret = m_pH2Conn->sendDataFrame(getStreamID(), fin, &iov, pkt_size);
        LS_DBG_L(this, "H2StreamBase::sendData(), total: %d, ret: %d", total, ret);
        ret = dataSent(ret);
        if (ret <= 0)
            break;
        remain -= ret;
    }
    return total - remain;
}


int H2StreamBase::writev(const struct iovec *vec, int count)
{
    if (count == 1)
        return write((const char *)vec->iov_base, vec->iov_len, 0);

    IOVec iov(vec, count);
    return writev(iov, iov.bytes());
}


int H2StreamBase::write(const char *buf, int len)
{
    return write(buf, len, 0);
}


// int H2StreamBase::write(const char *buf, int len, int flag)
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
//     LS_DBG_L(this, "H2StreamBase::write(%p, %d, %d) return %d", buf, allowed, flag, ret);
//     if (flag)
//         m_pH2Conn->wantFlush();
//     return dataSent(ret);
// }


int H2StreamBase::write(const char *buf, int len, int flag)
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
    LS_DBG_L(this, "H2StreamBase::write(%p, %d, %d), weight: %d", buf, len,
             flag, weight);

    if (getBytesSent() == 0 && len > H2STREAM_MAX_PKT_SIZE
        && m_pH2Conn->getBuf()->size() > 0)
    {
        ret = m_pH2Conn->flush();
        LS_DBG_L(this, "H2StreamBase::sendfile() flush return: %d", ret);
        if (ret)
            return (ret == -1)? LS_FAIL : 0;
    }

    while(!isPauseWrite() && end - p > 0)
    {
        pkt_size = getDataFrameSize(end - p);
        if (pkt_size <= 0)
            break;
        if (p + pkt_size - buf >= H2STREAM_MAX_BATCH_SIZE / weight)
        {
            if (pkt_size > H2STREAM_MAX_PKT_SIZE2)
            {
                int new_pkt_size = H2STREAM_MAX_BATCH_SIZE / weight - (p - buf);
                if (new_pkt_size < H2STREAM_MAX_PKT_SIZE2)
                    pkt_size = H2STREAM_MAX_PKT_SIZE2;
                else
                    pkt_size = new_pkt_size;
            }
            LS_DBG_L(this, "H2StreamBase::write() limited by weight, pkt_size: %d", pkt_size);
            if (end - p - pkt_size > 0)
                setFlag(HIO_FLAG_PAUSE_WRITE, 1);
        }
//         if (pkt_size > H2STREAM_MAX_PKT_SIZE)
//              pkt_size = H2STREAM_MAX_PKT_SIZE;
        if (pkt_size >= end - p && (flag & HIO_EOR))
        {
            fin = H2_FLAG_END_STREAM;
            markShutdown();
        }
        ret = m_pH2Conn->sendDataFrame(getStreamID(), fin, p, pkt_size);
        LS_DBG_L(this, "H2StreamBase::write(%p, %d, %d) return %d", p, pkt_size, fin, ret);
        if (fin)
            m_pH2Conn->wantFlush();
        ret = dataSent(ret);
        if (ret <= 0)
            break;
        p += ret;
    }
    return (p - buf > 0)? p - buf : ret;
}


int H2StreamBase::dataSent(int ret)
{
    if (ret == -1)
    {
        setFlag(HIO_FLAG_ABORT, 1);
        return LS_FAIL;
    }

    setActiveTime(DateTime::s_curTime);
    bytesSent(ret);
    m_iWindowOut -= ret;
    LS_DBG_L(this, "Total sent: %lld, current window: %d",
                (long long)getBytesSent(), m_iWindowOut);

    if (m_iWindowOut <= 0)
        setFlag(HIO_FLAG_PAUSE_WRITE, 1);
    return ret;
}



int H2StreamBase::adjWindowOut(int32_t n)
{
    m_iWindowOut += n;
    LS_DBG_L(this, "stream WINDOW_UPDATE: %d, window size: %d.",
                n, m_iWindowOut);
    if (m_iWindowOut < 0)
    {
        //window overflow
        return LS_FAIL;
    }
    else
    {
        setFlag(HIO_FLAG_PAUSE_WRITE, 0);
        if (isWantWrite())
            continueWrite();
    }
    return 0;
}


int H2StreamBase::sendfile(int fdSrc, off_t off, size_t size, int flag)
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
    LS_DBG_L(this, "H2StreamBase::sendfile(%d, %lld, %zd, %d), weight: %d",
             fdSrc, (long long)off, size, flag, weight);

    if (getBytesSent() == 0 && size > H2STREAM_MAX_PKT_SIZE
        && m_pH2Conn->getBuf()->size() > 0)
    {
        ret = m_pH2Conn->flush();
        LS_DBG_L(this, "H2StreamBase::sendfile() flush return: %d", ret);
        if (ret)
            return (ret == -1)? LS_FAIL : 0;
    }

    while(!isPauseWrite() && end - cur > 0)
    {
        if (end - cur < 1024 && cur - off > H2STREAM_MAX_PKT_SIZE)
            break;
        pkt_size = getDataFrameSize(end - cur);
        if (pkt_size <= 0)
            break;
        if (cur + pkt_size - off >= H2STREAM_MAX_BATCH_SIZE / weight)
        {
            if (pkt_size > H2STREAM_MAX_PKT_SIZE)
            {
                int new_pkt_size = H2STREAM_MAX_BATCH_SIZE / weight - (cur - off);
                if (new_pkt_size < H2STREAM_MAX_PKT_SIZE)
                    pkt_size = H2STREAM_MAX_PKT_SIZE;
                else
                    pkt_size = new_pkt_size;
            }
            LS_DBG_L(this, "H2StreamBase::sendfile() limited by weight, pkt_size: %d", pkt_size);
            if (end - cur - pkt_size > 0)
                setFlag(HIO_FLAG_PAUSE_WRITE, 1);
        }
        if (pkt_size > H2STREAM_MAX_PKT_SIZE)
             pkt_size = H2STREAM_MAX_PKT_SIZE;
        if (pkt_size >= end - cur && (flag & HIO_EOR))
        {
            fin = H2_FLAG_END_STREAM;
            markShutdown();
        }
        ret = m_pH2Conn->sendfileDataFrame(getStreamID(), fin, fdSrc, cur, pkt_size);
        LS_DBG_L(this, "H2StreamBase::sendfileDataFrame(%d, %lld, %d, %d) return %d",
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

