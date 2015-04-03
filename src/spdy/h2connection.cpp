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
#include <http/hiohandlerfactory.h>
#include <http/httplog.h>
#include <http/httprespheaders.h>
#include <http/httpstatuscode.h>
#include <spdy/h2stream.h>
#include <util/iovec.h>

#include <time.h>


static const char s_h2sUpgradeResponse[] =
    "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nUpgrade: h2c\r\n\r\n";
static const char *s_clientPreface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
#define H2_CLIENT_PREFACE_LEN   24


static hash_key_t int_hash(const void *p)
{
    return (int)(long)p;
}


static int  int_comp(const void *pVal1, const void *pVal2)
{
    return (int)(long)pVal1 - (int)(long)pVal2;
}


static inline void appendNbo4Bytes(LoopBuf *pBuf, uint32_t val)
{
    pBuf->append(val >> 24);
    pBuf->append((val >> 16) & 0xff);
    pBuf->append((val >> 8) & 0xff);
    pBuf->append(val & 0xff);
}


static inline void append4Bytes(LoopBuf *pBuf, const char *val)
{
    pBuf->append(*val++);
    pBuf->append(*val++);
    pBuf->append(*val++);
    pBuf->append(*val);
}


H2Connection::H2Connection()
    : m_bufInput(4096)
    , m_uiServerStreamID(2)
    , m_uiLastPingID(0)
    , m_uiLastStreamID(0)
    , m_uiGoAwayId(0)
    , m_mapStream(50, int_hash, int_comp)
    , m_iFlag(0)
    , m_iCurDataOutWindow(H2_FCW_INIT_SIZE)
    , m_iCurInBytesToUpdate(0)
    , m_iDataInWindow(H2_FCW_INIT_SIZE)
    , m_iStreamInInitWindowSize(H2_FCW_INIT_SIZE)
    , m_iServerMaxStreams(100)
    , m_iStreamOutInitWindowSize(H2_FCW_INIT_SIZE)
    , m_iClientMaxStreams(100)
    , m_iPeerMaxFrameSize(H2_DEFAULT_DATAFRAME_SIZE)
    , m_tmIdleBegin(0)
{
}


void H2Connection::init(HiosProtocol ver)
{
    m_iCurDataOutWindow = H2_FCW_INIT_SIZE;
    m_uiLastStreamID = 0;
    m_iStreamOutInitWindowSize = H2_FCW_INIT_SIZE;
    m_iServerMaxStreams = 100;
    m_iClientMaxStreams = 100;
    m_tmIdleBegin = 0;
    m_iCurrentFrameRemain = -H2_FRAME_HEADER_SIZE;
    m_pCurH2Header = (H2FrameHeader *)m_iaH2HeaderMem;
}


int H2Connection::onInitConnected()
{
    m_iState = HIOS_CONNECTED;
    setOS(getStream());
    getStream()->continueRead();
    return 0;
}


H2Connection::~H2Connection()
{
}


int H2Connection::onReadEx()
{
    int ret = onReadEx2();
    if (!isEmpty())
        flush();
    return ret;
}


int H2Connection::verifyClientPreface()
{
    char sPreface[H2_CLIENT_PREFACE_LEN];
    m_bufInput.moveTo(sPreface, H2_CLIENT_PREFACE_LEN);
    if (memcmp(sPreface, s_clientPreface, H2_CLIENT_PREFACE_LEN) != 0)
    {
        if (D_ENABLED(DL_LESS))
        {
            LOG_D((getLogger(), "[%s] missing client PREFACE string,"
                   " got [%.24s], close.", getLogId(), sPreface));
        }
        return LS_FAIL;
    }
    m_iFlag |= H2_CONN_FLAG_PREFACE;
    return 0;
}


int H2Connection::parseFrame()
{
    while (!m_bufInput.empty())
    {
        if (m_iCurrentFrameRemain < 0)
        {
            if (m_bufInput.size() < H2_FRAME_HEADER_SIZE)
                break;
            m_bufInput.moveTo((char *)m_pCurH2Header, H2_FRAME_HEADER_SIZE);
            uint32_t id = m_pCurH2Header->getStreamId();
            if (id && (id % 2 == 0)) 
            {
                if (D_ENABLED(DL_LESS))
                    LOG_D((getLogger(), "[%s] invalid frame id %d (type %d)",
                           getLogId(), id, m_pCurH2Header->getType()));
                return LS_FAIL;
            }
            
            m_iCurrentFrameRemain = m_pCurH2Header->getLength();
            if (D_ENABLED(DL_LESS))
            {
                LOG_D((getLogger(), "[%s] frame type %d, size: %d", getLogId(),
                       m_pCurH2Header->getType(), m_iCurrentFrameRemain));
            }
            if (m_iCurrentFrameRemain > H2_DEFAULT_DATAFRAME_SIZE)
            {
                if (D_ENABLED(DL_LESS))
                    LOG_D((getLogger(), "[%s] Frame size is too large, "
                           "Max: %d, Current Frame Size: %d.", getLogId(),
                           H2_DEFAULT_DATAFRAME_SIZE, m_iCurrentFrameRemain));
                return LS_FAIL;
            }
        }
        if (!(m_iFlag & H2_CONN_FLAG_SETTING_RCVD))
        {
            if (m_pCurH2Header->getType() != H2_FRAME_SETTINGS)
            {
                if (D_ENABLED(DL_LESS))
                    LOG_D((getLogger(), "[%s] expects SETTINGS frame "
                           "as part of client PREFACE, got frame type %d",
                           getLogId(), m_pCurH2Header->getType()));
                return LS_FAIL;
            }
        }

        if ((m_iFlag & (H2_CONN_HEADERS_START | H2_CONN_HEADERS_END))
                == H2_CONN_HEADERS_START
            && m_pCurH2Header->getType() != H2_FRAME_CONTINUATION)
        {
            return LS_FAIL;
        }

        if (m_pCurH2Header->getType() != H2_FRAME_DATA)
        {
            if (m_iCurrentFrameRemain > m_bufInput.size())
                return 0;
            if (processFrame(m_pCurH2Header) == -1)
                return LS_FAIL;
            if (m_iCurrentFrameRemain > 0)
                m_bufInput.pop_front(m_iCurrentFrameRemain);
            m_iCurrentFrameRemain = -H2_FRAME_HEADER_SIZE;
        }
        else
        {
            processDataFrame(m_pCurH2Header);
            if (m_iCurrentFrameRemain == 0)
                m_iCurrentFrameRemain = -H2_FRAME_HEADER_SIZE;
        }
    }

    if (m_iDataInWindow / 2 < m_iCurInBytesToUpdate)
    {
        if (D_ENABLED(DL_LESS))
        {
            LOG_D((getLogger(), "[%s] send WINDOW_UPDATE, for %d bytes.",
                   getLogId(), m_iDataInWindow));
        }
        sendWindowUpdateFrame(0, m_iCurInBytesToUpdate);
        m_iCurInBytesToUpdate = 0;
    }

    return 0;
}


