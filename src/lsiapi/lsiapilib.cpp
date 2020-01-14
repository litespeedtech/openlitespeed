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

#include <edio/multiplexerfactory.h>
#include <edio/evtcbque.h>
#include <edio/multiplexer.h>
#include <http/accesslog.h>
#include <http/handlertype.h>
#include <http/httplog.h>
#include <http/httpmethod.h>
#include <http/httpresourcemanager.h>
#include <http/httprespheaders.h>
#include <http/httpserverversion.h>
#include <http/httpsession.h>
#include <http/httpstatuscode.h>
#include <http/httpver.h>
#include <http/httpvhost.h>
#include <http/mtsessdata.h>
#include <http/phpconfig.h>
#include <http/reqparser.h>
#include <http/requestvars.h>
#include <http/staticfilecachedata.h>
#include <http/reqparser.h>
#include <http/clientinfo.h>
#include <http/ntwkiolink.h>

#include <log4cxx/logger.h>
#include <lsiapi/envmanager.h>
#include <lsiapi/internal.h>
#include <lsiapi/lsiapi.h>
#include <lsiapi/lsiapi_const.h>
#include <lsiapi/lsiapihooks.h>
#include <lsiapi/modulehandler.h>
#include <lsiapi/modulemanager.h>
#include <lsiapi/moduletimer.h>
#include "ediohandler.h"
#include <main/configctx.h>
#include <main/httpserver.h>
#include <main/mainserverconfig.h>
#include <sslpp/sslconnection.h>
#include <sslpp/sslutil.h>
#include <sslpp/sslcert.h>
#include <thread/mtnotifier.h>
#include <thread/workcrew.h>
#include <util/datetime.h>
#include <util/filtermatch.h>
#include <util/httputil.h>
#include <util/radixtree.h>
#include <util/stringtool.h>
#include <util/vmembuf.h>
#include <util/accessdef.h>
#include <unistd.h>
#include <openssl/x509.h>

static long schedule_mt_notify_event(const lsi_session_t *session);
static long schedule_mt_notify_event_params(const lsi_session_t *session,
                                           long lParam, void * pParam);
static int blocking_mt_notify_event(const lsi_session_t *session, int32_t flag,
                                    void *pParam);

static char * lsi_new_local_buf_ts(HttpSession * pSession, size_t &size, MtWriteMode writeMode)
{
    if (WRITE_THROUGH == writeMode)
    {
        size_t reqSize = size;
        // beginMtHandler sets up the VMemBuf in HttpSession so we
        // never expect the getRespBodyBuf to fail. TODO: beginMtHandler does
        // not check return values from setup funcs
        pSession->getMtSessData()->m_mtWrite.m_curVMemBuf = pSession->getRespBodyBuf();
        char * localBufPtr = pSession->getMtSessData()->m_mtWrite.m_curVMemBuf->getWriteBuffer(size);
        LSI_DBGH(pSession, "%s: got new writeBuffer i%p, wanted %lu got %lu)\n",
                 __func__, localBufPtr, reqSize, size);
        //assert(reqSize == size && "did NOT get requested size!");
        return localBufPtr;
    }
    else
    {
        char * lBuf = new char[size]; // TODO: get rid of magic number
        assert(lBuf && "lBuf new failed");
        if (!lBuf)
        {
            LSI_ERR(pSession, "%s: failed allocating local buf\n", __func__);
            return NULL;
        }
        else {
            LSI_DBGH(pSession, "%s: got new heap bufer (%p, %lu)\n",
                     __func__, lBuf, size);
            return lBuf;
        }
    }
}

static void lsi_flush_local_buf_ts(HttpSession * pSession)
{
    LSI_DBGH(pSession, "enter %s\n", __func__);

    MtWriteMode & writeMode = pSession->getMtSessData()->m_mtWrite.m_writeMode;
    MtWriteBuf & wBuf = pSession->getMtSessData()->m_mtWrite.m_writeBuf;
    if (wBuf.getBuf() && wBuf.getCurPos() != wBuf.getBuf())
    {
        if (WRITE_THROUGH == writeMode)
        {
            VMemBuf * curVMemBuf = pSession->getMtSessData()->m_mtWrite.m_curVMemBuf;
            LSI_DBGH(pSession, "%s: calling writeUsed (vMemBuf ptr %p size %lu)\n",
                     __func__, curVMemBuf, wBuf.getCurPos() - wBuf.getBuf());
            curVMemBuf->writeUsed(wBuf.getCurPos() - wBuf.getBuf());
            wBuf.set(NULL, 0, NULL);
        }
        else
        {
            if (wBuf.getBuf())
            {
                MtSessData * sd = pSession->getMtSessData();
                ls_mutex_lock(&sd->m_mtWrite.m_bufQ_lock);
                if (NULL == sd->m_mtWrite.m_bufQ)
                {
                    sd->m_mtWrite.m_bufQ = new MtLocalBufQ;
                }
                sd->m_mtWrite.m_bufQ->append(wBuf); // copies to new instance of MtWriteBuf, deleted in HttpSession

                MtLocalBufQ * lbq = sd->m_mtWrite.m_bufQ;

                ls_mutex_unlock(&sd->m_mtWrite.m_bufQ_lock);
                LSI_DBGH(pSession, "%s: pushed on bufQ %p buf %p size %ld\n",
                         __func__, lbq, wBuf.getBuf(), wBuf.getCurPos() - wBuf.getBuf());

                LSI_DBGH(pSession, "%s: notified main thread\n", __func__);
                schedule_mt_notify_event_params((lsi_session_t *)(LsiSession *)pSession,
                                                HSF_MT_FLUSH_RBDY_LBUF_Q, NULL);

                wBuf.set(NULL, 0, NULL);
            }
        }
    }
}

static int is_release_cb_added(const lsi_module_t *pModule, int level)
{
    return ((MODULE_DATA_ID(pModule)[level] != -1) ? 1 : 0);
}


// static int is_cb_added( const lsi_module_t *pModule, lsi_callback_pf cb, int index )
// {
//     LsiApiHooks * pLsiApiHooks = (LsiApiHooks *)LsiApiHooks::getGlobalApiHooks( index );
//     if (pLsiApiHooks->find(pModule, cb))
//         return 1;
//     else
//         return 0;
// }


static int lsiapi_add_release_data_hook(int index,
                                        const lsi_module_t *pModule, lsi_callback_pf cb)
{
    LsiApiHooks *pHooks;
    if ((index < 0) || (index >= LSI_DATA_COUNT))
        return LS_FAIL;
    pHooks = LsiApiHooks::getReleaseDataHooks(index);
    if (pHooks->add(pModule, cb, 0, LSI_FLAG_ENABLED) >= 0)
        return LS_OK;
    return LS_FAIL;
}


int add_global_hook(int index, const lsi_module_t *pModule,
                    lsi_callback_pf cb, short order, short flag)
{
    const int *priority = MODULE_PRIORITY(pModule);
    LsiApiHooks *pHooks;
    if ((index < 0) || (index >= LSI_HKPT_TOTAL_COUNT))
        return LS_FAIL;
    pHooks = (LsiApiHooks *) LsiApiHooks::getGlobalApiHooks(index);
    if (priority[index] < LSI_HOOK_PRIORITY_MAX
        && priority[index] > -1 * LSI_HOOK_PRIORITY_MAX)
        order = priority[index];
    LSM_DBGH(pModule, NULL, "add_global_hook, index %d [%s], priority %hd, flag %hd\n",
             index, LsiApiHooks::s_pHkptName[index], order,
             flag);
    return pHooks->add(pModule, cb, order, flag);
}


static int get_hook_level(lsi_param_t *pParam)
{
    return pParam->hook_chain->hook_level;
}


static int enable_hook(const lsi_session_t *session,
                       const lsi_module_t *pModule, int enable,
                       int *index, int iNumIndices)
{
    int i, ret = LS_OK;
    int aL4Indices[LSI_HKPT_L4_COUNT], iL4Count = 0;
    int aHttpIndices[LSI_HKPT_HTTP_COUNT], iHttpCount = 0;
    LsiSession *pSession = (LsiSession *)session;
    if (index == NULL || iNumIndices <= 0)
        return LS_FAIL;
    for (i = 0; i < iNumIndices; ++i)
    {
        switch (index[i])
        {
        case LSI_HKPT_L4_BEGINSESSION:
        case LSI_HKPT_L4_ENDSESSION:
        case LSI_HKPT_L4_RECVING:
        case LSI_HKPT_L4_SENDING:
            aL4Indices[iL4Count++] = index[i];
            break;

        case LSI_HKPT_HTTP_BEGIN:
        case LSI_HKPT_RCVD_REQ_HEADER:
        case LSI_HKPT_URI_MAP:
        case LSI_HKPT_RECV_REQ_BODY:
        case LSI_HKPT_RCVD_REQ_BODY:
        case LSI_HKPT_RCVD_RESP_HEADER:
        case LSI_HKPT_RECV_RESP_BODY:
        case LSI_HKPT_RCVD_RESP_BODY:
        case LSI_HKPT_HANDLER_RESTART:
        case LSI_HKPT_SEND_RESP_HEADER:
        case LSI_HKPT_SEND_RESP_BODY:
        case LSI_HKPT_HTTP_END:
            aHttpIndices[iHttpCount++] = index[i];
            break;
        default:
            break;
        }
    }
    if (iL4Count > 0)
        ret = ((NtwkIOLink *)pSession)->getSessionHooks()->setEnable(
                  pModule, enable, aL4Indices, iL4Count);

    if (ret == LS_OK && iHttpCount > 0)
        ret = ((HttpSession *)pSession)->getSessionHooks()->setEnable(
                  pModule, enable, aHttpIndices, iHttpCount);

    LSM_DBGH(pModule, NULL, "enable_hook, enable %d, "
             "num indices %d, return %d\n", enable,
             iNumIndices, ret);
    return ret;
}


static int get_hook_flag(const lsi_session_t *session, int index)
{
    LsiSession *pSession = (LsiSession *)session;
    HttpSession *pHttp = dynamic_cast<HttpSession *>(pSession);
    if (pHttp)
        return pHttp->getSessionHooks()->getFlag(index);
    else
    {
        NtwkIOLink *pNtwk = dynamic_cast<NtwkIOLink *>(pSession);
        if (pNtwk)
            return pNtwk->getSessionHooks()->getFlag(index);
    }
    return 0;
}


static  void log(const lsi_session_t *session, int level, const char *fmt, ...)
{
    if (log4cxx::Level::isEnabled(level))
    {
        HttpSession *pSess = (HttpSession *)((LsiSession *)session);
        LogSession *pLogSess = pSess ? pSess->getLogSession() : NULL;
        va_list ap;
        va_start(ap, fmt);
        LOG4CXX_NS::Logger::s_vlog(level, pLogSess, fmt, ap, 1);
        va_end(ap);
    }

}


static  void vlog(const lsi_session_t *session, int level, const char *fmt,
                  va_list vararg, int no_linefeed)
{
    if (log4cxx::Level::isEnabled(level))
    {
        HttpSession *pSess = (HttpSession *)((LsiSession *)session);
        LogSession *pLogSess = pSess ? pSess->getLogSession() : NULL;
        LOG4CXX_NS::Logger::s_vlog(level, pLogSess, fmt, vararg, no_linefeed);
    }
//     char achFmt[4096];
//     HttpSession *pSess = (HttpSession *)((LsiSession *)session);
//     LOG4CXX_NS::Logger *pLogger = NULL;
//     if (pSess)
//     {
//         pLogger = pSess->getLogger();
//         if (pSess->getLogId())
//         {
//             int n = snprintf(achFmt, sizeof(achFmt) - 2, "[%s] %s", pSess->getLogId(),
//                              fmt);
//             if ((size_t)n > sizeof(achFmt) - 2)
//                 achFmt[sizeof(achFmt) - 2 ] = 0;
//             fmt = achFmt;
//         }
//     }
//     HttpLog::vlog(pLogger, level, fmt, vararg, no_linefeed);

}


static  void lograw(const lsi_session_t *session, const char *buf, int len)
{
    HttpSession *pSess = (HttpSession *)((LsiSession *)session);
    LOG4CXX_NS::Logger *pLogger = NULL;
    if (pSess)
        pLogger = pSess->getLogger();
    if (!pLogger)
        pLogger = LOG4CXX_NS::Logger::getLogger(NULL);
    pLogger->lograw(buf, len);
}


static void module_log(const lsi_module_t *pMod, const lsi_session_t *session,
                       int level, const char *fmt, ...)
{
    if (log4cxx::Level::isEnabled(level) && level <= MODULE_LOG_LEVEL(pMod))
    {
        char new_fmt[8192];
        snprintf(new_fmt, 8191, "[%s] %s", MODULE_NAME(pMod), fmt);
        HttpSession *pSess = (HttpSession *)((LsiSession *)session);
        LogSession *pLogSess = pSess ? pSess->getLogSession() : NULL;
        va_list ap;
        va_start(ap, fmt);
        LOG4CXX_NS::Logger::s_vlog(level, pLogSess, new_fmt, ap, 1);
        va_end(ap);

    }
}


static void c_log(const char *pComponent, const lsi_session_t *session,
                       int level, const char *fmt, ...)
{
    if (log4cxx::Level::isEnabled(level))
    {
        char new_fmt[8192];
        snprintf(new_fmt, 8191, "[%s] %s", pComponent, fmt);
        HttpSession *pSess = (HttpSession *)((LsiSession *)session);
        LogSession *pLogSess = pSess ? pSess->getLogSession() : NULL;
        va_list ap;
        va_start(ap, fmt);
        LOG4CXX_NS::Logger::s_vlog(level, pLogSess, new_fmt, ap, 1);
        va_end(ap);

    }
}


static int lsiapi_register_env_handler(const char *env_name,
                                       unsigned int env_name_len, lsi_callback_pf cb)
{
    return EnvManager::getInstance().regEnvHandler(env_name, env_name_len, cb);
}


//If register a RELEASE hook, we need to make sure it is called only once,
//and then upodate and record the module's _data_id
//Return -1 wrong level number
//Return -2, already inited
static int init_module_data(const lsi_module_t *pModule,
                            lsi_datarelease_pf cb, int level)
{
    if (level < LSI_DATA_HTTP || level > LSI_DATA_L4)
        return LS_FAIL;

    if (MODULE_DATA_ID(pModule)[level] == -1)
    {
        MODULE_DATA_ID(pModule)[level] =
            ModuleManager::getInstance().getModuleDataCount(level);
        ModuleManager::getInstance().incModuleDataCount(level);
        return lsiapi_add_release_data_hook(level, pModule, (lsi_callback_pf)cb);
    }

    return -2;
}


