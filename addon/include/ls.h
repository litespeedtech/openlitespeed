/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2014  LiteSpeed Technologies, Inc.                        *
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
#include <stdio.h>
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
#define LSIAPI_VERSION_MINOR    0
/**
 * @brief Defines the version string of LSIAPI.
 */
#define LSIAPI_VERSION_STRING   "1.0"

/**
 * @def LSI_MAX_HOOK_PRIORITY 
 * @brief The max priority level allowed.
 * @since 1.0
 */
#define LSI_MAX_HOOK_PRIORITY     3000

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
 * @brief The length of the key for global data container of all modules.
 * @since 1.0
 */
#define LSI_MODULE_CONTAINER_KEYLEN   (sizeof(LSI_MODULE_CONTAINER_KEY) - 1)

/**
 * @def LSI_MODULE_SIGNATURE  
 * @brief Identifies the module as a LSIAPI module and the version of LSIAPI that the module compiled with.
 * @details The signature tells the server core first that it is actually a LSIAPI module,
 * and second, what version of the API that the binary compiled with in order to check
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
 * @brief A filter has two modes, TRANSFORMER and OBSERVER.  Flag if it is a transformer.
 * @details LSI_HOOK_FLAG_TRANSFORM is a flag for filter hook points, indicating 
 *  that the filter may change the content of the input data.
 *  If a filter does not change the input data, in OBSERVER mode, do not 
 *  set this flag.
 *  When no TRANSFORMERs are in a filter chain of a hook point, the server will do 
 *  optimizations to avoid deep copy by storing the data in the final buffer. 
 *  Its use is important for LSI_HKPT_RECV_REQ_BODY and LSI_HKPT_L4_RECVING filter hooks.
 * @since 1.0
 *   
 */

enum lsi_hook_flag
{
    /**
     * Transform true flag.
     */
    LSI_HOOK_FLAG_TRANSFORM  = 1,
};

/**
 * @enum lsi_module_data_level  
 * @brief Determines the scope of the module's user defined data.
 * @details Used by the level API parameter of add_hook, init_module_data, etc. functions.
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
     * LSI_HKPT_HTTP_BEGINSESSION is the point when the session begins an http connection.
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
     * HKPT_RECV_REQ_BODY is the point when request body data is being received.
     */
    LSI_HKPT_RECV_REQ_BODY,
    
    /**
     * LSI_HKPT_RECVED_REQ_BODY is the point when request body data was received.
     * This function accesses the body data file through a function pointer.
     */
    LSI_HKPT_RECVED_REQ_BODY,
    
    /**
     * LSI_HKPT_RECV_RESP_HEADER is the point when the response header was created.
     */
    LSI_HKPT_RECV_RESP_HEADER,
    
    /**
     * LSI_HKPT_RECV_RESP_BODY is the point when response body is being received
     * by the backend of the web server.
     */
    LSI_HKPT_RECV_RESP_BODY,
    
    /**
     * LSI_HKPT_RECVED_RESP_BODY is the point when the whole response body was received 
     * by the backend of the web server.
     */
    LSI_HKPT_RECVED_RESP_BODY,
    
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
     * LSI_HKPT_HTTP_END is the point when a session is ending on an http connection.
     */
    LSI_HKPT_HTTP_END,
    
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
 * @enum lsi_result_code 
 * @brief LSIAPI return value definition.
 * @details Used in the return value of API functions and callback functions unless otherwise stipulated.
 * If a function returns an int type value, it should always 
 * return LSI_RET_OK for no errors and LSI_RET_ERROR for other cases. 
 * For such functions that return a bool type (true / false), 1 means true and 0 means false.
 * @since 1.0
 */
enum lsi_result_code
{
    /**
     * Return code error.
     */
    LSI_RET_ERROR = -1,
    /**
     * Retrun code success.
     */
    LSI_RET_OK = 0,
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
     * Error in the response processing return code.
     */
    LSI_WRITE_RESP_ERROR = -1,
    /**
     * No further response data to write return code.
     */
    LSI_WRITE_RESP_FINISHED,
    /**
     * More response body data to write return code.
     */
    LSI_WRITE_RESP_CONTINUE,
};




/**
 * @enum lsi_req_variable
 * @brief LSIAPI request environment variables.
 * @details Used as API index parameter in env access function get_req_env_by_id.
 * The example reqinfohandler.c shows usage for all of these variables.
 * @since 1.0
 */
