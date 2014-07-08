/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
#include "httpglobals.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include <util/pool.h>
Pool g_pool;
//EventDispatcher      * HttpGlobals::s_pDispatcher = NULL;
Multiplexer          * HttpGlobals::s_pMultiplexer = NULL;
AccessControl        * HttpGlobals::s_pAccessCtrl = NULL;
HttpMime             * HttpGlobals::s_pMime = NULL;
int                    HttpGlobals::s_iMultiplexerType = 0;

#include <http/connlimitctrl.h>
ConnLimitCtrl          HttpGlobals::s_connLimitCtrl;

#include <http/clientcache.h>
ClientCache          * HttpGlobals::s_pClients = NULL;


#include <http/httpsession.h>

#include <http/clientinfo.h>
int HttpGlobals::s_iConnsPerClientSoftLimit = INT_MAX;
int HttpGlobals::s_iOverLimitGracePeriod = 10;
int HttpGlobals::s_iConnsPerClientHardLimit = 100;
int HttpGlobals::s_iBanPeriod = 60;
ThrottleLimits ThrottleControl::s_default;


#include <http/httplog.h>
int    HttpLog::s_debugLevel        = DL_IODATA;

#include <util/datetime.h>
time_t DateTime::s_curTime = time( NULL );
int    DateTime::s_curTimeUs = 0;

long HttpGlobals::s_lBytesRead = 0;
long HttpGlobals::s_lBytesWritten = 0;
long HttpGlobals::s_lSSLBytesRead = 0;
long HttpGlobals::s_lSSLBytesWritten = 0;
int  HttpGlobals::s_iIdleConns = 0;
ReqStats HttpGlobals::s_reqStats;

#include <http/ntwkiolink.h>
class NtwkIOLink::fp_list NtwkIOLink::s_normal
(
    NtwkIOLink::readEx,
    NtwkIOLink::writevEx,
    NtwkIOLink::onWrite,
    NtwkIOLink::onRead,
    NtwkIOLink::close_,
    NtwkIOLink::onTimer_
);

class NtwkIOLink::fp_list NtwkIOLink::s_normalSSL
(
    NtwkIOLink::readExSSL,
    NtwkIOLink::writevExSSL,
    NtwkIOLink::onWriteSSL,
    NtwkIOLink::onReadSSL,
    NtwkIOLink::closeSSL,
    NtwkIOLink::onTimer_
);

class NtwkIOLink::fp_list NtwkIOLink::s_throttle 
(
    NtwkIOLink::readExT,
    NtwkIOLink::writevExT,
    NtwkIOLink::onWriteT,
    NtwkIOLink::onReadT,
    NtwkIOLink::close_,
    NtwkIOLink::onTimer_T
);

class NtwkIOLink::fp_list NtwkIOLink::s_throttleSSL
(
    NtwkIOLink::readExSSL_T,
    NtwkIOLink::writevExSSL_T,
    NtwkIOLink::onWriteSSL_T,
    NtwkIOLink::onReadSSL_T,
    NtwkIOLink::closeSSL,
    NtwkIOLink::onTimerSSL_T
);

class NtwkIOLink::fp_list_list   NtwkIOLink::s_fp_list_list_normal 
(
    &NtwkIOLink::s_normal,
    &NtwkIOLink::s_normalSSL
);

class NtwkIOLink::fp_list_list   NtwkIOLink::s_fp_list_list_throttle
(
    &NtwkIOLink::s_throttle,
    &NtwkIOLink::s_throttleSSL
);

class NtwkIOLink::fp_list_list  *NtwkIOLink::s_pCur_fp_list_list =
    &NtwkIOLink::s_fp_list_list_normal;

#include <http/httpserverconfig.h>
HttpServerConfig HttpServerConfig::s_config;

#include <http/httpmethod.h>
const char * HttpMethod::s_psMethod[HttpMethod::HTTP_METHOD_END] =
{
    "UNKNOWN",
    "OPTIONS",
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "TRACE",
    "CONNECT",
    "MOVE",
    "PROPFIND",
    "PROPPATCH",
    "MKCOL",
    "COPY",
    "LOCK",
    "UNLOCK",
    "VERSION-CONTROL",
    "REPORT",
    "CHECKIN",
    "CHECKOUT",
    "UNCHECKOUT",
    "UPDATE",
    "MKWORKSPACE",
    "LABEL",
    "MERGE",
    "BASELINE-CONTROL",
    "MKACTIVITY",
    "BIND",
    "SEARCH",
    "PURGE",
    "REFRESH"
    
};
int HttpMethod::s_iMethodLen[HttpMethod::HTTP_METHOD_END] =
{
    7, 7, 3, 4, 4, 3, 6, 5, 7, 4,
    8, 9, 5, 4, 4, 6, 15, 6, 7, 8, 10, 7, 11, 5, 5, 16, 10, 4, 6, 5, 7
};

