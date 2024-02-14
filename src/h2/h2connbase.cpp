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
#include "h2connbase.h"
#include <http/httpheader.h>
#include <http/httprespheaders.h>
#include <http/httpstatuscode.h>
#include <http/httpserverconfig.h>
#include <http/ls_http_header.h>
#include <h2/h2protocol.h>
#include <h2/h2stream.h>
#include <h2/h2streampool.h>
#include <h2/unpackedheaders.h>
#include <log4cxx/logger.h>
#include <util/iovec.h>
#include <util/datetime.h>
#include <lsdef.h>

#include <ctype.h>

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


H2ConnBase::H2ConnBase()
    : m_bufInput(1024)
    , m_mapStream(64)
    , m_iCurrentFrameRemain(-H2_FRAME_HEADER_SIZE)
    , m_iCurDataOutWindow(H2_FCW_INIT_SIZE)
    , m_iDataInWindow(H2_FCW_INIT_SIZE)
    , m_iStreamInInitWindowSize(H2_FCW_64K * 16 * 8)
    , m_iServerMaxStreams(100)
    , m_iStreamOutInitWindowSize(H2_FCW_INIT_SIZE)
    , m_iMaxPushStreams(100)
    , m_iPeerMaxFrameSize(H2_DEFAULT_DATAFRAME_SIZE)
    , m_uiPushStreamId(2)
{
    LS_ZERO_FILL(m_uiLastStreamId, m_curH2Header);
    lshpack_dec_init(&m_hpack_dec);
    lshpack_enc_init(&m_hpack_enc);
    lshpack_enc_use_hist(&m_hpack_enc, 1);
}



int H2ConnBase::init()
{
    m_iCurDataOutWindow = H2_FCW_INIT_SIZE;
    m_uiLastStreamId = 0;
    m_current = NULL;
    m_iStreamOutInitWindowSize = H2_FCW_INIT_SIZE;
    m_iServerMaxStreams = 100;
    m_iMaxPushStreams = 100;
    m_tmIdleBegin = 0;
    m_uiShutdownStreams = 0;
    m_iCurPushStreams = 0;
    m_iCurrentFrameRemain = -H2_FRAME_HEADER_SIZE;
    return 0;
}


H2ConnBase::~H2ConnBase()
{
    lshpack_enc_cleanup(&m_hpack_enc);
    lshpack_dec_cleanup(&m_hpack_dec);
}


int H2ConnBase::verifyClientPreface()
{
    char sPreface[H2_CLIENT_PREFACE_LEN];
    m_bufInput.moveTo(sPreface, H2_CLIENT_PREFACE_LEN);
    if (memcmp(sPreface, H2_CLIENT_PREFACE, H2_CLIENT_PREFACE_LEN) != 0)
    {
        LS_DBG_L(getLogSession(), "missing client PREFACE string,"
                 " got [%.24s], close.", sPreface);
        return LS_FAIL;
    }
    set_h2flag(H2_CONN_FLAG_PREFACE);
    return 0;
}


int H2ConnBase::parseFrame()
{
    int ret;
    while (!m_bufInput.empty())
    {
        if (m_iCurrentFrameRemain < 0)
        {
            if (m_bufInput.size() < H2_FRAME_HEADER_SIZE)
                break;
            m_bufInput.moveTo((char *)&m_curH2Header, H2_FRAME_HEADER_SIZE);

            m_iCurrentFrameRemain = m_curH2Header.getLength();
            LS_DBG_L(getLogSession(), "frame type %d, size: %d, flag: 0x%hhx",
                     m_curH2Header.getType(), m_iCurrentFrameRemain,
                     m_curH2Header.getFlags());
            if (m_iCurrentFrameRemain > H2_DEFAULT_DATAFRAME_SIZE)
            {
                LS_DBG_L(getLogSession(), "Frame size is too large, "
                         "Max: %d, Current Frame Size: %d.",
                         H2_DEFAULT_DATAFRAME_SIZE, m_iCurrentFrameRemain);
                return H2_ERROR_FRAME_SIZE_ERROR;
            }
            //if (m_curH2Header.getType() != H2_FRAME_DATA)
            //    m_inputState = ST_CONTROL_FRAME;
        }
        if (!(m_h2flag & H2_CONN_FLAG_SETTING_RCVD))
        {
            if (m_curH2Header.getType() != H2_FRAME_SETTINGS)
            {
                LS_DBG_L(getLogSession(), "expects SETTINGS frame "
                         "as part of client PREFACE, got frame type %d",
                         m_curH2Header.getType());
                return LS_FAIL;
            }
        }

        if ((m_h2flag & H2_CONN_HEADERS_START) == H2_CONN_HEADERS_START
            && m_curH2Header.getType() != H2_FRAME_CONTINUATION)
            return LS_FAIL;

        if (m_curH2Header.getType() != H2_FRAME_DATA)
        {
            if (m_iCurrentFrameRemain > m_bufInput.size())
                break;
            if ((ret = processFrame(&m_curH2Header)) != LS_OK)
                return ret;
            if (m_iCurrentFrameRemain > 0)
                m_bufInput.pop_front(m_iCurrentFrameRemain);
            m_iCurrentFrameRemain = -H2_FRAME_HEADER_SIZE;
        }
        else
        {
            if ((m_curH2Header.getFlags() & H2_FLAG_PADDED)
                && m_bufInput.size() < 1)
                break;
            assert((int)m_curH2Header.getLength() == m_iCurrentFrameRemain);
            consumedWindowIn(m_iCurrentFrameRemain);
            if ((ret = processDataFrame(&m_curH2Header)) != LS_OK)
                return ret;
            if (m_iCurrentFrameRemain == 0)
            {
                m_inputState = ST_CONTROL_FRAME;
                m_iCurrentFrameRemain = -H2_FRAME_HEADER_SIZE;
            }
        }
    }
    return 0;
}


void H2ConnBase::consumedWindowIn(int bytes)
{
    m_iInBytesToUpdate += bytes;
    LS_DBG_L(getLogSession(), "WINDOW_IN consumed: %d/%d.",
             m_iInBytesToUpdate, m_iDataInWindow);
    if (m_iDataInWindow / 2 < m_iInBytesToUpdate)
        updateWindow();
}


void H2ConnBase::updateWindow()
{
    if (m_iInBytesToUpdate >= m_iDataInWindow)
    {
        LS_DBG_L(getLogSession(), "Connection RECV window used up, increase window size by %d to %d.",
                m_iDataInWindow / 2, m_iDataInWindow / 2 + m_iDataInWindow);
        m_iInBytesToUpdate += m_iDataInWindow / 2;
        m_iDataInWindow += m_iDataInWindow / 2;
    }
    else
        LS_DBG_L(getLogSession(), "Send connection WINDOW_UPDATE, used: %d/%d.",
                m_iInBytesToUpdate, m_iDataInWindow);
    sendFrame4Bytes(H2_FRAME_WINDOW_UPDATE, 0, m_iInBytesToUpdate, true);
    m_iInBytesToUpdate = 0;
}


int H2ConnBase::processInput()
{
    int ret;
    if (!(m_h2flag & H2_CONN_FLAG_PREFACE))
    {
        if (m_bufInput.size() < H2_CLIENT_PREFACE_LEN)
            return 0;
        if (verifyClientPreface() == LS_FAIL)
        {
            suspendRead();
            onCloseEx();
            return LS_FAIL;
        }
    }
    if ((ret = parseFrame()) != LS_OK)
    {
        H2ErrorCode err;
        if ((ret < 0)||(ret >= H2_ERROR_CODE_COUNT))
            err = H2_ERROR_PROTOCOL_ERROR;
        else
            err = (H2ErrorCode)ret;
        doGoAway(err);
        return LS_FAIL;
    }
    return 0;
}


