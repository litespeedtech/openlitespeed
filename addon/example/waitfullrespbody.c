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
#include "loopbuff.h"

#define     MNAME       waitfullrespbody
#define     TESTURI     "waitfullrespbody"
#define     DEST_URL    "/index.html"
lsi_module_t MNAME;
/////////////////////////////////////////////////////////////////////////////
#define     VERSION         "V1.0"


static int internalRedir(lsi_cb_param_t * rec)
{
    int action = LSI_URL_REDIRECT_INTERNAL;
    g_api->set_uri_qs(rec->_session, action, DEST_URL, sizeof(DEST_URL) -1, "", 0 );
    return LSI_RET_OK;
}

static int denyAccess(lsi_cb_param_t * rec)
{
    g_api->set_status_code( rec->_session, 406 );
    return LSI_RET_ERROR;
}

static int getTestType(lsi_cb_param_t * rec)
{
    int type = 0;
    int len;
    const char *qs = g_api->get_req_query_string(rec->_session, &len);
    if ( len >= strlen(TESTURI) && strncasecmp(qs, TESTURI, strlen(TESTURI)) == 0 )
        sscanf(qs + strlen(TESTURI), "%d", &type);

    if (type == 0)
        return 0;
    if ( type <= 10 )
        g_api->set_resp_wait_full_body( rec->_session );
    switch(type)
    {
    case 1:
        g_api->add_session_hook( rec->_session, LSI_HKPT_RECV_RESP_HEADER, &MNAME, internalRedir, LSI_HOOK_NORMAL, 0 );
        break;
    case 2:
        g_api->add_session_hook( rec->_session, LSI_HKPT_RECV_RESP_BODY, &MNAME, internalRedir, LSI_HOOK_NORMAL, 0 );
        break;
    case 3:
        g_api->add_session_hook( rec->_session, LSI_HKPT_RCVD_RESP_BODY, &MNAME, internalRedir, LSI_HOOK_NORMAL, 0 );
        break;
    case 4:
        g_api->add_session_hook( rec->_session, LSI_HKPT_SEND_RESP_HEADER, &MNAME, internalRedir, LSI_HOOK_NORMAL, 0 );
        break;
    case 5:
        g_api->add_session_hook( rec->_session, LSI_HKPT_RECV_RESP_HEADER, &MNAME, denyAccess, LSI_HOOK_NORMAL, 0 );
        break;
    case 6:
        g_api->add_session_hook( rec->_session, LSI_HKPT_RECV_RESP_BODY, &MNAME, denyAccess, LSI_HOOK_NORMAL, 0 );
        break;
    case 7:
        g_api->add_session_hook( rec->_session, LSI_HKPT_RCVD_RESP_BODY, &MNAME, denyAccess, LSI_HOOK_NORMAL, 0 );
        break;
    case 8:
        g_api->add_session_hook( rec->_session, LSI_HKPT_SEND_RESP_HEADER, &MNAME, denyAccess, LSI_HOOK_NORMAL, 0 );
        break;
    default:
        break;
    }
    return 0;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_RECV_REQ_HEADER, getTestType, LSI_HOOK_NORMAL, 0},
    
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init( lsi_module_t * pModule )
{
    pModule->_info = VERSION;  //set version string
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL, "", serverHooks };
