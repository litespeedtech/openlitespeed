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
#include <http/httpconnection.h>

#include <extensions/cgi/lscgiddef.h>
#include <extensions/extworker.h>
#include <extensions/registry/extappregistry.h>

#include <http/chunkinputstream.h>
#include <http/chunkoutputstream.h>
#include <http/connlimitctrl.h>
#include <http/eventdispatcher.h>
#include <http/datetime.h>
#include <http/handlertype.h>
#include <http/handlerfactory.h>
#include <http/htauth.h>
#include <http/httpconnpool.h>
#include <http/httpglobals.h>
#include <http/httphandler.h>
#include <http/httplog.h>
#include <http/httpresourcemanager.h>
#include <http/httpserverconfig.h>
#include <http/httpvhost.h>
#include <http/reqhandler.h>
#include <http/rewriteengine.h>
#include <http/smartsettings.h>
#include <http/userdir.h>
#include <http/vhostmap.h>

#include <http/staticfilecache.h>
#include <http/staticfilecachedata.h>
#include <ssi/ssiengine.h>
#include <ssi/ssiruntime.h>
#include <ssi/ssiscript.h>

#include <util/accesscontrol.h>
#include <util/accessdef.h>
#include <util/vmembuf.h>

#include <errno.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <util/ssnprintf.h>


#include <util/gzipbuf.h>

HttpConnection::HttpConnection()
{
    ::memset( &m_pChunkIS, 0, (char *)(&m_iReqServed + 1) - (char *)&m_pChunkIS );
    m_logID.resizeBuf( MAX_LOGID_LEN + 1 );
    m_logID.buf()[MAX_LOGID_LEN] = 0;
    m_request.setILog( this );
}

HttpConnection::~HttpConnection()
{
}

int HttpConnection::onInitConnected()
{
    m_lReqTime = DateTime::s_curTime;
    m_iReqTimeMs = DateTime::s_curTimeMS;
    m_response.setSSL( isSSL() );
    return 0;
}


const char * HttpConnection::buildLogId()
{
    AutoStr2 & id = m_logID;
//    int n = len + 32;
//    if ( !id.resizeBuf( n ) )
//        return;
    int len ;
    char * p = id.buf();
    char * pEnd = p + MAX_LOGID_LEN;
    len = safe_snprintf( id.buf(), MAX_LOGID_LEN, "%s:%hu-",  
                getPeerAddrString(), m_iRemotePort );
    id.setLen( len );
    p += len;
    len = safe_snprintf( p, pEnd - p, "%hu", m_iReqServed );
    p += len;
    const HttpVHost * pVHost = m_request.getVHost();
    if ( pVHost )
    {
        safe_snprintf( p, pEnd - p, "#%s", pVHost->getName() );
    }
    m_iLogIdBuilt = 1;
    return id.c_str();
}

#include <netinet/in_systm.h>
#include <netinet/tcp.h>

void HttpConnection::logAccess( int cancelled )
{
    HttpVHost * pVHost = ( HttpVHost * ) m_request.getVHost();
    if ( pVHost )
    {
        pVHost->decRef();
        pVHost->getReqStats()->incReqProcessed();

        if ( pVHost->BytesLogEnabled() )
        {
            long bytes = getReq()->getTotalLen();
            bytes += getResp()->getTotalLen();
            pVHost->logBytes( bytes);
        }
        if ((( !cancelled )||( m_response.replySent() ))
             &&pVHost->enableAccessLog() )
            pVHost->logAccess( this );
        else
            m_response.needLogAccess( 0 );
    }
    else
        HttpLog::logAccess( NULL, 0, this );

}


void HttpConnection::nextRequest()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpConnection::nextRequest()!",
            getLogId() ));
    if ( isSSL() )
        getSSL()->flush();        
    if ( m_pHandler )
    {
        HttpGlobals::s_reqStats.incReqProcessed();
        cleanUpHandler();
    }
    
    //for SSI, should resume 
    if ( m_request.getSSIRuntime() )
    {
        if ( m_pSubResp )
        {
            delete m_pSubResp;
            m_pSubResp = NULL;
        }
        m_request.restorePathInfo();
        if (( m_pGzipBuf )&&( !m_pGzipBuf->isStreamStarted() ))
            m_pGzipBuf->reinit();
       SSIEngine::resumeExecute( this );
        return;
    }    
    
    
    if ( !m_request.isKeepAlive())
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] Non-KeepAlive, CLOSING!",
                getLogId() ));
        if ( !inProcess() )
            closeConnection();
        else
            setState( HC_CLOSING );
        //setState( HC_CLOSING );
        //if ( !inProcess() )
        //    continueWrite();
    }
    else
    {
        logAccess( 0 );
        ++m_iReqServed;
        m_lReqTime = DateTime::s_curTime;
        m_iReqTimeMs = DateTime::s_curTimeMS;
        switchWriteToRead();
        if ( m_iLogIdBuilt )
        {
            safe_snprintf( m_logID.buf() +
                 m_logID.len(), 10, "%hu", m_iReqServed );
        }
        m_request.reset2();
        if ( m_pRespCache )
            releaseRespCache();        
        if ( m_pGzipBuf )
            releaseGzipBuf();
        if ( m_request.pendingHeaderDataLen() )
        {
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] Pending data in header buffer, set state to READING!",
                    getLogId() ));
            setState( HC_READING );
            setActiveTime( m_lReqTime );
            //if ( !inProcess() )
                processPending( 0 );
        }
        else
        {
            setState( HC_WAITING );
            ++HttpGlobals::s_iIdleConns;
        }
    }
}

int HttpConnection::read( char * pBuf, int size )
{
    int len = m_request.pendingHeaderDataLen();
    if ( len > 0 )
    {
        if ( len > size )
            len = size;
        memmove( pBuf, m_request.getHeaderBuf().begin() +
                        m_request.getCurPos(), len );
        m_request.pendingDataProcessed( len );
        return len;
    }
    return HttpIOLink::read( pBuf, size );
}

int HttpConnection::readv( struct iovec *vector, size_t count)
{
    const struct iovec * pEnd = vector + count;
    int total = 0;
    int ret;
    while( vector < pEnd )
    {
        if ( vector->iov_len > 0 )
        {
            ret = read( (char *)vector->iov_base, vector->iov_len );
            if ( ret > 0 )
                total += ret;
            if ( ret == (int)vector->iov_len )
            {
                ++vector;
                continue;
            }
            if (total)
                return total;
            return ret;
        }
        else
            ++vector;
    }
    return total;
}


int HttpConnection::processReqBody()
{
    int ret = m_request.prepareReqBodyBuf();
    if ( ret )
        return ret;
    setState( HC_READING_BODY );
    if ( m_request.isChunked() )
    {
        setupChunkIS();
    }
    else
        m_request.processReqBodyInReqHeaderBuf();
    return readReqBody();
}

void HttpConnection::setupChunkIS()
{
    assert ( m_pChunkIS == NULL );
    {
        m_pChunkIS = HttpGlobals::getResManager()->getChunkInputStream();
        m_pChunkIS->setStream( this );
        m_pChunkIS->open();
    }
}


bool HttpConnection::endOfReqBody()
{
    if ( m_pChunkIS )
    {
        return m_pChunkIS->eos();
    }
    else
    {
        return ( m_request.getBodyRemain() <= 0 );
    }
}

int HttpConnection::readReqBody()
{
    char * pBuf;
    size_t size = 0;
    int ret = 0;
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] Read Request Body!",
            getLogId() ));

    if ( !endOfReqBody() )
    {
        do
        {
            pBuf = m_request.getBodyBuf()->getWriteBuffer( size );
            if ( !pBuf )
            {
                LOG_ERR(( getLogger(), "[%s] out of swapping space while reading request body!", getLogId() ));
                return SC_400;
            }
            ret = readReqBody( pBuf, size );
            if ( ret > 0 )
            {
                m_request.contentRead( ret );
                m_request.getBodyBuf()->writeUsed( ret );
                if ( D_ENABLED( DL_LESS ))
                    LOG_D(( getLogger(), "[%s] Finsh request body %ld/%ld bytes!",
                        getLogId(), m_request.getContentFinished(),
                            m_request.getContentLength() ));
                if ( endOfReqBody() )
                    break;
                if ( ret < (int)size )
                    return 0;
            }
            else
            {
                if ( ret == -1 )
                {
                    return SC_400;
                }
                else
                    return 0;
            }
        }while( 1 );
    }
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] Finished request body %ld bytes!",
            getLogId(), m_request.getContentFinished() ));
    if ( m_pChunkIS )
    {
        if ( m_pChunkIS->getBufSize() > 0 )
        {
            if ( m_request.pendingHeaderDataLen() > 0 )
            {
                assert( m_pChunkIS->getBufSize() <= m_request.getCurPos() );
                m_request.rewindPendingHeaderData( m_pChunkIS->getBufSize() );
            }
            else
            {
                m_request.compactHeaderBuf();
                if ( m_request.appendPendingHeaderData(
                    m_pChunkIS->getChunkLenBuf(), m_pChunkIS->getBufSize() ) )
                    return SC_500;
            }
        }
        HttpGlobals::getResManager()->recycle( m_pChunkIS );
        m_pChunkIS = NULL;
        m_request.tranEncodeToContentLen();
    }
    //suspendRead();
    ret = processURI( 0 );
    return ret;

}

