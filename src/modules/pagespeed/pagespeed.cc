/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2014  LiteSpeed Technologies, Inc.                        *
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
#include <lsr/lsr_loopbuf.h>
#include <lsr/lsr_xpool.h>

#include "ls_caching_headers.h"
#include "ls_message_handler.h"
#include "ls_rewrite_driver_factory.h"
#include "ls_rewrite_options.h"
#include "ls_server_context.h"
#include "net/instaweb/automatic/public/proxy_fetch.h"
#include "net/instaweb/http/public/async_fetch.h"
#include "net/instaweb/http/public/content_type.h"
#include "net/instaweb/http/public/request_context.h"
#include "net/instaweb/public/global_constants.h"
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
#include <net/base/iovec.h>
#include "autostr.h"
#include "stringtool.h"
#include "ls_base_fetch.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include "../../../addon/example/loopbuff.h"
#include "pipenotifier.h"
#include "log_message_handler.h"
#include "../../src/http/httpglobals.h"

#define DBG(session, args...)                                       \
    g_api->log(session, LSI_LOG_DEBUG, args)




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
    DAV_PROPFIND,
    DAV_PROPPATCH,
    DAV_MKCOL,
    DAV_COPY,
    DAV_LOCK,
    DAV_UNLOCK,
    DAV_VERSION_CONTROL,
    DAV_REPORT,
    DAV_CHECKIN,
    DAV_CHECKOUT,
    DAV_UNCHECKOUT,
    DAV_UPDATE,
    DAV_MKWORKSPACE,
    DAV_LABEL,
    DAV_MERGE,
    DAV_BASELINE_CONTROL,
    DAV_MKACTIVITY,
    DAV_BIND,
    DAV_SEARCH,
    HTTP_PURGE,
    HTTP_REFRESH,
    HTTP_METHOD_END,
};


using namespace net_instaweb;

const char * kInternalEtagName = "@psol-etag";
// The process context takes care of proactively initialising
// a few libraries for us, some of which are not thread-safe
// when they are initialized lazily.
ProcessContext * process_context = new ProcessContext();

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
typedef struct
{
    LsiRewriteDriverFactory  *  driver_factory;
} ps_main_conf_t;

//VHost level data
typedef struct
{
    LsiServerContext      *     server_context;
    ProxyFetchFactory     *     proxy_fetch_factory;
    MessageHandler       *      handler;
} ps_vh_conf_t;

typedef struct
{
    LsiBaseFetch * base_fetch;
    lsi_session_t * session;

    bool html_rewrite;
    bool in_place;
    bool write_pending;
    bool fetch_done;

    PreserveCachingHeaders preserve_caching_headers;

    // for html rewrite
    ProxyFetch * proxy_fetch;

    // for in place resource
    RewriteDriver * driver;
    InPlaceResourceRecorder * recorder;
    ResponseHeaders * ipro_response_headers;
} ps_request_ctx_t; //session module data

typedef struct _PsMData
{
    ps_request_ctx_t * ctx;
    GoogleString url_string;

    LsiRewriteOptions      *      options;  //pointer to context level pointer
    ps_vh_conf_t         *        cfg_s;
    HTTP_METHOD    method;
    const char * user_agent;
    int uaLen;

    //RequestRouting::Response response_category;

    //return result
    int16_t     status_code;
    lsr_loopbuf_t  buff;

    PipeNotifier * pNotifier;

    bool end_resp_called;
} PsMData;


//LsiRewriteOptions* is data in all level and just inherit with the flow
//global --> Vhost  --> context  --> session
static ps_main_conf_t * pMainConf = NULL;

StringPiece str_to_string_piece( AutoStr2 s )
{
    return StringPiece( s.c_str(), s.len() );
}

//All of the parameters should have "pagespeed" as the first word.
const char * paramArray[] =
{
    "pagespeed",
    NULL //Must have NULL in the last item
};

int ps_set_cache_control( lsi_session_t * session, char * cache_control )
{
    g_api->set_resp_header( session, LSI_RESP_HEADER_CACHE_CTRL, NULL, 0, cache_control, strlen( cache_control ), LSI_HEADER_SET );
    return 0;
}

//If copy_request is 0, then copy response headers
//Othercopy request headers.
template<class Headers>
void copy_headers( lsi_session_t * session, int is_from_request, Headers * to )
{
#define MAX_REQUEST_OR_RESPONSE_HEADER  50
    struct iovec iov_key[MAX_REQUEST_OR_RESPONSE_HEADER], iov_val[MAX_REQUEST_OR_RESPONSE_HEADER];
    int count = 0;

    if( is_from_request )
    {
        count = g_api->get_req_headers( session, iov_key, iov_val, MAX_REQUEST_OR_RESPONSE_HEADER );
    }
    else
    {
        count = g_api->get_resp_headers( session, iov_key, iov_val, MAX_REQUEST_OR_RESPONSE_HEADER );
    }

    for( int i = 0; i < count; ++i )
    {
        StringPiece key = "", value = "";
        key.set( iov_key[i].iov_base, iov_key[i].iov_len );
        value.set( iov_val[i].iov_base, iov_val[i].iov_len );
        to->Add( key, value );
    }
}

//return 1001 for 1.1, 2000 for 2.0 etc
static int get_http_version( lsi_session_t * session )
{
    int major = 0, minor = 0;
    char val[10] = {0};
    int n = g_api->get_req_var_by_id( session, LSI_REQ_VAR_SERVER_PROTO, val, 10 );

    if( n >= 8 )  //should be http/
    {
        char * p = strchr( val, '/' );

        if( p )
        {
            sscanf( p, "%d.%d", &major, &minor );
        }
    }

    return major * 1000 + minor;
}

void net_instaweb::copy_response_headers_from_server( lsi_session_t * session,
        ResponseHeaders * headers )
{
    int version = get_http_version( session );
    headers->set_major_version( version / 1000 );
    headers->set_minor_version( version % 1000 );
    copy_headers( session, 0, headers );
    headers->set_status_code( g_api->get_status_code( session ) );

    struct iovec iov[1];

    if( g_api->get_resp_header( session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0, iov, 1 ) == 1 )
    {
        StringPiece content_type = "";
        content_type.set( iov[0].iov_base, iov[0].iov_len );
        headers->Add( HttpAttributes::kContentType, content_type );
    }

    // When we don't have a date header, set one with the current time.
    if( headers->Lookup1( HttpAttributes::kDate ) == NULL )
    {
        int64 date_ms;
        int32_t date_us;
        date_ms = ( g_api->get_cur_time( &date_us ) ) * 1000;
        date_ms += date_us / 1000;
        headers->SetDate( date_ms );
    }

    // TODO(oschaaf): ComputeCaching should be called in setupforhtml()?
    headers->ComputeCaching();
}

void net_instaweb::copy_request_headers_from_server( lsi_session_t * session,
        RequestHeaders * headers )
{
    int version = get_http_version( session );
    headers->set_major_version( version / 1000 );
    headers->set_minor_version( version % 1000 );
    copy_headers( session, 1, headers );
}

// PSOL produces caching headers that need some changes before we can send them
// out.  Make those changes and populate r->headers_out from pagespeed_headers.
int net_instaweb::copy_response_headers_to_server(
    lsi_session_t * session,
    const ResponseHeaders & pagespeed_headers,
    PreserveCachingHeaders preserve_caching_headers )
{
    g_api->set_status_code( session, pagespeed_headers.status_code() );

    int i;

    for( i = 0 ; i < pagespeed_headers.NumAttributes() ; i++ )
    {
        const GoogleString & name_gs = pagespeed_headers.Name( i );
        const GoogleString & value_gs = pagespeed_headers.Value( i );

        if( preserve_caching_headers == kPreserveAllCachingHeaders )
        {
            if( StringCaseEqual( name_gs, "ETag" ) ||
                    StringCaseEqual( name_gs, "Expires" ) ||
                    StringCaseEqual( name_gs, "Date" ) ||
                    StringCaseEqual( name_gs, "Last-Modified" ) ||
                    StringCaseEqual( name_gs, "Cache-Control" ) )
            {
                continue;
            }
        }
        else if( preserve_caching_headers == kPreserveOnlyCacheControl )
        {
            // Retain the original Cache-Control header, but send the recomputed
            // values for all other cache-related headers.
            if( StringCaseEqual( name_gs, "Cache-Control" ) )
            {
                continue;
            }
        } // else we don't preserve any headers

        AutoStr2 name, value;

        // To prevent the gzip module from clearing weak etags, we output them
        // using a different name here. The etag header filter module runs behind
        // the gzip compressors header filter, and will rename it to 'ETag'
        if( StringCaseEqual( name_gs, "etag" )
                && StringCaseStartsWith( value_gs, "W/" ) )
        {
            name.setStr( kInternalEtagName, strlen( kInternalEtagName ) );
        }
        else
        {
            name.setStr( name_gs.data(), name_gs.length() );
        }

        value.setStr( value_gs.data(), value_gs.length() );


        if( STR_EQ_LITERAL( name, "Cache-Control" ) )
        {
            ps_set_cache_control( session, const_cast<char *>( value_gs.c_str() ) );
            continue;
        }
        else if( STR_EQ_LITERAL( name, "Content-Type" ) )
        {
            g_api->set_resp_header( session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0,
                                    value.c_str(), value.len(), LSI_HEADER_SET );
            continue;
        }
        else if( STR_EQ_LITERAL( name, "Connection" ) )
        {
            continue;
        }
        else if( STR_EQ_LITERAL( name, "Keep-Alive" ) )
        {
            continue;
        }
        else if( STR_EQ_LITERAL( name, "Transfer-Encoding" ) )
        {
            continue;
        }
        else if( STR_EQ_LITERAL( name, "Server" ) )
        {
            continue;
        }


        bool need_set = false;

        // Populate the shortcuts to commonly used headers.
        if( STR_EQ_LITERAL( name, "Date" ) )
        {
            need_set = true;
        }
        else if( STR_EQ_LITERAL( name, "Etag" ) )
        {
            need_set = true;
        }
        else if( STR_EQ_LITERAL( name, "Expires" ) )
        {
            need_set = true;
        }
        else if( STR_EQ_LITERAL( name, "Last-Modified" ) )
        {
            need_set = true;
        }
        else if( STR_EQ_LITERAL( name, "Location" ) )
        {
            need_set = true;
        }
        else if( STR_EQ_LITERAL( name, "Server" ) )
        {
            need_set = true;
        }
        else if( STR_EQ_LITERAL( name, "Content-Length" ) )
        {
            need_set = true;
        }

        if( need_set )
        {
            g_api->set_resp_header( session, -1, name.c_str(), name.len(),
                                    value.c_str(), value.len(), LSI_HEADER_SET );
        }
    }

    return 0;
}