enum lsi_req_variable
{
    /**
     * Request remote port environment variable index
     */
    LSI_REQ_VAR_REMOTE_ADDR = 0,
    /**
     * Request remote port environment variable index
     */
    LSI_REQ_VAR_REMOTE_PORT,
    /**
     * Request remote host environment variable index
     */
    LSI_REQ_VAR_REMOTE_HOST,
    /**
     * Request remote user environment variable index
     */
    LSI_REQ_VAR_REMOTE_USER,
    /**
     * Request remote identifier environment variable index
     */
    LSI_REQ_VAR_REMOTE_IDENT,
    /**
     * Request Remote method environment variable index
     */
    LSI_REQ_VAR_REQ_METHOD,
    /**
     * Request Query string environment variable index
     */
    LSI_REQ_VAR_QUERY_STRING,
    /**
     * Request Authentication type environment variable index
     */
    LSI_REQ_VAR_AUTH_TYPE,
    /**
     * Request URI path environment variable index
     */
    LSI_REQ_VAR_PATH_INFO,
    /**
     * Script filename environment variable index
     */
    LSI_REQ_VAR_SCRIPTFILENAME,
    /**
     * Request filename port environment variable index
     */
    LSI_REQ_VAR_REQUST_FN,
    /**
     * Request URI environment variable index
     */
    LSI_REQ_VAR_REQ_URI,
    /**
     * Request document root directory environment variable index
     */
    LSI_REQ_VAR_DOC_ROOT,
    /**
     * Request  port environment variable index
     */
    LSI_REQ_VAR_SERVER_ADMIN,
    /**
     * Request server name environment variable index
     */
    LSI_REQ_VAR_SERVER_NAME,
    /**
     * Request server address environment variable index
     */
    LSI_REQ_VAR_SERVER_ADDR,
    /**
     * Request server port environment variable index
     */
    LSI_REQ_VAR_SERVER_PORT,
    /**
     * Request server prototype environment variable index
     */
    LSI_REQ_VAR_SERVER_PROTO,
    /**
     * Request server software version environment variable index
     */
    LSI_REQ_VAR_SERVER_SOFT,
    /**
     * Request API version environment variable index
     */
    LSI_REQ_VAR_API_VERSION,
    /**
     * Request request line environment variable index
     */
    LSI_REQ_VAR_REQ_LINE,
    /**
     * Request subrequest environment variable index
     */
    LSI_REQ_VAR_IS_SUBREQ,
    /**
     * Request time environment variable index
     */
    LSI_REQ_VAR_TIME,
    /**
     * Request year environment variable index
     */
    LSI_REQ_VAR_TIME_YEAR,
    /**
     * Request month environment variable index
     */
    LSI_REQ_VAR_TIME_MON,
    /**
     * Request day environment variable index
     */
    LSI_REQ_VAR_TIME_DAY,
    /**
     * Request hour environment variable index
     */
    LSI_REQ_VAR_TIME_HOUR,
    /**
     * Request minute environment variable index
     */
    LSI_REQ_VAR_TIME_MIN,
    /**
     * Request seconds environment variable index
     */
    LSI_REQ_VAR_TIME_SEC,
    /**
     * Request weekday environment variable index
     */
    LSI_REQ_VAR_TIME_WDAY,
    /**
     * Request script file name environment variable index
     */
    LSI_REQ_VAR_SCRIPT_NAME,
    /**
     * Request current URI environment variable index
     */
    LSI_REQ_VAR_CUR_URI,
    /**
     * Request URI base name environment variable index
     */
    LSI_REQ_VAR_REQ_BASENAME,
    /**
     * Request script user id environment variable index
     */
    LSI_REQ_VAR_SCRIPT_UID,
    /**
     * Request script global id environment variable index
     */
    LSI_REQ_VAR_SCRIPT_GID,
    /**
     * Request script user name environment variable index
     */
    LSI_REQ_VAR_SCRIPT_USERNAME,
    /**
     * Request script group name environment variable index
     */
    LSI_REQ_VAR_SCRIPT_GRPNAME,
    /**
     * Request script mode environment variable index
     */
    LSI_REQ_VAR_SCRIPT_MODE,
    /**
     * Request script base name environment variable index
     */
    LSI_REQ_VAR_SCRIPT_BASENAME,
    /**
     * Request script URI environment variable index
     */
    LSI_REQ_VAR_SCRIPT_URI,
    /**
     * Request original URI environment variable index
     */
    LSI_REQ_VAR_ORG_REQ_URI,
    /**
     * Request HTTPS environment variable index
     */
    LSI_REQ_VAR_HTTPS,
    /**
     * Request SSL version environment variable index
     */
    LSI_REQ_SSL_VERSION,
    /**
     * Request SSL session ID environment variable index
     */
    LSI_REQ_SSL_SESSION_ID,
    /**
     * Request SSL cipher environment variable index
     */
    LSI_REQ_SSL_CIPHER,
    /**
     * Request SSL cipher use key size environment variable index
     */
    LSI_REQ_SSL_CIPHER_USEKEYSIZE,
    /**
     * Remote SSL cipher ALG key size environment variable index
     */
    LSI_REQ_SSL_CIPHER_ALGKEYSIZE,
    /**
     * Request SSL client certification environment variable index
     */
    LSI_REQ_SSL_CLIENT_CERT,
    /**
     * Remote geographical IP address environment variable index
     */
    LSI_REQ_GEOIP_ADDR,
    /**
     * Request translated path environment variable index
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
     * append unless it exists
     */
    LSI_HEADER_MERGE, 
    /**
     * Add a new line
     */
    LSI_HEADER_ADD
};


/**
 * @enum lsi_url_op 
 * @brief The methods used for redirect a request to new URL. 
 * @details LSI_URI_NOCHANGE, LSI_URI_REWRITE and LSI_URL_REDIRECT_* can be combined with 
 * LSI_URL_QS_* 
 * @since 1.0
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
     * Internal redirect, the redirect is performed internally, 
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
     * External redirect with status code 307 Temporary Redirect.
     */
    LSI_URL_REDIRECT_307, 
    
    /**
     * Do not change Query String. Can be combined with LSI_URL_REWRITE and LSI_URL_REDIRECT_*.
     */
    LSI_URL_QS_NOCHANGE = 0<<4,
    /**
     * Append Query String. Can be combined with LSI_URL_REWRITE and LSI_URL_REDIRECT_*.
     */
    LSI_URL_QS_APPEND = 1<<4,
    
    /**
     * Set Query String. Can be combined with LSI_URL_REWRITE and LSI_URL_REDIRECT_*.
     */
    LSI_URL_QS_SET = 2<<4,
    
    /**
     * Delete Query String. Can be combined with LSI_URL_REWRITE and LSI_URL_REDIRECT_*.
     */
    LSI_URL_QS_DELETE = 3<<4,
    
    /**
     * indicates that encoding has been applied to URI.
     */
    LSI_URL_ENCODED = 128
};

