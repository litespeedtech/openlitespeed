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
#include <http/httpsession.h>
#include <http/httplistenerlist.h>
#include <lsdef.h>
#include <edio/aiosendfile.h>
#include <edio/evtcbque.h>
#include <http/chunkinputstream.h>
#include <http/chunkoutputstream.h>
#include <http/clientcache.h>
#include <http/connlimitctrl.h>
#include <http/handlerfactory.h>
#include <http/handlertype.h>
#include <http/hiochainstream.h>
#include <http/htauth.h>
#include <http/httphandler.h>
#include <http/httplog.h>
#include <http/httpmethod.h>
#include <http/httpmime.h>
#include <http/httpresourcemanager.h>
#include <http/httpserverconfig.h>
#include <http/httpstats.h>
#include <http/httpstatuscode.h>
#include <http/httpver.h>
#include <http/httpvhost.h>
#include <http/l4handler.h>
#include <http/mtsessdata.h>
#include <http/recaptcha.h>
#include <http/reqhandler.h>
#include <http/rewriteengine.h>
#include <http/serverprocessconfig.h>
#include <http/smartsettings.h>
#include <http/staticfilecache.h>
#include <http/staticfilecachedata.h>
#include <http/userdir.h>
#include <http/vhostmap.h>
#include <http/clientinfo.h>
#include <http/hiohandlerfactory.h>
#include "reqparser.h"
#include <log4cxx/logger.h>
#include <lsiapi/envmanager.h>
#include <lsiapi/lsiapi.h>
#include <lsiapi/lsiapihooks.h>
#include <lsiapi/modulemanager.h>
#include <lsr/ls_pool.h>
#include <lsr/ls_strtool.h>
#include <lsr/ls_threadcheck.h>
#include <socket/gsockaddr.h>
#include <ssi/ssiengine.h>
#include <ssi/ssiruntime.h>
#include <ssi/ssiscript.h>
#include <thread/mtnotifier.h>
#include <util/accesscontrol.h>
#include <util/accessdef.h>
#include <util/datetime.h>
#include <util/gzipbuf.h>
#include <util/httputil.h>
#include <util/vmembuf.h>
#include <util/blockbuf.h>
#include <extensions/extworker.h>
#include <extensions/cgi/lscgiddef.h>
#include <extensions/registry/extappregistry.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVERPUSHTAG           "ls_smartpush"
#define SERVERPUSHTAGLENGTH     (sizeof(SERVERPUSHTAG) - 1)


static const char * s_stateName[HSPS_END] =
{
    "HSPS_START",
    "HSPS_READ_REQ_HEADER",
    "HSPS_NEW_REQ",
    "HSPS_MATCH_VHOST",
    "HSPS_HKPT_HTTP_BEGIN",
    "HSPS_HKPT_RCVD_REQ_HEADER",
    "HSPS_PROCESS_NEW_REQ_BODY",
    "HSPS_READ_REQ_BODY",
    "HSPS_HKPT_RCVD_REQ_BODY",
    "HSPS_PROCESS_NEW_URI",
    "HSPS_VHOST_REWRITE",
    "HSPS_CONTEXT_MAP",
    "HSPS_CONTEXT_REWRITE",
    "HSPS_HKPT_URI_MAP",
    "HSPS_FILE_MAP",
    "HSPS_CHECK_AUTH_ACCESS",
    "HSPS_AUTHORIZER",
    "HSPS_HKPT_HTTP_AUTH",
    "HSPS_AUTH_DONE",  //21
    "HSPS_BEGIN_HANDLER_PROCESS",
    "HSPS_HKPT_RCVD_REQ_BODY_PROCESSING",
    "HSPS_HKPT_RCVD_RESP_HEADER",
    "HSPS_RCVD_RESP_HEADER_DONE",
    "HSPS_HKPT_RCVD_RESP_BODY",
    "HSPS_RCVD_RESP_BODY_DONE",
    "HSPS_HKPT_SEND_RESP_HEADER",
    "HSPS_SEND_RESP_HEADER_DONE",
    "HSPS_HKPT_HANDLER_RESTART",
    "HSPS_HANDLER_RESTART_CANCEL_MT",
    "HSPS_HANDLER_RESTART_DONE",
    "HSPS_HKPT_HTTP_END",
    "HSPS_HTTP_END_DONE",
    "HSPS_WAIT_MT_CANCEL",
    "HSPS_REDIRECT",
    "HSPS_EXEC_EXT_CMD",
    "HSPS_NEXT_REQUEST",
    "HSPS_CLOSE_SESSION",
    "HSPS_RELEASE_RESOURCE",
    "HSPS_HANDLER_PROCESSING",
    "HSPS_WEBSOCKET",
    "HSPS_DROP_CONNECTION",
    "HSPS_HTTP_ERROR",
};


#define setProcessState(newState) \
    do {\
    LS_DBG_M(getLogSession(), "%s(): %s -> %s", \
             __FUNCTION__, \
             s_stateName[m_processState], s_stateName[newState]); \
    m_processState = newState; \
    } while(0)


ls_atom_u32_t  HttpSession::s_m_sessSeq = 0;