int net_instaweb::copy_response_body_to_buff( lsi_session_t * session, const char * s, int len )
{
    PsMData * pMyData = ( PsMData * ) g_api->get_module_data( session, &MNAME, LSI_MODULE_DATA_HTTP );
    //pMyData->status_code = 200;
    lsr_loopbuf_append( &pMyData->buff, s, len );

    return 0;
}

int terminate_process_context_n_main_conf( lsi_cb_param_t * rec )
{
    delete process_context;
    process_context = NULL;

    if( pMainConf )
    {
        LsiRewriteOptions::Terminate();
        LsiRewriteDriverFactory::Terminate();
        delete pMainConf->driver_factory;
        //delete pMainConf->handler;
        //pMainConf->handler = NULL;
        delete pMainConf;
        pMainConf = NULL;
    }

    return 0;
}

static int release_vh_conf( void * p )
{
    if( p )
    {
        ps_vh_conf_t * cfg_s = ( ps_vh_conf_t * ) p;

        if( cfg_s->proxy_fetch_factory )
        {
            delete cfg_s->proxy_fetch_factory;
        }

        if( cfg_s->server_context )
        {
            delete cfg_s->server_context;
        }

        delete cfg_s;
    }

    return 0;
}

// TODO(chaizhenhua): merge into NgxBaseFetch::Release()
void ps_release_base_fetch( PsMData * pData )
{
    // In the normal flow BaseFetch doesn't delete itself in HandleDone() because
    // we still need to receive notification via pipe and call
    // CollectAccumulatedWrites.  If there's an error and we're cleaning up early
    // then HandleDone() hasn't been called yet and we need the base fetch to wait
    // for that and then delete itself.
    if( pData->ctx == NULL )
    {
        return ;
    }

    if( pData->ctx->base_fetch != NULL )
    {
        pData->ctx->base_fetch->Release();
        pData->ctx->base_fetch = NULL;
    }

    if( pData->pNotifier != NULL )
    {
        pData->pNotifier->uninitNotifier( ( Multiplexer * ) g_api->get_multiplexer() );
        delete pData->pNotifier;
        pData->pNotifier = NULL;
    }
}

static int ps_release_request_context( PsMData * pData )
{
    // proxy_fetch deleted itself if we called Done(), but if an error happened
    // before then we need to tell it to delete itself.
    //
    // If this is a resource fetch then proxy_fetch was never initialized.
    if( pData->ctx->proxy_fetch != NULL )
    {
        pData->ctx->proxy_fetch->Done( false /* failure */ );
        pData->ctx->proxy_fetch = NULL;
    }

    if( pData->ctx->driver != NULL )
    {
        pData->ctx->driver->Cleanup();
        pData->ctx->driver = NULL;
    }

    if( pData->ctx->recorder != NULL )
    {
        pData->ctx->recorder->Fail();
        pData->ctx->recorder->DoneAndSetHeaders( NULL );  // Deletes recorder.
        pData->ctx->recorder = NULL;
    }

    ps_release_base_fetch( pData );
    delete pData->ctx;
    pData->ctx = NULL;
    return 0;
}

static int ps_release_mydata( void * data )
{
    PsMData * pData = ( PsMData * ) data;

    if( !pData )
    {
        return 0;
    }

    if( pData->ctx )
    {
        ps_release_request_context( pData );
    }


    lsr_loopbuf_d( &pData->buff );

//     if (pData->orgUri)
//     {
//         delete []pData->orgUri;
//         pData->orgUri = NULL;
//     }

    //TODO: should this be released?
//     if (pData->user_agent)
//     {
//         pData->user_agent = NULL;
//     }



//     if (pData->host)
//     {
//         delete []pData->host;
//         pData->host = NULL;
//     }

    delete pData;
    return 0;
}

int ps_end_session( lsi_cb_param_t * rec )
{
    PsMData * pData = ( PsMData * ) g_api->get_module_data( rec->_session, &MNAME, LSI_MODULE_DATA_HTTP );

    if( pData != NULL )
    {
        g_api->free_module_data( rec->_session, &MNAME, LSI_MODULE_DATA_HTTP, ps_release_mydata );
        g_api->set_module_data( rec->_session, &MNAME, LSI_MODULE_DATA_HTTP, NULL );
    }

    g_api->log( rec->_session, LSI_LOG_DEBUG, "[%s]ps_end_session.\n", kModuleName );
    return 0;
}

bool ps_is_https( lsi_session_t * session )
{
    //g_api->get_req_env_by_id(session, LSI_REQ_VAR_HTTPS);
    return false;
}

void ps_ignore_sigpipe()
{
    struct sigaction act;
    memset( &act, 0, sizeof( act ) );
    act.sa_handler = SIG_IGN;
    sigemptyset( &act.sa_mask );
    act.sa_flags = 0;
    sigaction( SIGPIPE, &act, NULL );
}


void ps_init_dir( const StringPiece & directive,
                  const StringPiece & path )
{
    if( path.size() == 0 || path[0] != '/' )
    {
        g_api->log( NULL, LSI_LOG_ERROR, "[%s] %s %s must start with a slash.\n",
                    kModuleName, directive.as_string().c_str(), path.as_string().c_str() );
        return ;
    }

    net_instaweb::StdioFileSystem file_system;
    net_instaweb::NullMessageHandler message_handler;
    GoogleString gs_path;
    path.CopyToString( &gs_path );

    if( !file_system.IsDir( gs_path.c_str(), &message_handler ).is_true() )
    {
        if( !file_system.RecursivelyMakeDir( path, &message_handler ) )
        {
            g_api->log( NULL, LSI_LOG_ERROR, "[%s] %s path %s does not exist and could not be created.\n",
                        kModuleName, directive.as_string().c_str(), path.as_string().c_str() );
            return ;
        }

        // Directory created, but may not be readable by the worker processes.
    }

    if( geteuid() != 0 )
    {
        return;  // We're not root, so we're staying whoever we are.
    }

    uid_t user = HttpGlobals::s_uid;
    gid_t group = HttpGlobals::s_gid;

    struct stat gs_stat;

    if( stat( gs_path.c_str(), &gs_stat ) != 0 )
    {
        g_api->log( NULL, LSI_LOG_ERROR, "[%s] %s path %s stat() failed.\n",
                    kModuleName, directive.as_string().c_str(), path.as_string().c_str() );
        return ;
    }

    if( gs_stat.st_uid != user )
    {
        if( chown( gs_path.c_str(), user, group ) != 0 )
        {
            g_api->log( NULL, LSI_LOG_ERROR, "[%s] %s path %s unable to set permissions.\n",
                        kModuleName, directive.as_string().c_str(), path.as_string().c_str() );
            return ;
        }
    }

}

// int ps_dollar(
//     ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data )
// {
//     v->valid = 1;
//     v->no_cacheable = 0;
//     v->not_found = 0;
//     v->data = reinterpret_cast<u_char*>( const_cast<char*>( "$" ) );
//     v->len = 1;
//     return NGX_OK;
// }


//return 0 OK, -1 error
static int initGData()
{
    static int inited = 0;

    if( inited == 1 )
    {
        return 0;
    }

    assert( pMainConf == NULL );
    pMainConf = new ps_main_conf_t;

    if( !pMainConf )
    {
        g_api->log( NULL, LSI_LOG_ERROR, "[%s]GDItem init error.\n", kModuleName );
        return -1;
    }

    LsiRewriteOptions::Initialize();
    LsiRewriteDriverFactory::Initialize();

    pMainConf->driver_factory = new LsiRewriteDriverFactory(
        *process_context,
        new SystemThreadSystem(),
        "" /* hostname, not used */,
        -1 /* port, not used */ );

    if( pMainConf->driver_factory == NULL )
    {
        delete pMainConf;
        pMainConf = NULL;
        g_api->log( NULL, LSI_LOG_ERROR, "[%s]GDItem init error 2.\n", kModuleName );
        return -1;
    }

    //pMainConf->handler = new LsiMessageHandler(pMainConf->driver_factory->thread_system()->NewMutex());

    inited = 1;
    return 0;
}

inline void TrimLeadingSpace( const char ** p )
{
    register char ch;

    while( ( ( ch = **p ) == ' ' ) || ( ch == '\t' ) || ( ch == '\r' ) )
    {
        ++ ( *p );
    }
}

