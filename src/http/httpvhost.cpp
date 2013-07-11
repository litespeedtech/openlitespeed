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
#include <http/datetime.h>
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

#include <log4cxx/logrotate.h>
#include <log4cxx/logger.h>
#include <log4cxx/layout.h>
#include <sslpp/sslcontext.h>
#include <util/accesscontrol.h>
#include <util/hashstringmap.h>
#include <util/ni_fio.h>
#include <util/stringlist.h>
#include <util/stringtool.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <util/ssnprintf.h>


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



void  HttpVHost::logAccess( HttpConnection * pConn ) const
{
    if ( m_pAccessLog )
        m_pAccessLog->log( pConn );
    else
        HttpLog::logAccess( m_sName.c_str(), m_sName.len(), pConn );
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

void HttpVHost::logBytes( long bytes )
{
    char achBuf[80];
    int n = safe_snprintf( achBuf, 80, "%ld %ld .\n", DateTime::s_curTime, bytes );
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

extern const HttpHandler * getHandlerAllowed( const HttpVHost * pVHost,
                         int type, const char * pHandler );



HttpContext * HttpVHost::setContext( HttpContext * pContext,
    const char * pUri, int type, const char * pLocation, const char * pHandler,
    int allowBrowse, int match )
{
    const HttpHandler * pHdlr = getHandlerAllowed( this, type, pHandler );
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



HttpContext * HttpVHost::addContext( int match, const char * pUri, int type,
        const char * pLocation, const char * pHandler, int allowBrowse )
{
    if ( !pLocation && !pUri )
    {
        return NULL;
    }
    if ( match )
    {
        char achTmp[] = "/";
        HttpContext * pOld = getContext( achTmp );
        if ( !pOld )
        {
            return NULL;
        }
        HttpContext * pContext;
        pContext = (HttpContext *)pOld->findMatchContext( pUri );
        if ( pContext )
        {
            setContext( pContext, pUri, type, pLocation, pHandler, allowBrowse, match);
        }
        else
        {
            pContext = new HttpContext();
            setContext( pContext, pUri, type, pLocation, pHandler, allowBrowse, match);
            pOld->addMatchContext( pContext );
        }
        return pContext;
    }
    else
    {
        HttpContext * pOld = getContext( pUri );
        if ( pOld )
        {
            setContext( pOld, pUri, type, pLocation, pHandler, allowBrowse, 0);
            return pOld;
        }
        return addContext( pUri, type, pLocation, pHandler, allowBrowse );
    }
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
