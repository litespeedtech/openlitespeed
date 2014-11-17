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
#include <http/httpsession.h>
#include <stdio.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <socket/gsockaddr.h>

#include <extensions/cgi/lscgiddef.h>
#include <extensions/extworker.h>
#include <extensions/registry/extappregistry.h>
#include <http/chunkinputstream.h>
#include <http/chunkoutputstream.h>
#include <http/clientcache.h>
#include <http/connlimitctrl.h>
#include <util/datetime.h>
#include <http/eventdispatcher.h>
#include <http/handlerfactory.h>
#include <http/handlertype.h>
#include <http/htauth.h>
#include <http/httpglobals.h>
#include <http/httphandler.h>
#include <http/httplog.h>
#include <http/httpmime.h>
#include <http/httpresourcemanager.h>
#include <http/httpserverconfig.h>
#include <http/httpvhost.h>
#include <http/ntwkiolink.h>
#include <http/reqhandler.h>
#include <http/rewriteengine.h>
#include <http/smartsettings.h>
#include <http/staticfilecache.h>
#include <http/staticfilecachedata.h>
#include <http/userdir.h>
#include <http/vhostmap.h>
#include <lsiapi/lsiapihooks.h>
#include <lsiapi/envmanager.h>

#include <ssi/ssiengine.h>
#include <ssi/ssiruntime.h>
#include <ssi/ssiscript.h>
#include <util/accesscontrol.h>
#include <util/accessdef.h>
#include <util/gzipbuf.h>
#include <util/ssnprintf.h>
#include <util/vmembuf.h>

#include "http/l4handler.h"
#include "extensions/l4conn.h"

#include <lsiapi/internal.h>


HttpSession::HttpSession() : m_sessionHooks(0)
{
    memset( &m_pChunkIS, 0, (char *)(&m_iReqServed + 1) - (char *)&m_pChunkIS );
    m_pModuleConfig = NULL;
}

HttpSession::~HttpSession()
{
    LsiapiBridge::releaseModuleData( LSI_MODULE_DATA_HTTP, getModuleData());
}

int HttpSession::onInitConnected()
{
    m_lReqTime = DateTime::s_curTime;
    m_iReqTimeUs = DateTime::s_curTimeUs;
    if ( m_pNtwkIOLink )
    {
        setVHostMap( m_pNtwkIOLink->getVHostMap() );
    }

    
    m_iFlag = 0;
    setState( HSS_READING );
    getStream()->setFlag( HIO_FLAG_WANT_READ, 1 );
    m_request.setILog( getStream() );
    if (m_request.getBodyBuf())
        m_request.getBodyBuf()->reinit();
    m_request.reset();
    return 0;
}

inline int HttpSession::getModuleDenyCode(  int iHookLevel )
{
    int ret = m_request.getStatusCode();
    if ((!ret)||( ret == SC_200 ))
    {
        ret = SC_403;
        m_request.setStatusCode( SC_403 );
    }
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] blocked by %s hookpoint, return: %d !",
            getLogId(), LsiApiHooks::getHkptName( iHookLevel ), ret ));
    
    return ret;
}

inline int HttpSession::processHkptResult( int iHookLevel, int ret )
{
    if ((( getState() == HSS_EXT_REDIRECT )||( getState() == HSS_REDIRECT ))
        && *(m_request.getLocation()) )
    {
        //perform external redirect.
        continueWrite();
        return -1;
    }
    
    if ( ret < 0 )
    {
        getModuleDenyCode( iHookLevel );
        setState( HSS_HTTP_ERROR );
        continueWrite();
    }
    return ret;
}


/*
const char * HttpSession::buildLogId()
{
    AutoStr2 & id = getStream()->getIdBuf();
    int len ;
    char * p = id.buf();
    char * pEnd = p + MAX_LOGID_LEN;
    len = safe_snprintf( id.buf(), MAX_LOGID_LEN, "%s-",  
                getStream()->getLogId() );
    id.setLen( len );
    p += len;
    len = safe_snprintf( p, pEnd - p, "%hu", m_iReqServed );
    p += len;
    const HttpVHost * pVHost = m_request.getVHost();
    if ( pVHost )
    {
        safe_snprintf( p, pEnd - p, "#%s", pVHost->getName() );
    }
    return id.c_str();
}
*/

#include <netinet/in_systm.h>
#include <netinet/tcp.h>

void HttpSession::logAccess( int cancelled )
{
    HttpVHost * pVHost = ( HttpVHost * ) m_request.getVHost();
    if ( pVHost )
    {
        pVHost->decRef();
        pVHost->getReqStats()->incReqProcessed();

        if ( pVHost->BytesLogEnabled() )
        {
            long long bytes = getReq()->getTotalLen();
            bytes += getResp()->getTotalLen();
            pVHost->logBytes( bytes);
        }
        if ((( !cancelled )||( isRespHeaderSent() ))
             &&pVHost->enableAccessLog() )
            pVHost->logAccess( this );
        else
            setAccessLogOff();
    }
    else 
        if ( m_pNtwkIOLink )
            HttpLog::logAccess( NULL, 0, this );
}

void HttpSession::resumeSSI()
{
    if ( m_pSubResp )
    {
        delete m_pSubResp;
        m_pSubResp = NULL;
    }
    m_request.restorePathInfo();
    releaseSendFileInfo();
    if (( m_pGzipBuf )&&( !m_pGzipBuf->isStreamStarted() ))
        m_pGzipBuf->reinit();
    SSIEngine::resumeExecute( this );
    return;
}

inline void HttpSession::releaseStaticFileCacheData(StaticFileCacheData * &pCache)
{
    if (pCache && pCache->decRef() == 0)
        pCache->setLastAccess( DateTime::s_curTime );
}

inline void HttpSession::releaseFileCacheDataEx(FileCacheDataEx * &pECache)
{
    if (pECache && pECache->decRef() == 0)
    {
        if ( pECache->getfd() != -1 )
            pECache->closefd();
    }
}

void HttpSession::releaseSendFileInfo()
{
    StaticFileCacheData * &pData = m_sendFileInfo.getFileData();
    if ( pData )
    {
        releaseStaticFileCacheData(pData);
        FileCacheDataEx * &pECache = m_sendFileInfo.getECache();
        releaseFileCacheDataEx(pECache);
        memset(&m_sendFileInfo, 0, sizeof(m_sendFileInfo));
    }
}

void HttpSession::nextRequest()
{
    
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpSession::nextRequest()!",
            getLogId() ));
    getStream()->flush();
    
    if ( m_pHandler )
    {
        HttpGlobals::s_reqStats.incReqProcessed();
        cleanUpHandler();
    }
    
//     //for SSI, should resume 
//     if ( m_request.getSSIRuntime() )
//     {
//         return resumeSSI();
//     }    

    if ( m_pChunkOS )
    {
        m_pChunkOS->reset();
        releaseChunkOS();
    }
    
    if ( !m_request.isKeepAlive() || getStream()->isSpdy() )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] Non-KeepAlive, CLOSING!",
                getLogId() ));
        closeConnection();
    }
    else
    {
        if ( m_iFlag & HSF_HOOK_SESSION_STARTED )
        {
            if ( m_sessionHooks.isEnabled( LSI_HKPT_HTTP_END) ) 
                m_sessionHooks.runCallbackNoParam(LSI_HKPT_HTTP_END, (LsiSession *)this);
        }
        
        m_sessionHooks.reset();
        m_iFlag = 0;
        logAccess( 0 );
        ++m_iReqServed;
        m_lReqTime = DateTime::s_curTime;
        m_iReqTimeUs = DateTime::s_curTimeUs;
        m_request.reset2();
        releaseSendFileInfo();
        if ( m_pRespBodyBuf )
            releaseRespCache();        
        if ( m_pGzipBuf )
            releaseGzipBuf();

        getStream()->switchWriteToRead();
        if ( getStream()->isLogIdBuilt() )
        {
            safe_snprintf( getStream()->getIdBuf().buf() +
                 getStream()->getIdBuf().len(), 10, "-%hu", m_iReqServed );
        }
        
        if ( m_request.pendingHeaderDataLen() )
        {
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] Pending data in header buffer, set state to READING!",
                    getLogId() ));
            setState( HSS_READING );
            //if ( !inProcess() )
                processPending( 0 );
        }
        else
        {
            setState( HSS_WAITING );
            ++HttpGlobals::s_iIdleConns;
        }
    }
}

int HttpSession::read( char * pBuf, int size )
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
    return getStream()->read( pBuf, size );
}

int HttpSession::readv( struct iovec *vector, size_t count)
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


int HttpSession::processReqBody()
{
    int ret = m_request.prepareReqBodyBuf();
    if ( ret )
        return ret;
    setState( HSS_READING_BODY );
    if ( m_request.isChunked() )
    {
        setupChunkIS();
    }
    //else
    //    m_request.processReqBodyInReqHeaderBuf();
    return readReqBody();
}

void HttpSession::setupChunkIS()
{
    assert ( m_pChunkIS == NULL );
    {
        m_pChunkIS = HttpGlobals::getResManager()->getChunkInputStream();
        m_pChunkIS->setStream( getStream() );
        m_pChunkIS->open();
    }
}


bool HttpSession::endOfReqBody()
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

int HttpSession::reqBodyDone()
{
    int ret;
    setFlag(HSF_REQ_BODY_DONE, 1);
    if ( m_sessionHooks.isEnabled(LSI_HKPT_RCVD_REQ_BODY) )
    {
        ret = m_sessionHooks.runCallbackNoParam(LSI_HKPT_RCVD_REQ_BODY, (LsiSession *)this);
        if ( ret <= -1)
        {
            return getModuleDenyCode( LSI_HKPT_RCVD_REQ_BODY );
        }
    }
    
    return 0;
}