static void parseOption( LsiRewriteOptions * pOption, const char * sLine, int len, int level, const char * name )
{
    const char * pEnd = sLine + len;
    TrimLeadingSpace( &sLine );

    if( pEnd - sLine <= 10 || strncasecmp( sLine, "pagespeed", 9 ) != 0 || ( sLine[9] != ' ' && sLine[9] != '\t' ) )
    {
        if( pEnd - sLine > 0 )
        {
            g_api->log( NULL, LSI_LOG_INFO, "[%s]parseOption do not parse %s in level %s\n",
                        kModuleName, AutoStr2( sLine, ( int )( pEnd - sLine ) ).c_str(), name );
        }

        return ;
    }

    //move to the next arg position
    sLine += 10;

#define MAX_ARG_NUM 5
    StringPiece args[MAX_ARG_NUM];
    int narg = 0;
    const char * argBegin, *argEnd, *pError;

    skipLeadingSpace( &sLine );

    while( narg < MAX_ARG_NUM && sLine < pEnd &&
            StringTool::parseNextArg( sLine, pEnd, argBegin, argEnd, pError ) == 0 )
    {
        args[narg ++].set( argBegin, argEnd - argBegin );
        skipLeadingSpace( &sLine );
    }

    MessageHandler * handler = pMainConf->driver_factory->message_handler();
    RewriteOptions::OptionScope scope;

    switch( level )
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

    default
            :
        scope = RewriteOptions::kProcessScope;
        break;
    }

    if( narg == 2 &&
            ( net_instaweb::StringCaseEqual( "LogDir", args[0] ) ||
              net_instaweb::StringCaseEqual( "FileCachePath", args[0] ) ) )
    {
        ps_init_dir( args[0], args[1] );
    }

    // The directory has been prepared, but we haven't actually parsed the
    // directive yet.  That happens below in ParseAndSetOptions().

    pOption->ParseAndSetOptions( args, narg, handler, pMainConf->driver_factory, scope );
}


static void * parseConfig( const char * param, int paramLen, void * _initial_config, int level, const char * name )
{
    if( initGData() )
    {
        return NULL;
    }


    LsiRewriteOptions * pInitOption = ( LsiRewriteOptions * ) _initial_config;
    LsiRewriteOptions * pOption = NULL;

    if( pInitOption )
    {
        pOption = pInitOption->Clone();
    }
    else
    {
        pOption = new LsiRewriteOptions( pMainConf->driver_factory->thread_system() );
    }

    if( !pOption )
    {
        return NULL;
    }

    if( !param )
    {
        return ( void * ) pOption;
    }

    const char * pEnd = param + paramLen;
    const char * pStart = param;
    const char * p;
    int len = 0;

    while( pStart < pEnd )
    {
        p = strchr( pStart, '\n' );

        if( p )
        {
            len = p - pStart;
        }
        else
        {
            len = pEnd - pStart;
        }

        parseOption( pOption, pStart, len, level, name );
        pStart += len + 1;
    }

    return ( void * ) pOption;
}

static void freeConfig( void * _config )
{
    delete( LsiRewriteOptions * ) _config;
}

static int child_init( lsi_cb_param_t * rec );
static int dummy_port = 0;
static int post_config( lsi_cb_param_t * rec )
{
    LsiRewriteOptions * pConfig = ( LsiRewriteOptions * ) g_api->get_module_param( NULL, &MNAME );

    //TODO: check if the below should call multi times?
    pMainConf->driver_factory->set_main_conf( pConfig );



    std::vector<SystemServerContext *> server_contexts;
    int vhost_count = g_api->get_vhost_count();

    for( int s = 0; s < vhost_count; s++ )
    {
        const void * vhost = g_api->get_vhost( s );
        ps_vh_conf_t * cfg_s = new ps_vh_conf_t;


        cfg_s->server_context = pMainConf->driver_factory->MakeLsiServerContext(
                                    "dummy_hostname", --dummy_port );


        LsiRewriteOptions * vhost_option = ( LsiRewriteOptions * ) g_api->get_vhost_module_param( vhost, &MNAME );

        //Comment: when Amdin/html parse, this will be NULL, we need not to add it
        if( vhost_option != NULL )
        {
            cfg_s->server_context->global_options()->Merge( *vhost_option );
            cfg_s->handler = pMainConf->driver_factory->message_handler();// LsiMessageHandler(pMainConf->driver_factory->thread_system()->NewMutex());  //Why GoogleMessageHandler() but not LsMessageHandler


//       cfg_s->server_context->InitStats(pMainConf->driver_factory->statistics());

//       cfg_s->server_context->InitProxyFetchFactory();

            //Should have some way to get the current vhost param which the best matched one
            //LsiRewriteOptions *pSessionConfig = (LsiRewriteOptions *)g_api->get_module_param( CUr_vhost, &MNAME );

            if( cfg_s->server_context->global_options()->enabled() )
            {
                //GoogleMessageHandler handler;
                const char * file_cache_path = cfg_s->server_context->config()->file_cache_path().c_str();

                if( file_cache_path[0] == '\0' )
                {
                    ;//Error
                    return -1;//return const_cast<char*>("FileCachePath must be set");
                }
                else if( !pMainConf->driver_factory->file_system()->IsDir(
                             file_cache_path, cfg_s->handler ).is_true() )
                {
                    ;//Error
                    return -1;//          return const_cast<char*>("FileCachePath must be an nginx-writeable directory");
                }

                g_api->log( NULL, LSI_LOG_DEBUG, "mod_pagespeed post_config OK, file_cache_path is %s\n", file_cache_path );
            }






            //   cfg_s->proxy_fetch_factory = new ProxyFetchFactory(cfg_s->server_context);

            g_api->set_vhost_module_data( vhost, &MNAME, cfg_s );

            server_contexts.push_back( cfg_s->server_context );
        }
    }


    Statistics * global_statistics = NULL;

    GoogleString error_message = "";
    int error_index = -1;



    g_api->log( NULL, LSI_LOG_DEBUG, "mod_pagespeed post_config call PostConfig()\n" );
    pMainConf->driver_factory->PostConfig(
        server_contexts, &error_message, &error_index, &global_statistics );

    if( error_index != -1 )
    {
        g_api->log( NULL, LSI_LOG_ERROR, "mod_pagespeed is enabled. %s\n", error_message.c_str() );
        return -1;
    }


    if( !server_contexts.empty() )
    {
        ps_ignore_sigpipe();

        if( global_statistics == NULL )
        {
            LsiRewriteDriverFactory::InitStats( pMainConf->driver_factory->statistics() );
        }

        Install_log_message_handler();
        pMainConf->driver_factory->RootInit();
    }
    else
    {
        delete pMainConf->driver_factory;
        pMainConf->driver_factory = NULL;
    }

    //while debugging
    if( g_api->get_server_mode() == LSI_SERVER_MODE_FORGROUND )
    {
        child_init( rec );
    }


    return 0;
}

static int child_init( lsi_cb_param_t * rec )
{
    if( pMainConf->driver_factory == NULL )
    {
        return 0;
    }

    SystemRewriteDriverFactory::InitApr();

    //pMainConf->driver_factory->LoggingInit(cycle->log);
    pMainConf->driver_factory->ChildInit();

    int vhost_count = g_api->get_vhost_count();

    for( int s = 0; s < vhost_count; s++ )
    {
        const void * vhost = g_api->get_vhost( s );
        ps_vh_conf_t * cfg_s = ( ps_vh_conf_t * ) g_api->get_vhost_module_data( vhost, &MNAME );

        if( cfg_s != NULL )
        {
            cfg_s->proxy_fetch_factory = new ProxyFetchFactory( cfg_s->server_context );
            pMainConf->driver_factory->SetServerContextMessageHandler( cfg_s->server_context );
        }
    }


    pMainConf->driver_factory->StartThreads();
    return 0;
}


// static int module_init( lsi_cb_param_t *rec )
// {
//
//     return 0;
// }


void ps_event_cb( void * session );
int ps_create_base_fetch( PsMData * pMyData, lsi_session_t * session )
{
    CHECK( pMyData->pNotifier == NULL );
    pMyData->pNotifier = new PipeNotifier();
    pMyData->pNotifier->initNotifier( ( Multiplexer * ) g_api->get_multiplexer(), session );
    g_api->log( session, LSI_LOG_DEBUG, "[Module:ModPagespeed]ps_create_base_fetch created event notifier, session=%ld\n",
                ( long ) session );

    pMyData->ctx->base_fetch = new LsiBaseFetch(
        session, pMyData->pNotifier->getFdIn(), pMyData->cfg_s->server_context,
        RequestContextPtr( pMyData->cfg_s->server_context->NewRequestContext( session ) ),
        pMyData->ctx->preserve_caching_headers );

    //When the base_fetch is good then set the callback, otherwise, may cause crash.
    if( pMyData->ctx->base_fetch )
    {
        pMyData->pNotifier->setCb( ps_event_cb );
        return 0;
    }
    else
    {
        pMyData->pNotifier->uninitNotifier( ( Multiplexer * ) g_api->get_multiplexer() );
        delete pMyData->pNotifier;
        pMyData->pNotifier = NULL;
        return -1;
    }
}




bool check_pagespeed_applicable( PsMData * pMyData, lsi_session_t * session )
{
    //Check status code is it is 200
    if( g_api->get_status_code( session ) != 200 )
    {
        return false;
    }

    // We can't operate on Content-Ranges.
    iovec iov[1];

    if( g_api->get_resp_header( session, LSI_RESP_HEADER_CONTENT_RANGE, NULL, 0, iov, 1 ) != 0 )
    {
        g_api->log( session, LSI_LOG_DEBUG,
                    "[%s]Request not rewritten because: header Content-Range set.\n", kModuleName );
        return false;
    }

    if( g_api->get_resp_header( session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0, iov, 1 ) != 1 )
    {
        g_api->log( session, LSI_LOG_DEBUG,
                    "[%s]Request not rewritten because: no Content-Type set.\n", kModuleName );
        return false;
    }

    //html, xhtml, cehtml
    if( strcasestr( ( const char * )( iov[0].iov_base ), "html" ) == NULL )
    {
        g_api->log( session, LSI_LOG_DEBUG,
                    "[%s]Request not rewritten because: not 'html like' Content-Type.\n",
                    kModuleName );
        return false;
    }

    return true;
}

