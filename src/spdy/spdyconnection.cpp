/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
#include "spdyconnection.h"
#include "spdystream.h"
#include "spdystreampool.h"
#include <http/httplog.h>
#include <http/httpconnpool.h>
#include <http/httpconnection.h>

#include "edio/inputstream.h"
#include <util/iovec.h>

static hash_key_t int_hash( const void * p )
{
    return ( int )(long)p;
}

static int  int_comp( const void * pVal1, const void * pVal2 )
{
    return ( int )(long)pVal1 - ( int )(long)pVal2;
}

static inline void appendNbo4Bytes( LoopBuf * pBuf, uint32_t val )
{
    pBuf->append( val >> 24 );
    pBuf->append( ( val >> 16 ) & 0xff );
    pBuf->append( ( val >> 8 ) & 0xff );
    pBuf->append( val & 0xff );
}

static inline void append4Bytes( LoopBuf * pBuf, const char * val )
{
    pBuf->append( *val++ );
    pBuf->append( *val++ );
    pBuf->append( *val++ );
    pBuf->append( *val );
}

SpdyConnection::SpdyConnection()
        : m_bufInput( 4096 )
        , m_deflator()
        , m_inflator()
        , m_uiServerStreamID( 2 )     
        , m_uiLastPingID( 0 )
        , m_uiLastStreamID( 0 )
        , m_uiGoAwayId( 0 )
        , m_mapStream( 50, int_hash, int_comp )        
        , m_flag( 0 )
        , m_iCurDataOutWindow( SPDY_FCW_INIT_SIZE )
        , m_iCurInBytesToUpdate( 0 )
        , m_iDataInWindow( SPDY_FCW_INIT_SIZE )
        , m_iStreamInInitWindowSize( 5*1024*1024 )
        , m_iServerMaxStreams( 500 )
        , m_iStreamOutInitWindowSize( SPDY_FCW_INIT_SIZE )
        , m_iClientMaxStreams( 100 )
        , m_tmIdleBegin( 0 )
{
}

int SpdyConnection::init( HiosProtocol ver )
{
    m_iCurDataOutWindow = SPDY_FCW_INIT_SIZE;
    m_deflator.init( 0, ver );
    m_inflator.init( 1, ver );
    m_uiLastStreamID = 0;
    m_bVersion = ver + 1;
    if ( ver == HIOS_PROTO_SPDY3 )
        m_iStreamOutInitWindowSize = SPDY_FCW_INIT_SIZE;
    else
        m_iStreamOutInitWindowSize = 1024 * 1024 * 1024;   //For SPDY2, there is no flow control, set it to a large value
    m_iServerMaxStreams = 500;
    m_iClientMaxStreams = 100;
    
    m_tmIdleBegin = 0;
    m_iCurrentFrameRemain = -8;
    m_pcurrentSpdyHeader = (SpdyFrameHeader*)m_SpdyHeaderMem;
    return 0;
}


int SpdyConnection::onInitConnected()
{
    m_state = HIOS_CONNECTED;
    setOS( getStream() );
    getStream()->continueRead();
    //sendSettings(m_iServerMaxStreams, m_iStreamInInitWindowSize);
    if ( isFlowCtrl() )
    {
        //sendWindowUpdateFrame( 0, SPDY_FCW_INIT_SIZE );
        //m_iDataInWindow += SPDY_FCW_INIT_SIZE;
    }
    return 0;
}

SpdyConnection::~SpdyConnection()
{
}

SpdyConnection& SpdyConnection::operator=( const SpdyConnection & other )
{
    return *this;
}


int SpdyConnection::onReadEx()
{
    int ret = onReadEx2();
    if ( !isEmpty() )
        flush();
    return ret;
}

