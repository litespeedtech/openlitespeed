/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#include "h2connection.h"
#include <http/clientinfo.h>
#include <http/hiohandlerfactory.h>
#include <http/httpheader.h>
#include <http/httprespheaders.h>
#include <http/httpstatuscode.h>
#include <http/httpserverconfig.h>
#include <http/ls_http_header.h>
#include <h2/h2stream.h>
#include <h2/h2streampool.h>
#include <h2/unpackedheaders.h>
#include <log4cxx/logger.h>
#include <util/iovec.h>
#include <util/datetime.h>
#include <lsdef.h>

#include <ctype.h>
#include "h2streambase.h"

static const char s_h2sUpgradeResponse[] =
    "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nUpgrade: h2c\r\n\r\n";

#define MAX_CONTROL_FRAMES_RATE 1000
#define MAX_OUT_BUF_SIZE        65536


static inline void appendNbo4Bytes(char *pBuf, uint32_t val)
{
    *pBuf++ = val >> 24;
    *pBuf++ = ((val >> 16) & 0xff);
    *pBuf++ = ((val >> 8) & 0xff);
    *pBuf++ = (val & 0xff);
}


// TODO: compiler warning: unused function
// static inline void append4Bytes(LoopBuf *pBuf, const char *val)
// {
//     pBuf->append(*val++);
//     pBuf->append(*val++);
//     pBuf->append(*val++);
//     pBuf->append(*val);
// }


H2Connection::H2Connection()
{
}


HioHandler *H2Connection::get()
{
    H2Connection *pConn = new H2Connection();
    if (NULL == pConn->m_hpack_enc.hpe_buckets)
    {
        delete pConn;
        return NULL;
    }
    return pConn;
}


int H2Connection::onInitConnected()
{
    if (getStream()->isWriteBuffer())
        set_h2flag(H2_CONN_FLAG_DIRECT_BUF);
    setOS(getStream());
    getStream()->continueRead();
    return 0;
}


H2Connection::~H2Connection()
{
}


int H2Connection::onReadEx()
{
    int ret;
    clr_h2flag(H2_CONN_FLAG_WAIT_PROCESS | H2_CONN_FLAG_PENDING_STREAM);
    set_h2flag(H2_CONN_FLAG_IN_EVENT);
    ret = onReadEx2();
    if (isPauseWrite() && (m_h2flag & H2_CONN_FLAG_PENDING_STREAM))
        processPendingStreams();
    if ((m_h2flag & H2_CONN_FLAG_WAIT_PROCESS) != 0)
        onWriteEx2();
    clr_h2flag(H2_CONN_FLAG_IN_EVENT);
    //if (getBuf()->size() > 0)
    if (m_h2flag & H2_CONN_FLAG_WANT_FLUSH)
        flush();
    return ret;
}


int H2Connection::decodeHeaders(uint32_t id, unsigned char *pSrc, int length,
                                unsigned char iHeaderFlag)
{
    H2Stream *pStream;
    pStream = (H2Stream *)getNewStream(iHeaderFlag);
    if (!pStream)
    {
        sendRstFrame(m_uiLastStreamId, H2_ERROR_PROTOCOL_ERROR);
        return 0;
    }
    UnpackedHeaders *headers = new UnpackedHeaders();
    UpkdHdrBuilder builder(headers, false);
    unsigned char *bufEnd = pSrc + length;
    int rc = decodeReqHdrData(pStream, pSrc, bufEnd, &builder);
    if (rc == LSXPACK_OK)
        pStream->setReqHeaders(builder.retrieveHeaders());
    else
    {
        LS_DBG_L(log_s(), "decodeData() failure, return %d", rc);
        doGoAway(H2_ERROR_COMPRESSION_ERROR);
        return LS_FAIL;
    }

    if ( log4cxx::Level::isEnabled( log4cxx::Level::DBG_HIGH ) )
    {
        LS_DBG_H(log_s(), "decodeHeaders():\r\n%.*s",
                 headers->getBuf()->size() - 4,
                 headers->getBuf()->begin() + 4);
    }

    pStream->setFlag(HIO_FLAG_INIT_SESS, 1);
    if (pStream->getFlag(HIO_FLAG_PEER_SHUTDOWN))
    {
        add2PriorityQue(pStream);
        set_h2flag(H2_CONN_FLAG_PENDING_STREAM);
    }
    else
    {
        assignStreamHandler(pStream);
        if (pStream->isWantWrite() && (pStream->getWindowOut() > 0))
        {
            if (!pStream->next())
                m_priQue[pStream->getPriority()].append(pStream);
        }
        if (pStream->getState() != HIOS_CONNECTED)
            recycleStream(pStream);
    }
    return 0;
}


