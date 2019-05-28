/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
#include <lsr/ls_evtcb.h>
#include <lsr/ls_edio.h>

#include <stdio.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <inttypes.h>
#include <stdarg.h>


#define LSMODULE_EXPORT  __attribute__ ((visibility ("default")))


/**
 * @file ls.h
 * @brief Include file for the OpenLiteSpeed Web Server
 */

/**
 * @mainpage
 * LiteSpeed Web Server
 * LSIAPI Reference
 * @version 1.4
 * @date 2013-2018
 * @author LiteSpeed Development Team
 * @copyright LiteSpeed Technologies, Inc. and the GNU Public License.
 */

/**
 * @def LSIAPI_VERSION_MAJOR
 * @brief Defines the major version number of LSIAPI.
 */
#define LSIAPI_VERSION_MAJOR    1
/**
 * @def LSIAPI_VERSION_MINOR
 * @brief Defines the minor version number of LSIAPI.
 */
#define LSIAPI_VERSION_MINOR    2
/**
 * @def LSIAPI_VERSION_STRING
 * @brief Defines the version string of LSIAPI.
 */
#define LSIAPI_VERSION_STRING   "1.2"

/**
 * @def LSI_MODULE_SIGNATURE
 * @brief Identifies the module as a LSIAPI module and the version of LSIAPI
 * that the module was compiled with.
 * @details The signature tells the server core first that it is actually a
 * LSIAPI module, and second, what version of the API that the binary was
 * compiled with in order to check whether it is backwards compatible with
 * future versions.
 * @since 1.0
 */
//#define LSIAPI_MODULE_FLAG    0x4C53494D  //"LSIM"
#define LSI_MODULE_SIGNATURE    ((int64_t)0x4C53494D00000000LL + \
    (int64_t)(LSIAPI_VERSION_MAJOR << 16) + \
    (int64_t)(LSIAPI_VERSION_MINOR))



/**
 * @def LSI_MAX_RESP_BUFFER_SIZE
 * @brief The max buffer size for is_resp_buffer_available.
 * @since 1.0
 */
#define LSI_MAX_RESP_BUFFER_SIZE     (1024*1024)


#ifdef __cplusplus
extern "C" {
#endif


/**************************************************************************************************
 *                                       API Parameter ENUMs
 **************************************************************************************************/

/**
 * @defgroup module Overall Module Definitions and Functions
 * @brief Overall Module Definitions and Functions are used as initial entry
 * into the Litespeed Module API.  Most of the types and functions in this
 * group are optional, except the lsi_module_t which is required to define the
 * module to the system.
 */

/**
 * @defgroup logging Logging
 * @brief Logging is used to write messages to the LiteSpeed error log.  Almost
 * all developers will need to write to the log from time to time.  It is
 * strongly recommended that module developers use the LSM_XXX macros to write
 * to the log.
 */

/**
 * @defgroup data_storage Data Storage
 * @brief Data Storage is a single user allocated data pointer available via the
 * session key for each #LSI_DATA_LEVEL type.  It is particularly useful for
 * request handlers as it allows association of a user defined data area between
 * the difference functions of the session.
 */

/**
 * @defgroup session Request/Response Session Handler
 * @brief Session Request Handler processing facilities.
 */

/**
 * @defgroup environ Environment Variables
 * @brief Access to local and server created environment variables.
 */

/**
 * @defgroup uri URI/URL
 * @brief URI/URL processing part of the session process including inbound
 * cookies.
 */

/**
 * @defgroup reqmethod Request Method
 * @brief Request method (GET, POST, etc.) part of the session process.
 */

/**
 * @defgroup security Security
 * @brief Security part of the session process.
 */

/**
 * @defgroup req_header Request Header
 * @brief Request header part of the session process.
 */

/**
 * @defgroup req_body Request Body
 * @brief Request body access part of the session process.
 */

/**
 * @defgroup resp_header Response Header
 * @brief Response header part of the session process including outbound
 * cookies.
 */

/**
 * @defgroup post POST Request/Response
 * @brief POST specific definitions/functions.
 */

/**
 * @defgroup query Queries
 * @brief Queries of information available from the server.
 */

/**
 * @defgroup response Server Response
 * @brief Server response definitions/functions.
 */

/**
 * @defgroup hooks Hooks
 * @brief Hooks are callbacks that can be used to monitor or modify data.
 */

/**
 * @defgroup config Module Configuration
 * @brief LiteSpeed module configuration specific values/functions.
 */

/**
 * @defgroup vhost Virtual Host
 * @brief Virtual host definitions/functions.
 */

/**
 * @defgroup L4 Layer 4: TCP communications
 * @brief Layer 4, TCP communications definitions/functions.  Most of the
 * functionality is handled as hooks.
 */

/**
 * @defgroup utility Utility
 * @brief Utility specific definitions/functions.
 */

/**
 * @defgroup timer Timer
 * @brief Timer specific definitions/functions
 */

/**
 * @enum LSI_CFG_LEVEL
 * @brief Conditions when a particular configuration entry is relevant in user
 * configuration parameter parsing.
 * @since 1.0
 * @ingroup config
 */
enum LSI_CFG_LEVEL
{
    /**
     * Server level configuration.
     */
    LSI_CFG_SERVER = 1,
    /**
     * Listener level configuration.
     */
    LSI_CFG_LISTENER = 2,
    /**
     * Virtual Host level configuration.
     */
    LSI_CFG_VHOST = 4,
    /**
     * Context level configuration.
     */
    LSI_CFG_CONTEXT = 8,
};


/**
 * @enum LSI_HOOK_PRIORITY
 * @brief The default running priorities for hook filter functions.
 * @details Used when there are more than one filter at the same hook point.
 * Any number from -1 * LSI_HOOK_PRIORITY_MAX to LSI_HOOK_PRIORITY_MAX can also
 * be used.  The lower values will have higher priority, so they will run first.
 * Hook functions with the same order value will be called based on the order in
 * which they appear in the configuration list.
 * @since 1.0
 * @ingroup hooks
 */
enum LSI_HOOK_PRIORITY
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
    /**
    * The max priority level allowed.
    */
    LSI_HOOK_PRIORITY_MAX = 6000

};


/**
 * @enum LSI_HOOK_FLAG
 * @brief Flags applying to hook functions.
 * @details A filter has two modes, TRANSFORMER and OBSERVER.  Flag if it is a
 * transformer.
 * @details LSI_FLAG_TRANSFORM is a flag for filter hook points, indicating
 * that the filter may change the content of the input data.
 * If a filter does not change the input data, in OBSERVER mode, do not
 * set this flag.
 * When no TRANSFORMERs are in a filter chain of a hook point, the server will
 * do optimizations to avoid deep copy by storing the data in the final buffer.
 * Its use is important for #LSI_HKPT_RCVD_REQ_BODY and #LSI_HKPT_L4_RECVING
 * filter hooks.
 * @since 1.0
 * @ingroup hooks
 *
 * @note Hook flags are additive. When required, multiple flags should be set.
 */
enum LSI_HOOK_FLAG
{
    /**
     * The filter hook function is a Transformer which modifies the input data.
     * This is for filter type hook points. If no filters are Transformer
     * filters, optimization could be applied by the server core when processing
     * the data.
     */
    LSI_FLAG_TRANSFORM  = 1,

    /**
      * The hook function requires decompressed data.
      * This flag is for LSI_HKPT_RECV_RESP_BODY and LSI_HKPT_SEND_RESP_BODY.
      * If any filter requires decompression, the server core will add the
      * decompression filter at the beginning of the filter chain for compressed
      * data.
      */
    LSI_FLAG_DECOMPRESS_REQUIRED = 2,

    /**
     * This flag is for LSI_HKPT_SEND_RESP_BODY only, and should be added only
     * if a filter needs to process a static file.  If no filter is needed to
     * process a static file, sendfile() API will be used.
     */
    LSI_FLAG_PROCESS_STATIC = 4,

    /**
     * This flag enables the hook function.
     * It may be set statically on the global level,
     * or a call to enable_hook may be used.
     */
    LSI_FLAG_ENABLED = 8,
};


/**
 * @enum LSI_DATA_LEVEL
 * @brief Determines the scope of the module's user defined data.
 * @details Used by the level API parameter of various functions.
 * Determines what other functions are allowed to access the data and for how long
 * it will be available.
 * @since 1.0
 * @ingroup data_storage
 */
enum LSI_DATA_LEVEL
{
    /**
     * User data type for an HTTP session.
     */
    LSI_DATA_HTTP = 0,
    /**
     * User data for cached file type.
     */
    LSI_DATA_FILE,
    /**
     * User data type for TCP/IP data.
     */
    LSI_DATA_IP,
    /**
     * User data type for Virtual Hosts.
     */
    LSI_DATA_VHOST,
    /**
     * Module data type for a TCP layer 4 session.
     */
    LSI_DATA_L4,

    /**
     * Placeholder.  This is NOT an index
     */
    LSI_DATA_COUNT,
};


/**
 * @enum LSI_HKPT_LEVEL
 * @brief Hook Point level definitions.
 * @details Used in the API index parameter.  Determines which stage to hook the
 * callback function to.
 * @since 1.0
 * @see lsi_serverhook_s
 * @ingroup hooks
 */
enum LSI_HKPT_LEVEL
{
    /**
     * LSI_HKPT_L4_BEGINSESSION is the point when the session begins a TCP
     * connection.  A TCP connection is also called layer 4 connection.
     */
    LSI_HKPT_L4_BEGINSESSION = 0,   //<--- must be first of L4

    /**
     * LSI_HKPT_L4_ENDSESSION is the point when the session ends a TCP
     * connection.
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
     * LSI_HKPT_HTTP_BEGIN is the point when the session begins an http
     * connection.
     */
    LSI_HKPT_HTTP_BEGIN,       //<--- must be first of Http

    /**
     * LSI_HKPT_RCVD_REQ_HEADER is the point when the request header was just
     * received.
     */
    LSI_HKPT_RCVD_REQ_HEADER,

    /**
     * LSI_HKPT_URI_MAP is the point when a URI request is mapped to a context.
     */
    LSI_HKPT_URI_MAP,

    /**
     * LSI_HKPT_HTTP_AUTH is the point when authentication check is performed.
     * It is triggered right after HTTP built-in authentication has been
     * performed, such as HTTP BASIC/DIGEST authentication.
     */
    LSI_HKPT_HTTP_AUTH,

    /**
     * LSI_HKPT_RECV_REQ_BODY is the point when request body data is being
     * received.
     */
    LSI_HKPT_RECV_REQ_BODY,

    /**
     * LSI_HKPT_RCVD_REQ_BODY is the point when request body data was received.
     * This function accesses the body data file through a function pointer.
     */
    LSI_HKPT_RCVD_REQ_BODY,

    /**
     * LSI_HKPT_RECV_RESP_HEADER is the point when the response header was
     * created.
     */
    LSI_HKPT_RCVD_RESP_HEADER,

    /**
     * LSI_HKPT_RECV_RESP_BODY is the point when response body is being received
     * by the backend of the web server.
     */
    LSI_HKPT_RECV_RESP_BODY,

    /**
     * LSI_HKPT_RCVD_RESP_BODY is the point when the whole response body was
     * received by the backend of the web server.
     */
    LSI_HKPT_RCVD_RESP_BODY,

    /**
     * LSI_HKPT_HANDLER_RESTART is the point when the server core needs to
     * restart handler processing by discarding the current response, either
     * sending back an error page, or redirecting to a new URL.  It could be
     * triggered by internal redirect, a module deny access; it is only
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
     * LSI_HKPT_HTTP_END is the point when a session is ending an http
     * connection.
     */
    LSI_HKPT_HTTP_END,      //<--- must be last of Http

    /**
     * LSI_HKPT_MAIN_INITED is the point when the main (controller) process
     * has completed its initialization and configuration, before servicing any
     * requests.  It occurs once upon startup.
     */
    LSI_HKPT_MAIN_INITED,     //<--- must be first of Server

    /**
     * LSI_HKPT_MAIN_PREFORK is the point when the main (controller) process
     * is about to start (fork) a worker process.  This occurs for each worker,
     * and may happen during system startup, or if a worker has been restarted.
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
     * LSI_HKPT_WORKER_INIT is the point at the beginning of a worker process
     * starts, before going into its event loop to start serve requests.
     * It is invoked when a worker process forked from main (controller) process,
     * or server started without the main (controller) process (debugging mode).
     * Note that when forked from the main process, a corresponding
     * MAIN_POSTFORK hook will be triggered in the main process, it may
     * occur either before *or* after this hook.
     */
    LSI_HKPT_WORKER_INIT,

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

    /**
     * This is NOT an index, just a placeholder for the end of the enum.
     */
    LSI_HKPT_TOTAL_COUNT,
};


/**
 * @enum LSI_LOG_LEVEL
 * @brief The log level definitions.
 * @details Used in the level API parameter of the write log function.
 * All logs with a log level less than or equal to the server defined level will
 * be written to the log.
 * @since 1.0
 * @ingroup logging
 */
enum LSI_LOG_LEVEL
{
    /**
     * Error level output message.
     */
    LSI_LOG_ERROR  = 3000,
    /**
     * Warning level output message.
     */
    LSI_LOG_WARN   = 4000,
    /**
     * Notice level output message.
     */
    LSI_LOG_NOTICE = 5000,
    /**
     * Info level output message.
     */
    LSI_LOG_INFO   = 6000,
    /**
     * Debug level output message.
     */
    LSI_LOG_DEBUG  = 7000,

    /**
     * Within the Debug level, the level of the debug.  A Low debug level.
     */
    LSI_LOG_DEBUG_LOW  = 7020,
    /**
     * Within the Debug level, the level of the debug.  A Medium debug level.
     */
    LSI_LOG_DEBUG_MEDIUM = 7050,
    /**
     * Within the Debug level, the level of the debug.  A High debug level.
     */
    LSI_LOG_DEBUG_HIGH = 7080,

    /**
     * Trace level output message.
     */
    LSI_LOG_TRACE  = 8000,
};


/**
 * @enum LSI_RETCODE
 * @brief LSIAPI return value definition, specifically for hooks.
 * @details Used in the return value of API functions and callback functions
 * unless otherwise stipulated.  If a function returns an int type value, it
 * should always return #LSI_OK for no errors and #LSI_ERROR for other cases.
 * Hook functions can further specify specific functions by using #LSI_SUSPEND
 * or #LSI_DENY.  For such functions that return a bool type (true / false),
 * 1 means true and 0 means false.
 * @since 1.0
 * @ingroup hooks
 */
enum LSI_RETCODE
{
    /**
     * Return code for suspend current hookpoint
     */
    LSI_SUSPEND = -3,
    /**
     * Return code to deny access for the current hookpoint.
     */
    LSI_DENY = -2,
    /**
     * Return code error for any int function.
     */
    LSI_ERROR = -1,
    /**
     * Return code success for any int function.
     */
    LSI_OK = 0,

};


/**
 * @enum LSI_ONWRITE_RETCODE
 * @brief Write response return values.
 * @details Used as on_write_resp return value.
 * Continue should be used until the response sending is completed.
 * Finished will end the process of the server requesting further data.
 * @since 1.0
 * @ingroup response
 */