int SpdyConnection::onReadEx2()
{
    int n = 0, avaiLen = 0;
    while( true )
    {
        if ( m_bufInput.available() < 500 )
            m_bufInput.guarantee( 1024 );
        avaiLen = m_bufInput.contiguous();
        n = getStream()->read( m_bufInput.end(), avaiLen );
        //if ( D_ENABLED( DL_LESS ) )
        //{
        //    LOG_D(( getLogger(), "[%s] getStream()->read(), availLen = %d, ret = %d",
        //                    getLogId(), avaiLen, n ));          
        //}
        
        if ( n == -1 )
        {
            // must start to close connection
            // must turn off READING event in the meantime
            getStream()->suspendRead();
            onCloseEx();
            return -1;
        }
        else if ( n == 0 )
            break;
        m_bufInput.used( n );
        while( !m_bufInput.empty() )
        {
            if ( m_iCurrentFrameRemain < 0 )
            {
                if ( m_bufInput.size() < 8 )
                    break;
                m_bufInput.moveTo((char*)m_pcurrentSpdyHeader, 8);
                m_iCurrentFrameRemain = m_pcurrentSpdyHeader->getLength();
                if ( D_ENABLED( DL_LESS ) )
                {
                    if ( !m_pcurrentSpdyHeader->isControlFrame() )
                        LOG_D(( getLogger(), "[%s] DATA frame, size: %d", getLogId(), 
                                m_iCurrentFrameRemain ));
                }        
            }
            if ( m_pcurrentSpdyHeader->isControlFrame() )
            {
                if ( m_iCurrentFrameRemain > m_bufInput.size() )
                    break;
                if ( processControlFrame(m_pcurrentSpdyHeader) == -1 )
                {
                    doGoAway( SPDY_GOAWAY_PROTOCOL_ERROR );
                    return -1;

                }
                if ( m_iCurrentFrameRemain > 0 )
                    m_bufInput.pop_front( m_iCurrentFrameRemain );
                m_iCurrentFrameRemain = -8;
            }
            else
            {
                processDataFrame( m_pcurrentSpdyHeader );
                if ( m_iCurrentFrameRemain == 0 )
                    m_iCurrentFrameRemain = -8;
            }
        }

        if ( isFlowCtrl()&&
            ( m_iDataInWindow / 2 < m_iCurInBytesToUpdate ))
        {
            if ( D_ENABLED( DL_LESS ) )
            {
                LOG_D(( getLogger(), "[%s] bytes received for WINDOW_UPDATE: %d", 
                        getLogId(), m_iDataInWindow ));
            }        
            sendWindowUpdateFrame( 0, m_iCurInBytesToUpdate );
            m_iCurInBytesToUpdate = 0;
        }

    }
    return 0;
}

int SpdyConnection::processControlFrame( SpdyFrameHeader* pHeader)
{
    static int extraHeaderLen[2][11] = 
    {  
        {  0, 10, 6, 8, 4, 0, 4, 4, 6, 8, 8 },
        {  0, 10, 4, 8, 4, 0, 4, 8, 4, 8, 8 }
    } ;    
    int extraHeader = 8;
    if ( m_bVersion != pHeader->getVersion() )
    {
        //Spdy version does not match, 
        LOG_INFO(( "[%s] Protocol error, session is SPDY%d, frame "
                "header version is SPDY%d, go away!",
                   getLogId(), m_bVersion, pHeader->getVersion() ));
        return -1;
    }
    if(pHeader->getType() <= 10)
        extraHeader = extraHeaderLen[m_bVersion -2][pHeader->getType()];
    if(extraHeader > m_iCurrentFrameRemain)
        extraHeader = m_iCurrentFrameRemain;
    memset((char*)pHeader + 8, 0, 10);
    m_bufInput.moveTo((char*)pHeader + 8, extraHeader);
    m_iCurrentFrameRemain -= extraHeader;
    printLogMsg(pHeader);
    switch ( pHeader->getType() )
    {
    case SPDY_FRAME_SYN_STREAM:
        return processSynStreamFrame( pHeader );
    case SPDY_FRAME_HEADERS:
        return extractCompressedData();
    case SPDY_FRAME_SYN_REPLY:
        return extractCompressedData();
    case SPDY_FRAME_RST_STREAM:
        processRstFrame( pHeader );
        break;
    case SPDY_FRAME_GOAWAY:
        processGoAwayFrame( pHeader );
        break;
    case SPDY_FRAME_SETTINGS:
        return processSettingFrame( pHeader );
    case SPDY_FRAME_PING:
        processPingFrame( pHeader );
        break;
    case SPDY_FRAME_WINDOW_UPDATE:
        processWindowUpdateFrame( pHeader );
        break;
    case SPDY_FRAME_CREDENTIAL:
        break;
    default:
        LOG_INFO(( "[%s] SPDY%d protocol error, unknown frame type: %d, go away!",
                   getLogId(), m_bVersion, pHeader->getVersion() ));
        //break protocol, bad client
        return -1;
    }
    return 0;
}

void SpdyConnection::printLogMsg( SpdyFrameHeader* pHeader )
{
    if ( D_ENABLED( DL_LESS ) )
    {
            LOG_D(( getLogger(), "[%s] Received %s, size: %d, D0:%d, D1:%d\n",
                    getLogId(),getFrameName(pHeader->getType()), pHeader->getLength(), 
                    pHeader->getHboData( 0 ),  pHeader->getHboData( 1 )) );  
    }
}

