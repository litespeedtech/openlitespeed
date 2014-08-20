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
#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H


#include <edio/inputstream.h>
#include <http/httpreq.h>
#include <http/httpresp.h>
#include <http/sendfileinfo.h>
#include <http/ntwkiolink.h>
#include <lsiapi/lsimoduledata.h>

#include <util/linkedobj.h>

class ReqHandler;
class VHostMap;
class ChunkInputStream;
class ChunkOutputStream;
class ExtWorker;
class VMemBuf;
class GzipBuf;
class SSIScript;
class NtwkIOLink;
class LsiApiHooks;

enum  HttpSessionState {
    HSS_WAITING,
    HSS_READING,
    HSS_READING_BODY,
    HSS_EXT_AUTH,
    HSS_THROTTLING,
    HSS_PROCESSING,
    HSS_REDIRECT,
    HSS_EXT_REDIRECT,
    HSS_HTTP_ERROR,
    HSS_WRITING,
    HSS_AIO_PENDING,
    HSS_AIO_COMPLETE,
    HSS_COMPLETE,
};

#define HSF_URI_PROCESSED           (1<<0)
#define HSF_HANDLER_DONE            (1<<1)
#define HSF_RESP_HEADER_SENT        (1<<2)
#define HSF_MODULE_WRITE_SUSPENDED  (1<<3)
#define HSF_RESP_FLUSHED            (1<<4)
#define HSF_REQ_BODY_DONE           (1<<5)
#define HSF_REQ_WAIT_FULL_BODY      (1<<6)
#define HSF_RESP_WAIT_FULL_BODY     (1<<7)
#define HSF_RESP_HEADER_DONE        (1<<8)
#define HSF_ACCESS_LOG_OFF          (1<<9)
#define HSF_HOOK_SESSION_STARTED    (1<<10)
#define HSF_RECV_RESP_BUFFERED      (1<<11)
#define HSF_SEND_RESP_BUFFERED      (1<<12)
#define HSF_CHUNK_CLOSED            (1<<13)



class HttpSession : public LsiSession, public HioStreamHandler 
{    
    HttpReq               m_request;
    HttpResp              m_response;

    LsiModuleData         m_moduleData; //lsiapi user data of http level

    HttpSessionHooks  m_sessionHooks;
    
    ChunkInputStream    * m_pChunkIS;
    ChunkOutputStream   * m_pChunkOS;
    HttpResp            * m_pSubResp;
    ReqHandler          * m_pHandler;
    
    off_t                 m_lDynBodySent;
    
    VMemBuf             * m_pRespBodyBuf;
    GzipBuf             * m_pGzipBuf;
    NtwkIOLink          * m_pNtwkIOLink;
    
    SendFileInfo          m_sendFileInfo;
    
    long                  m_lReqTime;
    int32_t               m_iReqTimeUs;

    int32_t               m_iFlag;
    short                 m_iState;
    unsigned short        m_iReqServed;
    //int                   m_accessGranted;

    HttpSession( const HttpSession& rhs );
    void operator=( const HttpSession& rhs );

    void processPending( int ret );

    void parseHost( char * pHost );

    int  buildErrorResponse( const char * errMsg );

    void cleanUpHandler();
    void nextRequest();
    int  updateClientInfoFromProxyHeader( const char * pProxyHeader );
    
    static int readReqBodyTermination( LsiSession *pSession, char * pBuf, int size );
    
public:
    void closeConnection();
    void recycle();
    
    int isHookDisabled( int level ) const
    {   return m_sessionHooks.isDisabled( level );  }

    int isRespHeaderSent() const
    {   return m_iFlag & HSF_RESP_HEADER_SENT;   }
    
    void setFlag( int f )       {   m_iFlag |= f;               }
    void setFlag( int f, int v )
    {   m_iFlag = ( m_iFlag & ~f )|( v ? f : 0 );               }
    void clearFlag( int f )     {   m_iFlag &=~f;               }
    int32_t getFlag() const       {   return m_iFlag;             }
    int32_t getFlag( int mask)    {   return m_iFlag & mask;      }
    
private:   
    int checkAuthorizer( const HttpHandler * pHandler );
    int assignHandler( const HttpHandler * pHandler );
    int readReqBody();
    int reqBodyDone();
    int processReqBody();
    int processNewReq();
    int processURI( const char * pURI );
    int readToHeaderBuf();
    void sendHttpError( const char * pAdditional );
    int detectTimeout();
        
