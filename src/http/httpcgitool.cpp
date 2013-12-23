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
#include "httpcgitool.h"
#include "httpconnection.h"
#include "httpdefs.h"
#include "httpglobals.h"
#include "httpmime.h"
#include "httpreq.h"
#include "httpresp.h"
#include "httpserverversion.h"
#include "httpextconnector.h"
#include <extensions/fcgi/fcgienv.h>
#include <sslpp/sslconnection.h>
#include <sslpp/sslcert.h>

#include <util/autobuf.h>
#include <util/autostr.h>
#include <util/env.h>
#include <util/stringtool.h>

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/ssnprintf.h>

#include "iptogeo.h"
int HttpCgiTool::processHeaderLine( HttpExtConnector * pExtConn, const char * pLineBegin,
                                const char * pLineEnd, int &status )
{
    HttpRespHeaders::HEADERINDEX index;
    int tmpIndex;
    const char * pKeyEnd;
    const char * pValue = pLineBegin;
    char * p;
    HttpResp* pResp = pExtConn->getHttpConn()->getResp();
    HttpReq * pReq = pExtConn->getHttpConn()->getReq();
    
    index = HttpRespHeaders::getRespHeaderIndex( pValue );
    if ( index < HttpRespHeaders::H_HEADER_END )
    {
        pValue += HttpRespHeaders::getHeaderStringLen( index );

        while( isspace(*pValue) )
            ++pValue;
        pKeyEnd = pValue;
        if ( *pValue != ':' )
        {
            index = HttpRespHeaders::H_HEADER_END;
        }
        else
        {
            do { ++pValue; }
            while( isspace(*pValue) );
        }
    }
    if ( index == HttpRespHeaders::H_HEADER_END )
    {
        pKeyEnd = (char *)memchr( pValue, ':', pLineEnd - pValue );
        if ( pKeyEnd != NULL )
        {
            pValue = pKeyEnd + 1;
            while( isspace(*pValue) )
                ++pValue;
        }
        else
        {
            if ( !isspace( *pLineBegin ) )
                return 0;
        }
    }
    switch( index )
    {
    case HttpRespHeaders::H_CONTENT_TYPE:
        if ( pReq->getStatusCode() == SC_304 )
            return 0;
        p = (char *)memchr( pValue, ';', pLineEnd - pValue );
        if ( pReq->gzipAcceptable() )
        {
            register char ch = 0;
            register char *p1;
            if ( p )
            {
                p1 = (char*)p;
            }
            else
                p1 = (char*)pLineEnd;
            ch = *p1;
            *p1 = 0;
            if ( !HttpGlobals::getMime()->compressable( pValue ) )
                pReq->andGzip( ~GZIP_ENABLED );
            *p1 = ch;
        }
        if ( pReq->isKeepAlive() )
            pReq->smartKeepAlive( pValue );
        {
            if ( !HttpMime::needCharset( pValue ) )
                break;
            const AutoStr2 * pCharset = pReq->getDefaultCharset();
            if ( !pCharset )
                break;
            if ( p )
            {
                while( isspace( *(++p) ) )
                    ;
                if ( strncmp( p, "charset=", 8 ) == 0 )
                    break;
            }
            HttpRespHeaders& buf = pResp->getRespHeaders();
            AutoStr2 str = "";
            str.append( pLineBegin, pLineEnd - pLineBegin );
            str.append( pCharset->c_str(), pCharset->len() );
            str.append( "\r\n", 2 );
            buf.parseAdd(str.c_str(), str.len(), RespHeader::APPEND);
        }
        return 0;
    case HttpRespHeaders::H_CONTENT_ENCODING:
        pReq->andGzip( ~GZIP_ENABLED );
        break;
    case HttpRespHeaders::H_LOCATION:
        if ( (status & HEC_RESP_PROXY ) || (pReq->getStatusCode() != SC_200 ))
            break;
    case HttpRespHeaders::H_LITESPEED_LOCATION:
        if ( *pValue != '/' )
        {
            //set status code to 307
            pReq->setStatusCode( SC_302 );
        }
        else
        {
            if (( pReq->getStatusCode() == SC_404 )||
                ( index == HttpRespHeaders::H_LITESPEED_LOCATION ))
                pReq->setStatusCode( SC_200 ); 
            if ( index == HttpRespHeaders::H_LITESPEED_LOCATION )
            {
                char ch = *pLineEnd;
                *((char *)pLineEnd) = 0; 
                pReq->locationToUrl( pValue, pLineEnd - pValue );
                *((char *)pLineEnd) = ch;
            }
            else
                pReq->setLocation( pValue, pLineEnd - pValue );
            status |= HEC_RESP_LOC_SET;
            return 0;
        }
        break;
    case HttpRespHeaders::CGI_STATUS:
        tmpIndex = HttpStatusCode::codeToIndex( pValue );
        if ( tmpIndex != -1 )
        {
            pReq->updateNoRespBodyByStatus( tmpIndex );
            if (( tmpIndex >= SC_300 )&&( tmpIndex < SC_400 ))
            {
                if ( *pReq->getLocation() )
                {
                    pResp->appendHeader( "Location: ", 10,
                        pReq->getLocation(), pReq->getLocationLen() );
                    pReq->clearLocation();    
                }
            }
            if (( status & HEC_RESP_AUTHORIZER )&&( tmpIndex == SC_200))
                status |= HEC_RESP_AUTHORIZED;
        }
        return 0;
    case HttpRespHeaders::H_TRANSFER_ENCODING:
        pResp->setContentLen( -1 );
        return 0;
    case HttpRespHeaders::H_PROXY_CONNECTION:
    case HttpRespHeaders::H_CONNECTION:
        if ( strncasecmp( pValue, "close", 5 ) == 0 )
            status |= HEC_RESP_CONN_CLOSE;
        return 0;
    case HttpRespHeaders::H_CONTENT_LENGTH:
        if ( pResp->getContentLen() >= 0 )
        {
            long lContentLen = strtol( pValue, NULL, 10 );
            if (( lContentLen >= 0 )&&( lContentLen != LONG_MAX ))
            {
                pResp->setContentLen( lContentLen );
                status |= HEC_RESP_CONT_LEN;
                pReq->orContextState( RESP_CONT_LEN_SET );
            }
        }
        //fall through
    case HttpRespHeaders::H_KEEP_ALIVE:
    case HttpRespHeaders::H_SERVER:
    case HttpRespHeaders::H_DATE:
        return 0;
    default:
        //"script-control: no-abort" is not supported
        break;
    }
    if ( status & HEC_RESP_AUTHORIZED )
    {
        if (strncasecmp( pLineBegin, "Variable-", 9 ) == 0 )
        {
            if ( pKeyEnd > pLineBegin + 9 )
                pReq->addEnv( pLineBegin + 9, pKeyEnd - pLineBegin - 9,
                            pValue, pLineEnd - pValue );
        }
        return 0;
    }
    assert( pKeyEnd );
    return pResp->appendHeader( pLineBegin, pKeyEnd - pLineBegin, pValue, pLineEnd - pValue );
}

