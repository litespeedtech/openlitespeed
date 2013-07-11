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
#include "httpserver.h"

#include <edio/multiplexer.h>
#include <extensions/registry/extappregistry.h>
#include <extensions/extworker.h>

#include <http/accesslog.h>
#include <http/adns.h>
#include <http/clientcache.h>
#include <http/connlimitctrl.h>
#include <http/contextlist.h>
#include <http/datetime.h>
#include <http/eventdispatcher.h>
#include <http/httpcgitool.h>
#include <http/httpcontext.h>
#include <http/httpdefs.h>
#include <http/httpglobals.h>
#include <http/httplistener.h>
#include <http/httplistenerlist.h>
#include <http/httplog.h>
#include <http/httpmime.h>
#include <http/httpresp.h>
#include <http/httpresourcemanager.h>
#include <http/httpserverconfig.h>
#include <http/httpserverversion.h>
#include <http/httpvhost.h>
#include <http/httpvhostlist.h>
#include <http/httpsignals.h>
#include <http/httpsignals.h>
#include <http/staticfilecache.h>
#include <http/stderrlogger.h>
#include <http/vhostmap.h>

#include <log4cxx/logger.h>

#include <main/serverinfo.h>

#include <sslpp/sslcontext.h>
#include <sslpp/sslerror.h>
#include <util/accesscontrol.h>
#include <util/autostr.h>
#include <util/gpath.h>
#include <util/mysleep.h>
#include <util/stringtool.h>
#include <util/vmembuf.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <util/ssnprintf.h>

#define STATUS_FILE         DEFAULT_TMP_DIR "/.status"
#define FILEMODE            0644

#include "sslpp/sslocspstapling.h"


static int s_achPid[256];
static int s_curPid = 0;

static void sigchild( int sig )
{
    int status, pid;
    while( true )
    {
        pid = waitpid( -1, &status, WNOHANG|WUNTRACED );
        if ( pid <= 0 )
        {
            //if ((pid < 1)&&( errno == EINTR ))
            //    continue;
            break;
        }
        if ( s_curPid < 256 )
            s_achPid[s_curPid++] = pid;
        //PidRegistry::remove( pid );
    }
    HttpSignals::orEvent( HS_CHILD );
}


void HttpServer::cleanPid()
{
    int pid;
    sigset_t newmask, oldmask;
    ExtWorker * pWorker;
    
    sigemptyset( &newmask);
    sigaddset( &newmask, SIGCHLD );
    if ( sigprocmask( SIG_BLOCK, &newmask, &oldmask ) < 0 )
        return ;

    while( s_curPid > 0 )
    {
        pid = s_achPid[--s_curPid];
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( "Remove pid: %d", pid ));
        pWorker = PidRegistry::remove( pid );
        if ( pWorker )
            pWorker->removePid( pid );
    }
    sigprocmask( SIG_SETMASK, &oldmask, NULL );

}


class HttpServerImpl 
{
    friend class HttpServer;

private:

    // members
    HttpVHostMap        m_vhosts;
    HttpVHostMap        m_orgVHosts;
    VHostList           m_toBeReleasedVHosts;
    EventDispatcher     m_dispatcher;
    HttpListenerList    m_listeners;
    HttpListenerList    m_oldListeners;
    HttpListenerList    m_toBeReleasedListeners;
    ContextList         m_toBeReleasedContext;
    HttpContext         m_serverContext;
    AccessControl       m_accessCtrl;
    AutoStr             m_sSwapDirectory;
    AutoStr             m_sRTReportFile;
    HttpMime            m_httpMime;
    long                m_lStartTime;
    pid_t               m_pid;
    
    // interface functions
    HttpServerImpl( HttpServer * pServer )
        : m_sSwapDirectory( DEFAULT_SWAP_DIR )
        , m_sRTReportFile( DEFAULT_TMP_DIR "/.rtreport" )
    {
        ClientCache::initObjPool();
        ExtAppRegistry::init();
        HttpGlobals::setHttpMime( &m_httpMime );
        m_serverContext.allocateInternal();
        HttpResp::buildCommonHeaders();
        
#ifdef  USE_CARES
        Adns::init();
#endif
    }

    ~HttpServerImpl()
    {}

    int initAdns()
    {
#ifdef  USE_UDNS
        if ( HttpGlobals::s_adns.init() == -1 )
            return -1;
        HttpGlobals::getMultiplexer()->add( &HttpGlobals::s_adns, POLLIN|POLLHUP|POLLERR );
#endif
        return 0;
    }
    
    void releaseAll();
    int isServerOk();
    int setupSwap();
    void setSwapDir( const char * pDir );
    const char * getSwapDir() const     {   return m_sSwapDirectory.c_str();    }

    int start();
    int shutdown();
    int gracefulShutdown();
    int generateStatusReport();
    void setRTReportName( int proc );
    int generateRTReport();
    int generateProcessReport( int fd );
    HttpListener* newTcpListener( const char * pName, const char * pAddr );

    HttpListener * addListener( const char * pName, const char * pAddr );
    HttpListener * addListener( const char * pName )
    {   return addListener( pName, pName );     }

    SSLContext * newSSLContext( SSLContext * pContext, const char *pCertFile,
                                const char *pKeyFile, const char * pCAFile, const char * pCAPath,
                                const char * pCiphers, int certChain, int cv, int renegProtect );
    
    HttpListener* addSSLListener( const char * pName, const char * pAddr,
                                  SSLContext * pSSL );

    int removeListener( const char * pName );

    HttpListener* getListener( const char * pName )
    {
        return m_listeners.get( pName, NULL );
    }

    int addVHost( HttpVHost* pVHost );
    int removeVHost( const char *pName );
    HttpVHost * getVHost( const char * pName ) const
    {
        return m_vhosts.get( pName );
    }

    int enableVHost( const char * pVHostName, int enable );
    int updateVHost( const char * pVHostName, HttpVHost * pVHost );
    void updateMapping();
    void beginConfig();
    void endConfig(int error );
    void offsetChroot();

    int removeVHostFromListener( const char * pListenerName,
                            const char * pVHostName );
    int mapListenerToVHost( const char * pListenerName,
                            const char * pKey,
                            const char * pVHostName );
    int mapListenerToVHost( HttpListener * pListener,
                            const char * pKey,
                            const char * pVHostName );
    int mapListenerToVHost( HttpListener * pListener,
                            HttpVHost   * pVHost,
                            const char * pDomains );

    void onTimer();
    void onTimer60Secs();
    void onTimer30Secs();
    void onTimer10Secs();
    void onTimerSecond();

    void setBlackBoard( char * pBB );

    int  cleanUp( int pid, char * pBlackBoard );
    int  initSampleServer();
    int  reinitMultiplexer();
    int  authAdminReq( char * pAuth );

    void recycleContext( HttpContext * pContext )
    {
        pContext->setLastMod( DateTime::s_curTime );
        m_toBeReleasedContext.add( pContext, 1 );
    }
};

