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

//  Author: dxu@litespeedtech.com (David Xu)

#include "pagespeed.h"
#include <string.h>
#include <lsr/ls_loopbuf.h>
#include <lsr/ls_xpool.h>

#include "ls_caching_headers.h"
#include "ls_message_handler.h"
#include "ls_rewrite_driver_factory.h"
#include "ls_rewrite_options.h"
#include "ls_server_context.h"

#include <net/base/iovec.h>
#include <apr_poll.h>
#include "autostr.h"
#include "stringtool.h"
#include "ls_base_fetch.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pipenotifier.h"
#include "log_message_handler.h"
#include "../../src/http/serverprocessconfig.h"
#include <signal.h>

#include "net/instaweb/automatic/public/proxy_fetch.h"
#include "net/instaweb/http/public/async_fetch.h"
#include "net/instaweb/http/public/cache_url_async_fetcher.h"
#include "net/instaweb/http/public/content_type.h"
#include "net/instaweb/http/public/request_context.h"
#include "net/instaweb/public/global_constants.h"
//#include "net/instaweb/public/version.h"
#include "net/instaweb/rewriter/public/experiment_matcher.h"
#include "net/instaweb/rewriter/public/experiment_util.h"
#include "net/instaweb/rewriter/public/process_context.h"
#include "net/instaweb/rewriter/public/resource_fetch.h"
#include "net/instaweb/rewriter/public/rewrite_driver.h"
#include "net/instaweb/rewriter/public/rewrite_options.h"
#include "net/instaweb/rewriter/public/rewrite_query.h"
#include "net/instaweb/rewriter/public/rewrite_stats.h"
#include "net/instaweb/rewriter/public/static_asset_manager.h"
#include "net/instaweb/system/public/in_place_resource_recorder.h"
#include "net/instaweb/system/public/system_caches.h"
#include "net/instaweb/system/public/system_request_context.h"
#include "net/instaweb/system/public/system_rewrite_options.h"
#include "net/instaweb/system/public/system_server_context.h"
#include "net/instaweb/system/public/system_thread_system.h"
#include "net/instaweb/util/public/fallback_property_page.h"
#include "net/instaweb/util/public/google_message_handler.h"
#include "net/instaweb/util/public/google_url.h"
#include "net/instaweb/util/public/gzip_inflater.h"
#include "net/instaweb/util/public/null_message_handler.h"
#include "net/instaweb/util/public/query_params.h"
#include "net/instaweb/util/public/statistics_logger.h"
#include "net/instaweb/util/public/stdio_file_system.h"
#include "net/instaweb/util/public/string.h"
#include "net/instaweb/util/public/string_writer.h"
#include "net/instaweb/util/public/time_util.h"
#include "net/instaweb/util/stack_buffer.h"
#include "pagespeed/kernel/base/posix_timer.h"
#include "pagespeed/kernel/http/query_params.h"
#include "pagespeed/kernel/html/html_keywords.h"
#include "pagespeed/kernel/thread/pthread_shared_mem.h"

#define DBG(session, args...)                                       \
    g_api->log(session, LSI_LOG_DEBUG, args)

#define  POST_BUF_READ_SIZE 65536

// Needed for SystemRewriteDriverFactory to use shared memory.
#define PAGESPEED_SUPPORT_POSIX_SHARED_MEM


enum HTTP_METHOD
{
    HTTP_UNKNOWN = 0,
    HTTP_OPTIONS,
    HTTP_GET ,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT ,
    HTTP_DELETE,
    HTTP_TRACE,
    HTTP_CONNECT,
    HTTP_MOVE,

    //Some unused valus deleted.

    HTTP_PURGE,
    HTTP_REFRESH,
    HTTP_METHOD_END,
};


using namespace net_instaweb;

/**************************************************************************************************
 *
 * Understand the order of initialising
 * Onece find module in list, load module .so first,
 * then parse config will be called when it has parameter
 * then, _init() will be called for init the module after all modules loaded
 * now, all config read, and server start to serv.
 * then, main_inited HOOK will be called
 * pre_fork HOOK will be launched before children process forking.
 *
 *
 * ************************************************************************************************/


//process level data, can be accessed anywhere
struct ps_main_conf_t
{
    LsiRewriteDriverFactory    *driverFactory;
};

//VHost level data
struct ps_vh_conf_t
{
    LsServerContext            *serverContext;
    ProxyFetchFactory          *proxyFetchFactory;
    MessageHandler             *handler;
};

typedef struct
{
    LsiBaseFetch *baseFetch;
    lsi_session_t *session;

    // for html rewrite
    ProxyFetch *proxyFetch;

    // for in place resource
    RewriteDriver *driver;
    InPlaceResourceRecorder *recorder;

    bool htmlRewrite;
    bool inPlace;
    bool fetchDone;
    PreserveCachingHeaders preserveCachingHeaders;
} ps_request_ctx_t; //session module data

struct PsMData
{
    ps_request_ctx_t    *ctx;
    ps_vh_conf_t        *cfg_s;
    PipeNotifier        *pNotifier;
    void                *notifier_pointer;
    const char          *userAgent;
    int                  uaLen;
    GoogleString         urlString;
    HTTP_METHOD          method;

    //return result
    int16_t              statusCode;
    ResponseHeaders     *respHeaders;

    ls_loopbuf_t         buff;
    int8_t               endRespCalled;
    int8_t               doneCalled;
};

const char *kInternalEtagName = "@psol-etag";
// The process context takes care of proactively initialising
// a few libraries for us, some of which are not thread-safe
// when they are initialized lazily.
ProcessContext *g_pProcessContext = new ProcessContext();

//LsiRewriteOptions* is data in all level and just inherit with the flow
//global --> Vhost  --> context  --> session
static ps_main_conf_t *g_pMainConf = NULL;
static int g_bMainConfInited = 0;


//////////////////////
StringPiece StrToStringPiece(AutoStr2 s)
{
    return StringPiece(s.c_str(), s.len());
}

//All of the parameters should have "pagespeed" as the first word.
const char *paramArray[] =
{
    "pagespeed",
    NULL //Must have NULL in the last item
};

int SetCacheControl(lsi_session_t *session, char *cache_control)
{
    g_api->set_resp_header(session, LSI_RESP_HEADER_CACHE_CTRL, NULL, 0,
                           cache_control, strlen(cache_control), LSI_HEADER_SET);
    return 0;
}

//If copy_request is 0, then copy response headers
//Othercopy request headers.
template<class Headers>
void CopyHeaders(lsi_session_t *session, int is_from_request, Headers *to)
{
#define MAX_HEADER_NUM  50
    struct iovec iov_key[MAX_HEADER_NUM], iov_val[MAX_HEADER_NUM];
    int count = 0;

    if (is_from_request)
        count = g_api->get_req_headers(session, iov_key, iov_val, MAX_HEADER_NUM);
    else
        count = g_api->get_resp_headers(session, iov_key, iov_val, MAX_HEADER_NUM);

    for (int i = 0; i < count; ++i)
    {
        StringPiece key = "", value = "";
        key.set(iov_key[i].iov_base, iov_key[i].iov_len);
        value.set(iov_val[i].iov_base, iov_val[i].iov_len);
        to->Add(key, value);
    }
}

//return 1001 for 1.1, 2000 for 2.0 etc
static int GetHttpVersion(lsi_session_t *session)
{
    int major = 0, minor = 0;
    char val[10] = {0};
    int n = g_api->get_req_var_by_id(session, LSI_REQ_VAR_SERVER_PROTO, val, 10);

    if (n >= 8)   //should be http/
    {
        char *p = strchr(val, '/');
        if (p)
            sscanf(p, "%d.%d", &major, &minor);
    }

    return major * 1000 + minor;
}

void net_instaweb::CopyRespHeadersFromServer(
    lsi_session_t *session, ResponseHeaders *headers)
{
    int version = GetHttpVersion(session);
    headers->set_major_version(version / 1000);
    headers->set_minor_version(version % 1000);
    CopyHeaders(session, 0, headers);
    headers->set_status_code(g_api->get_status_code(session));

    struct iovec iov[1];
    if (g_api->get_resp_header(session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0,
                               iov, 1) == 1)
    {
        StringPiece content_type = "";
        content_type.set(iov[0].iov_base, iov[0].iov_len);
        headers->Add(HttpAttributes::kContentType, content_type);
    }

    // When we don't have a date header, set one with the current time.
    if (headers->Lookup1(HttpAttributes::kDate) == NULL)
    {
        int64 date_ms;
        int32_t date_us;
        date_ms = (g_api->get_cur_time(&date_us)) * 1000;
        date_ms += date_us / 1000;
        headers->SetDate(date_ms);
    }

    headers->ComputeCaching();
}

void net_instaweb::CopyReqHeadersFromServer(lsi_session_t *session,
        RequestHeaders *headers)
{
    int version = GetHttpVersion(session);
    headers->set_major_version(version / 1000);
    headers->set_minor_version(version % 1000);
    CopyHeaders(session, 1, headers);
}

