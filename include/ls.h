/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#ifndef LS_MODULE_H
#define LS_MODULE_H

#include <lsr/ls_types.h>

#include <stdio.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <inttypes.h>
#include <stdarg.h>


/**
 * @file ls.h
 */


/**
 * @brief Defines the major version number of LSIAPI.
 */
#define LSIAPI_VERSION_MAJOR    1
/**
 * @brief Defines the minor version number of LSIAPI.
 */
#define LSIAPI_VERSION_MINOR    1
/**
 * @brief Defines the version string of LSIAPI.
 */
#define LSIAPI_VERSION_STRING   "1.1"

/**
 * @def LSI_MAX_HOOK_PRIORITY
 * @brief The max priority level allowed.
 * @since 1.0
 */
#define LSI_MAX_HOOK_PRIORITY     6000

/**
 * @def LSI_MAX_RESP_BUFFER_SIZE
 * @brief The max buffer size for is_resp_buffer_available.
 * @since 1.0
 */
#define LSI_MAX_RESP_BUFFER_SIZE     (1024*1024)

/**
 * @def LSI_MAX_FILE_PATH_LEN
 * @brief The max file path length.
 * @since 1.0
 */
#define LSI_MAX_FILE_PATH_LEN        4096

/**
 * @def LSI_MODULE_CONTAINER_KEY
 * @brief The container key string of all modules' global data.
 * @since 1.0
 */
#define LSI_MODULE_CONTAINER_KEY     "_lsi_modules_container__"

/**
 * @def LSI_MODULE_CONTAINER_KEYLEN
 * @brief The length of the key for the global data container of all modules.
 * @since 1.0
 */
#define LSI_MODULE_CONTAINER_KEYLEN   (sizeof(LSI_MODULE_CONTAINER_KEY) - 1)

/**
 * @def LSI_MODULE_SIGNATURE
 * @brief Identifies the module as a LSIAPI module and the version of LSIAPI that the module was compiled with.
 * @details The signature tells the server core first that it is actually a LSIAPI module,
 * and second, what version of the API that the binary was compiled with in order to check
 * whether it is backwards compatible with future versions.
 * @since 1.0
 */
//#define LSIAPI_MODULE_FLAG    0x4C53494D  //"LSIM"
#define LSI_MODULE_SIGNATURE    (int64_t)0x4C53494D00000000LL + \
    (int64_t)(LSIAPI_VERSION_MAJOR << 16) + \
    (int64_t)(LSIAPI_VERSION_MINOR)