#include <http/userdir.h>

int HttpServerImpl::authAdminReq( char * pAuth )
{
    char * pEnd = strchr( pAuth, '\n' );
    if ( strncasecmp( pAuth, "auth:", 5 ) != 0 )
    {
        LOG_ERR(( "[ADMIN] missing authentication." ));
        return -1;
    }
    if ( !pEnd )
    {
        LOG_ERR(( "[ADMIN] missing '\n' in authentication request." ));
        return -1;
    }
    char * pUserName = pAuth + 5;
    char * pPasswd = (char *)memchr( pUserName, ':', pEnd - pUserName );
    int nameLen;
    if ( !pPasswd )
    {
        LOG_ERR(( "[ADMIN] invald authenticator." ));
        return -1;
    }
    nameLen = pPasswd - pUserName;
    *pPasswd++ = 0;
    HttpVHost * pAdmin = m_vhosts.get( DEFAULT_ADMIN_SERVER_NAME );
    if ( !pAdmin )
    {
        LOG_ERR(( "[ADMIN] missing admin vhost." ));
        return -1;
    }
    UserDir * pRealm = pAdmin->getRealm( ADMIN_USERDB );
    if ( !pRealm )
    {
        LOG_ERR(( "[ADMIN] missing authentication realm." ));
        return -1;
    }
    *pEnd = 0;
    int ret = pRealm->authenticate( NULL, pUserName, nameLen, pPasswd, ENCRYPT_PLAIN, NULL );
    if ( ret != 0 )
    {
        LOG_ERR(( "[ADMIN] authentication failed!" ));
    }
    *pEnd = '\n';
    return ret;
}


void HttpServerImpl::setSwapDir( const char * pDir )
{
    if ( pDir )
    {
        m_sSwapDirectory = pDir;
        chown( m_sSwapDirectory.c_str(), HttpGlobals::s_uid, HttpGlobals::s_gid );
    }
}



int HttpServerImpl::start()
{
    m_pid = getpid();
    DateTime::s_curTime = time( NULL );
    HttpSignals::init(sigchild);
    HttpCgiTool::buildServerEnv();
    if ( isServerOk() == -1 )
        return -1;
    LOG_NOTICE(( "[Child: %d] Setup swapping space...", m_pid ));
    if ( setupSwap() == - 1 )
    {
        LOG_ERR(( "[Child: %d] Failed to setup swapping space!", m_pid ));
        return -1;
    }
    LOG_NOTICE(( "[Child: %d] %s starts successfully!", m_pid,
                HttpServerVersion::getVersion() ));
    HttpGlobals::getConnLimitCtrl()->setListeners( &m_listeners );
    m_lStartTime = time( NULL );
    m_dispatcher.run();
    LOG_NOTICE(( "[Child: %d] Start shutting down gracefully ...", m_pid));
    gracefulShutdown();
    LOG_NOTICE(( "[Child: %d] Shut down successfully! ", m_pid));
    return 0;

}

int HttpServerImpl::shutdown()
{
    LOG_NOTICE(( "[Child: %d] Shutting down ...!", m_pid ));
    HttpSignals::setSigStop();
    return 0;
}


int HttpServerImpl::generateStatusReport()
{
    char achBuf[1024] = "";
    if ( HttpGlobals::s_psChroot )
        strcpy( achBuf, HttpGlobals::s_psChroot->c_str() );
    strcat( achBuf, STATUS_FILE );
    LOG4CXX_NS::Appender * pAppender = LOG4CXX_NS::Appender::getAppender( achBuf );
    pAppender->setAppendMode( 0 );
    if ( pAppender->open() )
    {
        LOG_ERR(( "Failed to open the status report: %s!", achBuf ));
        return -1;
    }
    int ret = m_listeners.writeStatusReport( pAppender->getfd() );
    if ( !ret )
        ret = m_vhosts.writeStatusReport( pAppender->getfd() );
    if ( ret )
    {
        LOG_ERR(( "Failed to generate the status report!" ));
    }
    pAppender->append( "EOF\n", 4 );
    pAppender->close();
    return 0;
}

int generateConnReport( int fd  )
{
    ConnLimitCtrl * pCtrl = HttpGlobals::getConnLimitCtrl();
    char achBuf[4096];
    int SSLConns = pCtrl->getMaxSSLConns() - pCtrl->availSSLConn();
    HttpGlobals::s_reqStats.finalizeRpt();
    if ( pCtrl->getMaxConns() - pCtrl->availConn() - HttpGlobals::s_iIdleConns < 0 )
    {
        HttpGlobals::s_iIdleConns = pCtrl->getMaxConns() - pCtrl->availConn();
    }
    int n = safe_snprintf( achBuf, 4096,
                "BPS_IN: %ld, BPS_OUT: %ld, "
                "SSL_BPS_IN: %ld, SSL_BPS_OUT: %ld\n"
                "MAXCONN: %d, MAXSSL_CONN: %d, PLAINCONN: %d, "
                "AVAILCONN: %d, IDLECONN: %d, SSLCONN: %d, AVAILSSL: %d\n"
                "REQ_RATE []: REQ_PROCESSING: %d, REQ_PER_SEC: %d, TOT_REQS: %d\n",
            HttpGlobals::s_lBytesRead/1024, HttpGlobals::s_lBytesWritten/1024,
            HttpGlobals::s_lSSLBytesRead/1024, HttpGlobals::s_lSSLBytesWritten/1024,
            pCtrl->getMaxConns(), pCtrl->getMaxSSLConns(),
            pCtrl->getMaxConns() - SSLConns - pCtrl->availConn(),
            pCtrl->availConn(), HttpGlobals::s_iIdleConns,
            SSLConns, pCtrl->availSSLConn(),
            pCtrl->getMaxConns() - pCtrl->availConn() - HttpGlobals::s_iIdleConns,
            HttpGlobals::s_reqStats.getRPS(), HttpGlobals::s_reqStats.getTotal() );
    write( fd, achBuf, n );

    HttpGlobals::s_lBytesRead = HttpGlobals::s_lBytesWritten
        = HttpGlobals::s_lSSLBytesRead = HttpGlobals::s_lSSLBytesWritten = 0;
    HttpGlobals::s_reqStats.reset();
    return 0;
}

int HttpServerImpl::generateProcessReport( int fd )
{
    char achBuf[4096];
    long delta = time(NULL) - m_lStartTime;
    long days;
    int hours, mins, seconds;
    seconds = delta % 60;
    delta /= 60;
    mins = delta % 60;
    delta /= 60;
    hours = delta % 24;
    days = delta / 24;
    char * p = achBuf;
    p += safe_snprintf( p, &achBuf[4096] - p, "VERSION: LiteSpeed Web Server/%s/%s\n",
                    "Open",
                    PACKAGE_VERSION  );
    p += safe_snprintf( p, &achBuf[4096] - p, "UPTIME:" );
    
    if ( days )
        p += safe_snprintf( p, &achBuf[4096] - p, " %ld day%s", days, ( days > 1 )?"s":"" );
    p += safe_snprintf( p, &achBuf[4096] - p, " %02d:%02d:%02d\n", hours, mins, seconds );
    write( fd, achBuf, p - achBuf );
    return 0;
    
}

