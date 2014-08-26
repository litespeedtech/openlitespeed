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
#include "accesslog.h"

#include <extensions/fcgi/fcgiapp.h>
#include <extensions/fcgi/fcgiappconfig.h>
#include <extensions/fcgi/fcgistarter.h>
#include <extensions/registry/extappregistry.h>

#include <util/datetime.h>
#include <http/httpsession.h>
#include <http/httpreq.h>
#include <http/httpresp.h>
#include <http/pipeappender.h>
#include <http/requestvars.h>

#include <log4cxx/appender.h>
#include <log4cxx/appendermanager.h>

#include <util/stringtool.h>
#include <util/ssnprintf.h>


#include <stdio.h>
#include <string.h>

struct LogFormatItem
{
    int         m_itemId;
    AutoStr2    m_sExtra;
};

class CustomFormat : public TPointerList<LogFormatItem>
{
public:
    CustomFormat()  {}
    ~CustomFormat() {   release_objects();    }
    
    int parseFormat( const char * psFormat );
};

int CustomFormat::parseFormat( const char * psFormat )
{
    char achBuf[4096];
    
    char *pEnd;
    char *p = achBuf;
    char * pBegin = achBuf;
    char * pItemEnd = NULL;
    int state = 0;
    int itemId;
    
    memccpy( achBuf, psFormat, 0, 4095 );
    pEnd = &achBuf[strlen( achBuf )];
    
    while( 1 )
    {
        if ( state == 0 )
        {
            if (( *p == '%' )||( !*p ))
            {
                state = 1;
                if ( pBegin && (p != pBegin ))
                {
                    LogFormatItem * pItem = new LogFormatItem();
                    if ( pItem )
                    {
                        pItem->m_itemId = REF_STRING;
                        pItem->m_sExtra.setStr( pBegin, p - pBegin );
                        push_back( pItem );
                    }
                    else
                        return -1;
                }
                if ( !*p )
                    break;
            }
            else if ( *p == '\\' )
            {
                switch( *(p+1) )
                {
                    case 'n':
                        *(p+1) = '\n';
                        break;
                    case 'r':
                        *(p+1) = '\r';
                        break;
                    case 't':
                        *(p+1) = '\t';
                        break;
                }
                memmove( p, p+1, pEnd - p );
                --pEnd;
                if ( !pBegin )
                    pBegin = p;
            }
        }
        else
        {
            if ( !*p )
                break;
            pBegin = NULL;
            if ( *p == '{' )
            {
                pBegin = p+1;
                p = strchr( p+1, '}' );
                if ( !p )
                {
                    return -1;
                }
                pItemEnd = p;
                ++p;
                
            }
            else if (( *p == '>' )||( *p == '<' ))
            {
                ++p;   
            }
            else if ( *p == '%' )
            {
                *(p-1) = '\\';
                        p -= 1;
                        state = 0;
                        continue;
            }
            itemId = -1;
            switch( *p )
            {
                case 'a':
                    itemId = REF_REMOTE_ADDR;
                    break;
                case 'A':
                    itemId = REF_SERVER_ADDR;
                    break;
                case 'B':
                    itemId = REF_RESP_BYTES;
                    break;            
                case 'b':
                    itemId = REF_RESP_BYTES;
                    break;
                case 'C':
                    if ( !pBegin )
                        break;
                    itemId = REF_COOKIE_VAL;
                    break;
                case 'D':
                    itemId = REF_REQ_TIME_MS;
                    break;
                case 'e':
                    if ( !pBegin )
                        break;
                    itemId = REF_ENV;
                    break;
                case 'f':
                    itemId = REF_SCRIPTFILENAME;
                    break;                    
                case 'h':
                    itemId = REF_REMOTE_HOST;
                    break;
                case 'H':
                    itemId = REF_SERVER_PROTO;
                    break;
                case 'i':
                    if ( !pBegin )
                        break;
                    *pItemEnd = 0;
                    itemId = HttpHeader::getIndex( pBegin );
                    if (( pItemEnd - pBegin != HttpHeader::getHeaderStringLen( itemId ) )||
                        ( itemId >= HttpHeader::H_HEADER_END ))
                    {
                        itemId = REF_HTTP_HEADER;
                    }
                    else
                        pBegin = NULL;
                    
                    break;
                case 'l':
                    itemId = REF_REMOTE_IDENT;
                    break;
                case 'm':
                    itemId = REF_REQ_METHOD;
                    break;
                case 'n':
                case 'o':
                    if ( !pBegin )
                        break;
                    itemId = REF_DUMMY;
                    break;            
                case 'p':
                    itemId = REF_SERVER_PORT;
                    break;
                case 'P':
                    itemId = REF_PID;
                    break;
                case 'q':
                    if ( p[-1] == '<' )
                        itemId = REF_ORG_QS;
                    else
                        itemId = REF_QUERY_STRING;
                    break;
                case 'r':
                    itemId = REF_REQ_LINE;
                    break;
                case 's':
                    itemId = REF_STATUS_CODE;
                    break;
                case 't':
                    itemId = REF_STRFTIME;
                    break;
                case 'T':
                    itemId = REF_REQ_TIME_SEC;
                    break;
                case 'u':
                    itemId = REF_REMOTE_USER;
                    break;
                case 'U':
                    if ( p[-1] == '>' )
                        itemId = REF_CUR_URI;
                    else 
                        itemId = REF_ORG_REQ_URI;
                    break;
                case 'V':
                    itemId = REF_SERVER_NAME;
                    break;
                case 'v':
                    itemId = REF_VH_CNAME;
                    break;
                case 'X':
                    itemId = REF_CONN_STATE;
                    break;
                case 'I':
                    itemId = REF_BYTES_IN;
                    break;
                case 'O':
                    itemId = REF_BYTES_OUT;
                    break;
                default:
                    break;
            }
            if ( itemId != -1 )
            {
                LogFormatItem * pItem = new LogFormatItem();
                if ( pItem )
                {
                    pItem->m_itemId = itemId;
                    if ( pBegin )
                        pItem->m_sExtra.setStr( pBegin, pItemEnd - pBegin );
                    push_back( pItem );
                }
                else
                    return -1;
            }
            state = 0;
            pBegin = p+1;
        }
        ++p;
    }
    return 0;
}