int net_instaweb::CopyRespHeadersToServer(
    lsi_session_t *session,
    const ResponseHeaders &pagespeed_headers,
    PreserveCachingHeaders preserve_caching_headers)
{
    int i;
    for (i = 0 ; i < pagespeed_headers.NumAttributes() ; i++)
    {
        const GoogleString &name_gs = pagespeed_headers.Name(i);
        const GoogleString &value_gs = pagespeed_headers.Value(i);

        if (preserve_caching_headers == kPreserveAllCachingHeaders)
        {
            if (StringCaseEqual(name_gs, "ETag") ||
                StringCaseEqual(name_gs, "Expires") ||
                StringCaseEqual(name_gs, "Date") ||
                StringCaseEqual(name_gs, "Last-Modified") ||
                StringCaseEqual(name_gs, "Cache-Control"))
                continue;
        }
        else if (preserve_caching_headers == kPreserveOnlyCacheControl)
        {
            // Retain the original Cache-Control header, but send the recomputed
            // values for all other cache-related headers.
            if (StringCaseEqual(name_gs, "Cache-Control"))
                continue;
        } // else we don't preserve any headers

        AutoStr2 name, value;

        // To prevent the gzip module from clearing weak etags, we output them
        // using a different name here. The etag header filter module runs behind
        // the gzip compressors header filter, and will rename it to 'ETag'
        if (StringCaseEqual(name_gs, "etag")
            && StringCaseStartsWith(value_gs, "W/"))
            name.setStr(kInternalEtagName, strlen(kInternalEtagName));
        else
            name.setStr(name_gs.data(), name_gs.length());

        value.setStr(value_gs.data(), value_gs.length());


        if (STR_EQ_LITERAL(name, "Cache-Control"))
        {
            SetCacheControl(session, const_cast<char *>(value_gs.c_str()));
            continue;
        }
        else if (STR_EQ_LITERAL(name, "Content-Type"))
        {
            g_api->set_resp_header(session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0,
                                   value.c_str(), value.len(), LSI_HEADER_SET);
            continue;
        }
        else if (STR_EQ_LITERAL(name, "Connection"))
            continue;
        else if (STR_EQ_LITERAL(name, "Keep-Alive"))
            continue;
        else if (STR_EQ_LITERAL(name, "Transfer-Encoding"))
            continue;
        else if (STR_EQ_LITERAL(name, "Server"))
            continue;

        bool need_set = false;

        // Populate the shortcuts to commonly used headers.
        if (STR_EQ_LITERAL(name, "Date"))
            need_set = true;
        else if (STR_EQ_LITERAL(name, "Etag"))
            need_set = true;
        else if (STR_EQ_LITERAL(name, "Expires"))
            need_set = true;
        else if (STR_EQ_LITERAL(name, "Last-Modified"))
            need_set = true;
        else if (STR_EQ_LITERAL(name, "Location"))
            need_set = true;
        else if (STR_EQ_LITERAL(name, "Server"))
            need_set = true;
        else if (STR_EQ_LITERAL(name, "Content-Length"))
        {
            need_set = false;
            g_api->set_resp_content_length(session, (int64_t) atol(value.c_str()));
        }

        if (need_set)
        {
            g_api->set_resp_header(session, -1, name.c_str(), name.len(),
                                   value.c_str(), value.len(), LSI_HEADER_SET);
        }
    }

    return 0;
}


int net_instaweb::CopyRespBodyToBuf(lsi_session_t *session,
        const char *s, int len, int done_called)
{
    PsMData *pMyData = (PsMData *) g_api->get_module_data(session, &MNAME,
                       LSI_MODULE_DATA_HTTP);
    //pMyData->status_code = 200;
    ls_loopbuf_append(&pMyData->buff, s, len);
    pMyData->doneCalled = done_called;
    return 0;
}

int TerminateMainConf(lsi_cb_param_t *rec)
{
    delete g_pProcessContext;
    g_pProcessContext = NULL;

    if (g_pMainConf)
    {
        LsiRewriteOptions::Terminate();
        LsiRewriteDriverFactory::Terminate();
        delete g_pMainConf->driverFactory;
        //delete pMainConf->handler;
        //pMainConf->handler = NULL;
        delete g_pMainConf;
        g_pMainConf = NULL;
    }

    return 0;
}

static int ReleaseVhConf(void *p)
{
    if (p)
    {
        ps_vh_conf_t *cfg_s = (ps_vh_conf_t *) p;

        if (cfg_s->proxyFetchFactory)
            delete cfg_s->proxyFetchFactory;

        if (cfg_s->serverContext)
            delete cfg_s->serverContext;

        delete cfg_s;
    }

    return 0;
}

void ReleaseBaseFetch(PsMData *pData)
{
    // In the normal flow BaseFetch doesn't delete itself in HandleDone() because
    // we still need to receive notification via pipe and call
    // CollectAccumulatedWrites.  If there's an error and we're cleaning up early
    // then HandleDone() hasn't been called yet and we need the base fetch to wait
    // for that and then delete itself.
    if (pData->ctx == NULL)
        return ;

    if (pData->ctx->baseFetch != NULL)
    {
        pData->ctx->baseFetch->Release();
        pData->ctx->baseFetch = NULL;
    }

    if (pData->pNotifier != NULL)
    {
        pData->pNotifier->uninitNotifier((Multiplexer *) g_api->get_multiplexer());
        delete pData->pNotifier;
        pData->pNotifier = NULL;
    }
}

static int ReleaseRequestContext(PsMData *pData)
{
    // proxy_fetch deleted itself if we called Done(), but if an error happened
    // before then we need to tell it to delete itself.
    //
    // If this is a resource fetch then proxy_fetch was never initialized.
    if (pData->ctx->proxyFetch != NULL)
    {
        pData->ctx->proxyFetch->Done(false /* failure */);
        pData->ctx->proxyFetch = NULL;
    }

    if (pData->ctx->driver != NULL)
    {
        pData->ctx->driver->Cleanup();
        pData->ctx->driver = NULL;
    }

    if (pData->ctx->recorder != NULL)
    {
        pData->ctx->recorder->Fail();
        pData->ctx->recorder->DoneAndSetHeaders(NULL);    // Deletes recorder.
        pData->ctx->recorder = NULL;
    }

    ReleaseBaseFetch(pData);
    delete pData->ctx;
    pData->ctx = NULL;
    return 0;
}

static int ReleaseMydata(void *data)
{
    PsMData *pData = (PsMData *) data;

    if (!pData)
        return 0;

    if (pData->ctx)
        ReleaseRequestContext(pData);

    if (pData->respHeaders)
        delete pData->respHeaders;

    if (pData->notifier_pointer)
        g_api->remove_event_notifier(&pData->notifier_pointer);

    ls_loopbuf_d(&pData->buff);

    delete pData;
    return 0;
}

int EndSession(lsi_cb_param_t *rec)
{
    PsMData *pData = (PsMData *) g_api->get_module_data(rec->_session, &MNAME,
                     LSI_MODULE_DATA_HTTP);

    if (pData != NULL)
    {
        g_api->free_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP,
                                ReleaseMydata);
        g_api->set_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP, NULL);
    }

    g_api->log(rec->_session, LSI_LOG_DEBUG, "[%s]ps_end_session.\n",
               ModuleName);
    return 0;
}

bool IsHttps(lsi_session_t *session)
{
//COmment: the below code to check if it is HTTPS but we should not check HTTPS
//    because it may cause other issue, so just always return false;
//     char s[12] = {0};
//     int len = g_api->get_req_var_by_id(session, LSI_REQ_SSL_VERSION, s, 12);
//     if( len > 3 )
//         return true;
    return false;
}

void IgnoreSigpipe()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGPIPE, &act, NULL);
}

void InitDir(const StringPiece &directive,
                 const StringPiece &path)
{
    if (path.size() == 0 || path[0] != '/')
    {
        g_api->log(NULL, LSI_LOG_ERROR, "[%s] %s %s must start with a slash.\n",
                   ModuleName, directive.as_string().c_str(),
                   path.as_string().c_str());
        return ;
    }

    net_instaweb::StdioFileSystem file_system;
    net_instaweb::NullMessageHandler message_handler;
    GoogleString gs_path;
    path.CopyToString(&gs_path);

    if (!file_system.IsDir(gs_path.c_str(), &message_handler).is_true())
    {
        if (!file_system.RecursivelyMakeDir(path, &message_handler))
        {
            g_api->log(NULL, LSI_LOG_ERROR,
                       "[%s] %s path %s does not exist and could not be created.\n",
                       ModuleName, directive.as_string().c_str(),
                       path.as_string().c_str());
            return ;
        }

        // Directory created, but may not be readable by the worker processes.
    }

    if (geteuid() != 0)
    {
        return;  // We're not root, so we're staying whoever we are.
    }

    uid_t user = ServerProcessConfig::getInstance().getUid();
    gid_t group = ServerProcessConfig::getInstance().getGid();

    struct stat gs_stat;

    if (stat(gs_path.c_str(), &gs_stat) != 0)
    {
        g_api->log(NULL, LSI_LOG_ERROR, "[%s] %s path %s stat() failed.\n",
                   ModuleName, directive.as_string().c_str(),
                   path.as_string().c_str());
        return ;
    }

    if (gs_stat.st_uid != user)
    {
        if (chown(gs_path.c_str(), user, group) != 0)
        {
            g_api->log(NULL, LSI_LOG_ERROR,
                       "[%s] %s path %s unable to set permissions.\n",
                       ModuleName, directive.as_string().c_str(),
                       path.as_string().c_str());
            return ;
        }
    }
}

//return 0 OK, -1 error
static int CreateMainConf()
{
    if (g_bMainConfInited == 1)
        return 0;

    g_pMainConf = new ps_main_conf_t;

    if (!g_pMainConf)
    {
        g_api->log(NULL, LSI_LOG_ERROR, "[%s]GDItem init error.\n", ModuleName);
        return LS_FAIL;
    }

    LsiRewriteOptions::Initialize();
    LsiRewriteDriverFactory::Initialize();

    g_pMainConf->driverFactory = new LsiRewriteDriverFactory(
        *g_pProcessContext,
        new SystemThreadSystem(),
        "" /* hostname, not used */,
        -1 /* port, not used */);

    if (g_pMainConf->driverFactory == NULL)
    {
        delete g_pMainConf;
        g_pMainConf = NULL;
        g_api->log(NULL, LSI_LOG_ERROR, "[%s]GDItem init error 2.\n", ModuleName);
        return LS_FAIL;
    }

    g_pMainConf->driverFactory->Init();
    g_bMainConfInited = 1;
    return 0;
}

inline void TrimLeadingSpace(const char **p)
{
    char ch;
    while (((ch = **p) == ' ') || (ch == '\t') || (ch == '\r'))
        ++ (*p);
}