void HttpServerImpl::setRTReportName( int proc )
{
    if ( proc != 1 )
    {
        char achBuf[256];
        safe_snprintf( achBuf, 256, "%s.%d", DEFAULT_TMP_DIR "/.rtreport", proc );
        m_sRTReportFile.setStr( achBuf );
    }
}


int HttpServerImpl::generateRTReport()
{
    LOG4CXX_NS::Appender * pAppender = LOG4CXX_NS::Appender::getAppender(
                        m_sRTReportFile.c_str() );
    pAppender->setAppendMode( 0 );
    if ( pAppender->open() )
    {
        LOG_ERR(( "Failed to open the real time report!" ));
        return -1;
    }
    int ret;
    ret = generateProcessReport( pAppender->getfd() );
    ret = generateConnReport( pAppender->getfd() );
    ret = m_listeners.writeRTReport( pAppender->getfd() );
    if ( !ret )
        ret = m_vhosts.writeRTReport( pAppender->getfd() );
    if ( ret )
    {
        LOG_ERR(( "Failed to generate the real time report!" ));
    }
    ret = ExtAppRegistry::generateRTReport( pAppender->getfd() );
    ret = HttpGlobals::getClientCache()->generateBlockedIPReport( pAppender->getfd() );
    
    pAppender->append( "EOF\n", 4 );
    pAppender->close();
    return 0;
}


HttpListener* HttpServerImpl::newTcpListener( const char * pName, const char * pAddr )
{
    HttpListener* pListener = new HttpListener( pName, pAddr );
    if ( pListener == NULL )
        return pListener;
    int ret;
    for( int i = 0; i < 3; ++i )
    {
        ret = pListener->start();
        if ( !ret )
            return pListener;
        if ( errno == EACCES )
            break;
        my_sleep( 100 );
    }
    LOG_ERR(( "HttpListener::start(): Can't listen at address %s: %s!", pName, ::strerror( ret ) ));
    //LOG_ERR(( "Unable to add listener [%s]!", pName ));
    delete pListener;
    return NULL;
}


HttpListener * HttpServerImpl::addListener( const char * pName, const char * pAddr )
{
    HttpListener* pListener = NULL;
    if ( !pName )
        return NULL;
    pListener = m_listeners.get( pName, pAddr );
    if ( pListener )
        return pListener;
    pListener = m_oldListeners.get( pName, pAddr );
    if ( !pListener )
    {
        pListener = newTcpListener( pName, pAddr );
        if ( pListener )
        {
            if ( D_ENABLED( DL_LESS ) )
                LOG_D(( "Created new Listener [%s].", pName ));
        }
        else
        {
            LOG_ERR(( "HttpServer::addListener(%s) failed to create new listener"
                    , pName ));
            return NULL;
        }
    }
    else
    {
        pListener->beginConfig();
        m_oldListeners.remove( pListener );
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( "Reuse current listener [%s].", pName ));
    }
    m_listeners.add( pListener );
    return pListener;
}



HttpListener* HttpServerImpl::addSSLListener( const char * pName, const char * pAddr, SSLContext * pSSL )

{
    HttpListener* pListener = NULL;
    if ( !pSSL )
        return NULL;
    pListener = addListener( pName, pAddr );
    if ( !pListener )
        return NULL;
    pListener->getVHostMap()->setSSLContext( pSSL );
    if ( pSSL->initSNI( pListener->getVHostMap() ) == -1 )
    {
        LOG_WARN(( "[SSL] TLS extension is not available in openssl library on this server, "
                    "server name indication is disabled, you will not able to use use per vhost"
                    " SSL certificates sharing one IP. Please upgrade your OpenSSL lib if you want to use this feature."
            ));
        
    }
   
    return pListener;
}

int HttpServerImpl::removeListener( const char * pName )
{
    HttpListener* pListener = m_listeners.get( pName, NULL );
    if ( pListener != NULL )
    {
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( "Removed listener [%s].", pName ));
        m_listeners.remove( pListener );
        pListener->stop();
        if ( pListener->getVHostMap()->getRef() > 0 )
        {
            m_toBeReleasedListeners.add( pListener );
        }
        else
            delete pListener;
    }
    return 0;
}

int HttpServerImpl::addVHost( HttpVHost* pVHost )
{
    assert( pVHost );
//    if ( m_vhosts.size() > MAX_VHOSTS )
//    {
//        return -1;
//    }
    int ret = m_vhosts.add( pVHost );
    return ret;
}

int HttpServerImpl::updateVHost( const char * pVHostName, HttpVHost * pVHost )
{
    HttpVHost * pOld = getVHost( pVHostName );
    if ( pOld )
    {
        m_vhosts.remove( pOld );
        pVHost->enable( pOld->isEnabled() );
        m_listeners.removeVHostMappings( pOld );
        assert( pOld->getMappingRef() == 0 );
        if ( pOld->getRef() > 0 )
        {
            pOld->getAccessLog()->setAsyncAccessLog( 0 );
            m_toBeReleasedVHosts.push_back( pOld );
        }
        else
            delete pOld;
    }
    return addVHost( pVHost );
    
}

int HttpServerImpl::removeVHost( const char *pName )
{
    HttpVHost * pVHost = getVHost( pName );
    if ( pVHost != NULL )
    {
        int ret = m_vhosts.remove( pVHost );
        if ( ret )
        {
            LOG_ERR(( "HttpServer::removeVHost(): virtual host %s failed! ",
                    pName ));
        }
        else
        {
            if ( D_ENABLED( DL_LESS ) )
                LOG_D(( "Removed virtual host %s!",
                    pName ));
        }
        return ret;
    }
    else
        return 0;
}




int HttpServerImpl::mapListenerToVHost( HttpListener * pListener,
                        HttpVHost   * pVHost,
                        const char * pDomains )
{
    if (!pListener)
        return -1;
    return pListener->getVHostMap()->mapDomainList( pVHost, pDomains );
}

int HttpServerImpl::mapListenerToVHost( HttpListener * pListener,
                        const char * pKey,
                        const char * pVHostName )
{
    HttpVHost * pVHost = getVHost( pVHostName );
    return mapListenerToVHost( pListener, pVHost, pKey );
}

int HttpServerImpl::mapListenerToVHost( const char * pListenerName,
                        const char * pKey,
                        const char * pVHostName )
{
    HttpListener * pListener = getListener( pListenerName );
    return mapListenerToVHost( pListener, pKey, pVHostName );
}

