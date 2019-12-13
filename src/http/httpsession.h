/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
#ifndef HTTPSESSION_H
#define HTTPSESSION_H

#include <ls.h>
#include <lsr/ls_atomic.h>
#include <edio/aioeventhandler.h>
#include <edio/aiooutputstream.h>
#include <http/httpreq.h>
#include <http/httpresp.h>

#include <http/httpsessionmt.h>
//#include <http/ntwkiolink.h>
#include <edio/eventreactor.h>
#include <http/hiostream.h>
#include <sslpp/sslconnection.h>
#include <lsiapi/lsiapihooks.h>

#include <http/sendfileinfo.h>
#include <lsiapi/internal.h>
#include <lsiapi/lsimoduledata.h>

class ReqHandler;
class VHostMap;
class ChunkInputStream;
class ChunkOutputStream;
class ExtWorker;
class VMemBuf;
class GzipBuf;
class SsiBlock;
class SsiRuntime;
class SsiScript;
class SsiStack;
class LsiApiHooks;
class Aiosfcb;
class ReqParser;
class CallbackLinkedObj;
class MtSessData;
class MtParamUriQs;
class MtParamSendfile;
class MtParamParseReqArgs;
class MtLocalBufQ;


enum  HttpSessionState
{
    HSS_FREE,
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
    HSS_IO_ERROR,
    HSS_COMPLETE,
    HSS_RECYCLING,
    HSS_TOBERECYCLED
};

enum HSPState
{
    HSPS_START,
    HSPS_READ_REQ_HEADER,
    HSPS_NEW_REQ,
    HSPS_MATCH_VHOST,
    HSPS_HKPT_HTTP_BEGIN,
    HSPS_HKPT_RCVD_REQ_HEADER,
    HSPS_PROCESS_NEW_REQ_BODY,
    HSPS_READ_REQ_BODY,
    HSPS_HKPT_RCVD_REQ_BODY,
    HSPS_PROCESS_NEW_URI,
    HSPS_VHOST_REWRITE,
    HSPS_CONTEXT_MAP,
    HSPS_CONTEXT_REWRITE,
    HSPS_HKPT_URI_MAP,
    HSPS_FILE_MAP,
    HSPS_CHECK_AUTH_ACCESS,
    HSPS_AUTHORIZER,
    HSPS_HKPT_HTTP_AUTH,
    HSPS_AUTH_DONE,
    HSPS_BEGIN_HANDLER_PROCESS,
    HSPS_HKPT_RCVD_REQ_BODY_PROCESSING,
    HSPS_HKPT_RCVD_RESP_HEADER,
    HSPS_RCVD_RESP_HEADER_DONE,
    HSPS_HKPT_RCVD_RESP_BODY,
    HSPS_RCVD_RESP_BODY_DONE,
    HSPS_HKPT_SEND_RESP_HEADER,
    HSPS_SEND_RESP_HEADER_DONE,
    HSPS_HKPT_HANDLER_RESTART,
    HSPS_HANDLER_RESTART_CANCEL_MT,
    HSPS_HANDLER_RESTART_DONE,
    HSPS_HKPT_HTTP_END,
    HSPS_HTTP_END_DONE,
    HSPS_WAIT_MT_CANCEL,
    HSPS_REDIRECT,
    HSPS_EXEC_EXT_CMD,
    HSPS_NEXT_REQUEST,
    HSPS_CLOSE_SESSION,
    HSPS_RELEASE_RESOURCE,
    HSPS_HANDLER_PRE_PROCESSING,
    HSPS_HANDLER_PROCESSING,
    HSPS_WEBSOCKET,
    HSPS_DROP_CONNECTION,
    HSPS_HTTP_ERROR,
    HSPS_END

};

#define HSF_URI_PROCESSED           (1<<0)
#define HSF_HANDLER_DONE            (1<<1)
#define HSF_RESP_HEADER_SENT        (1<<2)
#define HSF_HANDLER_WRITE_SUSPENDED (1<<3)
#define HSF_RESP_FLUSHED            (1<<4)
#define HSF_REQ_BODY_DONE           (1<<5)

