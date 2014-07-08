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
#include <string.h>
#define     MNAME       testredirect
#define     TEST_URL       "/testredirect"
#define     TEST_URL_LEN       (sizeof(TEST_URL) -1)

#define DEST_URL        "/index.html"



lsi_module_t MNAME;

int check_if_redirect(lsi_cb_param_t * rec)
{
    const char *uri;
    const char *qs;
    int action = LSI_URI_REWRITE;
    int useHandler = 0;
    int len;
    uri = g_api->get_req_uri(rec->_session, &len);
    if ( len >= strlen(TEST_URL) && strncasecmp(uri, TEST_URL, strlen(TEST_URL)) == 0 )
    {
        qs = g_api->get_req_query_string(rec->_session, NULL );
        sscanf(qs, "%d-%d", &action, &useHandler);
        if (!useHandler)
            g_api->set_uri_qs(rec->_session, action, DEST_URL, sizeof(DEST_URL) -1, "", 0 );
        else
            g_api->register_req_handler(rec->_session, &MNAME, TEST_URL_LEN);
    }
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_RECV_REQ_HEADER, check_if_redirect, LSI_HOOK_NORMAL, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init( lsi_module_t * pModule )
{
    return 0;
}

static int handlerBeginProcess(lsi_session_t *session)
{
    const char *qs;
    int action = LSI_URI_REWRITE;
    qs = g_api->get_req_query_string(session, NULL );
    sscanf(qs, "%d", &action);
    g_api->set_uri_qs(session, action, DEST_URL, sizeof(DEST_URL) -1, "", 0 );
    return 0;
}

lsi_handler_t myhandler = { handlerBeginProcess, NULL, NULL, NULL };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, &myhandler, NULL, "test  redirect v1.0", serverHooks };