int HttpSession::readReqBody()
{
    char * pBuf;
    size_t size = 0;
    int ret = 0;
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] Read Request Body!",
            getLogId() ));

    const LsiApiHooks *pReadHooks = m_sessionHooks.get(LSI_HKPT_RECV_REQ_BODY);
    int count = 0;
    int hasBufferedData = 1;
    int endBody;
    while( !(endBody = endOfReqBody()) || hasBufferedData)
    {
        hasBufferedData = 0;
        if (!endBody)
        {
            if (( ret < (int)size ) ||(++count == 10 ))
            {
                return 0;
            }
        }
        
        pBuf = m_request.getBodyBuf()->getWriteBuffer( size );
        if ( !pBuf )
        {
            LOG_ERR(( getLogger(), "[%s] out of swapping space while reading request body!", getLogId() ));
            return SC_400;
        }
        
        if ( !pReadHooks ||( pReadHooks->size() == 0))
            ret = readReqBodyTermination( (LsiSession *)this, pBuf, size );
        else
        {
            m_sessionHooks.triggered();    

            lsi_cb_param_t param;
            lsi_hook_info_t hookInfo;
            param._session = (LsiSession *)this;

            hookInfo._hooks = pReadHooks;
            hookInfo._termination_fp = (void*)readReqBodyTermination;
            param._cur_hook = (void *)((LsiApiHook *)pReadHooks->end() - 1);
            param._hook_info = &hookInfo;
            param._param = pBuf;
            param._param_len = size;
            param._flag_out = &hasBufferedData;
            ret = (*(((LsiApiHook *)param._cur_hook)->_cb))( &param );
            LOG_D (( getLogger(), "[%s][LSI_HKPT_RECV_REQ_BODY] read  %d ", getLogId(), ret ));
        }
        if ( ret > 0 )
        {
            m_request.getBodyBuf()->writeUsed( ret );
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] Finsh request body %ld/%ld bytes!",
                    getLogId(), m_request.getContentFinished(),
                        m_request.getContentLength() ));
            if ( m_pHandler && !getFlag(HSF_REQ_WAIT_FULL_BODY))
            {
                m_pHandler->onRead( this );
                if ( getState() == HSS_REDIRECT )
                    return 0;
            }
            
        }
        else if ( ret == -1 )
        {
            return SC_400;
        }
        else if ( ret == -2 )
        {
            return getModuleDenyCode( LSI_HKPT_RECV_REQ_BODY );
        }
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
    //getStream()->setFlag( HIO_FLAG_WANT_READ, 0 );
    suspendRead();
    return reqBodyDone();
}

int HttpSession::resumeHandlerProcess()
{
    if ( !( m_iFlag & HSF_URI_PROCESSED ) )
    {
        m_iFlag |= HSF_URI_PROCESSED;
        return processURI( 0 );
    }
    if ( m_pHandler )
        m_pHandler->onRead( this );
    else
        return handlerProcess( m_request.getHttpHandler() );
    return 0;

}



int HttpSession::restartHandlerProcess()
{
    int sessionLevels[] = { 
        LSI_HKPT_URI_MAP, LSI_HKPT_RECV_RESP_HEADER, 
        LSI_HKPT_RECV_RESP_BODY, LSI_HKPT_RCVD_RESP_BODY,
        LSI_HKPT_SEND_RESP_HEADER,
        LSI_HKPT_SEND_RESP_BODY, LSI_HKPT_HTTP_END };
    HttpContext *pContext = &(m_request.getVHost()->getRootContext());
    m_sessionHooks.restartSessionHooks( sessionLevels, sizeof( sessionLevels )/ sizeof(int), pContext->getSessionHooks() );
    
    if ( m_sessionHooks.isEnabled( LSI_HKPT_HANDLER_RESTART) )
    {
        m_sessionHooks.triggered();    
        m_sessionHooks.runCallbackNoParam(LSI_HKPT_HANDLER_RESTART, (LsiSession *)this);
        
    }

    m_iFlag &= ~( HSF_RESP_HEADER_DONE | HSF_RESP_WAIT_FULL_BODY | HSF_RESP_FLUSHED | HSF_HANDLER_DONE );
    
    if ( m_pHandler )
        cleanUpHandler();
    
    releaseSendFileInfo();
    
    if ( m_pChunkOS )
    {
        m_pChunkOS->reset();
        releaseChunkOS();
    }
    if ( m_pRespBodyBuf )
        releaseRespCache();    
    if ( m_pGzipBuf )
        releaseGzipBuf();

    m_response.reset();
    return 0;
}


int HttpSession::readReqBodyTermination( LsiSession *pSession, char * pBuf, int size )
{
    HttpSession *pThis = (HttpSession *)pSession;
    int len = 0;
    if ( pThis->m_pChunkIS )
    {
        len = pThis->m_pChunkIS->read( pBuf, size );
    }
    else
    {
        int toRead = pThis->m_request.getBodyRemain();
        if ( toRead > size )
            toRead = size ;
        if (toRead > 0)
            len = pThis->read( pBuf, toRead );
    }
    if ( len > 0 )
        pThis->m_request.contentRead( len );

    return len;
}


