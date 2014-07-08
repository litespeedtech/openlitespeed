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

#include "../include/ls.h"
#include "loopbuff.h"
#include <string.h>
#include <stdint.h>
#include "stdlib.h"
#include <unistd.h>

#define     MNAME       testenv
lsi_module_t MNAME;

#define URI_PREFIX   "/testenv"
#define max_file_len    1024

/**
 * HOW TO TEST
 * //testenv?modcompress:S1P-3000R1P-3000&moddecompress:S1P3000R1P3000
 */

int assignHandler(lsi_cb_param_t * rec)
{
    int len;
    const char *p;
    const char *pEnd;
    const char *uri = g_api->get_req_uri(rec->_session, &len);
    const char *nameStart;
    char name[128];
    int nameLen;
    const char *valStart;
    char val[128];
    int valLen;
    if (len < strlen(URI_PREFIX) ||
        strncasecmp(uri, URI_PREFIX, strlen(URI_PREFIX)) != 0)
        return 0;
    
    g_api->register_req_handler( rec->_session, &MNAME, 0);
    p = g_api->get_req_query_string(rec->_session, &len);
    pEnd = p + len;
    while (p && p < pEnd)
    {
        nameStart = p;
        p = strchr(p, ':');
        if (p)
        {
            nameLen = p - nameStart;
            strncpy(name, nameStart, nameLen);
            name[nameLen] = 0x00;
            ++p;
            
            valStart = p;
            p = strchr(p, '&');
            if(p)
            {
                valLen = p - valStart;
                ++p;
            }
            else
            {
                valLen = pEnd - valStart;
                p = pEnd;
            }

            strncpy(val, valStart, valLen);
            val[valLen] = 0x00;
            
            g_api->set_req_env(rec->_session, name, nameLen, val, valLen);
            g_api->log(rec->_session, LSI_LOG_INFO, "[Module:testEnv] setEnv name[%s] val[%s]\n",name, val);
            
        }
        else
            break;
    }
    
    return 0;
}

static int myhandler_process(lsi_session_t *session)
{
    int i;
    //200KB
    char buf[51] = {0};
    int count =0;
    for (i=0; i<2000; ++i)
    {
        snprintf(buf, 51,"%04d--0123456789012345678901234567890123456789**\r\n", ++count);
        g_api->append_resp_body(session, buf, 50);
    }
    g_api->end_resp(session);
    return 0;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_RECV_REQ_HEADER, assignHandler, LSI_HOOK_NORMAL, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init(lsi_module_t * pModule)
{
    return 0;
}

lsi_handler_t myhandler = { myhandler_process, NULL, NULL, NULL };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, &myhandler, NULL, "", serverHooks };