static int logTime( char * pBuf, int len, time_t lTime, const char * pFmt )
{
    struct tm gmt;
    struct tm* pTm = gmtime_r( &lTime, &gmt );
    int n;
    n = strftime( pBuf, len, pFmt, pTm );
    return n;
    
}


int AccessLog::appendStrNoQuote( char * pBuf, int len, const char * pSrc, int srcLen, AccessLog * pLogger )
{
    if (pLogger &&((srcLen > 4096 )||( pLogger->m_buf.available() <= srcLen + 100 )))
    {
        pLogger->flush();
        pLogger->m_pAppender->append( pSrc, srcLen );
        return -1;
    }
    else
    {
        if ( srcLen > 0 )
        {
            if ( srcLen > len  )
                srcLen = len;
            memmove( pBuf, pSrc, srcLen );
            return srcLen;
        }
    }
    return 0;
}

int AccessLog::customLog( HttpSession* pSession, CustomFormat * pLogFmt, char * pOutBuf, int buf_len, AccessLog * pLogger )
{
    CustomFormat::iterator iter = pLogFmt->begin();
    HttpReq * pReq = pSession->getReq();
    LogFormatItem *pItem;
    const char * pValue = NULL;
    char * pBuf = pOutBuf;
    char * pBufEnd = pBuf + buf_len;
    char * p;
    int n;
    int ret;
    while( iter != pLogFmt->end() )
    {
        pItem = *iter;
        switch( pItem->m_itemId )
        {
        case REF_STRING:
            ret = appendStrNoQuote( pBuf, pBufEnd - pBuf, pItem->m_sExtra.c_str(), pItem->m_sExtra.len(), pLogger );
            if ( ret > 0 )
                pBuf += ret;
            else if ( ret < 0 )
                pBuf = pLogger->m_buf.end();
            break;
        case REF_STRFTIME:
            if ( pItem->m_sExtra.c_str() )
            {
                pBuf += logTime( pBuf, pBufEnd - pBuf, pSession->getReqTime(), pItem->m_sExtra.c_str() );
            }
            else
            {
                DateTime::getLogTime( pSession->getReqTime(), pBuf );
                pBuf += 28;
            }
            break;
        case REF_CONN_STATE:
            if ( pSession->getStream()->isAborted() )
            {
                *pBuf++ = 'X';
            }
            else if ( pSession->getReq()->isKeepAlive() )
            {
                *pBuf++ = '+';
            }
            else
                *pBuf++ = '-';
            break;
        case REF_COOKIE_VAL:
        case REF_ENV:
        case REF_HTTP_HEADER:
            switch( pItem->m_itemId )
            {
            case REF_COOKIE_VAL:
                pValue = RequestVars::getCookieValue( pReq, pItem->m_sExtra.c_str(), pItem->m_sExtra.len(), n );
                break;
            case REF_ENV:
                pValue = RequestVars::getEnv(pSession, pItem->m_sExtra.c_str(), pItem->m_sExtra.len(), n );
                break;
            case REF_HTTP_HEADER:
                pValue = pReq->getHeader( pItem->m_sExtra.c_str(), pItem->m_sExtra.len(), n );
                break;
            }
            if ( pValue )
            {
                ret = appendStrNoQuote( pBuf, pBufEnd - pBuf, pValue, n, pLogger );
                if ( ret > 0 )
                    pBuf += ret;
                else if ( ret < 0 )
                    pBuf = pLogger->m_buf.end();
            }
            else
                *pBuf++ = '-';
            break;
            
        default:
            p = pBuf;
            n = RequestVars::getReqVar( pSession, pItem->m_itemId, p, pBufEnd - pBuf );
            if ( n > 0 )
            {
                if ( p != pBuf )
                {
                    ret = appendStrNoQuote( pBuf, pBufEnd - pBuf, p, n, pLogger );
                    if ( ret > 0 )
                        pBuf += ret;
                    else if ( ret < 0 )
                        pBuf = pLogger->m_buf.end();
                }
                else
                    pBuf += n;
            }
            else
                *pBuf++ = '-';
            break;
                        
        }
        ++iter;
    }
    *pBuf++ = '\n';
    return pBuf - pOutBuf;
}