static LsiModuleData *get_module_data_by_type(const lsi_session_t *session_,
        int type)
{
    NtwkIOLink *pNtwkLink;
    HttpSession *pSession = NULL;
    LsiSession *session = (LsiSession *)session_;
    switch (type)
    {
    case LSI_DATA_HTTP:
    case LSI_DATA_FILE:
    case LSI_DATA_VHOST:
        pSession = dynamic_cast< HttpSession *>(session);
        if (!pSession)
            return NULL;
        break;
    }

    switch (type)
    {
    case LSI_DATA_L4:
        pNtwkLink = dynamic_cast< NtwkIOLink *>(session);
        if (!pNtwkLink)  //Comment: make sure dealloc hookfunction is set.
            break;
        return pNtwkLink->getModuleData();
    case LSI_DATA_HTTP:
        return pSession->getModuleData();
    case LSI_DATA_FILE:
        if (!pSession->getSendFileInfo()->getFileData())
            break;
        return pSession->getSendFileInfo()->getFileData()->getModuleData();
    case LSI_DATA_IP:
        pSession = dynamic_cast< HttpSession *>(session);
        if (pSession)
            return pSession->getClientInfo()->getModuleData();
        else
        {
            pNtwkLink = dynamic_cast< NtwkIOLink *>(session);
            if (pNtwkLink)
                return pNtwkLink->getClientInfo()->getModuleData();
        }
        break;
    case LSI_DATA_VHOST:
        if (!pSession->getReq()->getVHost())
            break;
        return pSession->getReq()->getVHost()->getModuleData();
    }
    return NULL;

}


static int set_module_data(const lsi_session_t *session,
                           const lsi_module_t *pModule, int level, void *data)
{
    if (level < LSI_DATA_HTTP || level > LSI_DATA_L4)
        return LS_FAIL;

    if (!is_release_cb_added(pModule, level))
        return LS_FAIL;

    int ret = -1;
    LsiModuleData *pData = get_module_data_by_type(session, level);

    if (pData)
    {
        if (!pData->isDataInited())
        {
            if (data)
                pData->initData(ModuleManager::getInstance().getModuleDataCount(level));
            else
                return LS_OK;
        }
        pData->set(MODULE_DATA_ID(pModule)[level], data);
        ret = LS_OK;
    }

    return ret;
}


static void *get_module_data(const lsi_session_t *session,
                             const lsi_module_t *pModule, int level)
{
    LsiModuleData *pData = get_module_data_by_type(session, level);
    if (!pData)
        return NULL;
    return pData->get(MODULE_DATA_ID(pModule)[level]);
}


static void *get_cb_module_data(const lsi_param_t *param, int level)
{
    LsiModuleData *pData = get_module_data_by_type(param->session, level);
    if (!pData)
        return NULL;

    return pData->get(MODULE_DATA_ID(((lsiapi_hook_t *)
                                      param->cur_hook)->module)[level]);
}


static void free_module_data(const lsi_session_t *session,
                             const lsi_module_t *pModule, int level, lsi_datarelease_pf cb)
{
    LsiModuleData *pData = get_module_data_by_type(session, level);
    if (!pData)
        return;
    void *data = pData->get(MODULE_DATA_ID(pModule)[level]);
    if (data)
    {
        cb(data);
        pData->set(MODULE_DATA_ID(pModule)[level], NULL);
    }
}

static LSI_REQ_METHOD get_req_method(const lsi_session_t *pSession)
{
    if (pSession == NULL)
        return LSI_METHOD_UNKNOWN;
    return (LSI_REQ_METHOD)((HttpSession *)(LsiSession *)pSession)->getReq()->getMethod();
}


static const void *get_req_vhost(const lsi_session_t *pSession)
{
    if (pSession == NULL)
        return NULL;
    return ((HttpSession *)(LsiSession *)pSession)->getReq()->getVHost();
}


/////////////////////////////////////////////////////////////////////////////
static int stream_write_next(lsi_param_t *pParam, const char *buf,
                             int len)
{
    lsi_param_t param;
    param.session = pParam->session;
    param.cur_hook = (void *)(((lsiapi_hook_t *)pParam->cur_hook) + 1);
    param.hook_chain = pParam->hook_chain;
    param.ptr1 = buf;
    param.len1 = len;
    param.flag_out = pParam->flag_out;
    param.flag_in = pParam->flag_in;
    return LsiApiHooks::runForwardCb(&param);
}


/////////////////////////////////////////////////////////////////////////////
static int lsiapi_stream_read_next(lsi_param_t *pParam, char *pBuf,
                                   int size)
{
    lsi_param_t param;
    param.session = pParam->session;
    param.cur_hook = (void *)(((lsiapi_hook_t *)pParam->cur_hook) - 1);
    param.hook_chain = pParam->hook_chain;
    param.ptr1 = pBuf;
    param.len1 = size;
    param.flag_out = pParam->flag_out;
    param.flag_in = pParam->flag_in;
    return LsiApiHooks::runBackwardCb(&param);
}


static int lsiapi_stream_writev_next(lsi_param_t *pParam,
                                     struct iovec *iov, int count)
{
    lsi_param_t param;
    param.session = pParam->session;
    param.cur_hook = (void *)(((lsiapi_hook_t *)pParam->cur_hook) + 1);
    param.hook_chain = pParam->hook_chain;
    param.ptr1 = iov;
    param.len1 = count;
    param.flag_out = pParam->flag_out;
    param.flag_in = pParam->flag_in;
    return LsiApiHooks::runForwardCb(&param);
}


/////////////////////////////////////////////////////////////////////////////
static int get_uri_file_path(const lsi_session_t *session, const char *uri,
                             int uri_len, char *path, int max_len)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpReq *pReq = pSession->getReq();

    if (uri[0] != '/')
        return LS_FAIL;

    if (!pReq->getVHost())
        return -2;

    const AutoStr2 *rootStr = NULL;
    if (pReq->getContext())
        rootStr = pReq->getContext()->getRoot();
    else
        rootStr = pReq->getDocRoot();

    if (max_len <= rootStr->len() + uri_len)
        return -2; //Not enough space

    memcpy(path, rootStr->c_str(), rootStr->len());
    memcpy(path + rootStr->len(), uri, uri_len);
    path[rootStr->len() + uri_len] = 0x00;
    return LS_OK;
}


static int set_resp_content_length(const lsi_session_t *session, int64_t len)
{
    HttpResp *pResp = ((HttpSession *)((LsiSession *)session))->getResp();
    pResp->setContentLen(len);
    pResp->appendContentLenHeader();
    return LS_OK;
}


static int get_status_code(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpReq *pReq = pSession->getReq();
    return HttpStatusCode::getInstance().indexToCode(pReq->getStatusCode());
}


static  void set_status_code(const lsi_session_t *session, int code)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return;
    HttpReq *pReq = pSession->getReq();
    int index = HttpStatusCode::getInstance().codeToIndex(code);
    pReq->setStatusCode(index);
    pReq->updateNoRespBodyByStatus(index);
}


static int set_resp_header(const lsi_session_t *session,
                           unsigned int header_index, const char *name,
                           int nameLen, const char *val, int valLen,
                           int add_method)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    if (pSession->isRespHeaderSent())
        return LS_FAIL;
    HttpRespHeaders &respHeaders = pSession->getResp()->getRespHeaders();
    if (header_index < LSI_RSPHDR_END)
    {
        respHeaders.add((HttpRespHeaders::INDEX)header_index, val, valLen,
                        add_method);
        if (header_index == HttpRespHeaders::H_CONTENT_TYPE)
            pSession->updateContentCompressible();
    }
    else
        respHeaders.add(name, nameLen, val, valLen, add_method);
    return LS_OK;
}


//multi headers supported.
static int set_resp_header2(const lsi_session_t *session, const char *s, int len,
                            int add_method)
{
    int len0, len1;
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    if (pSession->isRespHeaderSent())
        return LS_FAIL;
    HttpRespHeaders &respHeaders = pSession->getResp()->getRespHeaders();

    //Save and check if the contenttype updated, if yes, processContentType
    //most case is change from NULL to a valid type.

    char *type0 = respHeaders.getContentTypeHeader(len0);
    respHeaders.parseAdd(s, len, add_method);
    char *type1 = respHeaders.getContentTypeHeader(len1);

    if (type0 != type1 || len0 != len1)
        pSession->updateContentCompressible();

    return LS_OK;
}


static int set_resp_cookies(const lsi_session_t *session, const char *pName,
                            const char *pVal, const char *path,
                            const char *domain, int expires,
                            int secure, int httponly)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpResp *pResp = pSession->getResp();
    if (pResp == NULL)
        return LS_FAIL;
    if (pSession->isRespHeaderSent())
        return LS_FAIL;
    return pResp->addCookie(pName, pVal, path, domain, expires, secure,
                            httponly);
}


static int set_resp_cookies_ts(const lsi_session_t *session, const char *pName,
                            const char *pVal, const char *path,
                            const char *domain, int expires,
                            int secure, int httponly)
{
    LSI_DBGH(session, "enter %s: pName %s pVal %s path %s domain %s expires %d secure %d httponly %d\n",
             __func__,  pName, pVal, path, domain, expires, secure, httponly);
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpResp *pResp = pSession->getResp();
    if (pResp == NULL)
        return LS_FAIL;
    MtLock lk(pSession->getMtSessData()->m_respHeaderLock);
    if (pSession->isRespHeaderSent())
    {
        LS_TH_BENIGN(lk.getPtr(), "ok exiting");
        return LS_FAIL;
    }
    int res = pResp->addCookie(pName, pVal, path, domain, expires, secure,
                            httponly);
    return res;
}


static int get_resp_header(const lsi_session_t *session,
                           unsigned int header_index, const char *name,
                           int nameLen, struct iovec *iov, int maxIovCount)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpRespHeaders &respHeaders = pSession->getResp()->getRespHeaders();
    if (header_index < LSI_RSPHDR_END)
        return respHeaders.getHeader((HttpRespHeaders::INDEX)header_index,
                                     iov, maxIovCount);
    else
        return respHeaders.getHeader(name, nameLen, iov, maxIovCount);
}


//"status" and "version" will also be treated as header in SPDY if it already be put in
//and it will be counted.
static int get_resp_headers_count(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpRespHeaders &respHeaders = pSession->getResp()->getRespHeaders();
    return respHeaders.getCount();  //For API, retuen the non-spdy case
}


static unsigned int get_resp_header_id(const lsi_session_t *session,
                                       const char *name)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpRespHeaders &respHeaders = pSession->getResp()->getRespHeaders();
    return respHeaders.getIndex(name);
}


static int get_resp_headers(const lsi_session_t *session, struct iovec *iov_key,
                            struct iovec *iov_val, int maxIovCount)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpRespHeaders &respHeaders = pSession->getResp()->getRespHeaders();
    return respHeaders.getAllHeaders(iov_key, iov_val, maxIovCount);
}


static int remove_resp_header(const lsi_session_t *session,
                              unsigned int header_index, const char *name,
                              int nameLen)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpRespHeaders &respHeaders = pSession->getResp()->getRespHeaders();
    if (header_index < LSI_RSPHDR_END)
        respHeaders.del((HttpRespHeaders::INDEX)header_index);
    else
        respHeaders.del(name, nameLen);
    return LS_OK;
}


static void send_resp_headers(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return;
    pSession->sendRespHeaders();
}


static int is_resp_headers_sent(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FALSE;
    return pSession->isRespHeaderSent();
}


static int get_req_raw_headers_length(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpReq *pReq = pSession->getReq();
    return pReq->getHttpHeaderLen();
}


static int get_req_raw_headers(const lsi_session_t *session, char *buf,
                               int maxlen)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpReq *pReq = pSession->getReq();
    const char *p = pReq->getHeaderBuf().begin();
    int size = pReq->getHttpHeaderLen();
    if (size > maxlen)
        size = maxlen;
    memcpy(buf, p, size);
    return size;
}


static int get_req_headers_count(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    HttpReq *pReq = pSession->getReq();
    int count = 0;
    const char *pTemp;
    for (int i = 0; i < HttpHeader::H_TRANSFER_ENCODING; ++i)
    {
        pTemp = pReq->getHeader(i);
        if (*pTemp)
            ++count;
    }

    count += pReq->getUnknownHeaderCount();
    return count;
}


static int get_req_headers(const lsi_session_t *session, struct iovec *iov_key,
                           struct iovec *iov_val, int maxIovCount)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    HttpReq *pReq = pSession->getReq();
    int i, n;
    const char *pTemp;
    int index = 0;
    for (i = 0; i <= HttpHeader::H_TRANSFER_ENCODING; ++i)
    {
        pTemp = pReq->getHeader(i);
        if (*pTemp)
        {
            iov_key[index].iov_base = (void *)RequestVars::getHeaderString(i);
            iov_key[index].iov_len = HttpHeader::getHeaderStringLen(i);

            iov_val[index].iov_base = (void *)pTemp;
            iov_val[index].iov_len = pReq->getHeaderLen(i);

            ++index;
        }
    }

    n = pReq->getUnknownHeaderCount();
    for (i = 0; i < n; ++i)
    {
        const char *pKey;
        const char *pVal;
        int keyLen;
        int valLen;
        pKey = pReq->getUnknownHeaderByIndex(i, keyLen, pVal, valLen);
        if (pKey)
        {
            iov_key[index].iov_base = (void *)pKey;
            iov_key[index].iov_len = keyLen;

            iov_val[index].iov_base = (void *)pVal;
            iov_val[index].iov_len = valLen;

            ++index;
        }
    }

    return index;
}


static int get_req_org_uri(const lsi_session_t *session, char *buf, int buf_size)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpReq *pReq = pSession->getReq();

    int orgLen = pReq->getOrgReqURILen();
    if (buf_size > orgLen)
    {
        memcpy(buf, pReq->getOrgReqURL(), orgLen);
        buf[orgLen] = 0x00;
    }
    return orgLen;
}


static const char *get_req_uri(const lsi_session_t *session, int *uri_len)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return NULL;
    HttpReq *pReq = pSession->getReq();
    if (uri_len)
        *uri_len = pReq->getURILen();
    return pReq->getURI();
}


