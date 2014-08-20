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
#include <http/httpvhost.h>
#include <http/accesscache.h>
#include <http/accesslog.h>
#include <http/awstats.h>
#include <http/contextlist.h>
#include <http/contexttree.h>
#include <util/datetime.h>
#include <http/handlerfactory.h>
#include <http/handlertype.h>
#include <http/hotlinkctrl.h>
#include <http/htpasswd.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <http/moduserdir.h>
#include <http/rewriteengine.h>
#include <http/rewriterule.h>
#include <http/rewritemap.h>
#include <http/userdir.h>
#include <http/htauth.h>
#include <http/httpserverconfig.h>
#include <http/httpstatuscode.h>
#include <http/httpmime.h>
#include <http/denieddir.h>
#include <log4cxx/logrotate.h>
#include <log4cxx/logger.h>
#include <log4cxx/layout.h>
#include <lsiapi/modulemanager.h>
#include <lsiapi/internal.h>
#include <sslpp/sslcontext.h>
#include <util/accesscontrol.h>
#include <util/hashstringmap.h>
#include <util/ni_fio.h>
#include <util/stringlist.h>
#include <util/stringtool.h>
#include "util/configctx.h"
#include <util/xmlnode.h>
#include <util/gpath.h>
#include <util/ssnprintf.h>
#include <main/httpserver.h>
#include "main/mainserverconfig.h"

#include <extensions/localworker.h>
#include <extensions/localworkerconfig.h>
#include <extensions/registry/extappregistry.h>
#include <extensions/registry/railsappconfig.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <lsiapi/lsiapihooks.h>


RealmMap::RealmMap( int initSize )
        : _shmap( initSize )
{}
RealmMap::~RealmMap()
{
    release_objects();
}
const UserDir* RealmMap::find( const char * pScript ) const
{
    iterator iter = _shmap::find( pScript );
    return ( iter != end() )
         ?iter.second()
         : NULL;
}

UserDir *RealmMap::get( const char * pFile, const char * pGroup )
{
    UserDir * pDir;
    HashDataCache * pGroupCache;
    iterator iter = begin();
    while( iter != end() )
    {
        pDir = iter.second();
        pGroupCache = pDir->getGroupCache();
        if (( strcmp( pDir->getUserStoreURI(), pFile ) == 0 )&&
            ((!pGroup)||
             ((pGroupCache)&&
              (strcmp( pDir->getGroupStoreURI(), pGroup ) == 0 ))))
            return pDir;
        iter = next( iter );
    }
    return NULL;
}
UserDir *HttpVHost::getFileUserDir(
          const char * pName, const char * pFile, const char * pGroup )
{
    PlainFileUserDir * pDir = (PlainFileUserDir *)m_realmMap.get( pFile, pGroup );
    if ( pDir )
        return pDir;
    if ( pName )
    {
        pDir = (PlainFileUserDir *)getRealm( pName );
        if ( pDir )
        {
            LOG_ERR(( "[%s] Realm %s exists.", m_sName.c_str(), pName ));
            return NULL;
        }
    }
    pDir = new PlainFileUserDir();
    if ( pDir )
    {
        char achName[100];
        if ( !pName )
        {
            safe_snprintf( achName, 100, "AnonyRealm%d", rand() );
            pName = achName;
        }
        pDir->setName( pName );
        m_realmMap.insert( pDir->getName(), pDir );
        pDir->setDataStore( pFile, pGroup );
    }
    return pDir;
}


void HttpVHost::offsetChroot( const char * pChroot, int len )
{
    char achTemp[512];
    const char * pOldName;
    if ( m_pAccessLog )
    {
        pOldName = m_pAccessLog->getAppender()->getName();
        if (strncmp( pChroot, pOldName, len ) == 0) 
        {
            strcpy( achTemp, pOldName + len );
            m_pAccessLog->getAppender()->setName( achTemp );
        }
    }
    if ( m_pLogger )
    {
        pOldName = m_pLogger->getAppender()->getName();
        if ( strncmp( pChroot, pOldName, len ) == 0 )
        {
            m_pLogger->getAppender()->close();
            off_t rollSize = m_pLogger->getAppender()->getRollingSize();
            strcpy( achTemp, pOldName + len );
            setErrorLogFile( achTemp );
            m_pLogger->getAppender()->setRollingSize( rollSize );
        }
    }
}


int HttpVHost::setErrorLogFile( const char * pFileName )
{
    LOG4CXX_NS::Appender *appender
        = LOG4CXX_NS::Appender::getAppender( pFileName, "appender.ps" );
    if ( appender )
    {
        if ( !m_pLogger )
            m_pLogger = LOG4CXX_NS::Logger::getLogger( m_sName.c_str() );
        if ( !m_pLogger )
            return -1;
        LOG4CXX_NS::Layout * layout = LOG4CXX_NS::Layout::getLayout(
                     ERROR_LOG_PATTERN, "layout.pattern" );
        appender->setLayout( layout );
        m_pLogger->setAppender( appender );
        m_pLogger->setParent( LOG4CXX_NS::Logger::getRootLogger() );
        return 0;
    }
    else
        return -1;
}

int HttpVHost::setAccessLogFile( const char * pFileName, int pipe )
{
    int ret = 0;

    if (( pFileName )&&( *pFileName ))
    {
        if ( !m_pAccessLog )
        {
            m_pAccessLog = new AccessLog();
            if ( !m_pAccessLog )
                return -1;
        }
        ret = m_pAccessLog->init( pFileName, pipe );
    }
    if ( ret )
    {
        if ( m_pAccessLog )
            delete m_pAccessLog;
        m_pAccessLog = NULL;
    }
    return ret;
}

const char * HttpVHost::getAccessLogPath() const
{
    if ( m_pAccessLog )
        return m_pAccessLog->getLogPath();
    return NULL;
}


/*****************************************************************
 * HttpVHost funcitons.
 *****************************************************************/
HttpVHost::HttpVHost( const char * pHostName )
    : m_pAccessLog( NULL )
    , m_pLogger( NULL )
    , m_pBytesLog( NULL )
    , m_iMaxKeepAliveRequests( 100 )
    , m_iSmartKeepAlive( 0 )
    , m_iFeatures( VH_ENABLE | VH_SERVER_ENABLE |
                   VH_ENABLE_SCRIPT | LS_ALWAYS_FOLLOW | 
                   VH_GZIP )
    , m_pAccessCache( NULL )
    , m_pHotlinkCtrl( NULL )
    , m_pAwstats( NULL )
    , m_sName( pHostName )
    , m_sAdminEmails("" )
    , m_sAutoIndexURI( "/_autoindex/default.php" )
    , m_iMappingRef( 0 )
    , m_uid( 500 )
    , m_gid( 500 )
    , m_iRewriteLogLevel( 0 )
    , m_iGlobalMatchContext( 1 )
    , m_pRewriteMaps( NULL )
    , m_pSSLCtx( NULL )
    , m_pSSITagConfig( NULL )
    , m_sSpdyAdHeader( "" )
{
    char achBuf[10] = "/";
    m_rootContext.set( achBuf, "/nON eXIST",
                HandlerFactory::getInstance( 0, NULL ), 1 );
    m_rootContext.allocateInternal();
    m_contexts.setRootContext( &m_rootContext );
}

HttpVHost::~HttpVHost()
{
    if ( m_pLogger )
    {
        m_pLogger->getAppender()->close();
    }
    if ( m_pAccessLog )
    {
        m_pAccessLog->flush();
        delete m_pAccessLog;
    }
    if ( m_pAccessCache )
        delete m_pAccessCache;
    if ( m_pHotlinkCtrl )
        delete m_pHotlinkCtrl;
    if ( m_pRewriteMaps )
        delete m_pRewriteMaps;
    if ( m_pAwstats )
        delete m_pAwstats;
    if ( m_pSSLCtx )
        delete m_pSSLCtx;
    
    
    LsiapiBridge::releaseModuleData( LSI_MODULE_DATA_VHOST, &m_moduleData );

}

int HttpVHost::setDocRoot( const char * psRoot )
{
    assert( psRoot != NULL );
    assert( *( psRoot + strlen( psRoot ) - 1 ) == '/' );
    m_rootContext.setRoot( psRoot );
    return 0;
}

AccessControl * HttpVHost::getAccessCtrl()
{   return m_pAccessCache->getAccessCtrl();   }
void HttpVHost::enableAccessCtrl()
{   m_pAccessCache = new AccessCache( 1543 );   }



//const StringList* HttpVHost::getIndexFileList() const
//{
///*
//    //which lines are invalid? have fun!
//    const char * const pT[3] = { "A", "B", "C" };
//    const char * pTT[3] = { "A", "B", "C" };
//    const char ** pT1 = pT;
//    pT1 = pTT;
//    const char * const* pT2 = pT;
//    pT2 = pTT;
//    const char * const* const pT3 = pT;
//    const char * const* const pT4 = pTT;
//    const char * * const pT5 = pT;
//    const char * * const pT6 = pTT;
//    const char * pV1;
//    *pT1 = pV1;
//    *pT2 = pV1;
//    *pT3 = pV1;
//    pT3 = pT2;
//    pT2 = pT3;
//    pT2 = pT1;
//    pT1 = pT2;
//*/
//
//    return &(m_indexFileList);
//}