enum LSI_ONWRITE_RETCODE
{
    /**
     * Error in the response processing.
     */
    LSI_RSP_ERROR = -1,
    /**
     * No further response data to write.
     */
    LSI_RSP_DONE = 0,
    /**
     * More response body data to write.
     */
    LSI_RSP_MORE,
};


/**
 * @enum LSI_CB_FLAGS
 * @brief definition of flags used in hook function input and output parameters
 * @details It defines flags for flag_in and flag_out of lsi_param_t.
 * #LSI_CBFO_BUFFERED is for flag_out, all other flags can be combined for
 * flag_in; the flags should be set or removed through a bitwise operator.
 *
 * @since 1.0
 * @ingroup module
 */
enum LSI_CB_FLAGS
{
    /**
     * This output flag, indicates that a filter buffered data in its own
     * buffer
     */
    LSI_CBFO_BUFFERED = 1,
    /**
     * This input flag requires the filter to flush its internal buffer to
     * the next filter, then pass this flag to the next filter.
     */
    LSI_CBFI_FLUSH = 1,
    /**
     * This input flag tells the filter it is the end of stream; there should
     * be no more data feeding into this filter.  The filter should ignore any
     * input data after this flag is set.  This flag implies #LSI_CBFI_FLUSH.
     * A filter should only set this flag after all buffered data has been sent
     * to the next filter.
     */
    LSI_CBFI_EOF = 2,
    /**
     * This flag is set for #LSI_HKPT_SEND_RESP_BODY only if the input data is
     * from a static file; if a filter does not need to check a static file, it
     * can skip processing data if this flag is set.
     */
    LSI_CBFI_STATIC = 4,
    /**
     * This flag is set for #LSI_HKPT_RCVD_RESP_BODY only if the request handler
     * does not abort in the middle of processing the response; for example,
     * backend PHP crashes, or HTTP proxy connection to backend has been reset.
     * If a hook function only needs to process a successful response, it should
     * check for this flag.
     */
    LSI_CBFI_RESPSUCC = 8,
};


/**
 * @enum LSI_REQ_VARIABLE
 * @brief LSIAPI request environment variables.
 * @details Used as API index parameter in env access function
 * get_req_var_by_id.  The example reqinfohandler.c shows usage for all of these
 * variables.
 * @since 1.0
 * @ingroup environ
 */
enum LSI_REQ_VARIABLE
{
    /**
     * Remote IP address environment variable
     */
    LSI_VAR_REMOTE_ADDR = 0,
    /**
     * Remote IP port environment variable
     */
    LSI_VAR_REMOTE_PORT,
    /**
     * Remote host environment variable
     */
    LSI_VAR_REMOTE_HOST,
    /**
     * Remote user environment variable
     */
    LSI_VAR_REMOTE_USER,
    /**
     * Remote identifier environment variable
     */
    LSI_VAR_REMOTE_IDENT,
    /**
     * Remote method environment variable
     */
    LSI_VAR_REQ_METHOD,
    /**
     * Query string environment variable
     */
    LSI_VAR_QUERY_STRING,
    /**
     * Authentication type environment variable
     */
    LSI_VAR_AUTH_TYPE,
    /**
     * URI path environment variable
     */
    LSI_VAR_PATH_INFO,
    /**
     * Script filename environment variable
     */
    LSI_VAR_SCRIPTFILENAME,
    /**
     * Filename port environment variable
     */
    LSI_VAR_REQUST_FN,
    /**
     * URI environment variable
     */
    LSI_VAR_REQ_URI,
    /**
     * Document root directory environment variable
     */
    LSI_VAR_DOC_ROOT,
    /**
     * Port environment variable
     */
    LSI_VAR_SERVER_ADMIN,
    /**
     * Server name environment variable
     */
    LSI_VAR_SERVER_NAME,
    /**
     * Server address environment variable
     */
    LSI_VAR_SERVER_ADDR,
    /**
     * Server port environment variable
     */
    LSI_VAR_SERVER_PORT,
    /**
     * Server prototype environment variable
     */
    LSI_VAR_SERVER_PROTO,
    /**
     * Server software version environment variable
     */
    LSI_VAR_SERVER_SOFT,
    /**
     * API version environment variable
     */
    LSI_VAR_API_VERSION,
    /**
     * Request line environment variable
     */
    LSI_VAR_REQ_LINE,
    /**
     * Subrequest environment variable
     */
    LSI_VAR_IS_SUBREQ,
    /**
     * Time environment variable
     */
    LSI_VAR_TIME,
    /**
     * Year environment variable
     */
    LSI_VAR_TIME_YEAR,
    /**
     * Month environment variable
     */
    LSI_VAR_TIME_MON,
    /**
     * Day environment variable
     */
    LSI_VAR_TIME_DAY,
    /**
     * Hour environment variable
     */
    LSI_VAR_TIME_HOUR,
    /**
     * Minute environment variable
     */
    LSI_VAR_TIME_MIN,
    /**
     * Seconds environment variable
     */
    LSI_VAR_TIME_SEC,
    /**
     * Weekday environment variable
     */
    LSI_VAR_TIME_WDAY,
    /**
     * Script file name environment variable
     */
    LSI_VAR_SCRIPT_NAME,
    /**
     * Current URI environment variable
     */
    LSI_VAR_CUR_URI,
    /**
     * URI base name environment variable
     */
    LSI_VAR_REQ_BASENAME,
    /**
     * Script user id environment variable
     */
    LSI_VAR_SCRIPT_UID,
    /**
     * Script global id environment variable
     */
    LSI_VAR_SCRIPT_GID,
    /**
     * Script user name environment variable
     */
    LSI_VAR_SCRIPT_USERNAME,
    /**
     * Script group name environment variable
     */
    LSI_VAR_SCRIPT_GRPNAME,
    /**
     * Script mode environment variable
     */
    LSI_VAR_SCRIPT_MODE,
    /**
     * Script base name environment variable
     */
    LSI_VAR_SCRIPT_BASENAME,
    /**
     * Script URI environment variable
     */
    LSI_VAR_SCRIPT_URI,
    /**
     * Original URI environment variable
     */
    LSI_VAR_ORG_REQ_URI,
    /**
     * Original QS environment variable
     */
    LSI_VAR_ORG_QS,
    /**
     * HTTPS environment variable
     */
    LSI_VAR_HTTPS,
    /**
     * SSL version environment variable
     */
    LSI_VAR_SSL_VERSION,
    /**
     * SSL session ID environment variable
     */
    LSI_VAR_SSL_SESSION_ID,
    /**
     * SSL cipher environment variable
     */
    LSI_VAR_SSL_CIPHER,
    /**
     * SSL cipher use key size environment variable
     */
    LSI_VAR_SSL_CIPHER_USEKEYSIZE,
    /**
     * SSL cipher ALG key size environment variable
     */
    LSI_VAR_SSL_CIPHER_ALGKEYSIZE,
    /**
     * SSL client certification environment variable
     */
    LSI_VAR_SSL_CLIENT_CERT,
    /**
     * Geographical IP address environment variable
     */
    LSI_VAR_GEOIP_ADDR,
    /**
     * Translated path environment variable
     */
    LSI_VAR_PATH_TRANSLATED,
    /**
     * Placeholder.
     */
    LSI_VAR_COUNT,    /** This is NOT an index */
};


/**
 * @enum LSI_HEADER_OP
 * @brief The methods used for adding a response header.
 * @details Used in API parameter add_method in response header access
 * functions. If there are no existing headers, any method that is called
 * will have the effect of adding a new header.  If there is an existing header,
 * #LSI_HEADEROP_SET will add a header and will replace the existing one.
 * #LSI_HEADEROP_APPEND will append a comma and the header value to the end of
 * the existing value list. # LSI_HEADEROP_MERGE is just like
 * #LSI_HEADEROP_APPEND unless the same value exists in the existing header;
 * In this case, it will do nothing.  #LSI_HEADEROP_ADD will add a new line to
 * the header, whether or not it already exists.
 * @since 1.0
 * @ingroup resp_header
 */
enum LSI_HEADER_OP
{
    /**
     * Set the header.
     */
    LSI_HEADEROP_SET = 0,
    /**
     * Add with a comma to separate
     */
    LSI_HEADEROP_APPEND,
    /**
     * Append unless it exists
     */
    LSI_HEADEROP_MERGE,
    /**
     * Add a new line
     */
    LSI_HEADEROP_ADD
};


/**
 * @enum LSI_URL_OP
 * @brief The methods used to redirect a request to a new URL.
 * @details #LSI_URL_NOCHANGE, #LSI_URL_REWRITE and LSI_URL_REDIRECT_* can be
 * combined with LSI_URL_QS_* values.
 * @since 1.0
 * @see lsi_api_t::set_uri_qs
 * @ingroup response
 */
enum LSI_URL_OP
{
    /**
     * Do not change URI, intended for modifying Query String only.
     */
    LSI_URL_NOCHANGE = 0,

    /**
     * Rewrite to the new URI and use the URI for subsequent processing stages.
     */
    LSI_URL_REWRITE,

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
     * Do not change Query String. Only valid with LSI_URL_REWRITE.
     */
    LSI_URL_QS_NOCHANGE = 0 << 4,

    /**
     * Append Query String. Can be combined with LSI_URL_REWRITE and LSI_URL_REDIRECT_*.
     */
    LSI_URL_QS_APPEND = 1 << 4,

    /**
     * Set Query String. Can be combined with LSI_URL_REWRITE and LSI_URL_REDIRECT_*.
     */
    LSI_URL_QS_SET = 2 << 4,

    /**
     * Delete Query String. Can be combined with LSI_URL_REWRITE and LSI_URL_REDIRECT_*.
     */
    LSI_URL_QS_DELETE = 3 << 4,

    /**
     * indicates that encoding has been applied to URI.
     */
    LSI_URL_ENCODED = 128
};


/**
 * @enum LSI_REQ_HEADER_ID
 * @brief The most common request-header ids.
 * @details Used as the API header id parameter in request header access
 * functions to access components of the request header.
 * @since 1.0
 * @ingroup req_header
 */
enum LSI_REQ_HEADER_ID
{
    /**
     * "Accept" request header.
     */
    LSI_HDR_ACCEPT = 0,
    /**
     * "Accept-Charset" request header.
     */
    LSI_HDR_ACC_CHARSET,
    /**
     * "Accept-Encoding" request header.
     */
    LSI_HDR_ACC_ENCODING,
    /**
     * "Accept-Language" request header.
     */
    LSI_HDR_ACC_LANG,
    /**
     * "Authorization" request header.
     */
    LSI_HDR_AUTHORIZATION,
    /**
     * "Connection" request header.
     */
    LSI_HDR_CONNECTION,
    /**
     * "Content-Type" request header.
     */
    LSI_HDR_CONTENT_TYPE,
    /**
     * "Content-Length" request header.
     */
    LSI_HDR_CONTENT_LENGTH,
    /**
     * "Cookie" request header.
     */
    LSI_HDR_COOKIE,
    /**
     * "Cookie2" request header.
     */
    LSI_HDR_COOKIE2,
    /**
     * "Host" request header.
     */
    LSI_HDR_HOST,
    /**
     * "Pragma" request header.
     */
    LSI_HDR_PRAGMA,
    /**
     * "Referer" request header.
     */
    LSI_HDR_REFERER,
    /**
     * "User-Agent" request header.
     */
    LSI_HDR_USERAGENT,
    /**
     * "Cache-Control" request header.
     */
    LSI_HDR_CACHE_CTRL,
    /**
     * "If-Modified-Since" request header.
     */
    LSI_HDR_IF_MODIFIED_SINCE,
    /**
     * "If-Match" request header.
     */
    LSI_HDR_IF_MATCH,
    /**
     * "If-No-Match" request header.
     */
    LSI_HDR_IF_NO_MATCH,
    /**
     * "If-Range" request header.
     */
    LSI_HDR_IF_RANGE,
    /**
     * "If-Unmodified-Since" request header.
     */
    LSI_HDR_IF_UNMOD_SINCE,
    /**
     * "Keep-Alive" request header.
     */
    LSI_HDR_KEEP_ALIVE,
    /**
     * "Range" request header.
     */
    LSI_HDR_RANGE,
    /**
     * "X-Forwarded-For" request header.
     */
    LSI_HDR_X_FORWARDED_FOR,
    /**
     * "Via" request header.
     */
    LSI_HDR_VIA,
    /**
     * "Transfer-Encoding" request header.
     */
    LSI_HDR_TRANSFER_ENCODING,
    
    /***
     * Add a padding to make the indexes match server internal indexes.
     */
    LSI_HDR_TE_PADDING,
    
    /**
     * "X-LiteSpeed-Purge" request header.
     */
    LSI_HDR_LITESPEED_PURGE,

};


/**
 * @enum LSI_RESP_HEADER_ID
 * @brief The most common response-header ids.
 * @details Used as the API header id parameter in response header access
 * functions to access components of the response header.
 * @since 1.0
 * @ingroup resp_header
 */
enum LSI_RESP_HEADER_ID
{
    /**
     * Accept ranges id
     */
    LSI_RSPHDR_ACCEPT_RANGES = 0,
    /**
     * Connection id
     */
    LSI_RSPHDR_CONNECTION,
    /**
     * Content type id.
     */
    LSI_RSPHDR_CONTENT_TYPE,
    /**
     * Content length id.
     */
    LSI_RSPHDR_CONTENT_LENGTH,
    /**
     * Content encoding id.
     */
    LSI_RSPHDR_CONTENT_ENCODING,
    /**
     * Content range id.
     */
    LSI_RSPHDR_CONTENT_RANGE,
    /**
     * Contnet disposition id.
     */
    LSI_RSPHDR_CONTENT_DISPOSITION,
    /**
     * Cache control id.
     */
    LSI_RSPHDR_CACHE_CTRL,
    /**
     * Date id.
     */
    LSI_RSPHDR_DATE,
    /**
     * E-tag id.
     */
    LSI_RSPHDR_ETAG,
    /**
     * Expires id.
     */
    LSI_RSPHDR_EXPIRES,
    /**
     * Keep alive message id.
     */
    LSI_RSPHDR_KEEP_ALIVE,
    /**
     * Lasst modified date id.
     */
    LSI_RSPHDR_LAST_MODIFIED,
    /**
     * Location id.
     */
    LSI_RSPHDR_LOCATION,
    /**
     * Litespeed location id.
     */
    LSI_RSPHDR_LITESPEED_LOCATION,
    /**
     * Cashe control id.
     */
    LSI_RSPHDR_LITESPEED_CACHE_CONTROL,
    /**
     * Pragma id.
     */
    LSI_RSPHDR_PRAGMA,
    /**
     * Proxy connection id.
     */
    LSI_RSPHDR_PROXY_CONNECTION,
    /**
     * Server id.
     */
    LSI_RSPHDR_SERVER,
    /**
     * Set cookie id.
     */
    LSI_RSPHDR_SET_COOKIE,
    /**
     * CGI status id.
     */
    LSI_RSPHDR_CGI_STATUS,
    /**
     * Transfer encoding id.
     */
    LSI_RSPHDR_TRANSFER_ENCODING,
    /**
     * Vary id.
     */
    LSI_RSPHDR_VARY,
    /**
     * Authentication id.
     */
    LSI_RSPHDR_WWW_AUTHENTICATE,
    /**
     * LiteSpeed Cache id.
     */
    LSI_RSPHDR_LITESPEED_CACHE,
    /**
     * LiteSpeed Purge id.
     */
    LSI_RSPHDR_LITESPEED_PURGE,
    /**
     * LiteSpeed Tag id.
     */
    LSI_RSPHDR_LITESPEED_TAG,
    /**
     * LiteSpeed Vary id.
     */
    LSI_RSPHDR_LITESPEED_VARY,
    /**
     * Powered by id.
     */
    LSI_RSPHDR_X_POWERED_BY,
    /**
     * Header end id.
     */
    LSI_RSPHDR_END,
    /**
     * Header unknown id.
     */
    LSI_RSPHDR_UNKNOWN = LSI_RSPHDR_END

};


