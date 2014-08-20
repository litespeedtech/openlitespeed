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
#include "httpextconnector.h"
#include "handlerfactory.h"
#include "handlertype.h"
#include "httpcgitool.h"
#include "httpconnection.h"
#include "httpdefs.h"
#include "httpglobals.h"
#include "httphandler.h"
#include "httplog.h"
#include "httpreq.h"
#include "httpresourcemanager.h"
#include "httpserverconfig.h"
#include <extensions/httpextprocessor.h>
#include <extensions/extworker.h>
#include <extensions/extrequest.h>
#include <extensions/loadbalancer.h>
#include <extensions/registry/extappregistry.h>
#include <util/gzipbuf.h>
#include <util/objpool.h>
#include <util/vmembuf.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>


HttpExtConnector::HttpExtConnector()
    : m_pHttpConn( NULL )
    , m_pProcessor( NULL )
    , m_pWorker( NULL )
    , m_iState( HEC_BEGIN_REQUEST )
    , m_iRespState( 0 )
    , m_iReqBodySent( 0 )
    , m_iRespBodyLen( 0 )
    , m_iRespBodySent( 0 )
{
}

HttpExtConnector::~HttpExtConnector()
{
}


int HttpExtConnector::cleanUp( HttpConnection* pConn )
{
    if ( D_ENABLED( DL_MEDIUM ) )
        LOG_D(( getLogger(), "[%s] HttpExtConnector::cleanUp() ...", getLogId() ));
    //if ( !(getState() & (HEC_ABORT_REQUEST|HEC_ERROR|HEC_COMPLETE)) )
    if ( !(getState() & (HEC_COMPLETE)) )
    {
        m_iState &= ~HEC_FWD_RESP_BODY;
        abortReq();
        if ( getProcessor() )
        {
            getProcessor()->finishRecvBuf();
        }
        releaseProcessor();
    }
    resetConnector();
    HandlerFactory::recycle( this );
    return 0;
}

int HttpExtConnector::releaseProcessor()
{
    if ( D_ENABLED( DL_MEDIUM ) )
        LOG_D(( getLogger(), "[%s] release ExtProcessor!", getLogId() ));
    if ( m_pProcessor )
    {
        HttpExtProcessor * pProcessor = m_pProcessor;
        m_pProcessor = NULL;
        pProcessor->cleanUp();
    }
    else
    {
        if ( next() && getWorker() )
        {
            getWorker()->removeReq( this );
            getHttpConn()->resumeEventNotify();
        }
        
    }
    return 0;
}



void HttpExtConnector::resetConnector()
{
    memset( &m_iState, 0, (char *)(&m_iRespBodySent + 1 ) - (char *)&m_iState );
    m_respHeaderBuf.clear();
}




int HttpExtConnector::parseHeader( const char * &pBuf, int &len, int proxy )
{
    int ret;
    int empty;
    size_t bufLen;
    const char * pWBuf ;
    empty = m_respHeaderBuf.empty();
    //empty = 0;
    if ( empty )
    {
        pWBuf = pBuf;
        bufLen = len;
    }
    else
    {
        if ( m_respHeaderBuf.append( pBuf, len ) < 0 )
            return -1;
        bufLen = m_respHeaderBuf.size();
        pWBuf = m_respHeaderBuf.begin();
    }
    if (( proxy )&&(m_iRespHeaderSize == 0 ))
    {
        m_iRespState |= HEC_RESP_PROXY;
        if ( bufLen >= 7 )
        {
            if ( memcmp( pWBuf, "HTTP/1.", 7 ) != 0 )
                return -2;
        }
    }
    ret = HttpCgiTool::parseRespHeader( this,
                                        pWBuf, bufLen, m_iRespState );
    if ( ret > 0 )
    {
        m_iRespHeaderSize += ret;
        if ( !empty )
        {
            m_respHeaderBuf.pop_front( ret );
            pBuf = m_respHeaderBuf.begin();
            len = m_respHeaderBuf.size();
        }
        else
        {
            pBuf += ret ;
            len -= ret;
        }
    }
    else if ( ret < 0 )
    {
        errResponse( SC_500, NULL );
        return -1;
    }
    if ( m_iRespState & 0xff )
    {
        return respHeaderDone(len);
    }
    else
    {
        if ( m_iRespHeaderSize >
            HttpServerConfig::getInstance().getMaxDynRespHeaderLen() )
        {
            LOG_WARN(( getLogger(), "The size of dynamic response header: %d is"
            " over the limit.",  m_iRespHeaderSize ));
            //abortReq(5);
            abortReq();
            errResponse( SC_500, NULL );
            return -1;
        }
        if ( empty && len > 0 )
            ret = m_respHeaderBuf.append( pBuf, len );
    }
    
    return ret;
}





