/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2016  LiteSpeed Technologies, Inc.                 *
*                                                                            *
*    This program is free software: you can redistribute it and/or modify    *
*    it under the terms of the GNU General Public License as published by    *
*    the Free Software Foundation, either version 3 of the License, or       *
*    (at your option) any later version.                                     *
*                                                                            *
*    This program is distributed in the hope that it will be useful,         *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of          *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
*    GNU General Public License for more details.                            *
*                                                                            *
*    You should have received a copy of the GNU General Public License       *
*    along with this program. If not, see http://www.gnu.org/licenses/.      *
*****************************************************************************/

#include "mod_lsphp.h"
#include <lsr/ls_strtool.h>
#include <lsr/ls_confparser.h>
#include <sys/timeb.h>

//#define NO_PHP

#ifdef NO_PHP
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#endif

#if __GNUC__ >= 4
    #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
    #define DLL_PUBLIC
    #define DLL_LOCAL
#endif

//#ifdef ZTS
//int mod_lsphp_globals_id;
//#else
//mod_lsphp_globals_t mod_lsphp_globals;
//#endif

#if (PHP_MAJOR_VERSION == 7)
    #if (PHP_MINOR_VERSION == 0)
#define MNAME mod_lsphp70
#define PHP_VERSION_STR "7.0"
    #elif (PHP_MINOR_VERSION == 1)
#define MNAME mod_lsphp71
#define PHP_VERSION_STR "7.1"
    #elif (PHP_MINOR_VERSION == 2)
#define MNAME mod_lsphp72
#define PHP_VERSION_STR "7.2"
    #endif
#elif (PHP_MAJOR_VERSION == 5)
    #if (PHP_MINOR_VERSION == 6)
#define MNAME mod_lsphp56
#define PHP_VERSION_STR "5.6"
    #elif (PHP_MINOR_VERSION == 5)
#define MNAME mod_lsphp55
#define PHP_VERSION_STR "5.5"
    #else
#error "PHP version 5.x not yet tested"
    #endif
#else
#error "PHP version x.x not yet tested"
#endif

#define MOD_LSPHP_ABOUT "mod_lsphp " PHP_VERSION_STR " LiteSpeed API PHP Processor."
DLL_PUBLIC lsi_module_t MNAME;

#define LSAPI_ENV_MAX_VALUE  8192

/* {{{ sapi_module_struct cgi_sapi_module
 */
sapi_module_struct lsiapi_sapi_module =
{
    "litespeed",
    "LiteSpeed Embedded V1.0",

    php_lsiapi_startup,             /* startup */
    php_module_shutdown_wrapper,    /* shutdown */

    NULL, //sapi_lsiapi_activate,           /* activate */
    NULL, //sapi_lsiapi_deactivate,         /* deactivate */

    sapi_lsiapi_ub_write,            /* unbuffered write */
    sapi_lsiapi_flush,               /* flush */
#if (PHP_MAJOR_VERSION >= 7)
    sapi_lsiapi_get_zend_stat,
#else
    sapi_lsiapi_get_stat,
#endif
    NULL,                           /* sapi_lsiapi_getenv */

    mod_lsphp_sapi_error,           /* error handler */

    sapi_header_handler,            /* Built on the module of the apache module header handler */
    sapi_lsiapi_send_headers,       /* send headers handler */
    NULL,                           /* send header handler */

    sapi_lsiapi_read_post,          /* read POST data */
    sapi_lsiapi_read_cookies,       /* read Cookies */

    sapi_lsiapi_register_variables, /* register server variables */
    sapi_lsiapi_log_message,        /* Log message */

#if PHP_MAJOR_VERSION > 7 || (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION >= 1)
    NULL,                           /* Get request time */
    NULL,                           /* Child terminate */
#else
    NULL,                           /* php.ini path override */
    NULL,                           /* block interruptions */
    NULL,                           /* unblock interruptions */
    NULL,                           /* default post reader */
    NULL,                           /* treat data */
    NULL,                           /* executable location */

    0,                              /* php.ini ignore */
#endif

    STANDARD_SAPI_MODULE_PROPERTIES

};
/* }}} */


/**
 * Define the server hooks.  For now, just a termination hook, but it gives me
 * the ability to add lots more later
 */

lsi_serverhook_t s_mod_lsphp_lsi_server_hooks[] =
{
    { LSI_HKPT_MAIN_ATEXIT, mod_lsphp_termination, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED },
    LSI_HOOK_END
};

/**
 * Define a handler, need to provide a struct _handler_st object, in which
 * the first function pointer should not be NULL
 */

lsi_reqhdlr_t s_mod_lsphp_request_handler = { mod_lsphp_begin_process,
                                              NULL, // read
                                              NULL, // write
                                              NULL, //mod_lsphp_cleanup_process,
                                              LSI_HDLR_DEFAULT_POOL,
                                              NULL,
                                              NULL };


typedef struct _mod_lsphp_config_st
{
    int m_slow_ms;
} mod_lsphp_config_t;

static mod_lsphp_config_t mod_lsphp_config = { 10000 };

//Setup the below array to let web server know these params
lsi_config_key_t mod_lsphp_config_key[] =
{
    {"LSAPI_SLOW_REQ_MSECS",  0, 0},
    {NULL,  0, 0}   //The last position must have a NULL to indicate end of the array
};

static void *mod_lsphp_parseConfig(module_param_info_t *param, int param_count,
                                   void *_initial_config, int level, const char *name);

lsi_confparser_t mod_lsphp_confparser = { mod_lsphp_parseConfig, NULL, mod_lsphp_config_key };


/**
 * Define a module handler, which is used for the entire module.
 * It uses the request handler below.
 */
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE,           // Required value
                       mod_lsphp_mod_init,// Module initialization called at load time.
                       &s_mod_lsphp_request_handler,   // Request Handle data pointer.
                       &mod_lsphp_confparser,          // Config parser
                       MOD_LSPHP_ABOUT,                // About
                       s_mod_lsphp_lsi_server_hooks};  // Server hooks

zend_module_entry litespeed_module_entry = {
    STANDARD_MODULE_HEADER,
    "litespeed",
    NULL,//litespeed_functions,
    NULL, //PHP_MINIT(litespeed),
    NULL, //PHP_MSHUTDOWN(litespeed),
    NULL,
    PHP_RSHUTDOWN(litespeed),
    NULL,
    NO_VERSION_YET,
    STANDARD_MODULE_PROPERTIES
};


#define PHP_MAGIC_TYPE "application/x-httpd-php"
#define PHP_SOURCE_MAGIC_TYPE "application/x-httpd-php-source"
#define PHP_SCRIPT "php7-script"

#define SAPI_LSAPI_MAX_HEADER_LENGTH 8192
#define LSAPI_MAX_URI_LENGTH         16384

#ifdef ZTS
#if (PHP_MAJOR_VERSION < 7)
zend_compiler_globals    *compiler_globals;
zend_executor_globals    *executor_globals;
php_core_globals         *core_globals;
sapi_globals_struct      *sapi_globals;
void ***tsrm_ls;
#endif // PHP5
#endif

//#define LOCK_TEST_AND_SET(s)   while (__sync_lock_test_and_set(&s,1)) while (s);
//#define UNLOCK_TEST_AND_SET(s) __sync_lock_release(&s);

static pid_t gettid(void)
{
    if (syscall(SYS_gettid) == getpid()) {
        return 0;
    }
    return syscall(SYS_gettid);
}


/*
static char *tid_string(char tid[])
{
    pid_t pid = gettid();
    if (pid == 0) {
        sprintf(tid, "Main thread");
    }
    else {
        sprintf(tid, "Secondary thread: %d", (int)pid);
    }
    return(tid);
}
*/


/* {{{ php_lsiapi_startup
 */
static int php_lsiapi_startup(sapi_module_struct *sapi_module)
{
    LSM_DBG((&MNAME), NULL, "php_lsiapi_startup\n");
    if (php_module_startup(sapi_module, &litespeed_module_entry, 1)==FAILURE) {
        LSM_DBG((&MNAME), NULL, "php_lsiapi_startup error\n");
        return FAILURE;
    }
    //argv0 = sapi_module->executable_location;
    return SUCCESS;
}
/* }}} */


/* {{{ sapi_lsiapi_flush
 */
static void sapi_lsiapi_flush(void * server_context)
{
    lsi_session_t   *session   = server_context;

    /* If we haven't registered a server_context yet,
     * then don't bother flushing. */
    TSRMLS_FETCH();
    LSM_DBG((&MNAME), session, "sapi_lsiapi_flush\n");
    if (!session) {
        return;
    }
    sapi_send_headers(TSRMLS_C);

    SG(headers_sent) = 1;
    g_api->flush(session);
    if (g_api->is_resp_handler_aborted(session) == -1) {
        mod_lsphp_t *mod_data = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);
        LSM_ERR((&MNAME), session, "NULL session handle in flush, session: %p\n", session);
        mod_data->m_FatalError = 1;
        php_handle_aborted_connection();
    }
}
/* }}} */