StringPiece net_instaweb::ps_determine_host( lsi_session_t * session )
{
    char hostC[512];
    int maxLen = 512;
    g_api->get_req_var_by_id( session, LSI_REQ_VAR_SERVER_NAME, hostC, maxLen );
    StringPiece host = hostC;
    return host;
}

int net_instaweb::ps_determine_port( lsi_session_t * session )
{
    int port = -1;
    char portC[12];
    int maxLen = 12;
    g_api->get_req_var_by_id( session, LSI_REQ_VAR_SERVER_PORT, portC, maxLen );
    StringPiece port_str = portC;
    bool ok = StringToInt( port_str, &port );

    if( !ok )
    {
        // Might be malformed port, or just IPv6 with no port specified.
        return -1;
    }

    return port;
}


GoogleString ps_determine_url( lsi_session_t * session )
{
    int port = ps_determine_port( session );
    GoogleString port_string;

    if( ( ps_is_https( session ) && ( port == 443 || port == -1 ) ) ||
            ( !ps_is_https( session ) && ( port == 80 || port == -1 ) ) )
    {
        // No port specifier needed for requests on default ports.
        port_string = "";
    }
    else
    {
        port_string = StrCat( ":", IntegerToString( port ) );
    }

    StringPiece host = ps_determine_host( session );

    int uriLen = g_api->get_req_org_uri( session, NULL, 0 );
    char * uri = new char[uriLen + 1];
    g_api->get_req_org_uri( session, uri, uriLen + 1 );

    GoogleString rc = StrCat( ps_is_https( session ) ? "https://" : "http://",
                              host, port_string, uri );
    delete []uri;
    return rc;
}

// Fix URL based on X-Forwarded-Proto.
// http://code.google.com/p/modpagespeed/issues/detail?id=546 For example, if
// Apache gives us the URL "http://www.example.com/" and there is a header:
// "X-Forwarded-Proto: https", then we update this base URL to
// "https://www.example.com/".  This only ever changes the protocol of the url.
//
// Returns true if it modified url, false otherwise.
bool ps_apply_x_forwarded_proto( lsi_session_t * session, GoogleString * url )
{
    int valLen = 0;
    const char * buf = g_api->get_req_header_by_id( session, LSI_REQ_HEADER_X_FORWARDED_FOR, &valLen );

    if( valLen == 0 || buf == NULL )
    {
        return false;  // No X-Forwarded-Proto header found.
    }

    AutoStr2 bufStr( buf );
    StringPiece x_forwarded_proto =
        str_to_string_piece( bufStr );

    if( !STR_CASE_EQ_LITERAL( bufStr, "http" ) &&
            !STR_CASE_EQ_LITERAL( bufStr, "https" ) )
    {
        LOG( WARNING ) << "Unsupported X-Forwarded-Proto: " << x_forwarded_proto
                       << " for URL " << url << " protocol not changed.";
        return false;
    }

    StringPiece url_sp( *url );
    StringPiece::size_type colon_pos = url_sp.find( ":" );

    if( colon_pos == StringPiece::npos )
    {
        return false;  // URL appears to have no protocol; give up.
    }

//
    // Replace URL protocol with that specified in X-Forwarded-Proto.
    *url = StrCat( x_forwarded_proto, url_sp.substr( colon_pos ) );

    return true;
}

bool is_pagespeed_subrequest( lsi_session_t * session, const char * ua, int & uaLen )
{
    if( ua && uaLen > 0 )
    {
        if( memmem( ua, uaLen, kModPagespeedSubrequestUserAgent, strlen( kModPagespeedSubrequestUserAgent ) ) )
        {
            g_api->log( session, LSI_LOG_DEBUG, "[Module:ModPagespeed]Request not rewritten because: User-Agent appears to be %s.\n",
                        kModPagespeedSubrequestUserAgent );
            return true;
        }
    }

    return false;
}


// int ps_async_wait_response( PsMData *pMyData, bool html_rewrite, lsi_session_t* session )
// {
//     g_api->log(session, LSI_LOG_DEBUG, "[Module:ModPagespeed]ps_async_wait_response html_rewrite is %d.\n", html_rewrite);
//     if (html_rewrite)
//         return 1;
//     else
//         return 0;
//
// }


void ps_beacon_handler_helper( PsMData * pMyData, lsi_session_t * session, StringPiece beacon_data )
{
    g_api->log( session, LSI_LOG_DEBUG, "ps_beacon_handler_helper: beacon[%d] %s",
                beacon_data.size(),  beacon_data.data() );

    StringPiece user_agent = StringPiece( pMyData->user_agent, pMyData->uaLen );
    CHECK( pMyData->cfg_s != NULL );

    pMyData->cfg_s->server_context->HandleBeacon(
        beacon_data,
        user_agent,
        RequestContextPtr( pMyData->cfg_s->server_context->NewRequestContext( session ) ) );

    ps_set_cache_control( session, const_cast<char *>( "max-age=0, no-cache" ) );
}

// Parses out query params from the request.
void ps_query_params_handler( lsi_session_t * session, StringPiece * data )
{

    int iQSLen;
    const char * pQS = g_api->get_req_query_string( session, &iQSLen );

    if( iQSLen == 0 )
    {
        *data = "";
    }
    else
    {
        ( *data ) = StringPiece( pQS, iQSLen );
    }

    //Then req body
    CHECK( g_api->is_req_body_finished( session ) );
    char buf[1024];
    int ret;
    bool bReqBodyStart = false;

    while( ( ret = g_api->read_req_body( session, buf, 1024 ) ) > 0 )
    {
        if( !bReqBodyStart )
        {
            std::string sp( "&", 1 );
            ( *data ).AppendToString( &sp );
            bReqBodyStart = true;
        }

        std::string sbuf( buf, ret );
        ( *data ).AppendToString( &sbuf );
    }
}


int ps_beacon_handler( PsMData * pMyData, lsi_session_t * session )
{
    // Use query params.
    StringPiece query_param_beacon_data;
    ps_query_params_handler( session, &query_param_beacon_data );
    ps_beacon_handler_helper( pMyData, session, query_param_beacon_data );
    pMyData->status_code = 204;
    return 0;
}

int ps_simple_handler( PsMData * pMyData, lsi_session_t * session,
                       LsiServerContext * server_context,
                       RequestRouting::Response response_category )
{
    LsiRewriteDriverFactory * factory =
        static_cast<LsiRewriteDriverFactory *>(
            server_context->factory() );
    LsiMessageHandler * message_handler = factory->lsi_message_handler();

    int uriLen = g_api->get_req_org_uri( session, NULL, 0 );
    char * uri = new char[uriLen + 1];

    g_api->get_req_org_uri( session, uri, uriLen + 1 );
    uri[uriLen] = 0x00;
    StringPiece request_uri_path = uri;//

    GoogleString & url_string = pMyData->url_string;
    GoogleUrl url( url_string );
    QueryParams query_params;

    if( url.IsWebValid() )
    {
        query_params.Parse( url.Query() );
    }

    GoogleString output;
    StringWriter writer( &output );
    // pMyData->status_code = 200;
    HttpStatus::Code status = HttpStatus::kOK;
    ContentType content_type = kContentTypeHtml;
    StringPiece cache_control = HttpAttributes::kNoCache;
    const char * error_message = NULL;

    switch( response_category )
    {
    case RequestRouting::kStaticContent:
    {
        StringPiece file_contents;

        if( !server_context->static_asset_manager()->GetAsset(
                    request_uri_path.substr( factory->static_asset_prefix().length() ),
                    &file_contents, &content_type, &cache_control ) )
        {
            return -1;
        }

        file_contents.CopyToString( &output );
        break;
    }

    case RequestRouting::kMessages:
    {
        GoogleString log;
        StringWriter log_writer( &log );

        if( !message_handler->Dump( &log_writer ) )
        {
            writer.Write( "Writing to ngx_pagespeed_message failed. \n"
                          "Please check if it's enabled in pagespeed.conf.\n",
                          message_handler );
        }
        else
        {
            HtmlKeywords::WritePre( log, &writer, message_handler );
        }

        break;
    }

    default
            :
        g_api->log( session, LSI_LOG_WARN, "ps_simple_handler: unknown RequestRouting." );
        return -1;
    }

    if( error_message != NULL )
    {
        pMyData->status_code = 404;
        content_type = kContentTypeHtml;
        output = error_message;
    }


    ResponseHeaders response_headers;
    response_headers.SetStatusAndReason( status );
    response_headers.set_major_version( 1 );
    response_headers.set_minor_version( 1 );
    response_headers.Add( HttpAttributes::kContentType, content_type.mime_type() );


//     g_api->set_resp_header( session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0, content_type.mime_type(),
//                             strlen( content_type.mime_type() ), LSI_HEADER_SET );

    // http://msdn.microsoft.com/en-us/library/ie/gg622941(v=vs.85).aspx
    // Script and styleSheet elements will reject responses with
    // incorrect MIME types if the server sends the response header
    // "X-Content-Type-Options: nosniff". This is a security feature
    // that helps prevent attacks based on MIME-type confusion.
    response_headers.Add( "X-Content-Type-Options", "nosniff" );
//     g_api->set_resp_header( session, -1, "X-Content-Type-Options", sizeof( "X-Content-Type-Options" ) - 1,
//                             "nosniff", 7, LSI_HEADER_SET );

    //TODO: add below
    int64 now_ms = factory->timer()->NowMs();
    response_headers.SetDate( now_ms );
    response_headers.SetLastModified( now_ms );
    response_headers.Add( HttpAttributes::kCacheControl, cache_control );
//     g_api->set_resp_header( session, -1, HttpAttributes::kCacheControl, strlen( HttpAttributes::kCacheControl ),
//                             cache_control.as_string().c_str(), cache_control.size(), LSI_HEADER_SET );

//   char* cache_control_s = string_piece_to_pool_string(r->pool, cache_control);
//   if (cache_control_s != NULL) {
    if( FindIgnoreCase( cache_control, "private" ) == StringPiece::npos )
    {
        g_api->set_resp_header( session, LSI_RESP_HEADER_ETAG, NULL, 0,
                                "W/\"0\"", strlen( "W/\"0\"" ), LSI_HEADER_SET );
    }

    //g_api->append_body_buf(session, output.c_str(), output.size());
    lsr_loopbuf_append( &pMyData->buff, output.c_str(), output.size() );

    copy_response_headers_to_server( session, response_headers, kDontPreserveHeaders );
    return 0;
}



