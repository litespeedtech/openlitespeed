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
#ifndef H2CONNBASE_H
#define H2CONNBASE_H

#include <edio/bufferedos.h>
#include <h2/h2protocol.h>
#include <http/ls_http_header.h>
#include <log4cxx/logsession.h>
#include <util/autobuf.h>
#include <util/dlinkqueue.h>
#include <lstl/thash.h>

#include <lsdef.h>

#include <sys/time.h>
#include <limits.h>

#include "lshpack.h"

enum h2flag
{
    H2_CONN_FLAG_GOAWAY         = (1<<0),
    H2_CONN_FLAG_PREFACE        = (1<<1),
    H2_CONN_FLAG_SETTING_RCVD   = (1<<2),
    H2_CONN_FLAG_SETTING_SENT   = (1<<3),
    H2_CONN_FLAG_CONFIRMED      = (1<<4),
    H2_CONN_FLAG_FLOW_CTRL      = (1<<5),
    H2_CONN_HEADERS_START       = (1<<6),
    H2_CONN_FLAG_WAIT_PROCESS   = (1<<7),
    H2_CONN_FLAG_NO_PUSH        = (1<<8),
    H2_CONN_FLAG_WANT_FLUSH     = (1<<9),
    H2_CONN_FLAG_IN_EVENT       = (1<<10),
    H2_CONN_FLAG_PAUSE_READ     = (1<<11),
    H2_CONN_FLAG_DIRECT_BUF     = (1<<12),
    H2_CONN_FLAG_AUTO_RECYCLE   = (1<<13),
    H2_CONN_FLAG_PENDING_STREAM = (1<<14),
};

inline enum h2flag operator|(enum h2flag a, enum h2flag b)
{
    return static_cast<enum h2flag>((uint)a | (uint)b);
}
inline enum h2flag operator~(enum h2flag a)
{
    return static_cast<enum h2flag>(~(uint)a);
}


#define H2_STREAM_PRIORITYS         (8)

class H2StreamBase;
class InputStream;
class UnpackedHeaders;

class H2ConnBase: public BufferedOS
{
public:
    H2ConnBase();
    virtual ~H2ConnBase();
    int onReadEx2();

    int isOutBufFull() const
    {
        return ((m_iCurDataOutWindow <= 0) || (getBuf()->size() >= 32768));
    }

    int getAllowedDataSize(int wanted) const
    {
        if (wanted > m_iCurDataOutWindow)
            wanted = m_iCurDataOutWindow;
        if (wanted > m_iPeerMaxFrameSize)
            wanted = m_iPeerMaxFrameSize;
        if (isDirectBuffer())
        {
            if (getBuf()->size() > 0)
                return 0;
        }
        else
            if (getBuf()->size() > 32768 && wanted > 4096)
                wanted = 4096;
        return wanted;
    }

    virtual LogSession* getLogSession() const = 0;
    virtual void suspendRead() = 0;
    virtual InputStream* getInStream() = 0;

    virtual int flush() = 0;

    virtual int onCloseEx() = 0;

    virtual void recycle() = 0;

    virtual void continueWrite() = 0;

    virtual bool isPauseWrite()  const = 0;
    virtual int assignStreamHandler(H2StreamBase *stream) = 0;
    virtual int verifyStreamId(uint32_t id)  {   return LS_OK;   }

    char *getDirectOutBuf(uint32_t stream_id, size_t &size);
    void directOutBufUsed(uint32_t stream_id, size_t len);

    static UnpackedHeaders *createUnpackedHdr(
        bool is_http, const ls_str_t *method, const ls_str_t *host,
        const ls_str_t *url, const ls_http_header_t *extra);
    static UnpackedHeaders *createUnpackedHdr(const char *method,
        const char *host, const char *url, ls_http_header_t *extra);

    void needWriteEvent()
    {
        if ((m_h2flag & H2_CONN_FLAG_IN_EVENT) == 0)
        {
            if (isEmpty())
                continueWrite();
        }
        else
            set_h2flag(H2_CONN_FLAG_WAIT_PROCESS);
    }

    virtual int onWriteEx2() = 0;

    virtual int decodeHeaders(uint32_t id, unsigned char *src, int length,
                              unsigned char iHeaderFlag) = 0;