#ifdef __cplusplus
extern "C" {
#endif


/**************************************************************************************************
 *                                       API Parameter ENUMs
 **************************************************************************************************/

/**
 * @enum lsi_config_level
 * @brief The parameter level specified in the callback routine, lsi_config_t::_parse_config,
 * in user configuration parameter parsing.
 * @since 1.0
 */
enum lsi_config_level
{
    /**
     * Server level.
     */
    LSI_SERVER_LEVEL = 0,
    /**
     * Listener level.
     */
    LSI_LISTENER_LEVEL,
    /**
     * Virtual Host level.
     */
    LSI_VHOST_LEVEL,
    /**
     * Context level.
     */
    LSI_CONTEXT_LEVEL,
};

/**
 * @enum lsi_server_mode
 * @brief The web server running mode is defined by how to start the web server.
 * @since 1.0
 */
enum lsi_server_mode
{
    /**
     * Normal server running mode
     */
    LSI_SERVER_MODE_DAEMON = 0,
    /**
     * Server running mode with "-d"
     */
    LSI_SERVER_MODE_FORGROUND,
};


/**
 * @enum lsi_hook_priority
 * @brief The default running priorities for hook filter functions.
 * @details Used when there are more than one filter at the same hook point.
 * Any number from -1 * LSI_MAX_HOOK_PRIORITY to LSI_MAX_HOOK_PRIORITY can also be used.
 * The lower values will have higher priority, so they will run first.
 * Hook functions with the same order value will be called based on the order in which they appear
 * in the configuration list.
 * @since 1.0
 */
enum lsi_hook_priority
{
    /**
     * Hook priority first predefined value.
     */
    LSI_HOOK_FIRST  = -10,
    /**
     * Hook priority early in the processing phase.
     */
    LSI_HOOK_EARLY  =  0,
    /**
     * Hook Priority at the typical level.
     */
    LSI_HOOK_NORMAL =  10,
    /**
     * Hook priority to run later in the processing phase.
     */
    LSI_HOOK_LATE   =  20,
    /**
     * Hook priority last predefined value.
     */
    LSI_HOOK_LAST   =  30,
};


/**
 * @enum lsi_hook_flag
 * @brief Flags applying to hook functions.
 * @details A filter has two modes, TRANSFORMER and OBSERVER.  Flag if it is a transformer.
 * @details LSI_HOOK_FLAG_TRANSFORM is a flag for filter hook points, indicating
 *  that the filter may change the content of the input data.
 *  If a filter does not change the input data, in OBSERVER mode, do not
 *  set this flag.
 *  When no TRANSFORMERs are in a filter chain of a hook point, the server will do
 *  optimizations to avoid deep copy by storing the data in the final buffer.
 *  Its use is important for LSI_HKPT_RECV_REQ_BODY and LSI_HKPT_L4_RECVING filter hooks.
 * @since 1.0
 *
 * @note Hook flags are additive. When required, multiple flags should be set.
 */
enum lsi_hook_flag
{
    /**
     * The filter hook function is a Transformer which modifies the input data.
     * This is for filter type hook points. If no filters are Transformer filters,
     * optimization could be applied by the server core when processing the data.
     */
    LSI_HOOK_FLAG_TRANSFORM  = 1,

    /**
      * The hook function requires decompressed data.
      * This flag is for LSI_HKPT_RECV_RESP_BODY and LSI_HKPT_SEND_RESP_BODY.
      * If any filter requires decompression, the server core will add the decompression filter at the
      * beginning of the filter chain for compressed data.
      */
    LSI_HOOK_FLAG_DECOMPRESS_REQUIRED = 2,

    /**
     * This flag is for LSI_HKPT_SEND_RESP_BODY only,
     * and should be added only if a filter needs to process a static file.
     * If no filter is needed to process a static file, sendfile() API will be used.
     */
    LSI_HOOK_FLAG_PROCESS_STATIC = 4,


    /**
     * This flag enables the hook function.
     * It may be set statically on the global level,
     * or a call to set_session_hook_enable_flag may be used.
     */
    LSI_HOOK_FLAG_ENABLED = 8,


};


/**
 * @enum lsi_module_data_level
 * @brief Determines the scope of the module's user defined data.
 * @details Used by the level API parameter of various functions.
 * Determines what other functions are allowed to access the data and for how long
 * it will be available.
 * @since 1.0
 */
enum lsi_module_data_level
{
    /**
     * User data type for an HTTP session.
     */
    LSI_MODULE_DATA_HTTP = 0,
    /**
     * User data for cached file type.
     */
    LSI_MODULE_DATA_FILE,
    /**
     * User data type for TCP/IP data.
     */
    LSI_MODULE_DATA_IP,
    /**
     * User data type for Virtual Hosts.
     */
    LSI_MODULE_DATA_VHOST,
    /**
     * Module data type for a TCP layer 4 session.
     */
    LSI_MODULE_DATA_L4,

    /**
     * Placeholder.
     */
    LSI_MODULE_DATA_COUNT, /** This is NOT an index */
};


/**
 * @enum lsi_container_type
 * @brief Data container types.
 * @details Used in the API type parameter.
 * Determines whether the data is contained in memory or in a file.
 * @since 1.0
 */
enum lsi_container_type
{
    /**
     * Memory container type for user global data.
     */
    LSI_CONTAINER_MEMORY = 0,
    /**
     * File container type for user global data.
     */
    LSI_CONTAINER_FILE,

    /**
     * Placeholder.
     */
    LSI_CONTAINER_COUNT,
};


/**
 * @enum lsi_hkpt_level
 * @brief Hook Point level definitions.
 * @details Used in the API index parameter.
 * Determines which stage to hook the callback function to.
 * @since 1.0
 * @see lsi_serverhook_s
 */
enum lsi_hkpt_level
{
    /**
     * LSI_HKPT_L4_BEGINSESSION is the point when the session begins a tcp connection.
     * A TCP connection is also called layer 4 connection.
     */
    LSI_HKPT_L4_BEGINSESSION = 0,   //<--- must be first of L4

    /**
     * LSI_HKPT_L4_ENDSESSION is the point when the session ends a tcp connection.
     */
    LSI_HKPT_L4_ENDSESSION,

    /**
     * LSI_HKPT_L4_RECVING is the point when a tcp connection is receiving data.
     */
    LSI_HKPT_L4_RECVING,

    /**
     * LSI_HKPT_L4_SENDING is the point when a tcp connection is sending data.
     */
    LSI_HKPT_L4_SENDING,

    //Http level hook points

    /**
     * LSI_HKPT_HTTP_BEGIN is the point when the session begins an http connection.
     */
    LSI_HKPT_HTTP_BEGIN,       //<--- must be first of Http

    /**
     * LSI_HKPT_RECV_REQ_HEADER is the point when the request header was just received.
     */
    LSI_HKPT_RECV_REQ_HEADER,

    /**
     * LSI_HKPT_URI_MAP is the point when a URI request is mapped to a context.
     */
    LSI_HKPT_URI_MAP,

    /**
     * LSI_HKPT_HTTP_AUTH is the point when authentication check is performed.
     * It is triggered right after HTTP built-in authentication has been performed,
     * such as HTTP BASIC/DIGEST authentication.
     */
    LSI_HKPT_HTTP_AUTH,

    /**
     * LSI_HKPT_RECV_REQ_BODY is the point when request body data is being received.
     */
    LSI_HKPT_RECV_REQ_BODY,

    /**
     * LSI_HKPT_RCVD_REQ_BODY is the point when request body data was received.
     * This function accesses the body data file through a function pointer.
     */
    LSI_HKPT_RCVD_REQ_BODY,

    /**
     * LSI_HKPT_RECV_RESP_HEADER is the point when the response header was created.
     */
    LSI_HKPT_RCVD_RESP_HEADER,

    /**
     * LSI_HKPT_RECV_RESP_BODY is the point when response body is being received
     * by the backend of the web server.
     */
    LSI_HKPT_RECV_RESP_BODY,

    /**
     * LSI_HKPT_RCVD_RESP_BODY is the point when the whole response body was received
     * by the backend of the web server.
     */
    LSI_HKPT_RCVD_RESP_BODY,

    /**
     * LSI_HKPT_HANDLER_RESTART is the point when the server core needs to restart handler processing
     * by discarding the current response, either sending back an error page, or redirecting to a new
     * URL.  It could be triggered by internal redirect, a module deny access; it is only
     * triggered after handler processing starts for the current request.
     */
    LSI_HKPT_HANDLER_RESTART,

    /**
     * LSI_HKPT_SEND_RESP_HEADER is the point when the response header is ready
     * to be sent by the web server.
     */
    LSI_HKPT_SEND_RESP_HEADER,

    /**
     * LSI_HKPT_SEND_RESP_BODY is the point when the response body is being sent
     * by the web server.
     */
    LSI_HKPT_SEND_RESP_BODY,

    /**
     * LSI_HKPT_HTTP_END is the point when a session is ending an http connection.
     */
    LSI_HKPT_HTTP_END,      //<--- must be last of Http

    /**
     * LSI_HKPT_MAIN_INITED is the point when the main (controller) process
     * has completed its initialization and configuration,
     * before servicing any requests.
     * It occurs once upon startup.
     */
    LSI_HKPT_MAIN_INITED,     //<--- must be first of Server

    /**
     * LSI_HKPT_MAIN_PREFORK is the point when the main (controller) process
     * is about to start (fork) a worker process.
     * This occurs for each worker, and may happen during
     * system startup, or if a worker has been restarted.
     */
    LSI_HKPT_MAIN_PREFORK,

    /**
     * LSI_HKPT_MAIN_POSTFORK is the point after the main (controller) process
     * has started (forked) a worker process.
     * This occurs for each worker, and may happen during
     * system startup, or if a worker has been restarted.
     */
    LSI_HKPT_MAIN_POSTFORK,

    /**
     * LSI_HKPT_WORKER_POSTFORK is the point in a worker process
     * after it has been created by the main (controller) process.
     * Note that a corresponding MAIN_POSTFORK hook may
     * occur in the main process either before *or* after this hook.
     */
    LSI_HKPT_WORKER_POSTFORK,

    /**
     * LSI_HKPT_WORKER_ATEXIT is the point in a worker process
     * just before exiting.
     * It is the last hook point of a worker process.
     */
    LSI_HKPT_WORKER_ATEXIT,

    /**
     * LSI_HKPT_MAIN_ATEXIT is the point in the main (controller) process
     * just before exiting.
     * It is the last hook point of the server main process.
     */
    LSI_HKPT_MAIN_ATEXIT,

    LSI_HKPT_TOTAL_COUNT,  /** This is NOT an index */
};


/**
 * @enum lsi_log_level
 * @brief The log level definitions.
 * @details Used in the level API parameter of the write log function.
 * All logs with a log level less than or equal to the server defined level will be
 * written to the log.
 * @since 1.0
 */
enum lsi_log_level
{
    /**
     * Error level output turned on.
     */
    LSI_LOG_ERROR  = 3000,
    /**
     * Warnings level output turned on.
     */
    LSI_LOG_WARN   = 4000,
    /**
     * Notice level output turned on.
     */
    LSI_LOG_NOTICE = 5000,
    /**
     * Info level output turned on.
     */
    LSI_LOG_INFO   = 6000,
    /**
     * Debug level output turned on.
     */
    LSI_LOG_DEBUG  = 7000,
    /**
     * Trace level output turned on.
     */
    LSI_LOG_TRACE  = 8000,
};


/**
 * @enum lsi_hk_result_code
 * @brief LSIAPI return value definition.
 * @details Used in the return value of API functions and callback functions unless otherwise stipulated.
 * If a function returns an int type value, it should always
 * return LSI_HK_RET_OK for no errors and LSI_HK_RET_ERROR for other cases.
 * For such functions that return a bool type (true / false), 1 means true and 0 means false.
 * @since 1.0
 */
enum lsi_hk_result_code
{
    /**
     * Return code for suspend current hookpoint
     */
    LSI_HK_RET_SUSPEND = -3,
    /**
     * Return code to deny access.
     */
    LSI_HK_RET_DENY = -2,
    /**
     * Return code error.
     */
    LSI_HK_RET_ERROR = -1,
    /**
     * Return code success.
     */
    LSI_HK_RET_OK = 0,

};


/**
 * @enum lsi_onwrite_result_code
 * @brief Write response return values.
 * @details Used as on_write_resp return value.
 * Continue should be used until the response sending is completed.
 * Finished will end the process of the server requesting further data.
 * @since 1.0
 */
enum lsi_onwrite_result_code
{
    /**
     * Error in the response processing.
     */
    LSI_WRITE_RESP_ERROR = -1,
    /**
     * No further response data to write.
     */
    LSI_WRITE_RESP_FINISHED = 0,
    /**
     * More response body data to write.
     */
    LSI_WRITE_RESP_CONTINUE,
};


/**
 * @enum lsi_cb_flag
 * @brief definition of flags used in hook function input and ouput parameters
 * @details It defines flags for _flag_in and _flag_out of lsi_cb_param_t.
 * LSI_CB_FLAG_IN_XXXX is for _flag_in, LSI_CB_FLAG_OUT_BUFFERED_DATA is for
 * _flag_out; the flags should be set or removed through a bitwise operator.
 *
 * @since 1.0
 */
enum lsi_cb_flags
{
    /**
     * Indicates that a filter buffered data in its own buffer.
     */
    LSI_CB_FLAG_OUT_BUFFERED_DATA = 1,

    /**
     * This flag requires the filter to flush its internal buffer to the next filter, then
     * pass this flag to the next filter.
     */
    LSI_CB_FLAG_IN_FLUSH = 1,

    /**
     * This flag tells the filter it is the end of stream; there should be no more data
     * feeding into this filter.  The filter should ignore any input data after this flag is
     * set.  This flag implies LSI_CB_FLAG_IN_FLUSH.  A filter should only set this flag after
     * all buffered data has been sent to the next filter.
     */
    LSI_CB_FLAG_IN_EOF = 2,

    /**
     * This flag is set for LSI_HKPT_SEND_RESP_BODY only if the input data is from
     * a static file; if a filter does not need to check a static file, it can skip processing
     * data if this flag is set.
     */
    LSI_CB_FLAG_IN_STATIC_FILE = 4,

    /**
     * This flag is set for LSI_HKPT_RCVD_RESP_BODY only if the request handler does not abort
     * in the middle of processing the response; for example, backend PHP crashes, or HTTP proxy
     * connection to backend has been reset.  If a hook function only needs to process
     * a successful response, it should check for this flag.
     */
    LSI_CB_FLAG_IN_RESP_SUCCEED = 8,

};


/**
 * @enum lsi_req_variable
 * @brief LSIAPI request environment variables.
 * @details Used as API index parameter in env access function get_req_var_by_id.
 * The example reqinfohandler.c shows usage for all of these variables.
 * @since 1.0
 */
enum lsi_req_variable
{
    /**
     * Remote addr environment variable
     */
    LSI_REQ_VAR_REMOTE_ADDR = 0,
    /**
     * Remote port environment variable
     */
    LSI_REQ_VAR_REMOTE_PORT,
    /**
     * Remote host environment variable
     */
    LSI_REQ_VAR_REMOTE_HOST,
    /**
     * Remote user environment variable
     */
    LSI_REQ_VAR_REMOTE_USER,
    /**
     * Remote identifier environment variable
     */
    LSI_REQ_VAR_REMOTE_IDENT,
    /**
     * Remote method environment variable
     */
    LSI_REQ_VAR_REQ_METHOD,
    /**
     * Query string environment variable
     */
    LSI_REQ_VAR_QUERY_STRING,
    /**
     * Authentication type environment variable
     */
    LSI_REQ_VAR_AUTH_TYPE,
    /**
     * URI path environment variable
     */
    LSI_REQ_VAR_PATH_INFO,
    /**
     * Script filename environment variable
     */
    LSI_REQ_VAR_SCRIPTFILENAME,
    /**
     * Filename port environment variable
     */
    LSI_REQ_VAR_REQUST_FN,
    /**
     * URI environment variable
     */
    LSI_REQ_VAR_REQ_URI,
    /**
     * Document root directory environment variable
     */
    LSI_REQ_VAR_DOC_ROOT,
    /**
     * Port environment variable
     */
    LSI_REQ_VAR_SERVER_ADMIN,
    /**
     * Server name environment variable
     */
    LSI_REQ_VAR_SERVER_NAME,
    /**
     * Server address environment variable
     */
    LSI_REQ_VAR_SERVER_ADDR,
    /**
     * Server port environment variable
     */
    LSI_REQ_VAR_SERVER_PORT,
    /**
     * Server prototype environment variable
     */
    LSI_REQ_VAR_SERVER_PROTO,
    /**
     * Server software version environment variable
     */
    LSI_REQ_VAR_SERVER_SOFT,
    /**
     * API version environment variable
     */
    LSI_REQ_VAR_API_VERSION,
    /**
     * Request line environment variable
     */
    LSI_REQ_VAR_REQ_LINE,
    /**
     * Subrequest environment variable
     */
    LSI_REQ_VAR_IS_SUBREQ,
    /**
     * Time environment variable
     */
    LSI_REQ_VAR_TIME,
    /**
     * Year environment variable
     */
    LSI_REQ_VAR_TIME_YEAR,
    /**
     * Month environment variable
     */
    LSI_REQ_VAR_TIME_MON,
    /**
     * Day environment variable
     */
    LSI_REQ_VAR_TIME_DAY,
    /**
     * Hour environment variable
     */
    LSI_REQ_VAR_TIME_HOUR,
    /**
     * Minute environment variable
     */
    LSI_REQ_VAR_TIME_MIN,
    /**
     * Seconds environment variable
     */
    LSI_REQ_VAR_TIME_SEC,
    /**
     * Weekday environment variable
     */
    LSI_REQ_VAR_TIME_WDAY,
    /**
     * Script file name environment variable
     */
    LSI_REQ_VAR_SCRIPT_NAME,
    /**
     * Current URI environment variable
     */
    LSI_REQ_VAR_CUR_URI,
    /**
     * URI base name environment variable
     */
    LSI_REQ_VAR_REQ_BASENAME,
    /**
     * Script user id environment variable
     */
    LSI_REQ_VAR_SCRIPT_UID,
    /**
     * Script global id environment variable
     */
    LSI_REQ_VAR_SCRIPT_GID,
    /**
     * Script user name environment variable
     */
    LSI_REQ_VAR_SCRIPT_USERNAME,
    /**
     * Script group name environment variable
     */
    LSI_REQ_VAR_SCRIPT_GRPNAME,
    /**
     * Script mode environment variable
     */
    LSI_REQ_VAR_SCRIPT_MODE,
    /**
     * Script base name environment variable
     */
    LSI_REQ_VAR_SCRIPT_BASENAME,
    /**
     * Script URI environment variable
     */
    LSI_REQ_VAR_SCRIPT_URI,
    /**
     * Original URI environment variable
     */
    LSI_REQ_VAR_ORG_REQ_URI,
    /**
     * HTTPS environment variable
     */
    LSI_REQ_VAR_HTTPS,
    /**
     * SSL version environment variable
     */
    LSI_REQ_SSL_VERSION,
    /**
     * SSL session ID environment variable
     */
    LSI_REQ_SSL_SESSION_ID,
    /**
     * SSL cipher environment variable
     */
    LSI_REQ_SSL_CIPHER,
    /**
     * SSL cipher use key size environment variable
     */
    LSI_REQ_SSL_CIPHER_USEKEYSIZE,
    /**
     * SSL cipher ALG key size environment variable
     */
    LSI_REQ_SSL_CIPHER_ALGKEYSIZE,
    /**
     * SSL client certification environment variable
     */
    LSI_REQ_SSL_CLIENT_CERT,
    /**
     * Geographical IP address environment variable
     */
    LSI_REQ_GEOIP_ADDR,
    /**
     * Translated path environment variable
     */
    LSI_REQ_PATH_TRANSLATED,
    /**
     * Placeholder.
     */
    LSI_REQ_COUNT,    /** This is NOT an index */
};


/**
 * @enum lsi_header_op
 * @brief The methods used for adding a response header.
 * @details Used in API parameter add_method in response header access functions.
 * If there are no existing headers, any method that is called
 * will have the effect of adding a new header.
 * If there is an existing header, LSI_HEADER_SET will
 * add a header and will replace the existing one.
 * LSI_HEADER_APPEND will append a comma and the header
 * value to the end of the existing value list.
 * LSI_HEADER_MERGE is just like LSI_HEADER_APPEND unless
 * the same value exists in the existing header.
 * In this case, it will do nothing.
 * LSI_HEADER_ADD will add a new line to the header,
 * whether or not it already exists.
 * @since 1.0
 */
enum lsi_header_op
{
    /**
     * Set the header.
     */
    LSI_HEADER_SET = 0,
    /**
     * Add with a comma to seperate
     */
    LSI_HEADER_APPEND,
    /**
     * Append unless it exists
     */
    LSI_HEADER_MERGE,
    /**
     * Add a new line
     */
    LSI_HEADER_ADD
};


/**
 * @enum lsi_url_op
 * @brief The methods used to redirect a request to a new URL.
 * @details LSI_URI_NOCHANGE, LSI_URI_REWRITE and LSI_URL_REDIRECT_* can be combined with
 * LSI_URL_QS_*
 * @since 1.0
 * @see lsi_api_s::set_uri_qs
 */
enum lsi_url_op
{
    /**
     * Do not change URI, intended for modifying Query String only.
     */
    LSI_URI_NOCHANGE = 0,

    /**
     * Rewrite to the new URI and use the URI for subsequent processing stages.
     */
    LSI_URI_REWRITE,

    /**
     * Internal redirect; the redirect is performed internally,
     * as if the server received a new request.
     */
    LSI_URL_REDIRECT_INTERNAL,

    /**
     * External redirect with status code 301 Moved Permanently.
     */
    LSI_URL_REDIRECT_301,

    /**
     * External redirect with status code 302 Found.
     */
    LSI_URL_REDIRECT_302,

    /**
     * External redirect with status code 303 See Other.
     */
    LSI_URL_REDIRECT_303,

    /**
     * External redirect with status code 307 Temporary Redirect.
     */
    LSI_URL_REDIRECT_307,

    /**
     * Do not change Query String. Only valid with LSI_URI_REWRITE.
     */
    LSI_URL_QS_NOCHANGE = 0 << 4,

    /**
     * Append Query String. Can be combined with LSI_URI_REWRITE and LSI_URL_REDIRECT_*.
     */
    LSI_URL_QS_APPEND = 1 << 4,

    /**
     * Set Query String. Can be combined with LSI_URI_REWRITE and LSI_URL_REDIRECT_*.
     */
    LSI_URL_QS_SET = 2 << 4,

    /**
     * Delete Query String. Can be combined with LSI_URI_REWRITE and LSI_URL_REDIRECT_*.
     */
    LSI_URL_QS_DELETE = 3 << 4,

    /**
     * indicates that encoding has been applied to URI.
     */
    LSI_URL_ENCODED = 128
};


/**
 * @enum lsi_req_header_id
 * @brief The most common request-header ids.
 * @details Used as the API header id parameter in request header access functions
 * to access components of the request header.
 * @since 1.0
 */
enum lsi_req_header_id
{
    /**
     * "Accept" request header.
     */
    LSI_REQ_HEADER_ACCEPT = 0,
    /**
     * "Accept-Charset" request header.
     */
    LSI_REQ_HEADER_ACC_CHARSET,
    /**
     * "Accept-Encoding" request header.
     */
    LSI_REQ_HEADER_ACC_ENCODING,
    /**
     * "Accept-Language" request header.
     */
    LSI_REQ_HEADER_ACC_LANG,
    /**
     * "Authorization" request header.
     */
    LSI_REQ_HEADER_AUTHORIZATION,
    /**
     * "Connection" request header.
     */
    LSI_REQ_HEADER_CONNECTION,
    /**
     * "Content-Type" request header.
     */
    LSI_REQ_HEADER_CONTENT_TYPE,
    /**
     * "Content-Length" request header.
     */
    LSI_REQ_HEADER_CONTENT_LENGTH,
    /**
     * "Cookie" request header.
     */
    LSI_REQ_HEADER_COOKIE,
    /**
     * "Cookie2" request header.
     */
    LSI_REQ_HEADER_COOKIE2,
    /**
     * "Host" request header.
     */
    LSI_REQ_HEADER_HOST,
    /**
     * "Pragma" request header.
     */
    LSI_REQ_HEADER_PRAGMA,
    /**
     * "Referer" request header.
     */
    LSI_REQ_HEADER_REFERER,
    /**
     * "User-Agent" request header.
     */
    LSI_REQ_HEADER_USERAGENT,
    /**
     * "Cache-Control" request header.
     */
    LSI_REQ_HEADER_CACHE_CTRL,
    /**
     * "If-Modified-Since" request header.
     */
    LSI_REQ_HEADER_IF_MODIFIED_SINCE,
    /**
     * "If-Match" request header.
     */
    LSI_REQ_HEADER_IF_MATCH,
    /**
     * "If-No-Match" request header.
     */
    LSI_REQ_HEADER_IF_NO_MATCH,
    /**
     * "If-Range" request header.
     */
    LSI_REQ_HEADER_IF_RANGE,
    /**
     * "If-Unmodified-Since" request header.
     */
    LSI_REQ_HEADER_IF_UNMOD_SINCE,
    /**
     * "Keep-Alive" request header.
     */
    LSI_REQ_HEADER_KEEP_ALIVE,
    /**
     * "Range" request header.
     */
    LSI_REQ_HEADER_RANGE,
    /**
     * "X-Forwarded-For" request header.
     */
    LSI_REQ_HEADER_X_FORWARDED_FOR,
    /**
     * "Via" request header.
     */
    LSI_REQ_HEADER_VIA,
    /**
     * "Transfer-Encoding" request header.
     */
    LSI_REQ_HEADER_TRANSFER_ENCODING

};


/**
 * @enum lsi_resp_header_id
 * @brief The most common response-header ids.
 * @details Used as the API header id parameter in response header access functions
 * to access components of the response header.
 * @since 1.0
 */
enum lsi_resp_header_id
{
    /**
     * Accept ranges id
     */
    LSI_RESP_HEADER_ACCEPT_RANGES = 0,
    /**
     * Connection id
     */
    LSI_RESP_HEADER_CONNECTION,
    /**
     * Content type id.
     */
    LSI_RESP_HEADER_CONTENT_TYPE,
    /**
     * Content length id.
     */
    LSI_RESP_HEADER_CONTENT_LENGTH,
    /**
     * Content encoding id.
     */
    LSI_RESP_HEADER_CONTENT_ENCODING,
    /**
     * Content range id.
     */
    LSI_RESP_HEADER_CONTENT_RANGE,
    /**
     * Contnet disposition id.
     */
    LSI_RESP_HEADER_CONTENT_DISPOSITION,
    /**
     * Cache control id.
     */
    LSI_RESP_HEADER_CACHE_CTRL,
    /**
     * Date id.
     */
    LSI_RESP_HEADER_DATE,
    /**
     * E-tag id.
     */
    LSI_RESP_HEADER_ETAG,
    /**
     * Expires id.
     */
    LSI_RESP_HEADER_EXPIRES,
    /**
     * Keep alive message id.
     */
    LSI_RESP_HEADER_KEEP_ALIVE,
    /**
     * Lasst modified date id.
     */
    LSI_RESP_HEADER_LAST_MODIFIED,
    /**
     * Location id.
     */
    LSI_RESP_HEADER_LOCATION,
    /**
     * Litespeed location id.
     */
    LSI_RESP_HEADER_LITESPEED_LOCATION,
    /**
     * Cashe control id.
     */
    LSI_RESP_HEADER_LITESPEED_CACHE_CONTROL,
    /**
     * Pragma id.
     */
    LSI_RESP_HEADER_PRAGMA,
    /**
     * Proxy connection id.
     */
    LSI_RESP_HEADER_PROXY_CONNECTION,
    /**
     * Server id.
     */
    LSI_RESP_HEADER_SERVER,
    /**
     * Set cookie id.
     */
    LSI_RESP_HEADER_SET_COOKIE,
    /**
     * CGI status id.
     */
    LSI_RESP_HEADER_CGI_STATUS,
    /**
     * Transfer encoding id.
     */
    LSI_RESP_HEADER_TRANSFER_ENCODING,
    /**
     * Vary id.
     */
    LSI_RESP_HEADER_VARY,
    /**
     * Authentication id.
     */
    LSI_RESP_HEADER_WWW_AUTHENTICATE,
    /**
     * Powered by id.
     */
    LSI_RESP_HEADER_X_POWERED_BY,
    /**
     * Header end id.
     */
    LSI_RESP_HEADER_END,
    /**
     * Header unknown id.
     */
    LSI_RESP_HEADER_UNKNOWN = LSI_RESP_HEADER_END

};


/*
 * Forward Declarations
 */

typedef struct lsi_module_s lsi_module_t;
typedef struct lsi_api_s lsi_api_t;
typedef struct lsi_cb_param_s lsi_cb_param_t;
typedef struct lsiapi_hookinfo_s  lsi_hook_t;
typedef struct lsi_gdata_cont_s lsi_gdata_container_t;
typedef struct lsi_shm_htable_s lsi_shm_htable_t;
typedef struct ls_shmpool_s  lsi_shmpool_t;
typedef struct ls_shmhash_s  lsi_shmhash_t;
typedef struct lsi_session_s  lsi_session_t;
typedef struct lsi_serverhook_s lsi_serverhook_t;
typedef struct lsi_handler_s  lsi_handler_t;
typedef struct lsi_config_s lsi_config_t;
typedef uint32_t lsi_shm_off_t;
typedef uint32_t lsi_hash_key_t;


/**
 * @typedef lsi_callback_pf
 * @brief The callback function and its parameters.
 * @since 1.0
 */
typedef int (*lsi_callback_pf)(lsi_cb_param_t *);

/**
 * @typedef lsi_release_callback_pf
 * @brief The memory release callback function for the user module data.
 * @since 1.0
 */
typedef int (*lsi_release_callback_pf)(void *);

/**
 * @typedef lsi_serialize_pf
 * @brief The serializer callback function for the user global file data.
 * Must use malloc to get the buffer and return the buffer.
 * @since 1.0
 */
typedef char   *(*lsi_serialize_pf)(void *pObject, int *out_length);

/**
 * @typedef lsi_deserialize_pf
 * @brief The deserializer callback function for the user global file data.
 * Must use malloc to get the buffer and return the buffer.
 * @since 1.0
 */
typedef void   *(*lsi_deserialize_pf)(char *, int length);

/**
 * @typedef lsi_timer_callback_pf
 * @brief The timer callback function for the set timer feature.
 * @since 1.0
 */
typedef void (*lsi_timer_callback_pf)(void *);

/**
 * @typedef lsi_hash_pf
 * @brief The hash callback function generates and returns a hash key.
 * @since 1.0
 */
typedef lsi_hash_key_t (*lsi_hash_pf)(const void *, int len);

/**
 * @typedef lsi_hash_value_comp_pf
 * @brief The hash compare callback function compares two hash keys.
 * @since 1.0
 */
typedef int (*lsi_hash_value_comp_pf)(const void *pVal1, const void *pVal2,
                                      int len);


/**************************************************************************************************
 *                                       API Structures
 **************************************************************************************************/


/**
 * @typedef lsi_cb_param_t
 * @brief Callback parameters passed to the callback functions.
 * @since 1.0
 **/
struct lsi_cb_param_s
{
    /**
     * @brief _session is a pointer to the session.
     * @since 1.0
     */
    lsi_session_t      *_session;

    /**
     * @brief _hook_info is a pointer to the struct lsiapi_hookinfo_t.
     * @since 1.0
     */
    lsi_hook_t         *_hook_info;

    /**
     * @brief _cur_hook is a pointer to the current hook.
     * @since 1.0
     */
    void               *_cur_hook;

    /**
     * @brief _param is a pointer to the first parameter.
     * @details Refer to the LSIAPI Developer's Guide's
     * Callback Parameter Definition table for a table
     * of expected values for each _param based on use.
     * @since 1.0
     */
    const void         *_param;

    /**
     * @brief _param_len is the length of the first parameter.
     * @since 1.0
     */
    int                 _param_len;

    /**
     * @brief _flag_out is a pointer to the second parameter.
     * @since 1.0
     */
    int                *_flag_out;

    /**
     * @brief _flag_in is the length of the second parameter.
     * @since 1.0
     */
    int                 _flag_in;
};


/**
 * @typedef lsi_handler_t
 * @brief Pre-defined handler functions.
 * @since 1.0
 */
struct lsi_handler_s
{
    /**
     * @brief begin_process is called when the server starts to process a request.
     * @details It is used to define the request handler callback function.
     * @since 1.0
     */
    int (*begin_process)(lsi_session_t *pSession);

    /**
     * @brief on_read_req_body is called on a READ event with a large request body.
     * @details on_read_req_body is called when a request has a
     * large request body that was not read completely.
     * If not provided, this function should be set to NULL.
     * The default function will execute and return 0.
     * @since 1.0
     */
    int (*on_read_req_body)(lsi_session_t *pSession);

    /**
     * @brief on_write_resp is called on a WRITE event with a large response body.
     * @details on_write_resp is called when the server gets a large response body
     * where the response did not write completely.
     * If not provided, set it to NULL.
     * The default function will execute and return LSI_WRITE_RESP_FINISHED.
     * @since 1.0
     */
    int (*on_write_resp)(lsi_session_t *pSession);

    /**
     * @brief on_clean_up is called when the server core is done with the handler, asking the
     * handler to perform clean up.
     * @details It is called after a handler called end_resp(), or if the server needs to switch handler;
     * for example, return an error page or perform an internal redirect while the handler is processing the request.
     * @since 1.0
     */
    int (*on_clean_up)(lsi_session_t *pSession);

};


/**
 * @typedef lsi_config_s
 * @brief Contains functions which are used to define parse and release functions for the
 * user defined configuration parameters.
 * @details
 * The parse functions will be called during module load.  The release function will be called on
 * session end.  However, it is recommended that the user always performs a manual release of all
 * allocated memory using the session_end filter hooks.
 * @since 1.0
 */
struct lsi_config_s
{
    /**
     * @brief _parse_config is a callback function for the server to call to parse the user defined
     * parameters and return a pointer to the user defined configuration data structure.
     * @details
     *
     * @since 1.0
     * @param[in] param - the \\0 terminated buffer holding configuration parameters.
     * @param[in] param_len - the total length of the configuration parameters.
     * @param[in] initial_config - a pointer to the default configuration inherited from the parent level.
     * @param[in] level - applicable level from enum #lsi_config_level.
     * @param[in] name - name of the Server/Listener/VHost or URI of the Context.
     * @return a pointer to a the user-defined configuration data, which combines initial_config with
     *         settings in param; if both param and initial_config are NULL, a hard-coded default
     *         configuration value should be returned.
     */
    void  *(*_parse_config)(const char *param, int param_len,
                            void *initial_config, int level, const char *name);

    /**
     * @brief _free_config is a callback function for the server to call to release a pointer to
     * the user defined data.
     * @since 1.0
     * @param[in] config - a pointer to configuration data structure to be released.
     */
    void (*_free_config)(void *config);

    /**
     * @brief _config_keys is a NULL terminated array of const char *.
     * It is used to filter the module user parameters by the server while parsing the configuration.
     * @since 1.0
     */
    const char **_config_keys;
};


/**
 * @typedef lsi_serverhook_s
 * @brief Global hook point specification.
 * @details An array of these entries, terminated by the lsi_serverhook_t_END entry,
 * at lsi_module_t::_serverhook defines the global hook points of a module.
 * @since 1.0
 */
struct lsi_serverhook_s
{
    /**
     * @brief specifies the hook point using level definitions from enum #lsi_hkpt_level.
     * @since 1.0
     */
    int             index;

    /**
     * @brief points to the callback hook function.
     * @since 1.0
     */
    lsi_callback_pf cb;

    /**
     * @brief defines the priority of this hook function within a function chain.
     * @since 1.0
     */
    short           priority;

    /**
     * @brief additive hook point flags using level definitions from enum #lsi_hook_flag.
     * @since 1.0
     */
    short           flag;

};


/**
 * @def lsi_serverhook_t_END
 * @brief Termination entry for the array of lsi_serverhook_t entries
 * at lsi_module_t::_serverhook.
 * @since 1.0
 */
#define lsi_serverhook_t_END    {0, NULL, 0, 0}


#define LSI_MODULE_RESERVED_SIZE    ((3 * sizeof(void *)) \
                                  + ((LSI_HKPT_TOTAL_COUNT + 1) * sizeof(int32_t)) \
                                  + (LSI_MODULE_DATA_COUNT * sizeof(int16_t)))