int ps_resource_handler( PsMData * pMyData,
                         lsi_session_t * session,
                         bool html_rewrite,
                         RequestRouting::Response response_category )
{
    ps_request_ctx_t * ctx = pMyData->ctx;
    ps_vh_conf_t * cfg_s = pMyData->cfg_s;
    CHECK( !( html_rewrite && ( ctx == NULL || ctx->html_rewrite == false ) ) );

    if( !html_rewrite && pMyData->method != HTTP_GET && pMyData->method != HTTP_HEAD )
    {
        return -1;
    }

    GoogleString url_string = ps_determine_url( session );;
    GoogleUrl url( url_string );
    CHECK( url.IsWebValid() );


    scoped_ptr<RequestHeaders> request_headers( new RequestHeaders );
    scoped_ptr<ResponseHeaders> response_headers( new ResponseHeaders );

    copy_request_headers_from_server( session, request_headers.get() );
    copy_response_headers_from_server( session, response_headers.get() );

    LsiRewriteOptions * options = ( LsiRewriteOptions * ) g_api->get_module_param( session, &MNAME );

    if( !options->enabled() )
    {
        // Disabled via query params or request headers.
        return -1;
    }

    // ps_determine_options modified url, removing any ModPagespeedFoo=Bar query
    // parameters.  Keep url_string in sync with url.
    url.Spec().CopyToString( &url_string );




    if( cfg_s->server_context->global_options()->respect_x_forwarded_proto() )
    {
        bool modified_url = ps_apply_x_forwarded_proto( session, &url_string );

        if( modified_url )
        {
            url.Reset( url_string );
            CHECK( url.IsWebValid() ) << "The output of ps_apply_x_forwarded_proto"
                                      << " should always be a valid url because it only"
                                      << " changes the scheme between http and https.";
        }
    }

    bool pagespeed_resource =
        !html_rewrite && cfg_s->server_context->IsPagespeedResource( url );
    bool is_an_admin_handler =
        response_category == RequestRouting::kStatistics ||
        response_category == RequestRouting::kGlobalStatistics ||
        response_category == RequestRouting::kConsole ||
        response_category == RequestRouting::kAdmin ||
        response_category == RequestRouting::kGlobalAdmin;


    if( html_rewrite )
    {
        ps_release_base_fetch( pMyData );
    }
    else
    {
        // create request ctx
        CHECK( ctx == NULL );
        ctx = new ps_request_ctx_t();

        ctx->session = session;
        ctx->write_pending = false;
        ctx->html_rewrite = false;
        ctx->in_place = false;
        //ctx->pagespeed_connection = NULL;
        ctx->preserve_caching_headers = kDontPreserveHeaders;

        if( !options->modify_caching_headers() )
        {
            ctx->preserve_caching_headers = kPreserveAllCachingHeaders;
        }
        else if( !options->IsDownstreamCacheIntegrationEnabled() )
        {
            // Downstream cache integration is not enabled. Disable original
            // Cache-Control headers.
            ctx->preserve_caching_headers = kDontPreserveHeaders;
        }
        else if( !pagespeed_resource && !is_an_admin_handler )
        {
            ctx->preserve_caching_headers = kPreserveOnlyCacheControl;
            // Downstream cache integration is enabled. If a rebeaconing key has been
            // configured and there is a ShouldBeacon header with the correct key,
            // disable original Cache-Control headers so that the instrumented page is
            // served out with no-cache.
            StringPiece should_beacon( request_headers->Lookup1( kPsaShouldBeacon ) );

            if( options->MatchesDownstreamCacheRebeaconingKey( should_beacon ) )
            {
                ctx->preserve_caching_headers = kDontPreserveHeaders;
            }
        }

        ctx->recorder = NULL;
        pMyData->url_string = url_string;
        pMyData->options = options;
        pMyData->ctx = ctx;
    }


//      if (!pagespeed_resource &&
//         !is_an_admin_handler &&
//         !html_rewrite &&
//         !(options->in_place_rewriting_enabled() &&
//             options->enabled() &&
//             options->IsAllowed( url.Spec() ) ) )
//     {
//                 // NOTE: We are using the below debug message as is for some of our system
//         // tests. So, be careful about test breakages caused by changing or
//         // removing this line.
//         g_api->log( session, LSI_LOG_DEBUG, "Passing on content handling for non-pagespeed resource '%s'\n",
//                 url_string.c_str() );
//
//         // set html_rewrite flag.
//         ctx->html_rewrite = true;
//         g_api->set_handler_write_state(session, 1);
//         return -1;
//     }




    if( ps_create_base_fetch( pMyData, session ) != 0 )
    {
        return -1;
    }



    ctx->base_fetch->SetRequestHeadersTakingOwnership( request_headers.release() );


    bool page_callback_added = false;
    scoped_ptr<ProxyFetchPropertyCallbackCollector>
    property_callback(
        ProxyFetchFactory::InitiatePropertyCacheLookup(
            !html_rewrite /* is_resource_fetch */,
            url,
            cfg_s->server_context,
            options,
            ctx->base_fetch,
            false /* requires_blink_cohort (no longer unused) */,
            &page_callback_added ) );

    if( pagespeed_resource )
    {
        ResourceFetch::Start(
            url,
            NULL,
            false /* using_spdy */, cfg_s->server_context, ctx->base_fetch );

        g_api->set_handler_write_state( session, 0 );
        return 0;
    }
    else if( is_an_admin_handler )
    {
        QueryParams query_params;
        query_params.Parse( url.Query() );

        PosixTimer timer;
        int64 now_ms = timer.NowMs();
        ctx->base_fetch->response_headers()->SetDateAndCaching(
            now_ms, 0 /* max-age */, ", no-cache" );

        if( response_category == RequestRouting::kStatistics ||
                response_category == RequestRouting::kGlobalStatistics )
        {
            cfg_s->server_context->StatisticsPage(
                response_category == RequestRouting::kGlobalStatistics,
                query_params,
                cfg_s->server_context->config(),
                ctx->base_fetch );
        }
        else if( response_category == RequestRouting::kConsole )
        {
            cfg_s->server_context->ConsoleHandler(
                *cfg_s->server_context->config(),
                SystemServerContext::kStatistics,
                query_params,
                ctx->base_fetch );
        }
        else if( response_category == RequestRouting::kAdmin ||
                 response_category == RequestRouting::kGlobalAdmin )
        {
            cfg_s->server_context->AdminPage(
                response_category == RequestRouting::kGlobalAdmin,
                url,
                query_params,
                cfg_s->server_context->config(),
                ctx->base_fetch );
        }
        else
        {
            CHECK( false );
        }

        g_api->set_handler_write_state( session, 0 );
        return 0;
    }

    if( html_rewrite )
    {
        // Do not store driver in request_context, it's not safe.
        RewriteDriver * driver;

        // If we don't have custom options we can use NewRewriteDriver which reuses
        // rewrite drivers and so is faster because there's no wait to construct
        // them.  Otherwise we have to build a new one every time.

        driver = cfg_s->server_context->NewRewriteDriver(
                     ctx->base_fetch->request_context() );

        StringPiece user_agent = ctx->base_fetch->request_headers()->Lookup1(
                                     HttpAttributes::kUserAgent );

        if( !user_agent.empty() )
        {
            driver->SetUserAgent( user_agent );
        }

        driver->SetRequestHeaders( *ctx->base_fetch->request_headers() );

        ctx->proxy_fetch = cfg_s->proxy_fetch_factory->CreateNewProxyFetch(
                               url_string, ctx->base_fetch, driver,
                               property_callback.release(),
                               NULL /* original_content_fetch */ );

        g_api->log( NULL, LSI_LOG_DEBUG, "Create ProxyFetch %s.\n", url_string.c_str() );
        g_api->set_handler_write_state( session, 0 );
        return 0;
    }

    if( options->in_place_rewriting_enabled() &&
            options->enabled() &&
            options->IsAllowed( url.Spec() ) )
    {
        // Do not store driver in request_context, it's not safe.
        RewriteDriver * driver;

        driver = cfg_s->server_context->NewRewriteDriver(
                     ctx->base_fetch->request_context() );


        StringPiece user_agent = ctx->base_fetch->request_headers()->Lookup1(
                                     HttpAttributes::kUserAgent );

        if( !user_agent.empty() )
        {
            driver->SetUserAgent( user_agent );
        }

        driver->SetRequestHeaders( *ctx->base_fetch->request_headers() );

        ctx->driver = driver;

        cfg_s->server_context->message_handler()->Message(
            kInfo, "Trying to serve rewritten resource in-place: %s",
            url_string.c_str() );

        ctx->in_place = true;
        ctx->base_fetch->set_handle_error( false );
        ctx->driver->FetchInPlaceResource(
            url, false /* proxy_mode */, ctx->base_fetch );

        bool success;

        if( ctx->base_fetch->isDoneCalled( success ) && success )
        {
            g_api->set_handler_write_state( session, 0 );
            return 0; //Will handle it
        }
        else
        {
            return -1;
        }
    }

    //NEVER RUN TO THIS POINT BECAUSE THE PREVOIUS CHEKCING NBYPASS THIS ONE
    // NOTE: We are using the below debug message as is for some of our system
    // tests. So, be careful about test breakages caused by changing or
    // removing this line.
    g_api->log( session, LSI_LOG_DEBUG, "Passing on content handling for non-pagespeed resource '%s'\n",
                url_string.c_str() );

    ctx->base_fetch->Done( false );
    ps_release_base_fetch( pMyData );
    // set html_rewrite flag.
    ctx->html_rewrite = true;
    g_api->set_handler_write_state( session, 1 );
    return -1;
}