UserDir * HttpVHost::getRealm( const char * pRealm )
{
    return (UserDir *)m_realmMap.find( pRealm );
}

const UserDir * HttpVHost::getRealm(const char * pRealm ) const
{
    return m_realmMap.find( pRealm );
}

void HttpVHost::setLogLevel( const char * pLevel )
{
    if ( m_pLogger )
        m_pLogger->setLevel( pLevel );
}



void HttpVHost::setErrorLogRollingSize( off_t size, int keep_days )
{
    if ( m_pLogger )
    {
        m_pLogger->getAppender()->setRollingSize( size );
        m_pLogger->getAppender()->setKeepDays( keep_days );
    }
}



void  HttpVHost::logAccess( HttpSession *pSession ) const
{
    if ( m_pAccessLog )
        m_pAccessLog->log( pSession );
    else
        HttpLog::logAccess( m_sName.c_str(), m_sName.len(), pSession );
}


void HttpVHost::setBytesLogFilePath( const char * pPath, off_t rollingSize )
{
    if ( m_pBytesLog )
    {
        m_pBytesLog->close();
    }
    m_pBytesLog = LOG4CXX_NS::Appender::getAppender( pPath );
    m_pBytesLog->setRollingSize( rollingSize );
}

void HttpVHost::logBytes( long long bytes )
{
    char achBuf[80];
    int n = safe_snprintf( achBuf, 80, "%ld %lld .\n", DateTime::s_curTime, bytes );
    m_pBytesLog->append( achBuf, n );
}



// const char * HttpVHost::getAdminEmails() const
// {   return m_sAdminEmails.c_str();  }

void HttpVHost::setAdminEmails( const char * pEmails)
{
    m_sAdminEmails = pEmails;
}

void HttpVHost::onTimer30Secs()
{
}


void HttpVHost::onTimer()
{
    using namespace LOG4CXX_NS;
    if ( HttpGlobals::s_iProcNo )
    {
        if ( m_pAccessLog )
        {
            if ( m_pAccessLog->reopenExist() == -1 )
            {
                LOG_ERR(( "[%s] Failed to open access log file %s.",
                        m_sName.c_str(), m_pAccessLog->getLogPath() ));
            }
            m_pAccessLog->flush();
        }
        if ( m_pLogger )
        {
            m_pLogger->getAppender()->reopenExist();
        }
    }
    else
    {
        if ( m_pAccessLog && !m_pAccessLog->isPipedLog())
        {
            if ( LogRotate::testRolling( m_pAccessLog->getAppender(),
                        HttpGlobals::s_uid, HttpGlobals::s_gid) )
            {
                if ( m_pAwstats )
                    m_pAwstats->update( this );
                else
                    LogRotate::testAndRoll( m_pAccessLog->getAppender(),
                            HttpGlobals::s_uid, HttpGlobals::s_gid );
            }
            else if ( m_pAwstats )
                m_pAwstats->updateIfNeed( time( NULL ), this );
        }
        if ( m_pBytesLog )
            LogRotate::testAndRoll( m_pBytesLog, HttpGlobals::s_uid, HttpGlobals::s_gid );
        if ( m_pLogger )
        {
            LogRotate::testAndRoll( m_pLogger->getAppender(),
                                HttpGlobals::s_uid, HttpGlobals::s_gid );
        }
    }
}

void HttpVHost::setChroot( const char * pRoot )
{
    m_sChroot = pRoot;
}

void HttpVHost::addRewriteMap( const char * pName, const char * pLocation )
{
    RewriteMap * pMap = new RewriteMap();
    if ( !pMap )
    {
        ERR_NO_MEM( "new RewriteMap()" );
        return;
    }
    pMap->setName( pName );
    int ret = pMap->parseType_Source( pLocation );
    if ( ret )
    {
        delete pMap;
        if ( ret == 2 )
            LOG_ERR(( "unknown or unsupported rewrite map type!" ));
        if ( ret == -1 )
            ERR_NO_MEM( "parseType_Source()" );
        return;
    }
    if ( !m_pRewriteMaps )
    {
        m_pRewriteMaps = new RewriteMapList();
        if ( !m_pRewriteMaps )
        {
            ERR_NO_MEM( "new RewriteMapList()" );
            delete pMap;
            return;
        }
    }
    m_pRewriteMaps->insert( pMap->getName(), pMap );
}

void HttpVHost::addRewriteRule( char * pRules )
{
    RewriteEngine::parseRules( pRules, &m_rewriteRules, m_pRewriteMaps );
}

void HttpVHost::updateUGid( const char * pLogId, const char * pPath)
{
    struct stat st;
    char achBuf[8192];
    char * p = achBuf;
    if ( getRootContext().getSetUidMode() != UID_DOCROOT )
    {
        setUid( HttpGlobals::s_uid );
        setGid( HttpGlobals::s_gid );
        return;
    }
    if( HttpGlobals::s_psChroot )
    {
        strcpy( p, HttpGlobals::s_psChroot->c_str() );
        p += HttpGlobals::s_psChroot->len();
    }
    memccpy( p, pPath, 0, &achBuf[8191] - p );
    int ret = nio_stat( achBuf, &st );
    if ( ret )
    {
        LOG_ERR(( "[%s] stat() failed on %s!",
                pLogId, achBuf ));
    }
    else
    {
        if ( st.st_uid < HttpGlobals::s_uidMin )
        {
            LOG_WARN(( "[%s] Uid of %s is smaller than minimum requirement %d, use server uid!",
                pLogId, achBuf, HttpGlobals::s_uidMin ));
            st.st_uid = HttpGlobals::s_uid;
        }
        if ( st.st_gid < HttpGlobals::s_gidMin )
        {
            st.st_gid = HttpGlobals::s_gid;
            LOG_WARN(( "[%s] Gid of %s is smaller than minimum requirement %d, use server gid!",
                pLogId, achBuf, HttpGlobals::s_gidMin ));
        }
        setUid( st.st_uid );
        setGid( st.st_gid );
    }
}
HttpContext * HttpVHost::setContext( HttpContext * pContext,
    const char * pUri, int type, const char * pLocation, const char * pHandler,
    int allowBrowse, int match )
{
   const HttpHandler *pHdlr = HandlerFactory::getInstance( type, pHandler );

    if ( !pHdlr )
    {
        LOG_ERR( ( "[%s] Can not find handler with type: %d, name: %s.",
                   LogIdTracker::getLogId(), type, ( pHandler ) ? pHandler : "" ) );
    }
    else
        if ( type > HandlerType::HT_CGI )
            pHdlr = isHandlerAllowed( pHdlr, type, pHandler );
    if ( !pHdlr )
    {
        allowBrowse = 0;
        pHdlr = HandlerFactory::getInstance( 0, NULL );
    }
    if ( pContext )
    {
        int ret =  pContext->set(pUri, pLocation, pHdlr, allowBrowse, match);
        if ( ret )
        {
            LOG_ERR_CODE( ret );
            delete pContext;
            pContext = NULL;
        }
    }
    return pContext;

}


HttpContext * HttpVHost::addContext( const char * pUri, int type,
                const char * pLocation, const char * pHandler, int allowBrowse )
{
    int ret;
    HttpContext* pContext = new HttpContext();
    if ( pContext )
    {
        setContext( pContext, pUri, type, pLocation, pHandler, allowBrowse, 0);
        ret = addContext( pContext );
        if ( ret != 0 )
        {
            delete pContext;
            pContext = NULL;
        }
    }
    return pContext;
    
}

const HttpContext* HttpVHost::matchLocation( const char * pURI, int regex ) const
{
    const HttpContext * pContext;
    if ( !regex )
    {
        pContext = m_contexts.matchLocation( pURI );
    }
    else
    {
        char achTmp[] = "/";
        HttpContext * pOld = getContext( achTmp );
        if ( !pOld )
        {
            return NULL;
        }
        pContext = pOld->findMatchContext( pURI, 1 );        
    }
    
    return pContext;
    
}

HttpContext* HttpVHost::getContext( const char * pURI, int regex ) const
{
    if ( !regex )
        return m_contexts.getContext( pURI );
    const HttpContext * pContext = m_contexts.getRootContext();
    pContext = pContext->findMatchContext( pURI );
    return (HttpContext *)pContext;
    
}

void HttpVHost::setSSLContext( SSLContext * pCtx )
{
    if ( pCtx == m_pSSLCtx )
        return;
    if ( m_pSSLCtx )
        delete m_pSSLCtx;
    m_pSSLCtx = pCtx;
}
HTAuth *HttpVHost::configAuthRealm( HttpContext *pContext,
                             const char *pRealmName )
{
    HTAuth *pAuth = NULL;

    if ( ( pRealmName != NULL ) && ( *pRealmName ) )
    {
        UserDir *pUserDB = getRealm( pRealmName );

        if ( !pUserDB )
        {
            ConfigCtx::getCurConfigCtx()->log_warn( "<realm> %s is not configured,"
                                   " deny access to this context", pRealmName );
        }
        else
        {
            pAuth = new HTAuth();

            if ( !pAuth )
            {
                ERR_NO_MEM( "new HTAuth()" );
            }
            else
            {
                pAuth->setName( pRealmName );
                pAuth->setUserDir( pUserDB );
                pContext->setHTAuth( pAuth );
                pContext->setConfigBit( BIT_AUTH, 1 );
                pContext->setAuthRequired( "valid-user" );
            }
        }

        if ( !pAuth )
        {
            pContext->allowBrowse( false );
        }
    }

    return pAuth;
}
int HttpVHost::configContextAuth( HttpContext *pContext,
                           const XmlNode *pContextNode  )
{
    const char *pRealmName = NULL;
    const XmlNode *pAuthNode = pContextNode->getChild( "auth" );
    HTAuth *pAuth = NULL;

    if ( !pAuthNode )
    {
        pAuthNode = pContextNode;
    }

    pRealmName = pAuthNode->getChildValue( "realm" );
    pAuth = configAuthRealm( pContext, pRealmName );

    if ( pAuth )
    {
        const char *pData = pAuthNode->getChildValue( "required" );

        if ( !pData )
            pData = "valid-user";

        pContext->setAuthRequired( pData );

        pData = pAuthNode->getChildValue( "authName" );

        if ( pData )
            pAuth->setName( pData );
    }

    return 0;
}