int HttpConnection::readReqBody( char * pBuf, int size )
{
    int len;
    if ( m_pChunkIS )
    {
        len = m_pChunkIS->read( pBuf, size );
    }
    else
    {
        int toRead = m_request.getBodyRemain();
        if ( toRead > size )
            toRead = size ;
        len = HttpIOLink::read( pBuf, toRead );
    }
    return len;
}


int HttpConnection::readToHeaderBuf()
{
    AutoBuf & headerBuf = m_request.getHeaderBuf();
    if ( D_ENABLED( DL_LESS ))
    {
        LOG_D(( getLogger(), "[%s] readToHeaderBuf(). ",
            getLogId() ));
    }
    do
    {
        int sz, avail;
        avail = headerBuf.available();
        if ( avail > 2048 )
            avail = 2048;
        char * pBuf = headerBuf.end();
        sz = HttpIOLink::read( pBuf,
                            avail );
        if ( sz > 0 )
        {
            if ( D_ENABLED( DL_LESS ))
            {
                LOG_D(( getLogger(), "[%s] read %d bytes to header buffer",
                    getLogId(), sz ));
            }
            headerBuf.used( sz );

            int ret = m_request.processHeader();
            if ( D_ENABLED( DL_LESS ))
            {
                LOG_D(( getLogger(), "[%s] processHeader() return %d, header state: %d. ",
                    getLogId(), ret, m_request.getStatus() ));
            }
            if ( ret != 1 )
            {
                return ret;
            }
            if ( headerBuf.available() <= 50 )
            {
                register int capacity = headerBuf.capacity();
                if ( capacity < HttpServerConfig::getInstance().getMaxHeaderBufLen() )
                {
                    int newSize = capacity + SmartSettings::getHttpBufIncreaseSize();
                    if ( headerBuf.reserve( newSize ) )
                    {
                        errno = ENOMEM;
                        return SC_500;
                    }
                }
                else
                {
                    LOG_NOTICE(( getLogger(),
                            "[%s] Http request header is too big, abandon!",
                            getLogId() ));
                    //m_request.setHeaderEnd();
                    //m_request.dumpHeader();
                    return SC_400;
                }
            }
        }
        if ( sz != avail )
            return 0;
    }while( 1 );
}

void HttpConnection::processPending( int ret )
{
    if (( getState() != HC_READING )||
        ( m_request.pendingHeaderDataLen() < 2 ))
        return;
    if ( D_ENABLED( DL_LESS ))
    {
        LOG_D(( getLogger(), "[%s] %d bytes pending in request header buffer:",
            getLogId(), m_request.pendingHeaderDataLen() ));
//            ::write( 2, m_request.getHeaderBuf().begin(),
//                      m_request.pendingHeaderDataLen() );
    }
    ret = m_request.processHeader();
    if ( ret == 1 )
        if ( isSSL() )
            ret = readToHeaderBuf();
        else
            return;
    if (( !ret )&&( m_request.getStatus() == HttpReq::HEADER_OK ))
        ret = processNewReq();
    if (( ret )&&( getState() < HC_SHUTDOWN ))
    {
        httpError( ret );
    }

}


int HttpConnection::processNewReq()
{
    m_lReqTime = DateTime::s_curTime;
    m_iReqTimeMs = DateTime::s_curTimeMS;
    m_response.needLogAccess(1);
    const HttpVHost * pVHost = m_request.matchVHost();
    if ( !pVHost )
    {
        if ( D_ENABLED( DL_LESS ))
        {
            register char * pHostEnd = (char*)m_request.getHostStr() + m_request.getHostStrLen();
            register char ch = *pHostEnd;
            *pHostEnd = 0;
            LOG_D(( getLogger(), "[%s] can not find a matching VHost for [%s]",
                    m_request.getHostStr(), getLogId() ));
            *pHostEnd = ch;
        }
        return SC_404;
    }
    setLogger( pVHost->getLogger() );
    if ( m_iLogIdBuilt )
    {
        AutoStr2 &id = m_logID;
        register char * p = id.buf() + id.len() + 1;
        while( *p && *p != '#' )
            ++p;
        *p++ = '#';
        memccpy( p, pVHost->getName(), 0, id.buf() + MAX_LOGID_LEN - p );
    }

    int ret = m_request.processNewReqData(getPeerAddr());
    if ( ret )
    {
        return ret;
    }
    if ((isThrottle())&&( getClientInfo()->getAccess() != AC_TRUST ))
        getThrottleCtrl()->adjustLimits( pVHost->getThrottleLimits() );
    if ( m_request.isKeepAlive() )
    {
        if ( m_iReqServed >= pVHost->getMaxKAReqs() )
        {
            m_request.keepAlive( false );
            if ( D_ENABLED( DL_LESS ) )
                LOG_D(( getLogger(), "[%s] Reached maximum requests on keep"
                        " alive connection, keep-alive off.",
                        getLogId()));
        }
        else if ( HttpGlobals::getConnLimitCtrl()->lowOnConnection() )
        {
            HttpGlobals::getConnLimitCtrl()->setConnOverflow( 1 );
            m_request.keepAlive( false );
            if ( D_ENABLED( DL_LESS ) )
                LOG_D(( getLogger(), "[%s] Running short of connection"
                        ", keep-alive off.",
                        getLogId()));
        }
        else if ( getClientInfo()->getOverLimitTime() )
        {
            m_request.keepAlive( false );
            if ( D_ENABLED( DL_LESS ) )
                LOG_D(( getLogger(), "[%s] Connections is over the soft limit"
                        ", keep-alive off.",
                        getLogId()));
        }
    }

    m_request.setStatusCode( SC_200 );
    long l = m_request.getContentLength();
    if ( l != 0 )
    {
        if ( l > HttpServerConfig::getInstance().getMaxReqBodyLen() )
        {
            LOG_NOTICE(( getLogger(),
                "[%s] Request body size: %ld is too big!",
                 getLogId(), l ));
            return SC_413;
        }
        return processReqBody();
    }
    else
    {
        //suspendRead();
        return processURI( 0 );
    }
}




int HttpConnection::checkAuthentication( const HTAuth * pHTAuth,
                    const AuthRequired * pRequired, int resume)
{

    const char * pAuthHeader = m_request.getHeader( HttpHeader::H_AUTHORIZATION );
    if ( !*pAuthHeader )
        return SC_401;
    int authHeaderLen = m_request.getHeaderLen( HttpHeader::H_AUTHORIZATION );
    char * pAuthUser;
    if ( !resume )
    {
        pAuthUser = m_request.allocateAuthUser();
        if ( !pAuthUser )
            return SC_500;
        *pAuthUser = 0;
    }
    else
        pAuthUser = (char *)m_request.getAuthUser();
    int ret = pHTAuth->authenticate( this, pAuthHeader, authHeaderLen, pAuthUser,
                    AUTH_USER_SIZE - 1, pRequired );
    if ( ret )
    {
        if ( ret > 0 )  // if ret = -1, performing an external authentication
        {
            if ( *pAuthUser )
                LOG_INFO(( getLogger(), "[%s] Authentication failed with user: '%s'.",
                    getLogId(), pAuthUser ));
        }
        else //save matched result
        {
            if ( resume )
                ret = SC_401;
//            else
//                m_request.saveMatchedResult();
        }
    }
    return ret;


}