int SpdyConnection::processSettingFrame( SpdyFrameHeader * pHeader )
{//process each setting ID/value pair
    static const char* cpEntryNames[] = 
        {   "",
            "SETTINGS_UPLOAD_BANDWIDTH",
            "SETTINGS_DOWNLOAD_BANDWIDTH",
            "SETTINGS_ROUND_TRIP_TIME",
            "SETTINGS_MAX_CONCURRENT_STREAMS", 
            "SETTINGS_CURRENT_CWND",
            "SETTINGS_DOWNLOAD_RETRANS_RATE",
            "SETTINGS_INITIAL_WINDOW_SIZE" };              
    uint32_t iEntryID, iEntryValue;
    SpdySettingPairs settingPairs;
    unsigned char ucEntryFlags;    
    int iEntries = pHeader->getHboData( 0 );
    if ( m_iCurrentFrameRemain != 8 * iEntries )
    {
        LOG_INFO(( getLogger(), "[%s] bad SETTING frame, frame size does not match, ignore.",
                        getLogId() ));          
        return 0;
    }
    

    for( int i = 0; i < iEntries; i++)
    {
        m_bufInput.moveTo( (char*)&settingPairs, 8);
        if ( m_bVersion == 2 )
            settingPairs.swapID();
        ucEntryFlags = settingPairs.getFlags();
        iEntryID = settingPairs.getID();
        iEntryValue = settingPairs.getValue();
        if ( D_ENABLED( DL_LESS ) )
        {
            LOG_D(( getLogger(), "[%s] %s(%d) value: %d, Flags=%d", getLogId(), 
                    (iEntryID < 8)?cpEntryNames[iEntryID]:"INVALID", iEntryID, iEntryValue, ucEntryFlags));
        }        
        switch( iEntryID )
        {
        case SPDY_SETTINGS_INITIAL_WINDOW_SIZE:
            m_iStreamOutInitWindowSize = iEntryValue ;
            break;
        case SPDY_SETTINGS_MAX_CONCURRENT_STREAMS:
            m_iClientMaxStreams = iEntryValue ;
            break;
        default:
            break;
        }        
    }
    m_iCurrentFrameRemain = 0;
    
    sendSettings(m_iServerMaxStreams, m_iStreamInInitWindowSize);
    return 0;
}

int SpdyConnection::processWindowUpdateFrame( SpdyFrameHeader * pHeader )
{
    int32_t id = pHeader->getHboData(0);
    int32_t delta = pHeader->getHboData(1);
    StreamMap::iterator it;
    if (( id == 0 )&&( isFlowCtrl() ))
    {
        m_iCurDataOutWindow += delta;
        if ( D_ENABLED( DL_LESS ) )
        {
             LOG_D(( getLogger(), "[%s] session WINDOW_UPDATE: %d, window size: %d ",
                            getLogId(), delta, m_iCurDataOutWindow ) );          
        }
        return 0;
    }
    it = m_mapStream.find(( void* )(long)id );
    if ( it != m_mapStream.end() ) 
    {
        SpdyStream * pStream = it.second();
        if ( pStream->adjWindowOut( delta ) == -1 )
            resetStream( it, SPDY_RST_STREAM_FLOW_CONTROL_ERROR );
    }
    return 0;
}


int SpdyConnection::processRstFrame( SpdyFrameHeader* pHeader )
{
    StreamMap::iterator it = m_mapStream.find(( void* )(long)pHeader->getHboData(0) );
    if ( it == m_mapStream.end() )
        return 0;
    SpdyStream* pSpdyStream = it.second();
    pSpdyStream->setFlag( HIO_FLAG_PEER_RESET, 1 );
    recycleStream( it );
    
    return 0;
}

void SpdyConnection::skipRemainData()
{
    int len = m_bufInput.size();
    if ( len > m_iCurrentFrameRemain )
        len = m_iCurrentFrameRemain;
    m_bufInput.pop_front( len );
    m_iCurrentFrameRemain -= len;
    
    if ( isFlowCtrl() )
        m_iCurInBytesToUpdate += len;
}

int SpdyConnection::processDataFrame( SpdyFrameHeader* pHeader )
{
    uint32_t streamID = pHeader->getDataStreamId();
    SpdyStream* pSpdyStream = findStream( streamID );
    if ( pSpdyStream == NULL )
    {
        skipRemainData(); 
        sendRstFrame( streamID, SPDY_RST_STREAM_INVALID_STREAM );
        return 0;
    }
    if ( pSpdyStream->isPeerShutdown() )
    {
        skipRemainData();
        sendRstFrame( streamID, SPDY_RST_STREAM_ALREADY_CLOSED );
        return 0;
    }
    while( m_iCurrentFrameRemain > 0 )
    {
        int len = m_bufInput.blockSize();
        if ( len == 0 )
            break;
        if ( len > m_iCurrentFrameRemain )
        {
            len = m_iCurrentFrameRemain;
        }
        m_iCurrentFrameRemain -= len;
        pSpdyStream->appendReqData( m_bufInput.begin(), len,
                              m_iCurrentFrameRemain ? 0 : pHeader->getFlags() );
        m_bufInput.pop_front( len );
        if ( isFlowCtrl() )
            m_iCurInBytesToUpdate += len;
    }
        
    if ( isSpdy3() && !pSpdyStream->isPeerShutdown() )
    {
        if ( D_ENABLED( DL_MORE ) )
        {
            LOG_D(( getLogger(), "[%s] processDataFrame() ID: %d, input window size: %d ",
                    getLogId(), streamID, pSpdyStream->getWindowIn() ) );
        }
        
        if ( pSpdyStream->getWindowIn() < m_iStreamInInitWindowSize / 2 )
        {
            sendWindowUpdateFrame( streamID, m_iStreamInInitWindowSize / 2 );
            pSpdyStream->adjWindowIn( m_iStreamInInitWindowSize / 2 );
        }
    }
    return 0;
}