#define MAX_INPUT_BATCH_SIZE   (256 * 1024)
int H2ConnBase::onReadEx2()
{
    int total = 0;
    int ret;
    while (true)
    {
        if (m_inputState == ST_CONTROL_FRAME)
        {
            if (m_iCurrentFrameRemain >= 0
                && m_curH2Header.getType() != H2_FRAME_DATA
                && m_bufInput.available() < m_iCurrentFrameRemain)
                m_bufInput.guarantee(m_iCurrentFrameRemain + 9);
            int availLen = m_bufInput.contiguous();
            int n = getInStream()->read(m_bufInput.end(), availLen);
            LS_DBG_L(getLogSession(), "H2ConnBase::onReadEx2 read(%d) return %d", availLen, n);

            if (n == -1)
                break;
            else if (n == 0)
                return total;
            m_bufInput.used(n);
            total += n;
            if (processInput() == LS_FAIL)
                return LS_FAIL;
        }
        else
        {
            if (m_current == NULL)
                m_inputState = ST_SKIP_REMAIN;
            else
                assert(m_current == findStream(m_curH2Header.getStreamId()));

            if (m_inputState == ST_DATA_PAYLOAD)
                ret = processDataFrameDirectBuffer(&m_curH2Header);
            else if (m_inputState == ST_SKIP_REMAIN)
                ret = processDataFrameSkipRemain(&m_curH2Header);
            else
            {
                ret = -1;
                assert(false);
            }
            if (ret > 0)
            {
                total += ret;
                continue;
            }
            if (ret == 0)
                return total;
            else
                break;
        }
        if (total > MAX_INPUT_BATCH_SIZE)
        {
            if (getBuf()->size() >= MAX_OUT_BUF_SIZE)
            {
                set_h2flag(H2_CONN_FLAG_PAUSE_READ);
                suspendRead();
            }
            if (m_bufInput.empty() && m_bufInput.capacity() > 4096)
                m_bufInput.reserve(4096);
            return 0;
        }
    }
    suspendRead();
    onCloseEx();
    return LS_FAIL;
}


int H2ConnBase::processFrame(H2FrameHeader *pHeader)
{
    static uint32_t requiredLength[11] =
    {  0, 0, 5, 4, 0, 0, 8, 0, 4, 0, 0 };

    m_tmLastFrameIn = DateTime::s_curTime;

    int type = pHeader->getType();
    if (type < H2_FRAME_MAX_TYPE)
    {
        if ((requiredLength[type] != 0)
            && (requiredLength[type] != pHeader->getLength()))
        {
            return H2_ERROR_FRAME_SIZE_ERROR;
        }
    }
    printLogMsg(pHeader);

    switch (type)
    {
    case H2_FRAME_HEADERS:
        return processHeadersFrame(pHeader);
    case H2_FRAME_PRIORITY:
        return processPriorityFrame(pHeader);
    case H2_FRAME_RST_STREAM:
        return processRstFrame(pHeader);
    case H2_FRAME_SETTINGS:
        return processSettingFrame(pHeader);
    case H2_FRAME_PUSH_PROMISE:
        return processPushPromiseFrame(pHeader);
    case H2_FRAME_GOAWAY:
        processGoAwayFrame(pHeader);
        break;
    case H2_FRAME_WINDOW_UPDATE:
        return processWindowUpdateFrame(pHeader);
    case H2_FRAME_CONTINUATION:
        return processContinuationFrame(pHeader);
    case H2_FRAME_PING:
        return processPingFrame(pHeader);
    case H2_FRAME_GREASE0:
    case H2_FRAME_GREASE1:
    case H2_FRAME_GREASE2:
    case H2_FRAME_GREASE3:
    case H2_FRAME_GREASE4:
    case H2_FRAME_GREASE5:
    case H2_FRAME_GREASE6:
    case H2_FRAME_GREASE7:
    default:
        break;
    }
    return 0;
}


void H2ConnBase::printLogMsg(H2FrameHeader *pHeader)
{
    if (LS_LOG_ENABLED(log4cxx::Level::DBG_LOW))
    {
        const char *message = "";
        int messageLen = 0;
        if (pHeader->getType() == H2_FRAME_GOAWAY)
        {
            message = (const char *)m_bufInput.begin() + 8;
            messageLen = pHeader->getLength() - 8;
        }
        LS_DBG_L(getLogSession()->getLogger(),
               "[%s-%d] Received %s, size: %d, flag: 0x%hhx, Message: '%.*s'\n",
               getLogSession()->getLogId(), pHeader->getStreamId(),
               getH2FrameName(pHeader->getType()), pHeader->getLength(),
               pHeader->getFlags(), messageLen, message);
    }
}


int H2ConnBase::processPriorityFrame(H2FrameHeader *pHeader)
{
    if (pHeader->getStreamId() == 0)
    {
        LS_DBG_L(getLogSession(), "bad PRIORITY frame, stream ID is zero.");
        return LS_FAIL;

    }
    if (processPriority(pHeader->getStreamId()))
        return H2_ERROR_PROTOCOL_ERROR;

    return 0;
}


int H2ConnBase::processPushPromiseFrame(H2FrameHeader *pHeader)
{
    doGoAway(H2_ERROR_PROTOCOL_ERROR);
    return 0;
}


static inline uint32_t beReadUint32(const unsigned char *p)
{
    uint32_t v = *p++;
    v = (v << 8) | *p++;
    v = (v << 8) | *p++;
    v = (v << 8) | *p;
    return v ;
}