static const char *get_mapped_context_uri(const lsi_session_t *session,
        int *length)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return NULL;
    HttpReq *pReq = pSession->getReq();
    if (pReq)
        *length = pReq->getContext()->getURILen();
    return pReq->getContext()->getURI();
}


static int is_req_handler_registered(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession)
    {
        HttpReq *pReq = pSession->getReq();
        if (pReq && pReq->getHttpHandler()
            && pReq->getHttpHandler()->getType() == HandlerType::HT_MODULE)
            return LS_TRUE;
    }

    return LS_FALSE;
}


static int register_req_handler(const lsi_session_t *session,
                                lsi_module_t *pModule, int scriptLen)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpReq *pReq = pSession->getReq();
    if (pReq && pReq->getHttpHandler()
        && pReq->getHttpHandler()->getType() == HandlerType::HT_MODULE)
        return LS_FAIL; //already registered by a module

    LsiModule *pHandler = MODULE_HANDLER(pModule);
    if ((pHandler != NULL)
        && (pModule->reqhandler != NULL))
    {
        pReq->setHandler(pHandler);
        pReq->setScriptNameLen(scriptLen);
        return LS_OK;
    }

    return LS_FAIL;
}


static int set_handler_write_state(const lsi_session_t *session, int state)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (state)
    {
        pSession->continueWrite();
        pSession->setState(HSS_WRITING);
        pSession->setFlag(HSF_HANDLER_WRITE_SUSPENDED | HSF_RESP_FLUSHED, 0);
    }
    else
    {
        //Should only work on HSS_WRITING state.
        if (pSession->getState() != HSS_WRITING)
            return LS_FAIL;

        pSession->suspendWrite();
        pSession->setFlag(HSF_HANDLER_WRITE_SUSPENDED, 1);
    }
    return 0;
}


//return time_id, if error return -1
static int lsi_set_timer(unsigned int timeout_ms, int repeat,
                         lsi_timercb_pf timer_cb, const void *timer_cb_param)
{
    int id = ModTimerList::getInstance().addTimer(timeout_ms, repeat, timer_cb,
             timer_cb_param);
    return id;
}


static int lsi_remove_timer(int timer_id)
{
    return ModTimerList::getInstance().removeTimer(timer_id);
}


/*
static long create_event(evtcb_pf cb,
                         const lsi_session_t *session, long lParam, void *pParam)
{
    return (long)EvtcbQue::getInstance().schedule(cb, session,
            lParam, pParam, false);
}
*/

static long create_event(evtcb_pf cb, const lsi_session_t *session,
                         long lParam, void *pParam, int nowait)
{
    return (long)EvtcbQue::getInstance().schedule(cb, session,
            lParam, pParam, nowait);
}


static long create_session_resume_event(const lsi_session_t *session,
                                        lsi_module_t *pModule)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    evtcb_pf cb = (evtcb_pf)(&HttpSession::hookResumeCallback);
    return create_event(cb, session, pSession->getSn(), NULL, true);
}


static long get_event_obj(evtcb_pf cb, const lsi_session_t *session,
                         long lParam, void *pParam)
{
   // return (long)EvtcbQue::getInstance().getNodeObj(cb, pSession, lParam, pParam);
    evtcbnode_s *pEvtObj = EvtcbQue::getInstance().getNodeObj(cb, session, lParam, pParam);
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession)
        pSession->setBackRefPtr(EvtcbQue::getSessionRefPtr(pEvtObj));
    return (long)pEvtObj;
}


static void schedule_event(long event_obj, int nowait)
{
    EvtcbQue::getInstance().schedule((evtcbnode_s *)event_obj, nowait);
}


static void cancel_event(const lsi_session_t *session, long event_obj)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    pSession->cancelEvent((evtcbnode_s *)event_obj);
}


static const char *get_req_header(const lsi_session_t *session, const char *key,
                                  int keyLen, int *valLen)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return NULL;
    HttpReq *pReq = pSession->getReq();
    size_t idx = HttpHeader::getIndex(key);
    if (idx != HttpHeader::H_HEADER_END)
    {
        *valLen = pReq->getHeaderLen(idx);
        return pReq->getHeader(idx);
    }

    return pReq->getHeader(key, keyLen, *valLen);
}


static const char *get_req_header_by_id(const lsi_session_t *session, int idx,
                                        int *valLen)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return NULL;
    HttpReq *pReq = pSession->getReq();
    if ((idx >= 0) && (idx < HttpHeader::H_HEADER_END))
    {
        *valLen = pReq->getHeaderLen(idx);
        return pReq->getHeader(idx);
    }
    return NULL;
}


static const char *get_req_cookies(const lsi_session_t *session, int *len)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return NULL;
    return get_req_header(session, "Cookie", 6, len);
}


static const char *get_client_ip(const lsi_session_t *session, int *len)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return NULL;
    *len = pSession->getPeerAddrStrLen();
    return pSession->getPeerAddrString();
}


static const char *get_req_query_string(const lsi_session_t *session, int *len)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return NULL;
    HttpReq *pReq = pSession->getReq();
    if (len)
        *len = pReq->getQueryStringLen();
    return pReq->getQueryString();
}


static int get_req_cookie_count(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpReq *pReq = pSession->getReq();
    return RequestVars::getCookieCount(pReq);
}


static const char *get_cookie_value(const lsi_session_t *session,
                                    const char *cookie_name, int nameLen,
                                    int *valLen)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL || !cookie_name || nameLen <= 0)
        return NULL;
    HttpReq *pReq = pSession->getReq();
    return RequestVars::getCookieValue(pReq, cookie_name, nameLen, *valLen);
}


static int get_cookie_by_index(const lsi_session_t *session, int index, ls_strpair_t *cookie)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpReq *pReq = pSession->getReq();
    pReq->parseCookies();
    const CookieList &list = pReq->getCookieList();
    if (index < 0 || index >= list.getSize())
      return LS_FALSE;

    const cookieval_t val = list.begin()[index];
    cookie->key.ptr = pReq->getHeaderBuf().getp(val.keyOff);
    cookie->key.len = val.keyLen;
    cookie->val.ptr = pReq->getHeaderBuf().getp(val.valOff);
    cookie->val.len = val.valLen;

    return LS_TRUE;
}


// static int get_req_body_file_fd( lsi_session_t *session )
// {
//     HttpSession *pSession = (HttpSession *)((LsiSession *)session);
//     if (pSession == NULL  )
//         return LS_FAIL;
//     HttpReq* pReq = ( (HttpSession *)((LsiSession *)session) )->getReq();
//     if (!pReq->getBodyBuf())
//         return LS_FAIL;
//
//     if (pReq->getBodyBuf()->getfd() != -1)
//         return pReq->getBodyBuf()->getfd();
//     else
//     {
//         char file_path[50];
//         strcpy(file_path, "/tmp/lsiapi-XXXXXX");
//         int fd = mkstemp( file_path );
//         if (fd != -1)
//         {
//             unlink(file_path);
//             fcntl( fd, F_SETFD, FD_CLOEXEC );
//             long len;
//             pReq->getBodyBuf()->exactSize( &len );
//             if ( -1 == pReq->getBodyBuf()->copyToFile( 0, len, fd, 0 ))
//             {
//                 close(fd);
//                 return LS_FAIL;
//             }
//         }
//
//         return fd;
//     }
// }


static const char *lsi_req_env[LSI_VAR_COUNT] =
{
    "REMOTE_ADDR",
    "REMOTE_PORT",
    "REMOTE_HOST",
    "REMOTE_USER",
    "REMOTE_IDENT",
    "REQUEST_METHOD",
    "QUERY_STRING",
    "AUTH_TYPE",
    "PATH_INFO",
    "SCRIPT_FILENAME",
    "REQUEST_FILENAME",
    "REQUEST_URI",
    "DOCUMENT_ROOT",
    "SERVER_ADMIN",
    "SERVER_NAME",
    "SERVER_ADDR",
    "SERVER_PORT",
    "SERVER_PROTOCOL",
    "SERVER_SOFTWARE",
    "API_VERSION",
    "THE_REQUEST",
    "IS_SUBREQ",
    "TIME",
    "TIME_YEAR",
    "TIME_MON",
    "TIME_DAY",
    "TIME_HOUR",
    "TIME_MIN",
    "TIME_SEC",
    "TIME_WDAY",
    "SCRIPT_NAME",
    "CURRENT_URI",
    "REQUEST_BASENAME",
    "SCRIPT_UID",
    "SCRIPT_GID",
    "SCRIPT_USERNAME",
    "SCRIPT_GROUPNAME",
    "SCRIPT_MODE",
    "SCRIPT_BASENAME",
    "SCRIPT_URI",
    "ORG_REQ_URI",
    "ORG_QS",
    "HTTPS",
    "SSL_VERSION",
    "SSL_SESSION_ID",
    "SSL_CIPHER",
    "SSL_CIPHER_USEKEYSIZE",
    "SSL_CIPHER_ALGKEYSIZE",
    "SSL_CLIENT_CERT",
    "GEOIP_ADDR",
    "PATH_TRANSLATED",

};


static int getVarNameStr(const char *name, unsigned int len)
{
    int ret = -1;
    for (int i = 0; i < LSI_VAR_COUNT; ++i)
    {
        if (strlen(lsi_req_env[i]) == len
            && strncasecmp(name, lsi_req_env[i], len) == 0)
        {
            ret = i;
            break;
        }
    }
    return ret;
}


static int get_req_var_by_id(const lsi_session_t *session, int type, char *val,
                             int maxValLen)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    int ret = -1;
    char *p = val;
    memset(val, 0, maxValLen);

    if (type <= LSI_VAR_HTTPS)
        ret = RequestVars::getReqVar(pSession,
                                     type - LSI_VAR_REMOTE_ADDR + REF_REMOTE_ADDR,
                                     p, maxValLen);
    else if (type < LSI_VAR_COUNT)
        ret = RequestVars::getReqVar2(pSession, type, p, maxValLen);
    else
        return LS_FAIL;

    if (p != val && ret > 0)
        memcpy(val, p, ret);

    if (ret < maxValLen && ret >= 0)
        val[ret] = 0;

    return ret;
}


static int  get_req_env(const lsi_session_t *session, const char *name,
                        unsigned int nameLen, char *val, int maxValLen)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    const char *p;
    int valLen = 0;
    int type = getVarNameStr(name, nameLen);
    if (type != -1)
        return get_req_var_by_id(session, type, val, maxValLen);
    else
    {
        p = RequestVars::getEnv(pSession, name, nameLen, valLen);
        if (!p || strlen(p) == 0)
        {
            p = getenv(name);
            if (p)
                valLen = strlen(p);
        }

        if (p)
        {
            if (valLen > maxValLen)
                valLen = maxValLen;
            memcpy(val, p, valLen);
        }
        return valLen;
    }
}


static void set_req_env(const lsi_session_t *session, const char *name,
                        unsigned int nameLen, const char *val, int valLen)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return;
//     assert( *(name + nameLen) == 0x00 );
//     assert( *(val + valLen) == 0x00 );
    RequestVars::setEnv(pSession, name, nameLen, val, valLen);
}


static int64_t get_req_content_length(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpReq *pReq = pSession->getReq();
    return pReq->getContentLength();
}


static int read_req_body(const lsi_session_t *session, char *buf, int bufLen)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpReq *pReq = pSession->getReq();
    size_t size;
    char *p;
    if (pReq == NULL)
        return LS_FAIL;

    if (pReq->getBodyBuf() == NULL)
        return LS_FAIL;

    p = pReq->getBodyBuf()->getReadBuffer(size);

    //connection closed? in reading?
    if (size == 0)
    {
        if (pReq->getBodyRemain() > 0)
            return LS_FAIL; //need wait and read again
    }

    if ((int)size > bufLen)
        size = bufLen;
    memcpy(buf, p, size);
    pReq->getBodyBuf()->readUsed(size);
    return size;
}


static int is_req_body_finished(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (!pSession || pSession->getFlag(HSF_REQ_BODY_DONE))
        return LS_TRUE;
    return LS_FALSE;
}


static int get_req_args_count(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (!pSession || pSession->getReqParser() == NULL)
        return 0;
    return pSession->getReqParser()->getArgCount();
}


static int get_req_arg_by_idx(const lsi_session_t *session, int index,
                             ls_strpair_t *pArg, char **filePath)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (!pSession || pSession->getReqParser() == NULL)
        return LS_FAIL;
    return pSession->getReqParser()->getArgByIndex(index, pArg, filePath);
}


static int get_qs_args_count(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (!pSession || pSession->getReqParser() == NULL)
        return 0;
    return pSession->getReqParser()->getQsArgCount();
}


static int get_qs_arg_by_idx(const lsi_session_t *session, int index,
                             ls_strpair_t *pArg)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (!pSession || pSession->getReqParser() == NULL)
        return LS_FAIL;
    return pSession->getReqParser()->getQsArgByIndex(index, pArg);
}


static int get_post_args_count(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (!pSession || pSession->getReqParser() == NULL)
        return 0;
    return pSession->getReqParser()->getArgCount();
}


static int get_post_arg_by_idx(const lsi_session_t *session, int index,
                             ls_strpair_t *pArg, char **filePath)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (!pSession || pSession->getReqParser() == NULL)
        return LS_FAIL;
    return pSession->getReqParser()->getArgByIndex(index, pArg, filePath);
}


static int is_post_file_upload(const lsi_session_t *session, int index)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (!pSession || pSession->getReqParser() == NULL)
        return LS_FAIL;
    return pSession->getReqParser()->isFile(index);
}


static int set_req_wait_full_body(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    if (pSession->getFlag(HSF_REQ_BODY_DONE))
        return LS_OK;
    log(session, LSI_LOG_DEBUG, "set_req_wait_full_body called\n");
    pSession->setFlag(HSF_REQ_WAIT_FULL_BODY);
    return LS_OK;
}


static int parse_req_args(const lsi_session_t *session, int parse_req_body,
                          int uploadPassByPath, const char *uploadTmpDir,
                          int uploadTmpFilePermission)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    return pSession->parseReqArgs(parse_req_body, uploadPassByPath,
                                  uploadTmpDir, uploadTmpFilePermission);
}


static int set_resp_wait_full_body(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    pSession->setFlag(HSF_RESP_WAIT_FULL_BODY);
    return LS_OK;
}


static int is_resp_buffer_available(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FALSE;

    if (pSession->getRespBodyBuf()
        && pSession->getRespBodyBuf()->getCurWBlkPos() -
        pSession->getRespBodyBuf()->getCurRBlkPos() > LSI_MAX_RESP_BUFFER_SIZE)
        return LS_FALSE;
    return LS_TRUE;
}