int HttpSession::readToHeaderBuf()
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
        sz = getStream()->read( pBuf,
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

void HttpSession::processPending( int ret )
{
    if (( getState() != HSS_READING )||
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
    {
        if ( isSSL() )
            ret = readToHeaderBuf();
        else
            return;
    }
    if (( !ret )&&( m_request.getStatus() == HttpReq::HEADER_OK ))
        ret = processNewReq();
    if (( ret )&&( getStream()->getState() < HIOS_SHUTDOWN ))
    {
        httpError( ret );
    }

}

int HttpSession::updateClientInfoFromProxyHeader( const char * pProxyHeader )
{
    char achIP[128];
    int len = m_request.getHeaderLen( HttpHeader::H_X_FORWARDED_FOR );
    char * p = (char *)memchr( pProxyHeader, ',', len );
    if ( p )
        len = p - pProxyHeader;
    if (( len <= 0 )||( len > 16 ))
    {
        //error, not a valid IPv4 address
        return 0;
    }
    
    memmove( achIP, pProxyHeader, len );
    achIP[len] = 0;
    struct sockaddr_in addr;
    memset( &addr, 0, sizeof( sockaddr_in ) );
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr( achIP );
    if ( addr.sin_addr.s_addr == INADDR_BROADCAST )
    {
        //Failed to parse the address string to IPv4 address
        return 0;
    }
    ClientInfo * pInfo = HttpGlobals::getClientCache()
                ->getClientInfo( (struct sockaddr *)&addr );
    if ( pInfo )
    {
        if ( pInfo->checkAccess() )
        {
            //Access is denied
            return SC_403;
        }
    }
    addEnv( "PROXY_REMOTE_ADDR", 17, 
                getPeerAddrString(), getPeerAddrStrLen() );

    m_pNtwkIOLink->changeClientInfo( pInfo );
    return 0;
}

int HttpSession::processWebSocketUpgrade( const HttpVHost * pVHost )
{
    HttpContext* pContext = pVHost->getContext(m_request.getURI(), 0);
    LOG_D ((getLogger(), "[%s] VH: web socket, name: [%s] URI: [%s]", getLogId(), pVHost->getName(), m_request.getURI() )); 
    if ( pContext && pContext->getWebSockAddr()->get() ) 
    {
        m_request.setStatusCode( SC_101 );
        logAccess( 0 );
        L4Handler *pL4Handler = new L4Handler();
        pL4Handler->assignStream( getStream() );
        pL4Handler->init(m_request, pContext->getWebSockAddr(), getPeerAddrString(), getPeerAddrStrLen());
        LOG_D ((getLogger(), "[%s] VH: %s web socket !!!", getLogId(), pVHost->getName() ));
        m_request.reset2();
        recycle();
        return 0;
        //DO NOT release pL4Handler, it will be releaseed itself.
    }
    else
    {
        LOG_INFO(( getLogger(), "[%s] Cannot found web socket backend URI: [%s]", getLogId(), m_request.getURI() )); 
        m_request.keepAlive( 0 );
        return SC_404;
    }
}


int HttpSession::processNewReq()
{
    int ret;
    if ( getStream()->isSpdy() )
    {
        m_request.keepAlive( 0 );
        m_request.orGzip( REQ_GZIP_ACCEPT | 
            HttpServerConfig::getInstance().getGzipCompress()
        );
    }
    if ((HttpGlobals::s_useProxyHeader == 1)||
        ((HttpGlobals::s_useProxyHeader == 2)&&( getClientInfo()->getAccess() == AC_TRUST )))
    {
        const char * pProxyHeader = m_request.getHeader( HttpHeader::H_X_FORWARDED_FOR );
        if ( *pProxyHeader )
        {
            ret = updateClientInfoFromProxyHeader( pProxyHeader );
            if ( ret )
                return ret;
        }
    }
    m_lReqTime = DateTime::s_curTime;
    m_iReqTimeUs = DateTime::s_curTimeUs;

    m_iFlag &= ~HSF_ACCESS_LOG_OFF;
    
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
    
    getStream()->setLogger( pVHost->getLogger() );
    if ( getStream()->isLogIdBuilt() )
    {
        AutoStr2 &id = getStream()->getIdBuf();
        register char * p = id.buf() + id.len();
        while( *p && *p != '#' )
            ++p;
        *p++ = '#';
        memccpy( p, pVHost->getName(), 0, id.buf() + MAX_LOGID_LEN - p );
    }
    
    HttpContext *pContext0 = ((HttpContext *)&(pVHost->getRootContext()));
    m_sessionHooks.inherit(pContext0->getSessionHooks());
    m_pModuleConfig = pContext0->getModuleConfig();
    
    //Run LSI_HKPT_HTTP_BEGIN after the inherit from vhost
    m_iFlag |= HSF_HOOK_SESSION_STARTED;
    if ( m_sessionHooks.isEnabled( LSI_HKPT_HTTP_BEGIN) )
    {
        m_sessionHooks.triggered();    
        ret = m_sessionHooks.runCallbackNoParam(LSI_HKPT_HTTP_BEGIN, (LsiSession *)this);
        if ( ret <= -1)
        {
            return getModuleDenyCode( LSI_HKPT_HTTP_BEGIN );
        }
        
    }
    

    ret = m_request.processNewReqData(getPeerAddr());
    if ( ret )
    {
        return ret;
    }
    
    if ( m_request.isWebsocket() )
    {
        return processWebSocketUpgrade( pVHost );
    }
    
    m_request.setStatusCode( SC_200 );
    if ( m_sessionHooks.isEnabled( LSI_HKPT_RECV_REQ_HEADER) ) 
    {
        m_sessionHooks.triggered();    
        if ( m_sessionHooks.runCallbackNoParam(LSI_HKPT_RECV_REQ_HEADER, (LsiSession *)this) < 0 )
        {
            return getModuleDenyCode( LSI_HKPT_RECV_REQ_HEADER );
        }
        if ( getState() == HSS_EXT_REDIRECT )
        {
            return m_request.getStatusCode();
        }
    }
    if ((m_pNtwkIOLink->isThrottle())&&( m_pNtwkIOLink->getClientInfo()->getAccess() != AC_TRUST ))
        m_pNtwkIOLink->getThrottleCtrl()->adjustLimits( pVHost->getThrottleLimits() );
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
        else if ( m_pNtwkIOLink->getClientInfo()->getOverLimitTime() )
        {
            m_request.keepAlive( false );
            if ( D_ENABLED( DL_LESS ) )
                LOG_D(( getLogger(), "[%s] Connections is over the soft limit"
                        ", keep-alive off.",
                        getLogId()));
        }
    }

    //m_processedURI = 0;
    off_t l = m_request.getContentLength();
    if ( l != 0 )
    {
        if ( l > HttpServerConfig::getInstance().getMaxReqBodyLen() )
        {
            LOG_NOTICE(( getLogger(),
                "[%s] Request body size: %ld is too big!",
                 getLogId(), l ));
            getStream()->setFlag( HIO_FLAG_WANT_READ, 0 );
            getStream()->suspendRead();
            return SC_413;
        }
        ret = processReqBody();
        if ( ret )
            return ret;
        if ( getFlag( HSF_REQ_WAIT_FULL_BODY | HSF_REQ_BODY_DONE ) == HSF_REQ_WAIT_FULL_BODY )
            return 0;
        //if (shouldReadAllReqBody())
        //    return 0;
    }
    else
    {
        ret = reqBodyDone();
        getStream()->setFlag( HIO_FLAG_WANT_READ, 0 );
        if ( ret )
            return ret;
    }
    
    if ( !(m_iFlag & HSF_URI_PROCESSED ))
    {
        m_iFlag |= HSF_URI_PROCESSED;
        return processURI( 0 );
    }
    return 0;
}




int HttpSession::checkAuthentication( const HTAuth * pHTAuth,
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

void HttpSession::resumeAuthentication()
{
    int ret = processURI( 1 );
    if ( ret )
        httpError( ret );
}

int HttpSession::checkAuthorizer( const HttpHandler * pHandler )
{
    int ret = assignHandler( pHandler );
    if ( ret )
        return ret;
    setState( HSS_EXT_AUTH );
    return m_pHandler->process( this, pHandler );
}


void HttpSession::authorized()
{
    if ( m_pHandler )
        cleanUpHandler();
    int ret = processURI( 2 );
    if ( ret )
        httpError( ret );

}

void HttpSession::addEnv( const char * pKey, int keyLen, const char * pValue, long valLen )
{
    if (pKey == NULL || keyLen <= 0)
        return ;
    
    m_request.addEnv(pKey, keyLen, pValue, (int)valLen);
    lsi_callback_pf cb = EnvManager::getInstance().findHandler(pKey);
    if (cb && (0 != EnvManager::getInstance().execEnvHandler((LsiSession *)this, cb, (void *)pValue, valLen)) )
        return ;
}

int HttpSession::redirect( const char * pNewURL, int len, int alloc )
{
    int ret = m_request.redirect( pNewURL, len, alloc );
    if ( ret )
        return ret;
    m_sendFileInfo.reset();
    if (m_pHandler)
        cleanUpHandler();
    m_request.setHandler(NULL);
    m_request.setContext(NULL);
    return processURI( 0 );
}


int HttpSession::processURI( int resume )
{
    const HttpVHost * pVHost = m_request.getVHost();
    if (!pVHost)
        return 0;
    
    int ret;
    int proceed = 1;
    
    if (this->getReq()->getHttpHandler() != NULL &&
        this->getReq()->getHttpHandler()->getHandlerType() == HandlerType::HT_MODULE)
    {
        return handlerProcess( m_request.getHttpHandler() );
    }

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
                const HttpContext *pContext = &(pVHost->getRootContext());
                m_request.setContext( pContext );
                if (m_sessionHooks.hasOwnCopy())
                    m_sessionHooks.reset();
                m_sessionHooks.inherit(((HttpContext *)pContext)->getSessionHooks());
                m_pModuleConfig = ((HttpContext *)pContext)->getModuleConfig();
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
            if ((!m_request.getContextState(SKIP_REWRITE) )
                &&( pContext->rewriteEnabled() & REWRITE_ON ))
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
                            if (m_sessionHooks.hasOwnCopy())
                                m_sessionHooks.reset();
                            m_sessionHooks.inherit(((HttpContext *)pContext)->getSessionHooks());
                            m_pModuleConfig = ((HttpContext *)pContext)->getModuleConfig();
                            goto DO_AUTH;
                        }
                        break;
                    }
                }
            }
            pContext = m_request.getContext();
            //since context chnaged, need to update m_sessionHooks & m_pModuleConfig
            m_pModuleConfig = ((HttpContext *)pContext)->getModuleConfig();
            if (!m_sessionHooks.hasOwnCopy())
                m_sessionHooks.inherit(((HttpContext *)pContext)->getSessionHooks());//FIXME

            if ( m_sessionHooks.isEnabled( LSI_HKPT_URI_MAP) )
            {
                m_sessionHooks.triggered();    
                if ( m_sessionHooks.runCallbackNoParam(LSI_HKPT_URI_MAP, (LsiSession *)this) < 0 )
                {
                    return getModuleDenyCode( LSI_HKPT_URI_MAP );
                }
                if ( getState() == HSS_EXT_REDIRECT )
                {
                    return m_request.getStatusCode();
                }
            }

            //Check is handler type is module, may be registered staticly or dynamically.
            if (m_request.getHttpHandler() && m_request.getHttpHandler()->getHandlerType() == HandlerType::HT_MODULE)
                goto DO_AUTH;
            
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
        int         ret1;

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
                    ret1 = checkAuthentication( aaa.m_pHTAuth, aaa.m_pRequired, resume);
                     if ( ret1 )
                    {
                        if ( D_ENABLED( DL_LESS ) )
                            LOG_D(( getLogger(), "[%s] checkAuthentication() return %d",
                                   getLogId(), ret1));
                        if ( ret1 == -1 ) //processing authentication
                        {
                            setState(HSS_EXT_AUTH );
                            return 0;
                        }
                        return ret1;
                    }
                }
                if ( aaa.m_pAuthorizer )
                    return checkAuthorizer( aaa.m_pAuthorizer );
            }
            if ( m_sessionHooks.isEnabled( LSI_HKPT_HTTP_AUTH ) )
            {
                m_sessionHooks.triggered();    
                ret1 = m_sessionHooks.runCallback(LSI_HKPT_HTTP_AUTH, (LsiSession *)this, 
                                                NULL, 0, NULL,0 );
                ret1 = processHkptResult( LSI_HKPT_HTTP_AUTH, ret1 );
                if ( ret1 )
                    return m_request.getStatusCode();
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
        setState(HSS_EXT_AUTH );
        return 0;
    default:        //error
        return ret;
    }
}

bool ReqHandler::notAllowed( int Method ) const
{
    return (Method > HttpMethod::HTTP_POST);
}

int HttpSession::setUpdateStaticFileCache( StaticFileCacheData * &pCache, 
                                              FileCacheDataEx *  &pECache,
                                              const char * pPath, int pathLen,
                                              int fd, struct stat &st ) 
{
    int ret;
    ret = HttpGlobals::getStaticFileCache()->getCacheElement( 
            pPath, pathLen, st, fd, pCache, pECache );
    if ( ret )
    {
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( getLogger(), "[%s] getCacheElement() return %d",
                    getLogId(), ret));
        pCache = NULL;
        return ret;
    }
    pCache->setLastAccess( DateTime::s_curTime );
    pCache->incRef();
    return 0;
}

int HttpSession::getParsedScript( SSIScript * &pScript )
{
    int ret;
    StaticFileCacheData * &pCache = m_sendFileInfo.getFileData();
    FileCacheDataEx *  &pECache = m_sendFileInfo.getECache();
    
    const AutoStr2 * pPath = m_request.getRealPath();
    ret = setUpdateStaticFileCache( pCache, pECache, pPath->c_str(), pPath->len(), 
                                    m_request.transferReqFileFd(), m_request.getFileStat() );
    if ( ret )
        return ret;

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


int HttpSession::startServerParsed()
{
    SSIScript * pScript = NULL;
    getParsedScript( pScript );
    if ( !pScript )
        return SC_500;

    return SSIEngine::startExecute( this, pScript );
}


int HttpSession::handlerProcess( const HttpHandler * pHandler )
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
        setFlag( HSF_REQ_WAIT_FULL_BODY );
    if ( getFlag( HSF_REQ_WAIT_FULL_BODY| HSF_REQ_BODY_DONE ) == HSF_REQ_WAIT_FULL_BODY )
        return 0;
    
    int ret = assignHandler(m_request.getHttpHandler() );
    if ( ret )
        return ret;
    
    setState( HSS_PROCESSING );
    //PORT_FIXME: turn off for now
    //pTC->incReqProcessed( m_pHandler->getType() == HandlerType::HT_DYNAMIC );
    
    ret = m_pHandler->process( this, m_request.getHttpHandler() );
    
    if (( ret == 0 )&&( HSS_COMPLETE == getState() ))
        nextRequest();
    else if (ret == 1)
    {
        continueWrite();
        ret = 0;
    }
    return ret;
}