static void ParseOption(LsiRewriteOptions *pOption, const char *sLine,
                        int len, int level, const char *name)
{
    const char *pEnd = sLine + len;
    TrimLeadingSpace(&sLine);

    if (pEnd - sLine <= 10 || strncasecmp(sLine, "pagespeed", 9) != 0
        || (sLine[9] != ' ' && sLine[9] != '\t'))
    {
        if (pEnd - sLine > 0)
        {
            g_api->log(NULL, LSI_LOG_DEBUG,
                       "[%s]parseOption do not parse %s in level %s\n",
                       ModuleName, AutoStr2(sLine, (int)(pEnd - sLine)).c_str(), name);
        }

        return ;
    }

    //move to the next arg position
    sLine += 10;

#define MAX_ARG_NUM 5
    StringPiece args[MAX_ARG_NUM];
    int narg = 0;
    const char *argBegin, *argEnd, *pError;

    skip_leading_space(&sLine);

    while (narg < MAX_ARG_NUM && sLine < pEnd &&
           StringTool::parseNextArg(sLine, pEnd, argBegin, argEnd, pError) == 0)
    {
        args[narg ++].set(argBegin, argEnd - argBegin);
        skip_leading_space(&sLine);
    }

    MessageHandler *handler = g_pMainConf->driverFactory->message_handler();
    RewriteOptions::OptionScope scope;

    switch (level)
    {
    case LSI_SERVER_LEVEL:
        scope = RewriteOptions::kProcessScope;// customized at process level only (command-line flags)
        break;

    case LSI_VHOST_LEVEL:
        scope = RewriteOptions::kServerScope;// customized at server level (e.g. VirtualHost)
        break;

    case LSI_CONTEXT_LEVEL:
        scope = RewriteOptions::kQueryScope;// customized at query (query-param, request headers, response headers)
        break;

    default:
        scope = RewriteOptions::kProcessScope;
        break;
    }

    if (narg == 2 &&
        (net_instaweb::StringCaseEqual("LogDir", args[0]) ||
         net_instaweb::StringCaseEqual("FileCachePath", args[0])))
        InitDir(args[0], args[1]);

    // The directory has been prepared, but we haven't actually parsed the
    // directive yet.  That happens below in ParseAndSetOptions().
    pOption->ParseAndSetOptions(args, narg, handler, g_pMainConf->driverFactory,
                                scope);
}


static void *ParseConfig(const char *param, int paramLen,
                         void *_initial_config, int level, const char *name)
{
    if (CreateMainConf())
        return NULL;

    LsiRewriteOptions *pInitOption = (LsiRewriteOptions *) _initial_config;
    LsiRewriteOptions *pOption = NULL;

    if (pInitOption)
        pOption = pInitOption->Clone();
    else
        pOption = new LsiRewriteOptions(
            g_pMainConf->driverFactory->thread_system());

    if (!pOption)
        return NULL;

    if (!param)
        return (void *) pOption;

    const char *pEnd = param + paramLen;
    const char *pStart = param;
    const char *p;
    int len = 0;

    while (pStart < pEnd)
    {
        p = strchr(pStart, '\n');

        if (p)
            len = p - pStart;
        else
            len = pEnd - pStart;

        ParseOption(pOption, pStart, len, level, name);
        pStart += len + 1;
    }

    return (void *) pOption;
}

static void FreeConfig(void *_config)
{
    delete(LsiRewriteOptions *) _config;
}

static int ChildInit(lsi_cb_param_t *rec);
static int dummy_port = 0;
static int PostConfig(lsi_cb_param_t *rec)
{
    std::vector<SystemServerContext *> server_contexts;
    int vhost_count = g_api->get_vhost_count();

    for (int s = 0; s < vhost_count; s++)
    {
        const void *vhost = g_api->get_vhost(s);

        LsiRewriteOptions *vhost_option = (LsiRewriteOptions *)
                                          g_api->get_vhost_module_param(vhost, &MNAME);

        //Comment: when Amdin/html parse, this will be NULL, we need not to add it
        if (vhost_option != NULL)
        {
            g_pMainConf->driverFactory->SetMainConf(vhost_option);

            ps_vh_conf_t *cfg_s = new ps_vh_conf_t;
            cfg_s->serverContext = g_pMainConf->driverFactory->MakeLsiServerContext(
                                        "dummy_hostname", --dummy_port);

            cfg_s->serverContext->global_options()->Merge(*vhost_option);
            cfg_s->handler =
                g_pMainConf->driverFactory->message_handler();
                // LsiMessageHandler(pMainConf->driver_factory->thread_system()->NewMutex());
                //Why GoogleMessageHandler() but not LsMessageHandler

            if (cfg_s->serverContext->global_options()->enabled())
            {
                //GoogleMessageHandler handler;
                const char *file_cache_path =
                    cfg_s->serverContext->Config()->file_cache_path().c_str();

                if (file_cache_path[0] == '\0')
                {
                    g_api->log(NULL, LSI_LOG_ERROR,
                               "mod_pagespeed post_config ERROR, file_cache_path is NULL\n");
                    return LS_FAIL;
                }
                else if (!g_pMainConf->driverFactory->file_system()->IsDir(
                             file_cache_path, cfg_s->handler).is_true())
                {
                    g_api->log(NULL, LSI_LOG_ERROR,
                               "mod_pagespeed post_config ERROR, FileCachePath must be an writeable directory.\n");
                    return LS_FAIL;
                }

                g_api->log(NULL, LSI_LOG_DEBUG,
                           "mod_pagespeed post_config OK, file_cache_path is %s\n",
                           file_cache_path);
            }

            g_api->set_vhost_module_data(vhost, &MNAME, cfg_s);
            server_contexts.push_back(cfg_s->serverContext);
        }
    }


    GoogleString error_message = "";
    int error_index = -1;
    Statistics *global_statistics = NULL;

    g_api->log(NULL, LSI_LOG_DEBUG,
               "mod_pagespeed post_config call PostConfig()\n");
    g_pMainConf->driverFactory->PostConfig(
        server_contexts, &error_message, &error_index, &global_statistics);

    if (error_index != -1)
    {
        server_contexts[error_index]->message_handler()->Message(
            kError, "mod_pagespeed is enabled. %s", error_message.c_str());
        //g_api->log( NULL, LSI_LOG_ERROR, "mod_pagespeed is enabled. %s\n", error_message.c_str() );
        return LS_FAIL;
    }


    if (!server_contexts.empty())
    {
        IgnoreSigpipe();

        if (global_statistics == NULL)
            LsiRewriteDriverFactory::InitStats(
                g_pMainConf->driverFactory->statistics());

        g_pMainConf->driverFactory->LoggingInit();
        g_pMainConf->driverFactory->RootInit();
    }
    else
    {
        delete g_pMainConf->driverFactory;
        g_pMainConf->driverFactory = NULL;
    }

    //while debugging
    if (g_api->get_server_mode() == LSI_SERVER_MODE_FORGROUND)
        ChildInit(rec);


    return 0;
}

static int ChildInit(lsi_cb_param_t *rec)
{
    if (g_pMainConf->driverFactory == NULL)
        return 0;

    SystemRewriteDriverFactory::InitApr();
    g_pMainConf->driverFactory->LoggingInit();
    g_pMainConf->driverFactory->ChildInit();

    int vhostCount = g_api->get_vhost_count();

    for (int s = 0; s < vhostCount; s++)
    {
        const void *vhost = g_api->get_vhost(s);
        ps_vh_conf_t *cfg_s = (ps_vh_conf_t *) g_api->get_vhost_module_data(vhost,
                              &MNAME);

        if (cfg_s && cfg_s->serverContext)
        {
            cfg_s->proxyFetchFactory = new ProxyFetchFactory(cfg_s->serverContext);
            g_pMainConf->driverFactory->SetServerContextMessageHandler(
                cfg_s->serverContext);
        }
    }


    g_pMainConf->driverFactory->StartThreads();
    return 0;
}

void EventCb(void *session);
int CreateBaseFetch(PsMData *pMyData, lsi_session_t *session,
                         RequestContextPtr request_context)
{
    CHECK(pMyData->pNotifier == NULL);
    pMyData->pNotifier = new PipeNotifier();
    pMyData->pNotifier->initNotifier((Multiplexer *) g_api->get_multiplexer(),
                                     session);
    g_api->log(session, LSI_LOG_DEBUG,
               "[Module:ModPagespeed]ps_create_base_fetch created event notifier, session=%ld\n",
               (long) session);

    pMyData->ctx->baseFetch = new LsiBaseFetch(
        session, pMyData->pNotifier->getFdIn(), pMyData->cfg_s->serverContext,
        request_context,
        pMyData->ctx->preserveCachingHeaders);

    //When the base_fetch is good then set the callback, otherwise, may cause crash.
    if (pMyData->ctx->baseFetch)
    {
        pMyData->pNotifier->setCb(EventCb);
        return 0;
    }
    else
    {
        pMyData->pNotifier->uninitNotifier((Multiplexer *)
                                           g_api->get_multiplexer());
        delete pMyData->pNotifier;
        pMyData->pNotifier = NULL;
        return LS_FAIL;
    }
}

bool CheckPagespeedApplicable(PsMData *pMyData, lsi_session_t *session)
{
//     //Check status code is it is 200
//     if( g_api->get_status_code( session ) != 200 )
//     {
//         return false;
//     }

    // We can't operate on Content-Ranges.
    iovec iov[1];

    if (g_api->get_resp_header(session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0,
                               iov, 1) != 1)
    {
        g_api->log(session, LSI_LOG_DEBUG,
                   "[%s]Request not rewritten because: no Content-Type set.\n",
                   ModuleName);
        return false;
    }

    StringPiece str = StringPiece((const char *)(iov[0].iov_base),
                                  iov[0].iov_len);
    const ContentType *content_type =  MimeTypeToContentType(str);
    if (content_type == NULL || !content_type->IsHtmlLike())
    {
        g_api->log(session, LSI_LOG_DEBUG,
                   "[%s]Request not rewritten because:[%s] not 'html like' Content-Type.\n",
                   ModuleName, str.as_string().c_str());
        return false;
    }

    return true;
}

StringPiece net_instaweb::DetermineHost(lsi_session_t *session)
{
    const int maxLen = 512;
    char hostC[maxLen];
    g_api->get_req_var_by_id(session, LSI_REQ_VAR_SERVER_NAME, hostC, maxLen);
    StringPiece host = hostC;
    return host;
}

int net_instaweb::DeterminePort(lsi_session_t *session)
{
    const int maxLen = 12;
    int port = -1;
    char portC[maxLen];
    g_api->get_req_var_by_id(session, LSI_REQ_VAR_SERVER_PORT, portC, maxLen);
    StringPiece port_str = portC;
    bool ok = StringToInt(port_str, &port);

    if (!ok)
    {
        // Might be malformed port, or just IPv6 with no port specified.
        return LS_FAIL;
    }

    return port;
}


GoogleString DetermineUrl(lsi_session_t *session)
{
    int port = DeterminePort(session);
    GoogleString port_string;

    if ((IsHttps(session) && (port == 443 || port == -1)) ||
        (!IsHttps(session) && (port == 80 || port == -1)))
    {
        // No port specifier needed for requests on default ports.
        port_string = "";
    }
    else
        port_string = StrCat(":", IntegerToString(port));

    StringPiece host = DetermineHost(session);

    int uriLen = g_api->get_req_org_uri(session, NULL, 0);
    char *uri = new char[uriLen + 1];
    g_api->get_req_org_uri(session, uri, uriLen + 1);

    GoogleString rc = StrCat(IsHttps(session) ? "https://" : "http://",
                             host, port_string, uri);
    delete []uri;
    return rc;
}