int H2ConnBase::processSettingFrame(H2FrameHeader *pHeader)
{
    //process each setting ID/value pair
    static const char *cpEntryNames[] =
    {
        "",
        "SETTINGS_HEADER_TABLE_SIZE",
        "SETTINGS_ENABLE_PUSH",
        "SETTINGS_MAX_CONCURRENT_STREAMS",
        "SETTINGS_INITIAL_WINDOW_SIZE",
        "SETTINGS_MAX_FRAME_SIZE",
        "SETTINGS_MAX_HEADER_LIST_SIZE"
    };
    uint32_t iEntryID, iEntryValue;
    if (pHeader->getStreamId() != 0)
    {
        LS_DBG_L(getLogSession(),
                 "bad SETTINGS frame, stream ID must be zero.");
        doGoAway(H2_ERROR_PROTOCOL_ERROR);
        return 0;

    }

    int iEntries = m_iCurrentFrameRemain / 6;
    if (m_iCurrentFrameRemain % 6 != 0)
    {
        LS_DBG_L(getLogSession(),
                 "bad SETTINGS frame, frame size does not match.");
        doGoAway(H2_ERROR_FRAME_SIZE_ERROR);
        return 0;
    }

    if (pHeader->getFlags() & H2_FLAG_ACK)
    {
        if (iEntries != 0)
        {
            LS_DBG_L(getLogSession(),
                     "bad SETTINGS frame with ACK flag, has %d entries.",
                     iEntries);
            return LS_FAIL;
        }
        if (m_h2flag & H2_CONN_FLAG_SETTING_SENT)
        {
            set_h2flag(H2_CONN_FLAG_CONFIRMED);
            return 0;
        }
        else
        {
            LS_DBG_L(getLogSession(),
                     "received SETTINGS frame with ACK flag before sending "
                     "our SETTINGS frame.");
            return LS_FAIL;
        }
    }

    int32_t   windowsSizeDiff = 0;
    unsigned char p[6];
    for (int i = 0; i < iEntries; i++)
    {
        m_bufInput.moveTo((char *)p, 6);
        iEntryID = ((uint16_t)p[0] << 8) | p[1];
        iEntryValue = beReadUint32(&p[2]);
        LS_DBG_L(getLogSession(), "%s(%d) value: %d",
                 (iEntryID < 7) ? cpEntryNames[iEntryID] :
                    ((iEntryID & 0xa0a) == 0xa0a) ? "GREASE" : "INVALID", iEntryID,
                 iEntryValue);
        switch (iEntryID)
        {
        case H2_SETTINGS_HEADER_TABLE_SIZE:
            if (iEntryValue > MAX_HEADER_TABLE_SIZE)
            {
                LS_DBG_L(getLogSession(), "HEADER_TABLE_SIZE value is invalid, %d.",
                         iEntryValue);
                return LS_FAIL;
            }
            lshpack_dec_set_max_capacity(&m_hpack_dec, iEntryValue);
            break;
        case H2_SETTINGS_MAX_FRAME_SIZE:
            if ((iEntryValue < H2_DEFAULT_DATAFRAME_SIZE) ||
                (iEntryValue > H2_MAX_DATAFRAM_SIZE))
            {
                LS_DBG_L(getLogSession(), "MAX_FRAME_SIZE value is invalid, %d.",
                         iEntryValue);
                return LS_FAIL;
            }
            m_iPeerMaxFrameSize = iEntryValue;
            break;
        case H2_SETTINGS_INITIAL_WINDOW_SIZE:
            if (iEntryValue > H2_FCW_MAX_SIZE)
            {
                doGoAway(H2_ERROR_FLOW_CONTROL_ERROR);
                return 0;
            }
//             if (iEntryValue < H2_FCW_MIN_SIZE)
//             {
//                 LS_WARN(getLogSession(), "SETTINGS_INITIAL_WINDOW_SIZE value is too low, %d,"
//                         " possible Slow Read Attack. Close connection.",
//                          iEntryValue);
//                 doGoAway(H2_ERROR_FLOW_CONTROL_ERROR);
//                 return 0;
//             }
            windowsSizeDiff = (int32_t)iEntryValue - m_iStreamOutInitWindowSize;
            m_iStreamOutInitWindowSize = iEntryValue ;
            break;
        case H2_SETTINGS_MAX_CONCURRENT_STREAMS:
            m_iMaxPushStreams = iEntryValue ;
            if (m_iMaxPushStreams == 0)
                set_h2flag(H2_CONN_FLAG_NO_PUSH);
            break;
        case H2_SETTINGS_MAX_HEADER_LIST_SIZE:
            break;
        case H2_SETTINGS_ENABLE_PUSH:
            if (iEntryValue > 1)
                return LS_FAIL;
            if (!iEntryValue)
            {
                m_iMaxPushStreams = 0;
                set_h2flag(H2_CONN_FLAG_NO_PUSH);
            }
            break;
        default:
            break;
        }
    }
    m_iCurrentFrameRemain = 0;
    set_h2flag(H2_CONN_FLAG_SETTING_RCVD);

    if (++m_iControlFrames > MAX_CONTROL_FRAMES_RATE)
    {
        LS_INFO(getLogSession(), "SETTINGS frame abuse detected, close connection.");
        return LS_FAIL;
    }
    sendSettingsFrame(false);
    sendFrame0Bytes(H2_FRAME_SETTINGS, H2_FLAG_ACK, 0);

    if (windowsSizeDiff != 0)
    {
        StreamMap::iterator itn, it = m_mapStream.begin();
        for (; it != m_mapStream.end();)
        {
            it->adjWindowOut(windowsSizeDiff);
            itn = m_mapStream.next(it);
            it = itn;
        }
    }
    return 0;
}


int H2ConnBase::sendSettingsFrame(bool disable_push)
{
    if (m_h2flag & H2_CONN_FLAG_SETTING_SENT)
        return 0;

    static uint8_t s_settings[][6] =
    {
        //{0x00, H2_SETTINGS_HEADER_TABLE_SIZE,     0x00, 0x00, 0x10, 0x00 },
        {0x00, H2_SETTINGS_MAX_CONCURRENT_STREAMS,  0x00, 0x00, 0x00, 0x64 },
        {0x00, H2_SETTINGS_INITIAL_WINDOW_SIZE,     0x00, 0x80, 0x00, 0x00 },
        {0x00, H2_SETTINGS_MAX_FRAME_SIZE,          0x00, 0x00, 0x40, 0x00 },
        {0x00, H2_SETTINGS_ENABLE_PUSH,             0x00, 0x00, 0x00, 0x00 },
        //{0x00, H2_SETTINGS_MAX_HEADER_LIST_SIZE,  0x00, 0x00, 0x40, 0x00 },
    };
    char buf[27 + 13 + 6];
    int settings_payload_size = 18;
    if (disable_push)
        settings_payload_size += 6;
    new (buf) H2FrameHeader(settings_payload_size, H2_FRAME_SETTINGS, 0, 0);
    memcpy(&buf[9], s_settings, settings_payload_size);
    LS_DBG_H(getLogSession(), "send SETTING frame, MAX_CONCURRENT_STREAMS: %d,"
             " INITIAL_WINDOW_SIZE: %d",
             m_iServerMaxStreams, m_iStreamInInitWindowSize);
    set_h2flag(H2_CONN_FLAG_SETTING_SENT);

    H2FrameHeader wu_header(4, H2_FRAME_WINDOW_UPDATE, 0, 0);
    memcpy(&buf[settings_payload_size + 9], &wu_header, 9);
    appendNbo4Bytes(&buf[settings_payload_size + 18], H2_FCW_64K * 16 * 16);
    m_iDataInWindow += H2_FCW_64K * 16 * 16;
    guaranteeOutput(buf, settings_payload_size + 22);
    return 0;
}


int H2ConnBase::resetStream( uint32_t id, H2StreamBase *pStream, H2ErrorCode code)
{
    if (pStream)
    {
        id = pStream->getStreamID();
    }
    else
        pStream = findStream(id);
    if (pStream || (id > 0 && getBuf()->size() < 16384))
        sendRstFrame(id, code);
    if (pStream)
    {
        pStream->setState(HIOS_RESET);
        recycleStream(pStream);
    }
    return 0;
}



int H2ConnBase::processWindowUpdateFrame(H2FrameHeader *pHeader)
{
    char sTemp[4];
    m_bufInput.moveTo(sTemp, 4);
    m_iCurrentFrameRemain -= 4;
    uint32_t id = pHeader->getStreamId();
    int32_t delta = beReadUint32((const unsigned char *)sTemp);
    delta &= 0x7FFFFFFFu;
    if (delta == 0)
    {
        LS_DBG_L(getLogSession(), "session WINDOW_UPDATE ERROR: %d", delta);
        if (id)
            resetStream(id, NULL, H2_ERROR_PROTOCOL_ERROR);
        else
            doGoAway(H2_ERROR_PROTOCOL_ERROR);
        return 0;
    }

    if (delta < 10)
    {
        if (++m_iControlFrames > MAX_CONTROL_FRAMES_RATE)
        {
            LS_INFO(getLogSession(), "WINDOW_UPDATE frame abuse detected, close connection.");
            return LS_FAIL;
        }
    }

    if (id == 0)
    {
        // || (id == 0 && ((delta % 4) != 0))
        uint32_t tmpVal = m_iCurDataOutWindow + delta;
        if (tmpVal > 2147483647)  //2^31 -1
        {
            //sendRstFrame(pHeader->getStreamId(), H2_ERROR_PROTOCOL_ERROR);
            LS_DBG_L(getLogSession(),
                     "connection WINDOW_UPDATE ERROR: %d, window size: %d, "
                     "total %d (m_iDataInWindow: %d) ",
                     delta, m_iCurDataOutWindow, tmpVal, m_iDataInWindow);
            doGoAway(H2_ERROR_FLOW_CONTROL_ERROR);
            return 0;
        }
        LS_DBG_L(getLogSession(), "connection WINDOW_UPDATE: %d, "
                 "current window size: %d, new: %d ",
                 delta, m_iCurDataOutWindow, tmpVal);
        if (m_iCurDataOutWindow <= 0 && tmpVal > 0)
        {
            m_iCurDataOutWindow = tmpVal;
            onWriteEx2();
        }
        else
            m_iCurDataOutWindow = tmpVal;
    }
    else
    {
        H2StreamBase *pStream = findStream(id);
        if (pStream != NULL)
        {
            int ret = pStream->adjWindowOut(delta);
            if (ret < 0)
                resetStream(id, pStream, H2_ERROR_FLOW_CONTROL_ERROR);
            else if (pStream->getWindowOut() <= delta)
                pStream->continueWrite();

        }
        else
        {
            if ((id & 1) == 1)
            {
                if (id > m_uiLastStreamId)
                    return LS_FAIL;
            }
            else
            {
                if (id > m_uiPushStreamId)
                    return LS_FAIL;
            }
        }

        //flush();
    }
    m_iCurrentFrameRemain = 0;
    return 0;
}