int HttpSession::assignHandler( const HttpHandler * pHandler )
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
    case HandlerType::HT_MODULE:
    case HandlerType::HT_LOADBALANCER:
    {
        if ( m_request.getStatus() != HttpReq::HEADER_OK )
            return SC_400;  //cannot use dynamic request handler to handle invalid request
/*        if ( m_request.getMethod() == HttpMethod::HTTP_POST )
            getClientInfo()->setLastCacheFlush( DateTime::s_curTime );  */      
//         if ( m_request.getSSIRuntime() )
//         {
//             if (( m_request.getSSIRuntime()->isCGIRequired() )&&
//                 ( handlerType != HandlerType::HT_CGI ))
//             {
//                 LOG_INFO(( getLogger(), "[%s] Server Side Include request a CGI script, [%s] is not a CGI script, access denied.",
//                     getLogId(), m_request.getURI() ));
//                 return SC_403;
//             }
//             assert( m_pSubResp == NULL );
//             m_pSubResp = new HttpResp();
//         }
//         else
        {
            resetResp();
        }       
        
        const char * pType = HandlerType::getHandlerTypeString( handlerType );
        if ( D_ENABLED( DL_LESS ) )
        {
             LOG_D(( getLogger(),
                 "[%s] run %s processor.",
                 getLogId(), pType ));
        }

        if ( getStream()->isLogIdBuilt() )
        {
            AutoStr2 &id = getStream()->getIdBuf();
            register char * p = id.buf() + id.len();
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



void HttpSession::sendHttpError( const char * pAdditional )
{
    register int statusCode = m_request.getStatusCode();
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpSession::sendHttpError(),code=%s",
            getLogId(),
            HttpStatusCode::getCodeString(m_request.getStatusCode()) + 1 ));
    if (( statusCode < 0 )||( statusCode >= SC_END ))
    {
        LOG_ERR(( "[%s] invalid HTTP status code: %d!", getLogId(), statusCode ));
        statusCode = SC_500;
    }
    if ( statusCode == SC_503 )
    {
        ++HttpGlobals::s_503Errors;
    }
    if ( isRespHeaderSent() )
        return;
//     if ( m_request.getSSIRuntime() )
//     {
//         if ( statusCode < SC_500 )
//         {
//             SSIEngine::printError( this, NULL );
//         }
//         continueWrite();
//         setState( HSS_COMPLETE );
//         return;
//     }    
    if ( (m_request.getStatus() != HttpReq::HEADER_OK )
        || HttpStatusCode::fatalError( statusCode )
        || m_request.getBodyRemain() > 0  )
    {
        m_request.keepAlive( false );
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
            pErrDoc = pContext->getErrDocUrl( statusCode );
            if ( pErrDoc )
            {
                if ( statusCode == SC_401 )
                    m_request.orContextState( KEEP_AUTH_INFO );
                if ( *pErrDoc->c_str() == '"' )
                    pAdditional = pErrDoc->c_str()+1;
                else
                {
                    m_request.setErrorPage();
                    if (( statusCode >= SC_300 )&&( statusCode <= SC_403 ))
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
    setState( HSS_WRITING );
    buildErrorResponse( pAdditional );
    endResponse(1);
    HttpGlobals::s_reqStats.incReqProcessed();
}

int HttpSession::buildErrorResponse( const char * errMsg )
{
    register int errCode = m_request.getStatusCode();
//     if ( m_request.getSSIRuntime() )
//     {
//         if (( errCode >= SC_300 )&&( errCode < SC_400 ))
//         {
//             SSIEngine::appendLocation( this, m_request.getLocation(),
//                  m_request.getLocationLen() );
//             return 0;
//         }
//         if ( errMsg == NULL )
//             errMsg = HttpStatusCode::getRealHtml( errCode );
//         if ( errMsg )
//         {
//             appendDynBody( 0, errMsg, strlen(errMsg) );
//         }
//         return 0;
//     }  
    
    resetResp();
    //m_response.prepareHeaders( &m_request );
    //register int errCode = m_request.getStatusCode();
    register unsigned int ver = m_request.getVersion();
    if ( ver > HTTP_1_0 )
    {
        LOG_ERR(( "[%s] invalid HTTP version: %d!", getLogId(), ver ));
        ver = HTTP_1_1;
    }
    if ( sendBody() )
    {
        const char * pHtml = HttpStatusCode::getRealHtml( errCode );
        if ( pHtml )
        {
            int len = HttpStatusCode::getBodyLen( errCode );
            m_response.setContentLen( len );
            m_response.getRespHeaders().add(HttpRespHeaders::H_CONTENT_TYPE,  "text/html", 9 );
            if ( errCode >= SC_307 )
            {
                m_response.getRespHeaders().add(HttpRespHeaders::H_CACHE_CTRL, "private, no-cache, max-age=0", 28 );
                m_response.getRespHeaders().add(HttpRespHeaders::H_PRAGMA, "no-cache", 8 );
            }
            
            int ret = appendDynBody( pHtml, len );
            if ( ret < len )
                LOG_ERR(( "[%s] Failed to create error response body" , getLogId() ));
                
            return 0;
        }
        else
        {
            m_response.setContentLen( 0 );
            //m_request.setNoRespBody();
        }
    }
    return 0;
}

int HttpSession::onReadEx()
{
    //printf( "^^^HttpSession::onRead()!\n" );
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpSession::onReadEx(), state: %d!",
            getLogId(), getState() ));

    int ret = 0;
    switch(  getState() )
    {
    case HSS_WAITING:
        --HttpGlobals::s_iIdleConns;
        setState( HSS_READING );
        //fall through;    
    case HSS_READING:
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

    case HSS_COMPLETE:
        break;
        
    case HSS_READING_BODY:
    case HSS_PROCESSING:
    case HSS_WRITING:
        if ( m_request.getBodyBuf() && !getFlag( HSF_REQ_BODY_DONE ) )
        {
            ret = readReqBody();
            if (( !ret )&& getFlag( HSF_REQ_BODY_DONE ) )
                ret = resumeHandlerProcess();
            break;
        }
        else
        {
            if ( m_pNtwkIOLink )
            {
                if ( m_pNtwkIOLink->detectCloseNow() )
                    return 0;
            }
            getStream()->setFlag( HIO_FLAG_WANT_READ, 0 );
        }
    default:
        suspendRead();
        return 0;
    }
    if (( ret )&&( getStream()->getState() < HIOS_SHUTDOWN ))
    {
        httpError( ret );
    }

    if ( HSS_COMPLETE == getState() )
        nextRequest();

    //processPending( ret );
//    printf( "onRead loops %d times\n", iCount );
    return 0;
}


int HttpSession::doWrite( )
{
    int ret = 0;
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpSession::doWrite()!",
            getLogId() ));
    ret = flush();
    if ( ret )
        return ret;
    
    if ( m_pHandler && !( m_iFlag & (HSF_HANDLER_DONE |HSF_MODULE_WRITE_SUSPENDED) ) )
    {
        ret = m_pHandler->onWrite( this );
        if ( D_ENABLED( DL_MORE ))
            LOG_D(( getLogger(), "[%s] m_pHandler->onWrite() return %d\n",
                    getLogId(), ret ));
    }
    else
        suspendWrite();
    
    if ( ret == 0 && 
        ( (m_iFlag & (HSF_HANDLER_DONE | 
                      HSF_RECV_RESP_BUFFERED | 
                      HSF_SEND_RESP_BUFFERED)) == HSF_HANDLER_DONE ))
    {   
        if (getRespCache() && !getRespCache()->empty())
            return 1;
        
        setFlag(HSF_RESP_FLUSHED, 0);
        flush();
    }
    else if ( ret == -1 )
    {
        getStream()->setState( HIOS_CLOSING );
    }
        
    return ret;
}

int HttpSession::onWriteEx()
{
    //printf( "^^^HttpSession::onWrite()!\n" );
    int ret = 0;
    switch( getState() )
    {
    case HSS_THROTTLING:
        if ( m_pNtwkIOLink->getClientInfo()->getThrottleCtrl().allowProcess(m_pHandler->getType()) )
        {
            setState( HSS_PROCESSING );
            m_pNtwkIOLink->getClientInfo()->getThrottleCtrl().incReqProcessed(m_pHandler->getType());
            ret = m_pHandler->process( this, m_request.getHttpHandler() );
            if (( ret )&&( getStream()->getState() < HIOS_SHUTDOWN ))
            {
                httpError( ret );
            }
        }
        break;
    case HSS_WRITING:
        doWrite();
        break;
    case HSS_COMPLETE:
        break;
    case HSS_REDIRECT:
    case HSS_EXT_REDIRECT:
    case HSS_HTTP_ERROR:
        if ( isRespHeaderSent() )
        {
            closeConnection();
            return -1;
        }

        restartHandlerProcess();
        suspendWrite();
        {
            const char * pLocation = m_request.getLocation();
            if ( *pLocation )
            {
                if ( getState() == HSS_REDIRECT )
                {
                    int ret = redirect( pLocation, m_request.getLocationLen() );
                    if ( !ret )
                        break;
                    
//                     if ( m_request.getSSIRuntime() )
//                     {
//                         SSIEngine::printError( this, NULL );
//                         continueWrite();
//                         setState( HSS_COMPLETE );
//                         return 0;
//                     }                

                    m_request.setStatusCode( ret );
                }
                if ( m_request.getStatusCode() < SC_300 )
                {
                    LOG_INFO(( getLogger(), "[%s] redirect status code: %d",
                            getLogId(), m_request.getStatusCode() ));
                    m_request.setStatusCode( SC_307 );
                }
            }
            sendHttpError( NULL );
        }
        break;
    default:
        suspendWrite();
        break;
    }
    if ( HSS_COMPLETE == getState() )
        nextRequest();
    else if (( m_iFlag & HSF_HANDLER_DONE )&&( m_pHandler ))
    {
        HttpGlobals::s_reqStats.incReqProcessed();
        cleanUpHandler();
    }
    //processPending( ret );
    return 0;
}

void HttpSession::cleanUpHandler()
{
    register ReqHandler * pHandler = m_pHandler;
    m_pHandler = NULL;
    pHandler->cleanUp( this );
}


int HttpSession::onCloseEx()
{
    closeConnection();
    return 0;
}


