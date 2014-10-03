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
#include <fcntl.h>

/******************************************************************************
 * 
 * HOW TO TEST:
 * Go to any url with the query string:
 * ?setrespheader[0123], then you will get different response headers
 * 
 * Tests:
 * mycb - The 1 bit in the query string is set.  Is an example of setting a response header.
 *      If 1 bit is not set, it is an example of removing a session hook.
 * mycb2 - The 2 bit in the query string is set.  Is an example of setting a response header.
 * mycb3 - The 4 bit is set.  Is an example of removing response headers.
 * mycb4 - The 8 bit is set.  Is an example of getting response headers.
 * 
 */

/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       setrespheader
/////////////////////////////////////////////////////////////////////////////

#define TEST_URL    "setrespheader"

lsi_module_t MNAME;

const char *headers[2] =
{
    "Set-Cookie",
    "Addheader"
};

const int numheaders = 2;

//test changing resp header
static int mycb(lsi_cb_param_t * rec)
{
    g_api->set_resp_header(rec->_session, LSI_RESP_HEADER_SERVER, NULL, 0, "/testServer 1.0", sizeof("/testServer 1.0") - 1, LSI_HEADER_SET);
    g_api->set_resp_header(rec->_session, LSI_RESP_HEADER_SET_COOKIE, NULL, 0, "An Example Cookie", sizeof("An Example Cookie") - 1, LSI_HEADER_ADD);
    g_api->set_resp_header(rec->_session, LSI_RESP_HEADER_SET_COOKIE, NULL, 0, "1 bit set!", sizeof("1 bit set!") - 1, LSI_HEADER_ADD);
    g_api->log( NULL, LSI_LOG_DEBUG, "#### mymodule1 test %s\n", "myCb" );
    return 0;
}

//test changing resp header
static int mycb2(lsi_cb_param_t * rec)
{
    g_api->set_resp_header2(rec->_session, "Addheader: 2 bit set!\r\n", sizeof("Addheader: 2 bit set!\r\n") -1, LSI_HEADER_SET);
    return 0;
}

static int mycb3(lsi_cb_param_t * rec)
{
    
    g_api->set_resp_header2(rec->_session, "Destructheader: 4 bit set! Removing other headers...\r\n", 
                            sizeof("Destructheader: 4 bit set! Removing other headers...\r\n") -1, LSI_HEADER_ADD);
    g_api->remove_resp_header( rec->_session, LSI_RESP_HEADER_SET_COOKIE, NULL, 0 );
    g_api->remove_resp_header( rec->_session, -1, "Addheader", sizeof("Addheader") - 1 );
    return 0;
}

static int mycb4(lsi_cb_param_t * rec)
{
    int i, j, iov_count = g_api->get_resp_headers_count( rec->_session );
    struct iovec iov_key[iov_count], iov_val[iov_count];
    memset( iov_key, 0, sizeof( struct iovec ) * iov_count );
    memset( iov_val, 0, sizeof( struct iovec ) * iov_count );
    
    g_api->set_resp_header2(rec->_session, "ProtectorHeader: 8 bit set! Duplicating headers for potential removal!\r\n", 
                            sizeof("ProtectorHeader: 8 bit set! Duplicating headers for potential removal!\r\n") -1, LSI_HEADER_ADD);
    iov_count = g_api->get_resp_headers( rec->_session, iov_key, iov_val, iov_count );
    for( i = iov_count - 1; i >= 0; --i )
    {
        for( j = 0; j < numheaders; ++j )
        {
            if ( strncmp( headers[j], (const char *)iov_key[i].iov_base, iov_key[i].iov_len ) == 0 )
            {
                char save[256];
                sprintf( save, "SavedHeader: %.*s\r\n",
                    iov_val[i].iov_len, (char *)iov_val[i].iov_base
                );
                g_api->set_resp_header2(rec->_session, save, strlen( save ), LSI_HEADER_ADD);
            }
        }
    }
    return 0;
}

int check_type(lsi_cb_param_t * rec)
{
    const char *qs;
    int sessionHookType = 0;
    qs = g_api->get_req_query_string(rec->_session, NULL );
    sessionHookType = strtol( qs + sizeof( TEST_URL ) - 1, NULL, 10 );
    if ( sessionHookType & 0x01 )
        mycb( rec );
    if ( sessionHookType & 0x02 )
        mycb2( rec );
    if ( sessionHookType & 0x08 )
        mycb4( rec );
    if ( sessionHookType & 0x04 )
        mycb3( rec );
    return LSI_RET_OK;
}

int check_if_remove_session_hook(lsi_cb_param_t * rec)
{
    const char *qs;
    int sessionHookType = 0;
    qs = g_api->get_req_query_string(rec->_session, NULL );
    if ( qs && strncasecmp(qs, TEST_URL, sizeof(TEST_URL) -1) == 0 )
    {
        sessionHookType = strtol( qs + sizeof( TEST_URL ) - 1, NULL, 10 );
        if (sessionHookType & 0x0f)
            g_api->set_session_hook_enable_flag(rec->_session, LSI_HKPT_SEND_RESP_HEADER, &MNAME, 1 );
    }
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_RECV_REQ_HEADER, check_if_remove_session_hook, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_ENABLED},
    {LSI_HKPT_SEND_RESP_HEADER, check_type, LSI_HOOK_NORMAL, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int init(lsi_module_t * pModule)
{
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init, NULL, NULL, "", serverHooks };
