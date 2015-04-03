/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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

#include <lsdef.h>
#include <edio/aiosendfile.h>
#include <http/chunkinputstream.h>
#include <http/chunkoutputstream.h>
#include <http/clientcache.h>
#include <http/connlimitctrl.h>
#include <http/handlerfactory.h>
#include <http/handlertype.h>
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
#include <http/reqhandler.h>
#include <http/rewriteengine.h>
#include <http/smartsettings.h>
#include <http/staticfilecache.h>
#include <http/staticfilecachedata.h>
#include <http/userdir.h>
#include <http/vhostmap.h>
#include <lsiapi/envmanager.h>
#include <lsiapi/lsiapi.h>
#include <lsiapi/lsiapihooks.h>
#include <lsr/ls_pool.h>
#include <lsr/ls_strtool.h>
#include <socket/gsockaddr.h>
#include <ssi/ssiengine.h>
// #include <ssi/ssiruntime.h>
#include <ssi/ssiscript.h>
#include <util/accesscontrol.h>
#include <util/accessdef.h>
#include <util/datetime.h>
#include <util/gzipbuf.h>
#include <util/vmembuf.h>

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
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


HttpSession::HttpSession()
    : m_request()
    , m_response(m_request.getPool())
    , m_processState(HSPS_READ_REQ_HEADER)
    , m_curHookLevel(0)
    , m_pAiosfcb(NULL)
{
    memset(&m_pChunkIS, 0, (char *)(&m_iReqServed + 1) -
           (char *)&m_pChunkIS);
    m_pModuleConfig = NULL;
    m_response.reset();
    m_request.reset();
}


HttpSession::~HttpSession()
{
    LsiapiBridge::releaseModuleData(LSI_MODULE_DATA_HTTP, getModuleData());
#ifdef LS_AIO_USE_AIO
    if (m_pAiosfcb != NULL)
        HttpResourceManager::getInstance().recycle(m_pAiosfcb);
    m_pAiosfcb = NULL;
#endif
}


int HttpSession::onInitConnected()
{
    m_pNtwkIOLink = getStream()->getNtwkIoLink();
    m_lReqTime = DateTime::s_curTime;
    m_iReqTimeUs = DateTime::s_curTimeUs;
    if (m_pNtwkIOLink)
        setVHostMap(m_pNtwkIOLink->getVHostMap());

    m_iFlag = 0;
    setState(HSS_READING);
    m_processState = HSPS_READ_REQ_HEADER;
    m_curHookLevel = 0;
    getStream()->setFlag(HIO_FLAG_WANT_READ, 1);
    m_request.setILog(getStream());
    if (m_request.getBodyBuf())
        m_request.getBodyBuf()->reinit();
#ifdef LS_AIO_USE_AIO
    if (HttpServerConfig::getInstance().getUseSendfile() == 2)
        m_pAiosfcb = HttpResourceManager::getInstance().getAiosfcb();
#endif
//     m_response.reset();
//     m_request.reset();
    return 0;
}


inline int HttpSession::getModuleDenyCode(int iHookLevel)
{
    int ret = m_request.getStatusCode();
    if ((!ret) || (ret == SC_200))
    {
        ret = SC_403;
        m_request.setStatusCode(SC_403);
    }
    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] blocked by %s hookpoint, return: %d !",
               getLogId(), LsiApiHooks::getHkptName(iHookLevel), ret));

    return ret;
}


inline int HttpSession::processHkptResult(int iHookLevel, int ret)
{
    if (((getState() == HSS_EXT_REDIRECT) || (getState() == HSS_REDIRECT))
        && (m_request.getLocation() != NULL))
    {
        //perform external redirect.
        continueWrite();
        return LS_FAIL;
    }

    if (ret < 0)
    {
        getModuleDenyCode(iHookLevel);
        setState(HSS_HTTP_ERROR);
        continueWrite();
    }
    return ret;
}


int HttpSession::runEventHkpt(int hookLevel, HSPState nextState)
{
    if (m_curHookLevel != hookLevel)
    {
        if (!m_sessionHooks.isEnabled(hookLevel))
        {
            m_processState = nextState;
            return 0;
        }
        const LsiApiHooks *pHooks = LsiApiHooks::getGlobalApiHooks(hookLevel);
        m_curHookLevel = hookLevel;
        m_curHkptParam._session = (LsiSession *)this;

        m_curHookInfo.hooks = pHooks;
        m_curHookInfo.term_fn = NULL;
        m_curHookInfo.enable_array = m_sessionHooks.getEnableArray(hookLevel);
        m_curHkptParam._cur_hook = ((lsiapi_hook_t *)pHooks->begin());
        m_curHkptParam._hook_info = &m_curHookInfo;
        m_curHkptParam._param = NULL ;
        m_curHkptParam._param_len = 0;
        m_curHkptParam._flag_out = 0;
        m_curHookRet = 0;
    }
    else  //resume current hookpoint
    {
        if (!m_curHookRet)
            m_curHkptParam._cur_hook = (lsiapi_hook_t *)m_curHkptParam._cur_hook + 1;
    }
    if (!m_curHookRet)
        m_curHookRet = m_curHookInfo.hooks->runCallback(hookLevel,
                       &m_curHkptParam);
    if (m_curHookRet <= -1)
    {
        if (m_curHookRet != LSI_HK_RET_SUSPEND)
            return getModuleDenyCode(hookLevel);
    }
    else if (!m_curHookRet)
    {
        m_curHookLevel = 0;
        m_processState = nextState;
    }
    return m_curHookRet;
}


/*
const char * HttpSession::buildLogId()
{
    AutoStr2 & id = getStream()->getIdBuf();
    int len ;
    char * p = id.buf();
    char * pEnd = p + MAX_LOGID_LEN;
    len = ls_snprintf( id.buf(), MAX_LOGID_LEN, "%s-",
                getStream()->getLogId() );
    id.setLen( len );
    p += len;
    len = ls_snprintf( p, pEnd - p, "%hu", m_iReqServed );
    p += len;
    const HttpVHost * pVHost = m_request.getVHost();
    if ( pVHost )
    {
        ls_snprintf( p, pEnd - p, "#%s", pVHost->getName() );
    }
    return id.c_str();
}
*/


void HttpSession::logAccess(int cancelled)
{
    HttpVHost *pVHost = (HttpVHost *) m_request.getVHost();
    if (pVHost)
    {
        pVHost->decRef();
        pVHost->getReqStats()->incReqProcessed();

        if (pVHost->BytesLogEnabled())
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
    else if (m_pNtwkIOLink && shouldLogAccess())
        HttpLog::logAccess(NULL, 0, this);
}


void HttpSession::resumeSSI()
{
    if (m_pSubResp)
    {
        delete m_pSubResp;
        m_pSubResp = NULL;
    }
    m_request.restorePathInfo();
    releaseSendFileInfo();
    if ((m_pGzipBuf) && (!m_pGzipBuf->isStreamStarted()))
        m_pGzipBuf->reinit();
    SSIEngine::resumeExecute(this);
    return;
}


inline void HttpSession::releaseStaticFileCacheData(StaticFileCacheData
        * &pCache)
{
    if (pCache && pCache->decRef() == 0)
        pCache->setLastAccess(DateTime::s_curTime);
}


inline void HttpSession::releaseFileCacheDataEx(FileCacheDataEx *&pECache)
{
    if (pECache && pECache->decRef() == 0)
    {
        if (pECache->getfd() != -1)
            pECache->closefd();
    }
}


void HttpSession::releaseSendFileInfo()
{
    StaticFileCacheData *&pData = m_sendFileInfo.getFileData();
    if (pData)
    {
        releaseStaticFileCacheData(pData);
        FileCacheDataEx *&pECache = m_sendFileInfo.getECache();
        releaseFileCacheDataEx(pECache);
        memset(&m_sendFileInfo, 0, sizeof(m_sendFileInfo));
    }
}


void HttpSession::nextRequest()
{

    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] HttpSession::nextRequest()!",
               getLogId()));
    getStream()->flush();

    if (m_pHandler)
    {
        HttpStats::getReqStats()->incReqProcessed();
        cleanUpHandler();
    }

//     //for SSI, should resume
//     if ( m_request.getSSIRuntime() )
//     {
//         return resumeSSI();
//     }

    if (m_pChunkOS)
    {
        m_pChunkOS->reset();
        releaseChunkOS();
    }

    if (!m_request.isKeepAlive() || getStream()->isSpdy())
    {
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(), "[%s] Non-KeepAlive, CLOSING!",
                   getLogId()));
        closeConnection();
    }
    else
    {
        if (m_iFlag & HSF_HOOK_SESSION_STARTED)
        {
            if (m_sessionHooks.isEnabled(LSI_HKPT_HTTP_END))
                m_sessionHooks.runCallbackNoParam(LSI_HKPT_HTTP_END, (LsiSession *)this);
        }

        m_sessionHooks.reset();
        m_sessionHooks.disableAll();
        m_iFlag = 0;
        logAccess(0);
        ++m_iReqServed;
        m_lReqTime = DateTime::s_curTime;
        m_iReqTimeUs = DateTime::s_curTimeUs;
        m_response.reset();
        m_request.reset2();
        releaseSendFileInfo();
        if (m_pRespBodyBuf)
            releaseRespCache();
        if (m_pGzipBuf)
            releaseGzipBuf();

        getStream()->switchWriteToRead();
        if (getStream()->isLogIdBuilt())
        {
            ls_snprintf(getStream()->getIdBuf().buf() +
                        getStream()->getIdBuf().len(), 10, "-%hu", m_iReqServed);
        }
        m_processState = HSPS_READ_REQ_HEADER;
        if (m_request.pendingHeaderDataLen())
        {
            if (D_ENABLED(DL_LESS))
                LOG_D((getLogger(),
                       "[%s] Pending data in header buffer, set state to READING!",
                       getLogId()));
            setState(HSS_READING);
            //if ( !inProcess() )
            processPending(0);
        }
        else
        {
            setState(HSS_WAITING);
            HttpStats::incIdleConns();
        }
    }
}


void HttpSession::httpError(int code, const char *pAdditional)
{
    if (code < 0)
        code = SC_500;
    m_request.setStatusCode(code);
    sendHttpError(pAdditional);
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
        m_pChunkIS->setStream(getStream());
        m_pChunkIS->open();
    }
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
    suspendRead();
    setFlag(HSF_REQ_BODY_DONE, 1);
    if (m_processState == HSPS_HANDLER_PROCESSING)
        m_processState = HSPS_HKPT_RCVD_REQ_BODY_PROCESSING;
    else
        m_processState = HSPS_HKPT_RCVD_REQ_BODY;
//     smProcessReq();



//     if ( m_sessionHooks.isEnabled(LSI_HKPT_RCVD_REQ_BODY))
//     {
//         int ret = m_sessionHooks.runCallbackNoParam(LSI_HKPT_RCVD_REQ_BODY, (LsiSession *)this);
//         if ( ret <= -1)
//         {
//             return getModuleDenyCode( LSI_HKPT_RCVD_REQ_BODY );
//         }
//     }

    return 0;
}


int HttpSession::readReqBody()
{
    char *pBuf;
    size_t size = 0;
    int ret = 0;
    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] Read Request Body!",
               getLogId()));

    const LsiApiHooks *pReadHooks = LsiApiHooks::getGlobalApiHooks(
                                        LSI_HKPT_RECV_REQ_BODY);
    int count = 0;
    int hasBufferedData = 1;
    int endBody;
    while (!(endBody = endOfReqBody()) || hasBufferedData)
    {
        hasBufferedData = 0;
        if (!endBody)
        {
            if ((ret < (int)size) || (++count == 10))
                return 0;
        }

        pBuf = m_request.getBodyBuf()->getWriteBuffer(size);
        if (!pBuf)
        {
            LOG_ERR((getLogger(),
                     "[%s] out of swapping space while reading request body!", getLogId()));
            return SC_400;
        }

        if (!pReadHooks || m_sessionHooks.isDisabled(LSI_HKPT_RECV_REQ_BODY))
            ret = readReqBodyTermination((LsiSession *)this, pBuf, size);
        else
        {
            lsi_cb_param_t param;
            lsiapi_hookinfo_t hookInfo;
            param._session = (LsiSession *)this;

            hookInfo.hooks = pReadHooks;
            hookInfo.term_fn = (filter_term_fn)readReqBodyTermination;
            hookInfo.enable_array = m_sessionHooks.getEnableArray(
                                        LSI_HKPT_RECV_REQ_BODY);
            param._cur_hook = (void *)((lsiapi_hook_t *)pReadHooks->end() - 1);
            param._hook_info = &hookInfo;
            param._param = pBuf;
            param._param_len = size;
            param._flag_out = &hasBufferedData;
            ret = LsiApiHooks::runBackwardCb(&param);
            LOG_D((getLogger(), "[%s][LSI_HKPT_RECV_REQ_BODY] read  %d ", getLogId(),
                   ret));
        }
        if (ret > 0)
        {
            m_request.getBodyBuf()->writeUsed(ret);
            if (D_ENABLED(DL_LESS))
                LOG_D((getLogger(), "[%s] Finsh request body %ld/%ld bytes!",
                       getLogId(), m_request.getContentFinished(),
                       m_request.getContentLength()));
            if (m_pHandler && !getFlag(HSF_REQ_WAIT_FULL_BODY))
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

    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] Finished request body %ld bytes!",
               getLogId(), m_request.getContentFinished()));
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


