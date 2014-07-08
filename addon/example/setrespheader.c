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
#include <fcntl.h>

/******************************************************************************
 * 
 * HOW TO TEST:
 * use any url with qurey string like 
 * ?setrespheader[0123], then you will get different responese headers
 * 
 */

/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       setrespheader
/////////////////////////////////////////////////////////////////////////////

#define TEST_URL    "setrespheader"

lsi_module_t MNAME;


//test changing resp header
static int mycb(lsi_cb_param_t * rec)
{
    g_api->set_resp_header(rec->_session, LSI_RESP_HEADER_SERVER, NULL, 0, "/testServer 1.0", sizeof("/testServer 1.0") - 1, LSI_HEADER_SET);
    g_api->set_resp_header(rec->_session, LSI_RESP_HEADER_SET_COOKIE, NULL, 0, "my-test-cookie1", sizeof("my-test-cookie1") - 1, LSI_HEADER_ADD);
    g_api->set_resp_header(rec->_session, LSI_RESP_HEADER_SET_COOKIE, NULL, 0, "my-test-cookie2...", sizeof("my-test-cookie2...") - 1, LSI_HEADER_ADD);
    g_api->log( NULL, LSI_LOG_DEBUG, "#### mymodule1 test %s\n", "myCb" );
    return 0;
}

//test changing resp header
static int mycb2(lsi_cb_param_t * rec)
{
    g_api->set_resp_header2(rec->_session, "Addheader: MyTest addtional header2\r\n", sizeof("Addheader: MyTest addtional header2\r\n") -1, LSI_HEADER_SET);
    return 0;
}

int check_if_remove_session_hook(lsi_cb_param_t * rec)
{
    const char *qs;
    int sessionHookType = 0;
    qs = g_api->get_req_query_string(rec->_session, NULL );
    
    if ( strncasecmp(qs, TEST_URL, sizeof(TEST_URL) -1) == 0 )
    {
        sscanf(qs + sizeof(TEST_URL) -1, "%d", &sessionHookType);
        if (!(sessionHookType & 0x01))
            g_api->remove_session_hook(rec->_session, LSI_HKPT_SEND_RESP_HEADER, &MNAME );
        
        if (sessionHookType & 0x02)
            g_api->add_session_hook(rec->_session, LSI_HKPT_SEND_RESP_HEADER, &MNAME, mycb2, LSI_HOOK_LAST, 0 );
    }
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_RECV_REQ_HEADER, check_if_remove_session_hook, LSI_HOOK_NORMAL, 0},
    {LSI_HKPT_SEND_RESP_HEADER, mycb, LSI_HOOK_LAST, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int init(lsi_module_t * pModule)
{
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init, NULL, NULL, "", serverHooks };
