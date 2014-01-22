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

#include <http/datetime.h>
#include <http/httplog.h>
#include <util/ssnprintf.h>
#include <util/iovec.h>



SpdyStream::SpdyStream()
        :m_uiStreamID(0)
        ,m_iPriority(0)
        ,m_pSpdyConn(NULL)
{
}


const char * SpdyStream::buildLogId()
{
    int len ;
    AutoStr2 & id = getIdBuf();
    
    len = safe_snprintf( id.buf(), MAX_LOGID_LEN, "%s-%d",
                         m_pSpdyConn->getStream()->getLogId(), m_uiStreamID );
    id.setLen( len );
    return id.c_str();
}


int SpdyStream::init(uint32_t StreamID, 
                     int Priority, SpdyConnection* pSpdyConn, uint8_t flags, HioStreamHandler * pHandler )
{
    HioStream::reset( DateTime::s_curTime );
    pHandler->assignStream( this );
    setLogIdBuild( 0 );

    setState(HIOS_CONNECTED);
    setFlag( (flags & ( SPDY_CTRL_FLAG_FIN | SPDY_CTRL_FLAG_UNIDIRECTIONAL)), 1 );

    m_bufIn.clear();
    m_uiStreamID  = StreamID;
    m_iWindowOut = pSpdyConn->getStreamOutInitWindowSize();
    m_iWindowIn = pSpdyConn->getStreamInInitWindowSize();
    m_iPriority = Priority;
    m_pSpdyConn = pSpdyConn;
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
    if ( ReadCount > 0 )
        setActiveTime( DateTime::s_curTime );

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
    m_pSpdyConn->continueWrite();
    
}
void SpdyStream::onTimer()
{
    getHandler()->onTimerEx();
}

int SpdyStream::sendFin()
{
    if ( getState() == HIOS_SHUTDOWN )
        return 0;
    
    setState(HIOS_SHUTDOWN);

    if ( D_ENABLED( DL_LESS ))
    {
        LOG_D(( getLogger(), "[%s] SpdyStream::sendFin()",
                getLogId() ));
    }
    m_pSpdyConn->sendFinFrame( m_uiStreamID );
    m_pSpdyConn->flush();
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
    m_pSpdyConn->continueWrite();
    //if (getHandler())
    //{
    //    getHandler()->recycle();
    //    setHandler( NULL );
    //}
    //m_pSpdyConn->recycleStream( m_uiStreamID );
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

int SpdyStream::getDataFrameSize( int wanted )
{
    if (( m_pSpdyConn->isOutBufFull() )||
        ( 0 >= m_iWindowOut ))
    {
        setFlag( HIO_FLAG_BUFF_FULL|HIO_FLAG_WANT_WRITE, 1 );
        m_pSpdyConn->continueWrite();
        return 0;
    }

    if ( wanted > m_iWindowOut )
        wanted = m_iWindowOut;
    if ( m_pSpdyConn->isFlowCtrl() && ( wanted > m_pSpdyConn->getCurDataOutWindow() ))
        wanted = m_pSpdyConn->getCurDataOutWindow();
    if ( wanted > SPDY_MAX_DATAFRAM_SIZE ) 
        wanted = SPDY_MAX_DATAFRAM_SIZE;
    return wanted;
}

int SpdyStream::writev( IOVec &vector, int total )
{
    int size;
    int ret; 
    if ( getState()==HIOS_DISCONNECTED )
        return -1;
    if (getFlag( HIO_FLAG_BUFF_FULL) )
        return 0;
    size = getDataFrameSize( total );
    if ( size <= 0 )
        return 0;
    if ( size < total )
    {
        //adjust vector
        IOVec iov( vector );
        total = iov.shrinkTo( size, 0 );
        ret = sendData( &iov, size );
    }
    else
        ret = sendData( &vector, size );
    if ( ret == -1 )
        return -1;
    return size;
    
}

int SpdyStream::write( const char * buf, int len )
{
    IOVec iov;
    int allowed;
    if ( getState() == HIOS_DISCONNECTED )
        return -1;
    allowed = getDataFrameSize( len );
    if ( allowed <= 0 )
        return 0;

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
    if ( m_pSpdyConn->isOutBufFull() )
        return 0;
    if ( m_iWindowOut <= 0 )
        return 0;
    setFlag( HIO_FLAG_BUFF_FULL, 0 );

    if ( isWantWrite() )
        getHandler()->onWriteEx();
    if ( isWantWrite() )
        m_pSpdyConn->continueWrite();
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
    ret = m_pSpdyConn->cacheWritev( *pIov );
    pIov->pop_front( 1 );
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

    setActiveTime( DateTime::s_curTime );
    bytesSent( total );
    m_pSpdyConn->dataFrameSent( total );
    if ( isFlowCtrl() )
    {
        m_iWindowOut -= total;
        if ( m_iWindowOut <= 0 )
            setFlag( HIO_FLAG_BUFF_FULL, 1 );
    }
    return total;
}


int SpdyStream::sendHeaders( IOVec &vector, int headerCount )
{
    if ( getState() == HIOS_DISCONNECTED )
        return -1;
    m_pSpdyConn->move2ReponQue(this);
    return m_pSpdyConn->sendRespHeaders(vector, headerCount, m_uiStreamID);
}

int SpdyStream::adjWindowOut( int32_t n  )
{
    if ( isFlowCtrl() )
    {
        m_iWindowOut += n;      
        if ( D_ENABLED( DL_LESS ) )
        {
             LOG_D(( getLogger(), "[%s] stream WINDOW_UPDATE: %d, window size: %d ",
                            getLogId(), n, m_iWindowOut ) );          
        }
        if ( m_iWindowOut < 0 )
        {
            //window overflow
            return -1;
        }
        else if ( isWantWrite() )
            continueWrite();
    }
    return 0;
}