enum LSI_COMPRESS_METHOD
{
    LSI_NO_COMPRESS = 0,
    LSI_GZIP_COMPRESS,
    LSI_BR_COMPRESS,
};


/**
 * @enum LSI_ACL_LEVEL
 * @brief The access control level definitions.
 * @details Used as the value for client access control level when evaulated
 * against a ACL list.
 * @see   get_client_access_level
 * @since 1.1
 * @ingroup security
 */
enum LSI_ACL_LEVEL
{
    /**
     * ACL denies access.
     */
    LSI_ACL_DENY,
    /**
     * ACL allows access.
     */
    LSI_ACL_ALLOW,
    /**
     * ACL defines a trust relationship.
     */
    LSI_ACL_TRUST,
    /**
     * ACL block access.
     */
    LSI_ACL_BLOCK,
    /**
     * ACL captcha verified.
     */
    LSI_ACL_CAPTCHA
};

/**
 * @enum LSI_REQ_METHOD
 * @brief The request method specified by the remote.
 * @details The original HTTP method as specified by the remote system.
 * @ingroup reqmethod
 */

enum LSI_REQ_METHOD
{
    /**
     * A type not in this list
     */
    LSI_METHOD_UNKNOWN = 0,
    /**
     * The communication options for the target resource
     */
    LSI_METHOD_OPTIONS,
    /**
     * GET is the most common type of method, the GET method requests a
     * representation of the specified resource.  Requests using GET should only
     * retrieve data.
     */
    LSI_METHOD_GET,
    /**
     * Asks for a response identical to GET but without the response body
     */
    LSI_METHOD_HEAD,
    /**
     * Used to submit an entity to the specified resource, often causing a
     * change in state or side effects on the server.  Often used to transmit
     * a file.
     */
    LSI_METHOD_POST,
    /**
     * Replaces all current representations of the target resource with the
     * request payload.
     */
    LSI_METHOD_PUT,
    /**
     * Deletes the specified resource.
     */
    LSI_METHOD_DELETE,
    /**
     * Performs a message loop-back test along the path to the target resource.
     */
    LSI_METHOD_TRACE,
    /**
     * Establishes a tunnel to the server identified by the target resource.
     */
    LSI_METHOD_CONNECT,
    /**
     * Equivalent to a copy and then delete.
     */
    LSI_METHOD_MOVE,
    /**
     * Allows set or remove properties of the URI.
     */
    LSI_METHOD_PATCH,
    /**
     * Allows the request of the properties of the URI.
     */
    LSI_METHOD_PROPFIND,
    /**
     * Allows set or remove properties of the URI.
     */
    LSI_METHOD_PROPPATCH,
    /**
     * Creates a new collection resource at the location specified by the
     * Request-URI.
     */
    LSI_METHOD_MKCOL,
    /**
     * Creates a duplicate of the source resource identified by the Request-URI.
     */
    LSI_METHOD_COPY,
    /**
     * Take out a lock of any access type and to refresh an existing lock.
     */
    LSI_METHOD_LOCK,
    /**
     * Remove the lock of an access type.
     */
    LSI_METHOD_UNLOCK,
    /**
     * Create a version-controlled resource a the request-URL.
     */
    LSI_METHOD_VERSION_CONTROL,
    /**
     * An extensible mechanism for obtaining information about a resource.
     */
    LSI_METHOD_REPORT,
    /**
     * Lets a DAV controlled resource to be checked in.
     */
    LSI_METHOD_CHECKIN,
    /**
     * Lets a DAV controlled resource to be checked out.
     */
    LSI_METHOD_CHECKOUT,
    /**
     * Cancels a previous checkout.
     */
    LSI_METHOD_UNCHECKOUT,
    /**
     * Changes the state of a checked in version controlled resource.
     */
    LSI_METHOD_UPDATE,
    /**
     * Creates a new DAV controlled workspace resource.
     */
    LSI_METHOD_MKWORKSPACE,
    /**
     * Modifies the labels that select a DAV controlled version.
     */
    LSI_METHOD_LABEL,
    /**
     * Does a logical merge of a specified version into a version controlled
     * resource (target).
     */
    LSI_METHOD_MERGE,
    /**
     * Puts a DAV controlled collection under baseline control.
     */
    LSI_METHOD_BASELINE_CONTROL,
    /**
     * Creates a new DAV controlled activity resource.
     */
    LSI_METHOD_MKACTIVITY,
    /**
     * Modifies the DAV collection identified by the Request-URI.
     */
    LSI_METHOD_BIND,
    /**
     * Requests a server side DAV search.
     */
    LSI_METHOD_SEARCH,
    /**
     * Clears the page's server cache and the page is rebuilt.
     */
    LSI_METHOD_PURGE,
    /**
     * Refresh the web or pricture frame after a given time interval.
     */
    LSI_METHOD_REFRESH,
    /**
     * Invalidates content in the server cache.
     */
    LSI_METHOD_BAN,
    /**
     * Identifies the last element in the list.
     */
    LSI_METHOD_METHOD_END,
};

/**
 * @enum LSI_DATA_TYPE
 * @brief The type of data being requested in a foreach query.
 * @details foreach is used to obtain a complete list of items by redirecting
 * the items one-by-one to a callback function.
 * @ingroup query
 */
typedef enum LSI_DATA_TYPE
{
    /**
     * The request header data components.
     */
    LSI_DATA_REQ_HEADER,
    /**
     * The request variables.
     */
    LSI_DATA_REQ_VAR,
    /**
     * The full list of environment variables for a request.
     */
    LSI_DATA_REQ_ENV,
    /**
     * PHP variables are considered "special" environment variables.
     */
    LSI_DATA_REQ_SPECIAL_ENV,
    /**
     * The CGI headers for a request.
     */
    LSI_DATA_REQ_CGI_HEADER,
    /**
     * The cookies for a request.
     */
    LSI_DATA_REQ_COOKIE,
    /**
     * The contents of the response header.
     */
    LSI_DATA_RESP_HEADER,
} LSI_DATA_TYPE;

/*
 * Forward Declarations (documented below)
 */

typedef struct lsi_module_s     lsi_module_t;
typedef struct lsi_api_s        lsi_api_t;
typedef struct lsi_param_s      lsi_param_t;
typedef struct lsi_hookinfo_s   lsi_hookchain_t;
// typedef struct evtcbhead_s      lsi_session_t;
typedef struct evtcbtail_s      lsi_session_t;
typedef struct lsi_serverhook_s lsi_serverhook_t;
typedef struct lsi_reqhdlr_s    lsi_reqhdlr_t;
typedef struct lsi_confparser_s lsi_confparser_t;

/**
 * @typedef lsi_cb_sess_param_t
 * @brief lsi_cb_sess_param_t can be used in a foreach query as the 'arg' to
 * allow return of both the session pointer and additional callback specific
 * data.
 * @ingroup query
 */
typedef struct lsi_cb_sess_param_s
{
    /**
     * @brief the session pointer.
     */
    lsi_session_t *session;
    /**
     * @brief any argument to be passed to the callback function.
     */
    void *cb_arg;
} lsi_cb_sess_param_t;

/**
 * @typedef lsi_foreach_cb
 * @brief lsi_foreach_cb is the template for 'foreach' query callback functions.
 * @ingroup query
 */
typedef void (*lsi_foreach_cb)(int key_id, const char * key, int key_len,
                               const char * value, int val_len, void * arg);


/**
 * @typedef lsi_callback_pf
 * @brief The module callback function and its parameters.
 * @since 1.0
 * @ingroup module
 */
typedef int (*lsi_callback_pf)(lsi_param_t *);

/**
 * @typedef lsi_datarelease_pf
 * @brief The memory release callback function for the user module data.
 * @since 1.0
 * @ingroup data_storage
 *
 */
typedef int (*lsi_datarelease_pf)(void *);

/**
 * @typedef lsi_timercb_pf
 * @brief The timer callback function for the set timer feature.
 * @ingroup timer
 * @since 1.0
 */
typedef void (*lsi_timercb_pf)(const void *);



/**************************************************************************************************
 *                                       API Structures
 **************************************************************************************************/


/**
 * @typedef lsi_param_t
 * @brief Callback parameters passed to the callback functions.
 * @since 1.0
 * @ingroup module
 **/
struct lsi_param_s
{
    /**
     * @brief session is a pointer to the session.
     * @details Read-only. Useful for logging or other session specific functions
     * once the session has been established.
     * @since 1.0
     */
    const lsi_session_t      *session;

    /**
     * @brief hook_chain is a pointer to the struct lsi_hookinfo_t.
     * @details Read-only. All registered callback functions for this hook point.
     * Can be useful to allow examination of the depth of the chain.
     * @since 1.0
     */
    const lsi_hookchain_t    *hook_chain;

    /**
     * @brief cur_hook is a pointer to the current hook.
     * @details Read-only. The current callback function.
     * @since 1.0
     */
    const void               *cur_hook;

    /**
     * @brief ptr1 is a pointer to the first parameter.
     * @details Usually a buffer, refer to the LiteSpeed Module Developer's Guide
     * for details in the relevant callback function descriptions and the
     * Callback Function Parameters section.
     * @since 1.0
     */
    const void         *ptr1;

    /**
     * @brief len1 is the length of the first parameter.
     * @details When ptr1 is an input, the input data size.
     * If ptr1 points to a result buffer, then len1 is the maximum size of
     * result data that the buffer can hold.
     * @since 1.0
     */
    int                 len1;

    /**
     * @brief flag_out is a pointer to the output flags.
     * @details Used for passing a flag to caller.  See the LSI_CBFO_* flags in
     * the #LSI_CB_FLAGS enum
     * @see LSI_CB_FLAGS
     * @since 1.0
     */
    int                *flag_out;

    /**
     * @brief flag_in represents the input flags.
     * @details Input flag.  See the LSI_CBFI_* flags in
     * the #LSI_CB_FLAGS enum
     * @see LSI_CB_FLAGS
     * @since 1.0
     */
    int                 flag_in;
};


/**
 * @def LSI_HDLR_DEFAULT_POOL
 * @brief For multi-threaded support, when registering a thread cleanup function
 * with register_thread_cleanup specifies to use the default pool.
 * Multi-threaded support is currently only supported for request handlers and
 * this value must be specified or some user default thread pool as the
 * #lsi_reqhdlr_t::ts_hdlr_ctx member.
 * @ingroup session
 */
#define LSI_HDLR_DEFAULT_POOL    (void *)(-1L)

/**
 * @typedef lsi_reqhdlr_t
 * @brief Pre-defined handler functions.  This structure is generally defined
 * as static, preinitialized, and is then the #lsi_module_t::reqhandler value
 * of the #lsi_module_t MNAME data definition.
 * @since 1.0
 * @ingroup session
 */
struct lsi_reqhdlr_s
{
    /**
     * @brief begin_process is called when the server starts to process a
     * request.  A request handler can do all required functions in just this
     * one callback if desired.
     * @since 1.0
     */
    int (*begin_process)(const lsi_session_t *pSession);

    /**
     * @brief on_read_req_body is called on a READ event with a large request
     * body (but will not be called for an empty or small request body).
     * @details on_read_req_body is called when a request has a large request
     * body that was not read completely.  If not provided, this function should
     * be set to NULL.  The default function will execute and return 0.
     * @since 1.0
     */
    int (*on_read_req_body)(const lsi_session_t *pSession);

    /**
     * @brief on_write_resp is optionally called on a WRITE event with a large
     * response body (it may not be called with no response body or a small
     * one).
     * @details on_write_resp is called when the server gets a large response
     * body where the response did not write completely.  If not provided, set
     * it to NULL.  The default function will execute and return #LSI_RSP_DONE.
     * @since 1.0
     */
    int (*on_write_resp)(const lsi_session_t *pSession);

    /**
     * @brief on_clean_up is called when the server core is done with the
     * handler, asking the handler to perform clean up.
     * @details It is called after a handler called end_resp(), or if the server
     * needs to switch handler; for example, return an error page or perform an
     * internal redirect while the handler is processing the request.
     * @since 1.0
     */
    int (*on_clean_up)(const lsi_session_t *pSession);

    /**
     * @brief mthdlr_ctx is a opaque pointer can be used to store handler level
     * context.  To enable multi-threaded support for the request handler this
     * value must be entered, and is usually #LSI_HDLR_DEFAULT_POOL
     * @details It can be used for thread pool object, or scripting engine.
     * @since 1.0
     */
    void *ts_hdlr_ctx;

    /**
     * @brief enqueue_req is called when the server need to pass a request to
     * a script engine that managing its own thread pool, job queue.
     * @details It is used to define the request handler callback function.
     * @since 1.0
     */
    int (*ts_enqueue_req)(void *ts_hdlr_ctx, const lsi_session_t *pSession);

    /**
     * @brief cancel_req is called when the server needs to cancel a request.
     * @details It is used to define the request handler callback function.
     * @since 1.0
     */
    int (*ts_cancel_req)(const lsi_session_t *pSession);
};


/**
 * @typedef lsi_config_key_t
 * @brief Configuration defines, not required to be sorted.
 * @details This is specified as an array with a { NULL, 0, 0 } terminating
 * element.
 * @ingroup config
 *
 */
typedef struct lsi_config_key_s
{
    /**
     * @brief config_key the unique configuration key
     */
    const char *config_key;

    /**
     * @brief id is any module developer assigned value greater than 0. 0 can
     * be used for compatibility with earlier versions.
     */
    uint16_t id;

    /**
     * @brief level is one of the #LSI_CFG_LEVEL enum values (as an int) to
     * specify the level the configuration entry will be used at.  0 can be used
     * for compatibility with earlier versions and will represent any level.
     */
     uint16_t level;
} lsi_config_key_t;


/**
 * @typedef module_param_info_t
 * @brief Module specific values to be assigned early as part of configuration
 * @details Often returned as an array of these objects.
 * @ingroup config
 */
typedef struct module_param_info_st
{
    /**
     * @brief key_index is the 0 based index of the array.
     */
    uint16_t    key_index;
    /**
     * @brief val_len is the length of the value
     */
    uint16_t    val_len;
    /**
     * @brief val is the value of the configuration entry.
     */
    char        *val;
} module_param_info_t;