int H2Connection::onReadEx2()
{
    int n = 0, avaiLen = 0;
    while (true)
    {
        if (m_bufInput.available() < 500)
            m_bufInput.guarantee(1024);
        avaiLen = m_bufInput.contiguous();
        n = getStream()->read(m_bufInput.end(), avaiLen);

        if (n == -1)
            break;
        else if (n == 0)
            return 0;
        m_bufInput.used(n);

        if (!(m_iFlag & H2_CONN_FLAG_PREFACE))
        {
            if (m_bufInput.size() < H2_CLIENT_PREFACE_LEN)
                return 0;
            if (verifyClientPreface() == LS_FAIL)
                break;
        }
        if (parseFrame() == LS_FAIL)
        {
            doGoAway(H2_ERROR_PROTOCOL_ERROR);
            return LS_FAIL;
        }
    }
    // must start to close connection
    // must turn off READING event in the meantime
    getStream()->suspendRead();
    onCloseEx();
    return LS_FAIL;
}


int H2Connection::processFrame(H2FrameHeader *pHeader)
{
    static uint32_t requiredLength[11] =
    {  0, 0, 5, 4, 0, 0, 8, 0, 4, 0, 0 };
    
    int type = pHeader->getType();
    //m_bufInput.moveTo((char *)pHeader + H2_FRAME_HEADER_SIZE, extraHeader);
    if ( type < H2_FRAME_MAX_TYPE)
    {
        if ((requiredLength[type] != 0) 
            && (requiredLength[type] != pHeader->getLength()))
        {
            doGoAway( H2_ERROR_FRAME_SIZE_ERROR );
            return 0;
        }
    }
    memset((char *)pHeader + H2_FRAME_HEADER_SIZE, 0, 10);
    printLogMsg(pHeader);
    
    switch (type)
    {
    case H2_FRAME_HEADERS:
        return processHeadersFrame(pHeader);
    case H2_FRAME_PRIORITY:
        return processPriorityFrame(pHeader);
    case H2_FRAME_RST_STREAM:
        processRstFrame(pHeader);
        break;
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
    default:
        sendPingFrame(0, (uint8_t *)"01234567" );
        break;
    }
    return 0;
}


void H2Connection::printLogMsg(H2FrameHeader *pHeader)
{
    if (D_ENABLED(DL_LESS))
    {
        const char * message = "";
        int messageLen = 0;
        if (pHeader->getType() == H2_FRAME_GOAWAY)
        {
            message = (const char *)m_bufInput.begin() + 8;
            messageLen = pHeader->getLength() - 8;
        }
        LOG_D((getLogger(), "[%s] Received %s, size: %d, stream: %d, flag: 0x%hhx, Message: '%.*s'\n",
               getLogId(), getH2FrameName(pHeader->getType()), pHeader->getLength(),
               pHeader->getStreamId(), pHeader->getFlags(),
               messageLen, message));
    }
}


int H2Connection::processPriorityFrame(H2FrameHeader *pHeader)
{
    if (pHeader->getStreamId() == 0 )
    {
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(),
                  "[%s] bad PRIORITY frame, stream ID is zero.",
                  getLogId()));
        return LS_FAIL;
        
    }

    skipRemainData();
    return 0;
}


int H2Connection::processPushPromiseFrame(H2FrameHeader *pHeader)
{
    skipRemainData();
    doGoAway(H2_ERROR_PROTOCOL_ERROR);
    return 0;
}