#define MIN_GZIP_SIZE 200

int HttpExtConnector::respHeaderDone(int len)
{
    if ( D_ENABLED( DL_MEDIUM ) )
        LOG_D(( getLogger(),
                "[%s] response header finished!",
                getLogId() ));
    if ( (m_iRespState & HEC_RESP_LOC_SET)&&
            *(getHttpConn()->getReq()->getLocation()) )
    {
        m_iState |= HEC_REDIRECT;
        getHttpConn()->changeHandler();
        getHttpConn()->continueWrite();
        return 1;
    }
    return m_pHttpConn->setupDynRespBodyBuf( m_iRespState );
}




int HttpExtConnector::processRespData( const char * pBuf, int len )
{

    if ( (getState() & (HEC_ABORT_REQUEST|HEC_ERROR|HEC_COMPLETE|HEC_REDIRECT)) )
        return len;
    if ( !(m_iRespState & 0xff) )
    {
        int ret = parseHeader( pBuf, len );
        if ((ret)||!( m_iRespState & 0xff ))
            return ret;
    }
    if ( len > 0 )
        return processRespBodyData( 0, pBuf, len );
    else
        return 0;
}

char * HttpExtConnector::getRespBuf( size_t& len )
{
    if (( m_pHttpConn->getGzipBuf() )||( !(m_iRespState & 0xff) )|| !m_pHttpConn->getRespCache())
    {
        len = G_BUF_SIZE;
        return HttpGlobals::g_achBuf;
    }
    else
    {
        return m_pHttpConn->getRespCache()->getWriteBuffer( len );
    }    
}

int HttpExtConnector::flushResp()
{
    if ( !m_pHttpConn->getRespCache() || !(m_iRespState & 0xff) )
        return 0;
    return m_pHttpConn->flushDynBody(m_iRespState & HEC_RESP_NOBUFFER);
}


int HttpExtConnector::processRespBodyData( int inplace, const char * pBuf, int len )
{
    if ( D_ENABLED( DL_MEDIUM ) )
        LOG_D((getLogger(), "[%s] HttpExtConnector::processRespBodyData()",
               getLogId() ));
        
        if ( m_pHttpConn->getRespCache() )
        {
            int ret = m_pHttpConn->appendDynBody( inplace, pBuf, len );
            if ( ret == -1 )
            {
                errResponse( SC_500, NULL );
            }
            else if ( m_pHttpConn->shouldSuspendReadingResp() )
            {
                m_pProcessor->suspendRead();
            }
            
            //        return checkRespSize();
            return ret;
        }
        else
            return len;
}

int HttpExtConnector::extInputReady()
{
            return 0;
            
}



void HttpExtConnector::abortReq()
{
    if (!(getState() & HEC_COMPLETE) )
    {
        if ( D_ENABLED( DL_MEDIUM ) )
            LOG_D(( getLogger(),
                "[%s] abort request... ", getLogId() ));
        m_iState |= HEC_ABORT_REQUEST;
        if ( m_pProcessor )
        {
            m_pProcessor->abort();
        }
    }
}

int HttpExtConnector::extOutputReady()
{
    int ret = 0;
    if ( getState() == HEC_BEGIN_REQUEST )
    {
        ret = m_pProcessor->begin();
        if ( ret > 0 )
            extProcessorReady();
        else
            return -1;
    }
    if (!( getState() & (HEC_FWD_REQ_HEADER | HEC_FWD_REQ_BODY) ))
    {
        if ( m_pProcessor )
            m_pProcessor->suspendWrite();
    }
    else
    {
        if (( getState() & HEC_FWD_REQ_HEADER ))
        {
            return sendReqHeader();
        }
        else if ( (getState() & HEC_FWD_REQ_BODY))
        {
            ret = sendReqBody();
        }
    }
    return ret;
}

void HttpExtConnector::extProcessorReady()
{
    setState( HEC_FWD_REQ_HEADER );
}

