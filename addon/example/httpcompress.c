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
#define _GNU_SOURCE

#include "../include/ls.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/******************************************************
 * Notes about this module:
 * The purpose of the module is to check for if a
 *  compressed version of the page requested exists,
 *  and if it does, send that version instead.
 * The defined implementation should be the one used.
 * 
 * The second implementation is shown for example:
 *  1. Uses add_session_hook
 *      - Splits the implementation into two 
 *      functions at different hook points.
 *  2. Uses different API for similar function
 * This module allows for custom handling for the case
 *  when the file path is longer than the allowed
 *  buffer (default = 4096).  Make sure to back up
 *  the original version if you intend to change it.
 * 
 * 
 *****************************************************/

/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       httpcompress
/////////////////////////////////////////////////////////////////////////////

/*
 * Differences between the implementations:
 *  The first implementation is hooked to
 *      RECV_RESP_HEADER.  The server is ready to
 *      send the regular page, check to see if
 *      compressed version exists, if so, send that file.
 *  The second implementation hooks to the
 *      URI_MAP hook point.  It checks if the compressed
 *      version exists, and if so, change the request
 *      and let the server handle it.  Add encoding
 *      to header.
 */
#define GZIP_HOOK_IMPL
//#define GZIP_TWO_HOOK_IMPL


lsi_module_t MNAME;


//static int isCompressible( lsi_session_t *session, 

#ifdef GZIP_HOOK_IMPL
static int httpCompress( lsi_cb_param_t *param )
{
    lsi_session_t *session = param->_session;
    int iPtrLen, iCompressible; // NOTE: iPtrLen will be used for various purposes in this method.
    char pCompressed[LSI_MAX_FILE_PATH_LEN];
    const char *pFilePath, 
        *pReqHeader = g_api->get_req_header_by_id( session, LSI_REQ_HEADER_ACC_ENCODING, &iCompressible );
    struct iovec iov;
    struct stat st;
    
    iPtrLen = g_api->get_resp_header( session, LSI_RESP_HEADER_CONTENT_ENCODING, NULL, 0, &iov, 1 );
    // The file must have been statically handled, must allow compression, and isn't already compressed
    if( memcmp((pFilePath = g_api->get_req_handler_type( session )), "static", 6) == 0
        && iCompressible != 0 
        && (memcmp( pReqHeader, "gzip", 4 ) == 0)
        && iPtrLen == 0
    )
    {
        pFilePath = g_api->get_req_file_path( session, &iPtrLen );
        if ( iPtrLen >= LSI_MAX_FILE_PATH_LEN - 4 )
        {
            //Do Action if path is too long
            return LSI_RET_OK;
        }
        
        snprintf( pCompressed, iPtrLen + 4, "%s%s", pFilePath, ".gz" );
        // Check if a compressed version exists
        if ( g_api->get_file_stat( session, pCompressed, iPtrLen + 4, &st ) == 0 )
        {
            g_api->set_resp_header(session, LSI_RESP_HEADER_CONTENT_ENCODING, 
                                   "Content-Encoding", 16, "gzip", 4, LSI_HEADER_SET);
            g_api->set_resp_content_length( session, st.st_size );
            g_api->send_file( session, pCompressed, 0, st.st_size );
        }
    }
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_RECV_RESP_HEADER, httpCompress, LSI_HOOK_NORMAL, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init( lsi_module_t *pModule )
{
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL, "httpCompress", serverHooks};
#endif

#ifdef GZIP_TWO_HOOK_IMPL
static int httpEncode( lsi_cb_param_t *param )
{
    g_api->set_resp_header(param->_session, LSI_RESP_HEADER_CONTENT_ENCODING, "Content-Encoding", 16, "gzip", 4, LSI_HEADER_SET);
    return LSI_RET_OK;
}

static int httpCompress( lsi_cb_param_t *param )
{
    lsi_session_t *session = param->_session;
    int iPtrLen, iUriLen, iCompressible; // NOTE: iPtrLen will be used for various purposes in this method.
    char pDocPath[LSI_MAX_FILE_PATH_LEN], *ptr;
    const char *pReqUri = g_api->get_req_uri( session, &iUriLen ), 
        *pReqHeader = g_api->get_req_header_by_id( session, LSI_REQ_HEADER_ACC_ENCODING, &iCompressible );
    struct stat st;
    
    if ( (iUriLen == 0) 
        || (iCompressible == 0) 
        || (memcmp( pReqHeader, "gzip", 4 ) != 0) 
    )
        return LSI_RET_OK;
    
    iPtrLen = g_api->get_file_path_by_uri( session, pReqUri, iUriLen, pDocPath, LSI_MAX_FILE_PATH_LEN );    
    if ( iPtrLen >= LSI_MAX_FILE_PATH_LEN - 4 )
    {
        //Do Action if path is too long
        return LSI_RET_OK;
    }
    if ( iPtrLen != 0 )
    {
        ptr = (char *)memrchr( pDocPath, '.', iPtrLen );
        if ( ptr == NULL ) // no file extension in request.
            return LSI_RET_ERROR;
        
        pReqHeader = g_api->get_mime_type_by_suffix( session, (const char *)(++ptr) );
        strncat( pDocPath, ".gz", 4 );
        iPtrLen += 4;
        if ( g_api->get_file_stat( session, pDocPath, iPtrLen, &st ) == 0 )
        {
            g_api->set_force_mime_type( session, pReqHeader );
            snprintf( pDocPath, (iUriLen +=4), "%s%s", pReqUri, ".gz" );
            //DEBUG g_api->log( NULL,  LSI_LOG_INFO, pDocPath );
            g_api->set_uri_qs( session, LSI_URL_REDIRECT_INTERNAL + LSI_URL_QS_NOCHANGE, 
                               (const char*)pDocPath, iUriLen, NULL, 0 );
            
            g_api->add_session_hook( session, LSI_HKPT_SEND_RESP_HEADER, 
                                     &MNAME, httpEncode, LSI_HOOK_NORMAL, 0 );
        }
    }
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_URI_MAP, httpCompress, LSI_HOOK_NORMAL, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init( lsi_module_t *pModule )
{
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL, "http compress mod", serverHooks };
#endif