void HttpConnection::resumeAuthentication()
{
    int ret = processURI( 1 );
    if ( ret )
        httpError( ret );
}

int HttpConnection::checkAuthorizer( const HttpHandler * pHandler )
{
    int ret = assignHandler( pHandler );
    if ( ret )
        return ret;
    setState( HC_EXT_AUTH );
    return m_pHandler->process( this, pHandler );
}


void HttpConnection::authorized()
{
    if ( m_pHandler )
        cleanUpHandler();
    int ret = processURI( 2 );
    if ( ret )
        httpError( ret );

}



int HttpConnection::redirect( const char * pNewURL, int len, int alloc )
{
    int ret = m_request.redirect( pNewURL, len, alloc );
    if ( ret )
        return ret;
    memset( m_request.getStaticFileData(), 0, sizeof( StaticFileData ) );
    return processURI( 0 );
}


int HttpConnection::processURI( int resume )
{
    const HttpVHost * pVHost = m_request.getVHost();
    int ret;
    int proceed = 1;
    int isCached = 0;
    const HttpContext * pContext;
    if ( !resume )
    {
        //virtual host level URL rewrite

        m_request.orContextState( PROCESS_CONTEXT );
        if ( (!m_request.getContextState(SKIP_REWRITE) )
            &&pVHost->getRootContext().rewriteEnabled() & REWRITE_ON )
        {
            ret = HttpGlobals::s_RewriteEngine.
            processRuleSet( pVHost->getRootContext().getRewriteRules(), this,
                            NULL, NULL );
            if ( getState() == HC_CLOSING )
                return 0;
            
            if ( ret == -3 )  //rewrite happens
            {
                m_request.postRewriteProcess(
                    HttpGlobals::s_RewriteEngine.getResultURI(),
                                             HttpGlobals::s_RewriteEngine.getResultURILen() );
            }
            else if ( ret )
            {
                if ( D_ENABLED( DL_LESS ) )
                    LOG_D(( getLogger(), "[%s] processRuleSet() return %d",
                            getLogId(), ret));
                    proceed = 0;
                m_request.setContext( &pVHost->getRootContext() );
            }
        }
    }

    while( proceed )
    {
        ret = 0;
        if ( !resume )
        {
            if ( m_request.getContextState( PROCESS_CONTEXT ) )
            {
                ret = m_request.processContext();
                if ( ret )
                {
                   if ( D_ENABLED( DL_LESS ) )
                       LOG_D(( getLogger(), "[%s] processContext() return %d",
                               getLogId(), ret));
                    if ( ret == -2 )
                        goto DO_AUTH;
                    break;
                }
            }
            const HttpContext * pContext = m_request.getContext();
            //context level URL rewrite
            if ( pContext->rewriteEnabled() & REWRITE_ON )
            {

                //while(( pContext->rewriteEnabled() & REWRITE_INHERIT )
                //Fix me :  Xuedong Replace above line with the  line below, and this is the code from litespeed project
                //          George need to review this                 
                while(( !pContext->hasRewriteConfig() )
                      &&( pContext->getParent() )
                      &&( pContext->getParent() != &pVHost->getRootContext() ))
                {
                    pContext = pContext->getParent();
                }
                if ( pContext->getRewriteRules())
                {
                    ret = HttpGlobals::s_RewriteEngine.processRuleSet(
                            pContext->getRewriteRules(), this, pContext,
                            &pVHost->getRootContext() );
                    if ( ret == -3 )
                    {
                        if ( m_request.postRewriteProcess(
                                HttpGlobals::s_RewriteEngine.getResultURI(),
                                HttpGlobals::s_RewriteEngine.getResultURILen() ) )
                        {
                            m_request.orContextState( REDIR_CONTEXT );
                            goto DO_AUTH;
                        }
                    }
                    else if ( ret )
                    {
                        if ( ret == -2 )
                        {
                            m_request.setContext( pContext );
                            goto DO_AUTH;
                        }
                        break;
                    }
                }
            }
            ret = m_request.processContextPath();
            if ( D_ENABLED( DL_LESS ) )
               LOG_D(( getLogger(), "[%s] processContextPath() return %d",
                       getLogId(), ret));
            if ( ret == -1 )    //internal redirect
                m_request.orContextState( REDIR_CONTEXT );
            else if (( ret > 0 )&&(ret != SC_404 ))
                return ret;
        }
DO_AUTH:
        AAAData     aaa;
        int         satisfyAny;

        if ( !m_request.getContextState( CONTEXT_AUTH_CHECKED|KEEP_AUTH_INFO ) )
        {
            m_request.getAAAData( aaa, satisfyAny );
            m_request.orContextState( CONTEXT_AUTH_CHECKED );
            int satisfy = 0;
            if (( !resume )&&( aaa.m_pAccessCtrl ))
            {
                if ( !aaa.m_pAccessCtrl->hasAccess( getPeerAddr() ) )
                {
                    if ( !satisfyAny || !aaa.m_pHTAuth )
                    {
                        LOG_INFO(( getLogger(),
                                "[%s][ACL] Access to context [%s] is denied!",
                                getLogId(), m_request.getContext()->getURI() ));
                        return SC_403;
                    }
                }
                else
                    satisfy = satisfyAny;
            }
            if (( !satisfy )&&( resume != 2 ))
            {
                if ( aaa.m_pRequired && aaa.m_pHTAuth )
                {
                    int ret1;
                    ret1 = checkAuthentication( aaa.m_pHTAuth, aaa.m_pRequired, resume);
                    if ( ret1 )
                    {
                        if ( D_ENABLED( DL_LESS ) )
                            LOG_D(( getLogger(), "[%s] checkAuthentication() return %d",
                                   getLogId(), ret1));
                        if ( ret1 == -1 ) //processing authentication
                        {
                            setState(HC_EXT_AUTH );
                            return 0;
                        }
                        return ret1;
                    }
                }
                if ( aaa.m_pAuthorizer )
                    return checkAuthorizer( aaa.m_pAuthorizer );
            }
        }
        if ( ret == SC_404 )
            return ret;
        if ( m_request.getContextState( REDIR_CONTEXT ) )
        {
            m_request.clearContextState( REDIR_CONTEXT );
            resume = 0;
        }
        else
            break;
    }
    
    switch( ret )
    {
    case -2:        //proxy
    case 0:         //ok
        return handlerProcess( m_request.getHttpHandler() );
    case -1:        //processing authentication
        setState(HC_EXT_AUTH );
        return 0;
    default:        //error
        return ret;
    }
}

bool ReqHandler::notAllowed( int Method ) const
{
    return (Method > HttpMethod::HTTP_POST);
}

int HttpConnection::getParsedScript( SSIScript * &pScript )
{
    int ret;
    struct stat &st = m_request.getFileStat();
    StaticFileData * pData = m_request.getStaticFileData();    
    StaticFileCacheData * &pCache = m_request.setStaticFileCache();
    if (( !pCache )||( pCache->isDirty( st,
                    m_request.getMimeType(), m_request.getDefaultCharset(), m_request.getETagFlags() )))
    {
        ret = HttpGlobals::getStaticFileCache()->getCacheElement( &m_request, pCache );
        if ( ret )
        {
            if ( D_ENABLED( DL_LESS ) )
                LOG_D(( getLogger(), "[%s] getCacheElement() return %d",
                        getLogId(), ret));
            pData->setCache( NULL );
            return ret;
        }
    }
    pCache->setLastAccess( DateTime::s_curTime );
    pCache->incRef();
    pScript = pCache->getSSIScript();
    if ( !pScript )
    {
        SSITagConfig * pTagConfig = NULL;
        if ( m_request.getVHost() )
            pTagConfig = m_request.getVHost()->getSSITagConfig();
        pScript = new SSIScript();
        ret = pScript->parse( pTagConfig, pCache->getKey() );
        if ( ret == -1 )
        {
            delete pScript;
            pScript = NULL;
        }
        else
            pCache->setSSIScript( pScript );
    }
    return 0;
}


int HttpConnection::startServerParsed()
{
    SSIScript * pScript = NULL;
    int ret;
    ret = getParsedScript( pScript );
    if ( !pScript )
        return SC_500;

    return SSIEngine::startExecute( this, pScript );
}


