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

#define     MNAME       testhttpauth
lsi_module_t MNAME;

int httpAuth(lsi_cb_param_t *rec)
{
    //test if the IP is 127.0.0.1 pass through, otherwise, reply 403
    char ip[16] = {0};
    g_api->get_req_var_by_id( rec->_session, LSI_REQ_GEOIP_ADDR, ip, 16);
    if (strcmp(ip, "127.0.0.1") == 0)
        return LSI_RET_OK;
    else
    {
        g_api->set_status_code(rec->_session, 403);
        g_api->log(rec->_session, LSI_LOG_INFO, "Access denied since ip = %s.\n", ip);
        return LSI_RET_ERROR;
    }
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_HTTP_AUTH, httpAuth, LSI_HOOK_NORMAL, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init(lsi_module_t * pModule)
{
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL, "", serverHooks};