static int get_resp_buffer_compress_method(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FALSE;

    if (pSession->getFlag(HSF_RESP_BODY_BRCOMPRESSED))
        return LSI_BR_COMPRESS;
    else if (pSession->getFlag(HSF_RESP_BODY_GZIPCOMPRESSED))
        return LSI_GZIP_COMPRESS;
    return LSI_NO_COMPRESS;//0
}


static int set_resp_buffer_compress_method(const lsi_session_t *session, int method)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    pSession->clearFlag(HSF_RESP_BODY_BRCOMPRESSED);
    pSession->clearFlag(HSF_RESP_BODY_GZIPCOMPRESSED);
    if (method == LSI_BR_COMPRESS)
        pSession->setFlag(HSF_RESP_BODY_BRCOMPRESSED);
    else if (method == LSI_GZIP_COMPRESS)
        pSession->setFlag(HSF_RESP_BODY_GZIPCOMPRESSED);

    return LS_OK;
}


//return 0 is OK, -1 error
static int append_resp_body(const lsi_session_t *session, const char *buf,
                            int len)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    if (pSession->isEndResponse())
        return LS_FAIL;

    return pSession->appendDynBody(buf, len);
}


//return 0 is OK, -1 error
static int append_resp_bodyv(const lsi_session_t *session,
                             const struct iovec *vector, int count)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    if (pSession->isEndResponse())
        return LS_FAIL;

    //TODO: should use the below vector
    //return pSession->appendRespBodyBufV(vector, count);
    //Currently, just use the appendDynBody for need to use the hook filters
    //THIS SHOULD BE UPDATED!!!
    bool error = 0;
    for (int i = 0; i < count; ++i)
    {
        if (pSession->appendDynBody((char *)((*vector).iov_base),
                                    (*vector).iov_len) <= 0)
        {
            error = 1;
            break;
        }
        ++vector;
    }

    return error;
}


static int init_file_type_mdata(const lsi_session_t *session,
                                const lsi_module_t *pModule, const char *path,
                                int pathLen)
{
    StaticFileCacheData *pCacheData;
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    int ret = pSession->initSendFileInfo(path, strlen(path));
    if (ret)
        return LS_FAIL;

    pCacheData = pSession->getSendFileInfo()->getFileData();
    if (pCacheData == NULL)
        return LS_FAIL;
    return dup(pCacheData->getFileData()->getfd());
}


static int send_file(const lsi_session_t *session, const char *path,
                     int64_t start, int64_t size)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    if (pSession->isEndResponse())
        return LS_FAIL;

    int ret = pSession->initSendFileInfo(path, strlen(path));
    if (ret)
        return ret;

    pSession->setSendFileOffsetSize(start, size);
    pSession->flush();
    return LS_OK;
}


static int send_file2(const lsi_session_t *session, int fd,
                      int64_t start, int64_t size)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    if (pSession->isEndResponse())
        return LS_FAIL;


    pSession->setSendFileOffsetSize(fd, start, size);
    pSession->flush();
    return LS_OK;
}


static int lsi_flush(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    pSession->setFlag(HSF_RESP_FLUSHED, 0);
    return pSession->flush();
}


static void lsi_end_resp(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return;
    pSession->endResponse(1);
}


static int set_uri_qs(const lsi_session_t *session, int action, const char *uri,
                      int uri_len, const char *qs, int qs_len)
{

    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    if (!action)
        return LS_OK;
    return pSession->setUriQueryString(action, uri, uri_len, qs, qs_len);
}


static const char   *get_server_root()
{
    return MainServerConfig::getInstance().getServerRoot();
}


static void *get_config(const lsi_session_t *session,
                        const lsi_module_t *pModule)
{
    ModuleConfig *pConfig = NULL;
    if (session)
        pConfig = ((LsiSession *)session)->getModuleConfig();
    else
        pConfig = ModuleManager::getInstance().getGlobalModuleConfig();

    if (!pConfig)
        return NULL;

    return pConfig->get(MODULE_ID(pModule))->config;

}


static void *lsiapi_get_multiplexer()
{   return MultiplexerFactory::getMultiplexer();   }


static ls_edio_t *edio_reg(int fd, edio_evt_cb evt_cb,
                           edio_timer_cb timer_cb, short events, void *pParam)
{
    EdioHandler *pHandler = new EdioHandler(fd, pParam, evt_cb, timer_cb);
    if (!pHandler)
        return NULL;
    MultiplexerFactory::getMultiplexer()->add(pHandler, events);
    return pHandler;
}


static void edio_remove(ls_edio_t *pHandle)
{
    if (!pHandle)
        return;
    EdioHandler *pHandler = (EdioHandler *)pHandle;
    MultiplexerFactory::getMultiplexer()->remove(pHandler);
    delete pHandler;
}


static void edio_modify(ls_edio_t *pHandle, short events, int add_remove)
{
    if (!pHandle)
        return;
    EdioHandler *pHandler = (EdioHandler *)pHandle;
    events &= (POLLIN | POLLOUT);
    if (events != 0)
        MultiplexerFactory::getMultiplexer()->modEvent(pHandler, events,
                add_remove);
}


static int lsiapi_is_suspended(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    return pSession->getFlag(HSF_SUSPENDED);
}


static int lsiapi_resume(const lsi_session_t *session, int retcode)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    pSession->resumeProcess(0, retcode);
    return LS_OK;
}



static int exec_ext_cmd(const lsi_session_t *session, const char *cmd, int len,
                        evtcb_pf cb, const long lParam, void *pParam)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    pSession->setExtCmdNotifier(cb, lParam, pParam);
    pSession->execExtCmd(cmd, len, EXEC_CMD_PARSE_RES);
    return LS_OK;
}


static int exec_ext_cmd_ts(const lsi_session_t *session, const char *cmd, int len,
                        evtcb_pf cb, const long lParam, void *pParam)
{
    assert(0); // not supported
    return LS_FAIL;
}


static char *get_ext_cmd_res_buf(const lsi_session_t *session, int *length)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return NULL;

    *length = pSession->getExtCmdBuf().size();
    return pSession->getExtCmdBuf().begin();
}

static int get_client_access_level(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FALSE;
    return pSession->getClientInfo()->getAccess();
}


static int get_file_path_by_uri(const lsi_session_t *session, const char *uri,
                                int uri_len, char *path, int max_len)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    return pSession->getReq()->translatePath(uri, uri_len, path, max_len);
}


// static int set_static_file_by_uri(lsi_session_t *session, const char * uri,
//                                   int uri_len)
// {
//     HttpSession *pSession = (HttpSession *)((LsiSession *)session);
//     if (pSession == NULL)
//         return LS_FAIL;
// //     int HttpReq::translatePath( const char * pURI, int uriLen,
// //                                 char * pReal, int len ) const
// //     int ret = pSession->initSendFileInfo(path, strlen(path));
// //     if (ret)
// //        return LS_FAIL;
//
// }

// static const struct stat * get_static_file_stat( lsi_session_t *session )
// {
//     HttpSession *pSession = (HttpSession *)((LsiSession *)session);
//     if (pSession == NULL)
//         return NULL;
//     SendFileInfo * pInfo = pSession->getSendFileInfo();
//     pInfo->getFileData();
// }


static const char *get_mime_type_by_suffix(const lsi_session_t *session,
        const char *suffix)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return NULL;
    return pSession->getReq()->getMimeBySuffix(suffix);
}


static int set_force_mime_type(const lsi_session_t *session, const char *mime)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL || !mime)
        return LS_FAIL;
    pSession->getReq()->setForcedType(mime);
    return LS_OK;
}


static const char *get_req_handler_type(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL || pSession->getCurHandler() == NULL)
        return NULL;
    if (pSession->getCurHandler() == NULL)
        return NULL;
    return HandlerType::getHandlerTypeString(
               pSession->getCurHandler()->getType());
}


static int get_file_stat(const lsi_session_t *session, const char *path,
                         int pathLen, struct stat *st)
{
    if (!path || !st)
        return LS_FAIL;
    //TODO: use static file cache to serve file stats
    if (session)
    {

    }
    return stat(path, st);
}


static const char *get_req_file_path(const lsi_session_t *session, int *pathLen)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return NULL;
    const AutoStr2 *pPath = pSession->getReq()->getRealPath();
    if (!pPath)
    {
        if (pathLen)
            *pathLen = 0;
        return NULL;
    }
    else
    {
        if (pathLen)
            *pathLen = pPath->len();
        return pPath->c_str();
    }
}


static int  is_access_log_on(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    return !pSession->getFlag(HSF_ACCESS_LOG_OFF);
}


static void set_access_log(const lsi_session_t *session, int enable)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return;
    pSession->setFlag(HSF_ACCESS_LOG_OFF, !enable);

}


static int  get_access_log_string(const lsi_session_t *session,
                                  const char *log_pattern, char *buf,
                                  int bufLen)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if ((pSession == NULL) || (!log_pattern) || (!buf))
        return LS_FAIL;

    int ret = AccessLog::getLogString(pSession, log_pattern, buf, bufLen);
    return ret;
}


static const char *get_module_name(const lsi_module_t *module)
{
    return MODULE_NAME(module);
}


static int is_resp_handler_aborted(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    //return pSession->isRespHandlerAborted();
    return LS_OK;
}


static void *get_resp_body_buf(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return NULL;
    return pSession->getRespBodyBuf();
}


static void *get_req_body_buf(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return NULL;
    return pSession->getReq()->getBodyBuf();
}


static void *get_new_body_buf(int64_t iInitialSize)
{
    VMemBuf *pBuf = HttpResourceManager::getInstance().getVMemBuf();
    pBuf->reinit(iInitialSize);
    return pBuf;
}


static int64_t get_body_buf_size(void *pBuf)
{
    if (!pBuf)
        return LS_FAIL;
    return ((VMemBuf *)pBuf)->getCurWOffset();
}


static int is_body_buf_eof(void *pBuf, int64_t offset)
{
    if (!pBuf)
        return 1;
    return ((VMemBuf *)pBuf)->eof(offset);
}


static const char *acquire_body_buf_block(void *pBuf, int64_t offset,
        int  *size)
{
    if (!pBuf)
        return NULL;
    return ((VMemBuf *)pBuf)->acquireBlockBuf(offset, size);

}


static void  release_body_buf_block(void *pBuf, int64_t offset)
{
    if (!pBuf)
        return;
    ((VMemBuf *)pBuf)->releaseBlockBuf(offset);
}


static int   get_body_buf_fd(void *pBuf)
{
    if (!pBuf)
        return LS_FAIL;
    return ((VMemBuf *)pBuf)->getfd();

}


static void  reset_body_buf(void *pBuf, int iWriteFlag)
{
    if (pBuf)
    {
        ((VMemBuf *)pBuf)->rewindReadBuf();
        if (iWriteFlag)
            ((VMemBuf *)pBuf)->rewindWriteBuf();
    }
}


static int   append_body_buf(void *pBuf, const char *pBlock, int size)
{
    if (!pBuf || !pBlock || size < 0)
        return LS_FAIL;
    return ((VMemBuf *)pBuf)->write(pBlock, size);

}


static int set_req_body_buf(const lsi_session_t *session, void *pBuf)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (!pBuf)
        return LS_FAIL;
    if (!pSession->getFlag(HSF_REQ_BODY_DONE))
        return LS_FAIL;
    pSession->getReq()->replaceBodyBuf((VMemBuf *)pBuf);
    return LS_OK;
}


static const lsi_module_t *get_module(lsi_param_t *param)
{
    return ((lsiapi_hook_t *)param->cur_hook)->module;
}


static time_t get_cur_time(int32_t *usec)
{
    if (usec)
        *usec = DateTime::s_curTimeUs;
    return DateTime::s_curTime;
}


static int get_vhost_count()
{
    return HttpServer::getInstance().getVHostCounts();
}


const void *get_vhost(int index)
{
    return (const void *)HttpServer::getInstance().getVHost(index);
}


//a special case of set_module_data
static int set_vhost_module_data(const void *vhost,
                                 const lsi_module_t *pModule, void *data)
{
    int iCount, ret = LS_FAIL;
    LsiModuleData *pData = ((HttpVHost *)vhost)->getModuleData();

    if (pData)
    {
        if (!pData->isDataInited())
        {
            if (data)
            {
                iCount = ModuleManager::getInstance().getModuleDataCount(
                             LSI_DATA_VHOST);
                pData->initData(iCount);
            }
            else
                return LS_OK;
        }
        pData->set(MODULE_DATA_ID(pModule)[LSI_DATA_VHOST], data);
        ret = LS_OK;
    }

    return ret;
}


static void *get_vhost_module_data(const void *vhost,
                                   const lsi_module_t *pModule)
{
    LsiModuleData *pData = ((HttpVHost *)vhost)->getModuleData();
    if (!pData)
        return NULL;
    return pData->get(MODULE_DATA_ID(pModule)[LSI_DATA_VHOST]);
}


static void *get_vhost_module_conf(const void *vhost,
                                    const lsi_module_t *pModule)
{
    if (vhost == NULL)
        return NULL;

    ModuleConfig *pConfig = ((HttpVHost *)
                             vhost)->getRootContext().getModuleConfig();
    if (!pConfig)
        return NULL;

    return pConfig->get(MODULE_ID(pModule))->config;
}

//Do not use it currently
int handoff_fd(const lsi_session_t *session, char **pData, int *pDataLen)
{
//     if (!session || !pData || !pDataLen)
//         return LS_FAIL;
//     HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    return LS_FAIL;//pSession->handoff(pData, pDataLen);
}


int get_local_sockaddr(const lsi_session_t *session, char *pIp, int maxLen)
{
    if (!session || !pIp)
        return LS_FAIL;
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    return pSession->getServerAddrStr(pIp, maxLen);
}


ls_xpool_t *get_session_pool(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    return pSession->getReq()->getPool();
}


int expand_current_server_variable(int level, const char *pVariable,
                                  char *buf, int maxLen)
{
    int ret = LS_FAIL;
    switch (level)
    {
    case LSI_CFG_SERVER:
    case LSI_CFG_LISTENER:
    case LSI_CFG_VHOST:
        ret = ConfigCtx::getCurConfigCtx()->expandVariable(pVariable, buf,
                maxLen, 1);
        break;

    case LSI_CFG_CONTEXT:
    default:
        break;
    }

    return ret;
}