int H2ConnBase::processRstFrame(H2FrameHeader *pHeader)
{
    uint32_t streamID = pHeader->getStreamId();

    if (streamID == 0)
        return LS_FAIL;
    if ((streamID & 1) != 0)
    {
        if (streamID > m_uiLastStreamId)
            return LS_FAIL;
    }
    else
    {
        if (streamID >= m_uiPushStreamId)
            return LS_FAIL;
    }

    H2StreamBase *stream = findStream(streamID);
    if (stream == NULL)
        return 0;

    ++m_uiRstStreams;
    if (m_uiRstStreams > 500 && m_uiRstStreams > m_uiStreams / 2)
    {
        LS_DBG_L(getLogSession(), "HTTP2 rapid reset attack detected, %d out of %d streams was reseted. stop offering HTTP/2.",
                m_uiRstStreams, m_uiStreams);
        disableHttp2ByIp();
        return LS_FAIL;
    }

    unsigned char p[4];
    m_bufInput.moveTo((char *)p, 4);
    m_iCurrentFrameRemain -= 4;
    uint32_t errorCode = beReadUint32(p);
    LS_DBG_L(getLogSession(), "streamID:%d processRstFrame, error code: %d",
             streamID, errorCode);
    stream->setFlag(HIO_FLAG_PEER_RESET, 1);
    stream->onPeerClose();
    return 0;
}


void H2ConnBase::skipRemainData()
{
    int len = m_bufInput.size();
    if (len > m_iCurrentFrameRemain)
        len = m_iCurrentFrameRemain;
    m_bufInput.pop_front(len);
    m_iCurrentFrameRemain -= len;
    if (m_iCurrentFrameRemain > 0)
        m_inputState = ST_SKIP_REMAIN;
}


int H2ConnBase::processDataFrame(H2FrameHeader *pHeader)
{
    uint32_t streamID = pHeader->getStreamId();

    printLogMsg(pHeader);
    if (streamID == 0 || streamID > m_uiLastStreamId)
        return LS_FAIL;

    H2StreamBase *stream = findStream(streamID);
    m_current = stream;
    if (stream == NULL || stream->isPeerShutdown())
    {
        LS_DBG_L(getLogSession(), "stream: %p, isPeerShutdown: %d ",
                 stream, stream?stream->isPeerShutdown():0);
        skipRemainData();
        resetStream(streamID, stream, H2_ERROR_STREAM_CLOSED);
        return 0;
        //doGoAway(H2_ERROR_STREAM_CLOSED);
        //return LS_FAIL;
    }
    if (pHeader->getFlags() & H2_FLAG_PADDED)
    {
        m_padLen = (uint8_t)(*(m_bufInput.begin()));
        if (m_iCurrentFrameRemain <= 1 + m_padLen)
            return LS_FAIL;

        m_bufInput.pop_front(1);
        //Do not pop_back right now since buf may have more data then this frame
        --m_iCurrentFrameRemain;
        stream->adjWindowToUpdate(1 + m_padLen);
    }
    else
        m_padLen = 0;

    if (m_iCurrentFrameRemain == m_padLen
        && !(pHeader->getFlags() & H2_CTRL_FLAG_FIN))
    {
        if (++m_iControlFrames > MAX_CONTROL_FRAMES_RATE)
        {
            LS_INFO(getLogSession(), "Zero length DATA frame abuse detected, close connection.");
            return LS_FAIL;
        }
    }
    if (m_iCurrentFrameRemain > m_padLen)
        m_inputState = ST_DATA_PAYLOAD;
    else
        m_inputState = ST_SKIP_REMAIN;
    int len;
    while (m_iCurrentFrameRemain > 0)
    {
        len = m_bufInput.blockSize();
        if (len == 0)
            return 0;
        if (len >= m_iCurrentFrameRemain)
            len = m_iCurrentFrameRemain;
        if (m_inputState == ST_DATA_PAYLOAD)
        {
            int data_len;
            if (len >= m_iCurrentFrameRemain - m_padLen)
            {
                data_len = m_iCurrentFrameRemain - m_padLen;
                m_inputState = ST_SKIP_REMAIN;
            }
            else
                data_len = len;
            m_iCurrentFrameRemain -= len;
            stream->bytesRecv(data_len);
            if (stream->processRcvdData(m_bufInput.begin(), data_len) == LS_FAIL)
            {
                resetStream(streamID, stream, H2_ERROR_PROTOCOL_ERROR);
                return 0;
            }
        }
        else
            m_iCurrentFrameRemain -= len;
        m_bufInput.pop_front(len);
    }

    LS_DBG_H(stream, "processDataFrame() buffered: %d, WINDOW_IN to update: %d.",
             stream->getBufIn()->size(), stream->getWindowToUpdate());

    if (pHeader->getFlags() & H2_CTRL_FLAG_FIN)
    {
        //if (m_iCurrentFrameRemain == 0)
        if (stream->onPeerShutdown() == LS_FAIL)
        {
            resetStream(streamID, stream, H2_ERROR_PROTOCOL_ERROR);
            return 0;
        }
    }
    else
        stream->windowUpdate();
    if (m_current && m_current->isWantRead())
        m_current->onRead();

    return 0;
}


int H2ConnBase::tryReadFrameHeader()
{
    m_iCurrentFrameRemain = -H2_FRAME_HEADER_SIZE;
    int ret = getInStream()->read((char *)&m_curH2Header, H2_FRAME_HEADER_SIZE);
    if (ret >= H2_FRAME_HEADER_SIZE)
    {
        m_iCurrentFrameRemain = m_curH2Header.getLength();
        LS_DBG_L(getLogSession(), "frame type %d, size: %d, flag: 0x%hhx",
                 m_curH2Header.getType(), m_iCurrentFrameRemain, m_curH2Header.getFlags());
        if (m_iCurrentFrameRemain > H2_DEFAULT_DATAFRAME_SIZE)
        {
            LS_DBG_L(getLogSession(), "Frame size is too large, "
                     "Max: %d, Current Frame Size: %d.",
                     H2_DEFAULT_DATAFRAME_SIZE, m_iCurrentFrameRemain);
            doGoAway(H2_ERROR_FRAME_SIZE_ERROR);
            return LS_FAIL;
        }
        if (m_curH2Header.getType() == H2_FRAME_DATA
            && (m_curH2Header.getFlags() & H2_FLAG_PADDED) == 0)
        {
            if (!m_current || m_current->getStreamID() != m_curH2Header.getStreamId())
                m_current = findStream(m_curH2Header.getStreamId());
            if (m_current)
            {
                if (m_iCurrentFrameRemain == 0
                    && (m_curH2Header.getFlags() & H2_CTRL_FLAG_FIN))
                {
                    m_iCurrentFrameRemain = -H2_FRAME_HEADER_SIZE;
                    m_inputState = ST_CONTROL_FRAME;
                    if (m_current->onPeerShutdown() == LS_FAIL)
                        resetStream(m_curH2Header.getStreamId(), m_current,
                                    H2_ERROR_PROTOCOL_ERROR);
                    m_current = NULL;
                    return 0;
                }
            }
            consumedWindowIn(m_iCurrentFrameRemain);
            m_inputState = ST_DATA_PAYLOAD;
            return H2_FRAME_HEADER_SIZE;
        }
    }
    else if (ret > 0)
        m_bufInput.append((char *)&m_curH2Header, ret);
    else if (ret < 0)
        return ret;
    m_inputState = ST_CONTROL_FRAME;
    return ret;
}