// Wrapper around GetQueryOptions()
RewriteOptions *DetermineRequestOptions(
    lsi_session_t *session,
    const RewriteOptions *domain_options, /* may be null */
    RequestHeaders *request_headers,
    ResponseHeaders *response_headers,
    RequestContextPtr request_context,
    ps_vh_conf_t *cfg_s,
    GoogleUrl *url,
    GoogleString *pagespeed_query_params,
    GoogleString *pagespeed_option_cookies)
{
    // Sets option from request headers and url.
    RewriteQuery rewrite_query;

    if (!cfg_s->serverContext->GetQueryOptions(
            request_context, domain_options, url, request_headers,
            response_headers, &rewrite_query))
    {
        // Failed to parse query params or request headers.  Treat this as if there
        // were no query params given.
        g_api->log(session, LSI_LOG_ERROR,
                   "ps_route request: parsing headers or query params failed.\n");
        return NULL;
    }

    *pagespeed_query_params =
        rewrite_query.pagespeed_query_params().ToEscapedString();
    *pagespeed_option_cookies =
        rewrite_query.pagespeed_option_cookies().ToEscapedString();

    // Will be NULL if there aren't any options set with query params or in
    // headers.
    return rewrite_query.ReleaseOptions();
}

// Check whether this visitor is already in an experiment.  If they're not,
// classify them into one by setting a cookie.  Then set options appropriately
// for their experiment.
//
// See InstawebContext::SetExperimentStateAndCookie()
bool SetExperimentStateAndCookie(lsi_session_t *session,
                                        ps_vh_conf_t *cfg_s,
                                        RequestHeaders *request_headers,
                                        RewriteOptions *options,
                                        const StringPiece &host)
{
    CHECK(options->running_experiment());
    bool need_cookie = cfg_s->serverContext->experiment_matcher()->
                       ClassifyIntoExperiment(*request_headers, options);

    if (need_cookie && host.length() > 0)
    {
        PosixTimer timer;
        int64 time_now_ms = timer.NowMs();
        int64 expiration_time_ms = (time_now_ms +
                                    options->experiment_cookie_duration_ms());

        // TODO(jefftk): refactor SetExperimentCookie to expose the value we want to
        // set on the cookie.
        int state = options->experiment_id();
        GoogleString expires;
        ConvertTimeToString(expiration_time_ms, &expires);
        GoogleString value = StringPrintf(
                                 "%s=%s; Expires=%s; Domain=.%s; Path=/",
                                 experiment::kExperimentCookie,
                                 experiment::ExperimentStateToCookieString(state).c_str(),
                                 expires.c_str(), host.as_string().c_str());

        g_api->set_resp_header(session, LSI_RESP_HEADER_SET_COOKIE, NULL, 0,
                               value.c_str(), value.size(), LSI_HEADER_MERGE);
    }

    return true;
}

// There are many sources of options:
//  - the request (query parameters, headers, and cookies)
//  - location block
//  - global server options
//  - experiment framework
// Consider them all, returning appropriate options for this request, of which
// the caller takes ownership.  If the only applicable options are global,
// set options to NULL so we can use server_context->global_options().
bool DetermineOptions(lsi_session_t *session,
                          RequestHeaders *request_headers,
                          ResponseHeaders *response_headers,
                          RewriteOptions *options,
                          RequestContextPtr request_context,
                          ps_vh_conf_t *cfg_s,
                          GoogleUrl *url,
                          GoogleString *pagespeed_query_params,
                          GoogleString *pagespeed_option_cookies,
                          bool html_rewrite)
{
    // Request-specific options, nearly always null.  If set they need to be
    // rebased on the directory options.
    RewriteOptions *request_options = DetermineRequestOptions(
                                          session, options, request_headers,
                                          response_headers, request_context,
                                          cfg_s, url, pagespeed_query_params,
                                          pagespeed_option_cookies);
    bool have_request_options = request_options != NULL;

    // Modify our options in response to request options if specified.
    if (have_request_options)
    {
        options->Merge(*request_options);
        delete request_options;
        request_options = NULL;
    }

    // If we're running an experiment and processing html then modify our options
    // in response to the experiment.  Except we generally don't want experiments
    // to be contaminated with unexpected settings, so ignore experiments if we
    // have request-specific options.  Unless EnrollExperiment is on, probably set
    // by a query parameter, in which case we want to go ahead and apply the
    // experimental settings even if it means bad data, because we're just seeing
    // what it looks like.
    if (options->running_experiment() &&
        html_rewrite &&
        (!have_request_options ||
         options->enroll_experiment()))
    {
        bool ok = SetExperimentStateAndCookie(
                      session, cfg_s, request_headers, options, url->Host());
        if (!ok)
            return false;
    }

    return true;
}


// Fix URL based on X-Forwarded-Proto.
// http://code.google.com/p/modpagespeed/issues/detail?id=546 For example, if
// Apache gives us the URL "http://www.example.com/" and there is a header:
// "X-Forwarded-Proto: https", then we update this base URL to
// "https://www.example.com/".  This only ever changes the protocol of the url.
//
// Returns true if it modified url, false otherwise.
bool ApplyXForwardedProto(lsi_session_t *session, GoogleString *url)
{
    int valLen = 0;
    const char *buf = g_api->get_req_header_by_id(session,
                      LSI_REQ_HEADER_X_FORWARDED_FOR, &valLen);

    if (valLen == 0 || buf == NULL)
    {
        return false;  // No X-Forwarded-Proto header found.
    }

    AutoStr2 bufStr(buf);
    StringPiece x_forwarded_proto =
        StrToStringPiece(bufStr);

    if (!STR_CASE_EQ_LITERAL(bufStr, "http") &&
        !STR_CASE_EQ_LITERAL(bufStr, "https"))
    {
        LOG(WARNING) << "Unsupported X-Forwarded-Proto: " << x_forwarded_proto
                     << " for URL " << url << " protocol not changed.";
        return false;
    }

    StringPiece url_sp(*url);
    StringPiece::size_type colon_pos = url_sp.find(":");

    if (colon_pos == StringPiece::npos)
    {
        return false;  // URL appears to have no protocol; give up.
    }

//
    // Replace URL protocol with that specified in X-Forwarded-Proto.
    *url = StrCat(x_forwarded_proto, url_sp.substr(colon_pos));

    return true;
}

bool IsPagespeedSubrequest(lsi_session_t *session, const char *ua,
                             int &uaLen)
{
    if (ua && uaLen > 0)
    {
        if (memmem(ua, uaLen, kModPagespeedSubrequestUserAgent,
                   strlen(kModPagespeedSubrequestUserAgent)))
        {
            g_api->log(session, LSI_LOG_DEBUG,
                       "[Module:ModPagespeed]Request not rewritten because: User-Agent appears to be %s.\n",
                       kModPagespeedSubrequestUserAgent);
            return true;
        }
    }

    return false;
}

void BeaconHandlerHelper(PsMData *pMyData, lsi_session_t *session,
                              StringPiece beacon_data)
{
    g_api->log(session, LSI_LOG_DEBUG,
               "ps_beacon_handler_helper: beacon[%d] %s",
               beacon_data.size(),  beacon_data.data());

    StringPiece user_agent = StringPiece(pMyData->userAgent, pMyData->uaLen);
    CHECK(pMyData->cfg_s != NULL);

    RequestContextPtr request_context(
        pMyData->cfg_s->serverContext->NewRequestContext(session));

    request_context->set_options(
        pMyData->cfg_s->serverContext->global_options()->ComputeHttpOptions());

    pMyData->cfg_s->serverContext->HandleBeacon(
        beacon_data,
        user_agent,
        request_context);

    SetCacheControl(session, const_cast<char *>("max-age=0, no-cache"));
}

// Parses out query params from the request.
//isPost: 1, use post bosy, 0, use query param
void QueryParamsHandler(lsi_session_t *session, StringPiece *data,
                             int isPost)
{
    if (!isPost)
    {
        int iQSLen;
        const char *pQS = g_api->get_req_query_string(session, &iQSLen);

        if (iQSLen == 0)
            *data = "";
        else
            (*data) = StringPiece(pQS, iQSLen);
    }
    else
    {
        //Then req body
        CHECK(g_api->is_req_body_finished(session));
        char buf[1024];
        int ret;
        bool bReqBodyStart = false;

        while ((ret = g_api->read_req_body(session, buf, 1024)) > 0)
        {
            if (!bReqBodyStart)
            {
                GoogleString sp("&", 1);
                (*data).AppendToString(&sp);
                bReqBodyStart = true;
            }

            GoogleString sbuf(buf, ret);
            (*data).AppendToString(&sbuf);
        }
    }
}


int BeaconHandler(PsMData *pMyData, lsi_session_t *session)
{
    // Use query params.
    StringPiece query_param_beacon_data;
    int isPost = (pMyData->method == HTTP_POST);
    QueryParamsHandler(session, &query_param_beacon_data, isPost);
    BeaconHandlerHelper(pMyData, session, query_param_beacon_data);
    pMyData->statusCode = (isPost ? 200 : 204);
    return 0;
}