int H2Connection::decodeReqHdrData(H2Stream *stream, const unsigned char *pSrc,
            const unsigned char *bufEnd, UpkdHdrBuilder *builder)
{
    int rc;
    lsxpack_err_code lsxpack_rc;
    lsxpack_header_t *hdr = NULL;
    size_t mini_space = 0;

    while (pSrc < bufEnd)
    {
        hdr = builder->prepareDecode(hdr, mini_space);
        if (!hdr)
            return LS_FAIL;
        rc = lshpack_dec_decode(&m_hpack_dec, &pSrc, bufEnd, hdr);
        if (rc < 0)
        {
            if (rc == LSHPACK_ERR_MORE_BUF)
            {
                mini_space = hdr->val_len;
                LS_DBG_L(stream, "HPACK decoder need more buffer: %zd.", mini_space);
                continue;
            }
            return LS_FAIL;
        }
        DUMP_LSXPACK(stream, hdr, "HPACK decode");
        lsxpack_rc = builder->process(hdr);
        if (lsxpack_rc != LSXPACK_OK)
            return lsxpack_rc;
        hdr = NULL;
    }

    return builder->end();
}


H2StreamBase *H2Connection::getNewStream(uint8_t ubH2_Flags)
{
    H2Stream *pStream;
    LS_DBG_H(log_s()->getLogger(),
             "[%s-%u] getNewStream(), stream map size: %u, push stream: %d, shutdown streams: %d, flag: %d",
             log_s()->getLogId(), m_uiLastStreamId, m_mapStream.size(),
             m_iCurPushStreams,
             m_uiShutdownStreams, (int)ubH2_Flags);

    if ((int)m_mapStream.size() - (int)m_uiShutdownStreams - m_iCurPushStreams
        >= m_iServerMaxStreams)
        return NULL;


    //pStream = new H2Stream();
    pStream = H2StreamPool::getH2Stream();
    ++m_uiStreams;
    pStream->set_key(m_uiLastStreamId);
    m_mapStream.insert(pStream);
    if (m_tmIdleBegin)
        m_tmIdleBegin = 0;
    enum stream_flag flag = (enum stream_flag)(ubH2_Flags & H2_CTRL_FLAG_FIN)
            | HIO_FLAG_FLOWCTRL | HIO_FLAG_SENDFILE | HIO_FLAG_WRITE_BUFFER;
    if (!(m_h2flag & H2_CONN_FLAG_NO_PUSH))
        flag = flag | HIO_FLAG_PUSH_CAPABLE;
    if (getStream()->getFlag(HIO_FLAG_ALTSVC_SENT))
        flag = flag | HIO_FLAG_ALTSVC_SENT;
    pStream->setFlag(flag, 1);
    pStream->init(this, &m_priority);
    pStream->setConnInfo(getStream()->getConnInfo());
    return pStream;
}