int SpdyConnection::processSynStreamFrame(SpdyFrameHeader* pHeader )
{
    int headerCount;
    int ret;
    int n = 0;
    int priority;
    SpdyStream* pStream;
    uint32_t id = pHeader->getHboData( 0 );
    if ( m_flag & SPDY_CONN_FLAG_GOAWAY )
    {
        return 0;
    }
    
    if ( id <= m_uiLastStreamID )
    {
        sendRstFrame( id, SPDY_RST_STREAM_PROTOCOL_ERROR );
        LOG_INFO(( "[%s] Protocol error, SYN_STREAM ID: %d is less the"
                " previously received stream ID: %d, cannot keep"
                " decompression state in sync, go away!",
                   getLogId(), id, m_uiLastStreamID ));
        
        return -1;
    }
    m_uiLastStreamID = id; 
    n = extractCompressedData();
    if( n < 0)
    {
        return -1;
    }
    if ( m_mapStream.size() >= (uint)m_iServerMaxStreams )
    {
        sendRstFrame( id, SPDY_RST_STREAM_REFUSED_STREAM );
        return 0;
    }
    ret = parseHeaders( m_bufInflate.begin(), n, headerCount );
    if ( ret > 0 )
    {
        if ( ret < SPDY_RST_STREAM_NUM_STATUS_CODES )
            sendRstFrame( id, (SpdyRstErrorCode)ret );
        return 0;
    }
    if ( ret == -2 )
    {   //method, url, and version not present together
        //Need to send HTTP 400 BAD REQUEST reply.
        append400BadReqReply( id );
        return 0;
    }

    priority = (( uint8_t* )pHeader)[16] >> (8 - m_bVersion );    
    pStream = getNewStream( id, priority, pHeader->getFlags() );  
    if ( pStream )
    {
        appendReqHeaders( pStream, headerCount );
        pStream->onInitConnected();
        if ( pStream->getState() == HIOS_DISCONNECTED )
            recycleStream( pStream->getStreamID() );
    }
    else
    {
        sendRstFrame( id, SPDY_RST_STREAM_INTERNAL_ERROR );
    }
    return 0;
}

int SpdyConnection::extractCompressedData()
{
    int n = 0, n1 = 0;;
    m_bufInflate.clear();
    int iDatalen = ( m_bufInput.blockSize() < m_iCurrentFrameRemain)? ( m_bufInput.blockSize()):(m_iCurrentFrameRemain);
    while ( iDatalen > 0 )
    {
        n = m_inflator.spdyHeaderInflate( m_bufInput.begin(), iDatalen, m_bufInflate );
        if ( n < 0)
        {
            logDeflateInflateError( n, 0);
            return -1;
        }  
        m_bufInput.pop_front(iDatalen);
        m_iCurrentFrameRemain -= iDatalen;
        iDatalen = m_iCurrentFrameRemain; 
        n1 += n;
    }
    return n1;
    
}

static int IstheKey( const char * str1, int Length1, const char * str2, int Length2 )
{
    if( ( Length1 == Length2 ) && ( strncmp( str1, str2, Length1 ) == 0 ))
        return 1;
    else
        return 0;
}

int SpdyConnection::checkReqline( char* pName, int ilength, uint8_t& flags)
{
    static const char* pReqs[2][3] = 
    {   
        { "method", "url", "version" },
        { ":method", ":path", ":version" }  
    };  
    static const int pReqslen[2][3] = 
    {   
        { 6, 3, 7 },
        { 7, 5, 8 }  
    };     
    int bitflag = 1, nv = m_bVersion -2;  
    for(int i=0; i<3; i++)
    {
        if( ((flags & bitflag) ==0) && IstheKey(( const char* )pName, ilength, pReqs[nv][i], pReqslen[nv][i] ) )
        {
            pName += (ilength + (nv+1)*2);
            m_NameValuePairListReqline[i].pValue = pName;
            if(nv == 0)
                m_NameValuePairListReqline[i].ValueLen = beReadUint16( (unsigned char*)pName - 2 );
            else
                m_NameValuePairListReqline[i].ValueLen = beReadUint32( (unsigned char*)pName - 4 );
            flags |= bitflag;
            return (m_NameValuePairListReqline[i].ValueLen + ilength + (nv+1)*2);
        }
        bitflag = bitflag << 1;
    }
    return 0;
}