#include <http/stderrlogger.h>

int HttpExtConnector::processErrData( const char * pBuf, int len )
{
    assert( pBuf );
    assert( len >= 0 );
    if ( !HttpGlobals::getStdErrLogger()->isEnabled() )
        return 0;
    char * pTemp = (char *)malloc( len + 1 );
    if ( pTemp )
    {
        memmove( pTemp, pBuf, len );
        *(pTemp + len ) = 0;
        LOG_NOTICE(( getLogger(), "[%s] [STDERR] %s", getLogId(), pTemp ));
        free( pTemp );
    }
    return 0;

}

int HttpExtConnector::endResponse( int endCode, int protocolStatus )
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( getLogger(),
            "[%s] [EXT] EndResponse( endCode=%d, protocolStatus=%d )",
            getLogId(), endCode, protocolStatus ));
    int ret = 0;
    if ( m_iState & HEC_COMPLETE )
        return 0;
    if ( !( m_iRespState & HttpReq::HEADER_OK ) && !(m_iState & HEC_ABORT_REQUEST) )
    {
        m_iRespState |= HttpReq::HEADER_OK;
        LOG_NOTICE((getLogger(), "[%s] Premature end of response header.", getLogId() ));
        return errResponse( SC_500, NULL );
    }
    m_iState |= HEC_COMPLETE;
    if ( !(m_iState & ( HEC_ABORT_REQUEST | HEC_ERROR)) && !endCode && getWorker() )
        getWorker()->getReqStats()->incReqProcessed();
    releaseProcessor();
    if ( m_iRespState & HEC_RESP_AUTHORIZED )
    {
        m_pHttpConn->authorized();
        return 0;
    }
    if ( ( m_iRespState & 0xff ) &&
        !(m_iState & (HEC_ERROR|HEC_REDIRECT )) )
    {
        return m_pHttpConn->endDynResp( !(m_iState & HEC_ABORT_REQUEST) );
    }
    else
    {
        m_pHttpConn->continueWrite();
    }
    return ret;
}






int HttpExtConnector::sendResp()
{
    if ( !m_pHttpConn->getResp()->getHeaderTotal() )
    {
        getHttpConn()->suspendWrite();
        return 1;
    }
    {
        int ret = m_pHttpConn->sendDynBody();
        if ( ret == -1 )
        {
            m_iState |= HEC_ERROR;
            //abortReq(6);
            abortReq();
            return -1;
        }
        if ( ret == 1 )
            return ret;
    }
    if ( (m_iState & ( HEC_COMPLETE | HEC_ERROR )) == 0 )
    {
        if ( D_ENABLED( DL_MEDIUM ) )
            LOG_D(( getLogger(),
                    "[%s] response buffer is empty, "
                    "suspend HttpConn write!\n", getLogId() ));
            getHttpConn()->suspendWrite();
        m_pHttpConn->resetRespBodyBuf();
        m_pProcessor->continueRead();
        return 1;
    }
    else
    {
        //        int l = getHttpConn()->getResp()->getContentLen();
        //        if (( l >= 0 )&&( l != m_iRespBodySent ))
        //            getHttpConn()->getReq()->keepAlive( 0 );
        //assert( !(m_iState & HEC_ERROR) );
        //if (iState & HEC_COMPLETE )
        if ( D_ENABLED( DL_MEDIUM ) )
            LOG_D(( getLogger(),
                    "[%s] ReqBody: %d, RespBody: %d, HEC_COMPLETE!",
                    getLogId(), m_iReqBodySent, m_pHttpConn->getDynBodySent() ));
            return 0;
        
    }
}


int HttpExtConnector::onWrite(HttpConnection* pConn, int aioSent )
{
    return sendResp();
}