/**
 * @typedef lsi_module_t
 * @brief Defines an LSIAPI module, this struct must be provided in the module code.
 * @since 1.0
 */
struct lsi_module_s
{
    /**
     * @brief identifies an LSIAPI module. It should be set to LSI_MODULE_SIGNATURE.
     * @since 1.0
     */
    int64_t                  _signature;

    /**
     * @brief a function pointer that will be called after the module is loaded.
     * Used to initialize module data.
     * @since 1.0
     */
    int (*_init)(lsi_module_t *pModule);

    /**
     * @brief _handler needs to be provided if this module is a request handler.
     * If not present, set to NULL.
     * @since 1.0
     */
    lsi_handler_t           *_handler;

    /**
     * @brief contains functions which are used to parse user defined
     * configuration parameters and to release the memory block.
     * If not present, set to NULL.
     * @since 1.0
     */
    lsi_config_t            *_config_parser;

    /**
     * @brief information about this module set by the developer;
     * it can contain module version and/or version(s) of libraries used by this module.
     * If not present, set to NULL.
     * @since 1.0
     */
    const char              *_info;

    /**
     * @brief information for global server level hook functions.
     * If not present, set to NULL.
     * @since 1.0
     */
    lsi_serverhook_t        *_serverhook;

    char                     _reserved[ LSI_MODULE_RESERVED_SIZE ];

};