    void set_h2flag(h2flag flag)
    {
        m_h2flag = m_h2flag | flag;   }
    void clr_h2flag(h2flag flag)
    {
        m_h2flag = (enum h2flag)(m_h2flag & (~flag));    }

    void wantFlush()
    {
        if ((m_h2flag & H2_CONN_FLAG_IN_EVENT))
            set_h2flag(H2_CONN_FLAG_WANT_FLUSH);
        else
            continueWrite();
    }

    void wantFlush2()
    {
        if ((m_h2flag & H2_CONN_FLAG_IN_EVENT))
            set_h2flag(H2_CONN_FLAG_WANT_FLUSH);
        else if (getBuf()->empty())
            continueWrite();
    }


    virtual int doGoAway(H2ErrorCode status) = 0;

    virtual int appendSendfileOutput(int fd, off_t off, int size) = 0;

    int init();

    void add2PriorityQue(H2StreamBase *streamBase);
    void removePriQue(H2StreamBase *streamBase);

    int appendOutput(const char *data, int size);
    int appendOutput(IOVec *pIov, int size);
    int guaranteeOutput(const char *data, int size)
    {
        int ret = appendOutput(data, size);
        if (ret > 0)
            wantFlush();
        return ret;
    }

    int32_t getStreamInInitWindowSize() const
    {   return m_iStreamInInitWindowSize;    }

    int32_t getStreamOutInitWindowSize() const
    {   return m_iStreamOutInitWindowSize;    }

    int32_t getCurDataOutWindow() const
    {   return m_iCurDataOutWindow;         }

    int32_t getPeerMaxFrameSize() const
    {   return m_iPeerMaxFrameSize;         }


    int sendHeaderContFrame(uint32_t uiStreamID, uint8_t flag,
                           H2FrameType type, const char *pBuf, int size);

    void sendStreamWindowUpdateFrame(uint32_t id, int32_t delta)
    {
        sendFrame4Bytes(H2_FRAME_WINDOW_UPDATE, id, delta, false);
        updateWindow();
    }

    int sendRstFrame(uint32_t uiStreamID, H2ErrorCode code)
    {
        return sendFrame4Bytes(H2_FRAME_RST_STREAM, uiStreamID, code, 0);
    }

    int sendFinFrame(uint32_t uiStreamID)
    {
        return sendFrame0Bytes(H2_FRAME_DATA, H2_FLAG_END_STREAM, uiStreamID);
    }

    int sendDataFrame(uint32_t uiStreamID, int flag, IOVec *pIov, int total);
    int sendDataFrame(uint32_t uiStreamId, int flag, const char *pBuf,
                      int len);
    int sendfileDataFrame(uint32_t uiStreamId, int flag, int fd,
                          off_t off, int len);

    void recycleStream(uint32_t uiStreamID);
    virtual void recycleStream(H2StreamBase *stream) = 0;

//     {
//         if ((m_iFlag & H2_CONN_FLAG_IN_EVENT) == 0)
//         {
//             if (isEmpty() && !getStream()->isWantWrite())
//                 getStream()->continueWrite();
//         }
//         else
//             m_iFlag |= H2_CONN_FLAG_WAIT_PROCESS;
//     }

    void incShutdownStream()    {   ++m_uiShutdownStreams;  }
    void decShutdownStream()    {   --m_uiShutdownStreams;  }
    int resetStream(uint32_t id, H2StreamBase *pStream, H2ErrorCode code);
    int getWeightedPriority(H2StreamBase* s);

    bool isDirectBuffer() const
    {   return m_h2flag & H2_CONN_FLAG_DIRECT_BUF;   }

    virtual ssize_t directBuffer(const char *data, size_t size)
    {   return -1;  }

    int sendReqHeaders(uint32_t id, uint32_t promise_streamId,
                       int flag, UnpackedHeaders *req_hdrs);


protected:
    typedef Thash<H2StreamBase, uint32_t, uint32_t, H2StreamHasher> StreamMap;

    H2StreamBase *findStream(uint32_t uiStreamID);
    int releaseAllStream();

    int processFrame(H2FrameHeader *pHeader);
    void printLogMsg(H2FrameHeader *pHeader);

    int checkReqline(char *pName, int ilength, uint8_t &flags);

