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
#ifndef HTTPREQ_H
#define HTTPREQ_H

#include <http/contextlist.h>
#include <util/autobuf.h>
#include <http/httpheader.h>
#include <http/httpmethod.h>
#include <http/httpver.h>
#include <http/staticfiledata.h>
#include <http/httpstatuscode.h>
#include <util/autostr.h>
#include <util/logtracker.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define CHUNKED                 -1
#define MAX_REDIRECTS           10

#define PROCESS_CONTEXT         (1<<0)
#define CONTEXT_AUTH_CHECKED    (1<<1)
#define REDIR_CONTEXT           (1<<2)
#define KEEP_AUTH_INFO          (1<<5)

#define REWRITE_QSD             (1<<6)
#define REWRITE_REDIR           (1<<7)
#define SKIP_REWRITE            (1<<8)

#define MP4_SEEK                (1<<9)
#define IS_ERROR_PAGE           (1<<10)

#define EXEC_EXT_CMD            (1<<12)
#define RESP_CONT_LEN_SET       (1<<13)


#define GZIP_ENABLED            1
#define REQ_GZIP_ACCEPT         2
#define UPSTREAM_GZIP           4
#define GZIP_REQUIRED           (GZIP_ENABLED | REQ_GZIP_ACCEPT)
#define GZIP_ADD_ENCODING       8

#define WAIT_FULL_REQ_BODY      (1<<15)



struct AAAData;
class AuthRequired;
class ClientInfo;
class ExpiresCtrl;
class HTAuth;
class HttpConnection;
class HttpContext;
class HttpRange;
class HttpHandler;
class HttpVHost;
class HotlinkCtrl;
class IOVec;
class MIMESetting;
class StaticFileCacheData;
class SSIRuntime;
class SSIConfig;
class VHostMap;
class VMemBuf;
class HttpRespHeaders;


typedef struct
{
    int keyOff;
    int keyLen;
    int valOff;
    int valLen;
} key_value_pair;

class HttpReq 
{
private:
    const VHostMap    * m_pVHostMap;
    AutoBuf             m_reqBuf;
    AutoBuf             m_headerBuf;

    int                 m_iReqHeaderBufFinished;

    int                 m_reqLineOff;
    int                 m_reqLineLen;

    key_value_pair      m_curURL;
    int                 m_reqURLOff;
    int                 m_reqURLLen;

    const MIMESetting * m_pMimeType;
    int                 m_iHttpHeaderEnd;


    short               m_commonHeaderLen[HttpHeader::H_TE];
    int                 m_commonHeaderOffset[HttpHeader::H_TE];
    int                 m_headerIdxOff;
    unsigned char       m_iHeaderStatus;
    unsigned char       m_iHS1;
    unsigned char       m_iHS3;
    unsigned char       m_iLeadingWWW;
    int                 m_iHS2;
    int                 m_method;
    unsigned int        m_ver;
    short               m_iKeepAlive;
    char                m_iAcceptGzip;
    char                m_iNoRespBody;
    long                m_lEntityLength;
    long                m_lEntityFinished;
    short               m_iRedirects;
    short               m_iContextState;
    const HttpHandler * m_pHttpHandler;

    //const HttpContext * m_pHTAContext;
    const HttpContext * m_pContext;
    const HttpContext * m_pFMContext;
    VMemBuf           * m_pReqBodyBuf;
    HttpRange         * m_pRange;
    const HttpVHost   * m_pVHost;
    const char        * m_pForcedType;
    StaticFileData      m_dataStaticFile;
    int                 m_iAuthUserOff;
    const HTAuth      * m_pHTAuth;
    const AuthRequired* m_pAuthRequired;
    SSIRuntime        * m_pSSIRuntime;
    int                 m_envIdxOff;
    int                 m_iHostOff;
    int                 m_iHostLen;
    int                 m_iPathInfoOff;
    int                 m_iPathInfoLen;
    const AutoStr2    * m_pRealPath;
    int                 m_iMatchedLen;
    int                 m_iNewHostLen;
    int                 m_iLocationOff;

    // The following member need not to be initialized
    AutoStr2            m_sRealPathStore;
    int                 m_iMatchedOff;
    struct stat         m_fileStat;
    int                 m_iScriptNameLen;
    key_value_pair    * m_urls;
    int                 m_code;
    int                 m_iLocationLen;
    int                 m_iNewHostOff;
    LogTracker        * m_pILog;