ls_hash_key_t _ua_hash(const void *p)
{
    return XXH64(p, strlen((const char *)p), 0x14567890);
}


typedef struct uacode_item_st
{
    char *ua;
    char *code;
} uacode_item;

/* client */
/* certs is an array of ls_str_t * */
uacode_item *make_uacode_item(const char *ua, char *code)
{
    uacode_item *item = new uacode_item;
    item->ua = strdup(ua);
    item->code = strdup(code);
    return item;
}

void free_uacode_item(uacode_item *item)
{
    if (item)
    {
        free(item->ua);
        free(item->code);
        delete item;
    }
}


static ls_hash_t *s_uacode_hasht = NULL;
void init_uacode_hasht()
{
    if (!s_uacode_hasht)
    {
        s_uacode_hasht = ls_hash_new(100, _ua_hash, (ls_hash_keycmp_ne)strcmp,
                                     NULL);
        assert(s_uacode_hasht);
    }
}



int set_uacode(const char *ua, char* code)
{
    init_uacode_hasht();
    ls_hash_iter it = ls_hash_find(s_uacode_hasht, ua);
    if (it == NULL)
    {
        uacode_item *item = make_uacode_item(ua, code);
        if (ls_hash_insert(s_uacode_hasht, item->ua, item) == NULL)
            return -1;
        return 0;
    }

    //If exist, do nothing
    return 0;
}

char *get_uacode(const char *ua)
{
    init_uacode_hasht();
    ls_hash_iter it = ls_hash_find(s_uacode_hasht, ua);
    if (it == NULL)
        return NULL;

    uacode_item *item = (uacode_item *)ls_hash_getdata(it);
    assert (item);
    return item->code;
}

//Atexit, free thr hashTable
void freeUaCodeT()
{
    if (s_uacode_hasht)
    {
        ls_hash_iter it ;
        for (it = ls_hash_begin(s_uacode_hasht);
             it != ls_hash_end(s_uacode_hasht);
             it = ls_hash_next(s_uacode_hasht, it))
        {
            uacode_item *item = (uacode_item *)ls_hash_getdata(it);
            free_uacode_item(item);
        }
        ls_hash_delete(s_uacode_hasht);
        s_uacode_hasht = NULL;
    }
}


static void foreach_req_header(const HttpSession *session, const char *filter,
                               lsi_foreach_cb cb, void *arg)
{
    LSI_DBGH(session, "enter %s: filter %s, cb %p, arg %p\n",
                              __func__, filter, cb, arg);
    const HttpReq *pReq = session->getReq();
    int i, n;
    const char *pTemp;
    FilterMatch fm(filter);

    for (i = 0; i <= HttpHeader::H_TRANSFER_ENCODING; ++i)
    {
        pTemp = pReq->getHeader(i);
        if (*pTemp)
        {
            if (!filter || fm.match(RequestVars::getHeaderString(i),
                         HttpHeader::getHeaderStringLen(i)))
            {
                cb(i, RequestVars::getHeaderString(i),
                   HttpHeader::getHeaderStringLen(i),
                   pTemp, pReq->getHeaderLen(i), arg);
            }
        }
    }

    n = pReq->getUnknownHeaderCount();
    for (i = 0; i < n; ++i)
    {
        const char *pKey;
        const char *pVal;
        int keyLen;
        int valLen;
        pKey = pReq->getUnknownHeaderByIndex(i, keyLen, pVal, valLen);
        if (pKey)
        {
            // if we check both key and val, change the conditional
            // per negation. can be removed if we only check one of the two
            bool match = !filter || fm.isNegated()
                ? (fm.match(pKey, keyLen) && fm.match(pVal, valLen))
                : (fm.match(pKey, keyLen) || fm.match(pVal, valLen));
            if (match)
            {
                cb(-1, pKey, keyLen, pVal, valLen, arg);
            }
        }
    }
}


static void foreach_special_env(const HttpSession *session, const char *filter,
                                lsi_foreach_cb cb, void *arg)
{
    LSI_DBGH(session, "enter %s: filter %s, cb %p, arg %p\n",
                              __func__, filter, cb, arg);
    PHPConfig *pConfig = session->getReq()->getContext()->getPHPConfig();
    if (!pConfig)
        return;
    FilterMatch fm(filter);
    PHPConfig::iterator iter;
    const char *pKey;
    const char *pVal;
    int keyLen;
    int valLen;
    for(iter = pConfig->begin(); iter!= pConfig->end(); pConfig->next(iter))
    {
        PHPValue * phpval = iter.second();
        pKey = phpval->getKey();
        keyLen = phpval->getKeyLen();
        pVal = phpval->getValue();
        valLen = phpval->getValLen();
        // if we check both key and val, change the conditional
        // per negation. can be removed if we only check one of the two
        bool match = !filter || fm.isNegated()
            ? (fm.match(pKey, keyLen) && fm.match(pVal, valLen))
            : (fm.match(pKey, keyLen) || fm.match(pVal, valLen));
        if (match)
        {
            cb(-1, pKey, keyLen, pVal, valLen, arg);
        }
    }
}


static void foreach_ssl_env(HttpSession *pHttpSession, lsi_foreach_cb cb,
                            void *arg)
{
    char buf[128];
    char *pBuf;
    int n;
    HioCrypto *pCrypto = pHttpSession->getCrypto();
    if (!pCrypto)
        return;

    pBuf = buf;
    n = pCrypto->getEnv(HioCrypto::CRYPTO_VERSION, pBuf, 128);
    cb(-1, "SSL_VERSION", 11, pBuf, n, arg);

    pBuf = buf;
    n = pCrypto->getEnv(HioCrypto::SESSION_ID, pBuf, 128);
    cb(-1, "SSL_SESSION_ID", 14, buf, n, arg);

    pBuf = buf;
    n = pCrypto->getEnv(HioCrypto::CIPHER, pBuf, 128);
    cb(-1, "SSL_CIPHER", 10, buf, n, arg);

    pBuf = buf;
    n = pCrypto->getEnv(HioCrypto::CIPHER_USEKEYSIZE, pBuf, 128);
    cb(-1, "SSL_CIPHER_USEKEYSIZE", 21, buf, n, arg);

    pBuf = buf;
    n = pCrypto->getEnv(HioCrypto::CIPHER_USEKEYSIZE, pBuf, 128);
    cb(-1, "SSL_CIPHER_ALGKEYSIZE", 21, buf, n, arg);

    int i = pCrypto->getVerifyMode();
    if (i != 0)
    {
        char achBuf[4096];
        X509 *pClientCert = pCrypto->getPeerCertificate();
        if (pCrypto->isVerifyOk())
        {
            if (pClientCert)
            {
                //IMPROVE: too many deep copy here.
                //n = SslCert::PEMWriteCert( pClientCert, achBuf, 4096 );
                //if ((n>0)&&( n <= 4096 ))
                //{
                //    cb(-1,  "SSL_CLIENT_CERT", 15, achBuf, n );
                //    ++count;
                //}
                n = snprintf(achBuf, sizeof(achBuf), "%lu",
                                X509_get_version(pClientCert) + 1);
                cb(-1, "SSL_CLIENT_M_VERSION", 20, achBuf, n, arg);
                n = SslUtil::lookupCertSerial(pClientCert, achBuf, 4096);
                if (n != -1)
                {
                    cb(-1, "SSL_CLIENT_M_SERIAL", 19, achBuf, n, arg);
                }
                X509_NAME_oneline(X509_get_subject_name(pClientCert), achBuf, 4096);
                cb(-1, "SSL_CLIENT_S_DN", 15, achBuf, strlen(achBuf), arg);
                X509_NAME_oneline(X509_get_issuer_name(pClientCert), achBuf, 4096);
                cb(-1, "SSL_CLIENT_I_DN", 15, achBuf, strlen(achBuf), arg);
                if (SslConnection::isClientVerifyOptional(i))
                {
                    lstrncpy(achBuf, "GENEROUS", sizeof(achBuf));
                    n = 8;
                }
                else
                {
                    lstrncpy(achBuf, "SUCCESS", sizeof(achBuf));
                    n = 7;
                }
            }
            else
            {
                lstrncpy(achBuf, "NONE", sizeof(achBuf));
                n = 4;
            }
        }
        else
            n = pCrypto->buildVerifyErrorString(achBuf, sizeof(achBuf));
        cb(-1, "SSL_CLIENT_VERIFY", 17, achBuf, n, arg);
    }
}


static void foreach_req_var_full(HttpSession *pSession,
                                 lsi_foreach_cb cb, void *arg)
{
    HttpReq *pReq = pSession->getReq();
    const char *pTemp;
    int n;
    char buf[128];
    //RadixNode *pNode;

    pTemp = pReq->getAuthUser();
    if (pTemp)
    {
        //NOTE: only Basic is support now
        cb(-1, "AUTH_TYPE", 9, "Basic", 5, arg);
        cb(-1, "REMOTE_USER", 11, pTemp, strlen(pTemp), arg);
    }
    const AutoStr2 *pDocRoot = pReq->getDocRoot();
    cb(-1, "DOCUMENT_ROOT", 13,
              pDocRoot->c_str(), pDocRoot->len() - 1, arg);
    cb(-1, "REMOTE_ADDR", 11, pSession->getPeerAddrString(),
              pSession->getPeerAddrStrLen(), arg);

    n = ls_snprintf(buf, 10, "%hu", pSession->getRemotePort());
    cb(-1, "REMOTE_PORT", 11, buf, n, arg);

    n = pSession->getServerAddrStr(buf, 128);

    cb(-1, "SERVER_ADDR", 11, buf, n, arg);

    cb(-1, "SERVER_NAME", 11, pReq->getHostStr(),  pReq->getHostStrLen(), arg);
    const AutoStr2 &sPort = pReq->getPortStr();
    cb(-1, "SERVER_PORT", 11, sPort.c_str(), sPort.len(), arg);
    cb(-1, "REQUEST_URI", 11, pReq->getOrgReqURL(),
              pReq->getOrgReqURLLen(), arg);

    n = pReq->getPathInfoLen();
    if (n > 0)
    {
        int m;
        char achTranslated[10240];
        m =  pReq->translatePath(pReq->getPathInfo(), n,
                                 achTranslated, sizeof(achTranslated));
        if (m != -1)
        {
            cb(-1, "PATH_TRANSLATED", 15, achTranslated, m, arg);
        }
        cb(-1, "PATH_INFO", 9, pReq->getPathInfo(), n, arg);

        if (pReq->getRedirects() > 0)
        {
            cb(-1, "ORIG_PATH_INFO", 14, pReq->getPathInfo(), n, arg);
        }
    }

    if (pReq->getRedirects() > 0)
    {
        pTemp = pReq->getRedirectURL(n);
        if (pTemp && (n > 0))
        {
            cb(-1, "REDIRECT_URL", 12, pTemp, n, arg);
        }
        pTemp = pReq->getRedirectQS(n);
        if (pTemp && (n > 0))
        {
            cb(-1, "REDIRECT_QUERY_STRING", 21, pTemp, n, arg);
        }
    }

    //add geo IP env here
    if (pReq->isGeoIpOn())
    {
        GeoInfo *pInfo = pSession->getClientInfo()->getGeoInfo();
        if (pInfo)
        {
            cb(-1, "GEOIP_ADDR", 10, pSession->getPeerAddrString(),
                      pSession->getPeerAddrStrLen(), arg);
            //FIXME: go through GEO ENV
            //count += pInfo->addGeoEnv(pEnv) + 1;
        }
    }
#ifdef USE_IP2LOCATION
    //add IP2Location env here
    if (pReq->isIpToLocOn())
    {
        LocInfo *pInfo = pSession->getClientInfo()->getLocInfo();
        if (pInfo)
        {
            cb(-1, "IP2LOCATION_ADDR", 16, pSession->getPeerAddrString(),
                      pSession->getPeerAddrStrLen(), arg);
            //FIXME: go through GEO ENV
            //count += pInfo->addLocEnv(pEnv) + 1;
        }
    }
#endif
    if (pSession->getStream()->isSpdy())
    {
        const char *pProto = HioStream::getProtocolName((HiosProtocol)
                             pSession->getStream()->getProtocol());
        cb(-1, "X_SPDY", 6, pProto, strlen(pProto), arg);
    }

    if (pSession->isHttps())
        foreach_ssl_env(pSession, cb, arg);

    char sVer[40];
    n = snprintf(sVer, 40, "Openlitespeed %s", PACKAGE_VERSION);
    cb(-1, "LSWS_EDITION", 12, sVer, n, arg);
    cb(-1, "X-LSCACHE", 9, "1", 1, arg);

    const AutoStr2 *psTemp = pReq->getRealPath();
    if (psTemp)
    {
        cb(-1, "SCRIPT_FILENAME", 15, psTemp->c_str(), psTemp->len(), arg);
    }
    cb(-1, "QUERY_STRING", 12, pReq->getQueryString(),
              pReq->getQueryStringLen(), arg);
    pTemp = pReq->getURI();
    cb(-1, "SCRIPT_NAME", 11, pTemp, pReq->getScriptNameLen(), arg);
    n = pReq->getVersion();
    cb(-1, "SERVER_PROTOCOL", 15, HttpVer::getVersionString(n),
              HttpVer::getVersionStringLen(n), arg);
    cb(-1, "SERVER_SOFTWARE", 15, HttpServerVersion::getVersion(),
              HttpServerVersion::getVersionLen(), arg);
    n = pReq->getMethod();
    cb(-1, "REQUEST_METHOD", 14, HttpMethod::get(n),
              HttpMethod::getLen(n), arg);
}


// does value only filtering - will have to add string array if
// we want key filtering as well
static void foreach_req_var(const HttpSession *session, const char *filter,
                                lsi_foreach_cb cb, void *arg)
{
    LSI_DBGH(session, "enter %s: filter %s, cb %p, arg %p\n",
                              __func__, filter, cb, arg);
    int i;
#define VALMAXSIZE 4096
    char val[VALMAXSIZE];
    int ret = -1;
    char *p = val;
    FilterMatch fm(filter);

    if (session == NULL)
        return;

    if (filter == NULL)
    {
        foreach_req_var_full(const_cast<HttpSession *>(session), cb, arg);
        return;
    }

    for (i = LSI_VAR_REMOTE_ADDR; i < LSI_VAR_COUNT; ++i)
    {
        p = val;
        memset(val, 0, VALMAXSIZE);
        ret = (i < LSI_VAR_HTTPS)
            ? RequestVars::getReqVar(const_cast<HttpSession *>(session),
                                     i - LSI_VAR_REMOTE_ADDR + REF_REMOTE_ADDR,
                                     p, VALMAXSIZE)
            : RequestVars::getReqVar2(const_cast<HttpSession *>(session), i, p, VALMAXSIZE);

        if (ret >= VALMAXSIZE || ret <= 0)
            continue;

        if (p == val)
        {
            val[ret] = 0;
        }
        if (!filter || fm.match(p, ret))
        {
            //TODO add the keys - make up strings?
            cb(i, NULL, 0, p, ret, arg);
        }
    }
}