/**
 * @typedef lsi_confparser_t
 * @brief Contains functions which are used to define parse and release
 * functions for the user defined configuration parameters.
 * @details The parse functions will be called during module load before the
 * module initialization callback.  Only if there are one or more valid module
 * parameters which are space separated config_key title/value pairs will a
 * function be called.  The free function will be called on session end.
 * However, it is recommended that the user always performs a manual release of
 * all allocated memory using the session_end filter hooks.
 * @since 1.0
 * @ingroup config
 */

struct lsi_confparser_s
{
    /**
     * @brief parse_config is a callback function for the server to call to
     * parse the user defined parameters and return a pointer to the user
     * defined configuration data structure.  Only if there are one or more
     * module parameters, which are space separated config_key title/value pairs
     * will this function be called.
     * @ingroup config
     *
     * @since 1.0
     * @param[in] params - the array which holds the #module_param_info_t
     * elements of parsed parameters.
     * @param[in] param_count - the count of the number of #module_param_info_t
     * elements in params above.
     * @param[in] initial_config - a pointer to the default configuration
     * inherited from the parent level.
     * @param[in] level - applicable level from enum #LSI_CFG_LEVEL.
     * @param[in] name - name of the Server/Listener/VHost or URI of the
     * Context.
     * @return a pointer to a the user-defined configuration data, which
     * combines initial_config with settings in param; if both param and
     * initial_config are NULL, a hard-coded default configuration value should
     * be returned.  It is valid to return a static here which would mean that
     * the free_config function should either not be specified or return
     * immediately.
     */
    void  *(*parse_config)(module_param_info_t *params, int param_count,
                           void *initial_config, int level, const char *name);

    /**
     * @brief free_config is a callback function for the server to call to
     * release a pointer to the user defined data.
     * @since 1.0
     * @ingroup config
     * @param[in] config - a pointer to configuration data structure to be
     * released.  If a static is used for the configuration values, then this
     * function should either do nothing or not be defined.
     */
    void (*free_config)(void *config);

    /**
     * @brief config_keys is a { NULL, 0, 0 } terminated array of
     * lsi_config_key_t.  It is used to filter the module user parameters by the
     * server while parsing the configuration.
     * @since 1.0
     * @ingroup config
     */
    lsi_config_key_t *config_keys;
};


/**
 * @typedef lsi_serverhook_t
 * @brief Global hook point specification.
 * @details An array of these entries, terminated by the #LSI_HOOK_END entry,
 * at lsi_module_t::_serverhook defines the global hook points of a module.
 * @since 1.0
 * @ingroup hooks
 */
struct lsi_serverhook_s
{
    /**
     * @brief specifies the hook point using level definitions from enum
     * #LSI_HKPT_LEVEL.
     * @since 1.0
     */
    int             index;

    /**
     * @brief points to the callback hook function.
     * @since 1.0
     */
    lsi_callback_pf cb;

    /**
     * @brief defines the priority of this hook function within a function
     * chain.
     * @since 1.0
     */
    short           priority;

    /**
     * @brief additive hook point flags using level definitions from enum
     * #LSI_HOOK_FLAG.
     * @since 1.0
     */
    short           flag;

};


/**
 * @def LSI_HOOK_END
 * @brief Termination entry for the array of lsi_serverhook_t entries
 * at lsi_module_t::_serverhook.
 * @since 1.0
 * @ingroup hooks
 */
#define LSI_HOOK_END    {0, NULL, 0, 0}

/**
 * @def LSI_MODULE_RESERVED_SIZE
 * @brief The amount of space left at the end of the lsi_module_t structure for
 * alignment.
 * @ingroup module
 */
#define LSI_MODULE_RESERVED_SIZE    ((3 * sizeof(void *)) \
                                     + ((LSI_HKPT_TOTAL_COUNT + 2) * sizeof(int32_t)) \
                                     + (LSI_DATA_COUNT * sizeof(int16_t)))

/**
 * @def MODULE_LOG_LEVEL
 * @brief The logging level currently active.
 * @param[in] x - module handle, usually &NAME
 * @ingroup logging
 */
#define MODULE_LOG_LEVEL(x)      (*(int32_t *)(x->reserved))


/**
 * @typedef lsi_module_t
 * @brief Defines an LSIAPI module, this struct MUST be provided in the module
 * code.
 * @details There must be a minimum of one of these defined, it is typically
 * named MNAME and is defined statically.  The name of the module must match the
 * name of this variable, thus a define earlier should define MNAME as the name
 * of the module.
 * @since 1.0
 * @ingroup module
 */
struct lsi_module_s
{
    /**
     * @brief identifies an LSIAPI module. It is required and must be set to
     * #LSI_MODULE_SIGNATURE.
     * @since 1.0
     */
    int64_t                  signature;

    /**
     * @brief an optional function pointer that will be called after the module
     * is loaded.  Used to initialize module data.
     * @since 1.0
     */
    int (*init_pf)(lsi_module_t *module);

    /**
     * @brief reqhandler is required only if this module is a request handler.
     * @since 1.0
     */
    lsi_reqhdlr_t           *reqhandler;

    /**
     * @brief optionally contains functions which are used to parse user defined
     * configuration parameters and to release the memory block.
     * @since 1.0
     */
    lsi_confparser_t        *config_parser;

    /**
     * @brief optional information about this module set by the developer;
     * it can contain module version and/or version(s) of libraries used by this
     * module.
     * @since 1.0
     */
    const char              *about;

    /**
     * @brief optional information for global server level hook functions.
     * @since 1.0
     */
    lsi_serverhook_t        *serverhook;

    /**
     * @brief reserved is not used at this time.
     */
    int32_t                  reserved[(LSI_MODULE_RESERVED_SIZE + 3) / 4 ];
};


typedef struct lsi_subreq_s
{
    int             m_flag;
    int             m_method;
    const char     *m_pUri;
    int             m_uriLen;
    const char     *m_pQs;
    int             m_qsLen;
    const char     *m_pReqBody;
    int32_t         m_reqBodyLen;
} lsi_subreq_t;



/**************************************************************************************************
 *                                       API Functions
 **************************************************************************************************/

/**
 * @typedef lsi_api_t
 * @brief LSIAPI function set.
 * @details Exported as the global variable g_api and each function is
 * referenced as a value pointed to by this variable
 * (like g_api->get_server_root()).
 * @since 1.0
 **/

struct lsi_api_s
{
    /**************************************************************************************************************
     *                                        SINCE LSIAPI 1.0                                                    *
     * ************************************************************************************************************/

    /**
     * @brief get_server_root is used to get the server root path.  Most
     * applications will not need to use this function.
     *
     * @since 1.0
     *
     * @return A \\0-terminated string containing the path to the server root
     * directory.
     * @ingroup module
     */
    const char *(*get_server_root)();

    /**
     * @brief log is used to write the formatted log to the error log associated
     * with a session.  It is strongly recommended that module developers use
     * the LSM_XXX macros for all logging as it is fully portable with future
     * versions.
     * @details session ID will be added to the log message automatically when
     * session is not NULL.  This function will not add a trailing \\n to the
     * end.
     * @ingroup logging
     * @since 1.0
     *
     * @param[in] pSession - current session, log file, and session ID are based
     * on session.
     * @param[in] level - enum defined in log level definitions #LSI_LOG_LEVEL.
     * @param[in] fmt - formatted string.
     */
    void (*log)(const lsi_session_t *pSession, int level, const char *fmt, ...)
#if __GNUC__
        __attribute__((format(printf, 3, 4)))
#endif
        ;

    /**
     * @brief vlog is used to write the formatted log to the error log
     * associated with a session.    It is strongly recommended that module
     * developers use the LSM_XXX macros for all logging as it is fully portable
     * with future versions.
     * @details session ID will be added to the log message automatically when
     * the session is not NULL.
     * @ingroup logging
     * @since 1.0
     *
     * @param[in] pSession - current session, log file and session ID are based
     * on session.
     * @param[in] level - enum defined in log level definitions #LSI_LOG_LEVEL.
     * @param[in] fmt - formatted string.
     * @param[in] vararg - the varying argument list.
     * @param[in] no_linefeed - 1 = do not add \\n at the end of message;
     * 0 = add \\n
     */
    void (*vlog)(const lsi_session_t *pSession, int level, const char *fmt,
                 va_list vararg, int no_linefeed);

    /**
     * @brief lograw is used to write additional log messages to the error log
     * file associated with a session.  No timestamp, log level, session ID
     * prefix is added; just the data in the buffer is logged into log files.
     * This is usually used for adding more content to previous log messages.
     * @ingroup logging
     * @since 1.0
     *
     * @param[in] session - log file is based on session; if set to NULL, the
     * default log file is used.
     * @param[in] buf - data to be written to log file.
     * @param[in] len - the size of data to be logged.
     */
    void (*lograw)(const lsi_session_t *session, const char *buf, int len);

    /**
     * @brief get_config is used to get the user defined module parameters which
     * are parsed by the callback parse_config and pointed to in the struct
     * lsi_confparser_t.
     * @ingroup config
     * @since 1.0
     *
     * @param[in] session - a pointer to the session, or use NULL for the server
     * level.
     * @param[in] module - a pointer to an lsi_module_t struct.
     * @return a pointer to the user-defined configuration data.
     */
    void *(*get_config)(const lsi_session_t *session,
                        const lsi_module_t *module);


    /**
     * @brief enable_hook is used to set the flag of a hook function in a
     * certain level to enable or disable the function.  This should only be
     * used after a session is already created and applies only to this session.
     * There are L4 level and HTTP level session hooks.
     * @ingroup hooks
     * @since 1.0
     *
     * @param[in] session - a pointer to the session, obtained from callback
     * parameters.
     * @param[in] pModule - a pointer to the lsi_module_t struct.
     * @param[in] enable - A combination of 1 or more #LSI_HOOK_FLAG values
     * to enable; set to 0 to disable the hook.
     * @param[in] index - A list of indices to set, as defined in the enum of
     *   Hook Point level definitions #LSI_HKPT_LEVEL.
     * @param[in] iNumIndices - The number of indices to set in index.
     * @return -1 on failure.
     */
    int (*enable_hook)(const lsi_session_t *session, const lsi_module_t *pModule,
                       int enable, int *index, int iNumIndices);



    /**
     * @brief get_hook_flag is used to get the #LSI_HOOK_FLAG flag of the hook
     * functions in a given level.
     * @ingroup hooks
     * @since 1.0
     *
     * @param[in] session - a pointer to the session, obtained from callback
     * parameters.
     * @param[in] index - A list of indices to set, as defined in the enum of
     *  Hook Point level definitions #LSI_HKPT_LEVEL.
     * @return integer value of #LSI_HOOK_FLAG
     */
    int (*get_hook_flag)(const lsi_session_t *session, int index);

    /**
     * @brief get_hook_level is used to return the #LSI_HKPT_LEVEL specified for
     * a given parameter definition in the hook chain
     * @ingroup hooks
     * @param[in] pParam - the set of parameters for the hooks in the hook
     * chain.
     * @return integer value of #LSI_HKPT_LEVEL
     */
    int (*get_hook_level)(lsi_param_t *pParam);

    /**
     * @brief get_module is used to retrieve module information associated with
     * a hook point based on callback parameters.
     * @ingroup hooks
     * @since 1.0
     *
     * @param[in] pParam - a pointer to callback parameters.
     * @return NULL on failure, a pointer to the lsi_module_t data structure on
     * success.
     */
    const lsi_module_t *(*get_module)(lsi_param_t *pParam);

    /**
     * @brief init_module_data is used to initialize module data of a certain
     * level(scope).  init_module_data must be called before using
     * set_module_data or get_module_data; it is often called during module
     * initialization.
     * @ingroup data_storage
     * @since 1.0
     *
     * @param[in] pModule - a pointer to the current module defined in
     * lsi_module_t struct.
     * @param[in] cb - a pointer to the user-defined callback function that
     * releases the user data or NULL if module data is to be released manually
     * (recommended).
     * @param[in] level - as defined in the module data level enum
     * #LSI_DATA_LEVEL.
     * @return -1 for wrong level, -2 for already initialized, 0 for success.
     */
    int (*init_module_data)(const lsi_module_t *pModule, lsi_datarelease_pf cb,
                            int level);

    /**
     * @brief Before using FILE type module data, the data must be initialized
     * by calling init_file_type_mdata with the file path.
     * @ingroup data_storage
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the lsi_session_t struct.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] path - a pointer to the path string.
     * @param[in] pathLen - the length of the path string.
     * @return -1 on failure, file descriptor on success.
     */
    int (*init_file_type_mdata)(const lsi_session_t *pSession,
                                const lsi_module_t *pModule, const char *path,
                                int pathLen);

    /**
     * @brief set_module_data is used to set the data for a session.  For
     * request handlers the data space is allocated and this function called
     * during the #lsi_reqhdlr_t::begin_process function.
     * @ingroup data_storage
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the lsi_session_t struct.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] level - as defined in the module data level enum
     * #LSI_DATA_LEVEL.
     * @param[in] sParam - a pointer to user defined and allocated data.
     * @return -1 for bad level or no release data callback function, 0 on
     * success.
     */
    int (*set_module_data)(const lsi_session_t *pSession,
                           const lsi_module_t *pModule, int level,
                           void *sParam);

    /**
     * @brief get_module_data gets the module data which was set by
     * set_module_data.  The return value is a pointer to the user's own data
     * structure.  For request handlers this function is called during all
     * processing to obtain the common data.
     * @ingroup data_storage
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the lsi_session_t struct.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] level - as defined in the module data level enum
     * #LSI_DATA_LEVEL.
     * @return NULL on failure, a pointer to the user defined data on success.
     */
    void *(*get_module_data)(const lsi_session_t *pSession,
                             const lsi_module_t *pModule, int level);

    /**
    * @brief get_cb_module_data gets the module data related to the current
    * callback.  The return value is a pointer to the user's own data structure.
    * @ingroup data_storage
    * @since 1.0
    *
    * @param[in] pParam - a pointer to callback parameters.
    * @param[in] level - as defined in the module data level enum
    * #LSI_DATA_LEVEL.
    * @return NULL on failure, a pointer to the user defined data on success.
    */
    void *(*get_cb_module_data)(const lsi_param_t *pParam, int level);

    /**
     * @brief free_module_data is to be called when the user needs to free the
     * module data immediately.  It is not used by the web server to call the
     * release callback later.
     * @ingroup data_storage
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the lsi_session_t struct.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] level - as defined in the module data level enum
     * #LSI_DATA_LEVEL.
     * @param[in] cb - a pointer to the user-defined callback function that
     * releases the user data.
     */
    void (*free_module_data)(const lsi_session_t *pSession,
                             const lsi_module_t *pModule, int level,
                             lsi_datarelease_pf cb);


    /**
     * @brief get_req_method obtains the request method specified by the source
     * system.
     * @ingroup reqmethod
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the lsi_session_t struct.
     * @returns #LSI_REQ_METHOD
     */
    enum LSI_REQ_METHOD (*get_req_method)(const lsi_session_t *pSession);