int SpdyConnection::parseHeaders(  char* pHeader, int ilength, int &NVPairCnt )
{
    uint8_t flags = 0;
    unsigned char* buff = ( unsigned char* )pHeader;
    unsigned char* buffEnd = buff + ilength;
    uint32_t NameCount, theLength;
    int n = 0;

    NVPairCnt = 0;

    NameCount = (m_bVersion == 3)? (beReadUint32Adv( buff )):(beReadUint16Adv( buff ));
    if ( NameCount == 0 )
        return 0;
    
    if(NameCount > 100)
        return SPDY_RST_STREAM_FRAME_TOO_LARGE;

    for ( uint i = 0; i < NameCount; i++ )
    {
        if( (theLength = (m_bVersion == 3)? (beReadUint32Adv( buff )):(beReadUint16Adv( buff ))) == 0)
        {
             return SPDY_RST_STREAM_PROTOCOL_ERROR;            
        }
        if ( buff + theLength > buffEnd )
            return SPDY_RST_STREAM_PROTOCOL_ERROR;
        if( (flags^0x7) && ((n = checkReqline((char*)buff, theLength, flags))>0 ))
            buff += n;
        else
        {
            int needThisHeader = 1;
            if( m_bVersion == 3 )
            {//Hack here :host ==> host
                if (theLength == 5 && 
                    strncmp(( const char* )buff, ":host", 5) == 0)
                {
                    -- theLength;
                    ++ buff ;
                }
                else if ( (*(char*)buff) == ':')
                {
                    needThisHeader = 0;
                }
            }            
            m_NameValuePairList[NVPairCnt].nameLen = theLength;
            m_NameValuePairList[NVPairCnt].pName = ( char* )buff;    
            
            buff += theLength;
            theLength = (m_bVersion == 3)? (beReadUint32Adv( buff )):(beReadUint16Adv( buff ));
            if ( buff + theLength > buffEnd )
                return SPDY_RST_STREAM_PROTOCOL_ERROR;
            if (needThisHeader)
            {
                replaceZero( (char*)buff, theLength );
                m_NameValuePairList[NVPairCnt].ValueLen = theLength;
                m_NameValuePairList[NVPairCnt++].pValue = ( char* )buff;
            }
            buff += theLength;  
        }    
    }
    if ( flags^0x7 )
    {   //method, url, and version not present together
        return -2;
    }

    return 0;
}

void SpdyConnection::replaceZero( char* pValue, int ilength )
{
    char *pEnd = pValue + ilength;
    while ( (pValue) && (pValue < pEnd) )
    {
       pValue = (char*) memchr(pValue, 0, ilength);
       if(pValue != NULL)
       {
           *pValue++ = ',';
           ilength = pEnd - pValue;
       }
    }
}
int SpdyConnection::appendReqHeaders( SpdyStream* pStream, int NVPairCnt )
{
    pStream->getBufIn()->guarantee( m_bufInflate.size() );
    for( int i = 0; i < 3; i++ )
    {
        pStream->appendInputData( m_NameValuePairListReqline[i].pValue, m_NameValuePairListReqline[i].ValueLen);
        if( i == 2 )
        {
            pStream->appendInputData( '\r' );
            pStream->appendInputData( '\n' );
        }
        else
        {
            pStream->appendInputData( ' ' );
        }
    } 
    for( int i = 0; i < NVPairCnt; i++ )
    {
        pStream->appendInputData( m_NameValuePairList[i].pName, m_NameValuePairList[i].nameLen );
        pStream->appendInputData( ':' );
        pStream->appendInputData( ' ' );
        pStream->appendInputData( m_NameValuePairList[i].pValue, m_NameValuePairList[i].ValueLen );
        pStream->appendInputData( '\r' );
        pStream->appendInputData( '\n' );
    }  
    pStream->appendInputData( '\r' );
    pStream->appendInputData( '\n' );
    return 0;
    
}

SpdyStream* SpdyConnection::getNewStream( uint32_t uiStreamID, int iPriority, uint8_t ubSpdy_Flags)
{
    SpdyStream* pStream;
    NtwkIOLink * pLink;

    HttpConnection * pConn = HttpConnPool::getConnection();
    if ( !pConn )
        return NULL;
    pLink = dynamic_cast<NtwkIOLink *>(getStream() );
    pConn->setNtwkIOLink( pLink );
    pStream = new SpdyStream();
    //pStream = SpdyStreamPool::getSpdyStream();
    m_mapStream.insert(( void* )(long)uiStreamID, pStream );
    if ( D_ENABLED( DL_MORE ) )
    {
        LOG_D(( getLogger(), "[%s] getNewStream(), ID: %d, stream map size: %d ",
                getLogId(), uiStreamID, m_mapStream.size() ) );
    }
    pStream->init( uiStreamID, iPriority, this, ubSpdy_Flags, pConn );
    pStream->setProtocol( pLink->getProtocol() );
    if ( m_bVersion == 3 )
        pStream->setFlag( HIO_FLAG_FLOWCTRL, 1 );
    return pStream;
}


void SpdyConnection::recycleStream( uint32_t uiStreamID )
{
    StreamMap::iterator it = m_mapStream.find(( void* )(long) uiStreamID );
    if ( it == m_mapStream.end() )
        return;
    recycleStream( it );
}