int HttpVHost::configBasics( const XmlNode *pVhConfNode, int iChrootLen )
{
    const char *pDocRoot = ConfigCtx::getCurConfigCtx()->getTag( pVhConfNode, "docRoot" );

    if ( pDocRoot == NULL )
    {
        return -1;
    }

    //ConfigCtx::getCurConfigCtx()->setVHost( this );
    char achBuf[MAX_PATH_LEN];
    char *pPath = achBuf;

    if ( ConfigCtx::getCurConfigCtx()->getValidPath( pPath, pDocRoot, "document root" ) != 0 )
        return -1;

    if ( ConfigCtx::getCurConfigCtx()->checkPath( pPath, "document root", followSymLink() ) == -1 )
        return -1;

    //pPath += m_sChroot.len();
    pPath += iChrootLen;
    ConfigCtx::getCurConfigCtx()->clearDocRoot();
    if ( setDocRoot( pPath ) != 0 )
    {
        ConfigCtx::getCurConfigCtx()->log_error( "failed to set document root - %s!", pPath );
        return -1;
    }
    ConfigCtx::getCurConfigCtx()->setDocRoot( pPath );
    //if ( ConfigCtx::getCurConfigCtx()->checkDeniedSubDirs( this, "/", pPath ) )
    if ( checkDeniedSubDirs( "/", pPath ) )
    {
        return -1;
    }

    enable( ConfigCtx::getCurConfigCtx()->getLongValue( pVhConfNode, "enabled", 0, 1, 1 ) );
    enableGzip( ( HttpServerConfig::getInstance().getGzipCompress() ) ?
                        ConfigCtx::getCurConfigCtx()->getLongValue( pVhConfNode, "enableGzip", 0, 1, 1 ) : 0 );

    const char *pAdminEmails = pVhConfNode->getChildValue( "adminEmails" );
    if ( !pAdminEmails )
    {
        pAdminEmails = "";
    }
    setAdminEmails( pAdminEmails );
    
    const char *pSpdyAdHeader = pVhConfNode->getChildValue( "spdyAdHeader" );
    if ( !pSpdyAdHeader )
        pSpdyAdHeader = "";
    setSpdyAdHeader( pSpdyAdHeader );
    
    return 0;

}

int HttpVHost::configWebsocket( const XmlNode *pWebsocketNode )
{
    const char *pUri = ConfigCtx::getCurConfigCtx()->getTag( pWebsocketNode, "uri", 1 );
    const char *pAddress = ConfigCtx::getCurConfigCtx()->getTag( pWebsocketNode, "address" );
    char achVPath[MAX_PATH_LEN];
    char achRealPath[MAX_PATH_LEN];

    if ( pUri == NULL || pAddress == NULL )
        return -1;

    HttpContext *pContext = getContext( pUri, 0 );

    if ( pContext == NULL )
    {
        strcpy( achVPath, "$DOC_ROOT" );
        strcat( achVPath, pUri );
        ConfigCtx::getCurConfigCtx()->getAbsoluteFile( achRealPath, achVPath );
        pContext = addContext( pUri, HandlerType::HT_NULL, achRealPath, NULL, 1 );

        if ( pContext == NULL )
            return -1;
    }

    GSockAddr gsockAddr;
    gsockAddr.parseAddr( pAddress );
    pContext->setWebSockAddr( gsockAddr );
    return 0;
}
int HttpVHost::configVHWebsocketList( const XmlNode *pVhConfNode )
{
    const XmlNode *p0 = pVhConfNode->getChild( "websocketlist", 1 );

    const XmlNodeList *pList = p0->getChildren( "websocket" );

    if ( pList )
    {
        XmlNodeList::const_iterator iter;

        for( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            configWebsocket( *iter );
        }
    }

    return 0;
}
int HttpVHost::configHotlinkCtrl( const XmlNode *pNode )
{
    int enabled = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "enableHotlinkCtrl", 0, 1, 0 );

    if ( !enabled )
        return 0;

    HotlinkCtrl *pCtrl = new HotlinkCtrl();

    if ( !pCtrl )
    {
        ConfigCtx::getCurConfigCtx()->log_error( "out of memory while creating HotlinkCtrl Object!" );
        return -1;
    }
    if ( pCtrl->config( pNode ) == -1 )
    {
        delete pCtrl;
        return -1;
    }
    setHotlinkCtrl( pCtrl );
    return 0;
}
int HttpVHost::configSecurity( const XmlNode *pVhConfNode)
{
    const XmlNode *p0 = pVhConfNode->getChild( "security", 1 );

    if ( p0 != NULL )
    {
        ConfigCtx currentCtx( "security" );

        if ( AccessControl::isAvailable( p0 ) )
        {
            enableAccessCtrl();
            getAccessCtrl()->config( p0 );
        }

        configRealmList( p0 );
        const XmlNode *p1 = p0->getChild( "hotlinkCtrl" );

        if ( p1 )
            configHotlinkCtrl( p1 );
    }

    return 0;
}
int HttpVHost::setAuthCache( const XmlNode *pNode,
                          HashDataCache *pAuth )
{
    int timeout = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "expire", 0, LONG_MAX, 60 );
    int maxSize = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "maxCacheSize", 0, LONG_MAX, 1024 );
    pAuth->setExpire( timeout );
    pAuth->setMaxSize( maxSize );
    return 0;
}
int HttpVHost::configRealm( const XmlNode *pRealmNode )
{
    int iChrootLen = 0;
    
    const char *pName = ConfigCtx::getCurConfigCtx()->getTag( pRealmNode, "name", 1 );
    if ( pName == NULL )
    {
        return -1;
    }
    ConfigCtx currentCtx( "realm", pName );

    const char *pType = pRealmNode->getChildValue( "type" );
    
    if ( HttpGlobals::s_psChroot != NULL )
    iChrootLen = HttpGlobals::s_psChroot->len();
    
    if ( !pType )
    {
        ConfigCtx::getCurConfigCtx()->log_warn( "Realm type is not specified, use default: file." );
        pType = "file";
    }

    const XmlNode *p2 = pRealmNode->getChild( "userDB" );

    if ( p2 == NULL )
    {
        ConfigCtx::getCurConfigCtx()->log_error( "missing <%s> in <%s>", "userDB", "realm" );
        return -1;
    }

    if ( strcasecmp( pType, "file" ) == 0 )
    {
        const char *pFile = NULL, *pGroup = NULL, *pGroupFile = NULL;
        char achBufFile[MAX_PATH_LEN];
        char achBufGroup[MAX_PATH_LEN];

        pFile = p2->getChildValue( "location" );

        if ( ( !pFile ) ||
                ( ConfigCtx::getCurConfigCtx()->getValidFile( achBufFile, pFile, "user DB" ) != 0 ) )
        {
            return -1;
        }

        const XmlNode *pGroupNode = pRealmNode->getChild( "groupDB" );

        if ( pGroupNode )
        {
            pGroup = pGroupNode->getChildValue( "location" );

            if ( pGroup )
            {
                if ( ConfigCtx::getCurConfigCtx()->getValidFile( achBufGroup, pGroup, "group DB" ) != 0 )
                {
                    return -1;
                }
                else
                    pGroupFile = &achBufGroup[iChrootLen];
            }

        }

        UserDir *pUserDir =
            getFileUserDir( pName, &achBufFile[iChrootLen],
                                    pGroupFile );

        if ( !pUserDir )
        {
            ConfigCtx::getCurConfigCtx()->log_error( "Failed to create authentication DB." );
            return -1;
        }

        setAuthCache( p2, pUserDir->getUserCache() );

        if ( pGroup )
        {
            setAuthCache( pGroupNode, pUserDir->getGroupCache() );
        }
    }
    else
    {
        ConfigCtx::getCurConfigCtx()->log_error( "unsupported DB type in realm %s!", pName );
    }

    return 0;
}