int HttpServerImpl::removeVHostFromListener( const char * pListenerName,
                        const char * pVHostName )
{
    HttpListener * pListener = getListener( pListenerName );
    HttpVHost * pVHost = getVHost( pVHostName );
    if (( pListener == NULL )||( pVHost == NULL ))
        return EINVAL;
    int ret = pListener->getVHostMap()->removeVHost( pVHost );
    if ( ret )
    {
        LOG_ERR(( "Deassociates [%s] with [%s] on hostname/IP [%s] %s!",
               pVHostName, pListenerName, "failed"));
    }
    else
    {
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( "Deassociates [%s] with [%s] on hostname/IP [%s] %s!",
                pVHostName, pListenerName, "succeed"));
    }
    return ret;
}

void HttpServerImpl::beginConfig()
{
    assert( m_oldListeners.size() == 0 );
    m_oldListeners.swap( m_listeners );
    m_vhosts.swap( m_orgVHosts );
    HttpVHost * pVHost = m_orgVHosts.get( DEFAULT_ADMIN_SERVER_NAME );
    if ( pVHost )
    {
        m_orgVHosts.remove( pVHost );
        m_vhosts.add( pVHost );
    }
    //HttpGlobals::getClientCache()->dirtyAll();
}


void HttpServerImpl::updateMapping()
{
    int n = m_listeners.size();
    for( int i = 0; i < n; ++i )
    {
        m_listeners[i]->getVHostMap()->updateMapping( m_vhosts );
    }
}

void HttpServerImpl::endConfig(int error )
{
    if ( error )
    {
        m_vhosts.moveNonExist( m_orgVHosts );
        m_listeners.moveNonExist( m_oldListeners );
        updateMapping();
    }
    m_oldListeners.saveInUseListnersTo( m_toBeReleasedListeners );
    m_orgVHosts.appendTo( m_toBeReleasedVHosts );
    m_listeners.endConfig();
}


int HttpServerImpl::enableVHost( const char * pVHostName, int enable )
{
    HttpVHost * pVHost = getVHost( pVHostName );
    if ( pVHost )
    {
        pVHost->enable( enable );
        return 0;
    }
    return -1;
}



void HttpServerImpl::onTimerSecond()
{
    HttpResp::updateDateHeader();
    HttpLog::onTimer();
    HttpGlobals::getClientCache()->onTimer();
    m_vhosts.onTimer();
    if ( m_lStartTime > 0 )
        generateRTReport();
}

void HttpServerImpl::onTimer10Secs()
{
    ExtAppRegistry::onTimer();
    HttpGlobals::getResManager()->onTimer();
    static int s_timeOut = 3;
    s_timeOut --;
    if ( !s_timeOut )
    {
        s_timeOut = 3;
        onTimer30Secs();
    }
}

void HttpServerImpl::onTimer30Secs()
{
    HttpGlobals::getClientCache()->onTimer30Secs();
    HttpGlobals::getStaticFileCache()->onTimer();
    m_vhosts.onTimer30Secs();
    static int s_timeOut = 2;
    s_timeOut --;
    if ( !s_timeOut )
    {
        s_timeOut = 2;
        onTimer60Secs();
    }
    if ( HttpGlobals::s_503AutoFix &&( HttpGlobals::s_503Errors >= 30) )
    {
        LOG_NOTICE(( "There are %d '503 Errors' in last 30 seconds, "
                     "request a graceful restart", HttpGlobals::s_503Errors ));
        HttpGlobals::getServerInfo()->setRestart( 1 );
        
    }
    HttpGlobals::s_503Errors = 0;
}

void HttpServerImpl::onTimer60Secs()
{

    m_toBeReleasedListeners.releaseUnused();
    m_toBeReleasedVHosts.releaseUnused();
    m_toBeReleasedContext.releaseUnused( DateTime::s_curTime,
                HttpServerConfig::getInstance().getConnTimeout() );

}

void HttpServerImpl::onTimer()
{
    onTimerSecond();
    static int s_timeOut = 10;
    s_timeOut --;
    if ( !s_timeOut )
    {
        s_timeOut = 10;
        onTimer10Secs();

    }
}




void HttpServerImpl::offsetChroot()
{
    
    char achTemp[512];
    strcpy( achTemp, HttpGlobals::getStdErrLogger()->getLogFileName() );
    HttpGlobals::getStdErrLogger()->
            setLogFileName( achTemp + HttpGlobals::s_psChroot->len());
    HttpLog::offsetChroot( HttpGlobals::s_psChroot->c_str(), HttpGlobals::s_psChroot->len() );
    HttpGlobals::getServerInfo()->m_pChroot =
            HttpGlobals::getServerInfo()->dupStr( HttpGlobals::s_psChroot->c_str(),
                    HttpGlobals::s_psChroot->len() );
    m_vhosts.offsetChroot( HttpGlobals::s_psChroot->c_str(), HttpGlobals::s_psChroot->len() );
}

void HttpServerImpl::releaseAll()
{
    ExtAppRegistry::stopAll();
    HttpGlobals::getStaticFileCache()->releaseAll();
    m_listeners.clear();
    m_oldListeners.clear();
    m_toBeReleasedListeners.clear();
    m_vhosts.release_objects();
    m_toBeReleasedVHosts.release_objects();
    ::signal(SIGCHLD, SIG_DFL);
    ExtAppRegistry::shutdown();
    ClientCache::clearObjPool();
    HttpGlobals::getResManager()->releaseAll();
    HttpMime::releaseMIMEList();
}

int HttpServerImpl::isServerOk()
{
    if ( m_listeners.size() == 0 )
    {
        LOG_ERR(( "No listener is available, stop server!" ));
        return -1;
    }
    if ( m_vhosts.size() == 0 )
    {
        LOG_ERR(( "No virtual host is available, stop server!" ));
        return -1;
    }
    return 0;
}

int removeMatchFile( const char * pDir, const char * prefix )
{
    DIR *d;
    struct dirent *entry;
    int i = 0;
    int prefixLen = strlen( prefix );
    char achTemp[512];
    memccpy( achTemp, pDir, 0, 510 );
    achTemp[511] = 0;
    int dirLen = strlen( achTemp );
    if ( achTemp[dirLen-1] != '/' )
    {
        achTemp[dirLen++] = '/';
        achTemp[dirLen] = 0;
    }
    
    if ((d=opendir(achTemp)) == NULL)
        return -1;

    while ( (entry=readdir(d)) != NULL )
    {
        if ( !prefixLen || strncmp( entry->d_name, prefix, prefixLen ) == 0 )
        {
            memccpy( &achTemp[dirLen], entry->d_name, 0, 510 - dirLen );
            unlink( achTemp );
            ++i;
        }
    }
    if (closedir(d))
        return -1;

    return i ;
}