/**
 * @enum lsi_req_header_id 
 * @brief The most common request-header ids.
 * @details Used to access components of the request header in as API header_id parameter in 
 * request header access functions.
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
 * @details Used to access components of the response header in as API header_id parameter in 
 * response header access functions.
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
struct lsi_module_t;
struct lsi_api_t;
struct lsi_cb_param_t;
struct lsi_hook_info_t;
struct lsi_gd_cont_val_t;




/**
 * @typedef lsi_callback_pf 
 * @brief The callback function and its parameters.
 * @since 1.0
 */
typedef int     ( *lsi_callback_pf)  ( struct lsi_cb_param_t * );
/**
 * @typedef lsi_release_callback_pf 
 * @brief The memory release callback function for the user module data.
 * @since 1.0
 */
typedef int     ( *lsi_release_callback_pf)( void * );


/**
 * @typedef lsi_serialize_pf 
 * @brief The serializer callback function for the user global file data.  
 * Must use malloc to get the buffer and return the buffer.
 * @since 1.0
 */
typedef char * (*lsi_serialize_pf)      (void *pObject, int *out_length);

/**
 * @typedef lsi_deserialize_pf 
 * @brief The deserializer callback function for the user global file data.  
 * Must use malloc to get the buffer and return the buffer.
 * @since 1.0
 */
typedef void * (*lsi_deserialize_pf)    (char *, int length);

/**
 * @typedef lsi_timer_callback_pf 
 * @brief The timer callback function for the set timer feature.
 * @since 1.0
 */
typedef void    ( *lsi_timer_callback_pf) ( void* );




/**************************************************************************************************
 *                                       API Structures
 **************************************************************************************************/


/**
 * @struct lsi_cb_param_t
 * @brief Callback parameters passed to the callback functions
 * @since 1.0
 **/
typedef struct lsi_cb_param_t
{
    /**
     * @brief _session is a pointer to the session. 
     * @since 1.0
     */
    void        * _session;
    
    /**
     * @brief _hook_info is a pointer to the struct lsi_hook_info_t.
     * @since 1.0
     */
    struct lsi_hook_info_t   * _hook_info;
    
    /**
     * @brief _cur_hook is a pointer to the current hook.
     * @since 1.0
     */
    void                    * _cur_hook;
    
    /**
     * @brief _param1 is a pointer to the first parameter. 
     * @details Refer to the LSIAPI Developer's Guide's
     * Callback Parameter Definition table for a table 
     * of expected values for each _param based on use.
     * @since 1.0
     */
    const void              *   _param1;
    
    /**
     * @brief _param1_len is the length of the first parameter.
     * @since 1.0
     */
    int                         _param1_len;
    
    /**
     * @brief _param2 is a pointer to the second parameter.
     * @since 1.0
     */
    void                    *   _param2;
    
    /**
     * @brief _param2_len is the length of the second parameter.
     * @since 1.0
     */
    int                         _param2_len;
} lsi_cb_param_t; 



/**
 * @struct lsi_handler_t
 * @brief Pre-defined handler functions
 * @since 1.0
 */
typedef struct lsi_handler_t
{
    /**
     * @brief begin_process will be called when the server starts to process a request.
     * @details It is used to define the request handler callback function.
     * @since 1.0
     */
    int  ( *begin_process )     ( void *session );
    
    /**
     * @brief on_read_req_body will be called on a READ event with a large request body. 
     * @details on_read_req_body will be called when a request has a 
     * large request body that was not read completely. 
     * If not provided, this function should be set to NULL.
     * The default function will execute and return 0.
     * @since 1.0
     */
    int  ( *on_read_req_body )  ( void *session );   
    
    /**
     * @brief on_write_resp will be called on a WRITE event with a large response body.
     * @details on_write_resp will be called when the server gets a large response body
     * where the response did not write completely.
     * If not provided, set it to NULL.
     * The default function will execute and return LSI_WRITE_RESP_FINISHED.
     * @since 1.0
     */
    int  ( *on_write_resp )  ( void *session );
    
} lsi_handler_t;

/**
 * @struct lsi_config_t
 * @brief Contains functions which are used to define parse and release functions for the
 * user defined configuration parameters.
 * @details 
 * The parse functions will be called during module load.  The release function will be called on 
 * session end.  However, it is recommended that the user always performs a manual release of all 
 * allocated memory using the session_end filter hooks.
 * @since 1.0
 */
typedef struct lsi_config_t
{
    /**
     * @brief _parse_config is a callback function for the server to call to parse the user defined 
     * parameters and return a pointer to the user defined data.
     * @since 1.0
     */
    void *                          ( *_parse_config ) ( const char *param, void *_initial_config );
    
    /**
     * @brief _free_config is a callback function for the server to call to release a pointer to 
     * the user defined data. 
     * @since 1.0
     */
    void                            ( *_free_config ) ( void *_config );
} lsi_config_t;

/**
 * @struct lsi_module_t
 * @brief Defines an LSIAPI module, this struct must be provided in the module code.
 * @since 1.0
 */