    int                 m_upgradeProto;
    
    HttpReq( const HttpReq& rhs ) ;
    void operator=( const HttpReq& rhs );

    int redirectDir( const char * pURI );
    int processPath( const char * pURI, int uriLen, char * pBuf,
                        char * pBegin, char * pEnd,  int &cacheable );
    int processURIEx(const char * pURI, int uriLen, int &cacheable);
    int processMatchList( const HttpContext * pContext, const char * pURI, int iURILen );
    int processMatchedURI( const char * pURI, int uriLen, char * pMatched, int len );
    int processSuffix( const char * pURI, const char * pURIEnd, int &cacheable );
    int processMime( const char * pSuffix );
    int filesMatch( const char * pEnd );

    int checkSymLink( int follow, char * pBuf, char * &p, char * pBegin );
    int checkSuffixHandler( const char * pURI, int len, int &cacheable );


    //parse headers
    int processRequestLine();
    int processHeaderLines();

    int translate( const char * pURI, int uriLen,
        const HttpContext* pContext, char * pReal, int len ) const;
    int internalRedirect( const char * pURL, int len, int alloc = 0 );
    //int internalRedirectURI( const char * pURI, int len );
    
    int contextRedirect( const HttpContext * pContext, int matchLen );
    int tryExtractHostFromUrl( const char * &pURL, int &len );

    void addHeader( size_t index, int off, int len )
    {
        m_commonHeaderOffset[index] = off;
        m_commonHeaderLen[index] = len;
    }
    key_value_pair * getCurHeaderIdx();
    key_value_pair * newUnknownHeader()
    {   return newKeyValueBuf( m_headerIdxOff );    }

    key_value_pair * newKeyValueBuf( int &idxOff);
    key_value_pair * getValueByKey( const AutoBuf& buf, int idxOff, const char * pName,
                        int namelen ) const;
    int  getListSize( int idxOff )
    {
        if ( !idxOff )
            return 0;
        char * p = m_reqBuf.getPointer( idxOff );
        return *( ((int *)p) + 1 );
    }
    key_value_pair * getOffset( int idxOff, int n )
    {
        if ( !idxOff )
            return 0;
        char * p = m_reqBuf.getPointer( idxOff );
        if ( n < *( ((int *)p) + 1 ))
            return ( key_value_pair *)( p + sizeof( int ) * 2 ) + n;
        else
            return NULL;
    }

    int appendIndexToUri( const char * pIndex, int indexLen );
    
    int dupString( const char * pString );
    int dupString( const char * pString, int len );

    int checkHotlink( const HotlinkCtrl * pHLC, const char * pSuffix );
    
protected:
    //this function is for testing purpose,
    // should only be called from a test program
    int appendTestHeaderData( const char * pBuf, int len )
    {   m_headerBuf.append( pBuf, len );
        return processHeader();
    }

public:

    enum
    {
        HEADER_REQUEST_LINE = 0,
        HEADER_REQUSET_LINE_DONE = 1,
        HEADER_HEADER = HEADER_REQUSET_LINE_DONE,
        HEADER_HEADER_LINE_END,
        HEADER_OK,
        HEADER_SKIP,
        HEADER_ERROR_TOLONG,
        HEADER_ERROR_INVALID
    };

    enum
    {
        UPD_PROTO_NONE = 0,
        UPD_PROTO_WEBSOCKET = 1,
    };
        
    explicit HttpReq();
    ~HttpReq();

    int processHeader();
    int processNewReqData( const struct sockaddr * pAddr );
    void reset();
    void reset2();

    void setILog( LogTracker * pILog ){   m_pILog = pILog;        }
    LOG4CXX_NS::Logger* getLogger() const   {   return m_pILog->getLogger();    }
    const char *  getLogId()                {   return m_pILog->getLogId();     }
    
    void setVHostMap( const VHostMap* pMap ){   m_pVHostMap = pMap;     }
    const VHostMap * getVHostMap() const    {   return m_pVHostMap;     }
    const HttpVHost * matchVHost();

    char getStatus() const                  {   return m_iHeaderStatus; }
    int getMethod() const                   {   return m_method;        }

    unsigned int getVersion() const         {   return m_ver;           }

    AutoBuf& getHeaderBuf()                 {   return m_headerBuf;     }

    const HttpVHost * getVHost() const          {   return m_pVHost;    }
    void setVHost( const HttpVHost * pVHost )   {   m_pVHost = pVHost;  }