void SpdyConnection::recycleStream( StreamMap::iterator it )
{
    SpdyStream* pSpdyStream = it.second();
    m_mapStream.erase( it );
    pSpdyStream->close();
    m_dqueStreamRespon[pSpdyStream->getPriority()].remove( pSpdyStream );
    if ( pSpdyStream->getHandler() )
        pSpdyStream->getHandler()->recycle();
    
    if ( D_ENABLED( DL_MORE ) )
    {
        LOG_D(( getLogger(), "[%s] recycleStream(), ID: %d, stream map size: %d "
                , getLogId(), pSpdyStream->getStreamID(), m_mapStream.size() ) );
    }
    //SpdyStreamPool::recycle( pSpdyStream );
    delete pSpdyStream;
}


int SpdyConnection::appendCtrlFrameHeader( SpdyFrameType type, uint8_t len )
{
    static unsigned char s_achFrameHeader[8] = 
    {   0x80, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x08  };
    s_achFrameHeader[1] = m_bVersion;
    s_achFrameHeader[3] = type;
    s_achFrameHeader[7] = len;
    
    getBuf()->append( (char*)s_achFrameHeader, 8 );
    
    return 0;
}


int SpdyConnection::sendFrame8Bytes( SpdyFrameType type, uint32_t uiVal1, uint32_t uiVal2 )
{
    getBuf()->guarantee( 16 );
    appendCtrlFrameHeader( type, 8 );
    appendNbo4Bytes( getBuf(), uiVal1 );
    appendNbo4Bytes( getBuf(), uiVal2 );
    flush();
    if ( D_ENABLED( DL_MORE ) )
    {
        LOG_D(( getLogger(), "[%s] send %s frame, stream: %d, value: %d"
                , getLogId(), getFrameName( type ), uiVal1, uiVal2 ) );
    }
    return 0;
}

int SpdyConnection::sendFrame4Bytes( SpdyFrameType type, uint32_t uiVal1 )
{
    getBuf()->guarantee( 12 );
    appendCtrlFrameHeader( type, 4 );
    appendNbo4Bytes( getBuf(), uiVal1 );
    flush();
    if ( D_ENABLED( DL_MORE ) )
    {
        LOG_D(( getLogger(), "[%s] send %s frame, value: %d"
                , getLogId(), getFrameName( type ), uiVal1 ) );
    }
    return 0;
}

int SpdyConnection::sendPing()
{   //Server initiate a Ping
    m_uiLastPingID = m_uiServerStreamID;
    m_uiServerStreamID += 2;
    gettimeofday( &m_timevalPing, NULL );
    return appendPing( m_uiServerStreamID );
}

int SpdyConnection::sendSingleSettings(uint32_t uiID, uint32_t uiValue, uint8_t flags)
{   
    static char clength[4] = { 0x00, 0x00, 0x00, 0x01 };
    getBuf()->guarantee( 12 );
    appendCtrlFrameHeader( SPDY_FRAME_SETTINGS, 12 );
    append4Bytes( getBuf(), clength );
    if(m_bVersion == 3)
        uiID = htonl( uiID ) | ((uint32_t)flags);
    else
        uiID |= ((uint32_t)flags) << 24;
    append4Bytes( getBuf(), (char *)&uiID );
    appendNbo4Bytes( getBuf(), uiValue );
    flush();
    return 0;
}

int SpdyConnection::sendSettings(uint32_t uiMaxStreamNum, uint32_t uiWindowSize)
{  
    static char cMaxStreamNumV2[8] = {      0x00, 0x00, 0x00, 0x01,
                                            0x04, 0x00, 0x00, 0x00 };  
    static char cMaxStreamNumV3[8] = {      0x00, 0x00, 0x00, 0x02,
                                            0x00, 0x00, 0x00, 0x04 };    
    static char cWindowSizeV3[4] = {        0x00, 0x00, 0x00, 0x07 };  

    getBuf()->guarantee( 28 );
    if(m_bVersion == 3)
    {
        appendCtrlFrameHeader( SPDY_FRAME_SETTINGS, 20 );
        getBuf()->append(cMaxStreamNumV3, 8 ); 
        appendNbo4Bytes( getBuf(), uiMaxStreamNum );
        append4Bytes( getBuf(), cWindowSizeV3 );
        appendNbo4Bytes( getBuf(), uiWindowSize );
        if ( D_ENABLED( DL_MORE ) )
        {
            LOG_D(( getLogger(), 
                    "[%s] send SETTING frame, MAX_CONCURRENT_STREAMS: %d,"
                    "  INITIAL_WINDOW_SIZE: %d"
                    , getLogId(), uiMaxStreamNum, uiWindowSize ) );
        }
    }
    else
    {
        appendCtrlFrameHeader( SPDY_FRAME_SETTINGS, 12 );
        getBuf()->append(cMaxStreamNumV2, 8 );    
        appendNbo4Bytes( getBuf(), uiMaxStreamNum );
        if ( D_ENABLED( DL_MORE ) )
        {
            LOG_D(( getLogger(), 
                    "[%s] send SETTING frame, MAX_CONCURRENT_STREAMS: %d,"
                     , getLogId(), uiMaxStreamNum ) );
        }
    }
    if ( isFlowCtrl() )
    {
        sendWindowUpdateFrame( 0, SPDY_FCW_INIT_SIZE );
        m_iDataInWindow += SPDY_FCW_INIT_SIZE;
    }
    else
        flush();
    return 0;
}

