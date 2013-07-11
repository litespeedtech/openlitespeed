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

#include "spdystream.h"

#include "spdyconnection.h"

#include <util/ssnprintf.h>
#include <http/httplog.h>
#include <util/iovec.h>



SpdyStream::SpdyStream()
        :m_uiStreamID(0)
        ,m_iPriority(0)
        ,m_pSpdyCnn(NULL)
{
}


const char * SpdyStream::buildLogId()
{
    int len ;
    AutoStr2 & id = getIdBuf();
    
    len = safe_snprintf( id.buf(), MAX_LOGID_LEN, "%s-%d",
                         m_pSpdyCnn->getStream()->getLogId(), m_uiStreamID );
    id.setLen( len );
    return id.c_str();
}


int SpdyStream::init(uint32_t StreamID, 
                     int Priority, SpdyConnection* pSpdyCnn, uint8_t flags, HioStreamHandler * pHandler )
{
    HioStream::reset();
    pHandler->assignStream( this );
    setLogIdBuild( 0 );

    setState(HIOS_CONNECTED);
    setFlag( (flags & ( SPDY_CTRL_FLAG_FIN | SPDY_CTRL_FLAG_UNIDIRECTIONAL)), 1 );

    m_bufIn.clear();
    m_uiStreamID  = StreamID;
    m_iBytesAllowSend = SPDY_MAX_DATAFRAM_SIZE;
    m_iWindowOut = pSpdyCnn->getClientInitWindowSize();
    m_iWindowIn = pSpdyCnn->getServerInitWindowSize();
    m_iPriority = Priority;
    m_pSpdyCnn = pSpdyCnn;
    if ( D_ENABLED( DL_LESS ))
    {
        LOG_D(( getLogger(), "[%s] SpdyStream::init(), id: %d. ",
                getLogId(), StreamID ));
    }
    return 0;
}
int SpdyStream::onInitConnected()
{
    getHandler()->onInitConnected();
    if ( isWantRead() )
        getHandler()->onReadEx();
    if ( isWantWrite() )
        getHandler()->onWriteEx();
    return 0;
}

SpdyStream::~SpdyStream()
{
    m_bufIn.clear();
}

int SpdyStream::appendReqData(char* pData, int len, uint8_t flags)
{
    if (m_bufIn.append(pData, len)==-1)
        return -1;
    if ( isFlowCtrl() )
        m_iWindowIn -= len;
    //Note: SPDY_CTRL_FLAG_FIN is directly mapped to HIO_FLAG_PEER_SHUTDOWN
    //      SPDY_CTRL_FLAG_UNIDIRECTIONAL is directly mapped to HIO_FLAG_LOCAL_SHUTDOWN
    if ( flags & ( SPDY_CTRL_FLAG_FIN | SPDY_CTRL_FLAG_UNIDIRECTIONAL) )
        setFlag( flags & ( SPDY_CTRL_FLAG_FIN | SPDY_CTRL_FLAG_UNIDIRECTIONAL), 1 );

    if (isWantRead())
        getHandler()->onReadEx();
    return len;
}
//***int SpdyStream::read( char * buf, int len )***//
// return > 0:  number of bytes of that has been read
// return = 0:  0 byte of data has been read, but there will be more data coming,
//              need to read again
// return = -1: EOF (End of File) There is no more data need to be read, the
//              stream can be removed
int SpdyStream::read( char * buf, int len )
{
    int ReadCount;
    if ( getState()==HIOS_DISCONNECTED )
        return -1;
    
    ReadCount = m_bufIn.moveTo( buf, len );
    if ( ReadCount == 0)
    {
        if ( getFlag( HIO_FLAG_PEER_SHUTDOWN ) ) 
        {
            return -1; //EOF (End of File) There is no more data need to be read
        }
    }
    return ReadCount;
}

void SpdyStream::continueRead()
{
    if ( D_ENABLED( DL_LESS ))
    {
        LOG_D(( getLogger(), "[%s] SpdyStream::continueRead()",
                getLogId() ));
    }
    setFlag(HIO_FLAG_WANT_READ, 1);
    if ( m_bufIn.size() > 0)
        getHandler()->onReadEx();
}
void SpdyStream:: continueWrite()
{
    if ( D_ENABLED( DL_LESS ))
    {
        LOG_D(( getLogger(), "[%s] SpdyStream::continueWrite()",
                getLogId() ));
    }
    setFlag(HIO_FLAG_WANT_WRITE, 1);
    m_pSpdyCnn->continueWrite();
    
}
void SpdyStream::onTimer()
{
    getHandler()->onTimerEx();
}