    const char * getURI() const
    {   return m_reqBuf.getPointer( m_curURL.keyOff);   }
    
    void setNewHost( const char * pInfo, int len) 
    {
        m_iNewHostLen = len;
        m_iNewHostOff = dupString( pInfo, m_iNewHostLen );
    }

    int getNewHostLen() const   {   return m_iNewHostLen;       }
    const char * getNewHost() const {   return m_reqBuf.getPointer( m_iNewHostOff );   }
    
    const char * getPathInfo() const
    {   return m_reqBuf.getPointer( m_iPathInfoOff );   }

    const char * getQueryString() const
    {   return ( m_curURL.valOff >= 0 )
                    ? m_headerBuf.getPointer(m_curURL.valOff)
                    : m_reqBuf.getPointer(-m_curURL.valOff);  }

    const char * getQueryString( key_value_pair * p ) const
    {   return ( p->valOff >= 0 )
                    ? m_headerBuf.getPointer(p->valOff)
                    : m_reqBuf.getPointer(-p->valOff);  }
                    
    int   getURILen() const             {   return m_curURL.keyLen; }
    int   getPathInfoLen() const        {   return m_iPathInfoLen;  }
    int   getQueryStringLen() const     {   return m_curURL.valLen; }

    int   isWebsocket() const           {   return ((UPD_PROTO_WEBSOCKET == m_upgradeProto) ? 1 : 0); }
 
    //request header

    const char * getHeader( size_t index ) const
    {   return m_headerBuf.begin() + m_commonHeaderOffset[ index];  }

    const char * getHeader( const char * pName, int namelen, int& valLen ) const
    {
        key_value_pair * pIdx = getValueByKey( m_headerBuf, m_headerIdxOff, pName, namelen );
        if ( pIdx )
        {
            valLen = pIdx->valLen;
            return m_headerBuf.getPointer( pIdx->valOff );
        }
        return NULL;
    }

    int isHeaderSet( size_t index ) const
    {   return m_commonHeaderOffset[index];     }
    int getHeaderLen( size_t index ) const
    {   return m_commonHeaderLen[ index ];      }

    const char * getHostStr()
    {   return m_headerBuf.getPointer( m_iHostOff );    }
    const char * getOrgReqLine() const
    {   return m_headerBuf.getPointer( m_reqLineOff );  }
    const char * getOrgReqURL() const
    {   return m_headerBuf.getPointer( m_reqURLOff );   }
    int getHttpHeaderLen() const
    {   return m_iHttpHeaderEnd - m_reqLineOff;         }
    int getHttpHeaderEnd() const        {   return m_iHttpHeaderEnd;    }
    
    int getOrgReqLineLen() const        {   return m_reqLineLen;        }
    int getOrgReqURLLen() const         {   return m_reqURLLen;         }

    const char * encodeReqLine( int &len );
    
    const char * getAuthUser() const
    {   return m_reqBuf.getPointer( m_iAuthUserOff );   }

    bool isChunked() const      {   return m_lEntityLength == CHUNKED;  }
    long getBodyRemain() const
    {   return m_lEntityLength - m_lEntityFinished; }

    long getContentFinished() const     {   return m_lEntityFinished;   }
    void contentRead( long lLen )       {   m_lEntityFinished += lLen;  }


    const   char* getContentType() const
    {   return getHeader( HttpHeader::H_CONTENT_TYPE );     }

    void setContentLength( long len )   {   m_lEntityLength = len;      }
    long getContentLength() const       {   return m_lEntityLength;     }
    int  getHostStrLen()                {   return m_iHostLen;          }
    int  getScriptNameLen() const       {   return m_iScriptNameLen;    }

    const HttpHandler* getHttpHandler() const{  return m_pHttpHandler;  }
    
    int  translatePath( const char * pURI, int uriLen,
                char * pReal, int len ) const;
    int  setCurrentURL( const char * pURL, int len, int alloc = 0 );
    int  redirect( const char * pURL, int len, int alloc = 0 );
    int  postRewriteProcess( const char * pURI, int len );
    int  processContextPath();
    int  processContext();
    int  checkPathInfo( const char * pURI, int iURILen, int &pathLen,
                        short &scriptLen, short &pathInfoLen,
                        const HttpContext * pContext );
    void saveMatchedResult();
    void restoreMatchedResult();
    char * allocateAuthUser();