    /**
     * @brief get_req_vhost obtains information about the virtual host from the
     * session information.
     * @ingroup vhost
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the lsi_session_t struct.
     * @returns An opaque pointer to the virtual host or NULL if there is not
     * one.
     */
    const void *(*get_req_vhost)(const lsi_session_t *pSession);

    /**
     * @brief stream_writev_next needs to be called in the #LSI_HKPT_L4_SENDING
     * hook point level just after it finishes the action and needs to call the
     * next step.
     * @ingroup hooks
     * @since 1.0
     *
     * @param[in] pParam - the callback parameters to be sent to the current
     * hook callback function.
     * @param[in] iov - the IO vector of data to be written.
     * @param[in] count - the size of the IO vector.
     * @return the return value from the hook filter callback function.
     */
    int (*stream_writev_next)(lsi_param_t *pParam, struct iovec *iov,
                              int count);

    /**
     * @brief stream_read_next needs to be called in filter
     * callback functions registered to the #LSI_HKPT_L4_RECVING and
     * #LSI_HKPT_RCVD_REQ_BODY hook point level
     * to get data from a higher level filter in the chain.
     * @ingroup hooks
     * @since 1.0
     *
     * @param[in] pParam - the callback parameters to be sent to the current
     * hook callback function.
     * @param[in,out] pBuf - a pointer to a buffer provided to hold the read
     * data.
     * @param[in] size - the buffer size.
     * @return the return value from the hook filter callback function.
     */
    int (*stream_read_next)(lsi_param_t *pParam, char *pBuf, int size);

    /**
     * @brief stream_write_next is used to write the response body to the next
     * function in the filter chain of #LSI_HKPT_SEND_RESP_BODY level.
     * This must be called in order to send the response body to the next
     * filter.
     * @ingroup hooks
     * @since 1.0
     *
     * @param[in] pParam - a pointer to the callback parameters.
     * @param[in] buf - a pointer to a buffer to be written.
     * @param[in] buflen - the size of the buffer.
     * @return -1 on failure, return value of the hook filter callback function
     * (size written).
     */
    int (*stream_write_next)(lsi_param_t *pParam, const char *buf, int len);


    /**
     * @brief get_req_raw_headers_length can be used to get the length of the
     * total request headers.
     * @ingroup req_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return length of the request header.
     */
    int (*get_req_raw_headers_length)(const lsi_session_t *pSession);

    /**
     * @brief get_req_raw_headers can be used to store all of the request
     * headers in a given buffer.  If maxlen is too small to contain all the
     * data, it will only copy the amount of the maxlen.
     * @ingroup req_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in,out] buf - a pointer to a buffer provided to hold the header
     * strings.
     * @param[in] maxlen - the size of the buffer.
     * @return - the length of the request header.
     */
    int (*get_req_raw_headers)(const lsi_session_t *pSession, char *buf,
                               int maxlen);

    /**
     * @brief get_req_headers_count can be used to get the count of the request
     * headers.
     * @ingroup req_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return the number of headers in the whole request header.
     */
    int (*get_req_headers_count)(const lsi_session_t *pSession);

    /**
     * @brief get_req_headers can be used to get all the request headers as a
     * vector list.
     * @ingroup req_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[out] iov_key - the IO vector that contains the keys.
     * @param[out] iov_val - the IO vector that contains the values.
     * @param[in] maxIovCount - size of the IO vectors.
     * @return the count of headers in the IO vectors.
     */
    int (*get_req_headers)(const lsi_session_t *pSession, struct iovec *iov_key,
                           struct iovec *iov_val, int maxIovCount);

    /**
     * @brief get_req_header can be used to get a request header based on the
     * given key.
     * @ingroup req_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] key - a pointer to a string describing the header label (key).
     * @param[in] keylen - the size of the string.
     * @param[in,out] valLen - a pointer to the length of the returned string.
     * @return a pointer to the request header key value string.
     */
    const char *(*get_req_header)(const lsi_session_t *pSession,
                                  const char *key, int keyLen, int *valLen);

    /**
     * @brief get_req_header_by_id can be used to get a request header based on
     * the given header id defined in #LSI_REQ_HEADER_ID
     * @ingroup req_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] id - header id defined in #LSI_REQ_HEADER_ID.
     * @param[in,out] valLen - a pointer to the length of the returned string.
     * @return a pointer to the request header key value string.
     */
    const char *(*get_req_header_by_id)(const lsi_session_t *pSession, int id,
                                        int *valLen);

    /**
     * @brief get_req_org_uri is used to get the original URI as delivered in
     * the request, before any processing has taken place.
     * @ingroup uri
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in,out] buf - a pointer to the buffer for the URI string.
     * @param[in] buf_size - max size of the buffer.
     *
     * @return length of the URI string.
     */
    int (*get_req_org_uri)(const lsi_session_t *pSession, char *buf,
                           int buf_size);

    /**
     * @brief get_req_uri can be used to get the URI of a HTTP session.
     * @ingroup uri
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in,out] uri_len - a pointer to int; if not NULL, the length of the
     * URI is returned.
     * @return pointer to the URI string. The string is readonly.
     */
    const char *(*get_req_uri)(const lsi_session_t *pSession, int *uri_len);

    /**
     * @brief get_mapped_context_uri can be used to get the context URI of an
     * HTTP session.
     * @ingroup uri
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in,out] length - the length of the returned string.
     * @return pointer to the context URI string.
     */
    const char *(*get_mapped_context_uri)(const lsi_session_t *pSession,
                                          int *length);

    /**
     * @brief is_req_handler_registered can be used to test if a request handler
     * of a session was already registered or not.
     * @ingroup session
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return 1 on true, and 0 on false.
     */
    int (*is_req_handler_registered)(const lsi_session_t *pSession);

    /**
     * @brief register_req_handler can be used to dynamically register a
     * handler.
     * The scriptLen is the length of the script.  To call this function,
     * the module needs to provide the lsi_reqhdlr_t (not set to NULL).
     * @ingroup session
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] scriptLen - the length of the script name string.
     * @return -1 on failure, 0 on success.
     */
    int (*register_req_handler)(const lsi_session_t *pSession,
                                lsi_module_t *pModule, int scriptLen);

    /**
     * @brief set_handler_write_state can change the calling of on_write_resp()
     * of a module handler.  If the state is 0, the calling will be suspended;
     * otherwise, if it is 1, it will continue to call when not finished.
     * @ingroup session
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] state - 0 to suspend the calling; 1 to continue.
     * @return -1 on error; 0 on success.
     *
     */
    int (*set_handler_write_state)(const lsi_session_t *pSession, int state);

    /**
     * @brief set_timer sets a timer.
     * @ingroup timer
     * @since 1.0
     *
     * @param[in] timeout_ms - Timeout in ms.
     * @param[in] repeat - 1 to repeat, 0 for no repeat.
     * @param[in] timer_cb - Callback function to be called on timeout.
     * @param[in] timer_cb_param - Optional parameter for the callback function.
     * @return Timer ID if successful, LS_FAIL if it failed.
     *
     */
    int (*set_timer)(unsigned int timeout_ms, int repeat,
                     lsi_timercb_pf timer_cb, const void *timer_cb_param);

    /**
     * @brief remove_timer removes a timer.
     * @ingroup timer
     * @since 1.0
     *
     * @param[in] time_id - timer id
     * @return 0.
     *
     */
    int (*remove_timer)(int time_id);


    /**
     * @brief create_event is an internal function
     */
    long (*create_event)(evtcb_pf cb, const lsi_session_t *pSession,
                         long lParam, void *pParam, int nowait);
    /**
     * @brief create_session_resume_event is an internal function
     */
    long (*create_session_resume_event)(const lsi_session_t *session,
                                        lsi_module_t *pModule);

    /**
     * @brief get_event_obj is an internal function
     */
    long (*get_event_obj)(evtcb_pf cb, const lsi_session_t *pSession,
                         long lParam, void *pParam);

    /**
     * @brief cancel_event is an internal function
     */
    void (*cancel_event)(const lsi_session_t *pSession, long event_obj);

    /**
     * @brief schedule_event is an internal function
     */
    void (*schedule_event)(long event_obj, int nowait);

    /**
     * @brief schedule_remove_session_cbs_event is an internal function
     */
    long (*schedule_remove_session_cbs_event)(const lsi_session_t *session);


    /**
     * @brief get_req_cookies is used to get all the request cookies.
     * @ingroup req_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in,out] len - a pointer to the length of the cookies string.
     * @return a pointer to the cookie key string.
     */
    const char *(*get_req_cookies)(const lsi_session_t *pSession, int *len);

    /**
     * @brief get_req_cookie_count is used to get the request cookies count.
     * @ingroup req_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return the number of cookies.
     */

    int (*get_req_cookie_count)(const lsi_session_t *pSession);

    /**
     * @brief get_cookie_value is to get a cookie based on the cookie name.
     * @ingroup req_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] cookie_name - a pointer to the cookie name string.
     * @param[in] nameLen - the length of the cookie name string.
     * @param[in,out] valLen - a pointer to the length of the cookie string.
     * @return a pointer to the cookie string.
     */
    const char *(*get_cookie_value)(const lsi_session_t *pSession,
                                    const char *cookie_name, int nameLen,
                                    int *valLen);

    /**
     * @brief get_cookie_by_index is to get a cookie based on a 0 based index.
     * @ingroup req_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] index - a index of the list of cookies parsed for current
     * request.
     * @param[out] cookie - a pointer to the cookie name string.  Note that this
     * is not a simple C string, but a ls_strpair_t pointer.
     * @return 1 if cookie is avaialble, 0 if index is out of range.
     */
    int (*get_cookie_by_index)(const lsi_session_t *pSession, int index,
                               ls_strpair_t *cookie);

    /**
     * @brief get_client_ip is used to get the request ip address.
     * @ingroup environ
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in,out] len - a pointer to the length of the IP string.
     * @return a pointer to the IP string.
     */
    const char *(*get_client_ip)(const lsi_session_t *pSession, int *len);

    /**
     * @brief get_req_query_string is used to get the request query string.
     * @ingroup uri
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in,out] len - a pointer to the length of the query string.
     * @return a pointer to the query string.
     */
    const char *(*get_req_query_string)(const lsi_session_t *pSession,
                                        int *len);

    /**
     * @brief get_req_var_by_id is used to get the value of a server variable
     * and environment variable by the env type. The caller is responsible for
     * managing the buffer holding the value returned.
     * @ingroup req_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] id - enum #LSI_REQ_VARIABLE defined as LSIAPI request variable
     * ID.
     * @param[in,out] val - a pointer to the allocated buffer holding value
     * string.
     * @param[in] maxValLen - the maximum size of the variable value string.
     * @return the length of the variable value string.
     */
    int (*get_req_var_by_id)(const lsi_session_t *pSession, int id, char *val,
                             int maxValLen);

    /**
     * @brief get_req_env is used to get the value of a server variable and
     * environment variable based on the name.  It will also get the env that is
     * set by set_req_env.
     * @ingroup environ
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] name - a pointer to the variable name string.
     * @param[in] nameLen - the length of the variable name string.
     * @param[in,out] val - a pointer to the variable value string.
     * @param[in] maxValLen - the maximum size of the variable value string.
     * @return the length of the variable value string.
     */
    int (*get_req_env)(const lsi_session_t *pSession, const char *name,
                       unsigned int nameLen, char *val, int maxValLen);

    /**
     * @brief set_req_env is used to set a request environment variable.
     * @ingroup environ
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] name - a pointer to the variable name string.
     * @param[in] nameLen - the length of the variable name string.
     * @param[in] val - a pointer to the variable value string.
     * @param[in] valLen - the size of the variable value string.
     */
    void (*set_req_env)(const lsi_session_t *pSession, const char *name,
                        unsigned int nameLen, const char *val, int valLen);

    /**
     * @brief register_env_handler is used to register a callback with a
     * set_req_env defined by an env_name, so that when such an env is set by
     * set_req_env or rewrite rule, the registered callback will be called.
     * @ingroup environ
     * @since 1.0
     *
     * @param[in] env_name - the string containing the environment variable
     * name.
     * @param[in] env_name_len - the length of the string.
     * @param[in] cb - a pointer to the user-defined callback function
     * associated with the environment variable.
     * @return Not used.
     */
    int (*register_env_handler)(const char *env_name,
                                unsigned int env_name_len, lsi_callback_pf cb);

    /**
     * @brief get_uri_file_path will get the real path mapped to the request
     * URI.
     * @ingroup uri
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] uri - a pointer to the URI string.
     * @param[in] uri_len - the length of the URI string.
     * @param[in,out] path - a pointer to the path string.
     * @param[in] max_len - the max length of the path string.
     * @return the length of the path string.
     */
    int (*get_uri_file_path)(const lsi_session_t *pSession, const char *uri,
                             int uri_len, char *path, int max_len);

    /**
     * @brief set_uri_qs changes the URI and Query String of the current
     * session; perform internal/external redirect.
     * @ingroup uri
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] action - action to be taken to URI and Query String, as
     * defined by #LSI_URL_OP, optionally combined with one of the Query String
     * qualifiers (defines with _QS_ in the name), and optionally
     * #LSI_URL_ENCODED.
     * - #LSI_URL_NOCHANGE - (action) do not change the URI.
     * - #LSI_URL_REWRITE - (action) rewrite a new URI and use for processing.
     * - #LSI_URL_REDIRECT_INTERNAL - (action) internal redirect, as if the
     * server received a new request.
     * - #LSI_URL_REDIRECT_301, #LSI_URL_REDIRECT_302, #LSI_URL_REDIRECT_303,
     * #LSI_URL_REDIRECT_307 - (action) external redirect.
     * - #LSI_URL_QS_NOCHANGE - (query string) do not change the Query String
     * (#LSI_URL_REWRITE only).
     * - #LSI_URL_QS_APPEND - (query string) append to the Query String.
     * - #LSI_URL_QS_SET - (query string) set the Query String.
     * - #LSI_URL_QS_DELETE - (query string) delete the Query String.
     * - #LSI_URL_ENCODED (URL encoding) if encoding has been applied to the
     * URI.
     * @param[in] uri - a pointer to the URI string.
     * @param[in] len - the length of the URI string.
     * @param[in] qs -  a pointer to the Query String.
     * @param[in] qs_len - the length of the Query String.
     * @return -1 on failure, 0 on success.
     * @note #LSI_URL_QS_NOCHANGE is only valid with #LSI_URL_REWRITE, in which
     * case qs and qs_len MUST be NULL.  In all other cases, a NULL specified
     * Query String has the effect of deleting the resultant Query String
     * completely.  In all cases of redirection, if the Query String is part of
     * the target URL, qs and qs_len must be specified, since the original Query
     * String is NOT carried over.
     *
     * \b Example of external redirection, changing the URI and setting a new
     * Query String:
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
    int (*set_uri_qs)(const lsi_session_t *pSession, int action,
                      const char *uri, int uri_len, const char *qs, int qs_len);

    /**
     * @brief get_req_content_length is used to get the content length of the
     * request body.
     * @ingroup req_body
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return content length.
     */
    int64_t (*get_req_content_length)(const lsi_session_t *pSession);