int SpdyConnection::processPingFrame( SpdyFrameHeader * pHeader )
{
    struct timeval CurTime;
    long msec;
    uint32_t id = pHeader->getHboData( 0 );
    if (( id & 1 ) == 1 )
    {
        return appendPing( id );
    }
    if ( id != m_uiLastPingID )
    {
        //log , mismatch ping ID.
        return 0;
    }
    
    gettimeofday( &CurTime, NULL );
    msec = ( CurTime.tv_sec - m_timevalPing.tv_sec ) * 1000;
    msec += ( CurTime.tv_usec - m_timevalPing.tv_usec ) / 1000;
    if ( D_ENABLED( DL_MORE ) )
    {
        LOG_D(( getLogger(), "[%s] Received PING, ID=%d, Round trip "
                "times=%d milli-seconds", getLogId(), m_uiLastPingID, msec ) );
    }
    return 0;
}

int SpdyConnection::append400BadReqReply( uint32_t uiStreamID )
{
    return 0;
}

SpdyStream* SpdyConnection::findStream( uint32_t uiStreamID )
{
    StreamMap::iterator it = m_mapStream.find(( void* )(long)uiStreamID );
    if ( it == m_mapStream.end() )
        return NULL;
    return it.second();
}

int SpdyConnection::flush()
{
    BufferedOS::flush();
    if ( !isEmpty() )
        getStream()->continueWrite();
    getStream()->flush();
    return 0;
}

int SpdyConnection::onCloseEx()
{
    if ( getStream()->isReadyToRelease() )
        return 0;
    if ( D_ENABLED( DL_LESS ) )
    {
        LOG_D(( getLogger(), "[%s] SpdyConnection::onCloseEx() ", getLogId() ) );
    }
    getStream()->setState( HIOS_CLOSING );
    releaseAllStream();
    return 0;
};

int SpdyConnection::onTimerEx()
{
    int result = 0;
    if ( m_flag & SPDY_CONN_FLAG_GOAWAY )
        result = releaseAllStream();
    else
        result = timerRoutine();
    return result;
}


int SpdyConnection::processGoAwayFrame( SpdyFrameHeader * pHeader )
{
    m_flag |= ( short )SPDY_CONN_FLAG_GOAWAY;

    onCloseEx();
    return true;
}

int SpdyConnection::doGoAway(SpdyGoAwayStatus status)
{
    if ( D_ENABLED( DL_LESS ) )
    {
        LOG_D(( getLogger(), "[%s] SpdyConnection::doGoAway(), status = %d ", getLogId(), status ) );
    }
    sendGoAwayFrame( status );
    releaseAllStream();
    getStream()->setState( HIOS_CLOSING );
    getStream()->continueWrite();
    return 0;
}


int SpdyConnection::sendGoAwayFrame(SpdyGoAwayStatus status )
{
    if ( m_bVersion == 3 )
        sendFrame8Bytes( SPDY_FRAME_GOAWAY, m_uiLastStreamID, status );
    else    
        sendFrame4Bytes( SPDY_FRAME_GOAWAY, m_uiLastStreamID );
    return 0;
}


int SpdyConnection::releaseAllStream()
{
    StreamMap::iterator itn, it = m_mapStream.begin();
    for ( ; it != m_mapStream.end(); )
    {
        itn = m_mapStream.next( it );
        recycleStream( it );
        it = itn;
    }
    getStream()->handlerReadyToRelease();
    return 0;
}

int SpdyConnection::timerRoutine()
{
    StreamMap::iterator itn, it = m_mapStream.begin();
    for ( ; it != m_mapStream.end(); )
    {
        itn = m_mapStream.next( it );
        it.second()->onTimer();
        if ( it.second()->getState() == HIOS_DISCONNECTED )
        {
            recycleStream( it );
        }
        it = itn;
    }
    if ( m_mapStream.size() == 0 )
    {
        if ( m_tmIdleBegin == 0 )
             m_tmIdleBegin = time( NULL );
        else if ( time( NULL ) - m_tmIdleBegin > 60 )
            doGoAway( SPDY_GOAWAY_OK );
    }
    else    
        m_tmIdleBegin = 0;
    return 0;
}

void SpdyConnection::logDeflateInflateError( int n, int iDeflate )
{
    static const char* cErroMsg[2] = { "Inflate Error, Error code =", "Deflate Error, Error code =" };
    LOG_INFO(( getLogger(), "[%s] Protocol Error, %s %d, go away!"
                , getLogId(), cErroMsg[iDeflate], n ) );
}