void HttpSession::closeConnection( )
{
    if ( m_pHandler )
    {
        HttpGlobals::s_reqStats.incReqProcessed();
        cleanUpHandler();
    }
    
    if ( m_iFlag & HSF_HOOK_SESSION_STARTED )  //m_sessionHooks.isTriggered() )
    {
        //m_sessionHooks.clearTriggered();
        if ( m_sessionHooks.isEnabled( LSI_HKPT_HTTP_END) )
            m_sessionHooks.runCallbackNoParam(LSI_HKPT_HTTP_END, (LsiSession *)this);
    }
    m_iFlag = 0;
    if ( getStream()->getState() == HIOS_CLOSING )
        getStream()->setState( HIOS_CLOSING );
    if ( getStream()->isReadyToRelease() )
        return;
    //FIXME: could be double counted in here. 
    //if ( getState() == HSS_WAITING )
    //    --HttpGlobals::s_iIdleConns;
    
    m_request.keepAlive( 0 );
    
    if ( shouldLogAccess() )
    {
        logAccess( getStream()->getFlag( HIO_FLAG_PEER_SHUTDOWN ) );
    }

    if ( m_pSubResp )
    {
        delete m_pSubResp;
        m_pSubResp = NULL;
    }    
    
    m_lReqTime = DateTime::s_curTime;
    m_request.reset();
    m_request.resetHeaderBuf();
    releaseSendFileInfo();
    
    if ( m_pChunkOS )
    {
        m_pChunkOS->reset();
        releaseChunkOS();
    }
    if ( m_pRespBodyBuf )
        releaseRespCache();    
    
    if ( m_pGzipBuf )
        releaseGzipBuf();

    if ( m_pChunkIS )
    {
        HttpGlobals::getResManager()->recycle( m_pChunkIS );
        m_pChunkIS = NULL;
    }
    
    //For reuse condition, need to reste the sessionhooks
    m_sessionHooks.reset();
    getStream()->handlerReadyToRelease();
    getStream()->close();
}

void HttpSession::recycle()
{
    if ( D_ENABLED( DL_MORE ))
        LOG_D(( getLogger(), "[%s] HttpSession::recycle()\n", getLogId() ));
    HttpGlobals::getResManager()->recycle( this );
}
void HttpSession::setupChunkOS(int nobuffer)
{
    if ( !m_request.isKeepAlive() )
        return;
    m_response.setContentLen( LSI_RESP_BODY_SIZE_CHUNKED );
    if (( m_request.getVersion() == HTTP_1_1 )&&( nobuffer != 2 ))
    {
        m_response.appendChunked();
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] use CHUNKED encoding!",
                    getLogId() ));
        if ( m_pChunkOS )
            releaseChunkOS();
        m_pChunkOS = HttpGlobals::getResManager()->getChunkOutputStream();
        m_pChunkOS->setStream( getStream());
        m_pChunkOS->open();
        if ( !nobuffer )
            m_pChunkOS->setBuffering( 1 );
    }
    else
        m_request.keepAlive( false );
}

void HttpSession::releaseChunkOS()
{
    HttpGlobals::getResManager()->recycle( m_pChunkOS );
    m_pChunkOS = NULL;
}

#if !defined( NO_SENDFILE )

int HttpSession::chunkSendfile( int fdSrc, off_t off, size_t size )
{
    int written;

    off_t begin = off;
    off_t end = off + size;
    short& left = m_pChunkOS->getSendFileLeft();
    while( off < end )
    {
        if ( left == 0 )
        {
            m_pChunkOS->buildSendFileChunk( end - off );
        }
        written = m_pChunkOS->flush();
        if ( written < 0 )
            return written;
        else if ( written > 0 )
            break;;
        written = getStream()->sendfile( fdSrc, off, left );

        if ( written > 0 )
        {
            left -= written;
            off += written;
            if ( left == 0 )
                m_pChunkOS->appendCRLF();
        }
        else if (written < 0 )
            return written;
        if ( written == 0 )
            break;
    } 
    return off - begin;
}

int HttpSession::writeRespBodySendFile( int fdFile, off_t offset, size_t size )
{
    int written;
    if ( m_pChunkOS )
        written = chunkSendfile( fdFile, offset, size );
    else
        written = getStream()->sendfile( fdFile, offset, size );
    if ( written > 0 )
    {
        m_response.written( written );
        if ( D_ENABLED( DL_MEDIUM ) )
            LOG_D(( getLogger(),
                "[%s] Response body sent: %lld \n",
                getLogId(), (long long)m_response.getBodySent() ));
    }
    return written;    
    
}
#endif  


/**
  * @return -1 error
  *          0 complete
  *          1 continue
  */