static void foreach_req_cookie(const HttpSession *session, const char *filter,
                                lsi_foreach_cb cb, void *arg)
{
    LSI_DBGH(session, "enter %s: filter %s, cb %p, arg %p\n",
                              __func__, filter, cb, arg);
    if (session == NULL)
        return;

    FilterMatch fm(filter);
    ls_strpair_t cookie;

    const HttpReq *pReq = session->getReq();
    if (NULL == pReq)
        return;

    const_cast<HttpReq *>(pReq)->parseCookies();
    const CookieList &list = const_cast<HttpReq *>(pReq)->getCookieList();

    for (int index = 0; index < list.getSize(); ++index)
    {
        const cookieval_t val = list.begin()[index];
        cookie.key.ptr = const_cast<HttpReq *>(pReq)->getHeaderBuf().getp(val.keyOff);
        cookie.key.len = val.keyLen;
        cookie.val.ptr = const_cast<HttpReq *>(pReq)->getHeaderBuf().getp(val.valOff);
        cookie.val.len = val.valLen;
        bool match = !filter || fm.isNegated() ?
            (fm.match(cookie.key.ptr, cookie.key.len) && fm.match(cookie.val.ptr, cookie.val.len)) :
            (fm.match(cookie.key.ptr, cookie.key.len) || fm.match(cookie.val.ptr, cookie.val.len));
        if (match)
        {
            cb(index, cookie.key.ptr, cookie.key.len, cookie.val.ptr, cookie.val.len, arg);
        }
    }
}


struct cb_arg_s
{
    lsi_foreach_cb cb;
    void *arg;
    const char * filter;
};


static int req_env_cb(void *pObj, void *pUData, const char *pKey, int iKeyLen)
{
    cb_arg_s *arg = (cb_arg_s *)pUData;
    ls_strpair_t *pPair = (ls_strpair_t *)pObj;

    FilterMatch fm(arg->filter);

    const char *key = ls_str_cstr(&pPair->key),
          *val = ls_str_cstr(&pPair->val);
    size_t keyLen = ls_str_len(&pPair->key),
           valLen = ls_str_len(&pPair->val);

    bool match = !arg->filter || fm.isNegated() ?
        (fm.match(key, keyLen) && fm.match(val, valLen)) :
        (fm.match(key, keyLen) || fm.match(val, valLen));
    if (match)
    {
        arg->cb(-1, key, keyLen, val, valLen, arg->arg);
    }
    return 0;
}

static void foreach_req_env(const HttpSession *session, const char *filter,
                            lsi_foreach_cb cb, void *arg)
{
    LSI_DBGH(session, "enter %s: filter %s, cb %p, arg %p\n",
                              __func__, filter, cb, arg);
    RadixNode *pNode;

    if ((pNode = (RadixNode *)session->getReq()->getEnvNode()) != NULL)
    {
        struct cb_arg_s new_arg = { cb, arg, filter };
        pNode->for_each2(req_env_cb, &new_arg);
    }
}

static void foreach_req_cgi_header(const HttpSession *session, const char *filter,
                                   lsi_foreach_cb cb, void *arg)
{
    LSI_DBGH(session, "enter %s: filter %s, cb %p, arg %p\n",
                              __func__, filter, cb, arg);
    LsiApiConst lsiApiConst;
    int headers = lsiApiConst.get_cgi_header_count();
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    FilterMatch fm(filter);

    if (session == NULL)
        return;

    HttpReq *pReq = pSession->getReq();
    if (!pReq)
        return;

    for (int i = 0; i < headers; ++i)
    {
        const char *val = pReq->getHeader(i);
        if (*val)
        {
            const char *key = lsiApiConst.get_cgi_header(i);
            const int  key_len = lsiApiConst.get_cgi_header_len(i);
            int val_len = pReq->getHeaderLen(i);
            //Note: WARNING: web server does not send authorization info to cgi
            //for security reasons
            //pass AUTHORIZATION header only when server does not check it.
            if ((i == HttpHeader::H_AUTHORIZATION)
                && (pReq->getAuthUser()))
                continue;
            if (filter)
            {
                bool match = fm.isNegated() ?
                             (fm.match(key, key_len) && fm.match(val, val_len)) :
                             (fm.match(key, key_len) || fm.match(val, val_len));
                if (!match)
                    continue;
            }

            cb(i, key, key_len, val, val_len, arg);
        }
    }
}

static void foreach_resp_header(const HttpSession *session, const char *filter,
                                   lsi_foreach_cb cb, void *arg)
{
    LSI_DBGH(session, "enter %s: filter %s, cb %p, arg %p\n",
                              __func__, filter, cb, arg);
    if (session == NULL)
        return;

    FilterMatch fm(filter);

    // have it work for both MT and normal handlers
    ls_mutex_t dummyLock;
    ls_mutex_setup(&dummyLock);
    ls_mutex_t * ptr =  session->getMtFlag(HSF_MT_HANDLER) ?
        &const_cast<HttpSession *>(session)->getMtSessData()->m_respHeaderLock : &dummyLock;
    MtLock lk(*ptr);
    HttpRespHeaders &respHeaders = const_cast<HttpSession *>(session)->getResp()->getRespHeaders();
    int count = respHeaders.getCount();
    struct iovec keys[count], vals[count];
    int found = respHeaders.getAllHeaders(keys, vals, count);
    LSI_DBGH(session, "foreach_resp_header: count %d, found %d\n", count, found);
    for (int i = 0; i < found; i++)
    {
        const char * pKey = (const char *) keys[i].iov_base;
        int keyLen = keys[i].iov_len;
        const char * pVal = (const char *) vals[i].iov_base;
        int valLen = vals[i].iov_len;
        bool match = !filter || fm.isNegated()
            ? (fm.match(pKey, keyLen) && fm.match(pVal, valLen))
            : (fm.match(pKey, keyLen) || fm.match(pVal, valLen));
        if (match)
        {
            cb(i, pKey, keyLen, pVal, valLen, arg);
        }
    }
}

static void lsi_foreach(const lsi_session_t *session, LSI_DATA_TYPE type,
                        const char *filter, lsi_foreach_cb cb, void *arg)
{
    LSI_DBGH(session, "enter %s: type %d, filter %s, cb %p, arg %p\n",
                              __func__, type, filter, cb, arg);
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (!pSession)
        return;
    switch(type)
    {
    case LSI_DATA_REQ_HEADER:
        foreach_req_header(pSession, filter, cb, arg);
        break;
    case LSI_DATA_REQ_VAR:
        foreach_req_var(pSession, filter, cb, arg);
        break;
    case LSI_DATA_REQ_ENV:
        foreach_req_env(pSession, filter, cb, arg);
        break;
    case LSI_DATA_REQ_SPECIAL_ENV:
        foreach_special_env(pSession, filter, cb, arg);
        break;
    case LSI_DATA_REQ_CGI_HEADER:
        foreach_req_cgi_header(pSession, filter, cb, arg);
        break;
    case LSI_DATA_REQ_COOKIE:
        foreach_req_cookie(pSession, filter, cb, arg);
        break;
    case LSI_DATA_RESP_HEADER:
        foreach_resp_header(pSession, filter, cb, arg);
        break;
    }
}

void register_thread_cleanup(const lsi_module_t *pModule, void (*routine)(void *), void * arg)
{
}


void register_thread_cleanup_ts(const lsi_module_t *pModule, void (*routine)(void *), void * arg)
{
    if (!pModule
        || !pModule->reqhandler
        || LSI_HDLR_DEFAULT_POOL != pModule->reqhandler->ts_hdlr_ctx)
    {
        return;
    }
    WorkCrew * pWC = ModuleHandler::getGlobalWorkCrew();
    if (!pWC)
    {
        LSI_ERR(NULL, "Global work crew pointer is NULL!\n");
        return;
    }
    pWC->pushCleanup(routine, arg);
}

static time_t get_cur_time_ts(int32_t *usec)
{
    LS_TH_IGN_RD_BEG();
    if (usec)
        *usec = ls_atomic_fetch_add(&DateTime::s_curTimeUs, 0);
    time_t now = ls_atomic_fetch_add(&DateTime::s_curTime, 0);
    LS_TH_IGN_RD_END();
    return now;
}


static int remove_resp_header_ts(const lsi_session_t *session,
                              unsigned int header_index, const char *name,
                              int nameLen)
{
    LSI_DBGH(session, "enter %s: header_index %d, name %s, nameLen %d\n",
                              __func__, header_index, name, nameLen);

    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    HttpRespHeaders &respHeaders = pSession->getResp()->getRespHeaders();

    MtLock lk(pSession->getMtSessData()->m_respHeaderLock);
    if (pSession->isMtHandlerCancelled())
    {
        LS_TH_BENIGN(lk.getPtr(), "ok exiting");
        return LS_FAIL;
    }
    if (pSession->getMtFlag(HSF_MT_RESP_HDR_SENT))
    {
        LS_TH_BENIGN(lk.getPtr(), "ok exiting");
        return LS_FAIL;
    }

    if (header_index < LSI_RSPHDR_END)
        respHeaders.del((HttpRespHeaders::INDEX)header_index);
    else
        respHeaders.del(name, nameLen);
    return LS_OK;
}


static int set_resp_content_length_ts(const lsi_session_t *session, int64_t len)
{
    LSI_DBGH(session, "enter %s: len %d\n", __func__, (int) len);
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    assert(pSession->getMtFlag(HSF_MT_HANDLER));
    HttpResp *pResp = ((HttpSession *)((LsiSession *)session))->getResp();

    MtLock lk(pSession->getMtSessData()->m_respHeaderLock);
    if (pSession->isMtHandlerCancelled())
    {
        LS_TH_BENIGN(lk.getPtr(), "ok exiting");
        return LS_FAIL;
    }
    if (pSession->getMtFlag(HSF_MT_RESP_HDR_SENT))
    {
        LS_TH_BENIGN(lk.getPtr(), "ok exiting");
        return LS_FAIL;
    }
    pResp->setContentLen(len);
    pResp->appendContentLenHeader();
    return LS_OK;
}


static int set_resp_header_ts(const lsi_session_t *session,
                           unsigned int header_index, const char *name,
                           int nameLen, const char *val, int valLen,
                           int add_method)
{
    LSI_DBGH(session,
             "enter %s: header_index %d, name %.*s, nameLen %d, val %.*s, valLen %d, add_method %d\n",
             __func__, header_index, nameLen, name, nameLen, valLen, val, valLen, add_method);

    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    assert(pSession->getMtFlag(HSF_MT_HANDLER));
    HttpRespHeaders &respHeaders = pSession->getResp()->getRespHeaders();

    MtLock lk(pSession->getMtSessData()->m_respHeaderLock);
    if (pSession->isMtHandlerCancelled())
    {
        LS_TH_BENIGN(lk.getPtr(), "ok exiting");
        return LS_FAIL;
    }
    if (pSession->getMtFlag(HSF_MT_RESP_HDR_SENT))
    {
        LS_TH_BENIGN(lk.getPtr(), "ok exiting");
        return LS_FAIL;
    }
    if (header_index < LSI_RSPHDR_END)
    {
        respHeaders.add((HttpRespHeaders::INDEX)header_index, val, valLen,
                add_method);
        if (header_index == HttpRespHeaders::H_CONTENT_TYPE)
            pSession->updateContentCompressible();
    }
    else
    {
        respHeaders.add(name, nameLen, val, valLen, add_method);
    }
    return LS_OK;
}


static int set_resp_header2_ts(const lsi_session_t *session, const char *s, int len,
        int add_method)
{
    LSI_DBGH(session, "enter %s: s %.*s, len %d,  add_method %d\n",
             __func__, len, s, len,  add_method);

    int len0, len1;
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    HttpRespHeaders &respHeaders = pSession->getResp()->getRespHeaders();

    //Save and check if the contenttype updated, if yes, processContentType
    //most case is change from NULL to a valid type.

    MtLock lk(pSession->getMtSessData()->m_respHeaderLock);
    if (pSession->isMtHandlerCancelled())
    {
        LS_TH_BENIGN(lk.getPtr(), "ok exiting");
        return LS_FAIL;
    }
    if (pSession->getMtFlag(HSF_MT_RESP_HDR_SENT))
    {
        LS_TH_BENIGN(lk.getPtr(), "ok exiting");
        return LS_FAIL;
    }
    char *type0 = respHeaders.getContentTypeHeader(len0);
    respHeaders.parseAdd(s, len, add_method);
    char *type1 = respHeaders.getContentTypeHeader(len1);

    if (type0 != type1 || len0 != len1)
        pSession->updateContentCompressible();

    return LS_OK;
}