static void lsiapi_foreach_callback(int key_id, const char * key, int key_len,
                                    const char * value, int val_len, void * arg)
{
    lsi_cb_sess_param_t *sess_param = (lsi_cb_sess_param_t *)arg;
    char val[LSAPI_ENV_MAX_VALUE];
#if (PHP_MAJOR_VERSION < 7)
    unsigned int new_length = (unsigned int)val_len;
#else
    size_t new_length = (size_t)val_len;
#endif
    char *val_ptr = val;
    lsi_session_t *session = sess_param->session;
    zval *track_vars_array = (zval *)sess_param->cb_arg;

    TSRMLS_FETCH();
    LSM_DBG((&MNAME), session, "foreach variable callback, id: %d, key: %s\n", key_id, key);
    if (val_len + 1 >= sizeof(val)) {
        LSM_DBG((&MNAME), session, "TRUNCATING value\n");
        val_len = sizeof(val) - 1;
        memcpy(val, value, val_len);
        val[val_len] = 0;
    }
    else {
        memcpy(val, value, val_len);
        val[val_len] = 0;
    }
    LSM_DBG((&MNAME), session, "CGI variable callback, value: %s\n", val);
    if (sapi_module.input_filter(PARSE_SERVER,
                                 (char *)key,
                                 &val_ptr,
                                 val_len,
                                 &new_length TSRMLS_CC) == 0) {
        LSM_DBG((&MNAME), session, "php_register %s=%s (%ld)\n", key, val_ptr, (long)new_length);
        php_register_variable_safe((char *)key, val_ptr, new_length, track_vars_array TSRMLS_CC);
    }
}


/* {{{ lsiapi_register_variables
 */
static void lsiapi_register_variables(lsi_session_t *session, zval *track_vars_array)
{
    char value[LSAPI_MAX_URI_LENGTH];
    lsi_cb_sess_param_t sess_param;
#if (PHP_MAJOR_VERSION < 7)
    unsigned int new_length;
#else
    size_t new_length;
#endif

    LSM_DBG((&MNAME), session,
            "lsiapi_register_variables - add PHP_SELF\n");
    if (SG(request_info).request_uri ) {
        char *php_self = (SG(request_info).request_uri );
        char *title = "PHP_SELF";
        int  php_self_len = strlen(php_self);
        if (php_self_len > sizeof(value) - 1) {
            php_self_len = sizeof(value) - 1;
            memcpy(value, php_self, php_self_len);
            value[php_self_len - 1] = 0; // NULL terminate it even in overrun
        }
        else {
            memcpy(value, php_self, php_self_len + 1); // to get \0
        }
        php_self = value;
        new_length = php_self_len;
        if (sapi_module.input_filter(PARSE_SERVER,
                                     title,
                                     &php_self,
                                     php_self_len,
                                     &new_length TSRMLS_CC) == 0) {
            LSM_DBG((&MNAME), session, "php_register %s=%s\n",
                    title, php_self);
            php_register_variable_safe(title, php_self, new_length,
                                       track_vars_array TSRMLS_CC);
        }
    }
    else {
        LSM_DBG((&MNAME), session,
                "sapi_lsiapi_register_variables - request_uri missing, can't register PHP_SELF\n");
    }
    sess_param.session = session;
    sess_param.cb_arg = track_vars_array;
    g_api->foreach(session, LSI_DATA_REQ_VAR, NULL, lsiapi_foreach_callback,
                   &sess_param);
    g_api->foreach(session, LSI_DATA_REQ_CGI_HEADER, NULL, lsiapi_foreach_callback,
                   &sess_param);
}


/* {{{ sapi_lsiapi_register_variables
 */
static void sapi_lsiapi_register_variables(zval *track_vars_array TSRMLS_DC)
{
    lsi_session_t *session = SG(server_context);
    mod_lsphp_t   *mod_data = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);

    if (mod_data->m_DidRegisterVariables) {
        LSM_DBG((&MNAME), session,
                "sapi_lsiapi_register_variables - already done\n");
        return;
    }
    mod_data->m_DidRegisterVariables = 1;
    LSM_DBG((&MNAME), session,
            "sapi_lsiapi_register_variables - add PHP_SELF\n");
    lsiapi_register_variables(session, track_vars_array);
}


static int lsiapi_read_req_body( lsi_session_t *session,
                                 mod_lsphp_t *mod_data,
                                 char * pchBuffer,
                                 int iSize ) {
    int     iReturn = 0;
    int     iThisRead = 0;
    int     bSizeReduced = 0;

    LSM_DBG((&MNAME), session, "read_req_body: %d\n", iSize);
    if (mod_data->m_FatalError) {
        LSM_DBG((&MNAME), session, "Fatal error previously logged (read_req_body)\n");
        return(-1);
    }
    if (mod_data->m_ContentLength == 0) {
        LSM_DBG((&MNAME), session, "read_req_body: NO DATA TO READ!\n");
        SG(post_read) = 1;  // George found the global to set.
        return 0;           // size
    }
    if (iSize + mod_data->m_ContentRead <= mod_data->m_ContentLength) {
        // Read the size
    }
    else {
        bSizeReduced = 1;
        iSize = (int)(mod_data->m_ContentLength - mod_data->m_ContentRead);
        if (!iSize) {
            LSM_DBG((&MNAME), session,
                    "read_req_body: No data left to read - early return\n");
            return(0);
        }
    }
    LSM_DBG((&MNAME), session, "read_req_body: %d\n", iSize);
    do {
        iThisRead = g_api->read_req_body( session, &pchBuffer[iReturn], iSize - iReturn);
        LSM_DBG((&MNAME), session, "read_req_body: single read result %d\n", iThisRead);
        if (iThisRead < 0) {
            LSM_ERR((&MNAME), session,
                    "Fatal error reading request body of %s\n",
                    mod_data->m_ScriptFileName);
            mod_data->m_FatalError = 1;
            return(-1);
        }
        mod_data->m_ContentRead += iThisRead;
        iReturn += iThisRead;
    } while (iReturn < iSize);
    if ((iReturn < 256) && (bSizeReduced)) {
        pchBuffer[iReturn] = 0;
        LSM_DBG((&MNAME), session, "read_req_body: %s\n",pchBuffer);
    }
    LSM_DBG((&MNAME), session,
            "read_req_body: read %lld of %lld, return: %d\n",
            (long long int)mod_data->m_ContentRead,
            (long long int)mod_data->m_ContentLength,
            iReturn);
    return iReturn;
}


/* {{{ sapi_lsiapi_read_post
 */
#if PHP_MAJOR_VERSION >= 7
static size_t sapi_lsiapi_read_post(char *buffer, size_t count_bytes TSRMLS_DC)
#else
static int sapi_lsiapi_read_post(char *buffer, uint count_bytes TSRMLS_DC)
#endif
{
    lsi_session_t   *session = SG(server_context);
    mod_lsphp_t     *mod_data = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);
    if (!mod_data) {
        LSM_ERR((&MNAME), session, "NULL session handle in read_post, session: %p\n", session);
        return(-1);
    }
    LSM_DBG((&MNAME), session, "sapi_lsiapi_read_post %d bytes\n",
            (int)count_bytes);
    return lsiapi_read_req_body( session,
                                 mod_data,
                                 buffer,
                                 (unsigned long long)count_bytes );
}
/* }}} */


static int lsiapi_read_raw_header( lsi_session_t *session,
                                  mod_lsphp_t *mod_data )
{
    int iResult = 0;
    if (!mod_data->m_ReadRawHeader) {
        char chHeaders[8192];
        char *pchHeader = chHeaders;
        int iLength;
        int len;
        mod_data->m_ReadRawHeader = 1;
        iLength = g_api->get_req_raw_headers(session, chHeaders, sizeof(chHeaders));
        chHeaders[iLength] = 0;
        len = iLength;
        while (len) {
            if (!*pchHeader) {
                *pchHeader = '`';
            }
            pchHeader++;
            len--;
        }
        LSM_DBG((&MNAME), session,
                "Read headers to examine details, length: %d\n:%s:\n",
                iLength, chHeaders);
    }
    return(iResult);
}

static char *lsiapi_read_cookies( lsi_session_t *session,
                                 mod_lsphp_t *mod_data,
                                 int *piCookieLength ) {
    char *pchCookies;
    LSM_DBG((&MNAME), session, "Read Cookies\n");
    if (mod_data->m_Cookies) {
        (*piCookieLength) = strlen(mod_data->m_Cookies);
        return(mod_data->m_Cookies);
    }
    if (mod_data->m_FatalError) {
        LSM_DBG((&MNAME), session,
                "Fatal error previously logged (ReadCookies)\n");
        return(NULL);
    }
    //lsiapi_read_raw_header(session, mod_data);
    pchCookies = (char *)g_api->get_req_cookies(session, piCookieLength);
    if (pchCookies) {
        mod_data->m_Cookies = malloc((*piCookieLength) + 1);
        memcpy(mod_data->m_Cookies, pchCookies, *piCookieLength);
        mod_data->m_Cookies[*piCookieLength] = 0;
        LSM_DBG((&MNAME), session,
                "Cookies: (%d) %s\n", *piCookieLength, mod_data->m_Cookies);
        return(mod_data->m_Cookies);
    }
    return(pchCookies);
}