int SimpleHandler(PsMData *pMyData, lsi_session_t *session,
                      LsServerContext *server_context,
                      RequestRouting::Response response_category)
{
    LsiRewriteDriverFactory *factory =
        static_cast<LsiRewriteDriverFactory *>(
            server_context->factory());
    LsiMessageHandler *message_handler = factory->GetLsiMessageHandler();

    int uriLen = g_api->get_req_org_uri(session, NULL, 0);
    char *uri = new char[uriLen + 1];

    g_api->get_req_org_uri(session, uri, uriLen + 1);
    uri[uriLen] = 0x00;
    StringPiece request_uri_path = uri;//

    GoogleString &url_string = pMyData->urlString;
    GoogleUrl url(url_string);
    QueryParams query_params;

    if (url.IsWebValid())
        query_params.ParseFromUrl(url);

    GoogleString output;
    StringWriter writer(&output);
    pMyData->statusCode = 200;
    HttpStatus::Code status = HttpStatus::kOK;
    ContentType content_type = kContentTypeHtml;
    StringPiece cache_control = HttpAttributes::kNoCache;
    const char *error_message = NULL;

    switch (response_category)
    {
    case RequestRouting::kStaticContent:
        {
            StringPiece file_contents;

            if (!server_context->static_asset_manager()->GetAsset(
                    request_uri_path.substr(factory->static_asset_prefix().length()),
                    &file_contents, &content_type, &cache_control))
                return LS_FAIL;

            file_contents.CopyToString(&output);
            break;
        }

    case RequestRouting::kMessages:
        {
            GoogleString log;
            StringWriter log_writer(&log);

            if (!message_handler->Dump(&log_writer))
            {
                writer.Write("Writing to pagespeed_message failed. \n"
                             "Please check if it's enabled in pagespeed.conf.\n",
                             message_handler);
            }
            else
                HtmlKeywords::WritePre(log, "", &writer, message_handler);

            break;
        }

    default:
        g_api->log(session, LSI_LOG_WARN,
                   "ps_simple_handler: unknown RequestRouting.");
        return LS_FAIL;
    }

    if (error_message != NULL)
    {
        pMyData->statusCode = HttpStatus::kNotFound;
        content_type = kContentTypeHtml;
        output = error_message;
    }


    pMyData->respHeaders = new ResponseHeaders;
    ResponseHeaders &response_headers = *pMyData->respHeaders;
    response_headers.SetStatusAndReason(status);
    response_headers.set_major_version(1);
    response_headers.set_minor_version(1);
    response_headers.Add(HttpAttributes::kContentType,
                         content_type.mime_type());


//     g_api->set_resp_header( session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0, content_type.mime_type(),
//                             strlen( content_type.mime_type() ), LSI_HEADER_SET );

    // http://msdn.microsoft.com/en-us/library/ie/gg622941(v=vs.85).aspx
    // Script and styleSheet elements will reject responses with
    // incorrect MIME types if the server sends the response header
    // "X-Content-Type-Options: nosniff". This is a security feature
    // that helps prevent attacks based on MIME-type confusion.
    response_headers.Add("X-Content-Type-Options", "nosniff");
//     g_api->set_resp_header( session, -1, "X-Content-Type-Options", sizeof( "X-Content-Type-Options" ) - 1,
//                             "nosniff", 7, LSI_HEADER_SET );

    //TODO: add below
    int64 now_ms = factory->timer()->NowMs();
    response_headers.SetDate(now_ms);
    response_headers.SetLastModified(now_ms);
    response_headers.Add(HttpAttributes::kCacheControl, cache_control);
//     g_api->set_resp_header( session, -1, HttpAttributes::kCacheControl, strlen( HttpAttributes::kCacheControl ),
//                             cache_control.as_string().c_str(), cache_control.size(), LSI_HEADER_SET );

//   char* cache_control_s = string_piece_to_pool_string(r->pool, cache_control);
//   if (cache_control_s != NULL) {
    if (FindIgnoreCase(cache_control, "private") == StringPiece::npos)
        response_headers.Add(HttpAttributes::kEtag, "W/\"0\"");

    //g_api->append_body_buf(session, output.c_str(), output.size());
    ls_loopbuf_append(&pMyData->buff, output.c_str(), output.size());

    return 0;
}



int ResourceHandler(PsMData *pMyData,
                        lsi_session_t *session,
                        bool html_rewrite,
                        RequestRouting::Response response_category)
{
    ps_request_ctx_t *ctx = pMyData->ctx;
    ps_vh_conf_t *cfg_s = pMyData->cfg_s;
    CHECK(!(html_rewrite && (ctx == NULL || ctx->htmlRewrite == false)));

    if (!html_rewrite &&
        pMyData->method != HTTP_GET &&
        pMyData->method != HTTP_HEAD &&
        pMyData->method != HTTP_POST &&
        response_category != RequestRouting::kCachePurge)
        return LS_FAIL;

    GoogleString url_string = DetermineUrl(session);;
    GoogleUrl url(url_string);
    CHECK(url.IsWebValid());


    scoped_ptr<RequestHeaders> request_headers(new RequestHeaders);
    scoped_ptr<ResponseHeaders> response_headers(new ResponseHeaders);

    CopyReqHeadersFromServer(session, request_headers.get());
    CopyRespHeadersFromServer(session, response_headers.get());

    RequestContextPtr request_context(
        cfg_s->serverContext->NewRequestContext(session));
    LsiRewriteOptions *options = (LsiRewriteOptions *) g_api->get_module_param(
                                     session, &MNAME);;
    GoogleString pagespeed_query_params;
    GoogleString pagespeed_option_cookies;

    //Diffirent from google code, here the option is always inherit from the context, and it should never be NULL
    if (!DetermineOptions(session, request_headers.get(),
                              response_headers.get(),
                              options, request_context, cfg_s, &url,
                              &pagespeed_query_params, &pagespeed_option_cookies,
                              html_rewrite))
        return LS_FAIL;

    if (options == NULL)
    {
        //Never happen!!!
        CHECK(0);
        options = (LsiRewriteOptions *)cfg_s->serverContext->global_options();
    }

    if (!options->enabled())
    {
        // Disabled via query params or request headers.
        return LS_FAIL;
    }

    request_context->set_options(options->ComputeHttpOptions());

    // ps_determine_options modified url, removing any ModPagespeedFoo=Bar query
    // parameters.  Keep url_string in sync with url.
    url.Spec().CopyToString(&url_string);




    if (cfg_s->serverContext->global_options()->respect_x_forwarded_proto())
    {
        bool modified_url = ApplyXForwardedProto(session, &url_string);

        if (modified_url)
        {
            url.Reset(url_string);
            CHECK(url.IsWebValid()) << "The output of ps_apply_x_forwarded_proto"
                                    << " should always be a valid url because it only"
                                    << " changes the scheme between http and https.";
        }
    }

    bool pagespeed_resource =
        !html_rewrite && cfg_s->serverContext->IsPagespeedResource(url);
    bool is_an_admin_handler =
        response_category == RequestRouting::kStatistics ||
        response_category == RequestRouting::kGlobalStatistics ||
        response_category == RequestRouting::kConsole ||
        response_category == RequestRouting::kAdmin ||
        response_category == RequestRouting::kGlobalAdmin ||
        response_category == RequestRouting::kCachePurge;

    if (html_rewrite)
        ReleaseBaseFetch(pMyData);
    else
    {
        // create request ctx
        CHECK(ctx == NULL);
        ctx = new ps_request_ctx_t();

        ctx->session = session;
        ctx->htmlRewrite = false;
        ctx->inPlace = false;
        //ctx->pagespeed_connection = NULL;
        ctx->preserveCachingHeaders = kDontPreserveHeaders;

        if (!options->modify_caching_headers())
            ctx->preserveCachingHeaders = kPreserveAllCachingHeaders;
        else if (!options->IsDownstreamCacheIntegrationEnabled())
        {
            // Downstream cache integration is not enabled. Disable original
            // Cache-Control headers.
            ctx->preserveCachingHeaders = kDontPreserveHeaders;
        }
        else if (!pagespeed_resource && !is_an_admin_handler)
        {
            ctx->preserveCachingHeaders = kPreserveOnlyCacheControl;
            // Downstream cache integration is enabled. If a rebeaconing key has been
            // configured and there is a ShouldBeacon header with the correct key,
            // disable original Cache-Control headers so that the instrumented page is
            // served out with no-cache.
            StringPiece should_beacon(request_headers->Lookup1(kPsaShouldBeacon));

            if (options->MatchesDownstreamCacheRebeaconingKey(should_beacon))
                ctx->preserveCachingHeaders = kDontPreserveHeaders;
        }

        ctx->recorder = NULL;
        pMyData->urlString = url_string;
        pMyData->ctx = ctx;
    }

    if (CreateBaseFetch(pMyData, session, request_context) != 0)
        return LS_FAIL;
    ctx->baseFetch->SetRequestHeadersTakingOwnership(
        request_headers.release());


    bool page_callback_added = false;
    scoped_ptr<ProxyFetchPropertyCallbackCollector>
    property_callback(
        ProxyFetchFactory::InitiatePropertyCacheLookup(
            !html_rewrite /* is_resource_fetch */,
            url,
            cfg_s->serverContext,
            options,
            ctx->baseFetch,
            false /* requires_blink_cohort (no longer unused) */,
            &page_callback_added));

    if (pagespeed_resource)
    {
        ResourceFetch::Start(
            url,
            NULL,
            false /* using_spdy */, cfg_s->serverContext, ctx->baseFetch);

        return 1;
    }
    else if (is_an_admin_handler)
    {
        QueryParams query_params;
        query_params.ParseFromUrl(url);

        PosixTimer timer;
        int64 now_ms = timer.NowMs();
        ctx->baseFetch->response_headers()->SetDateAndCaching(
            now_ms, 0 /* max-age */, ", no-cache");

        if (response_category == RequestRouting::kStatistics ||
            response_category == RequestRouting::kGlobalStatistics)
        {
            cfg_s->serverContext->StatisticsPage(
                response_category == RequestRouting::kGlobalStatistics,
                query_params,
                cfg_s->serverContext->Config(),
                ctx->baseFetch);
        }
        else if (response_category == RequestRouting::kConsole)
        {
            cfg_s->serverContext->ConsoleHandler(
                *cfg_s->serverContext->Config(),
                AdminSite::kStatistics,
                query_params,
                ctx->baseFetch);
        }
        else if (response_category == RequestRouting::kAdmin ||
                 response_category == RequestRouting::kGlobalAdmin)
        {
            cfg_s->serverContext->AdminPage(
                response_category == RequestRouting::kGlobalAdmin,
                url,
                query_params,
                cfg_s->serverContext->Config(),
                ctx->baseFetch);
        }
        else if (response_category == RequestRouting::kCachePurge)
        {
            AdminSite *admin_site = cfg_s->serverContext->admin_site();
            admin_site->PurgeHandler(url_string,
                                     cfg_s->serverContext->cache_path(),
                                     ctx->baseFetch);
        }
        else
            CHECK(false);

        return 1;
    }

    if (html_rewrite)
    {
        // Do not store driver in request_context, it's not safe.
        RewriteDriver *driver;

        // If we don't have custom options we can use NewRewriteDriver which reuses
        // rewrite drivers and so is faster because there's no wait to construct
        // them.  Otherwise we have to build a new one every time.

        driver = cfg_s->serverContext->NewRewriteDriver(
                     ctx->baseFetch->request_context());

        StringPiece user_agent = ctx->baseFetch->request_headers()->Lookup1(
                                     HttpAttributes::kUserAgent);

        if (!user_agent.empty())
            driver->SetUserAgent(user_agent);

        driver->SetRequestHeaders(*ctx->baseFetch->request_headers());
        driver->set_pagespeed_query_params(pagespeed_query_params);
        driver->set_pagespeed_option_cookies(pagespeed_option_cookies);

        ctx->proxyFetch = cfg_s->proxyFetchFactory->CreateNewProxyFetch(
                               url_string, ctx->baseFetch, driver,
                               property_callback.release(),
                               NULL /* original_content_fetch */);

        g_api->log(NULL, LSI_LOG_DEBUG, "Create ProxyFetch %s.\n",
                   url_string.c_str());
        return 0;
    }

    if (options->in_place_rewriting_enabled() &&
        options->enabled() &&
        options->IsAllowed(url.Spec()))
    {
        // Do not store driver in request_context, it's not safe.
        RewriteDriver *driver;

        driver = cfg_s->serverContext->NewRewriteDriver(
                     ctx->baseFetch->request_context());


        StringPiece user_agent = ctx->baseFetch->request_headers()->Lookup1(
                                     HttpAttributes::kUserAgent);

        if (!user_agent.empty())
            driver->SetUserAgent(user_agent);

        driver->SetRequestHeaders(*ctx->baseFetch->request_headers());

        ctx->driver = driver;

        cfg_s->serverContext->message_handler()->Message(
            kInfo, "Trying to serve rewritten resource in-place: %s",
            url_string.c_str());

        ctx->inPlace = true;
        ctx->baseFetch->SetIproLookup(true);
        ctx->driver->FetchInPlaceResource(
            url, false /* proxy_mode */, ctx->baseFetch);

        return 1;
    }

    //NEVER RUN TO THIS POINT BECAUSE THE PREVOIUS CHEKCING NBYPASS THIS ONE
    // NOTE: We are using the below debug message as is for some of our system
    // tests. So, be careful about test breakages caused by changing or
    // removing this line.
    g_api->log(session, LSI_LOG_DEBUG,
               "Passing on content handling for non-pagespeed resource '%s'\n",
               url_string.c_str());

    ctx->baseFetch->Done(false);
    ReleaseBaseFetch(pMyData);
    // set html_rewrite flag.
    ctx->htmlRewrite = true;
    return LS_FAIL;
}