#include <http/httpheader.h>
int HttpHeader::s_iHeaderLen[H_HEADER_END+1] =
{
    6, 14, 15, 15, 13, 10, 12, 14, 6, 7, 4, 6, 7, 10, //user-agent
    13,17, 8, 13, 8, 19, 10, 5, 15, 3, 17, 2, 6, 12, 19, 4,  //date
    7, 7, 7, 5, 16, 16, 16, 11, 13, 7, 13, //last-modified
    //13,3,4,8,18,16,11,6,4,16,10,11,6,20,19,25,
    0
};

int HttpRespHeaders::s_iHeaderLen[H_HEADER_END+1] =
{
    13, 10, 12, 14, 16, 13, 19, 13, //cache-control
    4, 4, 7, 10, 13, 8, 20, 23, //litespeed-cache-control
    6, 16, 6, 10, 6, 17, 4, 16, 12, //x-powered-by
    0
};

#include <http/denieddir.h>
DeniedDir            HttpGlobals::s_deniedDir;

#include "staticfilehandler.h"
StaticFileHandler        s_staticFileHandler;
RedirectHandler          s_redirectHandler;

#include "ssi/ssiengine.h"
SSIEngine                s_ssiHandler;

#include <http/staticfilecache.h>
StaticFileCache        HttpGlobals::s_staticFileCache( 1000 );

#include <lsiapi/modulehandler.h>
ModuleHandler           s_ModuleHandler;


#include <http/httprespheaders.h>

char HttpRespHeaders::s_sDateHeaders[30] = "Tue, 09 Jul 2013 13:43:01 GMT";
int             HttpRespHeaders::s_commonHeadersCount = 2;
http_header_t       HttpRespHeaders::s_commonHeaders[2];
http_header_t       HttpRespHeaders::s_gzipHeaders[2];
http_header_t       HttpRespHeaders::s_keepaliveHeader;
http_header_t       HttpRespHeaders::s_chunkedHeader;
http_header_t       HttpRespHeaders::s_concloseHeader;
http_header_t       HttpRespHeaders::s_acceptRangeHeader;


TLinkList<ModuleTimer> HttpGlobals::s_ModuleTimerList;
    
#include <http/httpver.h>
#include <http/httpstatuscode.h>

const char * const HttpVer::s_sHttpVer[2] =
{
    "HTTP/1.1",
    "HTTP/1.0"
};

