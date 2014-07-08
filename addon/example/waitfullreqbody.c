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


/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
//You can call the below to set to wait req body read and then start to handle the req 
//either in the filer or in the beginProcess() of the handler struct

#define     MNAME       waitfullreqbody
/////////////////////////////////////////////////////////////////////////////

#define     TESTURI         "/waitfullreqbody"
#define     VERSION         "V1.2"
#define     CONTENT_HEAD    "<html><head><title>waitfullreqbody</title></head><body><p>\r\nHead<p>\r\n"
#define     CONTENT_FORMAT  "<tr><td>%s</td><td>%s</td></tr><p>\r\n"
#define     CONTENT_TAIL    "</body></html><p>\r\n"


lsi_module_t MNAME;
#define MAX_BLOCK_BUFSIZE   8192

typedef struct _MyDatas
{
    LoopBuff inBuf;
} MyData;



static int httpRelease(void *data)
{
    MyData *myData = (MyData *)data;
    g_api->log( NULL, LSI_LOG_DEBUG, "#### waitfullreqbody %s\n", "httpRelease" );
    if (myData)
    {
        _loopbuff_dealloc(&myData->inBuf);
        free (myData);
    }
    return 0;
}

static int httpinit(lsi_cb_param_t * rec)
{
    MyData *myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP);
    if (myData == NULL )
    {
        myData = (MyData *) malloc(sizeof(MyData));
        _loopbuff_init(&myData->inBuf);
        _loopbuff_alloc(&myData->inBuf, MAX_BLOCK_BUFSIZE);
        g_api->log( NULL, LSI_LOG_DEBUG, "#### waitfullreqbody init\n" );
        g_api->set_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP, (void *)myData);
    } 
    else
    {
        _loopbuff_dealloc(&myData->inBuf);
    }
    
    return 0;
}

/**
 * httpreqread will try to read from the next step of the stream as much as possible
 * and hold the data, then double each char to the buffer. 
 * If finally it has hold data, set the hasBufferedData to 1 with *((int *)rec->_flag_out) = 1;
 * DO NOT SET TO 0, because the other module may already set to 1 when pass in this function.
 * 
 */
static int httpreqread(lsi_cb_param_t * rec)
{
    MyData *myData = NULL;
    char *pBegin;
    char tmpBuf[MAX_BLOCK_BUFSIZE];
    int len, sz, i;
    char *p = (char *)rec->_param;
    
    myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP);
    if (!myData)
        return -1;
        
    while((len = g_api->stream_read_next( rec, tmpBuf, MAX_BLOCK_BUFSIZE )) > 0)
    {
        g_api->log( NULL, LSI_LOG_DEBUG, "#### waitfullreqbody httpreqread, inLn = %d\n", len );
        _loopbuff_append(&myData->inBuf, tmpBuf, len);
    }
    
    while (_loopbuff_hasdata(&myData->inBuf) && (p - (char *)rec->_param < rec->_param_len) )
    {
        _loopbuff_reorder(&myData->inBuf);
        pBegin = _loopbuff_getdataref(&myData->inBuf);
        sz = _loopbuff_getdatasize(&myData->inBuf);

//#define TESTCASE_2
#ifndef TESTCASE_2
        //test case 1: double each cahr
        if (sz > rec->_param_len / 2)
            sz = rec->_param_len / 2;
        for ( i=0; i<sz; ++i)
        {
            *p++ = *pBegin;
            *p++ = *pBegin;
            ++ pBegin;
        }
#else
        //test case 2: shink to half of the org req body
        if (sz > rec->_param_len * 2)
            sz = rec->_param_len * 2;
        for ( i=0; i< sz / 2; ++i)
        {
            *p++ = *pBegin;
            pBegin += 2;
        }
#endif

        _loopbuff_erasedata(&myData->inBuf, sz);
    }
    
    rec->_param_len = p - (char *)rec->_param;
    if (_loopbuff_hasdata(&myData->inBuf))
        *((int *)rec->_flag_out) = 1;
    
    return rec->_param_len;
}


static int check_uri_and_reg_handler(lsi_cb_param_t * rec)
{
    const char *uri;
    int len;
    uri = g_api->get_req_uri(rec->_session, &len);
    if ( len >= strlen(TESTURI) && strncasecmp(uri, TESTURI, strlen(TESTURI)) == 0 )
    {
        g_api->register_req_handler( rec->_session, &MNAME, strlen(TESTURI) );
        //g_api->set_req_wait_full_body( rec->_session );
    }
    return LSI_RET_OK;
}


static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_RECV_REQ_HEADER, check_uri_and_reg_handler, LSI_HOOK_NORMAL, 0},
    {LSI_HKPT_HTTP_BEGIN, httpinit, LSI_HOOK_NORMAL, 0},
    {LSI_HKPT_RECV_REQ_BODY, httpreqread, LSI_HOOK_EARLY, LSI_HOOK_FLAG_TRANSFORM},
    
    lsi_serverhook_t_END   //Must put this at the end position
};


static int _init( lsi_module_t * pModule )
{
    pModule->_info = VERSION;  //set version string
    g_api->init_module_data(pModule, httpRelease, LSI_MODULE_DATA_HTTP );
    
    return 0;
}

static int handleReqBody( lsi_session_t *session )
{
    char buf[MAX_BLOCK_BUFSIZE];
    int ret;
    int count = 0;
    if (!g_api->is_req_body_finished(session))
    {
        sprintf(buf, CONTENT_FORMAT, "ERROR", "You must forget to set wait all req body!!!<p>\r\n");
        g_api->append_resp_body(session, buf, strlen(buf));
        return 1;
    }
    
    g_api->append_resp_body(session, "<tr><td>", strlen("<tr><td>"));
    g_api->append_resp_body(session, "WHOLE REQ BODY", strlen("WHOLE REQ BODY"));
    g_api->append_resp_body(session, "</td><td>", strlen("</td><td>"));     
    
    while( (ret = g_api->read_req_body( session, buf, MAX_BLOCK_BUFSIZE)) > 0)
    {
        g_api->append_resp_body(session, buf, ret);
        count += ret;
    }
    g_api->append_resp_body(session, "</td></tr><p>\r\n", strlen("</td></tr><p>\r\n"));

    sprintf(buf, "<p>total req length read is %d<p>\r\n", count);
    g_api->append_resp_body(session, buf, strlen(buf));
    g_api->end_resp(session);
    return 0;
}

static int handlerBeginProcess( lsi_session_t *session )
{
    g_api->set_req_wait_full_body( session );
    g_api->append_resp_body(session, CONTENT_HEAD, strlen(CONTENT_HEAD));
    if (g_api->is_req_body_finished(session))
    {
        g_api->append_resp_body(session, "Action in handlerBeginProcess<p>\r\n", strlen("Action in handlerBeginProcess<p>\r\n"));
        handleReqBody( session );
    }
    else
        g_api->flush(session);
    
    return 0;
}

static int cleanUp( lsi_session_t *session)
{
    g_api->free_module_data(session, &MNAME, LSI_MODULE_DATA_HTTP, httpRelease);
    return 0;
}

lsi_handler_t reqHandler = { handlerBeginProcess, handleReqBody, NULL, cleanUp };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, &reqHandler, NULL, "", serverHooks };
