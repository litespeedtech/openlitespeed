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
#ifndef HTTPIOLINK_H
#define HTTPIOLINK_H



#include <edio/eventreactor.h>
#include <edio/outputstream.h>
#include <http/clientinfo.h>

#include <sslpp/sslconnection.h>
#include <util/logtracker.h>

#include <sys/types.h>

#define IO_PEER_HUP 1
#define IO_PEER_ERR 2
#define IO_HTTP_ERR 4

class SSLConnection;
class SSLContext;
struct sockaddr;

enum
{
    HC_WAITING,
    HC_READING,
    HC_READING_BODY,
    HC_EXT_AUTH,
    HC_THROTTLING,
    HC_PROCESSING,
    HC_REDIRECT,
    HC_WRITING,
    HC_AIO_PENDING,
    HC_AIO_COMPLETE,
    HC_COMPLETE,
    HC_SHUTDOWN,
    HC_CLOSING
};

#define HIO_WANT_READ   1
#define HIO_WANT_WRITE  2


class HttpIOLink : public EventReactor, public OutputStream
                 , public LOG4CXX_NS::ILog
{
private:
    typedef int (*writev_fn)( HttpIOLink* pThis, IOVec &vector, int total );
    typedef int (*read_fn)( HttpIOLink* pThis, char *pBuf, int size );
    typedef int (*onRW_fn)( HttpIOLink * pThis );
    typedef void (*onTimer_fn)( HttpIOLink * pThis );
    typedef int (*close_fn)( HttpIOLink * pThis);
    class fn_list
    {
    public:
        read_fn         m_read_fn;
        writev_fn       m_writev_fn;
        onRW_fn         m_onWrite_fn;
        onRW_fn         m_onRead_fn;
        close_fn        m_close_fn;
        onTimer_fn      m_onTimer_fn;
        
        fn_list( read_fn rfn, writev_fn wvfn, onRW_fn onwfn, onRW_fn onrfn,
                close_fn cfn, onTimer_fn otfn )
            : m_read_fn( rfn )
            , m_writev_fn( wvfn )
            , m_onWrite_fn( onwfn )
            , m_onRead_fn( onrfn )
            , m_close_fn( cfn )
            , m_onTimer_fn( otfn )
            {}
    };

    class fn_list_list
    {
    public:
        fn_list       * m_pNoSSL;
        fn_list       * m_pSSL;
        
        fn_list_list( fn_list *lplain, fn_list *lssl )
            : m_pNoSSL( lplain )
            , m_pSSL( lssl )
            {}
    };

    static class fn_list        s_normal;
    static class fn_list        s_normalSSL;
    static class fn_list        s_throttle;
    static class fn_list        s_throttleSSL;

    static class fn_list_list   s_fn_list_list_normal;
    static class fn_list_list   s_fn_list_list_throttle;

    static class fn_list_list  *s_pCur_fn_list_list;


    ClientInfo        * m_pClientInfo;
    long                m_lActiveTime;

    char                m_iWant;
    char                m_iInProcess;
    char                m_iPeerShutdown;
    char                m_iState;
    int                 m_tmToken;
    int                 m_iSslLastWrite;
    off_t               m_lBytesRead;
    off_t               m_lBytesSent;
    SSLConnection       m_ssl;

    class fn_list     * m_pFnList;

    HttpIOLink( const HttpIOLink& rhs );
    void operator=( const HttpIOLink& rhs );

    bool wantRead() const
    {   return m_iWant & HIO_WANT_READ;     }
    int doRead()
    {
        if ( wantRead() )
            return onReadEx();
        else
            suspendRead();
        return 0;
    }

    int doReadT()
    {
        if ( wantRead() )
        {
            if ( allowRead() )
                return onReadEx();
            else
            {
                suspendRead();
                m_iWant |= HIO_WANT_READ;
            }
        }
        else
            suspendRead();
        return 0;
    }

    int doWrite()
    {
        if ( m_iWant & HIO_WANT_WRITE )
            return onWriteEx();
        return 0;
    }

    int checkWriteRet( int len, int size );
    int checkReadRet( int ret, int size );
    void setSSLAgain();

    static int writevEx     ( HttpIOLink* pThis, IOVec &vector, int total );
    static int writevExT    ( HttpIOLink* pThis, IOVec &vector, int total );
    static int writevExSSL  ( HttpIOLink* pThis, IOVec &vector, int total );
    static int writevExSSL_T( HttpIOLink* pThis, IOVec &vector, int total );

    static int readEx       ( HttpIOLink* pThis, char * pBuf, int size );
    static int readExT      ( HttpIOLink* pThis, char * pBuf, int size );
    static int readExSSL    ( HttpIOLink* pThis, char * pBuf, int size );
    static int readExSSL_T  ( HttpIOLink* pThis, char * pBuf, int size );

    static int onReadSSL( HttpIOLink * pThis );
    static int onReadSSL_T( HttpIOLink * pThis );
    static int onReadT( HttpIOLink *pThis );
    static int onRead(HttpIOLink *pThis );

    static int onWriteSSL( HttpIOLink * pThis );
    static int onWriteSSL_T( HttpIOLink * pThis );
    static int onWriteT( HttpIOLink *pThis );
    static int onWrite(HttpIOLink *pThis );

    static int close_( HttpIOLink * pThis );
    static int closeSSL( HttpIOLink * pThis );
    static void onTimer_T( HttpIOLink * pThis );
    static void onTimer_( HttpIOLink * pThis );
    static void onTimerSSL_T( HttpIOLink * pThis );

    void ignoreReadBuf();
        
    bool allowWrite() const
    {   return m_pClientInfo->allowWrite();  }
    
    int  detectClose();
    int write( const char * pBuf, int size );
    int sendfileEx( IOVec &vector, int total, int fdSrc, off_t off, size_t size );
    int my_sendfileEx( int fdSrc, off_t off, size_t size );
    
    void updateSSLEvent();
    void checkSSLReadRet( int ret );

    virtual int onReadEx() = 0;
    virtual int onWriteEx() = 0;
    virtual void closeConnection( int canceled = 0 ) = 0;
    virtual int onTimerEx() = 0;
    virtual int onInitConnected() = 0;
    virtual void recycle() = 0;

protected:
    void closeSocket();
    bool allowRead() const
    {   return m_pClientInfo->allowRead();   }
    char wantWrite() const    {   return m_iWant & HIO_WANT_WRITE;    }
    void setActiveTime( long lTime )
    {   m_lActiveTime = lTime;              }
    int close()     {   return (*m_pFnList->m_close_fn)( this );      }

public:


    HttpIOLink();
    virtual ~HttpIOLink();

    char inProcess() const    {   return m_iInProcess;    }

    int read( char * pBuf, int size )
    {   return (*m_pFnList->m_read_fn)( this, pBuf, size ); }

    // Output stream interfaces
    bool canHold( int size )    {   return allowWrite();        }

    void setPeerShutdown( int s) {    m_iPeerShutdown |= s;  }
    int  isConnCanceled()   {   return m_iPeerShutdown & IO_PEER_ERR;  }
    
    int writev( IOVec &vector, int total )
    {   return (*m_pFnList->m_writev_fn)( this, vector, total );    }
    
    int sendfile( IOVec &vector, int &total, int fdSrc, off_t off, size_t size );
    
    int flush()     {   return 0;   }

    void setNoSSL() {   m_pFnList = s_pCur_fn_list_list->m_pNoSSL;    }
    
    // SSL interface
    void setSSL( SSL* pSSL )
    {
        m_pFnList = s_pCur_fn_list_list->m_pSSL;
        m_ssl.setSSL( pSSL );
        m_ssl.setfd( getfd() );
    }
    
    SSLConnection* getSSL()     {   return &m_ssl;  }
    bool isSSL() const          {   return m_ssl.getSSL() != NULL;  }
    
    int SSLAgain();
    int acceptSSL();
    int flushSSL();

    
    int setLink( int fd, ClientInfo * pInfo, SSLContext * pSSLContext );

    const char * getPeerAddrString() const
    {   return m_pClientInfo->getAddrString();   }
    int getPeerAddrStrLen() const
    {   return m_pClientInfo->getAddrStrLen();   }

    const struct sockaddr * getPeerAddr() const
    {   return m_pClientInfo->getAddr();   }

    void changeClientInfo( ClientInfo * pInfo );
    
    
    //Event driven IO interface
    int handleEvents( short event );

    void setNotWantRead()   {   m_iWant &= ~HIO_WANT_READ;  }
    void suspendRead();
    void continueRead();
    void suspendWrite();
    void continueWrite();
    void switchWriteToRead();

    void checkThrottle();
    //void setThrottleLimit( int limit )
    //{   m_baseIO.getThrottleCtrl().setLimit( limit );    }

    void onTimer()
    {   (*m_pFnList->m_onTimer_fn)( this );   }

    //void stopThrottleTimer();
    //void startThrottleTimer();

    long getActiveTime() const
    {   return m_lActiveTime;               }
    ClientInfo * getClientInfo() const
    {   return m_pClientInfo;               }
    ThrottleControl* getThrottleCtrl() const
    {   return  &( m_pClientInfo->getThrottleCtrl() );  }
    char getState() const
    {   return m_iState;    }
    void setState( char state )
    {   m_iState = state;   }
    int isClosing() const
    {   return m_iState >= HC_SHUTDOWN;     }
    static void enableThrottle( int enable );
    int isThrottle() const
    {   return m_pFnList->m_onTimer_fn != onTimer_; }
    void suspendEventNotify();
    void resumeEventNotify();

    void tryRead();

    int getServerAddrStr( char * pBuf, int len );

    off_t getBytesRead() const  {   return m_lBytesRead;    }
    void resetBytesRead()       {   m_lBytesRead = 0;       }

    off_t getBytesSent() const  {   return m_lBytesSent;    }
    void resetBytesSent()       {   m_lBytesSent = 0;       }
    
};


#endif