int HttpSession::resumeHandlerProcess()
{
    if (!(m_iFlag & HSF_URI_PROCESSED))
    {
        m_processState = HSPS_PROCESS_NEW_URI;
        return smProcessReq();
    }
    if (m_pHandler)
        m_pHandler->onRead(this);
    else
        return handlerProcess(m_request.getHttpHandler());
    return 0;

}


int HttpSession::restartHandlerProcess()
{
    HttpContext *pContext = &(m_request.getVHost()->getRootContext());
    m_sessionHooks.reset();
    m_sessionHooks.inherit(pContext->getSessionHooks(), 0);
    if (m_sessionHooks.isEnabled(LSI_HKPT_HANDLER_RESTART))
        m_sessionHooks.runCallbackNoParam(LSI_HKPT_HANDLER_RESTART,
                                          (LsiSession *)this);

    m_iFlag &= ~(HSF_RESP_HEADER_DONE | HSF_RESP_WAIT_FULL_BODY |
                 HSF_RESP_FLUSHED
                 | HSF_HANDLER_DONE);

    if (m_pHandler)
        cleanUpHandler();

    releaseSendFileInfo();

    if (m_pChunkOS)
    {
        m_pChunkOS->reset();
        releaseChunkOS();
    }
    if (m_pRespBodyBuf)
        releaseRespCache();
    if (m_pGzipBuf)
        releaseGzipBuf();

    m_response.reset();
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
        int toRead = pThis->m_request.getBodyRemain();
        if (toRead > size)
            toRead = size ;
        if (toRead > 0)
            len = pThis->read(pBuf, toRead);
    }
    if (len > 0)
        pThis->m_request.contentRead(len);

    return len;
}


int HttpSession::readToHeaderBuf()
{
    AutoBuf &headerBuf = m_request.getHeaderBuf();
    if (D_ENABLED(DL_LESS))
    {
        LOG_D((getLogger(), "[%s] readToHeaderBuf(). ",
               getLogId()));
    }
    do
    {
        int sz, avail;
        avail = headerBuf.available();
        if (avail > 2048)
            avail = 2048;
        char *pBuf = headerBuf.end();
        sz = getStream()->read(pBuf, avail);
        if (sz > 0)
        {
            if (D_ENABLED(DL_LESS))
            {
                LOG_D((getLogger(), "[%s] read %d bytes to header buffer",
                       getLogId(), sz));
            }
            headerBuf.used(sz);

            int ret = m_request.processHeader();
            if (D_ENABLED(DL_LESS))
            {
                LOG_D((getLogger(), "[%s] processHeader() return %d, header state: %d. ",
                       getLogId(), ret, m_request.getStatus()));
            }
            if (ret != 1)
                return ret;
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
                    LOG_NOTICE((getLogger(),
                                "[%s] Http request header is too big, abandon!",
                                getLogId()));
                    //m_request.setHeaderEnd();
                    //m_request.dumpHeader();
                    return SC_400;
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
    if (D_ENABLED(DL_LESS))
    {
        LOG_D((getLogger(), "[%s] %d bytes pending in request header buffer:",
               getLogId(), m_request.pendingHeaderDataLen()));
//            ::write( 2, m_request.getHeaderBuf().begin(),
//                      m_request.pendingHeaderDataLen() );
    }
    ret = m_request.processHeader();
    if (ret == 1)
    {
        if (isSSL())
            smProcessReq();
        return;
    }
    if ((!ret) && (m_request.getStatus() == HttpReq::HEADER_OK))
    {
        m_iFlag &= ~HSF_URI_PROCESSED;
        m_processState = HSPS_NEW_REQ;
        smProcessReq();
    }

}


int HttpSession::updateClientInfoFromProxyHeader(const char *pProxyHeader)
{
    char achIP[128];
    int len = m_request.getHeaderLen(HttpHeader::H_X_FORWARDED_FOR);
    char *p = (char *)memchr(pProxyHeader, ',', len);
    if (p)
        len = p - pProxyHeader;
    if ((len <= 0) || (len > 16))
    {
        //error, not a valid IPv4 address
        return 0;
    }

    memmove(achIP, pProxyHeader, len);
    achIP[len] = 0;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(achIP);
    if (addr.sin_addr.s_addr == INADDR_BROADCAST)
    {
        //Failed to parse the address string to IPv4 address
        return 0;
    }
    ClientInfo *pInfo = ClientCache::getClientCache()
                        ->getClientInfo((struct sockaddr *)&addr);
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

    m_pNtwkIOLink->changeClientInfo(pInfo);
    return 0;
}


int HttpSession::processWebSocketUpgrade(const HttpVHost *pVHost)
{
    HttpContext *pContext = pVHost->getContext(m_request.getURI(), 0);
    LOG_D((getLogger(), "[%s] VH: web socket, name: [%s] URI: [%s]",
           getLogId(),
           pVHost->getName(), m_request.getURI()));
    if (pContext && pContext->getWebSockAddr()->get())
    {
        m_request.setStatusCode(SC_101);
        logAccess(0);
        L4Handler *pL4Handler = new L4Handler();
        pL4Handler->assignStream(getStream());
        pL4Handler->init(m_request, pContext->getWebSockAddr(),
                         getPeerAddrString(),
                         getPeerAddrStrLen());
        LOG_D((getLogger(), "[%s] VH: %s web socket !!!", getLogId(),
               pVHost->getName()));
        m_response.reset();
        m_request.reset2();
        recycle();
        return 0;
        //DO NOT release pL4Handler, it will be releaseed itself.
    }
    else
    {
        LOG_INFO((getLogger(), "[%s] Cannot found web socket backend URI: [%s]",
                  getLogId(), m_request.getURI()));
        m_request.keepAlive(0);
        return SC_404;
    }
}


int HttpSession::processHttp2Upgrade(const HttpVHost *pVHost)
{
    getNtwkIOLink()->switchToHttp2Handler(this);
    return 0;
}


int HttpSession::hookResumeCallback(int level, lsi_module_t *pModule)
{
    return resumeProcess(0, 0);
}


int HttpSession::processNewReqInit()
{
    int ret;
    HttpServerConfig &httpServConf = HttpServerConfig::getInstance();
    const HttpVHost *pVHost = m_request.matchVHost();
    if (!pVHost)
    {
        if (D_ENABLED(DL_LESS))
        {
            char *pHostEnd = (char *)m_request.getHostStr() +
                             m_request.getHostStrLen();
            char ch = *pHostEnd;
            *pHostEnd = 0;
            LOG_D((getLogger(), "[%s] can not find a matching VHost for [%s]",
                   m_request.getHostStr(), getLogId()));
            *pHostEnd = ch;
        }
        return SC_404;
    }

    getStream()->setLogger(pVHost->getLogger());


    if (getStream()->isSpdy())
    {
        m_request.keepAlive(0);
        m_request.orGzip(REQ_GZIP_ACCEPT | httpServConf.getGzipCompress());
    }
    if ((httpServConf.getUseProxyHeader() == 1)
        || ((httpServConf.getUseProxyHeader() == 2)
            && (getClientInfo()->getAccess() == AC_TRUST)))
    {
        const char *pProxyHeader = m_request.getHeader(
                                       HttpHeader::H_X_FORWARDED_FOR);
        if (*pProxyHeader)
        {
            ret = updateClientInfoFromProxyHeader(pProxyHeader);
            if (ret)
                return ret;
        }
    }
    m_lReqTime = DateTime::s_curTime;
    m_iReqTimeUs = DateTime::s_curTimeUs;


    m_iFlag &= ~HSF_ACCESS_LOG_OFF;

    if (getStream()->isLogIdBuilt())
    {
        AutoStr2 &id = getStream()->getIdBuf();
        char *p = id.buf() + id.len();
        while (*p && *p != '#')
            ++p;
        *p++ = '#';
        memccpy(p, pVHost->getName(), 0, id.buf() + MAX_LOGID_LEN - p);
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
        m_processState = HSPS_WEBSOCKET;
        return processWebSocketUpgrade(pVHost);
    }
    else if (httpServConf.getEnableH2c() == 1 && m_request.isHttp2Upgrade())
    {
        //  m_processState = HSPS_WEBSOCKET;
        processHttp2Upgrade(pVHost);
    }


    if ((m_pNtwkIOLink->isThrottle())
        && (m_pNtwkIOLink->getClientInfo()->getAccess() != AC_TRUST))
        m_pNtwkIOLink->getThrottleCtrl()->adjustLimits(
            pVHost->getThrottleLimits());

    if (m_request.isKeepAlive())
    {
        if (m_iReqServed >= pVHost->getMaxKAReqs())
        {
            m_request.keepAlive(false);
            if (D_ENABLED(DL_LESS))
                LOG_D((getLogger(), "[%s] Reached maximum requests on keep"
                       " alive connection, keep-alive off.",
                       getLogId()));
        }
        else if (ConnLimitCtrl::getInstance().lowOnConnection())
        {
            ConnLimitCtrl::getInstance().setConnOverflow(1);
            m_request.keepAlive(false);
            if (D_ENABLED(DL_LESS))
                LOG_D((getLogger(), "[%s] Running short of connection"
                       ", keep-alive off.",
                       getLogId()));
        }
        else if (m_pNtwkIOLink->getClientInfo()->getOverLimitTime())
        {
            m_request.keepAlive(false);
            if (D_ENABLED(DL_LESS))
                LOG_D((getLogger(), "[%s] Connections is over the soft limit"
                       ", keep-alive off.",
                       getLogId()));
        }
    }

    m_request.setStatusCode(SC_200);
    //Run LSI_HKPT_HTTP_BEGIN after the inherit from vhost
    m_iFlag |= HSF_HOOK_SESSION_STARTED;
    m_processState = HSPS_HKPT_HTTP_BEGIN;
    return 0;
}


int HttpSession::processNewReqBody()
{
    int ret = 0;
    off_t l = m_request.getContentLength();
    if (l != 0)
    {
        if (l > HttpServerConfig::getInstance().getMaxReqBodyLen())
        {
            LOG_NOTICE((getLogger(),
                        "[%s] Request body size: %ld is too big!",
                        getLogId(), l));
            getStream()->setFlag(HIO_FLAG_WANT_READ, 0);
            getStream()->suspendRead();
            return SC_413;
        }
        ret = m_request.prepareReqBodyBuf();
        if (ret)
            return ret;
        setState(HSS_READING_BODY);
        if (m_request.isChunked())
            setupChunkIS();
        //else
        //    m_request.processReqBodyInReqHeaderBuf();
        ret = readReqBody();
        if (!ret)
        {
            if (getFlag(HSF_REQ_WAIT_FULL_BODY | HSF_REQ_BODY_DONE) ==
                HSF_REQ_WAIT_FULL_BODY)
                m_processState = HSPS_READ_REQ_BODY;
            else if (m_processState != HSPS_HKPT_RCVD_REQ_BODY)
                m_processState = HSPS_PROCESS_NEW_URI;
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
                LOG_INFO((getLogger(), "[%s] Authentication failed with user: '%s'.",
                          getLogId(), pAuthUser));
        }
        else //save matched result
        {
            if (resume)
                ret = SC_401;
//            else
//                m_request.saveMatchedResult();
        }
    }
    return ret;


}


// void HttpSession::resumeAuthentication()
// {
//     int ret = processURI( 1 );
//     if ( ret )
//         httpError( ret );
// }


int HttpSession::checkAuthorizer(const HttpHandler *pHandler)
{
    int ret = assignHandler(pHandler);
    if (ret)
        return ret;
    setState(HSS_EXT_AUTH);
    return m_pHandler->process(this, pHandler);
}


void HttpSession::authorized()
{
    if (m_pHandler)
        cleanUpHandler();
    m_processState = HSPS_HKPT_HTTP_AUTH;
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
                   LsiSession *)this,
               cb, (void *)pValue, valLen)))
        return ;
}