int HttpSession::writeRespBodyDirect( const char * pBuf, int size )
{
    int written;
    if ( m_pChunkOS )
        written = m_pChunkOS->write( pBuf, size );
    else
        written = getStream()->write( pBuf, size );
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

int HttpSession::detectKeepAliveTimeout( int delta )
{
    register const HttpServerConfig& config = HttpServerConfig::getInstance();
    int c = 0;
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
        else if ((int)m_pNtwkIOLink->getClientInfo()->getConns() >
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
        getStream()->close();
    }
    return c;
}

int HttpSession::detectConnectionTimeout( int delta )
{
    register const HttpServerConfig& config = HttpServerConfig::getInstance();
    if ( (uint32_t)(DateTime::s_curTime) > getStream()->getActiveTime() +
        (uint32_t)(config.getConnTimeout()) )
    {
        if ( m_pNtwkIOLink->getfd() != m_pNtwkIOLink->getPollfd()->fd )
            LOG_ERR(( getLogger(),
                "[%s] BUG: fd %d does not match fd %d in pollfd!",
                getLogId(), m_pNtwkIOLink->getfd(), m_pNtwkIOLink->getPollfd()->fd ));
//            if ( D_ENABLED( DL_MEDIUM ) )
        if (( m_response.getBodySent() == 0 )|| !m_pNtwkIOLink->getEvents() )
        {
            LOG_INFO((getLogger(),
                "[%s] Connection idle time: %ld while in state: %d watching for event: %d,"
                "close!",
                getLogId(), DateTime::s_curTime - getStream()->getActiveTime(), getState(), m_pNtwkIOLink->getEvents() ));
            m_request.dumpHeader();
            if ( m_pHandler )
                m_pHandler->dump();
            if ( getState() == HSS_READING_BODY )
            {
                LOG_INFO((getLogger(),
                    "[%s] Request body size: %d, received: %d.",
                    getLogId(), m_request.getContentLength(),
                    m_request.getContentFinished() ));
            }
        }
        if (( getState() == HSS_PROCESSING )&&m_response.getBodySent() == 0 )
        {
            IOVec iov;
            iov.append( s_errTimeout, sizeof( s_errTimeout ) - 1 );
            getStream()->writev( iov, sizeof( s_errTimeout ) - 1 );
        }
        else
            getStream()->setAbortedFlag();
        closeConnection();
        return 1;
    }
    else
        return 0;
}

int HttpSession::isAlive()
{
    if ( getStream()->isSpdy() )
        return 1;
    return !m_pNtwkIOLink->detectClose();
    
}


int HttpSession::detectTimeout()
{
//    if ( D_ENABLED( DL_MEDIUM ) )
//        LOG_D((getLogger(),
//            "[%s] HttpSession::detectTimeout() ",
//            getLogId() ));
    int delta = DateTime::s_curTime - m_lReqTime;
    switch(getState() )
    {
    case HSS_WAITING:
        return detectKeepAliveTimeout( delta );
    case HSS_READING:
    case HSS_READING_BODY:
        //fall through
    default:
        return detectConnectionTimeout( delta);
    }
    return 0;
}

int HttpSession::onTimerEx( )
{
    if ( getState() ==  HSS_THROTTLING)
    {
        onWriteEx();
    }
    if ( detectTimeout() )
        return 1;
    if ( m_pHandler )
        m_pHandler->onTimer();
    return 0;
}

void HttpSession::releaseRespCache()
{
    if ( m_pRespBodyBuf )
    {
        HttpGlobals::getResManager()->recycle( m_pRespBodyBuf );
        m_pRespBodyBuf = NULL;
    }
}

static int writeRespBodyTermination( HttpSession *session, const char * pBuf, int len )
{
    return session->writeRespBodyDirect( pBuf, len );
}

int HttpSession::writeRespBody( const char * pBuf, int len )
{
    if ( m_sessionHooks.isDisabled( LSI_HKPT_SEND_RESP_BODY) )
        return writeRespBodyDirect( pBuf, len );
    
    return runFilter( LSI_HKPT_SEND_RESP_BODY, (void *)writeRespBodyTermination, pBuf, len, 0 );
}



int HttpSession::sendDynBody()
{
    size_t toWrite;
    char * pBuf;
    
    while ((( pBuf = m_pRespBodyBuf->getReadBuffer( toWrite ) )!= NULL )
            &&( toWrite > 0 ) )
    {
        register int len = toWrite;
        if ( m_response.getContentLen() > 0 )
        {
            register int allowed = m_response.getContentLen() - m_lDynBodySent;
            if ( len > allowed )
                len = allowed;
            if ( len <= 0 )
            {
                m_pRespBodyBuf->readUsed( toWrite );
                continue;
            }
        }
        
        int ret = writeRespBody( pBuf, len );
        if ( D_ENABLED( DL_MEDIUM ) )
            LOG_D(( getLogger(),
                "[%s] writeRespBody() len=%d return %d\n",
                getLogId(), len, ret ));
        if ( ret > 0 )
        {
            m_lDynBodySent += ret;
            if ( ret >= len )
                ret = toWrite;
            m_pRespBodyBuf->readUsed( ret );
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
    return 0;
}

int HttpSession::setupRespCache()
{
    if ( !m_pRespBodyBuf )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] allocate response body buffer", getLogId() ));
        m_pRespBodyBuf = HttpGlobals::getResManager()->getVMemBuf();
        if ( !m_pRespBodyBuf )
        {
            LOG_ERR(( getLogger(), "[%s] Failed to obtain VMemBuf, current pool size: %d,"
                            "capacity: %d.",
                        getLogId(), HttpGlobals::getResManager()->getVMemBufPoolSize(),
                        HttpGlobals::getResManager()->getVMemBufPoolCapacity() ));
            return -1;
        }
        if ( m_pRespBodyBuf->reinit(
                m_response.getContentLen() ) )
        {
            LOG_ERR(( getLogger(), "[%s] Failed to initialize VMemBuf, response body len: %ld.", getLogId(), m_response.getContentLen() ));
            return -1;
        }
        m_lDynBodySent = 0;
    }
    return 0;
}

extern int addModgzipFilter(lsi_session_t *session, int isSend, uint8_t compressLevel, int priority);
int HttpSession::setupGzipFilter()
{
    register char gz = m_request.gzipAcceptable();
    int recvhkptNogzip = m_sessionHooks.getFlag( LSI_HKPT_RECV_RESP_BODY ) & LSI_HOOK_FLAG_DECOMPRESS_REQUIRED;
    int  hkptNogzip = ( m_sessionHooks.getFlag( LSI_HKPT_RECV_RESP_BODY )
                       | m_sessionHooks.getFlag( LSI_HKPT_SEND_RESP_BODY ))
                      & LSI_HOOK_FLAG_DECOMPRESS_REQUIRED;
    if ( gz & (UPSTREAM_GZIP|UPSTREAM_DEFLATE) )
    {
        if ( recvhkptNogzip || hkptNogzip || !(gz & REQ_GZIP_ACCEPT) )
        {
            //setup decompression filter at RECV_RESP_BODY filter
            if ( addModgzipFilter((LsiSession *)this, 0, 0, -3000 ) == -1 )  //addModgzipFilter((LsiSession *)this, isSend, 0, -3000 );
                return -1;
            m_response.getRespHeaders().del( HttpRespHeaders::H_CONTENT_ENCODING );  //FIXME: check if removed in modgzip
                gz &= ~(UPSTREAM_GZIP|UPSTREAM_DEFLATE);
        }
    }
    
    if ( gz == GZIP_REQUIRED )
    {
        if ( !hkptNogzip )
        {
            if ( !m_pRespBodyBuf->empty() )
                m_pRespBodyBuf->rewindWriteBuf();
            if (( !m_request.getContextState( RESP_CONT_LEN_SET )
                ||m_response.getContentLen() > 200 ))
                if ( setupGzipBuf() == -1 )
                    return -1;
        }
        else //turn on compression at SEND_RESP_BODY filter
        {
            //g_api->add_session_hook( this, LSI_HKPT_SEND_RESP_BODY, gunzip_module, gunzip_function, LSI_HOOK_LAST, 0 );
            if ( addModgzipFilter((LsiSession *)this, 1, HttpServerConfig::getInstance().getCompressLevel(), 3000 ) == -1 )
                return -1;
            m_response.addGzipEncodingHeader();
        }
    }
    return 0;
}


int HttpSession::setupGzipBuf()
{
    if ( m_pRespBodyBuf )
    {
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( getLogger(), "[%s] GZIP response body in response body buffer!",
                getLogId() ));
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
        }

        if ( !m_pGzipBuf )
        {
            m_pGzipBuf = HttpGlobals::getResManager()->getGzipBuf();
        }
        if ( m_pGzipBuf )
        {
            m_pGzipBuf->setCompressCache( m_pRespBodyBuf );
            if (( m_pGzipBuf->init( GzipBuf::GZIP_DEFLATE,
                    HttpServerConfig::getInstance().getCompressLevel() ) == 0 )&&
                ( m_pGzipBuf->beginStream() == 0 ))
            {
                if ( D_ENABLED( DL_MEDIUM ) )
                    LOG_D(( getLogger(),
                        "[%s] setupGzipBuf() begin GZIP stream.\n",
                        getLogId() ));
                m_response.setContentLen( LSI_RESP_BODY_SIZE_UNKNOWN );
                m_response.addGzipEncodingHeader();
                m_request.orGzip( UPSTREAM_GZIP );
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




void HttpSession::releaseGzipBuf()
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


int HttpSession::appendDynBodyEx( const char * pBuf, int len )
{
    int ret = 0;
    if (len == 0)
        return 0;
    
    if (( m_pGzipBuf )&&(m_pGzipBuf->getType() == GzipBuf::GZIP_DEFLATE ))
    {
        ret = m_pGzipBuf->write( pBuf, len );
        //end of written response body
        if ( ret == 1 )
            ret = 0;
    }
    else
    {
        ret = appendRespBodyBuf( pBuf, len );
    }

    return ret;

}

int appendDynBodyTermination( HttpSession *conn, const char * pBuf, int len )
{
    return conn->appendDynBodyEx( pBuf, len );
}

int HttpSession::runFilter( int hookLevel, void * pfTermination, const char * pBuf, int len, int flagIn )
{
    int ret;
    int bit;
    int flagOut = 0;
    m_sessionHooks.triggered();    
    const LsiApiHooks *pHooks = m_sessionHooks.get(hookLevel);
    lsi_cb_param_t param;
    lsi_hook_info_t hookInfo;
    param._session = (LsiSession *)this;
    hookInfo._hooks = pHooks;
    hookInfo._termination_fp = pfTermination;
    param._cur_hook = (void *)pHooks->begin();
    param._hook_info = &hookInfo;
    param._param = pBuf;
    param._param_len = len;
    param._flag_out = &flagOut;
    param._flag_in = flagIn;
    ret = (*(((LsiApiHook *)param._cur_hook)->_cb))( &param );

    if ( hookLevel == LSI_HKPT_RECV_RESP_BODY )
        bit = HSF_RECV_RESP_BUFFERED;
    else 
        bit = HSF_SEND_RESP_BUFFERED;
    if ( flagOut )
        m_iFlag |= bit;
    else
        m_iFlag &= ~bit;
    
    if ( D_ENABLED( DL_MEDIUM ) )
        LOG_D (( getLogger(), "[%s][%s] input len: %d, flag in: %d, flag out: %d, ret: %d", 
                 getLogId(), LsiApiHooks::getHkptName( hookLevel ), len, flagIn, flagOut, ret ));
    return processHkptResult( hookLevel, ret );
    
}

int HttpSession::appendDynBody( const char * pBuf, int len )
{
    int ret;
    if ( !m_pRespBodyBuf )
    {
        setupDynRespBodyBuf();
        if( !m_pRespBodyBuf )
            return len;
    }

    if ( !getFlag( HSF_RESP_HEADER_DONE ) )
    {
        ret = respHeaderDone( 0 );    
        if ( ret )
        {
            return -1; 
        }
    }

    setFlag(HSF_RESP_FLUSHED, 0);
    
    if ( m_sessionHooks.isDisabled( LSI_HKPT_RECV_RESP_BODY) )
        return appendDynBodyEx( pBuf, len );
    return runFilter( LSI_HKPT_RECV_RESP_BODY, (void *)appendDynBodyTermination, pBuf, len, 0 );    
}

int HttpSession::appendRespBodyBuf( const char * pBuf, int len )
{
    char * pCache;
    size_t iCacheSize;
    const char * pBufOrg = pBuf;
    
    while( len > 0 )
    {
        pCache = m_pRespBodyBuf->getWriteBuffer( iCacheSize );
        if ( pCache )
        {
            int ret = len;
            if ( iCacheSize < (size_t)ret )
            {
                ret = iCacheSize;
            }
            if ( pCache != pBuf )
                memmove( pCache, pBuf, ret );
            pBuf += ret;
            len -= ret;
            m_pRespBodyBuf->writeUsed( ret );
        }
        else
        {
            LOG_ERR(( getLogger(), "[%s] out of swapping space while append to response buffer!", getLogId() ));
            return -1;
        }
    }
    return pBuf - pBufOrg;
}

int HttpSession::appendRespBodyBufV( const iovec *vector, int count )
{
    char * pCache;
    size_t iCacheSize;
    size_t len  = 0;
    char *pBuf;
    size_t iInitCacheSize;
    pCache = m_pRespBodyBuf->getWriteBuffer( iCacheSize );
    assert(iCacheSize > 0);
    iInitCacheSize = iCacheSize;
    
    if (!pCache)
    {
        LOG_ERR(( getLogger(), "[%s] out of swapping space while append[V] to response buffer!", getLogId() ));
        return -1;
    }
    
    for ( int i=0; i<count; ++i)
    {
        len = (*vector).iov_len;
        pBuf = (char *)((*vector).iov_base);
        ++vector;
        
        while(len > 0)
        {
            int ret = len;
            if ( iCacheSize < (size_t)ret )
            {
                ret = iCacheSize;
            }
            memmove( pCache, pBuf, ret );
            pCache += ret;
            iCacheSize -= ret;
            
            if(iCacheSize == 0)
            {
                m_pRespBodyBuf->writeUsed( iInitCacheSize - iCacheSize );
                pCache = m_pRespBodyBuf->getWriteBuffer( iCacheSize );
                iInitCacheSize = iCacheSize;
                pBuf += ret;
                len -= ret;
            }
            else
                break;  //quit the while loop
        }
    }

    if (iInitCacheSize > iCacheSize)
        m_pRespBodyBuf->writeUsed( iInitCacheSize - iCacheSize );
 
    
    return 0;
}

int HttpSession::shouldSuspendReadingResp()
{
    if ( m_pRespBodyBuf )
    {
        int buffered = m_pRespBodyBuf->getCurWBlkPos() - m_pRespBodyBuf->getCurRBlkPos();
        return (( buffered >= 1024 * 1024 )||( buffered < 0 ));
    }
    return 0; 
}

void HttpSession::resetRespBodyBuf()
{

    {
        if ( m_pGzipBuf )
            m_pGzipBuf->resetCompressCache();
        else
        {
            m_pRespBodyBuf->rewindReadBuf();
            m_pRespBodyBuf->rewindWriteBuf();
        }
    }
}
static char achOverBodyLimitError[] =
"<p>The size of dynamic response body is over the "
"limit, response is truncated by web server. "
"The limit is set by the "
"'Max Dynamic Response Body Size' in tuning section "
" of server configuration.";
int HttpSession::checkRespSize( int nobuffer )
{
    int ret = 0;
    if (m_pRespBodyBuf)
    {
        int curLen = m_pRespBodyBuf->writeBufSize();
        if ((nobuffer&&(curLen > 0))||(curLen > 1460 ))
        {
            if ( curLen > HttpServerConfig::getInstance().getMaxDynRespLen() )
            {
                LOG_WARN((getLogger(), "[%s] The size of dynamic response body is"
                                        " over the limit, abort!",
                        getLogId() ));
                appendDynBody( achOverBodyLimitError, sizeof( achOverBodyLimitError ) - 1 );
                if ( m_pHandler )
                    m_pHandler->abortReq();
                ret = 1;
                return ret;
            }

            flush();
        }
    }
    return ret;
}
//return 0 , caller should continue
//return !=0, the request has been redirected, should break the normal flow

int HttpSession::respHeaderDone(int iRespState)
{
    int ret = 0;
    if ( getFlag( HSF_RESP_HEADER_DONE ) )
        return 0;
    m_iFlag |= HSF_RESP_HEADER_DONE;
    if ( D_ENABLED( DL_MEDIUM ) )
        LOG_D(( getLogger(),
                "[%s] response header finished!",
                getLogId() ));
    if ( m_sessionHooks.isEnabled( LSI_HKPT_RECV_RESP_HEADER) )
    {
        m_sessionHooks.triggered();    
        ret = m_sessionHooks.runCallbackNoParam(LSI_HKPT_RECV_RESP_HEADER, (LsiSession *)this);
    }
    if ( processHkptResult( LSI_HKPT_RECV_RESP_HEADER, ret ) )
    {
        return 1;
    }
    if ( iRespState & HEC_RESP_AUTHORIZED )
        return 1;
    return 0;
    //setupDynRespBodyBuf( iRespState );
    
}

int HttpSession::endResponseInternal( int success )
{
    int ret = 0;
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] endResponseInternal()", getLogId() ));
    m_iFlag |= HSF_HANDLER_DONE;

    if ( sendBody() && m_sessionHooks.isEnabled( LSI_HKPT_RECV_RESP_BODY ) )
    {
        ret = runFilter( LSI_HKPT_RECV_RESP_BODY, (void *)appendDynBodyTermination, 
                          NULL, 0, LSI_CB_FLAG_IN_EOF ); 

    }

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
                LOG_D(( getLogger(), "[%s] endResponse() end GZIP stream.\n", getLogId() ));

        }
    }
    
    if ( !ret && m_sessionHooks.isEnabled( LSI_HKPT_RCVD_RESP_BODY) )
    {
        m_sessionHooks.triggered();    
        ret = m_sessionHooks.runCallback(LSI_HKPT_RCVD_RESP_BODY, (LsiSession *)this, 
                                         NULL, 0, NULL, success?LSI_CB_FLAG_IN_RESP_SUCCEED:0 );
        ret = processHkptResult( LSI_HKPT_RCVD_RESP_BODY, ret );
    }
    return ret;
}