    /**
     * @brief read_req_body is used to get the request body to a given buffer.
     * @ingroup req_body
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in,out] buf - a pointer to a buffer provided to hold the body.
     * @param[in] buflen - size of the buffer.
     * @return length of the request body.
     */
    int (*read_req_body)(const lsi_session_t *pSession, char *buf, int bufLen);

    /**
     * @brief is_req_body_finished is used to ensure that all the request body
     * data has been accounted for.
     * @ingroup req_body
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return 0 false, 1 true.
     */
    int (*is_req_body_finished)(const lsi_session_t *pSession);

    /**
     * @brief set_req_wait_full_body is used to make the server wait to call
     * begin_process until after the ENTIRE request body is received.
     * @details If the user calls this function within begin_process, the
     * server will call begin_process and on_read_req_body only after the full
     * request body is received.  Please refer to the LiteSpeed Module
     * Developer's Guide for a more in-depth explanation of the purpose of this
     * function.  It should not be used indescriminately as it may reduce
     * performance.
     * @ingroup req_body
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return 0 if no error; -1 for an error.
     */
    int (*set_req_wait_full_body)(const lsi_session_t *pSession);

    /**
     * @brief set_resp_wait_full_body is used to make the server wait for
     * the whole response body before starting to send response back to the
     * client.
     * @details If this function is called before the server sends back any
     * response data, the server will wait for the whole response body. If it is
     * called after the server begins sending back response data, the server
     * will stop sending more data to the client until the whole response body
     * has been received.  Please refer to the LiteSpeed Module Developer's
     * Guide for a more in-depth explanation of the purpose of this function.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return
     */
    int (*set_resp_wait_full_body)(const lsi_session_t *pSession);

    /**
     * @brief parse_req_args notifies the system that the URI arguments are to
     * be parsed AND/OR specify a temporary location and permissions
     * for any files uploaded.  It is a pre-requisite to the get_qs_argxxx
     * and get_req_argxxx functions that access the arguments.
     * @ingroup uri
     *
     * @param[in] session - a pointer to the session
     * @param[in] parse_req_body - 1 to parse the request body, 0 to just parse
     * the arguments.
     * @param[in] uploadPassByPath - Only used if this is a file upload and
     * staged through a temporary directory, set this to 1 and specify
     * the temporary directory.  Otherwise set to 0.
     * @param[in] uploadTmpDir - Only used if uploadPassByPath is set to 1 and
     * is then required to be a valid directory path and exist and the
     * application have write permission to it (/tmp/ is often used).  Otherwise
     * set to NULL.
     * @param[in] uploadTmpFilePermission - Only used if uploadPassByPath is set
     * to 1, Unix file permissions of the temporary file, set to 0 otherwise.
     * For example, 0600 is used if it's only used by the web server user.
     * @return LS_OK if there were no errors, LS_FAIL to indicate errors.
     */
    int (*parse_req_args)(const lsi_session_t *session, int parse_req_body,
                          int uploadPassByPath, const char *uploadTmpDir,
                          int uploadTmpFilePermission);

    /**
     * @brief get_req_args_count returns the count of arguments.
     * parse_req_args should be called first.
     * @ingroup uri
     *
     * @param[in] session - a pointer to the session
     * @return LS_FAIL to indicate errors; otherwise the number of request
     * arguments.
     */
    int (*get_req_args_count)(const lsi_session_t *session);

    /**
     * @brief get_req_arg_by_idx returns the arguments based on the 0 based
     * index from 0 to get_req_args_count().  parse_req_args should be called
     * first.
     * @ingroup uri
     *
     * @param[in] session - a pointer to the session
     * @param[in] index - which 0 based parameter to extract.
     * @param[out] pArg - If this is not a file, the parameter; if this is a
     * file, then this value is not used and it should be NULL.
     * Note that this is a strpair with the length and value independently
     * specified.  If a 0 terminated C string is needed, it should be copied to
     * a separate character array and the data not modified.
     * @param[out] **filePath - If this is a file, a pointer to the file name
     * posted to.  Otherwise this value is not used and it should be NULL.
     * @return LS_FAIL to indicate errors; LS_OK for success.
     */
    int (*get_req_arg_by_idx)(const lsi_session_t *session, int index,
                              ls_strpair_t *pArg, char **filePath);

    /**
     * @brief get_qs_args_count returns the count of query string arguments.
     * parse_req_args should be called first.
     * @ingroup uri
     *
     * @param[in] session - a pointer to the session
     * @return LS_FAIL to indicate errors; otherwise the number of query string
     * arguments.
     */
    int (*get_qs_args_count)(const lsi_session_t *session);

    /**
     * @brief get_qs_arg_by_idx returns the query string arguments based on the
     * 0 based index from 0 to get_qs_args_count().  parse_req_args should be
     * called first.
     * @ingroup uri
     *
     * @param[in] session - a pointer to the session
     * @param[in] index - which 0 based post parameter to extract.
     * @param[out] pArg - The returned query string parameter.
     * Note that this is a strpair with the length and value independently
     * specified.  If a 0 terminated C string is needed, it should be copied to
     * a separate character array and the data not modified.
     * @return LS_FAIL to indicate errors; LS_OK for success.
     */
    int (*get_qs_arg_by_idx)(const lsi_session_t *session, int index,
                              ls_strpair_t *pArg);

    /**
     * @brief get_post_args_count returns the count of POST arguments.
     * @ingroup post
     *
     * @param[in] session - a pointer to the session
     * @return LS_FAIL to indicate errors; otherwise the number of post
     * arguments.
     */
    int (*get_post_args_count)(const lsi_session_t *session);

    /**
     * @brief get_post_arg_by_idx returns the post arguments based on the
     * 0 based index from 0 to get_post_args_count().
     * @ingroup post
     *
     * @param[in] session - a pointer to the session
     * @param[in] index - which 0 based post parameter to extract.
     * @param[out] pArg - The 0 based post parameter.
     * Note that this is a strpair with the length and value independently
     * specified.  If a 0 terminated C string is needed, it should be copied to
     * a separate character array and the data not modified.
     * @param[out] **filePath - If this is a file, a pointer to the file name
     * posted to.
     * @return LS_FAIL to indicate errors; LS_OK for success.
     */
    int (*get_post_arg_by_idx)(const lsi_session_t *session, int index,
                              ls_strpair_t *pArg, char **filePath);

    /**
     * @brief is_post_file_upload returns whether this is a file upload.
     * @ingroup post
     *
     * @param[in] session - a pointer to the session
     * @param[in] index - which 0 based post parameter to test.
     * @return LS_FAIL to indicate errors; 0 to indicate that this is parameter
     * is not a file, 1 to indicate that this parameter is a file.
     */
    int (*is_post_file_upload)(const lsi_session_t *session, int index);

    /**
     * @brief set_status_code is used to set the response status code of an HTTP
     * session.  It can be used in hook point and handler processing functions,
     * but MUST be called before the response header has been sent.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] code - the http response status code.
     */
    void (*set_status_code)(const lsi_session_t *pSession, int code);

    /**
     * @brief get_status_code is used to get the response status code of an HTTP
     * session.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return the http response status code.
     */
    int (*get_status_code)(const lsi_session_t *pSession);

    /**
     * @brief is_resp_buffer_available is used to check if the response buffer
     * is available to fill in data.  This function should be called before
     * append_resp_body or append_resp_bodyv is called.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return -1 on failure, 0 false, 1 true.
     */
    int (*is_resp_buffer_available)(const lsi_session_t *pSession);

    /**
     * @brief is_resp_buffer_gzipped is used to check if the response buffer is
     * gzipped (compressed).
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return 0 no compress, 1 gzip, 2, br.
     */
    int (*get_resp_buffer_compress_method)(const lsi_session_t *pSession);

    /**
     * @brief set_resp_buffer_compress_method is used to set the response buffer
     * gzip flag.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] compress_method - compress method;
     *               set 1 if gzip compressed, 2 if br compressed,
     *               0 to clear the flag.
     * @return 0 if success, -1 if failure.
     */
    int (*set_resp_buffer_compress_method)(const lsi_session_t *pSession, int method);

    /**
     * @brief append_resp_body is used to append a buffer to the response body.
     * It can ONLY be used by handler functions.
     * The data will go through filters for post processing.
     * This function should NEVER be called from a filter post processing the
     * data.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] buf - a pointer to a buffer to be written.
     * @param[in] buflen - the size of the buffer.
     * @return the length of the request body.
     */
    int (*append_resp_body)(const lsi_session_t *pSession, const char *buf,
                            int len);

    /**
     * @brief append_resp_bodyv is used to append an iovec to the response body.
     * It can ONLY be used by handler functions.
     * The data will go through filters for post processing.
     * this function should NEVER be called from a filter post processing the
     * data.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] iov - the IO vector of data to be written.
     * @param[in] count - the size of the IO vector.
     * @return -1 on failure, return value of the hook filter callback function.
     */
    int (*append_resp_bodyv)(const lsi_session_t *pSession,
                             const struct iovec *iov, int count);

    /**
     * @brief send_file is used to send a file as the response body.
     * It can be used in handler processing functions.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] path - a pointer to the path string.
     * @param[in] start - file start offset.
     * @param[in] size - remaining size.
     * @return -1 or error codes from various layers of calls on failure, 0 on
     * success.
     */
    int (*send_file)(const lsi_session_t *pSession, const char *path,
                     int64_t start, int64_t size);

    /**
    * @brief send_file2 is used to send a file as the response body by handle,
    * position and size rather than just file name.  It can be used in handler
    * processing functions.
    * @ingroup response
    * @since 1.0
    *
    * @param[in] pSession - a pointer to the session.
    * @param[in] fd - file descriptor of the file to send.
    * @param[in] start - file start offset.
    * @param[in] size - remaining size.
    * @return -1 or error codes from various layers of calls on failure, 0 on
    * success.
    */
    int (*send_file2)(const lsi_session_t *pSession, int fd, int64_t start,
                      int64_t size);

    /**
     * @brief flush flushes the connection and sends the data.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     */
    int (*flush)(const lsi_session_t *pSession);

    /**
     * @brief end_resp is called when the response sending is complete.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     */
    void (*end_resp)(const lsi_session_t *pSession);

    /**
     * @brief set_resp_content_length sets the Content Length of the response.
     * If len is -1, the response will be set to chunk encoding.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] len - the content length.
     * @return 0.
     */
    int (*set_resp_content_length)(const lsi_session_t *pSession, int64_t len);

    /**
     * @brief set_resp_header is used to set a response header.  If the header
     * does not exist, it will add a new header.  If the header exists, it will
     * add the header based on the add_method - replace, append, merge and add
     * new line.  It can be used in #LSI_HKPT_RCVD_RESP_HEADER and handler
     * processing functions.
     * @ingroup resp_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] header_id - enum #LSI_RESP_HEADER_ID defined as
     * response header id.
     * @param[in] name - a pointer to the header id name string.
     * @param[in] nameLen - the length of the header id name string.
     * @param[in] val - a pointer to the header string to be set.
     * @param[in] valLen - the length of the header value string.
     * @param[in] add_method - enum #LSI_HEADER_OP defined for the method of
     * adding.
     * @return 0.
     */
    int (*set_resp_header)(const lsi_session_t *pSession,
                           unsigned int header_id, const char *name,
                           int nameLen, const char *val, int valLen,
                           int add_method);

    /**
     * @brief set_resp_header2 is used to parse the headers first then perform
     * set_resp_header.
     * @ingroup resp_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] headers - a pointer to the header string to be set.
     * @param[in] len - the length of the header value string.
     * @param[in] add_method - enum #LSI_HEADER_OP defined for the method of
     * adding.
     * @return 0.
     */
    int (*set_resp_header2)(const lsi_session_t *pSession, const char *headers,
                            int len, int add_method);

    /**
     * @brief set_resp_cookies is used to set response cookies.
     * @details The name, value, and domain are required.  If they do not exist,
     * the function will fail.
     * @ingroup response
     * @param[in] pSession - a pointer to the session.
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
    int (*set_resp_cookies)(const lsi_session_t *pSession, const char *pName,
                            const char *pVal, const char *path,
                            const char *domain, int expires, int secure,
                            int httponly);

    /**
     * @brief get_resp_header is used to get a response header's value in an
     * iovec array.  It will try to use header_id to search the header first.
     * If header_id is not #LSI_RSPHDR_UNKNOWN, the name and nameLen will NOT be
     * checked, and they can be set to NULL and 0.  Otherwise, if header_id is
     * #LSI_RSPHDR_UNKNOWN, then it will search through name and nameLen.
     * @ingroup resp_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] header_id - enum #LSI_RESP_HEADER_ID defined as response
     * header id.
     * @param[in] name - a pointer to the header id name string.
     * @param[in] nameLen - the length of the header id name string.
     * @param[out] iov - the IO vector that contains the headers.
     * @param[in] maxIovCount - size of the IO vector.
     * @return the count of headers in the IO vector.
     */
    int (*get_resp_header)(const lsi_session_t *pSession,
                           unsigned int header_id, const char *name,
                           int nameLen, struct iovec *iov, int maxIovCount);

    /**
     * @brief get_resp_headers_count is used to get the count of the response
     * headers.  Multiple line headers will be counted as different headers.
     * @ingroup resp_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return the number of headers in the whole response header.
     */
    int (*get_resp_headers_count)(const lsi_session_t *pSession);

    /**
     * @brief get_resp_header_id is used to get the ID (of the type
     * #LSI_RESP_HEADER_ID as an int) for a given header name.
     * @ingroup resp_header
     * @param[in] pSession - a pointer to the session.
     * @param[in] name - A C string of the name to be looked up.
     * @return A value of the type #LSI_RESP_HEADER_ID as an integer or -1 if
     * not found.
     */
    unsigned int (*get_resp_header_id)(const lsi_session_t *pSession,
                                       const char *name);

    /**
     * @brief get_resp_headers is used to get the whole response headers and
     * store them to the iovec array.  If maxIovCount is smaller than the count
     * of the headers, it will only store the first maxIovCount headers.
     * @ingroup resp_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in,out] iov_key - the IO vector that contains the header names.
     * @param[in,out] iov_val - the IO vector that contains the header values.
     * @param[in] maxIovCount - size of the IO vector.
     * @return the count of all the headers.
     */
    int (*get_resp_headers)(const lsi_session_t *pSession,
                            struct iovec *iov_key, struct iovec *iov_val,
                            int maxIovCount);

    /**
     * @brief remove_resp_header is used to remove a response header.
     * The header_id should match the name and nameLen if it isn't -1.
     * Providing the header_id will make finding of the header quicker.  It can
     * be used in #LSI_HKPT_RCVD_RESP_HEADER and handler processing functions.
     * @ingroup resp_header
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] header_id - enum #LSI_RESP_HEADER_ID defined as response
     * header id.
     * @param[in] name - a pointer to the header id name string.
     * @param[in] nameLen - the length of the header id name string.
     * @return 0.
     */
    int (*remove_resp_header)(const lsi_session_t *pSession,
                              unsigned int header_id, const char *name,
                              int nameLen);