/**************************************************************************************************
 *                                       API Functions
 **************************************************************************************************/

/**
 * @typedef lsi_api_t
 * @brief LSIAPI function set.
 * @since 1.0
 **/
struct lsi_api_s
{
    /**************************************************************************************************************
     *                                        SINCE LSIAPI 1.0                                                    *
     * ************************************************************************************************************/

    /**
     * @brief get_server_root is used to get the server root path.
     *
     * @since 1.0
     *
     * @return A \\0-terminated string containing the path to the server root directory.
     */
    const char *(*get_server_root)();

    /**
     * @brief log is used to write the formatted log to the error log associated with a session.
     * @details session ID will be added to the log message automatically when session is not NULL.
     * This function will not add a trailing \\n to the end.
     *
     * @since 1.0
     *
     * @param[in] pSession - current session, log file, and session ID are based on session.
     * @param[in] level - enum defined in log level definitions #lsi_log_level.
     * @param[in] fmt - formatted string.
     */
    void (*log)(lsi_session_t *pSession, int level, const char *fmt, ...);

    /**
     * @brief vlog is used to write the formatted log to the error log associated with a session
     * @details session ID will be added to the log message automatically when the session is not NULL.
     *
     * @since 1.0
     *
     * @param[in] pSession - current session, log file and session ID are based on session.
     * @param[in] level - enum defined in log level definitions #lsi_log_level.
     * @param[in] fmt - formatted string.
     * @param[in] vararg - the varying argument list.
     * @param[in] no_linefeed - 1 = do not add \\n at the end of message; 0 = add \\n
     */
    void (*vlog)(lsi_session_t *pSession, int level, const char *fmt,
                 va_list vararg, int no_linefeed);

    /**
     * @brief lograw is used to write additional log messages to the error log file associated with a session.
     * No timestamp, log level, session ID prefix is added; just the data in the buffer is logged into log files.
     * This is usually used for adding more content to previous log messages.
     *
     * @since 1.0
     *
     * @param[in] pSession - log file is based on session; if set to NULL, the default log file is used.
     * @param[in] buf - data to be written to log file.
     * @param[in] len - the size of data to be logged.
     */
    void (*lograw)(lsi_session_t *pSession, const char *buf, int len);

    /**
     * @brief get_module_param is used to get the user defined module parameters which are parsed by
     * the callback _parse_config and pointed to in the struct lsi_config_t.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession, or use NULL for the server level.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @return a pointer to the user-defined configuration data.
     */
    void *(* get_module_param)(lsi_session_t *pSession,
                               const lsi_module_t *pModule);


    /**
     * @brief set_session_hook_enable_flag is used to set the flag of a hook function in a certain level to enable or disable the function.
     * This should only be used after a session is already created
     * and applies only to this session.
     * There are L4 level and HTTP level session hooks.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession, obtained from
     *   callback parameters.
     * @param[in] pModule - a pointer to the lsi_module_t struct.
     * @param[in] enable - a value that the flag is set to, it is 0 to disable,
     *   otherwise is to enable.
     * @param[in] index - A list of indices to set, as defined in the enum of
     *   Hook Point level definitions #lsi_hkpt_level.
     * @param[in] iNumIndices - The number of indices to set in index.
     * @return -1 on failure.
     */
    int (*set_session_hook_enable_flag)(lsi_session_t *session,
                                        const lsi_module_t *pModule,
                                        int enable, int *index,
                                        int iNumIndices);

    /**
     * @brief get_module is used to retrieve module information associated with a hook point based on callback parameters.
     *
     * @since 1.0
     *
     * @param[in] pParam - a pointer to callback parameters.
     * @return NULL on failure, a pointer to the lsi_module_t data structure on success.
     */
    const lsi_module_t *(*get_module)(lsi_cb_param_t *pParam);

    /**
     * @brief init_module_data is used to initialize module data of a certain level(scope).
     * init_module_data must be called before using set_module_data or get_module_data.
     *
     * @since 1.0
     *
     * @param[in] pModule - a pointer to the current module defined in lsi_module_t struct.
     * @param[in] cb - a pointer to the user-defined callback function that releases the user data.
     * @param[in] level - as defined in the module data level enum #lsi_module_data_level.
     * @return -1 for wrong level, -2 for already initialized, 0 for success.
     */
    int (*init_module_data)(lsi_module_t *pModule, lsi_release_callback_pf cb,
                            int level);

    /**
     * @brief Before using FILE type module data, the data must be initialized by
     * calling init_file_type_mdata with the file path.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] path - a pointer to the path string.
     * @param[in] pathLen - the length of the path string.
     * @return -1 on failure, file descriptor on success.
     */
    int (*init_file_type_mdata)(lsi_session_t *pSession,
                                const lsi_module_t *pModule, const char *path, int pathLen);

    /**
     * @brief set_module_data is used to set the module data of a session level(scope).
     * sParam is a pointer to the user's own data structure.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] level - as defined in the module data level enum #lsi_module_data_level.
     * @param[in] sParam - a pointer to the user defined data.
     * @return -1 for bad level or no release data callback function, 0 on success.
     */
    int (*set_module_data)(lsi_session_t *pSession,
                           const lsi_module_t *pModule, int level, void *sParam);

    /**
     * @brief get_module_data gets the module data which was set by set_module_data.
     * The return value is a pointer to the user's own data structure.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] level - as defined in the module data level enum #lsi_module_data_level.
     * @return NULL on failure, a pointer to the user defined data on success.
     */
    void *(*get_module_data)(lsi_session_t *pSession,
                             const lsi_module_t *pModule, int level);

    /**
    * @brief get_cb_module_data gets the module data related to the current callback.
    * The return value is a pointer to the user's own data structure.
    *
    * @since 1.0
    *
    * @param[in] pParam - a pointer to callback parameters.
    * @param[in] level - as defined in the module data level enum #lsi_module_data_level.
    * @return NULL on failure, a pointer to the user defined data on success.
    */
    void *(*get_cb_module_data)(const lsi_cb_param_t *pParam, int level);

    /**
     * @brief free_module_data is to be called when the user needs to free the module data immediately.
     * It is not used by the web server to call the release callback later.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] level - as defined in the module data level enum #lsi_module_data_level.
     * @param[in] cb - a pointer to the user-defined callback function that releases the user data.
     */
    void (*free_module_data)(lsi_session_t *pSession,
                             const lsi_module_t *pModule, int level, lsi_release_callback_pf cb);

    /**
     * @brief stream_writev_next needs to be called in the LSI_HKPT_L4_SENDING hook point level just
     * after it finishes the action and needs to call the next step.
     *
     * @since 1.0
     *
     * @param[in] pParam - the callback parameters to be sent to the current hook callback function.
     * @param[in] iov - the IO vector of data to be written.
     * @param[in] count - the size of the IO vector.
     * @return the return value from the hook filter callback function.
     */
    int (*stream_writev_next)(lsi_cb_param_t *pParam, struct iovec *iov,
                              int count);

    /**
     * @brief stream_read_next needs to be called in filter
     * callback functions registered to the LSI_HKPT_L4_RECVING and LSI_HKPT_RECV_REQ_BODY hook point level
     * to get data from a higher level filter in the chain.
     *
     * @since 1.0
     *
     * @param[in] pParam - the callback parameters to be sent to the current hook callback function.
     * @param[in,out] pBuf - a pointer to a buffer provided to hold the read data.
     * @param[in] size - the buffer size.
     * @return the return value from the hook filter callback function.
     */
    int (*stream_read_next)(lsi_cb_param_t *pParam, char *pBuf, int size);

    /**
     * @brief stream_write_next is used to write the response body to the next function
     * in the filter chain of LSI_HKPT_SEND_RESP_BODY level.
     * This must be called in order to send the response body to the next filter.
     * It returns the size written.
     *
     * @since 1.0
     *
     * @param[in] pParam - a pointer to the callback parameters.
     * @param[in] buf - a pointer to a buffer to be written.
     * @param[in] buflen - the size of the buffer.
     * @return -1 on failure, return value of the hook filter callback function.
     */
    int (*stream_write_next)(lsi_cb_param_t *pParam, const char *buf, int len);

    /**
     * @brief get_gdata_container is used to get the global data container which is determined by the given key.
     * It will retrieve it if it exists, otherwise it will create a new one.
     * There are two types of containers: memory type and file type.
     * Memory type containers will be lost when the web server is restarted.
     * The file type container will be retained after restart.
     *
     * @since 1.0
     *
     * @param[in] type - enum of Data container types.
     * @param[in] key - a pointer to a container ID string.
     * @param[in] key_len - the string length.
     * @return NULL on failure, a pointer to the data container on success.
     */
    lsi_gdata_container_t *(*get_gdata_container)(int type, const char *key,
            int key_len);

    /**
     * @brief empty_gdata_container is used to delete all the data indices in the global data container.
     * It won't delete the physical files in the directory of the container.
     *
     * @since 1.0
     *
     * @param[in] pContainer - the global data container.
     * @return -1 on failure, 0 on success.
     */
    int (*empty_gdata_container)(lsi_gdata_container_t *pContainer);

    /**
     * @brief purge_gdata_container is used to delete the physical files of the empty container.
     *
     * @since 1.0
     *
     * @param[in] pContainer - the global data container.
     * @return -1 on failure, 0 on success.
     */
    int (*purge_gdata_container)(lsi_gdata_container_t *pContainer);

    /**
     * @brief get_gdata is used to get the global data which was already set to the container.
     * deserialize_cb needs to be provided in case the data index does not exist but the file
     * is cached.  In this case, the data can be deserialized from the file.
     * It will be released with the release_cb callback when the data is deleted.
     *
     * @since 1.0
     *
     * @param[in] pContainer - the global data container.
     * @param[in] key - a pointer to a container ID string.
     * @param[in] key_len - the string length.
     * @param[in] cb - a pointer to the user-defined callback function that releases the global data.
     * @param[in] renew_TTL - time to live.
     * @param[in] deserialize_cb - a pointer to the user-defined deserializer callback function used for file read.
     * @return a pointer to the user-defined global data on success,
     * NULL on failure.
     */
    void *(*get_gdata)(lsi_gdata_container_t *pContainer, const char *key,
                       int key_len,
                       lsi_release_callback_pf release_cb, int renew_TTL,
                       lsi_deserialize_pf deserialize_cb);

