/*
Copyright (c) 2014, LiteSpeed Technologies Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met: 

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer. 
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution. 
    * Neither the name of the Lite Speed Technologies Inc nor the
      names of its contributors may be used to endorse or promote
      products derived from this software without specific prior
      written permission.  

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/
#define _GNU_SOURCE 
#include "../include/ls.h"
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>

/**********************************************************************
 * 
 * This module is for testing params of different context levels.
 * You can set it with different params in server, VHost/module and context 
 * under VHost/module, and VHost/context levels, use such as "param1 123\n
 * param2 10\n..."
 * The uri in each context level must have "/test/", such as /test/, /test/1/2/,
 * and so on.
 * 
 * You can test "get /test/1/", "get /test/1/2/testfile"
 * 
 * The handler can be registered under the VHost/context or by enable the filters 
 * somewhere, and then when the context matched the url and then the filter will 
 * be called so that the filter will register itself as a handler.
 * 
 * The result should be
 * If the hook is enabled, the filer may run and set itself as 
 * handler, the result is always the context which match the longest uri,
 * the result is different for different context level.
 * 
 **********************************************************************/

#define     MNAME       testparam
lsi_module_t MNAME;

typedef struct _param_st{
    int param1;
    int param2;
    int param3;
    int param4;
    int param5;
} param_st;

static int testparam_parseLine(const char *buf, const char *key, int minValue, int maxValue, int defValue)
{
    const char *paramEnd = buf + strlen(buf);
    const char *p;
    int val = defValue;
    
    p = (const char *)strstr(buf, key);
    if (!p)
        return val;
    
    int keyLen = strlen(key);
    if (p + keyLen < paramEnd)
    {
        if (sscanf(p + keyLen, "%d", &val) == 1)
        {
            if (val < minValue || val > maxValue)
                val =defValue;
        }
    }
    
    return val;
}

//Setup the below array to let web server know these params
const char *myParam[] = {
    "param1", 
    "param2", 
    "param3", 
    "param4", 
    "param5", 
    NULL   //The last position must have a NULL to indicate end of the array
};

static void * testparam_parseConfig( const char *param, void *_initial_config, int level, const char *name )
{
    param_st *pInitConfig = (param_st *)_initial_config;
    param_st *pConfig = (param_st *) malloc(sizeof(struct _param_st));
    if (!pConfig)
        return NULL;
    
    if (pInitConfig)
    {
        memcpy(pConfig, pInitConfig, sizeof(struct _param_st));
    }
    else
    {
        memset(pConfig, 0, sizeof(struct _param_st));
    }
    
    if (!param)
        return (void *)pConfig;
    
    long val;
    val= testparam_parseLine( param, "param1", 0, 10000, -1 );
    if ( val != -1 )  pConfig->param1 = val;
    
    val = testparam_parseLine( param, "param2", 0, 10000, -1 );
    if ( val != -1 )  pConfig->param2 = val;
    
    val = testparam_parseLine( param, "param3", 0, 10000, -1 );
    if ( val != -1 )  pConfig->param3 = val;
    
    val = testparam_parseLine( param, "param4", 0, 10000, -1 );
    if ( val != -1 )  pConfig->param4 = val;
    
    val = testparam_parseLine( param, "param5", 0, 10000, -1 );
    if ( val != -1 )  pConfig->param5 = val;
    
    return (void *)pConfig;
}

static void testparam_freeConfig(void *_config)
{
    free(_config);
}

static int testparam_handlerBeginProcess(lsi_session_t *session)
{
    g_api->set_resp_header(session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0, "text/plain", sizeof("text/plain") - 1, LSI_HEADER_SET );
    
    param_st *pparam_st = (param_st *) g_api->get_module_param(session, &MNAME);
    char buf[1024];
    int len;
    if (pparam_st) 
    {
        sprintf(buf, "Current uri is %s.\nContext uri is %s.\nparam1 = %d\nparam2 = %d\nparam3 = %d\nparam4 = %d\nparam5 = %d\n",
            g_api->get_req_uri(session, NULL),
            g_api->get_mapped_context_uri(session, &len),
            pparam_st->param1, pparam_st->param2, 
            pparam_st->param3, pparam_st->param4, pparam_st->param5);
    }
    else
    {
        strcpy(buf, "There is no parameters extracted from Module Parameters - check web admin\n");
    }
    g_api->append_resp_body( session, buf, strlen(buf));
    g_api->end_resp(session);
    return 0;
}

static int reg_handler(lsi_cb_param_t * rec)
{
    const char *uri;
    int len;
    uri = g_api->get_req_uri(rec->_session, &len);
    if ( len >= 10 && strcasestr(uri, "/testparam") )
    {
        g_api->register_req_handler(rec->_session, &MNAME, 10);
    }
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_URI_MAP, reg_handler, LSI_HOOK_EARLY, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int testparam_init(lsi_module_t * pModule)
{
    param_st *pparam_st = (param_st *) g_api->get_module_param(NULL, pModule);
    if (pparam_st) 
    {
        g_api->log( NULL,  LSI_LOG_INFO, "[testparam]Global level param: param1 = %d param2 = %d param3 = %d param4 = %d param5 = %d\n",
            pparam_st->param1, pparam_st->param2, 
            pparam_st->param3, pparam_st->param4, pparam_st->param5);
    }
    else
        g_api->log( NULL,  LSI_LOG_INFO, "[testparam]Global level NO params, ERROR.\n");

    return 0;
}

lsi_handler_t testparam_myhandler = { testparam_handlerBeginProcess, NULL, NULL, NULL };
lsi_config_t testparam_dealConfig = { testparam_parseConfig, testparam_freeConfig, myParam };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, testparam_init, &testparam_myhandler, &testparam_dealConfig, "Version 1.1", serverHooks };