/* {{{ sapi_lsiapi_read_cookies
 */
static char *sapi_lsiapi_read_cookies(TSRMLS_D)
{
    lsi_session_t *session = SG(server_context);
    mod_lsphp_t   *mod_data   = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);
    int           iCookieLength = 80;
    if (!mod_data) {
        LSM_ERR((&MNAME), session, "NULL session handle in read_cookies, session: %p\n", session);
        return NULL;
    }
    LSM_DBG((&MNAME), session, "sapi_lsiapi_read_cookies\n");
    return lsiapi_read_cookies( session, mod_data, &iCookieLength );
}
/* }}} */


static int lsiapi_set_resp_status( lsi_session_t *session, int code ) {
    LSM_DBG((&MNAME), session, "set_resp_status: %d\n", code);
    g_api->set_status_code( session, code );
    return 0;
}


/* {{{ sapi_lsiapi_set_response_status
 */
static int sapi_lsiapi_set_response_status(lsi_session_t *session, int code TSRMLS_DC) {
    SG(sapi_headers).http_response_code = code;
    return(lsiapi_set_resp_status(session, code));
}


static int lsiapi_append_resp_header( lsi_session_t * session,
                                     mod_lsphp_t *mod_data,
                                     const char * pBuf,
                                     int len )
{
    LSM_DBG((&MNAME), session, "append_resp_header: %s len: %d\n",
            pBuf, len);
    if ( mod_data->m_FatalError ) {
        LSM_ERR((&MNAME), session,
                "append_resp_header but already had a fatal error\n");
        return -1;
    }
    // Remove trailing newlines
    while( len > 0 )
    {
        char ch = *(pBuf + len - 1 );
        if (( ch == '\n' )||( ch == '\r' ))
            --len;
        else
            break;
    }
    if ( len <= 0 ) {
        LSM_DBG((&MNAME), session, "append_resp_header: Turned out to be empty\n");
        return 0;
    }
    if (g_api->set_resp_header2(session, pBuf, len, LSI_HEADEROP_APPEND) == -1) {
        LSM_ERR((&MNAME), session, "Error adding header\n");
        mod_data->m_FatalError = 1;
        return -1;
    }
    return 0;
}


static int lsiapi_finalize_resp_headers( lsi_session_t * session,
                                         mod_lsphp_t *mod_data)
{
    LSM_DBG((&MNAME), session, "finalize_resp_headers\n");
    if ( mod_data->m_FatalError ) {
        LSM_ERR((&MNAME), session,
                "FinalizeRespHeaeder but already had a fatal error\n");
        return -1;
    }
    if ( mod_data->m_SentHeader ) {
        LSM_ERR((&MNAME), session,
                "finalize_resp_headers but header already sent\n");
        mod_data->m_FatalError = 1;
        return -1;
    }
    // No flush done at this time, why reduce performance unnecessarily,
    // but we'll pretend we did
    mod_data->m_SentHeader = 1;
    return 0;
}


static int sapi_header_handler(sapi_header_struct *sapi_header,
                               sapi_header_op_enum op,
                               sapi_headers_struct *sapi_headers TSRMLS_DC)
{
    lsi_session_t *session;
    mod_lsphp_t  *mod_data;
    int iRet;
    int add = 0;

    session = SG(server_context);
    mod_data   = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);
    if (!mod_data) {
        LSM_ERR((&MNAME), session, "NULL session handle in header_handler - usually a prior error, session: %p\n", session);
        return(-1);
    }

    LSM_DBG((&MNAME), session, "sapi_header_handler\n");

    switch (op) {
        case SAPI_HEADER_DELETE:
            iRet = g_api->remove_resp_header(session,
                                             LSI_RSPHDR_UNKNOWN,
                                             sapi_header->header,
                                             sapi_header->header_len);
            LSM_DBG((&MNAME), session,
                    "Delete: %s, iRet: %d\n", sapi_header->header, iRet);
            break;

        case SAPI_HEADER_DELETE_ALL:
            {
                #define MAX_HEADER_NUM  50
                struct iovec iov_key[MAX_HEADER_NUM], iov_val[MAX_HEADER_NUM];
                int i;
                LSM_DBG((&MNAME), session, "Delete all response headers\n");
                iRet = g_api->get_resp_headers(session,
                                               iov_key,
                                               iov_val,
                                               MAX_HEADER_NUM);
                if (iRet < 0) {
                    LSM_ERR((&MNAME), session,
                            "Fatal Error getting response headers\n");
                    mod_data->m_FatalError = 1;
                    return iRet;
                }
                for (i = 0; i < iRet; ++i) {
                    LSM_DBG((&MNAME), session,
                            "   delete [%d] %s=%s\n",
                            i,
                            (char *)iov_key[i].iov_base,
                            (char *)iov_val[i].iov_base);
                    iRet = g_api->remove_resp_header(session,
                                                     LSI_RSPHDR_UNKNOWN,
                                                     iov_key[i].iov_base,
                                                     iov_key[i].iov_len);
                    if (iRet == -1) {
                        LSM_INF((&MNAME), session,
                                "Error deleting header: %s\n",
                                (char *)iov_key[i].iov_base);
                        // ...and keep going
                    }
                }
            }
            iRet = 0;
            break;

        case SAPI_HEADER_ADD:
            add = 1;
        case SAPI_HEADER_REPLACE:
            {
                // Format in the header is name:value so look for the value
                char *pchValue;
                char *pchOriginalColon;
                char *operation = add ? "add" : "merge";
                LSM_DBG((&MNAME), session,
                        "%s response header: %s\n",operation,sapi_header->header);
                pchValue = strchr(sapi_header->header, ':');
                if (!pchValue) {
                    LSM_DBG((&MNAME), session,
                            "Value not found in header - just return 0 for now\n");
                    iRet = 0;
                    break;
                }
                pchOriginalColon = pchValue;
                *pchValue = 0; // Null terminate it so we can separate title from value
                do {
                    pchValue++;
                } while (*pchValue == ' ');
                iRet = g_api->set_resp_header(session,
                                              LSI_RSPHDR_UNKNOWN,
                                              sapi_header->header,
                                              strlen(sapi_header->header),
                                              pchValue,
                                              strlen(pchValue),
                                              add ? LSI_HEADEROP_ADD : LSI_HEADEROP_MERGE);
                if (iRet < 0) {
                    LSM_ERR((&MNAME), session, "Failed %s of response header: %s\n",
                            operation, sapi_header->header);
                }
                else {
                    LSM_DBG((&MNAME), session,
                            "%s response header: %s:%s, return %d\n",
                            operation, sapi_header->header, pchValue, iRet);
                }
                (*pchOriginalColon) = ':';
                break;
            }

        default:
            LSM_NOT((&MNAME), session,
                    "NOTE: UNKNOWN RESPONSE HEADER OP: %d\n", (int)op);
            return 0;
    }
    if ((0 == iRet) && (SG(sapi_headers).http_response_code != 200))
    {
        LSM_DBG((&MNAME), session, "sapi_header_handler, status: %d\n",
                SG(sapi_headers).http_response_code);
        sapi_lsiapi_set_response_status( session, SG(sapi_headers).http_response_code TSRMLS_CC );
    }
    return iRet;
}

/* {{{ sapi_lsiapi_send_headers
 */
static int sapi_lsiapi_send_headers(sapi_headers_struct *sapi_headers TSRMLS_DC)
{
    lsi_session_t *session;
    mod_lsphp_t  *mod_data;
    sapi_header_struct  *h;
    zend_llist_position pos;

    session = SG(server_context);
    mod_data   = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);
    if (!mod_data) {
        LSM_ERR((&MNAME), session,
                "NULL module data in send_headers - usually a prior error, session: %p\n", session);
        return(-1);
    }
    if (SG(sapi_headers).http_response_code != 200) {
        LSM_DBG((&MNAME), session, "sapi_lsiapi_send_headers, status: %d\n",
                SG(sapi_headers).http_response_code);
        sapi_lsiapi_set_response_status( session, SG(sapi_headers).http_response_code TSRMLS_CC );
    }
    h = zend_llist_get_first_ex(&sapi_headers->headers, &pos);
    while (h) {
        if ( h->header_len > 0 ) {
            LSM_DBG((&MNAME), session, "Appending header: %s\n", h->header);
            if (lsiapi_append_resp_header( session,
                                          mod_data,
                                          h->header,
                                          h->header_len ) == -1) {
                return(-1);
            }
        }
        h = zend_llist_get_next_ex(&sapi_headers->headers, &pos);
    }
    if (SG(sapi_headers).send_default_content_type) {
        char    *hd;
        int     len;
        char    headerBuf[SAPI_LSAPI_MAX_HEADER_LENGTH];

        hd = sapi_get_default_content_type(TSRMLS_C);
        len = snprintf( headerBuf,
                        SAPI_LSAPI_MAX_HEADER_LENGTH - 1,
                        "Content-type: %s",
                        hd );
        efree(hd);

        LSM_DBG((&MNAME), session, "default content type: %s\n", headerBuf);
        if (lsiapi_append_resp_header( session, mod_data, headerBuf, len ) == -1) {
            return(-1);
        }
    }
    if (lsiapi_finalize_resp_headers( session, mod_data) == -1) {
        LSM_DBG((&MNAME), session, "sapi_lsiapi_send_headers, ERROR!\n");
        return(-1);
    }
    LSM_DBG((&MNAME), session, "sapi_lsiapi_send_headers, OK\n");
    return SAPI_HEADER_SENT_SUCCESSFULLY;
}
/* }}} */