    /**
     * @brief delete_gdata is used to delete the global data which was already set to the container.
     *
     * @since 1.0
     *
     * @param[in] pContainer - the global data container.
     * @param[in] key - a pointer to a container ID string.
     * @param[in] key_len - the string length.
     * @return 0.
     */
    int (*delete_gdata)(lsi_gdata_container_t *pContainer, const char *key,
                        int key_len);

    /**
     * @brief set_gdata is used to set the global data (void *val) with a given key.
     * A TTL(time to live) needs to be set.
     * lsi_release_callback_pf needs to be provided for use when the data is to be deleted.
     * If the container is a FILE type, a lsi_serialize_pf will be called to get the buffer
     * which will be written to file.
     * If the data exists and force_update is 0, this function will not update the data.
     *
     * @since 1.0
     *
     * @param[in] pContainer - the global data container.
     * @param[in] key - a pointer to a container ID string.
     * @param[in] key_len - the string length.
     * @param[in] val - a pointer to the user-defined data.
     * @param[in] TTL - time to live.
     * @param[in] cb - a pointer to the user-defined callback function that releases the global data.
     * @param[in] force_update - the flag to force existing data to be overwritten.  0 to not force
     * an update, any number not equal to 0 to force update.
     * @param[in] serialize_cb - a pointer to the user-defined serializer callback function used for file write.
     * @return -1 on failure, 1 on not forced, 0 on success.
     */
    int (*set_gdata)(lsi_gdata_container_t *pContainer, const char *key,
                     int key_len, void *val, int TTL,
                     lsi_release_callback_pf release_cb, int force_update,
                     lsi_serialize_pf serialize_cb);

    /**
     * @brief get_req_raw_headers_length can be used to get the length of the total request headers.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return length of the request header.
     */
    int (*get_req_raw_headers_length)(lsi_session_t *pSession);

    /**
     * @brief get_req_raw_headers can be used to store all of the request headers in a given buffer.
     * If maxlen is too small to contain all the data,
     * it will only copy the amount of the maxlen.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in,out] buf - a pointer to a buffer provided to hold the header strings.
     * @param[in] maxlen - the size of the buffer.
     * @return - the length of the request header.
     */
    int (*get_req_raw_headers)(lsi_session_t *pSession, char *buf, int maxlen);

    /**
     * @brief get_req_headers_count can be used to get the count of the request headers.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return the number of headers in the whole request header.
     */
    int (*get_req_headers_count)(lsi_session_t *pSession);

    /**
     * @brief get_req_headers can be used to get all the request headers.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[out] iov_key - the IO vector that contains the keys.
     * @param[out] iov_val - the IO vector that contains the values.
     * @param[in] maxIovCount - size of the IO vectors.
     * @return the count of headers in the IO vectors.
     */
    int (*get_req_headers)(lsi_session_t *pSession, struct iovec *iov_key,
                           struct iovec *iov_val, int maxIovCount);

    /**
     * @brief get_req_header can be used to get a request header based on the given key.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] key - a pointer to a string describing the header label, (key).
     * @param[in] keylen - the size of the string.
     * @param[in,out] valLen - a pointer to the length of the returned string.
     * @return a pointer to the request header key value string.
     */
    const char *(*get_req_header)(lsi_session_t *pSession, const char *key,
                                  int keyLen, int *valLen);

    /**
     * @brief get_req_header_by_id can be used to get a request header based on the given header id
     * defined in lsi_req_header_id
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] id - header id defined in lsi_req_header_id.
     * @param[in,out] valLen - a pointer to the length of the returned string.
     * @return a pointer to the request header key value string.
     */
    const char *(*get_req_header_by_id)(lsi_session_t *pSession, int id,
                                        int *valLen);

    /**
     * @brief get_req_org_uri is used to get the original URI as delivered in the request,
     * before any processing has taken place.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in,out] buf - a pointer to the buffer for the URI string.
     * @param[in] buf_size - max size of the buffer.
     *
     * @return length of the URI string.
     */
    int (*get_req_org_uri)(lsi_session_t *pSession, char *buf, int buf_size);

    /**
     * @brief get_req_uri can be used to get the URI of a HTTP session.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in,out] uri_len - a pointer to int; if not NULL, the length of the URI is returned.
     * @return pointer to the URI string. The string is readonly.
     */
    const char *(*get_req_uri)(lsi_session_t *pSession, int *uri_len);

    /**
     * @brief get_mapped_context_uri can be used to get the context URI of an HTTP session.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in,out] length - the length of the returned string.
     * @return pointer to the context URI string.
     */
    const char *(*get_mapped_context_uri)(lsi_session_t *pSession,
                                          int *length);

    /**
     * @brief register_req_handler can be used to dynamically register a handler.
     * The scriptLen is the length of the script.  To call this function,
     * the module needs to provide the lsi_handler_t (not set to NULL).
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] scriptLen - the length of the script name string.
     * @return -1 on failure, 0 on success.
     */
    int (*register_req_handler)(lsi_session_t *pSession, lsi_module_t *pModule,
                                int scriptLen);

    /**
     * @brief set_handler_write_state can change the calling of on_write_resp() of a module handler.
     * If the state is 0, the calling will be suspended;
     * otherwise, if it is 1, it will continue to call when not finished.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] state - 0 to suspend the calling; 1 to continue.
     * @return
     *
     */
    int (*set_handler_write_state)(lsi_session_t *pSession, int state);

    /**
     * @brief set_timer sets a timer.
     *
     * @since 1.0
     *
     * @param[in] timeout_ms - Timeout in ms.
     * @param[in] repeat - 1 to repeat, 0 for no repeat.
     * @param[in] timer_cb - Callback function to be called on timeout.
     * @param[in] timer_cb_param - Optional parameter for the
     *      callback function.
     * @return Timer ID if successful, LS_FAIL if it failed.
     *
     */
    int (*set_timer)(unsigned int timeout_ms, int repeat,
                     lsi_timer_callback_pf timer_cb, void *timer_cb_param);

    /**
     * @brief remove_timer removes a timer.
     *
     * @since 1.0
     *
     * @param[in] time_id - timer id
     * @return 0.
     *
     */
    int (*remove_timer)(int time_id);

    /**
     * return notifier pointer
     */
    void *(*set_event_notifier)(lsi_session_t *pSession, lsi_module_t *pModule,
                                int level);
    /**
     * return 0 for no error, otherwise an error code is returned
     */
    int (*notify_event_notifier)(void **event_notifier_pointer);
    void (*remove_event_notifier)(void **event_notifier_pointer);







    /**
     * get_req_cookies is used to get all the request cookies.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in,out] len - a pointer to the length of the cookies string.
     * @return a pointer to the cookie key string.
     */
    const char *(*get_req_cookies)(lsi_session_t *pSession, int *len);

    /**
     * @brief get_req_cookie_count is used to get the request cookies count.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return the number of cookies.
     */
    int (*get_req_cookie_count)(lsi_session_t *pSession);

    /**
     * @brief get_cookie_value is to get a cookie based on the cookie name.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] cookie_name - a pointer to the cookie name string.
     * @param[in] nameLen - the length of the cookie name string.
     * @param[in,out] valLen - a pointer to the length of the cookie string.
     * @return a pointer to the cookie string.
     */
    const char *(*get_cookie_value)(lsi_session_t *pSession,
                                    const char *cookie_name, int nameLen, int *valLen);

    /**
     * @brief get_client_ip is used to get the request ip address.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in,out] len - a pointer to the length of the IP string.
     * @return a pointer to the IP string.
     */
    const char *(*get_client_ip)(lsi_session_t *pSession, int *len);

    /**
     * @brief get_req_query_string is used to get the request query string.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in,out] len - a pointer to the length of the query string.
     * @return a pointer to the query string.
     */
    const char *(*get_req_query_string)(lsi_session_t *pSession, int *len);

    /**
     * @brief get_req_var_by_id is used to get the value of a server variable and
     * environment variable by the env type. The caller is responsible for managing the
     * buffer holding the value returned.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] id - enum #lsi_req_variable defined as LSIAPI request variable ID.
     * @param[in,out] val - a pointer to the allocated buffer holding value string.
     * @param[in] maxValLen - the maximum size of the variable value string.
     * @return the length of the variable value string.
     */
    int (*get_req_var_by_id)(lsi_session_t *pSession, int id, char *val,
                             int maxValLen);

    /**
     * @brief get_req_env is used to get the value of a server variable and
     * environment variable based on the name.  It will also get the env that is set by set_req_env.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] name - a pointer to the variable name string.
     * @param[in] nameLen - the length of the variable name string.
     * @param[in,out] val - a pointer to the variable value string.
     * @param[in] maxValLen - the maximum size of the variable value string.
     * @return the length of the variable value string.
     */
    int (*get_req_env)(lsi_session_t *pSession, const char *name,
                       unsigned int nameLen, char *val, int maxValLen);

    /**
     * @brief set_req_env is used to set a request environment variable.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] name - a pointer to the variable name string.
     * @param[in] nameLen - the length of the variable name string.
     * @param[in] val - a pointer to the variable value string.
     * @param[in] valLen - the size of the variable value string.
     */
    void (*set_req_env)(lsi_session_t *pSession, const char *name,
                        unsigned int nameLen, const char *val, int valLen);

    /**
     * @brief register_env_handler is used to register a callback with a set_req_env defined by an env_name,
     * so that when such an env is set by set_req_env or rewrite rule,
     * the registered callback will be called.
     *
     * @since 1.0
     *
     * @param[in] env_name - the string containing the environment variable name.
     * @param[in] env_name_len - the length of the string.
     * @param[in] cb - a pointer to the user-defined callback function associated with the environment variable.
     * @return Not used.
     */
    int (*register_env_handler)(const char *env_name,
                                unsigned int env_name_len, lsi_callback_pf cb);

    /**
     * @brief get_uri_file_path will get the real file path mapped to the request URI.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] uri - a pointer to the URI string.
     * @param[in] uri_len - the length of the URI string.
     * @param[in,out] path - a pointer to the path string.
     * @param[in] max_len - the max length of the path string.
     * @return the length of the path string.
     */
    int (*get_uri_file_path)(lsi_session_t *pSession, const char *uri,
                             int uri_len, char *path, int max_len);

    /**
     * @brief set_uri_qs changes the URI and Query String of the current session; perform internal/external redirect.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] action - action to be taken to URI and Query String, defined by #lsi_url_op:
     * - LSI_URI_NOCHANGE - do not change the URI.
     * - LSI_URI_REWRITE - rewrite a new URI and use for processing.
     * - LSI_URL_REDIRECT_INTERNAL - internal redirect, as if the server received a new request.
     * - LSI_URL_REDIRECT_{301,302,303,307} - external redirect.
     * .
     * combined with one of the Query String qualifiers:
     * - LSI_URL_QS_NOCHANGE - do not change the Query String (LSI_URI_REWRITE only).
     * - LSI_URL_QS_APPEND - append to the Query String.
     * - LSI_URL_QS_SET - set the Query String.
     * - LSI_URL_QS_DELETE - delete the Query String.
     * .
     * and optionally LSI_URL_ENCODED if encoding has been applied to the URI.
     * @param[in] uri - a pointer to the URI string.
     * @param[in] len - the length of the URI string.
     * @param[in] qs -  a pointer to the Query String.
     * @param[in] qs_len - the length of the Query String.
     * @return -1 on failure, 0 on success.
     * @note LSI_URL_QS_NOCHANGE is only valid with LSI_URI_REWRITE, in which case qs and qs_len MUST be NULL.
     * In all other cases, a NULL specified Query String has the effect of deleting the resultant Query String completely.
     * In all cases of redirection, if the Query String is part of the target URL, qs and qs_len must be specified,
     * since the original Query String is NOT carried over.
     *
     * \b Example of external redirection, changing the URI and setting a new Query String:
     * @code
     *
       static int handlerBeginProcess( lsi_session_t *pSession )
       {
          ...
          g_api->set_uri_qs( pSession,
            LSI_URL_REDIRECT_307|LSI_URL_QS_SET, "/new_location", 13, "ABC", 3 );
          ...
       }
     * @endcode
     * would result in a response header similar to:
     * @code
       ...
       HTTP/1.1 307 Temporary Redirect
       ...
       Server: LiteSpeed
       Location: http://localhost:8088/new_location?ABC
       ...
       @endcode
       \n
     */
    int (*set_uri_qs)(lsi_session_t *pSession, int action, const char *uri,
                      int uri_len, const char *qs, int qs_len);

    /**
     * @brief get_req_content_length is used to get the content length of the request.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return content length.
     */
    int (*get_req_content_length)(lsi_session_t *pSession);

