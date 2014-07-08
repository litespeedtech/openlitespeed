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

#include "gd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

enum httpimg{ 
    HTTP_IMG_GIF, 
    HTTP_IMG_PNG,
    HTTP_IMG_JPEG
};

enum httpimg IMAGE_TYPE;

/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       imgresize
/////////////////////////////////////////////////////////////////////////////

lsi_module_t MNAME;
#define MAX_BLOCK_BUFSIZE   8192

typedef struct _MyData
{
    LoopBuff inWBuf;
    LoopBuff outWBuf;
    void *pSrcBuf;
} MyData;

/*Function Declarations*/
static int setWaitFull( lsi_cb_param_t * rec );
static int scanForImage( lsi_cb_param_t * rec );
static int parseParameters( lsi_cb_param_t * rec );
static int writeToNextFilter( lsi_cb_param_t * rec, MyData *myData );
static int getReqDimensions( const char *buf, int *width, int *height );
static void* resizeImage( const char *buf, int bufLen, int width, int height, MyData *myData, int *size );
static int _init();

int httpRelease(void *data)
{
    MyData *myData = (MyData *)data;
    g_api->log( NULL, LSI_LOG_DEBUG, "#### mymoduleresize %s\n", "httpRelease" );
    if (myData)
    {
        _loopbuff_dealloc(&myData->inWBuf);
        _loopbuff_dealloc(&myData->outWBuf);
        if ( myData->pSrcBuf )
            free( myData->pSrcBuf );
        free (myData);
    }
    return 0;
}

int httpinit(lsi_cb_param_t * rec)
{
    MyData *myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP);
    if (myData == NULL )
    {
        myData = (MyData *) malloc(sizeof(MyData));
        _loopbuff_init(&myData->inWBuf);
        _loopbuff_alloc(&myData->inWBuf, MAX_BLOCK_BUFSIZE);
        _loopbuff_init(&myData->outWBuf);
        _loopbuff_alloc(&myData->outWBuf, MAX_BLOCK_BUFSIZE);
        myData->pSrcBuf = NULL;
        
        g_api->log( NULL, LSI_LOG_DEBUG, "#### mymoduleresize init\n" );
        g_api->set_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP, (void *)myData);
    } 
    else
    {
        _loopbuff_dealloc(&myData->inWBuf);
        _loopbuff_dealloc(&myData->outWBuf);
    }
    
    return 0;
}

static int setWaitFull( lsi_cb_param_t * rec )
{
    g_api->set_resp_wait_full_body( rec->_session );
    return LSI_RET_OK;
}

static int scanForImage( lsi_cb_param_t * rec )
{
    MyData *myData = NULL;
    int iLen, inLen, iWidth = 0, iHeight = 0;
    const char *in, *pDimensions;
    
    in = rec->_param;
    inLen = rec->_param_len;
    myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP);
    
    if ( parseParameters( rec ) == 0 )
    {
        pDimensions = g_api->get_req_query_string( rec->_session, &iLen );
        if ( (iLen != 0)
            && (getReqDimensions( pDimensions, &iWidth, &iHeight ) == 0)
        )
        {
            //if ( resizeImage( in, inLen, iWidth, iHeight, myData ) != LSI_RET_OK )
                //return LSI_RET_ERROR;
        }
        else
        {
            _loopbuff_append( &myData->outWBuf, in, inLen );
        }
    }
    
    if ( writeToNextFilter( rec, myData ) != LSI_RET_OK )
        return LSI_RET_ERROR;
    return inLen;
}

/* Returns 0 for image, 1 for wrong input */
static int parseParameters( lsi_cb_param_t * rec )
{
    int iLen;
    const char *ptr;
    struct iovec iov;
    
    if ( (iLen = g_api->get_resp_header( rec->_session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0, &iov, 1 )) < 1 
        || iov.iov_len < 9
    )
        return 1;
    
    ptr = (const char *)iov.iov_base;
    if ( memcmp( ptr, "image/", 6 ) != 0 )
        return 1;
    
    if ( memcmp( ptr + 6, "png", 3 ) == 0 )
        IMAGE_TYPE = HTTP_IMG_PNG;
    else if ( memcmp( ptr + 6, "gif", 3 ) == 0 )
        IMAGE_TYPE = HTTP_IMG_GIF;
    else if ( memcmp( ptr + 6, "jpeg", 4 ) == 0 )
        IMAGE_TYPE = HTTP_IMG_JPEG;
    else
        return 1;
    return 0;
}