#define HSF_AIO_READING             (1<<6)
#define HSF_ACCESS_LOG_OFF          (1<<7)
#define HSF_NO_ERROR_PAGE           (1<<8)
#define HSF_EXEC_EXT_CMD            (1<<9)
#define HSF_EXEC_POPEN              (1<<10)
#define HSF_HOOK_SESSION_STARTED    (1<<11)
#define HSF_NO_ABORT                (1<<12)

#define HSF_RECV_RESP_BUFFERED      (1<<13)
#define HSF_SEND_RESP_BUFFERED      (1<<14)

#define HSF_RESP_BODY_GZIPCOMPRESSED    (1<<15)
#define HSF_RESP_BODY_BRCOMPRESSED    (1<<16)


#define HSF_REQ_WAIT_FULL_BODY      (1<<17)
#define HSF_RESP_WAIT_FULL_BODY     (1<<18)
#define HSF_RESP_HEADER_DONE        (1<<19)
#define HSF_SUSPENDED               (1<<20)

#define HSF_RESUME_SSI              (1<<21)

#define HSF_CHUNK_CLOSED            (1<<23)

#define HSF_SUB_SESSION             (1<<24)
#define HSF_SC_404                  (1<<25)

#define HSF_CAPTURE_RESP_BODY       (1<<26)
#define HSF_WAIT_SUBSESSION         (1<<27)
#define HSF_CUR_SUB_SESSION_DONE    (1<<28)
#define HSF_STX_FILE_CACHE_READY    (1<<29)
#define HSF_URI_MAPPED              (1<<30)
#define HSF_SAVE_STX_FILE_CACHE     (1<<31)


typedef int (*SubSessionCb)(HttpSession *pSubSession, void *param,
                            int flag);

#define HSF_MT_HANDLER              (1<<0)

#define HS_RESP_NOT_READY           ((size_t)-1ll)