// Send each buffer in the chain to the proxy_fetch for optimization.
// Eventually it will make it's way, optimized, to base_fetch.
void ps_send_to_pagespeed( PsMData * pMyData, lsi_cb_param_t * rec, ps_request_ctx_t * ctx )
{
    if( ctx->proxy_fetch == NULL )
    {
        return ;
    }

    CHECK( ctx->proxy_fetch != NULL );
    ctx->proxy_fetch->Write(
        StringPiece( ( const char * ) rec->_param, rec->_param_len ), pMyData->cfg_s->handler );

    if( rec->_flag_in & LSI_CB_FLAG_IN_EOF )
    {
        ctx->proxy_fetch->Done( true /* success */ );
        ctx->proxy_fetch = NULL;  // ProxyFetch deletes itself on Done().
    }
    else
    {
        ctx->proxy_fetch->Flush( pMyData->cfg_s->handler );
    }
}

void ps_strip_html_headers( lsi_session_t * session )
{
    g_api->remove_resp_header( session, LSI_RESP_HEADER_CONTENT_LENGTH, NULL, 0 );
    g_api->remove_resp_header( session, LSI_RESP_HEADER_ACCEPT_RANGES, NULL, 0 );
}

int ps_html_rewrite_fix_headers_filter( PsMData * pMyData, lsi_session_t * session, ps_request_ctx_t * ctx )
{
    if( ctx == NULL || !ctx->html_rewrite
            || ctx->preserve_caching_headers == kPreserveAllCachingHeaders )
    {
        return 0;
    }

    if( ctx->preserve_caching_headers == kDontPreserveHeaders )
    {
        // Don't cache html.  See mod_instaweb:instaweb_fix_headers_filter.
        LsiCachingHeaders caching_headers( session );
        ps_set_cache_control( session, ( char * ) caching_headers.GenerateDisabledCacheControl().c_str() );
    }

    // Pagespeed html doesn't need etags: it should never be cached.
    g_api->remove_resp_header( session, LSI_RESP_HEADER_ETAG, NULL, 0 );

    // An html page may change without the underlying file changing, because of
    // how resources are included.  Pagespeed adds cache control headers for
    // resources instead of using the last modified header.
    g_api->remove_resp_header( session, LSI_RESP_HEADER_LAST_MODIFIED, NULL, 0 );

//   // Clear expires
//   if (r->headers_out.expires) {
//     r->headers_out.expires->hash = 0;
//     r->headers_out.expires = NULL;
//   }

    return 0;
}

int ps_in_place_check_header_filter( PsMData * pMyData, lsi_session_t * session, ps_request_ctx_t * ctx )
{

    if( ctx->recorder != NULL )
    {
        g_api->log( session, LSI_LOG_DEBUG,
                    "ps in place check header filter recording: %s", pMyData->url_string.c_str() );

        CHECK( !ctx->in_place );

        // We didn't find this resource in cache originally, so we're recording it
        // as it passes us by.  At this point the headers from things that run
        // before us are set but not things that run after us, which means here is
        // where we need to check whether there's a "Content-Encoding: gzip".  If we
        // waited to do this in ps_in_place_body_filter we wouldn't be able to tell
        // the difference between response headers that have "C-E: gz" because we're
        // proxying for an upstream that gzipped the content and response headers
        // that have it because the gzip filter (which runs after us) is going to
        // produce gzipped output.
        //
        // The recorder will do this checking, so pass it the headers.
        ResponseHeaders response_headers;
        copy_response_headers_from_server( session, &response_headers );
        ctx->recorder->ConsiderResponseHeaders(
            InPlaceResourceRecorder::kPreliminaryHeaders, &response_headers );
        return 0;
    }

    if( !ctx->in_place )
    {
        return 0;
    }

    g_api->log( session, LSI_LOG_DEBUG,
                "[Module:ModPagespeed]ps in place check header filter initial: %s\n", pMyData->url_string.c_str() );

    int status_code = pMyData->ctx->base_fetch->response_headers()->status_code();
    //g_api->get_status_code( session );
    bool status_ok = ( status_code != 0 ) && ( status_code < 400 );

    ps_vh_conf_t * cfg_s = pMyData->cfg_s;
    LsiServerContext * server_context = cfg_s->server_context;
    MessageHandler * message_handler = cfg_s->handler;
    GoogleString url = ps_determine_url( session );
    // The URL we use for cache key is a bit different since it may
    // have PageSpeed query params removed.
    GoogleString cache_url = pMyData->url_string;

    // continue process
    if( status_ok )
    {
        ctx->in_place = false;

        server_context->rewrite_stats()->ipro_served()->Add( 1 );
        message_handler->Message(
            kInfo, "Serving rewritten resource in-place: %s",
            url.c_str() );

        return 0;
    }

    if( status_code == CacheUrlAsyncFetcher::kNotInCacheStatus )
    {
        server_context->rewrite_stats()->ipro_not_in_cache()->Add( 1 );
        server_context->message_handler()->Message(
            kInfo,
            "Could not rewrite resource in-place "
            "because URL is not in cache: %s",
            cache_url.c_str() );
        const SystemRewriteOptions * options = SystemRewriteOptions::DynamicCast(
                ctx->driver->options() );
        RequestHeaders request_headers;
        copy_request_headers_from_server( session, &request_headers );
        // This URL was not found in cache (neither the input resource nor
        // a ResourceNotCacheable entry) so we need to get it into cache
        // (or at least a note that it cannot be cached stored there).
        // We do that using an Apache output filter.
        ctx->recorder = new InPlaceResourceRecorder(
            RequestContextPtr( cfg_s->server_context->NewRequestContext( session ) ),
            cache_url,
            ctx->driver->CacheFragment(),
            request_headers.GetProperties(),
            options->respect_vary(),
            options->ipro_max_response_bytes(),
            options->ipro_max_concurrent_recordings(),
            options->implicit_cache_ttl_ms(),
            server_context->http_cache(),
            server_context->statistics(),
            message_handler );
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
        copy_response_headers_from_server( session, &response_headers );
        ctx->recorder->ConsiderResponseHeaders(
            InPlaceResourceRecorder::kPreliminaryHeaders, &response_headers );
        /*******************************
         *
         *
         */

    }
    else
    {
        server_context->rewrite_stats()->ipro_not_rewritable()->Add( 1 );
        message_handler->Message( kInfo,
                                  "Could not rewrite resource in-place: %s", url.c_str() );
    }

    ctx->driver->Cleanup();
    ctx->driver = NULL;
    // enable html_rewrite
    ctx->html_rewrite = true;
    ctx->in_place = false;

    // re init ctx
    ctx->fetch_done = false;
    ctx->write_pending = false;

    ps_release_base_fetch( pMyData );

    return -1;
}

int ps_in_place_body_filter( PsMData * pMyData, lsi_cb_param_t * rec, ps_request_ctx_t * ctx, ps_vh_conf_t * cfg_s )
{
    if( ctx->recorder == NULL )
    {
        return 1;
    }


    g_api->log( rec->_session, LSI_LOG_DEBUG,
                "[Module:ModPagespeed]ps in place body filter: %s\n", pMyData->url_string.c_str() );

    InPlaceResourceRecorder * recorder = ctx->recorder;
    StringPiece contents( ( char * ) rec->_param, rec->_param_len );

    recorder->Write( contents, recorder->handler() );

    if( rec->_flag_in & LSI_CB_FLAG_IN_FLUSH )
    {
        recorder->Flush( recorder->handler() );
    }

    if( rec->_flag_in & LSI_CB_FLAG_IN_EOF || recorder->failed() )
    {
        ResponseHeaders response_headers;
        copy_response_headers_from_server( rec->_session, &response_headers );
        ctx->recorder->DoneAndSetHeaders( &response_headers );
        ctx->recorder = NULL;
    }


    return 0;
}


int ps_html_rewrite_body_filter( PsMData * pMyData, lsi_cb_param_t * rec, ps_request_ctx_t * ctx, ps_vh_conf_t * cfg_s )
{
    int ret = 1;

    if( ctx->html_rewrite )
    {
        if( rec->_param_len > 0 ||  rec->_flag_in )
        {
            ps_send_to_pagespeed( pMyData, rec, ctx );
        }

        ret = 0;
    }

    return ret;
}