int H2Connection::h2cUpgrade(HioHandler *pSession, const char * pBuf, int size)
{
    if (pBuf)
    {
        m_bufInput.append(pBuf, size);
        onInitConnected();
        return processInput();
    }
    else
    {
        assert(pSession != NULL);
        H2Stream *pStream;
        //pStream = new H2Stream();
        pStream = H2StreamPool::getH2Stream();

        pStream->set_key(1); //Through upgrade h2c, it is 1.
        m_mapStream.insert(pStream);
        enum stream_flag flag = HIO_FLAG_FLOWCTRL | HIO_FLAG_WRITE_BUFFER;
        if (!(m_h2flag & H2_CONN_FLAG_NO_PUSH))
            flag = flag | HIO_FLAG_PUSH_CAPABLE;
        if (getStream()->getFlag(HIO_FLAG_ALTSVC_SENT))
            flag = flag | HIO_FLAG_ALTSVC_SENT;
        pStream->setFlag(flag, 1);
        pStream->init(this, NULL);
        onInitConnected();
        guaranteeOutput(s_h2sUpgradeResponse, sizeof(s_h2sUpgradeResponse) - 1);
        sendSettingsFrame(false);
        pStream->setFlag(HIO_FLAG_INIT_SESS, 0);
        pSession->attachStream(pStream);
        return pStream->onInitConnected();
    }
}


void H2Connection::recycleStream(H2StreamBase *stream)
{
    if ((stream->getStreamID() & 1) == 0)
        --m_iCurPushStreams;

    if (m_current == stream)
        m_current = NULL;

    m_mapStream.erase(stream);
    stream->closeEx();
    m_priQue[stream->getPriority()].remove(stream);
    if (((H2Stream *)stream)->getHandler())
        ((H2Stream *)stream)->getHandler()->recycle();

    LS_DBG_H(stream, "recycleStream(), stream map size: %u "
             , m_mapStream.size());
    H2StreamPool::recycle( (H2Stream *)stream );
    //delete stream;
}


int H2Connection::flush()
{
    closePendingOut();
    BufferedOS::flush();
    if (!isEmpty())
    {
        getStream()->continueWrite();
        getStream()->setFlag(HIO_FLAG_PAUSE_WRITE, 1);
    }
    else
        clr_h2flag(H2_CONN_FLAG_WANT_FLUSH);
    return getStream()->flush();
}


int H2Connection::onCloseEx()
{
    if (getStream()->isReadyToRelease())
        return 0;
    LS_DBG_L(log_s(), "H2Connection::onCloseEx() ");

    getStream()->tobeClosed();
    releaseAllStream();
    getStream()->handlerReadyToRelease();
    return 0;
};


int H2Connection::onTimerEx()
{
    int result = 0;
    if (m_h2flag & H2_CONN_FLAG_GOAWAY)
    {
        result = releaseAllStream();
        getStream()->handlerReadyToRelease();
    }
    else
    {
        result = timerRoutine();
        int idle = DateTime::s_curTime - getStream()->getActiveTime();
        if (!isEmpty() && idle > 20)
        {
            LS_DBG_L(log_s(), "write() timeout.");
            doGoAway(H2_ERROR_PROTOCOL_ERROR);
        }
        else if (idle > 10 && m_mapStream.size() == 0)
        {
            m_uiStreams = 0;
            m_uiRstStreams = 0;
        }
    }
    return result;
}


int H2Connection::doGoAway(H2ErrorCode status)
{
    LS_DBG_L(log_s(), "H2Connection::doGoAway(), status = %d ",
             status);
    releaseAllStream();
    sendGoAwayFrame(status);
    getStream()->handlerReadyToRelease();
    getStream()->tobeClosed();
    if ((m_h2flag & H2_CONN_FLAG_IN_EVENT) == 0)
        getStream()->continueWrite();
    return 0;
}


void H2Connection::disableHttp2ByIp()
{
    getStream()->getClientInfo()->setFlag(CIF_DISABLE_HTTP2);
}


#define H2_TMP_HDR_BUFF_SIZE 65536