    //int cacheWrite( const char * pBuf, int size );
    //int writeRespBuf();

    void releaseChunkOS();
    void releaseRequestResource();

    void setupChunkIS();
    void releaseChunkIS();
    int  doWrite();
    int  doRead();
    int processURI( int resume );
    int checkAuthentication( const HTAuth * pHTAuth,
                    const AuthRequired * pRequired, int resume);

    void logAccess( int cancelled );
    int  detectKeepAliveTimeout( int delta );
    int  detectConnectionTimeout( int delta );
    void resumeSSI();    
    int sendStaticFile(  SendFileInfo * pData );
    int sendStaticFileEx(  SendFileInfo * pData );
    void releaseSendFileInfo();
    int chunkSendfile( int fdSrc, off_t off, size_t size );
    int processWebSocketUpgrade(const HttpVHost* pVHost);
    int sendRespHeaders( );   
    int resumeHandlerProcess();
    int flushBody();
    int endResponseInternal( int success );

    int getModuleDenyCode( int iHookLevel );
    int processHkptResult( int iHookLevel, int ret );
    int restartHandlerProcess();
    int runFilter( int hookLevel, void *pfTerm, const char* pBuf, int len, int flagIn );
    int contentEncodingFixup();
    

public:
    int  flush();
    
        
public:
    NtwkIOLink * getNtwkIOLink() const      {   return m_pNtwkIOLink;   }
    void setNtwkIOLink( NtwkIOLink * p )    {   m_pNtwkIOLink = p;      }
    //below are wrapper functions
    SSLConnection* getSSL()     {   return m_pNtwkIOLink->getSSL();  }
    bool isSSL() const          {   return m_pNtwkIOLink->isSSL();  }
    const char * getPeerAddrString() const {    return m_pNtwkIOLink->getPeerAddrString();  }
    int getPeerAddrStrLen() const   {   return m_pNtwkIOLink->getPeerAddrStrLen();   }
    const struct sockaddr * getPeerAddr() const {   return m_pNtwkIOLink->getPeerAddr(); }
    
    void suspendRead()          {    getStream()->suspendRead();        };
    void continueRead()         {    getStream()->continueRead();       };
    void suspendWrite()         {    getStream()->suspendWrite();       };
    void continueWrite()        {    getStream()->continueWrite();      };
    void switchWriteToRead()    {    getStream()->switchWriteToRead();  };
    
    void suspendEventNotify()   {    m_pNtwkIOLink->suspendEventNotify(); };
    void resumeEventNotify()    {    m_pNtwkIOLink->resumeEventNotify(); };
    void tryRead()              {    m_pNtwkIOLink->tryRead();          };
    off_t getBytesRecv() const  {   return getStream()->getBytesRecv();    }
    off_t getBytesSent() const  {   return getStream()->getBytesSent();    }
    ClientInfo * getClientInfo() const {    return m_pNtwkIOLink->getClientInfo();  }
    
    HttpSessionState getState() const       {   return (HttpSessionState)m_iState;    }
    void setState( HttpSessionState state ) {   m_iState = (short)state;   }
    int getServerAddrStr( char * pBuf, int len );
    int isAlive();
    int setUpdateStaticFileCache( StaticFileCacheData * &pCache, 
                                  FileCacheDataEx * &pECache,
                                  const char * pPath, int pathLen,
                                  int fd, struct stat &st ); 
    
    void releaseFileCacheDataEx(FileCacheDataEx * &pECache);
    void releaseStaticFileCacheData(StaticFileCacheData * &pCache);
    
    int isEndResponse() const   { return (m_iFlag & HSF_HANDLER_DONE );     }
    
public:
    void setupChunkOS(int nobuffer);

    HttpSession();
    ~HttpSession();

        
    unsigned short getRemotePort() const
    {   return m_pNtwkIOLink->getRemotePort();   };

    void setVHostMap( const VHostMap* pMap )
    {   m_iReqServed = 0;
        m_request.setVHostMap( pMap);       }
        
    HttpReq* getReq()
    {   return &m_request;  }
//     HttpResp* getResp()
//     {   return &m_response; }

    long getReqTime() const {   return m_lReqTime;  }
    int32_t getReqTimeUs() const    {   return m_iReqTimeUs;    }

    int writeRespBodyDirect( const char * pBuf, int size );
    int writeRespBody( const char * pBuf, int len );
    
    bool sendBody() const
    {   return !m_request.noRespBody();  }