    /**
     * @brief get_file_path_by_uri is used to get the corresponding file path
     * based on request URI.
     * @details The difference between this function with simply append URI to
     * document root is that this function takes context configuration into
     * consideration. If a context points to a directory outside the document
     * root, this function can return the correct file path based on the context
     * path.
     * @ingroup module
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] uri - the URI.
     * @param[in] uri_len - size of the URI.
     * @param[in,out] path - the buffer holding the result path.
     * @param[in] max_len - size of the path buffer.
     *
     * @return if success, return the size of path, if error, return -1.
     */
    int (*get_file_path_by_uri)(const lsi_session_t *pSession, const char *uri,
                                int uri_len, char *path, int max_len);

    /**
     * @brief get_mime_type_by_suffix is used to get corresponding MIME type
     * based on file suffix.
     * @ingroup module
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] suffix - the file suffix, without the leading dot.
     * @return the readonly MIME type string.
     */
    const char *(*get_mime_type_by_suffix)(const lsi_session_t *pSession,
                                           const char *suffix);

    /**
     * @brief set_force_mime_type is used to force the server core to use a
     * MIME type with request in current session.
     * @ingroup module
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] mime - the MIME type string.
     *
     * @return 0 if success, -1 if failure.
     */
    int (*set_force_mime_type)(const lsi_session_t *pSession, const char *mime);

    /**
     * @brief get_req_file_path is used to get the static file path associated
     * with request in current session.
     * @ingroup module
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in,out] pathLen - the length of the path.
     *
     * @return the file path string if success, NULL if no static file
     * associated.
     */
    const char *(*get_req_file_path)(const lsi_session_t *pSession,
                                     int *pathLen);

    /**
     * @brief get_req_handler_type is used to get the type name of a handler
     * assigned to this request.
     * @ingroup module
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     *
     * @return type name string, if no handler assigned, return NULL.
     */
    const char *(*get_req_handler_type)(const lsi_session_t *pSession);

    /**
     * @brief is_access_log_on returns if access logging is enabled for this
     * session.
     * @ingroup logging
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     *
     * @return 1 if access logging is enabled, 0 if access logging is disabled.
     */
    int (*is_access_log_on)(const lsi_session_t *pSession);

    /**
     * @brief set_access_log turns access logging on or off.
     * @ingroup logging
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] on_off - set to 1 to turn on access logging, set to 0 to turn
     * off access logging.
     */
    void (*set_access_log)(const lsi_session_t *pSession, int on_off);

    /**
     * @brief get_access_log_string returns a string for access log based on the
     * log_pattern.
     * @ingroup logging
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] log_pattern - the log pattern to be used; for details, please
     * refer to Apache's custom log format.
     * @param[in,out] buf - the buffer holding the log string.
     * @param[in] bufLen - the length of buf.
     *
     * @return the length of the final log string, -1 if error.
     */
    int (*get_access_log_string)(const lsi_session_t *pSession,
                                 const char *log_pattern, char *buf,
                                 int bufLen);

    /**
     * @brief get_file_stat is used to get the status (directory information)
     * for a file.
     * @details The routine uses the system call stat(2).
     * @ingroup utility
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @param[in] path - the file path name.
     * @param[in] pathLen - the length of the path name.
     * @param[out] st - the structure to hold the returned file status.
     *
     * @return -1 on failure, 0 on success.
     */
    int (*get_file_stat)(const lsi_session_t *pSession, const char *path,
                         int pathLen, struct stat *st);

    /**
     * @brief is_resp_handler_aborted is used to check if the handler has been
     * aborted.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     * @return -1 if session does not exist, else 0.
     */
    int (*is_resp_handler_aborted)(const lsi_session_t *pSession);

    /**
     * @brief get_resp_body_buf returns a buffer that holds response body of
     * current session.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     *
     * @return the pointer to the response body buffer, NULL if response body is
     * not available.
     */
    void *(*get_resp_body_buf)(const lsi_session_t *pSession);

    /**
     * @brief get_req_body_buf returns a buffer that holds request body of
     * current session.
     * @ingroup req_body
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     *
     * @return the pointer to the request body buffer, NULL if request body is
     * not available.
     */
    void *(*get_req_body_buf)(const lsi_session_t *pSession);

    /**
     * @brief get_new_body_buf Allows resizing of the response buffer
     * @ingroup response
     * @since 1.0
     *
     * @param[in] iInitialSize - The number of bytes to allocate at a minimum.
     *
     * @return the pointer to the response body buffer, NULL if response body
     * is not available.
     */
    void *(*get_new_body_buf)(int64_t iInitialSize);

    /**
     * @brief get_body_buf_size returns the size of the specified response body
     * buffer.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pBuf - the body buffer.
     * @return the size of the body buffer, else -1 on error.
     */
    int64_t (*get_body_buf_size)(void *pBuf);

    /**
     * @brief is_body_buf_eof used determine if the specified \e offset
     *   is the body buffer end of file.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pBuf - the body buffer.
     * @param[in] offset - the offset in the body buffer.
     * @return 0 false, 1 true; if \e pBuf is NULL, 1 is returned.
     */
    int (*is_body_buf_eof)(void *pBuf, int64_t offset);

    /**
     * @brief acquire_body_buf_block used to acquire (retrieve) a portion of
     * the response body buffer.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pBuf - the body buffer.
     * @param[in] offset - the offset in the body buffer.
     * @param[out] size - a pointer which upon return contains the number of
     * bytes acquired.
     * @return a pointer to the body buffer block, else NULL on error.
     *
     * @note If the \e offset is past the end of file,
     *  \e size contains zero and a null string is returned.
     */
    const char *(*acquire_body_buf_block)(void *pBuf, int64_t offset,
                                          int *size);

    /**
     * @brief release_body_buf_block
     *  is used to release a portion of the response body buffer back to the system.
     * @details The entire block containing \e offset is released.
     *  If the current buffer read/write pointer is within this block,
     *  the block is \e not released.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pBuf - the body buffer.
     * @param[in] offset - the offset in the body buffer.
     * @return void.
     */
    void (*release_body_buf_block)(void *pBuf, int64_t offset);

    /**
     * @brief reset_body_buf resets a response body buffer to be used again.
     * @details Set iWriteFlag to 1 to reset the writing offset as well as
     * the reading offset.  Otherwise, this should be 0.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pBuf - the body buffer.
     * @param[in] iWriteFlag - The flag specifying whether or not to reset the
     * writing offset as well.
     * @return void.
     */
    void (*reset_body_buf)(void *pBuf, int iWriteFlag);

    /**
     * @brief append_body_buf appends (writes) data to the end of a response
     * body buffer.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pBuf - the body buffer.
     * @param[in] pBlock - the data buffer to append.
     * @param[in] size - the size (bytes) of the data buffer to append.
     * @return the number of bytes appended, else -1 on error.
     */
    int (*append_body_buf)(void *pBuf, const char *pBlock, int size);

    /**
     * @brief set_req_body_buf lets a filter module actually change the request
     * body managed by the server.
     * @ingroup req_body
     * @since 1.0
     *
     * @param[in] session - the session's handle.
     * @param[in] pBuf - the body buffer.
     * @return LS_OK if it worked, LS_ERR on error.
     */
    int (*set_req_body_buf)(const lsi_session_t *session, void *pBuf);

    /**
     * @brief get_body_buf_fd is used to get a file descriptor if request or
     * response body is saved in a file-backed MMAP buffer.
     * The file descriptor should not be closed in the module.
     * @ingroup response
     * @since 1.0
     *
     * @param[in] pBuf - the buffer to get the MMAP handle for.
     * @return -1 on failure, request body file fd on success.
     */
    int (*get_body_buf_fd)(void *pBuf);

    /**
     * @brief send_resp_headers is called by a module handler when the response
     * headers are complete.
     * @details calling this function is optional. Calling this function will
     * trigger the #LSI_HKPT_RCVD_RESP_HEADER hook point; if not called,
     * #LSI_HKPT_RCVD_RESP_HEADER will be triggerered when adding content to
     * response body.
     * @ingroup resp_header
     *
     * @since 1.0
     *
     * @param[in] pSession - a pointer to the session.
     */
    void (*send_resp_headers)(const lsi_session_t *pSession);

    /**
     * @brief is_resp_headers_sent checks if the response headers are already
     * sent.
     * @ingroup resp_header
     *
     * @param[in] pSession - a pointer to the session.
     * @return 1 if the headers were sent already, else 0 if not.
     */
    int (*is_resp_headers_sent)(const lsi_session_t *pSession);

    /**
     * @brief get_module_name returns the name of the module.
     * @ingroup module
     * @since 1.0
     *
     * @param[in] pModule - a pointer to lsi_module_t.
     * @return a const pointer to the name of the module, readonly.
     */
    const char *(*get_module_name)(const lsi_module_t *pModule);

    /**
     * @brief get_multiplexer gets the event multiplexer for the main event
     * loop.  For internal use only.
     *
     * @since 1.0
     * @return a pointer to the multiplexer used by the main event loop.
     */
    void *(*get_multiplexer)();

    /**
     * @brief edio_reg registers an event callback function.  For internal use
     * only.
     * @return an event handle
     */
    ls_edio_t *(*edio_reg)(int fd, edio_evt_cb evt_cb,
                           edio_timer_cb timer_cb, short events, void *pParam);

    /**
     * @brief edio_remove removes a registered event callback function.  For
     * internal use only.
     */
    void (*edio_remove)(ls_edio_t *handle);

    /**
     * @brief edio_modify modifies a registered event callback function.  For
     * internal use only.
     */
    void (*edio_modify)(ls_edio_t *handle, short events, int add_remove);

    /**
     * @brief get_client_access_level returns the current security access level
     * type for the session using #LSI_ACL_LEVEL
     * @ingroup security
     * @since 1.0
     *
     * @param[in] session - a pointer to the session.
     */

    int (*get_client_access_level)(const lsi_session_t *session);

    /**
     * @brief is_suspended returns if a session is in suspended mode.
     * @ingroup session
     * @since 1.0
     * @param[in] session - a pointer to the session
     * @return 0 false, -1 invalid pSession, none-zero true.
     */
    int (*is_suspended)(const lsi_session_t *session);

    /**
     * @brief resume continues processing of the suspended session.
     *    this should be at the end of a function call
     * @ingroup session
     * @since 1.0
     * @param[in] session - a pointer to the session.
     * @param[in] retcode - the result that a hook function returns as if no
     * suspend/resume happened.
     * @return -1 failed to resume, 0 successful.
     */
    int (*resume)(const lsi_session_t *session, int retcode);

    /**
     * @brief exec_ext_cmd runs a registered handler to perform a given
     * function.  For internal use only.
     */
    int (* exec_ext_cmd)(const lsi_session_t *pSession, const char *cmd,
                         int len, evtcb_pf cb, const long lParam, void *pParam);

    /**
     * @brief get_ext_cmd_res_buf returns the result buffer after processed by
     * a function run by exec_ext_cmd. For internal use only.
     */
    char *(* get_ext_cmd_res_buf)(const lsi_session_t *pSession, int *length);

//#ifdef notdef
//    int (*is_subrequest)(const lsi_session_t *session);
//#endif

    /**
     * @brief get_cur_time gets the current system time.
     * @ingroup utility
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
     * @ingroup vhost
     * @since 1.0
     *
     * @return the count of Virtual Hosts.
     */
    int (*get_vhost_count)();

    /**
     * @brief get_vhost gets a Virtual Host object.
     * @ingroup vhost
     * @since 1.0
     *
     * @param[in] index - the index of the Virtual Host, starting from 0.
     * @return a pointer to the Virtual Host object.
     */
    const void *(*get_vhost)(int index);

    /**
     * @brief set_vhost_module_data is used to set the module data for a Virtual
     * Host.
     * @details The routine is similar to set_module_data, but may be used
     * without reference to a session.
     * @ingroup vhost
     * @since 1.0
     *
     * @param[in] vhost - a pointer to the Virtual Host.
     * @param[in] module - a pointer to an lsi_module_t struct.
     * @param[in] data - a pointer to the user defined data.
     * @return -1 on failure, 0 on success.
     *
     * @see set_module_data
     */
    int (* set_vhost_module_data)(const void *vhost, const lsi_module_t *module,
                                  void *data);

    /**
     * @brief get_vhost_module_data - gets the module data which was set by
     * set_vhost_module_data.  The return value is a pointer to the user's own
     * data structure.
     * @details The routine is similar to get_module_data, but may be used
     * without reference to a session.
     * @ingroup vhost
     * @since 1.0
     *
     * @param[in] vhost - a pointer to the Virtual Host.
     * @param[in] module - a pointer to an lsi_module_t struct.
     * @return NULL on failure, a pointer to the user defined data on success.
     *
     * @see get_module_data
     */
    void       *(* get_vhost_module_data)(const void *vhost,
                                          const lsi_module_t *module);

    /**
     * @brief get_vhost_module_conf
     *  gets the user defined module parameters which are parsed by
     *  the callback parse_config and pointed to in the struct lsi_confparser_t.
     * @ingroup vhost
     * @since 1.0
     *
     * @param[in] vhost - a pointer to the Virtual Host.
     * @param[in] module - a pointer to an lsi_module_t struct.
     * @return NULL on failure, or a pointer to the user-defined configuration
     * data on success.
     *
     * @see get_config
     */
    void       *(* get_vhost_module_conf)(const void *vhost,
                                           const lsi_module_t *module);

    /**
     * @brief get_session_pool gets the session pool to allow modules to
     * allocate from.  For internal use only.
     * @since 1.0
     *
     * @param[in] session - a pointer to the session.
     * @return a pointer to the session pool.
     */
    ls_xpool_t *(*get_session_pool)(const lsi_session_t *session);

    /**
     * @brief get_local_sockaddr - gets the socket address in a character string
     * format.
     * @ingroup session
     * @since 1.0
     *
     * @param[in] session - a pointer to the HttpSession.
     * @param[out] ip - a pointer to the returned address string.
     * @param[in] maxLen - the size of the buffer at \e pIp.
     * @return the length of the string on success,
     *  else 0 for system errors, or -1 for bad parameters.
     */
    int (* get_local_sockaddr)(const lsi_session_t *session, char *ip,
                               int maxLen);


    /**
     * @brief handoff_fd return a duplicated file descriptor associated with
     * current session and all data received on this file descriptor, including
     * parsed request headers and data has not been processed.  After this
     * function call the server core will stop processing current session and
     * closed the original file descriptor. The session object will become
     * invalid.
     * @ingroup response
     *
     * @since 1.0
     *
     * @param[in] session - a pointer to the session.
     * @param[in,out] data - a pointer to a pointer pointing to the buffer
     * holding data received so far.  The buffer is allocated by server core,
     * must be released by the caller of this function with free().
     * @param[in,out] data_len - a pointer to the variable receive the size of
     * the buffer.
     * @return a file descriptor if success, -1 if failed. the file descriptor
     * is dup'd from the original file descriptor, it should be closed by the
     * caller of this function when done.
     */
    int (*handoff_fd)(const lsi_session_t *session, char **data, int *data_len);