int HttpVHost::configRealmList( const XmlNode *pRoot)
{
    const XmlNode *pNode = pRoot->getChild( "realmList", 1 );
    if ( pNode != NULL )
    {
        const XmlNodeList *pList = pNode->getChildren( "realm" );

        if ( pList )
        {
            XmlNodeList::const_iterator iter;

            for( iter = pList->begin(); iter != pList->end(); ++iter )
            {
                const XmlNode *pRealmNode = *iter;
                configRealm( pRealmNode );
            }
        }
    }

    return 0;
}
void HttpVHost::configRewriteMap( const XmlNode *pNode )
{
    const XmlNodeList *pList = pNode->getChildren( "map" );

    if ( pList )
    {
        XmlNodeList::const_iterator iter;

        for( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            XmlNode *pN1 = *iter;
            const char *pName = pN1->getChildValue( "name" );
            const char *pLocation = pN1->getChildValue( "location" );
            char achBuf[1024];
            const char *p = strchr( pLocation, '$' );

            if ( p )
            {
                memmove( achBuf, pLocation, p - pLocation );

                if ( ConfigCtx::getCurConfigCtx()->getAbsolute( &achBuf[ p - pLocation], p, 0 ) == 0 )
                    pLocation = achBuf;
            }

            addRewriteMap( pName, pLocation );
        }
    } 
}
int HttpVHost::configRewrite( const XmlNode *pNode )
{
    getRootContext().enableRewrite( ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "enable", 0, 1, 0 ) );
    setRewriteLogLevel( ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "logLevel", 0, 9, 0 ) );
    
    configRewriteMap( pNode );
    
    RewriteRule::setLogger( NULL, LogIdTracker::getLogId() );
    char *pRules = ( char * ) pNode->getChildValue( "rules" );

    if ( pRules )
    {
        getRootContext().configRewriteRule( getRewriteMaps(), pRules );
    }
    else
        getRootContext().configRewriteRule( getRewriteMaps(), pNode );

    return 0;
}
const XmlNode* HttpVHost::configIndex( const XmlNode *pVhConfNode, 
           const StringList *pStrList )
{
    const XmlNode *pNode;
    pNode = pVhConfNode->getChild( "index" );
    const char *pUSTag = "indexFiles_useServer";

    if ( !pNode )
    {
        pNode = pVhConfNode;
    }
    else
        pUSTag += 11;

    int useServer = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, pUSTag, 0, 2, 1 );
    StringList *pIndexList = NULL;

    if ( useServer != 1 )
    {
        getRootContext().configDirIndex( pNode );

        if ( useServer == 2 )
        {
            pIndexList = getRootContext().getIndexFileList();

            if ( pIndexList )
            {
                if ( pStrList )
                    pIndexList->append( *pStrList );                
            }
            else
                getRootContext().setConfigBit( BIT_DIRINDEX, 0 );
        }
    }

    getRootContext().configAutoIndex( pNode );  
    return pNode;
}
int HttpVHost::configIndexFile( const XmlNode *pVhConfNode, 
           const StringList *pStrList, const char* strIndexURI )
{
    const XmlNode *pNode = configIndex( pVhConfNode, pStrList );
    const char *pUSTag = pNode->getChildValue( "autoIndexURI" );
    if ( pUSTag )
        if ( *pUSTag != '/' )
        {
            ConfigCtx::getCurConfigCtx()->log_error( "Invalid AutoIndexURI, must be started with a '/'" );
            pUSTag = NULL;            
        }
    setAutoIndexURI( ( pUSTag ) ? pUSTag : strIndexURI );

    char achURI[] = "/_autoindex/";
    HttpContext * pContext = configContext( achURI , HandlerType::HT_NULL,
                   "$SERVER_ROOT/share/autoindex/", NULL, 1 );
    pContext->enableScript( 1 );
    return 0;
}

HttpContext* HttpVHost::addContext( int match, const char *pUri, int type, 
          const char *pLocation, const char *pHandler, int allowBrowse)
{
    if ( match )
    {
        char achTmp[] = "/";
        HttpContext *pOld;

        if ( isGlobalMatchContext() )
            pOld = &getRootContext();
        else
            pOld = getContext( achTmp );

        if ( !pOld )
        {
            return NULL;
        }

        HttpContext *pContext = new HttpContext();
        setContext( pContext, pUri, type, pLocation, pHandler, allowBrowse, match );
        pOld->addMatchContext( pContext );
        return pContext;
    }
    else
    {
        setGlobalMatchContext( 1 );
        HttpContext *pOld = getContext( pUri );

        if ( pOld )
        {
            if ( strcmp( pUri, "/" ) == 0 )
            {
                setContext( pOld, pUri, type, pLocation, pHandler, allowBrowse, match );
                return pOld;
            }

            return NULL;
        }

        return addContext( pUri, type, pLocation, pHandler, allowBrowse );
    }
}
HttpContext* HttpVHost::configContext( const char *pUri, int type, const char *pLocation, 
             const char *pHandler, int allowBrowse )
{
    char achRealPath[4096];
    int match = ( strncasecmp( pUri, "exp:", 4 ) == 0 );
    assert( type != HandlerType::HT_REDIRECT );

    if ( type >= HandlerType::HT_CGI )
        allowBrowse = 1;

    if ( type < HandlerType::HT_FASTCGI && type != HandlerType::HT_MODULE )
    {
        if ( allowBrowse )
        {
            if ( pLocation == NULL )
            {
                if ( ( type == HandlerType::HT_REDIRECT ) ||
                        ( type == HandlerType::HT_RAILS ) || match )
                {
                    ConfigCtx::getCurConfigCtx()->log_errorMissingTag( "location" );
                    return NULL;
                }
                else
                    pLocation = pUri + 1;
            }

            int ret = 0;

            switch( *pLocation )
            {
            case '$':

                if ( ( match ) && ( isdigit( * ( pLocation + 1 ) ) ) )
                {
                    strcpy( achRealPath, pLocation );
                    break;
                }

                //fall through
            default:
                ret = ConfigCtx::getCurConfigCtx()->getAbsoluteFile( achRealPath, pLocation );
                break;
            }

            if ( ret )
            {
                ConfigCtx::getCurConfigCtx()->log_errorPath( "context location",  pLocation );
                return NULL;
            }

            if ( !match )
            {
                int PathLen = strlen( achRealPath );

                if ( ConfigCtx::getCurConfigCtx()->checkPath( achRealPath, "context location", followSymLink() ) )
                    return NULL;

                PathLen = strlen( achRealPath );

                if ( access( achRealPath, F_OK ) != 0 )
                {
                    ConfigCtx::getCurConfigCtx()->log_error( "path is not accessible: %s", achRealPath );
                    return NULL;
                }

                if ( PathLen > 512 )
                {
                    ConfigCtx::getCurConfigCtx()->log_error( "path is too long: %s", achRealPath );
                    return NULL;
                }

                pLocation = achRealPath;
                if ( HttpGlobals::s_psChroot != NULL )
                       pLocation += HttpGlobals::s_psChroot->len();

                if ( allowBrowse )
                {
                    //ret = ConfigCtx::getCurConfigCtx()->checkDeniedSubDirs( this, pUri, pLocation );
                    ret = checkDeniedSubDirs( pUri, pLocation );

                    if ( ret )
                        return NULL;
                }
            }
            else
                pLocation = achRealPath;
        }
    }
    else
    {
        if ( pHandler == NULL )
        {
            ConfigCtx::getCurConfigCtx()->log_errorMissingTag( "handler" );
            return NULL;
        }
    }

    return addContext( match, pUri, type, pLocation, pHandler, allowBrowse );
}
int HttpVHost::configAwstats( const char* vhDomain, int vhAliasesLen,
                                    const XmlNode *pNode )
{
    const XmlNode *pAwNode = pNode->getChild( "awstats" );

    if ( !pAwNode )
        return 0;

    int val;
    const char *pValue;
    char iconURI[] = "/awstats/icon/";
    char achBuf[8192];
    int iChrootLen = 0;
    if ( HttpGlobals::s_psChroot != NULL )
    iChrootLen = HttpGlobals::s_psChroot->len();
    ConfigCtx currentCtx( "awstats" );
    val = ConfigCtx::getCurConfigCtx()->getLongValue( pAwNode, "updateMode", 0, 2, 0 );

    if ( val == 0 )
        return 0;

    if ( !getAccessLog() )
    {
        currentCtx.log_error( "Virtual host does not have its own access log file, "
                              "AWStats integration is disabled." );
        return -1;
    }

    if ( ConfigCtx::getCurConfigCtx()->getValidPath( achBuf, "$SERVER_ROOT/add-ons/awstats/",
                      "AWStats installation" ) == -1 )
    {
        currentCtx.log_error( "Cannot find AWStats installation at [%s],"
                              " AWStats add-on is disabled!" );
        return -1;
    }

    if ( ConfigCtx::getCurConfigCtx()->getValidPath( achBuf, "$SERVER_ROOT/add-ons/awstats/wwwroot/icon/",
                      "AWStats icon directory" ) == -1 )
    {
        currentCtx.log_error( "Cannot find AWStats icon directory at [%s],"
                              " AWStats add-on is disabled!" );
        return -1;
    }

    addContext( iconURI, HandlerType::HT_NULL,
                        &achBuf[iChrootLen], NULL, 1 );

    pValue = pAwNode->getChildValue( "workingDir" );

    if ( ( !pValue ) || ( ConfigCtx::getCurConfigCtx()->getAbsolutePath( achBuf, pValue ) == -1 ) ||
            ( ConfigCtx::getCurConfigCtx()->checkAccess( achBuf ) == -1 ) )
    {
        currentCtx.log_error( "AWStats working directory: %s does not exist or access denied, "
                              "please fix it, AWStats integration is disabled.", achBuf );
        return -1;
    }

    if ( ( restrained() ) &&
            ( strncmp( &achBuf[iChrootLen], getVhRoot()->c_str(),
                       getVhRoot()->len() ) ) )
    {
        currentCtx.log_error( "AWStats working directory: %s is not inside virtual host root: "
                              "%s%s, AWStats integration is disabled.", achBuf, 
                              ((iChrootLen==0)?(""):(HttpGlobals::s_psChroot->c_str())), getVhRoot()->c_str() );
        return -1;
    }

    Awstats *pAwstats = new Awstats();
    pAwstats->config(this, val, achBuf, pAwNode, 
           iconURI, vhDomain, vhAliasesLen );
    return 0;
}
#define MAX_URI_LEN  1024
HttpContext *HttpVHost::addRailsContext( const char *pURI,
        const char *pLocation, LocalWorker *pWorker )
{
    char achURI[MAX_URI_LEN];
    int uriLen = strlen( pURI );

    strcpy( achURI, pURI );

    if ( achURI[uriLen - 1] != '/' )
    {
        achURI[uriLen++] = '/';
        achURI[uriLen] = 0;
    }

    if ( uriLen > MAX_URI_LEN - 100 )
    {
        ConfigCtx::getCurConfigCtx()->log_error( "context URI is too long!" );
        return NULL;
    }

    char achBuf[MAX_PATH_LEN];
    snprintf( achBuf, MAX_PATH_LEN, "%spublic/", pLocation );
    HttpContext *pContext = configContext( achURI, HandlerType::HT_NULL,
                                           achBuf, NULL, 1 );

    if ( !pContext )
    {
        return NULL;
    }

    strcpy( &achURI[uriLen], "dispatch.rb" );
    HttpContext *pDispatch = addContext( 0, achURI, HandlerType::HT_NULL,
                                         NULL, NULL, 1 );

    if ( pDispatch )
        pDispatch->setHandler( pWorker );

    strcpy( &achURI[uriLen], "dispatch.lsapi" );
    pDispatch = addContext( 0, achURI, HandlerType::HT_NULL,
                            NULL, NULL, 1 );

    if ( pDispatch )
        pDispatch->setHandler( pWorker );

    pContext->setCustomErrUrls( "404", achURI );
    pContext->setRailsContext();
    return pContext;
}