int HttpConnection::handlerProcess( const HttpHandler * pHandler )
{
    if ( m_pHandler )
        cleanUpHandler();
    int type = pHandler->getHandlerType();
    if (( type >= HandlerType::HT_DYNAMIC )&&
        ( type != HandlerType::HT_PROXY ))
        if ( m_request.checkScriptPermission() == 0 )
            return SC_403;
    if ( pHandler->getHandlerType() == HandlerType::HT_SSI )
    {
        const HttpContext * pContext = m_request.getContext();
        if ( pContext && !pContext->isIncludesOn() )
        {
            LOG_INFO(( getLogger(), 
                "[%s] Server Side Include is disabled for [%s], deny access.",
                         getLogId(), pContext->getLocation() ));
            return SC_403;
        }
        return startServerParsed();
    }
    //PORT_FIXME: turn off for now
    /*
    int dyn = ( pHandler->getHandlerType() >= HandlerType::HT_DYNAMIC );
    ThrottleControl * pTC = &getClientInfo()->getThrottleCtrl();
    if (( getClientInfo()->getAccess() == AC_TRUST)||
        ( pTC->allowProcess( dyn ) ))
    {
    }
    else
    {
        if ( D_ENABLED( DL_LESS ) )
        {
            const ThrottleUnit * pTU = pTC->getThrottleUnit( dyn );
            LOG_D(( getLogger(), "[%s] %s throttling %d/%d",
                   getLogId(), (dyn)?"Dyn":"Static", pTU->getAvail(), pTU->getLimit() ));
            if ( dyn )
            {
                pTU = pTC->getThrottleUnit( 2 );
                LOG_D(( getLogger(), "[%s] dyn processor in use %d/%d",
                   getLogId(), pTU->getAvail(), pTU->getLimit() ));
            }
        }
        setState( HC_THROTTLING );
        return 0;
    }
    */
    if ( m_request.getContext() && m_request.getContext()->isRailsContext() )
        m_request.orContextState( WAIT_FULL_REQ_BODY );
    if ( m_request.getContextState( WAIT_FULL_REQ_BODY ) && !endOfReqBody() )
        return 0;
    int ret = assignHandler(m_request.getHttpHandler() );
    if ( ret )
        return ret;
    setState( HC_PROCESSING );
    //PORT_FIXME: turn off for now
    //pTC->incReqProcessed( m_pHandler->getType() == HandlerType::HT_DYNAMIC );
    
    return m_pHandler->process( this, m_request.getHttpHandler() );

    
}


int HttpConnection::assignHandler( const HttpHandler * pHandler )
{
    ReqHandler * pNewHandler;
    int handlerType;
    handlerType = pHandler->getHandlerType();
    pNewHandler = HandlerFactory::getHandler( handlerType );
    if ( pNewHandler == NULL )
    {
        LOG_ERR(( getLogger(), "[%s] Handler with type id [%d] is not implemented.",
                getLogId(),  handlerType ));
        return SC_500;
    }

//    if ( pNewHandler->notAllowed( m_request.getMethod() ) )
//    {
//        if ( D_ENABLED( DL_LESS ))
//            LOG_D(( getLogger(), "[%s] Method %s is not allowed.",
//                getLogId(),  HttpMethod::get( m_request.getMethod()) ));
//        return SC_405;
//    }

//    if ( D_ENABLED( DL_LESS ) )
//    {
//        LOG_D(( getLogger(),
//            "[%s] handler with type:%d assigned.",
//            getLogId(), handlerType ));
//    }
    if ( m_pHandler )
        cleanUpHandler();
    m_pHandler = pNewHandler;
    switch( handlerType )
    {
    case HandlerType::HT_FASTCGI:
    case HandlerType::HT_CGI:
    case HandlerType::HT_SERVLET:
    case HandlerType::HT_PROXY:
    case HandlerType::HT_LSAPI:
    case HandlerType::HT_LOADBALANCER:
    {
        if ( m_request.getStatus() != HttpReq::HEADER_OK )
            return SC_400;  //cannot use dynamic request handler to handle invalid request
/*        if ( m_request.getMethod() == HttpMethod::HTTP_POST )
            getClientInfo()->setLastCacheFlush( DateTime::s_curTime );  */      
        if ( m_request.getSSIRuntime() )
        {
            if (( m_request.getSSIRuntime()->isCGIRequired() )&&
                ( handlerType != HandlerType::HT_CGI ))
            {
                LOG_INFO(( getLogger(), "[%s] Server Side Include request a CGI script, [%s] is not a CGI script, access denied.",
                    getLogId(), m_request.getURI() ));
                return SC_403;
            }
            assert( m_pSubResp == NULL );
            m_pSubResp = new HttpResp();
        }
        else
        {
            m_response.reset();
        }       
        
        const char * pType = HandlerType::getHandlerTypeString( handlerType );
        if ( D_ENABLED( DL_LESS ) )
        {
             LOG_D(( getLogger(),
                 "[%s] run %s processor.",
                 getLogId(), pType ));
        }

        if ( m_iLogIdBuilt )
        {
            AutoStr2 &id = m_logID;
            register char * p = id.buf() + id.len() + 1;
            while( *p && *p != ':' )
                ++p;

            register int n = id.buf() + MAX_LOGID_LEN - 1 - p;
            if ( n > 0 )
            {
                *p++ = ':';
                memccpy( p, pType, 0, n );
            }
        }
        if ( !HttpServerConfig::getInstance().getDynGzipCompress() )
            m_request.andGzip(~GZIP_ENABLED);
        //m_response.reset();
        break;
    }
    default:
        break;
    }
    return 0;
}




int HttpConnection::beginWrite()
{
    //int nodelay = 0;
    //::setsockopt( getfd(), IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof( int ) );
    setState( HC_WRITING );
    if (( sendBody() )||(m_response.getBodySent() < 0 ))
    {
        return doWrite();
    }
    else
    {
        //m_pHandler->abortReq();
        writeComplete();
    }
    return 0;

}

void HttpConnection::sendHttpError( const char * pAdditional )
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpConnection::sendHttpError(),code=%s",
            getLogId(),
            HttpStatusCode::getCodeString(m_request.getStatusCode()) + 1 ));
    if ( m_request.getStatusCode() == SC_503 )
    {
        ++HttpGlobals::s_503Errors;
    }
    if ( m_request.getSSIRuntime() )
    {
        if ( m_request.getStatusCode() < SC_500 )
        {
            SSIEngine::printError( this, NULL );
        }
        continueWrite();
        setState( HC_COMPLETE );
        return;
    }    
    if ( (m_request.getStatus() != HttpReq::HEADER_OK )
        || HttpStatusCode::fatalError( m_request.getStatusCode() )
        || m_request.getBodyRemain() > 0  )
    {
        m_request.keepAlive( false );
        setPeerShutdown( IO_HTTP_ERR );
    }
    // Let HEAD request follow the errordoc URL, the status code could be changed
    //if ( sendBody() )
    {
        const HttpContext * pContext = m_request.getContext();
        if ( !pContext )
        {
            if ( m_request.getVHost() )
                pContext = &m_request.getVHost()->getRootContext();
        }
        if ( pContext )
        {
            const AutoStr2 * pErrDoc;
            int code = m_request.getStatusCode();
            pErrDoc = pContext->getErrDocUrl( code );
            if ( pErrDoc )
            {
                if ( code == SC_401 )
                    m_request.orContextState( KEEP_AUTH_INFO );
                if ( *pErrDoc->c_str() == '"' )
                    pAdditional = pErrDoc->c_str()+1;
                else
                {
                    m_request.setErrorPage();
                    if (( code >= SC_300 )&&( code <= SC_403 ))
                    {
                         if ( m_request.getContextState( REWRITE_REDIR ) )
                             m_request.orContextState( SKIP_REWRITE );
                    }

                    assert( pErrDoc->len() < 2048 );
                    int ret = redirect( pErrDoc->c_str(), pErrDoc->len(),1 );
                    if ( ret == 0 )
                        return;
                    if (( ret != m_request.getStatusCode() )&&(m_request.getStatusCode() == SC_404)
                            &&( ret != SC_503 )&&( ret > 0 ))
                    {
                        httpError( ret, NULL );
                        return;
                    }
                }
            }
        }  
/*        
        if ( pContext )
        {
            const AutoStr2 * pErrDoc;
            pErrDoc = pContext->getErrDocUrl( m_request.getStatusCode() );
            if ( pErrDoc )
            {
                if ( m_request.getStatusCode() == SC_401 )
                    m_request.orContextState( KEEP_AUTH_INFO );
                if ( redirect( pErrDoc->c_str(), pErrDoc->len() ) == 0 )
                    return;
            }
        }
        */
    }
    setState( HC_WRITING );
    buildErrorResponse( pAdditional );
    writeComplete();
    HttpGlobals::s_reqStats.incReqProcessed();
}