StatusCode HttpStatusCode::s_pSC[ SC_END ] =
{
    StatusCode( 0, " 200 OK\r\n",NULL ),
    StatusCode( SC_100, " 100 Continue\r\n", NULL),
    StatusCode( SC_101, " 101 Switching Protocols\r\n", NULL),
    StatusCode( SC_102, " 102 Processing\r\n", NULL),
    
    StatusCode( SC_200, " 200 OK\r\n", NULL),
    StatusCode( SC_201, " 201 Created\r\n", NULL),
    StatusCode( SC_202, " 202 Accepted\r\n", NULL),
    StatusCode( SC_203, " 203 Non-Authoritative Information\r\n", NULL),
    StatusCode( SC_204, " 204 No Content\r\n", NULL),
    StatusCode( SC_205, " 205 Reset Content\r\n", NULL),
    StatusCode( SC_206, " 206 Partial Content\r\n", NULL),
    StatusCode( SC_207, " 207 Multi-Status\r\n", NULL),
    StatusCode( SC_208, " 208 Already Reported\r\n", NULL),
    
    StatusCode( SC_300, " 300 Multiple Choices\r\n", NULL),
    StatusCode( SC_301, " 301 Moved Permanently\r\n",
                    "The document has been permanently moved to <A HREF=\"%s\">here</A>."),
    StatusCode( SC_302, " 302 Found\r\n", "The document has been temporarily moved to <A HREF=\"%s\">here</A>."),
    StatusCode( SC_303, " 303 See Other\r\n", "The answer to your request is located <A HREF=\"%s\">here</A>."),
    StatusCode( SC_304, " 304 Not Modified\r\n", NULL),
    StatusCode( SC_305, " 305 Use Proxy\r\n", "The resource is only accessible through the proxy!"),
    StatusCode( SC_306, "", NULL),
    StatusCode( SC_307, " 307 Temporary Redirect\r\n",
                        "The document has been temporarily moved to <A HREF=\"%s\">here</A>."),
                        
    StatusCode( SC_400, " 400 Bad Request\r\n", "It is not a valid request!"),
    StatusCode( SC_401, " 401 Unauthorized\r\n", "Proper authorization is required to access this resource!"),
    StatusCode( SC_402, " 402 Payment Required\r\n", NULL),
    StatusCode( SC_403, " 403 Forbidden\r\n", "Access to this resource on the server is denied!"),
    StatusCode( SC_404, " 404 Not Found\r\n", "The resource requested could not be found on this server!"),
    StatusCode( SC_405, " 405 Method Not Allowed\r\n", "This type request is not allowed!"),
    StatusCode( SC_406, " 406 Not Acceptable\r\n", NULL),
    StatusCode( SC_407, " 407 Proxy Authentication Required\r\n", NULL),
    StatusCode( SC_408, " 408 Request Time-out\r\n", NULL),
    StatusCode( SC_409, " 409 Conflict\r\n", "The request could not be completed due to a conflict "
                        "with the current state of the resource."),
    StatusCode( SC_410, " 410 Gone\r\n", "The requested resource is no longer available at the server "
                        "and no forwarding address is known."),
    StatusCode( SC_411, " 411 Length Required\r\n", "Lenth of body must be present in the request header!"),
    StatusCode( SC_412, " 412 Precondition Failed\r\n", NULL),
    StatusCode( SC_413, " 413 Request Entity Too Large\r\n", "The request body is over the maximum size allowed!"),
    StatusCode( SC_414, " 414 Request-URI Too Large\r\n", "The request URL is over the maximum size allowed!"),
    StatusCode( SC_415, " 415 Unsupported Media Type\r\n", "The media type is not supported by the server!"),
    StatusCode( SC_416, " 416 Requested range not satisfiable\r\n",
                        "None of the range specified overlap the current extent of the selected resource.\n"),
    StatusCode( SC_417, " 417 Expectation Failed\r\n", NULL),
    StatusCode( SC_418, " 418 reauthentication required\r\n", NULL),
    StatusCode( SC_419, " 419 proxy reauthentication required\r\n", NULL),
    StatusCode( SC_420, " 420 Policy Not Fulfilled\r\n", NULL),
    StatusCode( SC_421, " 421 Bad Mapping\r\n", NULL),    
    StatusCode( SC_422, " 422 Unprocessable Entity\r\n", NULL ),
    StatusCode( SC_423, " 423 Locked\r\n", NULL ),
    StatusCode( SC_424, " 424 Failed Dependency\r\n", NULL ),
    
    StatusCode( SC_500, " 500 Internal Server Error\r\n", "An internal server error has occured."),
    StatusCode( SC_501, " 501 Not Implemented\r\n", "The requested method is not implemented by the server."),
    StatusCode( SC_502, " 502 Bad Gateway\r\n", NULL),
    StatusCode( SC_503, " 503 Service Unavailable\r\n", "The server is temporarily busy, try again later!"),
    StatusCode( SC_504, " 504 Gateway Time-out\r\n", NULL),
    StatusCode( SC_505, " 505 HTTP Version not supported\r\n", "Only HTTP/1.0, HTTP/1.1 is supported." ),
    StatusCode( SC_506, " 506 Loop Detected\r\n", NULL),
    StatusCode( SC_507, " 507 Insufficient Storage\r\n", NULL),
    StatusCode( SC_508, " ", NULL ),
    StatusCode( SC_509, " 509 Bandwidth Limit Exceeded", NULL ),
    StatusCode( SC_510, " 510 Not Extended", NULL )
};

int HttpStatusCode::s_codeToIndex[7] =
{
    0, SC_100, SC_200, SC_300, SC_400, SC_500, SC_END
};

#include <http/httpresourcemanager.h>
HttpResourceManager    HttpGlobals::s_ResManager;


#include <http/httpstatusline.h>
#include <string.h>

#define STATUS_LINE_BUF_SIZE 64 * SC_END * 2

static  char s_achBuf[STATUS_LINE_BUF_SIZE];
static  char * s_pEnd = s_achBuf;