    int onReadEx();
    int onWriteEx();
    int onInitConnected();
    int onCloseEx();

    int redirect( const char * pNewURL, int len, int alloc = 0 );
    int getHandler( const char * pURI, ReqHandler* &pHandler );
    //int setLocation( const char * pLoc );

    //int startForward( int fd, int type );
    bool endOfReqBody();    
    void setWaitFullReqBody()
    {    setFlag( HSF_REQ_WAIT_FULL_BODY );    }
    
    int  onTimerEx();

    //void accessGranted()    {   m_accessGranted = 1;  }
    void changeHandler() {    setState( HSS_REDIRECT ); };
    
    //const char * buildLogId();

    void httpError( int code, const char * pAdditional = NULL)
    {   if (code < 0)
            code = SC_500;
        m_request.setStatusCode( code );
        sendHttpError( pAdditional );
    }
    int read( char * pBuf, int size );
    int readv( struct iovec *vector, size_t count);
    ReqHandler * getCurHandler() const  {   return m_pHandler;  }


    void resumeAuthentication();
    void authorized();
    
    void addEnv( const char * pKey, int keyLen, const char * pValue, long valLen );

    int writeRespBodySendFile( int fdFile, off_t offset, size_t size );
    int setupRespCache();   
    void releaseRespCache();
    int sendDynBody();
    int setupGzipFilter();
    int setupGzipBuf();
    void releaseGzipBuf();
    int appendDynBody( const char * pBuf, int len );  
    int appendDynBodyEx( const char * pBuf, int len );  

    int appendRespBodyBuf( const char * pBuf, int len );
    int appendRespBodyBufV( const iovec *vector, int count );
    
    int shouldSuspendReadingResp();
    void resetRespBodyBuf();
    int checkRespSize( int nobuffer );

    int respHeaderDone( int respState );
    
    void setRespBodyDone()
    {   m_iFlag |= HSF_HANDLER_DONE; 
        if ( m_pChunkOS )
        {
            setFlag(HSF_RESP_FLUSHED, 0);
            flush();
        }
    }
    
    int endResponse( int success );
    int setupDynRespBodyBuf();
    GzipBuf * getGzipBuf() const    {   return m_pGzipBuf;  }
    VMemBuf * getRespCache() const  {   return m_pRespBodyBuf; }
    off_t getDynBodySent() const    {   return m_lDynBodySent; }
    //int flushDynBody( int nobuff );
    int execExtCmd( const char * pCmd, int len );
    int handlerProcess( const HttpHandler * pHandler );
    int getParsedScript( SSIScript * &pScript );
    int startServerParsed();
    HttpResp* getResp()
    {   return (m_pSubResp)?m_pSubResp:&m_response; }    
    int flushDynBodyChunk();
    //int writeConnStatus( char * pBuf, int bufLen );  

    void resetResp()
    {   getResp()->reset(); }
    
    LOG4CXX_NS::Logger* getLogger() const   {   return getStream()->getLogger();   }
    
    const char * getLogId() {   return getStream()->getLogId();     }

    LogTracker * getLogTracker()    {   return getStream();     }

    SendFileInfo* getSendFileInfo() {   return &m_sendFileInfo;   }

    int openStaticFile( const char * pPath, int pathLen, int *status );

    

    /**
     * @brief initSendFileInfo() should be called before start sending a static file
     * 
     * @param pPath[in] an file path to a static file
     * @param pathLen[in] the lenght of the path
     * @return 0 if successful, HTTP status code in SC_xxx predefined macro.
     * 
     **/
    int initSendFileInfo( const char * pPath, int pathLen );

    /**
     * @brief setSendFileOffset() set the start point and length of the file to be send
     * 
     * @param start[in] file offset from which will start reading data  from
     * @param size[in]  the number of bytes to be sent 
     * 
     **/

    void setSendFileOffsetSize( off_t start, off_t size );

    int finalizeHeader( int ver, int code );
    LsiModuleData* getModuleData()      {   return &m_moduleData;   }
    
    LsiApiHooks * getModSessionHooks( int index )
    {   return m_sessionHooks.getCopy( index ); }
    void setSendFileBeginEnd( off_t start, off_t end );
    void prepareHeaders();
    void addLocationHeader();
    
    void setAccessLogOff()      {   m_iFlag |= HSF_ACCESS_LOG_OFF;  }
    int shouldLogAccess() const    
    {   return !(m_iFlag & HSF_ACCESS_LOG_OFF );    }

    int updateContentCompressible( );

};

#endif