int HttpSession::endResponse( int success )
{
    //If already called once, just return
    if (m_iFlag & HSF_HANDLER_DONE )
        return 0;

    if ( D_ENABLED( DL_MEDIUM ) )
        LOG_D(( getLogger(), "[%s] endResponse( %d )\n", getLogId(), success ));

    int ret = 0;

    ret = endResponseInternal( success );
    if ( ret )
        return ret;
    
    if ( !isRespHeaderSent() && (m_response.getContentLen() < 0) )
    {
        // header is not sent yet, no body sent yet.
        //content length = dynamic content + static send file
        long size = m_sendFileInfo.getRemain();
        if ( getRespCache() )
            size += getRespCache()->getCurWOffset();
        
        m_response.setContentLen( size );
        
    }

    setFlag(HSF_RESP_FLUSHED, 0);
    ret = flush();

    return ret;
}


int HttpSession::flushBody()
{
    int ret = 0;
    
    if ( m_iFlag & HSF_RECV_RESP_BUFFERED )
    {
        if ( m_sessionHooks.isEnabled( LSI_HKPT_RECV_RESP_BODY ) )
        {
            ret = runFilter( LSI_HKPT_RECV_RESP_BODY, (void *)appendDynBodyTermination, 
                            NULL, 0, LSI_CB_FLAG_IN_FLUSH ); 
        }
    }
    if ( !(m_iFlag & HSF_HANDLER_DONE) )
    {
        if ( m_pGzipBuf && m_pGzipBuf->isStreamStarted())
            m_pGzipBuf->flush();
        
    }
    if (( m_pRespBodyBuf && !m_pRespBodyBuf->empty() )/*&&( isRespHeaderSent() )*/)
    {
        ret = sendDynBody();
    }
    if (( ret == 0 )&&( m_sendFileInfo.getRemain() > 0 ))
    {
        ret = sendStaticFile( &m_sendFileInfo );
    }
    if ( ret > 0 )
    {
        return 1;
    }

    if ( m_sessionHooks.isEnabled( LSI_HKPT_SEND_RESP_BODY ) )
    {
        int flush = LSI_CB_FLAG_IN_FLUSH;
        if ( (( m_iFlag & ( HSF_HANDLER_DONE | HSF_RECV_RESP_BUFFERED )) == HSF_HANDLER_DONE ) )
            flush = LSI_CB_FLAG_IN_EOF;
        ret = runFilter( LSI_HKPT_SEND_RESP_BODY, (void *)writeRespBodyTermination, 
                        NULL, 0, flush ); 
    }
   
    //int nodelay = 1;
    //::setsockopt( getfd(), IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof( int ) );
    if ( m_pChunkOS )
    {
//            if ( !m_request.getSSIRuntime() )
        {
            if( m_pChunkOS->flush() != 0 )
                return 1;
            if ( (m_iFlag & ( HSF_HANDLER_DONE | HSF_RECV_RESP_BUFFERED 
                             | HSF_SEND_RESP_BUFFERED| HSF_CHUNK_CLOSED )) 
                        == HSF_HANDLER_DONE )
            {
                m_pChunkOS->close();
                if ( D_ENABLED( DL_LESS ))
                    LOG_D(( getLogger(), "[%s] Chunk closed!", getLogId() ));
                m_iFlag |= HSF_CHUNK_CLOSED;
            }
        }
    }
    return 0;
}

int HttpSession::flush()
{
    int ret = 0;
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] HttpSession::flush()!",
            getLogId() ));

    if (getFlag(HSF_RESP_FLUSHED))
    {    
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] HSF_RESP_FLUSHED flag is set, skip flush.", getLogId() ));
        return 0;
    }
    if ( !sendBody() && !getFlag( HSF_HANDLER_DONE ) )
    {
        ret = endResponseInternal( 1 );
        if ( ret )
            return 1;
    }
    else if ( getFlag( HSF_HANDLER_DONE | HSF_RESP_WAIT_FULL_BODY ) ==
        HSF_RESP_WAIT_FULL_BODY )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] cannot flush as response is not finished!",
                getLogId() ));
        return 0;
    }
    
    if ( !isRespHeaderSent() )
    {
        if ( sendRespHeaders() )
            return 0;
    }
    
    if ( sendBody() )
    {
        ret = flushBody();
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] flushBody() return %d",
                getLogId(), ret ));
    }
    
    if ( !ret )
        ret = getStream()->flush();
    if (ret == 0)
    {
        if ( getFlag( HSF_HANDLER_DONE 
                    | HSF_RECV_RESP_BUFFERED 
                    | HSF_SEND_RESP_BUFFERED ) == HSF_HANDLER_DONE ) 
        {
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] set HSS_COMPLETE flag.", getLogId() ));
            setState( HSS_COMPLETE );
        }
        else
        {
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] set HSF_RESP_FLUSHED flag.", getLogId() ));
            setFlag(HSF_RESP_FLUSHED, 1);
            return 0;
        }
    }
    continueWrite();
    return ret;
}



void HttpSession::addLocationHeader( ) 
{
    HttpRespHeaders &headers = m_response.getRespHeaders();
    const char * pLocation = m_request.getLocation();
    //int len = m_request.getLocationLen();
    headers.add(HttpRespHeaders::H_LOCATION, "", 0);
    if ( *pLocation == '/' )
    {
        const char * pHost = m_request.getHeader( HttpHeader::H_HOST );
        if ( isSSL() )
            headers.appendLastVal( "https://", 8 );
        else
            headers.appendLastVal( "http://", 7 );
        headers.appendLastVal( pHost, m_request.getHeaderLen( HttpHeader::H_HOST ) );
    }
    headers.appendLastVal( pLocation, m_request.getLocationLen() );
}

void HttpSession::prepareHeaders() 
{
    HttpRespHeaders &headers = m_response.getRespHeaders();
    headers.addCommonHeaders();

    if ( m_request.getAuthRequired() )
        m_request.addWWWAuthHeader( headers );
    
    if ( m_request.getLocationOff() )
        addLocationHeader();
    const AutoBuf * pExtraHeaders = m_request.getExtraHeaders();
    if ( pExtraHeaders )
    {
        headers.parseAdd( pExtraHeaders->begin(), pExtraHeaders->size(), LSI_HEADER_ADD );
    }
}


int HttpSession::sendRespHeaders()
{
    if ( isRespHeaderSent() )
        return 0;

    if ( !getFlag( HSF_RESP_HEADER_DONE ) )
    {
        if ( respHeaderDone( 0 ) )
            return 1;    
    }
    
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] sendRespHeaders()", getLogId() ));
    
    if ( sendBody() )
    {
        if ( m_sessionHooks.isEnabled( LSI_HKPT_SEND_RESP_BODY ) )
            m_response.setContentLen( LSI_RESP_BODY_SIZE_UNKNOWN );
        if ( m_response.getContentLen() >= 0 )
        {
            m_response.appendContentLenHeader();
        }
        else
        {
            setupChunkOS( 0 );
        }
    }
    prepareHeaders();

    if ( finalizeHeader( m_request.getVersion(), m_request.getStatusCode()) )
        return 1;

    getStream()->sendRespHeaders( &m_response.getRespHeaders() );
    m_iFlag |= HSF_RESP_HEADER_SENT;
    setState( HSS_WRITING );
    return 0;
}

int HttpSession::setupDynRespBodyBuf()
{
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] setupDynRespBodyBuf()", getLogId() ));
    if ( sendBody() )
    {
        if ( setupRespCache() == -1 )
            return -1;

        if ( setupGzipFilter() == -1 )
            return -1;
    }
    else
    {
        //abortReq();
        endResponseInternal( 1 );
        return 1;
    }
    return 0;
}


int HttpSession::flushDynBodyChunk()
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
int HttpSession::execExtCmd( const char * pCmd, int len )
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
// int HttpSession::writeConnStatus( char * pBuf, int bufLen )
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

int HttpSession::getServerAddrStr( char * pBuf, int len )
{
    char achAddr[128];
    socklen_t addrlen = 128;
    if ( getsockname( m_pNtwkIOLink->getfd(), (struct sockaddr *) achAddr, &addrlen ) == -1 )
    {
        return 0;
    }
    
    if (( AF_INET6 == ((struct sockaddr *)achAddr)->sa_family )&&
        ( IN6_IS_ADDR_V4MAPPED( &((struct sockaddr_in6 *)achAddr)->sin6_addr )) )
    {
        ((struct sockaddr *)achAddr)->sa_family = AF_INET;
        ((struct sockaddr_in *)achAddr)->sin_addr.s_addr = *((in_addr_t *)&achAddr[20]);
    }
    
    if ( GSockAddr::ntop( (struct sockaddr *)achAddr, pBuf, len ) == NULL )
        return 0;
    return strlen( pBuf );
}


