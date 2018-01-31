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

/* This is multi-threaded, so we have to run with ZTS (Zend Thread Safety) enabled */
//#define ZTS

#ifndef  _MOD_LSPHP_H_
#define  _MOD_LSPHP_H_

#if defined (c_plusplus) || defined (__cplusplus)
extern "C" {
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <lsdef.h>
#include <ls.h>

#include "php.h"
#include "SAPI.h"
#include "php_main.h"
#include "php_ini.h"
#include "php_variables.h"
#include "zend_highlight.h"
#include "zend.h"
#include "ext/standard/basic_functions.h"

#define MOD_LSPHP_MAX_ENV_VALUE 2048

/**
 * Note on the Environment variables associative array.
 * MOST of the environment variables are conversation specific.
 * That means that I need to regenerate the table for each session.
 * However, if there are environment variables that don't fit that
 * mold as time goes on and we need to gain performance, it might
 * make sense to make a global for those as well.  But I tried that
 * and it didn't seem to work out for now.
 */
typedef struct mod_lsphp_s {
    int                               m_FatalError;
    int                               m_SentHeader;
    char                            * m_ScriptFileName;
    char                            * m_Cookies;
    char                            * m_ContentType;
    char                            * m_QueryString;
    char                            * m_RequestMethod;
    char                            * m_ScriptName;
    struct stat                       m_StatScript;
#if (PHP_MAJOR_VERSION >= 7)
    zend_stat_t                       m_zend_stat;
#endif
    //long                              m_LastActive;
    //long                              m_ReqBegin;
    //zval                              m_zvalArrayEnv;
    //int                               m_ArrayEnvInit;
    //int                               m_SourceHighlight;
    //int                               m_RequestTypeGet;
    int64_t                           m_ContentLength;
    int64_t                           m_ContentRead;
    int                               m_ReadRawHeader;
    void                            * m_server_context;
    //char                              m_tid[80];
    int                               m_SessionShutdown; // Don't delete until it's set to avoid known bugs.
    int                               m_InProcess;
    int                               m_DidEndResponse;
    //int                               m_HTTPEnd;
    int                               m_DidRegisterVariables;
} mod_lsphp_t;

#ifdef ZTS
#if (PHP_MAJOR_VERSION >= 7)
ZEND_TSRMLS_CACHE_EXTERN()
#else // PHP version >= 7
#define ZEND_TSRMLS_CACHE_EXTERN()
#define ZEND_TSRMLS_CACHE_UPDATE()
#endif //PHP >= 7
#endif // ZTS


static int php_lsiapi_startup(sapi_module_struct *sapi_module);
static void sapi_lsiapi_flush(void * server_context);
//static int populate_env(lsi_session_t *session, mod_lsphp_t *mod_data TSRMLS_DC);
//static char *lsiapi_getenv(lsi_session_t *session,
//                           char *name,
//                           size_t iNameLen TSRMLS_DC);
//NOTUSED? static char *sapi_lsiapi_getenv(char *pchName, size_t iNameLen TSRMLS_DC);
static void lsiapi_foreach_callback(int key_id, const char * key, int key_len,
                                    const char * value, int val_len, void * arg);
static void sapi_lsiapi_register_variables(zval *track_vars_array TSRMLS_DC);
static int lsiapi_read_req_body( lsi_session_t *session, mod_lsphp_t *mod_data,
                                char * pchBuffer, int iSize );
#if PHP_MAJOR_VERSION >= 7
static size_t sapi_lsiapi_read_post(char *buffer, size_t count_bytes TSRMLS_DC);
#else
static int sapi_lsiapi_read_post(char *buffer, uint count_bytes TSRMLS_DC);
#endif
static char *lsiapi_read_cookies( lsi_session_t *session, mod_lsphp_t *mod_data,
                                 int *piCookieLength );
static char *sapi_lsiapi_read_cookies(TSRMLS_D);
static int lsiapi_set_resp_status( lsi_session_t *session, int code );
static int sapi_lsiapi_set_response_status(lsi_session_t *session, int code TSRMLS_DC);
static int lsiapi_append_resp_header( lsi_session_t * session, mod_lsphp_t *mod_data,
                                     const char * pBuf, int len );
static int lsiapi_finalize_resp_headers( lsi_session_t * session, mod_lsphp_t *mod_data);
static int sapi_header_handler(sapi_header_struct *sapi_header,
                               sapi_header_op_enum op,
                               sapi_headers_struct *sapi_headers TSRMLS_DC);
static int sapi_lsiapi_send_headers(sapi_headers_struct *sapi_headers TSRMLS_DC);
static void sapi_lsiapi_log_message(char *message
#if PHP_MAJOR_VERSION > 7 || (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION >= 1)
                                   , int syslog_type_int   /* unused */
#endif
                                   TSRMLS_DC);
static void mod_lsphp_sapi_error(int type, const char *error_msg, ...)
#if PHP_MAJOR_VERSION >= 7
                                 ZEND_ATTRIBUTE_FORMAT(printf, 2, 3)
#endif
                                 ;
//static int sapi_lsiapi_activate(TSRMLS_D);
static int lsiapi_release( lsi_session_t * session, mod_lsphp_t *mod_data);
//static int sapi_lsiapi_deactivate(TSRMLS_D);
static int lsiapi_write_unbuffered( lsi_session_t *session,
                                    mod_lsphp_t   *mod_data,
                                    const char    *buffer,
                                    const int      buffer_length );
#if (PHP_MAJOR_VERSION >= 7)
static size_t sapi_lsiapi_ub_write(const char *str, size_t str_length TSRMLS_DC);
#else
static int sapi_lsiapi_ub_write(const char *str, unsigned int str_length TSRMLS_DC);
#endif
static char *lsiapi_get_script_file_name( lsi_session_t *session,
                                          mod_lsphp_t *mod_data );
static char *lsiapi_get_content_type( lsi_session_t *session, mod_lsphp_t *mod_data );
static char *lsiapi_get_request_method( lsi_session_t *session, mod_lsphp_t *mod_data );
static char *lsiapi_get_query_string( lsi_session_t *session, mod_lsphp_t *mod_data );
static char *lsiapi_get_script_name( lsi_session_t *session, mod_lsphp_t *mod_data );
static int lsiapi_get_req_body_len( lsi_session_t *session, mod_lsphp_t *mod_data );
static char *lsiapi_get_header_auth( lsi_session_t *session, mod_lsphp_t *mod_data,
                                    char chAuthorization[] );
static int init_request_info( lsi_session_t *session, mod_lsphp_t *mod_data TSRMLS_DC );
#if (PHP_MAJOR_VERSION >= 7)
static zend_stat_t*sapi_lsiapi_get_zend_stat(void);
#else
static struct stat *sapi_lsiapi_get_stat(TSRMLS_D);
#endif
static int lsiapi_execute_script( lsi_session_t *session, mod_lsphp_t *mod_data,
                                 zend_file_handle * file_handle TSRMLS_DC);
static void override_ini(lsi_session_t *session);
static int lsiapi_module_main(lsi_session_t *session, mod_lsphp_t *mod_data TSRMLS_DC);
static int process_req( lsi_session_t *session, mod_lsphp_t *mod_data TSRMLS_DC );
#if PHP_MAJOR_VERSION > 4
/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO(arginfo_litespeed__void, 0)
ZEND_END_ARG_INFO()
/* }}} */
#else
#define arginfo_litespeed__void NULL
#endif
static PHP_RSHUTDOWN_FUNCTION(litespeed);
//static PHP_MINIT_FUNCTION(litespeed);
//static PHP_MSHUTDOWN_FUNCTION(litespeed);
//static int mod_lsphp_free_callback(void *pvData);
static int mod_lsphp_mod_init(lsi_module_t* module);
static int mod_lsphp_begin_process(const lsi_session_t *session);
//static int mod_lsphp_cleanup_process(const lsi_session_t *session);
//static int mod_lsphp_http_end(lsi_param_t *param);
static int mod_lsphp_termination(lsi_param_t *param);

extern zend_module_entry litespeed_module_entry;


#if defined (c_plusplus) || defined (__cplusplus)
}
#endif

#endif // _MOD_LSPHP_H_