int H2Connection::processSettingFrame(H2FrameHeader *pHeader)
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
    if (pHeader->getStreamId() != 0 )
    {
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(),
                  "[%s] bad SETTINGS frame, stream ID must be zero.",
                  getLogId()));
        doGoAway(H2_ERROR_PROTOCOL_ERROR);
        return 0;
        
    }
    
    int iEntries = m_iCurrentFrameRemain / 6;
    if (m_iCurrentFrameRemain % 6 != 0)
    {
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(),
                  "[%s] bad SETTINGS frame, frame size does not match.",
                  getLogId()));
        doGoAway(H2_ERROR_FRAME_SIZE_ERROR);
        return 0;
    }

    if (pHeader->getFlags() & H2_FLAG_ACK)
    {
        if (iEntries != 0)
        {
            if (D_ENABLED(DL_LESS))
                LOG_D((getLogger(),
                      "[%s] bad SETTINGS frame with ACK flag, has %d entries.",
                      getLogId(), iEntries));
            return LS_FAIL;
        }
        if (m_iFlag & H2_CONN_FLAG_SETTING_SENT)
        {
            m_iFlag |= H2_CONN_FLAG_CONFIRMED;
            return 0;
        }
        else
        {
            if (D_ENABLED(DL_LESS))
                LOG_D((getLogger(),
                      "[%s] received SETTINGS frame with ACK flag before sending our SETTINGS frame.",
                      getLogId()));
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
        if (D_ENABLED(DL_LESS))
        {
            LOG_D((getLogger(), "[%s] %s(%d) value: %d", getLogId(),
                   (iEntryID < 7) ? cpEntryNames[iEntryID] : "INVALID", iEntryID,
                   iEntryValue));
        }
        switch (iEntryID)
        {
        case H2_SETTINGS_HEADER_TABLE_SIZE:
            m_hpack.getReqDynTbl().updateMaxCapacity(iEntryValue);
            break;
        case H2_SETTINGS_MAX_FRAME_SIZE:
            if ((iEntryValue < H2_DEFAULT_DATAFRAME_SIZE) ||
                (iEntryValue > H2_MAX_DATAFRAM_SIZE))
            {
                if (D_ENABLED(DL_LESS))
                    LOG_D((getLogger(), "[%s] MAX_FRAME_SIZE value is invalid, %d.",
                          getLogId(), iEntryValue));
                return LS_FAIL;
            }
            m_iPeerMaxFrameSize = iEntryValue;
            break;
        case H2_SETTINGS_INITIAL_WINDOW_SIZE:
            if (iEntryValue > H2_FCW_MAX_SIZE )
            {
                doGoAway(H2_ERROR_FLOW_CONTROL_ERROR);
                return 0;
            }
            windowsSizeDiff = (int32_t)iEntryValue - m_iStreamOutInitWindowSize;
            m_iStreamOutInitWindowSize = iEntryValue ;
            break;
        case H2_SETTINGS_MAX_CONCURRENT_STREAMS:
            m_iClientMaxStreams = iEntryValue ;
            break;
        case H2_SETTINGS_MAX_HEADER_LIST_SIZE:
            break;
        case H2_SETTINGS_ENABLE_PUSH:
            if (iEntryValue > 1)
            {
                return LS_FAIL;
            }
        default:
            break;
        }
    }
    m_iCurrentFrameRemain = 0;
    m_iFlag |= H2_CONN_FLAG_SETTING_RCVD;

    sendSettingsFrame();
    sendFrame0Bytes(H2_FRAME_SETTINGS, H2_FLAG_ACK);

    if (windowsSizeDiff != 0)
    {
        StreamMap::iterator itn, it = m_mapStream.begin();
        for (; it != m_mapStream.end();)
        {
            it.second()->adjWindowOut(windowsSizeDiff);
            itn = m_mapStream.next(it);
            it = itn;
        }
    }
    return 0;
}


int H2Connection::processWindowUpdateFrame(H2FrameHeader *pHeader)
{
    char sTemp[4];
    m_bufInput.moveTo(sTemp, 4);
    m_iCurrentFrameRemain -= 4;
    uint32_t id = pHeader->getStreamId();
    int32_t delta = beReadUint32((const unsigned char *)sTemp);
    delta &= 0x7FFFFFFFu;
    if (delta == 0)
    {
        if (D_ENABLED(DL_LESS))
        {
            LOG_D((getLogger(),
                    "[%s] session WINDOW_UPDATE ERROR: %d",
                    getLogId(), delta));
        }
        if ( id )
            sendRstFrame(id, H2_ERROR_PROTOCOL_ERROR);
        else
            doGoAway(H2_ERROR_PROTOCOL_ERROR);
        return 0;
    }


    if (id == 0)
    {
        // || (id == 0 && ((delta % 4) != 0))
        uint32_t tmpVal = m_iCurDataOutWindow + delta;
        if (tmpVal > 2147483647)  //2^31 -1 
        {
            //sendRstFrame(pHeader->getStreamId(), H2_ERROR_PROTOCOL_ERROR);
            if (D_ENABLED(DL_LESS))
            {
                LOG_D((getLogger(),
                        "[%s] session WINDOW_UPDATE ERROR: %d, window size: %d, total %d (m_iDataInWindow: %d) ",
                        getLogId(), delta, m_iCurDataOutWindow, tmpVal, m_iDataInWindow));
            }
            doGoAway(H2_ERROR_FLOW_CONTROL_ERROR);
            return 0;
        }
        m_iCurDataOutWindow = tmpVal;
        StreamMap::iterator itn, it = m_mapStream.begin();
        for (; it != m_mapStream.end();)
        {
            it.second()->continueWrite();
            itn = m_mapStream.next(it);
            it = itn;
        }
    }
    else
    {
        if (D_ENABLED(DL_LESS))
        {
            LOG_D((getLogger(),
                   "[%s] session WINDOW_UPDATE: %d, window size: %d, streamID: %d ",
                   getLogId(), delta, m_iCurDataOutWindow, id));
        }
        H2Stream *pStream = findStream(id);
        if (pStream != NULL)
        {
            int ret = pStream->adjWindowOut(delta);
            if (ret < 0)
                sendRstFrame(id, H2_ERROR_FLOW_CONTROL_ERROR);
            else if ( pStream->adjWindowOut(delta) <= delta )
                pStream->continueWrite();
            
        }
        else
        {
            if (id <= m_uiLastStreamID)
                sendRstFrame(id, H2_ERROR_STREAM_CLOSED);
            else
                return LS_FAIL;
        }

        flush();
    }
    skipRemainData();
    m_iCurrentFrameRemain = 0;
    return 0;
}