void AccessLog::customLog( HttpSession* pSession, CustomFormat * pLogFmt )
{
    int n = customLog( pSession, pLogFmt, m_buf.end(), m_buf.available(), this );
    m_buf.used( n );
    if (( m_buf.available() < MAX_LOG_LINE_LEN )
        ||!asyncAccessLog() )
    {
        flush();
    }
}


CustomFormat * AccessLog::parseLogFormat( const char * pFmt )
{
    CustomFormat * pLogFmt = new CustomFormat();
    pLogFmt->parseFormat( pFmt );
    return pLogFmt;
}


int AccessLog::setCustomLog( const char * pFmt )
{
    if ( !pFmt )
        return -1;
    if ( !m_pCustomFormat )
    {
        m_pCustomFormat = new CustomFormat();
        if ( !m_pCustomFormat )
            return -1;
    }
    else
        m_pCustomFormat->release_objects();
    return m_pCustomFormat->parseFormat( pFmt );
}

AccessLog::AccessLog()
    : m_pAppender( NULL )
    , m_pManager( NULL )
    , m_pCustomFormat( NULL )
    , m_iAsync( 1 )
    , m_iPipedLog( 0 )
    , m_iAccessLogHeader(LOG_REFERER|LOG_USERAGENT)
    , m_buf( LOG_BUF_SIZE )
{
}

AccessLog::AccessLog(const char * pPath)
    : m_pAppender( NULL )
    , m_pManager( NULL )
    , m_pCustomFormat( NULL )
    , m_iAsync( 1 )
    , m_iPipedLog( 0 )
    , m_iAccessLogHeader(LOG_REFERER|LOG_USERAGENT)
    , m_buf( LOG_BUF_SIZE )
{
    m_pAppender = LOG4CXX_NS::Appender::getAppender( pPath );
}


AccessLog::~AccessLog()
{
    flush();
    if ( m_pManager )
    {
        delete m_pManager;
        delete m_pAppender;
    }
    if ( m_pCustomFormat )
        delete m_pCustomFormat;
}


int AccessLog::init( const char * pName, int pipe )
{
    int ret = 0;
    m_iPipedLog = pipe;

    if ( pipe )
    {
        setAsyncAccessLog( 0 );
        m_pManager = new LOG4CXX_NS::AppenderManager();
        FcgiApp * pApp = (FcgiApp *)ExtAppRegistry::getApp( EA_LOGGER, pName );
        if ( !pApp )
            return -1;
        int num = pApp->getConfig().getInstances();
        int i = 0;
        m_pManager->setStrategy( LOG4CXX_NS::AppenderManager::AM_TILLFULL );
        for( ; i < num; ++i )
        {
            m_pAppender = new PipeAppender( pName );
            if ( !m_pAppender )
                break;
            m_pManager->addAppender( m_pAppender );
        }
        if ( i == 0 )
            return -1;
    }
    else
    {
        if ( m_pManager )
        {
            delete m_pManager;
            delete m_pAppender;
            m_pManager      = NULL;
            m_pAppender     = NULL;
        }
        if ( m_pAppender )
        {
            if ( strcmp( m_pAppender->getName(), pName ) != 0 )
            {
                flush();
                m_pAppender->close();
                //m_pAppender->setName( pName );
            }
            else
                return 0;
        }
        m_pAppender = LOG4CXX_NS::Appender::getAppender( pName );
        if ( !m_pAppender )
            return -1;
        ret = m_pAppender->open();
    }
    return ret;
}

const char * AccessLog::getLogPath() const
{
    if ( m_iPipedLog )
        return NULL;
    else
        return m_pAppender->getName();
}

int AccessLog::reopenExist()
{
    if (( !m_iPipedLog )&&( m_pAppender ))
        return m_pAppender->reopenExist();
    return 0;
}