int HttpSession::redirect(const char *pNewURL, int len, int alloc)
{
    int ret = m_request.redirect(pNewURL, len, alloc);
    if (ret)
        return ret;
    m_sendFileInfo.reset();
    if (m_pHandler)
        cleanUpHandler();
    m_request.setHandler(NULL);
    m_request.setContext(NULL);
    m_processState = HSPS_PROCESS_NEW_URI;
    return smProcessReq();
}


int HttpSession::processVHostRewrite()
{
    const HttpContext *pContext = &(m_request.getVHost()->getRootContext());
    int ret = 0;
    if ((!m_request.getContextState(SKIP_REWRITE))
        && pContext->rewriteEnabled() & REWRITE_ON)
    {
        ret = RewriteEngine::getInstance().processRuleSet(
                                                pContext->getRewriteRules(),
                                                this, NULL, NULL);

        if (ret == -3)      //rewrite happens
        {
            m_request.postRewriteProcess(
                    RewriteEngine::getInstance().getResultURI(),
                    RewriteEngine::getInstance().getResultURILen());
            ret = 0;
        }
        else if (ret)
        {
            if (D_ENABLED(DL_LESS))
                LOG_D((getLogger(), "[%s] processRuleSet() return %d",
                       getLogId(), ret));
            m_iFlag &= ~HSF_URI_PROCESSED;
            m_request.setContext(pContext);
            m_sessionHooks.reset();
            m_sessionHooks.inherit(((HttpContext *)pContext)->getSessionHooks(), 0);
            m_pModuleConfig = ((HttpContext *)pContext)->getModuleConfig();
        }
    }
    m_processState = HSPS_CONTEXT_MAP;
    return ret;
}


int HttpSession::processContextMap()
{
    int ret;
    m_iFlag |= HSF_URI_PROCESSED;
    ret = m_request.processContext();
    if (ret)
    {
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(), "[%s] processContext() return %d",
                   getLogId(), ret));
        if (ret == -2)
        {
            m_processState = HSPS_CONTEXT_AUTH;
            ret = 0;
        }
    }
    else
        m_processState = HSPS_CONTEXT_REWRITE;
    return ret;
}


int HttpSession::processContextRewrite()
{
    int ret = 0;
    const HttpContext *pContext = m_request.getContext();
    const HttpContext *pVHostRoot = &m_request.getVHost()->getRootContext();
    //context level URL rewrite
    if ((!m_request.getContextState(SKIP_REWRITE))
        && (pContext->rewriteEnabled() & REWRITE_ON))
    {

        while ((!pContext->hasRewriteConfig())
               && (pContext->getParent())
               && (pContext->getParent() != pVHostRoot))
            pContext = pContext->getParent();
        if (pContext->getRewriteRules())
        {
            ret = RewriteEngine::getInstance().processRuleSet(
                                                pContext->getRewriteRules(),
                                                this, pContext, pVHostRoot);
        }
    }
    if (ret)
    {
        if (ret == -3)
        {
            ret = 0;
            if (m_request.postRewriteProcess(
                        RewriteEngine::getInstance().getResultURI(),
                        RewriteEngine::getInstance().getResultURILen()))
            {
                m_iFlag &= ~HSF_URI_PROCESSED;
                m_processState = HSPS_CONTEXT_AUTH;
            }
            else
                m_processState = HSPS_HKPT_URI_MAP;
        }
        else if (ret == -2)
        {
            ret = 0;
            m_request.setContext(pContext);
            m_sessionHooks.reset();
            m_sessionHooks.inherit(((HttpContext *)pContext)->getSessionHooks(), 0);
            m_pModuleConfig = ((HttpContext *)pContext)->getModuleConfig();
            m_processState = HSPS_CONTEXT_AUTH;
        }
    }
    else
        m_processState = HSPS_HKPT_URI_MAP;

    return ret;
}


int HttpSession::processFileMap()
{
    if (getReq()->getHttpHandler() == NULL ||
        getReq()->getHttpHandler()->getHandlerType() != HandlerType::HT_MODULE)
    {
        int ret = m_request.processContextPath();
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(), "[%s] processContextPath() return %d",
                   getLogId(), ret));
        if (ret == -1)        //internal redirect
            m_iFlag &= ~HSF_URI_PROCESSED;
        else if (ret > 0)
        {
            if (ret == SC_404)
                m_iFlag |= HSF_SC_404;
            else
                return ret;
        }
    }
    m_processState = HSPS_CONTEXT_AUTH;
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
                    LOG_INFO((getLogger(),
                              "[%s][ACL] Access to context [%s] is denied!",
                              getLogId(), m_request.getContext()->getURI()));
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
                    if (D_ENABLED(DL_LESS))
                        LOG_D((getLogger(), "[%s] checkAuthentication() return %d",
                               getLogId(), ret1));
                    if (ret1 == -1)     //processing authentication
                    {
                        setState(HSS_EXT_AUTH);
                        return 0;
                    }
                    return ret1;
                }
            }
            if (aaa.m_pAuthorizer)
                return checkAuthorizer(aaa.m_pAuthorizer);
        }
    }
    m_processState = HSPS_HKPT_HTTP_AUTH;
    return 0;
}


int HttpSession::processAuthorizer()
{
    return 0;
}


int HttpSession::processNewUri()
{
    if (getReq()->getHttpHandler() != NULL &&
        getReq()->getHttpHandler()->getHandlerType() == HandlerType::HT_MODULE)
        m_processState = HSPS_BEGIN_HANDLER_PROCESS;
    else
        m_processState = HSPS_VHOST_REWRITE;
    return 0;
}


bool ReqHandler::notAllowed(int Method) const
{
    return (Method > HttpMethod::HTTP_POST);
}


int HttpSession::setUpdateStaticFileCache(StaticFileCacheData *&pCache,
        FileCacheDataEx *&pECache,
        const char *pPath, int pathLen,
        int fd, struct stat &st)
{
    int ret;
    ret = StaticFileCache::getInstance().getCacheElement(pPath, pathLen, st,
                                                         fd, pCache, pECache);
    if (ret)
    {
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(), "[%s] getCacheElement() return %d",
                   getLogId(), ret));
        pCache = NULL;
        return ret;
    }
    pCache->setLastAccess(DateTime::s_curTime);
    pCache->incRef();
    return 0;
}


int HttpSession::getParsedScript(SSIScript *&pScript)
{
    int ret;
    StaticFileCacheData *&pCache = m_sendFileInfo.getFileData();
    FileCacheDataEx *&pECache = m_sendFileInfo.getECache();

    const AutoStr2 *pPath = m_request.getRealPath();
    ret = setUpdateStaticFileCache(pCache, pECache, pPath->c_str(),
                                   pPath->len(),
                                   m_request.transferReqFileFd(), m_request.getFileStat());
    if (ret)
        return ret;

    pScript = pCache->getSSIScript();
    if (!pScript)
    {
        SSITagConfig *pTagConfig = NULL;
        if (m_request.getVHost())
            pTagConfig = m_request.getVHost()->getSSITagConfig();
        pScript = new SSIScript();
        ret = pScript->parse(pTagConfig, pCache->getKey());
        if (ret == -1)
        {
            delete pScript;
            pScript = NULL;
        }
        else
            pCache->setSSIScript(pScript);
    }
    return 0;
}


int HttpSession::startServerParsed()
{
    SSIScript *pScript = NULL;
    getParsedScript(pScript);
    if (!pScript)
        return SC_500;

    return SSIEngine::startExecute(this, pScript);
}


int HttpSession::handlerProcess(const HttpHandler *pHandler)
{
    m_processState = HSPS_HANDLER_PROCESSING;
    if (m_pHandler)
        cleanUpHandler();
    int type = pHandler->getHandlerType();
    if ((type >= HandlerType::HT_DYNAMIC) &&
        (type != HandlerType::HT_PROXY))
        if (m_request.checkScriptPermission() == 0)
            return SC_403;
    if (pHandler->getHandlerType() == HandlerType::HT_SSI)
    {
        const HttpContext *pContext = m_request.getContext();
        if (pContext && !pContext->isIncludesOn())
        {
            LOG_INFO((getLogger(),
                      "[%s] Server Side Include is disabled for [%s], deny access.",
                      getLogId(), pContext->getLocation()));
            return SC_403;
        }
        return startServerParsed();
    }
    //PORT_FIXME: turn off for now
    /*
    int dyn = ( pHandler->getHandlerType() >= HandlerType::HT_DYNAMIC );
    ThrottleControl * pTC = &getClientInfo()->getThrottleCtrl();
    if (( getClientInfo()->getAccess() == AC_TRUST)||
        ( pTC->allowProcess( dyn )))
    {
    }
    else
    {
        if ( D_ENABLED( DL_LESS ))
        {
            const ThrottleUnit * pTU = pTC->getThrottleUnit( dyn );
            LOG_D(( getLogger(), "[%s] %s throttling %d/%d",
                   getLogId(), (dyn)?"Dyn":"Static", pTU->getAvail(), pTU->getLimit() ));
            if ( dyn )
            {
                pTU = pTC->getThrottleUnit( 2 );
                LOG_D(( getLogger(), "[%s] dyn processor in use %d/%d",
                   getLogId(), pTU->getAvail(), pTU->getLimit() ));
            }
        }
        setState( HC_THROTTLING );
        return 0;
    }
    */
    if (m_request.getContext() && m_request.getContext()->isRailsContext())
        setFlag(HSF_REQ_WAIT_FULL_BODY);
    if (getFlag(HSF_REQ_WAIT_FULL_BODY | HSF_REQ_BODY_DONE) ==
        HSF_REQ_WAIT_FULL_BODY)
        return 0;

    int ret = assignHandler(m_request.getHttpHandler());
    if (ret)
        return ret;

    setState(HSS_PROCESSING);
    //PORT_FIXME: turn off for now
    //pTC->incReqProcessed( m_pHandler->getType() == HandlerType::HT_DYNAMIC );

    ret = m_pHandler->process(this, m_request.getHttpHandler());


    if (ret == 1)
    {
#ifdef LS_AIO_USE_AIO
        if (getFlag(HSF_AIO_READING) == 0)
#endif
            continueWrite();
        ret = 0;
    }
//     NOTICE removed because of double call
//     else if ((ret == 0) && (HSS_COMPLETE == getState()))
//         nextRequest();
    return ret;
}


int HttpSession::assignHandler(const HttpHandler *pHandler)
{
    ReqHandler *pNewHandler;
    int handlerType;
    handlerType = pHandler->getHandlerType();
    pNewHandler = HandlerFactory::getHandler(handlerType);
    if (pNewHandler == NULL)
    {
        LOG_ERR((getLogger(), "[%s] Handler with type id [%d] is not implemented.",
                 getLogId(),  handlerType));
        return SC_500;
    }

//    if ( pNewHandler->notAllowed( m_request.getMethod() ) )
//    {
//        if ( D_ENABLED( DL_LESS ))
//            LOG_D(( getLogger(), "[%s] Method %s is not allowed.",
//                getLogId(),  HttpMethod::get( m_request.getMethod() )));
//        return SC_405;
//    }

//    if ( D_ENABLED( DL_LESS ))
//    {
//        LOG_D(( getLogger(),
//            "[%s] handler with type:%d assigned.",
//            getLogId(), handlerType ));
//    }
    if (m_pHandler)
        cleanUpHandler();
    m_pHandler = pNewHandler;
    switch (handlerType)
    {
    case HandlerType::HT_FASTCGI:
    case HandlerType::HT_CGI:
    case HandlerType::HT_SERVLET:
    case HandlerType::HT_PROXY:
    case HandlerType::HT_LSAPI:
    case HandlerType::HT_MODULE:
    case HandlerType::HT_LOADBALANCER:
        {
            if (m_request.getStatus() != HttpReq::HEADER_OK)
                return SC_400;  //cannot use dynamic request handler to handle invalid request
            /*        if ( m_request.getMethod() == HttpMethod::HTTP_POST )
                        getClientInfo()->setLastCacheFlush( DateTime::s_curTime );  */
//         if ( m_request.getSSIRuntime() )
//         {
//             if (( m_request.getSSIRuntime()->isCGIRequired() )&&
//                 ( handlerType != HandlerType::HT_CGI ))
//             {
//                 LOG_INFO(( getLogger(), "[%s] Server Side Include request a CGI script, [%s] is not a CGI script, access denied.",
//                     getLogId(), m_request.getURI() ));
//                 return SC_403;
//             }
//             assert( m_pSubResp == NULL );
//             m_pSubResp = new HttpResp();
//         }
//         else

            const char *pType = HandlerType::getHandlerTypeString(handlerType);
            if (D_ENABLED(DL_LESS))
            {
                LOG_D((getLogger(),
                       "[%s] run %s processor.",
                       getLogId(), pType));
            }

            if (getStream()->isLogIdBuilt())
            {
                AutoStr2 &id = getStream()->getIdBuf();
                char *p = id.buf() + id.len();
                while (*p && *p != ':')
                    ++p;

                int n = id.buf() + MAX_LOGID_LEN - 1 - p;
                if (n > 0)
                {
                    *p++ = ':';
                    memccpy(p, pType, 0, n);
                }
            }
            if (!HttpServerConfig::getInstance().getDynGzipCompress())
                m_request.andGzip(~GZIP_ENABLED);
            //m_response.reset();
            break;
        }
    default:
        break;
    }
    return 0;
}