int H2Connection::encodeRespHeaders(H2Stream* stream, HttpRespHeaders* hdrs,
                                    unsigned char* buf, int maxSize)
{
    unsigned char *pCur = buf;
    unsigned char *pBufEnd = pCur + maxSize;

    lsxpack_header_t *hdr;

    hdrs->prepareSendHpack();
    for(hdr = hdrs->begin(); hdr < hdrs->end(); ++hdr)
    {
        if (hdr->buf)
        {
            DUMP_LSXPACK(stream, hdr, "HPACK encode");
            pCur = lshpack_enc_encode(&m_hpack_enc, pCur, pBufEnd, hdr);
        }
    }

    return pCur - buf;
}


int H2Connection::sendRespHeaders(H2Stream *stream, HttpRespHeaders *hdrs,
                                  uint8_t flag)
{
    LS_DBG_H(stream, "sendRespHeaders()");

    unsigned char achHdrBuf[16 + H2_TMP_HDR_BUFF_SIZE];
    int rc = encodeRespHeaders(stream, hdrs, &achHdrBuf[16], H2_TMP_HDR_BUFF_SIZE);
    if (rc < 0)
        return LS_FAIL;
    return sendHeaderContFrame(stream->getStreamID(), flag, H2_FRAME_HEADERS,
                               (char *)&achHdrBuf[16], rc);
}



H2StreamBase* H2Connection::createPushStream(uint32_t pushStreamId,
                                             UnpackedHeaders *hdrs)
{
    H2Stream *pStream;
    //pStream = new H2Stream();
    pStream = H2StreamPool::getH2Stream();
    if (!pStream)
        return NULL;
    pStream->set_key(pushStreamId);
    m_mapStream.insert(pStream);
    if (m_tmIdleBegin)
        m_tmIdleBegin = 0;
    enum stream_flag flag = HIO_FLAG_PEER_SHUTDOWN | HIO_FLAG_FLOWCTRL
                    | HIO_FLAG_INIT_SESS | HIO_FLAG_IS_PUSH
                    | HIO_FLAG_WRITE_BUFFER
                    | HIO_FLAG_WANT_WRITE | HIO_FLAG_ALTSVC_SENT;

    pStream->setFlag(flag, 1);
    pStream->init(this, NULL);
    pStream->setConnInfo(getStream()->getConnInfo());
    pStream->setPriority(HIO_PRIORITY_PUSH);
    pStream->setReqHeaders(hdrs);
    add2PriorityQue(pStream);
    ++m_iCurPushStreams;
    return pStream;
}



int H2Connection::pushPromise(uint32_t streamId, UnpackedHeaders *hdrs)
{
    if (m_iCurPushStreams >= m_iMaxPushStreams)
    {
        LS_DBG_L(log_s(),
                 "reached max concurrent Server PUSH streams limit, skip push.");
        return -1;
    }
    uint32_t pushStreamId = m_uiPushStreamId;
    m_uiPushStreamId += 2;
    int ret = sendReqHeaders(streamId, pushStreamId, H2_CTRL_FLAG_FIN, hdrs);
    if (ret == 0)
    {
        if (createPushStream(pushStreamId, hdrs) == NULL)
        {
            (void) sendRstFrame(pushStreamId, H2_ERROR_INTERNAL_ERROR);
            ret = -1;
        }
    }
    return ret;
}


int H2Connection::assignStreamHandler(H2StreamBase *s)
{
    H2Stream *stream = (H2Stream *)s;
    assert(stream->getHandler() == NULL);
    HioHandler *pHandler = HioHandlerFactory::getHandler(HIOS_PROTO_HTTP);
    if (!pHandler)
    {
        resetStream(0, stream, H2_ERROR_INTERNAL_ERROR);
        return LS_FAIL;
    }
    pHandler->attachStream(stream);
    return stream->onInitConnected();
}