class HttpSession
    : public LsiSession
    , public InputStream
    , public HioHandler
    , public AioEventHandler
    , public ls_lfnodei_t
    , public LogSession
{
    HttpReq               m_request;
    HttpResp              m_response;

    LsiModuleData         m_moduleData; //lsiapi user data of http level
    const lsi_reqhdlr_t  *m_pModHandler;
    MtSessData           *m_pMtSessData;

    HttpSessionHooks      m_sessionHooks;
    HSPState              m_processState;
    int                   m_suspendPasscode;
    int                   m_curHookLevel;

    ChunkInputStream     *m_pChunkIS;
    ChunkOutputStream    *m_pChunkOS;
    HttpSession          *m_pCurSubSession;
    ReqHandler           *m_pHandler;

    lsi_hookinfo_t        m_curHookInfo;
    lsi_param_t           m_curHkptParam;
    int                   m_curHookRet;

    SubSessionCb         *m_pSubSessionCb;
    void                 *m_pSubSessionCbParam;
    HttpSession          *m_pParent;

    SsiStack             *m_pSsiStack;
    SsiRuntime           *m_pSsiRuntime;

    off_t                 m_lDynBodySent;

    ClientInfo           *m_pClientInfo;

    const AccessControl * m_pVHostAcl;
    int                   m_iVHostAccess;

    uint16_t              m_iRemotePort;
    uint16_t              m_iSubReqSeq;

    SendFileInfo          m_sendFileInfo;

    long                  m_lReqTime;
    int32_t               m_iReqTimeUs;

    uint32_t              m_iFlag;
    short                 m_iState;
    unsigned short        m_iReqServed;
    //int                   m_accessGranted;
    uint32_t              m_iMtFlag;
    ls_spinlock_t         m_lockMtRace;
    pthread_t             m_lockMtHolder;

    AioReq                m_aioReq;
    Aiosfcb              *m_pAiosfcb;

    uint32_t              m_sn;
    ReqParser            *m_pReqParser;

    AutoBuf               m_sExtCmdResBuf;
    evtcb_pf              m_cbExtCmd;
    long                  m_lExtCmdParam;
    void                 *m_pExtCmdParam;
    ls_atom_u32_t         m_sessSeq;

    static ls_atom_u32_t  s_m_sessSeq; // monotonically increasing sequence number across
                                      // all sessions (ok to wrap-around) for hashing into
                                      // queue+lock in eventcallback code


    HttpSession(const HttpSession &rhs);
    void operator=(const HttpSession &rhs);

    virtual const char *buildLogId();

    void processPending(int ret);

    void parseHost(char *pHost);

    int  buildErrorResponse(const char *errMsg);

    int  doRedirect();
    void postWriteEx();
    int  cleanUpHandler(HSPState nextState);
    void nextRequest();
    int  updateClientInfoFromProxyHeader(const char *pHeaderName,
                                         const char *pProxyHeader,
                                         int headerLen);

    static int readReqBodyTermination(LsiSession *pSession, char *pBuf,
                                      int size);
    static int call_onRead(lsi_session_t *p, long , void *);

    char getSsiStackDepth() const;

    static int call_nextRequest(lsi_session_t *p, long , void *);
    void markComplete(bool nowait);

    void releaseResources();
    void releaseReqParser();

    int broadcastMtWaiters(int32_t flags);
    int processMtEvent(long lParam, void *pParam);
    int processMtEvents(long lParam, void *pParam);
    int processMtRecycle();
    int processMtCancel();
    void mtParseReqArgs(MtParamParseReqArgs *pParam);
    void mtSendfile(MtParamSendfile *pParam);
    int mtFlushLocalRespBodyBuf(void * pParam);
    int mtFlushLocalRespBodyBufQueue();
    int mtFlushLocalRespBodyBufQueue(MtLocalBufQ *q);
    int mtFlushLocalBuf();

public:
    void reset() {
        resetEvtcb();
        memset(&m_pChunkIS, 0, 
               (char *)(&m_iReqServed + 1) - (char *)&m_pChunkIS);
    }

    void cleanUpSubSessions();
    
    int removeSessionCbs(long lParam, void * pParam);

    uint32_t getSn()    { return ls_atomic_fetch_add(&m_sn, 0);}
    uint32_t getSessSeq() const   { return ls_atomic_fetch_add((volatile ls_atom_u32_t *)&m_sessSeq, 0);}

    void runAllCallbacks();

    void closeSession();
    void recycle();

    int isHookDisabled(int level) const
    {   return m_sessionHooks.isDisabled(level);  }

    uint32_t testAndSetFlag(uint32_t f)
    {
        uint32_t flag;
        flag = ls_atomic_fetch_or(&m_iFlag, f);
        return ((flag & f) == 0);
    }

    uint32_t testFlag(uint32_t f) const
    {
        uint32_t flag;
        flag = ls_atomic_add_fetch((volatile uint32_t*)&m_iFlag, 0);
        return flag & f;
    }

    void setFlag(uint32_t f, int v)
    {
        if (v)
            ls_atomic_fetch_or(&m_iFlag, f);
        else
            ls_atomic_fetch_and(&m_iFlag, ~f);
    }


    void setFlag(uint32_t f)            {   setFlag(f, 1);          }
    void clearFlag(uint32_t f)          {   setFlag(f, 0);          }

    uint32_t getFlag(uint32_t f) const  {   return testFlag(f);     }


    int isRespHeaderSent() const
    {   return getFlag(HSF_RESP_HEADER_SENT);   }

    ReqParser  *getReqParser()
    {   return ls_atomic_add_fetch(&m_pReqParser,0);  }

    void setReqParser(ReqParser *reqParser)
    {   (void)ls_atomic_setptr(&m_pReqParser, reqParser);             }

    AutoBuf &getExtCmdBuf()     { return m_sExtCmdResBuf;       }

    void setExtCmdNotifier(evtcb_pf cb, const long lParam,
                           void *pParam)
    {
        m_cbExtCmd = cb;
        m_lExtCmdParam = lParam;
        m_pExtCmdParam = pParam;
    }

    void extCmdDone();

    void addBittoCookie(AutoStr2 &cookie, int bit);
    int isCookieHaveBit(const char *cookies, int bit);
    
    int pushToClient(const char *pUri, int uriLen, AutoStr2 &cookie);
    void processLinkHeader(const char* pValue, int valLen, AutoStr2 &cookie);
    
    static int hookResumeCallback(lsi_session_t *session, long lParam, void *);
    static int removeSessionCbsEvent(lsi_session_t *session, long lParam,
                                void *pParam);
    static int mtNotifyCallbackEvent(lsi_session_t *session, long lParam,
            void *pParam);
    static int mtNotifyCallback(lsi_session_t *session, long lParam,
                                  void *pParam);

    int setUriQueryString(MtParamUriQs * param);
    int setUriQueryString(int action, const char *uri,
                          int uri_len, const char *qs, int qs_len);

private:
    int runExtAuthorizer(const HttpHandler *pHandler);
    int assignHandler(const HttpHandler *pHandler);
    int readReqBody();
    int reqBodyDone();
    int processURI(const char *pURI);
    int readToHeaderBuf();
    int sendHttpError(const char *pAdditional);
    int sendDefaultErrorPage(const char *pAdditional);

    int detectTimeout();

    //int cacheWrite( const char * pBuf, int size );
    //int writeRespBuf();

    void releaseChunkOS();
    void releaseRequestResource();

    void setupChunkIS();
    void releaseChunkIS();
    int  doWrite();
    int  doRead();
    int processURI(int resume);
    int checkAuthentication(const HTAuth *pHTAuth,
                            const AuthRequired *pRequired, int resume);

    void logAccess(int cancelled);
    void incReqProcessed();
    void setHandler(ReqHandler *pHandler);
    int  detectKeepAliveTimeout(int delta);
    int  detectConnectionTimeout(int delta);
    void resumeSSI();
    int sendStaticFile(SendFileInfo *pData);
    int sendStaticFileEx(SendFileInfo *pData);
#ifdef LS_AIO_USE_AIO
    int aioRead(SendFileInfo *pData, void *pBuf = NULL);
    int sendStaticFileAio(SendFileInfo *pData);
#endif
    int writeRespBodyBlockInternal(SendFileInfo *pData, const char *pBuf,
                                   int written);
    int writeRespBodyBlockFilterInternal(SendFileInfo *pData, const char *pBuf,
                                         int written, lsi_param_t *param = NULL);
    int chunkSendfile(int fdSrc, off_t off, off_t size);
    int processWebSocketUpgrade(HttpVHost *pVHost);
    int processHttp2Upgrade(const HttpVHost *pVHost);

    //int resumeHandlerProcess();
    int flushBody();
    int endResponseInternal(int success);

    int getModuleDenyCode(int iHookLevel);
    int processHkptResult(int iHookLevel, int ret);

    int detachSubSession(HttpSession *pSubSess);
    int  passSendFileToParent(SendFileInfo *pData);
    int  setSendFile(SendFileInfo *pData);
    int detectLoopSubSession(lsi_subreq_t *pSubSessInfo);
    int processSubReq();
    int processSubSessionResponse(HttpSession *pSubSess);
    int includeSubSession();
    int curSubSessionCleanUp();

    int processOneLink(const char* p, const char* pEnd, AutoStr2 &cookie);
    int restartHandlerProcess();
    int restartHandlerProcessEx();
    int runFilter(int hookLevel, filter_term_fn pfTerm,
                  const char *pBuf,
                  int len, int flagIn);
    int contentEncodingFixup();
    int processVHostRewrite();
    int runEventHkpt(int hookLevel, HSPState nextState);
    int processNewReqInit();
    int processNewReqBody();
    int smProcessReq();
    void setRecaptchaEnvs();
    int processContextMap();
    int processContextRewrite();
    int processContextAuth();
    int processAuthorizer();
    int preUriMap();
    int processFileMap();
    int processNewUri();

    void resetEvtcb();
    void processServerPush();

    int execExtCmdEx();
    int processUnpackedHeaders();

public:
    int  flush();

    int16_t isHttps() const           {   return m_request.isHttps(); }
    HioCrypto *getCrypto() const    {   return m_request.getCrypto();  }
    

    const char *getPeerAddrString() const;
    int getPeerAddrStrLen() const;
    const struct sockaddr *getPeerAddr() const;
    bool shouldIncludePeerAddr() const;

    void suspendRead()          {    getStream()->suspendRead();        };
    void continueRead()         {    getStream()->continueRead();       };
    void suspendWrite()         {    getStream()->suspendWrite();       };
    void continueWrite()        {    getStream()->continueWrite();      };
    void wantRead(int want)     {   getStream()->wantRead(want);      }
    void wantWrite(int want)    {   getStream()->wantWrite(want);     }
    void switchWriteToRead()    {    getStream()->switchWriteToRead();  };

    void suspendEventNotify()   {    getStream()->suspendEventNotify(); };
    void resumeEventNotify()    {    getStream()->resumeEventNotify();  };

    off_t getBytesRecv() const  {   return getStream()->getBytesRecv();    }
    off_t getBytesSent() const  {   return getStream()->getBytesSent();    }

    void setClientInfo(ClientInfo *p)       {   m_pClientInfo = p;                  };
    ClientInfo *getClientInfo() const       {   return m_pClientInfo;               };
    int getVHostAccess();

    HttpSessionState getState() const       {   return (HttpSessionState)ls_atomic_fetch_or(const_cast<short *>(&m_iState), 0);  };
    void setState(HttpSessionState state)   {   ls_atomic_setshort(&m_iState, (short)state);            };
    int getServerAddrStr(char *pBuf, int len);
    int isAlive();
    int setUpdateStaticFileCache(const char *pPath, int pathLen,
                                 int fd, struct stat &st);

    int isEndResponse() const               { return testFlag(HSF_HANDLER_DONE);     };

    int resumeProcess(int resumeState, int retcode);

public:
    void setupChunkOS(int nobuffer);

    HttpSession();
    ~HttpSession();


    unsigned short getRemotePort() const
    {   return m_iRemotePort;   };

    void setVHostMap(const VHostMap *pMap)
    {
        m_iReqServed = 0;
        m_request.setVHostMap(pMap);
    }

    HttpReq *getReq()               {   return &m_request;  }
    const HttpReq *getReq() const   {   return &m_request;  }
//     HttpResp* getResp()
//     {   return &m_response; }

    long getReqTime() const {   return m_lReqTime;  }
    int32_t getReqTimeUs() const    {   return m_iReqTimeUs;    }

    int writeRespBodyDirect(const char *pBuf, int size);
    int writeRespBody(const char *pBuf, int len);

    int isNoRespBody() const
    {   return m_request.noRespBody();  }


    int onReadEx();
    int onWriteEx();
    int onInitConnected();
    int onCloseEx();

    int redirect(const char *pNewURL, int len, int alloc = 0);
    int redirectEx();

    int getHandler(const char *pURI, ReqHandler *&pHandler);
    //int setLocation( const char * pLoc );

    //int startForward( int fd, int type );
    bool endOfReqBody();
    void setWaitFullReqBody()
    {    setFlag(HSF_REQ_WAIT_FULL_BODY);    }

    int parseReqArgs(int doPostBody, int uploadPassByPath,
                     const char *uploadTmpDir, int uploadTmpFilePermission);

    int  onTimerEx();

    //void accessGranted()    {   m_accessGranted = 1;  }
    void changeHandler() {    setState(HSS_REDIRECT); };

    //const char * buildLogId();

    int httpError(int code, const char *pAdditional = NULL);
    int read(char *pBuf, int size);
    int readv(struct iovec *vector, size_t count);
    ReqHandler *getCurHandler() const  {   return m_pHandler;  }


    //void resumeAuthentication();
    void authorized();

    void addEnv(const char *pKey, int keyLen, const char *pValue, long valLen);

    off_t writeRespBodySendFile(int fdFile, off_t offset, off_t size);
    void rewindRespBodyBuf();
    int setupRespBodyBuf();
    void releaseRespBody();
    int sendDynBody();

    int appendDynBody(VMemBuf *pvBuf, int offset, int len)
    {
        int ret = getResp()->appendDynBody(pvBuf, offset, len);
        if (ret > 0)
            setFlag(HSF_RESP_FLUSHED, 0);
        return ret;
    }
    int appendDynBody(const char *pBuf, int len);
    int appendRespBodyBuf(const char *pBuf, int len);
    int appendRespBodyBufV(const iovec *vector, int count);

    int shouldSuspendReadingResp();
    void resetRespBodyBuf();
    int checkRespSize(int nobuffer);
    int checkRespSize();

    int createOverBodyLimitErrorPage();
    int respHeaderDone();

    void setRespBodyDone()
    {
        setFlag(HSF_HANDLER_DONE);
        if (m_pChunkOS)
        {
            setFlag(HSF_RESP_FLUSHED, 0);
            flush();
        }
    }

    int endResponse(int success);
    int detectSsiStackLoop(const SsiBlock *m_pBlock);

    int setupDynRespBodyBuf();

    VMemBuf *getRespBodyBuf() const  {   return getResp()->getRespBodyBuf(); }
    void setRespBodyBuf(VMemBuf *pBuf)   {   getResp()->setRespBodyBuf(pBuf);  }
    off_t getDynBodySent() const    {   return m_lDynBodySent; }
    //int flushDynBody( int nobuff );

    int useGzip();
    int setupGzipFilter();
    int setupGzipBuf();
    void releaseGzipBuf();
    GzipBuf *getGzipBuf() const     {   return getResp()->getGzipBuf();     }
    void setGzipBuf(GzipBuf *pGzip) {   getResp()->setGzipBuf(pGzip);       }

    int execExtCmd(const char *pCmd, int len, int mode = HSF_EXEC_EXT_CMD);

    int handlerProcess(const HttpHandler *pHandler);
    int getParsedScript(SsiScript *&pScript);
    int startServerParsed();

    int isExtAppNoAbort();

    SsiRuntime *getSsiRuntime() const       {   return m_pSsiRuntime;       }
    void setSsiRuntime(SsiRuntime *p)       {   m_pSsiRuntime = p;          }
    void releaseSsiRuntime();
    int setupSsiRuntime();

    int isDropConnection() const
    {   return m_processState == HSPS_DROP_CONNECTION;  }

    const HttpResp *getResp() const {   return &m_response;     }
    HttpResp *getResp()             {   return &m_response;     }
    int flushDynBodyChunk();
    //int writeConnStatus( char * pBuf, int bufLen );

    void resetResp()
    {   getResp()->reset();
        m_iFlag &= ~HSF_RESP_HEADER_DONE; }

    LogSession *getLogSession()     {   return this;     }

    SendFileInfo *getSendFileInfo() {   return &m_sendFileInfo;   }

    int openStaticFile(const char *pPath, int pathLen, int *status);

    int detectContentLenMismatch(int buffered);

    /**
     * @brief initSendFileInfo() should be called before start sending a static file
     *
     * @param pPath[in] an file path to a static file
     * @param pathLen[in] the lenght of the path
     * @return 0 if successful, HTTP status code in SC_xxx predefined macro.
     *
     **/
    int initSendFileInfo(const char *pPath, int pathLen);

    /**
     * @brief setSendFileOffset() set the start point and length of the file to be send
     *
     * @param start[in] file offset from which will start reading data  from
     * @param size[in]  the number of bytes to be sent
     *
     **/

    void setSendFileOffsetSize(off_t start, off_t size);

    void setSendFileOffsetSize(int fd, off_t start, off_t size);


    int finalizeHeader(int ver, int code);
    LsiModuleData *getModuleData()      {   return &m_moduleData;   }

    HttpSessionHooks *getSessionHooks() { return &m_sessionHooks;   }

    void setSendFileBeginEnd(off_t start, off_t end);
    void prepareHeaders();
    int sendRespHeaders();
    void addLocationHeader();

    void setAccessLogOff()      {   setFlag(HSF_ACCESS_LOG_OFF);    }
    int shouldLogAccess() const {   return !getFlag(HSF_ACCESS_LOG_OFF);    }

    void prepareSsiStack(const SsiScript *pScript);
    SsiStack *getSsiStack() const       {    return m_pSsiStack;     }
    SsiStack *popSsiStack();

    //subrequest routines.

    HttpSession *newSubSession(lsi_subreq_t *pSubSessInfo);

    int setReqBody(const char *pBodyBuf, int len);

    int attachSubSession(HttpSession *pSubSess);

    int execSubSession();

    int onSubSessionRespIncluded(HttpSession *pSubSess);

    int onSubSessionEndResp(HttpSession *pSubSess);

    int cancelSubSession(HttpSession *pSubSess);

    int closeSubSession(HttpSession *pSubSess);

    HttpSession *getParent() const      {   return m_pParent;   }

    void mergeRespHeaders(HttpRespHeaders::INDEX headerIndex,
                const char *pName, int nameLen, struct iovec *pIov, int count);

    const char *getOrgReqUrl(int *n);

    void testContentType();
    int updateContentCompressible();
    int suspendProcess();

    virtual int onAioEvent();
    int handleAioSFEvent(Aiosfcb *event);

    void setModHandler(const lsi_reqhdlr_t *pHandler)
    {   (void)ls_atomic_setptr(&m_pModHandler, pHandler);   }

    const lsi_reqhdlr_t *getModHandler()
    {   return ls_atomic_fetch_add(&m_pModHandler,0);}

    void setBackRefPtr(evtcbtail_t ** v);
    void resetBackRefPtr();
    void cancelEvent(evtcbnode_s * v);

    size_t getRespBodyBuffered();

    int testAndSetMtFlag(int32_t f)
    {
        uint32_t flag;
        flag = ls_atomic_fetch_or(&m_iMtFlag, f);
        return ((flag & f) == 0);
    }

    int32_t testMtFlag(int32_t f) const
    {
        uint32_t flag;
        flag = ls_atomic_add_fetch((volatile uint32_t *)&m_iMtFlag, 0);
        return flag & f;
    }

    void setMtFlag(int32_t f, int v)
    {
        if (v)
            ls_atomic_fetch_or(&m_iMtFlag, f);
        else
            ls_atomic_fetch_and(&m_iMtFlag, ~f);
    }


    void setMtFlag(int32_t f)               {   setMtFlag(f, 1);                            }
    void clearMtFlag(int32_t f)             {   setMtFlag(f, 0);                            }

    int32_t getMtFlag(int32_t f) const      {   return testMtFlag(f);                       }

    int  beginMtHandler(const lsi_reqhdlr_t *pHandler);
    void releaseMtSessData();
    
    int cancelMtHandler();
    int isMtHandlerCancelled() const        {   return testMtFlag(HSF_MT_CANCEL);           }

    void mtNotifyWriters();

    void setMtNotifiers(MtSessData *pNotifiers)
    {   (void)ls_atomic_setptr(&m_pMtSessData, pNotifiers);  }
    MtSessData *getMtSessData() const
    {   return m_pMtSessData;  }

    void lockMtRace()
    {
        ls_spinlock_lock(&m_lockMtRace);
        m_lockMtHolder = pthread_self();
    }
    int trylockMtRace()
    {
        int ret = ls_spinlock_trylock(&m_lockMtRace);
        if (0 == ret)
        {
            m_lockMtHolder = pthread_self();
        }
        return ret;
    }
    void unlockMtRace()
    {
        m_lockMtHolder = 0;
        ls_spinlock_unlock(&m_lockMtRace);
    }
    bool isRecaptchaEnabled() const
    {   return (m_request.getRecaptcha() != NULL);      }
    bool shouldAvoidRecaptcha();
    bool isUseRecaptcha(int isEarlyDetect = 0);
    bool hasPendingCaptcha() const;
    bool recaptchaAttemptsAvail() const;
    int rewriteToRecaptcha(bool blockIfTooManyAttempts = false);
    void checkSuccessfulRecaptcha();
    void blockParallelRecaptcha();
};

#endif
