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

/**
 * This test module will reply the counter of how many times of your IP accessing, 
 * and of the page being accessed, and how mnay times of this file be accessed
 * If test uri is /testmoduledata/file1, and if /file1 exists in testing vhost directory
 * then the uri will be handled
 */



/***
 * HOW TO TEST
 * Create a file "aaa", "bbb" in the /DEFAULT/html, 
 * then curl http://127.0.0.1:8088/testmoduledata/aaa and 
 * curl http://127.0.0.1:8088/testmoduledata/bbb,
 * you will see the result
 * 
 */
#include "../include/ls.h"
#include "loopbuff.h"
#include <string.h>
#include <stdint.h>
#include "stdlib.h"
#include <unistd.h>

#define     MNAME       testmoduledata
lsi_module_t MNAME;

#define URI_PREFIX   "/testmoduledata"
#define max_file_len    1024

typedef struct {
    long count;
} CounterData;

int releaseCounterDataCb( void *data )
{
    if (!data)
        return 0;
    
    CounterData *pData = (CounterData *)data;
    pData->count = 0;
    free(pData);
    return 0;
}

CounterData *allocateMydata(lsi_session_t *session, const lsi_module_t *module, int level)
{
    CounterData *myData = (CounterData*)malloc(sizeof(CounterData));
    if (myData == NULL )
        return NULL;

    memset(myData, 0, sizeof(CounterData));
    g_api->set_module_data(session, module, level, (void *)myData);
    return myData;
}

int assignHandler(lsi_cb_param_t * rec)
{
    const char *p;
    char path[max_file_len] = {0};
    CounterData *file_data;
    int len;
    const char *uri = g_api->get_req_uri(rec->_session, &len);
    if (len > strlen(URI_PREFIX) &&      
        strncasecmp(uri, URI_PREFIX, strlen(URI_PREFIX)) == 0)
    {
        p = uri + strlen(URI_PREFIX);
        if (0 == g_api->get_uri_file_path(rec->_session, p, strlen(p), path, max_file_len) &&
            access(path, 0) != -1)
        {
            g_api->set_req_env(rec->_session, "cache-control", 13, "no-cache", 8);
            g_api->register_req_handler( rec->_session, &MNAME, 0);
            //set the FILE data here, so that needn't to parse the file path again later
            g_api->init_file_type_mdata(rec->_session, &MNAME, path, strlen(path));
            file_data = (CounterData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_FILE);
            if (file_data == NULL)
            {
                file_data = allocateMydata(rec->_session, &MNAME, LSI_MODULE_DATA_FILE);
            }
        }
    }
    return 0;
}

static int myhandler_process(lsi_session_t *session)
{
    CounterData *ip_data = NULL, *vhost_data = NULL, *file_data = NULL;
    char output[128];
    
    ip_data = (CounterData *)g_api->get_module_data(session, &MNAME, LSI_MODULE_DATA_IP);
    if (ip_data == NULL)
    {
        ip_data = allocateMydata(session, &MNAME, LSI_MODULE_DATA_IP);
    }
    if (ip_data == NULL)
        return 500;
    
    vhost_data = (CounterData *)g_api->get_module_data(session, &MNAME, LSI_MODULE_DATA_VHOST);
    if (vhost_data == NULL)
    {
        vhost_data = allocateMydata(session, &MNAME, LSI_MODULE_DATA_VHOST);
    }
    if (vhost_data == NULL)
        return 500;
    
    file_data = (CounterData *)g_api->get_module_data(session, &MNAME, LSI_MODULE_DATA_FILE);
    if (file_data == NULL)
        return 500;
    
    ++ip_data->count;
    ++file_data->count;
    ++vhost_data->count;
    
    sprintf(output, "IP counter = %ld\nVHost counter = %ld\nFile counter = %ld\n", 
            ip_data->count, vhost_data->count, file_data->count);
    g_api->append_resp_body(session, output, strlen(output));
    g_api->end_resp(session);
    return 0;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_RECV_REQ_HEADER, assignHandler, LSI_HOOK_NORMAL, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init(lsi_module_t * pModule)
{
    g_api->init_module_data(pModule, releaseCounterDataCb, LSI_MODULE_DATA_VHOST );
    g_api->init_module_data(pModule, releaseCounterDataCb, LSI_MODULE_DATA_IP );
    g_api->init_module_data(pModule, releaseCounterDataCb, LSI_MODULE_DATA_FILE );
    return 0;
}

lsi_handler_t myhandler = { myhandler_process, NULL, NULL, NULL };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, &myhandler, NULL, "", serverHooks};