void HttpSession::sendHttpError(const char *pAdditional)
{
    int statusCode = m_request.getStatusCode();
    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] HttpSession::sendHttpError(),code=%s",
               getLogId(),
               HttpStatusCode::getInstance().getCodeString(m_request.getStatusCode()) + 1));
    if ((statusCode < 0) || (statusCode >= SC_END))
    {
        LOG_ERR(("[%s] invalid HTTP status code: %d!", getLogId(), statusCode));
        statusCode = SC_500;
    }
    if (statusCode == SC_503)
        HttpStats::inc503Errors();
    if (isRespHeaderSent())
    {
        endResponse(0);
        return;
    }
//     if ( m_request.getSSIRuntime() )
//     {
//         if ( statusCode < SC_500 )
//         {
//             SSIEngine::printError( this, NULL );
//         }
//         continueWrite();
//         setState( HSS_COMPLETE );
//         return;
//     }
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
        if (pContext)
        {
            const AutoStr2 *pErrDoc;
            pErrDoc = pContext->getErrDocUrl(statusCode);
            if (pErrDoc)
            {
                if (statusCode == SC_401)
                    m_request.orContextState(KEEP_AUTH_INFO);
                if (*pErrDoc->c_str() == '"')
                    pAdditional = pErrDoc->c_str() + 1;
                else
                {
                    m_request.setErrorPage();
                    if ((statusCode >= SC_300) && (statusCode <= SC_403))
                    {
                        if (m_request.getContextState(REWRITE_REDIR))
                            m_request.orContextState(SKIP_REWRITE);
                    }

                    assert(pErrDoc->len() < 2048);
                    int ret = redirect(pErrDoc->c_str(), pErrDoc->len(), 1);
                    if (ret == 0)
                        return;
                    if ((ret != m_request.getStatusCode())
                        && (m_request.getStatusCode() == SC_404)
                        && (ret != SC_503) && (ret > 0))
                    {
                        httpError(ret, NULL);
                        return;
                    }
                }
            }
        }
        /*
                if ( pContext )
                {
                    const AutoStr2 * pErrDoc;
                    pErrDoc = pContext->getErrDocUrl( m_request.getStatusCode() );
                    if ( pErrDoc )
                    {
                        if ( m_request.getStatusCode() == SC_401 )
                            m_request.orContextState( KEEP_AUTH_INFO );
                        if ( redirect( pErrDoc->c_str(), pErrDoc->len() ) == 0 )
                            return;
                    }
                }
                */
    }
    setState(HSS_WRITING);
    buildErrorResponse(pAdditional);
    endResponse(1);
    HttpStats::getReqStats()->incReqProcessed();
}


int HttpSession::buildErrorResponse(const char *errMsg)
{
    int errCode = m_request.getStatusCode();
//     if ( m_request.getSSIRuntime() )
//     {
//         if (( errCode >= SC_300 )&&( errCode < SC_400 ))
//         {
//             SSIEngine::appendLocation( this, m_request.getLocation(),
//                  m_request.getLocationLen() );
//             return 0;
//         }
//         if ( errMsg == NULL )
//             errMsg = HttpStatusCode::getInstance().getRealHtml( errCode );
//         if ( errMsg )
//         {
//             appendDynBody( 0, errMsg, strlen(errMsg));
//         }
//         return 0;
//     }

    resetResp();
    //m_response.prepareHeaders( &m_request );
    //register int errCode = m_request.getStatusCode();
    unsigned int ver = m_request.getVersion();
    if (ver > HTTP_1_0)
    {
        LOG_ERR(("[%s] invalid HTTP version: %d!", getLogId(), ver));
        ver = HTTP_1_1;
    }
    if (sendBody())
    {
        const char *pHtml = HttpStatusCode::getInstance().getRealHtml(errCode);
        if (pHtml)
        {
            int len = HttpStatusCode::getInstance().getBodyLen(errCode);
            m_response.setContentLen(len);
            m_response.getRespHeaders().add(HttpRespHeaders::H_CONTENT_TYPE,
                                            "text/html",
                                            9);
            if (errCode >= SC_307)
            {
                m_response.getRespHeaders().add(HttpRespHeaders::H_CACHE_CTRL,
                                                "private, no-cache, max-age=0", 28);
                m_response.getRespHeaders().add(HttpRespHeaders::H_PRAGMA, "no-cache", 8);
            }

            int ret = appendDynBody(pHtml, len);
            if (ret < len)
                LOG_ERR(("[%s] Failed to create error response body" , getLogId()));

            return 0;
        }
        else
        {
            m_response.setContentLen(0);
            //m_request.setNoRespBody();
        }
    }
    return 0;
}


int HttpSession::onReadEx()
{
    //printf( "^^^HttpSession::onRead()!\n" );
    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] HttpSession::onReadEx(), state: %d!",
               getLogId(), getState()));

    int ret = 0;
    switch (getState())
    {
    case HSS_WAITING:
        HttpStats::decIdleConns();
        setState(HSS_READING);
    //fall through;
    case HSS_READING:
        ret = smProcessReq();
//         ret = readToHeaderBuf();
//         if ( D_ENABLED( DL_LESS ))
//         {
//             LOG_D(( getLogger(), "[%s] readToHeaderBuf() return %d. ",
//                 getLogId(), ret ));
//         }
//         if ((!ret)&&( m_request.getStatus() == HttpReq::HEADER_OK ))
//         {
//             m_iFlag &= ~HSF_URI_PROCESSED;
//             m_processState = HSPS_NEW_REQ;
//             ret = smProcessReq();
//             if ( D_ENABLED( DL_LESS ))
//             {
//                 LOG_D(( getLogger(), "[%s] smProcessReq() return %d. ",
//                     getLogId(), ret ));
//             }
//
//         }
        break;

    case HSS_COMPLETE:
        break;

    case HSS_READING_BODY:
    case HSS_PROCESSING:
    case HSS_WRITING:
        if (m_request.getBodyBuf() && !getFlag(HSF_REQ_BODY_DONE))
        {
            ret = readReqBody();
            //TODO: This is a temp fix.  Cannot put smProcessReq in reqBodyDone
            // because it will call state machine twice, results in seg fault.
            // Basically, need to call state machine once it's finished reading
            if (m_processState == HSPS_HKPT_RCVD_REQ_BODY_PROCESSING
                || m_processState == HSPS_HKPT_RCVD_REQ_BODY)
                ret = smProcessReq();
            //if (( !ret )&& getFlag( HSF_REQ_BODY_DONE ))
            //    ret = resumeHandlerProcess();
            break;
        }
        else
        {
            if (m_pNtwkIOLink)
            {
                if (m_pNtwkIOLink->detectCloseNow())
                    return 0;
            }
            getStream()->setFlag(HIO_FLAG_WANT_READ, 0);
        }
    default:
        suspendRead();
        return 0;
    }

    if (HSS_COMPLETE == getState())
        nextRequest();

    //processPending( ret );
//    printf( "onRead loops %d times\n", iCount );
    return 0;
}


int HttpSession::doWrite()
{
    int ret = 0;
    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] HttpSession::doWrite()!",
               getLogId()));
    ret = flush();
    if (ret)
        return ret;

    if (m_pHandler
        && !(m_iFlag & (HSF_HANDLER_DONE | HSF_HANDLER_WRITE_SUSPENDED)))
    {
        ret = m_pHandler->onWrite(this);
        if (D_ENABLED(DL_MORE))
            LOG_D((getLogger(), "[%s] m_pHandler->onWrite() return %d\n",
                   getLogId(), ret));
    }
    else
        suspendWrite();

    if (ret == 0 &&
        ((m_iFlag & (HSF_HANDLER_DONE |
                     HSF_RECV_RESP_BUFFERED |
                     HSF_SEND_RESP_BUFFERED)) == HSF_HANDLER_DONE))
    {
        if (getRespCache() && !getRespCache()->empty())
            return 1;

        setFlag(HSF_RESP_FLUSHED, 0);
        flush();
    }
    else if (ret == -1)
        getStream()->setState(HIOS_CLOSING);

    return ret;
}


int HttpSession::onWriteEx()
{
    //printf( "^^^HttpSession::onWrite()!\n" );
    int ret = 0;
    switch (getState())
    {
    case HSS_THROTTLING:
        if (m_pNtwkIOLink->getClientInfo()->getThrottleCtrl().allowProcess(
                m_pHandler->getType()))
        {
            setState(HSS_PROCESSING);
            m_pNtwkIOLink->getClientInfo()->getThrottleCtrl().incReqProcessed(
                m_pHandler->getType());
            ret = m_pHandler->process(this, m_request.getHttpHandler());
            if ((ret) && (getStream()->getState() < HIOS_SHUTDOWN))
                httpError(ret);
        }
        break;
    case HSS_WRITING:
        doWrite();
        break;
    case HSS_COMPLETE:
        break;
    case HSS_REDIRECT:
    case HSS_EXT_REDIRECT:
    case HSS_HTTP_ERROR:
        if (isRespHeaderSent())
        {
            closeConnection();
            return LS_FAIL;
        }

        restartHandlerProcess();
        suspendWrite();
        {
            const char *pLocation = m_request.getLocation();
            if (pLocation)
            {
                if (getState() == HSS_REDIRECT)
                {
                    int ret = redirect(pLocation, m_request.getLocationLen());
                    if (!ret)
                        break;

//                     if ( m_request.getSSIRuntime() )
//                     {
//                         SSIEngine::printError( this, NULL );
//                         continueWrite();
//                         setState( HSS_COMPLETE );
//                         return 0;
//                     }

                    m_request.setStatusCode(ret);
                }
                if (m_request.getStatusCode() < SC_300)
                {
                    LOG_INFO((getLogger(), "[%s] redirect status code: %d",
                              getLogId(), m_request.getStatusCode()));
                    m_request.setStatusCode(SC_307);
                }
            }
            sendHttpError(NULL);
        }
        break;
    default:
        suspendWrite();
        break;
    }
    if (HSS_COMPLETE == getState())
        nextRequest();
    else if ((m_iFlag & HSF_HANDLER_DONE) && (m_pHandler))
    {
        HttpStats::getReqStats()->incReqProcessed();
        cleanUpHandler();
    }
    //processPending( ret );
    return 0;
}


void HttpSession::cleanUpHandler()
{
    ReqHandler *pHandler = m_pHandler;
    m_pHandler = NULL;
    pHandler->cleanUp(this);
}


int HttpSession::onCloseEx()
{
    closeConnection();
    return 0;
}