/* {{{ sapi_lsiapi_send_headers
 */
static void sapi_lsiapi_log_message(char *message
#if PHP_MAJOR_VERSION > 7 || (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION >= 1)
                                   , int syslog_type_int   /* unused */
#endif
                                   TSRMLS_DC)
{
    enum LSI_LOG_LEVEL eLSI_LOG_LEVEL = LSI_LOG_ERROR;
    lsi_session_t *session = SG(server_context);
    if (!session) {
        LSM_DBG((&MNAME), NULL, "LS LOG: no session: %s\n",message);
        fprintf(stderr, "LS LOG: %s\n", message);
    }
    else {
        LSM_DBG((&MNAME), NULL, "LS LOG: %s\n", message);
#if PHP_MAJOR_VERSION > 7 || (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION >= 1)
        switch (syslog_type_int) {
#if LOG_EMERG != LOG_CRIT
            case LOG_EMERG:
#endif
#if LOG_ALERT != LOG_CRIT
            case LOG_ALERT:
#endif
            case LOG_CRIT:
            case LOG_ERR:
                eLSI_LOG_LEVEL = LSI_LOG_ERROR;
                break;
            case LOG_WARNING:
                eLSI_LOG_LEVEL = LSI_LOG_WARN;
                break;
            case LOG_NOTICE:
                eLSI_LOG_LEVEL = LSI_LOG_NOTICE;
                break;
#if LOG_INFO != LOG_NOTICE
            case LOG_INFO:
                eLSI_LOG_LEVEL = LSI_LOG_INFO;
                break;
#endif
#if LOG_NOTICE != LOG_DEBUG
            case LOG_DEBUG:
                eLSI_LOG_LEVEL = LSI_LOG_DEBUG;
                break;
#endif
            default:
                eLSI_LOG_LEVEL = LSI_LOG_INFO;
                break;
        }
#endif
        LSM_LOG((&MNAME), eLSI_LOG_LEVEL, session, "LS LOG: %s\n", message);
    }
}
/* }}} */


// So that we get the php errors so we can log them ourselves.
static void mod_lsphp_sapi_error(int type,
                                 const char *error_msg,
                                 ...)
#if PHP_MAJOR_VERSION >= 7
                                 ZEND_ATTRIBUTE_FORMAT(printf, 2, 3)
#endif
{
    enum LSI_LOG_LEVEL eLSI_LOG_LEVEL;
    lsi_session_t *session = SG(server_context);
    LSM_DBG((&MNAME), session, "LS ERROR: %s\n", error_msg);
    // It appears from the code that the "type" is a bit map, so let's start with the most severe and move downwards
    if ((type & E_ERROR) ||
        (type & E_PARSE) ||
        (type & E_CORE_ERROR) ||
        (type & E_COMPILE_ERROR) ||
        (type & E_USER_ERROR)) {
        eLSI_LOG_LEVEL = LSI_LOG_ERROR;
    }
    else if ((type & E_WARNING) ||
             (type & E_CORE_WARNING) ||
             (type & E_COMPILE_WARNING) ||
             (type & E_USER_WARNING) ||
             (type & E_RECOVERABLE_ERROR)) {
        eLSI_LOG_LEVEL = LSI_LOG_WARN;
    }
    else if ((type & E_DEPRECATED) ||
             (type & E_NOTICE) ||
             (type & E_USER_NOTICE) ||
             (type & E_STRICT) ||
             (type & E_USER_DEPRECATED)) {
        eLSI_LOG_LEVEL = LSI_LOG_NOTICE;
    }
    else {
        eLSI_LOG_LEVEL = LSI_LOG_ERROR;
    }
    LSM_LOG((&MNAME), eLSI_LOG_LEVEL, session, "LS ERROR: %s\n", error_msg);
}



/* {{{ sapi_lsiapi_activate: This is where we'd read in the user's ini!
 */
//static int sapi_lsiapi_activate(TSRMLS_D)
//{
//    lsi_session_t *session = SG(server_context);
//    if (session) {
//        LSM_DBG((&MNAME), session, "sapi_lsiapi_activate\n");
//    }
//    return SUCCESS;
//}



static int lsiapi_release( lsi_session_t * session, mod_lsphp_t *mod_data)
{
    // I think for now, even if we have a fatal error,
    // we'll try to flush with a response code.  What can it hurt?
    if (mod_data->m_FatalError) {
        LSM_DBG((&MNAME), session, "Fatal error previously logged\n");
        //return(-1); Keep going to end the response.
    }
    if (!mod_data->m_SentHeader) {
        mod_data->m_SentHeader = 1;
    }
    LSM_DBG((&MNAME), session, "End Response NOW!\n");
    //...clean up as much as possible before the end_resp,
    // because from there on we shouldn't touch the session pointer.
    //if (g_api->set_module_data(session, &MNAME, LSI_DATA_HTTP, mod_data) < 0) {
    //    LSM_DBG((&MNAME), session,
    //            "Error in setting module data to NULL to better identify problems\n");
    //}
    SG(server_context) = NULL;
    if (!mod_data->m_DidEndResponse) {
        LSM_DBG((&MNAME), session, "Calling end_resp!\n");
        mod_data->m_DidEndResponse = 1;
        //g_api->end_resp(session);
    }
    //if (!mod_data->m_FatalError) {
    //    if (g_api->is_resp_handler_aborted(session) == -1) {
    //        LSM_ERR((&MNAME), session, "Session failure when ending\n");
    //        mod_data->m_FatalError = 1;
    //        return(-1);
    //    }
    //}
    mod_data->m_server_context = NULL;
    free(mod_data);
    mod_data = NULL;
    //LSM_DBG((&MNAME), session, "lsiapi_release now done\n");

    return(0);
}


static int free_request(lsi_session_t *session, mod_lsphp_t *mod_data TSRMLS_DC )
{
    LSM_DBG((&MNAME), session, "free_request\n");

    if (mod_data) {
        if ((!mod_data->m_SessionShutdown) || (mod_data->m_InProcess)/* || (!mod_data->m_HTTPEnd)*/) {
            LSM_DBG((&MNAME), session, "Too soon to free the session\n");
            return(0);
        }
        //LOCK_TEST_AND_SET(mod_data->m_ScriptRunning);
        LSM_DBG((&MNAME), session, "Ok to free data\n");
        //UNLOCK_TEST_AND_SET(mod_data->m_ScriptRunning);
    }
    if ((mod_data) && (mod_data->m_server_context)) {
        //LSM_DBG((&MNAME), session, "Original tid: %s\n", mod_data->m_tid);
        *((void **)mod_data->m_server_context) = NULL;
        mod_data->m_server_context = NULL;
    }
    else {
        LSM_DBG((&MNAME), session, "Can't reset server context - already NULL\n");
    }
    if ( SG(request_info).path_translated )
    {
        //efree( SG(request_info).path_translated );
        SG(request_info).path_translated = NULL; // it's actually ScriptFileName below
    }
    if (mod_data) {
        // Free any memory allocated
        if (mod_data->m_Cookies) {
            free(mod_data->m_Cookies);
            mod_data->m_Cookies = NULL;
        }
        if (mod_data->m_RequestMethod) {
            free(mod_data->m_RequestMethod);
            mod_data->m_RequestMethod = NULL;
        }
        if (mod_data->m_ContentType) {
            free(mod_data->m_ContentType);
            mod_data->m_ContentType = NULL;
        }
        if (mod_data->m_QueryString) {
            free(mod_data->m_QueryString);
            mod_data->m_QueryString = NULL;
        }
        if (mod_data->m_ScriptFileName) {
            free(mod_data->m_ScriptFileName);
            mod_data->m_ScriptFileName = NULL;
        }
        if (mod_data->m_ScriptName) {
            free(mod_data->m_ScriptName);
            mod_data->m_ScriptName = NULL;
        }
    }
    if (session) {
        if (lsiapi_release(session, mod_data) == -1) {
            LSM_DBG((&MNAME), session, "Release failed\n");
            return -1;
        }
    }
    // WARNING: From this point onwards you can't access session or module data!
    return(0);
}

/* {{{ sapi_lsiapi_deactivate
 */
//static int sapi_lsiapi_deactivate(TSRMLS_D)
//{
//    lsi_session_t *session = SG(server_context);
//    LSM_DBG((&MNAME), session, "sapi_lsiapi_deactivate\n");
//    return SUCCESS;
//}
/* }}} */


static int lsiapi_write_unbuffered( lsi_session_t *session,
                                    mod_lsphp_t   *mod_data,
                                    const char    *buffer,
                                    const int      buffer_length )