StatusLineString::StatusLineString( int version, int code )
{
    if ( code > 0 )
    {
        register int verLen = HttpVer::getVersionStringLen( version );
        register int codeLen = HttpStatusCode::getCodeStringLen( code );
        m_iLineLen = verLen + codeLen ;
        m_pLine = s_pEnd;
        memcpy( s_pEnd, HttpVer::getVersionString(version), verLen );
        memcpy( s_pEnd + verLen, HttpStatusCode::getCodeString( code ), codeLen );
        s_pEnd += m_iLineLen;
    }
    else
    {
        m_pLine = NULL;
        m_iLineLen = 0;
    }
}

StatusLineString HttpStatusLine::m_cache[2][SC_END] =
{
    {
        StatusLineString( HTTP_1_1, 0 ),
        StatusLineString( HTTP_1_1, 1 ),
        StatusLineString( HTTP_1_1, 2 ),
        StatusLineString( HTTP_1_1, 3 ),
        StatusLineString( HTTP_1_1, 4 ),
        StatusLineString( HTTP_1_1, 5 ),
        StatusLineString( HTTP_1_1, 6 ),
        StatusLineString( HTTP_1_1, 7 ),
        StatusLineString( HTTP_1_1, 8 ),
        StatusLineString( HTTP_1_1, 9 ),
        StatusLineString( HTTP_1_1, 10 ),
        StatusLineString( HTTP_1_1, 11 ),
        StatusLineString( HTTP_1_1, 12 ),
        StatusLineString( HTTP_1_1, 13 ),
        StatusLineString( HTTP_1_1, 14 ),
        StatusLineString( HTTP_1_1, 15 ),
        StatusLineString( HTTP_1_1, 16 ),
        StatusLineString( HTTP_1_1, 17 ),
        StatusLineString( HTTP_1_1, 18 ),
        StatusLineString( HTTP_1_1, 19 ),
        StatusLineString( HTTP_1_1, 20 ),
        StatusLineString( HTTP_1_1, 21 ),
        StatusLineString( HTTP_1_1, 22 ),
        StatusLineString( HTTP_1_1, 23 ),
        StatusLineString( HTTP_1_1, 24 ),
        StatusLineString( HTTP_1_1, 25 ),
        StatusLineString( HTTP_1_1, 26 ),
        StatusLineString( HTTP_1_1, 27 ),
        StatusLineString( HTTP_1_1, 28 ),
        StatusLineString( HTTP_1_1, 29 ),
        StatusLineString( HTTP_1_1, 30 ),
        StatusLineString( HTTP_1_1, 31 ),
        StatusLineString( HTTP_1_1, 32 ),
        StatusLineString( HTTP_1_1, 33 ),
        StatusLineString( HTTP_1_1, 34 ),
        StatusLineString( HTTP_1_1, 35 ),
        StatusLineString( HTTP_1_1, 36 ),
        StatusLineString( HTTP_1_1, 37 ),
        StatusLineString( HTTP_1_1, 38 ),
        StatusLineString( HTTP_1_1, 39 ),
        StatusLineString( HTTP_1_1, 40 ),
        StatusLineString( HTTP_1_1, 41 ),
        StatusLineString( HTTP_1_1, 42 ),
        StatusLineString( HTTP_1_1, 43 ),
        StatusLineString( HTTP_1_1, 44 ),
        StatusLineString( HTTP_1_1, 45 ),
        StatusLineString( HTTP_1_1, 46 ),
        StatusLineString( HTTP_1_1, 47 ),
        StatusLineString( HTTP_1_1, 48 ),
        StatusLineString( HTTP_1_1, 49 ),
        StatusLineString( HTTP_1_1, 50 ),
        StatusLineString( HTTP_1_1, 51 ),
        StatusLineString( HTTP_1_1, 52 ),
        StatusLineString( HTTP_1_1, 53 ),
        StatusLineString( HTTP_1_1, 54 ),
        StatusLineString( HTTP_1_1, 55 ),
        StatusLineString( HTTP_1_1, 56 )
    },
    {
        StatusLineString( HTTP_1_0, 0 ),
        StatusLineString( HTTP_1_0, 1 ),
        StatusLineString( HTTP_1_0, 2 ),
        StatusLineString( HTTP_1_0, 3 ),
        StatusLineString( HTTP_1_0, 4 ),
        StatusLineString( HTTP_1_0, 5 ),
        StatusLineString( HTTP_1_0, 6 ),
        StatusLineString( HTTP_1_0, 7 ),
        StatusLineString( HTTP_1_0, 8 ),
        StatusLineString( HTTP_1_0, 9 ),
        StatusLineString( HTTP_1_0, 10 ),
        StatusLineString( HTTP_1_0, 11 ),
        StatusLineString( HTTP_1_0, 12 ),
        StatusLineString( HTTP_1_0, 13 ),
        StatusLineString( HTTP_1_0, 14 ),
        StatusLineString( HTTP_1_0, 15 ),
        StatusLineString( HTTP_1_0, 16 ),
        StatusLineString( HTTP_1_0, 17 ),
        StatusLineString( HTTP_1_0, 18 ),
        StatusLineString( HTTP_1_0, 19 ),
        StatusLineString( HTTP_1_0, 20 ),
        StatusLineString( HTTP_1_0, 21 ),
        StatusLineString( HTTP_1_0, 22 ),
        StatusLineString( HTTP_1_0, 23 ),
        StatusLineString( HTTP_1_0, 24 ),
        StatusLineString( HTTP_1_0, 25 ),
        StatusLineString( HTTP_1_0, 26 ),
        StatusLineString( HTTP_1_0, 27 ),
        StatusLineString( HTTP_1_0, 28 ),
        StatusLineString( HTTP_1_0, 29 ),
        StatusLineString( HTTP_1_0, 30 ),
        StatusLineString( HTTP_1_0, 31 ),
        StatusLineString( HTTP_1_0, 32 ),
        StatusLineString( HTTP_1_0, 33 ),
        StatusLineString( HTTP_1_0, 34 ),
        StatusLineString( HTTP_1_0, 35 ),
        StatusLineString( HTTP_1_0, 36 ),
        StatusLineString( HTTP_1_0, 37 ),
        StatusLineString( HTTP_1_0, 38 ),
        StatusLineString( HTTP_1_0, 39 ),
        StatusLineString( HTTP_1_0, 40 ),
        StatusLineString( HTTP_1_0, 41 ),
        StatusLineString( HTTP_1_0, 42 ),
        StatusLineString( HTTP_1_0, 43 ),
        StatusLineString( HTTP_1_0, 44 ),
        StatusLineString( HTTP_1_0, 45 ),
        StatusLineString( HTTP_1_0, 46 ),
        StatusLineString( HTTP_1_0, 47 ),
        StatusLineString( HTTP_1_0, 48 ),
        StatusLineString( HTTP_1_0, 49 ),
        StatusLineString( HTTP_1_0, 50 ),
        StatusLineString( HTTP_1_0, 51 ),
        StatusLineString( HTTP_1_0, 52 ),
        StatusLineString( HTTP_1_0, 53 ),
        StatusLineString( HTTP_1_0, 54 ),
        StatusLineString( HTTP_1_0, 55 ),
        StatusLineString( HTTP_1_0, 56 )
    }
};