    /**
     * @brief expand_current_server_variable reallocates a server variable to
     * allow it to be modified and made larger.
     * @ingroup module
     * @param[in] level - A value of #LSI_CFG_LEVEL
     * @param[in] variable - variable of the Server/Listener/VHost or URI of
     * the Context
     * @param[in] buf - a buffer to store the result
     * @param[in] max_len - length of the buf
     * @return return the length written to the buf
     */
    int (*expand_current_server_variable)(int level, const char *variable,
                                         char *buf, int max_len);
    
    //FIXME: why these functions not in use
//     lsi_session_t * (* new_subreq)(lsi_session_t *pSession, lsi_subreq_t *pSubReq);
// 
//     int (* exec_subreq)(lsi_session_t *pSession, lsi_session_t *pSubSess);
//     
//     int (* close_subreq)(lsi_session_t *pSession, lsi_session_t *pSubSess);
//     
//     int (* include_subreq_resp)(lsi_session_t *pSession, lsi_session_t *pSubSess);

    
    /**
     * @brief module_log is used to write the formatted log to the error log
     * associated with a module.
     * @details session ID will be added to the log message automatically when
     * session is not NULL.  This function will not add a trailing \\n to the
     * end.  Module developers should use the LSM_XXX macros for all logging.
     * @ingroup logging
     * @since 1.0
     *
     * @param[in] module - pointer to module object, module name is logged.
     * @param[in] session - current session, log file, and session ID are based
     * on session may be NULL.
     * @param[in] level - enum defined in log level definitions #LSI_LOG_LEVEL.
     * @param[in] fmt - formatted string.
     */
    void (*module_log)(const lsi_module_t *module, const lsi_session_t *session,
                       int level, const char *fmt, ...)
#if __GNUC__
        __attribute__((format(printf, 4, 5)))
#endif
        ;


    /**
     * @brief c_log is used to write the formatted log to the error log
     * associated with a component.
     * @details session ID will be added to the log message automatically when
     * session is not NULL.  This function will not add a trailing \\n to the
     * end.  Module developers should use the LSM_XXX macros for all logging.
     * @ingroup logging
     * @since 1.0
     *
     * @param[in] component_name - pointer to the name of the component.
     * @param[in] session - current session, log file, and session ID are based
     * on session.
     * @param[in] level - enum defined in log level definitions #LSI_LOG_LEVEL.
     * @param[in] fmt - formatted string.
     */
    void (*c_log)(const char *component_name, const lsi_session_t *session,
                  int level, const char *fmt, ...)
#if __GNUC__
        __attribute__((format(printf, 4, 5)))
#endif
        ;

    /**
     * @brief _log_level_ptr is the address of variable that stores the level
     * of logging that server core uses, the variable controls the level of
     * details of logging messages.  Its value is defined in #LSI_LOG_LEVEL,
     * range is from LSI_LOG_ERROR to LSI_LOG_TRACE.
     * @ingroup logging
     */
    const int *_log_level_ptr;



    /**
     * @brief set_ua_code is only for pagespeed module to set the
     * user-agnet code which is used for group the optimization pages.
     */
    int (* set_ua_code) (const char *ua, char* code);

    /**
     * @brief get_ua_code is to get the user-agnet code whic is set by
     * pagespeed module.
     *
     */
    char *(* get_ua_code) (const char *ua);


    /**
     * @brief foreach is a convinent function to access a collection of data
     * related to a session.
     * @details Allows a large number of items to be passed back to the calling
     * function in a complex query by passing them one at a time to a callback
     * function.
     * @ingroup query
     * @param[in] session - current session
     * @param[in] type - type of data requested, see #LSI_DATA_TYPE
     * @param[in] filter - NULL for no filter.  If a filter is to be used, a
     * character string filter in the format where the first character is
     * meaningful with the remainder being direct compare:
     *    - optional leading '!' = negate comparison (which can be followed by
     *      any of the additional single character values)
     *    - s... simple string compare
     *    - i... simple string compare, ignore case
     *    - fd.. f indicates fnmatch, the next character d is an OR of the
     *      following values
     *        - 1:  FNM_NOESCAPE
     *        - 2:  FNM_PATHNAME
     *        - 4:  FNM_PERIOD
     *    - gd.. as for fd above, but 'g' indicates fnmatch with FNM_CASEFOLD
     *    - r... regex
     * @param[in] cb A callback function in the form #lsi_foreach_cb
     * @param[in] arg: An additional parameter passed as input to the callback
     * function.
     */
    void (*foreach)(const lsi_session_t *session, LSI_DATA_TYPE type,
                    const char *filter, lsi_foreach_cb cb, void *arg);

    /**
     * @brief register_thread_cleanup allows multi threaded modules
     *  using the LSI_HDLR_DEFAULT_POOL to register cleanup functions
     *  to be called when threads die via pthread_exit or pthread_cancel.
     *  This function is a no-op for single threaded modules or multi
     *  thread modules that use their own handlers.
     * @ingroup module
     * @param[in] module - the current module
     * @param[in] routine - the routine to be called during thread cleanup.
     * @param[in] arg - An argument to be passed through to the cleanup routine.
     *
     */
    void (*register_thread_cleanup)(const lsi_module_t *module,
                                    void (*routine)(void *), void * arg);

};

/**
 * @brief  g_api single export to support all LSIAPI functions being globally
 * available.
 * @details g_api is a global variable, it can be accessed from all modules to
 * make API calls.
 * @since 1.0
 */
extern __thread const lsi_api_t *g_api;

#define LSI_LOG_ENABLED(level) (*g_api->_log_level_ptr >= level)
#define LSI_LOG(level, session, ...) \
    do { \
        if (LSI_LOG_ENABLED(level)) g_api->log(session, level, __VA_ARGS__); \
    } while(0)
#define LSI_LOGRAW(...) g_api->lograw(__VA_ARGS__)
#define LSI_LOGIO(session, ...) LSI_LOG(LSI_LOG_TRACE, session, __VA_ARGS__)
#define LSI_DBGH(session, ...) LSI_LOG(LSI_LOG_DEBUG_HIGH, session, __VA_ARGS__)
#define LSI_DBGM(session, ...) LSI_LOG(LSI_LOG_DEBUG_MEDIUM, session, __VA_ARGS__)
#define LSI_DBGL(session, ...) LSI_LOG(LSI_LOG_DEBUG_LOW, session, __VA_ARGS__)
#define LSI_DBG(session, ...) LSI_LOG(LSI_LOG_DEBUG,  session, __VA_ARGS__)
#define LSI_INF(session, ...) LSI_LOG(LSI_LOG_INFO,   session, __VA_ARGS__)
#define LSI_NOT(session, ...) LSI_LOG(LSI_LOG_NOTICE, session, __VA_ARGS__)
#define LSI_WRN(session, ...) LSI_LOG(LSI_LOG_WARN,   session, __VA_ARGS__)
#define LSI_ERR(session, ...) LSI_LOG(LSI_LOG_ERROR,  session, __VA_ARGS__)

/**
 * @brief LSM_LOG_ENABLED returns whether logging is enabled at the requested
 * level for a session
 * @note LSM macros are recommended for module logs.
 * @ingroup logging
 * @param[in] m - module handle, usually &NAME
 * @param[in] l - #LSI_LOG_LEVEL
 * @returns 1 if enabled at that level, 0 if not
 */
#define LSM_LOG_ENABLED(m, l) \
    (*g_api->_log_level_ptr >= l && MODULE_LOG_LEVEL(m) >= l)

/**
 * @brief LSM_LOG Logs a message, using printf format, at a specified level.
 * A new line \\n needs to be explicitly added
 * @note LSM macros are recommended for module logs.
 * @ingroup logging
 * @param[in] mod - module handle, usually &NAME
 * @param[in] level - #LSI_LOG_LEVEL
 * @param[in] session - the session handle
 * @param[in] ... - a printf style format string.  Can be followed by any
 * number of required variables necessary to fill out the string.
 */
#define LSM_LOG(mod, level, session, ...) \
    do { \
        if (LSM_LOG_ENABLED(mod, level)) \
            g_api->module_log(mod, session, level, __VA_ARGS__); \
    } while(0)

/**
 * @brief LSM_LOGRAW used to write additional log messages to the default error
 * log file.  No timestamp, log level, session ID prefix is added; just the data
 * in the buffer is logged into log files.
 * This is usually used for adding more content to previous log messages.
 * @note LSM macros are recommended for module logs.
 * @ingroup logging
 * @param[in] ... - a printf style format string.  Can be followed by any
 * number of required variables necessary to fill out the string.
 */
#define LSM_LOGRAW(...) g_api->lograw(__VA_ARGS__)

/**
 * @brief LSM_LOGIO Logs a trace message, using printf format for a module.
 * A new line \\n needs to be explicitly added
 * @note LSM macros are recommended for module logs.
 * @ingroup logging
 * @param[in] m - module handle, usually &NAME
 * @param[in] s - the session handle
 * @param[in] ... - a printf style format string.  Can be followed by any
 * number of required variables necessary to fill out the string.
 */
#define LSM_LOGIO(m, s, ...) LSM_LOG(m, LSI_LOG_TRACE, s, __VA_ARGS__)

/**
 * @brief LSM_DBGH Logs a debug (high) message, using printf format for a module.
 * A new line \\n needs to be explicitly added
 * @note LSM macros are recommended for module logs.
 * @ingroup logging
 * @param[in] m - module handle, usually &NAME
 * @param[in] s - the session handle
 * @param[in] ... - a printf style format string.  Can be followed by any
 * number of required variables necessary to fill out the string.
 */
#define LSM_DBGH(m, s, ...) LSM_LOG(m, LSI_LOG_DEBUG_HIGH, s, __VA_ARGS__)

/**
 * @brief LSM_DBGM Logs a debug (medium) message, using printf format for a module.
 * A new line \\n needs to be explicitly added
 * @note LSM macros are recommended for module logs.
 * @ingroup logging
 * @param[in] m - module handle, usually &NAME
 * @param[in] s - the session handle
 * @param[in] ... - a printf style format string.  Can be followed by any
 * number of required variables necessary to fill out the string.
 */
#define LSM_DBGM(m, s, ...) LSM_LOG(m, LSI_LOG_DEBUG_MEDIUM, s, __VA_ARGS__)

/**
 * @brief LSM_DBGL Logs a debug (low) message, using printf format for a module.
 * A new line \\n needs to be explicitly added
 * @note LSM macros are recommended for module logs.
 * @ingroup logging
 * @param[in] m - module handle, usually &NAME
 * @param[in] s - the session handle
 * @param[in] ... - a printf style format string.  Can be followed by any
 * number of required variables necessary to fill out the string.
 */
#define LSM_DBGL(m, s, ...) LSM_LOG(m, LSI_LOG_DEBUG_LOW, s, __VA_ARGS__)

/**
 * @brief LSM_DBG Logs a debug message, using printf format for a module.
 * A new line \\n needs to be explicitly added
 * @note LSM macros are recommended for module logs.
 * @ingroup logging
 * @param[in] m - module handle, usually &NAME
 * @param[in] s - the session handle
 * @param[in] ... - a printf style format string.  Can be followed by any
 * number of required variables necessary to fill out the string.
 */
#define LSM_DBG(m, s, ...) LSM_LOG(m, LSI_LOG_DEBUG,  s, __VA_ARGS__)

/**
 * @brief LSM_INF Logs an informational message, using printf format for a module.
 * A new line \\n needs to be explicitly added
 * @note LSM macros are recommended for module logs.
 * @ingroup logging
 * @param[in] m - module handle, usually &NAME
 * @param[in] s - the session handle
 * @param[in] ... - a printf style format string.  Can be followed by any
 * number of required variables necessary to fill out the string.
 */
#define LSM_INF(m, s, ...) LSM_LOG(m, LSI_LOG_INFO,   s, __VA_ARGS__)

/**
 * @brief LSM_NOT Logs a notice message, using printf format for a module.
 * A new line \\n needs to be explicitly added
 * @note LSM macros are recommended for module logs.
 * @ingroup logging
 * @param[in] m - module handle, usually &NAME
 * @param[in] s - the session handle
 * @param[in] ... - a printf style format string.  Can be followed by any
 * number of required variables necessary to fill out the string.
 */
#define LSM_NOT(m, s, ...) LSM_LOG(m, LSI_LOG_NOTICE, s, __VA_ARGS__)

/**
 * @brief LSM_WRN Logs a warning message, using printf format for a module.
 * A new line \\n needs to be explicitly added
 * @note LSM macros are recommended for module logs.
 * @ingroup logging
 * @param[in] m - module handle, usually &NAME
 * @param[in] s - the session handle
 * @param[in] ... - a printf style format string.  Can be followed by any
 * number of required variables necessary to fill out the string.
 */
#define LSM_WRN(m, s, ...) LSM_LOG(m, LSI_LOG_WARN,   s, __VA_ARGS__)

/**
 * @brief LSM_ERR Logs an error message, using printf format for a module.
 * A new line \\n needs to be explicitly added
 * @note LSM macros are recommended for module logs.
 * @ingroup logging
 * @param[in] m - module handle, usually &NAME
 * @param[in] s - the session handle
 * @param[in] ... - a printf style format string.  Can be followed by any
 * number of required variables necessary to fill out the string.
 */
#define LSM_ERR(m, s, ...) LSM_LOG(m, LSI_LOG_ERROR,  s, __VA_ARGS__)

#define DECL_COMPONENT_LOG(id) \
    static const char *s_comp_log_id = id;

#define LSC_LOG_ENABLED(l) \
    (*g_api->_log_level_ptr >= l)


#define LSC_LOG(level, session, ...) \
    do { \
        if (LSC_LOG_ENABLED(level)) \
            g_api->c_log(s_comp_log_id, session, level, __VA_ARGS__); \
    } while(0)

#define LSC_LOGRAW(...) g_api->lograw(__VA_ARGS__)

#define LSC_LOGIO(s, ...) LSC_LOG(LSI_LOG_TRACE, s, __VA_ARGS__)

#define LSC_DBGH(s, ...) LSC_LOG(LSI_LOG_DEBUG_HIGH, s, __VA_ARGS__)

#define LSC_DBGM(s, ...) LSC_LOG(LSI_LOG_DEBUG_MEDIUM, s, __VA_ARGS__)

#define LSC_DBGL(s, ...) LSC_LOG(LSI_LOG_DEBUG_LOW, s, __VA_ARGS__)

#define LSC_DBG(s, ...) LSC_LOG(LSI_LOG_DEBUG,  s, __VA_ARGS__)
#define LSC_INF(s, ...) LSC_LOG(LSI_LOG_INFO,   s, __VA_ARGS__)
#define LSC_NOT(s, ...) LSC_LOG(LSI_LOG_NOTICE, s, __VA_ARGS__)
#define LSC_WRN(s, ...) LSC_LOG(LSI_LOG_WARN,   s, __VA_ARGS__)
#define LSC_ERR(s, ...) LSC_LOG(LSI_LOG_ERROR,  s, __VA_ARGS__)

#ifdef __cplusplus
}
#endif


#endif //LS_MODULE_H