int HttpConnection::buildErrorResponse( const char * errMsg )
{
    register int errCode = m_request.getStatusCode();
    if ( m_request.getSSIRuntime() )
    {
        if (( errCode >= SC_300 )&&( errCode < SC_400 ))
        {
            SSIEngine::appendLocation( this, m_request.getLocation(),
                 m_request.getLocationLen() );
            return 0;
        }
        if ( errMsg == NULL )
            errMsg = HttpStatusCode::getRealHtml( errCode );
        if ( errMsg )
        {
            appendDynBody( 0, errMsg, strlen(errMsg) );
        }
        return 0;
    }  
    
    m_response.reset();
    m_response.prepareHeaders( &m_request );
    //register int errCode = m_request.getStatusCode();
    register unsigned int ver = m_request.getVersion();
    if (( errCode < 0 )||( errCode >= SC_END ))
    {
        LOG_ERR(( "[%s] invalid HTTP status code: %d!", errCode ));
        errCode = SC_500;
    }
    if ( ver > HTTP_1_0 )
    {
        LOG_ERR(( "[%s] invalid HTTP version: %d!", ver ));
        ver = HTTP_1_1;
    }
    if ( sendBody() )
    {
        const char * pHtml = HttpStatusCode::getHtml( errCode );
        if ( pHtml )
        {
            int len = HttpStatusCode::getBodyLen( errCode );
            m_response.setContentLen( len );
            m_response.safeAppend( pHtml, HttpStatusCode::getTotalLen( errCode ));
            m_response.finalizeHeader( ver, errCode);
            m_response.written( len );
            return 0;
        }
    }
    m_response.endHeader();
    m_response.finalizeHeader( ver, errCode);
    return 0;
}



int HttpConnection::onReadEx()
{
    //printf( "^^^HttpConnection::onRead()!\n" );
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpConnection::onReadEx(), state: %d!",
            getLogId(), getState() ));

    int ret = 0;
    switch(  getState() )
    {
    case HC_WAITING:
        setActiveTime( DateTime::s_curTime );
        --HttpGlobals::s_iIdleConns;
        setState( HC_READING );
        //fall through;
    case HC_READING:
        ret = readToHeaderBuf();
        if ( D_ENABLED( DL_LESS ))
        {
            LOG_D(( getLogger(), "[%s] readToHeaderBuf() return %d. ",
                getLogId(), ret ));
        }
        if ((!ret)&&( m_request.getStatus() == HttpReq::HEADER_OK ))
        {
            ret = processNewReq();
            if ( D_ENABLED( DL_LESS ))
            {
                LOG_D(( getLogger(), "[%s] processNewReq() return %d. ",
                    getLogId(), ret ));
            }

        }
        if ( !ret )
            return 0;
        break;
    case HC_READING_BODY:
        ret = readReqBody();
        break;
    default:
        suspendRead();
        return 0;
    }
    if (( ret )&&( getState() < HC_SHUTDOWN ))
    {
        httpError( ret );
    }

    //processPending( ret );
//    printf( "onRead loops %d times\n", iCount );
    return 0;
}

int HttpConnection::flush()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpConnection::flush()!",
            getLogId() ));
    //int nodelay = 1;
    //::setsockopt( getfd(), IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof( int ) );
    if ( m_pChunkOS )
    {
        if ( !m_request.getSSIRuntime() )
        {
            if( m_pChunkOS->flush() == 0 )
            {
                releaseChunkOS();
            }
            else
                return 1;
        }
    }
    else
    {
        if ( m_response.getHeaderLeft() )
        {
            IOVec & iov = m_response.getIov();
            int total = m_response.getHeaderLeft();
            int written = HttpIOLink::writev( iov, total );
            if ( written >= total )
            {
                m_response.setHeaderLeft( 0 );
            }
            else
            {
                if ( written > 0 )
                {
                    m_response.setHeaderLeft( m_response.getHeaderLeft() - written );
                    iov.finish( written );
                }
                return 1;
            }
        }
    }
    if ( isSSL() )
    {
        return flushSSL();
    }
    return 0;
}

void HttpConnection::writeComplete()
{
    if ( !m_request.getSSIRuntime() )
    {
        if ( m_pChunkOS )
        {
            m_pChunkOS->close( m_response.getIov(),
                m_response.getHeaderLeft() );
            m_response.setHeaderLeft( 0 );
        }
    }
    if ( flush() == 0 )
    {
        if ( getState() == HC_WRITING )
        {
            nextRequest();
        }
    }
    else
    {
        continueWrite();
        setState( HC_COMPLETE );
    }
}


int HttpConnection::doWrite(int aioSent )
{
    int ret = -1;
    if ( m_pHandler )
    {
        ret = m_pHandler->onWrite( this, aioSent );
        if ( D_ENABLED( DL_MORE ))
            LOG_D(( getLogger(), "[%s] m_pHandler->onWrite() return %d\n",
                    getLogId(), ret ));
        if ( ret == 0 )
        {
            writeComplete();
        }
        else if ( ret == -1 )
        {
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] Handler write error, CLOSING!",
                    getLogId() ));
            setState( HC_CLOSING );
        }
    }
    else if ( m_pRespCache )
    {
        if ( m_response.getHeaderTotal() )
        {
            ret = sendDynBody();
        }
        if ( ret == 0 )
        {
            if ( !m_request.getSSIRuntime() )
            {
                writeComplete();
            }
            else
            {
                //resume SSI processing
            }
        }
        else if ( ret == -1 )
        {
            setState( HC_CLOSING );
        }
        
    }
    return ret;
}

int HttpConnection::onWriteEx()
{
    //printf( "^^^HttpConnection::onWrite()!\n" );
    int ret = 0;
    switch( getState() )
    {
    case HC_THROTTLING:
        if ( getClientInfo()->getThrottleCtrl().allowProcess(m_pHandler->getType()) )
        {
            setState( HC_PROCESSING );
            getClientInfo()->getThrottleCtrl().incReqProcessed(m_pHandler->getType());
            ret = m_pHandler->process( this, m_request.getHttpHandler() );
            if (( ret )&&( getState() < HC_SHUTDOWN ))
            {
                httpError( ret );
            }
        }
        break;
    case HC_WRITING:
        doWrite();
        break;
    case HC_COMPLETE:
        if ( flush() == 0 )
            nextRequest();
        return 0;
    case HC_CLOSING:
        return 0;
    default:
        suspendWrite();
        if ( getState() == HC_REDIRECT )
        {
            const char * pLocation = m_request.getLocation();
            if ( *pLocation )
            {
                int ret = redirect( pLocation, m_request.getLocationLen() );
                if ( !ret )
                    break;
                if ( m_request.getSSIRuntime() )
                {
                    SSIEngine::printError( this, NULL );
                    continueWrite();
                    setState( HC_COMPLETE );
                    return 0;
                }                
                m_request.setStatusCode( ret );
            }
            if ( m_request.getStatusCode() < SC_300 )
            {
                LOG_INFO(( getLogger(), "[%s] redirect status code: %d",
                        getLogId(), m_request.getStatusCode() ));
                m_request.setStatusCode( SC_307 );
            }
            sendHttpError( NULL );
        }
        break;
    }
    //processPending( ret );
    return 0;
}

void HttpConnection::cleanUpHandler()
{
    register ReqHandler * pHandler = m_pHandler;
    m_pHandler = NULL;
    pHandler->cleanUp( this );
}

void HttpConnection::closeConnection( int cancelled )
{
    m_request.keepAlive( 0 );
    if ( m_response.shouldLogAccess() )
    {
        logAccess( cancelled );
    }
    
    if ( m_pHandler )
    {
        HttpGlobals::s_reqStats.incReqProcessed();
        cleanUpHandler();
    }
    
    if ( m_pSubResp )
    {
        delete m_pSubResp;
        m_pSubResp = NULL;
    }    
    
    m_lReqTime = DateTime::s_curTime;
    m_request.reset();
    m_request.resetHeaderBuf();
    if ( m_pChunkOS )
    {
        m_pChunkOS->reset();
        releaseChunkOS();
    }
    if ( m_pRespCache )
        releaseRespCache();    
    
    if ( m_pGzipBuf )
        releaseGzipBuf();

    if ( m_pChunkIS )
    {
        HttpGlobals::getResManager()->recycle( m_pChunkIS );
        m_pChunkIS = NULL;
    }
            
    close();
}

