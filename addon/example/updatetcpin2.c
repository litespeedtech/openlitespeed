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
#include <string.h>

///////////////////////////////////
static const unsigned char s_decodeTable[128] =
{
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  /* 00-0F */
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  /* 10-1F */
    255,255,255,255,255,255,255,255,255,255,255,62,255,255,255,63,  /* 20-2F */
    52,53,54,55,56,57,58,59,60,61,255,255,255,255,255,255,          /* 30-3F */
    255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,               /* 40-4F */
    15,16,17,18,19,20,21,22,23,24,25,255,255,255,255,255,           /* 50-5F */
    255,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,               /* 60-6F */
    41,42,43,44,45,46,47,48,49,50,51,255,255,255,255,255            /* 70-7F */
};

static const unsigned char s_encodeTable[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            
int Base64_decode( const char * encoded, int size, char * decoded )
{
    register const char * pEncoded = encoded;
    register unsigned char e1, prev_e = 0;
    register char           phase = 0;
    register unsigned char * pDecoded = (unsigned char *)decoded;
    register const char * pEnd = encoded + size ;
    
    while(  pEncoded < pEnd )
    {
        register int ch = *pEncoded++;
        if ( ch < 0 )
            continue;
        e1 = s_decodeTable[ ch ];
        if ( e1 != 255 )
        {
            switch ( phase )
            {
            case 0:
                break;
            case 1:
                *pDecoded++ = ( ( prev_e << 2 ) | ( ( e1 & 0x30 ) >> 4 ) );
                break;
            case 2:
                *pDecoded++ = ( ( ( prev_e & 0xf ) << 4 ) | ( ( e1 & 0x3c ) >> 2 ) );
                break;
            case 3:
                *pDecoded++ = ( ( ( prev_e & 0x03 ) << 6 ) | e1 );
                phase = -1;
            break;
            }
            phase++;
            prev_e = e1;
        }
    }
    *pDecoded = 0;
    return pDecoded - (unsigned char *)decoded;
}

int Base64_encode( const char *decoded, int size, char * encoded )
{
    register const unsigned char * pDecoded = (const unsigned char *)decoded;
    register const unsigned char * pEnd = (const unsigned char *)decoded + size ;    
    register char * pEncoded = encoded;
    register unsigned char ch;
    
    size %= 3;
    pEnd -= size;

    while ( pEnd > pDecoded ) 
    {
        *pEncoded++ = s_encodeTable[( ( ch = *pDecoded++ ) >> 2 ) & 0x3f];
        *pEncoded++ = s_encodeTable[(( ch & 0x03 ) << 4 ) | ( *pDecoded >> 4 )];
        ch = *pDecoded++;
        *pEncoded++ = s_encodeTable[(( ch & 0x0f ) << 2 ) | ( *pDecoded >> 6 )];
        *pEncoded++ = s_encodeTable[ *pDecoded++ & 0x3f];
    }

    if ( size > 0 )
    {
        *pEncoded++ = s_encodeTable[( (ch = *pDecoded++) >> 2 ) & 0x3f];

        if (size == 1)
        {
            *pEncoded++ = s_encodeTable[( ch & 0x03 ) << 4];
            *pEncoded++ = '=';

        }
        else
        {
            *pEncoded++ = s_encodeTable[(( ch & 0x03 ) << 4) | ( *pDecoded >> 4 )];
            *pEncoded++ = s_encodeTable[( *pDecoded & 0x0f ) << 2];
        }
        *pEncoded++ = '=';
    }

    return pEncoded - encoded;
}


/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       updatetcpin2
/////////////////////////////////////////////////////////////////////////////

lsi_module_t MNAME;
#define MAX_BLOCK_BUFSIZE   8192

typedef struct _MyData2
{
    LoopBuff inBuf;
    LoopBuff outBuf;
} MyData2;

static int l4release2(void *data)
{
    MyData2 *myData = (MyData2 *)data;
    g_api->log( NULL, LSI_LOG_DEBUG, "#### updatetcpin1 test %s\n", "l4release" );
    
    if (myData)
    {
        _loopbuff_dealloc(&myData->inBuf);
        _loopbuff_dealloc(&myData->outBuf);
        free (myData);
    }
    
    return 0;
}


static int l4init2( lsi_cb_param_t * rec )
{
    MyData2 *myData = (MyData2 *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_L4);
    if (!myData)
    {
        myData = (MyData2 *) malloc(sizeof(MyData2));
        _loopbuff_init(&myData->inBuf);
        _loopbuff_init(&myData->outBuf);
        _loopbuff_alloc(&myData->inBuf, MAX_BLOCK_BUFSIZE);
        _loopbuff_alloc(&myData->outBuf, MAX_BLOCK_BUFSIZE);
        
        g_api->log( NULL, LSI_LOG_DEBUG, "#### updatetcpin2 test %s\n", "l4init" );
        g_api->set_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_L4, (void *)myData);
    }
    else
    {
        _loopbuff_cleardata(&myData->inBuf);
        _loopbuff_cleardata(&myData->outBuf);
    }
    
    return 0;
}

//expand the recieved data to base64 encode
static int l4recv2(lsi_cb_param_t * rec)
{
#define ENCODE_BLOCK_SIZE 800
#define DECODE_BLOCK_SIZE (ENCODE_BLOCK_SIZE * 3 / 4 + 4)
  
    MyData2 *myData = NULL;
    char *pBegin;
    char tmpBuf[ENCODE_BLOCK_SIZE];
    int len, sz;
    
    myData = (MyData2 *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_L4);
    if (!myData)
        return -1;
    
    while((len = g_api->stream_read_next( rec, tmpBuf, ENCODE_BLOCK_SIZE )) > 0)
    {
        g_api->log( NULL, LSI_LOG_DEBUG, "#### updatetcpin2 test l4recv, inLn = %d\n", len );
        _loopbuff_append(&myData->inBuf, tmpBuf, len);
    }
    
    while (_loopbuff_hasdata(&myData->inBuf))
    {
        _loopbuff_reorder(&myData->inBuf);
        pBegin = _loopbuff_getdataref(&myData->inBuf);
        sz = _loopbuff_getdatasize(&myData->inBuf);
        if (sz > ENCODE_BLOCK_SIZE)
            sz = ENCODE_BLOCK_SIZE;
        
        len = Base64_decode( (const char *)pBegin, sz, tmpBuf );
        if (len > 0)
        {
            _loopbuff_append(&myData->outBuf, tmpBuf, len);
            _loopbuff_erasedata(&myData->inBuf, sz);
        }
        else
            break;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    if (_loopbuff_getdatasize(&myData->outBuf) < rec->_param_len)
    {
        rec->_param_len = _loopbuff_getdatasize(&myData->outBuf);
    }
    
    if (rec->_param_len > 0)
    {
        _loopbuff_getdata(&myData->outBuf, (char *)rec->_param, rec->_param_len);
        _loopbuff_erasedata(&myData->outBuf, rec->_param_len);
    }
    
    return rec->_param_len;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_L4_BEGINSESSION, l4init2, LSI_HOOK_NORMAL, 0},
    {LSI_HKPT_L4_RECVING, l4recv2, LSI_HOOK_EARLY + 1, LSI_HOOK_FLAG_TRANSFORM},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int init(lsi_module_t * pModule)
{
    g_api->init_module_data(pModule, l4release2, LSI_MODULE_DATA_L4 );
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init, NULL, NULL, "", serverHooks };