int H2ConnBase::processDataFrameDirectBuffer(H2FrameHeader *pHeader)
{
    size_t remain;
    int total = 0;
    assert(m_iCurrentFrameRemain >= m_padLen);
    assert(m_current);
    while ((remain = m_iCurrentFrameRemain - m_padLen) > 0)
    {
        char *buf = m_current->getDirectInBuf(remain);
        int ret = getInStream()->read(buf, remain);
        LS_DBG_H(m_current,
                 "processDataFrameDirectBuffer() read(%p, %zd) return %d.",
                 buf, remain, ret);
        if (ret > 0)
        {
            m_current->directInBufUsed(ret);
            m_iCurrentFrameRemain -= ret;
            total += ret;
        }
        else if (ret == 0)
            return total;
        else
            return ret;
    }
    if (pHeader->getFlags() & H2_CTRL_FLAG_FIN)
    {
        if (m_current->onPeerShutdown() == LS_FAIL)
        {
            resetStream(pHeader->getStreamId(), m_current,
                        H2_ERROR_PROTOCOL_ERROR);
            return 0;
        }
    }
    else
        m_current->windowUpdate();
    if (m_current && m_current->isWantRead())
        m_current->onRead();
    if (m_padLen > 0)
        m_inputState = ST_SKIP_REMAIN;
    else
        return tryReadFrameHeader();

    return total;
}


int H2ConnBase::processDataFrameSkipRemain(H2FrameHeader *pHeader)
{
    while (m_iCurrentFrameRemain > 0)
    {
        char buf[m_iCurrentFrameRemain];
        int ret = getInStream()->read(buf, m_iCurrentFrameRemain);
        if (ret > 0)
        {
            m_iCurrentFrameRemain -= ret;
        }
        else
            return ret;
    }
    return tryReadFrameHeader();
}


int H2ConnBase::processDataFramePadding(H2FrameHeader *pHeader)
{
    while (m_padLen > 0)
    {
        if (m_current == NULL)
            m_current = findStream(pHeader->getStreamId());
        else
            assert(m_current == findStream(pHeader->getStreamId()));
        char buf[256];
        int ret = getInStream()->read(buf, m_padLen);
        if (ret > 0)
        {
            m_iCurrentFrameRemain -= ret;
            m_padLen -= ret;
        }
        else
            return ret;
    }
    m_iCurrentFrameRemain = -H2_FRAME_HEADER_SIZE;
    m_inputState = ST_CONTROL_FRAME;
    return 1;
}


int H2ConnBase::processContinuationFrame(H2FrameHeader *pHeader)
{
    if (m_h2flag & H2_CONN_FLAG_GOAWAY)
        return 0;
    uint32_t id = pHeader->getStreamId();
    if ((id != m_uiLastStreamId)
        || (m_h2flag & H2_CONN_HEADERS_START) == 0)
    {
        LS_DBG_L(getLogSession(), "received unexpected CONTINUATION frame, expect id: %d, "
                 " connection flag: %d", m_uiLastStreamId, m_h2flag);
        return LS_FAIL;
    }
    if (pHeader->getFlags() & H2_FLAG_END_HEADERS)
        clr_h2flag(H2_CONN_HEADERS_START);

    return processHeaderIn(id, pHeader->getFlags());
}


int H2ConnBase::processHeaderIn(uint32_t id, unsigned char iHeaderFlag)
{
    int iDatalen = (m_bufInput.size() < m_iCurrentFrameRemain - m_padLen) ?
                   (m_bufInput.size()) : (m_iCurrentFrameRemain - m_padLen);
    if (iDatalen <= 0)
    {
        if ((iHeaderFlag & H2_FLAG_END_HEADERS) == 0
            && ++m_iControlFrames > MAX_CONTROL_FRAMES_RATE)
        {
            LS_INFO(getLogSession(), "Zero-length HEADER/CONTINUATION frame abuse detected, close connection.");
            return LS_FAIL;
        }
        return 0;
    }

    //Check if we need to move the buffer to stream bufinflat
    bool needToMove = false;
    int bufPart1Len = m_bufInput.blockSize();
    if ((iHeaderFlag & H2_FLAG_END_HEADERS) == 0
        || iDatalen > bufPart1Len
        || m_bufInflate.size() > 0)
    {
        needToMove = true;
        if (iDatalen > bufPart1Len)
        {
            m_bufInflate.append(m_bufInput.begin(), bufPart1Len);
            m_bufInput.pop_front(bufPart1Len);
            m_bufInflate.append(m_bufInput.begin(), iDatalen - bufPart1Len);
            m_bufInput.pop_front(iDatalen - bufPart1Len);
        }
        else
        {
            m_bufInflate.append(m_bufInput.begin(), iDatalen);
            m_bufInput.pop_front(iDatalen);
        }
        if (m_bufInflate.size() > MAX_HTTP2_HEADERS_SIZE)
        {
            LS_INFO(getLogSession(), "HEADER abuse detected, close connection.");
            return LS_FAIL;
        }
        m_iCurrentFrameRemain -= iDatalen;
    }

    if (!(iHeaderFlag & H2_FLAG_END_HEADERS))
        return 0;
    unsigned char *pSrc;
    if (needToMove)
    {
        AutoBuf tmpBuf(0);
        tmpBuf.swap(m_bufInflate);
        pSrc = (unsigned char *)tmpBuf.begin();
        iDatalen = tmpBuf.size();
        return decodeHeaders(id, pSrc, iDatalen, iHeaderFlag);
    }
    pSrc = (unsigned char *)m_bufInput.begin();
    return decodeHeaders(id, pSrc, iDatalen, iHeaderFlag);
}


int H2ConnBase::processPriority(uint32_t id)
{
    unsigned char buf[5];
    m_bufInput.moveTo((char *)buf, 5);
    m_iCurrentFrameRemain -= 5;

    unsigned char *pSrc = buf;
    m_priority.m_exclusive = *pSrc >> 7;
    m_priority.m_dependStreamId = beReadUint32((const unsigned char *)pSrc) &
                                    0x7FFFFFFFu;
    if (m_priority.m_dependStreamId == id)
        return H2_ERROR_PROTOCOL_ERROR;
    //Add one to the value to obtain a weight between 1 and 256.
    m_priority.m_weight = *(pSrc + 4) + 1;

    LS_DBG_L(getLogSession()->getLogger(),
               "[%s-%d] stream priority, execlusive: %d, depend sid: %d, weight: %hd ",
             getLogSession()->getLogId(), id, (int)m_priority.m_exclusive, m_priority.m_dependStreamId, m_priority.m_weight);
    H2StreamBase *stream = findStream(id);
    if (stream)
    {
        if (stream->getFlag() & HIO_FLAG_PRI_SET)
        {
            if (++m_iControlFrames > MAX_CONTROL_FRAMES_RATE)
            {
                LS_INFO(getLogSession(), "PRIORITY frame abuse detected, close connection.");
                return LS_FAIL;
            }
        }
        else
            stream->setFlag(HIO_FLAG_PRI_SET, 1);
        stream->apply_priority(&m_priority);
    }
    return 0;
}

int H2ConnBase::processHeadersFrame(H2FrameHeader *pHeader)
{
    uint32_t id = pHeader->getStreamId();
    if (m_h2flag & H2_CONN_FLAG_GOAWAY)
        return 0;

    if (verifyStreamId(id) == LS_FAIL)
        return LS_FAIL;

    unsigned char iHeaderFlag = pHeader->getFlags();

    unsigned char *pSrc = (unsigned char *)m_bufInput.begin();
    if (iHeaderFlag & H2_FLAG_PADDED)
    {
        m_padLen = *pSrc;
        if (m_iCurrentFrameRemain <= 1 + m_padLen)
        {
             return H2_ERROR_PROTOCOL_ERROR;
        }
        m_bufInput.pop_front(1);
        --m_iCurrentFrameRemain;
    }
    else
        m_padLen = 0;

    if (iHeaderFlag & H2_FLAG_PRIORITY)
    {
        if (processPriority(id))
            return H2_ERROR_PROTOCOL_ERROR;
    }
    else
        memset(&m_priority, 0, sizeof(m_priority));

    if ((iHeaderFlag & H2_FLAG_END_HEADERS) == 0)
        set_h2flag(H2_CONN_HEADERS_START);

    m_bufInflate.clear();
    return processHeaderIn(id, iHeaderFlag);
}