void HttpSession::closeConnection()
{
    if (m_pHandler)
    {
        HttpStats::getReqStats()->incReqProcessed();
        cleanUpHandler();
    }

    if (m_iFlag & HSF_HOOK_SESSION_STARTED)
    {
        if (m_sessionHooks.isEnabled(LSI_HKPT_HTTP_END))
            m_sessionHooks.runCallbackNoParam(LSI_HKPT_HTTP_END, (LsiSession *)this);
    }
    if (getStream()->getState() == HIOS_CLOSING)
        getStream()->setState(HIOS_CLOSING);
    if (getStream()->isReadyToRelease())
        return;

    m_request.keepAlive(0);

    logAccess(getStream()->getFlag(HIO_FLAG_PEER_SHUTDOWN));

    if (m_pSubResp)
    {
        delete m_pSubResp;
        m_pSubResp = NULL;
    }

    m_lReqTime = DateTime::s_curTime;
//     m_response.reset();
    m_request.reset();
    m_request.resetHeaderBuf();
    releaseSendFileInfo();

    if (m_pChunkOS)
    {
        m_pChunkOS->reset();
        releaseChunkOS();
    }
    if (m_pRespBodyBuf)
        releaseRespCache();

    if (m_pGzipBuf)
        releaseGzipBuf();

    if (m_pChunkIS)
    {
        HttpResourceManager::getInstance().recycle(m_pChunkIS);
        m_pChunkIS = NULL;
    }

    //For reuse condition, need to reste the sessionhooks
    m_sessionHooks.reset();
    getStream()->handlerReadyToRelease();
    getStream()->close();
}


void HttpSession::recycle()
{
    if (getFlag(HSF_AIO_READING))
    {
        if (D_ENABLED(DL_MEDIUM))
            LOG_D((getLogger(), "[%s] Setting Cancel Flag! \n", getLogId()));
        m_pAiosfcb->setFlag(AIOSFCB_FLAG_CANCEL);
        return;
    }
    if (D_ENABLED(DL_MORE))
        LOG_D((getLogger(), "[%s] HttpSession::recycle()\n", getLogId()));
#ifdef LS_AIO_USE_AIO
    if (m_pAiosfcb != NULL)
    {
        m_pAiosfcb->reset();
        HttpResourceManager::getInstance().recycle(m_pAiosfcb);
        m_pAiosfcb = NULL;
    }
#endif
    HttpResourceManager::getInstance().recycle(this);
}


void HttpSession::setupChunkOS(int nobuffer)
{
    if (!m_request.isKeepAlive())
        return;
    m_response.setContentLen(LSI_RESP_BODY_SIZE_CHUNKED);
    if ((m_request.getVersion() == HTTP_1_1) && (nobuffer != 2))
    {
        m_response.appendChunked();
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(), "[%s] use CHUNKED encoding!",
                   getLogId()));
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

int HttpSession::chunkSendfile(int fdSrc, off_t off, size_t size)
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
            break;;
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


int HttpSession::writeRespBodySendFile(int fdFile, off_t offset,
                                       size_t size)
{
    int written;
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
        if (D_ENABLED(DL_MEDIUM))
            LOG_D((getLogger(),
                   "[%s] Response body sent: %lld \n",
                   getLogId(), (long long)m_response.getBodySent()));
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
        m_response.written(written);
    return written;
}


static char s_errTimeout[] =
    "HTTP/1.0 200 OK\r\n"
    "Cache-Control: private, no-cache, max-age=0\r\n"
    "Pragma: no-cache\r\n"
    "Connection: Close\r\n\r\n"
    "<html><head><title>408 Request Timeout</title></head><body>\n"
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
        if (D_ENABLED(DL_MEDIUM))
            LOG_D((getLogger(),
                   "[%s] Keep-alive timeout, close!",
                   getLogId()));
        c = 1;
    }
    else if ((delta > 2) && (m_iReqServed != 0))
    {
        if (ConnLimitCtrl::getInstance().getConnOverflow())
        {
            if (D_ENABLED(DL_MEDIUM))
                LOG_D((getLogger(),
                       "[%s] Connections over flow, close connection!",
                       getLogId()));
            c = 1;

        }
        else if ((int)m_pNtwkIOLink->getClientInfo()->getConns() >
                 (ClientInfo::getPerClientSoftLimit() >> 1))
        {
            if (D_ENABLED(DL_MEDIUM))
                LOG_D((getLogger(),
                       "[%s] Connections is over the limit, close!",
                       getLogId()));
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
        if (m_pNtwkIOLink->getfd() != m_pNtwkIOLink->getPollfd()->fd)
            LOG_ERR((getLogger(),
                     "[%s] BUG: fd %d does not match fd %d in pollfd!",
                     getLogId(), m_pNtwkIOLink->getfd(), m_pNtwkIOLink->getPollfd()->fd));
//            if ( D_ENABLED( DL_MEDIUM ))
        if ((m_response.getBodySent() == 0) || !m_pNtwkIOLink->getEvents())
        {
            LOG_INFO((getLogger(),
                      "[%s] Connection idle time: %ld while in state: %d watching for event: %d,"
                      "close!",
                      getLogId(), DateTime::s_curTime - getStream()->getActiveTime(), getState(),
                      m_pNtwkIOLink->getEvents()));
            m_request.dumpHeader();
            if (m_pHandler)
                m_pHandler->dump();
            if (getState() == HSS_READING_BODY)
            {
                LOG_INFO((getLogger(),
                          "[%s] Request body size: %d, received: %d.",
                          getLogId(), m_request.getContentLength(),
                          m_request.getContentFinished()));
            }
        }
        if ((getState() == HSS_PROCESSING) && m_response.getBodySent() == 0)
        {
            IOVec iov;
            iov.append(s_errTimeout, sizeof(s_errTimeout) - 1);
            getStream()->writev(iov, sizeof(s_errTimeout) - 1);
        }
        else
            getStream()->setAbortedFlag();
        closeConnection();
        return 1;
    }
    else
        return 0;
}


int HttpSession::isAlive()
{
    if (getStream()->isSpdy())
        return 1;
    return !m_pNtwkIOLink->detectClose();

}


int HttpSession::detectTimeout()
{
//    if ( D_ENABLED( DL_MEDIUM ))
//        LOG_D((getLogger(),
//            "[%s] HttpSession::detectTimeout() ",
//            getLogId() ));
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
    if (getState() ==  HSS_THROTTLING)
        onWriteEx();
    if (detectTimeout())
        return 1;
    if (m_pHandler)
        m_pHandler->onTimer();
    return 0;
}


void HttpSession::releaseRespCache()
{
    if (m_pRespBodyBuf)
    {
        HttpResourceManager::getInstance().recycle(m_pRespBodyBuf);
        m_pRespBodyBuf = NULL;
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


int HttpSession::sendDynBody()
{
    size_t toWrite;
    char *pBuf;

    while (((pBuf = m_pRespBodyBuf->getReadBuffer(toWrite)) != NULL)
           && (toWrite > 0))
    {
        int len = toWrite;
        if (m_response.getContentLen() > 0)
        {
            int allowed = m_response.getContentLen() - m_lDynBodySent;
            if (len > allowed)
                len = allowed;
            if (len <= 0)
            {
                m_pRespBodyBuf->readUsed(toWrite);
                continue;
            }
        }

        int ret = writeRespBody(pBuf, len);
        if (D_ENABLED(DL_MEDIUM))
            LOG_D((getLogger(),
                   "[%s] writeRespBody() len=%d return %d\n",
                   getLogId(), len, ret));
        if (ret > 0)
        {
            m_lDynBodySent += ret;
            if (ret >= len)
                ret = toWrite;
            m_pRespBodyBuf->readUsed(ret);
            if (ret < len)
            {
                continueWrite();
                return 1;
            }
        }
        else if (ret == -1)
        {
            continueRead();
            return LS_FAIL;
        }
        else
        {
            continueWrite();
            return 1;
        }
    }
    return 0;
}


int HttpSession::setupRespCache()
{
    if (!m_pRespBodyBuf)
    {
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(), "[%s] allocate response body buffer", getLogId()));
        m_pRespBodyBuf = HttpResourceManager::getInstance().getVMemBuf();
        if (!m_pRespBodyBuf)
        {
            LOG_ERR((getLogger(),
                     "[%s] Failed to obtain VMemBuf, current pool size: %d,"
                     "capacity: %d.",
                     getLogId(),
                     HttpResourceManager::getInstance().getVMemBufPoolSize(),
                     HttpResourceManager::getInstance().getVMemBufPoolCapacity()));
            return LS_FAIL;
        }
        if (m_pRespBodyBuf->reinit(
                m_response.getContentLen()))
        {
            LOG_ERR((getLogger(),
                     "[%s] Failed to initialize VMemBuf, response body len: %ld.", getLogId(),
                     m_response.getContentLen()));
            return LS_FAIL;
        }
        m_lDynBodySent = 0;
    }
    return 0;
}


extern int addModgzipFilter(lsi_session_t *session, int isSend,
                            uint8_t compressLevel);
int HttpSession::setupGzipFilter()
{
    char gz = m_request.gzipAcceptable();
    int recvhkptNogzip = m_sessionHooks.getFlag(LSI_HKPT_RECV_RESP_BODY) &
                         LSI_HOOK_FLAG_DECOMPRESS_REQUIRED;
    int  hkptNogzip = (m_sessionHooks.getFlag(LSI_HKPT_RECV_RESP_BODY)
                       | m_sessionHooks.getFlag(LSI_HKPT_SEND_RESP_BODY))
                      & LSI_HOOK_FLAG_DECOMPRESS_REQUIRED;
    if (gz & (UPSTREAM_GZIP | UPSTREAM_DEFLATE))
    {
        setFlag(HSF_RESP_BODY_COMPRESSED);
        if (recvhkptNogzip || hkptNogzip || !(gz & REQ_GZIP_ACCEPT))
        {
            //setup decompression filter at RECV_RESP_BODY filter
            if (addModgzipFilter((LsiSession *)this, 0, 0) == -1)
                return LS_FAIL;
            m_response.getRespHeaders().del(
                HttpRespHeaders::H_CONTENT_ENCODING);
            gz &= ~(UPSTREAM_GZIP | UPSTREAM_DEFLATE);
            clearFlag(HSF_RESP_BODY_COMPRESSED);
        }
    }
    else
        clearFlag(HSF_RESP_BODY_COMPRESSED);

    if (gz == GZIP_REQUIRED)
    {
        if (!hkptNogzip)
        {
            if (!m_pRespBodyBuf->empty())
                m_pRespBodyBuf->rewindWriteBuf();
            if ((!m_request.getContextState(RESP_CONT_LEN_SET)
                 || m_response.getContentLen() > 200))
                if (setupGzipBuf() == -1)
                    return LS_FAIL;
        }
        else //turn on compression at SEND_RESP_BODY filter
        {
            if (addModgzipFilter((LsiSession *)this, 1,
                                 HttpServerConfig::getInstance().getCompressLevel()) == -1)
                return LS_FAIL;
            m_response.addGzipEncodingHeader();
            //The below do not set the flag because compress won't update the resp VMBuf to decompressed
            //setFlag(HSF_RESP_BODY_COMPRESSED);
        }
    }
    return 0;
}


int HttpSession::setupGzipBuf()
{
    if (m_pRespBodyBuf)
    {
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(), "[%s] GZIP response body in response body buffer!",
                   getLogId()));
        if (m_pGzipBuf)
        {
            if (m_pGzipBuf->isStreamStarted())
            {
                m_pGzipBuf->endStream();
                if (D_ENABLED(DL_MEDIUM))
                    LOG_D((getLogger(),
                           "[%s] setupGzipBuf() end GZIP stream.\n",
                           getLogId()));
            }
        }

        if (!m_pGzipBuf)
            m_pGzipBuf = HttpResourceManager::getInstance().getGzipBuf();
        if (m_pGzipBuf)
        {
            m_pGzipBuf->setCompressCache(m_pRespBodyBuf);
            if ((m_pGzipBuf->init(GzipBuf::GZIP_DEFLATE,
                                  HttpServerConfig::getInstance().getCompressLevel()) == 0) &&
                (m_pGzipBuf->beginStream() == 0))
            {
                if (D_ENABLED(DL_MEDIUM))
                    LOG_D((getLogger(),
                           "[%s] setupGzipBuf() begin GZIP stream.\n",
                           getLogId()));
                m_response.setContentLen(LSI_RESP_BODY_SIZE_UNKNOWN);
                m_response.addGzipEncodingHeader();
                m_request.orGzip(UPSTREAM_GZIP);
                setFlag(HSF_RESP_BODY_COMPRESSED);
                return 0;
            }
            else
            {
                LOG_ERR((getLogger(),
                         "[%s] out of swapping space while initialize GZIP stream!", getLogId()));
                delete m_pGzipBuf;
                clearFlag(HSF_RESP_BODY_COMPRESSED);
                m_pGzipBuf = NULL;
            }
        }
    }
    return LS_FAIL;
}