int ps_body_filter( lsi_cb_param_t * rec )
{
    ps_vh_conf_t * cfg_s;
    ps_request_ctx_t * ctx;
    PsMData * pMyData = ( PsMData * ) g_api->get_module_data( rec->_session, &MNAME, LSI_MODULE_DATA_HTTP );

    if( pMyData == NULL || ( cfg_s = pMyData->cfg_s ) == NULL || ( ctx = pMyData->ctx ) == NULL )
    {
        return g_api->stream_write_next( rec, ( const char * ) rec->_param, rec->_param_len );
    }


//      int ret1 = ps_in_place_body_filter(pMyData, rec, ctx, cfg_s );
//      //ps_base_fetch_filter();
//      int ret2 = ps_html_rewrite_body_filter(pMyData, rec, ctx, cfg_s );

//     //disorder
    int ret1 = ps_html_rewrite_body_filter( pMyData, rec, ctx, cfg_s );
//     //ps_base_fetch_filter(pMyData);
    ps_in_place_body_filter( pMyData, rec, ctx, cfg_s );


    if( ret1 != 0 || ctx->base_fetch == NULL )
    {
        return g_api->stream_write_next( rec, ( const char * ) rec->_param, rec->_param_len );
    }


    /**
     * Whne the filter need to wait the event, just return
     *
     */
    //if (ret == 0)

    lsr_loopbuf_straight( &pMyData->buff );
    int len;
    int writtenTotal = 0;

    while( ( len = lsr_loopbuf_size( &pMyData->buff ) ) > 0 )
    {
        char * buf = lsr_loopbuf_begin( &pMyData->buff );

        if( ctx->base_fetch->isLastBufSent() )
        {
            rec->_flag_in = LSI_CB_FLAG_IN_EOF;
        }
        else
        {
            rec->_flag_in = LSI_CB_FLAG_IN_FLUSH;
        }

        int written = g_api->stream_write_next( rec, buf, len );
        lsr_loopbuf_pop_front( &pMyData->buff, written );
        writtenTotal += written;
    }

    if( !ctx->base_fetch->isLastBufSent() )
    {
        if( rec->_flag_out )
        {
            *rec->_flag_out |= LSI_CB_FLAG_OUT_BUFFERED_DATA;
        }
    }
    else
    {
        if( !pMyData->end_resp_called )
        {
            pMyData->end_resp_called = true;
            g_api->end_resp( rec->_session );
        }
    }

    g_api->log( rec->_session, LSI_LOG_DEBUG, "[%s]ps_body_filter flag_in %d, flag out %d, return %d, Accumulated %d, wriiten %d.\n",
                kModuleName, rec->_flag_in, *rec->_flag_out, ctx->base_fetch->isLastBufSent(), rec->_param_len, writtenTotal );
    return rec->_param_len;

}



