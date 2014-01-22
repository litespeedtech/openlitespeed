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
#include <http/clientinfo.h>
#include <http/hiostream.h>

#include <sslpp/sslconnection.h>
#include <util/logtracker.h>
#include <spdy/spdyprotocol.h>

#include <sys/types.h>

#define IO_HTTP_ERR 4

class VHostMap;
class SSLConnection;
class SSLContext;
struct sockaddr;


class NtwkIOLink : public EventReactor, public HioStream
{
private:
    typedef int (*writev_fn)( NtwkIOLink* pThis, IOVec &vector, int total );
    typedef int (*read_fn)( NtwkIOLink* pThis, char *pBuf, int size );
    typedef int (*onRW_fn)( NtwkIOLink * pThis );
    typedef void (*onTimer_fn)( NtwkIOLink * pThis );
    typedef int (*close_fn)( NtwkIOLink * pThis);
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
    const VHostMap    * m_pVHostMap;
    unsigned short      m_iRemotePort;

    char                m_iInProcess;
    char                m_iPeerShutdown;
    int                 m_tmToken;
    int                 m_iSslLastWrite;
    SSLConnection       m_ssl;

    class fn_list     * m_pFnList;

    NtwkIOLink( const NtwkIOLink& rhs );
    void operator=( const NtwkIOLink& rhs );

    int doRead()
    {
        if ( isWantRead() )
            return getHandler()->onReadEx();
        else
            suspendRead();
        return 0;
    }

    int doReadT()
    {
        if ( isWantRead() )
        {
            if ( allowRead() )
                return getHandler()->onReadEx();
            else
            {
                suspendRead();
                setFlag( HIO_FLAG_WANT_READ, 1 );
            }
        }
        else
            suspendRead();
        return 0;
    }

    int doWrite()
    {
        if ( isWantWrite() )
            return getHandler()->onWriteEx();
        else
        {
            suspendWrite();
            if ( isSSL() )
                flush();
        }
        return 0;
    }

    int checkWriteRet( int len, int size );
    int checkReadRet( int ret, int size );
    void setSSLAgain();

    static int writevEx     ( NtwkIOLink* pThis, IOVec &vector, int total );
    static int writevExT    ( NtwkIOLink* pThis, IOVec &vector, int total );
    static int writevExSSL  ( NtwkIOLink* pThis, IOVec &vector, int total );
    static int writevExSSL_T( NtwkIOLink* pThis, IOVec &vector, int total );

    static int readEx       ( NtwkIOLink* pThis, char * pBuf, int size );
    static int readExT      ( NtwkIOLink* pThis, char * pBuf, int size );
    static int readExSSL    ( NtwkIOLink* pThis, char * pBuf, int size );
    static int readExSSL_T  ( NtwkIOLink* pThis, char * pBuf, int size );

    static int onReadSSL( NtwkIOLink * pThis );
    static int onReadSSL_T( NtwkIOLink * pThis );
    static int onReadT( NtwkIOLink *pThis );
    static int onRead(NtwkIOLink *pThis );

    static int onWriteSSL( NtwkIOLink * pThis );
    static int onWriteSSL_T( NtwkIOLink * pThis );
    static int onWriteT( NtwkIOLink *pThis );
    static int onWrite(NtwkIOLink *pThis );

    static int close_( NtwkIOLink * pThis );
    static int closeSSL( NtwkIOLink * pThis );
    static void onTimer_T( NtwkIOLink * pThis );
    static void onTimer_( NtwkIOLink * pThis );
    static void onTimerSSL_T( NtwkIOLink * pThis );

    void drainReadBuf();
        
    bool allowWrite() const
    {   return m_pClientInfo->allowWrite();  }
    
    int write( const char * pBuf, int size );
    int sendfileEx( IOVec &vector, int total, int fdSrc, off_t off, size_t size );
    int my_sendfileEx( int fdSrc, off_t off, size_t size );
    
    void updateSSLEvent();
    void checkSSLReadRet( int ret );
    
    int setupHandler( HiosProtocol verSpdy );
    int sslSetupHandler();
    void doClose();

    void dumpState(const char * pFuncName, const char * action);

public:
    void setRemotePort( unsigned short port )
    {   
        m_iRemotePort = port;
        setLogIdBuild( 0 );  
    };
    
    unsigned short getRemotePort() const  {   return m_iRemotePort;       };
        
    int sendHeaders( IOVec &vector, int headerCount );
    
    const char * buildLogId();
    
public:
    void closeSocket();
    bool allowRead() const
    {   return m_pClientInfo->allowRead();   }
    int close()     {   return (*m_pFnList->m_close_fn)( this );      }
    int  detectClose();

public:


    NtwkIOLink();
    ~NtwkIOLink();

    char inProcess() const    {   return m_iInProcess;    }

    int read( char * pBuf, int size )
    {   return (*m_pFnList->m_read_fn)( this, pBuf, size ); }

    // Output stream interfaces
    bool canHold( int size )    {   return allowWrite();        }

    int writev( IOVec &vector, int total )
    {   return (*m_pFnList->m_writev_fn)( this, vector, total );    }
    
    int sendfile( IOVec &vector, int &total, int fdSrc, off_t off, size_t size );
    
    int flush();

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

    void setNotWantRead()   {   setFlag( HIO_FLAG_WANT_READ, 0 );  }
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

    ClientInfo * getClientInfo() const
    {   return m_pClientInfo;               }
    ThrottleControl* getThrottleCtrl() const
    {   return  &( m_pClientInfo->getThrottleCtrl() );  }
    int isClosing() const
    {   return getState() >= HIOS_SHUTDOWN;     }
    static void enableThrottle( int enable );
    int isThrottle() const
    {   return m_pFnList->m_onTimer_fn != onTimer_; }
    void suspendEventNotify();
    void resumeEventNotify();

    void tryRead();
    
    void tobeClosed() { setState(HIOS_CLOSING);  }

    void setVHostMap( const VHostMap* pMap ){   m_pVHostMap = pMap;     }
    const VHostMap * getVHostMap() const    {   return m_pVHostMap;     }
    
};


#endif