HttpContext *HttpVHost::configRailsContext( const char *contextUri, const char *appPath,
                int maxConns, const char *pRailsEnv, int maxIdle, const Env *pEnv, 
                const char *pRubyPath )
{
    char achFileName[MAX_PATH_LEN];

    if ( !RailsAppConfig::getpRailsDefault() )
        return NULL;

    int ret = ConfigCtx::getCurConfigCtx()->getAbsolutePath( achFileName, appPath );

    if ( ret == -1 )
    {
        ConfigCtx::getCurConfigCtx()->log_error( "path to Rails application is invalid!" );
        return NULL;
    }


    LocalWorker *pWorker = RailsAppConfig::newRailsApp( this, contextUri, achFileName, getName(), maxConns, pRailsEnv, 
                 maxIdle, pEnv, RailsAppConfig::getpRailsDefault()->getRunOnStartUp(), pRubyPath ) ;

    if ( !pWorker )
        return NULL;

    return addRailsContext( contextUri, achFileName,
                            pWorker );
}

HttpContext *HttpVHost::configRailsContext( const XmlNode *pNode,
        const char *contextUri, const char *appPath )
{

    int maxConns = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "maxConns", 1, 2000,
                   RailsAppConfig::getpRailsDefault()->getMaxConns() );
    const char *railsEnv[3] = { "development", "production", "staging" };
    int production = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "railsEnv", 0, 2, RailsAppConfig::getRailsEnv() );
    const char *pRailsEnv = railsEnv[production];

    const char *pRubyPath = pNode->getChildValue( "rubyBin" );

    long maxIdle = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "extMaxIdleTime", -1, INT_MAX,
                   RailsAppConfig::getpRailsDefault()->getMaxIdleTime() );

    if ( maxIdle == -1 )
        maxIdle = INT_MAX;

    return configRailsContext( contextUri, appPath,
                               maxConns, pRailsEnv, maxIdle, NULL, pRubyPath );
}

HttpContext *HttpVHost::importWebApp( const char *contextUri, const char *appPath,
        const char *pWorkerName, int allowBrowse )
{
    char achFileName[MAX_PATH_LEN];

    int ret = ConfigCtx::getCurConfigCtx()->getAbsolutePath( achFileName, appPath );

    if ( ret == -1 )
    {
        ConfigCtx::getCurConfigCtx()->log_error( "path to Web-App is invalid!" );
        return NULL;
    }

    int pathLen = strlen( achFileName );

    if ( pathLen > MAX_PATH_LEN - 20 )
    {
        ConfigCtx::getCurConfigCtx()->log_error( "path to Web-App is too long!" );
        return NULL;
    }

    if ( access( achFileName, F_OK ) != 0 )
    {
        ConfigCtx::getCurConfigCtx()->log_errorPath( "Web-App",  achFileName );
        return NULL;
    }

    strcpy( &achFileName[pathLen], "WEB-INF/web.xml" );

    if ( access( achFileName, F_OK ) != 0 )
    {
        ConfigCtx::getCurConfigCtx()->log_errorPath( "Web-App configuration",  achFileName );
        return NULL;
    }

    char achURI[MAX_URI_LEN];
    int uriLen = strlen( contextUri );
    strcpy( achURI, contextUri );

    if ( achFileName[pathLen - 1] != '/' )
        ++pathLen;

    if ( achURI[uriLen - 1] != '/' )
    {
        achURI[uriLen++] = '/';
        achURI[uriLen] = 0;
    }

    if ( uriLen > MAX_URI_LEN - 100 )
    {
        ConfigCtx::getCurConfigCtx()->log_error( "context URI is too long!" );
        return NULL;
    }

    XmlNode *pRoot = ConfigCtx::getCurConfigCtx()->parseFile( achFileName, "web-app");

    if ( !pRoot )
    {
        ConfigCtx::getCurConfigCtx()->log_error( "invalid Web-App configuration file: %s."
                                    , achFileName );
        return NULL;
    }

    achFileName[pathLen] = 0;
    HttpContext *pContext = configContext( achURI, HandlerType::HT_NULL,
                                           achFileName, NULL, allowBrowse );

    if ( !pContext )
    {
        delete pRoot;
        return NULL;
    }

    strcpy( &achURI[uriLen], "WEB-INF/" );
    strcpy( &achFileName[pathLen], "WEB-INF/" );
    HttpContext *pInnerContext;
    pInnerContext = configContext( achURI, HandlerType::HT_NULL,
                                   achFileName, NULL, false );

    if ( !pInnerContext )
    {
        delete pRoot;
        return NULL;
    }
    configServletMapping(pRoot, achURI, uriLen, pWorkerName, allowBrowse );

    delete pRoot;
    return pContext;
}
void HttpVHost::configServletMapping(XmlNode *pRoot, char* pachURI, int iUriLen,
    const char *pWorkerName, int allowBrowse )
{
    HttpContext *pInnerContext;
    const XmlNodeList *pList = pRoot->getChildren( "servlet-mapping" );

    if ( pList )
    {
        XmlNodeList::const_iterator iter;

        for( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            const XmlNode *pMappingNode = *iter;
            const char   *pUrlPattern =
                pMappingNode->getChildValue( "url-pattern" );

            if ( pUrlPattern )
            {
                if ( *pUrlPattern == '/' )
                {
                    ++pUrlPattern;
                }

                strcpy( &pachURI[iUriLen], pUrlPattern );
                int patternLen = strlen( pUrlPattern );

                //remove the trailing '*'
                if ( * ( pUrlPattern + patternLen - 1 ) == '*' )
                    pachURI[iUriLen + patternLen - 1] = 0;

                pInnerContext = configContext( pachURI, HandlerType::HT_SERVLET,
                                               NULL, pWorkerName, allowBrowse );

                if ( !pInnerContext )
                {
                    ConfigCtx::getCurConfigCtx()->log_error( "Failed to import servlet mapping for %s",
                                                pMappingNode->getChildValue( "url-pattern" ) );
                }
            }
        }
    }
}
static int getRedirectCode( const XmlNode *pContextNode, int &code,
                            const char *pLocation )
{
    code = ConfigCtx::getCurConfigCtx()->getLongValue( pContextNode, "externalRedirect", 0, 1, 1 );

    if ( code == 0 )
        --code;
    else
    {
        int orgCode = ConfigCtx::getCurConfigCtx()->getLongValue( pContextNode, "statusCode", 0, 505, 0 );

        if ( orgCode )
        {
            code = HttpStatusCode::codeToIndex( orgCode );

            if ( code == -1 )
            {
                ConfigCtx::getCurConfigCtx()->log_warn( "Invalid status code %d, use default: 302!", orgCode );
                code = SC_302;
            }
        }
    }

    if ( ( code == -1 ) && ( code >= SC_300 ) && ( code < SC_400 ) )
    {
        if ( ( pLocation == NULL ) || ( *pLocation == 0 ) )
        {
            ConfigCtx::getCurConfigCtx()->log_error( "Destination URI must be specified!" );
            return -1;
        }
    }

    return 0;
}