void HttpSession::releaseGzipBuf()
{
    if (m_pGzipBuf)
    {
        if (m_pGzipBuf->getType() == GzipBuf::GZIP_DEFLATE)
            HttpResourceManager::getInstance().recycle(m_pGzipBuf);
        else
            HttpResourceManager::getInstance().recycleGunzip(m_pGzipBuf);
        m_pGzipBuf = NULL;
    }
}


int HttpSession::appendDynBodyEx(const char *pBuf, int len)
{
    int ret = 0;
    if (len == 0)
        return 0;

    if ((m_pGzipBuf) && (m_pGzipBuf->getType() == GzipBuf::GZIP_DEFLATE))
    {
        ret = m_pGzipBuf->write(pBuf, len);
        //end of written response body
        if (ret == 1)
            ret = 0;
    }
    else
    {
        if (!m_pRespBodyBuf)
        {
            setupDynRespBodyBuf();
            if (!m_pRespBodyBuf)
                return len;
        }
        ret = appendRespBodyBuf(pBuf, len);
    }

    return ret;

}


int appendDynBodyTermination(HttpSession *conn, const char *pBuf, int len)
{
    return conn->appendDynBodyEx(pBuf, len);
}


int HttpSession::runFilter(int hookLevel,
                           filter_term_fn pfTermination,
                           const char *pBuf, int len, int flagIn)
{
    int ret;
    int bit;
    int flagOut = 0;
    const LsiApiHooks *pHooks = LsiApiHooks::getGlobalApiHooks(hookLevel);
    lsi_cb_param_t param;
    lsiapi_hookinfo_t hookInfo;
    param._session = (LsiSession *)this;
    hookInfo.hooks = pHooks;
    hookInfo.enable_array = m_sessionHooks.getEnableArray(hookLevel);
    hookInfo.term_fn = pfTermination;
    param._cur_hook = (void *)pHooks->begin();
    param._hook_info = &hookInfo;
    param._param = pBuf;
    param._param_len = len;
    param._flag_out = &flagOut;
    param._flag_in = flagIn;
    ret = LsiApiHooks::runForwardCb(&param);

    if (hookLevel == LSI_HKPT_RECV_RESP_BODY)
        bit = HSF_RECV_RESP_BUFFERED;
    else
        bit = HSF_SEND_RESP_BUFFERED;
    if (flagOut)
        m_iFlag |= bit;
    else
        m_iFlag &= ~bit;

    if (D_ENABLED(DL_MEDIUM))
        LOG_D((getLogger(),
               "[%s][%s] input len: %d, flag in: %d, flag out: %d, ret: %d",
               getLogId(), LsiApiHooks::getHkptName(hookLevel), len, flagIn, flagOut,
               ret));
    return processHkptResult(hookLevel, ret);

}


int HttpSession::appendDynBody(const char *pBuf, int len)
{
    if (!m_pRespBodyBuf)
    {
        setupDynRespBodyBuf();
        if (!m_pRespBodyBuf)
            return len;
    }

    if (!getFlag(HSF_RESP_HEADER_DONE))
    {
        int ret = respHeaderDone();
        if (ret)
            return LS_FAIL;
    }

    setFlag(HSF_RESP_FLUSHED, 0);

    if (m_sessionHooks.isDisabled(LSI_HKPT_RECV_RESP_BODY))
        return appendDynBodyEx(pBuf, len);
    return runFilter(LSI_HKPT_RECV_RESP_BODY,
                     (filter_term_fn)appendDynBodyTermination, pBuf, len, 0);
}


int HttpSession::appendRespBodyBuf(const char *pBuf, int len)
{
    char *pCache;
    size_t iCacheSize;
    const char *pBufOrg = pBuf;

    while (len > 0)
    {
        pCache = m_pRespBodyBuf->getWriteBuffer(iCacheSize);
        if (pCache)
        {
            int ret = len;
            if (iCacheSize < (size_t)ret)
                ret = iCacheSize;
            if (pCache != pBuf)
                memmove(pCache, pBuf, ret);
            pBuf += ret;
            len -= ret;
            m_pRespBodyBuf->writeUsed(ret);
        }
        else
        {
            LOG_ERR((getLogger(),
                     "[%s] out of swapping space while append to response buffer!",
                     getLogId()));
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
    pCache = m_pRespBodyBuf->getWriteBuffer(iCacheSize);
    assert(iCacheSize > 0);
    iInitCacheSize = iCacheSize;

    if (!pCache)
    {
        LOG_ERR((getLogger(),
                 "[%s] out of swapping space while append[V] to response buffer!",
                 getLogId()));
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
                m_pRespBodyBuf->writeUsed(iInitCacheSize - iCacheSize);
                pCache = m_pRespBodyBuf->getWriteBuffer(iCacheSize);
                iInitCacheSize = iCacheSize;
                pBuf += ret;
                len -= ret;
            }
            else
                break;  //quit the while loop
        }
    }

    if (iInitCacheSize > iCacheSize)
        m_pRespBodyBuf->writeUsed(iInitCacheSize - iCacheSize);


    return 0;
}


int HttpSession::shouldSuspendReadingResp()
{
    if (m_pRespBodyBuf)
    {
        int buffered = m_pRespBodyBuf->getCurWBlkPos() -
                       m_pRespBodyBuf->getCurRBlkPos();
        return ((buffered >= 1024 * 1024) || (buffered < 0));
    }
    return 0;
}


void HttpSession::resetRespBodyBuf()
{

    {
        if (m_pGzipBuf)
            m_pGzipBuf->resetCompressCache();
        else
        {
            m_pRespBodyBuf->rewindReadBuf();
            m_pRespBodyBuf->rewindWriteBuf();
        }
    }
}


static char achOverBodyLimitError[] =
    "<p>The size of dynamic response body is over the "
    "limit, response is truncated by web server. "
    "The limit is set by the "
    "'Max Dynamic Response Body Size' in tuning section "
    " of server configuration.";


int HttpSession::checkRespSize(int nobuffer)
{
    int ret = 0;
    if (m_pRespBodyBuf)
    {
        int curLen = m_pRespBodyBuf->writeBufSize();
        if ((nobuffer && (curLen > 0)) || (curLen > 1460))
        {
            if (curLen > HttpServerConfig::getInstance().getMaxDynRespLen())
            {
                LOG_WARN((getLogger(), "[%s] The size of dynamic response body is"
                          " over the limit, abort!",
                          getLogId()));
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


//return 0 , caller should continue
//return !=0, the request has been redirected, should break the normal flow

int HttpSession::respHeaderDone()
{
    if (getFlag(HSF_RESP_HEADER_DONE))
        return 0;
    m_iFlag |= HSF_RESP_HEADER_DONE;
    if (D_ENABLED(DL_MEDIUM))
        LOG_D((getLogger(),
               "[%s] response header finished!",
               getLogId()));

    int ret = 0;
    if (m_sessionHooks.isEnabled(LSI_HKPT_RCVD_RESP_HEADER))
        ret = m_sessionHooks.runCallbackNoParam(LSI_HKPT_RCVD_RESP_HEADER,
                                                (LsiSession *)this);
    if (processHkptResult(LSI_HKPT_RCVD_RESP_HEADER, ret))
        return 1;
    return 0;
    //setupDynRespBodyBuf( iRespState );

}


int HttpSession::endResponseInternal(int success)
{
    int ret = 0;
    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] endResponseInternal()", getLogId()));
    m_iFlag |= HSF_HANDLER_DONE;

    if (!getFlag(HSF_RESP_HEADER_DONE))
    {
        if (respHeaderDone())
            return 1;
    }

    if (sendBody() && m_sessionHooks.isEnabled(LSI_HKPT_RECV_RESP_BODY))
    {
        ret = runFilter(LSI_HKPT_RECV_RESP_BODY,
                        (filter_term_fn)appendDynBodyTermination,
                        NULL, 0, LSI_CB_FLAG_IN_EOF);

    }

    if (m_pGzipBuf)
    {
        if (m_pGzipBuf->endStream())
        {
            LOG_ERR((getLogger(),
                     "[%s] out of swapping space while terminating GZIP stream!", getLogId()));
            ret = -1;
        }
        else
        {
            if (D_ENABLED(DL_MEDIUM))
                LOG_D((getLogger(), "[%s] endResponse() end GZIP stream.\n", getLogId()));

        }
    }

    if (!ret && m_sessionHooks.isEnabled(LSI_HKPT_RCVD_RESP_BODY))
    {
        ret = m_sessionHooks.runCallbackNoParam(LSI_HKPT_RCVD_RESP_BODY,
                                                (LsiSession *)this);
//         ret = m_sessionHooks.runCallback(LSI_HKPT_RCVD_RESP_BODY, (LsiSession *)this,
//                                          NULL, 0, NULL, success?LSI_CB_FLAG_IN_RESP_SUCCEED:0 );
        ret = processHkptResult(LSI_HKPT_RCVD_RESP_BODY, ret);
    }
    return ret;
}


int HttpSession::endResponse(int success)
{
    //If already called once, just return
    if (m_iFlag & HSF_HANDLER_DONE)
        return 0;

    if (D_ENABLED(DL_MEDIUM))
        LOG_D((getLogger(), "[%s] endResponse( %d )\n", getLogId(), success));

    int ret = 0;

    ret = endResponseInternal(success);
    if (ret)
        return ret;

    if (!isRespHeaderSent() && (m_response.getContentLen() < 0))
    {
        // header is not sent yet, no body sent yet.
        //content length = dynamic content + static send file
        long size = m_sendFileInfo.getRemain();
        if (getRespCache())
            size += getRespCache()->getCurWOffset();

        m_response.setContentLen(size);

    }

    setFlag(HSF_RESP_FLUSHED, 0);
    ret = flush();

    return ret;
}


int HttpSession::flushBody()
{
    int ret = 0;

    if (m_iFlag & HSF_RECV_RESP_BUFFERED)
    {
        if (m_sessionHooks.isEnabled(LSI_HKPT_RECV_RESP_BODY))
        {
            ret = runFilter(LSI_HKPT_RECV_RESP_BODY,
                            (filter_term_fn)appendDynBodyTermination,
                            NULL, 0, LSI_CB_FLAG_IN_FLUSH);
        }
    }
    if (!(m_iFlag & HSF_HANDLER_DONE))
    {
        if (m_pGzipBuf && m_pGzipBuf->isStreamStarted())
            m_pGzipBuf->flush();

    }
    if ((m_pRespBodyBuf
         && !m_pRespBodyBuf->empty())/*&&( isRespHeaderSent() )*/)
        ret = sendDynBody();
    if ((ret == 0) && (m_sendFileInfo.getRemain() > 0))
        ret = sendStaticFile(&m_sendFileInfo);
    if (ret > 0)
        return LS_AGAIN;

    if (m_sessionHooks.isEnabled(LSI_HKPT_SEND_RESP_BODY))
    {
        int flush = LSI_CB_FLAG_IN_FLUSH;
        if (((m_iFlag & (HSF_HANDLER_DONE | HSF_RECV_RESP_BUFFERED)) ==
             HSF_HANDLER_DONE))
            flush = LSI_CB_FLAG_IN_EOF;
        ret = runFilter(LSI_HKPT_SEND_RESP_BODY,
                        (filter_term_fn)writeRespBodyTermination,
                        NULL, 0, flush);
        if (m_iFlag & HSF_SEND_RESP_BUFFERED)
            return LS_AGAIN;
    }

    //int nodelay = 1;
    //::setsockopt( getfd(), IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof( int ));
    if (m_pChunkOS)
    {
//            if ( !m_request.getSSIRuntime() )
        {
            if (m_pChunkOS->flush() != 0)
                return LS_AGAIN;
            if ((m_iFlag & (HSF_HANDLER_DONE | HSF_RECV_RESP_BUFFERED
                            | HSF_SEND_RESP_BUFFERED | HSF_CHUNK_CLOSED))
                == HSF_HANDLER_DONE)
            {
                m_pChunkOS->close();
                if (D_ENABLED(DL_LESS))
                    LOG_D((getLogger(), "[%s] Chunk closed!", getLogId()));
                m_iFlag |= HSF_CHUNK_CLOSED;
            }
        }
    }
    return LS_DONE;
}


int HttpSession::flush()
{
    int ret = LS_DONE;
    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] HttpSession::flush()!",
               getLogId()));
    if (getFlag(HSF_SUSPENDED))
    {
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(), "[%s] HSF_SUSPENDED flag is set, cannot flush.",
                   getLogId()));
        suspendWrite();
        return LS_AGAIN;
    }

    if (getFlag(HSF_RESP_FLUSHED))
    {
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(), "[%s] HSF_RESP_FLUSHED flag is set, skip flush.",
                   getLogId()));
        suspendWrite();
        return LS_DONE;
    }
    if (!sendBody() && !getFlag(HSF_HANDLER_DONE))
    {
        ret = endResponseInternal(1);
        if (ret)
            return LS_AGAIN;
    }
    else if (getFlag(HSF_HANDLER_DONE | HSF_RESP_WAIT_FULL_BODY) ==
             HSF_RESP_WAIT_FULL_BODY)
    {
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(), "[%s] cannot flush as response is not finished!",
                   getLogId()));
        return LS_DONE;
    }

    if (!isRespHeaderSent())
    {
        if (sendRespHeaders())
            return LS_DONE;
    }

    if (sendBody())
    {
        ret = flushBody();
        if (D_ENABLED(DL_LESS))
            LOG_D((getLogger(), "[%s] flushBody() return %d",
                   getLogId(), ret));
    }

    if (getFlag(HSF_AIO_READING))
        return ret;

    if (!ret)
        ret = getStream()->flush();
    if (ret == LS_DONE)
    {
        if (getFlag(HSF_HANDLER_DONE
                    | HSF_RECV_RESP_BUFFERED
                    | HSF_SEND_RESP_BUFFERED) == HSF_HANDLER_DONE)
        {
            if (D_ENABLED(DL_LESS))
                LOG_D((getLogger(), "[%s] set HSS_COMPLETE flag.", getLogId()));
            setState(HSS_COMPLETE);
        }
        else
        {
            if (D_ENABLED(DL_LESS))
                LOG_D((getLogger(), "[%s] set HSF_RESP_FLUSHED flag.", getLogId()));
            setFlag(HSF_RESP_FLUSHED, 1);
            return LS_DONE;
        }
    }

    continueWrite();
    return ret;
}