int HttpCgiTool::parseRespHeader( HttpExtConnector * pExtConn,
                                const char * pBuf, int size, int &status )
{
    const char * pEnd = pBuf + size;
    const char * pLineEnd;
    const char * pLineBegin;
    const char * pCur = pBuf;
    const char * pValue;
    while( pCur < pEnd )
    {
        pLineBegin = pCur;
        pLineEnd = (const char *)memchr( pCur, '\n', pEnd - pCur );
        if ( pLineEnd == NULL )
        {
            break;
        }
        pCur = pLineEnd + 1;
        while(( pLineEnd > pLineBegin )&&(*(pLineEnd - 1) == '\r' ))
            --pLineEnd;
        if ( pLineEnd == pLineBegin )
        {   //empty line detected
            status |= HttpReq::HEADER_OK;
            break;
        }
        pValue = pLineBegin;
        while( (pLineEnd > pValue) &&(isspace( pLineEnd[-1] )) )
            --pLineEnd;
        if ( pValue == pLineEnd )
            continue;
        int index;
        if ( (*(pValue+4) == '/') && memcmp( pValue, "HTTP/1.", 7 ) == 0 )
        {
            index = HttpStatusCode::codeToIndex( pValue + 9 );
            if ( index != -1 )
            {
                pExtConn->getHttpConn()->getReq()->updateNoRespBodyByStatus( index );
                status |= HEC_RESP_NPH2;
                if (( status & HEC_RESP_AUTHORIZER )&&( index == SC_200))
                    status |= HEC_RESP_AUTHORIZED;
            }
            continue;
        }
        if ( processHeaderLine( pExtConn, pValue,
                                pLineEnd, status ) == -1 )
            return -1;
    }
    return pCur - pBuf;
}