static int writeToNextFilter( lsi_cb_param_t * rec, MyData *myData )
{
    int iBytesWritten;
    _loopbuff_reorder(&myData->outWBuf);
    iBytesWritten = g_api->stream_write_next( rec, _loopbuff_getdataref(&myData->outWBuf),
                                                _loopbuff_getdatasize(&myData->outWBuf) );
    if ( iBytesWritten < 0 )
        return LSI_RET_ERROR;
    _loopbuff_erasedata(&myData->outWBuf, iBytesWritten);
    *((int *)( rec->_flag_out)) = _loopbuff_hasdata( &myData->outWBuf );
    return LSI_RET_OK;
}

/*return 0 on success, -1 for garbage queries*/
static int getReqDimensions( const char *buf, int *width, int *height )
{
    //TODO: figure out how to get actual requested dimensions
    *width = 400;
    *height = 500;
    return 0;
}

static void *resizeImage( const char *buf, int bufLen, int width, int height, MyData *myData, int *size )
{
    
    //FILE *out;
    char *ptr;
    //gdIOCtxPtr ioc;
    gdImagePtr dest, src = gdImageCreateFromPngPtr( bufLen, (void *)buf );
    
    //TODO: get aspect ratio
    dest = gdImageCreateTrueColor( width, height );
    
    gdImageCopyResampled( dest, src, 0, 0, 0, 0, 
                          width, height, 
                          src->sx, src->sy );
    /*
    ioc = gdNewDynamicCtx( sizeof(*dest), NULL );
    
    gdImagePngCtx( dest, ioc );
    
    gdDPExtractData( ioc, size );
    */
    ptr = gdImagePngPtr( dest, size );
    /*
    out = fopen("/home/user/Downloads/test.png", "wb");
    if (!out) {
        return ptr;
    }
    if (fwrite(ptr, 1, *size, out) != *size) {
        return ptr;
    }
    if (fclose(out) != 0) {
        return ptr;
    }
    //*/
    
    
    return ptr;
}

static int scanDynamic( lsi_cb_param_t * rec )
{
    off_t offset = 0;
    int iSrcSize, iDestSize, iWidth, iHeight, iLen = 0;
    void *pRespBodyBuf, *pSrcBuf, *pDestBuf;
    const char *ptr, *pDimensions;
    MyData *myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP);
    
    //TODO: Check to make sure that I'm accessing the right thing - use parseParameters?
    
    if ( parseParameters( rec ) == 0 )
    {
        
        pDimensions = g_api->get_req_query_string( rec->_session, &iLen );
        if ( (iLen == 0)
            || (getReqDimensions( pDimensions, &iWidth, &iHeight ) != 0)
        )
        {
            return LSI_RET_ERROR;
        }
        pRespBodyBuf = g_api->get_resp_body_buf( rec->_session );
        iSrcSize = g_api->get_body_buf_size( pRespBodyBuf );
        myData->pSrcBuf = malloc( iSrcSize );
        pSrcBuf = myData->pSrcBuf;
        while( !g_api->is_body_buf_eof( pRespBodyBuf, offset ) )
        {
            ptr = g_api->acquire_body_buf_block( pRespBodyBuf, offset, &iLen );
            if ( !ptr )
                break;
            memcpy( pSrcBuf + offset, ptr, iLen );
            g_api->release_body_buf_block( pRespBodyBuf, offset );
            offset += iLen;
        }
    }
    //*
    g_api->reset_body_buf( pRespBodyBuf );
    if ( g_api->append_body_buf( pRespBodyBuf, pSrcBuf, iSrcSize ) != iSrcSize )
        return LSI_RET_ERROR;
    iSrcSize = g_api->get_body_buf_size( pRespBodyBuf );
    //g_api->set_resp_content_length( rec->_session, iSrcSize );
    //*/
    pDestBuf = resizeImage( pSrcBuf, iSrcSize, iWidth, iHeight, myData, &iDestSize );
    if ( !pDestBuf )
        return LSI_RET_ERROR;    
    
    while( g_api->is_resp_buffer_available( rec->_session ) <= 0 );
    
    
    /*
    if ( g_api->append_body_buf( pRespBodyBuf, pDestBuf, iDestSize ) != iDestSize )
        return LSI_RET_ERROR;
    g_api->set_resp_content_length( rec->_session, iDestSize );
    //*/
    
    
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_HTTP_BEGIN, httpinit, LSI_HOOK_NORMAL, 0},
    {LSI_HKPT_RECV_REQ_HEADER, setWaitFull, LSI_HOOK_NORMAL, 0},
    {LSI_HKPT_SEND_RESP_HEADER, scanDynamic, LSI_HOOK_NORMAL, 0},
    //{LSI_HKPT_SEND_RESP_BODY, scanForImage, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_TRANSFORM + LSI_HOOK_FLAG_PROCESS_STATIC},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init( lsi_module_t *pModule )
{
    g_api->init_module_data(pModule, httpRelease, LSI_MODULE_DATA_HTTP );
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL, "imgresize", serverHooks};