    void setErrorPage( )                {   m_iContextState |= IS_ERROR_PAGE;       }
    int  isErrorPage() const            {   return m_iContextState & IS_ERROR_PAGE; }
    
    short isKeepAlive() const           {   return m_iKeepAlive;        }
    void keepAlive( short keepalive )   {   m_iKeepAlive = keepalive;   }

    int getPort() const;
    const AutoStr2& getPortStr() const;

    const AutoStr2* getLocalAddrStr() const;

    const AutoStr2* getRealPath() const {   return m_pRealPath;         }

    int  getLocationOff() const         {   return m_iLocationOff;      }
    int  getLocationLen() const         {   return m_iLocationLen;      }
    int  setLocation( const char * pLoc, int len );
    const char * getLocation() const
    {  return m_reqBuf.getPointer( m_iLocationOff ); }
    void clearLocation()
    {   m_iLocationOff = m_iLocationLen = 0;    }
    
    int  addWWWAuthHeader( HttpRespHeaders &buf ) const;
    const AuthRequired* getAuthRequired() const {   return m_pAuthRequired; }
    
    //void matchContext();
    void updateKeepAlive();
    struct stat& getFileStat()              {   return m_fileStat;      }
    const struct stat& getFileStat() const  {   return m_fileStat;      }

    HttpRange * getRange() const            {   return m_pRange;        }
    void setRange( HttpRange * pRange )     {   m_pRange = pRange;      }

    VMemBuf * getBodyBuf() const            {   return m_pReqBodyBuf;   }
    int prepareReqBodyBuf();

    char gzipAcceptable() const             {   return m_iAcceptGzip;   }
    void setGzip( char b)                   {   m_iAcceptGzip = b;      }
    void andGzip( char b)                   {   m_iAcceptGzip &= b;     }

    char noRespBody() const                 {   return m_iNoRespBody;   }
    void setNoRespBody()                    {   m_iNoRespBody = 1;      }
    void updateNoRespBodyByStatus( int code )
    {
        switch( m_code = code ) { case SC_100: case SC_101: case SC_204:
                           case SC_205: case SC_304: m_iNoRespBody = 1; }
    }

    
    void processReqBodyInReqHeaderBuf();
    void resetHeaderBuf();
    int  pendingHeaderDataLen() const
    {   return m_headerBuf.size() - m_iReqHeaderBufFinished;  }
    
    void rewindPendingHeaderData( int len )
    {   m_iReqHeaderBufFinished -= len; }
    int  appendPendingHeaderData( const char * pBuf, int len );
    void compactHeaderBuf()
    {   m_headerBuf.resize( m_iHttpHeaderEnd );
        m_iReqHeaderBufFinished = m_iHttpHeaderEnd;
    }
    
    int getCurPos() const       {   return  m_iReqHeaderBufFinished;    }
    void setHeaderEnd() {   m_iHttpHeaderEnd = m_iReqHeaderBufFinished; }
    void pendingDataProcessed( int len ){m_iReqHeaderBufFinished += len;}

    void setStatusCode( int code )      {   m_code = code;              }
    int  getStatusCode() const          {   return m_code;              }
    StaticFileData* getStaticFileData() {   return &m_dataStaticFile;   }
    void tranEncodeToContentLen();

    const AutoStr2 * getDocRoot() const;
    const ExpiresCtrl * shouldAddExpires();
    void dumpHeader();
    int  getUGidChroot( uid_t *pUid, gid_t *pGid,
                    const AutoStr2 **pChroot );
    const AutoStr2 * getDefaultCharset() const;
    const MIMESetting * getMimeType() const {   return m_pMimeType;     }
    void  smartKeepAlive( const char * pValue );
    //int setRewriteURI( const char * pURL, int len );
    int setRewriteURI( const char * pURL, int len, int no_escape = 1 );
    int setRewriteQueryString(  const char * pQS, int len );
    int setRewriteLocation( char * pURI, int uriLen,
                            const char * pQS, int qsLen, int escape );
    void setForcedType( const char * pType ) {   m_pForcedType = pType;     }
    const char * getForcedType() const      {   return m_pForcedType;       }