static char DEFAULT_PATH[] = "/bin:/usr/bin:/usr/ucb:/usr/bsd:/usr/local/bin";
static int DEFAULT_PATHLEN = sizeof( DEFAULT_PATH ) - 1;
#define CGI_FWD_HEADERS 12
static const char *CGI_HEADERS[HttpHeader::H_TRANSFER_ENCODING ] =
{   
    "HTTP_ACCEPT", "HTTP_ACCEPT_CHARSET", "HTTP_ACCEPT_ENCODING",
    "HTTP_ACCEPT_LANGUAGE", "HTTP_AUTHORIZATION", "HTTP_CONNECTION",
    "CONTENT_TYPE", "CONTENT_LENGTH", "HTTP_COOKIE", "HTTP_COOKIE2",
    "HTTP_HOST", "HTTP_PRAGMA", "HTTP_REFERER", "HTTP_USER_AGENT",
    "HTTP_CACHE_CONTROL",
    "HTTP_IF_MODIFIED_SINCE", "HTTP_IF_MATCH",
    "HTTP_IF_NONE_MATCH",
    "HTTP_IF_RANGE",
    "HTTP_IF_UNMODIFIED_SINCE",
    "HTTP_KEEPALIVE",
    "HTTP_RANGE",
    "HTTP_X_FORWARDED_FOR",
    "HTTP_VIA",
    
    //"HTTP_TRANSFER_ENCODING"
     };
static int CGI_HEADER_LEN[HttpHeader::H_TRANSFER_ENCODING ] =
{    11, 19, 20, 20, 18, 15, 12, 14, 11, 12, 9, 11, 12, 15, 18,
     22, 13, 18, 13, 24, 14, 10, 20, 8  };


int HttpCgiTool::buildEnv( IEnv* pEnv, HttpConnection* pConn )
{
    HttpReq * pReq = pConn->getReq();
    int n;
    pEnv->add( "GATEWAY_INTERFACE",17, "CGI/1.1", 7 );
    if ( getenv( "PATH" ) == NULL )
    {
        pEnv->add( "PATH", 4, DEFAULT_PATH, DEFAULT_PATHLEN );
    }
    n = pReq->getVersion();
    pEnv->add( "SERVER_PROTOCOL", 15,
        HttpVer::getVersionString( n ),
        HttpVer::getVersionStringLen( n ));
    const char * pServerStr;
    pServerStr = HttpServerVersion::getVersion();
    n = HttpServerVersion::getVersionLen();
    pEnv->add( "SERVER_SOFTWARE", 15, pServerStr, n);
    n = pReq->getMethod();
    pEnv->add( "REQUEST_METHOD", 14, HttpMethod::get( n ),
            HttpMethod::getLen( n ));



//    //FIXME: do nslookup
//
//    tmp = dnslookup(r->cn->peer.sin_addr, r->c->dns);
//    if (tmp) {
//            ADD_ENV(pEnv, "REMOTE_HOST", tmp);
//            free(tmp);
//    }
//
//    //ADD_ENV(pEnv, "REMOTE_HOST", achTemp );

    addSpecialEnv( pEnv, pReq );
    buildCommonEnv( pEnv, pConn );
    addHttpHeaderEnv( pEnv, pReq );
    pEnv->add( 0, 0, 0, 0);
    return 0;
}