void H2ConnBase::recycleStream(uint32_t uiStreamID)
{
    StreamMap::iterator it = m_mapStream.find(uiStreamID);
    if (it == m_mapStream.end())
        return;
    recycleStream(it);
}


int H2ConnBase::getWeightedPriority(H2StreamBase* s)
{
    int pri = s->getPriority();
    int ret = 1;
    for(int i = pri - 1; i >= 0; --i)
    {
        if (m_priQue[i].size() > 0)
            return 16;
    }
    ret = m_priQue[pri].size() + 1;
    if (ret > 16)
        ret = 16;
    return ret;
}


int H2ConnBase::sendDataFrame(uint32_t uiStreamId, int flag,
                                IOVec *pIov, int total)
{
    int ret = 0;
    H2FrameHeader header(total, H2_FRAME_DATA, flag, uiStreamId);

    LS_DBG_H(getLogSession()->getLogger(), "[%s-%d] send DATA frame, FIN: %d, iov: %p, len: %d"
             , getLogSession()->getLogId(), uiStreamId, flag, pIov, total);
    wantFlush2();

    appendOutput((char *)&header, 9);
    if (pIov)
        ret = appendOutput(pIov, total);
    if (ret != -1)
        m_iCurDataOutWindow -= total;
    return ret;
}


int H2ConnBase::sendDataFrame(uint32_t uiStreamId, int flag,
                                const char * pBuf, int len)
{
    int ret = 0;
    H2FrameHeader header(len, H2_FRAME_DATA, flag, uiStreamId);

    LS_DBG_H(getLogSession()->getLogger(), "[%s-%d] send DATA frame, FIN: %d, data: %p, len: %d"
             , getLogSession()->getLogId(), uiStreamId, flag, pBuf, len);
    wantFlush2();

    appendOutput((char *)&header, 9);
    if (pBuf)
        ret = appendOutput(pBuf, len);
//    if (pBuf)
//        ret = cacheWrite(pBuf, len);
    if (ret != -1)
        m_iCurDataOutWindow -= ret;
    return ret;
}


int H2ConnBase::sendfileDataFrame(uint32_t uiStreamId, int flag, int fd,
                                    off_t off, int len)
{
    int ret = 0;
    H2FrameHeader header(len, H2_FRAME_DATA, flag, uiStreamId);

    LS_DBG_H(getLogSession()->getLogger(), "[%s-%d] sendfile DATA frame, FIN: %d, fd: %d, off: %lld, len: %d"
             , getLogSession()->getLogId(), uiStreamId, flag, fd, (long long)off, len);
    wantFlush2();

    appendOutput((char *)&header, 9);
    if (len)
        ret = appendSendfileOutput(fd, off, len);
    if (ret != -1)
        m_iCurDataOutWindow -= ret;
    return ret;
}


int H2ConnBase::sendFrame8Bytes(H2FrameType type, uint32_t uiStreamId,
                                  uint32_t uiVal1, uint32_t uiVal2)
{
    char buf[20];
    new (buf) H2FrameHeader(8, type, 0, uiStreamId);
    appendNbo4Bytes(&buf[9], uiVal1);
    appendNbo4Bytes(&buf[13], uiVal2);
    LS_DBG_H(getLogSession()->getLogger(), "[%s-%d] send %s frame, stream: %d, value: %d"
             , getLogSession()->getLogId(), uiStreamId, getH2FrameName(type), uiVal1, uiVal2);
    guaranteeOutput(buf, 17);
    return 0;
}


int H2ConnBase::sendFrame4Bytes(H2FrameType type, uint32_t uiStreamId,
                                uint32_t uiVal1, bool immed_flush)
{
    char buf[20];
    new (buf) H2FrameHeader(4, type, 0, uiStreamId);

    appendNbo4Bytes(&buf[9], uiVal1);
    LS_DBG_H(getLogSession()->getLogger(), "[%s-%d] send %s frame, value: %d",
             getLogSession()->getLogId(),
             uiStreamId, getH2FrameName(type), uiVal1);
    if (immed_flush)
    {
        appendOutput(buf, 13);
        flush();
    }
    else
        guaranteeOutput(buf, 13);
    return 0;
}


int H2ConnBase::sendFrame0Bytes(H2FrameType type, unsigned char flags,
                                  uint32_t uiStreamId)
{
    char buf[20];
    new (buf) H2FrameHeader(0, type, flags, uiStreamId);
    LS_DBG_H(getLogSession()->getLogger(), "[%s-%d] send %s frame, with Flag: %d"
             , getLogSession()->getLogId(), uiStreamId, getH2FrameName(type), flags);
    guaranteeOutput(buf, 9);
    return 0;
}


int H2ConnBase::sendPingFrame(uint8_t flags, uint8_t *pPayload)
{
    char buf[20];
    new (buf) H2FrameHeader(8, H2_FRAME_PING, flags, 0);
    memcpy(&buf[9], (char *)pPayload, H2_PING_FRAME_PAYLOAD_SIZE);
    LS_DBG_H(getLogSession()->getLogger(), "[%s-%d] send %s frame, with Flag: %d"
             , getLogSession()->getLogId(), 0, "PING", flags);
    appendOutput(buf, 17);
    flush();
    return 0;
}


int H2ConnBase::processPingFrame(H2FrameHeader *pHeader)
{
    struct timeval CurTime;
    long msec;
    if (pHeader->getStreamId() != 0)
    {
        //doGoAway(H2_ERROR_PROTOCOL_ERROR);
        LS_DBG_L(getLogSession(), "invalid PING frame id %d",
                 pHeader->getStreamId());
        return LS_FAIL;
    }

    if (!(pHeader->getFlags() & H2_FLAG_ACK) || m_timevalPing.tv_sec == 0)
    {
        uint8_t payload[H2_PING_FRAME_PAYLOAD_SIZE];
        m_bufInput.moveTo((char *)payload, H2_PING_FRAME_PAYLOAD_SIZE);
        m_iCurrentFrameRemain -= H2_PING_FRAME_PAYLOAD_SIZE;
        if (pHeader->getFlags() & H2_FLAG_ACK)
        {
            LS_DBG_L(getLogSession(), "unexpected PING frame, %.8s", payload);
            return 0;
        }
        if (++m_iControlFrames > MAX_CONTROL_FRAMES_RATE)
        {
            LS_INFO(getLogSession(), "PING frame abuse detected, close connection.");
            return LS_FAIL;
        }
        return sendPingFrame(H2_FLAG_ACK, payload);
    }

    gettimeofday(&CurTime, NULL);
    msec = (CurTime.tv_sec - m_timevalPing.tv_sec) * 1000;
    msec += (CurTime.tv_usec - m_timevalPing.tv_usec) / 1000;
    LS_DBG_H(getLogSession(), "Received PING, Round trip "
             "times=%ld milli-seconds", msec);
    m_timevalPing.tv_sec = 0;
    return 0;
}


H2StreamBase *H2ConnBase::findStream(uint32_t uiStreamID)
{
    StreamMap::iterator it = m_mapStream.find(uiStreamID);
    if (it == m_mapStream.end())
        return NULL;
    return it;
}


int H2ConnBase::processGoAwayFrame(H2FrameHeader *pHeader)
{
    if (!(m_h2flag & H2_CONN_FLAG_GOAWAY))
    {
        doGoAway((pHeader->getStreamId() != 0)?
                 H2_ERROR_PROTOCOL_ERROR:
                 H2_ERROR_NO_ERROR);
    }
    set_h2flag(H2_CONN_FLAG_GOAWAY);

    onCloseEx();
    return LS_OK;
}


int H2ConnBase::sendGoAwayFrame(H2ErrorCode status)
{
    sendFrame8Bytes(H2_FRAME_GOAWAY, 0, m_uiLastStreamId, status);
    flush();
    return 0;
}