#define STATIC_FILE_BLOCK_SIZE 16384
#include <fcntl.h>

int HttpSession::openStaticFile( const char * pPath, int pathLen, int *status )
{
    int fd;
    *status = 0;
    fd = ::open( pPath, O_RDONLY );
    if ( fd == -1 )
    {
        switch( errno )
        {
        case EACCES:
            *status = SC_403;
            break;
        case ENOENT:
            *status = SC_404;
            break;
        case EMFILE:
        case ENFILE:
            *status = SC_503;
            break;
        default:
            *status = SC_500;
            break;
        }    
    }
    else
    {
        *status = m_request.checkSymLink( pPath, pathLen, pPath );
        if ( *status )
        {
            close( fd );
            fd = -1;
        }
    }
    return fd; 
}

int HttpSession::initSendFileInfo( const char * pPath, int pathLen )
{
    int ret;
    struct stat st;
    StaticFileCacheData * &pData = m_sendFileInfo.getFileData();
    FileCacheDataEx * &pECache = m_sendFileInfo.getECache();
    if (( pData )&&( !pData->isSamePath( pPath, pathLen ) ))
    {
        releaseStaticFileCacheData(pData);
        pData = NULL;
        releaseFileCacheDataEx(pECache);
        pECache = NULL;
    }
    int fd = openStaticFile( pPath, pathLen, &ret );
    if ( fd == -1 )
        return ret;
    fstat( fd, &st );
    ret = setUpdateStaticFileCache( pData, pECache, pPath, pathLen, fd, st ); 
    if ( ret )
    {
        close( fd );
        return ret;
    }
    if ( pData->getFileData()->getfd() != -1 && fd != pData->getFileData()->getfd() )
        close( fd );
    if (pData->getFileData()->getfd() == -1)
        pData->getFileData()->setfd(fd);
    if ( pData->getFileData() == pECache )
    {
        return 0;
    }
    
    releaseFileCacheDataEx(pECache);
    pECache = NULL;
    return m_sendFileInfo.getFileData()->readyCacheData( pECache, 0 );
}

void HttpSession::setSendFileOffsetSize( off_t start, off_t size )
{
    setSendFileBeginEnd( start,  size >= 0 ? (start + size) : m_sendFileInfo.getECache()->getFileSize() );
}

void HttpSession::setSendFileBeginEnd( off_t start, off_t end )
{
    m_sendFileInfo.setCurPos( start );
    m_sendFileInfo.setCurEnd( end );
    if (( end > start) && getFlag( HSF_RESP_WAIT_FULL_BODY ) )
    {
        m_iFlag &= ~HSF_RESP_WAIT_FULL_BODY;
        if ( D_ENABLED( DL_MEDIUM ) )
           LOG_D(( getLogger(),
                  "[%s] RESP_WAIT_FULL_BODY turned off by sending static file().\n",
                 getLogId() ));
    }
    setFlag(HSF_RESP_FLUSHED, 0);
    if ( !getFlag( HSF_RESP_HEADER_DONE ) )
        respHeaderDone( 0 );    

}

int HttpSession::sendStaticFileEx(  SendFileInfo * pData )
{
    const char * pBuf;
    off_t written;
    off_t remain;
    long len;
#if !defined( NO_SENDFILE )
    int fd = pData->getECache()->getfd();
    if ( HttpServerConfig::getInstance().getUseSendfile() &&
         pData->getECache()->getfd() != -1 && !isSSL() 
         && (!getGzipBuf()||
            ( pData->getECache() == pData->getFileData()->getGziped() ) ) )
    {
        len = writeRespBodySendFile( fd, pData->getCurPos(), pData->getRemain() );
        if ( len > 0 )
        {
            pData->incCurPos( len );
        }
        return (pData->getRemain() > 0 );
    }
#endif
    while( (remain = pData->getRemain()) > 0 )
    {
        len = (remain < STATIC_FILE_BLOCK_SIZE)? remain : STATIC_FILE_BLOCK_SIZE ;
        written = remain;
        pBuf = pData->getECache()->getCacheData(
                pData->getCurPos(), written, HttpGlobals::g_achBuf, len );
        if ( written <= 0 )
        {
            return -1;
        }
        if ( getGzipBuf() )
        {
            len = appendDynBodyEx( pBuf, written );
            if ( !len )
                len = written;
        }
        else
            len = writeRespBodyDirect( pBuf, written );
        if ( len > 0 )
        {
            pData->incCurPos( len );
            if ( len < written )
                return 1;
        }
        else
            return 1;
    }
    return ( pData->getRemain() > 0 );
}


int HttpSession::sendStaticFile(  SendFileInfo * pData )
{
    if ( D_ENABLED( DL_MEDIUM ) )
        LOG_D(( getLogger(), "[%s] SendStaticFile()\n", getLogId() ));

    if ( m_sessionHooks.isDisabled( LSI_HKPT_SEND_RESP_BODY ) ||
         !(m_sessionHooks.getFlag( LSI_HKPT_SEND_RESP_BODY )& LSI_HOOK_FLAG_PROCESS_STATIC ))
        return sendStaticFileEx( pData );

    m_sessionHooks.triggered();    
    
    lsi_cb_param_t param;
    lsi_hook_info_t hookInfo;
    int buffered = 0;
    param._session = (LsiSession *)this;

    hookInfo._termination_fp = (void*)writeRespBodyTermination;
    param._hook_info = &hookInfo;

    hookInfo._hooks = m_sessionHooks.get(LSI_HKPT_SEND_RESP_BODY);

    param._flag_in  = 0;
    param._flag_out = &buffered;
    const char * pBuf;
    off_t written;
    off_t remain;
    long len;
    while( (remain = pData->getRemain()) > 0 )
    {
        len = (remain < STATIC_FILE_BLOCK_SIZE)? remain : STATIC_FILE_BLOCK_SIZE ;
        written = remain;
        pBuf = pData->getECache()->getCacheData(
                pData->getCurPos(), written, HttpGlobals::g_achBuf, len );
        if ( written <= 0 )
        {
            return -1;
        }
        param._cur_hook = (void *)hookInfo._hooks->begin();
        param._param = pBuf;
        param._param_len = written;
//         if (( written >= remain )
//             &&( (m_iFlag & (HSF_HANDLER_DONE | 
//                             HSF_RECV_RESP_BUFFERED ) == HSF_HANDLER_DONE ))
//             param._flag_in = LSI_CB_FLAG_IN_EOF;
        len = (*(((LsiApiHook *)param._cur_hook)->_cb))( &param );
        LOG_D (( "[HttpSession::sendStaticFile] sent: %d, buffered: %d", len, buffered ));

        if ( len > 0 )
        {
            pData->incCurPos( len );
            if (( len < written )|| buffered )
            {
                if ( buffered )
                    setFlag( HSF_SEND_RESP_BUFFERED, 1 );
                return 1;
            }
        }
        else
        {
            if ( len < 0 )
                return -1;
            break;
        }
    }
    return ( pData->getRemain() > 0 );
    
}

int HttpSession::finalizeHeader( int ver, int code )
{
    //setup Send Level gzip filters
    //checkAndInstallGzipFilters(1);
    
    int ret;
    getResp()->getRespHeaders().addStatusLine(ver, code, m_request.isKeepAlive());
    HttpVHost *host = (HttpVHost *)m_request.getVHost();
    if ( host )
    {
        const AutoStr2& str = host->getSpdyAdHeader();
        if( !isSSL() && str.len() > 0 )
            getResp()->appendHeader( "Alternate-Protocol", 18, str.c_str(), str.len() );
    }
    
    if ( m_sessionHooks.isEnabled( LSI_HKPT_SEND_RESP_HEADER) )
    {
        m_sessionHooks.triggered();    
        ret = m_sessionHooks.runCallbackNoParam(LSI_HKPT_SEND_RESP_HEADER, (LsiSession *)this);
        ret = processHkptResult( LSI_HKPT_SEND_RESP_HEADER, ret );
        if ( ret )
            return ret;
        
    }
    if ( sendBody() )
        return contentEncodingFixup();
    return 0;
}

int HttpSession::updateContentCompressible( )
{
    int compressible = 0;
    if ( m_request.gzipAcceptable() == GZIP_REQUIRED )
    {
        int len;
        char * pContentType = ( char *)m_response.getRespHeaders().getHeader( HttpRespHeaders::H_CONTENT_TYPE, &len );
        if ( pContentType )
        {
            char ch = pContentType[len];
            pContentType[len] = 0;
            compressible = HttpGlobals::getMime()->compressable( pContentType );
            pContentType[len] = ch;
        }
        if ( !compressible )
            m_request.andGzip( ~GZIP_ENABLED );
    }
    return compressible;
}

int HttpSession::contentEncodingFixup()
{
    int len;
    char gz = m_request.gzipAcceptable();
    int requireChunk = 0;
    const char * pContentEncoding = m_response.getRespHeaders().getHeader( HttpRespHeaders::H_CONTENT_ENCODING, &len );
    if ( !(gz & REQ_GZIP_ACCEPT) )
    {
        if ( pContentEncoding )
        {
            if ( addModgzipFilter((LsiSession *)this, 1, 0, -3000 ) == -1 )
                return -1;
            m_response.getRespHeaders().del( HttpRespHeaders::H_CONTENT_ENCODING );
            requireChunk = 1;
        }
    }
    else if ( !pContentEncoding && updateContentCompressible() )
    {
        if (( !m_request.getContextState( RESP_CONT_LEN_SET )
                || m_response.getContentLen() > 200 ) )
        {
            if ( addModgzipFilter((LsiSession *)this, 1, HttpServerConfig::getInstance().getCompressLevel(), 3000 ) == -1 )
                return -1;
            m_response.addGzipEncodingHeader();
            requireChunk = 1;
        }
    }
    if ( requireChunk )
    {
        if ( !m_pChunkOS )
        {
            setupChunkOS( 1 );
            m_response.getRespHeaders().del( HttpRespHeaders::H_CONTENT_LENGTH );
        }
    }
    return 0;
}