static char GISS_ENV[128];

static int GISS_ENV_LEN;
void HttpCgiTool::buildServerEnv()
{
    GISS_ENV_LEN = safe_snprintf( GISS_ENV, sizeof( GISS_ENV ) - 1,
        "\021\007GATEWAY_INTERFACECGI/1.1"
        "\017%cSERVER_SOFTWARE",
        HttpServerVersion::getVersionLen());
    memmove( &GISS_ENV[GISS_ENV_LEN], HttpServerVersion::getVersion(),
        HttpServerVersion::getVersionLen() );
    GISS_ENV_LEN += HttpServerVersion::getVersionLen();
}

int HttpCgiTool::buildFcgiEnv( FcgiEnv* pEnv, HttpConnection* pConn )
{
    static const char* SP_ENVs[] =
    {   "\017\010SERVER_PROTOCOLHTTP/1.1",
        "\017\010SERVER_PROTOCOLHTTP/1.0",
        "\017\010SERVER_PROTOCOLHTTP/0.9"
    };
    static const char * RM_ENVs[10] =
    {
        "\016\007REQUEST_METHODUNKNOWN",
        "\016\007REQUEST_METHODOPTIONS",
        "\016\003REQUEST_METHODGET",
        "\016\004REQUEST_METHODHEAD",
        "\016\004REQUEST_METHODPOST",
        "\016\003REQUEST_METHODPUT",
        "\016\006REQUEST_METHODDELETE",
        "\016\005REQUEST_METHODTRACE",
        "\016\007REQUEST_METHODCONNECT",
        "\016\004REQUEST_METHODMOVE"
        
    };

    static int RM_ENV_LEN[10] =
    {   23, 23, 19, 20, 20, 19, 22, 21, 23, 20 };
    
    
    HttpReq * pReq = pConn->getReq();
    int n;

    pEnv->add( GISS_ENV, GISS_ENV_LEN );
    n = pReq->getVersion();
    pEnv->add( SP_ENVs[n], 25 );
    n = pReq->getMethod();
    if ( n < 10 )
        pEnv->add( RM_ENVs[n], RM_ENV_LEN[n] );
    else
        pEnv->add( "REQUEST_METHOD", 016, HttpMethod::get( n ),
                                    HttpMethod::getLen( n ) );

    addSpecialEnv( pEnv, pReq );
    buildCommonEnv( pEnv, pConn );
    addHttpHeaderEnv( pEnv, pReq );
    return 0;
}

int HttpCgiTool::addSpecialEnv( IEnv * pEnv, HttpReq * pReq )
{
    const AutoStr2 * psTemp = pReq->getRealPath();
    if ( psTemp )
    {
        pEnv->add( "SCRIPT_FILENAME", 15, psTemp->c_str(), psTemp->len() );
    }
    pEnv->add( "QUERY_STRING", 12, pReq->getQueryString(),
        pReq->getQueryStringLen());
    //const char * pTemp = pReq->getOrgURI();
    const char * pTemp = pReq->getURI();
    pEnv->add( "SCRIPT_NAME", 11, pTemp, pReq->getScriptNameLen());
    return 0;
}

