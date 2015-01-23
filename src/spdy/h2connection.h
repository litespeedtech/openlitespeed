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
#ifndef H2CONNECTION_H
#define H2CONNECTION_H
#include "h2protocol.h"
#include "hpack.h"
#include <edio/bufferedos.h>
#include "util/autobuf.h"
#include <util/dlinkqueue.h>
#include <util/ghash.h>
#include "http/hiostream.h"
#include <sys/time.h>

#include <limits.h>

#define H2_CONN_FLAG_GOAWAY         (1<<0)
#define H2_CONN_FLAG_PREFACE        (1<<1)
#define H2_CONN_FLAG_SETTING_RCVD   (1<<2)
#define H2_CONN_FLAG_SETTING_SENT   (1<<3)
#define H2_CONN_FLAG_CONFIRMED      (1<<4)


#define H2_STREAM_PRIORITYS         (256 + 1)


class H2Stream;

class H2Connection: public HioStreamHandler, public BufferedOS
{
public:
    H2Connection();
    virtual ~H2Connection();
    int onReadEx();
    int onReadEx2();
    int onWriteEx();

    int isOutBufFull() const
    {   return ((m_iCurDataOutWindow <= 0 )||(getBuf()->size() >= H2_MAX_DATAFRAM_SIZE)); }

    int flush();
    
    int onCloseEx();

    void recycle();
    
    //Following functions are just placeholder

    //Placeholder
    int init();
    int onInitConnected();

    int onTimerEx();
    void move2ReponQue(H2Stream* pH2Stream);
    int timerRoutine();

    LOG4CXX_NS::Logger* getLogger() const   
    {
        return getStream()->getLogger();
    }

    const char * getLogId() 
    {
        return getStream()->getLogId();
    }
    void continueWrite()
    {   getStream()->continueWrite();   }

    int32_t getStreamInInitWindowSize() const
    {   return m_iStreamInInitWindowSize;    }

    int32_t getStreamOutInitWindowSize() const
    {   return m_iStreamOutInitWindowSize;    }
    
    int32_t getCurDataOutWindow() const
    {   return m_iCurDataOutWindow;         }

    int addBufToGzip(char *hdrBuf, unsigned int& szHdrBuf, int iH2Ver, struct iovec *iov, int iov_count, LoopBuf *buf, int &total, int flushWhenEnd = 0);
    int addBufToGzip(char *hdrBuf, unsigned int& szHdrBuf, int iH2Ver, const char *s, int len, LoopBuf *buf, int &total);
    int sendRespHeaders(HttpRespHeaders *pRespHeaders, uint32_t uiStreamID);

    int sendWindowUpdateFrame( uint32_t id, int32_t delta )
    {   return sendFrame4Bytes( H2_FRAME_WINDOW_UPDATE, id, delta );   } 

    int sendRstFrame( uint32_t uiStreamID, H2ErrorCode code )
    {
        return sendFrame4Bytes( H2_FRAME_RST_STREAM, uiStreamID, code );
    }
    int sendFinFrame( uint32_t uiStreamID )
    {
        char achHeader[9];
        H2FrameHeader header( 0, H2_FRAME_DATA, H2_FLAG_END_STREAM, uiStreamID );
        memcpy(achHeader, (char *)&header, 9);         
        return cacheWrite( achHeader, 9 );
    }

    void dataFrameSent( int bytes )
    {   
        m_iCurDataOutWindow -= bytes;
    }
    
    void createInitH2cUpgradedStream( HioStreamHandler *pSession );

        
    void recycleStream( uint32_t uiStreamID );
    static void replaceZero( char* pValue, int ilength );
    
    NtwkIOLink * getNtwkIoLink();
    
    
private:
    typedef THash< H2Stream* > StreamMap;
    
    H2Stream* findStream(uint32_t uiStreamID);
    int releaseAllStream();

    int processFrame( H2FrameHeader* pHeader);
    void printLogMsg( H2FrameHeader* pHeader);

