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
#define     MNAME       testredirect
#define     TEST_URL       "/testredirect"
#define     TEST_URL_LEN       (sizeof(TEST_URL) -1)

#define DEST_URL        "/index.html"

/* This module tests redirect operations.
 * Valid urls are /testredirect?x-y
 * x is a LSI URL Operation.  Valid operations are outlined in the lsi_url_op enum in ls.h
 * y determines who handles the operation, 0 for the server, 1 for module.
 * 
 * Note: If y = 1, only the redirect URL Ops are valid.
 */

lsi_module_t MNAME;

int parse_qs( const char *qs, int *action, int *useHandler )
{
    int len;
    const char *pBuf;
    if ( !qs || (len = strlen( qs )) <= 0 )
        return -1;
    *action = strtol( qs, NULL, 10 );
    pBuf = memchr( qs, '-', len );
    if ( pBuf && useHandler )
        *useHandler = strtol( pBuf + 1, NULL, 10 );
    return 0;
}

void report_error( lsi_session_t *session, const char *qs )
{
    char errBuf[512];
    sprintf( errBuf, "Error: Invalid argument.\n"
                    "Query String was %s.\n"
                    "Expected d-d, where d is an integer.\n"
                    "Valid values for the first d are LSI URL Ops.\n"
                    "Valid values for the second d are\n"
                    "\t 0 for normal operations,\n"
                    "\t 1 for module handled operations.\n"
                    "If using module handled operations, can only use redirect URL Ops.\n",
                    qs
            );
    g_api->append_resp_body( session, errBuf, strlen( errBuf ));
    g_api->end_resp( session );
}

//Checks to make sure URL Action is valid.  0 for valid, -1 for invalid.
int test_range( int action )
{
    action = action & 127;
    if ( action == 0 || action == 1 )
        return 0;
    else if ( action > 16 && action < 23 )
        return 0;
    else if ( action > 32 && action < 39 )
        return 0;
    else if ( action > 48 && action < 55 )
        return 0;
    return -1;
}

int check_if_redirect( lsi_cb_param_t * rec )
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
        if ( parse_qs( qs, &action, &useHandler ) < 0 )
        {
            report_error( rec->_session, qs );
            return LSI_RET_OK;
        }
        if ( test_range( action ) < 0 )
            report_error( rec->_session, qs );
        else if ( !useHandler )
            g_api->set_uri_qs(rec->_session, action, DEST_URL, sizeof(DEST_URL) -1, "", 0 );
        else if ( action > 1 )
            g_api->register_req_handler(rec->_session, &MNAME, TEST_URL_LEN);
        else
            report_error( rec->_session, qs );
    }
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_RECV_REQ_HEADER, check_if_redirect, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_ENABLED},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init( lsi_module_t * pModule )
{
    return 0;
}

static int handlerBeginProcess( lsi_session_t *session )
{
    const char *qs;
    int action = LSI_URI_REWRITE;
    qs = g_api->get_req_query_string(session, NULL );
    if ( parse_qs( qs, &action, NULL ) < 0 )
    {
        report_error( session, qs );
        return LSI_RET_OK;
    }
    if ( action == 17 || action == 33 || action == 49 )
        report_error( session, qs );
    else
        g_api->set_uri_qs( session, action, DEST_URL, sizeof(DEST_URL) -1, "", 0 );
    return 0;
}

lsi_handler_t myhandler = { handlerBeginProcess, NULL, NULL, NULL };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, &myhandler, NULL, "test  redirect v1.0", serverHooks };