int HttpVHost::configContext( const XmlNode *pContextNode )
{
    const char *pUri = NULL;
    const char *pLocation = NULL;
    const char *pHandler = NULL;
    bool allowBrowse = false;
    int match;
    pUri = ConfigCtx::getCurConfigCtx()->getTag( pContextNode, "uri", 1 );
    if ( pUri == NULL )
        return -1;

    ConfigCtx currentCtx( "context", pUri );
    //int len = strlen( pUri );

    const char *pValue = ConfigCtx::getCurConfigCtx()->getTag( pContextNode, "type" );

    if ( pValue == NULL )
        return -1;

    int role = HandlerType::ROLE_RESPONDER;
    int type = HandlerType::getHandlerType( pValue, role );

    if ( ( type == HandlerType::HT_END ) || ( role != HandlerType::ROLE_RESPONDER ) )
    {
        currentCtx.log_errorInvalTag( "type", pValue );
        return -1;
    }

    pLocation = pContextNode->getChildValue( "location" );
    pHandler = pContextNode->getChildValue( "handler" );

    char achHandler[256] = {0};

    if ( pHandler )
    {
        if ( ConfigCtx::getCurConfigCtx()->expandVariable( pHandler, achHandler, 256 ) < 0 )
        {
            currentCtx.log_notice( "add String is too long for scripthandler, value: %s", pHandler );
            return -1;
        }

        pHandler = achHandler;
    }

    allowBrowse = ConfigCtx::getCurConfigCtx()->getLongValue( pContextNode, "allowBrowse", 0, 1, 1 );

    match = ( strncasecmp( pUri, "exp:", 4 ) == 0 );

    if ( ( *pUri != '/' ) && ( !match ) )
    {
        currentCtx.log_error( "URI must start with '/' or 'exp:', invalid URI - %s", pUri );
        return -1;
    }

    HttpContext *pContext = NULL;

    switch( type )
    {
    case HandlerType::HT_JAVAWEBAPP:
        allowBrowse = true;
        pContext = importWebApp( pUri, pLocation, pHandler,
                                 allowBrowse );
        break;
    case HandlerType::HT_RAILS:
        allowBrowse = true;
        pContext = configRailsContext( pContextNode, pUri, pLocation );
        break;
    case HandlerType::HT_REDIRECT:
        if ( configRedirectContext( pContextNode, pLocation, pUri, pHandler, allowBrowse, 
                               match, type ) == -1 )
            return -1;
        break;
    default:
        pContext = configContext( pUri, type, pLocation,
                                  pHandler, allowBrowse );
    }

    if ( pContext )
    {
        if ( configContextAuth( pContext, pContextNode ) == -1 )
            return -1;
        return pContext->config(getRewriteMaps(), pContextNode, type );
    }

    return -1;
}

int HttpVHost::configRedirectContext( const XmlNode *pContextNode, const char *pLocation, 
          const char *pUri, const char *pHandler, bool allowBrowse, int match, 
          int type )
{
    HttpContext *pContext;
    int code;
    if ( getRedirectCode( pContextNode, code, pLocation ) == -1 )
        return -1;

    if ( !pLocation )
        pLocation = "";

    pContext = addContext( match, pUri, type, pLocation,
                            pHandler, allowBrowse );

    if ( pContext )
    {
        pContext->redirectCode( code );
        pContext->setConfigBit( BIT_ALLOW_OVERRIDE, 1 );
    }
    return 0;
}


static int compareContext(const void *p1, const void *p2)
{
    return strcmp( (*((XmlNode**)p1))->getChildValue("uri", 1),
                   (*((XmlNode**)p2))->getChildValue("uri", 1) );
}

int HttpVHost::configVHContextList( const XmlNode *pVhConfNode, const XmlNodeList *pModuleList )
{
    addContext( "/", HandlerType::HT_NULL,
                        getDocRoot()->c_str(), NULL, 1 );

    setGlobalMatchContext( 1 );

    const XmlNode *p0 = pVhConfNode->getChild( "contextList", 1 );

    XmlNodeList *pList = (XmlNodeList *)(p0->getChildren( "context" ));

    if ( pList )
    {
        pList->sort( compareContext );
        XmlNodeList::const_iterator iter;

        for( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            configContext( *iter );
        }
    }

    /***
     * Since modulelist may contain URIs, and these URIs may be new for the current conetxts,
     * so they should be added as new one to the current contexts just beofre call contextInherit()
     * The benifit of doing that is the children inheritance will be good
     */
    if (pModuleList)
        checkAndAddNewUriFormModuleList(pModuleList);
    
    contextInherit();
    return 0;
}

void HttpVHost::checkAndAddNewUriFormModuleList(const XmlNodeList *pModuleList)
{
    XmlNodeList::const_iterator iter0;
    for( iter0 = pModuleList->begin(); iter0 != pModuleList->end(); ++iter0 )
    {
        const XmlNode *p0 = (*iter0)->getChild( "urlfilterlist", 1 );
        const XmlNodeList *pfilterList = p0->getChildren( "urlfilter" );
        if ( pfilterList )
        {
            XmlNode *pNode = NULL;
            XmlNodeList::const_iterator iter;
            const char *pValue;
            HttpContext *pContext;
            int bRegex = 0;
            
            for( iter = pfilterList->begin(); iter != pfilterList->end(); ++iter )
            {
                pNode = *iter;
                pValue = pNode->getChildValue("uri", 1);
                if (!pValue || strlen(pValue) == 0)
                    break;
                
                if (pValue[0] != '/')
                    bRegex = 1;
                
                pContext = getContext(pValue, bRegex);
                if ( pContext == NULL )
                {
                    char achVPath[MAX_PATH_LEN];
                    char achRealPath[MAX_PATH_LEN];
                    strcpy( achVPath, "$DOC_ROOT" );
                    strcat( achVPath, pValue );
                    ConfigCtx::getCurConfigCtx()->getAbsoluteFile( achRealPath, achVPath );
                    pContext = addContext( pValue, HandlerType::HT_NULL, achRealPath, NULL, 1 );
                    if ( pContext == NULL )
                    {
                        LOG_ERR( ( "[%s] checkAndAddNewUriFormModuleList try to add the context [%s] failed.",
                                   LogIdTracker::getLogId(), pValue) );
                        break;
                    }
                }
            }
        }
    }
}

//only save the param, not parse it and do not inherit the sessionhooks
int HttpVHost::configVHModuleUrlFilter1( lsi_module_t *pModule, const XmlNodeList *pfilterList )
{
    int ret = 0;
    XmlNode *pNode = NULL;
    XmlNodeList::const_iterator iter;
    const char *pValue;
    HttpContext *pContext;
    int bRegex = 0;
    
    
    for( iter = pfilterList->begin(); iter != pfilterList->end(); ++iter )
    {
        pNode = *iter;
        pValue = pNode->getChildValue("uri", 1);
        if (!pValue || strlen(pValue) == 0)
        {
            ret = -1;
            break;
        }
        
        if (pValue[0] != '/')
            bRegex = 1;
        
        pContext = getContext(pValue, bRegex);
        assert(pContext != NULL && "this Context should have already been added to context by checkAndAddNewUriFormModuleList().\n");
        
        HttpContext *parentContext = ((HttpContext *)pContext->getParent());
        ModuleConfig *parentConfig = parentContext->getModuleConfig();
        assert(parentConfig != NULL && "parentConfig should not be NULL here.");
        
        if (pContext->getModuleConfig() == NULL || pContext->getSessionHooks() == NULL )
        {
            pContext->setInternalSessionHooks( parentContext->getSessionHooks() );
            pContext->setModuleConfig( parentConfig, 0 );
        }
        
        //Check the ModuleConfig of this context is own or just a pointer
        lsi_module_config_t *module_config;
        if (pContext->isModuleConfigOwn())
        {
            module_config = pContext->getModuleConfig()->get( MODULE_ID( pModule ));
            ModuleConfig::saveConfig(pNode, pModule, module_config);
        }
        else
        {
            module_config = new lsi_module_config_t;
            memcpy(module_config, pContext->getModuleConfig()->get(MODULE_ID( pModule )), sizeof(lsi_module_config_t));
            module_config->own_data_flag = 0;
            module_config->sparam = NULL;
            ModuleConfig::saveConfig(pNode, pModule, module_config);
       
            pContext->setOneModuleConfig( MODULE_ID( pModule ), module_config );
            delete module_config;
        }
    }
    return ret;
}


lsi_module_config_t *parseModuleConfigParam(lsi_module_t *pModule, const HttpContext *pContext)
{
    lsi_module_config_t *config = ((HttpContext *)pContext)->getModuleConfig()->get(MODULE_ID( pModule ));
    if (config->own_data_flag == 2 || pContext->getParent() == NULL )
        return config;
        
    
    //HttpSessionHooks *parentHooks = ((HttpContext *)pContext->getParent())->getSessionHooks();
    ModuleConfig *parentModuleConfig = ((HttpContext *)pContext->getParent())->getModuleConfig();

    void *init_config = parseModuleConfigParam(pModule, pContext->getParent())->config;

    if (config->filters_enable == -1)
        config->filters_enable = parentModuleConfig->get(MODULE_ID( pModule ))->filters_enable;
    
    if (config->own_data_flag == 1)
    {
        assert(config->sparam != NULL);
        config->config = pModule->_config_parser->_parse_config(config->sparam->c_str(), init_config, LSI_CONTEXT_LEVEL, pContext->getURI());
        delete config->sparam;
        config->sparam = NULL;
        config->own_data_flag = 2;
    }
    else
        config->config = init_config;
    
    ((HttpContext *)pContext)->initExternalSessionHooks();
    ModuleManager::getInstance().updateHttpApiHook(((HttpContext *)pContext)->getSessionHooks(), ((HttpContext *)pContext)->getModuleConfig(),
                                                   MODULE_ID( pModule ));
    
    return config;
}