int H2Connection::processRstFrame(H2FrameHeader *pHeader)
{
    uint32_t streamID = pHeader->getStreamId();
    H2Stream *pH2Stream = findStream(streamID);
    if (pH2Stream == NULL)
    {
        skipRemainData();
        doGoAway(H2_ERROR_PROTOCOL_ERROR);
        //sendRstFrame(streamID, H2_ERROR_PROTOCOL_ERROR);
        return 0;
    }

    unsigned char p[4];
    m_bufInput.moveTo((char *)p, 4);
    uint32_t errorCode = beReadUint32(p);
    LOG_D((getLogger(), "[%s] streamID:%d processRstFrame, error code: %d",
           getLogId(),
           streamID, errorCode));
    pH2Stream->setFlag(HIO_FLAG_PEER_RESET, 1);
    recycleStream(streamID);
    return 0;
}


void H2Connection::skipRemainData()
{
    int len = m_bufInput.size();
    if (len > m_iCurrentFrameRemain)
        len = m_iCurrentFrameRemain;
    m_bufInput.pop_front(len);
    m_iCurrentFrameRemain -= len;

    m_iCurInBytesToUpdate += len;
}


int H2Connection::processDataFrame(H2FrameHeader *pHeader)
{
    uint32_t streamID = pHeader->getStreamId();
    H2Stream *pH2Stream = findStream(streamID);
    if (pH2Stream == NULL)
    {
        skipRemainData();
        doGoAway(H2_ERROR_PROTOCOL_ERROR);
        //sendRstFrame(streamID, H2_ERROR_PROTOCOL_ERROR);
        return 0;
    }
    if (pH2Stream->isPeerShutdown())
    {
        skipRemainData();
        doGoAway(H2_ERROR_PROTOCOL_ERROR);
        //sendRstFrame(streamID, H2_ERROR_STREAM_CLOSED);
        return 0;
    }

    uint8_t padLen = 0;
    if (pHeader->getFlags() & H2_FLAG_PADDED)
    {
        padLen = (uint8_t)(*(m_bufInput.begin()));
        m_bufInput.pop_front(1);
        //Do not pop_back right now since buf may have more data then this frame
        m_iCurrentFrameRemain -= (1 + padLen);
    }

    while (m_iCurrentFrameRemain > 0)
    {
        int len = m_bufInput.blockSize();
        if (len == 0)
            break;
        if (len > m_iCurrentFrameRemain)
            len = m_iCurrentFrameRemain;
        m_iCurrentFrameRemain -= len;
        if ( pH2Stream->appendReqData(m_bufInput.begin(), len,
                                      m_iCurrentFrameRemain ? 0 
                                      : pHeader->getFlags()) == LS_FAIL)
        {
            sendRstFrame(streamID, H2_ERROR_PROTOCOL_ERROR);
        }
        m_bufInput.pop_front(len);
        m_iCurInBytesToUpdate += len;
    }

    m_bufInput.pop_front(padLen);

    if (!pH2Stream->isPeerShutdown())
    {
        if (D_ENABLED(DL_MORE))
        {
            LOG_D((getLogger(),
                   "[%s] processDataFrame() ID: %d, input window size: %d ",
                   getLogId(), streamID, pH2Stream->getWindowIn()));
        }

        if (pH2Stream->getWindowIn() < m_iStreamInInitWindowSize / 2)
        {
            sendWindowUpdateFrame(streamID, m_iStreamInInitWindowSize / 2);
            pH2Stream->adjWindowIn(m_iStreamInInitWindowSize / 2);
        }
    }
    return 0;
}


int H2Connection::processContinuationFrame(H2FrameHeader *pHeader)
{
    uint32_t id = pHeader->getStreamId();
    if (m_iFlag & H2_CONN_FLAG_GOAWAY)
        return 0;
    if ((id != m_uiLastStreamID)
        || (m_iFlag & (H2_CONN_HEADERS_START | H2_CONN_HEADERS_END))
                != H2_CONN_HEADERS_START )
    {
        return LS_FAIL;
    }

    int iDatalen = (m_bufInput.size() < m_iCurrentFrameRemain) ?
                   (m_bufInput.size()) : (m_iCurrentFrameRemain);
    if (iDatalen <= 0)
        return 0;

    H2Stream *pStream = findStream(id);
    if (pStream == NULL)
    {
        return LS_FAIL;
    }

    if (iDatalen > m_bufInput.blockSize())
    {
        int len = m_bufInput.blockSize();
        m_bufInflate.append(m_bufInput.begin(), len);
        m_bufInput.pop_front(len);
        m_iCurrentFrameRemain -= len;
        m_bufInflate.append(m_bufInput.begin(), iDatalen - len);
    }
    else
    {
        m_bufInflate.append(m_bufInput.begin(), iDatalen);
    }
    
    if (pHeader->getFlags() & H2_FLAG_END_HEADERS)
    {
        m_iFlag |= H2_CONN_HEADERS_END;
        AutoBuf tmpBuf;
        tmpBuf.swap(m_bufInflate);
        unsigned char *pSrc = (unsigned char *)tmpBuf.begin();
        return decodeHeaders(pStream, pSrc, tmpBuf.size());
    }
    else
        return 0;
}


int H2Connection::decodeHeaders(H2Stream *pStream, unsigned char *pSrc, int length)
{
    char method[10] = {0};
    int methodLen = 0;
    char *uri = NULL;
    int uriLen = 0;
    int contentLen = -1;

    unsigned char *bufEnd = pSrc + length;
    int rc = decodeData(pSrc, bufEnd, method, &methodLen, &uri, &uriLen, &contentLen);

    if (rc < 0)
        return LS_FAIL;

    if (m_mapStream.size() >= (uint)m_iServerMaxStreams)
    {
        sendRstFrame(pStream->getStreamID(), H2_ERROR_REFUSED_STREAM);
        free(uri);
        return 0;
    }

    appendReqHeaders(pStream, method, methodLen, uri, uriLen);
    if (contentLen >= 0)
        pStream->setContentlen(contentLen);
    pStream->setReqHeaderEnd(1);
    pStream->appendInputData("\r\n", 2);
    pStream->onInitConnected();

    if (pStream->getState() == HIOS_DISCONNECTED)
        recycleStream(pStream->getStreamID());

    free(uri);
    return 0;
}