// Send each buffer in the chain to the proxyFetch for optimization.
// Eventually it will make it's way, optimized, to base_fetch.
void SendToPagespeed(PsMData *pMyData, lsi_cb_param_t *rec,
                          ps_request_ctx_t *ctx)
{
    if (ctx->proxyFetch == NULL)
        return ;

    CHECK(ctx->proxyFetch != NULL);
    ctx->proxyFetch->Write(
        StringPiece((const char *) rec->_param, rec->_param_len),
        pMyData->cfg_s->handler);

    if (rec->_flag_in & LSI_CB_FLAG_IN_EOF)
    {
        ctx->proxyFetch->Done(true /* success */);
        ctx->proxyFetch = NULL;  // ProxyFetch deletes itself on Done().
    }
    else
        ctx->proxyFetch->Flush(pMyData->cfg_s->handler);
}

void StripHtmlHeaders(lsi_session_t *session)
{
    g_api->remove_resp_header(session, LSI_RESP_HEADER_CONTENT_LENGTH, NULL, 0);
    g_api->remove_resp_header(session, LSI_RESP_HEADER_ACCEPT_RANGES, NULL, 0);
}

int HtmlRewriteFixHeadersFilter(PsMData *pMyData,
                                lsi_session_t *session, ps_request_ctx_t *ctx)
{
    if (ctx == NULL || !ctx->htmlRewrite
        || ctx->preserveCachingHeaders == kPreserveAllCachingHeaders)
        return 0;

    if (ctx->preserveCachingHeaders == kDontPreserveHeaders)
    {
        // Don't cache html.  See mod_instaweb:instaweb_fix_headers_filter.
        LsiCachingHeaders caching_headers(session);
        SetCacheControl(session,
                        (char *) caching_headers.GenerateDisabledCacheControl().c_str());
    }

    // Pagespeed html doesn't need etags: it should never be cached.
    g_api->remove_resp_header(session, LSI_RESP_HEADER_ETAG, NULL, 0);

    // An html page may change without the underlying file changing, because of
    // how resources are included.  Pagespeed adds cache control headers for
    // resources instead of using the last modified header.
    g_api->remove_resp_header(session, LSI_RESP_HEADER_LAST_MODIFIED, NULL, 0);

//   // Clear expires
//   if (r->headers_out.expires) {
//     r->headers_out.expires->hash = 0;
//     r->headers_out.expires = NULL;
//   }

    return 0;
}

int InPlaceCheckHeaderFilter(PsMData *pMyData, lsi_session_t *session,
                             ps_request_ctx_t *ctx)
{

    if (ctx->recorder != NULL)
    {
        g_api->log(session, LSI_LOG_DEBUG,
                   "ps in place check header filter recording: %s",
                   pMyData->urlString.c_str());

        CHECK(!ctx->inPlace);

        // The recorder will do this checking, so pass it the headers.
        ResponseHeaders response_headers;
        CopyRespHeadersFromServer(session, &response_headers);
        ctx->recorder->ConsiderResponseHeaders(
            InPlaceResourceRecorder::kPreliminaryHeaders, &response_headers);
        return 0;
    }

    if (!ctx->inPlace)
        return 0;

    g_api->log(session, LSI_LOG_DEBUG,
               "[Module:ModPagespeed]ps in place check header filter initial: %s\n",
               pMyData->urlString.c_str());

    int status_code =
        pMyData->ctx->baseFetch->response_headers()->status_code();
    //g_api->get_status_code( session );
    bool status_ok = (status_code != 0) && (status_code < 400);

    ps_vh_conf_t *cfg_s = pMyData->cfg_s;
    LsServerContext *server_context = cfg_s->serverContext;
    MessageHandler *message_handler = cfg_s->handler;
    GoogleString url = DetermineUrl(session);
    // The URL we use for cache key is a bit different since it may
    // have PageSpeed query params removed.
    GoogleString cache_url = pMyData->urlString;

    // continue process
    if (status_ok)
    {
        ctx->inPlace = false;

        server_context->rewrite_stats()->ipro_served()->Add(1);
        message_handler->Message(
            kInfo, "Serving rewritten resource in-place: %s",
            url.c_str());

        return 0;
    }

    if (status_code == CacheUrlAsyncFetcher::kNotInCacheStatus)
    {
        server_context->rewrite_stats()->ipro_not_in_cache()->Add(1);
        server_context->message_handler()->Message(
            kInfo,
            "Could not rewrite resource in-place "
            "because URL is not in cache: %s",
            cache_url.c_str());
        const SystemRewriteOptions *options = SystemRewriteOptions::DynamicCast(
                ctx->driver->options());

        RequestContextPtr request_context(
            cfg_s->serverContext->NewRequestContext(session));
        request_context->set_options(options->ComputeHttpOptions());

        RequestHeaders request_headers;
        CopyReqHeadersFromServer(session, &request_headers);
        // This URL was not found in cache (neither the input resource nor
        // a ResourceNotCacheable entry) so we need to get it into cache
        // (or at least a note that it cannot be cached stored there).
        // We do that using an Apache output filter.
        ctx->recorder = new InPlaceResourceRecorder(
            request_context,
            cache_url,
            ctx->driver->CacheFragment(),
            request_headers.GetProperties(),
            options->ipro_max_response_bytes(),
            options->ipro_max_concurrent_recordings(),
            server_context->http_cache(),
            server_context->statistics(),
            message_handler);
//     // set in memory flag for in place_body_filter
//     r->filter_need_in_memory = 1;

        // We don't have the response headers at all yet because we haven't yet gone
        // to the backend.


        /************************
         *
         * DIFF FROM NGX
         *
         *
         */
        ResponseHeaders response_headers;
        CopyRespHeadersFromServer(session, &response_headers);
        ctx->recorder->ConsiderResponseHeaders(
            InPlaceResourceRecorder::kPreliminaryHeaders, &response_headers);
        /*******************************
         *
         *
         */

    }
    else
    {
        server_context->rewrite_stats()->ipro_not_rewritable()->Add(1);
        message_handler->Message(kInfo,
                                 "Could not rewrite resource in-place: %s",
                                 url.c_str());
    }

    ctx->driver->Cleanup();
    ctx->driver = NULL;
    // enable html_rewrite
    ctx->htmlRewrite = true;
    ctx->inPlace = false;

    // re init ctx
    ctx->fetchDone = false;

    ReleaseBaseFetch(pMyData);

    return LS_FAIL;
}

int InPlaceBodyFilter(PsMData *pMyData, lsi_cb_param_t *rec,
                            ps_request_ctx_t *ctx, ps_vh_conf_t *cfg_s)
{
    if (ctx == NULL || ctx->recorder == NULL)
        return 1;

    g_api->log(rec->_session, LSI_LOG_DEBUG,
               "[Module:ModPagespeed]ps in place body filter: %s\n",
               pMyData->urlString.c_str());

    InPlaceResourceRecorder *recorder = ctx->recorder;
    StringPiece contents((char *) rec->_param, rec->_param_len);

    recorder->Write(contents, recorder->handler());

    if (rec->_flag_in & LSI_CB_FLAG_IN_FLUSH)
        recorder->Flush(recorder->handler());

    if (rec->_flag_in & LSI_CB_FLAG_IN_EOF || recorder->failed())
    {
        ResponseHeaders response_headers;
        CopyRespHeadersFromServer(rec->_session, &response_headers);
        ctx->recorder->DoneAndSetHeaders(&response_headers);
        ctx->recorder = NULL;
    }

    return 0;
}


int HtmlRewriteBodyFilter(PsMData *pMyData, lsi_cb_param_t *rec,
                                ps_request_ctx_t *ctx, ps_vh_conf_t *cfg_s)
{
    int ret = 1;

    if (ctx->htmlRewrite)
    {
        if (rec->_param_len > 0 ||  rec->_flag_in)
            SendToPagespeed(pMyData, rec, ctx);

        ret = 0;
    }

    return ret;
}

