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
#include <http/httpiolink.h>
#include <http/httpreq.h>
#include <http/httpresp.h>
#include <util/linkedobj.h>


class ReqHandler;
class VHostMap;
class ChunkInputStream;
class ChunkOutputStream;
class ExtWorker;
class VMemBuf;
class GzipBuf;
class SSIScript;

class HttpConnection : public HttpIOLink, public InputStream
{
    
    AutoStr2              m_logID;
    LOG4CXX_NS::Logger  * m_pLogger;
    
    HttpReq               m_request;
    HttpResp              m_response;
    
    ChunkInputStream    * m_pChunkIS;
    ChunkOutputStream   * m_pChunkOS;
    HttpResp            * m_pSubResp;
    ReqHandler          * m_pHandler;
    
    off_t                 m_lDynBodySent;
    int                   m_iRespBodyCacheOffset;
    
    VMemBuf             * m_pRespCache;
    GzipBuf             * m_pGzipBuf;

    long                  m_lReqTime;
    int32_t               m_iReqTimeMs;

    unsigned short        m_iReqServed;
    unsigned short        m_iRemotePort;
    int                   m_iLogIdBuilt;
    //int                   m_accessGranted;

    HttpConnection( const HttpConnection& rhs );
    void operator=( const HttpConnection& rhs );

    void processPending( int ret );

    void parseHost( char * pHost );

    int  buildErrorResponse( const char * errMsg );

    void cleanUpHandler();
    void nextRequest();
    void closeConnection( int cancel = 0);
    void recycle();
    int checkAuthorizer( const HttpHandler * pHandler );
    int assignHandler( const HttpHandler * pHandler );
    int readReqBody( char * pBuf, int size );
    int readReqBody();
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
    int  flush();

    void setupChunkIS();
    void releaseChunkIS();
    int  doWrite(int aioSent = 0 );
    int  doRead();
    int writeRespBodyPlain( IOVec& iov, int &headerTotal,
                            const char * pBuf, int size );
    int processURI( int resume );
    int checkAuthentication( const HTAuth * pHTAuth,
                    const AuthRequired * pRequired, int resume);

    void logAccess( int cancelled );
    
protected:

public:

    void setupChunkOS(int nobuffer);

    HttpConnection();
    ~HttpConnection();


    void setRemotePort( unsigned short port )
    {   m_iRemotePort = port;   m_iLogIdBuilt = 0;  }
    unsigned short getRemotePort() const
    {   return m_iRemotePort;       }

    void setVHostMap( const VHostMap* pMap )
    {   m_iReqServed = 0;
        m_request.setVHostMap( pMap);       }
        
    HttpReq* getReq()
    {   return &m_request;  }
//     HttpResp* getResp()
//     {   return &m_response; }

    long getReqTime() const {   return m_lReqTime;  }
    int32_t getReqTimeMs() const    {   return m_iReqTimeMs;    }

    int writeRespBody( const char * pBuf, int size );

    int writeRespBodyv( IOVec& vector, int total );
    
    bool sendBody() const
    {   return !m_request.noRespBody();  }

    int  beginWrite();

    void writeComplete();

    int onReadEx();
    int onWriteEx();
    int onInitConnected();

    int redirect( const char * pNewURL, int len, int alloc = 0 );
    int getHandler( const char * pURI, ReqHandler* &pHandler );
    //int setLocation( const char * pLoc );

    //int startForward( int fd, int type );
    bool endOfReqBody();    
    int  onTimerEx();

    //void accessGranted()    {   m_accessGranted = 1;  }
    void changeHandler()
    {
        setState( HC_REDIRECT );
    }

    void setLogger( LOG4CXX_NS::Logger* pLogger )
    {   m_pLogger = pLogger;    }
    LOG4CXX_NS::Logger* getLogger() const
    {   return m_pLogger;   }
    
    const char * buildLogId();
    const char *  getLogId()
    {   
        if ( m_iLogIdBuilt )
            return m_logID.c_str();
        return buildLogId();
    }

    void httpError( int code, const char * pAdditional = NULL)
    {   m_request.setStatusCode( code );
        sendHttpError( pAdditional );
    }
    int read( char * pBuf, int size );
    int readv( struct iovec *vector, size_t count);
    ReqHandler * getCurHandler() const  {   return m_pHandler;  }


    void resumeAuthentication();
    void setBuffering( int s );
    void authorized();

    int writeRespBodySendFile( int fdFile, off_t offset, size_t size );
    int setupRespCache();   
    void releaseRespCache();
    int sendDynBody();
    int setupGzipFilter();
    int setupGzipBuf( int type );
    void releaseGzipBuf();
    int appendDynBody( int inplace, const char * pBuf, int len );  
    int appendRespCache( const char * pBuf, int len );
    int shouldSuspendReadingResp();
    void resetRespBodyBuf();
    int checkRespSize( int nobuffer );
    int endDynResp( int cacheable );
    int prepareDynRespHeader( int complete, int nobuffer );
    int setupDynRespBodyBuf( int &iRespState );
    GzipBuf * getGzipBuf() const    {   return m_pGzipBuf;  }
    VMemBuf * getRespCache() const  {   return m_pRespCache; }
    off_t getDynBodySent() const    {   return m_lDynBodySent; }
    int processModSecRules( int phase );
    int flushDynBody( int nobuff );
    int execExtCmd( const char * pCmd, int len );
    int handlerProcess( const HttpHandler * pHandler );
    int getParsedScript( SSIScript * &pScript );
    int startServerParsed();
    HttpResp* getResp()
    {   return (m_pSubResp)?m_pSubResp:&m_response; }    
    int flushDynBodyChunk();
    //int writeConnStatus( char * pBuf, int bufLen );  

    

};

#endif