    int processDataFrame(H2FrameHeader *pHeader);
    int processDataFrameDirectBuffer(H2FrameHeader *pHeader);
    int processDataFramePadding(H2FrameHeader *pHeader);
    int processDataFrameSkipRemain(H2FrameHeader *pHeader);

    int parseHeaders(char *pHeader, int ilength, int &NVPairCnt);

    int processPriorityFrame(H2FrameHeader *pHeader);
    int processSettingFrame(H2FrameHeader *pHeader);
    int processHeadersFrame(H2FrameHeader *pHeader);
    int processHeaderFrame(H2FrameHeader *pHeader);
    int processPingFrame(H2FrameHeader *pHeader);
    int processGoAwayFrame(H2FrameHeader *pHeader);
    int processRstFrame(H2FrameHeader *pHeader);
    int processWindowUpdateFrame(H2FrameHeader *pHeader);
    int processPushPromiseFrame(H2FrameHeader *pHeader);
    int processContinuationFrame(H2FrameHeader *pHeader);

    int processHeaderIn(uint32_t id, unsigned char iHeaderFlag);
    int processPriority(uint32_t id);

    int sendPingFrame(uint8_t flags, uint8_t *pPayload);
    int sendGoAwayFrame(H2ErrorCode status);

    int  sendFrame8Bytes(H2FrameType type, uint32_t uiStreamId,
                         uint32_t uiVal1, uint32_t uiVal2);
    int  sendFrame4Bytes(H2FrameType type, uint32_t uiStreamId,
                         uint32_t uiVal2, bool immed_flush);
    int  sendFrame0Bytes(H2FrameType type, uint8_t  flags,
                         uint32_t uiStreamId);

    int sendSettingsFrame(bool disable_push);

    void skipRemainData();

    int timerRoutine();
    void consumedWindowIn(int bytes);
    void updateWindow();

    int verifyClientPreface();
    int parseFrame();
    int processInput();
    int encodeReqHeaders(unsigned char* buf, unsigned char* buf_end, const UnpackedHeaders* hdrs);

    int processQueue();
    int processPendingStreams();

    void closePendingOut()
    {
        if (m_pendingOutSize)
            closePendingOut2();
    }
    void closePendingOut2();

    virtual void disableHttp2ByIp()  {};

private:
    int bufferOutput(const char *data, int size);
    int tryReadFrameHeader();

protected:
    LoopBuf             m_bufInput;
    AutoBuf             m_bufInflate;
    TDLinkQueue<H2StreamBase>  m_priQue[H2_STREAM_PRIORITYS];
    StreamMap           m_mapStream;
    struct lshpack_enc  m_hpack_enc;
    struct lshpack_dec  m_hpack_dec;

    int32_t         m_iCurrentFrameRemain;
    int32_t         m_iCurDataOutWindow;
    int32_t         m_iDataInWindow;
    int32_t         m_iStreamInInitWindowSize;
    int32_t         m_iServerMaxStreams;
    int32_t         m_iStreamOutInitWindowSize;
    int32_t         m_iMaxPushStreams;
    int32_t         m_iPeerMaxFrameSize;
    uint32_t        m_uiPushStreamId;

    uint32_t        m_uiLastStreamId;
    uint32_t        m_uiStreams;
    uint32_t        m_uiRstStreams;
    H2StreamBase *  m_current;
    struct timeval  m_timevalPing;
    Priority_st     m_priority;

    uint32_t        m_uiShutdownStreams;
    uint32_t        m_uiGoAwayId;
    int32_t         m_iCurPushStreams;
    uint32_t        m_tmLastFrameIn;
    uint32_t        m_tmLastTimer;
    int32_t         m_iInBytesToUpdate;
    int32_t         m_tmIdleBegin;

    uint32_t        m_pendingStreamId;
    enum h2flag     m_h2flag;
    uint16_t        m_pendingOutSize;
    uint16_t        m_pendingUsed;
    short           m_iControlFrames;
    char            m_inputState;
    uint8_t         m_padLen;
    H2FrameHeader   m_curH2Header;


    enum
    {
        ST_CONTROL_FRAME,
        ST_HEADER_PAYLOAD,
        ST_DATA_PAYLOAD,
        ST_SKIP_REMAIN,
        //ST_PADDING,
    };

    LS_NO_COPY_ASSIGN(H2ConnBase);
};

#endif // H2CONNECTION_H