int H2Connection::processHeadersFrame(H2FrameHeader *pHeader)
{
    uint32_t id = pHeader->getStreamId();
    if (m_iFlag & H2_CONN_FLAG_GOAWAY)
        return 0;

    if (id == 0)
    {
        LOG_INFO(("[%s] Protocol error, HEADER frame STREAM ID is 0",
                  getLogId()));
        return LS_FAIL;
    }

    if (id <= m_uiLastStreamID)
    {
        sendRstFrame(id, H2_ERROR_PROTOCOL_ERROR);
        LOG_INFO(("[%s] Protocol error, STREAM ID: %d is less the"
                  " previously received stream ID: %d, go away!",
                  getLogId(), id, m_uiLastStreamID));
        return LS_FAIL;
    }

    m_uiLastStreamID = id;
    Priority_st priority;
    unsigned char iHeaderFlag = pHeader->getFlags();

    m_iFlag |= H2_CONN_HEADERS_START;
    unsigned char *pSrc = (unsigned char *)m_bufInput.begin();
    if (iHeaderFlag & H2_FLAG_PADDED)
    {
        uint8_t padLen = *pSrc;
        m_bufInput.pop_front(1);
        m_bufInput.pop_back(padLen);
        m_iCurrentFrameRemain -= padLen + 1;
    }

    if (iHeaderFlag & H2_FLAG_PRIORITY)
    {
        pSrc = (unsigned char *)m_bufInput.begin();
        priority.m_exclusive = *pSrc >> 7;
        priority.m_dependStreamId = beReadUint32((const unsigned char *)pSrc) &
                                    0x7FFFFFFFu;

        //Add one to the value to obtain a weight between 1 and 256.
        priority.m_weight = *(pSrc + 4) + 1;
        m_bufInput.pop_front(5);
        m_iCurrentFrameRemain -= 5;
    }
    else
        memset(&priority, 0, sizeof(priority));

    int iDatalen = (m_bufInput.size() < m_iCurrentFrameRemain) ?
                   (m_bufInput.size()) : (m_iCurrentFrameRemain);
    if (iDatalen <= 0)
        return 0;

    H2Stream *pStream = getNewStream(id, iHeaderFlag, priority);
    if (!pStream)
    {
        sendRstFrame(id, H2_ERROR_INTERNAL_ERROR);
        return 0;
    }
    
    //Check if we need to move the buffer to stream bufinflat
    bool needToMove = false;
    int bufPart1Len = m_bufInput.blockSize();
    if (((iHeaderFlag & H2_FLAG_END_HEADERS) == 0)
        || iDatalen > bufPart1Len)
    {
        needToMove = true;
        m_bufInflate.clear();
        if (iDatalen > bufPart1Len)
        {
            m_bufInflate.append(m_bufInput.begin(), bufPart1Len);
            m_bufInput.pop_front(bufPart1Len);
            m_iCurrentFrameRemain -= bufPart1Len;
            m_bufInflate.append(m_bufInput.begin(), iDatalen - bufPart1Len);
        }
        else
        {
            m_bufInflate.append(m_bufInput.begin(), iDatalen);
        }
    }

    if (iHeaderFlag & H2_FLAG_END_HEADERS)
    {
        m_iFlag |= H2_CONN_HEADERS_END;
        if (needToMove)
        {
            AutoBuf tmpBuf;
            tmpBuf.swap(m_bufInflate);
            pSrc = (unsigned char *)tmpBuf.begin();
        }
        else
            pSrc = (unsigned char *)m_bufInput.begin();
        return decodeHeaders(pStream, pSrc, iDatalen);
    }
    else
        return 0;
}


int H2Connection::decodeData(unsigned char *pSrc, unsigned char *bufEnd, 
                             char *method, int *methodLen, char **uri, 
                             int *uriLen, int *contentLen)
{
    int rc, n = 0;
    m_bufInflate.clear();

    AutoBuf namevaleBuf(8192);
    uint16_t name_len = 0 ;
    uint16_t val_len = 0;
    AutoBuf cookieStr( 0 );
    int regular_header = 0;
    
    bool authority = false;
    bool scheme = false;
    bool error = false;
    while ((rc = m_hpack.decHeader(pSrc, bufEnd, namevaleBuf,
                                   name_len, val_len)) > 0)
    {
        char *name = namevaleBuf.begin();
        char *val = name + name_len;

        if (name[0] == ':')
        {
            if (regular_header)
                error = true;
            name_len = 0;
            if (memcmp(name, ":authority", 10) == 0)
            {
                if (authority)
                {
                    error = true;
                    break;
                }
                name = (char *)"host";
                name_len = 4;
                authority = true;
            }
            else if (memcmp(name, ":method", 7) == 0)
            {
                //If second time have the :method, ERROR
                if (*methodLen)
                {
                    error = true;
                    break;
                }

                if (val_len < 10 && val_len > 2)
                {
                    strncpy(method, val, val_len);
                    *methodLen = val_len;
                }
            }
            else if (memcmp(name, ":path", 5) == 0)
            {
                //If second time have the :path, ERROR
                if (*uri)
                {
                    error = true;
                    break;
                }
                *uri = strndup(val, val_len);
                *uriLen = val_len;
            }
            else if (memcmp(name, ":scheme", 7) == 0)
            {
                if (scheme)
                {
                    error = true;
                    break;
                }
                scheme = true;
                //Do nothing
                //We set to (char *)"HTTP/1.1"
            }
            else
                error = true;
        }
        else
        {
            regular_header = true;
            if (name_len == 6 && memcmp(name, "cookie", 6) == 0)
            {
                if (cookieStr.size() > 0)
                    cookieStr.append("; ", 2);

                cookieStr.append(val, val_len);
                name_len = 0;
            }
            else if (name_len == 2 && memcmp(name, "te", 2) == 0 )
            {
                if ( val_len != 8 || strncasecmp("trailers", val, 8) != 0 )
                    return LS_FAIL;
            }
            else if (name_len == 14 && memcmp(name, "content-length", 14) == 0)
            {
                char * pEnd = val + val_len;
                int64_t len = strtoll(val, &pEnd, 10);
                if (len < INT_MAX)
                    *contentLen = len;
            }
        }

        if (name_len > 0)
        {
            m_bufInflate.append(name, name_len);
            m_bufInflate.append(": ", 2);
            m_bufInflate.append(val, val_len);
            m_bufInflate.append("\r\n", 2);
            n += name_len + val_len + 4;
        }
    }

    if (error || rc < 0 || methodLen == 0 || uriLen == 0 || *uri == NULL)
    {
        if (*uri)
            free(*uri);
        return -1;
    }

    /*TODO: we suppose cookies are in one frame, if in another frame, it will
     * create multi-line cookie in req header.
     */
    if (cookieStr.size() > 0)
    {
        m_bufInflate.append("cookie", 6);
        m_bufInflate.append(": ", 2);
        m_bufInflate.append(cookieStr.begin(), cookieStr.size());
        m_bufInflate.append("\r\n", 2);
        n += (6 + cookieStr.size() + 4);
    }

    return n;
}


