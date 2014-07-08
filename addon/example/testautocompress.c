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
#include <stdlib.h>
#include <string.h>

#define     MNAME       testautocompress
lsi_module_t MNAME;
/////////////////////////////////////////////////////////////////////////////
#define     VERSION         "V1.0"

static int viewData0(lsi_cb_param_t *rec, const char *level)
{
    if (rec->_param_len < 100)
        g_api->log(rec->_session, LSI_LOG_INFO, "[testautocompress] viewData [%s] %s\n",
                           level, (const char *)rec->_param);
    else
    {
        g_api->log(rec->_session, LSI_LOG_INFO, "[testautocompress] viewData [%s] ",  level);
        g_api->lograw(rec->_session, rec->_param, 40);
        g_api->lograw(rec->_session, "(...)", 5);
        g_api->lograw(rec->_session, rec->_param + rec->_param_len - 40, 40);
        g_api->lograw(rec->_session, "\n", 1);
    }
    return g_api->stream_write_next(rec, rec->_param, rec->_param_len);
}

static int viewData1(lsi_cb_param_t *rec) {   return viewData0(rec, "RECV");  }
static int viewData2(lsi_cb_param_t *rec) {   return viewData0(rec, "SEND");  }

static int beginSession(lsi_cb_param_t *rec)
{
    g_api->add_session_hook(rec->_session, LSI_HKPT_RECV_RESP_BODY, &MNAME, viewData1, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_DECOMPRESS_REQUIRED );
    g_api->add_session_hook(rec->_session, LSI_HKPT_SEND_RESP_BODY, &MNAME, viewData2, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_DECOMPRESS_REQUIRED );
    return 0;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_HTTP_BEGIN, beginSession, LSI_HOOK_NORMAL, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init()
{
    MNAME._info = VERSION;  //set version string
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL, "", serverHooks};
