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
#include "loopbuff.h"

#include <stdlib.h>
#include <memory.h>
//#include <zlib.h>
#include <string.h>


/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       updatetcpout1
/////////////////////////////////////////////////////////////////////////////

lsi_module_t MNAME;
#define MAX_BLOCK_BUFSIZE   8192

typedef struct _MyData
{
    LoopBuff writeBuf;
} MyData;

int l4release(void *data)
{
    MyData *myData = (MyData *)data;
    g_api->log( NULL, LSI_LOG_DEBUG, "#### updatetcpout1 test %s\n", "l4release" );
    
    if (myData)
    {
        _loopbuff_dealloc(&myData->writeBuf);
        free (myData);
    }
    
    return 0;
}


int l4init( lsi_cb_param_t * rec )
{
    
    MyData *myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_L4);
    if (!myData)
    {
        myData = (MyData *) malloc(sizeof(MyData));
        _loopbuff_init(&myData->writeBuf);
        _loopbuff_alloc(&myData->writeBuf, MAX_BLOCK_BUFSIZE);
        
        g_api->log( NULL, LSI_LOG_DEBUG, "#### updatetcpout1 test %s\n", "l4init" );
        
        g_api->set_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_L4, (void *)myData);
    }
    else
    {
        _loopbuff_cleardata(&myData->writeBuf);
    }
    
    
    return 0;
}

static int l4send(lsi_cb_param_t * rec)
{
    MyData *myData = NULL;
    char *pBegin;
    struct iovec * iov = (struct iovec *)rec->_param;
    int count = rec->_param_len;
    int total = 0;
    char s[4] = {0};
    int written = 0;
    int i, j;
    struct iovec iovOut;

    g_api->log( NULL, LSI_LOG_DEBUG, "#### updatetcpoutdata test %s\n", "tcpsend" );
    myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_L4);
    if (MAX_BLOCK_BUFSIZE > _loopbuff_getdatasize(&myData->writeBuf))
    {
        
        for (i=0; i<count; ++i)
        {
            pBegin = (char *)iov[i].iov_base;
            
            for ( j=0; j<iov[i].iov_len; ++j )
            {
                sprintf(s, "=%02X", (unsigned char)pBegin[j]);
                _loopbuff_append(&myData->writeBuf, s, 3);
                ++ total;
            }
        }
    }

    _loopbuff_reorder(&myData->writeBuf);
    iovOut.iov_base = _loopbuff_getdataref(&myData->writeBuf);
    iovOut.iov_len = _loopbuff_getdatasize(&myData->writeBuf);
    written = g_api->stream_writev_next( rec, &iovOut, 1 );
    
    _loopbuff_erasedata(&myData->writeBuf, written);
    
    g_api->log( NULL, LSI_LOG_DEBUG, "#### updatetcpoutdata test, next caller written %d, return %d, left %d\n",  written, total, _loopbuff_getdatasize(&myData->writeBuf));
    
    int hasData  =1;
    if (_loopbuff_getdatasize(&myData->writeBuf))
        rec->_flag_out = (void *)&hasData;
    
    return total;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_L4_BEGINSESSION, l4init, LSI_HOOK_NORMAL, 0},
    {LSI_HKPT_L4_SENDING, l4send, LSI_HOOK_EARLY, LSI_HOOK_FLAG_TRANSFORM},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int init(lsi_module_t * pModule)
{
    g_api->init_module_data(pModule, l4release, LSI_MODULE_DATA_L4 );
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init, NULL, NULL, "", serverHooks};