{
    int iResult;
    //char buffer_print[1500];
    //int  truncated;;
    //if (buffer_length + 1 >= sizeof(buffer_print)) {
    //    memcpy(buffer_print, buffer, sizeof(buffer_print) - 1);
    //    buffer_print[sizeof(buffer_print) - 1] = 0;
    //    truncated = 1;
    //}
    //else {
    //    memcpy(buffer_print, buffer, buffer_length);
    //    buffer_print[buffer_length] = 0;
    //    truncated = 0;
    //}
    //LSM_DBG((&MNAME), session, "Write unbuffered %d bytes: %s %s\n",
    //        buffer_length, buffer_print, truncated ? "TRUNCATED" : "");
    if (!mod_data) {
        LSM_ERR((&MNAME), session, "Error attempting to write to closed module\n");
        return(-1);
    }
    if (mod_data->m_FatalError) {
        LSM_DBG((&MNAME), session, "Fatal error previously logged\n");
        return(-1);
    }
    iResult = g_api->append_resp_body(session, buffer, buffer_length);
    if (iResult < 0) {
        LSM_ERR((&MNAME), session,
                "Session failure when appending %d bytes unbuffered\n", buffer_length);
        mod_data->m_FatalError = 1;
    }
    LSM_DBG((&MNAME), session, "Write unbuffered return %d\n", iResult);
    return(iResult);
}


#if (PHP_MAJOR_VERSION >= 7)
static size_t sapi_lsiapi_ub_write(const char *str, size_t str_length TSRMLS_DC)
#else
static int sapi_lsiapi_ub_write(const char *str, unsigned int str_length TSRMLS_DC)
#endif
{
    lsi_session_t *session = SG(server_context);
    mod_lsphp_t *mod_data = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);

    if (lsiapi_write_unbuffered(session, mod_data, str, str_length) < 0) {
        php_handle_aborted_connection();
    }
    return str_length; /* we always consume all the data passed to us. */
}


static char *lsiapi_get_script_file_name( lsi_session_t *session,
                                          mod_lsphp_t   *mod_data) {

    char value[MOD_LSPHP_MAX_ENV_VALUE];
    int  length;

    LSM_DBG((&MNAME), session, "Get Script File Name\n");
    if ((mod_data) && (mod_data->m_FatalError)) {
        LSM_DBG((&MNAME), session,
                "Fatal error previously logged (get_script_file_name)\n");
        return NULL;
    }
    if (mod_data->m_ScriptFileName) {
        return mod_data->m_ScriptFileName;
    }
    length = g_api->get_req_var_by_id(session, LSI_VAR_SCRIPTFILENAME, value,
                                      sizeof(value) );
    if (length <= 0) {
        LSM_ERR((&MNAME), session,
                 "Fatal error, expected script file name (length %d)\n", length);
        return NULL;
    }
    mod_data->m_ScriptFileName = malloc(length + 1);
    if (!mod_data->m_ScriptFileName) {
        value[length] = 0;
        LSM_ERR((&MNAME), session,
                "Fatal error, Insufficient memory for script: %s (length %d)\n",
                value, length);
        return NULL;
    }
    memcpy(mod_data->m_ScriptFileName, value, length);
    mod_data->m_ScriptFileName[length] = 0;
    LSM_DBG((&MNAME), session, "Get Script File Name result: %s\n",
            mod_data->m_ScriptFileName);
    return mod_data->m_ScriptFileName;
}


static char *lsiapi_get_content_type( lsi_session_t *session,
                                     mod_lsphp_t *mod_data ) {
    char *pchContentType;
    int  iLength = 80;
    LSM_DBG((&MNAME), session, "Get Content Type\n");
    if (mod_data->m_ContentType) {
        return( mod_data->m_ContentType );
    }
    if (mod_data->m_FatalError) {
        LSM_DBG((&MNAME), session, "Fatal error previously logged (GetContentType)\n");
        return(NULL);
    }
    //lsiapi_read_raw_header(session, mod_data);
    pchContentType = (char *)g_api->get_req_header_by_id(session,
                                                         LSI_HDR_CONTENT_TYPE,
                                                         &iLength);
    if (!pchContentType) {
        LSM_DBG((&MNAME), session, "ContentType NOT DEFINED\n");
        return NULL;
    }
    mod_data->m_ContentType = malloc(iLength + 1);
    if (mod_data->m_ContentType) {
        memcpy(mod_data->m_ContentType, pchContentType, iLength);
        mod_data->m_ContentType[iLength] = 0;
    }
    LSM_DBG((&MNAME), session, "ContentType %s (%d bytes)\n", mod_data->m_ContentType, iLength);
    return mod_data->m_ContentType;
}

static char *lsiapi_get_request_method( lsi_session_t *session,
                                        mod_lsphp_t *mod_data) {
    char value[MOD_LSPHP_MAX_ENV_VALUE];
    int  length;

    LSM_DBG((&MNAME), session, "get_request_method\n");
    if (mod_data->m_RequestMethod) {
        return( mod_data->m_RequestMethod );
    }
    if (mod_data->m_FatalError) {
        LSM_DBG((&MNAME), session,
                "Fatal error previously logged (get_request_method)\n");
        return NULL;
    }
    length = g_api->get_req_var_by_id( session, LSI_VAR_REQ_METHOD,
                                       value, sizeof(value));
    if (length <= 0) {
        LSM_ERR((&MNAME), session, "Invalid request method length (%d)\n", length);
        return(NULL);
    }
    mod_data->m_RequestMethod = malloc(length + 1);
    memcpy(mod_data->m_RequestMethod, value, length);
    mod_data->m_RequestMethod[length] = 0;
    LSM_DBG((&MNAME), session, "get_request_method result: %s\n",
            mod_data->m_RequestMethod);
    return mod_data->m_RequestMethod;
}

static char *lsiapi_get_query_string( lsi_session_t *session, mod_lsphp_t *mod_data ) {
    // Not in the header in the API, but the environment
    int  iLength = LSAPI_MAX_URI_LENGTH;
    char *pchQueryString;

    LSM_DBG((&MNAME), session, "get_query_string\n");
    if (mod_data->m_FatalError) {
        LSM_DBG((&MNAME), session,
                "Fatal error previously logged (get_query_string)\n");
        return "";
    }
    //g_api->parse_req_args(session,0,0,NULL,0);
    pchQueryString = (char *)g_api->get_req_query_string( session, &iLength );
    if (!pchQueryString) {
        LSM_DBG((&MNAME), session, "get_query_string NOT AVAILABLE!\n");
        return "";
    }
    else {
        mod_data->m_QueryString = malloc(iLength + 1);
        if (mod_data->m_QueryString) {
            memcpy(mod_data->m_QueryString, pchQueryString, iLength);
            mod_data->m_QueryString[iLength] = 0;
            pchQueryString = mod_data->m_QueryString;
        }
        else {
            LSM_ERR((&MNAME), session,
                    "get_query_string Error allocating string, %d bytes", iLength);
            return "";
        }
    }
    if (!pchQueryString) {
        pchQueryString = "";
    }
    LSM_DBG((&MNAME), session, "get_query_string = %s\n", pchQueryString);

    return pchQueryString;
}


static char *lsiapi_get_script_name( lsi_session_t *session,
                                     mod_lsphp_t *mod_data ) {
    // Not in the header in the API, but the environment
    char value[MOD_LSPHP_MAX_ENV_VALUE];
    int  length;

    LSM_DBG((&MNAME), session, "get_script_name\n");
    if (mod_data->m_ScriptName) {
        return mod_data->m_ScriptName;
    }
    if (mod_data->m_FatalError) {
        LSM_DBG((&MNAME), session,
                "Fatal error previously logged (get_script_name)\n");
        return NULL;
    }
    length = g_api->get_req_var_by_id(session, LSI_VAR_REQ_URI, value, sizeof(value) );
    if (length <= 0) {
        LSM_ERR((&MNAME), session, "Fatal Error getting URI (length: %d)\n", length);
        return NULL;
    }
    mod_data->m_ScriptName = malloc(length + 1);
    if (!mod_data->m_ScriptName) {
        LSM_ERR((&MNAME), session,
                "Fatal Error, insufficient memory getting URI (%d)\n", length);
        return NULL;
    }
    memcpy(mod_data->m_ScriptName, value, length);
    mod_data->m_ScriptName[length] = 0;
    LSM_DBG((&MNAME), session, "get_script_name result: %s\n", mod_data->m_ScriptName);
    return mod_data->m_ScriptName;
}

static int lsiapi_get_req_body_len( lsi_session_t *session, mod_lsphp_t *mod_data ) {
    // Requires a separate API call.
    int  iResult;
    LSM_DBG((&MNAME), session, "get_req_body_len\n");
    if (mod_data->m_FatalError) {
        LSM_DBG((&MNAME), session,
                "Fatal error previously logged (get_req_body_len)\n");
        return(-1);
    }
    iResult = (int)g_api->get_req_content_length( session );
    LSM_DBG((&MNAME), session, "get_req_body_len result: %d\n", iResult);
    return(iResult);
}