int HttpCgiTool::buildCommonEnv( IEnv * pEnv, HttpConnection * pConn )
{
    int count = 0;
    HttpReq * pReq = pConn->getReq();
    const char * pTemp;
    int n;
    int i;
    char buf[128];

    pTemp = pReq->getAuthUser();
    if (  *pTemp )
    {
        //FIXME: only Basic is support now
        pEnv->add( "AUTH_TYPE", 9, "Basic", 5 );
        pEnv->add( "REMOTE_USER", 11, pTemp, strlen( pTemp ) );
        count += 2;
    }
    //ADD_ENV("REMOTE_IDENT", "" )        //FIXME: not supported yet
    //extensions of CGI/1.1
    const AutoStr2 * pDocRoot = pReq->getDocRoot();
    pEnv->add( "DOCUMENT_ROOT", 13,
            pDocRoot->c_str(), pDocRoot->len()-1 );
    pEnv->add( "REMOTE_ADDR", 11, pConn->getPeerAddrString(),
            pConn->getPeerAddrStrLen() );
    
    n = safe_snprintf( buf, 10, "%hu", pConn->getRemotePort() );
    pEnv->add( "REMOTE_PORT", 11, buf, n );

    n = pConn->getServerAddrStr( buf, 128 );
    
    pEnv->add( "SERVER_ADDR", 11, buf, n );
    
    pEnv->add( "SERVER_NAME", 11, pReq->getHostStr(),  pReq->getHostStrLen() );
    const AutoStr2 &sPort = pReq->getPortStr();
    pEnv->add( "SERVER_PORT", 11, sPort.c_str(), sPort.len() );
    pEnv->add( "REQUEST_URI", 11, pReq->getOrgReqURL(), pReq->getOrgReqURLLen() );
    count += 7;
    
    n = pReq->getPathInfoLen();
    if ( n > 0)
    {
        int m;
        char achTranslated[10240];
        m =  pReq->translatePath( pReq->getPathInfo(), n,
                        achTranslated, sizeof( achTranslated ) );
        if ( m != -1 )
        {
            pEnv->add( "PATH_TRANSLATED", 15, achTranslated, m );
            ++count;
        }
        pEnv->add( "PATH_INFO", 9, pReq->getPathInfo(), n);
        ++count;
    }
    
    //add geo IP env here
    if ( pReq->isGeoIpOn() )
    {
        GeoInfo * pInfo = pConn->getClientInfo()->getGeoInfo();
        if ( pInfo )
        {
            pEnv->add( "GEOIP_ADDR", 10, pConn->getPeerAddrString(),
                    pConn->getPeerAddrStrLen() );
            count += pInfo->addGeoEnv( pEnv )+1;
        }
    }    

    n = pReq->getEnvCount();
    count += n;
    for( i = 0; i < n; ++i )
    {
        const char * pKey;
        const char * pVal;
        int keyLen;
        int valLen;
        pKey = pReq->getEnvByIndex( i, keyLen, pVal, valLen );
        if ( pKey )
            pEnv->add( pKey, keyLen, pVal, valLen );
    }
    
    if ( pConn->isSSL() )
    {
        SSLConnection * pSSL = pConn->getSSL();
        pEnv->add( "HTTPS", 5, "on",  2 );
        const char * pVersion = pSSL->getVersion();
        n = strlen( pVersion );
        pEnv->add( "SSL_VERSION", 11, pVersion, n );
        count += 2;
        SSL_SESSION * pSession = pSSL->getSession();
        if ( pSession )
        {
            int idLen = SSLConnection::getSessionIdLen( pSession );
            n = idLen * 2;
            assert( n < (int)sizeof( buf ) );
            StringTool::hexEncode(
                (char *)SSLConnection::getSessionId( pSession ),
                idLen, buf );
            pEnv->add( "SSL_SESSION_ID", 14, buf, n );
            ++count;
        }

        const SSL_CIPHER * pCipher = pSSL->getCurrentCipher();
        if ( pCipher )
        {
            const char * pName = pSSL->getCipherName();
            n = strlen( pName );
            pEnv->add( "SSL_CIPHER", 10, pName, n );
            int algkeysize;
            int keysize = SSLConnection::getCipherBits( pCipher, &algkeysize );
            n = safe_snprintf( buf, 20, "%d", keysize );
            pEnv->add( "SSL_CIPHER_USEKEYSIZE", 21, buf, n );
            n = safe_snprintf( buf, 20, "%d", algkeysize );
            pEnv->add( "SSL_CIPHER_ALGKEYSIZE", 21, buf, n );
            count += 3;
        }

        X509 * pClientCert = pSSL->getPeerCertificate();
        if ( pClientCert )
        {
            //IMPROVE: too many deep copy here.
            char achBuf[4096];
            n = SSLCert::PEMWriteCert( pClientCert, achBuf, 4096 );
            if ((n>0)&&( n <= 4096 ))
            {
                pEnv->add( "SSL_CLIENT_CERT", 15, achBuf, n );
                ++count;
            }
        }
        
    }    
    return count;
}