void HttpConnection::recycle()
{
    if ( m_response.shouldLogAccess() )
    {
        LOG_WARN(( "[%s] Request has not been logged into access log, "
                  "response body sent: %ld!",
                    getLogId(), m_response.getBodySent() ));
        
    }
    HttpConnPool::recycle( this );
}
void HttpConnection::setupChunkOS(int nobuffer)
{
    m_response.setContentLen( -1 );
    if (( m_request.getVersion() == HTTP_1_1 )&&( nobuffer != 2 ))
    {
        m_response.appendChunked();
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] use CHUNKED encoding!",
                    getLogId() ));
            if ( m_pChunkOS )
                releaseChunkOS();
            m_pChunkOS = HttpGlobals::getResManager()->getChunkOutputStream();
        m_pChunkOS->setStream( this, &m_response.getIov() );
        m_pChunkOS->open();
        if ( !nobuffer )
            m_pChunkOS->setBuffering( 1 );
    }
    else
        m_request.keepAlive( false );
}

void HttpConnection::releaseChunkOS()
{
    HttpGlobals::getResManager()->recycle( m_pChunkOS );
    m_pChunkOS = NULL;
}

void HttpConnection::setBuffering( int s )
{
    if ( m_pChunkOS )
        m_pChunkOS->setBuffering( s );
}

#if !defined( NO_SENDFILE )
int HttpConnection::writeRespBodySendFile( int fdFile, off_t offset, size_t size )
{
    int& headerTotal = m_response.getHeaderLeft();
    int written = sendfile( m_response.getIov(), headerTotal,
                            fdFile, offset, size );
    if ( written > 0 )
        m_response.written( written );
    return written;    
    
}
#endif  


/**
  * @return -1 error
  *          0 complete
  *          1 continue
  */


int HttpConnection::writeRespBodyPlain(
        IOVec& iov, int &headerTotal, const char * pBuf, int size )
{
    int written;
    //printf( "HttpConnection::writeRespBody()!\n" );
    iov.append( pBuf, size );
    written = HttpIOLink::writev( iov, headerTotal + size );
    if ( headerTotal > 0 )
    {
        if ( written >= headerTotal )
        {
            iov.clear();
            written -= headerTotal;
            headerTotal = 0;
        }
        else
        {
            iov.pop_back(1);
            if ( written > 0 )
            {
                headerTotal -= written;
                iov.finish( written );
                return 0;
            }
        }
    }
    else
        iov.pop_back(1);

    return written;
}


int HttpConnection::writeRespBody( const char * pBuf, int size )
{
    IOVec & iov = m_response.getIov();
    int& headerTotal = m_response.getHeaderLeft();
    int written;
    if ( m_pChunkOS )
        written = m_pChunkOS->write( iov, headerTotal, pBuf, size );
    else
        written = writeRespBodyPlain( iov, headerTotal, pBuf, size );
    if ( written > 0 )
        m_response.written( written );
    return written;
}


int HttpConnection::writeRespBodyv( IOVec &vector, int total )
{
    int written;
    //printf( "HttpConnection::writeRespBody()!\n" );
    assert( m_pChunkOS == NULL );
    int& totalHeader = m_response.getHeaderLeft();
    if ( totalHeader )
    {
        IOVec & iov = m_response.getIov();
        iov.append( vector );
        total += totalHeader;
        written = HttpIOLink::writev( iov, total );
        iov.pop_back(vector.len());
        if ( written >= totalHeader )
        {
            iov.clear();
            written -= totalHeader;
            totalHeader = 0;
        }
        else
        {
            if ( written > 0 )
            {
                totalHeader -= written;
                iov.finish( written );
                return 0;
            }
        }
    }
    else
        written = HttpIOLink::writev( vector, total );
    if ( written > 0 )
        m_response.written( written );
    return written;
}


static char s_errTimeout[] =
    "HTTP/1.0 200 OK\r\n"
    "Cache-Control: private, no-cache, max-age=0\r\n"
    "Pragma: no-cache\r\n"
    "Connection: Close\r\n\r\n"
    "<html><head><title>408 Request Timeout</title></head><body>\n"
    "<h2>Request Timeout</h2>\n"
    "<p>This request takes too long to process, it is timed out by the server. "
    "If it should not be timed out, please contact administrator of this web site "
    "to increase 'Connection Timeout'.\n"
    "</p>\n"
//    "<hr />\n"
//    "Powered By LiteSpeed Web Server<br />\n"
//    "<a href='http://www.litespeedtech.com'><i>http://www.litespeedtech.com</i></a>\n"
    "</body></html>\n";


int HttpConnection::detectTimeout()
{
    register int c = 0;
    register const HttpServerConfig& config = HttpServerConfig::getInstance();
//    if ( D_ENABLED( DL_MEDIUM ) )
//        LOG_D((getLogger(),
//            "[%s] HttpConnection::detectTimeout() ",
//            getLogId() ));
    int delta = DateTime::s_curTime - m_lReqTime;
    switch(getState() )
    {
    case HC_WAITING:
        if ( delta >= config.getKeepAliveTimeout() )
        {
            if ( D_ENABLED( DL_MEDIUM ) )
                LOG_D((getLogger(),
                    "[%s] Keep-alive timeout, close!",
                    getLogId() ));
            c = 1;
        }
        else if (( delta > 2)&&(m_iReqServed != 0 ))
        {
            if ( HttpGlobals::getConnLimitCtrl()->getConnOverflow() )
            {
                if ( D_ENABLED( DL_MEDIUM ) )
                    LOG_D((getLogger(),
                        "[%s] Connections over flow, close connection!",
                        getLogId() ));
                c = 1;
                
            }
            else if ((int)getClientInfo()->getConns() >
                    (HttpGlobals::s_iConnsPerClientSoftLimit >> 1 ) )
            {
                if ( D_ENABLED( DL_MEDIUM ) )
                    LOG_D((getLogger(),
                        "[%s] Connections is over the limit, close!",
                        getLogId() ));
                c = 1;
            }
        }
        if ( c )
        {
            --HttpGlobals::s_iIdleConns;
            setPeerShutdown( IO_PEER_ERR );
            setState( HC_CLOSING );
            close();
        }
        return c;
    case HC_SHUTDOWN:
        if ( delta > 1 )
        {
            if ( D_ENABLED( DL_MEDIUM ) )
                LOG_D((getLogger(), "[%s] Shutdown time out!", getLogId() ));
            closeSocket();
        }
        return 1;
    case HC_CLOSING:
        closeConnection();
        return 1;
    case HC_READING:
    case HC_READING_BODY:
        if (!( getEvents() & POLLIN ) && allowRead())
        {
            LOG_WARN(( getLogger(), "[%s] Oops! POLLIN is turned off for this HTTP connection "
                        "while reading request, turn it on, this should never happen!", getLogId() ));
            continueRead();
        }
        //fall through
    default:

//        if ( getClientInfo()->getAccess() == AC_BLOCK )
//        {
//            LOG_INFO(( "[%s] Connection limit violation, access is denied, "
//                       "close existing connection!", getLogId() ));
//            setPeerShutdown( IO_PEER_ERR );
//            closeConnection();
//            return 1;
//        }

        if ( DateTime::s_curTime > getActiveTime() +
            config.getConnTimeout() )
        {
            if ( getfd() != getPollfd()->fd )
                LOG_ERR(( getLogger(),
                    "[%s] BUG: fd %d does not match fd %d in pollfd!",
                    getLogId(), getfd(), getPollfd()->fd ));
//            if ( D_ENABLED( DL_MEDIUM ) )
            if (( m_response.getBodySent() == 0 )|| !getEvents() )
            {
                LOG_INFO((getLogger(),
                    "[%s] Connection idle time: %ld while in state: %d watching for event: %d,"
                    "close!",
                    getLogId(), DateTime::s_curTime - getActiveTime(), getState(), getEvents() ));
                m_request.dumpHeader();
                if ( m_pHandler )
                    m_pHandler->dump();
                if ( getState() == HC_READING_BODY )
                {
                    LOG_INFO((getLogger(),
                        "[%s] Request body size: %d, received: %d.",
                        getLogId(), m_request.getContentLength(),
                        m_request.getContentFinished() ));
                }
            }
            if (( getState() == HC_PROCESSING )&&m_response.getBodySent() == 0 )
            {
                IOVec iov;
                iov.append( s_errTimeout, sizeof( s_errTimeout ) - 1 );
                HttpIOLink::writev( iov, sizeof( s_errTimeout ) - 1 );
            }
            else
                setPeerShutdown( IO_PEER_ERR );
            closeConnection();
            return 1;
        }
    }
    return 0;
}