void AccessLog::log( const char * pVHostName, int len, HttpSession* pSession )
{
    if ( pVHostName )
    {
        m_buf.append( '[' );
        appendStr( pVHostName, len );
        m_buf.append( ']' );
        m_buf.append( ' ' );
    }
    log( pSession );
}


void AccessLog::log( HttpSession* pSession )
{
    int  n;
    HttpReq*  pReq  = pSession->getReq();
    HttpResp* pResp = pSession->getResp();
    const char * pUser = pReq->getAuthUser();
    off_t contentWritten = pResp->getBodySent();
    char * pAddr;
    char achTemp[100];
    pSession->setAccessLogOff();
    if ( m_iPipedLog )
    {
        if ( !m_pManager )
            return;
        m_pAppender = m_pManager->getAppender();
        if ( !m_pAppender )
            return;
    }
    
    if ( m_pCustomFormat )
        return customLog( pSession, m_pCustomFormat );
 
    pAddr = achTemp;
    n = RequestVars::getReqVar( pSession, REF_REMOTE_HOST, pAddr, sizeof( achTemp )  );

    m_buf.appendNoCheck( pAddr, n );
    if ( ! *pUser )
    {
        m_buf.appendNoCheck( " - - ", 5 );
    }
    else
    {
        n = safe_snprintf( m_buf.end(), 70, " - \"%s\" ", pUser );
        m_buf.used( n );
    }

    DateTime::getLogTime( pSession->getReqTime(), m_buf.end() );
    m_buf.used( 30 );
    n = pReq->getOrgReqLineLen();
    char * pOrgReqLine = (char *)pReq->getOrgReqLine();
    if ( pReq->getVersion() == HTTP_1_0 )
        *(pOrgReqLine + n - 1) = '0';
    if (( n > 4096 )||( m_buf.available() < 100 + n ))
    {
        flush();
        m_pAppender->append( pOrgReqLine, n );
    }
    else
        m_buf.appendNoCheck(pOrgReqLine, n );
    m_buf.append( '"' );
    m_buf.appendNoCheck(
        HttpStatusCode::getCodeString( pReq->getStatusCode() ), 5 );
    if ( contentWritten == 0 )
    {
        m_buf.append( '-' );
    }
    else
    {
        n = StringTool::str_off_t( m_buf.end(), 30, contentWritten );
        m_buf.used( n );
    }
    if ( getAccessLogHeader() & LOG_REFERER )
    {
        m_buf.append( ' ' );
        appendStr( pReq->getHeader( HttpHeader::H_REFERER ),
                pReq->getHeaderLen( HttpHeader::H_REFERER ));
    }
    if ( getAccessLogHeader() & LOG_USERAGENT )
    {
        m_buf.append( ' ' );
        appendStr( pReq->getHeader( HttpHeader::H_USERAGENT),
                pReq->getHeaderLen( HttpHeader::H_USERAGENT) );
    }
    if ( getAccessLogHeader() & LOG_VHOST )
    {
        m_buf.append( ' ' );
        appendStr( pReq->getHeader( HttpHeader::H_HOST ),
                pReq->getHeaderLen( HttpHeader::H_HOST ) );
    }
    m_buf.append( '\n' );
    if (( m_buf.available() < MAX_LOG_LINE_LEN )
        ||!asyncAccessLog() )
    {
        flush();
    }
}

int AccessLog::appendStr( const char * pStr, int len)
{
    if ( *pStr )
    {
        m_buf.append( '"' );
        if ((len > 4096 )||( m_buf.capacity() <= len + 2 ))
        {
            flush();
            m_pAppender->append( pStr, len );
        }
        else
        {
            m_buf.appendNoCheck( pStr, len );
        }
        m_buf.append( '"' );
    }
    else
    {
        m_buf.appendNoCheck( "\"-\"", 3 );
    }
    return 0;
}


void AccessLog::flush()
{
    if ( m_buf.size() )
    {
        m_pAppender->append( m_buf.begin(), m_buf.size() );
        m_buf.clear();
    }
}

void AccessLog::accessLogReferer( int referer )
{
    if ( referer )
        m_iAccessLogHeader |= LOG_REFERER;
    else
        m_iAccessLogHeader &= ~LOG_REFERER;
}

void AccessLog::accessLogAgent( int agent )
{
    if ( agent )
        m_iAccessLogHeader |= LOG_USERAGENT;
    else
        m_iAccessLogHeader &= ~LOG_USERAGENT;
}

char AccessLog::getCompress() const
{   return m_pAppender->getCompress();  }

void AccessLog::closeNonPiped()
{
    if (( !m_iPipedLog )&&( m_pAppender->getfd() != -1 ))
        m_pAppender->close();
}

void AccessLog::setRollingSize( off_t size )
{
    m_pAppender->setRollingSize( size );
}