void HttpSession::addLocationHeader()
{
    HttpRespHeaders &headers = m_response.getRespHeaders();
    const char *pLocation = m_request.getLocation();
    //int len = m_request.getLocationLen();
    headers.add(HttpRespHeaders::H_LOCATION, "", 0);
    if (*pLocation == '/')
    {
        const char *pHost = m_request.getHeader(HttpHeader::H_HOST);
        if (*pHost)
        {
            if (isSSL())
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

    if (m_request.getAuthRequired())
        m_request.addWWWAuthHeader(headers);

    if (m_request.getLocation() != NULL)
        addLocationHeader();
    const AutoBuf *pExtraHeaders = m_request.getExtraHeaders();
    if (pExtraHeaders)
        headers.parseAdd(pExtraHeaders->begin(), pExtraHeaders->size(),
                         LSI_HEADER_ADD);
}


int HttpSession::sendRespHeaders()
{
    if (!getFlag(HSF_RESP_HEADER_DONE))
    {
        if (respHeaderDone())
            return 1;
    }

    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] sendRespHeaders()", getLogId()));

    if (sendBody())
    {
        if (m_sessionHooks.isEnabled(LSI_HKPT_SEND_RESP_BODY))
            m_response.setContentLen(LSI_RESP_BODY_SIZE_UNKNOWN);
        if (m_response.getContentLen() >= 0)
            m_response.appendContentLenHeader();
        else
            setupChunkOS(0);
    }
    prepareHeaders();

    if (finalizeHeader(m_request.getVersion(), m_request.getStatusCode()))
        return 1;

    getStream()->sendRespHeaders(&m_response.getRespHeaders());
    m_iFlag |= HSF_RESP_HEADER_SENT;
    setState(HSS_WRITING);
    return 0;
}


int HttpSession::setupDynRespBodyBuf()
{
    if (D_ENABLED(DL_LESS))
        LOG_D((getLogger(), "[%s] setupDynRespBodyBuf()", getLogId()));
    if (sendBody())
    {
        if (setupRespCache() == -1)
            return LS_FAIL;

        if (setupGzipFilter() == -1)
            return LS_FAIL;
    }
    else
    {
        //abortReq();
        endResponseInternal(1);
        return 1;
    }
    return 0;
}


int HttpSession::flushDynBodyChunk()
{
    if (m_pGzipBuf && m_pGzipBuf->isStreamStarted())
    {
        m_pGzipBuf->endStream();
        if (D_ENABLED(DL_MEDIUM))
            LOG_D((getLogger(), "[%s] flushDynBodyChunk() end GZIP stream.\n",
                   getLogId()));
    }
    return 0;

}


int HttpSession::execExtCmd(const char *pCmd, int len)
{

    ReqHandler *pNewHandler;
    ExtWorker *pWorker = ExtAppRegistry::getApp(
                             EA_CGID, LSCGID_NAME);
    if (m_pHandler)
        cleanUpHandler();
    pNewHandler = HandlerFactory::getHandler(HandlerType::HT_CGI);
    m_request.setRealPath(pCmd, len);
    m_request.orContextState(EXEC_EXT_CMD);
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
    char achAddr[128];
    socklen_t addrlen = 128;
    if (getsockname(m_pNtwkIOLink->getfd(), (struct sockaddr *) achAddr,
                    &addrlen) == -1)
        return 0;

    if ((AF_INET6 == ((struct sockaddr *)achAddr)->sa_family) &&
        (IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)achAddr)->sin6_addr)))
    {
        ((struct sockaddr *)achAddr)->sa_family = AF_INET;
        ((struct sockaddr_in *)achAddr)->sin_addr.s_addr = *((
                    in_addr_t *)&achAddr[20]);
    }

    if (GSockAddr::ntop((struct sockaddr *)achAddr, pBuf, len) == NULL)
        return 0;
    return strlen(pBuf);
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
    }
    return fd;
}


int HttpSession::initSendFileInfo(const char *pPath, int pathLen)
{
    int ret;
    struct stat st;
    StaticFileCacheData *&pData = m_sendFileInfo.getFileData();
    FileCacheDataEx *&pECache = m_sendFileInfo.getECache();
    if ((pData) && (!pData->isSamePath(pPath, pathLen)))
    {
        releaseStaticFileCacheData(pData);
        pData = NULL;
        releaseFileCacheDataEx(pECache);
        pECache = NULL;
    }
    int fd = openStaticFile(pPath, pathLen, &ret);
    if (fd == -1)
        return ret;
    fstat(fd, &st);
    ret = setUpdateStaticFileCache(pData, pECache, pPath, pathLen, fd, st);
    if (ret)
    {
        close(fd);
        return ret;
    }
    if (pData->getFileData()->getfd() != -1
        && fd != pData->getFileData()->getfd())
        close(fd);
    if (pData->getFileData()->getfd() == -1)
        pData->getFileData()->setfd(fd);
    if (pData->getFileData() == pECache)
        return 0;

    releaseFileCacheDataEx(pECache);
    pECache = NULL;
    return m_sendFileInfo.getFileData()->readyCacheData(pECache, 0);
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
        m_iFlag &= ~HSF_RESP_WAIT_FULL_BODY;
        if (D_ENABLED(DL_MEDIUM))
            LOG_D((getLogger(),
                   "[%s] RESP_WAIT_FULL_BODY turned off by sending static file().\n",
                   getLogId()));
    }
    setFlag(HSF_RESP_FLUSHED, 0);
//     if ( !getFlag( HSF_RESP_HEADER_DONE ))
//         respHeaderDone();

}


int HttpSession::writeRespBodyBlockInternal(SendFileInfo *pData,
        const char *pBuf,
        int written)
{
    int len;
    if (getGzipBuf())
    {
        len = appendDynBodyEx(pBuf, written);
        if (!len)
            len = written;
    }
    else
        len = writeRespBodyDirect(pBuf, written);
    if (D_ENABLED(DL_MEDIUM))
        LOG_D((getLogger(),
               "[%s] %s return %ld.\n",
               getLogId(),
               getGzipBuf() ? "appendDynBodyEx()" : "writeRespBodyDirect()",
               len));

    if (len > 0)
        pData->incCurPos(len);
    return len;
}