int HttpVHost::configVHModuleUrlFilter2( lsi_module_t *pModule, const XmlNodeList *pfilterList )
{
    if ( !pModule || !pModule->_config_parser || !pModule->_config_parser->_parse_config)
        return 0;
    
    int ret = 0;
    XmlNode *pNode = NULL;
    XmlNodeList::const_iterator iter;
    const char *pValue;
    HttpContext *pContext;
    int bRegex = 0;
    
    for( iter = pfilterList->begin(); iter != pfilterList->end(); ++iter )
    {
        pNode = *iter;
        pValue = pNode->getChildValue("uri", 1);
        if (!pValue || strlen(pValue) == 0)
        {
            ret = -1;
            break;
        }
        
        if (pValue[0] != '/')
            bRegex = 1;
        
        pContext = getContext(pValue, bRegex);
        parseModuleConfigParam(pModule, (const HttpContext *)pContext);
    }
    return ret;
}


int HttpVHost::configModuleConfigInContext( const XmlNode *pContextNode, int saveParam )
{
    const char *pUri = NULL;
    const char *pHandler = NULL;
    const char *pParam = NULL;
    const char *pType = NULL;
    
    pUri = pContextNode->getChildValue( "uri", 1 );
    pHandler = pContextNode->getChildValue( "handler" );
    pParam = pContextNode->getChildValue( "param" );
    pType = pContextNode->getChildValue( "type" );
    
    if (!pUri || !pType || !pParam || !pHandler ||
        strlen(pUri)== 0 || strlen(pType) == 0 ||
        strlen(pParam) == 0 || strlen(pHandler) == 0)
        return -1;
    
    if (strcmp(pType, "module") != 0)
        return -1;
    
    int bRegex = 0;
    if (pUri[0] != '/')
        bRegex = 1;

    HttpContext *pContext = getContext(pUri, bRegex);
    if (!pContext)
        return -1;
    
    //find the module and check if exist, if NOT just return
    ModuleManager::iterator moduleIter;
    moduleIter = ModuleManager::getInstance().find(pHandler);
    if (moduleIter == ModuleManager::getInstance().end())
        return -1;
    
    lsi_module_t *pModule = moduleIter.second()->getModule();
    lsi_module_config_t *config = pContext->getModuleConfig()->get(MODULE_ID( pModule ));
    
    if (!pModule->_config_parser || !pModule->_config_parser->_parse_config)
        return -1;
    
    
    if(saveParam)
    {
        lsi_module_config_t *module_config;
        int toBeDel = 0;
        if (pContext->isModuleConfigOwn())
        {
            module_config = config;
        }
        else
        {
            module_config = new lsi_module_config_t;
            toBeDel = 1;
            memcpy(module_config, config, sizeof(lsi_module_config_t));
            module_config->own_data_flag = 0;
            module_config->sparam = NULL;
        }
        
        if (!module_config->sparam)
            module_config->sparam = new AutoStr2;

        if (module_config->sparam)
        {
            module_config->own_data_flag =1 ;
            module_config->sparam->append("\n", 1);
            module_config->sparam->append(pParam, strlen(pParam));
        }
        if (toBeDel)
        {
            pContext->setOneModuleConfig( MODULE_ID( pModule ), module_config );
            delete module_config;
        }
    }
    else
    {
        parseModuleConfigParam(pModule, (const HttpContext *)pContext);
    }
    return 0;
}

int HttpVHost::parseVHModulesParams( const XmlNode *pVhConfNode, const XmlNodeList *pModuleList, int saveParam )
{
    int ret = 0;
    const XmlNode *p0;
    XmlNode *pNode = NULL;
    const char *moduleName;
    XmlNodeList::const_iterator iter;
    if (pModuleList)
    {
        for( iter = pModuleList->begin(); iter != pModuleList->end(); ++iter )
        {
            pNode = *iter;
            p0 = pNode->getChild( "urlfilterlist", 1 );
            
            const XmlNodeList *pfilterList = p0->getChildren( "urlfilter" );
            if ( pfilterList )
            {
                moduleName = pNode->getChildValue("name", 1);
                if (!moduleName)
                {
                    ret = -1;
                    break;
                }
                
                ModuleManager::iterator moduleIter;
                moduleIter = ModuleManager::getInstance().find(moduleName);
                if (moduleIter == ModuleManager::getInstance().end())
                {
                    ret = -2;
                    break;
                }
                
                if (saveParam)
                    configVHModuleUrlFilter1(moduleIter.second()->getModule(), pfilterList);
                else
                    configVHModuleUrlFilter2(moduleIter.second()->getModule(), pfilterList);
            }
        }
    }
    
    //now for the contextlist part to find out the modulehandler param
    p0 = pVhConfNode->getChild( "contextList", 1 );
    XmlNodeList *pList = (XmlNodeList *)(p0->getChildren( "context" ));

    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            configModuleConfigInContext( *iter, saveParam );
        }
    }
    
    return ret;
}

/****
 * COMMENT: About the context under a VHost
 * In the XML of the VHost, there are Modulelist and ContextList which may contain
 * uri which relatives with module, that will cause the context of this uri having its own internal_ctx,
 * before all the context inherit, I need to malloc the internal_ctx for these contexts.
 */
int HttpVHost::config( const XmlNode *pVhConfNode)
{
    int iChrootlen = 0;
    if ( HttpGlobals::s_psChroot )
        iChrootlen = HttpGlobals::s_psChroot->len();

    assert( pVhConfNode != NULL );
    if ( configBasics( pVhConfNode, iChrootlen ) != 0 )
        return 1;

    updateUGid( LogIdTracker::getLogId(), getDocRoot()->c_str() );

    const XmlNode *p0;
    HttpContext *pRootContext = &getRootContext();
    initErrorLog( pVhConfNode, 0 );
    configSecurity( pVhConfNode );
    {
        ConfigCtx currentCtx( "epsr" );
        ExtAppRegistry::configExtApps( pVhConfNode, this );
    }

    getRootContext().setParent(
        &HttpServer::getInstance().getServerContext() );
    initAccessLog( pVhConfNode, 0 );
    configVHScriptHandler( pVhConfNode );
    configAwstats( ConfigCtx::getCurConfigCtx()->getVhDomain()->c_str(), ConfigCtx::getCurConfigCtx()->getVhAliases()->len(), 
                   pVhConfNode );
    p0 = pVhConfNode->getChild( "vhssl" );

    if ( p0 )
    {
        ConfigCtx currentCtx( "ssl" );
        SSLContext *pSSLCtx = new SSLContext( SSLContext::SSL_ALL );
        if ( pSSLCtx->config( p0 ) )
            setSSLContext( pSSLCtx );
        else
            delete pSSLCtx;
    }

    p0 = pVhConfNode->getChild( "expires" );

    if ( p0 )
    {
        getExpires().config( p0,&(&HttpServer::getInstance())->getServerContext().getExpires(),
                                     pRootContext );
        HttpGlobals::getMime()->getDefault()->getExpires()->copyExpires(
            (&HttpServer::getInstance())->getServerContext().getExpires() );
        const char *pValue = p0->getChildValue( "expiresByType" );

        if ( pValue && ( *pValue ) )
        {
            pRootContext->initMIME();
            pRootContext->getMIME()->setExpiresByType( pValue,
                    HttpGlobals::getMime(), LogIdTracker::getLogId() );
        }
    }

    p0 = pVhConfNode->getChild( "rewrite" );

    if ( p0 )
    {
        configRewrite( p0 );
    }

    configIndexFile( pVhConfNode, (&HttpServer::getInstance())->getIndexFileList(), 
                MainServerConfig::getInstance().getAutoIndexURI() );

    p0 = pVhConfNode->getChild( "customErrorPages", 1 );
    if ( p0 )
    {
        ConfigCtx currentCtx( "errorpages" );
        pRootContext->configErrorPages(p0 );     
    }

    pRootContext->inherit( NULL );
    
    
    p0 = pVhConfNode->getChild( "modulelist", 1 );
    const XmlNodeList *pModuleList = p0->getChildren( "module" );
    if ( pModuleList )
    {
        ModuleConfig *pConfig = new ModuleConfig;
        pConfig->init(ModuleManager::getInstance().getModuleCount());
        pConfig->inherit(ModuleManager::getGlobalModuleConfig());
        ModuleConfig::parseConfigList(pModuleList, pConfig, LSI_VHOST_LEVEL, this->getName());
        pRootContext->setModuleConfig(pConfig, 1);
    }
    else
    {
        pRootContext->setModuleConfig(((HttpContext *)pRootContext->getParent())->getModuleConfig(), 0);
    }
    
    ModuleConfig *pModuleConfig = pRootContext->getModuleConfig();
    
    if (pModuleConfig->isMatchGlobal())
    {
        HttpSessionHooks *parentHooks = ((HttpContext *)pRootContext->getParent())->getSessionHooks();
        pRootContext->setInternalSessionHooks(parentHooks);
    }
    else
    {
        pRootContext->initExternalSessionHooks();
        ModuleManager::getInstance().inheritHttpApiHooks(pRootContext->getSessionHooks(), pModuleConfig);
    }
    
    //Here, checking pModuleList because modulelist may have new context
    configVHContextList( pVhConfNode, pModuleList );
    configVHWebsocketList(pVhConfNode );

    //check again for "urlfilterList" part and "contextList" part,
    //First, just save all params and filterEnbale value
    parseVHModulesParams( pVhConfNode, pModuleList, 1 );
    
    //Now since all params and filterEnbale value saved,
    //inheirt and parse,
    //finally, release the temp aurostr2
    parseVHModulesParams( pVhConfNode, pModuleList, 0 );
    
    return 0;
}