    /**
     * @brief read_req_body is used to get the request body to a given buffer.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in,out] buf - a pointer to a buffer provided to hold the header strings.
     * @param[in] buflen - size of the buffer.
     * @return length of the request body.
     */
    int (*read_req_body)(lsi_session_t *pSession, char *buf, int bufLen);

    /**
     * @brief is_req_body_finished is used to ensure that all the request body data
     * has been accounted for.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return 0 false, 1 true.
     */
    int (*is_req_body_finished)(lsi_session_t *pSession);

    /**
     * @brief set_req_wait_full_body is used to make the server wait to call
     * begin_process until after the full request body is received.
     * @details If the user calls this function within begin_process, the
     * server will call on_read_req_body only after the full request body is received.
     * Please refer to the
     * LiteSpeed Module Developer's Guide for a more in-depth explanation of the
     * purpose of this function.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return
     */
    int (*set_req_wait_full_body)(lsi_session_t *pSession);

    /**
     * @brief set_resp_wait_full_body is used to make the server wait for
     * the whole response body before starting to send response back to the client.
     * @details If this function is called before the server sends back any response
     * data, the server will wait for the whole response body. If it is called after the server
     * begins sending back response data, the server will stop sending more data to the client
     * until the whole response body has been received.
     * Please refer to the
     * LiteSpeed Module Developer's Guide for a more in-depth explanation of the
     * purpose of this function.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return
     */
    int (*set_resp_wait_full_body)(lsi_session_t *pSession);

    /**
     * @brief set_status_code is used to set the response status code of an HTTP session.
     * It can be used in hook point and handler processing functions,
     * but MUST be called before the response header has been sent.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] code - the http response status code.
     */
    void (*set_status_code)(lsi_session_t *pSession, int code);

    /**
     * @brief get_status_code is used to get the response status code of an HTTP session.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return the http response status code.
     */
    int (*get_status_code)(lsi_session_t *pSession);

    /**
     * @brief is_resp_buffer_available is used to check if the response buffer
     * is available to fill in data.  This function should be called before
     * append_resp_body or append_resp_bodyv is called.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return -1 on failure, 0 false, 1 true.
     */
    int (*is_resp_buffer_available)(lsi_session_t *pSession);

    /**
     * @brief is_resp_buffer_gzipped is used to check if the response buffer is gzipped (compressed).
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return 0 false, 1 true.
     */
    int (*is_resp_buffer_gzippped)(lsi_session_t *pSession);

    /**
     * @brief set_resp_buffer_gzip_flag is used to set the response buffer gzip flag.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] set - gzip flag; set 1 if gzipped (compressed), 0 to clear the flag.
     * @return 0 if success, -1 if failure.
     */
    int (*set_resp_buffer_gzip_flag)(lsi_session_t *pSession, int set);

    /**
     * @brief append_resp_body is used to append a buffer to the response body.
     * It can ONLY be used by handler functions.
     * The data will go through filters for post processing.
     * This function should NEVER be called from a filter post processing the data.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] buf - a pointer to a buffer to be written.
     * @param[in] buflen - the size of the buffer.
     * @return the length of the request body.
     */
    int (*append_resp_body)(lsi_session_t *pSession, const char *buf, int len);

    /**
     * @brief append_resp_bodyv is used to append an iovec to the response body.
     * It can ONLY be used by handler functions.
     * The data will go through filters for post processing.
     * this function should NEVER be called from a filter post processing the data.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] iov - the IO vector of data to be written.
     * @param[in] count - the size of the IO vector.
     * @return -1 on failure, return value of the hook filter callback function.
     */
    int (*append_resp_bodyv)(lsi_session_t *pSession, const struct iovec *iov,
                             int count);

    /**
     * @brief send_file is used to send a file as the response body.
     * It can be used in handler processing functions.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] path - a pointer to the path string.
     * @param[in] start - file start offset.
     * @param[in] size - remaining size.
     * @return -1 or error codes from various layers of calls on failure, 0 on success.
     */
    int (*send_file)(lsi_session_t *pSession, const char *path, int64_t start,
                     int64_t size);

    /**
     * @brief flush flushes the connection and sends the data.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     */
    void (*flush)(lsi_session_t *pSession);

    /**
     * @brief end_resp is called when the response sending is complete.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     */
    void (*end_resp)(lsi_session_t *pSession);

    /**
     * @brief set_resp_content_length sets the Content Length of the response.
     * If len is -1, the response will be set to chunk encoding.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] len - the content length.
     * @return 0.
     */
    int (*set_resp_content_length)(lsi_session_t *pSession, int64_t len);

    /**
     * @brief set_resp_header is used to set a response header.
     * If the header does not exist, it will add a new header.
     * If the header exists, it will add the header based on
     * the add_method - replace, append, merge and add new line.
     * It can be used in LSI_HKPT_RECV_RESP_HEADER and handler processing functions.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] header_id - enum #lsi_resp_header_id defined as response-header id.
     * @param[in] name - a pointer to the header id name string.
     * @param[in] nameLen - the length of the header id name string.
     * @param[in] val - a pointer to the header string to be set.
     * @param[in] valLen - the length of the header value string.
     * @param[in] add_method - enum #lsi_header_op defined for the method of adding.
     * @return 0.
     */
    int (*set_resp_header)(lsi_session_t *pSession, unsigned int header_id,
                           const char *name, int nameLen, const char *val, int valLen,
                           int add_method);

    /**
     * @brief set_resp_header2 is used to parse the headers first then perform set_resp_header.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] headers - a pointer to the header string to be set.
     * @param[in] len - the length of the header value string.
     * @param[in] add_method - enum #lsi_header_op defined for the method of adding.
     * @return 0.
     */
    int (*set_resp_header2)(lsi_session_t *pSession, const char *headers,
                            int len, int add_method);

    /**
     * @brief set_resp_cookies is used to set response cookies.
     * @details The name, value, and domain are required.  If they do not exist,
     * the function will fail.
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] pName - a pointer to a cookie name.
     * @param[in] pValue - a pointer to a cookie value.
     * @param[in] path - a pointer to the path of the requested object.
     * @param[in] domain - a pointer to the domain of the requested object.
     * @param[in] expires - an expiration date for when to delete the cookie.
     * @param[in] secure - a flag that determines if communication should be
     * encrypted.
     * @param[in] httponly - a flag that determines if the cookie should be
     * limited to HTTP and HTTPS requests.
     * @return 0 if successful, else -1 if there was an error.
     */
    int (*set_resp_cookies)(lsi_session_t *pSession, const char *pName,
                            const char *pVal,
                            const char *path, const char *domain, int expires,
                            int secure, int httponly);

    /**
     * @brief get_resp_header is used to get a response header's value in an iovec array.
     * It will try to use header_id to search the header first.
     * If header_id is not LSI_RESP_HEADER_UNKNOWN, the name and nameLen will NOT be checked,
     * and they can be set to NULL and 0.
     * Otherwise, if header_id is LSI_RESP_HEADER_UNKNOWN, then it will search through name and nameLen.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] header_id - enum #lsi_resp_header_id defined as response-header id.
     * @param[in] name - a pointer to the header id name string.
     * @param[in] nameLen - the length of the header id name string.
     * @param[out] iov - the IO vector that contains the headers.
     * @param[in] maxIovCount - size of the IO vector.
     * @return the count of headers in the IO vector.
     */
    int (*get_resp_header)(lsi_session_t *pSession, unsigned int header_id,
                           const char *name, int nameLen, struct iovec *iov, int maxIovCount);

    /**
     * @brief get_resp_headers_count is used to get the count of the response headers.
     * Multiple line headers will be counted as different headers.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return the number of headers in the whole response header.
     */
    int (*get_resp_headers_count)(lsi_session_t *pSession);

    unsigned int (*get_resp_header_id)(lsi_session_t *pSession,
                                       const char *name);

    /**
     * @brief get_resp_headers is used to get the whole response headers and store them to the iovec array.
     * If maxIovCount is smaller than the count of the headers,
     * it will only store the first maxIovCount headers.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] iov - the IO vector that contains the headers.
     * @param[in] maxIovCount - size of the IO vector.
     * @return the count of all the headers.
     */
    int (*get_resp_headers)(lsi_session_t *pSession, struct iovec *iov_key,
                            struct iovec *iov_val, int maxIovCount);

    /**
     * @brief remove_resp_header is used to remove a response header.
     * The header_id should match the name and nameLen if it isn't -1.
     * Providing the header_id will make finding of the header quicker.  It can be used in
     * LSI_HKPT_RECV_RESP_HEADER and handler processing functions.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] header_id - enum #lsi_resp_header_id defined as response-header id.
     * @param[in] name - a pointer to the header id name string.
     * @param[in] nameLen - the length of the header id name string.
     * @return 0.
     */
    int (*remove_resp_header)(lsi_session_t *pSession, unsigned int header_id,
                              const char *name, int nameLen);

    /**
     * @brief get_file_path_by_uri is used to get the corresponding file path based on request URI.
     * @details The difference between this function with simply append URI to document root is that this function
     * takes context configuration into consideration. If a context points to a directory outside the document root,
     * this function can return the correct file path based on the context path.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] uri - the URI.
     * @param[in] uri_len - size of the URI.
     * @param[in,out] path - the buffer holding the result path.
     * @param[in] max_len - size of the path buffer.
     *
     * @return if success, return the size of path, if error, return -1.
     */
    int (*get_file_path_by_uri)(lsi_session_t *pSession, const char *uri,
                                int uri_len, char *path, int max_len);

    /**
     * @brief get_mime_type_by_suffix is used to get corresponding MIME type based on file suffix.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] suffix - the file suffix, without the leading dot.
     *
     * @return the readonly MIME type string.
     */
    const char *(*get_mime_type_by_suffix)(lsi_session_t *pSession,
                                           const char *suffix);

    /**
     * @brief set_force_mime_type is used to force the server core to use a MIME type with request in current session.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] mime - the MIME type string.
     *
     * @return 0 if success, -1 if failure.
     */
    int (*set_force_mime_type)(lsi_session_t *pSession, const char *mime);

    /**
     * @brief get_req_file_path is used to get the static file path associated with request in current session.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in,out] pathLen - the length of the path.
     *
     * @return the file path string if success, NULL if no static file associated.
     */
    const char *(*get_req_file_path)(lsi_session_t *pSession, int *pathLen);

    /**
     * @brief get_req_handler_type is used to get the type name of a handler assigned to this request.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     *
     * @return type name string, if no handler assigned, return NULL.
     */
    const char *(*get_req_handler_type)(lsi_session_t *pSession);

    /**
     * @brief is_access_log_on returns if access logging is enabled for this session.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     *
     * @return 1 if access logging is enabled, 0 if access logging is disabled.
     */
    int (*is_access_log_on)(lsi_session_t *pSession);

    /**
     * @brief set_access_log turns access logging on or off.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] on_off - set to 1 to turn on access logging, set to 0 to turn off access logging.
     */
    void (*set_access_log)(lsi_session_t *pSession, int on_off);

    /**
     * @brief get_access_log_string returns a string for access log based on the log_pattern.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] log_pattern - the log pattern to be used; for details, please refer to Apache's custom log format.
     * @param[in,out] buf - the buffer holding the log string.
     * @param[in] bufLen - the length of buf.
     *
     * @return the length of the final log string, -1 if error.
     */
    int (*get_access_log_string)(lsi_session_t *pSession,
                                 const char *log_pattern, char *buf, int bufLen);

    /**
     * @brief get_file_stat is used to get the status of a file.
     * @details The routine uses the system call stat(2).
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] path - the file path name.
     * @param[in] pathLen - the length of the path name.
     * @param[out] st - the structure to hold the returned file status.
     *
     * @return -1 on failure, 0 on success.
     */
    int (*get_file_stat)(lsi_session_t *pSession, const char *path,
                         int pathLen, struct stat *st);

    /**
     * @brief is_resp_handler_aborted is used to check if the handler has been aborted.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return -1 if session does not exist, else 0.
     */
    int (*is_resp_handler_aborted)(lsi_session_t *pSession);

    /**
     * @brief get_resp_body_buf returns a buffer that holds response body of current session.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     *
     * @return the pointer to the response body buffer, NULL if response body is not available.
     */
    void *(*get_resp_body_buf)(lsi_session_t *pSession);

    /**
     * @brief get_req_body_buf returns a buffer that holds request body of current session.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     *
     * @return the pointer to the request body buffer, NULL if request body is not available.
     */
    void *(*get_req_body_buf)(lsi_session_t *pSession);

    void *(*get_new_body_buf)(int64_t iInitialSize);

    /**
     * @brief get_body_buf_size returns the size of the specified body buffer.
     *
     * @since 1.0
     *
     * @param[in] pBuf - the body buffer.
     * @return the size of the body buffer, else -1 on error.
     */
    int64_t (*get_body_buf_size)(void *pBuf);