int HttpSession::writeRespBodyBlockFilterInternal(SendFileInfo *pData,
        const char *pBuf,
        int written,
        lsi_cb_param_t *param)
{
    int len;
    param->_cur_hook = (void *)param->_hook_info->hooks->begin();
    param->_param = pBuf;
    param->_param_len = written;

//     if (( written >= pData->getRemain() )
//         &&((m_iFlag & (HSF_HANDLER_DONE |
//                         HSF_RECV_RESP_BUFFERED ) == HSF_HANDLER_DONE )))
//         param->_flag_in = LSI_CB_FLAG_IN_EOF;

    len = LsiApiHooks::runForwardCb(param);
    if (D_ENABLED(DL_MEDIUM))
        LOG_D(("[%s] %s sent: %d, buffered: %d",
               getLogId(), "runForwardCb()",
               len, *(param->_flag_out)));

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
            else if (len < written)
            {
                if (D_ENABLED(DL_MEDIUM))
                    LOG_D((getLogger(),
                           "[%s] Socket still busy, %d bytes to write\n",
                           getLogId(), written - len));
                return 1;
            }
        }
        pData->resetAioBuf();
        if (D_ENABLED(DL_MEDIUM))
            LOG_D((getLogger(),
                   "[%s] Buffer finished, continue with aioRead.\n",
                   getLogId()));
        suspendWrite();
    }
    if (!pData->getECache()->isCached() && pData->getRemain() > 0)
    {
        if (getFlag(HSF_AIO_READING))
        {
            if (D_ENABLED(DL_MEDIUM))
                LOG_D((getLogger(),
                       "[%s] Aio Reading already in progress.\n",
                       getLogId()));
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
    long len;

#if !defined( NO_SENDFILE )
    int fd = pData->getECache()->getfd();
    int iModeSF = HttpServerConfig::getInstance().getUseSendfile();
    if (iModeSF && fd != -1 && !isSSL() && !getStream()->isSpdy()
        && (!getGzipBuf() ||
            (pData->getECache() == pData->getFileData()->getGziped()))
        && getStream()->getFlag(HIO_FLAG_SENDFILE))
    {
        len = writeRespBodySendFile(fd, pData->getCurPos(), pData->getRemain());
        if (D_ENABLED(DL_MEDIUM))
            LOG_D((getLogger(),
                   "[%s] writeRespBodySendFile() return %ld.\n",
                   getLogId(), len));
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
        if (D_ENABLED(DL_MEDIUM))
            LOG_D((getLogger(), "[%s] sendStaticFileAio() return %ld.\n",
                   getLogId(), len));
        if (len)
            return len;
    }
#endif
    while ((remain = pData->getRemain()) > 0)
    {
        len = (remain < STATIC_FILE_BLOCK_SIZE) ? remain : STATIC_FILE_BLOCK_SIZE ;
        written = remain;
        pBuf = pData->getECache()->getCacheData(
                   pData->getCurPos(), written, HttpResourceManager::getGlobalBuf(), len);
        if (written <= 0)
            return LS_FAIL;

        len = writeRespBodyBlockInternal(pData, pBuf, written);
        if (len < 0)
            return len;
        else if (len == 0)
            break;
        else if (len < written)
            return 1;
    }
    return (pData->getRemain() > 0);
}


int HttpSession::sendStaticFile(SendFileInfo *pData)
{
    if (D_ENABLED(DL_MEDIUM))
        LOG_D((getLogger(), "[%s] SendStaticFile()\n", getLogId()));

    if (m_sessionHooks.isDisabled(LSI_HKPT_SEND_RESP_BODY) ||
        !(m_sessionHooks.getFlag(LSI_HKPT_SEND_RESP_BODY)&
          LSI_HOOK_FLAG_PROCESS_STATIC))
        return sendStaticFileEx(pData);

    const char *pBuf;
    off_t written;
    off_t remain;
    long len;
#ifdef LS_AIO_USE_AIO
    if (HttpServerConfig::getInstance().getUseSendfile() == 2)
    {
        len = sendStaticFileAio(pData);
        LOG_D((getLogger(), "[%s] sendStaticFileAio() return %ld.\n",
               getLogId(), len));
        if (len)
            return len;
    }
#endif
    lsi_cb_param_t param;
    lsiapi_hookinfo_t hookInfo;
    int buffered = 0;
    param._session = (LsiSession *)this;

    hookInfo.term_fn = (filter_term_fn)
                       writeRespBodyTermination;
    hookInfo.hooks = LsiApiHooks::getGlobalApiHooks(LSI_HKPT_SEND_RESP_BODY);
    hookInfo.enable_array = m_sessionHooks.getEnableArray(
                                LSI_HKPT_SEND_RESP_BODY);
    param._hook_info = &hookInfo;

    param._flag_in  = 0;
    param._flag_out = &buffered;


    while ((remain = pData->getRemain()) > 0)
    {
        len = (remain < STATIC_FILE_BLOCK_SIZE) ? remain : STATIC_FILE_BLOCK_SIZE ;
        written = remain;
        pBuf = pData->getECache()->getCacheData(
                   pData->getCurPos(), written, HttpResourceManager::getGlobalBuf(), len);
        if (written <= 0)
            return LS_FAIL;

        len = writeRespBodyBlockFilterInternal(pData, pBuf, written, &param);
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

    int ret;
    getResp()->getRespHeaders().addStatusLine(ver, code,
            m_request.isKeepAlive());
    HttpVHost *host = (HttpVHost *)m_request.getVHost();
    if (host)
    {
        const AutoStr2 &str = host->getSpdyAdHeader();
        if (!isSSL() && str.len() > 0)
            getResp()->appendHeader("Alternate-Protocol", 18, str.c_str(), str.len());
    }

    if (m_sessionHooks.isEnabled(LSI_HKPT_SEND_RESP_HEADER))
    {
        ret = m_sessionHooks.runCallbackNoParam(LSI_HKPT_SEND_RESP_HEADER,
                                                (LsiSession *)this);
        ret = processHkptResult(LSI_HKPT_SEND_RESP_HEADER, ret);
        if (ret)
            return ret;

    }
    if (sendBody())
        return contentEncodingFixup();
    return 0;
}


int HttpSession::updateContentCompressible()
{
    int compressible = 0;
    if (m_request.gzipAcceptable() == GZIP_REQUIRED)
    {
        int len;
        char *pContentType = (char *)m_response.getRespHeaders().getHeader(
                                 HttpRespHeaders::H_CONTENT_TYPE, &len);
        if (pContentType)
        {
            char ch = pContentType[len];
            pContentType[len] = 0;
            compressible = HttpMime::getMime()->compressable(pContentType);
            pContentType[len] = ch;
        }
        if (!compressible)
            m_request.andGzip(~GZIP_ENABLED);
    }
    return compressible;
}


int HttpSession::contentEncodingFixup()
{
    int len;
    char gz = m_request.gzipAcceptable();
    int requireChunk = 0;
    const char *pContentEncoding = m_response.getRespHeaders().getHeader(
                                       HttpRespHeaders::H_CONTENT_ENCODING, &len);
    if (!(gz & REQ_GZIP_ACCEPT))
    {
        if (pContentEncoding)
        {
            if (addModgzipFilter((LsiSession *)this, 1, 0) == -1)
                return LS_FAIL;
            m_response.getRespHeaders().del(HttpRespHeaders::H_CONTENT_ENCODING);
            clearFlag(HSF_RESP_BODY_COMPRESSED);
            requireChunk = 1;
        }
    }
    else if (!pContentEncoding && updateContentCompressible())
    {
        if ((!m_request.getContextState(RESP_CONT_LEN_SET)
             || m_response.getContentLen() > 200))
        {
            if (addModgzipFilter((LsiSession *)this, 1,
                                 HttpServerConfig::getInstance().getCompressLevel()) == -1)
                return LS_FAIL;
            m_response.addGzipEncodingHeader();
            //The below do not set the flag because compress won't update the resp VMBuf to decompressed
            //setFlag(HSF_RESP_BODY_COMPRESSED);
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


int HttpSession::handoff(char **pData, int *pDataLen)
{
    if (isSSL() || getStream()->isSpdy())
        return LS_FAIL;
    if (m_iReqServed != 0)
        return LS_FAIL;
    int fd = dup(m_pNtwkIOLink->getfd());
    if (fd != -1)
    {
        AutoBuf &headerBuf = m_request.getHeaderBuf();
        *pDataLen = headerBuf.size() - HEADER_BUF_PAD;
        *pData = (char *)malloc(*pDataLen);
        memmove(*pData, headerBuf.begin() + HEADER_BUF_PAD, *pDataLen);

        getStream()->setAbortedFlag();
        closeConnection();
    }
    return fd;

}


int HttpSession::onAioEvent()
{
#ifdef LS_AIO_USE_AIO
    int buffered = 0;
    int len, written = m_aioReq.getReturn();
    char *pBuf = (char *)m_aioReq.getBuf();

    if (D_ENABLED(DL_MEDIUM))
        LOG_D((getLogger(), "[%s] Aio Read read %d.\n",
               getLogId(), written));

    if (written <= 0)
        return LS_FAIL;

    clearFlag(HSF_AIO_READING);
    if (m_sessionHooks.isDisabled(LSI_HKPT_SEND_RESP_BODY) ||
        !(m_sessionHooks.getFlag(LSI_HKPT_SEND_RESP_BODY)&
          LSI_HOOK_FLAG_PROCESS_STATIC))
        len = writeRespBodyBlockInternal(&m_sendFileInfo, pBuf, written);
    else
    {
        lsi_cb_param_t param;
        lsiapi_hookinfo_t hookInfo;
        param._session = (LsiSession *)this;

        hookInfo.term_fn = (filter_term_fn)
                           writeRespBodyTermination;
        hookInfo.hooks = LsiApiHooks::getGlobalApiHooks(LSI_HKPT_SEND_RESP_BODY);
        hookInfo.enable_array = m_sessionHooks.getEnableArray(
                                    LSI_HKPT_SEND_RESP_BODY);
        param._hook_info = &hookInfo;

        param._flag_in  = 0;
        param._flag_out = &buffered;

        len = writeRespBodyBlockFilterInternal(&m_sendFileInfo, pBuf, written,
                                               &param);
    }
    if (len < 0)
        return LS_FAIL;
    else if ((len > 0)
             && (len < written || buffered))
    {
        if (D_ENABLED(DL_MEDIUM))
            LOG_D((getLogger(),
                   "[%s] HandleEvent: Socket busy, len = %d, buffered = %d\n",
                   getLogId(), len, buffered));
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
    if (D_ENABLED(DL_MEDIUM))
        LOG_D((getLogger(), "[%s] Got Aio SF Event!  Returned %d\n",
               getLogId(), written));
    ret = getStream()->aiosendfiledone(event);
    if (written < 0)
    {
        if (ret)     //Not EAGAIN
            closeConnection();
        else
            continueWrite();
        return ret;
    }
    else if (written > 0)
    {
        m_response.written(written);
        if (D_ENABLED(DL_MEDIUM))
            LOG_D((getLogger(),
                   "[%s] Aio Response body sent: %lld \n",
                   getLogId(), (long long)m_response.getBodySent()));
        m_sendFileInfo.incCurPos(written);
        if ((unsigned int)written < event->getSize())
        {
            continueWrite();
            return 0;
        }
    }
    //File should be completely sent.
    if (m_sendFileInfo.getRemain() > 0)
    {
        if (D_ENABLED(DL_MEDIUM))
            LOG_D((getLogger(),
                   "[%s] Handle Aio Event: Reached an invalid conclusion! \n",
                   getLogId()));
        return LS_FAIL;
    }
    return onWriteEx();
}


int HttpSession::smProcessReq()
{
    int ret = 0;
    while (ret == 0 && m_processState < HSPS_HANDLER_PROCESSING)
    {
        switch (m_processState)
        {
        case HSPS_READ_REQ_HEADER:
            ret = readToHeaderBuf();
            if (!ret)
            {
                if (m_request.getStatus() == HttpReq::HEADER_OK)
                {
                    m_iFlag &= ~HSF_URI_PROCESSED;
                    m_processState = HSPS_NEW_REQ;
                }
                else
                    return 0;
            }
            else
                break;
        case HSPS_NEW_REQ:
            ret = processNewReqInit();
            if (m_processState != HSPS_HKPT_HTTP_BEGIN)
                break;
        //fall through
        case HSPS_HKPT_HTTP_BEGIN:
            ret = runEventHkpt(LSI_HKPT_HTTP_BEGIN, HSPS_HKPT_RECV_REQ_HEADER);
            if (ret || m_processState != HSPS_HKPT_RECV_REQ_HEADER)
                break;
        //fall through
        case HSPS_HKPT_RECV_REQ_HEADER:
            ret = runEventHkpt(LSI_HKPT_RECV_REQ_HEADER, HSPS_PROCESS_NEW_REQ_BODY);
            if (ret || m_processState != HSPS_PROCESS_NEW_REQ_BODY)
                break;
        //fall through
        case HSPS_PROCESS_NEW_REQ_BODY:
            ret = processNewReqBody();
            if (ret || m_processState != HSPS_HKPT_RCVD_REQ_BODY)
                break;
        //fall through
        case HSPS_HKPT_RCVD_REQ_BODY:
            ret = runEventHkpt(LSI_HKPT_RCVD_REQ_BODY, HSPS_PROCESS_NEW_URI);
            if (ret || m_processState != HSPS_PROCESS_NEW_URI)
                break;
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
            ret = runEventHkpt(LSI_HKPT_URI_MAP, HSPS_FILE_MAP);
            if (ret || m_processState != HSPS_FILE_MAP)
                break;
        //fall through
        case HSPS_FILE_MAP:
            ret = processFileMap();
            if (ret || m_processState != HSPS_CONTEXT_AUTH)
                break;
        //fall through
        case HSPS_CONTEXT_AUTH:
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
            if (!(m_iFlag & HSF_URI_PROCESSED))
            {
                m_processState = HSPS_CONTEXT_MAP;
                break;
            }
            else if ((m_iFlag & HSF_SC_404))
            {
                ret = SC_404;
                m_iFlag &= ~HSF_SC_404;
                break;
            }
            else
                m_processState = HSPS_BEGIN_HANDLER_PROCESS;
        case HSPS_BEGIN_HANDLER_PROCESS:
            ret = handlerProcess(m_request.getHttpHandler());
            break;
        case HSPS_HKPT_RCVD_REQ_BODY_PROCESSING:
            ret = runEventHkpt(LSI_HKPT_RCVD_REQ_BODY, HSPS_HANDLER_PROCESSING);
            if (m_processState == HSPS_HANDLER_PROCESSING)
                if (m_pHandler)
                    m_pHandler->onRead(this);
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
        case HSPS_READ_REQ_BODY:
            return 0;
        case HSPS_RCVD_RESP_HEADER_DONE:
        case HSPS_RCVD_RESP_BODY_DONE:
        case HSPS_SEND_RESP_HEADER_DONE:
            //temporary state,
            m_processState = HSPS_HANDLER_PROCESSING;
            return 0;
        case HSPS_HTTP_END_DONE:
        case HSPS_HANDLER_RESTART_DONE:

        default:
            assert("Unhanlded processing state, should not happen" == NULL);
            break;
        }

    }
    if (ret == LSI_HK_RET_SUSPEND)
    {
        suspendRead();
        suspendWrite();
        m_iFlag |= HSF_SUSPENDED;
        return 0;
    }
    else if (ret > 0)
    {
        m_processState = HSPS_HTTP_ERROR;
        if (getStream()->getState() < HIOS_SHUTDOWN)
            httpError(ret);
    }
    return ret;
}


int HttpSession::resumeProcess(int passCode, int retcode)
{
    //if ( passCode != m_suspendPasscode )
    //    return LS_FAIL;
    if (!(m_iFlag & HSF_SUSPENDED))
        return LS_FAIL;
    m_iFlag &= ~HSF_SUSPENDED;
    if (!(m_iFlag & HSF_REQ_BODY_DONE))
        continueRead();
    if (m_pHandler
        && !(m_iFlag & (HSF_HANDLER_DONE | HSF_HANDLER_WRITE_SUSPENDED)))
        continueWrite();


    switch (m_processState)
    {
    case HSPS_HKPT_HTTP_BEGIN:
    case HSPS_HKPT_RECV_REQ_HEADER:
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
    suspendRead();
    suspendWrite();
    return m_suspendPasscode;
}

