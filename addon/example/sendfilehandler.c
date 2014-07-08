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

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       sendfilehandler
/////////////////////////////////////////////////////////////////////////////

lsi_module_t MNAME;
int dummycall(lsi_cb_param_t * rec)
{
    const char *in = (const char *)rec->_param;
    int inLen = rec->_param_len;
    int sent = g_api->stream_write_next( rec,  in, inLen );
    return sent;
}

int reg_handler(lsi_cb_param_t * rec)
{
    const char *uri;
    int len;
    uri = g_api->get_req_uri(rec->_session, &len);
    if ( len >= 9 && strncasecmp(uri, "/sendfile", 9) == 0 )
    {
        g_api->register_req_handler(rec->_session, &MNAME, 9);
    }
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_RECV_REQ_HEADER, reg_handler, LSI_HOOK_NORMAL, 0},
    //{LSI_HKPT_SEND_RESP_BODY, dummycall, LSI_HOOK_NORMAL, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int init(lsi_module_t * pModule)
{
    return 0;

}

static int myhandler_process(lsi_session_t *session)
{
    struct stat sb;
    const char *file = "/home/user/ls0312/DEFAULT/html/test1";
    off_t off = 0;
    off_t sz = -1; //-1 set to send all data
    struct iovec vec[5] = { {"123\r\n",5}, {"123 ",4}, {"523",3}, {"---",3}, {"1235\r\n",6}};
    
    g_api->append_resp_body( session, "Hi, test send file\r\n", sizeof("Hi, test send file\r\n") - 1 );
    g_api->append_resp_bodyv( session, vec, 5 );
    
    if (stat(file, &sb) != -1) 
    {
        sz = sb.st_size - 5;
    }
    
    if (g_api->send_file( session, file, off, sz ))
    {
        g_api->append_resp_body(session, "Sorry, send file error\r\n", sizeof("Sorry, send file error\r\n") - 1);
    }
    
    g_api->append_resp_body(session, "<p>The LAST<p>\r\n", 16);
    g_api->end_resp(session);
    return 0;
}

lsi_handler_t myhandler = { myhandler_process, NULL, NULL, NULL };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init, &myhandler, NULL, "", serverHooks };
