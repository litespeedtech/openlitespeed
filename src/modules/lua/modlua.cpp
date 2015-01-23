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
#include <ls.h>
#include "lsluaengine.h"
#include "lsluasession.h"
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAXFILENAMELEN  0x1000

#define     MNAME       mod_lua
extern lsi_module_t MNAME;

/* Module Data */
typedef struct {
    LsLuaSession * pSession;
} luaModuleData_t;

static int releaseLuaData( void * data)
{
    if (!data)
        return 0;
    luaModuleData_t * pData = (luaModuleData_t *) data;

    pData->pSession = NULL;
    free(pData);
    return 0;
}

static luaModuleData_t * allocateLuaData(lsi_session_t * session, const lsi_module_t * module, int level)
{
    luaModuleData_t * pData = (luaModuleData_t *) malloc(sizeof(luaModuleData_t));
    if (!pData)
        return NULL;
    memset(pData, 0, sizeof(luaModuleData_t));
    g_api->set_module_data(session, module, level, pData);
    return pData;
}

static LsLuaSession * getLsLuaSession_from_moduleData(lsi_session_t * pSess)
{
    luaModuleData_t * pData = (luaModuleData_t *) g_api->get_module_data(pSess, &MNAME, LSI_MODULE_DATA_HTTP);
    if (pData)
        return pData->pSession;
    else
        return NULL;
}

#if 0
static int http_end(lsi_cb_param_t *rec)
{
    extern void http_end_callback(void *);

    g_api->log( NULL, LSI_LOG_DEBUG,  "HTTP_END [%s]\n", g_api->get_req_uri(rec->_session, NULL));
    // http_end_callback(rec->_session);
    return LSI_RET_OK;
}

static int http_begin(lsi_cb_param_t *rec)
{
    //const char *uri;
    //uri = g_api->get_req_uri(rec->_session, NULL );
    // g_api->log( NULL, LSI_LOG_DEBUG,  "HTTP_BEGIN [%s]\n", uri);
    return LSI_RET_OK;
}

static int recv_resp_header(struct lsi_cb_param_t *rec)
{
    const char *uri;
    uri = g_api->get_req_uri(rec->_session);
    g_api->log( NULL, LSI_LOG_DEBUG,  "RECV_RESP_HEADER [%s]\n", uri);
    return LSI_RET_OK;
}

static int recv_resp_body(struct lsi_cb_param_t *rec)
{
    const char *uri;
    uri = g_api->get_req_uri(rec->_session);
    g_api->log( NULL, LSI_LOG_DEBUG,  "RECV_RESP_BODY [%s]\n", uri);
    return LSI_RET_OK;
}

static int recvd_resp_body(struct lsi_cb_param_t *rec)
{
    const char *uri;
    uri = g_api->get_req_uri(rec->_session);
    g_api->log( NULL, LSI_LOG_DEBUG,  "RECVD_RESP_BODY [%s]\n", uri);
    return LSI_RET_OK;
}

static int send_resp_header(struct lsi_cb_param_t *rec)
{
    const char *uri;
    uri = g_api->get_req_uri(rec->_session);
    g_api->log( NULL, LSI_LOG_DEBUG,  "SEND_RESP_HEADER [%s]\n", uri);
    return LSI_RET_OK;
}

static int send_resp_body(struct lsi_cb_param_t *rec)
{
    const char *uri;
    uri = g_api->get_req_uri(rec->_session);
    g_api->log( NULL, LSI_LOG_DEBUG,  "SEND_RESP_BODY [%s]\n", uri);
    return LSI_RET_OK;
}

static int recved_req_handler(struct lsi_cb_param_t *rec)
{
    const char *uri;
    uri = g_api->get_req_uri(rec->_session);
    g_api->log( NULL, LSI_LOG_DEBUG,  "recved_req_handler [%s]\n", uri);
    return LSI_RET_OK;
}

//
//  LSI_HKPT_RECV_REQ_BODY 
//
int recv_req_body(struct lsi_cb_param_t *rec)
{
    const char *uri;
    uri = g_api->get_req_uri(rec->_session);
    g_api->log( NULL, LSI_LOG_DEBUG,  "RECV_REQ_BODY LUA exec_handler [%s]\n", uri);
    return LSI_RET_OK;
}