int HttpServerImpl::gracefulShutdown()
{
    //suspend listener socket, 
    //m_listeners.suspendAll();
    //close all listener socket.
    m_lStartTime = -1;
    m_listeners.stopAll();
    //close keepalive connections
    HttpServerConfig::getInstance().setMaxKeepAliveRequests(0);
    HttpServerConfig::getInstance().setKeepAliveTimeout(0);
    HttpServerConfig::getInstance().setConnTimeOut( 15 );
    HttpGlobals::s_tmPrevToken = TIMER_PRECISION-1;
    HttpGlobals::s_tmToken = 0;
    HttpGlobals::getMultiplexer()->timerExecute();  //close keepalive connections
    // change to lower priority
    nice( 3 );
    //linger for a while
    return m_dispatcher.linger( HttpServerConfig::getInstance().getConnTimeout() * 2 );
        
}

int HttpServerImpl::reinitMultiplexer()
{
    if ( HttpGlobals::s_iMultiplexerType )
    {
        if ( m_dispatcher.reinit() )
            return -1;
        HttpGlobals::getMultiplexer()->add(
                HttpGlobals::getStdErrLogger(), POLLIN|POLLHUP|POLLERR );
    }
    int n = m_listeners.size();
    for( int i = 0; i < n; ++i )
    {
        if ( !(m_listeners[i]->getBinding() & ( 1L << (HttpGlobals::s_iProcNo-1) )))
            m_listeners[i]->stop();
        else if ( HttpGlobals::s_iMultiplexerType )
            HttpGlobals::getMultiplexer()->add( m_listeners[i], POLLIN|POLLHUP|POLLERR );
    }
    initAdns();
    return 0;
}


int HttpServerImpl::setupSwap()
{
    char achDir[512];
    strcpy( achDir, getSwapDir() );
    if ( *(strlen( achDir ) - 1 + achDir) != '/' )
        strcat( achDir, "/" );
    if ( !GPath::isValid( achDir ) )
    {
        char * p = strchr( achDir+1, '/' );
        while( p )
        {
            *p = 0;
            mkdir( achDir, 0700 );
            *p++ = '/';
            p = strchr( p, '/' );
        }
    }
    if ( !GPath::isWritable( achDir ) )
    {
        LOG_WARN(( "Specified swapping directory is not writable:%s,"
                    " use default!", achDir ));
        strcpy( achDir, DEFAULT_SWAP_DIR );
        if ( *(strlen( achDir ) - 1 + achDir) != '/' )
            strcat( achDir, "/" );
        mkdir( achDir, 0700 );
    }
    if ( !GPath::isWritable( achDir ) )
    {
        LOG_ERR(( "Swapping directory is not writable:%s", achDir ));
        fprintf ( stderr, "Swapping directory is not writable:%s", achDir );
        return -1;
    }
    if ( HttpGlobals::s_iProcNo != 1 )
    {
        safe_snprintf( achDir + strlen( achDir ), 256 - strlen( achDir ),
            "s%d/", HttpGlobals::s_iProcNo );
        mkdir( achDir, 0700 );
    }
    if ( (strncmp( achDir, DEFAULT_SWAP_DIR,
            strlen(DEFAULT_SWAP_DIR) ) == 0 )||( getuid() != 0 ))
    {
        removeMatchFile( achDir, "" );
    }
    strcat( achDir, "s-XXXXXX" );
    VMemBuf::setTempFileTemplate( achDir );
    return 0;

}
#include <new>
void HttpServerImpl::setBlackBoard( char * pBuf)
{
    ServerInfo * pInfo = new (pBuf) ServerInfo(
                    pBuf + ( sizeof( ServerInfo ) + 15 ) / 16 * 16,
                    pBuf + 32760 );
    if ( pInfo )
    {
        PidRegistry::setSimpleList( &(pInfo->m_pidList) );
        HttpGlobals::setServerInfo( pInfo );
    }
}





#ifdef RUN_TEST


#include <extensions/cgi/cgidworker.h>
#include <extensions/fcgi/fcgiapp.h>
#include <extensions/fcgi/fcgiappconfig.h>
#include <extensions/jk/jworker.h>
#include <extensions/jk/jworkerconfig.h>
#include <http/handlertype.h>
#include <http/htauth.h>
#include <http/staticfilecachedata.h>
#include <util/sysinfo/systeminfo.h>