static char *lsiapi_get_header_auth( lsi_session_t *session,
                                     mod_lsphp_t *mod_data,
                                     char chAuthorization[] ) {
    char *pchAuthorization;
    int  iLength = 0;
    LSM_DBG((&MNAME), session, "Get Header Authorization\n");
    if (mod_data->m_FatalError) {
        LSM_DBG((&MNAME), session, "Fatal error previously logged (get_header_auth)\n");
        return NULL;
    }
    lsiapi_read_raw_header(session, mod_data);
    pchAuthorization = (char *)g_api->get_req_header_by_id(session,
                                                           LSI_HDR_AUTHORIZATION,
                                                           &iLength);
    if (!iLength) {
        LSM_DBG((&MNAME), session, "Get Header Authorization - 0 length\n");
        chAuthorization[0] = 0;
        return(pchAuthorization);
    }
    memcpy(chAuthorization, pchAuthorization, iLength);
    chAuthorization[iLength] = 0;
    pchAuthorization = chAuthorization;
    LSM_DBG((&MNAME), session,
            "Get Header Authorization result, length: %d: %s\n",
            iLength, pchAuthorization);
    return pchAuthorization;
}


static int lsiapi_get_protocol_number( lsi_session_t *session, mod_lsphp_t *mod_data ) {
    // Not in the header in the API, but the environment
    char value[MOD_LSPHP_MAX_ENV_VALUE];

    LSM_DBG((&MNAME), session, "get_protocol_number\n");
    if (mod_data->m_FatalError) {
        LSM_DBG((&MNAME), session,
                "Fatal error previously logged (get_protocol_number)\n");
        return 1000; // Force 1.0
    }
    g_api->get_req_var_by_id(session, LSI_VAR_SERVER_PROTO, value, sizeof(value));
    if (value[0] == 0) {
        LSM_DBG((&MNAME), session, "Protocol not found - defaulting to 1.0\n");
        return 1000; // Force 1.0
    }
    if ((!(strncasecmp(value,"HTTP/",5))) &&
        (strlen(value) == 8) &&
        (value[6] == '.') &&
        ((value[5] >= '0') && (value[5] <= '9')) &&
        ((value[7] >= '0') && (value[7] <= '9'))) {
        int iProtocol = 1000 + ((value[5] - '0' - 1) * 10) + (value[7] - '0');
        LSM_DBG((&MNAME), session, "Protocol value converted from %s to %d\n",
                value, iProtocol);
        return iProtocol;
    }
    LSM_DBG((&MNAME), session,
            "Protocol value unexpected - default to 1.0 (%s)\n", value);
    return(1000);
}


/* NOTUSED?
static char *malloc_str(char *str)

{
    char *new_str;
    int  len = strlen(str) + 1;

    new_str = malloc(len);
    if (!new_str) {
        return(NULL);
    }
    strcpy(new_str,str); // About the only time a strcpy is safe.
    return(new_str);
}
*/


static int init_request_info( lsi_session_t *session, mod_lsphp_t *mod_data TSRMLS_DC )
{
    int iRet = -1;
    char chAuthorization[SAPI_LSAPI_MAX_HEADER_LENGTH];
    //NOTUSED? char value[MOD_LSPHP_MAX_ENV_VALUE];
    LSM_DBG((&MNAME), session, "init_request_info\n");

    mod_data->m_ScriptFileName = lsiapi_get_script_file_name( session, mod_data );
    if (!mod_data->m_ScriptFileName) {
        sapi_lsiapi_set_response_status( session, 400 /*HTTP_BAD_REQUEST*/ TSRMLS_CC );
    }
    else if (lstat(mod_data->m_ScriptFileName, &mod_data->m_StatScript)) {
        if (errno == ENOENT ) {
            LSM_ERR((&MNAME), session,
                    "Script: %s not found\n", mod_data->m_ScriptFileName);
            sapi_lsiapi_set_response_status( session, 404 /*HTTP_NOT_FOUND*/ TSRMLS_CC );
        }
        else {
            LSM_ERR((&MNAME), session,
                    "Script: %s access error: %s\n",
                    mod_data->m_ScriptFileName, strerror(errno));
            sapi_lsiapi_set_response_status( session, 400 /*HTTP_BAD_REQUEST*/ TSRMLS_CC );
        }
    }
    else if (((mod_data->m_StatScript.st_mode & S_IFMT) == S_IFBLK) ||
             ((mod_data->m_StatScript.st_mode & S_IFMT) == S_IFCHR) ||
             ((mod_data->m_StatScript.st_mode & S_IFMT) == S_IFDIR) ||
             ((mod_data->m_StatScript.st_mode & S_IFMT) == S_IFIFO) ||
             ((mod_data->m_StatScript.st_mode & S_IFMT) == S_IFSOCK)) {
        LSM_ERR((&MNAME), session, "Script: %s not a regular file\n",
                mod_data->m_ScriptFileName);
        sapi_lsiapi_set_response_status( session, 403 /*HTTP_FORBIDDEN*/ TSRMLS_CC );
    }
    else {
        char *pAuth;
        SG(request_info).content_type = lsiapi_get_content_type( session, mod_data );
        SG(request_info).request_method = lsiapi_get_request_method( session, mod_data );
        SG(request_info).query_string = lsiapi_get_query_string( session, mod_data );
        SG(request_info).proto_num = lsiapi_get_protocol_number( session, mod_data );
        SG(request_info).request_uri = lsiapi_get_script_name( session, mod_data );
        SG(request_info).content_length = lsiapi_get_req_body_len( session, mod_data );
        SG(request_info).path_translated = mod_data->m_ScriptFileName;
        mod_data->m_server_context = &SG(server_context);
        SG(server_context) = (lsi_session_t *)session;

        /* It is not reset by zend engine, set it to 200. */
        sapi_lsiapi_set_response_status(session, 200 /* HTTP_OK */TSRMLS_CC ); // Initialize it to ok from here.

        pAuth = lsiapi_get_header_auth( session, mod_data, chAuthorization );
        php_handle_auth_data(pAuth TSRMLS_CC);

        LSM_DBG((&MNAME), session, "   content_type: %s\n",SG(request_info).content_type);
        LSM_DBG((&MNAME), session, "   request_meth: %s\n",SG(request_info).request_method);
        LSM_DBG((&MNAME), session, "   query_string: %s\n",SG(request_info).query_string);
        LSM_DBG((&MNAME), session, "   request_uri : %s\n",SG(request_info).request_uri);
        LSM_DBG((&MNAME), session, "   content_len : %ld\n",SG(request_info).content_length);
        LSM_DBG((&MNAME), session, "   path_transla: %s\n",SG(request_info).path_translated);

        iRet = 0;
    }
    return iRet;
}

#if (PHP_MAJOR_VERSION >= 7)
static zend_stat_t *sapi_lsiapi_get_zend_stat(TSRMLS_D)
{
    lsi_session_t *session = SG(server_context);
    mod_lsphp_t *mod_data = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);

    if (!mod_data) {
        LSM_ERR((&MNAME), session, "NULL session handle in get_stat, session: %p\n", session);
        return NULL;
    }
    if (!mod_data->m_ScriptFileName) {
        LSM_ERR((&MNAME), session, "sapi_lsiapi_get_stat but no script saved!");
        return NULL;
    }
    LSM_DBG((&MNAME), session, "sapi_lsiapi_get_stat: %s\n", mod_data->m_ScriptFileName);

    mod_data->m_zend_stat.st_uid = mod_data->m_StatScript.st_uid;
    mod_data->m_zend_stat.st_gid = mod_data->m_StatScript.st_gid;
    mod_data->m_zend_stat.st_dev = mod_data->m_StatScript.st_dev;
    mod_data->m_zend_stat.st_ino = mod_data->m_StatScript.st_ino;
    mod_data->m_zend_stat.st_atime = mod_data->m_StatScript.st_atime;
    mod_data->m_zend_stat.st_mtime = mod_data->m_StatScript.st_mtime;
    mod_data->m_zend_stat.st_ctime = mod_data->m_StatScript.st_ctime;
    mod_data->m_zend_stat.st_size = mod_data->m_StatScript.st_size;
    mod_data->m_zend_stat.st_nlink = mod_data->m_StatScript.st_nlink;

    return &mod_data->m_zend_stat;
}
#else // 5.x
static struct stat *sapi_lsiapi_get_stat(TSRMLS_D)
{
    lsi_session_t *session = SG(server_context);
    mod_lsphp_t *mod_data = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);

    if (!mod_data) {
        LSM_ERR((&MNAME), session, "NULL session handle in get_stat, session: %p\n", session);
        return NULL;
    }
    if (!mod_data->m_ScriptFileName) {
        LSM_ERR((&MNAME), session, "sapi_lsiapi_get_stat but no script saved!");
        return NULL;
    }
    LSM_DBG((&MNAME), session, "sapi_lsiapi_get_stat: %s\n",
            mod_data->m_ScriptFileName);
    return &mod_data->m_StatScript;
}
#endif


