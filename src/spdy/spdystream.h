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
#ifndef SPDYSTREAM_H
#define SPDYSTREAM_H

#include <http/hiostream.h>

#include <util/linkedobj.h>
#include <util/loopbuf.h>

#include <inttypes.h>


class SpdyConnection;

class SpdyStream: public DLinkedObj, public HioStream
{

public:
    SpdyStream();
    ~SpdyStream();
    
    int init(uint32_t StreamID,
             int Priority, SpdyConnection* pSpdyConn, uint8_t Spdy_Flags, HioStreamHandler * pHandler );
    int onInitConnected();    

    int getPriority() const 
    {
        return m_iPriority;
    }
   
    int appendReqData(char* pData, int len, uint8_t Spdy_Flags);
    
    int read( char * buf, int len );
    
    uint32_t getStreamID() 
    {   return m_uiStreamID;    }
    
    int write( const char * buf, int len );
    int writev( const struct iovec * vec, int count );
    int writev( IOVec &vector, int total );

    int sendfile( int fdSrc, off_t off, size_t size )
    {
        return 0;
    };
    void switchWriteToRead() {};

    int flush();
    int sendRespHeaders( HttpRespHeaders * pHeaders );

    void suspendRead() 
    {   setFlag(HIO_FLAG_WANT_READ, 0);     }
    void suspendWrite() 
    {   setFlag(HIO_FLAG_WANT_WRITE, 0);    }

    void continueRead();
    void continueWrite();

    void onTimer();
    int close();
    
    int onWrite();

    int isFlowCtrl() const          {   return getFlag( HIO_FLAG_FLOWCTRL );    }
    
    int32_t getWindowOut() const    {   return m_iWindowOut;    }
    int adjWindowOut( int32_t n  );

    int32_t getWindowIn() const     {   return m_iWindowIn;     }
    void adjWindowIn( int32_t n  )  {   m_iWindowIn += n;       }
    
    void clearBufIn()               {   m_bufIn.clear();        }
    LoopBuf* getBufIn()             {   return &m_bufIn;        }
    int appendInputData( const char* pData, int len )
    {
        return m_bufIn.append(pData, len);
    }

    int getDataFrameSize( int wanted );

    
    void appendInputData( char ch )
    {
        return m_bufIn.append( ch );
    }

    
private:
    SpdyStream(const SpdyStream& other);
    SpdyStream& operator=(const SpdyStream& other);
    bool operator==(const SpdyStream& other) const;

    void buildDataFrameHeader( char * pHeader, int length );
    int sendData( IOVec * pIov, int total );
    int sendFin();
    
    

protected:
    virtual const char * buildLogId();

private:
    uint32_t    m_uiStreamID;
    int         m_iPriority;
    int32_t     m_iWindowOut;
    int32_t     m_iWindowIn;
    SpdyConnection* m_pSpdyConn;
    LoopBuf     m_bufIn;
};





#endif // SPDYSTREAM_H