int HttpServerImpl::initSampleServer()
{
    beginConfig();
    HttpServerConfig & serverConfig = HttpServerConfig::getInstance();
    char achBuf[256], achBuf1[256];
    char * p = getcwd( achBuf, 256 );
    strcat( achBuf, "/serverroot" );
    assert( p != NULL );
    char * pEnd = p + strlen( p );
    strcpy( achBuf1, achBuf );

    m_dispatcher.init( "poll" );
    
    serverConfig.setConnTimeOut( 3000000 );
    serverConfig.setKeepAliveTimeout( 15 );
    serverConfig.setSmartKeepAlive( 0 );
    serverConfig.setFollowSymLink( 1 );
    serverConfig.setMaxKeepAliveRequests( 10000 );

    ThrottleLimits * pLimit = ThrottleControl::getDefault();
    pLimit->setDynReqLimit( INT_MAX );
    pLimit->setStaticReqLimit( INT_MAX );
    pLimit->setInputLimit( INT_MAX );
    pLimit->setOutputLimit( INT_MAX );

    serverConfig.setDebugLevel( 10 );
    HttpGlobals::s_iConnsPerClientSoftLimit = 1000;
    serverConfig.setGzipCompress( 1 );
    serverConfig.setDynGzipCompress( 1 );
    serverConfig.setMaxURLLen( 8192 );
    serverConfig.setMaxHeaderBufLen( 16380 );
    serverConfig.setMaxDynRespLen( 1024 * 256 );
    HttpGlobals::getConnLimitCtrl()->setMaxConns( 1000 );
    HttpGlobals::getConnLimitCtrl()->setMaxSSLConns( 1 );
    //enableScript( 1 );
    setSwapDir( "/tmp/lshttpd1/swap" );

    m_accessCtrl.addSubNetControl("192.168.2.0", "255.255.255.0", false );
    HttpGlobals::setAccessControl( &m_accessCtrl );
    //strcpy( pEnd, "/logs/error.log" );
    //setErrorLogFile( achBuf );
    strcpy( pEnd, "/logs/access.log" );
    HttpLog::setAccessLogFile( achBuf, 0 );
    strcpy( pEnd, "/logs/stderr.log" );
    HttpGlobals::getStdErrLogger()->setLogFileName( achBuf );
    HttpLog::setLogPattern( "%d [%p] %m" );
    HttpLog::setLogLevel( "DEBUG" );
    strcat( achBuf1, "/cert/server.crt" );
    strcpy( pEnd, "/cert/server.pem" );
    if ( addListener( "*:3080" ) == 0 )
    {
    }
    SSLContext * pSSL = newSSLContext( NULL, achBuf1, achBuf, NULL, NULL,
                 "ALL:!ADH:!EXPORT56:RC4+RSA:+HIGH:+MEDIUM:+SSLv2:+EXP" , 0, 0, 0 );
    addSSLListener( "*:1443", "*:1443", pSSL );
                 

    HttpVHost * pVHost = new HttpVHost( "vhost1" );
    HttpVHost * pVHost2 = new HttpVHost( "vhost2" );
    assert( pVHost != NULL );
    pVHost->getRootContext().setParent(
        &HttpServer::getInstance().getServerContext() );
    pVHost2->getRootContext().setParent(
        &HttpServer::getInstance().getServerContext() );
    strcpy( pEnd, "/logs/vhost1.log" );
    pVHost->setErrorLogFile( achBuf );
    pVHost->setErrorLogRollingSize( 8192, 10 );
    pVHost->setLogLevel( "DEBUG" );
    //strcpy( pEnd, "/logs/vhost1_access.log" );
    //pVHost->setAccessLogFile( achBuf );

    pVHost->enableAccessCtrl();
    pVHost->getAccessCtrl()->addSubNetControl("192.168.1.0", "255.255.255.0", false );

    pVHost->getRootContext().addDirIndexes(
            "index.htm,index.php,index.html,default.html" );
    pVHost->setSmartKA( 0 );
    pVHost->setMaxKAReqs( 100 );

    pLimit = pVHost->getThrottleLimits();
    pLimit->setDynReqLimit( 2 );
    pLimit->setStaticReqLimit( 20 );

    //load Http mime types
    strcpy( pEnd, "/conf/mime.properties" );
    if ( HttpGlobals::getMime()->loadMime( achBuf ) == 0 )
    {
    }
    char achMIMEHtml[] = "text/html";
    HttpGlobals::getMime()->updateMIME( achMIMEHtml,
                HttpMime::setCompressable, (void *)1, NULL );
    StaticFileCacheData::setUpdateStaticGzipFile( 1, 6, 300, 1024 * 1024 );
    
    //protected context
    strcpy( pEnd, "/wwwroot/protected/" );
    HttpContext* pContext = pVHost->addContext( "/protected/",
                        HandlerType::HT_NULL, achBuf, NULL, true );


                        
    //FIXME: problem to check, space charactor in realm string
    strcpy( pEnd, "/htpasswd" );
    strcpy( &achBuf1[pEnd - achBuf], "/htgroup" );
    UserDir * pDir
        = pVHost->getFileUserDir( "RistrictedArea", achBuf, achBuf1);

    HTAuth * pAccess = new HTAuth();
    pContext->setAuthRequired( "user test" );
    pAccess->setUserDir( pDir );
    pAccess->setName( pDir->getName() );
    pContext->setHTAuth( pAccess );

    AccessControl * pControl = new AccessControl();
    pControl->addSubNetControl("192.168.0.0", "255.255.255.0", false );
    pControl->addIPControl("192.168.0.10", true );
    pContext->setAccessControl( pControl );

    //FileMatch Context
    pContext = new HttpContext();
    if ( pContext )
    {
        pContext->setFilesMatch( ".ht*", 0 );
        pContext->addAccessRule( "*", 0 );
        m_serverContext.addFilesMatchContext( pContext );
    }    
        
    strcpy( pEnd, "/wwwroot/" );
    pContext =  pVHost->addContext( "/", HandlerType::HT_NULL, achBuf, NULL, true );

    strcpy( pEnd, "/wwwroot/phpinfo.php$3?username=$1" );
    HttpContext * pMatchContext = new HttpContext();
    pVHost->setContext( pMatchContext, "exp: ^/~(([a-z])[a-z0-9]+)(.*)",
                        HandlerType::HT_NULL, achBuf, NULL, true );
    pContext->addMatchContext( pMatchContext );

    pMatchContext = new HttpContext();
    pVHost->setContext(  pMatchContext, "exp: ^/!(.*)", HandlerType::HT_REDIRECT,
                        "/phpinfo.php/path/info?username=$1", NULL, true );
    pMatchContext->redirectCode( SC_302 );
    pContext->addMatchContext( pMatchContext );

    pContext = pVHost->addContext( "/redirauth/",
                HandlerType::HT_REDIRECT, "/private/", NULL , false );

    pContext = pVHost->addContext( "/redirect/",
                HandlerType::HT_REDIRECT, "/test", NULL, false );

    pContext = pVHost->addContext( "/redirabs/", HandlerType::HT_REDIRECT,
                "http:/index.html", NULL, false );
    pContext->redirectCode( SC_301 );

    pContext = pVHost->addContext( "/redircgi/", HandlerType::HT_REDIRECT,
                                    "/cgi-bin/", NULL, false );

    pContext = pVHost->addContext( "/denied/", HandlerType::HT_NULL,
                                    "denied/", NULL, false );


    //CGI context
    strcpy( pEnd, "/cgi-bin/" );
    pContext = pVHost->addContext( "/cgi-bin/", HandlerType::HT_CGI,
                                    achBuf, "lscgid", true );

    //servlet engines
    JWorker * pSE = (JWorker *)ExtAppRegistry::addApp( EA_JENGINE, "tomcat1" );
    assert( pSE );
    pSE->setURL( "localhost:8009" );
    pSE->getConfig().setMaxConns( 10 );

//    m_builder.importWebApp( pVHost, "/examples/",
//        "/opt/tomcat/webapps/examples/", "tomcat1", true );


    ExtWorker * pProxy = ExtAppRegistry::addApp( EA_PROXY, "proxy1" );
    assert( pProxy );
    pProxy->setURL( "192.168.0.10:5080" );
    pProxy->getConfigPointer()->setMaxConns( 10 );

    //Fast cgi application
    strcpy( pEnd, "/fcgi-bin/lt-echo-cpp" );
    //strcpy( pEnd, "/fcgi-bin/echo" );
    //FcgiApp * pFcgiApp = addFcgiApp( "localhost:5558" );
    FcgiApp * pFcgiApp = (FcgiApp *)ExtAppRegistry::addApp(
                    EA_FCGI, "localhost:5558" );
    assert( pFcgiApp != NULL );
    pFcgiApp->setURL( "UDS://tmp/echo" );
    pFcgiApp->getConfig().setAppPath( achBuf );
    pFcgiApp->getConfig().setBackLog( 20 );
    pFcgiApp->getConfig().setInstances( 3 );
    pFcgiApp->getConfig().setMaxConns( 3 );


    strcpy( pEnd, "/fcgi-bin/logger.pl" );
    pFcgiApp = (FcgiApp *)ExtAppRegistry::addApp(
                    EA_LOGGER, "logger" );
    assert( pFcgiApp );
    //pFcgiApp.setURL( "localhost:5556" );
    pFcgiApp->setURL( "UDS://tmp/logger.sock" );
    pFcgiApp->getConfig().setAppPath( achBuf );
    pFcgiApp->getConfig().setBackLog( 20 );
    pFcgiApp->getConfig().setMaxConns( 10 );
    pFcgiApp->getConfig().setInstances( 10 );
    pVHost->setAccessLogFile( "logger", 1 );
    pVHost->getAccessLog()->setLogHeaders(  LOG_REFERER | LOG_USERAGENT );
    pFcgiApp->getConfig().setVHost( pVHost );
        
    strcpy( pEnd, "/fcgi-bin/php" );
    pFcgiApp = (FcgiApp *)ExtAppRegistry::addApp(
                    EA_FCGI, "php-fcgi" );
    assert( pFcgiApp );
    //pFcgiApp.setURL( "localhost:5556" );
    pFcgiApp->setURL( "UDS://tmp/php.sock" );
    pFcgiApp->getConfig().setAppPath( achBuf );
    pFcgiApp->getConfig().setBackLog( 20 );
    pFcgiApp->getConfig().setMaxConns( 20 );
    pFcgiApp->getConfig().addEnv( "FCGI_WEB_SERVER_ADDRS=127.0.0.1" );
    pFcgiApp->getConfig().addEnv( "PHP_FCGI_CHILDREN=2" );
    pFcgiApp->getConfig().addEnv( "PHP_FCGI_MAX_REQUESTS=1000" );

    //echo Fast CGI context
    pContext = pVHost->addContext( "/fcgi-bin/echo", HandlerType::HT_FASTCGI,
                                    NULL, "localhost:5558", false );
    //setContext( pContext, "/", HandlerType::HT_FASTCGI, NULL, "localhost:5558", false );


    pContext = pVHost->addContext( "/fcgi-bin/php", HandlerType::HT_FASTCGI,
                                    NULL, "localhost:5556", false );

    pContext = pVHost->addContext( "/proxy/", HandlerType::HT_PROXY,
                                    NULL, "proxy1", false );
    //pContext->allowOverride( 0 );
    //pContext->setConfigBit( BIT_ALLOW_OVERRIDE, 1 );

//    ScriptHandlerMap& map = pVHost->getScriptHandlerMap();
    pVHost->getRootContext().initMIME();
    HttpMime * pMime = pVHost->getMIME();
    HttpGlobals::getMime()->addMimeHandler( "php", NULL, pFcgiApp, NULL, "" );
    //pMime->addMimeHandler( "cgi", NULL, HandlerType::HT_CGI, NULL, NULL, "" );
    pMime->addMimeHandler( "jsp", NULL, pSE, NULL, "" );
    pVHost->enableScript( 1 );

    pContext = new HttpContext();
    if ( pContext )
    {
        char achType[] = "application/x-httpd-php";
        pContext->setFilesMatch( "phpinfo", 0 );
        pContext->setForceType( achType, "" );
        m_serverContext.addFilesMatchContext( pContext );
    }
    
    //pVHost->setCustomErrUrls( 404, "/baderror404.html" );
    //pVHost->setCustomErrUrls( 403, "/cgi-bin/error403" );

//    CgidWorker * pWorker = (CgidWorker *)ExtAppRegistry::addApp(
//                        EA_CGID, LSCGID_NAME );
//    CgidWorker::start("/home/gwang/lsws/", NULL, getuid(), getgid(),
//                        getpriority(PRIO_PROCESS, 0));

    strcpy( pEnd, "/wwwroot/" );
    //printf( "WWW root = %s\n", achBuf );
    pVHost->setDocRoot( achBuf );
    if ( addVHost( pVHost ) == 0 )
    {
    }
    if ( addVHost( pVHost2 ) == 0 )
    {
    }
    if ( mapListenerToVHost( "*:3080","*", "vhost1" ) == 0 )
    {
    }
    if ( mapListenerToVHost( "*:3080","test.com", "vhost2" )  )
    {
    }
    if ( mapListenerToVHost( "*:1443","*", "vhost1" ) == 0 )
    {
    }
    pVHost->getRootContext().inherit( NULL );
    pVHost->contextInherit();
    endConfig(0);

    strcpy( pEnd, "/conf/httpd.conf" );
    return 0;
}

