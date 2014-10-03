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
#include <lsr/lsr_base64.h>
#include <lsr/lsr_loopbuf.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       updatetcpin1
/////////////////////////////////////////////////////////////////////////////

lsi_module_t MNAME;
#define MAX_BLOCK_BUFSIZE   8192

typedef struct _MyData
{
    lsr_loopbuf_t inBuf;
    lsr_loopbuf_t outBuf;
} MyData;

int l4release(void *data)
{
    MyData *myData = (MyData *)data;
    g_api->log( NULL, LSI_LOG_DEBUG, "#### updatetcpin1 test %s\n", "l4release" );
    
    if (myData)
    {
        lsr_loopbuf_d( &myData->inBuf );
        lsr_loopbuf_d( &myData->outBuf );
        free (myData);
    }
    
    return 0;
}


int l4init1( lsi_cb_param_t * rec )
{
    MyData *myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_L4);
    if (!myData)
    {
        myData = (MyData *) malloc(sizeof(MyData));
        lsr_loopbuf( &myData->inBuf, MAX_BLOCK_BUFSIZE );
        lsr_loopbuf( &myData->outBuf, MAX_BLOCK_BUFSIZE );
        
        g_api->log( NULL, LSI_LOG_DEBUG, "#### updatetcpin1 test %s\n", "l4init" );
        g_api->set_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_L4, (void *)myData);
    }
    else
    {
        lsr_loopbuf_clear( &myData->inBuf );
        lsr_loopbuf_clear( &myData->outBuf );
    }
    
    return 0;
}

//expand the recieved data to base64 encode
int l4recv1(lsi_cb_param_t * rec)
{
#define PLAIN_BLOCK_SIZE 600
#define ENCODE_BLOCK_SIZE (PLAIN_BLOCK_SIZE * 4 / 3 + 1)
  
    MyData *myData = NULL;
    char *pBegin;
    char tmpBuf[ENCODE_BLOCK_SIZE];
    int len, sz;
    
    myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_L4);
    if (!myData)
        return -1;
        
    while((len = g_api->stream_read_next( rec, tmpBuf, ENCODE_BLOCK_SIZE )) > 0)
    {
        g_api->log( NULL, LSI_LOG_DEBUG, "#### updatetcpin1 test l4recv, inLn = %d\n", len );
        lsr_loopbuf_append( &myData->inBuf, tmpBuf, len );
    }
    
    while( !lsr_loopbuf_empty( &myData->inBuf ))
    {
        lsr_loopbuf_straight( &myData->inBuf );
        pBegin = lsr_loopbuf_begin( &myData->inBuf );
        sz = lsr_loopbuf_size( &myData->inBuf );
        if (sz > PLAIN_BLOCK_SIZE)
            sz = PLAIN_BLOCK_SIZE;
        len = lsr_base64_encode( (const char *)pBegin, sz, tmpBuf );
        if (len > 0)
        {
            lsr_loopbuf_append( &myData->outBuf, tmpBuf, len );
            lsr_loopbuf_pop_front( &myData->inBuf, sz );
        }
        else
            break;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    if ( lsr_loopbuf_size( &myData->outBuf ) < rec->_param_len )
    {
        rec->_param_len = lsr_loopbuf_size( &myData->outBuf );
    }
    
    if (rec->_param_len > 0)
    {
        lsr_loopbuf_move_to( &myData->outBuf, (char *)rec->_param, rec->_param_len);
    }
    
    return rec->_param_len;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_L4_BEGINSESSION, l4init1, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_ENABLED},
    {LSI_HKPT_L4_RECVING, l4recv1, LSI_HOOK_EARLY, LSI_HOOK_FLAG_TRANSFORM | LSI_HOOK_FLAG_ENABLED},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int init( lsi_module_t * pModule )
{
    g_api->init_module_data(pModule, l4release, LSI_MODULE_DATA_L4 );
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init, NULL, NULL, "", serverHooks};