typedef struct lsi_module_t
{
    /**
     * @brief _signature contains a function pointer that will be called after the module is loaded.
     * Used to initialize module data and perform hook function registration.
     * @since 1.0
     */
    int64_t                         _signature;
    
    /**
     * @brief _init is a function pointer that will be called after the module is loaded.
     * Used to initialize module data and perform hook function registration.
     * @since 1.0
     */
    int                            ( *_init )( );
    
    /**
     * @brief _handler needs to be provided if this module is a request handler. 
     * If not present, set to NULL.
     * @since 1.0
     */
    struct lsi_handler_t *          _handler;
    
    /**
     * @brief _config_parser contains functions which are used to parse user defined 
     * configuration parameters and to release the memory block.
     * If not present, set to NULL.
     * @since 1.0
     */
    struct lsi_config_t *           _config_parser;
    
    
    /**
     * @brief _info contains information about this module set by the developer, 
     * it can contain module version and/or version(s) of libraries used by this module.
     * If not present, set to NULL.
     * @since 1.0
     */
    const char *                    _info;
    
    /**
     * @brief Initially set to NULL.  After a module is loaded, 
     * it will be set to the module name.
     * @since 1.0
     */
    void                     *   _name;
    
    /**
     * @brief Initially set to 0.  After a module is loaded, 
     * it will be set to the module id.
     * @since 1.0
     */
    int                             _id;
    
    /**
     * @brief Initially set to 0.  After a module is loaded, 
     * it will be set to the user data id.
     * @since 1.0
     */
    short                           _data_id[LSI_MODULE_DATA_COUNT];
    
    /**
     * @brief Initially set to 0.  After a module is loaded, 
     * it will be set to the priorities for each hook level.
     * @since 1.0
     */
    int                             _priority[LSI_HKPT_TOTAL_COUNT]; 
} lsi_module_t;



/**************************************************************************************************
 *                                       API Functions
 **************************************************************************************************/

/**
 * @struct lsi_api_t
 * @brief LSIAPI function set.
 * @since 1.0
 **/