int H2ConnBase::releaseAllStream()
{
    StreamMap::iterator itn, it = m_mapStream.begin();
    for (; it != m_mapStream.end();)
    {
        itn = m_mapStream.next(it);
        recycleStream(it);
        it = itn;
    }
    return 0;
}


int H2ConnBase::sendHeaderContFrame(uint32_t uiStreamID, uint8_t flag,
                                      H2FrameType type, const char *pBuf, int size)
{
    const char *p = pBuf;
    int frameSize;
    while(size > 0)
    {
        if (size > m_iPeerMaxFrameSize)
        {
            frameSize = m_iPeerMaxFrameSize;
            size -= m_iPeerMaxFrameSize;
        }
        else
        {
            frameSize = size;
            size = 0;
            flag |= H2_FLAG_END_HEADERS;
        }
        LS_DBG_H(getLogSession()->getLogger(),
                 "[%s-%d] send %s frame, flag: %d, size: %d",
                 getLogSession()->getLogId(), uiStreamID,
                 getH2FrameName(type), flag, frameSize);
        H2FrameHeader header(frameSize, type, flag, uiStreamID);
        memcpy((char *)p - 9, &header, 9);
        guaranteeOutput((const char *)p - 9, frameSize + 9);
        if (size > 0)
        {
            p += frameSize;
            type = H2_FRAME_CONTINUATION;
            flag = 0;
        }
    }
    return 0;
}


int H2ConnBase::sendReqHeaders(uint32_t id, uint32_t promise_streamId,
                               int flag, UnpackedHeaders *req_hdrs)
{
    unsigned char hdr_buf[H2_TMP_HDR_BUFF_SIZE + 16];
    unsigned char *cur = &hdr_buf[16];
    unsigned char *buf_end = cur + H2_TMP_HDR_BUFF_SIZE;

    if (promise_streamId)
    {
        uint32_t sid = htonl(promise_streamId);
        memcpy(cur, &sid, 4);
        cur += 4;
    }
    req_hdrs->prepareSendHpack();
    cur += encodeReqHeaders(cur, buf_end, req_hdrs);

    int total = cur - hdr_buf - 16;
    return sendHeaderContFrame(id, flag, (promise_streamId)
                                ? H2_FRAME_PUSH_PROMISE : H2_FRAME_HEADERS,
                               (const char *)&hdr_buf[16], total);
}


int H2ConnBase::encodeReqHeaders(unsigned char *buf, unsigned char *buf_end,
                                 const UnpackedHeaders *hdrs)
{
    unsigned char *cur = buf;

    lsxpack_header *hdr = (lsxpack_header *)hdrs->begin();
    const lsxpack_header *end = hdrs->end();

    while (hdr < end)
    {
        if (hdr->buf)
        {
            DUMP_LSXPACK(getLogSession(), hdr, "HPACK encode reqHeader");
            cur = lshpack_enc_encode(&m_hpack_enc, cur, buf_end, hdr);
        }
        ++hdr;
    }
    return cur - buf;
}


UnpackedHeaders *H2ConnBase::createUnpackedHdr(
    bool is_http, const ls_str_t *method, const ls_str_t *host,
    const ls_str_t *url, const ls_http_header_t *extra)
{
    UnpackedHeaders *hdrs = new UnpackedHeaders();
    if (!hdrs)
        return NULL;
    hdrs->setMethod(method->ptr, method->len);
    hdrs->setUrl(url->ptr, url->len);
    hdrs->setHost(host->ptr, host->len);
    const ls_http_header_t *p = extra;
    while(p->name.ptr)
    {
        hdrs->appendHeader(UPK_HDR_UNKNOWN, p->name.ptr, p->name.len,
                           p->value.ptr, p->value.len);
        ++p;
    }
    return hdrs;
}


UnpackedHeaders *H2ConnBase::createUnpackedHdr(const char *method,
    const char *host, const char *url, ls_http_header_t *extra)
{
    UnpackedHeaders *hdrs = new UnpackedHeaders();
    hdrs->setMethod(method, strlen(method));
    hdrs->setUrl(url, strlen(url));
    hdrs->setHost(host, strlen(host));
    ls_http_header_t *p = extra;
    while(p->name.ptr)
    {
        hdrs->appendHeader(UPK_HDR_UNKNOWN, p->name.ptr, p->name.len,
                           p->value.ptr, p->value.len);
        ++p;
    }
    return hdrs;
}

void H2ConnBase::removePriQue(H2StreamBase *stream)
{
    LS_DBG_H(stream, "remove from priority queue: %d",
             stream->getPriority());

    m_priQue[stream->getPriority()].remove(stream);
}


void H2ConnBase::add2PriorityQue(H2StreamBase *stream)
{
    if (stream->next())
        removePriQue(stream);
    LS_DBG_H(stream, "add to priority queue: %d",
             stream->getPriority());

    m_priQue[stream->getPriority()].append(stream);
    set_h2flag(H2_CONN_FLAG_WAIT_PROCESS);
    if ((m_h2flag & H2_CONN_FLAG_IN_EVENT) == 0 && m_iCurDataOutWindow > 0)
        continueWrite();
}


#define SSL_REC_MAX_PAYLOAD 16384
int H2ConnBase::bufferOutput(const char *data, int size)
{
    int blockSize;
    int remain = size;
    int ret;
    assert(m_pendingOutSize == 0);
    while (remain > 0)
    {
        blockSize = SSL_REC_MAX_PAYLOAD - getBuf()->size();
        if (blockSize <= 0 || blockSize > remain)
            blockSize = remain;
        ret = getBuf()->append(data, blockSize);
        if (getBuf()->size() == SSL_REC_MAX_PAYLOAD)
            BufferedOS::flush();
        if (ret <= 0)
            break;
        remain -= ret;
        data += ret;
    }
    LS_DBG_H(getLogSession(), "[H2] bufferOutput(%d), buffered: %d, total buffer size = %d",
             size, size - remain, getBuf()->size());
    assert(ret > 0);
    return size - remain;
}


int H2ConnBase::appendOutput(const char *data, int size)
{
    int ret;
    int remain = size;
    closePendingOut();
    if (getBuf()->size() == 0 && isDirectBuffer())
    {
        ret = directBuffer(data, size);
        LS_DBG_H(getLogSession(), "SSL bufferedWrite(%p, %d) return %d",
                data, size, ret);
        if (ret >= size)
            return size;
        if (ret > 0)
        {
            data += ret;
            remain = size - ret;
        }
    }

    ret = bufferOutput(data, remain);
    return size - (remain - ret);
}


int H2ConnBase::appendOutput(IOVec *pIov, int size)
{
    size_t ret;
    size_t len;
    size_t remain = size;
    int write_through = (getBuf()->size() == 0 && isDirectBuffer());
    closePendingOut();
    struct iovec *iov = pIov->begin();
    while(remain > 0)
    {
        if (iov->iov_len > remain)
            len = remain;
        else
            len = iov->iov_len;
        if (write_through)
        {
            ret = directBuffer((char *)iov->iov_base, len);
            LS_DBG_H(getLogSession(), "SSL bufferedWrite(%p, %zd) return %zd",
                        iov->iov_base, len, ret);
            if (ret < len)
                write_through = 0;
        }
        else
        {
            ret = bufferOutput((char *)iov->iov_base, len);
        }
        if (ret >= iov->iov_len)
        {
            pIov->pop_front(1);
            ++iov;
            assert(iov == pIov->begin());
        }
        else
        {
            iov->iov_base = (char *)iov->iov_base + ret;
            iov->iov_len -= ret;
        }
        remain -= ret;
    }
    return size;
}