int H2Connection::appendReqHeaders(H2Stream *pStream, char *method, 
                                   int methodLen, char *uri, int uriLen)
{
    if (method && methodLen && uri && uriLen)
    {
        pStream->appendInputData(method, methodLen);
        pStream->appendInputData(" ", 1);
        pStream->appendInputData(uri, uriLen);
        pStream->appendInputData(" HTTP/1.1\r\n", 11);
    }

    pStream->appendInputData(m_bufInflate.begin(), m_bufInflate.size());

    return 0;
}


H2Stream *H2Connection::getNewStream(uint32_t uiStreamID,
                                     uint8_t ubH2_Flags, Priority_st &priority)
{
    H2Stream *pStream;
    HioHandler *pSession =
        HioHandlerFactory::getHioHandler(HIOS_PROTO_HTTP);
    if (!pSession)
        return NULL;
    pStream = new H2Stream();
    m_mapStream.insert((void *)(long)uiStreamID, pStream);
    if (D_ENABLED(DL_MORE))
    {
        LOG_D((getLogger(), "[%s] getNewStream(), ID: %d, stream map size: %d ",
               getLogId(), uiStreamID, m_mapStream.size()));
    }
    pStream->init(uiStreamID, this, ubH2_Flags, pSession, &priority);
    pStream->setProtocol(HIOS_PROTO_HTTP2);
    pStream->setFlag(HIO_FLAG_FLOWCTRL, 1);
    return pStream;
}


void H2Connection::upgradedStream(HioHandler *pSession)
{
    assert(pSession != NULL);
    H2Stream *pStream = new H2Stream();
    uint32_t uiStreamID = 1;  //Through upgrade h2c, it is 1.
    m_mapStream.insert((void *)(long)uiStreamID, pStream);
    pStream->init(uiStreamID, this, 0, pSession);
    pStream->setProtocol(HIOS_PROTO_HTTP2);
    pStream->setFlag(HIO_FLAG_FLOWCTRL, 1);
    pStream->setReqHeaderEnd(1);
    onInitConnected();
    getBuf()->append(s_h2sUpgradeResponse, sizeof(s_h2sUpgradeResponse) - 1);
    sendSettingsFrame();
    pStream->onInitConnected(true);
}


void H2Connection::recycleStream(uint32_t uiStreamID)
{
    StreamMap::iterator it = m_mapStream.find((void *)(long) uiStreamID);
    if (it == m_mapStream.end())
        return;
    recycleStream(it);
}


void H2Connection::recycleStream(StreamMap::iterator it)
{
    H2Stream *pH2Stream = it.second();
    m_mapStream.erase(it);
    pH2Stream->close();

    //TODO: use array for m_dqueStreamRespon
    //m_dqueStreamRespon[pH2Stream->getPriority()].remove(pH2Stream);
    m_dqueStreamRespon.remove(pH2Stream);
    if (pH2Stream->getHandler())
        pH2Stream->getHandler()->recycle();

    if (D_ENABLED(DL_MORE))
    {
        LOG_D((getLogger(), "[%s] recycleStream(), ID: %d, stream map size: %d "
               , getLogId(), pH2Stream->getStreamID(), m_mapStream.size()));
    }
    //H2StreamPool::recycle( pH2Stream );
    delete pH2Stream;
}


int H2Connection::sendFrame8Bytes(H2FrameType type, uint32_t uiStreamId,
                                  uint32_t uiVal1, uint32_t uiVal2)
{
    getBuf()->guarantee(17);
    appendCtrlFrameHeader(type, 8, 0, uiStreamId);
    appendNbo4Bytes(getBuf(), uiVal1);
    appendNbo4Bytes(getBuf(), uiVal2);
    flush();
    if (D_ENABLED(DL_MORE))
    {
        LOG_D((getLogger(), "[%s] send %s frame, stream: %d, value: %d"
               , getLogId(), getH2FrameName(type), uiVal1, uiVal2));
    }
    return 0;
}


int H2Connection::sendFrame4Bytes(H2FrameType type, uint32_t uiStreamId,
                                  uint32_t uiVal1)
{
    getBuf()->guarantee(13);
    appendCtrlFrameHeader(type, 4, 0, uiStreamId);

    appendNbo4Bytes(getBuf(), uiVal1);
    flush();
    if (D_ENABLED(DL_MORE))
    {
        LOG_D((getLogger(), "[%s] send %s frame, value: %d", getLogId(),
               getH2FrameName(type), uiVal1));
    }
    return 0;
}


