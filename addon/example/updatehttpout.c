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
#include <lsr/lsr_loopbuf.h>
#include <lsr/lsr_xpool.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
//#include <zlib.h>
#include <string.h>

/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       updatehttpout
#define     TEST_STRING "_TEST_testmodule_ADD_A_STRING_HERE_<!--->"
/////////////////////////////////////////////////////////////////////////////

lsi_module_t MNAME;
#define MAX_BLOCK_BUFSIZE   8192

typedef struct _MyData
{
    lsr_loopbuf_t inWBuf;
    lsr_loopbuf_t outWBuf;
} MyData;



int httpRelease(void *data)
{
    g_api->log( NULL, LSI_LOG_DEBUG, "#### mymodulehttp %s\n", "httpRelease" );
    return 0;
}

int httpinit(lsi_cb_param_t * rec)
{
    MyData *myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP);
    lsr_xpool_t *pool = g_api->get_session_pool( rec->_session );
    myData = (MyData *)lsr_xpool_alloc( pool, sizeof( MyData ) );
    lsr_loopbuf_x( &myData->inWBuf, MAX_BLOCK_BUFSIZE, pool );
    lsr_loopbuf_x( &myData->outWBuf, MAX_BLOCK_BUFSIZE, pool );
    
    g_api->log( NULL, LSI_LOG_DEBUG, "#### mymodulehttp init\n" );
    g_api->set_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP, (void *)myData);
    return 0;
}

int httprespwrite(lsi_cb_param_t * rec)
{
    MyData *myData = NULL;
    lsr_xpool_t *pool = g_api->get_session_pool( rec->_session );
    const char *in = rec->_param;
    int inLen = rec->_param_len;
    int written, total = 0;
//    int j;
//    char s[4] = {0};
    myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP);

//     for ( j=0; j<inLen; ++j )
//     {
//         sprintf(s, "%c ", (unsigned char)in[j]);
//         lsr_loopbuf_xappend( &myData->m_outWBuf, s, 2, pool );
//         total += 2;
//     }

    //If have content, append a string for testing
    if (inLen > 0)
    {
        lsr_loopbuf_xappend( &myData->outWBuf, TEST_STRING, sizeof( TEST_STRING ) - 1, pool );
        lsr_loopbuf_xappend( &myData->outWBuf, in, inLen, pool );
        total = inLen + sizeof(TEST_STRING) -1;
    }
    
    lsr_loopbuf_xstraight( &myData->outWBuf, pool );
    
    written = g_api->stream_write_next( rec, lsr_loopbuf_begin( &myData->outWBuf ),
                                        lsr_loopbuf_size( &myData->outWBuf ) );
    lsr_loopbuf_pop_front( &myData->outWBuf, written );
    
    g_api->log( NULL, LSI_LOG_DEBUG, "#### mymodulehttp test, next caller written %d, return %d, left %d\n",  
                         written, total, lsr_loopbuf_size( &myData->outWBuf ));

    
    if ( !lsr_loopbuf_empty( &myData->outWBuf ))
    {
        int hasData = 1;
        rec->_flag_out = &hasData;
    }
    return inLen; //Because all data used, ruturn thr orignal length
}

static char *getNullEndString(const char *s, int len, char *str, int maxLength)
{
    if (len >= maxLength)
        len = maxLength - 1;
    memcpy(str, s, len);
    str[len] = 0x00;
    return str;
}


int httpreqHeaderRecved(lsi_cb_param_t * rec)
{
    const char *host, *ua, *uri, *accept;
    char *headerBuf;
    int hostLen, uaLen, acceptLen, headerLen; 
    char uaBuf[1024], hostBuf[128], acceptBuf[512];
    lsr_xpool_t *pPool = g_api->get_session_pool( rec->_session );
    
    uri = g_api->get_req_uri(rec->_session, NULL);
    host = g_api->get_req_header(rec->_session, "Host", 4, &hostLen);
    ua = g_api->get_req_header(rec->_session, "User-Agent", 10, &uaLen);
    accept = g_api->get_req_header(rec->_session, "Accept", 6, &acceptLen);
    g_api->log( NULL, LSI_LOG_DEBUG, "#### mymodulehttp test, httpreqHeaderRecved URI:%s host:%s, ua:%s accept:%s\n", 
                          uri, getNullEndString(host, hostLen, hostBuf, 128), getNullEndString(ua, uaLen, uaBuf, 1024), getNullEndString(accept, acceptLen, acceptBuf, 512) );
    
    headerLen = g_api->get_req_raw_headers_length(rec->_session);
    headerBuf = (char *)lsr_xpool_alloc( pPool, headerLen + 1 );
    memset(headerBuf, 0, headerLen + 1);
    g_api->get_req_raw_headers(rec->_session, headerBuf, headerLen);
    g_api->log( NULL, LSI_LOG_DEBUG, "#### mymodulehttp test, httpreqHeaderRecved whole header: %s, length: %d\n", 
                         headerBuf, headerLen);
  
    lsr_xpool_free( pPool, headerBuf );
    return 0;
    
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_HTTP_BEGIN, httpinit, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_ENABLED},
    {LSI_HKPT_RECV_RESP_BODY, httprespwrite, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_TRANSFORM | LSI_HOOK_FLAG_DECOMPRESS_REQUIRED | LSI_HOOK_FLAG_ENABLED},
    {LSI_HKPT_RECV_REQ_HEADER, httpreqHeaderRecved, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_ENABLED},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init(lsi_module_t * pModule)
{
    g_api->init_module_data(pModule, httpRelease, LSI_MODULE_DATA_HTTP );
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL, "", serverHooks};