#include <httpdtest.h>
#include <http/httpconnection.h>
#include <util/httpfetch.h>
int HttpServer::test_main( )
{
    printf( "sizeof( HttpConnection ) = %d, \n"
            "sizeof( HttpReq ) = %d, \n"
            "sizeof( HttpResp ) = %d, \n"
            "sizeof( NtwkIOLink ) = %d, \n"
            "sizeof( HttpVHost ) = %d, \n"
            "sizeof( LogTracker ) = %d, \n",
            sizeof( HttpConnection ),
            sizeof( HttpReq ),
            sizeof( HttpResp ),
            sizeof( NtwkIOLink ),
            sizeof( HttpVHost ),
            sizeof( LogTracker ) );
    HttpFetch fetch;
    HttpGlobals::s_pServerRoot="/home/gwang/lsws/";
    
//    if ( fetch.startReq( "http://www.litespeedtech.com/index.html", 0, "lst_index.html" ) == 0 )
//        fetch.process();
    
    HttpServerConfig::getInstance().setGzipCompress( 1 );
    HttpdTest::runTest();
    SystemInfo::maxOpenFile(2048);
    m_impl->initSampleServer();
    start();
    releaseAll();
    return 0;
}
#endif




/////////////////////////////////////////////////////////////////
//      Interface function
/////////////////////////////////////////////////////////////////

HttpServer::HttpServer()
{
    m_impl = new HttpServerImpl( this );
}

HttpServer::~HttpServer()
{
//    if ( m_impl )
//        delete m_impl;
}

HttpListener* HttpServer::addListener( const char * pName, const char * pAddr )
{
    return m_impl->addListener( pName, pAddr );
}

HttpListener* HttpServer::addSSLListener( const char * pName,
            const char * pAddr, SSLContext * pCtx )
{
    return m_impl->addSSLListener( pName, pAddr, pCtx );
}

int HttpServer::removeListener( const char * pName )
{
    return m_impl->removeListener( pName );
}

HttpListener* HttpServer::getListener( const char * pName ) const
{
    return m_impl->getListener( pName );
}

int HttpServer::addVHost( HttpVHost* pVHost )
{
    return m_impl->addVHost( pVHost );
}

int HttpServer::removeVHost( const char *pName )
{
    return m_impl->removeVHost( pName );
}

HttpVHost * HttpServer::getVHost( const char * pName ) const
{
    return m_impl->getVHost( pName );
}

int HttpServer::mapListenerToVHost( HttpListener * pListener,
                            const char * pKey,
                            const char * pVHostName )
{
    return m_impl->mapListenerToVHost( pListener, pKey, pVHostName );
}

int HttpServer::mapListenerToVHost( const char * pListener,
                        const char * pKey,
                        const char * pVHost )
{
    return m_impl->mapListenerToVHost( pListener, pKey, pVHost );
}

int HttpServer::mapListenerToVHost( HttpListener * pListener,
                      HttpVHost   * pVHost, const char * pDomains )
{
    return m_impl->mapListenerToVHost( pListener, pVHost, pDomains );
}
                            

int HttpServer::removeVHostFromListener( const char * pListener,
                        const char * pVHost )
{
    return m_impl->removeVHostFromListener( pListener, pVHost );
}

int HttpServer::start()
{
    return m_impl->start();
}