    /**
     * @brief is_body_buf_eof is used determine if the specified \e offset
     *   is the body buffer end of file.
     *
     * @since 1.0
     *
     * @param[in] pBuf - the body buffer.
     * @param[in] offset - the offset in the body buffer.
     * @return 0 false, 1 true; if \e pBuf is NULL, 1 is returned.
     */
    int (*is_body_buf_eof)(void *pBuf, int64_t offset);

    /**
     * @brief acquire_body_buf_block
     *  is used to acquire (retrieve) a portion of the body buffer.
     *
     * @since 1.0
     *
     * @param[in] pBuf - the body buffer.
     * @param[in] offset - the offset in the body buffer.
     * @param[out] size - a pointer which upon return contains the number of bytes acquired.
     * @return a pointer to the body buffer block, else NULL on error.
     *
     * @note If the \e offset is past the end of file,
     *  \e size contains zero and a null string is returned.
     */
    const char *(*acquire_body_buf_block)(void *pBuf, int64_t offset,
                                          int *size);

    /**
     * @brief release_body_buf_block
     *  is used to release a portion of the body buffer back to the system.
     * @details The entire block containing \e offset is released.
     *  If the current buffer read/write pointer is within this block,
     *  the block is \e not released.
     *
     * @since 1.0
     *
     * @param[in] pBuf - the body buffer.
     * @param[in] offset - the offset in the body buffer.
     * @return void.
     */
    void (*release_body_buf_block)(void *pBuf, int64_t offset);

    /**
     * @brief reset_body_buf
     *  resets a body buffer to be used again.
     * @details Set iWriteFlag to 1 to reset the writing offset as well as
     * the reading offset.  Otherwise, this should be 0.
     *
     * @since 1.0
     *
     * @param[in] pBuf - the body buffer.
     * @param[in] iWriteFlag - The flag specifying whether or not to reset
     *      the writing offset as well.
     * @return void.
     */
    void (*reset_body_buf)(void *pBuf, int iWriteFlag);

    /**
     * @brief append_body_buf appends (writes) data to the end of a body buffer.
     *
     * @since 1.0
     *
     * @param[in] pBuf - the body buffer.
     * @param[in] pBlock - the data buffer to append.
     * @param[in] size - the size (bytes) of the data buffer to append.
     * @return the number of bytes appended, else -1 on error.
     */
    int (*append_body_buf)(void *pBuf, const char *pBlock, int size);

    int (*set_req_body_buf)(lsi_session_t *session, void *pBuf);

    /**
     * @brief get_body_buf_fd is used to get a file descriptor if request or response body is saved in a file-backed MMAP buffer.
     * The file descriptor should not be closed in the module.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return -1 on failure, request body file fd on success.
     */
    int (*get_body_buf_fd)(void *pBuf);

    /**
     * @brief end_resp_headers is called by a module handler when the response headers are complete.
     * @details calling this function is optional. Calling this function will trigger the
     * LSI_HKPT_RECV_RESP_HEADER hook point; if not called, LSI_HKPT_RECV_RESP_HEADER will be
     * tiggerered when adding content to response body.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     */
    void (*end_resp_headers)(lsi_session_t *pSession);

    /**
     * @brief is_resp_headers_sent checks if the response headers are already
     * sent.
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return 1 if the headers were sent already, else 0 if not.
     */
    int (*is_resp_headers_sent)(lsi_session_t *pSession);

    /**
     * @brief get_module_name returns the name of the module.
     *
     * @since 1.0
     *
     * @param[in] pModule - a pointer to lsi_module_t.
     * @return a const pointer to the name of the module, readonly.
     */
    const char *(*get_module_name)(const lsi_module_t *pModule);

    /**
     * @brief get_multiplexer gets the event multiplexer for the main event loop.
     *
     * @since 1.0
     * @return a pointer to the multiplexer used by the main event loop.
     */
    void *(*get_multiplexer)();

    /**
     * @brief is_suspended returns if a session is in suspended mode.
     *
     * @since 1.0
     * @param[in] pSession - a pointer to the HttpSession.
     * @return 0 false, -1 invalid pSession, none-zero true.
     */
    int (*is_suspended)(lsi_session_t *pSession);

    /**
     * @brief resume continues processing of the suspended session.
     *    this should be at the end of a function call
     *
     * @since 1.0
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in] retcode - the result that a hook function returns as if there is no suspend/resume happened.
     * @return -1 failed to resume, 0 successful.
     */
    int (*resume)(lsi_session_t *pSession, int retcode);

    /**
     * @def LSI_SHM_MAX_NAME_LEN
     * @brief Shared Memory maximum characters in name of Shared Memory Pool or Hash Table
     * (shm_pool_init and shm_htable_init).
     * @since 1.0
     */
#define LSI_SHM_MAX_NAME_LEN    (11)

    /**
     * @def LSI_SHM_MAX_ALLOC_SIZE
     * @brief Shared Memory maximum total memory allocatable (~2GB)
     * @since 1.0
     */
#define LSI_SHM_MAX_ALLOC_SIZE  (2000000000)

    /**
     * @def LSI_SHM_INIT
     * @brief Shared Memory flag bit requesting initialization.
     * @since 1.0
     */
#define LSI_SHM_INIT    (0x0001)

    /**
     * @def LSI_SHM_SETTOP
     * @brief Shared Memory flag bit requesting setting top of linked list.
     * @since 1.0
     */
#define LSI_SHM_SETTOP  (0x0002)

    /**
     * @def LSI_SHM_CREATED
     * @brief Shared Memory flag bit indicating newly created.
     * @since 1.0
     */
#define LSI_SHM_CREATED (0x0001)

    /**
     * @brief shm_pool_init initializes a shared memory pool.
     * @details If the pool does not exist, a new one is created.
     *
     * @since 1.0
     *
     * @param[in] pName - the name of the shared memory pool.
     * This name should not exceed #LSI_SHM_MAX_NAME_LEN characters in length.
     * If NULL, the system default name and size are used.
     * @param[in] initSize - the initial size in bytes of the shared memory pool.
     * If 0, the system default size is used.  The shared memory size grows as needed.
     * The maximum total allocatable shared memory is defined by #LSI_SHM_MAX_ALLOC_SIZE.
     * @return a pointer to the Shared Memory Pool object, to be used with subsequent shm_pool_* functions.
     * @note shm_pool_init is generally the first routine called to access the shared memory pool system.
     *   The handle returned is used in all related routines to access and modify shared memory.
     *   The user should always maintain data in terms of shared memory offsets,
     *   and convert to pointers only when accessing or modifying the data, since offsets
     *   to the data remain constant but pointers may change at any time if shared memory is remapped.
     * @code
     *
       lsi_shmpool_t *pShmpool;
       lsi_shm_off_t dataOffset;

       {
           char *ptr;

           ...

           pShmpool = g_api->shm_pool_init( "SHMPool", 0 );
           if ( pShmpool == NULL )
               error;

           ...

           dataOffset = g_api->shm_pool_alloc( pShmpool, 16 );
           if ( dataOffset == 0 )
               error;
           ptr = (char *)g_api->shm_pool_off2ptr( pShmpool, dataOffset );
           if ( ptr == NULL )
               error;
           strcpy( ptr, "Hello World" );

           ...
       }

       {
           ...

           printf( "%s\n", (char *)g_api->shm_pool_off2ptr( pShmpool, dataOffset ) );
           g_api->shm_pool_free( pShmpool, dataOffset, 16 );

           ...
       }
     * @endcode
     * @see shm_pool_alloc, shm_pool_free, shm_pool_off2ptr
     */
    lsi_shmpool_t *(*shm_pool_init)(const char *pName, const size_t initSize);

    /**
     * @brief shm_pool_alloc allocates a shared memory block from the shared memory pool.
     *
     * @since 1.0
     *
     * @param[in] pShmpool - a pointer to the Shared Memory Pool object (from shm_pool_init).
     * @param[in] size - the size in bytes of the memory block to allocate.
     * @return the offset of the allocated memory in the pool, else 0 on error.
     * @note the allocated memory is uninitialized,
     * even if returned offsets are the same as in previous allocations.
     * @see shm_pool_init, shm_pool_free, shm_pool_off2ptr
     */
    lsi_shm_off_t (*shm_pool_alloc)(lsi_shmpool_t *pShmpool, size_t size);

    /**
     * @brief shm_pool_free frees a shared memory block back to the shared memory pool.
     *
     * @since 1.0
     *
     * @param[in] pShmpool - a pointer to the Shared Memory Pool object (from shm_pool_init).
     * @param[in] offset - the offset of the memory block to free, returned from a previous call to shm_pool_alloc.
     * @param[in] size - the size of this memory block which MUST be the same as was allocated for this offset.
     * @return void.
     * @warning it is the responsibility of the user to ensure that offset and size are valid,
     *   that offset was returned from a previous call to shm_pool_alloc,
     *   and that size is the same block size used in the allocation.
     *   Bad things will happen with invalid parameters, which also includes calling shm_pool_free for an already freed block.
     * @see shm_pool_init, shm_pool_alloc
     */
    void (*shm_pool_free)(lsi_shmpool_t *pShmpool, lsi_shm_off_t offset,
                          size_t size);

    /**
     * @brief shm_pool_off2ptr converts a shared memory pool offset to a user space pointer.
     *
     * @since 1.0
     *
     * @param[in] pShmpool - a pointer to the Shared Memory Pool object (from shm_pool_init).
     * @param[in] offset - an offset in the shared memory pool.
     * @return a pointer in the user's space for the shared memory offset, else NULL on error.
     * @note the user should always maintain data in terms of shared memory offsets,
     *   and convert to pointers only when accessing or modifying the data, since offsets
     *   to the data remain constant but pointers may change at any time if shared memory is remapped.
     * @warning it is the responsibility of the user to ensure that offset is valid.
     * @see shm_pool_init, shm_htable_off2ptr
     */
    uint8_t *(*shm_pool_off2ptr)(lsi_shmpool_t *pShmpool,
                                 lsi_shm_off_t offset);

#ifdef notdef
    //helper functions for atomic operations on SHM data
    void (*shm_atom_incr_int8)(lsi_shm_off_t offset, int8_t  inc_by);
    void (*shm_atom_incr_int16)(lsi_shm_off_t offset, int16_t inc_by);
    void (*shm_atom_incr_int32)(lsi_shm_off_t offset, int32_t inc_by);
    void (*shm_atom_incr_int64)(lsi_shm_off_t offset, int64_t inc_by);
#endif

    /**
     * @brief shm_htable_init initializes a shared memory hash table.
     * @details If the hash table does not exist, a new one is created.
     *
     * @since 1.0
     *
     * @param[in] pShmpool - a pointer to a Shared Memory Pool object (from shm_pool_init).
     * If NULL, a Shared Memory Pool object with the name specified by pName is used.
     * @param[in] pName - the name of the hash table.
     * This name should not exceed #LSI_SHM_MAX_NAME_LEN characters in length.
     * If NULL, the system default name is used.
     * @param[in] initSize - the initial size (in entries/buckets) of the hash table index.
     * If 0, the system default number is used.
     * @param[in] hash_pf - a function to be used for hash key generation (optional).
     * If NULL, a default hash function is used.
     * @param[in] comp_pf - a function to be used for key comparison (optional).
     * If NULL, the default compare function is used (strcmp(3)).
     * @return a pointer to the Shared Memory Hash Table object, to be used with subsequent shm_htable_* functions.
     * @note shm_hash_init initializes the hash table system after a shared memory pool has been initialized.
     *   The handle returned is used in all related routines to access and modify shared memory.
     *   The user should always maintain data in terms of shared memory offsets,
     *   and convert to pointers only when accessing or modifying the data, since offsets
     *   to the data remain constant but pointers may change at any time if shared memory is remapped.
     * @code
     *
       lsi_shmpool_t *pShmpool;
       lsi_shmhash_t *pShmhash;
       char myKey[] = "myKey";
       lsi_shm_off_t valOffset;

       {
           int valLen;
           char *valPtr;

           ...

           pShmpool = g_api->shm_pool_init( "SHMPool", 0 );
           if ( pShmpool == NULL )
               error;
           pShmhash = g_api->shm_htable_init( pShmpool, "SHMHash", 0, NULL, NULL );
           if ( pShmhash == NULL )
               error;

           ...

           valOffset = g_api->shm_htable_find(
             pShmhash, (const uint8_t *)myKey, sizeof(myKey) - 1, &valLen );
           if ( valOffset == 0 )
               error;
           valPtr = (char *)g_api->shm_htable_off2ptr( pShmhash, valOffset );
           if ( valPtr == NULL )
               error;
           printf( "[%.*s]\n", valLen, valPtr );    // if values are printable characters

           ...
       }
     * @endcode
     * @see shm_pool_init, shm_htable_add, shm_htable_find, shm_htable_get, shm_htable_set, shm_htable_update, shm_htable_clear
     */
    lsi_shmhash_t *(*shm_htable_init)(lsi_shmpool_t *pShmpool,
                                      const char *pName, size_t initSize, lsi_hash_pf hash_pf,
                                      lsi_hash_value_comp_pf comp_pf);