static int lsiapi_execute_script( lsi_session_t *session,
                                 mod_lsphp_t *mod_data,
                                 zend_file_handle * file_handle TSRMLS_DC)
{
    int  result;
    char path[LSAPI_MAX_URI_LENGTH];
    struct timeb entry_time;

    lstrncpy(path,SG(request_info).path_translated,sizeof(path));
    LSM_DBG((&MNAME), session, "lsiapi_execute_script: %s\n",
            SG(request_info).path_translated);
    file_handle->type = ZEND_HANDLE_FILENAME;
    file_handle->handle.fd = 0;
    file_handle->filename = path;
    file_handle->free_filename = 0;
    file_handle->opened_path = NULL;
    if (mod_lsphp_config.m_slow_ms) {
        ftime(&entry_time);
    }
    if ((result = php_execute_script(file_handle TSRMLS_CC)) < 0) {
    //zend_execute_scripts(ZEND_INCLUDE, NULL, 1, file_handle);
        LSM_ERR((&MNAME), session, "\nError in script %s\n",
                SG(request_info).path_translated);
    }
    if (mod_lsphp_config.m_slow_ms) {
        struct timeb exit_time;
        ftime(&exit_time);
        time_t elapsed_time;
        elapsed_time = ((exit_time.time * 1000) + exit_time.millitm) -
                       ((entry_time.time * 1000) + entry_time.millitm);
        LSM_DBG((&MNAME), session,
                "lsiapi_execute_script: took %ld ms\n", elapsed_time);
        if (elapsed_time >= mod_lsphp_config.m_slow_ms) {
            LSM_WRN((&MNAME), session,
                    "Long running script %s took %ld ms\n", path, elapsed_time);
        }
    }

    LSM_DBG((&MNAME), session,
            "lsiapi_execute_script: %s DONE! return code: %d, exit status: %d\n",
            SG(request_info).path_translated, result, EG(exit_status));
    return 0;
}


static void alter_ini( int key_id, const char * key, int keyLen,
                       const char * value, int valLen, void * arg )
{
    lsi_session_t *session = (lsi_session_t *)arg;
#if PHP_MAJOR_VERSION >= 7
    zend_string * psKey;
#endif
    int type = ZEND_INI_PERDIR;
    int stage = PHP_INI_STAGE_RUNTIME;
    if ( '\001' == *key ) {
        ++key;
        if ( *key == 4 ) {
            type = ZEND_INI_SYSTEM;
        }
        else
        {
            stage = PHP_INI_STAGE_HTACCESS;
        }
        ++key;
        --keyLen;
        if (( keyLen == 7 )&&( strncasecmp( key, "engine", 6 )== 0 ))
        {
            if ( *value == '0' )
            {
                LSM_DBG((&MNAME), session, "alter_ini ignore engine\n");
                //LS(engine) = 0;
            }
        }
        else
        {
            char key_print[1024];
            char value_print[1024];
            if (keyLen + 1 > sizeof(key_print)) {
                memcpy(key_print,key,sizeof(key_print));
                key_print[sizeof(key_print) - 1] = 0;
            }
            else {
                memcpy(key_print,key,keyLen);
                key_print[keyLen] = 0;
            }
            if (valLen + 1 > sizeof(value_print)) {
                memcpy(value_print,value,sizeof(value_print));
                value_print[sizeof(value_print) - 1] = 0;
            }
            else {
                memcpy(value_print,value,valLen);
                value_print[valLen] = 0;
            }
            LSM_DBG((&MNAME), session, "override_ini, key: %s, value: %s\n", key_print, value_print);

#if PHP_MAJOR_VERSION >= 7
            --keyLen;
            psKey = zend_string_init(key, keyLen, 1);
            zend_alter_ini_entry_chars(psKey, (char *)value, valLen,
                                       type, stage);
            zend_string_release(psKey);
#else
            zend_alter_ini_entry((char *)key, keyLen, (char *)value, valLen,
                                 type, stage);
#endif
            LSM_DBG((&MNAME), session, "override_ini, alter done for key\n");
        }
    }
}


static void override_ini(lsi_session_t *session)
{
    LSM_DBG((&MNAME), session, "override_ini\n");
    g_api->foreach(session, LSI_DATA_REQ_SPECIAL_ENV, NULL, alter_ini, session);
    LSM_DBG((&MNAME), session, "override_ini DONE\n");
}


#ifdef NO_PHP
static int no_php(const lsi_session_t *session, int no_end_resp) {
    // Ok, assume that the URI is an actual file, open it, read it and append
    // it as the body.  This should take PHP out of the mix entirely.
#define NO_PHP_BUFFER_SIZE  8192
    char buffer[NO_PHP_BUFFER_SIZE];
    char script[LSAPI_MAX_URI_LENGTH];
    int  fd;
    ssize_t  bytes;
    int rc;
    LSM_DBG((&MNAME), session, "no_php entry, session: %p\n", session);
    if (!lsiapi_get_script_file_name((lsi_session_t *)session, NULL)) {
        LSM_ERR((&MNAME), session, "script file name expected but not found\n");
        return(LSI_ERROR);
    }
    fd = open(script, O_RDONLY);
    if (fd < 0) {
        LSM_ERR((&MNAME), session, "Error %s opening script: %s\n", strerror(errno), script);
        return(LSI_ERROR);
    }
    do {
        bytes = read(fd, buffer, NO_PHP_BUFFER_SIZE);
        if (bytes < 0) {
            close(fd);
            LSM_ERR((&MNAME), session, "Error %s reading script: %s\n", strerror(errno), script);
            return(LSI_ERROR);
        }
        if (bytes > 0) {
            rc = g_api->append_resp_body(session, buffer, bytes);
            if (rc < 0) {
                close(fd);
                LSM_ERR((&MNAME), session, "Error %s sending block in script: %s\n", strerror(errno), script);
                return(LSI_ERROR);
            }
        }
    } while (bytes == NO_PHP_BUFFER_SIZE);
    close(fd);
    if (!no_end_resp) {
        g_api->end_resp(session);
    }
    return(LSI_OK);
}
#endif // NO_PHP


static int lsiapi_module_main(lsi_session_t *session, mod_lsphp_t *mod_data TSRMLS_DC)
{
    if (php_request_startup(TSRMLS_C) == FAILURE ) {
        LSM_DBG((&MNAME), session, "lsiapi_module_main failure UNLOCK\n");
        //UNLOCK_TEST_AND_SET(mod_data->m_ScriptRunning);
        return -1;
    }
    LSM_DBG((&MNAME), session, "lsiapi_module_main\n");

    if (SG(request_info).content_length == 0)
        SG(post_read) = 1;
    //if (show_source) {
    //    zend_syntax_highlighter_ini syntax_highlighter_ini;

    //    php_get_highlight_struct(&syntax_highlighter_ini);
    //    highlight_file(SG(request_info).path_translated,
    //                   &syntax_highlighter_ini TSRMLS_CC);
    //} else {
    {
        zend_file_handle file_handle;
        memset(&file_handle,0,sizeof(file_handle));
#ifdef NO_PHP
        no_php(session, 1);
#else
        lsiapi_execute_script( session, mod_data, &file_handle TSRMLS_CC);
#endif
    }
    zend_try {
        //LSAPI_Flush(pLSAPI_Request);
        php_request_shutdown(NULL);

    } zend_end_try();
    // Assume all session and module data is now gone!
    LSM_DBG((&MNAME), session, "END lsiapi_module_main\n");
    return 0;
}


// NOTE: This is the equivalent of php_handler in mod_lsphp.
static int process_req( lsi_session_t *session, mod_lsphp_t *mod_data TSRMLS_DC )
{
    int ret = 0;
    LSM_DBG((&MNAME), session, "process_req LOCK\n");
    //LOCK_TEST_AND_SET(mod_data->m_ScriptRunning);
    // The line below is the following two lines!
    zend_first_try {
        //LS(engine) = 1;
        if (ret >= 0) {
            override_ini(session);

            ret = init_request_info( session, mod_data TSRMLS_CC );
        }
        if (ret >= 0 ) {
            ret = lsiapi_module_main( session, mod_data TSRMLS_CC );
        }
    } zend_end_try();
    LSM_DBG((&MNAME), session, "process_req finishing, ret: %d, Response is %s\n",
            ret, g_api->get_resp_buffer_compress_method(session) == 1 ?
                         "GZIPPED" : "NOT GZIPPED");
    //UNLOCK_TEST_AND_SET(mod_data->m_ScriptRunning);
    //free_request(session, mod_data TSRMLS_CC);
    return ret;
}


//static PHP_MINIT_FUNCTION(litespeed)
//{
//#ifdef ZTS
//    LSM_DBG((&MNAME), NULL, "MINIT for litespeed\n");
//#endif
//#ifdef ZTS
//    ts_allocate_id(&mod_lsphp_globals_id,
//#if (PHP_MAJOR_VERSION >= 7)
//                   sizeof(mod_lsphp_globals_t),
//#else
//                   sizeof(zend_mod_lsphp_globals),
//#endif
//                   (ts_allocate_ctor) NULL,
//                   NULL);
//#endif
//    user_config_cache_init();
//    REGISTER_INI_ENTRIES();
//    return SUCCESS;
//}