#endif
//
//  LSI_HKPT_RECV_REQ_HEADER 
//      If we choose to use HOOK POINT for handler selection,
//          this is the HOOK POINT...
//
int recv_req_header(lsi_cb_param_t *rec)
{
    // luaHandlerX( rec->_session );
    const char *uri;
    int len;
    uri = g_api->get_req_uri(rec->_session, &len);
    if ((len > 5) && (!memcmp(uri+len-4, ".lua", 4))) 
    {
        g_api->register_req_handler(rec->_session, &MNAME, 0);
        // g_api->log( rec->_session, LSI_LOG_DEBUG,  "LUA RECV_REQ_HEADER [%s]\n", uri);
    }
        
#if 0
    g_api->log( NULL, LSI_LOG_NOTICE,  "LUA RECV_REQ_HEADER LUA [%s]\n", uri);

    char luafile[MAXFILENAMELEN];   /* 4k filenamepath */
    char buf[0x1000];
    register int    n;

    if (g_api->get_uri_file_path( rec->_session, uri, strlen(uri), luafile, MAXFILENAMELEN))
    {
        n = snprintf(buf, 0x1000, "luahandler: FAILED TO COMPOSE LUA script path %s\r\n", uri);
        g_api->append_resp_body( rec->_session, buf, n);
        g_api->end_resp( rec->_session );
        return LSI_RET_OK;
    }
    g_api->log( NULL, LSI_LOG_NOTICE,  "LUA RECV_REQ_HEADER LUA PATH [%s]\n", luafile);


    if ( memcmp(uri, "/lslua/", 7) == 0 )
    {
        if (LsLuaEngine::isReady(rec->_session))
        {
            g_api->log( NULL, LSI_LOG_DEBUG,  "LUA recv_req_header [%s]\n", uri);
            g_api->register_req_handler(rec->_session, &MNAME, 0);
            // This enforce the handler wont be called until all the data is there.
            // g_api->set_wait_full_req_body( rec->_session );
        }
    }
#endif
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    { LSI_HKPT_RECV_REQ_HEADER, recv_req_header, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_ENABLED },
    lsi_serverhook_t_END    //Must put this at the end position
};

static int _init( lsi_module_t * pModule )
{
    //
    //  load LUA Engine here
    //
    if (LsLuaEngine::init())
    {
        return -1;
    }
    
    pModule->_info = LsLuaEngine::version();  //set version string
    
    //
    // enable call back
    //
    g_api->log( NULL, LSI_LOG_NOTICE,  "LUA: %s ENGINE READY\n", LsLuaEngine::getLuaName());

    //
    //  module data for LsLuaSession look up
    //
    g_api->init_module_data( pModule, releaseLuaData, LSI_MODULE_DATA_HTTP );
    return 0;
}

static int luaHandler(lsi_session_t *session )
{
    luaModuleData_t * pData;

// myTimerStart();
    pData = (luaModuleData_t *) g_api->get_module_data(session, &MNAME, LSI_MODULE_DATA_HTTP);
    if (!pData)
    {
        pData = allocateLuaData(session, &MNAME, LSI_MODULE_DATA_HTTP);
        if (!pData)
        {
            g_api->log( NULL, LSI_LOG_ERROR,  "FAILED TO ALLOCATE MODULE DATA\n");
            return LSI_RET_ERROR;
        }
    }
    pData->pSession = NULL;

    char * uri;
    uri = (char *)g_api->get_req_uri( session, NULL );
    
    char luafile[MAXFILENAMELEN];   /* 4k filenamepath */
    char buf[0x1000];
    register int    n;

    if (g_api->is_req_body_finished(session))
    {
        ;
    }
    else
    {
        g_api->set_req_wait_full_body( session );
    }

    if (g_api->get_uri_file_path( session, uri, strlen(uri), luafile, MAXFILENAMELEN))
    {
        n = snprintf(buf, 0x1000, "luahandler: FAILED TO COMPOSE LUA script path %s\r\n", uri);
        g_api->append_resp_body( session, buf, n);
        g_api->end_resp( session );
        return LSI_RET_OK;
    }
    // g_api->log( NULL, LSI_LOG_NOTICE,  "LUA HANDLER PATH [%s]\n", luafile);

    // set to 1 = continueWrite, 0 = suspendWrite - the default is 1 if onWrite is provied.
    // So I just set this off... 
    g_api->set_handler_write_state( session, 0);
    
    LsLuaEngine::setDebugLevel((int) g_api->_debugLevel);

        LsLuaUserParam * pUser = (LsLuaUserParam *) g_api->get_module_param(session, &MNAME);
        if (LsLuaEngine::api()->jitMode)
        {
            LsLuaEngine::runScriptX(session, luafile, pUser, &(pData->pSession));
        }
        else
        {
            LsLuaEngine::runScript(session, luafile, pUser, &(pData->pSession));
        }
// myTimerStop();
    return LSI_RET_OK;
}