int BodyFilter(lsi_cb_param_t *rec)
{
    ps_vh_conf_t *cfg_s;
    ps_request_ctx_t *ctx;
    PsMData *pMyData = (PsMData *) g_api->get_module_data(rec->_session,
                       &MNAME, LSI_MODULE_DATA_HTTP);

    if (pMyData == NULL || (cfg_s = pMyData->cfg_s) == NULL
        || (ctx = pMyData->ctx) == NULL)
        return g_api->stream_write_next(rec, (const char *) rec->_param,
                                        rec->_param_len);


//      int ret1 = ps_in_place_body_filter(pMyData, rec, ctx, cfg_s );
//      //ps_base_fetch_filter();
//      int ret2 = ps_html_rewrite_body_filter(pMyData, rec, ctx, cfg_s );

//     //disorder
    int ret1 = HtmlRewriteBodyFilter(pMyData, rec, ctx, cfg_s);
//     //ps_base_fetch_filter(pMyData);
    InPlaceBodyFilter(pMyData, rec, ctx, cfg_s);


    if (ret1 != 0 || ctx->baseFetch == NULL)
        return g_api->stream_write_next(rec, (const char *) rec->_param,
                                        rec->_param_len);


    ls_loopbuf_straight(&pMyData->buff);
    int len;
    int writtenTotal = 0;

    while ((len = ls_loopbuf_size(&pMyData->buff)) > 0)
    {
        char *buf = ls_loopbuf_begin(&pMyData->buff);

        if (pMyData->doneCalled)
            rec->_flag_in = LSI_CB_FLAG_IN_EOF;
        else
            rec->_flag_in = LSI_CB_FLAG_IN_FLUSH;

        int written = g_api->stream_write_next(rec, buf, len);
        ls_loopbuf_popfront(&pMyData->buff, written);
        writtenTotal += written;
    }

    if (!pMyData->doneCalled)
    {
        if (rec->_flag_out)
            *rec->_flag_out |= LSI_CB_FLAG_OUT_BUFFERED_DATA;
    }
    else
    {
        if (!pMyData->endRespCalled)
        {
            pMyData->endRespCalled = true;
            g_api->end_resp(rec->_session);
        }
    }

    g_api->log(rec->_session, LSI_LOG_DEBUG,
               "[%s]ps_body_filter flag_in %d, flag out %d, done_called %d, Accumulated %d, wriiten %d.\n",
               ModuleName, rec->_flag_in, *rec->_flag_out, pMyData->doneCalled,
               rec->_param_len, writtenTotal);
    return rec->_param_len;

}


void UpdateEtag(lsi_cb_param_t *rec)
{
    struct iovec iov[1] = {{NULL, 0}};
    int iovCount = g_api->get_resp_header(rec->_session, -1, kInternalEtagName,
                                          strlen(kInternalEtagName), iov, 1);

    if (iovCount == 1)
    {
        g_api->remove_resp_header(rec->_session, -1, kInternalEtagName,
                                  strlen(kInternalEtagName));
        g_api->set_resp_header(rec->_session, LSI_RESP_HEADER_ETAG, NULL, 0,
                               (const char *) iov[0].iov_base,
                               iov[0].iov_len, LSI_HEADER_SET);
    }
}

//TODO: comment out seems to be useless
// int ps_base_fetch_filter(PsMData *pMyData, lsi_cb_param_t *rec, ps_request_ctx_t* ctx)
// {
//   if (ctx == NULL || ctx->base_fetch == NULL)
//     return 0;
//
//   g_api->log(rec->_session, LSI_LOG_DEBUG,
//                "http pagespeed write filter: %s", ctx->url_string.c_str());
//
//   return 0;
// }


//Check the resp header and set the htmlWrite
int HtmlRewriteHeaderFilter(PsMData *pMyData, lsi_session_t *session,
                                  ps_request_ctx_t *ctx, ps_vh_conf_t *cfg_s)
{
    // Poll for cache flush on every request (polls are rate-limited).
    //cfg_s->server_context->FlushCacheIfNecessary();

    if (ctx->htmlRewrite == false)
        return 0;

    if (!CheckPagespeedApplicable(pMyData, session))
    {
        ctx->htmlRewrite = false;
        return 0;
    }

    g_api->log(session, LSI_LOG_DEBUG,
               "[Module:ModPagespeed]http pagespeed html rewrite header filter \"%s\"\n",
               pMyData->urlString.c_str());


    int rc = ResourceHandler(pMyData, session, true,
                                 RequestRouting::kResource);

    if (rc != 0)
    {
        ctx->htmlRewrite = false;
        return 0;
    }

    StripHtmlHeaders(session);
    CopyRespHeadersFromServer(session,
                                      ctx->baseFetch->response_headers());

    return 0;
}

int HeaderFilter(lsi_cb_param_t *rec)
{
    UpdateEtag(rec);

    ps_vh_conf_t *cfg_s;
    ps_request_ctx_t *ctx;
    PsMData *pMyData = (PsMData *) g_api->get_module_data(rec->_session,
                       &MNAME, LSI_MODULE_DATA_HTTP);

    if (pMyData == NULL || (cfg_s = pMyData->cfg_s) == NULL
        || (ctx = pMyData->ctx) == NULL)
        return 0;

    if (IsPagespeedSubrequest(rec->_session, pMyData->userAgent,
                                pMyData->uaLen))
        return 0;

    HtmlRewriteHeaderFilter(pMyData, rec->_session, ctx, cfg_s);
    HtmlRewriteFixHeadersFilter(pMyData, rec->_session, ctx);
    InPlaceCheckHeaderFilter(pMyData, rec->_session, ctx);

    return 0;
}


// Set us up for processing a request.  Creates a request context and determines
// which handler should deal with the request.
RequestRouting::Response RouteRequest(PsMData *pMyData,
        lsi_session_t  *session, bool is_resource_fetch)
{
    ps_vh_conf_t *cfg_s = pMyData->cfg_s;

    if (!cfg_s->serverContext->global_options()->enabled())
    {
        // Not enabled for this server block.
        return RequestRouting::kPagespeedDisabled;
    }

    GoogleString url_string = DetermineUrl(session);
    GoogleUrl url(url_string);

    if (!url.IsWebValid())
    {
        g_api->log(session, LSI_LOG_ERROR, "[%s]invalid url \"%s\".\n",
                   ModuleName, url_string.c_str());
        return RequestRouting::kInvalidUrl;
    }

    if (IsPagespeedSubrequest(session, pMyData->userAgent, pMyData->uaLen))
        return RequestRouting::kPagespeedSubrequest;
    else if (
        url.PathSansLeaf() == dynamic_cast<LsiRewriteDriverFactory *>(
            cfg_s->serverContext->factory())->static_asset_prefix())
        return RequestRouting::kStaticContent;

    const LsiRewriteOptions *global_options = cfg_s->serverContext->Config();
    StringPiece path = url.PathSansQuery();

    if (StringCaseEqual(path, global_options->GetStatisticsPath()))
        return RequestRouting::kStatistics;
    else if (StringCaseEqual(path, global_options->GetGlobalStatisticsPath()))
        return RequestRouting::kGlobalStatistics;
    else if (StringCaseEqual(path, global_options->GetConsolePath()))
        return RequestRouting::kConsole;
    else if (StringCaseEqual(path, global_options->GetMessagesPath()))
        return RequestRouting::kMessages;
    else if ( // The admin handlers get everything under a path (/path/*) while
        // all the other handlers only get exact matches (/path).  So match
        // all paths starting with the handler path.
        !global_options->GetAdminPath().empty() &&
        StringCaseStartsWith(path, global_options->GetAdminPath()))
        return RequestRouting::kAdmin;
    else if (!global_options->GetGlobalAdminPath().empty() &&
             StringCaseStartsWith(path, global_options->GetGlobalAdminPath()))
        return RequestRouting::kGlobalAdmin;
    else if (global_options->enable_cache_purge() &&
             !global_options->purge_method().empty() &&
             pMyData->method == HTTP_PURGE)
        return RequestRouting::kCachePurge;

    const GoogleString *beacon_url;

    if (IsHttps(session))
        beacon_url = & (global_options->beacon_url().https);
    else
        beacon_url = & (global_options->beacon_url().http);

    if (url.PathSansQuery() == StringPiece(*beacon_url))
        return RequestRouting::kBeacon;

    return RequestRouting::kResource;
}

//Now only when BEACON this hook will be enabled, so that do not check
static int UriMapFilter(lsi_cb_param_t *rec)
{
    PsMData *pMyData = (PsMData *) g_api->get_module_data(rec->_session,
                       &MNAME, LSI_MODULE_DATA_HTTP);
    int ret = BeaconHandler(pMyData, rec->_session);
    if (ret == 0)
    {
        //statuc_code already set.
        g_api->register_req_handler(rec->_session, &MNAME, 0);
        g_api->log(rec->_session, LSI_LOG_DEBUG,
                   "[%s]ps_uri_map_filter register_req_handler OK.\n",
                   ModuleName);
    }
    return 0;
}