    // set to new value regardless whether key is in table or not.
    /**
     * @brief shm_htable_set sets a value in the hash table.
     * @details The new value is set whether or not the key currently exists in the table;
     *   i.e., either add a new entry or update an existing one.
     *
     * @since 1.0
     *
     * @param[in] pShmhash - a pointer to the Shared Memory Hash Table object (from shm_htable_init).
     * @param[in] pKey - the hash table entry key.
     * @param[in] keyLen - the length of the key at pKey.
     * @param[in] pValue - the new value to set in the hash table entry.
     * @param[in] valLen - the length of the value at pValue.
     * @return the offset to the entry value in the hash table, else 0 on error.
     * @see shm_htable_init, shm_htable_add, shm_htable_find, shm_htable_get, shm_htable_update, shm_htable_off2ptr
     */
    lsi_shm_off_t (*shm_htable_set)(lsi_shmhash_t *pShmhash,
                                    const uint8_t *pKey, int keyLen, const uint8_t *pValue, int valLen);

    // key must NOT be in table already
    /**
     * @brief shm_htable_add adds a new entry to the hash table.
     * @details The key must NOT currently exist in the table.
     *
     * @since 1.0
     *
     * @param[in] pShmhash - a pointer to the Shared Memory Hash Table object (from shm_htable_init).
     * @param[in] pKey - the hash table entry key.
     * @param[in] keyLen - the length of the key at pKey.
     * @param[in] pValue - the new value to set in the hash table entry.
     * @param[in] valLen - the length of the value at pValue.
     * @return the offset to the entry value in the hash table, else 0 on error.
     * @see shm_htable_init, shm_htable_find, shm_htable_get, shm_htable_set, shm_htable_update, shm_htable_off2ptr
     */
    lsi_shm_off_t (*shm_htable_add)(lsi_shmhash_t *pShmhash,
                                    const uint8_t *pKey, int keyLen, const uint8_t *pValue, int valLen);

    // key must be in table already
    /**
     * @brief shm_htable_update updates a value in the hash table.
     * @details The key MUST currently exist in the table.
     *
     * @since 1.0
     *
     * @param[in] pShmhash - a pointer to the Shared Memory Hash Table object (from shm_htable_init).
     * @param[in] pKey - the hash table entry key.
     * @param[in] keyLen - the length of the key at pKey.
     * @param[in] pValue - the new value to set in the hash table entry.
     * @param[in] valLen - the length of the value at pValue.
     * @return the offset to the entry value in the hash table, else 0 on error.
     * @see shm_htable_init, shm_htable_add, shm_htable_find, shm_htable_get, shm_htable_set, shm_htable_off2ptr
     */
    lsi_shm_off_t (*shm_htable_update)(lsi_shmhash_t *pShmhash,
                                       const uint8_t *pKey, int keyLen, const uint8_t *pValue, int valLen);

    /**
     * @brief shm_htable_find finds a value in the hash table.
     * @details The key MUST currently exist in the table.
     *
     * @since 1.0
     *
     * @param[in] pShmhash - a pointer to the Shared Memory Hash Table object (from shm_htable_init).
     * @param[in] pKey - the hash table entry key.
     * @param[in] keyLen - the length of the key at pKey.
     * @param[out] pvalLen - the length of the value for this entry.
     * @return the offset to the entry value in the hash table, else 0 on error.
     * @see shm_htable_init, shm_htable_add, shm_htable_get, shm_htable_set, shm_htable_update, shm_htable_off2ptr
     */
    lsi_shm_off_t (*shm_htable_find)(lsi_shmhash_t *pShmhash,
                                     const uint8_t *pKey, int keyLen, int *pvalLen);

    /**
     * @brief shm_htable_get gets an entry from the hash table.
     * @details An entry is returned whether or not the key currently exists in the table;
     *   i.e., if the key exists, it is returned (find), else a new one is created (add).
     *
     * @since 1.0
     *
     * @param[in] pShmhash - a pointer to the Shared Memory Hash Table object (from shm_htable_init).
     * @param[in] pKey - the hash table entry key.
     * @param[in] keyLen - the length of the key at pKey.
     * @param[in,out] pvalLen - the length of the value for this entry.
     * @param[in,out] pFlags - various flags (parameters and returns).
     * - #LSI_SHM_INIT (in) - initialize (clear) entry IF and only if newly created.
     * - #LSI_SHM_SETTOP (in) - set entry to the top of the linked list.
     * - #LSI_SHM_CREATED (out) - a new entry was created.
     * @return the offset to the entry value in the hash table, else 0 on error.
     * @note the parameter specified by the user at pvalLen is used only if a new entry is created (#LSI_SHM_CREATED is set);
     *   else, the length of the existing entry value is returned through this pointer.
     *   This parameter cannot change the size of an existing entry.
     * @see shm_htable_init, shm_htable_add, shm_htable_find, shm_htable_set, shm_htable_update, shm_htable_off2ptr
     */
    lsi_shm_off_t (*shm_htable_get)(lsi_shmhash_t *pShmhash,
                                    const uint8_t *pKey, int keyLen, int *pvalLen, int *pFlags);

    /**
     * @brief shm_htable_delete deletes/removes a hash table entry.
     *
     * @since 1.0
     *
     * @param[in] pShmhash - a pointer to the Shared Memory Hash Table object (from shm_htable_init).
     * @param[in] pKey - the hash table entry key.
     * @param[in] keyLen - the length of the key at pKey.
     * @return void.
     * @see shm_htable_init, shm_htable_clear
     */
    void (*shm_htable_delete)(lsi_shmhash_t *pShmhash, const uint8_t *pKey,
                              int keyLen);

    /**
     * @brief shm_htable_clear deletes all hash table entries for the given hash table.
     *
     * @since 1.0
     *
     * @param[in] pShmhash - a pointer to the Shared Memory Hash Table object (from shm_htable_init).
     * @return void.
     * @see shm_htable_init, shm_htable_delete
     */
    void (*shm_htable_clear)(lsi_shmhash_t *pShmhash);

#ifdef notdef
    int (*shm_htable_destroy)(lsi_shmhash_t *pShmhash);
#endif

    /**
     * @brief shm_htable_off2ptr converts a hash table offset to a user space pointer.
     *
     * @since 1.0
     *
     * @param[in] pShmhash - a pointer to the Shared Memory Hash Table object (from shm_htable_init).
     * @param[in] offset - an offset in the shared memory hash table.
     * @return a pointer in the user's space for the shared memory offset, else NULL on error.
     * @note the user should always maintain data in terms of shared memory offsets,
     *   and convert to pointers only when accessing or modifying the data, since offsets
     *   to the data remain constant but pointers may change at any time if shared memory is remapped.
     * @warning it is the responsibility of the user to ensure that offset is valid.
     * @see shm_htable_init, shm_pool_off2ptr
     */
    uint8_t *(*shm_htable_off2ptr)(lsi_shmhash_t *pShmhash,
                                   lsi_shm_off_t offset);

#ifdef notdef
    int (*is_subrequest)(lsi_session_t *pSession);
#endif

    /**
     * @brief get_cur_time gets the current system time.
     *
     * @since 1.0
     *
     * @param[in,out] usec - if not NULL,
     *  upon return contains the microseconds of the current time.
     * @return the seconds of the current time
     *  (the number of seconds since the Epoch, time(2)).
     */
    time_t (*get_cur_time)(int32_t *usec);

    /**
     * @brief get_vhost_count gets the count of Virtual Hosts in the system.
     *
     * @since 1.0
     *
     * @return the count of Virtual Hosts.
     */
    int (*get_vhost_count)();

    /**
     * @brief get_vhost gets a Virtual Host object.
     *
     * @since 1.0
     *
     * @param[in] index - the index of the Virtual Host, starting from 0.
     * @return a pointer to the Virtual Host object.
     */
    const void *(*get_vhost)(int index);

    /**
     * @brief set_vhost_module_data
     *  is used to set the module data for a Virtual Host.
     * @details The routine is similar to set_module_data,
     *  but may be used without reference to a session.
     *
     * @since 1.0
     *
     * @param[in] vhost - a pointer to the Virtual Host.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] data - a pointer to the user defined data.
     * @return -1 on failure, 0 on success.
     *
     * @see set_module_data
     */
    int (* set_vhost_module_data)(const void *vhost,
                                  const lsi_module_t *pModule, void *data);

    /**
     * @brief get_vhost_module_data
     *  gets the module data which was set by set_vhost_module_data.
     * The return value is a pointer to the user's own data structure.
     * @details The routine is similar to get_module_data,
     *  but may be used without reference to a session.
     *
     * @since 1.0
     *
     * @param[in] vhost - a pointer to the Virtual Host.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @return NULL on failure, a pointer to the user defined data on success.
     *
     * @see get_module_data
     */
    void       *(* get_vhost_module_data)(const void *vhost,
                                          const lsi_module_t *pModule);

    /**
     * @brief get_vhost_module_param
     *  gets the user defined module parameters which are parsed by
     *  the callback _parse_config and pointed to in the struct lsi_config_t.
     *
     * @since 1.0
     *
     * @param[in] vhost - a pointer to the Virtual Host.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @return NULL on failure,
     *  a pointer to the user-defined configuration data on success.
     *
     * @see get_module_param
     */
    void       *(* get_vhost_module_param)(const void *vhost,
                                           const lsi_module_t *pModule);

    /**
     * @brief get_session_pool gets the session pool to allow modules to allocate from.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @return a pointer to the session pool.
     */
    ls_xpool_t *(*get_session_pool)(lsi_session_t *pSession);

    /**
     * @brief get_local_sockaddr
     *  gets the socket address in a character string format.
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[out] pIp - a pointer to the returned address string.
     * @param[in] maxLen - the size of the buffer at \e pIp.
     * @return the length of the string on success,
     *  else 0 for system errors, or -1 for bad parameters.
     */
    int (* get_local_sockaddr)(lsi_session_t *pSession, char *pIp, int maxLen);

    /**
     * @brief get_server_mode
     *  gets the mode of the server defined by enum #lsi_server_mode.
     *
     * @since 1.0
     *
     * @return the mode of the server.
     */
    int (* get_server_mode)();

    /**
     * @brief handoff_fd return a duplicated file descriptor associated with current session and
     *    all data received on this file descriptor, including parsed request headers
     *    and data has not been processed.
     *    After this function call the server core will stop processing current session and closed
     *    the original file descriptor. The session object will become invalid.
     *
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the HttpSession.
     * @param[in,out] pData - a pointer to a pointer pointing to the buffer holding data received so far
     *     The buffer is allocated by server core, must be released by the caller of this function with free().
     * @param[in,out] pDataLen - a pointer to the variable receive the size of the buffer.
     * @return a file descriptor if success, -1 if failed. the file descriptor is dupped from the original
     *         file descriptor, it should be closed by the caller of this function when done.
     */
    int (*handoff_fd)(lsi_session_t *pSession, char **pData, int *pDataLen);

    /** @since 1.0
     * @param[in] level - lsi_config_level
     * @param[in] pVarible - varible of the Server/Listener/VHost or URI of the Context
     * @param[in] buf - a buffer to store the result
     * @param[in] max_len - length of the buf
     * @return return the length written to the buf
     */
    int (*expand_current_server_varible)(int level, const char *pVarible,
                                         char *buf, int max_len);

    /**
     *
     * @brief _debugLevel is the level of debugging than server core uses,
     *    it controls the level of details of debugging messages.
     *    its range is from 0 to 10, debugging is disabled when set to 0,
     *    highest level of debug output is used when set to 10.
     *
     */
    unsigned char   _debugLevel;

};

/**
 *
 * @brief  make LSIAPI functions globally available.
 * @details g_api is a global variable, it can be accessed from all modules to make API calls.
 *
 * @since 1.0
 */
extern const lsi_api_t *g_api;

/**
 *
 * @brief inline function to check if debug logging is enabled or not,
 * It should be checked before calling g_api->log() to write a debug log message to
 * minimize the cost of debug logging when debug logging was disabled.
 *
 * @param[in] level the debug logging level, can be in range of 0-9, use 0 to check if debug logging is on or off.
 *                  use 1-9 to check if message at specific level should be logged.
 * @return  if current debug level is higher (bigger number) than @param level, return 1, otherwise return 0
 *
 */
static inline int lsi_isdebug(unsigned int level)
{   return (g_api->_debugLevel > level);  }


#ifdef __cplusplus
}
#endif


#endif //LS_MODULE_H