#define MAX_RESP_BODY_BUF (128 * 1024)
static int append_resp_body_ts_ex(HttpSession *pSession, const char *buf,
                                  int len)
{
    MtNotifier *pNotifier;
    const char *pEnd = buf + len;
    const char *p = buf;
    size_t buffered;
    LSI_DBGH(pSession, "enter %s: begin write buf %p, %d bytes\n", __func__, buf, len);

    MtWriteMode & writeMode = pSession->getMtSessData()->m_mtWrite.m_writeMode;

    pNotifier = pSession->getMtSessData()->getWriteNotifier();
    while (p < pEnd)
    {
        if (pSession->isMtHandlerCancelled())
            return LS_FAIL;

        MtWriteBuf & wBuf = pSession->getMtSessData()->m_mtWrite.m_writeBuf;
        size_t availSize = wBuf.getBuf() ?  wBuf.getBufEnd() - wBuf.getCurPos() : 0;
        size_t size = 8192; // FIXME get rid of magic number

        if (wBuf.getBuf() && 0 == availSize)
        {
            // we've filled the buffer, flush it and replace
            LSI_DBGH(pSession, "%s: calling flush local buf\n", __func__);

            lsi_flush_local_buf_ts(pSession);
        }

        if (NULL == wBuf.getBuf())
        {
            // first time through or we just flushed
            char * buf = lsi_new_local_buf_ts(pSession, size, writeMode);
            availSize = size;

            assert(buf && "got NULL buf!");

            wBuf.set(buf, size, buf);
        }

        // have a writable local buffer

        size_t wantSize = pEnd - p;
        size_t copySize = wantSize <= availSize ? wantSize : availSize;

        LSI_DBGH(pSession, "%s: appending (%p, %lu) to wBuf: %p, %lu, %p\n",
                 __func__, p, copySize, wBuf.getBuf(), wBuf.getBlockSize(), wBuf.getCurPos());

        memmove(wBuf.getCurPos(), p, copySize);
        p += copySize;
        wBuf.set(wBuf.getBuf(), wBuf.getBufEnd() - wBuf.getBuf(), wBuf.getCurPos() + copySize);


        buffered = pSession->getRespBodyBuffered();
        if (buffered >= MAX_RESP_BODY_BUF)
        {
            // time to flush, do our local buf first
            LSI_DBGH(pSession, "%s: calling flush (partial) local buf\n", __func__);

            lsi_flush_local_buf_ts(pSession); // marks wBuf empty, will get new next loop iter

            pNotifier->lock();
            buffered = pSession->getRespBodyBuffered();
            if (buffered >= MAX_RESP_BODY_BUF
                && !pSession->isMtHandlerCancelled())
            {
                // need to write out the response buffer
                if (buffered == HS_RESP_NOT_READY)
                {
                    // beginMtHandler sets up the VMemBuf in HttpSession so this should
                    // never happen
                    pSession->setMtFlag(HSF_MT_INIT_RESP_BUF);
                    // will call setupDynRespBodyBuf, setupRespCache, setRespBodyBuf, HttpResourceManager::getInstance().getVMemBuf()
                }
                else
                    pSession->setMtFlag(HSF_MT_FLUSH, 1);
                    // processMtEvents clears HSF_MT_FLUSH, CLEARS HSF_RESP_FLUSHED, calls flush()
                schedule_mt_notify_event(pSession);

                LSI_DBGH(pSession, "%s finished: %ld, begin wait...\n",
                         __func__, p - buf );
                pNotifier->wait();
                LSI_DBGH(pSession, "%s end wait\n", __func__);
            }
            pNotifier->unlock();
        }
    }
    LSI_DBGH(pSession, "%s finish write %ld bytes\n", __func__, p - buf);
    return p - buf;
}


//return 0 is OK, -1 error
static int append_resp_body_ts(const lsi_session_t *session, const char *buf,
                            int len)
{
    LSI_DBGH(session, "enter %s: buf %p, len %d\n", __func__, buf, len);

    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    assert(pSession->getMtFlag(HSF_MT_HANDLER));
    if (pSession->isEndResponse() || pSession->isMtHandlerCancelled())
        return LS_FAIL;

    ls_mutex_lock(&pSession->getMtSessData()->m_mutex_writer);
    int ret = append_resp_body_ts_ex(pSession, buf, len);
    ls_mutex_unlock(&pSession->getMtSessData()->m_mutex_writer);
    return ret;
}


static int append_resp_bodyv_ts(const lsi_session_t *session,
                             const struct iovec *vector, int count)
{
    LSI_DBGH(session, "enter %s: vector %p, count %d\n", __func__, vector, count);

    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    //FIXME: determine what handling is needed for filter hooks - if
    // filters try to lock mutex_writer, this will hang!
    ls_mutex_lock(&pSession->getMtSessData()->m_mutex_writer);
    bool error = 0;
    for (int i = 0; i < count; ++i)
    {
        if (pSession->isEndResponse() || pSession->isMtHandlerCancelled())
        {
            error = LS_FAIL;
            break;
        }

        if (append_resp_body_ts_ex(pSession, (char *)((*vector).iov_base),
                    (int)(*vector).iov_len) <= 0)
        {
            error = 1;
            break;
        }
        ++vector;
    }

    ls_mutex_unlock(&pSession->getMtSessData()->m_mutex_writer);
    return error;
}


static int read_req_body_ts(const lsi_session_t *session, char *buf, int bufLen)
{
    LSI_DBGH(session, "enter %s: buf %p, bufLen %d\n", __func__, buf, bufLen);

    MtNotifier *pNotifier;
    VMemBuf *pReqBuf;
    char *p;
    size_t size;
    int iReqBodyDone = 0, iSpaceRemain = bufLen;
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    if (NULL == (pReqBuf = pSession->getReq()->getBodyBuf()))
        return LS_FAIL;

    // Someone else is already reading.
    if (!pSession->testAndSetMtFlag(HSF_MT_READING))
        return LS_FAIL;

    LSI_DBGH(pSession, "read_req_body_ts(), begin read %d bytes\n", bufLen);
    pNotifier = pSession->getMtSessData()->getReadNotifier();
    while ((!pSession->isMtHandlerCancelled()) // Session is not cancelled.
        && (iSpaceRemain > 0)) // I still want more data.
    {
        iReqBodyDone = pSession->getFlag(HSF_REQ_BODY_DONE);
        p = pReqBuf->getReadBuffer(size);
        LSI_DBGH(pSession, "read_req_body_ts(), getReadBuffer() size: %ld\n", size);
        if (size > 0)
        {
            if ((int)size > iSpaceRemain)
                size = iSpaceRemain;
            memcpy(buf, p, size);
            pReqBuf->readUsed(size);
            buf += size; // update pointer
            iSpaceRemain = iSpaceRemain - (int)size; // update how much data needs to be read
        }
        else
        {
            if (iReqBodyDone)
                break;
            pNotifier->lock(); // lock
            // NOTICE: Checking if the req body is done MUST be done before updating the size.
            iReqBodyDone = pSession->getFlag(HSF_REQ_BODY_DONE);
            p = pReqBuf->getReadBuffer(size);

            if (0 == size && !iReqBodyDone && !pSession->isMtHandlerCancelled())
            {
                LSI_DBGH(pSession, "read_req_body_ts(), begin wait ...\n");
                pNotifier->wait();
                LSI_DBGH(pSession, "read_req_body_ts(), end wait\n");
            }
            pNotifier->unlock();
        }
    }

    pSession->clearMtFlag(HSF_MT_READING);
    if (pSession->isMtHandlerCancelled())
    {
        LSI_DBGH(pSession, "read_req_body_ts(), MT handler canceled\n");
        return LS_FAIL;
    }
    LSI_DBGH(pSession, "read_req_body_ts(), read %d bytes\n", bufLen - iSpaceRemain);

    return bufLen - iSpaceRemain;
}


static long schedule_mt_notify_event(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession->testAndSetMtFlag(HSF_MT_NOTIFIED))
    {
        evtcb_pf cb = (evtcb_pf)(&HttpSession::mtNotifyCallback);
        return create_event(cb, session, pSession->getSn(), NULL, true);
    }
    return 0;
}


// DEBUG TEMP:
long schedule_remove_session_cbs_event(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    evtcb_pf cb = (evtcb_pf)(&HttpSession::removeSessionCbsEvent);
    return create_event(cb, session, pSession->getSn(), NULL, 0); // nowait = false
}


static long schedule_mt_notify_event_params(const lsi_session_t *session,
                                           long lParam, void * pParam)
{
    evtcb_pf cb = (evtcb_pf)(&HttpSession::mtNotifyCallbackEvent);
    return create_event(cb, session, lParam, pParam, true);
}


static int blocking_mt_notify_event(const lsi_session_t *session, int32_t flag,
                                    void *pParam = NULL)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    MtNotifier *pNotifier = &pSession->getMtSessData()->m_event;
    int32_t flaggedAndNotified = flag | (pParam ? 0 : HSF_MT_NOTIFIED);

    while (!pSession->isMtHandlerCancelled()
        && pSession->testMtFlag(flag))
    {
        if (NULL == pParam)
            schedule_mt_notify_event(pSession);
        else
            schedule_mt_notify_event_params(pSession, flag, pParam);
        LSI_DBGM(pSession, "lock event waiters lock.\n");
        pNotifier->lock();
        if ((!pSession->isMtHandlerCancelled())
            && (flaggedAndNotified == pSession->testMtFlag(flaggedAndNotified)))
        {
            LSI_DBGM(pSession, "enter event wait.\n");
            pNotifier->wait();
            LSI_DBGM(pSession, "exited event wait.\n");
        }
        LSI_DBGM(pSession, "unlock event waiters lock.\n");
        pNotifier->unlock();
    }

    return (pSession->isMtHandlerCancelled() ? LS_FAIL : LS_OK);
}


static int parse_req_args_ts(const lsi_session_t *session, int parse_req_body,
                          int uploadPassByPath, const char *uploadTmpDir,
                          int uploadTmpFilePermission)
{
    LSI_DBGH(session,
             "enter %s: parse_req_body %d, uploadPassByPath %d, uploadTmpDir %s, uploadTmpFilePermission %d\n",
             __func__, parse_req_body, uploadPassByPath, uploadTmpDir, uploadTmpFilePermission);

    MtParamParseReqArgs params;
    MtSessData *pMtData;
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    assert(pSession->getMtFlag(HSF_MT_HANDLER));
    if (pSession->isMtHandlerCancelled())
        return LS_FAIL;
    if (!pSession->testAndSetMtFlag(HSF_MT_PARSE_REQ_ARGS))
    {
        LSI_ERR(pSession, "Another thread already requested parsing request args.\n");
        return LS_FAIL;
    }

    pMtData = pSession->getMtSessData();
    params.m_pUploadTmpDir = uploadTmpDir;
    params.m_parseReqBody = parse_req_body;
    params.m_uploadPassByPath = uploadPassByPath;
    params.m_tmpFilePerms = uploadTmpFilePermission;

    if (LS_FAIL == blocking_mt_notify_event(pSession, HSF_MT_PARSE_REQ_ARGS, &params))
    {
        pSession->clearMtFlag(HSF_MT_PARSE_REQ_ARGS);
        return LS_FAIL;
    }
    return params.m_ret;
}


/**
 * The various calls to getRemain() used in send_file_ts_ex are all safe.
 * Currently, the only way the remaining value is going to change is by decreasing
 * as more data is written. If both getRemain calls return > 0, I will wait and
 * I am guaranteed to be notified because I have the notify lock.
 * If the first returns > 0 and the second returns 0, I will not wait.
 * If the first returns 0, then I am done.
 */
static inline off_t get_sendfile_remain_ign_helper(SendFileInfo *pSendFileInfo)
{
    off_t ret;
    LS_TH_IGN_RD_BEG();
    ret = pSendFileInfo->getRemain();
    LS_TH_IGN_RD_END();
    return ret;
}


static int send_file_ts_ex(HttpSession *pSession, const char *path,
                    int fd, int64_t start, int64_t size)
{
    LSI_DBGH(pSession, "enter %s: path %p, fd %d, start  %ld, size %ld\n",
             __func__, path, fd, start, size);
    MtParamSendfile params;
    MtSessData *pMtData = pSession->getMtSessData();
    MtNotifier *pNotifier;
    SendFileInfo *pSendFileInfo;

    if (!pSession->testAndSetMtFlag(HSF_MT_SENDFILE))
    {
        LSI_ERR(pSession, "Another thread is in the middle of sending a file.\n");
        return LS_FAIL;
    }

    params.m_pFile = path;
    params.m_fd = fd;
    params.m_start = start;
    params.m_size = size;
    params.m_ret = 0;

    if (LS_FAIL == blocking_mt_notify_event(pSession, HSF_MT_SENDFILE, &params))
    {
        LSI_ERR(pSession, "Blocking notify failed. Perhaps session cancelled?\n");
        pSession->clearMtFlag(HSF_MT_SENDFILE);
        return LS_FAIL;
    }

    if (LS_FAIL == params.m_ret)
    {
        LSI_ERR(pSession, "lsi_sendfile_ts_ex() failed to send.\n");
        return LS_FAIL;
    }

    pNotifier = pMtData->getWriteNotifier();
    pSendFileInfo = pSession->getSendFileInfo();
    while (get_sendfile_remain_ign_helper(pSendFileInfo) > 0)
    {
        if (pSession->isMtHandlerCancelled())
        {
            LSI_ERR(pSession, "Handler cancelled in the middle of sending a file.\n");
            return LS_FAIL;
        }
        pNotifier->lock();
        if (!pSession->isMtHandlerCancelled()
           && (get_sendfile_remain_ign_helper(pSendFileInfo) > 0))
        {
            pSession->setMtFlag(HSF_MT_FLUSH, 1);
            schedule_mt_notify_event(pSession);
            LSI_DBGH(pSession, "send_file_ts_ex(), wait for remaining %lu bytes\n",
                     get_sendfile_remain_ign_helper(pSendFileInfo));
            pNotifier->wait();
            LSI_DBGH(pSession, "send_file_ts_ex(), out of wait\n");
        }
        pNotifier->unlock();
    }

    return LS_OK;
}


static int send_file_ts(const lsi_session_t *session, const char *path,
                     int64_t start, int64_t size)
{
    LSI_DBGH(session, "enter %s: path %p, start  %ld, size %ld\n",
             __func__, path, start, size);
    int ret = LS_FAIL;
    MtSessData *pMtData;
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    if (pSession->isEndResponse())
        return LS_FAIL;
    pMtData = pSession->getMtSessData();

    ls_mutex_lock(&pMtData->m_mutex_writer);
    if (!pSession->isEndResponse())
        ret = send_file_ts_ex(pSession, path, 0, start, size);
    ls_mutex_unlock(&pMtData->m_mutex_writer);
    return ret;
}


static int send_file2_ts(const lsi_session_t *session, int fd,
                      int64_t start, int64_t size)
{
    LSI_DBGH(session, "enter %s: fd %d, start  %ld, size %ld\n",
             __func__, fd, start, size);
    int ret = LS_FAIL;
    MtSessData *pMtData;
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;

    if (pSession->isEndResponse())
        return LS_FAIL;
    pMtData = pSession->getMtSessData();

    ls_mutex_lock(&pMtData->m_mutex_writer);
    if (!pSession->isEndResponse())
        ret = send_file_ts_ex(pSession, NULL, fd, start, size);
    ls_mutex_unlock(&pMtData->m_mutex_writer);
    return ret;
}