static int RecvReqHeaderCheck(lsi_cb_param_t *rec)
{
    //init the VHost data
    ps_vh_conf_t *cfg_s = (ps_vh_conf_t *) g_api->get_module_data(
                              rec->_session, &MNAME, LSI_MODULE_DATA_VHOST);

    if (cfg_s == NULL || cfg_s->serverContext == NULL)
    {
        g_api->log(rec->_session, LSI_LOG_ERROR,
                   "[%s]recv_req_header_check error, cfg_s == NULL || cfg_s->server_context == NULL.\n",
                   ModuleName);
        return LSI_HK_RET_OK;
    }

    LsiRewriteOptions *pConfig = (LsiRewriteOptions *) g_api->get_module_param(
                                     rec->_session, &MNAME);

    if (!pConfig)
    {
        g_api->log(rec->_session, LSI_LOG_ERROR,
                   "[%s]recv_req_header_check error 2.\n", ModuleName);
        return LSI_HK_RET_OK;
    }

    if (!pConfig->enabled())
    {
        g_api->log(rec->_session, LSI_LOG_DEBUG,
                   "[%s]recv_req_header_check returned [Not enabled].\n",
                   ModuleName);
        return LSI_HK_RET_OK;
    }

    char httpMethod[10] = {0};
    int methodLen = g_api->get_req_var_by_id(rec->_session,
                    LSI_REQ_VAR_REQ_METHOD, httpMethod, 10);
    int method = HTTP_UNKNOWN;

    switch (methodLen)
    {
    case 3:
        if ((httpMethod[0] | 0x20) == 'g')     //"GET"
            method = HTTP_GET;

        break;

    case 4:
        if ((httpMethod[0] | 0x20) == 'h')     //"HEAD"
            method = HTTP_HEAD;
        else if ((httpMethod[0] | 0x20) == 'p')     //"POST"
            method = HTTP_POST;

        break;

    case 5:
        if ((httpMethod[0] | 0x20) == 'p')     //"PURGE"
            method = HTTP_PURGE;
        break;

    default:
        break;
    }

    if (method == HTTP_UNKNOWN)
    {
        g_api->log(rec->_session, LSI_LOG_DEBUG,
                   "[%s]recv_req_header_check returned, method %s.\n",
                   ModuleName,
                   httpMethod);
        return 0;
    }

    //cfg_s->server_context->FlushCacheIfNecessary();

    //If URI_MAP called before, should release then first
    PsMData *pMyData = (PsMData *) g_api->get_module_data(rec->_session,
                       &MNAME, LSI_MODULE_DATA_HTTP);

    if (pMyData)
        g_api->free_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP,
                                ReleaseMydata);

    pMyData = new PsMData;

    if (pMyData == NULL)
    {
        g_api->log(rec->_session, LSI_LOG_DEBUG,
                   "[%s]recv_req_header_check returned, pMyData == NULL.\n",
                   ModuleName);
        return LSI_HK_RET_OK;
    }

    pMyData->ctx = NULL;
    pMyData->cfg_s = NULL;
    pMyData->userAgent = NULL;
    pMyData->uaLen = 0;
    pMyData->statusCode = 0;
    pMyData->respHeaders = NULL;
    pMyData->pNotifier = NULL;
    pMyData->endRespCalled = 0;
    pMyData->doneCalled = 0;
    ls_loopbuf(&pMyData->buff, 4096);
    pMyData->urlString = DetermineUrl(rec->_session);
    pMyData->method = (HTTP_METHOD) method;
    pMyData->cfg_s = cfg_s;
    pMyData->userAgent = g_api->get_req_header_by_id(rec->_session,
                          LSI_REQ_HEADER_USERAGENT, &pMyData->uaLen);
    pMyData->notifier_pointer = 0;
    g_api->set_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP,
                           pMyData);
    RequestRouting::Response response_category =
        RouteRequest(pMyData, rec->_session, true);

    int ret;
    g_api->set_req_wait_full_body(rec->_session);

    //Disable cache module
    g_api->set_req_env(rec->_session, "cache-control", 13, "no-cache", 8);

    int iEnableHkpt;
    switch (response_category)
    {
    case RequestRouting::kBeacon:
        iEnableHkpt = LSI_HKPT_URI_MAP;
        g_api->set_session_hook_enable_flag(rec->_session, &MNAME, 1,
                                            &iEnableHkpt, 1);
        //Go to URI_MAP and next step and this is the reason of
        //why URI_MAP does not check the response_category, if
        //in the future we need to do more thing in URI_MAP, check.
        break;

    case RequestRouting::kStaticContent:
    case RequestRouting::kMessages:
        ret = SimpleHandler(pMyData, rec->_session, cfg_s->serverContext,
                                response_category);
        if (ret == 0)
        {
            g_api->register_req_handler(rec->_session, &MNAME, 0);
            g_api->log(rec->_session, LSI_LOG_DEBUG,
                       "[%s]recv_req_header_check register_req_handler OK after call ps_simple_handler.\n",
                       ModuleName);
        }
        break;

    case RequestRouting::kStatistics:
    case RequestRouting::kGlobalStatistics:
    case RequestRouting::kConsole:
    case RequestRouting::kAdmin:
    case RequestRouting::kGlobalAdmin:
    case RequestRouting::kCachePurge:
    case RequestRouting::kResource:
        ret = ResourceHandler(pMyData, rec->_session, false,
                                  response_category);
        if (ret == 1) //suspended
        {
            g_api->log(rec->_session, LSI_LOG_DEBUG,
                       "[%s]recv_req_header_check suspend hook.\n", ModuleName);
            pMyData->notifier_pointer = g_api->set_event_notifier(rec->_session,
                                        &MNAME, LSI_HKPT_RECV_REQ_HEADER);
            return LSI_HK_RET_SUSPEND;
        }
        break;

    default:
        break;
    }

    return LSI_HK_RET_OK;
}


int BaseFetchHandler(PsMData *pMyData, lsi_session_t *session)
{
    ps_request_ctx_t *ctx = pMyData->ctx;
    int rc = ctx->baseFetch->CollectAccumulatedWrites(session);
    g_api->log(session, LSI_LOG_DEBUG,
               "ps_base_fetch_handler called CollectAccumulatedWrites, ret %d, %ld\n",
               rc, (long) session);

    if (rc == LSI_HK_RET_OK)
        ctx->fetchDone = true;

    return 0;
}

//This event shoule occur only once!
void EventCb(void *session_)
{
    lsi_session_t *session = (lsi_session_t *) session_;
    PsMData *pMyData = (PsMData *) g_api->get_module_data(session, &MNAME,
                       LSI_MODULE_DATA_HTTP);

    if (pMyData->ctx->baseFetch)
    {
        if (!pMyData->ctx->fetchDone)
            BaseFetchHandler(pMyData, session);
        else
        {
            CHECK(0);//should NEVER BE HERE since call only once
            pMyData->ctx->baseFetch->CollectAccumulatedWrites(session);
        }
    }

    g_api->log(session, LSI_LOG_DEBUG,
               "[%s]ps_event_cb triggered, session=%ld\n",
               ModuleName, (long) session_);

    //For suspended case, use the following to resume
    if (pMyData->notifier_pointer)
    {
        int status_code =
            pMyData->ctx->baseFetch->response_headers()->status_code();
        bool status_ok = (status_code != 0) && (status_code < 400);
        if (status_ok)
        {
            pMyData->statusCode = status_code;
            g_api->register_req_handler(session, &MNAME, 0);
            g_api->log(session, LSI_LOG_DEBUG,
                       "[%s]ps_event_cb register_req_handler OK.\n", ModuleName);
        }
        g_api->notify_event_notifier(&pMyData->notifier_pointer);

        /**
         * Do not call remove_event_notifier because after notify_event_notifier  
         * called, it will be removed.
         */
        //g_api->remove_event_notifier(&pMyData->notifier_pointer);
        assert(pMyData->notifier_pointer == 0);
    }
}

static int PsHandlerProcess(lsi_session_t *session)
{
    PsMData *pMyData = (PsMData *) g_api->get_module_data(session, &MNAME,
                       LSI_MODULE_DATA_HTTP);

    if (!pMyData)
    {
        g_api->log(session, LSI_LOG_ERROR,
                   "[%s]internal error during myhandler_process.\n", ModuleName);
        return 500;
    }

    g_api->set_status_code(session, pMyData->statusCode);

    if (pMyData->respHeaders)
        CopyRespHeadersToServer(session, *pMyData->respHeaders,
                                        kDontPreserveHeaders);
    else
    {
        if (pMyData->ctx && pMyData->ctx->baseFetch)
            pMyData->ctx->baseFetch->CollectHeaders(session);
    }

    while (ls_loopbuf_size(&pMyData->buff))
    {
        ls_loopbuf_straight(&pMyData->buff);
        const char *pBuf = ls_loopbuf_begin(&pMyData->buff);
        int len = ls_loopbuf_size(&pMyData->buff);
        int written = g_api->append_resp_body(session,  pBuf, len);

        if (written > 0)
            ls_loopbuf_popfront(&pMyData->buff, written);
        else
        {
            g_api->log(session, LSI_LOG_DEBUG,
                       "[%s]internal error during processing.\n", ModuleName);
            return 500;
        }
    }

    g_api->end_resp(session);
    g_api->free_module_data(session, &MNAME, LSI_MODULE_DATA_HTTP,
                            ReleaseMydata);
    return 0;
}


static lsi_serverhook_t serverHooks[] =
{
    { LSI_HKPT_MAIN_INITED,         PostConfig,         LSI_HOOK_NORMAL,    LSI_HOOK_FLAG_ENABLED },
    { LSI_HKPT_WORKER_POSTFORK,     ChildInit,          LSI_HOOK_NORMAL,    LSI_HOOK_FLAG_ENABLED },
    { LSI_HKPT_MAIN_ATEXIT,         TerminateMainConf,  LSI_HOOK_NORMAL,    LSI_HOOK_FLAG_ENABLED },
    { LSI_HKPT_RECV_REQ_HEADER,     RecvReqHeaderCheck, LSI_HOOK_NORMAL,    LSI_HOOK_FLAG_ENABLED },
    { LSI_HKPT_URI_MAP,             UriMapFilter,       LSI_HOOK_LAST,      0 },
    { LSI_HKPT_HANDLER_RESTART,     EndSession,         LSI_HOOK_LAST,      LSI_HOOK_FLAG_ENABLED },
    { LSI_HKPT_HTTP_END,            EndSession,         LSI_HOOK_LAST,      LSI_HOOK_FLAG_ENABLED },
    { LSI_HKPT_RCVD_RESP_HEADER,    HeaderFilter,       LSI_HOOK_LAST,      LSI_HOOK_FLAG_ENABLED },
    { LSI_HKPT_SEND_RESP_BODY,      BodyFilter,         LSI_HOOK_NORMAL,    LSI_HOOK_FLAG_ENABLED | LSI_HOOK_FLAG_TRANSFORM |
                                                       LSI_HOOK_FLAG_PROCESS_STATIC | LSI_HOOK_FLAG_DECOMPRESS_REQUIRED },
    lsi_serverhook_t_END   //Must put this at the end position
};

static int Init(lsi_module_t *pModule)
{
    g_api->init_module_data(pModule, ReleaseMydata, LSI_MODULE_DATA_HTTP);
    g_api->init_module_data(pModule, ReleaseVhConf, LSI_MODULE_DATA_VHOST);
    return 0;
}

lsi_config_t dealConfig = { ParseConfig, FreeConfig, paramArray };
lsi_handler_t _handler = { PsHandlerProcess, NULL, NULL, NULL };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, Init, &_handler, &dealConfig,
    "v1.4-1.9.32.3", serverHooks, {0} };

