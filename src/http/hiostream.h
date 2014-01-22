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
#ifndef HIOSTREAM_H
#define HIOSTREAM_H

#include <inttypes.h>
#include <sys/types.h>
#include <edio/inputstream.h>
#include <edio/outputstream.h>
#include <util/logtracker.h>

class IOVec;

class HioStreamHandler;

enum HioState 
{
    HIOS_DISCONNECTED,
    HIOS_CONNECTED,
    HIOS_CLOSING,
    HIOS_SHUTDOWN,
};

enum HiosProtocol 
{
    HIOS_PROTO_HTTP  = 0,
    HIOS_PROTO_SPDY2 = 1,
    HIOS_PROTO_SPDY3 = 2,
    HIOS_PROTO_SPDY31 = 3,
    HIOS_PROTO_MAX 
};

#define HIO_FLAG_PEER_SHUTDOWN      (1<<0)
#define HIO_FLAG_LOCAL_SHUTDOWN     (1<<1)
#define HIO_FLAG_WANT_READ          (1<<2)
#define HIO_FLAG_WANT_WRITE         (1<<3)
#define HIO_FLAG_ABORT              (1<<4)
#define HIO_FLAG_PEER_RESET         (1<<5)
#define HIO_FLAG_HANDLER_RELEASE    (1<<6)
#define HIO_FLAG_BUFF_FULL          (1<<7)
#define HIO_FLAG_FLOWCTRL           (1<<8)

class HioStream : public InputStream, public OutputStream, public LogTracker
{
    HioStreamHandler  * m_pHandler;
    off_t               m_lBytesRecv;
    off_t               m_lBytesSent;
    char                m_iState;
    char                m_iProtocol;
    short               m_iFlag;
    uint32_t            m_tmLastActive;
    
public:
    HioStream()
        : m_pHandler( NULL )
        , m_lBytesRecv( 0 )
        , m_lBytesSent( 0 )
        , m_iState( HIOS_DISCONNECTED )
        , m_iProtocol( HIOS_PROTO_HTTP )
        , m_iFlag( 0 )
        , m_tmLastActive( 0 )
    {}
    virtual ~HioStream();
    
    virtual int sendfile( IOVec &vector, int &total, int fdSrc, off_t off, size_t size ) = 0;
    virtual int readv( struct iovec *vector, size_t count ) 
    {       return -1;      }
    
    virtual int sendHeaders( IOVec &vector, int headerCount ) = 0;
    
    virtual void suspendRead()  = 0;
    virtual void continueRead() = 0;
    virtual void suspendWrite() = 0;
    virtual void continueWrite() = 0;
    virtual void switchWriteToRead() = 0;
    virtual void onTimer() = 0;
    //virtual uint32_t GetStreamID() = 0;
    
    void reset( int32_t timeStamp )
    {
        memset( &m_pHandler, 0, (char*)(&m_iFlag+1) - (char *)&m_pHandler );
        m_tmLastActive = timeStamp;
    }
        
    HioStreamHandler * getHandler() const   {   return m_pHandler;  }
    void setHandler( HioStreamHandler * p ) {   m_pHandler = p;     }
    
    short isWantRead() const    {   return m_iFlag & HIO_FLAG_WANT_READ;     }
    short isWantWrite() const   {   return m_iFlag & HIO_FLAG_WANT_WRITE;    }
    short isReadyToRelease() const {    return m_iFlag & HIO_FLAG_HANDLER_RELEASE;    }
    
    void setFlag( int flagbit, int val )
    {   m_iFlag = (val)? ( m_iFlag | flagbit ):( m_iFlag & ~flagbit );         }
    short getFlag( int flagbit ) const   {    return flagbit & m_iFlag;            }
    
    short isAborted() const     {   return m_iFlag & HIO_FLAG_ABORT;         }
    void  setAbortedFlag()      {   m_iFlag |= HIO_FLAG_ABORT;               }

    void handlerReadyToRelease(){   m_iFlag |= HIO_FLAG_HANDLER_RELEASE;     }
    short canWrite()  const     {   return m_iFlag & HIO_FLAG_BUFF_FULL;     }
    
    char  getProtocol() const   {   return m_iProtocol;      }
    void  setProtocol( int p )  {   m_iProtocol = p;         }
    
    int   isSpdy() const        {   return m_iProtocol;      }
        
    short getState() const          {   return m_iState;     }
    void  setState( HioState st )   {   m_iState = st;       }
    
    void  bytesRecv( int n )    {   m_lBytesRecv += n;      }
    void  bytesSent( int n )    {   m_lBytesSent += n;      }

    off_t getBytesRecv() const  {   return m_lBytesRecv;    }
    off_t getBytesSent() const  {   return m_lBytesSent;    }

    void setActiveTime( uint32_t lTime )
    {   m_tmLastActive = lTime;              }
    uint32_t getActiveTime() const
    {   return m_tmLastActive;               }
    
    short isPeerShutdown() const {  return m_iFlag & HIO_FLAG_PEER_SHUTDOWN;    }
    
    static const char * getProtocolName( HiosProtocol proto );
    
private:
    HioStream(const HioStream& other);
    HioStream& operator=(const HioStream& other);
    bool operator==(const HioStream& other) const;
    
};

class HioStreamHandler
{
    HioStream * m_pStream;
    
public:
    HioStreamHandler()
        : m_pStream( NULL )
        {}
    virtual ~HioStreamHandler();
    
    HioStream * getStream() const           {   return m_pStream;   }
    void setStream( HioStream * p )         {   m_pStream = p;      }
    void assignStream( HioStream * p )  
    {
        m_pStream  = p;
        p->setHandler( this );  
    }
    
    virtual int onInitConnected() = 0;
    virtual int onReadEx()  = 0;
    virtual int onWriteEx() = 0;
    virtual int onCloseEx() = 0;
    virtual int onTimerEx() = 0;
    
    virtual void recycle() = 0; 
    
private:
    HioStreamHandler(const HioStreamHandler& other);
    HioStreamHandler& operator=(const HioStreamHandler& other);
    bool operator==(const HioStreamHandler& other) const;
};


#endif // HIOSTREAM_H