ls_inline int notifyCollapsibleMtFlag(HttpSession *pSession, int flag)
{
    assert(pSession->getMtFlag(HSF_MT_HANDLER));
    if (pSession->isMtHandlerCancelled())
        return LS_FAIL;

    if (pSession->testAndSetMtFlag(flag))
    {
        LSI_DBGH(pSession, "mark MT flag %X\n", flag);
        schedule_mt_notify_event(pSession);
    }
    else
        LSI_DBGH(pSession, "pending MT flag %X, skip ntofication\n", flag);
    return 0;
}


static int lsi_flush_ts(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    LSI_DBGH(pSession, "lsi_flush_ts() called\n");

//    lsi_flush_local_buf_ts(pSession);

    return notifyCollapsibleMtFlag(pSession, HSF_MT_FLUSH);
}


static void send_resp_headers_ts(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return;
    LSI_DBGH(pSession, "enter %s\n", __func__);
    (void) notifyCollapsibleMtFlag(pSession, HSF_MT_SND_RSP_HDRS);
}


static void lsi_end_resp_ts(const lsi_session_t *session)
{
    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return;


    LSI_DBGH(pSession, "enter %s\n", __func__);

//    lsi_flush_local_buf_ts(pSession);

    (void) notifyCollapsibleMtFlag(pSession, HSF_MT_END_RESP);
}


static int set_uri_qs_ts(const lsi_session_t *session, int action, const char *uri,
                      int uri_len, const char *qs, int qs_len)
{

    HttpSession *pSession = (HttpSession *)((LsiSession *)session);
    if (pSession == NULL)
        return LS_FAIL;
    if (!action)
        return LS_OK;
    // 2 ways to handle the data - either do a blocking wait
    // to make sure strings remain valid until processed, and
    // then when waking unlock here, or dynamically alloc
    // safe copies for the main thread to use and let main
    // thread unlock when it's done with them
    // since this func always returned LS_OK, skip the wait
    MtParamUriQs * pParam = (MtParamUriQs *) malloc(sizeof(MtParamUriQs));
    pParam->m_action = action;
    pParam->m_uri = ls_str_new(uri, uri_len);
    pParam->m_qs = ls_str_new(qs, qs_len);
    schedule_mt_notify_event_params(pSession, HSF_MT_SET_URI_QS, pParam);
    return LS_OK;
}


// Timer is not available for MT handler.
static int lsi_set_timer_ts(unsigned int timeout_ms, int repeat,
                         lsi_timercb_pf timer_cb, const void *timer_cb_param)
{
    return LS_FAIL;
}


static int lsi_remove_timer_ts(int timer_id)
{
    return LS_FAIL;
}


static  void log_ts(const lsi_session_t *session, int level, const char *fmt, ...)
{
    if (log4cxx::Level::isEnabled(level))
    {
        char new_fmt[8192];
        snprintf(new_fmt, 8191, "[T%d] %s", ls_thr_seq(), fmt);
        HttpSession *pSess = (HttpSession *)((LsiSession *)session);
        LogSession *pLogSess = pSess ? pSess->getLogSession() : NULL;
        va_list ap;
        va_start(ap, fmt);
        LOG4CXX_NS::Logger::s_vlog(level, pLogSess, new_fmt, ap, 1);
        va_end(ap);
    }

}


static  void vlog_ts(const lsi_session_t *session, int level, const char *fmt,
                  va_list vararg, int no_linefeed)
{
    if (log4cxx::Level::isEnabled(level))
    {
        char new_fmt[8192];
        snprintf(new_fmt, 8191, "[T%d] %s", ls_thr_seq(), fmt);
        HttpSession *pSess = (HttpSession *)((LsiSession *)session);
        LogSession *pLogSess = pSess ? pSess->getLogSession() : NULL;
        LOG4CXX_NS::Logger::s_vlog(level, pLogSess, new_fmt, vararg, no_linefeed);
    }

}


static void module_log_ts(const lsi_module_t *pMod, const lsi_session_t *session,
                       int level, const char *fmt, ...)
{
    if (log4cxx::Level::isEnabled(level) && level <= MODULE_LOG_LEVEL(pMod))
    {
        char new_fmt[8192];
        snprintf(new_fmt, 8191, "[%s] [T%d] %s", MODULE_NAME(pMod),
                 ls_thr_seq(), fmt);
        HttpSession *pSess = (HttpSession *)((LsiSession *)session);
        LogSession *pLogSess = pSess ? pSess->getLogSession() : NULL;
        va_list ap;
        va_start(ap, fmt);
        LOG4CXX_NS::Logger::s_vlog(level, pLogSess, new_fmt, ap, 1);
        va_end(ap);

    }
}


static void c_log_ts(const char *pComponent, const lsi_session_t *session,
                       int level, const char *fmt, ...)
{
    if (log4cxx::Level::isEnabled(level))
    {
        char new_fmt[8192];
        snprintf(new_fmt, 8191, "[%s] [T%d] %s", pComponent, ls_thr_seq(), fmt);
        HttpSession *pSess = (HttpSession *)((LsiSession *)session);
        LogSession *pLogSess = pSess ? pSess->getLogSession() : NULL;
        va_list ap;
        va_start(ap, fmt);
        LOG4CXX_NS::Logger::s_vlog(level, pLogSess, new_fmt, ap, 1);
        va_end(ap);

    }
}


static lsi_api_t g_lsiapi;
static lsi_api_t g_lsiapi_ts;
__thread const lsi_api_t *g_api = &g_lsiapi_ts;


void lsiapi_init_server_api()
{
    lsi_api_t *pApi = &g_lsiapi;
    pApi->enable_hook = enable_hook;
    pApi->get_hook_level = get_hook_level;
    pApi->get_hook_flag = get_hook_flag;

    pApi->register_env_handler = lsiapi_register_env_handler;
    pApi->get_module = get_module;


    pApi->get_server_root = get_server_root;
    pApi->get_config = get_config;
    pApi->init_module_data = init_module_data;

    pApi->get_module_data = get_module_data;
    pApi->get_cb_module_data = get_cb_module_data;
    pApi->set_module_data = set_module_data;
    pApi->free_module_data = free_module_data;
    pApi->get_req_method = get_req_method;
    pApi->get_req_vhost = get_req_vhost;
    pApi->stream_writev_next = lsiapi_stream_writev_next;
    pApi->stream_read_next = lsiapi_stream_read_next;

    pApi->vlog = vlog;
    pApi->log = log;
    pApi->lograw = lograw;

    pApi->set_status_code = set_status_code;
    pApi->get_status_code = get_status_code;

    pApi->is_req_handler_registered = is_req_handler_registered;
    pApi->register_req_handler = register_req_handler;
    pApi->set_handler_write_state = set_handler_write_state;
    pApi->set_timer = lsi_set_timer;
    pApi->remove_timer = lsi_remove_timer;

    pApi->create_event = create_event;
    pApi->create_session_resume_event = create_session_resume_event;

    pApi->get_event_obj = get_event_obj;
    pApi->schedule_event = schedule_event;
    pApi->cancel_event = cancel_event;

    pApi->get_req_raw_headers_length = get_req_raw_headers_length;
    pApi->get_req_raw_headers = get_req_raw_headers;
    pApi->get_req_headers_count = get_req_headers_count;
    pApi->get_req_headers = get_req_headers;

    pApi->get_req_header = get_req_header;
    pApi->get_req_header_by_id = get_req_header_by_id;
    pApi->get_req_org_uri = get_req_org_uri;
    pApi->get_req_uri = get_req_uri;
    pApi->get_uri_file_path = get_uri_file_path;
    pApi->get_mapped_context_uri = get_mapped_context_uri;

    pApi->get_client_ip = get_client_ip;
    pApi->get_req_query_string = get_req_query_string;
    pApi->get_req_cookies = get_req_cookies;
    pApi->get_req_cookie_count = get_req_cookie_count;
    pApi->get_cookie_value = get_cookie_value;
    pApi->get_cookie_by_index = get_cookie_by_index;

    pApi->get_req_env = get_req_env;
    pApi->set_req_env = set_req_env;
    pApi->get_req_var_by_id = get_req_var_by_id;
    pApi->get_req_content_length = get_req_content_length;
    pApi->read_req_body = read_req_body;
    pApi->is_req_body_finished = is_req_body_finished;
    pApi->get_resp_buffer_compress_method = get_resp_buffer_compress_method;

    pApi->get_req_args_count = get_req_args_count;
    pApi->get_req_arg_by_idx = get_req_arg_by_idx;
    pApi->get_qs_args_count = get_qs_args_count;
    pApi->get_qs_arg_by_idx = get_qs_arg_by_idx;
    pApi->get_post_args_count = get_post_args_count;
    pApi->get_post_arg_by_idx = get_post_arg_by_idx;
    pApi->is_post_file_upload = is_post_file_upload;

    pApi->set_resp_buffer_compress_method = set_resp_buffer_compress_method;
    pApi->set_req_wait_full_body = set_req_wait_full_body;
    pApi->parse_req_args = parse_req_args;
    pApi->set_resp_wait_full_body = set_resp_wait_full_body;

    pApi->is_resp_buffer_available = is_resp_buffer_available;
    pApi->append_resp_body = append_resp_body;
    pApi->append_resp_bodyv = append_resp_bodyv;
    pApi->send_file = send_file;
    pApi->send_file2 = send_file2;
    pApi->init_file_type_mdata = init_file_type_mdata;
    pApi->flush = lsi_flush;
    pApi->end_resp = lsi_end_resp;

    pApi->set_resp_content_length = set_resp_content_length;
    pApi->set_uri_qs = set_uri_qs;

    //pApi->lsiapi_tcp_writev = lsiapi_tcp_writev;
    pApi->stream_write_next = stream_write_next;


    pApi->set_resp_header = set_resp_header;
    pApi->set_resp_header2 = set_resp_header2;
    pApi->set_resp_cookies = set_resp_cookies;
    pApi->get_resp_header = get_resp_header;
    pApi->get_resp_headers_count = get_resp_headers_count;
    pApi->get_resp_header_id = get_resp_header_id;
    pApi->get_resp_headers = get_resp_headers;
    pApi->remove_resp_header = remove_resp_header;
    pApi->send_resp_headers = send_resp_headers;
    pApi->is_resp_headers_sent = is_resp_headers_sent;

    pApi->get_multiplexer = lsiapi_get_multiplexer;
    pApi->edio_reg = edio_reg;
    pApi->edio_remove = edio_remove;
    pApi->edio_modify = edio_modify;

    pApi->is_suspended = lsiapi_is_suspended;
    pApi->resume = lsiapi_resume;

    pApi->exec_ext_cmd = exec_ext_cmd;
    pApi->get_ext_cmd_res_buf = get_ext_cmd_res_buf;
    pApi->get_client_access_level = get_client_access_level;


    pApi->get_file_path_by_uri = get_file_path_by_uri;
    //pApi->get_static_file_stat = get_static_file_stat;
    pApi->get_mime_type_by_suffix = get_mime_type_by_suffix;
    pApi->set_force_mime_type = set_force_mime_type;
    pApi->get_req_file_path = get_req_file_path;
    pApi->get_req_handler_type = get_req_handler_type;

    pApi->is_access_log_on = is_access_log_on;
    pApi->set_access_log = set_access_log;
    pApi->get_access_log_string = get_access_log_string;

    pApi->get_file_stat = get_file_stat;

    pApi->is_resp_handler_aborted = is_resp_handler_aborted;
    pApi->get_resp_body_buf = get_resp_body_buf;

    pApi->get_module_name = get_module_name;
    pApi->get_req_body_buf = get_req_body_buf;
    pApi->get_new_body_buf = get_new_body_buf;
    pApi->get_body_buf_size = get_body_buf_size;
    pApi->acquire_body_buf_block = acquire_body_buf_block;
    pApi->release_body_buf_block = release_body_buf_block;
    pApi->get_body_buf_fd = get_body_buf_fd;
    pApi->is_body_buf_eof = is_body_buf_eof;
    pApi->reset_body_buf = reset_body_buf;
    pApi->append_body_buf = append_body_buf;
    pApi->set_req_body_buf = set_req_body_buf;
    pApi->get_cur_time = get_cur_time;

    pApi->get_vhost_count = get_vhost_count;
    pApi->get_vhost = get_vhost;
    pApi->set_vhost_module_data = set_vhost_module_data;
    pApi->get_vhost_module_data = get_vhost_module_data;
    pApi->get_vhost_module_conf = get_vhost_module_conf;
    pApi->get_session_pool = get_session_pool;

    pApi->handoff_fd = handoff_fd;
    pApi->get_local_sockaddr = get_local_sockaddr;
    pApi->expand_current_server_variable = expand_current_server_variable;

    pApi->set_ua_code = set_uacode;
    pApi->get_ua_code = get_uacode;

    pApi->foreach = lsi_foreach;

    pApi->module_log = module_log;
    pApi->c_log      = c_log;
    pApi->_log_level_ptr = log4cxx::Level::getDefaultLevelPtr();
    pApi->schedule_remove_session_cbs_event = schedule_remove_session_cbs_event;
    pApi->register_thread_cleanup = register_thread_cleanup_ts;

    g_lsiapi_ts = g_lsiapi;

    pApi = &g_lsiapi_ts;
    //override functions need MT safe version.
    pApi->set_resp_content_length = set_resp_content_length_ts;
    pApi->set_resp_header = set_resp_header_ts;
    pApi->set_resp_header2 = set_resp_header2_ts;
    pApi->set_resp_cookies = set_resp_cookies_ts;
    pApi->send_resp_headers = send_resp_headers_ts;
    pApi->remove_resp_header = remove_resp_header_ts;
    pApi->parse_req_args = parse_req_args_ts;
    pApi->append_resp_body = append_resp_body_ts;
    pApi->append_resp_bodyv = append_resp_bodyv_ts;
    pApi->read_req_body = read_req_body_ts;
    pApi->send_file = send_file_ts;
    pApi->send_file2 = send_file2_ts;
    pApi->flush = lsi_flush_ts;
    pApi->end_resp = lsi_end_resp_ts;
    pApi->set_uri_qs = set_uri_qs_ts;
    pApi->set_timer = lsi_set_timer_ts;
    pApi->remove_timer = lsi_remove_timer_ts;
    pApi->exec_ext_cmd = exec_ext_cmd_ts;
    pApi->get_cur_time = get_cur_time_ts;
    pApi->vlog = vlog_ts;
    pApi->log = log_ts;
    pApi->module_log = module_log_ts;
    pApi->c_log      = c_log_ts;
    pApi->register_thread_cleanup = register_thread_cleanup_ts;

    g_api = &g_lsiapi;
}