int HttpConnection::onTimerEx( )
{
    if ( detectTimeout() )
        return 1;
    if ( m_pHandler )
        m_pHandler->onTimer();
    return 0;
}

void HttpConnection::releaseRespCache()
{
    if ( m_pRespCache )
    {
        HttpGlobals::getResManager()->recycle( m_pRespCache );
        m_pRespCache = NULL;
    }
}
int HttpConnection::sendDynBody()
{
    while( true )
    {
        size_t toWrite;
        char * pBuf = m_pRespCache->getReadBuffer( toWrite );
        
        if ( toWrite > 0 )
        {
            register int len = toWrite;
            if ( m_response.getContentLen() > 0 )
            {
                register int allowed = m_response.getContentLen() - m_lDynBodySent;
                if ( len > allowed )
                    len = allowed;
                if ( len <= 0 )
                {
                    m_pRespCache->readUsed( toWrite );
                    continue;
                }
            }
            
            int ret = writeRespBody( pBuf, len );
            if ( D_ENABLED( DL_MEDIUM ) )
                LOG_D(( getLogger(),
                    "[%s] writeRespBody() return %d\n",
                    getLogId(), ret ));
            if ( ret > 0 )
            {
                m_lDynBodySent += ret;
                if ( ret >= len )
                    ret = toWrite;
                m_pRespCache->readUsed( ret );
                if ( ret < len )
                {
                    continueWrite();
                    return 1;
                }
            }
            else if ( ret == -1 )
            {
                continueRead();
                return -1;
            }
            else
            {
                continueWrite();
                return 1;
            }
        }
        else
            break;
    }
    return 0;
}

int HttpConnection::setupRespCache()
{
    if ( !m_pRespCache )
    {
        m_pRespCache = HttpGlobals::getResManager()->getVMemBuf();
        if ( !m_pRespCache )
        {
            LOG_ERR(( getLogger(), "[%s] Failed to obtain VMemBuf, current pool size: %d,"
                            "capacity: %d.",
                        getLogId(), HttpGlobals::getResManager()->getVMemBufPoolSize(),
                        HttpGlobals::getResManager()->getVMemBufPoolCapacity() ));
            return -1;
        }
        if ( m_pRespCache->reinit(
                m_response.getContentLen() ) )
        {
            LOG_ERR(( getLogger(), "[%s] Failed to initialize VMemBuf, response body len: %ld.", getLogId(), m_response.getContentLen() ));
            return -1;
        }
        m_lDynBodySent = 0;
    }
    return 0;
}
int HttpConnection::setupGzipFilter()
{
    register char gz = m_request.gzipAcceptable();
    if ( gz == GZIP_REQUIRED )
        //&&(( m_iState & HEC_RESP_GZIP)||(m_iRespBodyLen > MIN_GZIP_SIZE )||
        //   ( len > MIN_GZIP_SIZE )))
    {
        //if ( !m_pRespCache->empty() )
        //    m_pRespCache->rewindWriteBuf();
        if (( !m_request.getContextState( RESP_CONT_LEN_SET )||
             m_response.getContentLen() > 200 ))
            if ( setupGzipBuf(GzipBuf::GZIP_DEFLATE) == -1 )
                return -1;

    }
    else if ( gz == UPSTREAM_GZIP )
    {
        //if ( !m_pRespCache->empty() )
        //    m_pRespCache->rewindWriteBuf();
        if ( setupGzipBuf(GzipBuf::GZIP_INFLATE) == -1 )
            return -1;
    }
    return 0;
}

int HttpConnection::setupGzipBuf( int type )
{
    if ( m_pRespCache )
    {
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( getLogger(), "[%s] %s response body!",
                getLogId(), (type == GzipBuf::GZIP_DEFLATE)? "GZIP" :"GUNZIP" ));
        if ( m_pGzipBuf )
        {
            if ( m_pGzipBuf->isStreamStarted() )
            {
                m_pGzipBuf->endStream();
                if ( D_ENABLED( DL_MEDIUM ) )
                    LOG_D(( getLogger(),
                        "[%s] setupGzipBuf() end GZIP stream.\n",
                        getLogId() ));
            }
            if ( m_pGzipBuf->getType() != type )
            {
                releaseGzipBuf();
            }
        }
        m_iRespBodyCacheOffset = m_pRespCache->getCurWOffset();
        if ( !m_pGzipBuf )
        {
            if (type == GzipBuf::GZIP_DEFLATE)
                m_pGzipBuf = HttpGlobals::getResManager()->getGzipBuf();
            else
                m_pGzipBuf = HttpGlobals::getResManager()->getGunzipBuf();
        }
        if ( m_pGzipBuf )
        {
            m_pGzipBuf->setCompressCache( m_pRespCache );
            if (( m_pGzipBuf->init( type,
                    HttpServerConfig::getInstance().getCompressLevel() ) == 0 )&&
                ( m_pGzipBuf->beginStream() == 0 ))
            {
                if ( D_ENABLED( DL_MEDIUM ) )
                    LOG_D(( getLogger(),
                        "[%s] setupGzipBuf() begin GZIP stream.\n",
                        getLogId() ));
                return 0;
            }
            else
            {
                LOG_ERR(( getLogger(), "[%s] out of swapping space while initialize GZIP stream!", getLogId() ));
                delete m_pGzipBuf;
                m_pGzipBuf = NULL;
            }
        }
    }
    return -1;
}




void HttpConnection::releaseGzipBuf()
{
    if ( m_pGzipBuf )
    {
        if ( m_pGzipBuf->getType() == GzipBuf::GZIP_DEFLATE )
            HttpGlobals::getResManager()->recycle( m_pGzipBuf );
        else
            HttpGlobals::getResManager()->recycleGunzip( m_pGzipBuf );
        m_pGzipBuf = NULL;
    }
}

int HttpConnection::appendDynBody( int inplace, const char * pBuf, int len )
{
    int ret = 0;
    if ( m_pGzipBuf )
    {
        ret = m_pGzipBuf->write( pBuf, len );
        //end of written response body
        if (( ret == 1 )&&(m_pGzipBuf->getType() == GzipBuf::GZIP_DEFLATE ))
            ret = 0;
    }
    else
    {
        if ( inplace )
        {
            if ( m_pRespCache )
                m_pRespCache->writeUsed( len );
        }
        else
            ret = appendRespCache( pBuf, len );
    }

    return ret;

}

int HttpConnection::appendRespCache( const char * pBuf, int len )
{
    char * pCache;
    size_t iCacheSize;
    while( len > 0 )
    {
        pCache = m_pRespCache->getWriteBuffer( iCacheSize );
        if ( pCache )
        {
            int ret = len;
            if ( iCacheSize < (size_t)ret )
            {
                ret = iCacheSize;
            }
            memmove( pCache, pBuf, ret );
            pBuf += ret;
            len -= ret;
            m_pRespCache->writeUsed( ret );
        }
        else
        {
            LOG_ERR(( getLogger(), "[%s] out of swapping space while append to response buffer!", getLogId() ));
            return -1;
        }
    }
    return 0;
}

int HttpConnection::shouldSuspendReadingResp()
{
    return m_pRespCache->getCurWBlkPos() >= 2048 * 1024; 
}