int HttpExtConnector::process( HttpConnection* pConn, const HttpHandler * pHandler )
{
    assert( pConn );
    //resetConnector();
    assert( pHandler );
    setHttpConn( pConn );
    setAttempts( 0 );
    if ( pHandler->getHandlerType() == HandlerType::HT_LOADBALANCER )
    {
        LoadBalancer * pLB = (LoadBalancer *)pHandler;
        setLB( pLB );
        ExtWorker * pWorker = pLB->selectWorker(pConn, this);
        setWorker( pWorker );
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( getLogger(), "[%s] Assign new request to ExtProcessor [%s]!",
                getLogId(), pWorker? pWorker->getName(): "unavailable" ));
    }
    else
    {
        setLB( NULL );
        setWorker( (ExtWorker *)pHandler );
    }
    m_iState = HEC_BEGIN_REQUEST;

    if ( getWorker() == NULL )
    {
        LOG_ERR(( getLogger(), "[%s] External processor is not available!",
                getLogId() ));
        return SC_500; //Fast cgi App Not found
    }
    int ret = m_pWorker->processRequest( this );
    if ( ret > 1 )
    {
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( getLogger(), "[%s] Can't add new request to ExtProcessor [%s]!",
                getLogId(), m_pWorker->getName() ));
    }
    else
    {
        if ( ret == 1 )
            ret = 0;
        m_iRespState |= ( m_pWorker->getConfigPointer()->getBuffering()&
                      ( HEC_RESP_NPH | HEC_RESP_NOBUFFER));
        if ( m_pWorker->getRole() == EXTAPP_AUTHORIZER )
            m_iRespState |= HEC_RESP_AUTHORIZER;
    }
    return ret;
}

int  HttpExtConnector::tryRecover()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( "[%s] HttpExtConnector::tryRecover()...",
                getLogId() ));
    if ( isRecoverable() )
    {
        int attempts = incAttempts();
        int maxAttempts;
        LoadBalancer * pLB = getLB();
        if ( pLB )
        {
            maxAttempts = pLB->getWorkerCount() * 3;
        }
        else
            maxAttempts = 3;
        resetConnector();
        if ( attempts < maxAttempts )
        {
            if ( D_ENABLED( DL_LESS ) )
                LOG_D(( "[%s] trying to recover from connection problem, attempt: #%d!",
                        getLogId(), attempts ));
            if ( pLB && ( attempts % 3 ) == 0 )
            {
                ExtWorker * pWorker = pLB->selectWorker(m_pHttpConn, this);
                if ( D_ENABLED( DL_LESS ) )
                {
                    if ( pWorker )
                    {
                        LOG_D(( "[%s] [LB] retry worker: [%s]",
                             getLogId(), pWorker->getName() ));
                    }
                    else
                    {
                        LOG_D(( "[%s] [LB] Backup worker is unavailable." ));
                    }
                }
                setWorker( pWorker );
                if ( D_ENABLED( DL_LESS ) )
                    LOG_D(( "[%s] trying to recover from connection problem, attempt: #%d!",
                            getLogId(), attempts ));
            }            
            else if ( ((attempts % 3) >= 2) && !m_pWorker->isReady() )
            {
                if ( D_ENABLED( DL_LESS ) )
                    LOG_D(( "[%s] try to restart external application [%s] at [%s]...",
                        getLogId(), m_pWorker->getName(),
                        m_pWorker->getURL() ));
                m_pWorker->start();
            }
            if ( m_pWorker )
            {
                int ret = m_pWorker->processRequest( this, 1 );
                if (( ret == 0)||( ret == 1 ))
                    return 0;
            }
        }
        //if ( m_pWorker )
        //    m_pWorker->tryRestart();
        errResponse( SC_503, NULL );
    }
    else
        endResponse( 0, 0 );
    return -1;
}


int HttpExtConnector::sendReqHeader()
{
    int ret = getProcessor()->sendReqHeader();
    if ( ret == -1 )
        return -1;
    ret = reqHeaderDone();
    return ret;
}


int HttpExtConnector::reqHeaderDone()
{
    if ( D_ENABLED( DL_MEDIUM ) )
        LOG_D(( getLogger(),
            "[%s] request header is done\n", getLogId() ));
    HttpReq * pReq = getHttpConn()->getReq();
    getProcessor()->beginReqBody();
    getProcessor()->continueRead();
    if (!(m_iRespState & HEC_RESP_AUTHORIZER)&&( pReq->getBodyBuf() ))
    {
        pReq->getBodyBuf()->rewindReadBuf();
        setState( HEC_REQ_HEADER_DONE );
        return sendReqBody();
    }
    else
    {
        setState( HEC_FWD_RESP_HEADER );
        return reqBodyDone();
    }
}

int HttpExtConnector::reqBodyDone()
{
    if ( D_ENABLED( DL_MEDIUM ) )
        LOG_D(( getLogger(), "[%s] Request body done!\n", getLogId() ));
    //getProcessor()->suspendWrite();
    return getProcessor()->endOfReqBody();
}