int HttpCgiTool::addHttpHeaderEnv( IEnv * pEnv, HttpReq * pReq )
{
    int i, n;
    const char * pTemp;
    for( i = 0; i < HttpHeader::H_TRANSFER_ENCODING; ++i )
    {
        pTemp = pReq->getHeader( i );
        if ( *pTemp )
        {
            //Note: web server does not send authorization info to cgi for
            // security reason
            //pass AUTHORIZATION header only when server does not check it.
            if (( i == HttpHeader::H_AUTHORIZATION )
                &&( *pReq->getAuthUser() ))
                continue;
            pEnv->add( CGI_HEADERS[i], CGI_HEADER_LEN[i],
                        pTemp, pReq->getHeaderLen( i ));
        }
    }

    n = pReq->getUnknownHeaderCount();
    for( i = 0; i < n; ++i )
    {
        const char * pKey;
        const char * pVal;
        int keyLen;
        int valLen;
        pKey = pReq->getUnknownHeaderByIndex( i, keyLen, pVal, valLen );
        if ( pKey )
        {
            char * p;
            const char * pKeyEnd = pKey + keyLen;
            char achHeaderName[256];
            memcpy( achHeaderName, "HTTP_", 5 );
            p = &achHeaderName[5];
            if ( keyLen > 250 )
                keyLen = 250;
            while( pKey < pKeyEnd )
            {
                char ch = *pKey++;
                if ( ch == '-' )
                    *p++ = '_';
                else
                    *p++ = toupper( ch );
            }
            keyLen += 5;
            pEnv->add( achHeaderName, keyLen, pVal, valLen );
        }
    }
    return 0;
}
//Fix me:   Xuedong copy this new function from litespeed side, since this function
//          is calling the new function "void setContentTypeHeaderInfo( int offset, int len )"  
//          in "class HttpResp", which is related to the new variable "m_iContentTypeStarts", 
//          and "m_iContentTypeLen". George may need to review this.

int HttpCgiTool::processContentType( HttpReq * pReq, HttpResp* pResp, 
                        const char * pValue, int valLen )
{
    const char * p;
    do
    {
        p = (char *)memchr( pValue, ';', valLen );
        if ( pReq->gzipAcceptable() )
        {
            register char ch = 0;
            register char *p1;
            if ( p )
            {
                p1 = (char*)p;
            }
            else
                p1 = (char*)pValue + valLen;
            ch = *p1;
            *p1 = 0;
            if ( !HttpGlobals::getMime()->compressable( pValue ) )
                pReq->andGzip( ~GZIP_ENABLED );
            *p1 = ch;
        }
        if ( pReq->isKeepAlive() )
            pReq->smartKeepAlive( pValue );
        {
            if ( !HttpMime::needCharset( pValue ) )
                break;
            const AutoStr2 * pCharset = pReq->getDefaultCharset();
            if ( !pCharset )
                break;
            if ( p )
            {
                while( isspace( *(++p) ) )
                    ;
                if ( strncmp( p, "charset=", 8 ) == 0 )
                    break;
            }
            HttpRespHeaders& buf = pResp->getRespHeaders();
            buf.add(HttpRespHeaders::H_CONTENT_TYPE, "Content-Type", 12, pValue, valLen);
            buf.appendLastVal( "Content-Type", 12, pCharset->c_str(), pCharset->len() );
        }
        return 0;
    }
    while( 0 );
    
    return pResp->appendHeader( "Content-Type", 12, pValue, valLen );
}