    int checkReqline( char* pName, int ilength, uint8_t& flags);
    
    int processDataFrame( H2FrameHeader* pHeader);
    int parseHeaders(char* pHeader, int ilength, int &NVPairCnt ); 
    H2Stream* getNewStream(uint32_t uiStreamID, uint8_t ubH2_Flags, Priority_st& priority);
    
    int processPriorityFrame( H2FrameHeader* pHeader );
    int processSettingFrame( H2FrameHeader * pHeader );
    int processHeadersFrame( H2FrameHeader* pHeader );
    int processHeaderFrame( H2FrameHeader* pHeader );
    int processPingFrame( H2FrameHeader * pHeader );
    int processGoAwayFrame( H2FrameHeader * pHeader );
    int processRstFrame( H2FrameHeader* pHeader );
    int processWindowUpdateFrame( H2FrameHeader* pHeader );
    int processPushPromiseFrame( H2FrameHeader* pHeader );
    int processContinuationFrame(H2FrameHeader* pHeader );
    
    int sendPingFrame( uint8_t flags, uint8_t *pPayload );
    int sendSettingsFrame();
    int sendGoAwayFrame(H2ErrorCode status);
    int doGoAway(H2ErrorCode status);
    int append400BadReqReply(uint32_t uiStreamID);
    void resetStream( H2Stream * pStream, H2ErrorCode code );
    void resetStream( StreamMap::iterator it, H2ErrorCode code );

    int appendCtrlFrameHeader( H2FrameType type, uint32_t len, unsigned char flags = 0, uint32_t uiStreamID = 0 )
    {
        H2FrameHeader header( len, type, flags, uiStreamID );
        getBuf()->append( (char*)&header, 9 );
        return 0;
    }
    int  sendFrame8Bytes( H2FrameType type, uint32_t uiStreamId, uint32_t uiVal1, uint32_t uiVal2 );
    int  sendFrame4Bytes( H2FrameType type, uint32_t uiStreamId, uint32_t uiVal2 );
    int  sendFrame0Bytes( H2FrameType type, uint8_t  flags );

    
    void recycleStream( StreamMap::iterator it );
    void logDeflateInflateError( int n, int iDeflate );
    int appendReqHeaders(H2Stream* arg1, int isFirstAppend );
    int extractCompressedData(unsigned char *pSrc, unsigned char *bufEnd);
    void skipRemainData();
    int compressHeaders( HttpRespHeaders *pRespHeaders, unsigned char *buf, int maxSize );
    
    int verifyClientPreface();
    int parseFrame();
    
private:
    LoopBuf         m_bufInput;
    AutoBuf         m_bufInflate;
    uint32_t        m_uiServerStreamID;
    uint32_t        m_uiLastPingID;
    uint32_t        m_uiLastStreamID;
    uint32_t        m_uiGoAwayId;
    int32_t         m_iCurrentFrameRemain;
    struct timeval  m_timevalPing;
    NameValuePair   m_NameValuePairListReqline[3];
    DLinkQueue      m_dqueStreamRespon[H2_STREAM_PRIORITYS];
    StreamMap       m_mapStream;
    short           m_state;
    short           m_iFlag;
    char            m_bVersion;
    
    int32_t         m_iCurDataOutWindow;
    int32_t         m_iCurInBytesToUpdate;
    int32_t         m_iDataInWindow;

    int32_t         m_iStreamInInitWindowSize;
    int32_t         m_iServerMaxStreams;
    int32_t         m_iStreamOutInitWindowSize;
    int32_t         m_iClientMaxStreams;
    int32_t         m_iPeerMaxFrameSize;
    int32_t         m_tmIdleBegin;
    int32_t         m_H2HeaderMem[10];
    H2FrameHeader*  m_pCurH2Header;

private:
    H2Connection(const H2Connection& other);
    Hpack m_hp;
    virtual H2Connection& operator=(const H2Connection& other);

};

#endif // H2CONNECTION_H