void HttpConnection::resetRespBodyBuf()
{

    {
        if ( m_pGzipBuf )
            m_pGzipBuf->resetCompressCache();
        else
        {
            m_pRespCache->rewindReadBuf();
            m_pRespCache->rewindWriteBuf();
        }
    }
}
static char achOverBodyLimitError[] =
"<p>The size of dynamic response body is over the "
"limit, response is truncated by web server. "
"The limit is set by the "
"'Max Dynamic Response Body Size' in tuning section "
" of server configuration.";
int HttpConnection::checkRespSize( int nobuffer )
{
    int ret = 0;
    int curLen = m_pRespCache->writeBufSize();
    if ((nobuffer&&(curLen > 0))||(curLen > 1460 ))
    {
        if ( curLen > HttpServerConfig::getInstance().getMaxDynRespLen() )
        {
            LOG_WARN((getLogger(), "[%s] The size of dynamic response body is"
                                    " over the limit, abort!",
                    getLogId() ));
            if ( m_pGzipBuf )
                ret = m_pGzipBuf->write( achOverBodyLimitError,
                            sizeof( achOverBodyLimitError ) - 1 );
            else
                ret = appendRespCache( achOverBodyLimitError,
                            sizeof( achOverBodyLimitError ) - 1 );
            if ( m_pHandler )
                m_pHandler->abortReq();
            ret = 1;
            return ret;
        }

        if ( !m_response.getHeaderTotal() )
        {
            prepareDynRespHeader(0, nobuffer );
            if ( beginWrite() == -1 )
                ret = -1;
        }
        else
        {
            setState( HC_WRITING );
            //sendResp();
            continueWrite();
        }

    }
    return ret;
}


int HttpConnection::endDynResp( int cacheable )
{
    int ret = 0;
     if ( !m_request.getSSIRuntime() )
     {
        if ( m_pGzipBuf )
        {
            if ( m_pGzipBuf->endStream() )
            {
                LOG_ERR(( getLogger(), "[%s] out of swapping space while terminating GZIP stream!", getLogId() ));
                ret = -1;
            }
            else
            {
                if ( D_ENABLED( DL_MEDIUM ) )
                LOG_D(( getLogger(),
                    "[%s] endDynResp() end GZIP stream.\n",
                    getLogId() ));
            }
        }
        if ( !m_response.getHeaderTotal() )
        {
            prepareDynRespHeader(1, 1);
            if ( beginWrite() == -1 )
                ret = -1;
        }
        else
        {
            //sendResp();
            setState( HC_WRITING );
            continueWrite();
        }
    }
    else
    {
        nextRequest();
    }
    return ret;
}

int HttpConnection::prepareDynRespHeader( int complete, int nobuffer )
{
    if ( m_response.getHeaderTotal() > 0 )
        return 0;
    m_response.getIov().clear();
    if ( sendBody() )
    {
        if ( getGzipBuf() && (getGzipBuf()->getType() == GzipBuf::GZIP_DEFLATE) )
            m_response.addGzipEncodingHeader();
        if (( complete )&&(( getRespCache()->writeBufSize() > 0 )||
              m_response.getContentLen() == 0))
        {
            m_response.setContentLen( getRespCache()->writeBufSize() );
            m_response.appendContentLenHeader();
        }
        else if ((( m_response.getContentLen() <= 200 )
                 &&( m_request.getContextState( RESP_CONT_LEN_SET ) ))
                ||(( m_response.getContentLen() > 0 )&&( !getGzipBuf())))
        {
            m_response.appendContentLenHeader();
        }
        else
        {
            setupChunkOS( nobuffer );
        }
    }
    m_response.prepareHeaders( &m_request );
    m_response.endHeader();
    m_response.finalizeHeader( m_request.getVersion(), m_request.getStatusCode());
    return 0;
}

int HttpConnection::setupDynRespBodyBuf( int &iRespState )
{
    if (( sendBody() )&& !( iRespState & HEC_RESP_AUTHORIZED ))
    {
        if ( (iRespState &( HEC_RESP_NPH | HEC_RESP_NPH2)) ==
                    ( HEC_RESP_NPH | HEC_RESP_NPH2) )
        {
            iRespState |= HEC_RESP_NOBUFFER;
        }
        if ( setupRespCache() == -1 )
            return -1;

        if ( setupGzipFilter() == -1 )
            return -1;
    }
    else
    {
        //abortReq();
        return 1;
    }
    return 0;
}

int HttpConnection::flushDynBody( int nobuff )
{
    if ( m_pGzipBuf && m_pGzipBuf->isStreamStarted() )
    {
        if (!nobuff && !m_pGzipBuf->shouldFlush() )
            return 0;
        m_pGzipBuf->flush();
    }
    return checkRespSize(nobuff);
}
int HttpConnection::flushDynBodyChunk()
{
    if ( m_pGzipBuf && m_pGzipBuf->isStreamStarted() )
    {
        m_pGzipBuf->endStream();
        if ( D_ENABLED( DL_MEDIUM ) )
           LOG_D(( getLogger(),
                  "[%s] flushDynBodyChunk() end GZIP stream.\n",
                 getLogId() ));
    }
    return 0;
    
}
int HttpConnection::execExtCmd( const char * pCmd, int len )
{

    ReqHandler * pNewHandler;
    ExtWorker * pWorker = ExtAppRegistry::getApp(
                    EA_CGID, LSCGID_NAME );
    if ( m_pHandler )
        cleanUpHandler();
    pNewHandler = HandlerFactory::getHandler( HandlerType::HT_CGI );
    m_request.setRealPath( pCmd, len );
    m_request.orContextState( EXEC_EXT_CMD );
    m_pHandler = pNewHandler;
    return m_pHandler->process( this, pWorker );

}

//Fix me: This function is related the new variable "DateTime::s_curTimeMS",
//          Not sure if we need it. George may need to review this.
//
// int HttpConnection::writeConnStatus( char * pBuf, int bufLen )
// {
//     static const char * s_pState[] = 
//     {   "KA",   //HC_WAITING,
//         "RE",   //HC_READING,
//         "RB",   //HC_READING_BODY,
//         "EA",   //HC_EXT_AUTH,
//         "TH",   //HC_THROTTLING,
//         "PR",   //HC_PROCESSING,
//         "RD",   //HC_REDIRECT,
//         "WR",   //HC_WRITING,
//         "AP",   //HC_AIO_PENDING,
//         "AC",   //HC_AIO_COMPLETE,
//         "CP",   //HC_COMPLETE,
//         "SD",   //HC_SHUTDOWN,
//         "CL"    //HC_CLOSING
//     };
//     int reqTime= (DateTime::s_curTime - m_lReqTime) * 10 + (DateTime::s_curTimeMS - m_iReqTimeMs)/100000;
//     int n = snprintf( pBuf, bufLen, "%s\t%hd\t%s\t%d.%d\t%d\t%d\t",
//         getPeerAddrString(), m_iReqServed, s_pState[getState()],
//         reqTime / 10, reqTime %10 ,
//         m_request.getHttpHeaderLen() + (unsigned int)m_request.getContentFinished(),
//         m_request.getHttpHeaderLen() + (unsigned int)m_request.getContentLength() );
//     char * p = pBuf + n;
//     p += StringTool::str_off_t( p, 20, m_response.getBodySent() );
//     *p++ = '\t';
//     p += StringTool::str_off_t( p, 20, m_response.getContentLen() );
//     *p++ = '\t';
// 
//     const char * pVHost = "";
//     int nameLen = 0;
//     if ( m_request.getVHost() )
//         pVHost = m_request.getVHost()->getVhName( nameLen );
//     memmove( p, pVHost, nameLen );
//     p += nameLen;
//     
//     *p++ = '\t';
// /*
//     const HttpHandler * pHandler = m_request.getHttpHandler();
//     if ( pHandler )
//     {
//         int ll = strlen( pHandler->getName() );
//         memmove( p, pHandler->getName(), ll );
//         p+= ll;
//     }
// */
//     if ( m_pHandler )
//     {
//         n = m_pHandler->writeConnStatus( p, pBuf + bufLen - p );
//         p += n;
//     }
//     else
//         *p++ = '-';
//     *p++ = '\t';
//     *p++ = '"';
//     if ( m_request.isRequestLineDone() )
//     {
//         const char * q = m_request.getOrgReqLine();
//         int ll = m_request.getOrgReqLineLen();
//         if ( ll > MAX_REQ_LINE_DUMP )
//         {
//             int l = MAX_REQ_LINE_DUMP / 2 - 2;
//             memmove( p, q, l );
//             p += l;
//             *p++ = '.';
//             *p++ = '.';
//             *p++ = '.';
//             q = q + m_request.getOrgReqLineLen() - l;
//             memmove( p, q, l );
//         }
//         else
//         {
//             memmove( p, q, ll );
//             p += ll;
//         }
//     }
//     *p++ = '"';
//     *p++ = '\n';
//     return p - pBuf;
// 
// 
// }

