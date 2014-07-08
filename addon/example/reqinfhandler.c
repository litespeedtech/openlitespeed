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
/*******************************************************************
 * How to test this sample
 * 1, build it, put the file reqinfomodule.so to $LSWS_HOME/modules
 * 2, in web admin console configuration, enable this module
 * 3, visit http(s)://.../reqinfo(/echo|/md5|/upload) with or without request body 
 * 
 ********************************************************************/

#include "../include/ls.h"
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>

/**
 * HOW TO TEST
 * Put reqform.html to the DEFAULT/html and access http://.../reqform.html
 * 
 */

/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       reqinfhandler
/////////////////////////////////////////////////////////////////////////////

#define     TESTURI     "/reqinfo"

lsi_module_t MNAME;

typedef struct _MyData
{
    int type;
    FILE *fp;
    MD5_CTX ctx;
    int req_body_len;   //length in "Content-Length"
    int recved_req_body_len;    //recved
    int recved_done;    //finished or due to connection error
    int resp_done;
} MyData;


static int releaseData(void *data)
{
    MyData *myData = (MyData *)data;
    if (myData)
    {
        free (myData);
        g_api->log( NULL, LSI_LOG_DEBUG, "#### reqinfomodule releaseData\n" );
    }
    return 0;
}

static int resetData(lsi_cb_param_t * rec)
{
    MyData *myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP);
    if (myData)
    {
        memset(myData, 0, sizeof(MyData));
    }
    return 0;
}

static int initData(lsi_cb_param_t * rec)
{
    MyData *myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP);
    if (!myData)
    {
        myData = (MyData *) calloc(1, sizeof(MyData));
        g_api->log( NULL, LSI_LOG_DEBUG, "#### reqinfomodule init data\n" );
        g_api->set_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP, (void *)myData);
    }
    return 0;
}

static int check_uri_and_reg_handler(lsi_cb_param_t * rec)
{
    const char *uri;
    int len;
    uri = g_api->get_req_uri(rec->_session, &len);
    if ( len >= strlen(TESTURI) && strncasecmp(uri, TESTURI, strlen(TESTURI)) == 0 )
    {
        g_api->register_req_handler( rec->_session, &MNAME, strlen(TESTURI) );
    }
    return LSI_RET_OK;
}

