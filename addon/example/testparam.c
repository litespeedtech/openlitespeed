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
#include "lsr/lsr_strtool.h"
#include "lsr/lsr_confparser.h"
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
    int param21;
    int param22;
    int param31;
    int param32;
    int param33;
    int param41;
    int param42;
    int param43;
    int param44;
    int param51;
    int param52;
    int param53;
    int param54;
    int param55;
} param_st;

//Setup the below array to let web server know these params
const char *myParam[] = {
    "param1", 
    "param2", 
    "param3", 
    "param4", 
    "param5", 
    NULL   //The last position must have a NULL to indicate end of the array
};

//return 0 for correctly parsing
static int testparam_parseList( lsr_objarray_t *pList, param_st *pConfig)
{
    int count = lsr_objarray_getsize( pList );
    assert(count > 0);
    
    unsigned long maxParamNum = 0;
    lsr_str_t *p = (lsr_str_t *)lsr_objarray_getobj( pList, 0 );
    
    if ( lsr_str_len( p ) != 6 || strncmp( "param", lsr_str_c_str( p ), 5 ) != 0 )
        return -2;
    
    
    //Comment: case param2, maxParamNum is 2, the line should be param2 [21 [22]],
    //in this way, count is 3, or 2 or 1
    maxParamNum = (unsigned long)strtol( lsr_str_c_str( p ) + 5, NULL, 10 );
    if (maxParamNum > 5)
        maxParamNum = 0;
    
    long val;
    int i;
    for (i =1; i < count && i < maxParamNum + 1; ++i)
    {
        p = (lsr_str_t *)lsr_objarray_getobj( pList, i );
        val = strtol( lsr_str_c_str( p ), NULL, 10 );
        
        int *pParam;
        switch(maxParamNum)
        {
        case 1:
            pParam = &pConfig->param1;
            break;
        case 2:
            pParam = &pConfig->param21 + (i - 1);
            break;
        case 3:
            pParam = &pConfig->param31 + (i - 1);
            break;
        case 4:
            pParam = &pConfig->param41 + (i - 1);
            break;
        case 5:
            pParam = &pConfig->param51 + (i - 1);
            break;
        }
        
        *pParam = val;
    }

    return 0;
}


static void * testparam_parseConfig( const char *param, int param_len, void *_initial_config, int level, const char *name )
{
    lsr_confparser_t confparser;
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
    
    lsr_confparser( &confparser );
    
    const char *pBufBegin = param, *pBufEnd = param + param_len;
    const char *pLine, *pLineEnd;
    while((pLine = lsr_get_conf_line( &pBufBegin, pBufEnd, &pLineEnd )) != NULL)
    {
        lsr_objarray_t *pList = lsr_conf_parse_line( &confparser, pLine, pLineEnd );
        if ( !pList )
            continue;
        
        testparam_parseList(pList, pConfig);
    }
    lsr_confparser_d( &confparser );
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
        sprintf(buf, "Current uri is %s.\nContext uri is %s.\nparam1 = %d\nparam2 = %d %d\nparam3 = %d %d %d\nparam4 = %d %d %d %d\nparam5 = %d %d %d %d %d\n",
                g_api->get_req_uri(session, NULL),
                g_api->get_mapped_context_uri(session, &len),
                pparam_st->param1, pparam_st->param21, pparam_st->param22,
                pparam_st->param31, pparam_st->param32, pparam_st->param33,  
                pparam_st->param41, pparam_st->param42, pparam_st->param43, pparam_st->param44, 
                pparam_st->param51, pparam_st->param52, pparam_st->param53, pparam_st->param54, pparam_st->param55);
 
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
    if ( len >= 10 && strcasestr(uri, "/testparam"))
    {
        g_api->register_req_handler(rec->_session, &MNAME, 10);
    }
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_URI_MAP, reg_handler, LSI_HOOK_EARLY, LSI_HOOK_FLAG_ENABLED},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int testparam_init(lsi_module_t * pModule)
{
    param_st *pparam_st = (param_st *) g_api->get_module_param(NULL, pModule);
    if (pparam_st) 
    {
        g_api->log( NULL,  LSI_LOG_INFO, "[testparam]Global level param: param1 = %d param2 = %d %d param3 = %d %d %d param4 = %d %d %d %d param5 = %d %d %d %d %d\n",
            pparam_st->param1, pparam_st->param21, pparam_st->param22,
            pparam_st->param31, pparam_st->param32, pparam_st->param33,  
            pparam_st->param41, pparam_st->param42, pparam_st->param43, pparam_st->param44, 
            pparam_st->param51, pparam_st->param52, pparam_st->param53, pparam_st->param54, pparam_st->param55);
    }
    else
        g_api->log( NULL,  LSI_LOG_INFO, "[testparam]Global level NO params, ERROR.\n");

    return 0;
}

lsi_handler_t testparam_myhandler = { testparam_handlerBeginProcess, NULL, NULL, NULL };
lsi_config_t testparam_dealConfig = { testparam_parseConfig, testparam_freeConfig, myParam };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, testparam_init, &testparam_myhandler, &testparam_dealConfig, "Version 1.1", serverHooks };