int H2Connection::sendFrame0Bytes(H2FrameType type, unsigned char flags)
{
    getBuf()->guarantee(9);
    appendCtrlFrameHeader(type, 0, flags);
    flush();
    if (D_ENABLED(DL_MORE))
    {
        LOG_D((getLogger(), "[%s] send %s frame, with Flag: %d"
               , getLogId(), getH2FrameName(type), flags));
    }
    return 0;
}


int H2Connection::sendPingFrame(uint8_t flags, uint8_t *pPayload)
{
    //Server initiate a Ping
    getBuf()->guarantee(17);
    appendCtrlFrameHeader(H2_FRAME_PING, 8, flags);
    getBuf()->append((char *)pPayload, H2_PING_FRAME_PAYLOAD_SIZE);
    flush();
    return 0;
}


int H2Connection::sendSettingsFrame()
{
    if (m_iFlag & H2_CONN_FLAG_SETTING_SENT)
        return 0;

    static char s_settings[][6] =
    {
        //{0x00, 0x01, 0x00, 0x00, 0x10, 0x00 },
        {0x00, 0x03, 0x00, 0x00, 0x00, 0x64 },
        {0x00, 0x04, 0x00, 0x01, 0x00, 0x00 },
        //{0x00, 0x05, 0x00, 0x00, 0x40, 0x00 },
        //{0x00, 0x06, 0x00, 0x00, 0x40, 0x00 },
    };

    getBuf()->guarantee(21);
    appendCtrlFrameHeader(H2_FRAME_SETTINGS, 12);
    getBuf()->append((char *)s_settings, 12);
    if (D_ENABLED(DL_MORE))
    {
        LOG_D((getLogger(),
               "[%s] send SETTING frame, MAX_CONCURRENT_STREAMS: %d,"
               "  INITIAL_WINDOW_SIZE: %d"
               , getLogId(), m_iServerMaxStreams, m_iStreamInInitWindowSize));
    }
    m_iFlag |= H2_CONN_FLAG_SETTING_SENT;

    sendWindowUpdateFrame( 0, H2_FCW_INIT_SIZE );
    m_iDataInWindow += H2_FCW_INIT_SIZE;

    return 0;
}


int H2Connection::processPingFrame(H2FrameHeader *pHeader)
{
    struct timeval CurTime;
    long msec;
    if (pHeader->getStreamId() != 0)
    {
        //doGoAway(H2_ERROR_PROTOCOL_ERROR);
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(), "[%s] invalid PING frame id %d",
                    getLogId(), pHeader->getStreamId()));
        return LS_FAIL;
    }

    if (!(pHeader->getFlags() & H2_FLAG_ACK))
    {
        uint8_t payload[H2_PING_FRAME_PAYLOAD_SIZE];
        m_bufInput.moveTo((char *)payload, H2_PING_FRAME_PAYLOAD_SIZE);
        return sendPingFrame(H2_FLAG_ACK, payload);
    }

    gettimeofday(&CurTime, NULL);
    msec = (CurTime.tv_sec - m_timevalPing.tv_sec) * 1000;
    msec += (CurTime.tv_usec - m_timevalPing.tv_usec) / 1000;
    if (D_ENABLED(DL_MORE))
    {
        LOG_D((getLogger(), "[%s] Received PING, ID=%d, Round trip "
               "times=%d milli-seconds", getLogId(), m_uiLastPingID, msec));
    }
    return 0;
}


H2Stream *H2Connection::findStream(uint32_t uiStreamID)
{
    StreamMap::iterator it = m_mapStream.find((void *)(long)uiStreamID);
    if (it == m_mapStream.end())
        return NULL;
    return it.second();
}


int H2Connection::flush()
{
    BufferedOS::flush();
    if (!isEmpty())
        getStream()->continueWrite();
    getStream()->flush();
    return LS_DONE;
}


int H2Connection::onCloseEx()
{
    if (getStream()->isReadyToRelease())
        return 0;
    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] H2Connection::onCloseEx() ", getLogId()));
    
    getStream()->setState(HIOS_CLOSING);
    releaseAllStream();
    return 0;
};


int H2Connection::onTimerEx()
{
    int result = 0;
    if (m_iFlag & H2_CONN_FLAG_GOAWAY)
        result = releaseAllStream();
    else
        result = timerRoutine();
    return result;
}


int H2Connection::processGoAwayFrame(H2FrameHeader *pHeader)
{
    if (!(m_iFlag & H2_CONN_FLAG_GOAWAY))
        doGoAway(H2_ERROR_NO_ERROR);
    m_iFlag |= (short)H2_CONN_FLAG_GOAWAY;

    onCloseEx();
    return true;
}


int H2Connection::doGoAway(H2ErrorCode status)
{
    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] H2Connection::doGoAway(), status = %d ",
               getLogId(), status));
    sendGoAwayFrame(status);
    releaseAllStream();
    getStream()->setState(HIOS_CLOSING);
    getStream()->continueWrite();
    return 0;
}


int H2Connection::sendGoAwayFrame(H2ErrorCode status)
{
    sendFrame8Bytes(H2_FRAME_GOAWAY, 0, m_uiLastStreamID, status);
    return 0;
}


int H2Connection::releaseAllStream()
{
    StreamMap::iterator itn, it = m_mapStream.begin();
    for (; it != m_mapStream.end();)
    {
        itn = m_mapStream.next(it);
        recycleStream(it);
        it = itn;
    }
    getStream()->handlerReadyToRelease();
    return 0;
}