//static PHP_MSHUTDOWN_FUNCTION(litespeed)
//{
//    UNREGISTER_INI_ENTRIES();
//    return SUCCESS;
//}


static PHP_RSHUTDOWN_FUNCTION(litespeed)
{
    lsi_session_t *session = SG(server_context);
    mod_lsphp_t *mod_data;

    if (!session) {
        LSM_ERR((&MNAME), NULL, "RSHUTDOWN NO SESSION\n");
        return SUCCESS;
    }
    LSM_DBG((&MNAME), session, "RSHUTDOWN for litespeed\n");
    mod_data = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);
    if (mod_data) {
        mod_data->m_SessionShutdown = 1;
        free_request((lsi_session_t *)session, mod_data TSRMLS_CC);
    }
    else {
        LSM_DBG((&MNAME), NULL, "Unexpected NULL mod_data in RSHUTDOWN, session: %p\n", session);
    }
    return SUCCESS;
}


//static int mod_lsphp_free_callback(void *pvData) {
//    if (pvData) {
//        free(pvData);
//    }
//    return 0;
//}


#ifdef ZTS
static void mod_lsphp_thread_cleanup(void *param) {
    // Free the thread data in PHP
    LSM_DBG((&MNAME), NULL, "Free thread data\n");
    ts_free_thread();
}
#endif // ZTS


//return 0 for correctly parsing
static int mod_lsphp_parseList(module_param_info_t *param, mod_lsphp_config_t *config)
{
    int *pParam;
    ls_confparser_t confparser;

    LSM_DBG((&MNAME), NULL, "mod_lsphp_parseList\n");

    ls_confparser(&confparser);
    memset(config,0,sizeof(*config));
    ls_objarray_t *pList = ls_confparser_line(&confparser, param->val,
                                              param->val + param->val_len);

    int count = ls_objarray_getsize(pList);
    if (count == 0) {
        LSM_DBG((&MNAME), NULL, "mod_lsphp_parseList 0 count, early return\n");
        return 0;
    }
    unsigned long maxParamNum = param->key_index + 1;
    if (maxParamNum > 1) {
        LSM_DBG((&MNAME), NULL, "maxParamNum initially set to %ld, dropping to 1\n", maxParamNum);
        maxParamNum = 1;
    }
    ls_str_t *p;
    long val;
    int i;

    for (i = 0; i < count && i < maxParamNum; ++i)
    {
        p = (ls_str_t *)ls_objarray_getobj(pList, i);
        val = strtol(ls_str_cstr(p), NULL, 10);
        LSM_DBG((&MNAME), NULL, "pList[%d] = %ld\n", i, val);

        switch(param->key_index)
        {
        case 0:
            pParam = &config->m_slow_ms;
            break;
        }
        *pParam = val;
    }

    ls_confparser_d(&confparser);
    return 0;
}


static void *mod_lsphp_parseConfig(module_param_info_t *param, int param_count,
                                   void *_initial_config, int level, const char *name)
{
    int i;
    mod_lsphp_config_t *pInitConfig = (mod_lsphp_config_t *)_initial_config;
    mod_lsphp_config_t *pConfig = &mod_lsphp_config;
    LSM_DBG((&MNAME), NULL, "mod_lsphp_parseConfig\n");
    if (pInitConfig)
        memcpy(pConfig, pInitConfig, sizeof(mod_lsphp_config_t));
    else
        memset(pConfig, 0, sizeof(mod_lsphp_config_t));

    if (!param)
        return (void *)pConfig;

    for (i=0; i<param_count; ++i)
    {
        mod_lsphp_parseList(&param[i], pConfig);
    }
    return (void *)pConfig;
}

static int mod_lsphp_mod_init(lsi_module_t* module)
{
    LSM_DBG((&MNAME), NULL, "mod_lsphp_mod_init\n");
    g_api->init_module_data(module, NULL/*mod_lsphp_free_callback*/, LSI_DATA_HTTP);
#ifdef ZTS
    LSM_DBG((&MNAME), NULL, "mod_lsphp_mod_init - calling tsrm_startup\n");
    tsrm_startup(1, 1, 0, NULL);
    g_api->register_thread_cleanup(&MNAME, mod_lsphp_thread_cleanup, NULL);
#endif
#ifdef ZTS
#if (PHP_MAJOR_VERSION < 7)
    compiler_globals = ts_resource(compiler_globals_id);
    executor_globals = ts_resource(executor_globals_id);
    core_globals = ts_resource(core_globals_id);
    sapi_globals = ts_resource(sapi_globals_id);
    tsrm_ls = ts_resource(0);
#endif // PHP5
#endif
#ifdef ZEND_SIGNALS
    zend_signal_startup();
#endif
    sapi_startup(&lsiapi_sapi_module);
    lsiapi_sapi_module.startup(&lsiapi_sapi_module);

    LSM_DBG((&MNAME), NULL, "Calling get_config\n");
    mod_lsphp_config_t *config = (mod_lsphp_config_t *) g_api->get_config(NULL, &MNAME);
    if (config) {
        LSM_NOT((&MNAME), NULL, "%s set to %d by configuration\n",
                mod_lsphp_config_key[0].config_key, mod_lsphp_config.m_slow_ms);
    }
    return(0);
}


static int mod_lsphp_begin_process(const lsi_session_t *session) {
    mod_lsphp_t *mod_data;
    int rc;
#ifdef ZTS
    void *resource;
    pid_t tid = gettid(); // For debugging
#endif

    LSM_DBG((&MNAME), session, "Begin_Process entry, session: %p\n", session);
#ifdef NO_PHP
    //return(no_php(session, 0));
#endif // NO_PHP
#ifdef ZTS
    resource = (void *)ts_resource(0);
    TSRMLS_FETCH(); // v5
    ZEND_TSRMLS_CACHE_UPDATE(); // v7
    LSM_DBG((&MNAME), session, "    tid: %d, resource: %p, their tid: %lx\n", tid, resource, tsrm_thread_id());
#endif

    mod_data = malloc(sizeof(mod_lsphp_t));
    if (!mod_data) {
        LSM_ERR((&MNAME), session, "Error in get of module data\n");
        return(-1);
    }
    memset(mod_data, 0, sizeof(*mod_data));
    mod_data->m_InProcess = 1;
    if (g_api->set_module_data(session, &MNAME, LSI_DATA_HTTP, mod_data) < 0) {
        LSM_ERR((&MNAME), session, "Insufficient memory to create module data\n");
        return(-1);
    }
    //mod_data->m_server_context = &SG(server_context);
    //SG(server_context) = (lsi_session_t *)session;
    mod_data->m_ContentLength = g_api->get_req_content_length(session);
    rc = process_req((lsi_session_t *)session, mod_data TSRMLS_CC);
    mod_data->m_InProcess = 0;
    // This is now the earliest a session can be freed!
    free_request((lsi_session_t *)session, mod_data TSRMLS_CC);
    return rc;
}


// Not necessary at this time.
//int MOD_LSPHP_On_Read_Request_Body(const lsi_session_t* session)
//{
//    /* We'll only get here when the ENTIRE request has been read.  */
//    mod_lsphp_t *mod_data;
//    mod_data = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);
//    LSM_DBG((&MNAME), session, "On_Read_Request_Body\n");
//    return processReq((lsi_session_t *)session, mod_data TSRMLS_CC);
//}


//static int mod_lsphp_cleanup_process(const lsi_session_t *session) {
//    /* This function is ONLY here to deal with SG(server_context) and the fact
//     * that it's not handled by PHP in a rational way */
//    mod_lsphp_t *mod_data;
//    LSM_DBG((&MNAME), session, "mod_lsphp_cleanup_process\n");
//
//    mod_data = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);
//    if (mod_data) {
//        mod_data->m_HTTPEnd = 1;
//        free_request((lsi_session_t *)session, mod_data TSRMLS_CC);
//    }
//    return(0);
//}


//static int mod_lsphp_http_end(lsi_param_t *param)
//{
//    lsi_session_t *session = (lsi_session_t *)param->session;
//    mod_lsphp_t *mod_data;
//
//    LSM_DBG((&MNAME), session, "http_end for in mod_lsphp\n");
//    if (!session) {
//        LSM_ERR((&MNAME), NULL, "http_end NO SESSION\n");
//        return SUCCESS;
//    }
//    LSM_DBG((&MNAME), session, "http_end for litespeed\n");
//    mod_data = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);
//    if (mod_data) {
//        mod_data->m_HTTPEnd = 1;
//        free_request((lsi_session_t *)session, mod_data TSRMLS_CC);
//    }
//    else {
//        LSM_DBG((&MNAME), NULL, "Unexpected NULL mod_data in http_end\n");
//    }
//    return SUCCESS;
//}



static int mod_lsphp_termination(lsi_param_t *param) {
    php_module_shutdown(TSRMLS_C);
    //zend_hash_destroy(&LS(m_HashTable_user_config_cache));
    sapi_shutdown();

#ifdef ZTS
    tsrm_shutdown();
#endif
    return(0);
}


