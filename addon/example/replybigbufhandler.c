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
#include <stdlib.h>
/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       replybigbufhandler
/////////////////////////////////////////////////////////////////////////////

lsi_module_t MNAME;

#define REPLY_BUFFER_LENGTH (1*1024*1024)
/**
 * This example is for testing reply a big buffer (20MB)
 * Since the buffer is so big, we can not append all the reply data to response cache
 * So OnWrite event need to be used to write the left data
 */

static int freeMydata(void *data)
{
    //The data is only the size, did not malloced a memory bloack, so no free needed
    return 0;
}

static int reg_handler(lsi_cb_param_t * rec)
{
    const char *uri;
    int len;
    uri = g_api->get_req_uri(rec->_session, &len);
    if ( len >= 12 && strncasecmp(uri, "/replybigbuf", 12) == 0 )
    {
        g_api->register_req_handler(rec->_session, &MNAME, 12);
        g_api->set_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP, (void *)REPLY_BUFFER_LENGTH);
    }
    return LSI_RET_OK;
}

static int disable_compress( lsi_cb_param_t *rec )
{
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_RECV_REQ_HEADER, reg_handler, LSI_HOOK_NORMAL, 0},
    {LSI_HKPT_RCVD_RESP_BODY, disable_compress, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_DECOMPRESS_REQUIRED},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int init( lsi_module_t * pModule)
{
    g_api->init_module_data(pModule, freeMydata, LSI_MODULE_DATA_HTTP );
    return 0;
}

//The first time the below function will be called, then onWriteEvent will be called next and next
static int myhandler_process(lsi_session_t *session)
{
    g_api->set_resp_header(session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0, "text/html", 9, LSI_HEADER_SET );
    int writeSize = sizeof("replybigbufhandler module reply the first line\r\n") - 1;
    long leftSize = (long)g_api->get_module_data( session, &MNAME, LSI_MODULE_DATA_HTTP );
    g_api->append_resp_body(session, "replybigbufhandler module reply the first line\r\n", writeSize);
    leftSize -= writeSize;
    g_api->set_module_data(session, &MNAME, LSI_MODULE_DATA_HTTP, (void *)leftSize);
    g_api->flush(session);
    return 0;
}

//return 0: error, 1: done, 2: not finished, definitions are in ls.h 
static int onWriteEvent( lsi_session_t *session)    
{
#define BLOACK_SIZE    (1024)
    int i;
    char buf[BLOACK_SIZE] = {0};
    int writeSize = 0;
    long leftSize = (long)g_api->get_module_data( session, &MNAME, LSI_MODULE_DATA_HTTP );
    
    while(leftSize > 0 && g_api->is_resp_buffer_available(session))
    {
        for (i=0;i<BLOACK_SIZE; ++i)
            buf[i] = 'A' + rand() % 52;
        writeSize = ((leftSize > BLOACK_SIZE) ? BLOACK_SIZE : leftSize);
        g_api->append_resp_body(session, buf, writeSize);
        leftSize -= writeSize;
    }
    g_api->set_module_data(session, &MNAME, LSI_MODULE_DATA_HTTP, (void *)leftSize);
    if (leftSize == 0)  
        g_api->append_resp_body(session, "\r\nEnd\r\n", 7);
    
    if (leftSize > 0)
        return LSI_WRITE_RESP_CONTINUE;
    else
        return LSI_WRITE_RESP_FINISHED;
}

lsi_handler_t myhandler = { myhandler_process, NULL, onWriteEvent, NULL };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init, &myhandler, NULL, "", serverHooks};