//
//  onCleanupEvent - remove junk...
//
static int onCleanupEvent( lsi_session_t *session )
{
    extern void http_end_callback(void *, LsLuaSession *);
    
    // g_api->log( session, LSI_LOG_DEBUG,  "LUA onCleanupEvent\n");
    http_end_callback(session, getLsLuaSession_from_moduleData(session));
    return 0;
}

//
//  onReadEvent - activated when whole request body has been read by the server.
//
static int onReadEvent( lsi_session_t *session )
{
    LsLuaSession * pSession;

    pSession = getLsLuaSession_from_moduleData(session);
    
    // pSession = LsLuaSession::findByLsiSession(session);
    if (!pSession)
    {
        g_api->log( session, LSI_LOG_NOTICE,  "ERROR: LUA onReadEvent Session NULL\n");
        return 0;
    }

    //
    // Code from David... we need to worry about this later
    // This check statement should be an assert instead!
    if (!g_api->is_req_body_finished(session))
    {
        char buf[8192];
#define     CONTENT_FORMAT  "<tr><td>%s</td><td>%s</td></tr><p>\r\n"
        snprintf(buf, 8192, CONTENT_FORMAT, "ERROR", "You must forget to set wait all req body!!!<p>\r\n");
        g_api->append_resp_body(session, buf, strlen(buf));
        return 0;
    }
    if (pSession->isLuaWaitReqBody() && (!pSession->isLuaDoneFlag()))
    {
        // wake up the LUA script which waiting for Request body.
        pSession->resume(pSession->getLuaState(), 0);
    }
    return 0;
} 

//
//  A typical handler should set write state to 0... no onWriteEvent
//    g_api->set_handler_write_state( session, 0);
//
//  For lua script -
//      We turn on the onWriteEvent while there is no more buffer for us to write.
//      when we get call
//      (1) check if there is buffer available for us to continue.
//      (2) resume the yielded code. (session level)
//
static int onWriteEvent( lsi_session_t *session )
{
    LsLuaSession * pSession;
    pSession = getLsLuaSession_from_moduleData(session);
    
    // pSession = LsLuaSession::findByLsiSession(session);
    // g_api->log( session, LSI_LOG_DEBUG,  "LUA onWrite %p <%p>", session, pSession);
    if (pSession)
    {
        if (pSession->onWrite(session) == 1)
        {
            return LSI_WRITE_RESP_CONTINUE;
        }
    }
    return LSI_WRITE_RESP_FINISHED;
}

const char *myParam[] = {
    "lib", 
    "maxruntime",
    "maxlinecount",
    NULL   //The last position must have a NULL to indicate end of the array
};

/**
 * Define a handler, need to provide a struct _handler_functions_st object, in which 
 * the first function pointer should not be NULL
 */
static lsi_handler_t lslua_mod_handler = { luaHandler, onReadEvent, onWriteEvent , onCleanupEvent};

//
//  module parameters
//  lib /usr/lib/liblua.so
//  lib /usr/lib/libluajit.so
//  maxruntime 300 
//
static lsi_config_t lslua_mod_config = { LsLuaEngine::parseParam, LsLuaEngine::removeParam , myParam };

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE,
                        _init,
                        &lslua_mod_handler,
                        &lslua_mod_config,
                        "v1.0",
                        serverHooks,
                        {0}
};