static lsi_serverhook_t serverHooks[] = {
    {LSI_HKPT_HTTP_BEGIN, initData, LSI_HOOK_NORMAL, 0},
    {LSI_HKPT_HTTP_END, resetData, LSI_HOOK_NORMAL, 0},
    {LSI_HKPT_RECV_REQ_HEADER, check_uri_and_reg_handler, LSI_HOOK_NORMAL, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init( lsi_module_t * pModule )
{
    g_api->init_module_data(pModule, releaseData, LSI_MODULE_DATA_HTTP );
    return 0;
}

//0:no body or no deal, 1,echo, 2: md5, 3, save to file
static int getReqBodyDealerType(lsi_session_t *session)
{
    char path[512];
    int n;
    if( g_api->get_req_content_length( session ) > 0 )
    {
        n = g_api->get_req_var_by_id( session, LSI_REQ_VAR_PATH_INFO, path, 512);
        if( n >= 5 && strncasecmp(path, "/echo", 5) == 0)
            return 1;
        else if( n >= 4 && strncasecmp(path, "/md5", 4) == 0)
            return 2;
        else if( n >= 7 && strncasecmp(path, "/upload", 7) == 0)
            return 3;
    }
    
    //All other cases, no deal
    return 0;    
}



//The following matchs the index, DO NOT change the order
const char *reqArray[] = {
    "REMOTE_ADDR",
    "REMOTE_PORT",
    "REMOTE_HOST",
    "REMOTE_USER",
    "REMOTE_IDENT",
    "REQUEST_METHOD",
    "QUERY_STRING",
    "AUTH_TYPE",
    "PATH_INFO",
    "SCRIPT_FILENAME",
    "REQUEST_FILENAME",
    "REQUEST_URI",
    "DOCUMENT_ROOT",
    "SERVER_ADMIN",
    "SERVER_NAME",
    "SERVER_ADDR",
    "SERVER_PORT",
    "SERVER_PROTOCOL",
    "SERVER_SOFTWARE",
    "API_VERSION",
    "THE_REQUEST",
    "IS_SUBREQ",
    "TIME",
    "TIME_YEAR",
    "TIME_MON",
    "TIME_DAY",
    "TIME_HOUR",
    "TIME_MIN",
    "TIME_SEC",
    "TIME_WDAY",
    "SCRIPT_NAME",
    "CURRENT_URI",
    "REQUEST_BASENAME",
    "SCRIPT_UID",
    "SCRIPT_GID",
    "SCRIPT_USERNAME",
    "SCRIPT_GROUPNAME",
    "SCRIPT_MODE",
    "SCRIPT_BASENAME",
    "SCRIPT_URI",
    "ORG_REQ_URI",
    "HTTPS",
    "SSL_VERSION",
    "SSL_SESSION_ID",
    "SSL_CIPHER",
    "SSL_CIPHER_USEKEYSIZE",
    "SSL_CIPHER_ALGKEYSIZE",
    "SSL_CLIENT_CERT",
    "GEOIP_ADDR",
    "PATH_TRANSLATED",
};

//The following is used by name value, you can change the order
const char *envArray[] = {
    "PATH",
    
    "REMOTE_ADDR",
    "REMOTE_PORT",
    "REMOTE_HOST",
    "REMOTE_USER",
    "REMOTE_IDENT",
    "REQUEST_METHOD",
    "QUERY_STRING",
    "AUTH_TYPE",
    "PATH_INFO",
    "SCRIPT_FILENAME",
    "REQUEST_FILENAME",
    "REQUEST_URI",
    "DOCUMENT_ROOT",
    "SERVER_ADMIN",
    "SERVER_NAME",
    "SERVER_ADDR",
    "SERVER_PORT",
    "SERVER_PROTOCOL",
    "SERVER_SOFTWARE",
    "API_VERSION",
    "THE_REQUEST",
    "IS_SUBREQ",
    "TIME",
    "TIME_YEAR",
    "TIME_MON",
    "TIME_DAY",
    "TIME_HOUR",
    "TIME_MIN",
    "TIME_SEC",
    "TIME_WDAY",
    "SCRIPT_NAME",
    "CURRENT_URI",
    "REQUEST_BASENAME",
    "SCRIPT_UID",
    "SCRIPT_GID",
    "SCRIPT_USERNAME",
    "SCRIPT_GROUPNAME",
    "SCRIPT_MODE",
    "SCRIPT_BASENAME",
    "SCRIPT_URI",
    "ORG_REQ_URI",
    "HTTPS",
    "SSL_VERSION",
    "SSL_SESSION_ID",
    "SSL_CIPHER",
    "SSL_CIPHER_USEKEYSIZE",
    "SSL_CIPHER_ALGKEYSIZE",
    "SSL_CLIENT_CERT",
    "GEOIP_ADDR",
    "PATH_TRANSLATED",    
};

const char *reqHeaderArray[] = {
    "Accept",
    "Accept-Charset",
    "Accept-Encoding",
    "Accept-Language",
    "Accept-Datetime",
    "Authorization",
    "Cache-Control",
    "Connection",
    "Content-Type",
    "Content-Length",
    "Content-MD5",
    "Cookie",
    "Date",
    "Expect",
    "From",
    "Host",
    "Pragma",
    "Proxy-Authorization"
    "If-Match",
    "If-Modified-Since",
    "If-None-Match",
    "If-Range",
    "If-Unmodified-Since",
    "Max-Forwards",
    "Origin",
    "Referer",
    "User-Agent",
    "Range",
    "Upgrade",
    "Via",
    "X-Forwarded-For",
};

static inline void append( lsi_session_t *session, const char *s, int n)
{
    if ( n == 0)
        n = strlen(s);
    g_api->append_resp_body( session, (char *)s, n);
}
    

#define CONTENT_HEAD    "<html><head><title>Server request varibles</title></head><body>\r\n"
#define CONTENT_FORMAT  "<tr><td>%s</td><td>%s</td></tr>\r\n"
#define CONTENT_TAIL    "</body></html>\r\n"
static int handleReqBody( lsi_session_t *session)
{
    unsigned char md5[16];
    char buf[8192];
    int ret, i;
    int readbytes = 0, written = 0;
    MyData *myData = (MyData *)g_api->get_module_data(session, &MNAME, LSI_MODULE_DATA_HTTP);
    if ( myData ==  NULL || myData->type == 0)
    {
       return 0;
    }
    
    while(1)
    {
        ret = g_api->read_req_body( session, buf, 8192);
        if (ret > 0)
        {
            myData->recved_req_body_len += ret;
            readbytes += ret;
            
            if (myData->type == 1)
            {
                append(session, buf, ret);
                written += ret;
            }
            else if (myData->type == 2)
                MD5_Update(&myData->ctx, buf, ret);
            else
                fwrite(buf, 1, ret, myData->fp);
            
            if (myData->recved_req_body_len >= myData->req_body_len)
            {
                myData->recved_done = 1;
                break;
            }
        }
        else if (ret == 0)
        {
            myData->recved_done = 1;
            break;
        }
        else //-1
        {
            break;
        }
    };
    
    if (myData->recved_done == 1)
    {
        if (myData->type == 2)
        {
            MD5_Final(md5, &myData->ctx);
            for ( i =0;i<16; ++i)
            {
                sprintf(buf + i * 2, "%02X", md5[i]);
            }
            append(session, "MD5 is<br>", 10);
            append(session, buf, 32);
            written += 42;
        }
        else if (myData->type == 3)
        {
            fclose(myData->fp);
            append(session, "File uploaded.<br>", 18);
            written += 18;
            
        }
        
        myData->resp_done = 1;
    }
        
    if ( written > 0)
        g_api->flush(session);
    g_api->set_handler_write_state(session, 1);
    
    return readbytes;
}

static int handlerBeginProcess( lsi_session_t *session)
{
    #define VALMAXSIZE 512
    #define LINEMAXSIZE (VALMAXSIZE + 50)
    char val[VALMAXSIZE], line[LINEMAXSIZE] = {0};
    int n;
    int i;
    const char *p;
    char *buf;
    MyData *myData = (MyData *)g_api->get_module_data(session, &MNAME, LSI_MODULE_DATA_HTTP);
    
    //Create response body
    append( session, CONTENT_HEAD, 0);
    
    //Original request header
    n = g_api->get_req_raw_headers_length(session);
    buf = (char *)malloc(n + 1);
    memset(buf, 0, n + 1);
    n = g_api->get_req_raw_headers(session, buf, n + 1);
    append( session, "Original request<table border=1><tr><td><pre>\r\n", 0);
    append( session, buf, n);
    append( session, "\r\n</pre></td></tr>\r\n", 0);
    free(buf);
    
    append( session, "\r\n</table><br>Request headers<br><table border=1>\r\n", 0);
    for (i=0; i<sizeof(reqHeaderArray) / sizeof(char *); ++i)
    {
        p = g_api->get_req_header(session, reqHeaderArray[i], strlen(reqHeaderArray[i]), &n);
        if (p && p[0] != 0 && n > 0)
        {
            memcpy(val, p, n);
            val[n] = 0;
            n = snprintf(line, LINEMAXSIZE -1 , CONTENT_FORMAT, reqHeaderArray[i], val );
            append( session, line, n ); 
        }
    }
        
  
    append( session, "\r\n</table><br>Server req env<br><table border=1>\r\n", 0);
    //Server req env
    for (i=LSI_REQ_VAR_REMOTE_ADDR; i<LSI_REQ_COUNT; ++i)
    {
        n = g_api->get_req_var_by_id( session, i, val, VALMAXSIZE);
        if (n > 0)
        {
            val[n] = 0;
            n = snprintf(line, LINEMAXSIZE - 1, CONTENT_FORMAT, reqArray[i], val);
            append( session, line, n ); 
        }
    }
    
    append( session, "\r\n</table><br>env varibles<br><table border=1>\r\n", 0);
    for (i=0; i<sizeof(envArray) / sizeof(char *); ++i)
    {
        //env varibles
        n = g_api->get_req_env( session, envArray[i], strlen(envArray[i]),  val, VALMAXSIZE);
        if (n > 0)
        {
            val[n] = 0;
            n = snprintf(line, LINEMAXSIZE - 1, CONTENT_FORMAT, envArray[i], val );
            append( session, line, n ); 
        }
    }


    p = g_api->get_req_cookies( session, &n);
    if (p && p[0] != 0 && n > 0)
    {
        append( session, "\r\n</table><br>Request cookies<br><table border=1>\r\n", 0);
        append( session, "<tr><td>Cookie</td><td>", 0);
        append( session, p, n );
        append( session, "</td></tr>", 0);
    }

    n = g_api->get_req_cookie_count(session);
    if (n > 0)
    {
        //try get a certen cookie
        p = g_api->get_cookie_value(session, "LSWSWEBUI", 9, &n);
        if (p && n > 0)
        {
            append( session, "<tr><td>cookie_LSWSWEBUI</td><td>", 0);
            append( session, p, n );
            append( session, "</td></tr>", 0);
        }
    }
    append( session, "</table>", 0 );    
    
    
    n = getReqBodyDealerType(session);
        
    myData->req_body_len = g_api->get_req_content_length( session );
    myData->recved_req_body_len = 0;
    myData->type = n;
    sprintf(line, "Will deal with the req body.Type = %d, req body lenghth = %d<br>",
        n, myData->req_body_len);

    append( session, line, 0 );
    if ( myData->type == 0)
    {
        append( session, CONTENT_TAIL, 0 );
        myData->recved_done = 1;
        myData->resp_done = 1;
    }
    
    g_api->set_status_code(session, 200);
    
    if ( myData->type == 3) //Save to file
    {
        myData->fp = fopen("/tmp/uploadfile", "wb");
    }
    else if ( myData->type == 2 ) //Md5
    {
        MD5_Init( &myData->ctx);
    }
        
    g_api->flush(session);
    
    if ( myData->type != 0)
        handleReqBody( session );
    return 0;
}

static int cleanUp( lsi_session_t *session)
{
    g_api->free_module_data(session, &MNAME, LSI_MODULE_DATA_HTTP, releaseData);
    return 0;
}

static int handlerWriteRemainResp( lsi_session_t *session)    
{
    MyData *myData = (MyData *)g_api->get_module_data(session, &MNAME, LSI_MODULE_DATA_HTTP);
    if ( myData ==  NULL || myData->type == 0 || myData->resp_done == 1)
       return LSI_WRITE_RESP_FINISHED;
    
    else
        return LSI_WRITE_RESP_CONTINUE;
}

lsi_handler_t reqHandler = { handlerBeginProcess, handleReqBody, handlerWriteRemainResp, cleanUp };

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, &reqHandler, NULL, "", serverHooks};