    void addEnv( const char * pKey, int keyLen, const char * pValue, int valLen );
    const char * getEnv( const char * pKey, int keyLen, int &valLen );
    const char * getEnvByIndex( int idx, int &keyLen, const char * &pValue, int &valLen  );
    int  getEnvCount()
    {   return getListSize( m_envIdxOff );  }
    int  getUnknownHeaderCount()
    {   return getListSize( m_headerIdxOff );  }
    const char * getUnknownHeaderByIndex( int idx, int &keyLen,
        const char * &pValue, int &valLen  );
    char getRewriteLogLevel() const;
    void setHandler( const HttpHandler * pHandler )
    {   m_pHttpHandler = pHandler;      }

//     void setHTAContext( const HttpContext *pCtx){   m_pHTAContext = pCtx;   }
//     const HttpContext * getHTAContext() const   {   return m_pHTAContext;   }
    void setContext( const HttpContext *pCtx){   m_pContext = pCtx;   }
    const HttpContext * getContext() const   {   return m_pContext;   }    
    const HttpContext * getFMContext() const    {   return m_pFMContext;    }
    
    void incRedirects()                     {   ++m_iRedirects;             }
    short getRedirects() const              {   return m_iRedirects;        }

    void orContextState( short s )          {   m_iContextState |= s;       }
    void clearContextState( short s )       {   m_iContextState &= ~s;      }
    short getContextState( short s ) const  {   return m_iContextState & s; }
    int detectLoopRedirect(const char * pURI, int uriLen,
                const char * pArg, int qsLen, int isSSL );
    int detectLoopRedirect();
    int saveCurURL();
    const char * getOrgURI()
    {
        return m_reqBuf.getPointer( ( m_iRedirects )?m_urls[0].keyOff : m_curURL.keyOff);
    }
    int  getOrgURILen()
    {
        return ( m_iRedirects )? m_urls[0].keyLen : m_curURL.keyLen;
    }
    StaticFileCacheData * & setStaticFileCache();
    void appendHeaderIndexes( IOVec * pIOV, int cntUnknown );
    void getAAAData( struct AAAData &aaa, int &satisfyAny );

    long getTotalLen() const    {   return m_lEntityFinished + getHttpHeaderLen();  }

    const AutoBuf * getExtraHeaders() const;
    
    int GetReqMethod(const char *pCur, const char *pBEnd );
    int GetReqHost(const char* pCur, const char *pBEnd);
    int GetReqURL(const char *pCur, const char *pBEnd);
    int GetReqVersion(const char *pCur, const char *pBEnd);
    int RemoveSpace(const char **pCur, const char *pBEnd);
    int GetReqURI(const char *pCur, const char *pBEnd );
    const char* SkipSpace(const char *pOrg, const char *pDest);
    int ProcessHeader(int index);
    int HostProcess(const char* pCur, const char *pBEnd);
    int SkipSpaceBothSide(const char* &pHBegin, const char* &pHEnd);
    char isGeoIpOn() const;
    int getDecodedOrgReqURI( char * &pValue );
    SSIRuntime * getSSIRuntime() const  {   return m_pSSIRuntime;   }
    void setSSIRuntime( SSIRuntime * p )    {   m_pSSIRuntime = p;  }
    SSIConfig * getSSIConfig();
    int isXbitHackFull() const;
    int isIncludesNoExec() const;
    long getLastMod() const
    {   return m_fileStat.st_mtime;    }    
    void backupPathInfo();
    void restorePathInfo(); 
    void setRealPath( const char * pRealPath, int len )
    {
        m_sRealPathStore.setStr(pRealPath, len );
        m_pRealPath = &m_sRealPathStore;
    }  
    int   getOrgReqURILen()
    {
        if ( ( m_iRedirects )?m_urls[0].valOff : m_curURL.valOff )
            return m_reqURLLen - 1 - (( m_iRedirects )?m_urls[0].valLen : m_curURL.valLen);
        else
            return m_reqURLLen;
    }  
    int isMatched() const
    {
        return m_iMatchedLen ;
    }   
    void stripRewriteBase( const HttpContext * pCtx, 
            const char * &m_pSourceURL, int &m_sourceURLLen );   
    
    int locationToUrl( const char * pLocation, int len );
    
    int internalRedirectURI( const char * pURI, int len, int resetPathInfo = 1, int no_escape=1);
//     void setErrorPage( )                {   m_iContextState |= IS_ERROR_PAGE;       }
//     int  redirect( const char * pURL, int len, int alloc = 0 );
//     int internalRedirect( const char * pURL, int len, int alloc );

    int getETagFlags() const;
    int checkScriptPermission();
    
    
};


#endif