int H2Connection::timerRoutine()
{
    StreamMap::iterator itn, it = m_mapStream.begin();
    for (; it != m_mapStream.end();)
    {
        itn = m_mapStream.next(it);
        it.second()->onTimer();
        if (it.second()->getState() == HIOS_DISCONNECTED)
            recycleStream(it);
        it = itn;
    }
    if (m_mapStream.size() == 0)
    {
        if (m_tmIdleBegin == 0)
            m_tmIdleBegin = time(NULL);
        else if (time(NULL) - m_tmIdleBegin > 60)
            doGoAway(H2_ERROR_NO_ERROR);
    }
    else
        m_tmIdleBegin = 0;
    return 0;
}


#define H2_TMP_HDR_BUFF_SIZE 16384
#define MAX_LINE_COUNT_OF_MULTILINE_HEADER  100

int H2Connection::encodeHeaders(HttpRespHeaders *pRespHeaders,
                                  unsigned char *buf, int maxSize)
{
    unsigned char *pCur = buf;
    unsigned char *pBufEnd = pCur + maxSize;

    iovec iov[MAX_LINE_COUNT_OF_MULTILINE_HEADER];
    iovec *pIov;
    iovec *pIovEnd;
    int count;
    char *key;
    int keyLen;

    char *p = (char *)HttpStatusCode::getInstance().getCodeString(
                  pRespHeaders->getHttpCode()) + 1;
    pCur = m_hpack.encHeader(pCur, pBufEnd, (char *)":status", 7, p, 3, 0);

    for (int pos = pRespHeaders->HeaderBeginPos();
         pos != pRespHeaders->HeaderEndPos();
         pos = pRespHeaders->nextHeaderPos(pos))
    {
        count = pRespHeaders->getHeader(pos, &key, &keyLen, iov,
                                        MAX_LINE_COUNT_OF_MULTILINE_HEADER);

        if (count <= 0)
            continue;

        if (keyLen + 8 > pBufEnd - pCur)
            return LS_FAIL;

        pIov = iov;
        pIovEnd = &iov[count];
        for (pIov = iov; pIov < pIovEnd; ++pIov)
            pCur = m_hpack.encHeader(pCur, pBufEnd, key, keyLen,
                                     (char *)pIov->iov_base, pIov->iov_len, 0);
    }

    return pCur - buf;;
}


int H2Connection::sendRespHeaders(HttpRespHeaders *pRespHeaders,
                                  uint32_t uiStreamID)
{
    if (D_ENABLED(DL_MORE))
    {
        LOG_D((getLogger(), "[%s] sendRespHeaders(), ID: %d"
               , getLogId(), uiStreamID));
    }

    unsigned char achHdrBuf[H2_TMP_HDR_BUFF_SIZE];
    int rc = encodeHeaders(pRespHeaders, achHdrBuf, H2_TMP_HDR_BUFF_SIZE);
    if (rc < 0)
        return LS_FAIL;

    getBuf()->guarantee(rc + 9);
    appendCtrlFrameHeader(H2_FRAME_HEADERS, rc,   H2_FLAG_END_HEADERS,
                          uiStreamID);   //H2_FLAG_END_STREAM
    getBuf()->append((const char *)achHdrBuf, rc);
    flush();
    return 0;
}


void H2Connection::add2PriorityQue(H2Stream *pH2Stream)
{
    //TODO: use array for m_dqueStreamRespon
    //m_dqueStreamRespon[pH2Stream->getPriority()].append(pH2Stream);
    m_dqueStreamRespon.append(pH2Stream);
}


int H2Connection::onWriteEx()
{
    H2Stream *pH2Stream = NULL;
    int wantWrite = 0;

    if (D_ENABLED(DL_MORE))
    {
        LOG_D((getLogger(), "[%s] onWriteEx() state: %d, output buffer size=%d\n ",
               getLogId(), m_iState, getBuf()->size()));
    }
    flush();
    if (!isEmpty())
        return 0;
    if (getStream()->canWrite() & HIO_FLAG_BUFF_FULL)
        return 0;


//TODO: use array for m_dqueStreamRespon
//     for (int i = 0; i < H2_STREAM_PRIORITYS; ++i)
//     {
//         if (m_dqueStreamRespon[i].empty())
//             continue;
//         DLinkedObj *it = m_dqueStreamRespon[i].begin();//H2Stream*
//         DLinkedObj *itn;
//         for (; it != m_dqueStreamRespon[i].end();)
//         {
//             pH2Stream = (H2Stream *)it;
//             itn = it->next();
//             if (pH2Stream->isWantWrite())
//             {
//                 pH2Stream->onWrite();
//                 if (pH2Stream->isWantWrite() && (pH2Stream->getWindowOut() > 0))
//                     ++wantWrite;
//             }
//             if (pH2Stream->getState() == HIOS_DISCONNECTED)
//                 recycleStream(pH2Stream->getStreamID());
//             it = itn;
//         }
//         if (getStream()->canWrite() & HIO_FLAG_BUFF_FULL)
//             return 0;
//     }

//     if (m_dqueStreamRespon.empty())
//         return 0;

    DLinkedObj *it = m_dqueStreamRespon.begin();//H2Stream*
    DLinkedObj *itn;
    for (; it != m_dqueStreamRespon.end();)
    {
        pH2Stream = (H2Stream *)it;
        itn = it->next();
        if (pH2Stream->isWantWrite())
        {
            pH2Stream->onWrite();
            if (pH2Stream->isWantWrite() && (pH2Stream->getWindowOut() > 0))
                ++wantWrite;
        }
        if (pH2Stream->getState() == HIOS_DISCONNECTED)
            recycleStream(pH2Stream->getStreamID());
        it = itn;
    }

    if (wantWrite == 0)
        getStream()->suspendWrite();
    return 0;
}


void H2Connection::recycle()
{
    if (D_ENABLED(DL_MORE))
        LOG_D((getLogger(), "[%s] H2Connection::recycle()", getLogId()));
    delete this;
}


NtwkIOLink *H2Connection::getNtwkIoLink()
{
    return getStream()->getNtwkIoLink();
}