void updateEtag( lsi_cb_param_t * rec )
{
    struct iovec iov[1] = {{NULL, 0}};
    int iovCount = g_api->get_resp_header( rec->_session, -1, kInternalEtagName, strlen( kInternalEtagName ), iov, 1 );

    if( iovCount == 1 )
    {
        g_api->remove_resp_header( rec->_session, -1, kInternalEtagName, strlen( kInternalEtagName ) );
        g_api->set_resp_header( rec->_session, LSI_RESP_HEADER_ETAG, NULL, 0, ( const char * ) iov[0].iov_base, iov[0].iov_len, LSI_HEADER_SET );
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
int ps_html_rewrite_header_filter( PsMData * pMyData, lsi_session_t * session, ps_request_ctx_t * ctx, ps_vh_conf_t * cfg_s )
{
    //if (r != r->main) {
    // Don't handle subrequests.
    // return ngx_http_next_header_filter(r);

    // Poll for cache flush on every request (polls are rate-limited).
    //cfg_s->server_context->FlushCacheIfNecessary();

    if( ctx->html_rewrite == false )
    {
        return 0;
    }

    /*
        if ( !check_pagespeed_applicable( pMyData, session ) )
        {
            ctx->html_rewrite = false;
            return 0;
        }*/



    g_api->log( session, LSI_LOG_DEBUG,
                "[Module:ModPagespeed]http pagespeed html rewrite header filter \"%s\"\n", pMyData->url_string.c_str() );


    int rc = ps_resource_handler( pMyData, session, true, RequestRouting::kResource );

    if( rc != 0 )
    {
        ctx->html_rewrite = false;
        return 0;
    }



    ps_strip_html_headers( session );
    copy_response_headers_from_server( session, ctx->base_fetch->response_headers() );

    return 0;
}

int ps_header_filter( lsi_cb_param_t * rec )
{
    updateEtag( rec );

    ps_vh_conf_t * cfg_s;
    ps_request_ctx_t * ctx;
    PsMData * pMyData = ( PsMData * ) g_api->get_module_data( rec->_session, &MNAME, LSI_MODULE_DATA_HTTP );

    if( pMyData == NULL || ( cfg_s = pMyData->cfg_s ) == NULL || ( ctx = pMyData->ctx ) == NULL )
    {
        return 0;
    }

    if( is_pagespeed_subrequest( rec->_session, pMyData->user_agent, pMyData->uaLen ) )
    {
        return 0;
    }



//     ps_in_place_check_header_filter( pMyData, rec->_session, ctx );
//     ps_html_rewrite_fix_headers_filter( pMyData, rec->_session, ctx );
//     ps_html_rewrite_header_filter( pMyData, rec->_session, ctx, cfg_s );


//     //disorder
    ps_html_rewrite_header_filter( pMyData, rec->_session, ctx, cfg_s );
    ps_html_rewrite_fix_headers_filter( pMyData, rec->_session, ctx );
    ps_in_place_check_header_filter( pMyData, rec->_session, ctx );



    return 0;
}



// Set us up for processing a request.  Creates a request context and determines
// which handler should deal with the request.
RequestRouting::Response ps_route_request( PsMData * pMyData, lsi_session_t * session, bool is_resource_fetch )
{
    ps_vh_conf_t * cfg_s = pMyData->cfg_s;

    if( !cfg_s->server_context->global_options()->enabled() )
    {
        // Not enabled for this server block.
        return RequestRouting::kPagespeedDisabled;
    }

//FIXME: should check status code here? since only call in URI_Map and it is set to 200 at that time
//     if ( g_api->get_status_code( session ) != 200 )
//         return RequestRouting::kErrorResponse;

    GoogleString url_string = ps_determine_url( session );
    GoogleUrl url( url_string );

    if( !url.IsWebValid() )
    {
        //ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "invalid url");
        // Let nginx deal with the error however it wants; we will see a NULL ctx in
        // the body filter or content handler and do nothing.
        return RequestRouting::kInvalidUrl;
    }

    if( is_pagespeed_subrequest( session, pMyData->user_agent, pMyData->uaLen ) )
    {
        return RequestRouting::kPagespeedSubrequest;
    }
    else if(
        url.PathSansLeaf() == dynamic_cast<LsiRewriteDriverFactory *>(
            cfg_s->server_context->factory() )->static_asset_prefix() )
    {
        return RequestRouting::kStaticContent;
    }

    const LsiRewriteOptions * global_options = cfg_s->server_context->config();
    StringPiece path = url.PathSansQuery();

    if( StringCaseEqual( path, global_options->statistics_path() ) )
    {
        return RequestRouting::kStatistics;
    }
    else if( StringCaseEqual( path, global_options->global_statistics_path() ) )
    {
        return RequestRouting::kGlobalStatistics;
    }
    else if( StringCaseEqual( path, global_options->console_path() ) )
    {
        return RequestRouting::kConsole;
    }
    else if( StringCaseEqual( path, global_options->messages_path() ) )
    {
        return RequestRouting::kMessages;
    }
    else if(  // The admin handlers get everything under a path (/path/*) while
        // all the other handlers only get exact matches (/path).  So match
        // all paths starting with the handler path.
        !global_options->admin_path().empty() &&
        StringCaseStartsWith( path, global_options->admin_path() ) )
    {
        return RequestRouting::kAdmin;
    }
    else if( !global_options->global_admin_path().empty() &&
             StringCaseStartsWith( path, global_options->global_admin_path() ) )
    {
        return RequestRouting::kGlobalAdmin;
    }

    const GoogleString * beacon_url;

    if( ps_is_https( session ) )
    {
        beacon_url = & ( global_options->beacon_url().https );
    }
    else
    {
        beacon_url = & ( global_options->beacon_url().http );
    }

    if( url.PathSansQuery() == StringPiece( *beacon_url ) )
    {
        return RequestRouting::kBeacon;
    }

    return RequestRouting::kResource;
}

static int ps_set_waitfullreq( lsi_cb_param_t * rec )
{
    g_api->set_req_wait_full_body( rec->_session );
    return LSI_RET_OK;
}

static int ps_uri_map_filter( lsi_cb_param_t * rec )
{
    //init the VHost data
    ps_vh_conf_t * cfg_s = ( ps_vh_conf_t * ) g_api->get_module_data( rec->_session, &MNAME, LSI_MODULE_DATA_VHOST );

    if( cfg_s == NULL || cfg_s->server_context == NULL )
    {
        g_api->log( rec->_session, LSI_LOG_ERROR, "[%s]ps_uri_map_filter error, cfg_s == NULL || cfg_s->server_context == NULL.\n", kModuleName );
        return LSI_RET_OK;
    }

    LsiRewriteOptions * pConfig = ( LsiRewriteOptions * ) g_api->get_module_param( rec->_session, &MNAME );

    if( !pConfig )
    {
        g_api->log( rec->_session, LSI_LOG_ERROR, "[%s]ps_uri_map_filter error 2.\n", kModuleName );
        return LSI_RET_OK;
    }

    if( !pConfig->enabled() )
    {
        g_api->log( rec->_session, LSI_LOG_DEBUG, "[%s]ps_uri_map_filter returned [Not enabled].\n", kModuleName );
        return LSI_RET_OK;
    }

    char httpMethod[10] = {0};
    int methodLen = g_api->get_req_var_by_id( rec->_session, LSI_REQ_VAR_REQ_METHOD, httpMethod, 10 );
    int method = HTTP_UNKNOWN;

    switch( methodLen )
    {
    case 3:
        if( ( httpMethod[0] | 0x20 ) == 'g' )  //"GET"
        {
            method = HTTP_GET;
        }

        break;

    case 4:
        if( strncasecmp( httpMethod, "HEAD", 4 ) == 0 )
        {
            method = HTTP_HEAD;
        }
        else if( strncasecmp( httpMethod, "POST", 4 ) == 0 )
        {
            method = HTTP_POST;
        }

        break;

    default
            :
        break;
    }

    if( method == HTTP_UNKNOWN )
    {
        g_api->log( rec->_session, LSI_LOG_INFO, "[%s]ps_uri_map_filter returned, method %s.\n", kModuleName, httpMethod );
        return 0;
    }

    //cfg_s->server_context->FlushCacheIfNecessary();


    //If URI_MAP called before, should release then first
    PsMData * pMyData = ( PsMData * ) g_api->get_module_data( rec->_session, &MNAME, LSI_MODULE_DATA_HTTP );

    if( pMyData )
    {
        g_api->free_module_data( rec->_session, &MNAME, LSI_MODULE_DATA_HTTP, ps_release_mydata );
    }

    pMyData = new PsMData;

    if( pMyData == NULL )
    {
        g_api->log( rec->_session, LSI_LOG_INFO, "[%s]ps_uri_map_filter returned, pMyData == NULL.\n", kModuleName );
        return LSI_RET_OK;
    }

    pMyData->ctx = NULL;
    pMyData->options = NULL;
    pMyData->cfg_s = NULL;
    pMyData->method = HTTP_UNKNOWN;
    pMyData->user_agent = NULL;
    pMyData->uaLen = 0;
    pMyData->status_code = 0;
    pMyData->pNotifier = NULL;
    lsr_loopbuf( &pMyData->buff, 4096 );
    pMyData->url_string = ps_determine_url( rec->_session );
    pMyData->method = ( HTTP_METHOD ) method;
    pMyData->cfg_s = cfg_s;
    pMyData->user_agent = g_api->get_req_header_by_id( rec->_session, LSI_REQ_HEADER_USERAGENT, &pMyData->uaLen );
    g_api->set_module_data( rec->_session, &MNAME, LSI_MODULE_DATA_HTTP, pMyData );
    RequestRouting::Response response_category =
        ps_route_request( pMyData, rec->_session, true );

    int ret = -1;

    switch( response_category )
    {
    case RequestRouting::kBeacon:
        ret = ps_beacon_handler( pMyData, rec->_session );
        break;

    case RequestRouting::kStaticContent:
    case RequestRouting::kMessages:
        ret = ps_simple_handler( pMyData, rec->_session, cfg_s->server_context, response_category );
        break;

    case RequestRouting::kStatistics:
    case RequestRouting::kGlobalStatistics:
    case RequestRouting::kConsole:
    case RequestRouting::kAdmin:
    case RequestRouting::kGlobalAdmin:
    case RequestRouting::kResource:
        ret = ps_resource_handler( pMyData, rec->_session, false, response_category );

        if( ret == 0 )
        {
            pMyData->status_code = -1;

        }

        break;

    default
            :
        break;
    }

    /*******
     * Through the hanler testing code, we will find if it will be hanldered by modpagespeed or not_found
     * if YES, even status code is not 200, will also return 0 and the code is set to Mydata
     */
    if( ret == 0 )
    {
        g_api->register_req_handler( rec->_session, &MNAME, 0 );
        g_api->log( rec->_session, LSI_LOG_INFO, "[%s]ps_uri_map_filter register_req_handler OK.\n", kModuleName );
    }


    //Disable cache module
    g_api->set_req_env( rec->_session, "cache-control", 13, "no-cache", 8 );

    return LSI_RET_OK;
}

int ps_base_fetch_handler( PsMData * pMyData, lsi_session_t * session )
{
    ps_request_ctx_t * ctx = pMyData->ctx;

    g_api->log( session, LSI_LOG_DEBUG, "ps_base_fetch_handler: %ld", ( long ) session );
    // collect response headers from pagespeed
    ctx->base_fetch->CollectHeaders( session );

    int rc = ctx->base_fetch->CollectAccumulatedWrites( session );
    g_api->log( session, LSI_LOG_DEBUG, "ps_base_fetch_handler called CollectAccumulatedWrites, ret %d, %ld\n",
                rc, ( long ) session );

    if( rc == LSI_RET_OK )
    {
        ctx->fetch_done = true;
    }

    return 0;
}

void ps_event_cb( void * session_ )
{
    lsi_session_t * session = ( lsi_session_t * ) session_;
    PsMData * pMyData = ( PsMData * ) g_api->get_module_data( session, &MNAME, LSI_MODULE_DATA_HTTP );

    if( pMyData->ctx->base_fetch )
    {
        if( !pMyData->ctx->fetch_done )
        {
            ps_base_fetch_handler( pMyData, session );
        }
        else
        {
            pMyData->ctx->base_fetch->CollectAccumulatedWrites( session );
        }
    }

    g_api->set_handler_write_state( session, 1 );
    g_api->log( session, LSI_LOG_DEBUG, "[%s]ps_event_cb triggered, session=%ld\n", kModuleName, ( long ) session_ );
}

static int onWriteEvent( lsi_session_t * session )
{
    PsMData * pMyData = ( PsMData * ) g_api->get_module_data( session, &MNAME, LSI_MODULE_DATA_HTTP );

    while( lsr_loopbuf_size( &pMyData->buff ) )
    {
        lsr_loopbuf_straight( &pMyData->buff );
        const char * pBuf = lsr_loopbuf_begin( &pMyData->buff );
        int len = lsr_loopbuf_size( &pMyData->buff );
        int written = g_api->append_resp_body( session,  pBuf, len );

        if( written > 0 )
        {
            lsr_loopbuf_pop_front( &pMyData->buff, written );
        }
        else
        {
            g_api->log( session, LSI_LOG_DEBUG, "[%s]internal error during processing.\n", kModuleName );
        }
    }

    pMyData->status_code = 200;
    g_api->set_status_code( session, 200 );


    if( !pMyData->ctx->base_fetch->isLastBufSent() )
    {
        return LSI_WRITE_RESP_CONTINUE;
    }
    else
    {
        g_api->free_module_data( session, &MNAME, LSI_MODULE_DATA_HTTP, ps_release_mydata );
        return LSI_WRITE_RESP_FINISHED;
    }
}

static int myhandler_process( lsi_session_t * session )
{
    PsMData * pMyData = ( PsMData * ) g_api->get_module_data( session, &MNAME, LSI_MODULE_DATA_HTTP );

    if( !pMyData )
    {
        g_api->log( session, LSI_LOG_ERROR, "[%s]internal error during myhandler_process.\n", kModuleName );
        return 500;
    }

    if( pMyData->status_code == -1 && ( pMyData->ctx == NULL || pMyData->ctx->in_place == false ) )
    {
        g_api->set_handler_write_state( session, 0 );
        return 0;
    }



    if( pMyData->status_code > 0 )
    {
        g_api->set_status_code( session, pMyData->status_code );
    }

    while( lsr_loopbuf_size( &pMyData->buff ) )
    {
        lsr_loopbuf_straight( &pMyData->buff );
        const char * pBuf = lsr_loopbuf_begin( &pMyData->buff );
        int len = lsr_loopbuf_size( &pMyData->buff );
        int written = g_api->append_resp_body( session,  pBuf, len );

        if( written > 0 )
        {
            lsr_loopbuf_pop_front( &pMyData->buff, written );
        }
        else
        {
            g_api->log( session, LSI_LOG_DEBUG, "[%s]internal error during processing.\n", kModuleName );
            return 500;
        }
    }

    g_api->end_resp( session );
    g_api->free_module_data( session, &MNAME, LSI_MODULE_DATA_HTTP, ps_release_mydata );
    return 0;
}


static lsi_serverhook_t serverHooks[] =
{
    { LSI_HKPT_MAIN_INITED,         post_config,        LSI_HOOK_NORMAL,    LSI_HOOK_FLAG_ENABLED},
    //{ LSI_HKPT_MAIN_PREFORK,      module_init,        LSI_HOOK_NORMAL,    LSI_HOOK_FLAG_ENABLED},
    { LSI_HKPT_WORKER_POSTFORK,     child_init,         LSI_HOOK_NORMAL,    LSI_HOOK_FLAG_ENABLED},
    { LSI_HKPT_MAIN_ATEXIT,         terminate_process_context_n_main_conf,  LSI_HOOK_NORMAL, LSI_HOOK_FLAG_ENABLED},
    {LSI_HKPT_RECV_REQ_HEADER,      ps_set_waitfullreq, LSI_HOOK_NORMAL,    LSI_HOOK_FLAG_ENABLED},
    {LSI_HKPT_URI_MAP,              ps_uri_map_filter,  LSI_HOOK_LAST,      LSI_HOOK_FLAG_ENABLED},
    {LSI_HKPT_HANDLER_RESTART,      ps_end_session,     LSI_HOOK_LAST,      LSI_HOOK_FLAG_ENABLED},
    {LSI_HKPT_HTTP_END,             ps_end_session,     LSI_HOOK_LAST,      LSI_HOOK_FLAG_ENABLED},
    {LSI_HKPT_RECV_RESP_HEADER,     ps_header_filter,   LSI_HOOK_LAST,      LSI_HOOK_FLAG_ENABLED},
    {
        LSI_HKPT_SEND_RESP_BODY,       ps_body_filter,     LSI_HOOK_NORMAL,    LSI_HOOK_FLAG_TRANSFORM
        | LSI_HOOK_FLAG_PROCESS_STATIC | LSI_HOOK_FLAG_DECOMPRESS_REQUIRED | LSI_HOOK_FLAG_ENABLED
    },
    lsi_serverhook_t_END   //Must put this at the end position
};

static int init( lsi_module_t * pModule )
{
    g_api->init_module_data( pModule, ps_release_mydata, LSI_MODULE_DATA_HTTP );
    g_api->init_module_data( pModule, release_vh_conf, LSI_MODULE_DATA_VHOST );
    return 0;
}

lsi_config_t dealConfig = { parseConfig, freeConfig, paramArray };
lsi_handler_t _handler = { myhandler_process, NULL, onWriteEvent, NULL };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init, &_handler, &dealConfig, "v1.0-1.8.31.4", serverHooks, {0} };