int HttpExtConnector::sendReqBody()
{
    HttpReq * pReq = getHttpConn()->getReq();
    VMemBuf * pVMemBuf = pReq->getBodyBuf();
    size_t size;
    char * pBuf;
    int count = 0;
    while(( (pBuf = pVMemBuf->getReadBuffer(size)) != NULL )&&( size > 0 ))
    {
        int written = getProcessor()->sendReqBody(pBuf, size );
        if ( written > 0 )
        {
            pVMemBuf->readUsed( written );
            m_iReqBodySent += written;
        }
        if ( D_ENABLED( DL_MEDIUM ) )
            LOG_D(( getLogger(),
                "[%s] processor sent request body %d bytes, total sent: %d\n",
                            getLogId(), written, m_iReqBodySent ));
        if (( written != (int)size )||( ++count == 10 ))
        {
            if ( written != -1 )
                getProcessor()->continueWrite();
            return written;
        }
    }
    setState( getState() & ~HEC_FWD_REQ_BODY );
    reqBodyDone();
    return 0;
}


int HttpExtConnector::onRead(HttpConnection* pConn )
{
//    if ( getState() & HEC_FWD_REQ_BODY )
//    {
//        writeReqBody();
//    }
//    else
    getHttpConn()->suspendRead();
    assert( false );
    return 0;
}

bool HttpExtConnector::isRecoverable()
{
    if ( m_iState & HEC_FWD_RESP_BODY )
        return false;
    //if ( m_iReqBodySent == 0 )
    //    return true;
    //HttpReq * pReq = getHttpConn()->getReq();
    return true;
}

int HttpExtConnector::errResponse( int code, const char * pErr )
{
    m_iState |= HEC_ERROR;
    if ( getHttpConn() )
    {
        if (!( m_iState & HEC_FWD_RESP_BODY ))
        {
            getHttpConn()->getReq()->setStatusCode( code );
            getHttpConn()->changeHandler();
        }
        getHttpConn()->continueWrite();
    }
    return -1;
}

void HttpExtConnector::onTimer()
{
//    if ( m_pProcessor )
//        m_pProcessor->onProcessorTimer();
}

void HttpExtConnector::suspend()
{
    getHttpConn()->suspendEventNotify();
}


void HttpExtConnector::extProcessorError( int errCode )
{
    if ( !(getState() & (HEC_ABORT_REQUEST|HEC_ERROR|HEC_COMPLETE)) )
    {
        const char * pName;
        if ( m_pWorker )
            pName = m_pWorker->getURL();
        else
            pName = "";
        LOG_ERR(( getLogger(), "[%s] [%s]: %s", getLogId(),
               pName, strerror( errCode ) ));
        errResponse( SC_503,
                     "<html><title>Not Avaiable</title>"
                     "<body><h1>Failed to communicate with external application!</h1>"
                     "</body></html>" );
    }
}

const char *  HttpExtConnector::getLogId()
{
    if ( m_pHttpConn )
        return m_pHttpConn->getLogId();
    else
        return "idle";
}

LOG4CXX_NS::Logger* HttpExtConnector::getLogger() const
{
    if ( m_pHttpConn )
        return m_pHttpConn->getLogger();
    else
        return NULL;

}

void HttpExtConnector::dump()
{
    LOG_INFO(( getLogger(), "[%s] HttpExtConnector state: %d, "
                "request body sent: %d, response body size: %d, response body sent:%d, "
                "left in buffer: %ld, attempts: %d."
        , getLogId(), m_iState, m_iReqBodySent, m_pHttpConn->getResp()->getContentLen(), m_pHttpConn->getDynBodySent(),
        (m_pHttpConn->getRespCache())?m_pHttpConn->getRespCache()->writeBufSize():0, getAttempts() ));    
    if ( m_pProcessor )
    {
        m_pProcessor->dump();
    }
    else
        LOG_INFO(( getLogger(), "[%s] External processor is not available.",
                    getLogId() ));
}

int HttpExtConnector::dumpAborted()
{
    return ( m_pProcessor != NULL);
}


int HttpExtConnector::isAlive()
{
    return getHttpConn()->isAlive();
}

void HttpExtConnector::setHttpError( int error )
{
    errResponse( error, NULL );
}