int HttpVHost::configVHScriptHandler( const XmlNode *pVhConfNode )
{
    int configType = 0; //old type
    const XmlNode *p0 = pVhConfNode->getChild( "scriptHandlerList" );

    if ( p0 == NULL )
    {
        configType = 1;
        p0 = pVhConfNode->getChild( "scriptHandler" );
    }

    if ( p0 == NULL )
        return 0;

    if ( configType == 0 )
    {
        const XmlNodeList *pList = p0->getChildren( "scriptHandler" );

        if ( !pList || pList->size() == 0 )
            return 0;

        getRootContext().initMIME();
        HttpMime::configScriptHandler1( this, pList, getMIME() );
    }
    else
    {
        const XmlNodeList *pList = p0->getChildren( "add" );

        if ( pList && pList->size() > 0 )
        {
            getRootContext().initMIME();
            HttpMime::configScriptHandler2( this, pList, getMIME() );
        }
    }

    return 0;
}
const HttpHandler * HttpVHost::isHandlerAllowed( const HttpHandler *pHdlr, int type, 
                                 const char *pHandler )
{
    const HttpVHost *pVHost1 = ( ( const ExtWorker * ) pHdlr )->
                                getConfigPointer()->getVHost();

    if ( pVHost1 )
    {
        if ( this != pVHost1 )
        {
            LOG_ERR( ( "[%s] Access to handler [%s:%s] from [%s] is denied!",
                        LogIdTracker::getLogId(),
                        HandlerType::getHandlerTypeString( type ),
                        pHandler, getName() ) );
            return  NULL;
        }
    } 
    return pHdlr;
}
void HttpVHost::configVHChrootMode( const XmlNode *pNode )
{
    int val = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "chrootMode", 0, 2, 0 );
    const char *pValue = pNode->getChildValue( "chrootPath" );
    int len = 0;
    if ( HttpGlobals::s_psChroot != NULL )
        len = HttpGlobals::s_psChroot->len();

    if ( pValue )
    {
        char achPath[4096];
        char *p  = achPath;
        int  ret = ConfigCtx::getCurConfigCtx()->getAbsoluteFile( p, pValue );

        if ( ret )
            val = 0;
        else
        {
            char *p1 = p + len;

            if ( restrained() &&
                    strncmp( p1, getVhRoot()->c_str(),
                             getVhRoot()->len() ) != 0 )
            {
                ConfigCtx::getCurConfigCtx()->log_error( "Chroot path % must be inside virtual host root %s",
                                            p1, getVhRoot()->c_str() );
                val = 0;
            }

            setChroot( p );
        }

    }
    else
        val = 0;

    setChrootMode( val );

}
HttpVHost *HttpVHost::configVHost( const XmlNode *pNode, const char *pName,
        const char *pDomain, const char *pAliases, const char *pVhRoot,
        const XmlNode *pConfigNode)
{
    while( 1 )
    {
        if ( strcmp( pName, DEFAULT_ADMIN_SERVER_NAME ) == 0 )
        {
            ConfigCtx::getCurConfigCtx()->log_error( "invalid <name>, %s is used for the "
                                        "administration server, ignore!",  pName );
            break;
        }

        ConfigCtx currentCtx( "vhost",  pName );

        if ( !pVhRoot )
            pVhRoot = ConfigCtx::getCurConfigCtx()->getTag( pNode, "vhRoot" );

        if ( pVhRoot == NULL )
        {
            break;
        }

        if ( ConfigCtx::getCurConfigCtx()->getValidChrootPath( pVhRoot, "vhost root" ) != 0 )
            break;

        if ( !pDomain )
            pDomain = pName;

        ConfigCtx::getCurConfigCtx()->setVhDomain( pDomain );

        if ( !pAliases )
            pAliases = "";

        ConfigCtx::getCurConfigCtx()->setVhAliases( pAliases );
        HttpVHost *pVHnew = new HttpVHost( pName );
        assert( pVHnew != NULL );
        pVHnew->setVhRoot( ConfigCtx::getCurConfigCtx()->getVhRoot() );
        pVHnew->followSymLink( ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "allowSymbolLink", 0, 2,
                               HttpServerConfig::getInstance().getFollowSymLink() ) );
        pVHnew->enableScript( ConfigCtx::getCurConfigCtx()->getLongValue(
                                  pNode, "enableScript", 0, 1, 1 ) );
        pVHnew->serverEnabled( ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "enabled", 0, 1, 1 ) );
        pVHnew->restrained( ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "restrained", 0, 1, 0 ) );

        pVHnew->setUidMode( ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "setUIDMode", 0, 2, 0 ) );

        pVHnew->setMaxKAReqs( ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "maxKeepAliveReq", 0, 32767,
                              HttpServerConfig::getInstance().getMaxKeepAliveRequests() ) );

        pVHnew->setSmartKA( ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "smartKeepAlive", 0, 1,
                            HttpServerConfig::getInstance().getSmartKeepAlive() ) );

        pVHnew->getThrottleLimits()->config( pNode,
                                             ThrottleControl::getDefault(), &currentCtx );

        
        
        
        
        if ( pVHnew->config( pConfigNode ) == 0 )
        {
            return pVHnew;
        }

        delete pVHnew;
        currentCtx.log_error( "configuration failed!" );

        break;
    }

    return NULL;
}
HttpVHost *HttpVHost::configVHost( XmlNode *pNode)
{
    XmlNode *pVhConfNode = NULL;
    HttpVHost *pVHost = NULL;
    bool gotConfigFile = false;

    while( 1 )
    {
        const char *pName = ConfigCtx::getCurConfigCtx()->getTag( pNode, "name", 1 );

        if ( pName == NULL )
        {
            break;
        }

        //m_sVhName.setStr( pName );
        ConfigCtx::getCurConfigCtx()->setVhName( pName );
        const char *pVhRoot = ConfigCtx::getCurConfigCtx()->getTag( pNode, "vhRoot" );

        if ( pVhRoot == NULL )
        {
            break;
        }

        char achVhConf[MAX_PATH_LEN];

        if ( ConfigCtx::getCurConfigCtx()->getValidChrootPath( pVhRoot, "vhost root" ) != 0 )
            break;

        const char *pConfFile = pNode->getChildValue( "configFile" );

        if ( pConfFile != NULL )
        {
            if ( ConfigCtx::getCurConfigCtx()->getValidFile( achVhConf, pConfFile, "vhost config" ) == 0 )
            {
                pVhConfNode = ConfigCtx::getCurConfigCtx()->parseFile( achVhConf, "virtualHostConfig" );

                if ( pVhConfNode == NULL )
                {
                    ConfigCtx::getCurConfigCtx()->log_error( "cannot load configure file - %s !", achVhConf );
                }
                else
                    gotConfigFile = true;
            }
        }

        if ( !pVhConfNode )
            pVhConfNode = pNode;


        const char *pDomain  = pVhConfNode->getChildValue( "domainName" );
        const char *pAliases = pVhConfNode->getChildValue( "hostAliases" );
        pVHost = HttpVHost::configVHost( pNode, pName, pDomain, pAliases, pVhRoot, 
                                         pVhConfNode );
        break;
    }

    if ( gotConfigFile )
        delete pVhConfNode;

    return pVHost;
}
int HttpVHost::checkDeniedSubDirs( const char *pUri, const char *pLocation )
{
    int len = strlen( pLocation );

    if ( * ( pLocation + len - 1 ) != '/' )
        return 0;

    DeniedDir::iterator iter = HttpGlobals::getDeniedDir()->lower_bound( pLocation );
    iter = HttpGlobals::getDeniedDir()->next_included( iter, pLocation );

    while( iter != HttpGlobals::getDeniedDir()->end() )
    {
        const char *pDenied = HttpGlobals::getDeniedDir()->getPath( iter );
        const char *pSubDir = pDenied + len;
        char achNewURI[MAX_URI_LEN + 1];
        int n = safe_snprintf( achNewURI, MAX_URI_LEN, "%s%s", pUri, pSubDir );

        if ( n == MAX_URI_LEN )
        {
            ConfigCtx::getCurConfigCtx()->log_error( "URI is too long when add denied dir %s for context %s",
                                        pDenied, pUri );
            return -1;
        }

        if ( getContext( achNewURI ) )
            continue;

        if ( !addContext( achNewURI, HandlerType::HT_NULL, pDenied,
                                 NULL, false ) )
        {
            ConfigCtx::getCurConfigCtx()->log_error( "Failed to block denied dir %s for context %s",
                                        pDenied, pUri );
            return -1;
        }

        iter = HttpGlobals::getDeniedDir()->next_included( iter, pLocation );
    }

    return 0;
}


