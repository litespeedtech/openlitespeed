/*
Copyright (c) 2013, LiteSpeed Technologies Inc.
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
#include <time.h>
#include <string.h>

/**
 * HOW TO TEST
 * test url ....../testtimer wit Get or post, request also can have a request body.
 * One line response will be displayed in browser and suspend,
 * and after 5 seconds, it will continue to display the left. * 
 * If with a req body, it will be displayed in the browser
 * 
 */
/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       testtimer
/////////////////////////////////////////////////////////////////////////////

lsi_module_t MNAME;
static int onReadEvent( lsi_session_t *session);

int reg_handler(lsi_cb_param_t * rec)
{
    const char *uri;
    int len;
    uri = g_api->get_req_uri(rec->_session, &len);
    if ( len >= 10 && strncasecmp(uri, "/testtimer", 10) == 0 )
    {
        g_api->register_req_handler(rec->_session, &MNAME, 10);
        g_api->set_req_env(rec->_session, "cache-control", 13, "max-age 20", 10);
    }
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_RECV_REQ_HEADER, reg_handler, LSI_HOOK_NORMAL, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int init(lsi_module_t * pModule )
{
    return 0;
}

void timer_callback(void * session)
{
    char buf[1024];
    sprintf(buf, "timer triggered and cotinue to write, time: %ld", (long)(time(NULL)));
    g_api->append_resp_body((lsi_session_t *)session, buf, strlen(buf));
    g_api->set_handler_write_state((lsi_session_t *)session, 1);
}

//The first time the below function will be called, then onWriteEvent will be called next and next
static int myhandler_process(lsi_session_t *session)
{
    char tmBuf[30];
    time_t t;
    struct tm *tmp;
    t = time(NULL);
    tmp = gmtime(&t);
    strftime( tmBuf, 30, "%a, %d %b %Y %H:%M:%S GMT", tmp );
    g_api->set_resp_header(session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0, "text/html", sizeof("text/html") - 1, LSI_HEADER_SET );
    g_api->set_resp_header(session, LSI_RESP_HEADER_LAST_MODIFIED, NULL, 0, tmBuf, 29, LSI_HEADER_SET);
    
    char buf[1024];
    sprintf(buf, "This test will take 5 seconds, now time is %ld<p>", (long)(time(NULL)));
    g_api->append_resp_body(session, buf, strlen(buf));
    g_api->flush(session);
    g_api->set_handler_write_state(session, 0);
    g_api->set_timer(5000, timer_callback, session);

    return 0;
}

static int onReadEvent( lsi_session_t *session)
{
    char buf[8192];
    g_api->append_resp_body(session, "I got req body:<br>", sizeof("I got req body:<br>") -1 );
    int ret;
    while( (ret = g_api->read_req_body( session, buf, 8192)) > 0)
        g_api->append_resp_body(session, buf, ret);
    return 0;
}

static int onWriteEvent( lsi_session_t *session)
{
    g_api->append_resp_body(session, "<br>Written finished and bye.<p>", sizeof("<br>Written finished and bye.<p>") -1 );
    return LSI_WRITE_RESP_FINISHED;
}

lsi_handler_t myhandler = { myhandler_process, onReadEvent, onWriteEvent, NULL };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init, &myhandler, NULL, "", serverHooks};