int H2ConnBase::timerRoutine()
{
    int stuckRead = 0;
    StreamMap::iterator itn, it = m_mapStream.begin();
    for (; it != m_mapStream.end();)
    {
        itn = m_mapStream.next(it);
        H2Stream *pStream = (H2Stream *)it;
        if (pStream->onTimer() == 0)
        {
            if (pStream->getState() != HIOS_CONNECTED)
                recycleStream(pStream);
            else if (pStream->isStuckOnRead())
                ++stuckRead;
        }
        else
            recycleStream(pStream);
        it = itn;
    }
    if (m_mapStream.size() == 0)
    {

        if (m_tmIdleBegin == 0)
            m_tmIdleBegin = DateTime::s_curTime;
        else
        {
            int idle = DateTime::s_curTime - m_tmIdleBegin;
            if (idle > HttpServerConfig::getInstance().getSpdyKeepaliveTimeout())
            {
                LS_DBG_L(getLogSession(), "Session idle for %d seconds, GOAWAY.",
                         idle);

                doGoAway(H2_ERROR_NO_ERROR);
            }
        }
    }
    else
    {
        if (stuckRead > 0)
        {
            if (m_timevalPing.tv_sec == 0)
            {
                LS_DBG_L(getLogSession(),
                         "send PING to check if peer is still alive.");
                sendPingFrame(0, (uint8_t *)"RUALIVE?");
                m_timevalPing.tv_sec = DateTime::s_curTime;
                m_timevalPing.tv_usec = DateTime::s_curTimeUs;

            }
            else if (DateTime::s_curTime - m_timevalPing.tv_sec > 20)
            {
                LS_DBG_L(getLogSession(), "PING timeout.");
                doGoAway(H2_ERROR_PROTOCOL_ERROR);
            }
        }
        m_tmIdleBegin = 0;
    }
    m_iControlFrames = 0;
    m_tmLastTimer = DateTime::s_curTime;
    return 0;
}


int H2ConnBase::processPendingStreams()
{
    TDLinkQueue<H2StreamBase> *pQue = &m_priQue[0];
    TDLinkQueue<H2StreamBase> *pEnd = &m_priQue[H2_STREAM_PRIORITYS];
    //int32_t cur_window = m_iCurDataOutWindow;
    H2StreamBase *stream;
    int wantWrite = 0;

    for( ; pQue < pEnd && m_iCurDataOutWindow > 0; ++pQue)
    {
        LS_DBG_H(getLogSession(),
                "process priority queue [%ld]", pQue - &m_priQue[0]);
        if (pQue->empty())
            continue;
        int count = pQue->size();
        while(count-- > 0 && m_iCurDataOutWindow > 0
              && (stream =(H2Stream *)pQue->pop_front()) != NULL)
        {
            if (stream->getState() != HIOS_CONNECTED)
            {
                recycleStream(stream);
                continue;
            }
            if (stream->getFlag(HIO_FLAG_INIT_SESS))
            {
                if (assignStreamHandler(stream) == LS_FAIL)
                    continue;
                if (stream->isWantWrite() && (stream->getWindowOut() > 0))
                {
                    if (!stream->next())
                        m_priQue[stream->getPriority()].append(stream);
                }
            }
            else
                m_priQue[stream->getPriority()].push_front(stream);

            if (stream->getState() != HIOS_CONNECTED)
                recycleStream(stream);
            if (isOutBufFull())
            {
                ++wantWrite;
                break;
            }
        }
        if (wantWrite > 0)
            break;
    }

    //if (getBuf()->size() > 0)
    if (m_h2flag & H2_CONN_FLAG_WANT_FLUSH)
        flush();
    return wantWrite;
}


int H2ConnBase::processQueue()
{
    TDLinkQueue<H2StreamBase> *pQue = &m_priQue[0];
    TDLinkQueue<H2StreamBase> *pEnd = &m_priQue[H2_STREAM_PRIORITYS];
    //int32_t cur_window = m_iCurDataOutWindow;
    H2StreamBase *stream;
    int wantWrite = 0;

    for( ; pQue < pEnd && m_iCurDataOutWindow > 0; ++pQue)
    {
        LS_DBG_H(getLogSession(),
                "process priority queue [%ld]", pQue - &m_priQue[0]);
        if (pQue->empty())
            continue;
        int count = pQue->size();
        while(count-- > 0 && m_iCurDataOutWindow > 0
              && !isPauseWrite()
              && (stream =(H2Stream *)pQue->pop_front()) != NULL)
        {
            if (stream->getState() != HIOS_CONNECTED)
            {
                recycleStream(stream);
                continue;
            }
            if (stream->getFlag(HIO_FLAG_INIT_SESS))
            {
                if (assignStreamHandler(stream) == LS_FAIL)
                    continue;
            }
            else if (stream->isWantWrite())
            {
                if (!isPauseWrite())
                    stream->onWrite();
                else
                    m_priQue[stream->getPriority()].push_front(stream);
            }
            if (stream->isWantWrite() && (stream->getWindowOut() > 0))
            {
                ++wantWrite;
                if (!stream->next())
                    m_priQue[stream->getPriority()].append(stream);
            }

            if (stream->getState() != HIOS_CONNECTED)
                recycleStream(stream);
            if (isOutBufFull())
            {
                ++wantWrite;
                break;
            }
        }
        if (wantWrite > 0)
            break;
    }

    //if (getBuf()->size() > 0)
    if (m_h2flag & H2_CONN_FLAG_WANT_FLUSH)
        flush();
    return wantWrite;
}


char *H2ConnBase::getDirectOutBuf(uint32_t stream_id, size_t &size)
{
    if (isDirectBuffer())
        return NULL;
    if (m_pendingOutSize > 0 )
    {
        if (stream_id != m_pendingStreamId)
            closePendingOut2();
        else
            assert(m_pendingUsed < m_pendingOutSize);
    }
    LoopBuf *buf = getBuf();
    if (m_pendingOutSize == 0)
    {
        int max_size = size;
        int buf_size = buf->size() % 16384;
        if (max_size > 16384 - buf_size)
            max_size = 16384 - buf_size;
        if (max_size < H2_FRAME_HEADER_SIZE)
            max_size += 16384;
        if (max_size > m_iPeerMaxFrameSize)
            max_size = m_iPeerMaxFrameSize;
        if (buf->guarantee(max_size) == LS_FAIL)
            return NULL;
        m_pendingOutSize = max_size - H2_FRAME_HEADER_SIZE;
        m_pendingUsed = 0;
        LS_DBG_L(getLogSession(), "Start new pending DATA frame buffer: %d.",
                 m_pendingOutSize);
    }
    buf->used(H2_FRAME_HEADER_SIZE + m_pendingUsed);
    int avail = buf->contiguous();
    char *p = buf->end();
    if (avail > m_pendingOutSize - m_pendingUsed)
        avail = m_pendingOutSize - m_pendingUsed;
    buf->used(-(H2_FRAME_HEADER_SIZE + m_pendingUsed));
    assert(avail > 0);
    size = avail;
    LS_DBG_L(getLogSession(), "return pending DATA frame buffer: %p(%d), at %d/%d.",
             p, avail, m_pendingUsed, m_pendingOutSize);

    return p;
}


void H2ConnBase::directOutBufUsed(uint32_t stream_id, size_t len)
{
    assert(stream_id == m_pendingStreamId);
    assert((int)len <= m_pendingOutSize - m_pendingUsed);
    m_pendingUsed += len;
    LS_DBG_L(getLogSession(), "H2ConnBase::directOutBufUsed(%d, %zd) pending DATA frame buffer used %d/%d.",
             stream_id, len, m_pendingUsed, m_pendingOutSize);
    if (m_pendingUsed == m_pendingOutSize)
        closePendingOut2();
}


void H2ConnBase::closePendingOut2()
{
    if (m_pendingUsed > 0)
    {
        LS_DBG_L(getLogSession(), "H2ConnBase::closePendingOut() stream: %d, used %d/%d.",
                 m_pendingStreamId, m_pendingUsed, m_pendingOutSize);
        H2FrameHeader header(m_pendingUsed, H2_FRAME_DATA, 0, m_pendingStreamId);
        getBuf()->append((char *)&header, H2_FRAME_HEADER_SIZE);
        getBuf()->used(m_pendingUsed);
        m_pendingUsed = 0;
        wantFlush();
    }
    m_pendingStreamId = 0;
    m_pendingOutSize = 0;
}