int H2Connection::onWriteEx2()
{
    int buffered;

    LS_DBG_H(log_s(),
             "onWriteEx2() output buffer size=%d, Data Out Window: %d",
             getBuf()->size(), m_iCurDataOutWindow);
    if ((buffered = getBuf()->size()) > 0)
    {
        if (buffered >= 1369)
        {
            flush();
            if (!isEmpty())
                return 1;
        }
        else
            set_h2flag(H2_CONN_FLAG_WANT_FLUSH);
    }
    if (getStream()->isPauseWrite())
    {
        if (!getStream()->isWantWrite() && m_iCurDataOutWindow > 0)
            getStream()->continueWrite();
        return 1;
    }
    int wantWrite = processQueue();
    if (wantWrite && !getStream()->isWantWrite() && m_iCurDataOutWindow > 0)
        getStream()->continueWrite();

    return wantWrite;
}


int H2Connection::onWriteEx()
{
    set_h2flag(H2_CONN_FLAG_IN_EVENT);
    int wantWrite = onWriteEx2();
    clr_h2flag(H2_CONN_FLAG_IN_EVENT);

    if ((wantWrite == 0 || m_iCurDataOutWindow <= 0) && isEmpty())
    {
        getStream()->suspendWrite();
        if (m_h2flag & H2_CONN_FLAG_PAUSE_READ)
        {
            clr_h2flag(H2_CONN_FLAG_PAUSE_READ);
            getStream()->continueRead();
        }
    }
    return 0;
}



void H2Connection::recycle()
{
    LS_DBG_H(log_s(), "H2Connection::recycle()");
    if (m_mapStream.size() > 0)
    {
        releaseAllStream();
        getStream()->handlerReadyToRelease();
    }
    detachStream();
    delete this;
}


ssize_t H2Connection::directBuffer(const char *data, size_t size)
{
    return getStream()->bufferedWrite(data, size);
}


int H2Connection::appendSendfileOutput(int fd, off_t off, int size)
{
    int ret;
    int remain = size;
    if (getBuf()->size() == 0 && getStream()->isSendfileAvail())
    {
        ret = getStream()->sendfile(fd, off, size, 0);
        LS_DBG_H(log_s(), "stream->sendfile(%d, %lld, %d) return %d",
                fd, (long long)off, size, ret);
        if (ret >= size)
            return size;
        if (ret > 0)
        {
            off += ret;
            remain = size - ret;
        }
    }

    getBuf()->guarantee(remain);
    int avail = getBuf()->contiguous();
    if (avail > remain)
        avail = remain;
    ret = pread(fd, getBuf()->end(), avail, off);
    if (ret > 0)
    {
        getBuf()->used(ret);
        remain -= ret;
        if (remain > 0)
        {
            if (ret != avail)
                return -1;
            off += ret;
            ret = pread(fd, getBuf()->end(), remain, off);
            if (ret > 0)
            {
                getBuf()->used(ret);
                remain -= ret;
            }
        }
    }
    if (remain > 0)
    {
        return -1;
    }

    LS_DBG_H(log_s(), "[H2] buffer %d, total buffer size = %d", remain,
             getBuf()->size());
    if (getBuf()->size() >= 16384)
        BufferedOS::flush();
    return size;
}


int H2Connection::verifyStreamId(uint32_t id)
{
    if (id == 0)
    {
        LS_INFO(log_s(), "Protocol error, HEADER frame STREAM ID is 0");
        return LS_FAIL;
    }

    if ((id & 1) == 0)
    {
        LS_DBG_L(log_s(), "invalid HEADER frame id %d ",
                    id);
        return LS_FAIL;
    }

    if (id <= m_uiLastStreamId)
    {
        H2StreamBase *pStream = findStream(id);
        if (pStream )
        {
            H2ErrorCode ret = H2_ERROR_PROTOCOL_ERROR;
            if (pStream->isPeerShutdown())
                ret = H2_ERROR_STREAM_CLOSED;
            resetStream(id, pStream, ret);
        }
        LS_INFO(log_s(), "Protocol error, STREAM ID: %d is less the"
                " previously received stream ID: %d, go away!",
                id, m_uiLastStreamId);
        return LS_FAIL;
    }
    m_uiLastStreamId = id;
    return LS_OK;
}