int HttpServer::shutdown()
{
    return m_impl->shutdown();
}

AccessControl* HttpServer::getAccessCtrl() const
{
    return &(m_impl->m_accessCtrl);
}

void HttpServer::setMaxConns( int32_t conns )
{
    if ( conns > DEFAULT_MAX_CONNS )
        conns = DEFAULT_MAX_CONNS;
    HttpGlobals::getConnLimitCtrl()->setMaxConns( conns );
}

void HttpServer::setMaxSSLConns( int32_t conns )
{
    if ( conns > DEFAULT_MAX_SSL_CONNS )
        conns = DEFAULT_MAX_SSL_CONNS;
    HttpGlobals::getConnLimitCtrl()->setMaxSSLConns( conns );
}

int HttpServer::getVHostCounts() const
{
    return m_impl->m_vhosts.size();
}

void HttpServer::beginConfig()
{
    m_impl->beginConfig();
}

void HttpServer::endConfig( int error )
{
    m_impl->endConfig( error );
}


void HttpServer::onTimer()
{
    m_impl->onTimer();
}


const AutoStr2 * HttpServer::getErrDocUrl( int statusCode ) const
{
    return m_impl->m_serverContext.getErrDocUrl( statusCode );
}

void HttpServer::releaseAll()
{
    if ( m_impl )
    {
        m_impl->releaseAll();
        delete m_impl;
        m_impl = NULL;
    }
}

void HttpServer::setLogLevel( const char * pLevel )
{
    HttpLog::setLogLevel( pLevel );
}

int HttpServer::setErrorLogFile( const char * pFileName )
{
    return HttpLog::setErrorLogFile( pFileName );
}

int HttpServer::setAccessLogFile( const char * pFileName, int pipe )
{
    return HttpLog::setAccessLogFile( pFileName, pipe );
}


void HttpServer::setErrorLogRollingSize( off_t size, int keep_days )
{
    HttpLog::getErrorLogger()->getAppender()->setRollingSize( size );
    HttpLog::getErrorLogger()->getAppender()->setKeepDays( size );
}


AccessLog* HttpServer::getAccessLog() const
{
    return HttpLog::getAccessLog();
}

const StringList * HttpServer::getIndexFileList() const
{
    return m_impl->m_serverContext.getIndexFileList();
}



void HttpServer::setSwapDir( const char * pDir )
{
    m_impl->setSwapDir( pDir );
}

const char * HttpServer::getSwapDir()
{
    return m_impl->getSwapDir();
}

int HttpServer::setupSwap()
{
    return m_impl->setupSwap();
}


void HttpServer::onVHostTimer()
{
    m_impl->m_vhosts.onTimer();
}

int HttpServer::enableVHost( const char * pVHostName, int enable )
{
    return m_impl->enableVHost( pVHostName, enable );
}

int HttpServer::isServerOk()
{
    return m_impl->isServerOk();
}

void HttpServer::offsetChroot()
{
    return m_impl->offsetChroot();
}

void HttpServer::setProcNo( int proc )
{   HttpGlobals::s_iProcNo = proc;
    m_impl->setRTReportName( proc );
}

void HttpServer::setBlackBoard( char * pBuf)
{   m_impl->setBlackBoard( pBuf );      }

void HttpServer::generateStatusReport()
{   m_impl->generateStatusReport();     }

int HttpServer::updateVHost( const char *pName, HttpVHost * pVHost )
{
    return m_impl->updateVHost( pName, pVHost );
}

void HttpServer::passListeners()
{
    m_impl->m_listeners.passListeners();
}

void HttpServer::recoverListeners()
{
    m_impl->m_oldListeners.recvListeners();
}

int HttpServer::initMultiplexer( const char * pType )
{
    return m_impl->m_dispatcher.init( pType );
}

int HttpServer::reinitMultiplexer()
{
    return m_impl->reinitMultiplexer();
}

int HttpServer::initAdns()
{
    return m_impl->initAdns();
}

int HttpServer::authAdminReq( char * pAuth )
{
    return m_impl->authAdminReq( pAuth );
}

HttpContext& HttpServer::getServerContext()
{
    return m_impl->m_serverContext;
}

void HttpServer::recycleContext( HttpContext * pContext )
{
    return m_impl->recycleContext( pContext );
}

void recycleContext( HttpContext * pContext )
{
    HttpServer::getInstance().recycleContext( pContext );
}
SSLContext * HttpServerImpl::newSSLContext( SSLContext * pContext, const char *pCertFile,
                    const char *pKeyFile, const char * pCAFile, const char * pCAPath,
                    const char * pCiphers, int certChain, int cv, int renegProtect )
{
    if (( !pContext )||
        ( pContext->isKeyFileChanged( pKeyFile )||
          pContext->isCertFileChanged( pCertFile )))
    {
        SSLContext* pNewContext = new SSLContext( SSLContext::SSL_ALL );
        if ( D_ENABLED( DL_LESS ) )
           LOG_D(( "[SSL] Create SSL context with"
                   " Certificate file: %s and Key File: %s.",
                 pCertFile, pKeyFile ));
        pNewContext->setRenegProtect( renegProtect );
        if ( !pNewContext->setKeyCertificateFile( pKeyFile,
            SSLContext::FILETYPE_PEM, pCertFile,
            SSLContext::FILETYPE_PEM, certChain ) )
        {
            LOG_ERR(( "[SSL] Config SSL Context with"
                      " Certificate File: %s"
                      " and Key File:%s get SSL error: %s",
                    pCertFile, pKeyFile,
                    SSLError().what() ));
            delete pNewContext;
        }
        else if ( (pCAFile || pCAPath) &&
            !pNewContext->setCALocation( pCAFile, pCAPath, cv ) )        
        {
            LOG_ERR(( "[SSL] Failed to setup Certificate Authority "
                      "Certificate File: '%s', Path: '%s', SSL error: %s",
                pCAFile?pCAFile:"", pCAPath?pCAPath:"", SSLError().what() ));
            delete pNewContext;
        }
        else
            pContext = pNewContext;
    }
    if ( pContext )
    {
        if ( D_ENABLED( DL_LESS ) )
           LOG_D(( "[SSL] set ciphers to:%s",
                 pCiphers ));
        pContext->setCipherList( pCiphers );
    }
    return pContext;
}

SSLContext * HttpServer::newSSLContext( SSLContext * pContext, const char *pCertFile,
                    const char *pKeyFile, const char * pCAFile, const char * pCAPath,
                    const char * pCiphers, int certChain, int cv, int renegProtect )
{
    return m_impl->newSSLContext( pContext, pCertFile, pKeyFile, pCAFile, pCAPath, pCiphers, certChain, cv, renegProtect );
}
//int HttpServer::importWebApp( HttpVHost * pVHost, const char * contextUri,
//                        const char* appPath, const char * pWorkerName
//                        )
//{
//    return m_impl->m_builder.importWebApp( pVHost, contextUri, appPath,
//            pWorkerName );
//}