typedef struct lsi_api_t
{
    /**
     * @brief get_server_root is used to get the server root path.
     * 
     * @since 1.0
     * 
     * @return A \\0-terminated string containing the path to the server root directory. 
     */
    const char * ( *get_server_root )();
    
    /**
     * @brief log is used to write the formatted log to logfile. 
     * It can be used anywhere and anytime.
     * 
     * @since 1.0
     * 
     * @param[in] level - enum defined in log level definitions.
     * @param[in] fmt - formatted string.
     */
    void ( *log )( int level, const char * fmt, ... );

    /**
     * @brief session_log is used to write the formatted log to error log associated with a session.
     * @details Session ID will be added to log message automatically when session is not NULL.
     * This function will not add a trailing \\n to the end. 
     * 
     * @since 1.0
     * 
     * @param[in] session - current session, log file, and session ID are based on session.
     * @param[in] level - enum defined in log level definitions.
     * @param[in] fmt - formatted string.
     */
    void ( *session_log )( void *session, int level, const char * fmt, ... );

    /**
     * @brief session_vlog is used to write the formatted log to error log associated with a session
     * @details session ID will be added to the log message automatically when the session is not NULL
     * 
     * @since 1.0
     * 
     * @param[in] session - current session, log file and session ID is based on session.
     * @param[in] level - enum defined in log level definitions.
     * @param[in] fmt - formatted string.
     * @param[in] vararg - the varying argument list
     * @param[in] no_linefeed - 1 = do not add \\n at the end of message; 0 = add \\n
     */
    void ( *session_vlog )( void *session, int level, const char * fmt, va_list vararg, int no_linefeed );
    
    /**
     * @brief session_lograw is used to write additional log messages to error log file associated with a session
     * No timestamp, log level, session ID prefix added, just the data in the buffer logged into log files
     * This is usually used for adding more content to previous log message
     * 
     * @since 1.0
     * 
     * @param[in] session - log file is based on session, if set to NULL, the default log file is used.
     * @param[in] buf - data to be written to log file.
     * @param[in] len - the size of data to be logged 
     */
    void ( *session_lograw )( void *session, const char * buf, int len );

    /**
     * @brief get_module_param is used to get the user defined module parameters which are parsed by 
     * the callback _parse_config and pointed to in the struct lsi_config_t.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession or use a NULL to the server level.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @return a pointer to a the user-defined configuration data.
     */
    void * (* get_module_param)( void *session, const lsi_module_t *pModule );
    
    /**
     * @brief add_hook function is used to add a global level hook function.
     * It should only be used in lsi_module::_init().
     * 
     * @since 1.0
     * 
     * @param[in] index - as defined in the enum of Hook Point level definitions.
     * @param[in] pModule - the  pointer to the lsi_module_t struct.
     * @param[in] cb - the pointer to the associated callback function.
     * @param[in] priority - the default hook-point priority. lower value indicates 
     * higher priority and will get executed first. 
     * May be overwritten by the web admin configuration.
     * @param[in] flag - flag for a hook-point, LSI_HOOK_FLAG_TRANSFORM should be set 
     * if a hook-point modifies input data, otherwise put 0.
     * @return -1 on failure, 0 for already added, location in the hook-point chain on success.
     * 
     */
    int ( *add_hook )( int index, const lsi_module_t *pModule, lsi_callback_pf cb, short priority, short flag );
    
    /**
     * @brief add_session_hook is used to add a session level hook function.
     * This should only be used after a session is already created
     * and the hook will only be used in this session. 
     * There are L4 level and HTTP level session hooks.
     * 
     * @since 1.0
     * 
     * @param[in] session - pointer to void, obtained from callback parameters, 
     * ultimately points to LsiSession.
     * @param[in] index - as defined in the enum of Hook Point level definitions.
     * @param[in] pModule - a pointer to the lsi_module_t struct.
     * @param[in] cb - the pointer to the associated callback function.
     * @param[in] priority - the default hook-point priority. Lower value indicates 
     * higher priority and will get executed first. 
     * May be overwritten by the web admin configuration.
     * @param[in] flag - flag for a hook point, LSI_HOOK_FLAG_TRANSFORM should be set 
     * if a hook-point modifies input data, otherwise put 0.
     * @return -1 on failure, 0 for already added, location in the filter chain on success.
     */
    int ( *add_session_hook )( void * session, int index, const lsi_module_t *pModule, lsi_callback_pf cb, short priority, short flag );
    
    /**
     * @brief remove_session_hook is used to remove a session level hook function.
     * This should only be used after a session is already created
     * and the hook will only be used in this session. 
     * There are L4 level and HTTP level session hooks.
     * 
     * @since 1.0
     * 
     * @param[in] session - pointer to void, obtained from callback parameters, 
     * ultimately points to LsiSession.
     * @param[in] index - as defined in the enum of Hook Point level definitions.
     * @param[in] pModule - a pointer to the lsi_module_t struct.
     */
    int ( *remove_session_hook )( void * session, int index, const lsi_module_t *pModule );
    
    /**
     * @brief init_module_data is used to initialize module data of a certain level(scope).
     * init_module_data must be called before using set_module_data or get_module_data.
     * 
     * @since 1.0
     * 
     * @param[in] pModule - a pointer to the current module defined in lsi_module_t struct
     * @param[in] cb - a pointer to the user-defined callback function that releases the user data.
     * @param[in] level - as defined in the module data level enum.
     * @return -1 for wrong level, -2 for already initialized, 
     * hook location of lsi_release_callback_pf from add_hook on success.
     */
    int ( *init_module_data )( lsi_module_t *pModule, lsi_release_callback_pf cb, int level );
    
    /**
     * @brief Before using FILE type module data, the data must be initialized by 
     * calling init_file_type_mdata with the file path.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] path - a pointer to the path string.
     * @param[in] pathLen - the length of the path string.
     * @return -1 on failure, file descriptor on success.
     */    
    int ( *init_file_type_mdata )( void *session, const lsi_module_t *pModule, const char *path, int pathLen );
       
    /**
     * @brief set_module_data is used to set the module data of a session level(scope).
     * The sParam is a pointer to the user's own data structure.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] level - as defined in the module data level enum.
     * @param[in] sParam - a pointer to the user defined data.
     * @return -1 for bad level or no release data callback function, 0 on success.
     */
    int ( *set_module_data )( void *session, const lsi_module_t *pModule, int level, void *sParam );
    
    /**
     * @brief get_module_data gets the module data which was set by set_module_data.
     * The return value is a pointer to the user's own data structure.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] level - as defined in the module data level enum.
     * @return NULL on failure, a pointer to the user defined data on success.
     */
    void * ( *get_module_data )( void *session, const lsi_module_t *pModule, int level );
    
    /**
     * @brief free_module_data is to be called when the user needs to free the module data immediately.
     * It is not used by the web server to call the release callback later.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] level - as defined in the module data level enum.
     * @param[in] cb - a pointer to the user-defined callback function that releases the user data.
     */    
    void ( *free_module_data )( void * session, const lsi_module_t *pModule, int level, lsi_release_callback_pf cb );
    
    /**
     * @brief stream_writev_next needs to be called in the LSI_HKPT_L4_SENDING hook point level just 
     * after it finishes the action and needs to call the next step.
     * 
     * @since 1.0
     * 
     * @param[in] pParmam - the callback parameters to be sent to the current hook callback function.
     * @param[in] iov - the IO vector of data to be written.
     * @param[in] count - the size of the IO vector.
     * @return the return value from the hook filter callback function.
     */    
    int ( *stream_writev_next )( lsi_cb_param_t * pParam, struct iovec *iov, int count );
    
    /**
     * @brief stream_read_next needs to be called in the LSI_HKPT_L4_RECVING hook point level just 
     * after it finishes the action and needs to call the next step.
     * 
     * @since 1.0
     * 
     * @param[in] pParmam - the callback parameters to be sent to the current hook callback function.
     * @param[in,out] pBuf - a pointer to a buffer provided to hold the read data.
     * @param[in] size - the buffer size.
     * @return the return value from the hook filter callback function.
     */    
    int ( *stream_read_next )( lsi_cb_param_t * pParam, char *pBuf, int size );
     
    /**
     * @brief filter_next is used to write the response body to the next function 
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
    int ( *filter_next )( lsi_cb_param_t * pParam, const char *buf, int len );
    
    /**
     * @brief get_gd_container is used to get the global data container which is determined by the given key.
     * It will retrieve it if it exists, otherwise it will create a new one. 
     * There are two types of the containers, one is memory type and the other is the file type.
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
    struct lsi_gd_cont_val_t *( *get_gd_container)( int type, const char *key, int key_len );
    
    /**
     * @brief empty_gd_container is used to delete all the data indices in the global data container. 
     * It won't delete the physical files in the directory of the container.
     * 
     * @since 1.0
     * 
     * @param[in] container - the global data container.
     * @return -1 on failure, 0 on success.
     */
    int ( *empty_gd_container )( struct lsi_gd_cont_val_t *container );
    
    /**
     * @brief purge_gd_container is used to delete the physical files of the empty container.
     * 
     * @since 1.0
     * 
     * @param[in] container - the global data container.
     * @return -1 on failure, 0 on success.
     */
    int ( *purge_gd_container )( struct lsi_gd_cont_val_t *container );
    
    /**
     * @brief get_gdata is used to get the global data which was already set to the container.
     * deserialize_cb needs to be provided in case the data index does not exist but the file
     * is cached.  In this case, the data can be deserialized from the file.
     * It will be released with the release_cb callback when the data is deleted.
     * 
     * @since 1.0
     * 
     * @param[in] container - the global data container.
     * @param[in] key - a pointer to a container ID string.
     * @param[in] key_len - the string length.
     * @param[in] cb - a pointer to the user-defined callback function that releases the global data.
     * @param[in] renew_TTL - time to live. 
     * @param[in] deserialize_cb - a pointer to the user-defined deserializer callback function used for file read.
     * @return a pointer to the user-defined global data, (as defined in gd_item_val_t.value) on success,
     * NULL on failure.
     */
    void * ( *get_gdata )( struct lsi_gd_cont_val_t *container, const char *key, int key_len, 
                           lsi_release_callback_pf release_cb, int renew_TTL, lsi_deserialize_pf deserialize_cb);
    
    /**
     * @brief delete_gdata is used to delete the global data which was already set to the container.
     * 
     * @since 1.0
     * 
     * @param[in] container - the global data container.
     * @param[in] key - a pointer to a container ID string.
     * @param[in] key_len - the string length.
     * @return 0.
     */
    int ( *delete_gdata )( struct lsi_gd_cont_val_t *container, const char *key, int key_len );
    
    /**
     * @brief set_gdata is used to set the global data (void * val) with a given key.  
     * A TTL(time to live) needs to be set.
     * lsi_release_callback_pf needs to be provided for use when the data is to be deleted.
     * If the container is a FILE type, a lsi_serialize_pf will be called to get the buffer 
     * which will be written to file.
     * If the data exists and force_update is 0, this function won't update the data.
     * 
     * @since 1.0
     * 
     * @param[in] container - the global data container.
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
    int ( *set_gdata )( struct lsi_gd_cont_val_t *container, const char *key, int key_len, void *val, int TTL, 
                        lsi_release_callback_pf release_cb, int force_update, lsi_serialize_pf serialize_cb );
        
    /**
     * @brief get_req_raw_headers_length can be used to get the length of the total request headers.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @return length of the request header.
     */
    int ( *get_req_raw_headers_length )( void *session );
    
    /**
     * @brief get_req_raw_headers can be used to store all of the request headers in a given buffer.
     * If maxlen is too small to contain all the data, 
     * it will only copy the amount of the maxlen.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in,out] buf - a pointer to a buffer provided to hold the header strings.
     * @param[in] maxlen - the size of the buffer.
     * @return - the length of the request header.
     */
    int ( *get_req_raw_headers )( void *session, char *buf, int maxlen );
    
    /**
     * @brief get_req_header can be used to get a request header based on the given key.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] key - a pointer to a string describing the header label, (key).
     * @param[in] keylen - the size of the string.
     * @param[in,out] valLen - a pointer to the length of the returned string.
     * @return a pointer to the request header key value string.
     */
    const char* ( *get_req_header )( void *session, const char *key, int keyLen, int *valLen );
    
    /**
     * @brief get_req_header_by_id can be used to get a request header based on the given header id 
     * defined in lsi_req_header_id
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] id - header id defined in lsi_req_header_id
     * @param[in,out] valLen - a pointer to the length of the returned string.
     * @return a pointer to the request header key value string.
     */
    const char* ( *get_req_header_by_id )( void *session, int id, int *valLen );
    
    /**
     * @brief get_req_org_uri is used to get the original URI as delivered in the request, 
     * before any processing has taken place.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in,out] buf - a pointer to the buffer for the URI string.
     * @param[in] buf_size - max size of the buffer.
     * 
     * @return length of the URI string.
     */
    int ( *get_req_org_uri )( void *session, char *buf, int buf_size );
    
    
    /**
     * @brief get_req_uri can be used to get the URI of a HTTP session.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @return pointer to the URI string.
     */
    const char* ( *get_req_uri )( void *session );
    
    
    /**
     * @brief get_mapped_context_uri can be used to get the context URI of an HTTP session.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in,out] length - the length of the returned string.
     * @return pointer to the context URI string.
     */
    const char* ( *get_mapped_context_uri )( void *session, int *length );
    
    
    /**
     * @brief register_req_handler can be used to dynamically register a handler.  
     * The scriptLen is the length of the script.  To call this function, 
     * the module needs to provide the lsi_handler_t (not set to NULL).
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] pModule - a pointer to an lsi_module_t struct.
     * @param[in] scriptLen - the length of the script name string.
     * @return -1 on failure, 0 on success.
     */    
    int ( *register_req_handler )( void *session, lsi_module_t *pModule, int scriptLen );
    
    
    /**
     * @brief set_handler_write_state can change the calling of on_write_resp() of a module handler,
     * if the state is 0, the calling will be suspended, 
     * otherwise, if it is 1, it will continue to call when not finished.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] state - 
     * @return 
     * 
     */
    int ( *set_handler_write_state )( void *session, int state );
    
    /**
     * @brief set_timer sets a timer.
     * 
     * @since 1.0
     * 
     * @param[in] timeout_ms - timeout in ms.
     * @param[in] timer_cb - callback function to be called on timeout.
     * @return timer id
     * 
     */
    int ( *set_timer )( unsigned int timeout_ms, lsi_timer_callback_pf timer_cb, void* timer_cb_param );
    
    /**
     * @brief remove_timer removes a timer.
     * 
     * @since 1.0
     * 
     * @param[in] time_id - timer id
     * @return 0.
     * 
     */
    int ( *remove_timer )( int time_id );
    
    /**
     * get_req_cookies is used to get all the request cookies.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in,out] len - a pointer to the length of the cookies string.
     * @return a pointer to the cookie key string.
     */
    const char * ( *get_req_cookies )( void *session, int *len );
    
    /**
     * @brief get_req_cookie_count is used to get the request cookies count.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @return the number of cookies.
     */
    int ( *get_req_cookie_count )( void *session );
    
    /**
     * @brief get_cookie_value is to get a cookie based on the cookie name.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] cookie_name - a pointer to the cookie name string.
     * @param[in] nameLen - the length of the cookie name string.
     * @param[in,out] valLen - a pointer to the length of the cookie string.
     * @return a pointer to the cookie string.
     */    
    const char * ( *get_cookie_value )( void *session, const char * cookie_name, int nameLen, int *valLen );
    
    /**
     * @brief get_client_ip is used to get the request ip address.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in,out] len - a pointer to the length of the IP string.
     * @return a pointer to the IP string.
     */
    const char * ( *get_client_ip )( void *session, int *len );
    
    /**
     * @brief get_req_query_string is used to get the request query string.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in,out] len - a pointer to the length of the query string.
     * @return a pointer to the query string.
     */
    const char * ( *get_req_query_string )( void *session, int *len );
    
    /**
     * @brief get_req_env is used to get the value of a server variable and 
     * environment variable based on the name.  It will also get the env that is set by set_req_env.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] name - a pointer to the variable name string.
     * @param[in] nameLen - the length of the variable name string.
     * @param[in,out] val - a pointer to the variable value string.
     * @param[in] maxValLen - the maximum size of the variable value string.
     * @return the length of the variable value string.
     */
    int ( *get_req_env )( void *session, const char *name, unsigned int nameLen, char *val, int maxValLen );
    
    
    /**
     * @brief get_req_env_by_id is used to get the value of a server variable and 
     * environment variable by the env type.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] id - enum defined as LSIAPI request variable ID.
     * @param[in,out] val - a pointer to the variable value string.
     * @param[in] maxValLen - the maximum size of the variable value string.
     * @return the length of the variable value string.
     */    
    int ( *get_req_env_by_id )( void *session, int id, char *val, int maxValLen );
    
    
    /**
     * @brief set_req_env is used to set a request environment variable.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] name - a pointer to the variable name string.
     * @param[in] nameLen - the length of the variable name string.
     * @param[in] val - a pointer to the variable value string.
     * @param[in] valLen - the size of the variable value string.
     */    
    void ( *set_req_env )( void *session, const char *name, unsigned int nameLen, const char *val, int valLen );
    
    
    /**
     * @brief register_env_handler is used to register a callback with an set_req_env defined by a env_name, 
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
    int ( *register_env_handler )( const char *env_name, unsigned int env_name_len, lsi_callback_pf cb );
    
    /**
     * @brief get_uri_file_path will get the real file path mapped to the request uri.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] uri - a pointer to the URI string.
     * @param[in] uri_len - the length of the URI string.
     * @param[in,out] path - a pointer to the path string.
     * @param[in] max_len - the max length of the path string.
     * @return the length of the path string.
     */
    int ( *get_uri_file_path )( void *session, const char *uri, int uri_len, char *path, int max_len );
    
    /**
     * @brief set_uri_qs will redirect the current session to a new uri.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] action - action to be taken after changing URI and Query String
     * @param[in] uri - a pointer to the URI string.
     * @param[in] len - the length of the URI string.
     * @param[in] qs -  a pointer to the Query String. 
     * @param[in] qs_len - the length of the Query String.
     * @return -1 on failure, 0 on success.
     */    
    int ( *set_uri_qs )( void *session, int action, const char *uri, int uri_len, const char *qs, int qs_len );
    
    /**
     * @brief get_req_content_length is used to get the content length of the request.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @return content length.
     */    
    int ( *get_req_content_length )( void *session );
    
    /**
     * @brief read_req_body is used to get the request body to a given buffer.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in,out] buf - Pointer to a buffer provided to hold the header strings.
     * @param[in] buflen - size of the buffer.
     * @return length of the request body.
     */    
    int ( *read_req_body )( void *session, char *buf, int bufLen );
    
    /**
     * @brief get_req_body_file_fd is used to get file descriptor of the memory file of the request body. 
     * The file descriptor should not be closed in the module.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @return -1 on failure, request body file fd on success.
     */    
    int ( *get_req_body_file_fd )( void *session );
    
    /**
     * @brief is_req_body_finished 
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @return 
     * 
     */
    int ( *is_req_body_finished )( void *session );
    
    /**
     * @brief set_req_wait_full_body is used to make the server wait to call
     * begin_process until after the full request body is received.  
     * @details If the user calls this function within begin_process, the 
     * server will call on_read_req_body only after the full request body is received.
     * Please refer to the Request Data Access section of the 
     * LSWS Module Developer's Guide for a more in-depth explanation of the
     * purpose of this function if you are still confused.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @return 
     */
    int ( *set_req_wait_full_body )( void *session );
    
    /**
     * @brief set_status_code is used to set the response status code of a HTTP session.
     * It can be used in Hook Point levels LSI_HKPT_RECV_REQ_HEADER, 
     * LSI_HKPT_HTTP_REQ_BODY_RECVING and LSI_HKPT_HTTP_REQ_BODY_RECVED 
     * and handler processing functions.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] code - the http response status code.
     */    
    void ( *set_status_code )( void *session, int code );
    
    
    /**
     * @brief get_status_code is used to get the response status code of an HTTP session.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @return the http response status code.
     */
    int ( *get_status_code )( void *session );
    
    
    /**
     * @brief is_resp_buffer_available is used to check if the response buffer is available to fill in data.
     * This function is supposed to be called before append_resp_body or append_resp_bodyv is called.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @return -1 on failure, 0 false, 1 true.
     */
    int ( *is_resp_buffer_available)( void *session );
    
    /**
     * @brief append_resp_body is used to append a buffer to the response body.
     * It can ONLY be used by handler functions.
     * The data will go through filters for post processing.  
     * This function should NEVER be called from a filter post processing the data.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] buf - a pointer to a buffer to be written.
     * @param[in] buflen - the size of the buffer.
     * @return the length of the request body.
     */    
    int ( *append_resp_body )( void *session, const char *buf, int len );
    
    /**
     * @brief append_resp_bodyv is used to append an iovec to the response body.
     * It can ONLY be used by handler functions.
     * The data will go through filters for post processing.  
     * this function should NEVER be called from a filter post processing the data.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] iov - the IO vector of data to be written.
     * @param[in] count - the size of the IO vector.
     * @return -1 on failure, return value of the hook filter callback function.
     */    
    int ( *append_resp_bodyv )( void *session, const struct iovec *iov, int count );
    
    /**
     * @brief send_file is used to send a file as the response body.
     * It can be used in handler processing functions.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] path - a pointer to the path string.
     * @param[in] start - file start offset.
     * @param[in] size - remaining size.
     * @return -1 or error codes from various layers of calls on failure, 0 on success.
     */
    int ( *send_file )( void *session, const char *path, off_t start, off_t size );
    
    
    /**
     * @brief flush flushes the connection and sends the data.  
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     */
    void ( *flush )( void *session );
    
    /**
     * @brief end_resp is called when the response sending is complete.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     */
    void ( *end_resp )( void *session );
    
    /**
     * @brief set_resp_content_length sets the Content Length of the response. 
     * If len is -1, the response will be set to chunck encoding.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] len - the content length.
     * @return 0. 
     */    
    int ( *set_resp_content_length )( void *session, off_t len );
    
    /**
     * @brief set_resp_header is used to set a response header.  
     * If the header does not exist, it will add a new header. 
     * If the header exists, it will add the header based on 
     * the add_method - replace, append, merge and add new line.
     * It can be used in LSI_HKPT_RECV_RESP_HEADER and handler processing functions.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] header_id - enum defined as response-header id
     * @param[in] name - a pointer to the header id name string.
     * @param[in] nameLen - the length of the header id name string.
     * @param[in] val - a pointer to the header string to be set.
     * @param[in] valLen - the length of the header value string.
     * @param[in] add_method - enum defined for the method of adding.
     * @return 0.
     */    
    int ( *set_resp_header )( void *session, unsigned int header_id, const char *name, int nameLen, const char *val, int valLen, int add_method );
    
    /**
     * @brief set_resp_header2 is used to parse the headers first then perform set_resp_header.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] headers - a pointer to the header string to be set.
     * @param[in] len - the length of the header value string.
     * @param[in] add_method - enum defined for the method of adding.
     * @return 0.
     */    
    int ( *set_resp_header2 )( void *session, const char *headers, int len, int add_method );
    
    /**
     * @brief get_resp_header is used to get a response header's value in an iovec array. 
     * It will try to use header_id to search the header first. 
     * If header_id is not LSI_RESP_HEADER_UNKNOWN, the name and nameLen will NOT be checked, 
     * they can be set to NULL and 0.
     * Otherwise, if header_id is LSI_RESP_HEADER_UNKNOWN, then it will search through name and nameLen.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] header_id - enum defined as response-header indices
     * @param[in] name - a pointer to the header id name string.
     * @param[in] nameLen - the length of the header id name string.
     * @param[out] iov - the IO vector that contains the headers.
     * @param[in] maxIovCount - size of the IO vector.
     * @return the count of headers in the IO vector.
     */    
    int ( *get_resp_header )( void *session, unsigned int header_id, const char *name, int nameLen, struct iovec *iov, int maxIovCount );
    
    /**
     * @brief get_resp_headers_count is used to get the count of the response headers. 
     * Multiple lines headers will be counted as different headers.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @return the number of headers in the whole response header.
     */
    int ( *get_resp_headers_count )( void *session );
    
    /**
     * @brief get_resp_headers it used to get the whole response headers and store them to the iovec array.
     * If maxIovCount is smaller than the count of the headers, 
     * it will only store the first maxIovCount headers.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] iov - the IO vector that contains the headers.
     * @param[in] maxIovCount - size of the IO vector.
     * @return the count of all the headers.
     */
    int ( *get_resp_headers )( void *session, struct iovec *iov, int maxIovCount );
    
    /**
     * @brief remove_resp_header is used to remove a response header.  
     * The header_id should match the name and nameLen if it isn't -1.
     * Providing the header_id will make finding of the header quicker. It can be used in 
     * LSI_HKPT_RECV_RESP_HEADER and handler processing functions.
     * 
     * @since 1.0
     * 
     * @param[in] session - a pointer to void, ultimately to HttpSession.
     * @param[in] header_id - enum defined as response-header id
     * @param[in] name - a pointer to the header id name string.
     * @param[in] nameLen - the length of the header id name string.
     * @return 0.
     */    
    int ( *remove_resp_header )( void *session, unsigned int header_id, const char *name, int nameLen );
    
    /**
     * @brief get_multiplexer gets the session multiplexer. 
     * 
     * @since 1.0
     * @return a pointer to the multiplexer used by the main event loop. 
     */    
    void * ( *get_multiplexer )();

    
} lsi_api_t;

/**
  * @brief  make LSIAPI functions globally available.  
  * @details g_api is a global variable, it can be accessed from all modules to make API calls.  
  * 
  * @since 1.0
  */    

extern const lsi_api_t * g_api;

#ifdef __cplusplus
}
#endif


#endif //LS_MODULE_H