#include <http/stderrlogger.h>
StdErrLogger           HttpGlobals::s_stdErrLogger;

int             HttpGlobals::s_iProcNo = 0;
AutoStr2 *      HttpGlobals::s_psChroot = NULL;
const char *    HttpGlobals::s_pServerRoot = NULL;
const char *    HttpGlobals::s_pAdminSock = NULL;
ServerInfo *    HttpGlobals::s_pServerInfo = NULL;
SUExec *        HttpGlobals::s_pSUExec = NULL;
CgidWorker *    HttpGlobals::s_pCgid = NULL;
HttpVHost *     HttpGlobals::s_pGlobalVHost = NULL;

uid_t  HttpGlobals::s_uid = 500;
gid_t  HttpGlobals::s_gid = 500;
uid_t  HttpGlobals::s_uidMin = 100;
gid_t  HttpGlobals::s_gidMin = 100;
uid_t  HttpGlobals::s_ForceGid = 0;
int    HttpGlobals::s_priority = 3;
pid_t  HttpGlobals::s_pidCgid = -1;

int    HttpGlobals::s_umask = 022;
int    HttpGlobals::s_tmPrevToken = 0;
int    HttpGlobals::s_tmToken = 0;
int    HttpGlobals::s_dnsLookup = 1;
int    HttpGlobals::s_children = 1;
int    HttpGlobals::s_503Errors = 0;
int    HttpGlobals::s_503AutoFix = 1;
int    HttpGlobals::s_useProxyHeader = 0;

int  HttpGlobals::s_rubyProcLimit = 10;
int  HttpGlobals::s_railsAppLimit = 1;
char HttpGlobals::g_achBuf[G_BUF_SIZE+8];

#include <http/rewriteengine.h>
RewriteEngine HttpGlobals::s_RewriteEngine;

#ifdef USE_UDNS
#include <http/adns.h>
Adns   HttpGlobals::s_adns;
#endif
IpToGeo * HttpGlobals::s_pIpToGeo = NULL;