int SpdyConnection::sendRespHeaders( IOVec &vector, int iheaderCount, uint32_t uiStreamID )
{
    //Defalte the data and then add to m_pOutputBuff
    int n = 0, total = 0;
    uint32_t temp32;
    char* pData = (char*)&temp32;
    IOVec::iterator it;
    int headerOffset = getBuf()->size();
    
    getBuf()->guarantee( 28 );
    appendCtrlFrameHeader( SPDY_FRAME_SYN_REPLY, 0 );
    appendNbo4Bytes( getBuf(), uiStreamID );

    total = 4;
    if ( m_bVersion == 2 )
    { 
        *((uint16_t*)pData) = 0;
        getBuf()->append(pData, 2);
        total = 6;
        *((uint16_t*)pData) = htons(( uint16_t )iheaderCount );
    }
    else
        *((uint32_t*)pData) = htonl(( uint32_t )iheaderCount );
    
    if ( D_ENABLED( DL_MORE ) )
    {
        LOG_D(( getLogger(), "[%s] sendRespHeaders(), ID: %d, headerCount: %d"
                , getLogId(), uiStreamID, iheaderCount ) );
    }
    n = m_deflator.spdyHeaderDeflate(pData, 8-total, getBuf() ,0 );
    if( n < 0 )
    {
        logDeflateInflateError( n, 1);
        return -1;
    }
    total += n;    

    it = vector.begin();
    for ( ; it != vector.end(); it++ )
    {
        if( ( it->iov_len == 0)&&(it->iov_base == NULL) )
            continue;
        n = m_deflator.spdyHeaderDeflate(( char* )( it->iov_base ), 
                    it->iov_len, getBuf(),  ( it + 1 == vector.end() )? Z_SYNC_FLUSH : 0);        
        //assert( n >= 0 );
        if( n < 0 )
        {
            logDeflateInflateError( n, 1);
            return -1;
        }        
        total += n;
    }
    temp32 = htonl( total );
    getBuf()->update(( headerOffset + 4 ), (char*)&temp32, 4 );        //Length
    if ( D_ENABLED( DL_MORE ) )
    {
        LOG_D(( getLogger(), "[%s] sent SYN_REPLY for "
                "StreamID=%lu, total=%d\n", getLogId(), uiStreamID, total ) );
    }
    return total;
}
void SpdyConnection::move2ReponQue( SpdyStream* pSpdyStream )
{
    m_dqueStreamRespon[pSpdyStream->getPriority()].append( pSpdyStream );
}
int SpdyConnection::onWriteEx()
{
    SpdyStream* pSpdyStream = NULL;
    int wantWrite = 0;

    if ( D_ENABLED( DL_MORE ) )
    {
        LOG_D(( getLogger(), "[%s] onWriteEx() state: %d, output buffer size=%d\n ",
                getLogId(), m_state, getBuf()->size() ) );
    }
    flush();
    if ( !isEmpty() )
         return 0;
    if ( getStream()->canWrite() & HIO_FLAG_BUFF_FULL )
        return 0;
    
    
    for ( int i = 0; i < SPDY_STREAM_PRIORITYS; ++i )
    {
        if ( m_dqueStreamRespon[i].empty() )
            continue;
        DLinkedObj* it = m_dqueStreamRespon[i].begin();//SpdyStream*
        DLinkedObj* itn;
        for ( ; it != m_dqueStreamRespon[i].end(); )
        {
            pSpdyStream = ( SpdyStream* )it;
            itn = it->next();
            if ( pSpdyStream->isWantWrite() )
            {
                pSpdyStream->onWrite();
                if ( pSpdyStream->isWantWrite() && ( pSpdyStream->getWindowOut() > 0 ) )
                    ++wantWrite;
            }
            if ( pSpdyStream->getState() == HIOS_DISCONNECTED )
            {
                recycleStream( pSpdyStream->getStreamID() );
            }
            it = itn;
        }
        if ( getStream()->canWrite() & HIO_FLAG_BUFF_FULL )
            return 0;
    }
    if ( wantWrite == 0 )
        getStream()->suspendWrite();
    return 0;
}


void SpdyConnection::recycle()
{
    if ( D_ENABLED( DL_MORE ) )
    {
        LOG_D(( getLogger(), "[%s] SpdyConnection::recycle()",
                getLogId() ));
    }
    if ( !m_mapStream.empty() ) 
        releaseAllStream();
    delete this;
}

void SpdyConnection::resetStream( SpdyStream * pStream, SpdyRstErrorCode code )
{
    sendRstFrame( pStream->getStreamID(), code );
    recycleStream( pStream->getStreamID() );

}

void SpdyConnection::resetStream( StreamMap::iterator it, SpdyRstErrorCode code )
{
    sendRstFrame( it.second()->getStreamID(), code ); 
    recycleStream( it );
}