HttpSession::HttpSession()
    : m_request()
    , m_response(m_request.getPool())
    , m_pModHandler(NULL)
    , m_pMtSessData(NULL)
    , m_processState(HSPS_READ_REQ_HEADER)
    , m_curHookLevel(0)
    , m_pVHostAcl(NULL)
    , m_iVHostAccess(0)
    , m_lockMtHolder(0)
    , m_pAiosfcb(NULL)
    , m_sn(1)
    , m_pReqParser(NULL)
    , m_sessSeq(ls_atomic_add_fetch(&s_m_sessSeq, 1)) // ok to overflow / wrap around
    //, m_sessSeq(0)
{
    ls_spinlock_setup(&m_lockMtRace);
    lockMtRace();
    memset(&m_pChunkIS, 0, (char *)(&m_iReqServed + 1) - (char *)&m_pChunkIS);
    m_pModuleConfig = NULL;
    m_response.reset();
    m_request.reset();
    resetEvtcb();
    LS_DBG_M("[T%d %s] sess seq now %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, ls_atomic_fetch_add(&m_sessSeq, 0));
}


HttpSession::~HttpSession()
{
    LsiapiBridge::releaseModuleData(LSI_DATA_HTTP, getModuleData());
#ifdef LS_AIO_USE_AIO
    if (m_pAiosfcb != NULL)
        HttpResourceManager::getInstance().recycle(m_pAiosfcb);
    m_pAiosfcb = NULL;
#endif
    m_sExtCmdResBuf.clear();
    unlockMtRace();
    releaseSsiRuntime();
}

const char *HttpSession::getPeerAddrString() const
{    return (m_pClientInfo ? m_pClientInfo->getAddrString() : "");  }

int HttpSession::getPeerAddrStrLen() const
{   return (m_pClientInfo ? m_pClientInfo->getAddrStrLen() : 0);   }

const struct sockaddr *HttpSession::getPeerAddr() const
{   return (m_pClientInfo ? m_pClientInfo->getAddr() : NULL); }


bool HttpSession::shouldIncludePeerAddr() const
{
    if (HttpServerConfig::getInstance().getUseProxyHeader() != 3)
        return true;

    int iProxyAddrLen;
    const char *pProxyAddr = m_request.getEnv("PROXY_REMOTE_ADDR", 17, iProxyAddrLen);

    // If behind proxy, client is trusted. Else check client info. Return NOT.
    return !((pProxyAddr != NULL)
                || (AC_TRUST == getClientInfo()->getAccess()));
}


int HttpSession::onInitConnected()
{
    const ConnInfo *pInfo = getStream()->getConnInfo();
    m_lReqTime = DateTime::s_curTime;
    m_iReqTimeUs = DateTime::s_curTimeUs;

    if (pInfo->m_pCrypto)
    {
        m_request.setCrypto(pInfo->m_pCrypto);
        m_request.setHttps();
    }
    else
        m_request.setCrypto(NULL);

    /**
     * If setConnInfo() failed, the m_pServerAddrInfo can be NULL
     */
    if(pInfo->m_pServerAddrInfo)
        setVHostMap(pInfo->m_pServerAddrInfo->getVHostMap());

    //assert(pInfo->m_pClientInfo);
    setClientInfo(pInfo->m_pClientInfo);
    m_iRemotePort = pInfo->m_remotePort;
    m_iFlag = 0;
    ls_atomic_setint(&m_iMtFlag, 0);

    m_curHookLevel = 0;
    getStream()->setFlag(HIO_FLAG_WANT_READ, 1);
    setLogger(getStream()->getLogger());
    clearLogId();
    //m_request.setILog(this);
    m_request.setILog(getStream());

    setState(HSS_WAITING);
    HttpStats::incIdleConns();

    m_processState = HSPS_START;
    setProcessState(HSPS_READ_REQ_HEADER);
    if (m_request.getBodyBuf())
        m_request.getBodyBuf()->reinit();
#ifdef LS_AIO_USE_AIO
    if (HttpServerConfig::getInstance().getUseSendfile() == 2)
        m_pAiosfcb = HttpResourceManager::getInstance().getAiosfcb();
#endif
//     m_response.reset();
//     m_request.reset();
    //++m_sn;
    ls_atomic_fetch_add(&m_sn, 1);
    // ls_atomic_setint(&m_sessSeq, ls_atomic_add_fetch(&s_m_sessSeq, 1)); // ok to overflow / wrap
    LS_DBG_L("[T%d %s] sess seq now %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, ls_atomic_fetch_add(&m_sessSeq, 0));
    resetEvtcb();
    releaseReqParser();
    m_sExtCmdResBuf.clear();
    m_cbExtCmd = NULL;
    m_lExtCmdParam = 0;
    m_pExtCmdParam = NULL;
    if (processUnpackedHeaders() == LS_FAIL)
    {
        m_processState = HSPS_READ_REQ_HEADER;
        getStream()->setFlag(HIO_FLAG_WANT_READ, 1);
    }
    return 0;
}


void HttpSession::releaseReqParser()
{
    ReqParser *pReqParser = this->getReqParser();
    if (pReqParser)
    {
        delete pReqParser;
        this->setReqParser(NULL);
    }
}


inline int HttpSession::getModuleDenyCode(int iHookLevel)
{
    int ret = m_request.getStatusCode();
    if ((!ret) || (ret == SC_200))
    {
        ret = SC_403;
        m_request.setStatusCode(SC_403);
    }
    LS_DBG_L(getLogSession(), "blocked by the %s hookpoint, return: %d!",
             LsiApiHooks::getHkptName(iHookLevel), ret);

    return ret;
}


inline int HttpSession::processHkptResult(int iHookLevel, int ret)
{
    if (((getState() == HSS_EXT_REDIRECT) || (getState() == HSS_REDIRECT))
        && (m_request.getLocation() != NULL))
    {
        //perform external redirect.
        getStream()->wantWrite(1);
        return LS_FAIL;
    }

    if (ret < 0)
    {
        getModuleDenyCode(iHookLevel);
        setState(HSS_HTTP_ERROR);
        getStream()->wantWrite(1);
    }
    return ret;
}


int HttpSession::runEventHkpt(int hookLevel, HSPState nextState)
{
    if (m_curHookLevel != hookLevel)
    {
        if (!m_sessionHooks.isEnabled(hookLevel))
        {
            setProcessState(nextState);
            return 0;
        }
        const LsiApiHooks *pHooks = LsiApiHooks::getGlobalApiHooks(hookLevel);
        m_curHookLevel = hookLevel;
        m_curHkptParam.session = (LsiSession *)this;

        m_curHookInfo.hooks = pHooks;
        m_curHookInfo.term_fn = NULL;
        m_curHookInfo.enable_array = m_sessionHooks.getEnableArray(hookLevel);
        m_curHookInfo.hook_level = hookLevel;
        m_curHkptParam.cur_hook = ((lsiapi_hook_t *)pHooks->begin());
        m_curHkptParam.hook_chain = &m_curHookInfo;
        m_curHkptParam.ptr1 = NULL ;
        m_curHkptParam.len1 = 0;
        m_curHkptParam.flag_out = 0;
        m_curHookRet = 0;
    }
    else  //resume current hookpoint
    {
        if (!m_curHookRet)
            m_curHkptParam.cur_hook = (lsiapi_hook_t *)m_curHkptParam.cur_hook + 1;
    }
    if (!m_curHookRet)
        m_curHookRet = m_curHookInfo.hooks->runCallback(hookLevel,
                       &m_curHkptParam);
    if (m_curHookRet <= -1)
    {
        if (m_curHookRet != LSI_SUSPEND)
            return getModuleDenyCode(hookLevel);
    }
    else if (!m_curHookRet)
    {
        m_curHookLevel = 0;
        setProcessState(nextState);
    }

    return m_curHookRet;
}



const char * HttpSession::buildLogId()
{
    int len ;
    char * p = m_logId.ptr;
    char * pEnd = m_logId.ptr + MAX_LOGID_LEN;
    appendLogId(getStream()->getLogId(), true);


    p += m_logId.len;
    if (m_iReqServed > 0)
    {
        len = ls_snprintf( p, pEnd - p, "-%hu", m_iReqServed );
        p += len;
    }
    const HttpVHost * pVHost = m_request.getVHost();
    if (pVHost)
    {
        ls_snprintf( p, pEnd - p, "#%s", pVHost->getName() );
    }
    return m_logId.ptr;
}


void HttpSession::logAccess(int cancelled)
{
    HttpVHost *pVHost = (HttpVHost *) m_request.getVHost();
    if (pVHost)
    {
        pVHost->decRef();

        if (pVHost->BytesLogEnabled()
            && (m_iFlag & HSF_SUB_SESSION) == 0)
        {
            long long bytes = getReq()->getTotalLen();
            bytes += getResp()->getTotalLen();
            pVHost->logBytes(bytes);
        }
        if (((!cancelled) || (isRespHeaderSent()))
            && pVHost->enableAccessLog()
            && shouldLogAccess())
            pVHost->logAccess(this);
        else
            setAccessLogOff();
    }
    else if (shouldLogAccess())
        HttpLog::logAccess(NULL, 0, this);
}


void HttpSession::incReqProcessed()
{
    if (m_iFlag & HSF_SUB_SESSION)
        return;
    HttpStats::getReqStats()->incReqProcessed();
    HttpVHost *pVHost = (HttpVHost *) m_request.getVHost();
    if (pVHost)
        pVHost->getReqStats()->incReqProcessed();
}

int HttpSession::setupSsiRuntime()
{
    if (!m_pSsiRuntime)
    {
        SsiRuntime *pRuntime = new SsiRuntime();
        if (!pRuntime)
            return -1; //internal error, 500

        pRuntime->initConfig(m_request.getSsiConfig());
        pRuntime->setMainReq(&m_request);
        setSsiRuntime(pRuntime);
    }
    return 0;
}


void HttpSession::resumeSSI()
{
//     m_request.restorePathInfo();
    m_sendFileInfo.release(); //Must called before VHost released!!!
    if ((getGzipBuf()) && (!getGzipBuf()->isStreamStarted()))
        getGzipBuf()->reinit();
    SsiEngine::resumeExecute(this);
    return;
}


void HttpSession::nextRequest()
{
    EvtcbQue::getInstance().removeSessionCb(this);

    LS_DBG_L(getLogSession(), "HttpSession::nextRequest()!");
    if (m_pHandler)
    {
        if (cleanUpHandler(HSPS_NEXT_REQUEST) == LS_AGAIN)
            return;
        incReqProcessed();
    }

    LS_DBG_M(getLogSession(), "calling removeSessionCb on this %p\n", this);
    EvtcbQue::getInstance().removeSessionCb(this);

    getStream()->flush();
    setState(HSS_WAITING);


    m_sendFileInfo.release();
    //m_sendFileInfo.reset();
    releaseReqParser();

    //for SSI, should resume
    if (getSsiRuntime())
        return releaseSsiRuntime();

    if (!m_request.isKeepAlive() || getStream()->isSpdy() || !endOfReqBody())
    {
        LS_DBG_L(getLogSession(), "Non-KeepAlive, CLOSING!");
        setProcessState(HSPS_CLOSE_SESSION);
        closeSession();
    }
    else
    {
        if (testFlag(HSF_HOOK_SESSION_STARTED))
        {
            if (m_sessionHooks.isEnabled(LSI_HKPT_HTTP_END))
                m_sessionHooks.runCallbackNoParam(LSI_HKPT_HTTP_END, (LsiSession *)this);
        }

        LsiapiBridge::releaseModuleData(LSI_DATA_HTTP, getModuleData());

        ls_atomic_fetch_add(&m_sn, 1);
        //ls_atomic_setint(&m_sessSeq, ls_atomic_add_fetch(&s_m_sessSeq, 1)); // ok to overflow / wrap
        LS_DBG_L("[T%d %s] sess seq now %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, ls_atomic_fetch_add(&m_sessSeq, 0));
        m_curHookLevel = 0;
        m_curHookRet = 0;
        m_sessionHooks.reset();
        m_sessionHooks.disableAll();
        m_iFlag = 0;
        ls_atomic_setint(&m_iMtFlag, 0);
        logAccess(0);
        ++m_iReqServed;
        getStream()->resetBytesCount();

        m_lReqTime = DateTime::s_curTime;
        m_iReqTimeUs = DateTime::s_curTimeUs;
        m_sendFileInfo.release();
        m_response.reset();
        m_request.reset(1);
        releaseMtSessData();

        const ConnInfo *pInfo = getStream()->getConnInfo();
        if (pInfo->m_pCrypto)
        {
            m_request.setCrypto(pInfo->m_pCrypto);
            m_request.setHttps();
        }
        else
            m_request.setCrypto(NULL);

        if (getRespBodyBuf())
            releaseRespBody();
        if (getGzipBuf())
            releaseGzipBuf();

        getStream()->switchWriteToRead();
        if (isLogIdBuilt())
        {
            char buf[16];
            ls_snprintf(buf, 10, "-%hu", m_iReqServed);
            lockAppendLogId(buf);
        }

        releaseResources();
        setClientInfo(pInfo->m_pClientInfo);
        setProcessState(HSPS_READ_REQ_HEADER);

        if (m_request.pendingHeaderDataLen())
        {
            LS_DBG_L(getLogSession(),
                     "Pending data in header buffer, set state to READING!");
            setState(HSS_READING);
            //if ( !inProcess() )
            processPending(0);
        }
        else
        {
            setState(HSS_WAITING);
            HttpStats::incIdleConns();
        }
        resetBackRefPtr();
    }
}


int HttpSession::httpError(int code, const char *pAdditional)
{
    if (m_request.isErrorPage() && code == SC_404)
        sendDefaultErrorPage(pAdditional);
    else
    {
        if (code < 0)
            code = SC_500;
        m_request.setStatusCode(code);
        sendHttpError(pAdditional);
    }
    releaseReqParser();
    m_sExtCmdResBuf.clear();
    //++m_sn;
    ls_atomic_fetch_add(&m_sn, 1);
    //ls_atomic_setint(&m_sessSeq, ls_atomic_add_fetch(&s_m_sessSeq, 1)); // ok to overflow / wrap
    LS_DBG_M("[T%d %s] sess seq now %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, ls_atomic_fetch_add(&m_sessSeq, 0));

    return 0;
}


int HttpSession::read(char *pBuf, int size)
{
    int len = m_request.pendingHeaderDataLen();
    if (len > 0)
    {
        if (len > size)
            len = size;
        memmove(pBuf, m_request.getHeaderBuf().begin() +
                m_request.getCurPos(), len);
        m_request.pendingDataProcessed(len);
        if (size > len)
        {
            int ret = getStream()->read(pBuf + len, size - len);
            if (ret > 0)
                len += ret;
        }
        return len;
    }
    return getStream()->read(pBuf, size);
}


int HttpSession::readv(struct iovec *vector, size_t count)
{
    const struct iovec *pEnd = vector + count;
    int total = 0;
    int ret;
    while (vector < pEnd)
    {
        if (vector->iov_len > 0)
        {
            ret = read((char *)vector->iov_base, vector->iov_len);
            if (ret > 0)
                total += ret;
            if (ret == (int)vector->iov_len)
            {
                ++vector;
                continue;
            }
            if (total)
                return total;
            return ret;
        }
        else
            ++vector;
    }
    return total;
}


void HttpSession::setupChunkIS()
{
    assert(m_pChunkIS == NULL);
    {
        m_pChunkIS = HttpResourceManager::getInstance().getChunkInputStream();
        m_pChunkIS->setStream(this);
        m_pChunkIS->open();
    }
}


int HttpSession::detectContentLenMismatch(int buffered)
{
    if (m_request.getContentLength() >= 0)
    {
        if (buffered + m_request.getContentFinished()
            != m_request.getContentLength())
        {
            LS_DBG_L(getLogSession(), "Protocol Error: Content-Length: %"
                     PRId64 " != Finished: %" PRId64 " + Buffered: %d!",
                     m_request.getContentLength(),
                     m_request.getContentFinished(),
                     buffered);
            return LS_FAIL;
        }
    }
    return 0;
}



bool HttpSession::endOfReqBody()
{
    if (m_pChunkIS)
        return m_pChunkIS->eos();
    else
        return (m_request.getBodyRemain() <= 0);
}


int HttpSession::reqBodyDone()
{
    int ret = 0;
//    HSPState next;
    getStream()->wantRead(0);
    setFlag(HSF_REQ_BODY_DONE, 1);
    LS_DBG_L(getLogSession(),
             "HttpSession::reqBodyDone().");

    if (this->getReqParser())
    {
        int rc = this->getReqParser()->parseDone();
        LS_DBG_L(getLogSession(),
                 "HttpSession::reqBodyDone, parseDone returned %d.", rc);
    }

    if (getFlag(HSF_REQ_WAIT_FULL_BODY) == HSF_REQ_WAIT_FULL_BODY
        && (m_iFlag & HSF_URI_MAPPED))
    {
        ret = handlerProcess(m_request.getHttpHandler());
        if (ret)
            return ret;
    }

    if (m_processState == HSPS_HANDLER_PROCESSING)
        setProcessState(HSPS_HKPT_RCVD_REQ_BODY_PROCESSING);
    else
    {
        setProcessState(HSPS_HKPT_RCVD_REQ_BODY);
        if (getMtFlag(HSF_MT_HANDLER) && m_pHandler)
            m_pHandler->onRead(this);
    }

//#define DAVID_OUTPUT_VMBUF
#ifdef DAVID_OUTPUT_VMBUF
    VMemBuf *pVMBuf = m_request.getBodyBuf();
    if (pVMBuf)
    {
        FILE *f = fopen("/tmp/vmbuf", "wb");
        if (f)
        {
            pVMBuf->rewindReadBuf();
            char *pBuf;
            size_t size;
            while (((pBuf = pVMBuf->getReadBuffer(size)) != NULL)
                   && (size > 0))
            {
                fwrite(pBuf, size, 1, f);
                pVMBuf->readUsed(size);
            }
            fclose(f);
        }
    }
#endif //DAVID_OUTPUT_VMBUF

//     smProcessReq();



//     if ( m_sessionHooks.isEnabled(LSI_HKPT_RCVD_REQ_BODY))
//     {
//         int ret = m_sessionHooks.runCallbackNoParam(LSI_HKPT_RCVD_REQ_BODY, (LsiSession *)this);
//         if ( ret <= -1)
//         {
//             return getModuleDenyCode( LSI_HKPT_RCVD_REQ_BODY );
//         }
//     }

    return ret;
}


int HttpSession::call_onRead(lsi_session_t *p, long , void *)
{
    HttpSession *pSession = (HttpSession *)p;
    pSession->onReadEx();
    return 0;
}


int HttpSession::readReqBody()
{
    char *pBuf;
    char tmpBuf[8192];
    size_t size = 0;
    int ret = 0;
    LS_DBG_L(getLogSession(), "Read Request Body!");

    const LsiApiHooks *pReadHooks = LsiApiHooks::getGlobalApiHooks(
                                        LSI_HKPT_RECV_REQ_BODY);
    int count = 0;
    int hasBufferedData = 1;
    int endBody;
    ReqParser *pParser = getReqParser();
    int readbytes = 0;
    int isspdy = getStream()->isSpdy();
    while (!(endBody = endOfReqBody()) || hasBufferedData)
    {
        hasBufferedData = 0;
        if (!endBody)
        {
            if ((ret < (int)size) || (++count == 10 && !isspdy))
            {
                if (count == 10)
                    EvtcbQue::getInstance().schedule(call_onRead, this, 0, NULL, false);
                return 0;
            }
        }

        if (pParser && pParser->isParsePost() && pParser->isParseUploadByFilePath())
        {
            pBuf = tmpBuf;
            size = 8192;
        }
        else
        {
            pBuf = m_request.getBodyBuf()->getWriteBuffer(size);
            if (!pBuf)
            {
                LS_ERROR(getLogSession(), "Ran out of swapping space while "
                         "reading request body!");
                return SC_400;
            }
        }

        if (!pReadHooks || m_sessionHooks.isDisabled(LSI_HKPT_RECV_REQ_BODY))
            ret = readReqBodyTermination((LsiSession *)this, pBuf, size);
        else
        {
            lsi_param_t param;
            lsi_hookinfo_t hookInfo;
            param.session = (LsiSession *)this;

            hookInfo.hooks = pReadHooks;
            hookInfo.hook_level = LSI_HKPT_RECV_REQ_BODY;
            hookInfo.term_fn = (filter_term_fn)readReqBodyTermination;
            hookInfo.enable_array = m_sessionHooks.getEnableArray(
                                        LSI_HKPT_RECV_REQ_BODY);
            param.cur_hook = (void *)((lsiapi_hook_t *)pReadHooks->end() - 1);
            param.hook_chain = &hookInfo;
            param.ptr1 = pBuf;
            param.len1 = size;
            param.flag_out = &hasBufferedData;
            ret = LsiApiHooks::runBackwardCb(&param);
            LS_DBG_L(getLogSession(), "[LSI_HKPT_RECV_REQ_BODY] read %d bytes",
                     ret);
        }
        if (ret > 0)
        {
            readbytes += ret;
            if (pParser && pParser->isParsePost())
            {
                if (!pParser->isParseUploadByFilePath())
                    m_request.getBodyBuf()->writeUsed(ret);

                //Update buf
                ret = pParser->parseUpdate(pBuf, ret);
                if (ret != 0)
                {
                    LS_ERROR(getLogSession(),
                             "pParser->parseUpdate() internal error(%d).",
                             ret);
                    return SC_500;
                }
            }
            else
                m_request.getBodyBuf()->writeUsed(ret);

            LS_DBG_L(getLogSession(), "Read %lld/%lld bytes of request body!",
                     (long long)m_request.getContentFinished(),
                     (long long)m_request.getContentLength());

            if (endOfReqBody())
            {
                getStream()->wantRead(0);
                break;
            }
            if (m_pHandler && !getFlag(HSF_REQ_WAIT_FULL_BODY) && !isspdy)
            {
                m_pHandler->onRead(this);
                if (getState() == HSS_REDIRECT)
                    return 0;
            }
        }
        else if (ret == -1)
            return SC_400;
        else if (ret == -2)
            return getModuleDenyCode(LSI_HKPT_RECV_REQ_BODY);
    }
    LS_DBG_L(getLogSession(), "Finished request body %lld bytes!",
             (long long)m_request.getContentFinished());
    if (!endOfReqBody() && isspdy && readbytes && m_pHandler)
    {
        m_pHandler->onRead(this);
        //return 0;
    }

    if (m_pChunkIS)
    {
        if (m_pChunkIS->getBufSize() > 0)
        {
            if (m_request.pendingHeaderDataLen() > 0)
            {
                assert(m_pChunkIS->getBufSize() <= m_request.getCurPos());
                m_request.rewindPendingHeaderData(m_pChunkIS->getBufSize());
            }
            else
            {
                m_request.compactHeaderBuf();
                if (m_request.appendPendingHeaderData(
                        m_pChunkIS->getChunkLenBuf(), m_pChunkIS->getBufSize()))
                    return SC_500;
            }
        }
        HttpResourceManager::getInstance().recycle(m_pChunkIS);
        m_pChunkIS = NULL;
        m_request.tranEncodeToContentLen();
    }
    //getStream()->setFlag( HIO_FLAG_WANT_READ, 0 );
    return reqBodyDone();
}


// int HttpSession::resumeHandlerProcess()
// {
//     if (!testFlag(HSF_URI_PROCESSED))
//     {
//         setProcessState(HSPS_PROCESS_NEW_URI);
//         return smProcessReq();
//     }
//     if (m_pHandler)
//         m_pHandler->onRead(this);
//     else
//         return handlerProcess(m_request.getHttpHandler());
//     return 0;
//
// }


int HttpSession::restartHandlerProcess()
{
    HttpContext *pContext = &(m_request.getVHost()->getRootContext());
    m_sessionHooks.reset();
    m_sessionHooks.inherit(pContext->getSessionHooks(), 0);
    if (m_sessionHooks.isEnabled(LSI_HKPT_HANDLER_RESTART))
        m_sessionHooks.runCallbackNoParam(LSI_HKPT_HANDLER_RESTART,
                                          (LsiSession *)this);

    clearFlag(HSF_RESP_HEADER_DONE | HSF_RESP_WAIT_FULL_BODY
              | HSF_RESP_FLUSHED | HSF_URI_MAPPED | HSF_HANDLER_DONE);
    return restartHandlerProcessEx();
}


int HttpSession::restartHandlerProcessEx()
{
    if (m_pHandler && cleanUpHandler(HSPS_HANDLER_RESTART_CANCEL_MT) == LS_AGAIN)
        return LS_AGAIN;

    m_sendFileInfo.release();

    if (m_pChunkOS)
    {
        m_pChunkOS->reset();
        releaseChunkOS();
    }
    if (getRespBodyBuf())
        releaseRespBody();
    if (getGzipBuf())
        releaseGzipBuf();

    m_response.reset();
    ls_atomic_add(&m_sn, 1);
    // ls_atomic_setint(&m_sessSeq, ls_atomic_add_fetch(&s_m_sessSeq, 1)); // ok to overflow / wrap
    LS_DBG_L("[T%d %s] sess seq now %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, ls_atomic_fetch_add(&m_sessSeq, 0));
    return 0;
}


int HttpSession::readReqBodyTermination(LsiSession *pSession, char *pBuf,
                                        int size)
{
    HttpSession *pThis = (HttpSession *)pSession;
    int len = 0;
    if (pThis->m_pChunkIS)
        len = pThis->m_pChunkIS->read(pBuf, size);
    else
    {
        off_t toRead = pThis->m_request.getBodyRemain();
        if (toRead > size)
            toRead = size ;
        if (toRead > 0)
            len = pThis->read(pBuf, toRead);
    }
    if (len > 0)
        pThis->m_request.contentRead(len);

    return len;
}


int HttpSession::processUnpackedHeaders()
{
    UnpackedHeaders *header = getStream()->getReqHeaders();
    if (!header)
        return LS_FAIL;
    int ret = m_request.processUnpackedHeaders(header);
    LS_DBG_L(getLogSession(),
                "processHeader() returned %d, header state: %d.",
                ret, m_request.getStatus());
    if (ret == 0)
    {
        m_iFlag &= ~HSF_URI_PROCESSED;
        m_processState = HSPS_NEW_REQ;
        smProcessReq();
        return 0;
    }
    m_processState = HSPS_HTTP_ERROR;
    if (getStream()->getState() < HIOS_SHUTDOWN)
        httpError(ret);
    return ret;
}


int HttpSession::readToHeaderBuf()
{
    AutoBuf &headerBuf = m_request.getHeaderBuf();
    LS_DBG_L(getLogSession(), "readToHeaderBuf().");
    do
    {
        int sz, avail;
        avail = headerBuf.available();
        if (avail > 2048)
            avail = 2048;
        char *pBuf = headerBuf.end();
        sz = getStream()->read(pBuf, avail);
        if (sz == -1)
            return sz;
        else if (sz > 0)
        {
            LS_DBG_L(getLogSession(), "Read %d bytes to header buffer.", sz);
            headerBuf.used(sz);

            int ret = m_request.processHeader();
            LS_DBG_L(getLogSession(),
                     "processHeader() returned %d, header state: %d.",
                     ret, m_request.getStatus());
            if (ret != 1)
            {
                if (ret == 0 && m_request.getStatus() == HttpReq::HEADER_OK)
                {
                    clearFlag(HSF_URI_PROCESSED);
                    setProcessState(HSPS_NEW_REQ);
                    releaseReqParser();
                }
                return ret;
            }
            if (headerBuf.available() <= 50)
            {
                int capacity = headerBuf.capacity();
                if (capacity < HttpServerConfig::getInstance().getMaxHeaderBufLen())
                {
                    int newSize = capacity + SmartSettings::getHttpBufIncreaseSize();
                    if (headerBuf.reserve(newSize))
                    {
                        errno = ENOMEM;
                        return SC_500;
                    }
                }
                else
                {
                    LS_NOTICE(getLogSession(),
                              "Http request header is too big, abandon!");
                    //m_request.setHeaderEnd();
                    //m_request.dumpHeader();
                    return SC_414;
                }
            }
        }
        if (sz != avail)
            return 0;
    }
    while (1);
}


void HttpSession::processPending(int ret)
{
    if ((getState() != HSS_READING) ||
        (m_request.pendingHeaderDataLen() < 2))
        return;
    LS_DBG_L(getLogSession(), "%d bytes pending in request header buffer.",
             m_request.pendingHeaderDataLen());
//            ::write( 2, m_request.getHeaderBuf().begin(),
//                      m_request.pendingHeaderDataLen() );
    ret = m_request.processHeader();
    if (ret == 1)
    {
        if (isHttps())
            smProcessReq();
        return;
    }
    if ((!ret) && (m_request.getStatus() == HttpReq::HEADER_OK))
    {
        clearFlag(HSF_URI_PROCESSED);
        setProcessState(HSPS_NEW_REQ);
        smProcessReq();
    }

    if ((ret) && (getStream()->getState() < HIOS_SHUTDOWN))
        httpError(ret);

}


int HttpSession::updateClientInfoFromProxyHeader(const char *pHeaderName,
        const char *pProxyHeader,
        int headerLen)
{
    char achIP[256];
    char achAddr[128];
    char *sAddr;
    char *colon;
    struct sockaddr *pAddr;
    void *pIP;
    const char *p;
    const char *pIpBegin = pProxyHeader;
    const char *pEnd = pProxyHeader + headerLen;
    int len;
    p = pProxyHeader;
    while (pIpBegin < pEnd)
    {
        while ((pIpBegin < pEnd) && isspace(*pIpBegin))
            ++pIpBegin;

        p = (char *)memchr(pIpBegin, ',', pEnd - pIpBegin);
        if (!p)
            p = pEnd;
        len = p - pIpBegin;
        if ((len <= 0) || (len > 255))
        {
            //error, not a valid IP address
            pIpBegin = p + 1;
            continue;
        }

        memmove(achIP, pIpBegin, len);
        sAddr = achIP;
        achIP[len] = 0;
        pAddr = (struct sockaddr *)achAddr;
        memset(pAddr, 0, sizeof(sockaddr_in6));
        if (achIP[0] == '[')
        {
            char *close = (char *)memchr(achIP, ']', len);
            if (close)
            {
                *close = '\0';
            }
            sAddr++;
            --len;
        }
        if (strncasecmp(sAddr, "::ffff:", 7) == 0)
        {
            sAddr += 7;
            len -= 7;
        }

        colon = (char *)memchr(sAddr, ':', len);
        if ((colon && memchr(sAddr, '.', len) == NULL) )
        {
            pAddr->sa_family = AF_INET6;
            pIP = &((struct sockaddr_in6 *)pAddr)->sin6_addr;
        }
        else
        {
            if (colon)
                *colon = '\0';
            pAddr->sa_family = AF_INET;
            pIP = &((struct sockaddr_in *)pAddr)->sin_addr;
        }
        if (inet_pton(pAddr->sa_family, sAddr, pIP) == 1)
            break;
        pIpBegin = p + 1;
    }
    if (pIpBegin >= pEnd)
    {
        LS_INFO(getLogSession(),
                "Failed to parse %s header [%.*s], use original IP",
                pHeaderName, headerLen, pProxyHeader);
        return 0;
    }

    ClientInfo *pInfo = ClientCache::getClientCache()->getClientInfo(pAddr);
    LS_DBG_L(getLogSession(),
             "update REMOTE_ADDR based on %s header to %s",
             pHeaderName, achIP);
    if (pInfo)
    {
        if (pInfo->checkAccess())
        {
            //Access is denied
            return SC_403;
        }
    }
    addEnv("PROXY_REMOTE_ADDR", 17,
           getPeerAddrString(), getPeerAddrStrLen());

    if (pInfo == m_pClientInfo)
        return 0;
    //NOTE: turn of connection account for now, it does not work well for
    //  changing client info based on x-forwarded-for header
    //  causes double dec at ntwkiolink level, no dec at session level
    //m_pClientInfo->decConn();
    //pInfo->incConn();

    m_pClientInfo = pInfo;
    return 0;
}


int HttpSession::processWebSocketUpgrade(HttpVHost *pVHost)
{
    HttpContext *pContext = pVHost->bestMatch(m_request.getURI(),
                                                    m_request.getURILen());
    LS_DBG_L(getLogSession(),
             "Request web socket upgrade, VH name: [%s] URI: [%s]",
             pVHost->getName(), m_request.getURI());
    if (pContext && pContext->getWebSockAddr()->get())
    {
        m_request.setStatusCode(SC_101);
        logAccess(0);
        L4Handler *pL4Handler = new L4Handler();
        pL4Handler->attachStream(getStream());
        pL4Handler->init(m_request, pContext->getWebSockAddr(),
                         getPeerAddrString(),
                         getPeerAddrStrLen());
        LS_DBG_L(getLogSession(), "VH: %s upgrade to web socket.",
                 pVHost->getName());
        setState(HSS_TOBERECYCLED);
        return 0;
        //DO NOT release pL4Handler, it will be releaseed itself.
    }
    else
    {
        LS_INFO(getLogSession(), "Cannot find web socket backend. URI: [%s]",
                m_request.getURI());
        m_request.keepAlive(0);
        return SC_404;
    }
}


int HttpSession::processHttp2Upgrade(const HttpVHost *pVHost)
{
    HioHandler *pHandler;
    pHandler = HioHandlerFactory::getHioHandler(HIOS_PROTO_HTTP2);
    if (!pHandler)
        return LS_FAIL;
    HioStream *pStream = getStream();
    pStream->clearLogId();
    pStream->switchHandler(this, pHandler);
    pStream->setProtocol(HIOS_PROTO_HTTP2);
    pHandler->attachStream(pStream);
    pHandler->h2cUpgrade(this);

    return 0;
}


void HttpSession::extCmdDone()
{
    m_iFlag &= ~HSF_EXEC_POPEN;
    EvtcbQue::getInstance().schedule(m_cbExtCmd,
                                     this,
                                     m_lExtCmdParam,
                                     m_pExtCmdParam, false);
}


int HttpSession::hookResumeCallback(lsi_session_t *session, long lParam,
                                    void *)
{
    HttpSession *pSession = (HttpSession *)(LsiSession *)session;
    if (!pSession)
        return -1;

    if ((uint32_t)lParam != pSession->getSn())
    {
        LS_DBG_L(pSession->getLogSession(),
                 "hookResumeCallback called. sn mismatch[%d %d], Session = %p",
                 (uint32_t)lParam, pSession->getSn(), pSession);

        //  abort();
        return -1;
    }
    return ((HttpSession *)pSession)->resumeProcess(0, 0);
}


int HttpSession::processNewReqInit()
{
    LS_DBG_L(getLogSession(),
             "processNewReq(), request header buffer size: %d, "
             "header used: %d, processed: %d.",
             m_request.getHeaderBuf().size(),
             m_request.getHttpHeaderEnd(), m_request.getCurPos());

    if ( m_request.getHttpHeaderLen() > 0 )
        LS_DBG_H(getLogSession(), "Headers: %.*s",
                m_request.getHttpHeaderLen(), m_request.getOrgReqLine() );

    int ret;
    HttpServerConfig &httpServConf = HttpServerConfig::getInstance();
    int useProxyHeader = httpServConf.getUseProxyHeader();
    if (getStream()->isSpdy())
    {
        //m_request.orGzip(REQ_GZIP_ACCEPT | httpServConf.getGzipCompress());
    }
    if ((useProxyHeader == 1)
        || (((useProxyHeader == 2) || (useProxyHeader == 3))
            && (getClientInfo()->getAccess() == AC_TRUST)))
    {
        const char *pName;
        const char *pProxyHeader;
        int len;
        if (((useProxyHeader == 2) || (useProxyHeader == 3))
            && m_request.isCfIpSet())
        {
            pName = "CF-Connecting-IP";
            pProxyHeader = m_request.getCfIpHeader(len);
        }
        else
        {
            pName = "X-Forwarded-For";
            pProxyHeader = m_request.getHeader(
                               HttpHeader::H_X_FORWARDED_FOR);
            len = m_request.getHeaderLen(HttpHeader::H_X_FORWARDED_FOR);
        }

        LS_DBG_L(getLogSession(), "HttpSession::processNewReqInit client IP from '%s: %.*s'.",
                 pName, len, pProxyHeader);

        if (*pProxyHeader)
        {
            ret = updateClientInfoFromProxyHeader(pName, pProxyHeader, len);
            if (ret)
                return ret;
        }
    }

    if (m_request.isUserAgentType(UA_GOOGLEBOT_POSSIBLE))
    {
        uint32_t flags = m_pClientInfo->isFlagSet(CIF_GOOG_FAKE | CIF_GOOG_REAL);
        if (flags == 0)
            m_pClientInfo->setFlag(CIF_GOOG_TEST);
        else if (flags & CIF_GOOG_REAL)
            m_request.setUserAgentType(UA_GOOGLEBOT_CONFIRMED);
        else
            m_request.setUserAgentType(UA_GOOGLEBOT_FAKE);
    }

    if (m_request.isHttps())
        m_request.addEnv("HTTPS", 5, "on", 2);

    m_lReqTime = DateTime::s_curTime;
    m_iReqTimeUs = DateTime::s_curTimeUs;

    clearFlag(HSF_ACCESS_LOG_OFF);
    const HttpVHost *pVHost = m_request.matchVHost();
    if (!pVHost)
    {
        if (LS_LOG_ENABLED(LOG4CXX_NS::Level::DBG_LESS))
        {
            char *pHostEnd = (char *)m_request.getHostStr() +
                             m_request.getHostStrLen();
            char ch = *pHostEnd;
            *pHostEnd = 0;
            LS_DBG_L(getLogSession(), "Cannot find a matching VHost.");
            *pHostEnd = ch;
        }

        m_sessionHooks.inherit(NULL, 1);
        return SC_404;
    }

    getStream()->setLogger(pVHost->getLogger());
    setLogger(pVHost->getLogger());
    if (getStream()->isLogIdBuilt())
    {
        lockAddOrReplaceFrom('#', pVHost->getName());
    }

    HttpContext *pContext0 = ((HttpContext *) & (pVHost->getRootContext()));
    m_sessionHooks.inherit(pContext0->getSessionHooks(), 0);
    m_pModuleConfig = pContext0->getModuleConfig();
    resetResp();

    ret = m_request.processNewReqData(getPeerAddr());
    if (ret)
        return ret;

    if (m_request.isWebsocket())
    {

        setProcessState(HSPS_WEBSOCKET);
        return processWebSocketUpgrade((HttpVHost *)pVHost);
    }
    else if (httpServConf.getEnableH2c() == 1 && m_request.isHttp2Upgrade())
    {
        //  setProcessState(HSPS_WEBSOCKET);
        processHttp2Upgrade(pVHost);
    }

    if (getClientInfo()->getAccess() != AC_TRUST)
    {
        if (getStream()->isThrottle()
            && (!(m_iFlag & HSF_SUB_SESSION)))
            getClientInfo()->getThrottleCtrl().adjustLimits(
                pVHost->getThrottleLimits());
        if (isUseRecaptcha())
        {
            if (rewriteToRecaptcha())
            {
                LS_DBG_M(getLogSession(), "[RECAPTCHA] VHost level triggered, redirect.");
                return 0;
            }
            else
            {
                LS_DBG_M(getLogSession(), "[RECAPTCHA] VHost level block parallel.");
                blockParallelRecaptcha();
                return SC_403;
            }
        }
    }

    if (m_request.isKeepAlive())
    {
        if (m_iReqServed >= pVHost->getMaxKAReqs())
        {
            m_request.keepAlive(false);
            LS_DBG_L(getLogSession(), "Reached maximum requests on keep"
                     "-alive connection, turn keep-alive off.");
        }
        else if (ConnLimitCtrl::getInstance().lowOnConnection())
        {
            ConnLimitCtrl::getInstance().setConnOverflow(1);
            m_request.keepAlive(false);
            LS_DBG_L(getLogSession(), "Nearing connection count soft limit,"
                     " turn keep-alive off.");
        }
        else if (getClientInfo()->getOverLimitTime())
        {
            m_request.keepAlive(false);
            LS_DBG_L(getLogSession(),
                     "Number of connections is over the soft limit,"
                     " turn keep-alive off.");
        }
    }

    m_request.setStatusCode(SC_200);
    //Run LSI_HKPT_HTTP_BEGIN after the inherit from vhost
    setFlag(HSF_HOOK_SESSION_STARTED);
    setProcessState(HSPS_HKPT_HTTP_BEGIN);
    return 0;
}


int HttpSession::parseReqArgs(int doPostBody, int uploadPassByPath,
                              const char *uploadTmpDir,
                              int uploadTmpFilePermission)
{
    ReqParser * pReqParser = getReqParser();
    bool new_parser = false;
    int ret = LS_OK;
    if (doPostBody)
    {
        if (m_request.getBodyType() == REQ_BODY_UNKNOWN)
            doPostBody = 0;
    }
    if (m_request.getQueryStringLen() == 0 && !doPostBody)
        return LS_OK;
    if (!pReqParser)
    {
        if (doPostBody && uploadTmpDir)
        {
            //If not exist, create it
            struct stat stBuf;
            int st = stat(uploadTmpDir, &stBuf);
            if (st == -1)
            {
                mkdir(uploadTmpDir, 0777);
                /**
                * Comment: call chmod because the mkdir will use the umask
                * so that the mod may not be 0777.
                */
                chmod(uploadTmpDir, 0777);
            }
        }
        pReqParser = new ReqParser();
        if (pReqParser)
        {
            if(0 != pReqParser->init(&m_request,
                                    uploadPassByPath,
                                    uploadTmpDir,
                                    uploadTmpFilePermission))
            {
                delete pReqParser;
                return LS_FAIL;
            }
        }
        else
            return LS_FAIL;
        new_parser = true;
    }
    if (doPostBody)
    {
        if (!pReqParser->isParsePost())
            ret = pReqParser->beginParsePost();
    }
    if (new_parser && !ls_atomic_casptr(&m_pReqParser, NULL, pReqParser))
    {
        delete pReqParser;
        return LS_FAIL;
    }
    return ret;
}

int HttpSession::processNewReqBody()
{
    int ret = 0;
    off_t l = m_request.getContentLength();
    if (l != 0)
    {
        if (l > HttpServerConfig::getInstance().getMaxReqBodyLen())
        {
            LS_NOTICE(getLogSession(), "Request body is too big! %lld",
                      (long long)l);
            m_iFlag |= HSF_REQ_BODY_DONE;
            getStream()->wantRead(0);
            return SC_413;
        }
        setState(HSS_READING_BODY);
        ret = m_request.prepareReqBodyBuf();
        if (ret)
            return ret;
        if (m_request.isChunked())
            setupChunkIS();
        //else
        //    m_request.processReqBodyInReqHeaderBuf();
        ret = readReqBody();
        //if (!ret && getFlag(HSF_REQ_BODY_DONE))
        if (!ret)
        {
            if ((getReqParser() && !getReqParser()->isParseDone())
                || testFlag(HSF_REQ_WAIT_FULL_BODY | HSF_REQ_BODY_DONE) ==
                HSF_REQ_WAIT_FULL_BODY)
                setProcessState(HSPS_READ_REQ_BODY);
            else if (m_processState != HSPS_HKPT_RCVD_REQ_BODY)
                setProcessState(HSPS_PROCESS_NEW_URI);
        }
    }
    else
        reqBodyDone();
    return ret;
}


int HttpSession::checkAuthentication(const HTAuth *pHTAuth,
                                     const AuthRequired *pRequired, int resume)
{

    const char *pAuthHeader = m_request.getHeader(HttpHeader::H_AUTHORIZATION);
    if (!*pAuthHeader)
        return SC_401;
    int authHeaderLen = m_request.getHeaderLen(HttpHeader::H_AUTHORIZATION);
    char *pAuthUser;
    if (!resume)
    {
        pAuthUser = m_request.allocateAuthUser();
        if (!pAuthUser)
            return SC_500;
        *pAuthUser = 0;
    }
    else
        pAuthUser = (char *)m_request.getAuthUser();
    int ret = pHTAuth->authenticate(this, pAuthHeader, authHeaderLen,
                                    pAuthUser,
                                    AUTH_USER_SIZE - 1, pRequired);
    if (ret)
    {
        if (ret > 0)      // if ret = -1, performing an external authentication
        {
            if (pAuthUser)
                LS_INFO(getLogSession(), "User '%s' failed to authenticate.",
                        pAuthUser);
        }
        else //save matched result
        {
            if (resume)
                ret = SC_401;
//            else
//                m_request.saveMatchedResult();
        }
    }
    else
    {
        LS_DBG_H(getLogSession(), "User '%s' authenticated.", pAuthUser);
        addEnv("HttpAuth", 8, "1", 1);
    }
    return ret;


}


// void HttpSession::resumeAuthentication()
// {
//     int ret = processURI( 1 );
//     if ( ret )
//         httpError( ret );
// }


int HttpSession::runExtAuthorizer(const HttpHandler *pHandler)
{
    assert(m_pHandler == NULL);
    int ret = assignHandler(pHandler);
    if (ret)
        return ret;
    setState(HSS_EXT_AUTH);
    setProcessState(HSPS_HANDLER_PROCESSING);
    return m_pHandler->process(this, pHandler);
}


void HttpSession::authorized()
{
    if (m_pHandler && cleanUpHandler(HSPS_HKPT_HTTP_AUTH) == LS_AGAIN)
        return;
    setProcessState(HSPS_HKPT_HTTP_AUTH);
    smProcessReq();
}


void HttpSession::addEnv(const char *pKey, int keyLen, const char *pValue,
                         long valLen)
{
    if (pKey == NULL || keyLen <= 0)
        return ;

    m_request.addEnv(pKey, keyLen, pValue, (int)valLen);
    lsi_callback_pf cb = EnvManager::getInstance().findHandler(pKey);
    if (cb && (0 != EnvManager::getInstance().execEnvHandler((
                   LsiSession *)this, cb, (void *)pValue, valLen)))
        return ;
}


int HttpSession::redirect(const char *pNewURL, int len, int alloc)
{
    int ret = m_request.redirect(pNewURL, len, alloc);
    if (ret)
        return ret;
    m_sendFileInfo.release();
    return redirectEx();
}


int HttpSession::redirectEx()
{
    if (m_pHandler && cleanUpHandler(HSPS_REDIRECT) == LS_AGAIN)
        return 0;
    m_request.setHandler(NULL);
    m_request.setContext(NULL);
    m_response.reset();
    setProcessState(HSPS_PROCESS_NEW_URI);
    m_iFlag &= ~HSF_URI_MAPPED;
    return smProcessReq();
}


int HttpSession::doRedirect()
{
    int ret = 0;
    const char *pLocation = m_request.getLocation();
    if (pLocation)
    {
        if (getState() == HSS_REDIRECT)
        {
            int ret = redirect(pLocation, m_request.getLocationLen());
            if (!ret)
                return ret;

            m_request.setStatusCode(ret);
        }
        if (m_request.getStatusCode() < SC_300)
        {
            LS_INFO(getLogSession(), "Redirect status code: %d.",
                    m_request.getStatusCode());
            m_request.setStatusCode(SC_307);
        }
    }
    sendHttpError(NULL);
    return ret;
}


int HttpSession::getVHostAccess()
{
    const HttpVHost *pVHost = m_request.getVHost();
    const AccessControl *pAcl = (NULL == pVHost ? NULL : pVHost->getAccessCtrl());

    if (NULL == pAcl)
        m_iVHostAccess = AC_ALLOW;
    else if (pAcl == m_pVHostAcl)
    {
        LS_DBG_M(getLogSession(), "getVHostAccess access unchanged %p.", pAcl);
    }
    else
    {
        m_iVHostAccess = pAcl->hasAccess(getPeerAddr());
        m_pVHostAcl = pAcl;
    }

    LS_DBG_M(getLogSession(), "getVHostAccess, acl returned access %d", m_iVHostAccess);
    return m_iVHostAccess;
}


int HttpSession::processVHostRewrite()
{
    const HttpContext *pContext = &(m_request.getVHost()->getRootContext());
    int ret = 0;
    m_request.orContextState(PROCESS_CONTEXT);
    if ((!m_request.getContextState(SKIP_REWRITE))
        && pContext->rewriteEnabled() & REWRITE_ON)
    {
        ret = RewriteEngine::getInstance().processRuleSet(
                  pContext->getRewriteRules(),
                  this, NULL, NULL);
        if (getStream()->getState() == HIOS_CLOSING)
        {
            setState(HSS_TOBERECYCLED);
            setProcessState(HSPS_DROP_CONNECTION);
            return 0;
        }
        if (ret == -3)      //rewrite happens
        {
            m_request.postRewriteProcess(
                RewriteEngine::getInstance().getResultURI(),
                RewriteEngine::getInstance().getResultURILen());
            ret = 0;
        }
        else if (ret)
        {
            LS_DBG_L(getLogSession(), "processRuleSet() returned %d.", ret);
            clearFlag(HSF_URI_PROCESSED);
            m_request.setContext(pContext);
            m_sessionHooks.reset();
            m_sessionHooks.inherit(((HttpContext *)pContext)->getSessionHooks(), 0);
            m_pModuleConfig = ((HttpContext *)pContext)->getModuleConfig();
            if (ret == -2)
            {
                setProcessState(HSPS_BEGIN_HANDLER_PROCESS);
                return 0;
            }
        }
#ifdef USE_RECV_REQ_HEADER_HOOK
        addEnv("HAVE_REWRITE", 11, "1", 1);
#endif
    }
    setProcessState(HSPS_CONTEXT_MAP);
    return ret;
}


bool HttpSession::shouldAvoidRecaptcha()
{
    ClientInfo *pClientInfo = getClientInfo();
    if (AC_CAPTCHA == pClientInfo->getAccess())
    {
        LS_DBG_M(getLogSession(), "[RECAPTCHA] Client %.*s has been verified.",
                pClientInfo->getAddrStrLen(), pClientInfo->getAddrString());
        return true;
    }
    else if (AC_TRUST == pClientInfo->getAccess())
    {
        LS_DBG_M(getLogSession(), "[RECAPTCHA] Client %.*s is trusted, skip recaptcha.",
                pClientInfo->getAddrStrLen(), pClientInfo->getAddrString());
        return true;
    }

    if (AC_TRUST == getVHostAccess())
    {
        LS_DBG_M(getLogSession(), "[RECAPTCHA] Client %.*s is in vhost whitelist.",
                pClientInfo->getAddrStrLen(), pClientInfo->getAddrString());
        return true;
    }

    if (m_request.isFavicon())
    {
        LS_DBG_M(getLogSession(), "[RECAPTCHA] Favicon request, skip recaptcha.");
        return true;
    }

//     if (getStream() && getStream()->isFromLocalAddr())
//     {
//         LS_DBG_M(getLogSession(), "[RECAPTCHA] %.*s is from local address, skip recaptcha.",
//                 pClientInfo->getAddrStrLen(), pClientInfo->getAddrString());
//         return true;
//     }

    const char *pUserAgent = m_request.getUserAgent();
    int iUserAgentLen = m_request.getUserAgentLen();

    const Recaptcha *pRecaptcha = m_request.getRecaptcha(),
            *pServerRecaptcha = HttpServerConfig::getInstance()
               .getGlobalVHost()->getRecaptcha();

    // check bots
    if (m_request.isGoodBot()
        || pServerRecaptcha->isBotWhitelisted(pUserAgent, iUserAgentLen)
        || (pRecaptcha != pServerRecaptcha
            && pRecaptcha->isBotWhitelisted(pUserAgent, iUserAgentLen)))
    {
        if (m_request.isUnlimitedBot())
            return true;
        pClientInfo->incAllowedBotHits();
        if (!pClientInfo->isReachBotLimit())
            return true;
    }

    return false;
}


bool HttpSession::isUseRecaptcha(int isEarlyDetect)
{
    const Recaptcha *pRecaptcha = m_request.getRecaptcha();
    if (NULL == pRecaptcha)
        return 0;

    if (shouldAvoidRecaptcha())
        return 0;

    ConnLimitCtrl &ctrl = ConnLimitCtrl::getInstance();
    int sslConns = ctrl.getMaxSSLConns() - ctrl.availSSLConn();
    int plainConns = ctrl.getMaxConns() - sslConns - ctrl.availConn();
    const Recaptcha *pServerRecaptcha = HttpServerConfig::getInstance()
               .getGlobalVHost()->getRecaptcha();

    /**
     * If the connection is new, the count is inaccurate; the count is updated
     * _after_ the first request is served.
     */
    if (!m_iReqServed)
    {
        if (isHttps())
            ++sslConns;
        else
            ++plainConns;
    }

    LS_DBG_H(getLogSession(),
            "[RECAPTCHA] Concurrent Stats reg: %d, ssl: %d",
            plainConns, sslConns);

    int ret = (plainConns >= pServerRecaptcha->getRegConnLimit()
                || sslConns >= pServerRecaptcha->getSslConnLimit());

    if (!isEarlyDetect)
    {
        HttpVHost *pVHost = (HttpVHost *) m_request.getVHost();

        if (!pVHost->isRecaptchaEnabled())
            return 0;

        LS_DBG_H(getLogSession(),"[RECAPTCHA] VHost Refs %d(volatile)",
                pVHost->getRef());
        ret |= (pVHost->getRef() >= pRecaptcha->getRegConnLimit());
    }

    return ret;
}


bool HttpSession::hasPendingCaptcha() const
{
    return getClientInfo()->isFlagSet(CIF_CAPTCHA_PENDING);
}


bool HttpSession::recaptchaAttemptsAvail() const
{
    const Recaptcha *pRecaptcha = m_request.getRecaptcha();
    if (!pRecaptcha)
        return false;
    return (getClientInfo()->getCaptchaTries() < pRecaptcha->getMaxTries());
}


// Return 1 to indicate rewritten, 0 otherwise.
int HttpSession::rewriteToRecaptcha(bool blockIfTooManyAttempts)
{
    LS_DBG_M(getLogSession(), "[RECAPTCHA] Try to rewrite request to recaptcha.");
    ClientInfo *pClientInfo = getClientInfo();

    if (blockIfTooManyAttempts && !recaptchaAttemptsAvail())
        return 0;

    if (!m_request.isCaptcha() &&  hasPendingCaptcha())
    {
        LS_DBG_M(getLogSession(), "[RECAPTCHA] Client %.*s has pending captcha.",
                pClientInfo->getAddrStrLen(), pClientInfo->getAddrString());
        return 0;
    }

    if (m_request.rewriteToRecaptcha())
    {
        pClientInfo->setFlag(CIF_CAPTCHA_PENDING);
        pClientInfo->incCaptchaTries();
        LS_DBG_M("[RECAPTCHA] new count %d", pClientInfo->getCaptchaTries());
        m_processState = HSPS_BEGIN_HANDLER_PROCESS;
    }
    else if (!getFlag(HSF_REQ_BODY_DONE))
    {
        LS_DBG_M(getLogSession(), "[RECAPTCHA] Need to finish reading body.");
        setFlag(HSF_URI_MAPPED);
        setFlag(HSF_REQ_WAIT_FULL_BODY);
        m_processState = HSPS_PROCESS_NEW_REQ_BODY;
    }

    return 1;
}


void HttpSession::checkSuccessfulRecaptcha()
{
    ClientInfo *pClientInfo = getClientInfo();
    if (!pClientInfo)
        return;

    if (!m_request.getContext()->isCheckCaptcha())
    {
        LS_WARN(getLogSession(),
                "[RECAPTCHA] WARNING: Got a suspicious response header during a non recaptcha request.");
        return;
    }

    LS_DBG_M(getLogSession(), "[RECAPTCHA] Successful, trust %.*s",
            pClientInfo->getAddrStrLen(), pClientInfo->getAddrString());
    pClientInfo->setAccess(AC_CAPTCHA);
    pClientInfo->resetCaptchaTries();
    pClientInfo->clearFlag(CIF_CAPTCHA_PENDING);
}


void HttpSession::blockParallelRecaptcha()
{
    LS_DBG_H(getLogSession(), "[RECAPTCHA] Have pending captcha, do not allow multiple.");
    // m_processState = HSPS_DROP_CONNECTION;
    // setState(HSS_IO_ERROR);
    m_request.setStatusCode(SC_403);
}


int HttpSession::processContextMap()
{
    const HttpContext *pOldCtx = m_request.getContext();
    HttpContext *pCtx;
    int ret;
    setFlag(HSF_URI_PROCESSED);
    ret = m_request.processContext();
    if (ret)
    {
        LS_DBG_L(getLogSession(), "processContext() returned %d.", ret);
        if (ret == -2)
        {
            setProcessState(HSPS_CHECK_AUTH_ACCESS);
            ret = 0;
        }
    }
    else
    {
        setProcessState(HSPS_CONTEXT_REWRITE);
        if (((pCtx = (HttpContext *)m_request.getContext()) != pOldCtx)
            && pCtx->getModuleConfig() != NULL)
        {
            m_sessionHooks.inherit(pCtx->getSessionHooks(), 0);
            m_pModuleConfig = pCtx->getModuleConfig();
        }
    }
    return ret;
}


int HttpSession::processContextRewrite()
{
    int ret = 0;
    const HttpContext *pContext = m_request.getContext();
    const HttpContext *pVHostRoot = &m_request.getVHost()->getRootContext();
    //context level URL rewrite
    if (m_request.getContextState(SKIP_REWRITE))
    {
        setProcessState(HSPS_HKPT_URI_MAP);
        return 0;
    }

    while ((!pContext->hasRewriteConfig())
            && (pContext->getParent())
            && (pContext->getParent() != pVHostRoot))
        pContext = pContext->getParent();

    if (!(pContext->rewriteEnabled() & REWRITE_ON))
    {
        LS_DBG_L(getLogSession(),
                "[REWRITE] Rewrite engine is not enabled for context '%s'",
                pContext->getContextURI()->c_str());
        setProcessState(HSPS_HKPT_URI_MAP);
        return 0;
    }
    ret = RewriteEngine::getInstance().processRuleSet(
                                                pContext->getRewriteRules(),
                                                this, pContext, pVHostRoot);
    if (getStream()->getState() == HIOS_CLOSING) //drop connection
    {
        setState(HSS_TOBERECYCLED);
        setProcessState(HSPS_DROP_CONNECTION);
        return 0;
    }
    if (ret == -3)
    {
        ret = 0;
        if (m_request.postRewriteProcess(
                RewriteEngine::getInstance().getResultURI(),
                RewriteEngine::getInstance().getResultURILen()))
        {
            clearFlag(HSF_URI_PROCESSED);
            m_request.orContextState(PROCESS_CONTEXT);
            setProcessState(HSPS_CHECK_AUTH_ACCESS);
        }
        else
            setProcessState(HSPS_HKPT_URI_MAP);
    }
    else if (ret == -2)
    {
        ret = 0;
        m_request.setContext(pContext);
        m_sessionHooks.reset();
        m_sessionHooks.inherit(((HttpContext *)pContext)->getSessionHooks(), 0);
        m_pModuleConfig = ((HttpContext *)pContext)->getModuleConfig();
        setProcessState(HSPS_CHECK_AUTH_ACCESS);
    }
    else
        setProcessState(HSPS_HKPT_URI_MAP);

    return ret;
}


/**
 * Return 1, will bepass URI_MAP hooks
 */
int HttpSession::preUriMap()
{
//     int valLen = 0;
//     const char *pEnv = m_request.getEnv("modpagespeed", 12, valLen);
//     if (valLen == 2 && strncasecmp(pEnv, "on", 2) == 0)
//         return 0;
    if (HttpServerConfig::getInstance().getUsePagespeed())
        return 0;

    m_request.checkUrlStaicFileCache();
    static_file_data_t *pDataSt = m_request.getUrlStaticFileData();
    int ret = (pDataSt ? 1 : 0);
    if (LS_LOG_ENABLED(LOG4CXX_NS::Level::DBG_LESS))
    {
        LS_DBG_L(getLogSession(), "preUriMap check serving by static url file cache: %d",
                 ret);
    }

    if (ret)
    {
        m_request.addEnv("staticcacheserve", 16, "1", 1);
        if (pDataSt->pData->getBypassModsec())
            m_request.addEnv("modsecurity", 11, "off", 3);
    }

    return ret;
}

int HttpSession::processFileMap()
{
    m_request.checkUrlStaicFileCache();

    if (getReq()->getHttpHandler() == NULL ||
        getReq()->getHttpHandler()->getType() != HandlerType::HT_MODULE)
    {
        int ret = m_request.processContextPath();
        LS_DBG_L(getLogSession(), "processContextPath() returned %d.", ret);
        if (ret == -1)        //internal redirect
            clearFlag(HSF_URI_PROCESSED);
        else if (ret > 0)
        {
            if (ret == SC_404)
                setFlag(HSF_SC_404);
            else
                return ret;
        }
    }

    setProcessState(HSPS_CHECK_AUTH_ACCESS);
    return 0;
}


//404 error must go through authentication first
int HttpSession::processContextAuth()
{
    AAAData     aaa;
    int         satisfyAny;
    int         ret1;

    if (!m_request.getContextState(CONTEXT_AUTH_CHECKED | KEEP_AUTH_INFO))
    {
        m_request.getAAAData(aaa, satisfyAny);
        m_request.orContextState(CONTEXT_AUTH_CHECKED);
        int satisfy = 0;
        if (aaa.m_pAccessCtrl)
        {
            if (!aaa.m_pAccessCtrl->hasAccess(getPeerAddr()))
            {
                if (!satisfyAny || !aaa.m_pHTAuth)
                {
                    LS_INFO(getLogSession(),
                            "[ACL] Access to context [%s] is denied!",
                            m_request.getContext()->getURI());
                    return SC_403;
                }
            }
            else
                satisfy = satisfyAny;
        }
        if (!satisfy)
        {
            if (aaa.m_pRequired && aaa.m_pHTAuth)
            {
                ret1 = checkAuthentication(aaa.m_pHTAuth, aaa.m_pRequired, 0);
                if (ret1)
                {
                    LS_DBG_L(getLogSession(),
                             "checkAuthentication() returned %d.", ret1);
                    if (ret1 == -1)     //processing authentication
                    {
                        setState(HSS_EXT_AUTH);
                        return LSI_SUSPEND;
                    }
                    return ret1;
                }
            }
            if (aaa.m_pAuthorizer)
                return runExtAuthorizer(aaa.m_pAuthorizer);
        }
    }
    setProcessState(HSPS_HKPT_HTTP_AUTH);
    return 0;
}


int HttpSession::processAuthorizer()
{
    return 0;
}


int HttpSession::processNewUri()
{
    if (getReq()->getHttpHandler() != NULL &&
        getReq()->getHttpHandler()->getType() == HandlerType::HT_MODULE)
        setProcessState(HSPS_BEGIN_HANDLER_PROCESS);
    else
        setProcessState(HSPS_VHOST_REWRITE);
    return 0;
}


bool ReqHandler::notAllowed(int Method) const
{
    return (Method > HttpMethod::HTTP_POST);
}


int HttpSession::setUpdateStaticFileCache(const char *pPath, int pathLen,
                                          int fd, struct stat &st)
{
    StaticFileCacheData *pCache;
    int ret;
    ret = StaticFileCache::getInstance().getCacheElement(pPath, pathLen, st,
            fd, &pCache);
    if (ret)
    {
        LS_DBG_L(getLogSession(), "getCacheElement() returned %d.", ret);
        return ret;
    }
    pCache->setLastAccess(DateTime::s_curTime);
    m_sendFileInfo.setFileData(pCache);
    return 0;
}


int HttpSession::getParsedScript(SsiScript *&pScript)
{
    int ret;
    const AutoStr2 *pPath = m_request.getRealPath();
    ret = setUpdateStaticFileCache(pPath->c_str(),
                                   pPath->len(),
                                   m_request.transferReqFileFd(), m_request.getFileStat());
    if (ret)
        return ret;

    StaticFileCacheData *pCache = m_sendFileInfo.getFileData();
    pScript = pCache->getSsiScript();
    if (!pScript)
    {
        pScript = new SsiScript();
        SsiTagConfig *pTagConfig = NULL;
        if (m_request.getVHost())
            pTagConfig = m_request.getVHost()->getSsiTagConfig();
        ret = pScript->parse(pTagConfig, pCache->getKey());
        if (ret == -1)
        {
            delete pScript;
            pScript = NULL;
        }
        else
            pCache->setSsiScript(pScript);
    }
    return 0;
}


int HttpSession::detectSsiStackLoop(const SsiBlock *pBlock)
{
    SsiStack * pStack = m_pSsiStack;
    while(pStack != NULL)
    {
        if (pBlock == pStack->getCurrentBlock())
            return 1;
        pStack = pStack->getPrevious();
    }
    return 0;
}


char HttpSession::getSsiStackDepth() const
{
    return m_pSsiStack? m_pSsiStack->getDepth() : 0;
}


void HttpSession::prepareSsiStack(const SsiScript *pScript)
{
    int depth = 0;
    if (m_pSsiStack == NULL)
    {
        depth = m_pParent? m_pParent->getSsiStackDepth() : 0;
    }
    else
        depth = m_pSsiStack->getDepth();
    m_pSsiStack = new SsiStack(m_pSsiStack, depth + 1);
    m_pSsiStack->setScript(pScript);
}


SsiStack *HttpSession::popSsiStack()
{
    SsiStack *pStack = m_pSsiStack;
    if (pStack)
    {
        m_pSsiStack = pStack->getPrevious();
        delete pStack;
    }
    return m_pSsiStack;
}


int HttpSession::startServerParsed()
{
    SsiScript *pScript = NULL;
    getParsedScript(pScript);
    if (!pScript)
        return SC_500;

    setState(HSS_PROCESSING);
    return SsiEngine::beginExecute(this, pScript);
}


int HttpSession::handlerProcess(const HttpHandler *pHandler)
{
    if ((m_iFlag & HSF_URI_MAPPED) == 0)
    {
        setProcessState(HSPS_HANDLER_PRE_PROCESSING);
        return 0;
    }

    setProcessState(HSPS_HANDLER_PROCESSING);
    if (m_pHandler && cleanUpHandler(HSPS_HANDLER_PROCESSING) == LS_AGAIN)
        return 0;

    if (pHandler == NULL)
        return SC_403;

     if (m_request.getContext()
        && m_request.getContext()->isAppContext())
    {
        if (m_request.getStatusCode() == SC_404)
        {
            m_request.setStatusCode(SC_200);
            setFlag(HSF_REQ_WAIT_FULL_BODY);
            m_request.clearContextState(REWRITE_PERDIR);
            if (m_request.getContext()->isNodejsContext())
                m_request.clearRedirects();
            else
                m_request.fixRailsPathInfo();
        }
    }

    int type = pHandler->getType();
    if ((type >= HandlerType::HT_DYNAMIC) &&
        (type != HandlerType::HT_PROXY))
        if (m_request.checkScriptPermission() == 0)
            return SC_403;
    if (pHandler->getType() == HandlerType::HT_SSI)
    {
        const HttpContext *pContext = m_request.getContext();
        if (pContext && !pContext->isIncludesOn())
        {
            LS_INFO(getLogSession(),
                    "Server Side Include is disabled for [%s], deny access.",
                    pContext->getLocation());
            return SC_403;
        }
        return startServerParsed();
    }
    int dyn = (pHandler->getType() >= HandlerType::HT_DYNAMIC);
    ThrottleControl *pTC = &getClientInfo()->getThrottleCtrl();
    if ((getClientInfo()->getAccess() == AC_TRUST)
        || (getVHostAccess() == AC_TRUST)
        || (pTC->allowProcess(dyn)))
    {
    }
    else
    {
        if (LS_LOG_ENABLED(LOG4CXX_NS::Level::DBG_LESS))
        {
            const ThrottleUnit *pTU = pTC->getThrottleUnit(dyn);
            LS_DBG_L(getLogSession(), "%s throttling %d/%d",
                     (dyn) ? "Dyn" : "Static", pTU->getAvail(), pTU->getLimit());
            if (dyn)
            {
                pTU = pTC->getThrottleUnit(2);
                LS_DBG_L(getLogSession(), "dyn processor in use %d/%d",
                         pTU->getAvail(), pTU->getLimit());
            }
        }
        setState(HSS_THROTTLING);
        return 0;
    }
    if (m_request.getContext() && m_request.getContext()->isRailsContext())
        setFlag(HSF_REQ_WAIT_FULL_BODY);
    if (testFlag(HSF_REQ_WAIT_FULL_BODY | HSF_REQ_BODY_DONE) ==
        HSF_REQ_WAIT_FULL_BODY)
    {
        getStream()->wantRead(1);
        return 0;
    }

    int ret = assignHandler(pHandler);
    if (ret)
        return ret;

    const HttpVHost *pVHost = m_request.getVHost();
    if ((type == HandlerType::HT_CGI) &&
        (((pVHost) && (pVHost->enableCGroup())) ||
         ((!pVHost) && (ServerProcessConfig::getInstance().getCGroupAllow()) &&
          (ServerProcessConfig::getInstance().getCGroupDefault()))))
    {
        LS_DBG_L(getLogSession(), "HttpSession::CGroup activate");
        m_request.addEnv("LS_CGROUP", 9, "Y", 1);
    }
    else
    {
        LS_DBG_L(getLogSession(), "HttpSession::CGroup don't activate, type: %s "
                 "vHost: %p, vHost->enableCGroup: %s, config.getCGroupAllow: %s\n",
                 (m_request.getHttpHandler()->getType() == HandlerType::HT_CGI) ? "CGI" : "NOT CGI",
                 pVHost, ((pVHost) && (pVHost->enableCGroup())) ? "YES" : "NO",
                 ServerProcessConfig::getInstance().getCGroupAllow() ? "YES" : "NO");
    }

    setState(HSS_PROCESSING);
    pTC->incReqProcessed( dyn );
    ret = m_pHandler->process(this, m_request.getHttpHandler());

    if (ret == 1)
    {
#ifdef LS_AIO_USE_AIO
        if (testFlag(HSF_AIO_READING) == 0)
#endif
            wantWrite(1);
        ret = 0;
    }
    return ret;
}


int HttpSession::assignHandler(const HttpHandler *pHandler)
{
    ReqHandler *pNewHandler = NULL;
    int handlerType;
    handlerType = pHandler->getType();
    pNewHandler = HandlerFactory::getHandler(handlerType);
    if (pNewHandler == NULL)
    {
        LS_ERROR(getLogSession(), "Handler with type id [%d] is not implemented.",
                 handlerType);
        return SC_500;
    }

//    if ( pNewHandler->notAllowed( m_request.getMethod() ) )
//    {
//        LS_DBG_L( getLogSession(), "Method %s is not allowed.",
//                  HttpMethod::get( m_request.getMethod()));
//        return SC_405;
//    }

//    LS_DBG_L( getLogSession(), "handler with type:%d assigned.",
//              handlerType ));
    switch (handlerType)
    {
    case HandlerType::HT_STATIC:
        //So, if serve with static file, try use cache first
        if (m_request.getUrlStaticFileData())
        {
            m_sendFileInfo.setFileData(m_request.getUrlStaticFileData()->pData);
            m_sendFileInfo.setECache(m_request.getUrlStaticFileData()->pData->getFileData());
            m_sendFileInfo.setCurPos(0);
            m_sendFileInfo.setCurEnd(m_request.getUrlStaticFileData()->pData->getFileSize());
            m_sendFileInfo.setParam(NULL);
            setFlag(HSF_STX_FILE_CACHE_READY);
            LS_DBG_L( getLogSession(), "[static file cache] handling static file [ref=%d %d].",
                m_request.getUrlStaticFileData()->pData->getRef(),
                m_request.getUrlStaticFileData()->pData->getFileData()->getRef());
        }
        break;
    case HandlerType::HT_PROXY:
        m_request.applyHeaderOps(this, NULL);
        //fall through
    case HandlerType::HT_FASTCGI:
    case HandlerType::HT_CGI:
    case HandlerType::HT_SERVLET:
    case HandlerType::HT_LSAPI:
    case HandlerType::HT_MODULE:
    case HandlerType::HT_LOADBALANCER:
        {
            if (m_request.getStatus() != HttpReq::HEADER_OK)
                return SC_400;  //cannot use dynamic request handler to handle invalid request
            /*        if ( m_request.getMethod() == HttpMethod::HTTP_POST )
                        getClientInfo()->setLastCacheFlush( DateTime::s_curTime );  */
            if (getSsiStack())
            {
                if ((getSsiStack()->isCGIRequired()) &&
                    (handlerType != HandlerType::HT_CGI))
                {
                    LS_INFO(getLogSession(), "Server Side Include requested a CGI "
                            "script, [%s] is not a CGI script, access denied.",
                            m_request.getURI());
                    return SC_403;
                }
            }
            else
                resetResp();

            const char *pType = HandlerType::getHandlerTypeString(handlerType);
            if (handlerType == HandlerType::HT_MODULE)
            {
                pType = MODULE_NAME(((const LsiModule *)pHandler)->getModule());
                LS_DBG_L(getLogSession(), "Run module [%s] handler.", pType);
            }
            else
                LS_DBG_L(getLogSession(), "Run %s processor.", pType);

            if (isLogIdBuilt())
            {
                lockAddOrReplaceFrom(':', pType);
            }
            if (!HttpServerConfig::getInstance().getDynGzipCompress())
                m_request.andGzip(~GZIP_ENABLED);
            //m_response.reset();
            break;
        }
    default:
        break;
    }

    setHandler(pNewHandler);
    return 0;
}


int HttpSession::sendHttpError(const char *pAdditional)
{
    int statusCode = m_request.getStatusCode();
    LS_DBG_L(getLogSession(), "HttpSession::sendHttpError(), code = '%s'.",
             HttpStatusCode::getInstance().getCodeString(m_request.getStatusCode()) +
             1);
    if ((statusCode < 0) || (statusCode >= SC_END))
    {
        LS_ERROR(getLogSession(), "Invalid HTTP status code: %d!", statusCode);
        statusCode = SC_500;
    }
    if (statusCode == SC_503)
    {
        HttpStats::inc503Errors();
        LS_NOTICE(getLogSession(), "oops! %s",
                  HttpStatusCode::getInstance()
                                    .getCodeString(m_request.getStatusCode()));
        m_request.dumpHeader();
    }
    if (m_iFlag & HSF_NO_ERROR_PAGE)
        return statusCode;

    if (isRespHeaderSent())
    {
        endResponse(0);
        return 0;
    }
    if (getSsiRuntime())
    {
        if (statusCode < SC_500)
            SsiEngine::printError(this, NULL);
        markComplete(true);
        getStream()->wantWrite(1);
        return 0;
    }
    if ((m_request.getStatus() != HttpReq::HEADER_OK)
        || HttpStatusCode::getInstance().fatalError(statusCode)
        || m_request.getBodyRemain() > 0)
        m_request.keepAlive(false);
    // Let HEAD request follow the errordoc URL, the status code could be changed
    //if ( sendBody() )
    {
        const HttpContext *pContext = m_request.getContext();
        if (!pContext)
        {
            if (m_request.getVHost())
                pContext = &m_request.getVHost()->getRootContext();
        }
        const AutoStr2 *pErrDoc;
        if (!pContext)
            return sendDefaultErrorPage(pAdditional);
        if ((pErrDoc = pContext->getErrDocUrl(statusCode)) == NULL)
            return sendDefaultErrorPage(pAdditional);
        if (statusCode == SC_401)
            m_request.orContextState(KEEP_AUTH_INFO);
        if (*pErrDoc->c_str() == '"')
        {
            pAdditional = pErrDoc->c_str() + 1;
            return sendDefaultErrorPage(pAdditional);
        }
        m_request.setErrorPage();
        if ((statusCode >= SC_300) && (statusCode <= SC_403))
        {
            if (m_request.getContextState(REWRITE_REDIR))
                m_request.orContextState(SKIP_REWRITE);
        }

        if (pErrDoc->len() < 2048)
        {
            int ret = redirect(pErrDoc->c_str(), pErrDoc->len(),
                               !m_request.isAppContext());
            if (ret == 0 || statusCode == ret)
                return 0;
            if ((ret != m_request.getStatusCode())
                && (m_request.getStatusCode() == SC_404)
                && (ret != SC_503) && (ret > 0))
            {
                return httpError(ret, NULL);
            }
        }
        else
        {
            LS_WARN(getLogSession(), "Custom error page for %.4s is too long, "
                    "%d bytes, use default error page.\n",
                    HttpStatusCode::getInstance().getCodeString(statusCode),
                    pErrDoc->len());
        }
    }
    return sendDefaultErrorPage(pAdditional);
}


int HttpSession::sendDefaultErrorPage(const char * pAdditional)
{
    setState(HSS_WRITING);
    buildErrorResponse(pAdditional);
    endResponse(1);
    HttpStats::getReqStats()->incReqProcessed();
    return 0;
}



int HttpSession::buildErrorResponse(const char *errMsg)
{
    int errCode = m_request.getStatusCode();

    char *location = NULL;
    int nLocation = 0 ;
    if ( errCode >= SC_300 && errCode < SC_400 )
    {
        const char *p = m_response.getRespHeaders().getHeader(
                                HttpRespHeaders::H_LOCATION, &nLocation);
        if (nLocation > 0)
            location = strndup(p, nLocation);
    }

//     if (getSsiRuntime())
//     {
//         if (( errCode >= SC_300 )&&( errCode < SC_400 ))
//         {
//             SsiEngine::appendLocation( this, m_request.getLocation(),
//                  m_request.getLocationLen() );
//             return 0;
//         }
//         if (errMsg == NULL)
//             errMsg = HttpStatusCode::getInstance().getRealHtml( errCode );
//         if (errMsg)
//         {
//             appendDynBody(errMsg, strlen(errMsg));
//         }
//         return 0;
//     }

    resetResp();
    m_sendFileInfo.release();

    if (nLocation > 0)
    {
        m_response.getRespHeaders().add(HttpRespHeaders::H_LOCATION, location,
                                        nLocation);
        free(location);
    }

    //m_response.prepareHeaders( &m_request );
    //register int errCode = m_request.getStatusCode();
    unsigned int ver = m_request.getVersion();
    if (ver > HTTP_1_0)
    {
        LS_ERROR(getLogSession(), "Invalid HTTP version: %d.", ver);
        ver = HTTP_1_1;
    }
    if (!isNoRespBody())
    {
        const char *pBody;
        int len = 0;
        if (errMsg)
        {
            pBody = errMsg;
            len = strlen(errMsg);
            m_response.setContentTypeHeader("text/html", 9, NULL);
        }
        else
        {
            pBody = HttpStatusCode::getInstance().getRealHtml(errCode);
            if (pBody == NULL)
            {
                m_response.setContentLen(0);
                sendRespHeaders();
                return 0;
            }
            len = HttpStatusCode::getInstance().getBodyLen(errCode);
            m_response.getRespHeaders().add(HttpRespHeaders::H_CONTENT_TYPE,
                                            "text/html",
                                            9);
            if (errCode >= SC_307)
            {
                m_response.getRespHeaders().add(HttpRespHeaders::H_CACHE_CTRL,
                                                "private, no-cache, max-age=0", 28);
                m_response.getRespHeaders().add(HttpRespHeaders::H_PRAGMA, "no-cache", 8);
            }
        }

        m_response.setContentLen(len);
        sendRespHeaders();
        int ret = writeRespBody(pBody, len);
        if ((ret >= 0) && (ret < len))
        {
            //handle the case that cannot write the body out, need to buffer it in respbodybuf.
            appendDynBody(&pBody[ret], len - ret);
        }

        return 0;
    }
    return 0;
}


int HttpSession::onReadEx()
{
    //printf( "^^^HttpSession::onRead()!\n" );
    LS_DBG_L(getLogSession(), "HttpSession::onReadEx(), state: %d!",
             getState());

    int ret = 0;
    switch (getState())
    {
    case HSS_WAITING:
        HttpStats::decIdleConns();
        setState(HSS_READING);
        //fall through
    case HSS_READING:
        if (getFlag(HSF_SUSPENDED))
            getStream()->wantRead(0);
        else
            ret = smProcessReq();
//         ret = readToHeaderBuf();
//         LS_DBG_L(getLogSession(), "readToHeaderBuf() return %d.", ret);
//         if ((!ret)&&( m_request.getStatus() == HttpReq::HEADER_OK ))
//         {
//             clearFlag(HSF_URI_PROCESSED);
//             setProcessState(HSPS_NEW_REQ);
//             ret = smProcessReq();
//             LS_DBG_L(getLogSession(), "smProcessReq() return %d.", ret);
//         }
        break;

    case HSS_COMPLETE:
        break;

    case HSS_READING_BODY:
    case HSS_PROCESSING:
    case HSS_WRITING:
        if (m_request.getBodyBuf() && !testFlag(HSF_REQ_BODY_DONE)
            && !(m_iFlag & HSF_HANDLER_DONE))
        {
            ret = readReqBody();
            //TODO: This is a temp fix.  Cannot put smProcessReq in reqBodyDone
            // because it will call state machine twice, results in seg fault.
            // Basically, need to call state machine once it's finished reading
            if (m_processState == HSPS_HKPT_RCVD_REQ_BODY_PROCESSING
                || m_processState == HSPS_HKPT_RCVD_REQ_BODY)
                ret = smProcessReq();
            //if (( !ret )&& testFlag( HSF_REQ_BODY_DONE ))
            //    ret = resumeHandlerProcess();
            break;
        }
        else
        {
            if (getStream())
            {
                if (getStream()->detectClose())
                    return 0;
            }
        }
        //fall through
    default:
        getStream()->wantRead(0);
        return 0;
    }

    runAllCallbacks();
    //processPending( ret );
//    printf( "onRead loops %d times\n", iCount );
    if (getState() == HSS_TOBERECYCLED)
    {
        releaseResources();
        recycle();
    }

    return 0;
}


int HttpSession::doWrite()
{
    int ret = 0;
    LS_DBG_L(getLogSession(), "HttpSession::doWrite()!");
    ret = flush();
    if (ret)
        return ret;

    if (m_iFlag & HSF_RESUME_SSI)
    {
        m_iFlag &= ~HSF_RESUME_SSI;
        resumeSSI();
        return 0;
    }

    if (m_pCurSubSession)
    {
        if (m_pCurSubSession->getState() != HSS_COMPLETE)
        {
            ret = m_pCurSubSession->onWriteEx();
            if (ret != 0)
                return ret;
            if (!m_pCurSubSession->getStream()->isWantWrite())
                getStream()->wantWrite(0);
        }
        if (m_pCurSubSession->m_iFlag & HSF_HANDLER_DONE)
            onSubSessionRespIncluded(m_pCurSubSession);
        return 0;
    }

    if (m_pHandler
        && !(testFlag(HSF_HANDLER_DONE | HSF_HANDLER_WRITE_SUSPENDED)))
    {
        ret = m_pHandler->onWrite(this);
        LS_DBG_H(getLogSession(), "m_pHandler->onWrite() returned %d.\n", ret);
    }
    else
        suspendWrite();

    if (ret == 0 &&
        ((testFlag(HSF_HANDLER_DONE |
                   HSF_RECV_RESP_BUFFERED |
                   HSF_SEND_RESP_BUFFERED)) == HSF_HANDLER_DONE))
    {
        if (getRespBodyBuf() && !getRespBodyBuf()->empty())
            return 1;

        setFlag(HSF_RESP_FLUSHED, 0);
        flush();

        if(getFlag(HSF_SAVE_STX_FILE_CACHE))
        {
            HttpVHost *host = getReq()->getVHost();
            //Need to verify can be cached
            if (host && m_sendFileInfo.getFileData())
            {
                host->addUrlStaticFileMatch(m_sendFileInfo.getFileData(),
                                            getReq()->getOrgReqURL(),
                                            getReq()->getOrgReqURLLen());
                LS_DBG_L( getLogSession(), "[static file cache] create cache." );
            }
            setFlag(HSF_SAVE_STX_FILE_CACHE, 0);
        }
    }
    else if (ret == -1)
        getStream()->tobeClosed();

    return ret;
}


void HttpSession::postWriteEx()
{
    if (HSS_COMPLETE == getState())
    {
        ;//nextRequest();
        //runAllEventNotifier();
        //CallbackQueue::getInstance().scheduleSessionEvent(stx_nextRequest, 0, this);
    }
    else if (testFlag(HSF_HANDLER_DONE) && (m_pHandler))
    {
        if (cleanUpHandler(HSPS_END) == LS_OK)
            incReqProcessed();
    }

}


int HttpSession::onWriteEx()
{
    //printf( "^^^HttpSession::onWrite()!\n" );
    int ret = 0;

    if (m_iFlag & HSF_CUR_SUB_SESSION_DONE)
        curSubSessionCleanUp();

    switch (getState())
    {
    case HSS_THROTTLING:
        ret = handlerProcess(m_request.getHttpHandler());
        if ((ret) && (getStream()->getState() < HIOS_SHUTDOWN))
            httpError(ret);

        break;
    case HSS_WRITING:
        doWrite();
        break;
    case HSS_COMPLETE:
        getStream()->wantWrite(false);
        break;
    case HSS_REDIRECT:
    case HSS_EXT_REDIRECT:
    case HSS_HTTP_ERROR:
        if (isRespHeaderSent())
        {
            closeSession();
            //runAllEventNotifier();
            return LS_FAIL;
        }

        restartHandlerProcess();
        suspendWrite();
        doRedirect();
        break;
    default:
        suspendWrite();
        break;
    }

    postWriteEx();

    //processPending( ret );
    runAllCallbacks();
    return 0;
}


void HttpSession::setHandler(ReqHandler *pHandler)
{
    if (m_pHandler)
        m_pHandler->cleanUp(this);
    m_pHandler = pHandler;
}


int HttpSession::cleanUpHandler(HSPState nextStateAfterMtEnd)
{
    if (getMtFlag(HSF_MT_HANDLER|HSF_MT_END) == HSF_MT_HANDLER)
    {
        if (nextStateAfterMtEnd != HSPS_END)
        {
            setProcessState(HSPS_WAIT_MT_CANCEL);
            getMtSessData()->m_mtEndNextState = nextStateAfterMtEnd;
        }
        if (cancelMtHandler() == LS_AGAIN)
            return LS_AGAIN;
    }
    ReqHandler *pHandler = m_pHandler;
    m_pHandler = NULL;
    pHandler->cleanUp(this);
    return LS_OK;
}


int HttpSession::onCloseEx()
{
    closeSession();
    return 0;
}


void HttpSession::releaseSsiRuntime()
{
    while (m_pSsiStack)
        popSsiStack();
    if (m_pSsiRuntime)
    {
        if (!(m_iFlag & HSF_SUB_SESSION))
            delete m_pSsiRuntime;
        m_pSsiRuntime = NULL;
    }
}


void HttpSession::releaseResources()
{
    if (m_pHandler)
    {
        if (cleanUpHandler(HSPS_RELEASE_RESOURCE))
            return;
        incReqProcessed();
    }
    m_iFlag &= ~HSF_URI_MAPPED;

    releaseReqParser();

    if (m_pChunkOS)
    {
        m_pChunkOS->reset();
        releaseChunkOS();
    }

    if (m_pChunkIS)
    {
        HttpResourceManager::getInstance().recycle(m_pChunkIS);
        m_pChunkIS = NULL;
    }

    m_sendFileInfo.release();
    m_response.reset(1);
    m_request.reset();
    releaseMtSessData();

    m_pVHostAcl = NULL;
    m_iVHostAccess = 0;

    m_lReqTime = DateTime::s_curTime;
    m_iReqTimeUs = DateTime::s_curTimeUs;
    m_iSubReqSeq = 0;

    if (getRespBodyBuf())
        releaseRespBody();

    if (getGzipBuf())
        releaseGzipBuf();

    //For reuse condition, need to reset the sessionhooks
    m_sessionHooks.reset();

    EvtcbQue::getInstance().removeSessionCb(this);
    resetBackRefPtr();
}


void HttpSession::closeSession()
{
    //++m_sn;
    if (m_pHandler)
    {
        if (cleanUpHandler(HSPS_CLOSE_SESSION) == LS_AGAIN)
            return;
        incReqProcessed();
    }
    ls_atomic_fetch_add(&m_sn, 1);
    // ls_atomic_setint(&m_sessSeq, ls_atomic_add_fetch(&s_m_sessSeq, 1)); // ok to overflow / wrap
    LS_DBG_L("[T%d %s] sess seq now %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, ls_atomic_fetch_add(&m_sessSeq, 0));

    getStream()->tobeClosed();
    if (getStream()->isReadyToRelease())
        return;

    if (testFlag(HSF_HOOK_SESSION_STARTED))
    {
        if (m_sessionHooks.isEnabled(LSI_HKPT_HTTP_END))
            m_sessionHooks.runCallbackNoParam(LSI_HKPT_HTTP_END, (LsiSession *)this);
    }

    m_iFlag &= ~HSF_URI_MAPPED;

    m_request.keepAlive(0);

    logAccess(getStream()->getFlag(HIO_FLAG_PEER_SHUTDOWN));

    releaseResources();

    LsiapiBridge::releaseModuleData(LSI_DATA_HTTP, getModuleData());

    //For reuse condition, need to reset the sessionhooks
    m_sessionHooks.reset();

    LS_DBG_M(getLogSession(), "calling removeSessionCb on this %p\n", this);
    EvtcbQue::getInstance().removeSessionCb(this);


    //if (!(testFlag(HSF_SUB_SESSION)))
    //    getStream()->wantWrite(0);
    getStream()->handlerReadyToRelease();
    getStream()->shutdown();
    resetBackRefPtr();
}


void HttpSession::recycle()
{
    LS_DBG_M(getLogSession(), "calling removeSessionCb on this %p\n", this);
    EvtcbQue::getInstance().removeSessionCb(this);

    if (getReqParser() && !getMtFlag(HSF_MT_HANDLER))
    {
        releaseReqParser();
    }
    m_sExtCmdResBuf.clear();

    resetBackRefPtr();

    if (getFlag(HSF_AIO_READING) || getMtFlag(HSF_MT_HANDLER))
    {
        if (getMtFlag(HSF_MT_HANDLER))
        {
            LS_DBG_M(getLogSession(), "session %p is still in use by MT_HANDLER, postone recycling.\n", this);
            setMtFlag(HSF_MT_RECYCLE);
        }
        if (getFlag(HSF_AIO_READING))
        {
            LS_DBG_M(getLogSession(), "Set AIO Cancel Flag!\n");
            m_pAiosfcb->setFlag(AIOSFCB_FLAG_CANCEL);
        }
        m_iState = HSS_RECYCLING;
        detachStream();
        return;
    }
    ls_atomic_add(&m_sn, 1);
    ls_atomic_setint(&m_sessSeq, ls_atomic_add_fetch(&s_m_sessSeq, 1)); // ok to overflow / wrap
    LS_DBG_L("[T%d %s] sess seq now %d\n", ls_thr_seq(), __PRETTY_FUNCTION__, ls_atomic_fetch_add(&m_sessSeq, 0));
    LS_DBG_H(getLogSession(), "HttpSession::recycle() %p\n", this);
#ifdef LS_AIO_USE_AIO
    if (m_pAiosfcb != NULL)
    {
        m_pAiosfcb->reset();
        HttpResourceManager::getInstance().recycle(m_pAiosfcb);
        m_pAiosfcb = NULL;
    }
#endif
    m_iState = HSS_FREE;
    detachStream();
    if (!trylockMtRace())
        LS_DBG_M(getLogSession(), "[Tm] ok trylockMtRace.");
    else
        LS_DBG_M(getLogSession(), "[Tm] failed trylockMtRace.");
    if (0 == pthread_equal(pthread_self(), m_lockMtHolder))
    {
        LS_ERROR(getLogSession(),
                 "BUG: could not lock MtRace and we don't own it! (holder %ld)",
                 m_lockMtHolder);
    }
    assert(pthread_equal(pthread_self(), m_lockMtHolder) && "CAN'T LOCK MtRace!");
    HttpResourceManager::getInstance().recycle(this);
}


void HttpSession::setupChunkOS(int nobuffer)
{
    if (getStream()->isSpdy() || (m_iFlag & HSF_SUB_SESSION))
        return;
    m_response.setContentLen(LSI_RSP_BODY_SIZE_CHUNKED);
    if ((m_request.getVersion() == HTTP_1_1) && (nobuffer != 2))
    {
        m_response.appendChunked();
        LS_DBG_L(getLogSession(), "Use CHUNKED encoding!");
        if (m_pChunkOS)
            releaseChunkOS();
        m_pChunkOS = HttpResourceManager::getInstance().getChunkOutputStream();
        m_pChunkOS->setStream(getStream());
        m_pChunkOS->open();
        if (!nobuffer)
            m_pChunkOS->setBuffering(1);
    }
    else
        m_request.keepAlive(false);
}


void HttpSession::releaseChunkOS()
{
    HttpResourceManager::getInstance().recycle(m_pChunkOS);
    m_pChunkOS = NULL;
}

#if !defined( NO_SENDFILE )

int HttpSession::chunkSendfile(int fdSrc, off_t off, off_t size)
{
    int written;

    off_t begin = off;
    off_t end = off + size;
    short &left = m_pChunkOS->getSendFileLeft();
    while (off < end)
    {
        if (left == 0)
            m_pChunkOS->buildSendFileChunk(end - off);
        written = m_pChunkOS->flush();
        if (written < 0)
            return written;
        else if (written > 0)
            break;
        written = getStream()->sendfile(fdSrc, off, left);

        if (written > 0)
        {
            left -= written;
            off += written;
            if (left == 0)
                m_pChunkOS->appendCRLF();
        }
        else if (written < 0)
            return written;
        if (written == 0)
            break;
    }
    return off - begin;
}


off_t HttpSession::writeRespBodySendFile(int fdFile, off_t offset,
        off_t size)
{
    off_t written;
    if (m_pChunkOS)
        written = chunkSendfile(fdFile, offset, size);
    else if (HttpServerConfig::getInstance().getUseSendfile() == 2)
    {
        m_pAiosfcb->setReadFd(fdFile);
        m_pAiosfcb->setOffset(offset);
        m_pAiosfcb->setSize(size);
        m_pAiosfcb->setRet(0);
        m_pAiosfcb->setUData(this);
        return getStream()->aiosendfile(m_pAiosfcb);
    }
    else
        written = getStream()->sendfile(fdFile, offset, size);

    if (written > 0)
    {
        m_response.written(written);
        LS_DBG_M(getLogSession(), "Response body sent: %lld.\n",
                 (long long)m_response.getBodySent());
    }
    return written;

}
#endif


/**
  * @return -1 error
  *          0 complete
  *          1 continue
  */



int HttpSession::writeRespBodyDirect(const char *pBuf, int size)
{
    int written;
    if (m_pChunkOS)
        written = m_pChunkOS->write(pBuf, size);
    else
        written = getStream()->write(pBuf, size);
    if (written > 0)
    {
        LS_DBG_H(getLogSession(), "writeRespBodyDirect(): write(%p, %d) written %d, total sent: %lld\n",
                 pBuf, size, written, (long long)m_response.getBodySent() + written);
        m_response.written(written);
    }
    return written;
}


int HttpSession::isExtAppNoAbort()
{
    if (getFlag(HSF_NO_ABORT))
        return 1;
    return 0;
}


static char s_errTimeout[] =
    "<html><head><title>500 Internal Server Error</title></head><body>\n"
    "<h2>Request Timeout</h2>\n"
    "<p>This request takes too long to process, it is timed out by the server. "
    "If it should not be timed out, please contact administrator of this web site "
    "to increase 'Connection Timeout'.\n"
    "</p>\n"
//    "<hr />\n"
//    "Powered By LiteSpeed Web Server<br />\n"
//    "<a href='http://www.litespeedtech.com'><i>http://www.litespeedtech.com</i></a>\n"
    "</body></html>\n";


int HttpSession::detectKeepAliveTimeout(int delta)
{
    const HttpServerConfig &config = HttpServerConfig::getInstance();
    int c = 0;
    if (delta >= config.getKeepAliveTimeout())
    {
        LS_DBG_M(getLogSession(), "Keep-alive timed out, close conn!");
        c = 1;
    }
    else if ((delta > 2) && (m_iReqServed != 0))
    {
        if (ConnLimitCtrl::getInstance().getConnOverflow())
        {
            LS_DBG_M(getLogSession(), "Too many connections, close conn!");
            c = 1;

        }
        else if ((int)getClientInfo()->getConns() >
                 (ClientInfo::getPerClientSoftLimit() >> 1))
        {
            LS_DBG_M(getLogSession(), "Number of connections is over the soft "
                     "limit, close conn!");
            c = 1;
        }
    }
    if (c)
    {
        HttpStats::decIdleConns();
        getStream()->close();
    }
    return c;
}


int HttpSession::detectConnectionTimeout(int delta)
{
    const HttpServerConfig &config = HttpServerConfig::getInstance();
    if ((uint32_t)(DateTime::s_curTime) > getStream()->getActiveTime() +
        (uint32_t)(config.getConnTimeout()))
    {
//         if (pNtwkIOLink->getfd() != pNtwkIOLink->getPollfd()->fd)
//             LS_ERROR(getLogSession(),
//                      "BUG: fd %d does not match fd %d in pollfd!",
//                      pNtwkIOLink->getfd(), pNtwkIOLink->getPollfd()->fd);

        if ((m_response.getBodySent() == 0)// || !pNtwkIOLink->getEvents())
            || !getStream()->getFlag(HIO_FLAG_WANT_WRITE|HIO_FLAG_WANT_READ))
        {
            LS_INFO(getLogSession(), "Connection idle time too long: %ld while"
                    " in state: %d watching for event: %d, close!",
                    DateTime::s_curTime - getStream()->getActiveTime(),
                    getState(), getStream()->getFlag());
            m_request.dumpHeader();
            if (m_pHandler)
                m_pHandler->dump();
            if (getState() == HSS_READING_BODY)
            {
                LS_INFO(getLogSession(), "Request body size: %lld, received: %lld.",
                        (long long)m_request.getContentLength(),
                        (long long)m_request.getContentFinished());
            }
        }
        if ((getState() == HSS_PROCESSING) && m_response.getBodySent() == 0)
        {
            if (!isRespHeaderSent())
                httpError(SC_500, s_errTimeout);
            else
            {
                IOVec iov;
                iov.append(s_errTimeout, sizeof(s_errTimeout) - 1);
                getStream()->writev(iov, sizeof(s_errTimeout) - 1);
            }
        }
        else
            getStream()->setAbortedFlag();
        closeSession();
        return 1;
    }
    else
        return 0;
}


int HttpSession::isAlive()
{
    if (getStream()->isSpdy() || (m_iFlag & HSF_SUB_SESSION))
        return 1;

    return !getStream()->detectClose();

}


int HttpSession::detectTimeout()
{
//    LS_DBG_M(getLogger(), "HttpSession::detectTimeout() ");
    int delta = DateTime::s_curTime - m_lReqTime;
    switch (getState())
    {
    case HSS_WAITING:
        return detectKeepAliveTimeout(delta);
    case HSS_READING:
    case HSS_READING_BODY:
    //fall through
    default:
        return detectConnectionTimeout(delta);
    }
    return 0;
}


int HttpSession::onTimerEx()
{
    if (getClientInfo())
    {
        ThrottleControl *pTC = &getClientInfo()->getThrottleCtrl();
        pTC->resetQuotas();
    }
    if (getState() ==  HSS_THROTTLING)
        onWriteEx();
    if (detectTimeout())
        return 1;
    if (m_pHandler)
        m_pHandler->onTimer();
    return 0;
}


void HttpSession::releaseRespBody()
{
    VMemBuf *pRespBodyBuf = getRespBodyBuf();
    if (pRespBodyBuf)
    {
        HttpResourceManager::getInstance().recycle(pRespBodyBuf);
        setRespBodyBuf(NULL);
    }
}


static int writeRespBodyTermination(LsiSession *session, const char *pBuf,
                                    int len)
{
    return ((HttpSession *)session)->writeRespBodyDirect(pBuf, len);
}


int HttpSession::writeRespBody(const char *pBuf, int len)
{
    if (m_sessionHooks.isDisabled(LSI_HKPT_SEND_RESP_BODY))
        return writeRespBodyDirect(pBuf, len);

    return runFilter(LSI_HKPT_SEND_RESP_BODY,
                     (filter_term_fn)writeRespBodyTermination, pBuf, len, 0);
}


void HttpSession::rewindRespBodyBuf()
{
    if (getRespBodyBuf())
    {
        off_t wPos = getRespBodyBuf()->getCurWBlkPos();
        if (wPos < 2048 * 1024)
            return;
        LS_DBG_M(getLogger(),
                 "[%s] Rewind RespBodyBuf, current size: %lld \n",
                 getLogId(), (long long)wPos);
        if (getGzipBuf())
            getGzipBuf()->resetCompressCache();
        else
        {
            getRespBodyBuf()->rewindReadBuf();
            getRespBodyBuf()->rewindWriteBuf();
        }
    }
}


int HttpSession::sendDynBody()
{
    size_t toWrite;
    char *pBuf;

    while (((pBuf = getRespBodyBuf()->getReadBuffer(toWrite)) != NULL)
           && (toWrite > 0))
    {
        int len = toWrite;
        if (m_response.getContentLen() > 0)
        {
            off_t allowed = m_response.getContentLen() - m_lDynBodySent;
            if (len > allowed)
                len = allowed;
            if (len <= 0)
            {
                getRespBodyBuf()->readUsed(toWrite);
                continue;
            }
        }

        int ret = writeRespBody(pBuf, len);
        LS_DBG_M(getLogSession(), "writeRespBody() len = %d, returned %d.\n",
                 len, ret);
        if (ret > 0)
        {
            assert(ret <= len);
            m_lDynBodySent += ret;
            getRespBodyBuf()->readUsed(ret);
            if (ret < len)
            {
                getStream()->wantWrite(1);
                return 1;
            }
        }
        else if (ret == -1)
        {
            getStream()->wantRead(1);
            return LS_FAIL;
        }
        else
        {
            getStream()->wantWrite(1);
            return 1;
        }
    }
    return 0;
}


int HttpSession::setupRespBodyBuf()
{
    if (!getRespBodyBuf())
    {
        LS_DBG_L(getLogSession(), "Allocate response body buffer.");
        setRespBodyBuf(HttpResourceManager::getInstance().getVMemBuf());
        if (!getRespBodyBuf())
        {
            LS_ERROR(getLogSession(), "Failed to obtain VMemBuf. "
                     "Current pool size: %d, capacity: %d.",
                     HttpResourceManager::getInstance().getVMemBufPoolSize(),
                     HttpResourceManager::getInstance().getVMemBufPoolCapacity());
            return LS_FAIL;
        }
        if (getRespBodyBuf()->reinit(
                m_response.getContentLen()))
        {
            LS_ERROR(getLogSession(), "Failed to initialize VMemBuf, "
                     "response body len: %lld.",
                     (long long)m_response.getContentLen());
            return LS_FAIL;
        }
        m_lDynBodySent = 0;
    }
    return 0;
}



inline int HttpSession::useGzip()
{
    if (/*(!m_request.getContextState(RESP_CONT_LEN_SET)
                 ||*/ m_response.getContentLen() > 200)
        return 1;
    else
        return 0;
}


extern int addModgzipFilter(lsi_session_t *session, int isSend,
                            uint8_t compressLevel);
int HttpSession::setupGzipFilter()
{
    if (testFlag(HSF_RESP_HEADER_SENT))
        return 0;

    char gz = m_request.gzipAcceptable();
    int recvhkptNogzip = m_sessionHooks.getFlag(LSI_HKPT_RECV_RESP_BODY) &
                         LSI_FLAG_DECOMPRESS_REQUIRED;
    int  hkptNogzip = (m_sessionHooks.getFlag(LSI_HKPT_RECV_RESP_BODY)
                       | m_sessionHooks.getFlag(LSI_HKPT_SEND_RESP_BODY))
                      & LSI_FLAG_DECOMPRESS_REQUIRED;
    if (gz & (UPSTREAM_GZIP | UPSTREAM_DEFLATE))
    {
        setFlag(HSF_RESP_BODY_GZIPCOMPRESSED);
        if (recvhkptNogzip || hkptNogzip || !(gz & REQ_GZIP_ACCEPT))
        {
            //setup decompression filter at RECV_RESP_BODY filter
            if (addModgzipFilter((LsiSession *)this, 0, 0) == -1)
                return LS_FAIL;
            m_response.getRespHeaders().del(
                HttpRespHeaders::H_CONTENT_ENCODING);
            gz &= ~(UPSTREAM_GZIP | UPSTREAM_DEFLATE);
            clearFlag(HSF_RESP_BODY_GZIPCOMPRESSED);
        }
    }
    else
        clearFlag(HSF_RESP_BODY_GZIPCOMPRESSED);

    if (gz == GZIP_REQUIRED)
    {
        if (!hkptNogzip)
        {
            if (!getRespBodyBuf()->empty())
                getRespBodyBuf()->rewindWriteBuf();
            if (m_response.getContentLen() > 200 ||
                m_response.getContentLen() < 0)
            {
                if (setupGzipBuf() == -1)
                    return LS_FAIL;
            }
        }
        else //turn on compression at SEND_RESP_BODY filter
        {
            if (addModgzipFilter((LsiSession *)this, 1,
                                 HttpServerConfig::getInstance().getCompressLevel()) == -1)
                return LS_FAIL;
            m_response.addGzipEncodingHeader();
            //The below do not set the flag because compress won't update the resp VMBuf to decompressed
            //setFlag(HSF_RESP_BODY_GZIPCOMPRESSED);
        }
    }
    return 0;
}


int HttpSession::setupGzipBuf()
{
    if (getRespBodyBuf())
    {
        LS_DBG_L(getLogSession(), "GZIP the response body in the buffer.");
        if (getGzipBuf())
        {
            if (getGzipBuf()->isStreamStarted())
            {
                getGzipBuf()->endStream();
                LS_DBG_M(getLogSession(), "setupGzipBuf() end GZIP stream.\n");
            }
        }

        if (!getGzipBuf())
            setGzipBuf(HttpResourceManager::getInstance().getGzipBuf());
        if (getGzipBuf())
        {
            getGzipBuf()->setCompressCache(getRespBodyBuf());
            if ((getGzipBuf()->init(GzipBuf::COMPRESSOR_COMPRESS,
                                  HttpServerConfig::getInstance().getCompressLevel()) == 0) &&
                (getGzipBuf()->beginStream() == 0))
            {
                LS_DBG_M(getLogSession(), "setupGzipBuf() begin GZIP stream.\n");
                m_response.setContentLen(LSI_RSP_BODY_SIZE_UNKNOWN);
                m_response.addGzipEncodingHeader();
                m_request.orGzip(UPSTREAM_GZIP);
                setFlag(HSF_RESP_BODY_GZIPCOMPRESSED);
                return 0;
            }
            else
            {
                LS_ERROR(getLogSession(), "Ran out of swapping space while "
                         "initializing GZIP stream!");
                delete getGzipBuf();
                clearFlag(HSF_RESP_BODY_GZIPCOMPRESSED);
                setGzipBuf(NULL);
            }
        }
    }
    return LS_FAIL;
}


void HttpSession::releaseGzipBuf()
{
    GzipBuf *pGzipBuf = getGzipBuf();
    if (pGzipBuf)
    {
        if (pGzipBuf->getType() == GzipBuf::COMPRESSOR_COMPRESS)
            HttpResourceManager::getInstance().recycle(pGzipBuf);
        else
            HttpResourceManager::getInstance().recycleGunzip(pGzipBuf);
        setGzipBuf(NULL);
    }
}



int appendDynBodyTermination(HttpSession *conn, const char *pBuf, int len)
{
    return conn->getResp()->appendDynBody(pBuf, len);
}


int HttpSession::runFilter(int hookLevel,
                           filter_term_fn pfTermination,
                           const char *pBuf, int len, int flagIn)
{
    int ret;
    int bit;
    int flagOut = 0;
    const LsiApiHooks *pHooks = LsiApiHooks::getGlobalApiHooks(hookLevel);
    lsi_param_t param;
    lsi_hookinfo_t hookInfo;
    param.session = (LsiSession *)this;
    hookInfo.hooks = pHooks;
    hookInfo.enable_array = m_sessionHooks.getEnableArray(hookLevel);
    hookInfo.term_fn = pfTermination;
    hookInfo.hook_level = hookLevel;
    param.cur_hook = (void *)pHooks->begin();
    param.hook_chain = &hookInfo;
    param.ptr1 = pBuf;
    param.len1 = len;
    param.flag_out = &flagOut;
    param.flag_in = flagIn;
    ret = LsiApiHooks::runForwardCb(&param);

    if (hookLevel == LSI_HKPT_RECV_RESP_BODY)
        bit = HSF_RECV_RESP_BUFFERED;
    else
        bit = HSF_SEND_RESP_BUFFERED;
    if (flagOut)
        setFlag(bit);
    else
        clearFlag(bit);

    LS_DBG_M(getLogSession(),
             "[%s] input len: %d, flag in: %d, flag out: %d, ret: %d",
             LsiApiHooks::getHkptName(hookLevel), len, flagIn, flagOut, ret);
    return processHkptResult(hookLevel, ret);

}


int HttpSession::appendDynBody(const char *pBuf, int len)
{
    int ret;
    if (!getRespBodyBuf())
    {
        setupDynRespBodyBuf();
        if (!getRespBodyBuf())
            return len;
    }

    if (m_sessionHooks.isDisabled(LSI_HKPT_RECV_RESP_BODY))
        ret = getResp()->appendDynBody(pBuf, len);
    else
        ret = runFilter(LSI_HKPT_RECV_RESP_BODY,
                     (filter_term_fn)appendDynBodyTermination, pBuf, len, 0);
    if (ret > 0)
        setFlag(HSF_RESP_FLUSHED, 0);
    return ret;
}


int HttpSession::appendRespBodyBuf(const char *pBuf, int len)
{
    char *pCache;
    size_t iCacheSize;
    const char *pBufOrg = pBuf;

    while (len > 0)
    {
        pCache = getRespBodyBuf()->getWriteBuffer(iCacheSize);
        if (pCache)
        {
            int ret = len;
            if (iCacheSize < (size_t)ret)
                ret = iCacheSize;
            if (pCache != pBuf)
            {
                LS_DBG_H(getLogSession(), "copy %p bytes %d to body buf %p",
                         pBuf, ret, pCache);
                memmove(pCache, pBuf, ret);
            }
            pBuf += ret;
            len -= ret;
            getRespBodyBuf()->writeUsed(ret);
        }
        else
        {
            LS_ERROR(getLogSession(), "Ran out of swapping space while "
                     "appending to response buffer!");
            return LS_FAIL;
        }
    }
    return pBuf - pBufOrg;
}


int HttpSession::appendRespBodyBufV(const iovec *vector, int count)
{
    char *pCache;
    size_t iCacheSize;
    size_t len  = 0;
    char *pBuf;
    size_t iInitCacheSize;
    pCache = getRespBodyBuf()->getWriteBuffer(iCacheSize);
    assert(iCacheSize > 0);
    iInitCacheSize = iCacheSize;

    if (!pCache)
    {
        LS_ERROR(getLogSession(), "Ran out of swapping space while "
                 "append[V]ing to response buffer!");
        return LS_FAIL;
    }

    for (int i = 0; i < count; ++i)
    {
        len = (*vector).iov_len;
        pBuf = (char *)((*vector).iov_base);
        ++vector;

        while (len > 0)
        {
            int ret = len;
            if (iCacheSize < (size_t)ret)
                ret = iCacheSize;
            memmove(pCache, pBuf, ret);
            pCache += ret;
            iCacheSize -= ret;

            if (iCacheSize == 0)
            {
                getRespBodyBuf()->writeUsed(iInitCacheSize - iCacheSize);
                pCache = getRespBodyBuf()->getWriteBuffer(iCacheSize);
                iInitCacheSize = iCacheSize;
                pBuf += ret;
                len -= ret;
            }
            else
                break;  //quit the while loop
        }
    }

    if (iInitCacheSize > iCacheSize)
        getRespBodyBuf()->writeUsed(iInitCacheSize - iCacheSize);


    return 0;
}


int HttpSession::shouldSuspendReadingResp()
{
    if (getRespBodyBuf())
    {
        int buffered = getRespBodyBuf()->getCurWBlkPos() -
                       getRespBodyBuf()->getCurRBlkPos();
        return ((buffered >= 1024 * 1024) || (buffered < 0));
    }
    return 0;
}


void HttpSession::resetRespBodyBuf()
{

    {
        if (getGzipBuf())
            getGzipBuf()->resetCompressCache();
        else
        {
            getRespBodyBuf()->rewindReadBuf();
            getRespBodyBuf()->rewindWriteBuf();
        }
    }
}


/**
 * The below errors are different, the page is a whole page,
 * the 2nd error is just appended to the content which is already sent
 */
static char achError413Page[] =
    "<HTML><HEAD><TITLE>Request Entity Too Large</TITLE></HEAD>"
    "<BODY BGCOLOR=#FFFFFF><HR><H1>413 Request Entity Too Large</H1>"
    "The dynamic response body size is over the limit. The limit is "
    "set in the key 'maxDynRespSize' located in the tuning section of"
    " the server configuration, and labeled 'max dynamic response body"
    " size'.<HR></BODY></HTML>";

static char achOverBodyLimitError[] =
    "<p>The dynamic response body size is over the limit, the response "
    "will be truncated by the web server. The limit is set in the key "
    "'maxDynRespSize' located in the tuning section of the server "
    "configuration, and labeled 'max dynamic response body size'.";


size_t HttpSession::getRespBodyBuffered()
{
    VMemBuf *pRespBodyBuf = getRespBodyBuf();
    return pRespBodyBuf ? pRespBodyBuf->writeBufSize(): HS_RESP_NOT_READY;
}


int HttpSession::checkRespSize(int nobuffer)
{
    int ret = 0;
    VMemBuf *pRespBodyBuf = getRespBodyBuf();
    if (pRespBodyBuf)
    {
        int curLen = pRespBodyBuf->writeBufSize();
        if ((nobuffer && (curLen > 0)) || (curLen > 1460))
        {
            if (curLen > HttpServerConfig::getInstance().getMaxDynRespLen())
            {
                LS_WARN(getLogSession(), "The dynamic response body size is"
                        " over the limit, abort!");
                appendDynBody(achOverBodyLimitError, sizeof(achOverBodyLimitError) - 1);
                if (m_pHandler)
                    m_pHandler->abortReq();
                ret = 1;
                return ret;
            }

            flush();
        }
    }
    return ret;
}

int HttpSession::checkRespSize()
{
    if (getRespBodyBuf() && m_response.getBodySent() >
            HttpServerConfig::getInstance().getMaxDynRespLen())
    {
        LS_WARN(getLogSession(), "The dynamic response body size is"
                    " over the limit, abort!");
        appendDynBody(achOverBodyLimitError, sizeof(achOverBodyLimitError) - 1);
        if (m_pHandler)
            m_pHandler->abortReq();
        return 1;
    }
    else
        return 0;
}


int HttpSession::createOverBodyLimitErrorPage()
{
    int len = sizeof(achError413Page) - 1;
    appendDynBody(achError413Page, len);
    return len;
}

//return 0 , caller should continue
//return !=0, the request has been redirected, should break the normal flow

int HttpSession::respHeaderDone()
{
    if (testAndSetFlag(HSF_RESP_HEADER_DONE) == 0)
        return 0;
    LS_DBG_M(getLogSession(), "Response header finished!");

    int ret = 0;
    if (m_sessionHooks.isEnabled(LSI_HKPT_RCVD_RESP_HEADER))
        ret = m_sessionHooks.runCallbackNoParam(LSI_HKPT_RCVD_RESP_HEADER,
                                                (LsiSession *)this);

    if (processHkptResult(LSI_HKPT_RCVD_RESP_HEADER, ret))
        return 1;

    if (m_pMtSessData && !m_sessionHooks.isDisabled(LSI_HKPT_RECV_RESP_BODY))
    {
        m_pMtSessData->m_mtWrite.m_writeMode = DBL_BUF;
    }
    return 0;
}


int HttpSession::endResponseInternal(int success)
{
    int ret = 0;
    LS_DBG_L(getLogSession(), "endResponseInternal()");

    if (!getFlag(HSF_RESP_HEADER_DONE))
    {
        if (respHeaderDone())
            return 1;
    }

    if (!isNoRespBody() && m_sessionHooks.isEnabled(LSI_HKPT_RECV_RESP_BODY))
    {
        ret = runFilter(LSI_HKPT_RECV_RESP_BODY,
                        (filter_term_fn)appendDynBodyTermination,
                        NULL, 0, LSI_CBFI_EOF);
    }

    if (getGzipBuf())
    {
        if (getGzipBuf()->endStream())
        {
            LS_ERROR(getLogSession(), "Ran out of swapping space while "
                     "terminating GZIP stream!");
            ret = -1;
        }
        else
            LS_DBG_M(getLogSession(), "endResponse() end GZIP stream.");
    }

    if (!ret && m_sessionHooks.isEnabled(LSI_HKPT_RCVD_RESP_BODY))
    {
        ret = m_sessionHooks.runCallbackNoParam(LSI_HKPT_RCVD_RESP_BODY,
                                                (LsiSession *)this);
//         ret = m_sessionHooks.runCallback(LSI_HKPT_RCVD_RESP_BODY, (LsiSession *)this,
//                                          NULL, 0, NULL, success?LSI_CBFI_RESPSUCC:0 );
        ret = processHkptResult(LSI_HKPT_RCVD_RESP_BODY, ret);
    }
    return ret;
}


int HttpSession::endResponse(int success)
{
    //If already called once, just return
    if (testAndSetFlag(HSF_HANDLER_DONE) == 0)
        return 0;

    if (m_iFlag & HSF_EXEC_EXT_CMD)
    {
        m_iFlag &= ~HSF_EXEC_EXT_CMD;
        resumeSSI();
        return 0;
    }

    LS_DBG_M(getLogSession(), "endResponse( %d )", success);

    int ret = 0;

    ret = endResponseInternal(success);
    if (ret)
        return ret;
    // FIXME ols orig code
//     if (!isRespHeaderSent() && (m_response.getContentLen() < 0))
    if (!m_request.noRespBody() && !isRespHeaderSent()
        && ((m_response.getContentLen() == LSI_RSP_BODY_SIZE_UNKNOWN)
            || m_request.getContextState(RESP_CONT_LEN_SET)))
    {
        // header is not sent yet, no body sent yet.
        //content length = dynamic content + static send file
        long size = m_sendFileInfo.getRemain();
        if (getRespBodyBuf())
            size += getRespBodyBuf()->getCurWOffset();

        m_response.setContentLen(size);
    }

    setState(HSS_WRITING);
    setFlag(HSF_RESP_FLUSHED, 0);
    if (getStream()->isSpdy())
        getStream()->continueWrite();
    else
        ret = flush();

    return ret;
}


int HttpSession::flushBody()
{
    int ret = 0;

    if (testFlag(HSF_RECV_RESP_BUFFERED))
    {
        if (m_sessionHooks.isEnabled(LSI_HKPT_RECV_RESP_BODY))
        {
            ret = runFilter(LSI_HKPT_RECV_RESP_BODY,
                            (filter_term_fn)appendDynBodyTermination,
                            NULL, 0, LSI_CBFI_FLUSH);
        }
    }
    if (!(testFlag(HSF_HANDLER_DONE)))
    {
        if (getGzipBuf() && getGzipBuf()->isStreamStarted())
            getGzipBuf()->flush();

    }
    if ((getRespBodyBuf()
         && !getRespBodyBuf()->empty())/*&&( isRespHeaderSent() )*/)
        ret = sendDynBody();
    if ((ret == 0) && (m_sendFileInfo.getRemain() > 0))
        ret = sendStaticFile(&m_sendFileInfo);
    if (ret > 0)
        return LS_AGAIN;

    if (m_sessionHooks.isEnabled(LSI_HKPT_SEND_RESP_BODY))
    {
        int flush = LSI_CBFI_FLUSH;
        if (((testFlag(HSF_HANDLER_DONE | HSF_RECV_RESP_BUFFERED))) ==
             HSF_HANDLER_DONE)
            flush = LSI_CBFI_EOF;
        ret = runFilter(LSI_HKPT_SEND_RESP_BODY,
                        (filter_term_fn)writeRespBodyTermination,
                        NULL, 0, flush);
        if (testFlag(HSF_SEND_RESP_BUFFERED))
            return LS_AGAIN;
    }

    //int nodelay = 1;
    //::setsockopt( getfd(), IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof( int ));
    if (!ret && m_pChunkOS)
    {
//            if ( !m_request.getSsiRuntime() )
        {
            if ((testFlag(HSF_HANDLER_DONE | HSF_RECV_RESP_BUFFERED
                          | HSF_SEND_RESP_BUFFERED | HSF_CHUNK_CLOSED))
                == HSF_HANDLER_DONE)
            {
                m_pChunkOS->close();
                LS_DBG_L(getLogSession(), "Chunk closed!");
                setFlag(HSF_CHUNK_CLOSED);
            }
            if (m_pChunkOS->flush() != 0)
                return LS_AGAIN;
        }
    }
    return LS_DONE;
}


int HttpSession::call_nextRequest(evtcbtail_t *p, long , void *)
{
    HttpSession *pSession = (HttpSession *)p;
    pSession->nextRequest();
    return 0;
}


inline void HttpSession::markComplete(bool nowait)
{
    LS_DBG_L(getLogSession(), "mark HSS_COMPLETE.");

    setState(HSS_COMPLETE);
    if (getStream()->isSpdy())
    {
        getStream()->shutdown();
    }

    if (m_pHandler)
    {
        if(cleanUpHandler(HSPS_NEXT_REQUEST) == LS_AGAIN)
            return;
        incReqProcessed();
    }
    if (!(m_iFlag & HSF_SUB_SESSION))
        EvtcbQue::getInstance().schedule(call_nextRequest,
                                        this,
                                        getSn(),
                                        NULL, nowait);
}


int HttpSession::flush()
{
    int ret = LS_DONE;
    if (getStream() == NULL)
    {
        LS_DBG_L(getLogSession(), "HttpSession::flush(), getStream() == NULL!");
        return LS_FAIL;
    }
    LS_DBG_L(getLogSession(), "HttpSession::flush()!");


    if (m_iState == HSS_COMPLETE)
        return 1;
    else if (m_iState == HSS_IO_ERROR)
        return -1;

    if (getFlag(HSF_SUSPENDED))
    {
        LS_DBG_L(getLogSession(), "HSF_SUSPENDED flag is set, cannot flush.");
        suspendWrite();
        return LS_AGAIN;
    }

    if (getFlag(HSF_RESP_FLUSHED))
    {
        LS_DBG_L(getLogSession(), "HSF_RESP_FLUSHED flag is set, skip flush.");
        suspendWrite();
        return LS_DONE;
    }
    if (isNoRespBody() && !getFlag(HSF_HANDLER_DONE))
    {
        ret = endResponseInternal(1);
        if (ret)
            return LS_AGAIN;
    }
    else if (getFlag(HSF_HANDLER_DONE | HSF_RESP_WAIT_FULL_BODY) ==
             HSF_RESP_WAIT_FULL_BODY)
    {
        LS_DBG_L(getLogSession(), "Cannot flush as response is not finished!");
        return LS_DONE;
    }

    if (!isRespHeaderSent())
    {
        if (sendRespHeaders())
            return LS_DONE;
    }

    if (!isNoRespBody())
    {
        ret = flushBody();
        LS_DBG_L(getLogSession(), "flushBody() return %d", ret);
    }

    if (getFlag(HSF_AIO_READING))
        return ret;

    if (!ret)
        ret = getStream()->flush();
    if (ret == LS_DONE)
    {
        //setFlag(HSF_RESP_FLUSHED, 1);
        if (getFlag(HSF_HANDLER_DONE
                    | HSF_SUSPENDED
                    | HSF_RECV_RESP_BUFFERED
                    | HSF_SEND_RESP_BUFFERED) == HSF_HANDLER_DONE)
        {
            LS_DBG_L(getLogSession(), "Set the HSS_COMPLETE flag.");
            if (getState() != HSS_COMPLETE)
                markComplete(false);
            return ret;
        }
        else
        {
            LS_DBG_L(getLogSession(), "Set the HSF_RESP_FLUSHED flag.");
            setFlag(HSF_RESP_FLUSHED, 1);
            if (getMtFlag(HSF_MT_HANDLER))
                mtNotifyWriters();
            return LS_DONE;
        }
    }
    else if (ret == -1)
    {
        LS_DBG_L(getLogger(), "[%s] set state to HSS_IO_ERROR.", getLogId());
        setState(HSS_IO_ERROR);
    }

    if ((testFlag(HSF_HANDLER_WRITE_SUSPENDED) == 0))
        getStream()->wantWrite(1);

    return ret;
}


void HttpSession::addLocationHeader()
{
    HttpRespHeaders &headers = m_response.getRespHeaders();
    const char *pLocation = m_request.getLocation();
//     int len = m_request.getLocationLen();
    headers.add(HttpRespHeaders::H_LOCATION, "", 0);
    if (*pLocation == '/')
    {
        const char *pHost = m_request.getHeader(HttpHeader::H_HOST);
        if (*pHost)
        {
            if (isHttps())
                headers.appendLastVal("https://", 8);
            else
                headers.appendLastVal("http://", 7);
            headers.appendLastVal(pHost, m_request.getHeaderLen(HttpHeader::H_HOST));
        }
    }
    headers.appendLastVal(pLocation, m_request.getLocationLen());
}


void HttpSession::prepareHeaders()
{
    HttpRespHeaders &headers = m_response.getRespHeaders();
    headers.addCommonHeaders();

    if (m_request.isCfIpSet())
        headers.addTruboCharged();

    if (m_request.getAuthRequired())
        m_request.addWWWAuthHeader(headers);

    if (m_request.getLocationLen() > 0 && m_request.getLocation() != NULL)
        addLocationHeader();

    m_request.applyHeaderOps(this, &headers);
}

/**
 * Every 8 bit cover 1 byte(2 bytes HEX)
 *
 * xxxxxxxx xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx ...
 * 0      7 8      15        23        31  ...
 *
 * Example
 * 1       2          3         4
 * 1000000 01000000   11000000  00100000
 *
 */
void HttpSession::addBittoCookie(AutoStr2 &cookie, int bit)
{
    int i;
    int len = cookie.len();
    char s[3] = {'0', '0', 0};

    for (i = len/2; i <= bit / 8; ++i)
        cookie.append(s, 2);

    i = bit / 8;
    s[0] = *(cookie.c_str() + i * 2);
    s[1] = *(cookie.c_str() + i * 2 + 1);

    uint8_t bit8 = (uint8_t)strtoul(s, NULL, 16);
    bit8 |= 1 << (bit % 8);
    sprintf(s, "%02x", bit8);
    char *p = (char *)(cookie.c_str());
    memcpy(p + i * 2, s, 2);
}

/**
 * 1 fot true, 0 for false
 */
int HttpSession::isCookieHaveBit(const char *cookie, int bit)
{
    int size = strlen(cookie);
    if (bit / 8 >= size/2)
        return 0;

    char s[3] = {0};
    int i= bit / 8;
    s[0] = cookie[i * 2];
    s[1] = cookie[i * 2 + 1];
    uint8_t bit8 = (uint8_t)strtoul(s, NULL, 16);
    if (bit8 & (1 << (bit % 8)))
        return 1;
    else
        return 0;
}

int HttpSession::pushToClient(const char *pUri, int uriLen, AutoStr2 &cookie)
{
    char buf[16384];
    if (*pUri != '/')
    {
        int len = m_request.toLocalAbsUrl(pUri, uriLen, buf, sizeof(buf) - 1);
        if (len == -1)
            return -1;
        uriLen = len;
    }
    else
    {
        int maxSz = sizeof(buf) - 1;
        if (uriLen > maxSz)
            uriLen = maxSz;

        memcpy(buf, pUri, uriLen);
    }
    pUri = buf;
    buf[uriLen] = 0;

    HttpVHost *pVHost = (HttpVHost *) m_request.getVHost();
    int id = pVHost->getIdBitOfUrl(pUri);

    if (id == -1)
        id = pVHost->addUrlToUrlIdHash(pUri);
    else
    {
        if (cookie.len() > 0)
        {
            int state = isCookieHaveBit(cookie.c_str(), id);
            if (state == 1)
                return 0;
        }
    }

    ls_str_t uri;
    ls_str_t host;
    uri.ptr = (char *)pUri;
    uri.len = uriLen;
    host.ptr = (char *)m_request.getHeader(HttpHeader::H_HOST);
    host.len = m_request.getHeaderLen(HttpHeader::H_HOST);
    ls_strpair_t extraHeaders[8];
    ls_strpair_t *p = extraHeaders;
//     URICache::iterator iter;
//     URICache *pCache = m_request.getVHost()->getURICache();
//     char *pEnd = (char *)pUri + uriLen;
//     char ch = *pEnd;
//     *pEnd = 0;
//     iter = pCache->find(pUri);
//     *pEnd = ch;
//     if (iter != pCache->end())
//     {
//         StaticFileCacheData *pData = iter.second()->getStaticCache();
//         if (pData && pData->getETagValue())
//         {
//             p->key.ptr = (char *)HttpHeader::getHeaderNameLowercase(HttpHeader::H_IF_MATCH);
//             p->key.len = 8;
//             p->val.ptr = (char *)pData->getETagValue();
//             p->val.len = pData->getETagValueLen();
//             p++;
//         }
//     }

    p->val.len = m_request.getHeaderLen(HttpHeader::H_ACC_ENCODING);
    if (p->val.len > 0)
    {
        p->val.ptr = (char *)m_request.getHeader(HttpHeader::H_ACC_ENCODING);
        p->key.ptr = (char *)HttpHeader::getHeaderNameLowercase(
                                        HttpHeader::H_ACC_ENCODING);
        p->key.len = HttpHeader::getHeaderStringLen(HttpHeader::H_ACC_ENCODING);
        p++;
    }

    p->val.len = m_request.getHeaderLen(HttpHeader::H_USERAGENT);
    if (p->val.len > 0)
    {
        p->val.ptr = (char *)m_request.getHeader(HttpHeader::H_USERAGENT);
        p->key.ptr = (char *)HttpHeader::getHeaderNameLowercase(
                                        HttpHeader::H_USERAGENT);
        p->key.len = HttpHeader::getHeaderStringLen(HttpHeader::H_USERAGENT);
        p++;
    }
    char referer[16484];
    p->val.len = snprintf(referer, sizeof(referer), "https://%.*s%.*s",
                          (int)host.len, host.ptr, m_request.getOrgReqURLLen(),
                          m_request.getOrgReqURL());
    p->val.ptr = referer;
    p->key.ptr = (char *)HttpHeader::getHeaderNameLowercase(
                                        HttpHeader::H_REFERER);
    p->key.len = HttpHeader::getHeaderStringLen(HttpHeader::H_REFERER);
    p++;

    memset(p, 0, sizeof(*p));
    p = extraHeaders;

    addBittoCookie(cookie, id);
    return getStream()->push(&uri, &host, extraHeaders);
}


int HttpSession::processOneLink(const char *p, const char *pEnd,
                                AutoStr2 &cookie)
{
    p = (const char *)memchr(p, '<', pEnd - p);
    if (!p)
        return 0;

    const char *pUrlBegin = p + 1;
    while(pUrlBegin < pEnd && isspace(*pUrlBegin))
        ++pUrlBegin;
    p = (const char *)memchr(pUrlBegin, '>', pEnd - pUrlBegin);
    if (!p)
        return 0;
    const char *pUrlEnd = p++;
    while(isspace(pUrlEnd[-1]))
        --pUrlEnd;
    while(p < pEnd && (isspace(*p) || *p == ';'))
        ++p;

    const char *p1 = (const char *)memmem(p, pEnd - p, "preload", 7);
    if (!p1)
        return 0;
    p1 = (const char *)memmem(p, pEnd - p, "nopush", 6);
    if (p1)
        return 0;
    return pushToClient(pUrlBegin, pUrlEnd - pUrlBegin, cookie);
}

//Example:
// Link: </css/style.css>; rel=preload;
// Link: </dont/want/to/push/this.css>; rel=preload; as=stylesheet; nopush
// Link: </css>; as=style; rel=preload, </js>; as=script; rel=preload;
void HttpSession::processLinkHeader(const char* pValue, int valLen,
                                    AutoStr2 &cookie)
{
    if (!getStream()->getFlag(HIO_FLAG_PUSH_CAPABLE))
        return;

    const char *p = pValue;
    const char *pLineEnd = p + valLen;
    const char *pEnd;

    while(p < pLineEnd)
    {
        pEnd = (const char *)memchr(p, ',', pLineEnd - p);
        if (!pEnd)
            pEnd = pLineEnd;

        processOneLink(p, pEnd, cookie);
        p = pEnd + 1;
    }

}


void HttpSession::processServerPush()
{
    struct iovec iovs[100];
    struct iovec *p = iovs, *pEnd;
    pEnd = p + m_response.getRespHeaders().getHeader(
        HttpRespHeaders::H_LINK, iovs, 100);

    cookieval_t *cookie0 = m_request.getCookie(SERVERPUSHTAG,
                                               SERVERPUSHTAGLENGTH);
    AutoStr2    cookie1;
    if (cookie0 && cookie0->valLen > 0)
    {
        cookie1.setStr(m_request.getHeaderBuf().getp(cookie0->valOff),
                       cookie0->valLen);
    }

    while(p < pEnd)
    {
        processLinkHeader((const char *)p->iov_base, p->iov_len, cookie1);
        ++p;
    }

    if (cookie1.len() > 0)
    {
        if (cookie0 && cookie0->valLen > 0 && cookie1.len() == cookie0->valLen
            && strncasecmp(cookie1.c_str(),
                           m_request.getHeaderBuf().getp(cookie0->valOff),
                           cookie0->valLen) ==0)
        {
            LS_DBG_L(getLogSession(),
                     "processServerPush bypassed since no new url bit set.");
        }
        else
        {

            AutoStr2    cookie2(SERVERPUSHTAG, SERVERPUSHTAGLENGTH);
            cookie2.append("=", 1);
            cookie2.append(cookie1.c_str(), cookie1.len());
            m_response.getRespHeaders().add( HttpRespHeaders::H_SET_COOKIE,
                                             cookie2.c_str(), cookie2.len(),
                                             LSI_HEADEROP_MERGE);
        }
    }
}


int HttpSession::sendRespHeaders()
{
    if (!getFlag(HSF_RESP_HEADER_DONE))
    {
        if (respHeaderDone())
        {
            return 1;
        }
    }

    LS_DBG_L(getLogSession(), "sendRespHeaders()");

    MtSessData * p = getMtSessData();
    if (p)
    {
        ls_mutex_lock(&p->m_respHeaderLock);
        setMtFlag(HSF_MT_RESP_HDR_SENT);
    }
    m_response.getRespHeaders().prepareFinalize();

    int isNoBody = isNoRespBody();
    if (!isNoBody)
    {
        if (m_sessionHooks.isEnabled(LSI_HKPT_SEND_RESP_BODY))
            m_response.setContentLen(LSI_RSP_BODY_SIZE_UNKNOWN);
        if (m_response.getContentLen() >= 0)
            m_response.appendContentLenHeader();
        else
            setupChunkOS(0);
    }
    prepareHeaders();
    if (p)
        ls_mutex_unlock(&p->m_respHeaderLock);

    if (!isNoBody && m_response.getRespHeaders().hasPush()
        && getStream()->getFlag(HIO_FLAG_PUSH_CAPABLE)
        && m_request.getUserAgentType() != UA_SAFARI)
        processServerPush();

    if (finalizeHeader(m_request.getVersion(), m_request.getStatusCode()))
    {
        clearFlag(HSF_RESP_HEADER_SENT);
        return 1;
    }
    setFlag(HSF_RESP_HEADER_SENT);

    if (LS_LOG_ENABLED(LOG4CXX_NS::Level::DBG_HIGH))
        m_response.getRespHeaders().dump(getLogSession(), 1);

    if (LS_LOG_ENABLED(LOG4CXX_NS::Level::DBG_HIGH))
        m_response.getRespHeaders().dump(getLogSession(), 1);

    getStream()->sendRespHeaders(&m_response.getRespHeaders(), isNoBody);
    setState(HSS_WRITING);
    return 0;
}


int HttpSession::setupDynRespBodyBuf()
{
    LS_DBG_L(getLogSession(), "setupDynRespBodyBuf()");
    if (!isNoRespBody())
    {
        if (setupRespBodyBuf() == -1)
            return LS_FAIL;

        if (setupGzipFilter() == -1)
            return LS_FAIL;
    }
    else
    {
        //abortReq();
        //endResponseInternal(1);
        return 1;
    }
    return 0;
}


int HttpSession::flushDynBodyChunk()
{
    if (getGzipBuf() && getGzipBuf()->isStreamStarted())
    {
        getGzipBuf()->endStream();
        LS_DBG_M(getLogSession(), "flushDynBodyChunk() end GZIP stream.");
    }
    return 0;

}


int HttpSession::execExtCmd(const char *pCmd, int len, int mode)
{
    m_request.setRealPath(pCmd, len);
    m_request.orContextState(mode);
    return execExtCmdEx();
}


int HttpSession::execExtCmdEx()
{
    if (m_pHandler && cleanUpHandler(HSPS_EXEC_EXT_CMD) == LS_AGAIN)
        return -2;
    ReqHandler *pNewHandler;
    ExtWorker *pWorker = ExtAppRegistry::getApp(
                             EA_CGID, LSCGID_NAME);
    m_sExtCmdResBuf.clear();
    pNewHandler = HandlerFactory::getHandler(HandlerType::HT_CGI);
    m_pHandler = pNewHandler;
    return m_pHandler->process(this, pWorker);
}


//Fix me: This function is related the new variable "DateTime::s_curTimeUs",
//          Not sure if we need it. George may need to review this.
//
// int HttpSession::writeConnStatus( char * pBuf, int bufLen )
// {
//     static const char * s_pState[] =
//     {   "KA",   //HC_WAITING,
//         "RE",   //HC_READING,
//         "RB",   //HC_READING_BODY,
//         "EA",   //HC_EXT_AUTH,
//         "TH",   //HC_THROTTLING,
//         "PR",   //HC_PROCESSING,
//         "RD",   //HC_REDIRECT,
//         "WR",   //HC_WRITING,
//         "AP",   //HC_AIO_PENDING,
//         "AC",   //HC_AIO_COMPLETE,
//         "CP",   //HC_COMPLETE,
//         "SD",   //HC_SHUTDOWN,
//         "CL"    //HC_CLOSING
//     };
//     int reqTime= (DateTime::s_curTime - m_lReqTime) * 10 + (DateTime::s_curTimeUs - m_iReqTimeMs)/100000;
//     int n = snprintf( pBuf, bufLen, "%s\t%hd\t%s\t%d.%d\t%d\t%d\t",
//         getPeerAddrString(), m_iReqServed, s_pState[getState()],
//         reqTime / 10, reqTime %10 ,
//         m_request.getHttpHeaderLen() + (unsigned int)m_request.getContentFinished(),
//         m_request.getHttpHeaderLen() + (unsigned int)m_request.getContentLength() );
//     char * p = pBuf + n;
//     p += StringTool::offsetToStr( p, 20, m_response.getBodySent() );
//     *p++ = '\t';
//     p += StringTool::offsetToStr( p, 20, m_response.getContentLen() );
//     *p++ = '\t';
//
//     const char * pVHost = "";
//     int nameLen = 0;
//     if ( m_request.getVHost() )
//         pVHost = m_request.getVHost()->getVhName( nameLen );
//     memmove( p, pVHost, nameLen );
//     p += nameLen;
//
//     *p++ = '\t';
// /*
//     const HttpHandler * pHandler = m_request.getHttpHandler();
//     if ( pHandler )
//     {
//         int ll = strlen( pHandler->getName() );
//         memmove( p, pHandler->getName(), ll );
//         p+= ll;
//     }
// */
//     if ( m_pHandler )
//     {
//         n = m_pHandler->writeConnStatus( p, pBuf + bufLen - p );
//         p += n;
//     }
//     else
//         *p++ = '-';
//     *p++ = '\t';
//     *p++ = '"';
//     if ( m_request.isRequestLineDone() )
//     {
//         const char * q = m_request.getOrgReqLine();
//         int ll = m_request.getOrgReqLineLen();
//         if ( ll > MAX_REQ_LINE_DUMP )
//         {
//             int l = MAX_REQ_LINE_DUMP / 2 - 2;
//             memmove( p, q, l );
//             p += l;
//             *p++ = '.';
//             *p++ = '.';
//             *p++ = '.';
//             q = q + m_request.getOrgReqLineLen() - l;
//             memmove( p, q, l );
//         }
//         else
//         {
//             memmove( p, q, ll );
//             p += ll;
//         }
//     }
//     *p++ = '"';
//     *p++ = '\n';
//     return p - pBuf;
//
//
// }


int HttpSession::getServerAddrStr(char *pBuf, int len)
{
    const AutoStr2 *pAddr;
    const ConnInfo *pInfo = getStream()->getConnInfo();
    if (pInfo->m_pServerAddrInfo)
    {
        pAddr = pInfo->m_pServerAddrInfo->getAddrStr();
        if (pAddr->len() <= len)
            len = pAddr->len();
        memcpy(pBuf, pAddr->c_str(), len);
    }
    else
        len = 0;
    return len;
}


#define STATIC_FILE_BLOCK_SIZE 16384

int HttpSession::openStaticFile(const char *pPath, int pathLen,
                                int *status)
{
    int fd;
    *status = 0;
    fd = ::open(pPath, O_RDONLY);
    if (fd == -1)
    {
        switch (errno)
        {
        case EACCES:
            *status = SC_403;
            break;
        case ENOENT:
            *status = SC_404;
            break;
        case EMFILE:
        case ENFILE:
            *status = SC_503;
            break;
        default:
            *status = SC_500;
            break;
        }
    }
    else
    {
        *status = m_request.checkSymLink(pPath, pathLen, pPath);
        if (*status)
        {
            close(fd);
            fd = -1;
        }
        else
            fcntl(fd, F_SETFD, FD_CLOEXEC);
    }
    return fd;
}

void HttpSession::setSendFileOffsetSize(int fd, off_t start, off_t size)
{
    m_sendFileInfo.setParam((void *)(long)fd);
    setSendFileBeginEnd(start,
                        size >= 0 ? (start + size)
                        : m_sendFileInfo.getECache()->getFileSize());
}


int HttpSession::initSendFileInfo(const char *pPath, int pathLen)
{
    int ret;
    struct stat st;
    StaticFileCacheData *pData = m_sendFileInfo.getFileData();

    if ((pData) && (!pData->isSamePath(pPath, pathLen)))
    {
        m_sendFileInfo.release();
    }
    int fd = openStaticFile(pPath, pathLen, &ret);
    if (fd == -1)
        return ret;
    ret = fstat(fd, &st);
    if (ret != -1)
        ret = setUpdateStaticFileCache(pPath, pathLen, fd, st);
    if (ret)
    {
        close(fd);
        return ret;
    }
    pData = m_sendFileInfo.getFileData();
    FileCacheDataEx *pECache = m_sendFileInfo.getECache();
    if (pData->getFileData()->getfd() != -1
        && fd != pData->getFileData()->getfd())
        close(fd);
    if (pData->getFileData()->getfd() == -1)
        pData->getFileData()->setfd(fd);
    if (pData->getFileData() == pECache)
    {
        pECache->incRef();
        return 0;
    }

    return m_sendFileInfo.readyCacheData(0);
}


void HttpSession::setSendFileOffsetSize(off_t start, off_t size)
{
    setSendFileBeginEnd(start,
                        size >= 0 ? (start + size) : m_sendFileInfo.getECache()->getFileSize());
}


void HttpSession::setSendFileBeginEnd(off_t start, off_t end)
{
    m_sendFileInfo.setCurPos(start);
    m_sendFileInfo.setCurEnd(end);
    if ((end > start) && getFlag(HSF_RESP_WAIT_FULL_BODY))
    {
        clearFlag(HSF_RESP_WAIT_FULL_BODY);
        LS_DBG_M(getLogSession(),
                 "RESP_WAIT_FULL_BODY turned off by sending static file().");
    }
    setFlag(HSF_RESP_FLUSHED, 0);
    if (!getFlag(HSF_RESP_HEADER_DONE))
        respHeaderDone();

}


int HttpSession::writeRespBodyBlockInternal(SendFileInfo *pData,
        const char *pBuf,
        int written)
{
    int len;
    if (getGzipBuf())
    {
        len = getResp()->appendDynBody(pBuf, written);
        if (!len)
            len = written;
    }
    else
        len = writeRespBodyDirect(pBuf, written);
    LS_DBG_M(getLogSession(), "%s tried %d return %d.\n",
             getGzipBuf() ? "appendDynBodyEx()" : "writeRespBodyDirect()",
             written, len);

    if (len > 0)
        pData->incCurPos(len);
    return len;
}


int HttpSession::writeRespBodyBlockFilterInternal(SendFileInfo *pData,
        const char *pBuf,
        int written,
        lsi_param_t *param)
{
    int len;
    param->cur_hook = (void *)param->hook_chain->hooks->begin();
    param->ptr1 = pBuf;
    param->len1 = written;

//     if (( written >= pData->getRemain() )
//         &&((getFlag(HSF_HANDLER_DONE |
//                         HSF_RECV_RESP_BUFFERED ) == HSF_HANDLER_DONE )))
//         param->_flag_in = LSI_CBFI_EOF;

    len = LsiApiHooks::runForwardCb(param);
    LS_DBG_M(getLogSession(), "runForwardCb() sent: %d, buffered: %d",
             len, *(param->flag_out));

    if (len > 0)
        pData->incCurPos(len);
    return len;
}


#ifdef LS_AIO_USE_AIO
int HttpSession::aioRead(SendFileInfo *pData, void *pBuf)
{
    int remain = pData->getRemain();
    int len = (remain < STATIC_FILE_BLOCK_SIZE) ? remain :
              STATIC_FILE_BLOCK_SIZE;
    if (!pBuf)
        pBuf = ls_palloc(STATIC_FILE_BLOCK_SIZE);
    remain = m_aioReq.read(pData->getECache()->getfd(), pBuf,
                           len, pData->getCurPos(), (AioEventHandler *)this);
    if (remain != 0)
        return LS_FAIL;
    setFlag(HSF_AIO_READING);
    return 1;
}


//returns -1 on error, 1 on success, 0 for didn't do anything new (cached)
int HttpSession::sendStaticFileAio(SendFileInfo *pData)
{
    long len;
    off_t remain;
    off_t written = pData->getAioLen();
    void *pBuf = pData->getAioBuf();

    if (pBuf)
    {
        if (written)
        {
            remain = pData->getCurPos() - m_aioReq.getOffset();
            written -= remain;
            len = writeRespBodyBlockInternal(pData,
                                             (const char *)pBuf + remain,
                                             written);
            if (len < 0)
                return LS_FAIL;
            else if (!getStream()->isSpdy() && len < written)
            {
                LS_DBG_M(getLogSession(),
                         "Socket still busy, %lld bytes to write",
                         (long long)(written - len));
                return 1;
            }
        }
        pData->resetAioBuf();
        LS_DBG_M(getLogSession(), "Buffer finished, continue with aioRead.");
        suspendWrite();
    }
    if (!pData->getECache()->isCached() && pData->getRemain() > 0)
    {
        if (getFlag(HSF_AIO_READING))
        {
            LS_DBG_M(getLogSession(), "Aio Reading already in progress.");
            return 1;
        }
        return aioRead(pData, (void *)
                       pBuf);  //Filter down the pBuf if socket stopped.
    }
    return 0;
}
#endif // LS_AIO_USE_AIO


int HttpSession::sendStaticFileEx(SendFileInfo *pData)
{
    const char *pBuf;
    off_t written;
    off_t remain;
    off_t len;
    int count = 0;

#if !defined( NO_SENDFILE )
    int fd = pData->getfd();
    int iModeSF = HttpServerConfig::getInstance().getUseSendfile();
    if (iModeSF && fd != -1 && !isHttps() && !getStream()->isSpdy()
        && (!getGzipBuf() ||
            (pData->getECache() == pData->getFileData()->getGzip())))
    {
        /**
         * Update to make sure sendfile size is limit to 2GB per calling
         */
        off_t ssize = pData->getRemain();
        if (ssize > INT_MAX)
            ssize = INT_MAX;
        len = writeRespBodySendFile(fd, pData->getCurPos(), ssize);
        LS_DBG_M(getLogSession(), "writeRespBodySendFile() write %ld returned %ld.", 
                 ssize, len);
        if (len > 0)
        {
            if (iModeSF == 2 && m_pChunkOS == NULL)
            {
                setFlag(HSF_AIO_READING);
                suspendWrite();
            }
            else
                pData->incCurPos(len);
        }
        return (pData->getRemain() > 0);
    }
#endif
#ifdef LS_AIO_USE_AIO
    if (HttpServerConfig::getInstance().getUseSendfile() == 2)
    {
        len = sendStaticFileAio(pData);
        LS_DBG_M(getLogSession(), "sendStaticFileAio() returned %lld.",
                 (long long)len);
        if (len)
            return len;
    }
#endif

    BlockBuf tmpBlock;
    while ((remain = pData->getRemain()) > 0)
    {
        len = (remain < STATIC_FILE_BLOCK_SIZE) ? remain : STATIC_FILE_BLOCK_SIZE ;
        written = remain;
        if (pData->getECache())
        {
            pBuf = pData->getECache()->getCacheData(
                       pData->getCurPos(), written, HttpResourceManager::getGlobalBuf(), len);
            if (written <= 0)
                return LS_FAIL;
        }
        else
        {
            pBuf = VMemBuf::mapTmpBlock(pData->getfd(), tmpBlock, pData->getCurPos());
            if (!pBuf)
                return -1;
            written = tmpBlock.getBufEnd() - pBuf;
            if (written > remain)
                written = remain;
            if (written <= 0)
                return -1;
        }

        len = writeRespBodyBlockInternal(pData, pBuf, written);
        if (!pData->getECache())
            VMemBuf::releaseBlock(&tmpBlock);
        if (len < 0)
            return len;
        else if (len == 0)
            break;
        else if ((!getStream()->isSpdy() && len < written) || ++count >= 10)
            return 1;
    }
    return (pData->getRemain() > 0);
}


int HttpSession::setSendFile(SendFileInfo *pData)
{
    if (m_sendFileInfo.getRemain() > 0)
        return 1;
    m_sendFileInfo.release();
    m_sendFileInfo.copy(*pData);
    if (m_sendFileInfo.getFileData())
        m_sendFileInfo.getFileData()->incRef();
    if (m_sendFileInfo.getECache())
        m_sendFileInfo.getECache()->incRef();
    pData->setCurPos(pData->getCurEnd());
    m_iFlag &= ~HSF_RESP_FLUSHED;
    return 0;
}


int HttpSession::passSendFileToParent(SendFileInfo *pData)
{
    HioChainStream *pStream = (HioChainStream *)getStream();
    if (pStream->getParentSession())
        return pStream->getParentSession()->setSendFile(pData);
    return 1;
}


int HttpSession::sendStaticFile(SendFileInfo *pData)
{
    LS_DBG_M(getLogSession(), "SendStaticFile()");

    if (m_iFlag & HSF_SUB_SESSION && !getGzipBuf())
        return passSendFileToParent(pData);
    if (m_sessionHooks.isDisabled(LSI_HKPT_SEND_RESP_BODY) ||
        !(m_sessionHooks.getFlag(LSI_HKPT_SEND_RESP_BODY)&
          LSI_FLAG_PROCESS_STATIC))
        return sendStaticFileEx(pData);

    const char *pBuf;
    off_t written;
    off_t remain;
    long len;
#ifdef LS_AIO_USE_AIO
    if (HttpServerConfig::getInstance().getUseSendfile() == 2)
    {
        len = sendStaticFileAio(pData);
        LS_DBG_L(getLogSession(), "sendStaticFileAio() returned %ld.", len);
        if (len)
            return len;
    }
#endif
    lsi_param_t param;
    lsi_hookinfo_t hookInfo;
    int buffered = 0;
    param.session = (LsiSession *)this;

    hookInfo.term_fn = (filter_term_fn)
                       writeRespBodyTermination;
    hookInfo.hooks = LsiApiHooks::getGlobalApiHooks(LSI_HKPT_SEND_RESP_BODY);
    hookInfo.enable_array = m_sessionHooks.getEnableArray(
                                LSI_HKPT_SEND_RESP_BODY);
    hookInfo.hook_level = LSI_HKPT_SEND_RESP_BODY;
    param.hook_chain = &hookInfo;

    param.flag_in  = 0;
    param.flag_out = &buffered;


    BlockBuf tmpBlock;
    while ((remain = pData->getRemain()) > 0)
    {
        len = (remain < STATIC_FILE_BLOCK_SIZE) ? remain : STATIC_FILE_BLOCK_SIZE ;
        written = remain;
        if (pData->getECache())
        {
            pBuf = pData->getECache()->getCacheData(
                       pData->getCurPos(), written, HttpResourceManager::getGlobalBuf(), len);
            if (written <= 0)
                return LS_FAIL;
        }
        else
        {
            pBuf = VMemBuf::mapTmpBlock(pData->getfd(), tmpBlock, pData->getCurPos());
            if (!pBuf)
                return -1;
            written = tmpBlock.getBufEnd() - pBuf;
            if (written > remain)
                written = remain;
            if (written <= 0)
                return -1;
        }
        len = writeRespBodyBlockFilterInternal(pData, pBuf, written, &param);
        if (!pData->getECache())
            VMemBuf::releaseBlock(&tmpBlock);
        if (len < 0)
            return len;
        else if (len == 0)
            break;
        else if (len < written || buffered)
        {
            if (buffered)
                setFlag(HSF_SEND_RESP_BUFFERED, 1);
            return 1;
        }
    }
    return (pData->getRemain() > 0);

}


int HttpSession::finalizeHeader(int ver, int code)
{
    //setup Send Level gzip filters
    //checkAndInstallGzipFilters(1);

    int ret = 0;
    if (m_sessionHooks.isEnabled(LSI_HKPT_SEND_RESP_HEADER))
    {
        ret = m_sessionHooks.runCallbackNoParam(LSI_HKPT_SEND_RESP_HEADER,
                                                (LsiSession *)this);
        ret = processHkptResult(LSI_HKPT_SEND_RESP_HEADER, ret);
        if (ret)
            return ret;

    }
    if (!isNoRespBody())
        ret = contentEncodingFixup();

    getResp()->getRespHeaders().addStatusLine(ver, code,
            m_request.isKeepAlive());

    return ret;
}

void HttpSession::testContentType()
{
    HttpResp *pResp = getResp();
    HttpReq *pReq = getReq();
    int valLen;
    char *pValue = (char *)pResp->getRespHeaders().getHeader(
                                HttpRespHeaders::H_CONTENT_TYPE, &valLen);
    if (!pValue)
        return;
    const MimeSetting *pMIME = NULL;
    int canCompress = pReq->gzipAcceptable() | pReq->brAcceptable();
    HttpContext *pContext = &(pReq->getVHost()->getRootContext());
    const ExpiresCtrl *pExpireDefault = pReq->shouldAddExpires();
    int enbale = pContext->getExpires().isEnabled();

    if (canCompress || enbale)
    {
        char *pEnd;
        const char *p;
        p = (char *)memchr(pValue, ';', valLen);
        if (p)
            pEnd = (char *)p;
        else
            pEnd = (char *)pValue + valLen;
        char ch = *pEnd;
        *pEnd = 0;
        pMIME = pContext->lookupMimeSetting((char *)pValue);
        LS_DBG_L(getLogSession(), "Response content type: [%s], pMIME: %p\n",
                 pValue, pMIME);
        *pEnd = ch;

    }
    if (!pMIME || !pMIME->getExpires()->compressible())
    {
        pReq->andGzip(~GZIP_ENABLED);
        pReq->andBr(~BR_ENABLED);
    }

    if (enbale)
    {
        if (pMIME && pMIME->getExpires()->getBase())
            pExpireDefault = (ExpiresCtrl *)pMIME->getExpires();
        if (pExpireDefault == NULL)
            pExpireDefault = &pContext->getExpires();

        if (pExpireDefault->getBase())
            pResp->addExpiresHeader(DateTime::s_curTime, pExpireDefault);
    }
}


int HttpSession::updateContentCompressible()
{
    int compressible = 0;
    if ((m_request.gzipAcceptable() == GZIP_REQUIRED)
        || (m_request.brAcceptable() == BR_REQUIRED))
    {
        int len;
        char *pContentType = (char *)m_response.getRespHeaders().getHeader(
                                 HttpRespHeaders::H_CONTENT_TYPE, &len);
        if (pContentType)
        {
            char ch = pContentType[len];
            pContentType[len] = 0;
            compressible = HttpMime::getMime()->compressible(pContentType);
            pContentType[len] = ch;
        }
        if (!compressible)
        {
            m_request.andGzip(~GZIP_ENABLED);
            m_request.andBr(~BR_ENABLED);
        }
    }
    return compressible;
}


int HttpSession::contentEncodingFixup()
{
    int len;
    int requireChunk = 0;
    const char *pContentEncoding = m_response.getRespHeaders().getHeader(
                                       HttpRespHeaders::H_CONTENT_ENCODING, &len);
    if ((!(m_request.gzipAcceptable() & REQ_GZIP_ACCEPT))
        && (!(m_request.brAcceptable() & REQ_BR_ACCEPT)))
    {
        if (pContentEncoding)
        {
            if (addModgzipFilter((LsiSession *)this, 1, 0) == -1)
                return LS_FAIL;
            m_response.getRespHeaders().del(HttpRespHeaders::H_CONTENT_ENCODING);
            clearFlag(HSF_RESP_BODY_GZIPCOMPRESSED);
            requireChunk = 1;
        }
    }
    else if (!pContentEncoding && updateContentCompressible())
    {
        if (m_response.getContentLen() > 200)// && getReq()->getStatusCode() < SC_400)
        {
            if (addModgzipFilter((LsiSession *)this, 1,
                                 HttpServerConfig::getInstance().getCompressLevel()) == -1)
                return LS_FAIL;
            m_response.addGzipEncodingHeader();
            //The below do not set the flag because compress won't update the resp VMBuf to decompressed
            //setFlag(HSF_RESP_BODY_GZIPCOMPRESSED);
            requireChunk = 1;
        }
    }
    if (requireChunk)
    {
        if (!m_pChunkOS)
        {
            setupChunkOS(1);
            m_response.getRespHeaders().del(HttpRespHeaders::H_CONTENT_LENGTH);
        }
    }
    return 0;
}


// int HttpSession::handoff(char **pData, int *pDataLen)
// {
//     if (isSSL() || getStream()->isSpdy())
//         return LS_FAIL;
//     if (m_iReqServed != 0)
//         return LS_FAIL;
//     int fd = dup(getStream()->getNtwkIoLink()->getfd());
//     if (fd != -1)
//     {
//         AutoBuf &headerBuf = m_request.getHeaderBuf();
//         *pDataLen = headerBuf.size() - HEADER_BUF_PAD;
//         *pData = (char *)malloc(*pDataLen);
//         memmove(*pData, headerBuf.begin() + HEADER_BUF_PAD, *pDataLen);
//
//         getStream()->setAbortedFlag();
//         closeSession();
//     }
//     return fd;
//
// }


int HttpSession::onAioEvent()
{
#ifdef LS_AIO_USE_AIO
    int buffered = 0;
    int len, written = m_aioReq.getReturn();
    char *pBuf = (char *)m_aioReq.getBuf();

    LS_DBG_M(getLogSession(), "Aio Read read %d.", written);

    if (written <= 0)
        return LS_FAIL;

    clearFlag(HSF_AIO_READING);
    if (m_sessionHooks.isDisabled(LSI_HKPT_SEND_RESP_BODY) ||
        !(m_sessionHooks.getFlag(LSI_HKPT_SEND_RESP_BODY)&
          LSI_FLAG_PROCESS_STATIC))
        len = writeRespBodyBlockInternal(&m_sendFileInfo, pBuf, written);
    else
    {
        lsi_param_t param;
        lsi_hookinfo_t hookInfo;
        param.session = (LsiSession *)this;
        hookInfo.hook_level = LSI_HKPT_SEND_RESP_BODY;
        hookInfo.term_fn = (filter_term_fn)
                           writeRespBodyTermination;
        hookInfo.hooks = LsiApiHooks::getGlobalApiHooks(LSI_HKPT_SEND_RESP_BODY);
        hookInfo.enable_array = m_sessionHooks.getEnableArray(
                                    LSI_HKPT_SEND_RESP_BODY);
        param.hook_chain = &hookInfo;

        param.flag_in  = 0;
        param.flag_out = &buffered;

        len = writeRespBodyBlockFilterInternal(&m_sendFileInfo, pBuf, written,
                                               &param);
    }
    if (len < 0)
        return LS_FAIL;
    else if ((len > 0)
             && (len < written || buffered))
    {
        LS_DBG_M(getLogSession(),
                 "HandleEvent: Socket busy, len = %d, buffered = %d.",
                 len, buffered);
        if (buffered)
            written = 0;
        m_sendFileInfo.setAioBuf(pBuf);
        m_sendFileInfo.setAioLen(written);
        getStream()->flush();
        continueWrite();
        return 0;
    }

    if (m_sendFileInfo.getRemain() > 0)
        return aioRead(&m_sendFileInfo, pBuf);

    ls_pfree(pBuf);
    return onWriteEx();
#endif
    return LS_FAIL;
}


int HttpSession::handleAioSFEvent(Aiosfcb *event)
{
    int ret;
    int written = event->getRet();
    clearFlag(HSF_AIO_READING);
    if (event->getFlag(AIOSFCB_FLAG_CANCEL))
    {
        recycle();
        return 0;
    }
    LS_DBG_M(getLogSession(), "Got Aio SF Event! Returned %d.", written);
    ret = getStream()->aiosendfiledone(event);
    if (written < 0)
    {
        if (ret)     //Not EAGAIN
            closeSession();
        else
            continueWrite();
        return ret;
    }
    else if (written > 0)
    {
        m_response.written(written);
        LS_DBG_M(getLogSession(), "Aio Response body sent: %lld.",
                 (long long)m_response.getBodySent());
        m_sendFileInfo.incCurPos(written);
    }
    if (m_sendFileInfo.getRemain() > 0)
    {
        continueWrite();
        return 0;
    }
    return onWriteEx();
}

void HttpSession::setBackRefPtr(evtcbtail_t ** v)
{
    LS_DBG_M(getLogSession(),
                 "setBackRefPtr() called, set to %p.", *v);
    evtcbtail_t::back_ref_ptr = v;
}


void HttpSession::resetEvtcb()
{
    LS_DBG_M("%s calling resetEvtcbTail on this %p\n", __func__, this);
    EvtcbQue::getInstance().resetEvtcbTail(this);
    evtcbtail_t::back_ref_ptr = NULL;
}


void HttpSession::cancelEvent(evtcbnode_s * v)
{
    if (back_ref_ptr == EvtcbQue::getSessionRefPtr(v))
    {
        back_ref_ptr = NULL;
    }
    EvtcbQue::getInstance().recycle(v);
}


void HttpSession::resetBackRefPtr()
{
    if (evtcbtail_t::back_ref_ptr)
    {
        LS_DBG_M(getLogSession(),
                 "resetBackRefPtr() called. previous value is %p:%p.",
                 evtcbtail_t::back_ref_ptr, *evtcbtail_t::back_ref_ptr);
        *evtcbtail_t::back_ref_ptr = NULL;
        evtcbtail_t::back_ref_ptr = NULL;
    }
}

int HttpSession::smProcessReq()
{
    assert((m_iFlag & HSF_SUSPENDED) == 0);
    int ret = 0;
    int loop = 0;
    int stateLoop = 0;
    HSPState lastState = HSPS_START;
    while (ret == 0 && m_processState < HSPS_HANDLER_PROCESSING)
    {
        LS_DBG_M(getLogSession(), "Run State: %s", s_stateName[m_processState]);
        switch (m_processState)
        {
        case HSPS_READ_REQ_HEADER:
            ret = readToHeaderBuf();
            if (m_processState != HSPS_NEW_REQ)
            {
                if (ret > 0)
                    break;
                return 0;
            }
        //fall through
        case HSPS_NEW_REQ:
            ret = processNewReqInit();
            if (m_processState != HSPS_HKPT_HTTP_BEGIN)
                break;
        //fall through
        case HSPS_HKPT_HTTP_BEGIN:
            ret = runEventHkpt(LSI_HKPT_HTTP_BEGIN, HSPS_HKPT_RCVD_REQ_HEADER);
            if (ret || m_processState != HSPS_HKPT_RCVD_REQ_HEADER)
                break;
        //fall through
        case HSPS_HKPT_RCVD_REQ_HEADER:
            ret = runEventHkpt(LSI_HKPT_RCVD_REQ_HEADER, HSPS_PROCESS_NEW_REQ_BODY);
            if (ret || m_processState != HSPS_PROCESS_NEW_REQ_BODY)
                break;
        //fall through
        case HSPS_PROCESS_NEW_REQ_BODY:
            ret = processNewReqBody();
            if (ret || m_processState != HSPS_HKPT_RCVD_REQ_BODY)
                break;
        //fall through
        case HSPS_HKPT_RCVD_REQ_BODY:
            m_processState = HSPS_PROCESS_NEW_URI;
//             ret = runEventHkpt(LSI_HKPT_RCVD_REQ_BODY, HSPS_PROCESS_NEW_URI);
//             if (ret || m_processState != HSPS_PROCESS_NEW_URI)
//                 break;
        //fall through
        case HSPS_PROCESS_NEW_URI:
            ret = processNewUri();
            if (ret || m_processState != HSPS_VHOST_REWRITE)
                break;
        //fall through
        case HSPS_VHOST_REWRITE:
            ret = processVHostRewrite();
            if (ret || m_processState != HSPS_CONTEXT_MAP)
                break;
        //fall through
        case HSPS_CONTEXT_MAP:
            ret = processContextMap();
            if (ret || m_processState != HSPS_CONTEXT_REWRITE)
                break;
        //fall through
        case HSPS_CONTEXT_REWRITE:
            ret = processContextRewrite();
            if (ret || m_processState != HSPS_HKPT_URI_MAP)
                break;
        //fall through
        case HSPS_HKPT_URI_MAP:
            setFlag(HSF_URI_MAPPED);
            preUriMap();
            ret = runEventHkpt(LSI_HKPT_URI_MAP, HSPS_FILE_MAP);
            if (ret || m_processState != HSPS_FILE_MAP)
                break;

            /**
             * In this state, if req body done or no req body, go through the hook
             */
            if (getFlag(HSF_REQ_BODY_DONE) == HSF_REQ_BODY_DONE)
            {
                ret = runEventHkpt(LSI_HKPT_RCVD_REQ_BODY, HSPS_FILE_MAP);
                if (ret || m_processState != HSPS_FILE_MAP)
                    break;
            }

        //fall through
        case HSPS_FILE_MAP:
            ret = processFileMap();
            if (ret || m_processState != HSPS_CHECK_AUTH_ACCESS)
                break;
        //fall through
        case HSPS_CHECK_AUTH_ACCESS:
            ret = processContextAuth();
            if (ret || m_processState != HSPS_HKPT_HTTP_AUTH)
                break;
        //fall through
        case HSPS_HKPT_HTTP_AUTH:
            ret = runEventHkpt(LSI_HKPT_HTTP_AUTH, HSPS_AUTH_DONE);
            if (ret || m_processState != HSPS_AUTH_DONE)
                break;
        //fall through
        case HSPS_AUTH_DONE:
            if (!(getFlag(HSF_URI_PROCESSED)))
            {
                setProcessState(HSPS_CONTEXT_MAP);
                break;
            }
            else if (getFlag(HSF_SC_404))
            {
                ret = SC_404;
                clearFlag(HSF_SC_404);
                break;
            }
            else
                setProcessState(HSPS_BEGIN_HANDLER_PROCESS);
            //fall through
        case HSPS_BEGIN_HANDLER_PROCESS:
            ret = handlerProcess(m_request.getHttpHandler());
            break;

        case HSPS_HANDLER_PRE_PROCESSING:
            m_iFlag |= HSF_URI_MAPPED;
            preUriMap();
            runEventHkpt(LSI_HKPT_URI_MAP, HSPS_BEGIN_HANDLER_PROCESS);
            break;

        case HSPS_HKPT_RCVD_REQ_BODY_PROCESSING:
            ret = runEventHkpt(LSI_HKPT_RCVD_REQ_BODY, HSPS_HANDLER_PROCESSING);
            if (m_processState == HSPS_HANDLER_PROCESSING)
            {
                if (m_pHandler)
                    m_pHandler->onRead(this);
            }
            break;
        case HSPS_HKPT_RCVD_RESP_HEADER:
            ret = runEventHkpt(LSI_HKPT_RCVD_RESP_HEADER, HSPS_RCVD_RESP_HEADER_DONE);
            if (m_processState == HSPS_RCVD_RESP_HEADER_DONE)
                return 0;
            break;
        case HSPS_HKPT_RCVD_RESP_BODY:
            ret = runEventHkpt(LSI_HKPT_RCVD_RESP_BODY, HSPS_RCVD_RESP_BODY_DONE);
            if (m_processState == HSPS_RCVD_RESP_BODY_DONE)
                return 0;
            break;
        case HSPS_HKPT_SEND_RESP_HEADER:
            ret = runEventHkpt(LSI_HKPT_SEND_RESP_HEADER, HSPS_SEND_RESP_HEADER_DONE);
            if (m_processState == HSPS_SEND_RESP_HEADER_DONE)
                return 0;
            break;
        case HSPS_HKPT_HTTP_END:
            ret = runEventHkpt(LSI_HKPT_HTTP_END, HSPS_HTTP_END_DONE);
            if (m_processState == HSPS_HTTP_END_DONE)
                return 0;
            break;
        case HSPS_HKPT_HANDLER_RESTART:
            ret = runEventHkpt(LSI_HKPT_HANDLER_RESTART, HSPS_HANDLER_RESTART_DONE);
            if (m_processState == HSPS_HANDLER_RESTART_DONE)
                return 0;
            break;
        case HSPS_HANDLER_RESTART_CANCEL_MT:
            if (restartHandlerProcessEx() == 0)
                ret = doRedirect();
            else
                return 0;
            break;
        case HSPS_READ_REQ_BODY:
            return 0;
        case HSPS_RCVD_RESP_HEADER_DONE:
        case HSPS_RCVD_RESP_BODY_DONE:
        case HSPS_SEND_RESP_HEADER_DONE:
            //temporary state,
            setProcessState(HSPS_HANDLER_PROCESSING);
            return 0;
        case HSPS_WEBSOCKET:
        case HSPS_HTTP_ERROR:
        case HSPS_DROP_CONNECTION:
            return 0;
        case HSPS_HTTP_END_DONE:
        case HSPS_HANDLER_RESTART_DONE:
            break;
        case HSPS_REDIRECT:
            ret = redirectEx();
            break;
        case HSPS_CLOSE_SESSION:
            if (getStream())
            {
                closeSession();
                return 0;
            }
            //fall through
        case HSPS_RELEASE_RESOURCE:
            releaseResources();
            return 0;
        case HSPS_NEXT_REQUEST:
            nextRequest();
            if (m_processState != HSPS_READ_REQ_HEADER)
                return 0;
            ret = 0;
            break;
        case HSPS_EXEC_EXT_CMD:
            ret = execExtCmdEx();
            break;
        default:
            assert("Unhandled processing state, should not happen" == NULL);
            break;
        }
        if (ret != 0)
            break;

        ++loop;
        if (lastState != m_processState)
        {
            lastState = m_processState;
            stateLoop = 0;
        }
        else
            ++stateLoop;
        if (loop > 20)
        {
            char error[256];
            snprintf(error, 256, "smProcessReq() loop: %d, current state: %s, "
                     "last state: %s, count: %d", loop,
                     s_stateName[m_processState],
                     s_stateName[lastState], stateLoop);
            assert(&error[0] == NULL);
        }

    }
    if (ret == LSI_SUSPEND)
    {
        getStream()->wantRead(0);
        getStream()->wantWrite(0);
        setFlag(HSF_SUSPENDED);
        return 0;
    }
    else if (ret > 0)
    {
        setProcessState(HSPS_HTTP_ERROR);
        if (getStream()->getState() < HIOS_SHUTDOWN)
            httpError(ret);
    }
    return ret;
}


int HttpSession::resumeProcess(int passCode, int retcode)
{
    //if ( passCode != m_suspendPasscode )
    //    return LS_FAIL;
    if (!testFlag(HSF_SUSPENDED))
        return LS_FAIL;
    clearFlag(HSF_SUSPENDED);
    if (!testFlag(HSF_REQ_BODY_DONE))
        getStream()->wantRead(1);
    if (m_pHandler
        && !(testFlag(HSF_HANDLER_DONE | HSF_HANDLER_WRITE_SUSPENDED)))
        continueWrite();

    switch (m_processState)
    {
    case HSPS_HKPT_HTTP_BEGIN:
    case HSPS_HKPT_RCVD_REQ_HEADER:
    case HSPS_HKPT_RCVD_REQ_BODY:
    case HSPS_HKPT_URI_MAP:
    case HSPS_HKPT_HTTP_AUTH:
    case HSPS_HKPT_RCVD_RESP_HEADER:
    case HSPS_HKPT_RCVD_RESP_BODY:
    case HSPS_HKPT_SEND_RESP_HEADER:
    case HSPS_HKPT_HANDLER_RESTART:
    case HSPS_HKPT_HTTP_END:
        m_curHookRet = retcode;
        smProcessReq();
        break;
    case HSPS_BEGIN_HANDLER_PROCESS:
        break;


    default:
        break;
    }
    return 0;
}


int HttpSession::suspendProcess()
{
    if (m_suspendPasscode)
        return 0;
    while ((m_suspendPasscode = rand()) == 0)
        ;
    getStream()->wantRead(0);
    getStream()->wantWrite(0);
    return m_suspendPasscode;
}

/*
void HttpSession::runAllEventNotifier()
{
    EventObj *pObj;
    lsi_event_callback_pf   cb;

    while( m_pEventObjHead && m_pEventObjHead->m_pParam == this)
    {
        pObj = m_pEventObjHead;
        if (pObj->m_eventCb)
        {
            cb = pObj->m_eventCb;
            pObj->m_eventCb = NULL;
            (*cb)(pObj->m_lParam, pObj->m_pParam);
            if ( !m_pEventObjHead )
                return;  //all pending events removed
        }
        else
            return;  //recurrsive call of runAllEventNotifier
        assert(pObj==m_pEventObjHead);
        m_pEventObjHead = (EventObj *)m_pEventObjHead->remove();
        CallbackQueue::getInstance().recycle(pObj);
        if ( m_pEventObjHead == (EventObj *)CallbackQueue::getInstance().end())
            break;
    }
    m_pEventObjHead = NULL;
}
*/


void HttpSession::runAllCallbacks()
{
    EvtcbQue::getInstance().run(this);
}


int HttpSession::beginMtHandler(const lsi_reqhdlr_t *pHandler)
{
    LS_DBG_H(getLogSession(), "Set up MtHandler");

    m_pMtSessData = new MtSessData();
    if (m_pMtSessData)
    {
        setMtFlag(HSF_MT_HANDLER);
        setModHandler(pHandler);
        setupDynRespBodyBuf();
        return LS_OK;
    }
    else
        return SC_500;
}


void HttpSession::releaseMtSessData()
{
    MtSessData *pMtSessData = getMtSessData();
    setMtNotifiers(NULL);
    LS_TH_BENIGN(&pMtSessData, "ok delete");
    delete pMtSessData;
    ls_atomic_setint(&m_iMtFlag, 0);
}


int HttpSession::cancelMtHandler()
{
    LS_DBG_M(getLogSession(), "cancelMtHandler() called. %d", getSn());

    if (ls_atomic_add_fetch(&m_iMtFlag, 0) == 0)
    {
        LS_DBG_M(getLogSession(), "Mt handler already finished. %d", getSn());
        return LS_OK;
    }
    if (testAndSetMtFlag(HSF_MT_CANCEL))
    {
        LS_DBG_M(getLogSession(), "mark HSF_MT_CANCEL. %d", getSn());
    }
    else
    {
        LS_DBG_M(getLogSession(), "HSF_MT_CANCEL has been set, Mt handler may blocking, notify again. %d", getSn());
    }
    LS_DBG_M(getLogSession(), "Notify blocking handler threads");
    MtSessData * pMtSessData = getMtSessData();
    if (pMtSessData)
    {
        pMtSessData->m_read.broadcastTillNoWaiting();
        pMtSessData->m_write.broadcastTillNoWaiting();
        pMtSessData->m_event.broadcastTillNoWaiting();
    }
    return LS_AGAIN;
}


void HttpSession::mtNotifyWriters()
{
    MtNotifier *pNotifier = &getMtSessData()->m_write;
    pNotifier->lock();
    if (pNotifier->getWaiters() > 0)
    {
        rewindRespBodyBuf();
        LS_DBG_L(getLogSession(), "MT:notify() writers");
        pNotifier->notifyEx();
    }
    pNotifier->unlock();
}


int HttpSession::mtNotifyCallbackEvent(lsi_session_t *session, long lParam,
                                void *pParam)
{
    HttpSession *pSession = (HttpSession *)(LsiSession *)session;
    if (!pSession)
        return -1;
    return pSession->processMtEvent(lParam, pParam);
}

int HttpSession::removeSessionCbsEvent(lsi_session_t *session, long lParam,
                                void *pParam)
{
    HttpSession *pSession = (HttpSession *)(LsiSession *)session;
    if (!pSession)
        return -1;
    return pSession->removeSessionCbs(lParam, pParam);
}


int HttpSession::mtNotifyCallback(lsi_session_t *session, long lParam,
                                void *pParam)
{
    HttpSession *pSession = (HttpSession *)(LsiSession *)session;
    if (!pSession)
        return -1;
    return pSession->processMtEvents(lParam, pParam);
}


void HttpSession::mtParseReqArgs(MtParamParseReqArgs *pParams)
{
    if ((NULL == pParams) || (getMtFlag(HSF_MT_CANCEL)))
    {
        if (pParams)
            pParams->m_ret = LS_FAIL;
        return;
    }

    pParams->m_ret = parseReqArgs(pParams->m_parseReqBody, pParams->m_uploadPassByPath,
                            pParams->m_pUploadTmpDir, pParams->m_tmpFilePerms);
}


void HttpSession::mtSendfile(MtParamSendfile *pParams)
{
    if ((NULL == pParams) || (getMtFlag(HSF_MT_CANCEL)))
    {
        if (pParams)
            pParams->m_ret = LS_FAIL;
        return;
    }
    pParams->m_ret = LS_OK;
    if (pParams->m_fd)
        setSendFileOffsetSize(pParams->m_fd, pParams->m_start, pParams->m_size);
    else if (LS_OK == (pParams->m_ret
            = initSendFileInfo(pParams->m_pFile, strlen(pParams->m_pFile))))
    {
        setSendFileOffsetSize(pParams->m_start, pParams->m_size);
    }
}


int HttpSession::mtFlushLocalBuf()
{
    LS_DBG_H(getLogSession(), "enter %s\n", __func__);

    MtSessData * sd = getMtSessData();
    MtWriteMode & writeMode = sd->m_mtWrite.m_writeMode;
    MtWriteBuf & wBuf = sd->m_mtWrite.m_writeBuf;
    if (!wBuf.getBuf() || wBuf.getCurPos() == wBuf.getBuf())
        return LS_OK;
    if (WRITE_THROUGH == writeMode)
    {
        VMemBuf * curVMemBuf = sd->m_mtWrite.m_curVMemBuf;
        LS_DBG_H(getLogSession(), "%s: calling writeUsed (vMemBuf ptr %p size %lu)\n",
                    __func__, curVMemBuf, wBuf.getCurPos() - wBuf.getBuf());
        curVMemBuf->writeUsed(wBuf.getCurPos() - wBuf.getBuf());
        wBuf.set(NULL, 0, NULL);
        return LS_OK;
    }

    ls_mutex_lock(&sd->m_mtWrite.m_bufQ_lock);
    if (NULL == sd->m_mtWrite.m_bufQ)
    {
        sd->m_mtWrite.m_bufQ = new MtLocalBufQ;
    }
    sd->m_mtWrite.m_bufQ->append(wBuf); // copies to new instance of MtWriteBuf, deleted in HttpSession

    MtLocalBufQ * lbq = sd->m_mtWrite.m_bufQ;
    sd->m_mtWrite.m_bufQ = NULL;
    ls_mutex_unlock(&sd->m_mtWrite.m_bufQ_lock);
    LS_DBG_H(getLogSession(), "%s: pushed on bufQ %p buf %p size %ld\n",
                __func__, lbq, wBuf.getBuf(), wBuf.getCurPos() - wBuf.getBuf());

    wBuf.set(NULL, 0, NULL);
    return mtFlushLocalRespBodyBufQueue(lbq);
}


int HttpSession::mtFlushLocalRespBodyBufQueue()
{
    LS_DBG_H(getLogSession(), "enter %s\n", __func__);

    ls_mutex_lock(&getMtSessData()->m_mtWrite.m_bufQ_lock);
    MtLocalBufQ * q = getMtSessData()->m_mtWrite.m_bufQ;
    getMtSessData()->m_mtWrite.m_bufQ = NULL;
    ls_mutex_unlock(&getMtSessData()->m_mtWrite.m_bufQ_lock);

    if (!q)
    {
        return LS_OK;
    }
    return mtFlushLocalRespBodyBufQueue(q);
}


int HttpSession::mtFlushLocalRespBodyBufQueue(MtLocalBufQ *q)
{
    LS_DBG_H(getLogSession(), "%s processing %d bufs\n", __func__, q->size());

    int count = q->size();
    MtWriteBuf * w = NULL;
    while (count-- > 0 && (NULL != (w = q->pop_front())))
    {

        if (!getMtFlag(HSF_MT_CANCEL))
        {
            LS_DBG_H(getLogSession(), "%s: calling appendDynBody (%p, %lu)\n",
                     __func__, w->getBuf(), w->getCurPos() - w->getBuf());

            int ret = appendDynBody(w->getBuf(), w->getCurPos() - w->getBuf());

            LS_DBG_H(getLogSession(), "%s: appendDynBody returned %d\n",
                     __func__, ret);
            if (ret != w->getCurPos() - w->getBuf())
            {
                LS_ERROR(getLogSession(), "%s: bad appendDynBody call ret %d instead of %ld\n",
                         __func__, ret, w->getCurPos() - w->getBuf());
            }
        }
        delete [] w->getBuf();
        delete w;
    }
    delete q;
    return LS_OK;
}


#ifdef NOTUSED
int HttpSession::mtFlushLocalRespBodyBuf(void *pParam)
{
    LS_DBG_H(getLogSession(), "enter %s\n", __func__);

    if (!pParam)
    {
        return LS_FAIL;
    }

    MtFlushLocalBufParams *p = (MtFlushLocalBufParams *) pParam;

    if (getMtFlag(HSF_MT_CANCEL))
    {
        delete [] p->m_buf;
        delete p;
        return LS_FAIL;
    }

    LS_DBG_H(getLogSession(), "%s: calling appendDynBody (%p, %lu)\n",
             __func__, p->m_buf,  p->m_bufEnd - p->m_buf);

    int ret = appendDynBody(p->m_buf, p->m_bufEnd - p->m_buf);

    LS_DBG_H(getLogSession(), "%s: appendDynBody returned %d\n",
             __func__, ret);
    if (ret != p->m_bufEnd - p->m_buf)
    {
        LS_ERROR(getLogSession(), "%s: bad appendDynBody call ret %d instead of %ld\n",
                __func__, ret, p->m_bufEnd - p->m_buf);
    }
    delete [] p->m_buf;
    delete p;
    return ret;
}
#endif

int HttpSession::broadcastMtWaiters(int32_t flags)
{
    LS_DBG_L(getLogSession(), "mtNotifyCallback: broadcast flags set, clear %u",
             flags);
    MtNotifier *pNotifier = &getMtSessData()->m_event;
    LS_DBG_M(getLogSession(), "[Tm] lock event waiters lock.");
    pNotifier->lock();
    clearMtFlag(flags);
    pNotifier->broadcastEx();
    LS_DBG_M(getLogSession(), "[Tm] unlock event waiters lock.");
    pNotifier->unlock();
    return LS_OK;
}


int HttpSession::processMtEvent(long lParam, void *pParam)
{
    switch (lParam)
    {
    case HSF_MT_SET_URI_QS:
        return setUriQueryString((MtParamUriQs *) pParam);
    case HSF_MT_SENDFILE:
        mtSendfile((MtParamSendfile *)pParam);
        return broadcastMtWaiters(lParam);
    case HSF_MT_PARSE_REQ_ARGS:
        mtParseReqArgs((MtParamParseReqArgs *)pParam);
        return broadcastMtWaiters(lParam);
    case HSF_MT_FLUSH_RBDY_LBUF_Q:
        return mtFlushLocalRespBodyBufQueue();
#ifdef NOTUSED
    case HSF_MT_FLUSH_RBDY_LBUF:
        mtFlushLocalRespBodyBuf(pParam);
        return broadcastMtWaiters(lParam);
#endif
    default:
        break;
    }
    return -1;
}


int HttpSession::processMtRecycle()
{
    if (getMtFlag(HSF_MT_END))
    {
        LS_DBG_L(getLogSession(),
                "MT handler released canceled session, recycle session.");
        assert(getState() == HSS_RECYCLING);
        releaseResources();
        recycle();
    }
    else
    {
        LS_DBG_L(getLogSession(),
            "MT handler still holding recycled session, do not process, flags: %d.",
            getMtFlag(0xffffffff));
    }
    return 0;
}


int HttpSession::processMtCancel()
{
    if (getMtFlag(HSF_MT_END))
    {
        LS_DBG_L(getLogSession(), "canceled MT handler finished");
        int state = getMtSessData()->m_mtEndNextState;
        releaseMtSessData();
        if (state != 0)
        {
            setProcessState((HSPState)state);
            smProcessReq();
        }
        return 0;
    }
    LS_DBG_L(getLogSession(), "MT handler marked canceled, "
             "ignore pending flags: %X", getMtFlag(HSF_MT_MASK));
    clearMtFlag(HSF_MT_MASK);
    return -1;
}


int HttpSession::processMtEvents(long lParam, void *pParam)
{
    LS_DBG_L(getLogSession(), "processMtEvents(), flags: %X.",
             getMtFlag(0xffffffff));
    if (pParam == (void *)HSF_MT_END)
    {
        setMtFlag(HSF_MT_END);
        if (!trylockMtRace())
            LS_DBG_M(getLogSession(), "[Tm] ok trylockMtRace.");
        else
            LS_DBG_M(getLogSession(), "[Tm] failed trylockMtRace.");
    }
    if (!getMtSessData())
        return 0;

    if ((uint32_t)lParam != getSn())
    {
        LS_DBG_L(getLogSession(),
                 "processMtEvents called. sn mismatch[%d %d], Session = %p",
                 (uint32_t)lParam, getSn(), this);

        //  abort();
        return -1;
    }

    if (getMtFlag(HSF_MT_RECYCLE))
        return processMtRecycle();

    if (getMtFlag(HSF_MT_CANCEL))
        return processMtCancel();

    clearMtFlag(HSF_MT_NOTIFIED);

    if (getMtFlag(HSF_MT_INIT_RESP_BUF))
    {
        setupDynRespBodyBuf();
        clearMtFlag(HSF_MT_INIT_RESP_BUF);
        getMtSessData()->m_write.notify();
    }

    if (getMtFlag(HSF_MT_SND_RSP_HDRS))
    {
        if (!isRespHeaderSent())
        {
            sendRespHeaders();
        }
        clearMtFlag(HSF_MT_SND_RSP_HDRS);
    }

    if (getMtFlag(HSF_MT_FLUSH))
    {
        LS_DBG_L(getLogSession(), "mtNotifyCallback: flush()");
        clearMtFlag(HSF_MT_FLUSH);
        mtFlushLocalBuf();
        setFlag(HSF_RESP_FLUSHED, 0);
        flush();
    }

    if (getMtFlag(HSF_MT_END_RESP))
    {
        LS_DBG_L(getLogSession(), "mtNotifyCallback: endResponse()");
        mtFlushLocalBuf();
        endResponse(1);
        clearMtFlag(HSF_MT_END_RESP);
    }

    if (getMtFlag(HSF_MT_END))
    {
        LS_DBG_L(getLogSession(), "mtNotifyCallback: MT handler finish");
        assert(getStream());
        if (testFlag(HSF_HANDLER_DONE) == 0)
        {
            LS_DBG_L(getLogSession(), "mtNotifyCallback: MT handler does not call endResponse, call it now");
            mtFlushLocalBuf();
            endResponse(1);
        }
        releaseMtSessData();
        //return smProcessReq();
    }

    return 0;
}

int HttpSession::setUriQueryString(MtParamUriQs * param)
{
    if (!param)
    {
        return -1;
    }
    int ret = setUriQueryString(param->m_action, ls_str_cstr(param->m_uri),
                                ls_str_len(param->m_uri), ls_str_cstr(param->m_qs),
                                ls_str_len(param->m_qs));
    ls_str_delete(param->m_uri);
    ls_str_delete(param->m_qs);
    free(param);
    return ret;
}

int HttpSession::setUriQueryString(int action, const char *uri,
                      int uri_len, const char *qs, int qs_len)
{
#define URI_OP_MASK     15
#define URL_QS_OP_MASK  112
#define MAX_URI_QS_LEN 8192
    char tmpBuf[MAX_URI_QS_LEN + 8];
    char *pStart = tmpBuf;
    char *pQs = NULL;
    int final_qs_len = 0;
    int len = 0;
    int urlLen;

    int code;
    int uri_act;
    int qs_act;

    if (!action)
        return LS_OK;
    uri_act = action & URI_OP_MASK;
    if ((uri_act == LSI_URL_REDIRECT_INTERNAL) &&
        (getState() <= HSS_READING_BODY))
        uri_act = LSI_URL_REWRITE;
    if ((uri_act == LSI_URL_NOCHANGE) || (!uri) || (uri_len == 0))
    {
        uri = getReq()->getURI();
        uri_len = getReq()->getURILen();
        action &= ~LSI_URL_ENCODED;
    }
    if ((size_t)uri_len > sizeof(tmpBuf) - 4) // leave room for extra
        uri_len = sizeof(tmpBuf) - 4;

    switch (uri_act)
    {
    case LSI_URL_REDIRECT_INTERNAL:
    case LSI_URL_REDIRECT_301:
    case LSI_URL_REDIRECT_302:
    case LSI_URL_REDIRECT_303:
    case LSI_URL_REDIRECT_307:
        if (!(action & LSI_URL_ENCODED))
            len = HttpUtil::escape(uri, uri_len, tmpBuf, sizeof(tmpBuf) - 3);
        else
        {
            memcpy(tmpBuf, uri, uri_len);
            len = uri_len;
        }
        break;
    default:
        if (action & LSI_URL_ENCODED)
        {
            len = HttpUtil::unescape(uri, tmpBuf, uri_len);
            if (len == -1)
                len = 0; // To avoid a bad index below
        }
        else
        {
            memcpy(tmpBuf, uri, uri_len);
            len = uri_len;
        }
    }
    urlLen = len;

    pStart = &tmpBuf[len];
    qs_act = action & URL_QS_OP_MASK;
    if (!qs || qs_len == 0)
    {
        if ((qs_act == LSI_URL_QS_DELETE) ||
            (qs_act == LSI_URL_QS_SET))
        {
            pQs = NULL;
            final_qs_len = 0;
        }
        else if (qs_act == LSI_URL_QS_APPEND)
            qs_act = LSI_URL_QS_NOCHANGE;
    }
    else
    {
        *pStart++ = '?';
        pQs = pStart;
        if (qs_act == LSI_URL_QS_DELETE)
        {
            //TODO: remove parameter from query string match the input
            //         {
            //             pQs = pSession->getReq()->getQueryString();
            //             final_qs_len = pSession->getReq()->getQueryString();
            //         }
            qs = NULL;
            qs_len = 0;
        }
        else if (qs_act == LSI_URL_QS_APPEND)
        {
            final_qs_len = getReq()->getQueryStringLen();
            if (final_qs_len > 0)
            {
                memcpy(pStart, getReq()->getQueryString(),
                       final_qs_len);
                pStart += final_qs_len++;
                *pStart++ = '&';
            }
        }
        else
            qs_act = LSI_URL_QS_SET;
        if (qs)
        {
            memcpy(pStart, qs, qs_len);
            final_qs_len += qs_len;
            pStart += qs_len;
        }
    }
    len = pStart - tmpBuf;

    switch (uri_act)
    {
    case LSI_URL_NOCHANGE:
    case LSI_URL_REWRITE:
        if (uri_act != LSI_URL_NOCHANGE)
            getReq()->setRewriteURI(tmpBuf, urlLen, 1);
        if (qs_act & URL_QS_OP_MASK)
            getReq()->setRewriteQueryString(pQs, final_qs_len);
//        pSession->getReq()->addEnv(11);
        break;
    case LSI_URL_REDIRECT_INTERNAL:
        getReq()->setLocation(tmpBuf, len);
        setState(HSS_REDIRECT);
        continueWrite();
        break;
    case LSI_URL_REDIRECT_301:
    case LSI_URL_REDIRECT_302:
    case LSI_URL_REDIRECT_303:
    case LSI_URL_REDIRECT_307:
        getReq()->setLocation(tmpBuf, len);

        if (uri_act == LSI_URL_REDIRECT_307)
            code = SC_307;
        else
            code = SC_301 + uri_act - LSI_URL_REDIRECT_301;
        getReq()->setStatusCode(code);
        setState(HSS_EXT_REDIRECT);
        continueWrite();
        break;
    default:
        break;
    }

    return LS_OK;
}

int HttpSession::removeSessionCbs(long lParam, void * pParam)
{
    LS_DBG_M(getLogSession(), "calling removeSessionCb on this %p\n", this);
    return EvtcbQue::getInstance().removeSessionCb(this);
}