int SpdyStream::sendFin()
{
    char achHeader[8];
    
    if ( getState() == HIOS_SHUTDOWN )
        return 0;
    
    setState(HIOS_SHUTDOWN);

    if ( D_ENABLED( DL_LESS ))
    {
        LOG_D(( getLogger(), "[%s] SpdyStream::sendFin()",
                getLogId() ));
    }
    buildDataFrameHeader( achHeader, 0 );
    m_pSpdyCnn->cacheWrite( achHeader, sizeof( achHeader ) );
    m_pSpdyCnn->flush();
    return 0;
}

int SpdyStream::close()
{
    if (getState()!=HIOS_CONNECTED)
        return 0;
    if (getHandler())
        getHandler()->onCloseEx();
    sendFin();
    setFlag( HIO_FLAG_WANT_WRITE, 1 );
    setState(HIOS_DISCONNECTED);
    m_pSpdyCnn->continueWrite();
    //if (getHandler())
    //{
    //    getHandler()->recycle();
    //    setHandler( NULL );
    //}
    //m_pSpdyCnn->recycleStream( m_uiStreamID );
    return 0;
}


int SpdyStream::flush()
{
    if ( D_ENABLED( DL_LESS ))
    {
        LOG_D(( getLogger(), "[%s] SpdyStream::flush()",
                getLogId() ));
    }
    return 0;
}

int SpdyStream::writev( IOVec &vector, int total )
{
    int totalSended = 0;
    IOVec::iterator it;
    if ( getState()==HIOS_DISCONNECTED )
        return -1;
    if (getFlag( HIO_FLAG_BUFF_FULL) )
        return 0;
    
    it = vector.begin();
    for (; it != vector.end(); it++)
    {
        totalSended  += write( (char*)(it->iov_base), it->iov_len) ;
        if (getFlag( HIO_FLAG_BUFF_FULL) )
            return totalSended;
    }
    return totalSended;
}

int SpdyStream::write( const char * buf, int len )
{
    IOVec iov;
    int allowed;
    if ( getState() == HIOS_DISCONNECTED )
        return -1;
    if (( m_pSpdyCnn->isOutBufFull() )||
        ( 0 >= m_iBytesAllowSend ))
    {
        setFlag( HIO_FLAG_BUFF_FULL|HIO_FLAG_WANT_WRITE, 1 );
        m_pSpdyCnn->continueWrite();
        return 0;
    }
    
    allowed = m_iBytesAllowSend;
    if ( len < allowed )
        allowed = len;

    iov.append( buf, allowed );
    if ( sendData( &iov, allowed ) == -1 )
    {
        return -1;
    }
    return allowed;
}


int SpdyStream::onWrite()
{
    if ( D_ENABLED( DL_LESS ))
    {
        LOG_D(( getLogger(), "[%s] SpdyStream::onWrite()",
                getLogId() ));
    }
    if ( m_pSpdyCnn->isOutBufFull() )
        return 0;
    m_iBytesAllowSend = SPDY_MAX_DATAFRAM_SIZE;
    if ( m_iBytesAllowSend > m_iWindowOut )
        m_iBytesAllowSend = m_iWindowOut;
    if ( m_iBytesAllowSend <= 0 )
        return 0;
    setFlag( HIO_FLAG_BUFF_FULL, 0 );

    if ( isWantWrite() )
        getHandler()->onWriteEx();
    if ( isWantWrite() )
        m_pSpdyCnn->continueWrite();
    return 0;
}

void SpdyStream::buildDataFrameHeader( char * pHeader, int length )
{
    *(uint32_t *)pHeader = htonl( m_uiStreamID );
    *((uint32_t *)pHeader + 1 ) = htonl( length );
    if ( getState() >= HIOS_CLOSING )
        pHeader[4] = 1;

}

int SpdyStream::sendData( IOVec * pIov, int total )
{
    char achHeader[8];
    int ret;
    buildDataFrameHeader( achHeader, total );
    pIov->push_front( achHeader, 8 );
    ret = m_pSpdyCnn->cacheWritev( *pIov );
    if ( D_ENABLED( DL_LESS ))
    {
        LOG_D(( getLogger(), "[%s] SpdyStream::sendData(), total: %d, ret: %d",
                getLogId(), total, ret ));
    }
    if ( ret == -1 )
    {
        setFlag( HIO_FLAG_ABORT, 1 );
        return -1;
    }
    bytesSent( total );
    m_iBytesAllowSend -= total;
    if ( isFlowCtrl() )
        m_iWindowOut -= total;
    if ( m_iBytesAllowSend <= 0 )
        setFlag( HIO_FLAG_BUFF_FULL, 1 );
    return total;
}


int SpdyStream::sendHeaders( IOVec &vector, int headerCount )
{
    if ( getState() == HIOS_DISCONNECTED )
        return -1;
    m_pSpdyCnn->move2ReponQue(this);
    return m_pSpdyCnn->sendRespHeaders(vector, headerCount, m_uiStreamID);
}




