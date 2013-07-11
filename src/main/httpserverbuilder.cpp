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
#include "httpserverbuilder.h"

#include <http/accesslog.h>
#include <http/awstats.h>
#include <http/connlimitctrl.h>
#include <http/denieddir.h>
#include <http/handlerfactory.h>
#include <http/handlertype.h>
#include <http/hotlinkctrl.h>
#include <http/htauth.h>
#include <http/htpasswd.h>
#include <http/httpiolink.h>
#include <http/httpcontext.h>
#include <http/httpdefs.h>
#include <http/httpglobals.h>
#include <http/httplistener.h>
#include <http/httplog.h>
#include <http/httpmime.h>
#include <http/httpserverconfig.h>
#include <http/httpserverversion.h>
#include <http/httpstatuscode.h>
#include <http/httpvhost.h>
#include <http/phpconfig.h>
#include <http/platforms.h>
#include <http/rewriterule.h>
#include <http/rewriteengine.h>
#include <http/staticfilecachedata.h>
#include <http/stderrlogger.h>
#include <http/vhostmap.h>

#include <socket/gsockaddr.h>
#include <sslpp/sslengine.h>
#include <sslpp/sslcontext.h>

#include <main/httpserver.h>
#include <main/serverinfo.h>

#include <extensions/registry/extappregistry.h>
#include <extensions/cgi/cgidworker.h>
#include <extensions/cgi/lscgiddef.h>
#include <extensions/cgi/cgidconfig.h>
#include <extensions/cgi/suexec.h>
#include <extensions/fcgi/fcgiapp.h>
#include <extensions/fcgi/fcgiappconfig.h>
#include <extensions/loadbalancer.h>

#include <util/accesscontrol.h>
#include <util/daemonize.h>
#include <util/gpath.h>
#include <util/ni_fio.h>
#include <util/rlimits.h>
#include <util/stringlist.h>
#include <util/stringtool.h>
#include <util/sysinfo/nicdetect.h>
#include <util/sysinfo/systeminfo.h>
#include <util/xmlnode.h>
#include <util/vmembuf.h>

#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <util/ssnprintf.h>
#include <http/httpglobals.h>

#include "plainconf.h"

#define MAX_URI_LEN  1024

static const char * MISSING_TAG = "[%s] missing <%s>";
static const char * MISSING_TAG_IN = "[%s] missing <%s> in <%s>";
static const char * INVAL_TAG = "[%s] <%s> is invalid: %s";
static const char * INVAL_TAG_IN = "[%s] invalid tag <%s> within <%s>!";
static const char * INVAL_PATH = "[%s] Path for %s is invalid: %s";
static const char * INACCESSIBLE_PATH = "[%s] Path for %s is not accessible: %s";


static char  s_sLogId[128] = "config" ;
static int   s_iIdLen = 6;
inline const char * getLogId()             {
    return s_sLogId;
}
inline void setLogId( const char * pId )
{
    s_iIdLen = safe_snprintf( s_sLogId, sizeof( s_sLogId ) -1, "%s", pId);
    s_sLogId[ s_iIdLen ] = 0;
}
inline void appendLogId( const char * pId )
{
    s_iIdLen += safe_snprintf( s_sLogId + s_iIdLen, sizeof( s_sLogId ) -1 - s_iIdLen,
                          "%s", pId );
    s_sLogId[ s_iIdLen ] = 0;
}

class LogIdTracker
{
    AutoStr m_sOldId;
public:
    LogIdTracker( const char * pNewId )
    {
        m_sOldId = getLogId();
        setLogId( pNewId );
    }
    LogIdTracker()
    {
        m_sOldId = getLogId();
    }
    ~LogIdTracker()
    {
        setLogId( m_sOldId.c_str() );
    }
};



static long long getLongValue( const char * pValue, int base = 10 )
{
    long long l = strlen( pValue );
    long long m = 1;
    char ch = *( pValue + l - 1 );
    if (  ch == 'G' || ch == 'g' )
    {
        m = 1024 * 1024 * 1024;
    }
    else if (  ch == 'M' || ch == 'm' )
    {
        m = 1024 * 1024;
    }
    else if ( ch == 'K' || ch == 'k' )
    {
        m = 1024;
    }
    return strtoll(pValue, (char **)NULL, base) * m;
}

static const char * getTag( const XmlNode * pNode, const char * pName )
{
    const char * pRet = pNode->getChildValue( pName );
    if ( !pRet )
    {
        LOG_ERR(( MISSING_TAG_IN, getLogId(), pName, pNode->getName() ));
    }
    return pRet;
}
static long long getLongValue( const XmlNode * pNode, const char * pTag,
            long long min, long long max, long long def, int base = 10 )
{
    const char * pValue = pNode->getChildValue( pTag );
    long long val;
    if ( pValue )
    {
        val = getLongValue( pValue, base );
        if (( max == INT_MAX )&&( val > max ))
            val = max;
        if (((min != LLONG_MIN)&&(val < min))||
            ((max != LLONG_MAX)&&(val > max )) )
        {
            LOG_WARN(( "[%s] invalid value of <%s>:%s, use default=%ld",
                    getLogId(), pTag, pValue, def ));
            return def;
        }

        return val;
    }
    else
        return def;

}


static const char * getAccessFileName( const XmlNode * pNode, const char * pDefault )
{
    const char * pValue = pNode->getChildValue( "accessFileName" );
    if ( !pValue )
        return pDefault;
    else
    {
        if (( *pValue != '.' )||( strchr( pValue, '/' ))|| strlen( pValue ) > 14 )
        {
            LOG_WARN(( "[%s] Access File Name must be less than 14 characters long, "
                       "start with '.' and not contain '/', use default name.",
                       getLogId() ));
            return pDefault;
        }
    }
    return pValue;
}

static void initExpires( const XmlNode * pExpires, ExpiresCtrl * pCtrl,
                         const ExpiresCtrl * pDefault, HttpContext * pContext )
{
    const char * pValue;
    if ( !pDefault )
        pDefault = pCtrl;
    pValue = pExpires->getChildValue( "enableExpires" );
    char enable = pDefault->isEnabled();
    if ( pValue )
    {
        enable = getLongValue( pExpires, "enableExpires" , 0, 1, enable );
        if ( pContext )
            pContext->setConfigBit( BIT_ENABLE_EXPIRES, 1 );
    }
    pCtrl->enable( enable );
    pValue = pExpires->getChildValue( "expiresDefault" );
    if ( pValue )
    {
        pCtrl->parse( pValue );
        if ( pContext )
            pContext->setConfigBit( BIT_EXPIRES_DEFAULT, 1 );
    }
    else
    {
        pCtrl->setBase( pDefault->getBase() );
        pCtrl->setAge( pDefault->getAge() );
    }
}


#include <http/clientcache.h>
static void setThrottleLimit( ThrottleLimits * pLimits, const XmlNode * pNode1,
                              const ThrottleLimits * pDefault)
{
    int outlimit = -1;
    int inlimit = -1;
    if (( pDefault == pLimits )||( pDefault->getOutputLimit() != INT_MAX ))
    {
        outlimit = getLongValue( pNode1, "outBandwidth", -1, INT_MAX, -1);
        inlimit = getLongValue( pNode1, "inBandwidth", -1, INT_MAX, -1);
        if ( inlimit == -1 )
            inlimit = outlimit;
        if ( outlimit < 0 )
            outlimit = inlimit = getLongValue( pNode1, "throttleLimit", 0, INT_MAX, -1);
        if (( outlimit > 0 )&&( outlimit < INT_MAX - THROTTLE_UNIT ))
        {
            outlimit =
                (( outlimit + THROTTLE_UNIT - 1 ) / THROTTLE_UNIT) * THROTTLE_UNIT;
            if ( inlimit < INT_MAX - 1024 )
                inlimit = ( inlimit + 1023 ) & ~1023;
        }
        if ( outlimit == -1 )
        {
            if ( pDefault != pLimits )
            {
                outlimit = pDefault->getOutputLimit();
                inlimit = pDefault->getInputLimit();
            }
            else
            {
                outlimit = INT_MAX;
                inlimit = INT_MAX;
            }
        }
    }
    if ( outlimit <= 0 )
        outlimit = INT_MAX;
    if ( inlimit <= 0 )
        inlimit = INT_MAX;
    pLimits->setOutputLimit( outlimit );
    pLimits->setInputLimit( inlimit );
    int limit = getLongValue( pNode1, "dynReqPerSec", 0, INT_MAX,
                              pDefault->getDynReqLimit() );
    if ( limit == 0 )
        limit = INT_MAX;
    pLimits->setDynReqLimit( limit );
    limit = getLongValue( pNode1, "staticReqPerSec", 0, INT_MAX,
                          pDefault->getStaticReqLimit() );
    if ( limit == 0 )
        limit = INT_MAX;
    pLimits->setStaticReqLimit( limit );
}

HttpServerBuilder::~HttpServerBuilder()
{
    if ( m_railsDefault )
        delete m_railsDefault;
}

//FIXME: when check dir permission, using current uid,
//  may not be the one when the server is up running

XmlNode* HttpServerBuilder::parseFile(const char* configFilePath, const char* rootTag)
{
    char achError[4096];
    XmlTreeBuilder tb;
    XmlNode* pRoot = tb.parse(configFilePath, achError, 4095);
    if ( pRoot == NULL )
    {
        LOG_ERR(( "%s", achError ));
        return NULL;
    }
    // basic validation
    if ( strcmp(pRoot->getName(), rootTag) != 0 )
    {
        LOG_ERR((
                    "[%s] %s: root tag expected: <%s>, real root tag : <%s>!\n",
                    getLogId(), configFilePath, rootTag, pRoot->getName()));
        delete pRoot;
        return NULL;
    }
    
#if (0)
    char sPlainFile[512] = {0};
    strcpy(sPlainFile, configFilePath);
    strcat(sPlainFile, ".txt");
    plainconf::testOutputConfigFile(pRoot, sPlainFile);
    //const char *p = plainconf::getConfDeepValue(pRoot, "logging|log|fileName");
    //p = plainconf::getConfDeepValue(pRoot, "logging|log|enableStderrLog");
    //p = plainconf::getConfDeepValue(pRoot, "logging|accessLog|fileName");
    //p = plainconf::getConfDeepValue(pRoot, "mime");
       
#endif
    
    return pRoot;

}

#define VH_ROOT     "VH_ROOT"
#define DOC_ROOT    "DOC_ROOT"
#define SERVER_ROOT "SERVER_ROOT"

int HttpServerBuilder::getRootPath( const char *&pRoot, const char *&pFile )
{
    int offset = 1;
    if ( *pFile == '$' )
    {
        if (strncasecmp( pFile+1, VH_ROOT, 7 ) == 0)
        {
            if ( m_achVhRoot[0] )
            {
                pRoot = m_achVhRoot;
                pFile += 8;
            }
            else
            {
                LOG_ERR((
                            "[%s] Virtual host root path is not available for %s.",
                            getLogId(), pFile ));
                return -1;
            }
        }
        else if ( strncasecmp( pFile+1, DOC_ROOT, 8 ) == 0)
        {
            if ( m_pCurVHost )
            {
                pRoot = m_pCurVHost->getDocRoot()->c_str();
                pFile += 9;
            }
            else
            {
                LOG_ERR((
                            "[%s] Document root path is not available for %s.",
                            getLogId(), pFile ));
                return -1;
            }
        }
        else if ( strncasecmp( pFile+1, SERVER_ROOT, 11 ) == 0 )
        {
            pRoot = getServerRoot();
            pFile += 12;
        }
    }
    else
    {
        offset = 0;
        if (( *pFile != '/' )&&( m_pCurVHost ))
        {
            pRoot = m_pCurVHost->getDocRoot()->c_str();
        }
    }
    if (( offset )&&( *pFile == '/' ))
        ++pFile;
    return 0;
}

int HttpServerBuilder::offsetChroot( char * dest, const char *path )
{
    if ( m_sChroot.c_str() &&
            (strncmp( m_sChroot.c_str(), path, m_sChroot.len() ) == 0) &&
            ( *(path + m_sChroot.len() ) == '/' ) )
    {
        struct stat st;
        if ( nio_stat( dest, &st ) == -1 )
        {
            if ( nio_stat( dest + m_sChroot.len(), &st ) == 0 )
            {
                memmove( dest, dest + m_sChroot.len(),
                         strlen( dest ) - m_sChroot.len() + 1 );
            }
        }
    }
    return 0;
}

#define VH_NAME "VH_NAME"
int HttpServerBuilder::expandVariable( const char * pValue, char * pBuf,
                                       int bufLen, int allVariable )
{
    const char * pBegin = pValue;
    char * pBufEnd = pBuf + bufLen - 1;
    char * pCur = pBuf;
    int len;
    while ( *pBegin )
    {
        len = strcspn( pBegin, "$" );
        if ( len > 0 )
        {
            if ( len > pBufEnd - pCur )
                return -1;
            memmove( pCur, pBegin, len );
            pCur += len;
            pBegin += len;
        }
        if ( *pBegin == '$' )
        {
            if ( strncasecmp( pBegin+1, VH_NAME, 7 ) == 0 )
            {
                pBegin += 8;
                int nameLen = m_sVhName.len();
                if ( nameLen > 0 )
                {
                    if ( nameLen > pBufEnd - pCur )
                        return -1;
                    memmove( pCur, m_sVhName.c_str(), nameLen );
                    pCur += nameLen;
                }
            }
            else if (( allVariable )
                     &&(( strncasecmp( pBegin+1, VH_ROOT, 7 ) == 0 )
                        ||( strncasecmp( pBegin+1, DOC_ROOT, 8 ) == 0 )
                        ||( strncasecmp( pBegin+1, SERVER_ROOT, 11 ) == 0 )))
            {
                const char * pRoot = "";
                getRootPath( pRoot, pBegin );
                if ( *pRoot )
                {
                    int len = strlen( pRoot );
                    if ( len > pBufEnd - pCur )
                        return -1;
                    memmove( pCur, pRoot, len );
                    pCur += len;
                }
            }
            else
            {
                if ( pCur == pBufEnd )
                    return -1;
                *pCur++ = '$';
                ++pBegin;
            }
        }
    }
    *pCur = 0;
    return pCur - pBuf;
}

char * HttpServerBuilder::getExpandedTag( const XmlNode * pNode,
        const char * pName, char *pBuf, int bufLen )
{
    const char * pVal = getTag( pNode, pName );
    if ( !pVal )
        return NULL;
    if ( expandVariable( pVal, pBuf, bufLen ) >= 0 )
        return pBuf;
    LOG_NOTICE(( "[%s] String is too long for tag: %s, value: %s, maxlen: %d",
                 getLogId(), pName, pVal, bufLen ));
    return NULL;
}


int HttpServerBuilder::getAbsolute( char * res, const char * path, int pathOnly )
{
    const char * pRoot = "";
    const char * pPath = path;
    int ret;
    char achBuf[MAX_PATH_LEN];
    char * dest = achBuf;
    int len = MAX_PATH_LEN;
    if ( getRootPath( pRoot, pPath ) )
        return -1;
    if ( m_sChroot.c_str() )
    {
        if (( *pRoot )||( strncmp( path, m_sChroot.c_str(), m_sChroot.len() ) != 0 ))
        {
            memmove( dest, m_sChroot.c_str(), m_sChroot.len() );
            dest += m_sChroot.len();
            len -= m_sChroot.len();
        }
    }
    if ( pathOnly )
        ret = GPath::getAbsolutePath( dest, len, pRoot, pPath);
    else
        ret = GPath::getAbsoluteFile( dest, len, pRoot, pPath );
    if ( ret )
    {
        LOG_ERR((
                    "[%s] Failed to tanslate to absolute path with root=%s, "
                    "path=%s!", getLogId(), pRoot, path ));
    }
    else
    {
        // replace "$VH_NAME" with the real name of the virtual host.
        if ( expandVariable( achBuf, res, MAX_PATH_LEN ) < 0 )
        {
            LOG_NOTICE(( "[%s] Path is too long: %s",
                         getLogId(), pPath ));
            return -1;
        }
    }
    return ret;
}



int HttpServerBuilder::getAbsoluteFile(char* dest, const char* file)
{
    return getAbsolute( dest, file, 0 );
}

int HttpServerBuilder::getAbsolutePath(char* dest, const char* path)
{
    return getAbsolute( dest, path, 1 );
}



int HttpServerBuilder::getValidFile(char * dest, const char * file, const char * desc )
{
    if ( (getAbsoluteFile(dest, file ) != 0 )
            || access( dest, F_OK ) != 0 )
    {
        LOG_ERR(( INVAL_PATH, getLogId(), desc,  dest ));
        return -1;
    }
    return 0;
}

int HttpServerBuilder::getValidPath(char * dest, const char * path, const char * desc )
{
    if ( getAbsolutePath(dest, path ) != 0 )
    {
        LOG_ERR(( INVAL_PATH, getLogId(), desc,  path ));
        return -1;
    }
    if ( access( dest, F_OK ) != 0 )
    {
        LOG_ERR(( INACCESSIBLE_PATH, getLogId(), desc,  dest ));
        return -1;
    }
    return 0;
}

int HttpServerBuilder::getValidChrootPath(char * dest, const char * path, const char * desc )
{
    if ( getValidPath( dest, path, desc ) == -1 )
        return -1;
    if (( m_sChroot.c_str() )&&
            ( strncmp( dest, m_sChroot.c_str(), m_sChroot.len() ) == 0 ))
    {
        memmove( dest, dest + m_sChroot.len(),
                 strlen( dest ) - m_sChroot.len() + 1 );
    }
    return 0;
}

const HttpHandler * getHandlerAllowed( const HttpVHost * pVHost, int type, const char * pHandler )
{
    const HttpHandler * pHdlr = HandlerFactory::getInstance( type, pHandler );
    if ( !pHdlr )
    {
        LOG_ERR(( "[%s] Can not find handler with type: %d, name: %s.",
                  getLogId(), type, (pHandler)?pHandler:"" ));
    }
    else
    {
        if ( type > HandlerType::HT_CGI )
        {
            const HttpVHost * pVHost1 = ((const ExtWorker *)pHdlr)->
                                        getConfigPointer()->getVHost();
            if ( pVHost1)
            {
                if ( pVHost != pVHost1 )
                {
                    LOG_ERR(( "[%s] Access to handler [%s:%s] from [%s] is denied!",
                              getLogId(),
                              HandlerType::getHandlerTypeString( type ),
                              pHandler, (pVHost)?pVHost->getName(): "Server" ));
                    pHdlr = NULL;
                }
            }
        }
    }
    return pHdlr;
}



int HttpServerBuilder::checkDeniedSubDirs( HttpVHost * pVHost,
        const char * pUri, const char *pLocation )
{
    int len = strlen( pLocation );
    if ( *( pLocation + len - 1 ) != '/' )
        return 0;
    DeniedDir::iterator iter = HttpGlobals::getDeniedDir()->lower_bound( pLocation );
    iter = HttpGlobals::getDeniedDir()->next_included( iter, pLocation );
    while ( iter != HttpGlobals::getDeniedDir()->end() )
    {
        const char * pDenied = HttpGlobals::getDeniedDir()->getPath( iter );
        const char * pSubDir = pDenied + len;
        char achNewURI[MAX_URI_LEN + 1];
        int n = safe_snprintf( achNewURI, MAX_URI_LEN, "%s%s", pUri, pSubDir );
        if ( n == MAX_URI_LEN )
        {
            LOG_ERR(( "[%s] URI is too long when add denied dir %s for context %s",
                      getLogId(), pDenied, pUri ));
            return -1;
        }
        if ( pVHost->getContext( achNewURI ) )
            continue;
        if ( !pVHost->addContext( achNewURI, HandlerType::HT_NULL, pDenied,
                                  NULL, false ) )
        {
            LOG_ERR(( "[%s] Failed to block denied dir %s for context %s",
                      getLogId(), pDenied, pUri ));
            return -1;
        }
        iter = HttpGlobals::getDeniedDir()->next_included( iter, pLocation );
    }
    return 0;
}

static int checkAccess( char * pReal )
{
    if ( HttpGlobals::getDeniedDir()->isDenied( pReal ) )
    {
        LOG_ERR(( "[%s] Path is in the access denied list:%s",
                  getLogId(), pReal ));
        return -1;
    }
    return 0;
}

static int checkPath( char *pPath, const char * desc, int follow )
{
    char achOld[MAX_PATH_LEN];
    struct stat st;
    int ret = nio_stat( pPath, &st );
    if ( ret == -1 )
    {
        LOG_ERR(( INVAL_PATH, getLogId(), desc,  pPath ));
        return -1;
    }
    memccpy( achOld, pPath, 0, MAX_PATH_LEN - 1 );
    ret = GPath::checkSymLinks( pPath, pPath + strlen( pPath ),
                                pPath + MAX_PATH_LEN, pPath, follow );
    if ( ret == -1 )
    {
        if ( errno == EACCES )
            LOG_ERR(( "[%s] Path of %s contains symbolic link or"
                      " ownership does not match:%s",
                      getLogId(), desc, pPath ));
        else
            LOG_ERR(( INVAL_PATH, getLogId(), desc,  pPath ));
        return -1;
    }
    if ( S_ISDIR( st.st_mode ) )
    {
        if (*( pPath + ret - 1 ) != '/' )
        {
            *(pPath + ret ) = '/';
            *(pPath + ret + 1 ) = 0;
        }
    }
    if ( strcmp( achOld, pPath ) != 0 )
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( "[%s] the real path of %s is %s.", getLogId(), achOld, pPath ));
    if ( HttpGlobals::s_psChroot ) //offset chroot
        pPath += HttpGlobals::s_psChroot->len();
    if ( checkAccess( pPath ) )
        return -1;
    return 0;
}

static void configAccessControl( AccessControl * pAC, const XmlNode * pNode1 )
{
    int c;
    const char * pValue = pNode1->getChildValue( "allow");
    if ( pValue )
    {
        c = pAC->addList( pValue, true );
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( "[%s] add %d entries into allowed list.",
                    getLogId(), c ));
    }
    else
        LOG_WARN(( "[%s] Access Control: No entries in allowed list",
                   getLogId() ));
    pValue = pNode1->getChildValue("deny");
    if ( pValue )
    {
        c = pAC->addList( pValue, false );
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( "[%s] add %d entries into denied list.",
                    getLogId(), c ));
    }
}


static int initAccessControl( AccessControl* pAC, const XmlNode* pNode )
{
    const XmlNode * pNode1 = pNode->getChild("accessControl");
    pAC->clear();
    if ( pNode1 )
    {
        configAccessControl( pAC, pNode1 );
    }
    else
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( "[%s] no rule for access control.",
                    getLogId() ));
    }
    return 0;
}


static HttpContext * addContext( HttpVHost * pVHost, int match,
                                 const char * pUri, int type, const char * pLocation, const char * pHandler,
                                 int allowBrowse )
{
    if ( match )
    {
        char achTmp[] = "/";
        HttpContext * pOld;
        if ( pVHost->isGlobalMatchContext() )
            pOld = &pVHost->getRootContext();
        else
            pOld = pVHost->getContext( achTmp );
        if ( !pOld )
        {
            return NULL;
        }
        HttpContext * pContext = new HttpContext();
        pVHost->setContext( pContext, pUri, type, pLocation, pHandler, allowBrowse, match);
        pOld->addMatchContext( pContext );
        return pContext;
    }
    else
    {
        pVHost->setGlobalMatchContext( 1 );
        HttpContext * pOld = pVHost->getContext( pUri );
        if ( pOld )
        {
            if ( strcmp( pUri, "/" ) == 0 )
            {
                pVHost->setContext( pOld, pUri, type, pLocation, pHandler, allowBrowse, match);
                return pOld;
            }
            return NULL;
        }
        return pVHost->addContext( pUri, type, pLocation, pHandler, allowBrowse );
    }
}



HttpContext * HttpServerBuilder::configContext( HttpVHost * pVHost,
        const char * pUri, int type, const char * pLocation, const char * pHandler,
        int allowBrowse )
{
    char achRealPath[4096];
    int match = ( strncasecmp( pUri, "exp:", 4 ) == 0 );
    assert( type != HandlerType::HT_REDIRECT );
    if ( type >= HandlerType::HT_CGI )
        allowBrowse = 1;
    if ( type < HandlerType::HT_FASTCGI )
    {
        if ( allowBrowse )
        {
            if ( pLocation == NULL )
            {
                if (( type == HandlerType::HT_REDIRECT )||
                    ( type == HandlerType::HT_RAILS )||match)
                {
                    LOG_ERR(( MISSING_TAG, getLogId(), "location" ));
                    return NULL;
                }
                else
                    pLocation = pUri+1;
            }
            int ret = 0;
            switch ( *pLocation )
            {
            case '$':
                if (( match )&&( isdigit( *(pLocation+1) )))
                {
                    strcpy( achRealPath, pLocation );
                    break;
                }
                //fall through
            default:
                ret = getAbsoluteFile( achRealPath, pLocation );
                break;
            }
            if (ret)
            {
                LOG_ERR(( INVAL_PATH, getLogId(), "context location",  pLocation ));
                return NULL;
            }
            if ( !match )
            {
                int PathLen = strlen( achRealPath );
                if ( checkPath( achRealPath, "context location", pVHost->followSymLink() ) )
                    return NULL;
                PathLen = strlen( achRealPath );
                if ( access( achRealPath, F_OK ) != 0 )
                {
                    LOG_ERR(( "[%s] path is not accessible: %s", getLogId(), achRealPath ));
                    return NULL;
                }
                if ( PathLen > 512 )
                {
                    LOG_ERR(( "[%s] path is too long: %s", getLogId(), achRealPath ));
                    return NULL;
                }
                pLocation = achRealPath;
                pLocation += m_sChroot.len();
                if ( allowBrowse )
                {
                    ret = checkDeniedSubDirs( pVHost, pUri, pLocation );
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
            LOG_ERR(( MISSING_TAG, getLogId(), "handler" ));
            return NULL;
        }
//        if (( match )&&( pLocation == NULL ))
//        {
//            pLocation = "&";
//        }
    }
    return addContext( pVHost, match, pUri, type, pLocation, pHandler, allowBrowse );
}


static int accessControlAvail( const XmlNode * pNode )
{
    const XmlNode * pNode1 = pNode->getChild( "accessControl" );
    if ( pNode1 )
    {
        const char * pAllow = pNode1->getChildValue( "allow" );

        if (((pAllow)&&strchr( pAllow, 'T' ))
                ||( pNode1->getChildValue( "deny" ) ))
            return 1;
    }
    return 0;
}

static HTAuth * setAuthRealm( HttpVHost * pVHost, HttpContext * pContext,
                              const char * pRealmName )
{
    HTAuth * pAuth = NULL;
    if (( pRealmName != NULL )&&( *pRealmName ))
    {
        UserDir * pUserDB = pVHost->getRealm( pRealmName );
        if ( !pUserDB )
        {
            LOG_WARN(( "[%s] <realm> %s is not configured,"
                       " deny access to this context",
                       getLogId(), pRealmName ));
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

static int setContextAuth( HttpVHost * pVHost, HttpContext * pContext,
                           const XmlNode * pContextNode )
{
    const char *pRealmName = NULL;
    const XmlNode* pAuthNode = pContextNode->getChild( "auth" );
    HTAuth * pAuth = NULL;
    if ( !pAuthNode )
    {
        pAuthNode = pContextNode;
    }
    pRealmName = pAuthNode->getChildValue("realm");
    pAuth = setAuthRealm( pVHost, pContext, pRealmName );
    if ( pAuth )
    {
        const char * pData = pAuthNode->getChildValue( "required" );
        if ( !pData )
            pData = "valid-user";
        pContext->setAuthRequired( pData );

        pData = pAuthNode->getChildValue( "authName" );
        if ( pData )
            pAuth->setName( pData );
    }
    return 0;
}

static int setContextAccess( HttpContext * pContext, const XmlNode * pContextNode )
{
    AccessControl * pAccess = NULL;
    if ( accessControlAvail( pContextNode ) )
    {
        pAccess = new AccessControl();
        initAccessControl( pAccess, pContextNode );
        pContext->setAccessControl( pAccess );
    }
    return 0;
}

static void setContextAutoIndex( HttpContext * pContext, const XmlNode * pContextNode )
{
    if ( pContextNode->getChildValue( "autoIndex" ) )
    {
        pContext->setAutoIndex( getLongValue( pContextNode, "autoIndex", 0, 1, 0 ) );
    }
}

static int setContextDirIndex( HttpContext * pContext, const XmlNode * pContextNode )
{
    pContext->clearDirIndexes();
    const char * pValue = pContextNode->getChildValue("indexFiles");
    if ( pValue )
    {
        pContext->addDirIndexes( pValue );
    }
    return 0;
}

const HttpHandler * HttpServerBuilder::getHandler( const HttpVHost * pVHost,
        const char * pType, const char * pHandler )
{
    int role = HandlerType::ROLE_RESPONDER;
    int type = HandlerType::getHandlerType( pType, role );
    if (( type == HandlerType::HT_END )||
            ( type == HandlerType::HT_JAVAWEBAPP )||
            ( type == HandlerType::HT_RAILS ))
    {
        LOG_ERR(( "[%s] Invalid External app type:[%s]" , getLogId(), pType ));
        return NULL;
    }
    const HttpHandler * pHdlr = getHandlerAllowed( pVHost, type, pHandler );
    if ( !pHdlr )
    {
        LOG_ERR(( "[%s] Can not find External Application: %s, type: %s",
                  getLogId(), pHandler, pType ));
    }
    return pHdlr;
}

const HttpHandler * HttpServerBuilder::getHandler( const HttpVHost * pVHost, const XmlNode * pNode )
{
    const char* pType = pNode->getChildValue( "type" );

    char achHandler[256];
    const char* pHandler = getExpandedTag( pNode, "handler", achHandler, 256 );
    return getHandler( pVHost, pType, pHandler );
}

int HttpServerBuilder::setContextExtAuthorizer( HttpVHost * pVHost, HttpContext * pContext,
        const XmlNode * pContextNode )
{
    const HttpHandler * pAuth;
    const char * pHandler = pContextNode->getChildValue( "authorizer" );
    if ( pHandler )
    {
        pAuth = getHandler( pVHost, "fcgiauth", pHandler );
    }
    else
    {
        const XmlNode * pNode = pContextNode->getChild( "extAuthorizer" );
        if ( !pNode )
            return 0;
        pAuth = getHandler( pVHost, pNode );
    }
    if ( !pAuth )
        return 1;
    if ( ((ExtWorker * )pAuth)->getRole() != EXTAPP_AUTHORIZER )
    {
        LOG_ERR(( "[%s] External Application [%s] is not a Authorizer role.",
                  getLogId(), pAuth->getName() ));
        return 1;
    }
    pContext->setAuthorizer( pAuth );
    return 0;
}


static int getRedirectCode( const XmlNode * pContextNode, int &code,
                            const char * pLocation )
{
    code = getLongValue( pContextNode, "externalRedirect", 0, 1, 1 );
    if ( code == 0 )
        --code;
    else
    {
        int orgCode = getLongValue( pContextNode, "statusCode", 0, 505, 0 );
        if ( orgCode )
        {
            code = HttpStatusCode::codeToIndex( orgCode );
            if ( code == -1 )
            {
                LOG_WARN(( "[%s] Invalid status code %d, use default: 302!",
                           getLogId(), orgCode ));
                code = SC_302;
            }
        }
    }
    if ((code == -1 )&&( code >= SC_300 )&&( code < SC_400 ))
    {
        if (( pLocation == NULL )||( *pLocation == 0 ))
        {
            LOG_ERR(( "[%s] Destination URI must be specified!", getLogId() ));
            return -1;
        }
    }
    return 0;
}



int HttpServerBuilder::configContext( HttpVHost * pVHost, const XmlNode* pContextNode )
{
    const char *pUri = NULL;
    const char *pLocation = NULL;
    const char *pHandler = NULL;
    bool allowBrowse = false;
    int match;
    pUri = getTag( pContextNode, "uri");
    if ( pUri == NULL )
        return -1;
    LogIdTracker tracker;
    appendLogId( ":" );
    appendLogId( "context" );
    appendLogId( ":" );
    appendLogId( pUri );
    //int len = strlen( pUri );

    const char * pValue = getTag( pContextNode, "type");
    if ( pValue == NULL )
        return -1;
    int role = HandlerType::ROLE_RESPONDER;
    int type = HandlerType::getHandlerType( pValue, role );
    if (( type == HandlerType::HT_END )||( role != HandlerType::ROLE_RESPONDER ))
    {
        LOG_ERR(( INVAL_TAG, getLogId(), "type", pValue ));
        return -1;
    }
    pLocation = pContextNode->getChildValue("location");
    pHandler = pContextNode->getChildValue( "handler" );

    char achHandler[256] = {0};
    if ( pHandler )
    {
        if ( expandVariable( pHandler, achHandler, 256 ) < 0 )
        {
            LOG_NOTICE(( "[%s] add String is too long for scripthandler, value: %s",
                getLogId(), pHandler ));
            return -1;
        }
        pHandler = achHandler;
    }
    
    allowBrowse = getLongValue( pContextNode,"allowBrowse", 0, 1, 1);

    match = ( strncasecmp( pUri, "exp:", 4 ) == 0 );
    if (( *pUri != '/' )&&( !match ))
    {
        LOG_ERR(( "[%s] URI must start with '/' or 'exp:', invalid URI - %s",
                  getLogId(), pUri ));
        return -1;
    }
    int code;
    HttpContext * pContext;
    switch ( type )
    {
    case HandlerType::HT_JAVAWEBAPP:
        allowBrowse = true;
        pContext = importWebApp( pVHost, pUri, pLocation, pHandler,
                                 allowBrowse );
        break;
    case HandlerType::HT_RAILS:
        allowBrowse = true;
        pContext = configRailsContext( pContextNode, pVHost, pUri, pLocation );
        break;
    case HandlerType::HT_REDIRECT:
        if ( getRedirectCode( pContextNode, code, pLocation ) == -1 )
            return -1;
        if ( !pLocation )
            pLocation = "";
        pContext = addContext( pVHost, match, pUri, type, pLocation,
                               pHandler, allowBrowse );
        if ( pContext )
        {
            pContext->redirectCode( code );
            pContext->setConfigBit( BIT_ALLOW_OVERRIDE, 1 );
        }
        break;
    default:
        pContext = configContext( pVHost, pUri, type, pLocation,
                                  pHandler, allowBrowse);
    }
    if ( pContext )
    {
        setContextAutoIndex( pContext, pContextNode );
        setContextDirIndex( pContext, pContextNode );
        if ( type == HandlerType::HT_CGI )
        {
            int val = getLongValue( pContextNode, "allowSetUID", 0, 1, -1 );
            if ( val != -1 )
            {
                pContext->setConfigBit( BIT_SETUID, 1 );
                pContext->setConfigBit( BIT_ALLOW_SETUID, val );
            }
        }
        if ( setContextAuth( pVHost, pContext, pContextNode ) == -1 )
            return -1;
        setContextAccess( pContext, pContextNode );
        setContextExtAuthorizer( pVHost, pContext, pContextNode );

        initExpires( pContextNode, &pContext->getExpires(), NULL, pContext );
        pValue = pContextNode->getChildValue( "expiresByType" );
        if ( pValue && ( *pValue ))
            pContext->setExpiresByType( pValue );

        pValue = pContextNode->getChildValue( "extraHeaders" );
        if ( pValue && ( *pValue ))
            pContext->setExtraHeaders( getLogId(), pValue, (int)strlen( pValue ) );

        pValue = pContextNode->getChildValue( "addDefaultCharset" );
        if ( pValue )
        {
            if ( strcasecmp(pValue, "on" ) == 0 )
            {
                pValue = pContextNode->getChildValue( "defaultCharsetCustomized" );
                if ( !pValue )
                    pContext->setDefaultCharsetOn();
                else
                    pContext->setDefaultCharset( pValue );
            }
            else
                pContext->setDefaultCharset( NULL );
        }
        configContextMime( pContext, pContextNode );
        const XmlNode * pNode = pContextNode->getChild( "rewrite" );
        if ( pNode )
        {
            pContext->enableRewrite( getLongValue( pNode, "enable", 0, 1, 0 ));
            pValue = pNode->getChildValue( "option" );
            if (( pValue )&&( strstr( pValue, "inherit" ) ))
            {
                pContext->setRewriteInherit( 1 );
            }
            pValue = pNode->getChildValue( "base" );
            if ( pValue )
            {
                if ( *pValue != '/' )
                {
                    LOG_ERR(( "[%s]Invalid rewrite base: '%s'", getLogId(), pValue ));
                }
                else
                {
                    pContext->setRewriteBase( pValue );
                }
            }
            pValue = pNode->getChildValue( "rules" );
            if ( pValue )
            {
                configContextRewriteRule( pVHost, pContext, (char *)pValue );
            }
            else
                configContextRewriteRule( pVHost, pContext, pNode );
                
        }
        pNode = pContextNode->getChild( "customErrorPages" );
        if ( pNode )
            configCustomErrorPages( pContext, pNode );

        return 0;
    }
    return -1;
}

int HttpServerBuilder::configContextRewriteRule( HttpVHost *pVHost,
        HttpContext * pContext, char * pRule )
{
    RewriteRuleList * pRuleList = new RewriteRuleList();
    if ( pRuleList )
    {
        RewriteRule::setLogger( NULL, getLogId() );
        if ( RewriteEngine::parseRules( pRule, pRuleList,
                                        pVHost->getRewriteMaps() ) == 0 )
        {
            pContext->setRewriteRules( pRuleList );
        }
        else
            delete pRuleList;
    }
    return 0;
}

int HttpServerBuilder::configContextRewriteRule( HttpVHost *pVHost,
        HttpContext * pContext, const XmlNode *pRewriteNode )
{
    //Try to get the "RewriteCond" and "RewriteRule" from pRewriteNode
    char rules[8192] = {0};
    XmlNodeList list;
    pRewriteNode->getAllChildren(list);
    XmlNodeList::const_iterator iter;
    for( iter = list.begin(); iter != list.end(); ++iter )
    {
        if (strncasecmp((*iter)->getName(), "Rewrite", 7) == 0)
        {
            strcat(rules, (*iter)->getName());
            strcat(rules, " ");
            strcat(rules, (*iter)->getValue());
            strcat(rules, "\n");
        }
    }
    

    if (strlen(rules) > 0)
        configContextRewriteRule( pVHost,pContext, rules );
    
    return 0;
}

int HttpServerBuilder::configContextMime( HttpContext * pContext,
        const XmlNode * pContextNode )
{
    const char * pValue = pContextNode->getChildValue( "addMIMEType" );
    if ( pValue )
        pContext->addMIME( pValue );
    pValue = pContextNode->getChildValue( "forceType" );
    if ( pValue )
    {
        pContext->setForceType( (char *)pValue, getLogId() );
    }
    pValue = pContextNode->getChildValue( "defaultType" );
    if ( pValue )
    {
        pContext->initMIME();
        pContext->getMIME()->initDefault( (char *)pValue );
    }
    return 0;
}


int HttpServerBuilder::configVHBasics(HttpVHost* pVHost, const XmlNode* pVhConfNode)
{
    const char * pDocRoot = getTag( pVhConfNode, "docRoot");
    if ( pDocRoot == NULL )
    {
        return -1;
    }
    m_pCurVHost = pVHost;
    char achBuf[MAX_PATH_LEN];
    char * pPath = achBuf;
    if (getValidPath(pPath, pDocRoot, "document root" ) != 0 )
        return -1;
    if ( checkPath( pPath, "document root", pVHost->followSymLink() ) == -1 )
        return -1;
    pPath += m_sChroot.len();
    if ( pVHost->setDocRoot( pPath ) != 0 )
    {
        LOG_ERR(( "[%s] failed to set document root - %s!",
                  getLogId(), pPath ));
        return -1;
    }
    if ( checkDeniedSubDirs( pVHost, "/", pPath ) )
    {
        return -1;
    }
    pVHost->enable( getLongValue(pVhConfNode, "enabled", 0, 1, 1 ));
    pVHost->enableGzip( (HttpServerConfig::getInstance().getGzipCompress())?
                        getLongValue( pVhConfNode, "enableGzip", 0, 1, 1 ): 0 );

    const char * pAdminEmails = pVhConfNode->getChildValue( "adminEmails" );
    if ( !pAdminEmails )
    {
        pAdminEmails = "";
    }
    pVHost->setAdminEmails( pAdminEmails );

    return 0;

}

static int convertToRegex( const char  * pOrg, char * pDestBuf, int bufLen )
{
    const char * p = pOrg;
    char * pDest = pDestBuf;
    char * pEnd = pDest + bufLen - 1;
    int n;
    while (( *p )&&( pDest < pEnd - 3))
    {
        n = strspn( p, " ," );
        if ( n )
            p += n;
        n = strcspn( p, " ," );
        if ( !n )
            continue;
        if (( pDest != pDestBuf )&&( pDest < pEnd ))
            *pDest++ = ' ';
        if ((strncasecmp( p, "REGEX[", 6 ) != 0 )&&
                ((memchr( p, '?', n ) )||(memchr( p, '*', n ) ) )&&
                (pEnd - pDest > 10 ))
        {
            const char * pB = p;
            const char * pE = p + n;
            memmove( pDest, "REGEX[", 6 );
            pDest += 6;
            while (( pB < pE )&&( pEnd - pDest > 3 ))
            {
                if ( '?' == *pB )
                    *pDest++ = '.';
                else if ( '*' == *pB )
                {
                    *pDest++ = '.';
                    *pDest++ = '*';
                }
                else if ( '.' == *pB )
                {
                    *pDest++ = '\\';
                    *pDest++ = '.';
                }
                else
                    *pDest++ = *pB;
                ++pB;
            }
            *pDest++ = ']';
        }
        else
        {
            int len = n;
            if ( pEnd - pDest < n )
                len = pEnd -pDest;
            memmove( pDest, p, len );
            pDest += len;
        }
        p += n;
    }
    *pDest = 0;
    *pEnd = 0;
    return pDest - pDestBuf;
}




int HttpServerBuilder::expandDomainNames( const char *pDomainNames,
        char * pDestDomains, int len, char dilemma )
{
    if ( !pDomainNames )
    {
        pDestDomains[0] = 0;
        return 0;
    }
    const char * p = pDomainNames;
    char * pDest = pDestDomains;
    char * pEnd = pDestDomains + len - 1;
    const char * pStr;
    int n;
    while (( *p )&&( pDest < pEnd ))
    {
        n = strspn( p, " ," );
        if ( n )
            p += n;
        n = strcspn( p, " ," );
        if ( !n )
            continue;
        if (( strncasecmp( p, "$vh_domain", 10) == 0 )&&
                ( 10 == n ))
        {
            pStr = m_vhDomain.c_str();
            len = m_vhDomain.len();
        }
        else if (( strncasecmp( p, "$vh_aliases", 11 ) == 0 )&&
                 ( 11 == n ))
        {
            pStr = m_vhAliases.c_str();
            len = m_vhAliases.len();
        }
        else
        {
            pStr = p;
            len = n;
        }
        if (( pDest != pDestDomains )&&( pDest < pEnd ))
            *pDest++ = dilemma;

        if ( len > pEnd - pDest)
            len = pEnd - pDest;
        memmove( pDest, pStr, len );
        pDest += len;
        p += n;
    }
    *pDest = 0;
    *pEnd = 0;
    return pDest - pDestDomains;
}


#ifdef RUN_TEST
int HttpServerBuilder::testDomainMod()
{
    m_vhDomain = "www.test.com";
    m_vhAliases = "*.test.com, www?.test.org";
    char achTest1[] = "   $vh_Domain ,  $vh_aliases  mydomain.com";
    char achBuf1[ 4096];
    char result1[] = "www.test.com *.test.com, www?.test.org mydomain.com";
    char achBuf2[4096];
    char result2[] = "www.test.com REGEX[.*\\.test\\.com] REGEX[www.\\.test\\.org] mydomain.com";
    expandDomainNames( achTest1, achBuf1, 4096, ' ' );
    printf( "expandDomainName()\n"
            "expect: %s\n"
            "result: %s\n", result1, achBuf1 );
    convertToRegex( achBuf1, achBuf2, 4096 );
    printf( "convertToRegex()\n"
            "expect: %s\n"
            "result: %s\n", result2, achBuf2 );
    return 0;
}
#endif




int HttpServerBuilder::configVHContextList( HttpVHost* pVHost,
        const XmlNode* pVhConfNode )
{
    HttpContext *pRootContext =
        pVHost->addContext( "/", HandlerType::HT_NULL,
                            pVHost->getDocRoot()->c_str(), NULL, 1 );
//    pRootContext->setAutoIndex( pVHost->getRootContext().isAutoIndexOn() );

    pVHost->setGlobalMatchContext( 1 );

    const XmlNode* p0 = pVhConfNode->getChild("contextList");
    if ( p0 == NULL )
        p0 = pVhConfNode;

    const XmlNodeList* pList = p0->getChildren( "context");
    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for ( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            configContext( pVHost, *iter );
        }
    }
    
//    else
//        LOG_NOTICE(( "[%s] no context configuration.",
//                getLogId() ));
    pVHost->contextInherit();
    return 0;
}

int HttpServerBuilder::configScriptHandler2( HttpVHost * pVHost,
        const XmlNodeList* pList, HttpMime * pHttpMime )
{
    LogIdTracker tracker;
    appendLogId( ":scripthandler:add" );
//    map.release_objects();
    XmlNodeList::const_iterator iter;
    for ( iter = pList->begin(); iter != pList->end(); ++iter )
    {
        const char *value = (char *)(*iter)->getValue();
        const char *pSuffixByTab = strchr(value, '\t');
        const char *pSuffixBySpace = strchr(value, ' ');
        const char *suffix  = NULL;
        if (pSuffixByTab == NULL && pSuffixBySpace == NULL)
        {
            LOG_ERR(( INVAL_TAG, getLogId(), "ScriptHandler", value ));
            continue;
        }         
        else if (pSuffixByTab == NULL)
            suffix = pSuffixBySpace;
        else if (pSuffixBySpace == NULL)
            suffix = pSuffixByTab;
        else
            suffix = ((pSuffixByTab > pSuffixBySpace)? pSuffixBySpace: pSuffixByTab);

        const char *type = strchr(value, ':');
        if (suffix == NULL || type == NULL || strchr( suffix, '.' ) || type > suffix)
        {
            LOG_ERR(( INVAL_TAG, getLogId(), "suffix", suffix ));
            continue;
        
        }
        ++ suffix; //should all spaces be handled here?? Now only one white space handled.
        
        char pType[256] = {0};
        memcpy(pType, value, type - value);
        
        char handler[256] = {0};
        memcpy(handler, type + 1, suffix - type - 2);
        
        char achHandler[256] = {0};
        if ( expandVariable( handler, achHandler, 256 ) < 0 )
        {
            LOG_NOTICE(( "[%s] add String is too long for scripthandler, value: %s",
                 getLogId(), handler ));
            continue;
        }
  
        const HttpHandler * pHdlr = getHandler( pVHost, pType, achHandler );
        
        if ( !pHdlr )
        {
            LOG_ERR(( "[%s] use static file handler for suffix [%s]",
                      getLogId(), suffix ));
            pHdlr = HandlerFactory::getInstance( 0, NULL );
        }   
        
        char* pMime = NULL;  //hardcode to NULL for we do not use this value anymore
        HttpMime * pParent = NULL;
        if ( pHttpMime )
            pParent = HttpGlobals::getMime();
        else
            pHttpMime = HttpGlobals::getMime();
        pHttpMime->addMimeHandler( suffix, pMime,
                                   pHdlr, pParent, getLogId() );
    }
        
    return 0;
}

int HttpServerBuilder::configScriptHandler1( HttpVHost * pVHost,
        const XmlNodeList* pList, HttpMime * pHttpMime )
{
    LogIdTracker tracker;
    appendLogId( ":scripthandler" );
//    map.release_objects();
    XmlNodeList::const_iterator iter;
    for ( iter = pList->begin(); iter != pList->end(); ++iter )
    {
        XmlNode * pNode = *iter;
        char* pSuffix = (char *)pNode->getChildValue("suffix");
        if ( !pSuffix || !*pSuffix || strchr( pSuffix, '.' ) )
        {
            LOG_ERR(( INVAL_TAG, getLogId(), "suffix", pSuffix ));        
        }
        
        
        const HttpHandler * pHdlr = getHandler( pVHost, pNode );        
        if ( !pHdlr )
        {
            LOG_ERR(( "[%s] use static file handler for suffix [%s]",
                      getLogId(), pSuffix ));
            pHdlr = HandlerFactory::getInstance( 0, NULL );
        }
        {
            char* pMime = (char *)pNode->getChildValue( "mime" );
            HttpMime * pParent = NULL;
            if ( pHttpMime )
                pParent = HttpGlobals::getMime();
            else
                pHttpMime = HttpGlobals::getMime();
            pHttpMime->addMimeHandler( pSuffix, pMime,
                                       pHdlr, pParent, getLogId() );
        }

    }
        
    return 0;
}


int HttpServerBuilder::configVHScriptHandler( HttpVHost* pVHost,
        const XmlNode* pVhConfNode )
{
    int configType = 0; //old type
    const XmlNode* p0 = pVhConfNode->getChild("scriptHandlerList");
    if ( p0 == NULL )
    {
        configType = 1;
        p0 = pVhConfNode->getChild("scriptHandler");
    }
     
    if ( p0 == NULL )
        return 0;
    
    if (configType == 0)
    {
        const XmlNodeList *pList = p0->getChildren( "scriptHandler" );
        if ( !pList || pList->size() == 0 )
                return 0;
        pVHost->getRootContext().initMIME();
        configScriptHandler1( pVHost, pList, pVHost->getMIME() );
    }
    else
    {
        const XmlNodeList *pList = p0->getChildren( "add" );
        if ( pList && pList->size() > 0 )
        {
            pVHost->getRootContext().initMIME();
            configScriptHandler2( pVHost, pList, pVHost->getMIME() );
        }
    }
    return 0;
}


int HttpServerBuilder::configVHSecurity( HttpVHost* pVHost, const XmlNode* pVhConfNode )
{
    const XmlNode * p0 = pVhConfNode->getChild("security");
    if ( p0 != NULL )
    {
        LogIdTracker tracker;
        appendLogId( ":security" );
        if ( accessControlAvail( p0 ) )
        {
            pVHost->enableAccessCtrl();
            initAccessControl( pVHost->getAccessCtrl(), p0 );
        }
        initRealmList( pVHost, p0 );
        const XmlNode * p1 = p0->getChild( "hotlinkCtrl" );
        if ( p1 )
            initHotlinkCtrl( pVHost, p1 );
    }
    return 0;
}

int HttpServerBuilder::initHotlinkCtrl( HttpVHost *pVHost, const XmlNode * pNode )
{
    int enabled = getLongValue( pNode, "enableHotlinkCtrl", 0, 1, 0 );
    if ( !enabled )
        return 0;
    HotlinkCtrl * pCtrl = new HotlinkCtrl();
    if ( !pCtrl )
    {
        LOG_ERR(( "[%s] out of memory while creating HotlinkCtrl Object!",
                  getLogId() ));
        return -1;
    }
    if ( pCtrl->setSuffixes( pNode->getChildValue( "suffixes" ) ) <= 0 )
    {
        LOG_ERR(("[%s] no suffix is configured, disable hotlink protection.",
                 getLogId() ));
        delete pCtrl;
        return -1;
    }
    pCtrl->setDirectAccess( getLongValue( pNode, "allowDirectAccess", 0, 1, 0 ) );
    const char * pRedirect = pNode->getChildValue( "redirectUri" );
    if ( pRedirect )
        pCtrl->setRedirect( pRedirect );
    int self = getLongValue( pNode, "onlySelf", 0, 1, 0 );
    if ( !self )
    {
        char achBuf[4096];
        const char * pValue = pNode->getChildValue( "allowedHosts" );
        if ( pValue )
        {
            expandDomainNames( pValue, achBuf, 4096, ',' );
            pValue = achBuf;
        }
        int ret = pCtrl->setHosts( pValue );
        int ret2 = pCtrl->setRegex( pNode->getChildValue( "matchedHosts" ) );
        if (( ret <= 0 )&&
                ( ret2 < 0 ))
        {
            LOG_WARN(( "[%s] no valid host is configured, only self"
                       " reference is allowed.", getLogId() ));
            self = 1;
        }
    }
    pCtrl->setOnlySelf( self );
    pVHost->setHotlinkCtrl( pCtrl );
    return 0;
}



int HttpServerBuilder::setAuthCache( const XmlNode * pNode,
                                     HashDataCache* pAuth )
{
    int timeout = getLongValue( pNode, "expire", 0, LONG_MAX, 60 );
    int maxSize = getLongValue( pNode, "maxCacheSize", 0, LONG_MAX, 1024 );
    pAuth->setExpire( timeout );
    pAuth->setMaxSize( maxSize );
    return 0;
}

int HttpServerBuilder::configRealm( HttpVHost * pVHost,
                                    const char * pName, const XmlNode * pRealmNode )
{
    const char * pType = pRealmNode->getChildValue( "type" );
    if ( !pType )
    {
        LOG_WARN(( "[%s] Realm type is not specified, use default: file.",
                   getLogId() ));
        pType = "file";
    }

    const XmlNode * p2 = pRealmNode->getChild("userDB");
    if ( p2 == NULL )
    {
        LOG_ERR(( MISSING_TAG_IN, getLogId(), "userDB", "realm" ));
        return -1;
    }
    if ( strcasecmp( pType, "file" ) == 0 )
    {
        const char * pFile = NULL, *pGroup = NULL, *pGroupFile=NULL;
        char achBufFile[MAX_PATH_LEN];
        char achBufGroup[MAX_PATH_LEN];

        pFile = p2->getChildValue( "location" );
        if (( !pFile )||
                ( getValidFile(achBufFile, pFile, "user DB") != 0 ))
        {
            return -1;
        }
        const XmlNode * pGroupNode = pRealmNode->getChild( "groupDB" );
        if ( pGroupNode )
        {
            pGroup = pGroupNode->getChildValue( "location" );
            if ( pGroup )
            {
                if ( getValidFile(achBufGroup, pGroup, "group DB") != 0 )
                {
                    return -1;
                }
                else
                    pGroupFile = &achBufGroup[m_sChroot.len()];
            }

        }
        UserDir * pUserDir =
            pVHost->getFileUserDir( pName, &achBufFile[m_sChroot.len()],
                                    pGroupFile );
        if ( !pUserDir )
        {
            LOG_ERR(( "[%s] Failed to create authentication DB.", getLogId() ));
            return -1;
        }
        setAuthCache( p2, pUserDir->getUserCache() );
        if (pGroup )
        {
            setAuthCache( pGroupNode, pUserDir->getGroupCache() );
        }
    }
    else
    {
        LOG_ERR(( "[%s] unsupported DB type in realm %s!",
                  getLogId(), pName ));
    }
    return 0;
}


int HttpServerBuilder::initRealmList( HttpVHost * pVHost, const XmlNode * pRoot )
{
    const XmlNode * pNode = pRoot->getChild( "realmList" );

    if ( pNode != NULL )
    {
        const XmlNodeList *pList = pNode->getChildren( "realm");
        if ( pList )
        {
            XmlNodeList::const_iterator iter;
            for ( iter = pList->begin(); iter != pList->end(); ++iter )
            {
                const XmlNode * pRealmNode = *iter;
                const char * pName = getTag( pRealmNode, "name" );
                if ( pName == NULL )
                {
                    continue;
                }
                LogIdTracker tracker;
                appendLogId( ":" );
                appendLogId( "realm" );
                appendLogId( ":" );
                appendLogId( pName );
                configRealm( pVHost, pName, pRealmNode );
            }
        }
    }
    return 0;

}

int HttpServerBuilder::configCustomErrorPages( HttpContext* pContext,
        const XmlNode* pNode )
{
    LogIdTracker tracker;
    appendLogId( ":errorpages" );
    int add = 0;
    const XmlNodeList * pList = pNode->getChildren( "errorPage" );
    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for ( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            const XmlNode * pNode = *iter;
            const char* pCode = pNode->getChildValue("errCode");
            const char* pUrl = pNode->getChildValue("url");
            if ( pContext->setCustomErrUrls(pCode, pUrl) != 0 )
                LOG_ERR(( "[%s] failed to set up custom error page %s - %s!",
                          getLogId(), pCode, pUrl ));
            else
                ++add ;
        }
    }

    return (add == 0);
}


int HttpServerBuilder::initRewrite( HttpVHost *pVHost, const XmlNode * pNode )
{
    pVHost->getRootContext().enableRewrite( getLongValue( pNode, "enable", 0, 1, 0 ) );
    pVHost->setRewriteLogLevel( getLongValue( pNode, "logLevel", 0, 9, 0 ) );
    const XmlNodeList *pList = pNode->getChildren( "map" );
    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for ( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            XmlNode * pN1 = *iter;
            const char * pName = pN1->getChildValue( "name" );
            const char * pLocation = pN1->getChildValue( "location" );
            char achBuf[1024];
            const char *p = strchr( pLocation, '$' );
            if ( p )
            {
                memmove( achBuf, pLocation, p - pLocation );
                if ( getAbsolute( &achBuf[ p- pLocation], p, 0 ) == 0 )
                    pLocation = achBuf;
            }
            pVHost->addRewriteMap( pName, pLocation );
        }
    }
    RewriteRule::setLogger( NULL, getLogId() );
    char * pRules = (char *)pNode->getChildValue( "rules" );
    if ( pRules )
    {
        configContextRewriteRule( pVHost, &pVHost->getRootContext(), pRules );
    }
    else
        configContextRewriteRule( pVHost, &pVHost->getRootContext(), pNode );
        
    return 0;
}

static const char * getAutoIndexURI( const XmlNode * pNode )
{
    const char * pURI = pNode->getChildValue( "autoIndexURI" );
    if ( pURI )
    {
        if ( *pURI != '/' )
        {
            LOG_ERR(( "[%s] Invalid AutoIndexURI, must be started with a '/'", getLogId() ));
            return NULL;
        }
    }
    return pURI;
}

int HttpServerBuilder::configVHIndexFile( HttpVHost * pVHost,
        const XmlNode * pVhConfNode )
{
    const XmlNode * pNode;
    pNode = pVhConfNode->getChild( "index" );
    const char * pUSTag = "indexFiles_useServer";
    if ( !pNode )
    {
        pNode = pVhConfNode;
    }
    else
        pUSTag += 11;
    int useServer = getLongValue( pNode, pUSTag, 0, 2, 1 );
    StringList * pIndexList = NULL;
    if ( useServer != 1 )
    {
        setContextDirIndex( &pVHost->getRootContext(), pNode );
        if ( useServer == 2 )
        {
            pIndexList = pVHost->getRootContext().getIndexFileList();
            if ( pIndexList )
            {
                if ( m_pServer->getIndexFileList() )
                    pIndexList->append(*m_pServer->getIndexFileList() );
            }
            else
                pVHost->getRootContext().setConfigBit( BIT_DIRINDEX, 0 );
        }
    }

    setContextAutoIndex( &pVHost->getRootContext(), pNode );

    pUSTag = getAutoIndexURI( pNode );
    pVHost->setAutoIndexURI( (pUSTag)?pUSTag: m_sAutoIndexURI.c_str() );

    char achURI[] = "/_autoindex/";
    configContext( pVHost, achURI , HandlerType::HT_NULL,
                   "$SERVER_ROOT/share/autoindex/", NULL, 1 );
    return 0;
}


int HttpServerBuilder::initAwstats( HttpVHost * pVHost,
                                    const XmlNode *pNode )
{
    const XmlNode * pAwNode = pNode->getChild( "awstats" );
    if ( !pAwNode )
        return 0;
    int val;
    int len;
    int handlerType;
    const char * pURI;
    const char * pValue;
    char iconURI[]="/awstats/icon/";
    char achBuf[8192];
    LogIdTracker tracker;
    appendLogId( ":awstats" );
    val = getLongValue( pAwNode, "updateMode", 0, 2, 0 );
    if ( val == 0 )
        return 0;

    if ( !pVHost->getAccessLog() )
    {
        LOG_ERR(( "[%s] Virtual host does not have its own access log file, "
                  "AWStats integration is disabled.",
                  getLogId() ));
        return -1;
    }

    if ( getValidPath( achBuf, "$SERVER_ROOT/add-ons/awstats/",
                       "AWStats installation" ) == -1)
    {
        LOG_ERR(( "[%s] Cannot find AWStats installation at [%s],"
                  " AWStats add-on is disabled!" ));
        return -1;
    }
    if ( getValidPath( achBuf, "$SERVER_ROOT/add-ons/awstats/wwwroot/icon/",
                       "AWStats icon directory" ) == -1)
    {
        LOG_ERR(( "[%s] Cannot find AWStats icon directory at [%s],"
                  " AWStats add-on is disabled!" ));
        return -1;
    }
    pVHost->addContext( iconURI, HandlerType::HT_NULL,
                        &achBuf[m_sChroot.len()], NULL, 1 );

    pValue = pAwNode->getChildValue( "workingDir" );
    if (( !pValue )||( getAbsolutePath( achBuf, pValue ) == -1 )||
            ( checkAccess( achBuf ) == -1 ))
    {
        LOG_ERR(( "[%s] AWStats working directory: %s does not exist or access denied, "
                  "please fix it, AWStats integration is disabled.",
                  getLogId(), achBuf ));
        return -1;
    }
    if ( ( pVHost->restrained())&&
            ( strncmp(&achBuf[m_sChroot.len()], pVHost->getVhRoot()->c_str(),
                      pVHost->getVhRoot()->len())))
    {
        LOG_ERR(( "[%s] AWStats working directory: %s is not inside virtual host root: "
                  "%s%s, AWStats integration is disabled.",
                  getLogId(), achBuf, m_sChroot.c_str(), pVHost->getVhRoot()->c_str() ));
        return -1;
    }
    Awstats * pAwstats = new Awstats();
    pAwstats->setMode( val );
    len = strlen( achBuf );
    if ( achBuf[len-1] == '/' )
        achBuf[len-1] = 0;
    //mkdir( achBuf, 0755 );
    pAwstats->setWorkingDir( achBuf );

    pURI = pAwNode->getChildValue( "awstatsURI" );
    if (( !pURI )||( *pURI != '/' )||(*( pURI + strlen( pURI ) - 1 ) != '/')
            ||(strlen( pURI ) > 100 ))
    {
        LOG_WARN(( "[%s] AWStats URI is invalid"
                   ", use default [/awstats/].",
                   getLogId() ));
        iconURI[9] = 0;;
        pURI = iconURI;
    }
    pAwstats->setURI( pURI );

    if ( val == AWS_STATIC )
    {
        handlerType = HandlerType::HT_NULL;
        strcat( achBuf, "/html/" );
    }
    else
    {
        getValidPath( achBuf, "$SERVER_ROOT/add-ons/awstats/wwwroot/cgi-bin/",
                      "AWStats CGI-BIN directory" );
        if ( pVHost->getRootContext().determineMime( "pl", NULL)->getHandler()->getHandlerType() )
            handlerType = HandlerType::HT_NULL;
        else
            handlerType = HandlerType::HT_CGI;
    }
    HttpContext * pContext =
        pVHost->addContext( pURI, handlerType, &achBuf[m_sChroot.len()], NULL, 1 );
    if ( val == AWS_STATIC )
    {
        safe_snprintf( achBuf, 8192,
                  "RewriteRule ^$ awstats.%s.html\n",
                  pVHost->getName() );
    }
    else
    {
        safe_snprintf( achBuf, 8192, "RewriteRule ^$ awstats.pl\n"
                  "RewriteCond %%{QUERY_STRING} !configdir=\n"
                  "RewriteRule ^awstats.pl "
                  "$0?config=%s&configdir=%s/conf [QSA]\n",
                  pVHost->getName(),
                  pAwstats->getWorkingDir() + m_sChroot.len() );
        pContext->setUidMode( UID_DOCROOT );
    }
    pContext->enableRewrite( 1 );
    configContextRewriteRule( pVHost, pContext, achBuf );

    pValue = pAwNode->getChildValue( "realm" );
    if ( pValue )
        setAuthRealm( pVHost, pContext, pValue );

    pValue = pAwNode->getChildValue( "siteDomain" );
    if ( !pValue )
    {
        LOG_WARN(( "[%s] SiteDomain configuration is invalid"
                   ", use default [%s].",
                   getLogId(), m_vhDomain.c_str() ));
        pValue = m_vhDomain.c_str();
    }
    else
    {
        expandDomainNames( pValue, achBuf, 4096 );
        pValue = achBuf;
    }
    pAwstats->setSiteDomain( pValue );

    pValue = pAwNode->getChildValue( "siteAliases" );
    int needConvert = 1;
    if ( !pValue )
    {
        if ( m_vhAliases.len() == 0 )
        {
            safe_snprintf( achBuf, 8192, "127.0.0.1 localhost REGEX[%s]",
                      pAwstats->getSiteDomain() );
            LOG_WARN(( "[%s] SiteAliases configuration is invalid"
                       ", use default [%s].",
                       getLogId(), achBuf ));
            pValue = achBuf;
            needConvert = 0;
        }
        else
            pValue = "$vh_aliases";
    }
    if ( needConvert )
    {
        expandDomainNames( pValue, &achBuf[4096], 4096, ' ' );
        convertToRegex( &achBuf[4096], achBuf, 4096 );
        pValue = achBuf;
        LOG_INFO(( "[%s] SiteAliases is set to '%s'",
                   getLogId(), achBuf ));
    }
    pAwstats->setAliases( pValue );

    val = getLongValue( pAwNode, "updateInterval", 3600, 3600*24, 3600*24 );
    if ( val%3600 != 0 )
        val = (( val + 3599 ) / 3600) * 3600;

    pAwstats->setInterval( val );

    val = getLongValue( pAwNode, "updateOffset", 0, LONG_MAX, 0 );
    if ( val > pAwstats->getInterval() )
        val %= pAwstats->getInterval();
    pAwstats->setOffset( val );

    pVHost->setAwstats( pAwstats );
    return 0;
}



int HttpServerBuilder::configVhost( HttpVHost* pVHost,
                                    const XmlNode* pVhConfNode )
{

    assert( pVhConfNode != NULL );

    if ( configVHBasics( pVHost, pVhConfNode ) != 0 )
        return 1;
    pVHost->updateUGid( getLogId(), pVHost->getDocRoot()->c_str() );

    const XmlNode* p0;
    HttpContext * pRootContext = &pVHost->getRootContext();
    initErrorLog( *pVHost, pVhConfNode, 0 );
    configVHSecurity( pVHost, pVhConfNode );
    {
        LogIdTracker tracker;
        appendLogId( ":epsr" );
        initExtProcessors( pVhConfNode, pVHost );
    }

    pVHost->getRootContext().setParent(
        &HttpServer::getInstance().getServerContext() );

    initAccessLog( *pVHost, pVhConfNode, 0 );
    configVHScriptHandler( pVHost, pVhConfNode );
    initAwstats( pVHost, pVhConfNode );
    p0 = pVhConfNode->getChild( "vhssl" );
    if ( p0 )
    {
        SSLContext * pSSLCtx = newSSLContext( p0 );
        if ( pSSLCtx )
            pVHost->setSSLContext( pSSLCtx );
    }

    p0 = pVhConfNode->getChild( "expires" );
    if ( p0 )
    {
        initExpires( p0, &pVHost->getExpires(),
                     &m_pServer->getServerContext().getExpires(),
                     pRootContext );
        HttpGlobals::getMime()->getDefault()->getExpires()->copyExpires(
            m_pServer->getServerContext().getExpires() );
        const char *pValue = p0->getChildValue( "expiresByType" );
        if ( pValue && ( *pValue ))
        {
            pRootContext->initMIME();
            pRootContext->getMIME()->setExpiresByType( pValue,
                    HttpGlobals::getMime(), getLogId() );
        }
    }

    p0 = pVhConfNode->getChild( "rewrite" );
    if ( p0 )
    {
        initRewrite( pVHost, p0 );
    }

    configVHIndexFile( pVHost, pVhConfNode );

    p0 = pVhConfNode->getChild( "customErrorPages" );
    if ( p0 )
        configCustomErrorPages( pRootContext, p0 );

    pRootContext->inherit( NULL );

    configVHContextList( pVHost, pVhConfNode );


    return 0;
}



void HttpServerBuilder::configVHChrootMode( HttpVHost * pVHost,
        const XmlNode * pNode )
{
    int val = getLongValue( pNode, "chrootMode", 0, 2, 0 );
    const char * pValue = pNode->getChildValue( "chrootPath" );
    if ( pValue )
    {
        char achPath[4096];
        char *p  = achPath;
        int  ret = getAbsoluteFile( p, pValue );
        if ( ret )
            val = 0;
        else
        {
            char * p1 = p + m_sChroot.len();
            if ( pVHost->restrained() &&
                    strncmp( p1, pVHost->getVhRoot()->c_str(),
                             pVHost->getVhRoot()->len() ) != 0 )
            {
                LOG_ERR(( "[%s] Chroot path % must be inside virtual host root %s",
                          getLogId(), p1, pVHost->getVhRoot()->c_str() ));
                val = 0;
            }
            pVHost->setChroot( p );
        }

    }
    else
        val = 0;
    pVHost->setChrootMode( val );

}


HttpVHost * HttpServerBuilder::configVHost( const XmlNode* pNode, const char * pName,
        const char * pDomain, const char * pAliases,
        const char * pVhRoot,
        const XmlNode * pConfigNode )
{
    while ( 1 )
    {
        if ( strcmp( pName, DEFAULT_ADMIN_SERVER_NAME ) == 0 )
        {
            LOG_ERR(( "[%s] invalid <name>, %s is used for the "
                      "administration server, ignore!", getLogId(), pName ));
            break;
        }
        LogIdTracker tracker( "config:vhost:" );
        appendLogId( pName );
        if ( !pVhRoot )
            pVhRoot = getTag( pNode, "vhRoot");
        if ( pVhRoot == NULL )
        {
            break;
        }
        if ( getValidChrootPath(m_achVhRoot, pVhRoot, "vhost root" ) != 0 )
            break;
        if ( !pDomain )
            pDomain = pName;
        m_vhDomain.setStr( pDomain );
        if ( !pAliases )
            pAliases = "";
        m_vhAliases.setStr( pAliases );
        HttpVHost* pVHnew = new HttpVHost( pName );
        assert( pVHnew != NULL );
        pVHnew->setVhRoot( m_achVhRoot );
        pVHnew->followSymLink( getLongValue( pNode, "allowSymbolLink", 0, 2,
                                             HttpServerConfig::getInstance().getFollowSymLink() ));
        pVHnew->enableScript( getLongValue(
                                  pNode, "enableScript", 0, 1, 1 ) );
        pVHnew->serverEnabled( getLongValue( pNode, "enabled", 0, 1, 1 ) );
        pVHnew->restrained( getLongValue( pNode, "restrained", 0, 1, 0 ) );

        pVHnew->setUidMode( getLongValue( pNode, "setUIDMode", 0, 2, 0 ) );

        pVHnew->setMaxKAReqs( getLongValue( pNode, "maxKeepAliveReq", 0, 32767,
                                            HttpServerConfig::getInstance().getMaxKeepAliveRequests() ) );

        pVHnew->setSmartKA( getLongValue( pNode, "smartKeepAlive", 0, 1,
                                          HttpServerConfig::getInstance().getSmartKeepAlive() ) );
        setThrottleLimit( pVHnew->getThrottleLimits(), pNode,
                          ThrottleControl::getDefault() );

        if ( configVhost(  pVHnew, pConfigNode ) == 0 )
        {
            return pVHnew;
        }
        delete pVHnew;
        LOG_ERR(( "[%s] configuration failed!",
                  getLogId()));

        break;
    }

    return NULL;
}



HttpVHost * HttpServerBuilder::configVHost( XmlNode* pNode )
{
    XmlNode* pVhConfNode = NULL;
    HttpVHost * pVHost = NULL;
    bool gotConfigFile = false;

    while ( 1 )
    {
        const char* pName = getTag( pNode, "name" );
        if ( pName == NULL )
        {
            break;
        }
        m_sVhName.setStr( pName );
        const char* pVhRoot = getTag( pNode, "vhRoot" );
        if ( pVhRoot == NULL )
        {
            break;
        }
        char achVhConf[MAX_PATH_LEN];
        if ( getValidChrootPath(m_achVhRoot, pVhRoot, "vhost root" ) != 0 )
            break;

        const char * pConfFile = pNode->getChildValue( "configFile" );
        if ( pConfFile != NULL )
        {
            if ( getValidFile( achVhConf, pConfFile, "vhost config" ) == 0 )
            {
                pVhConfNode = parseFile(achVhConf, "virtualHostConfig");
                if ( pVhConfNode == NULL )
                {
                    LOG_ERR(( "[%s] cannot load configure file - %s !",
                              getLogId(), achVhConf ));
                }
                else
                    gotConfigFile = true;
            }
        }
        if (!pVhConfNode)
            pVhConfNode = pNode;
        
        
        const char * pDomain  = pVhConfNode->getChildValue( "domainName" );
        const char * pAliases = pVhConfNode->getChildValue( "hostAliases" );
        pVHost = configVHost( pNode, pName, pDomain, pAliases, pVhRoot, pVhConfNode );
        break;
    }
    
    //If gotConfigFile is true, it means we alloc memory in this function
    if ( gotConfigFile )
        delete pVhConfNode;
    
    return pVHost;
}



int HttpServerBuilder::initVHosts( const XmlNode* pRoot )
{
    LogIdTracker tracker( "config:server:vhosts" );
    const XmlNode* pNode = pRoot->getChild("virtualHostList");
    if (!pNode)
        pNode = pRoot;
    
    const XmlNodeList *pList = pNode->getChildren("virtualHost");
    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for ( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            const XmlNode * pVhostNode = *iter;
            m_achVhRoot[0] = 0;
            HttpVHost * pVHost = configVHost( const_cast <XmlNode *> (pVhostNode) );
            if ( pVHost )
            {
                if ( m_pServer->addVHost( pVHost ) )
                    delete pVHost;
            }
        }
    }
    return 0;
}




int HttpServerBuilder::configVHTemplate( const XmlNode* pNode )
{
    const char * pTemplateName = getTag( pNode, "name" );
    if ( !pTemplateName )
    {
        return -1;
    }
    LogIdTracker tracker;
    appendLogId( pTemplateName );

    const char * pConfFile = getTag( pNode, "templateFile" );
    if ( !pConfFile )
    {
        return -1;
    }
    char achTmpConf[MAX_PATH_LEN];
    if ( getValidFile( achTmpConf, pConfFile, "vhost template config" ) != 0 )
        return -1;
    XmlNode* pTmpConfNode = parseFile(achTmpConf, "virtualHostTemplate");
    if ( pTmpConfNode == NULL )
    {
        LOG_ERR(( "[%s] cannot load configure file - %s !",
                  getLogId(), achTmpConf ));
        return -1;
    }
    XmlNode * pVhConfNode = pTmpConfNode->getChild( "virtualHostConfig" );
    if ( !pVhConfNode )
    {
        LOG_ERR(( "[%s] missing <virtualHostConfig> tag in the template" ));
        delete pTmpConfNode;
        return -1;
    }
    TPointerList<HttpListener> listeners;
    const char * pListeners = pNode->getChildValue( "listeners" );
    if ( pListeners )
    {
        StringList  listenerNames;
        listenerNames.split( pListeners, strlen( pListeners ) + pListeners, "," );
        StringList::iterator iter;
        for ( iter = listenerNames.begin(); iter != listenerNames.end(); ++iter )
        {
            HttpListener * p = m_pServer->getListener( (*iter)->c_str() );
            if ( !p )
            {
                LOG_ERR(( "[%s] Listener [%s] does not exist", getLogId(),
                          (*iter)->c_str() ));
            }
            else
                listeners.push_back( p );
        }
    }
    const XmlNodeList *pList = pNode->getChildren("member");
    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for ( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            const char * pName = (*iter)->getChildValue( "vhName" );
            const char * pDomain = (*iter)->getChildValue( "vhDomain" );
            const char * pAliases = (*iter)->getChildValue( "vhAliases" );
            const char * pVhRoot = (*iter)->getChildValue( "vhRoot" );
            if ( !pName )
                continue;
            if ( !pDomain )
                pDomain = pName;
            if ( m_pServer->getVHost( pName ) )
            {
                LOG_INFO(( "[%s] Virtual host %s already exists, skip template configuration.",
                           getLogId(), pName ));
                continue;
            }
            m_sVhName.setStr( pName );
            LogIdTracker tracker;
            appendLogId( ":" );
            appendLogId( pName );

            HttpVHost * pVHost = configVHost( pTmpConfNode, pName,
                                              pDomain, pAliases, pVhRoot, pVhConfNode );
            if ( !pVHost )
                continue;
            if ( m_pServer->addVHost( pVHost ) )
            {
                delete pVHost;
                continue;
            }
            if ( listeners.size() > 0 )
            {
                TPointerList<HttpListener>::iterator iter;
                for ( iter = listeners.begin(); iter != listeners.end(); ++iter )
                {
                    m_pServer->mapListenerToVHost( (*iter), pVHost, pDomain );
                    if ( pAliases )
                        m_pServer->mapListenerToVHost( (*iter), pVHost, pAliases );

                }

            }

        }
    }
    delete pTmpConfNode;
    return 0;
}



int HttpServerBuilder::initVHTemplates( const XmlNode* pRoot )
{
    LogIdTracker tracker( "config:template:" );
    const XmlNode* pNode = pRoot->getChild("vhTemplateList");
    if ( !pNode )
        pNode = pRoot;
    const XmlNodeList *pList = pNode->getChildren("vhTemplate");
    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for ( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            configVHTemplate( *iter );
        }
    }
    return 0;
}


int HttpServerBuilder::addListener(  const XmlNode* pNode, int isAdmin )
{
    // extract listner info
    while ( 1 )
    {
        if ( strcmp( pNode->getName(), "listener" )!= 0 )
        {
            break;
        }
        const char * pName = getTag( pNode,  "name" );
        if ( pName == NULL )
        {
            break;
        }

        LogIdTracker track;
        appendLogId( ":" );
        appendLogId( pName );
        const char * pAddr = getTag( pNode,  "address" );
        if ( pAddr == NULL )
        {
            break;
        }
        int secure = 0;
        const char * pSecure = pNode->getChildValue( "secure" );
        if ( pSecure )
            secure = atoi( pSecure );
        HttpListener * pListener;
        if ( secure == 0 )
        {
            pListener = m_pServer->addListener(pName, pAddr);
            if ( pListener == NULL )
            {
                LOG_ERR(( "[%s] failed to start listener on address %s!",
                          getLogId(), pAddr ));
                break;
            }
        }
        else
        {
            const char* pCertFile = getTag( pNode,  "certFile" );
            if ( !pCertFile )
            {
                break;
            }
            const char* pKeyFile = getTag( pNode, "keyFile" );
            if ( !pKeyFile )
            {
                break;
            }
            char achCert[MAX_PATH_LEN];
            if ( getValidFile(achCert, pCertFile, "certificate file" ) != 0 )
                break;
            char achKey[MAX_PATH_LEN];
            if ( getValidFile(achKey, pKeyFile, "key file") != 0 )
                break;
            const char * pCiphers = pNode->getChildValue( "ciphers" );
            if ( !pCiphers )
            {
                pCiphers = "ALL:!ADH:!EXPORT56:RC4+RSA:+HIGH:+MEDIUM:+SSLv2:+EXP";
            }
            const char * pCAPath = pNode->getChildValue( "CACertPath" );
            const char * pCAFile = pNode->getChildValue( "CACertFile" );
            char achCAFile[MAX_PATH_LEN];
            char achCAPath[MAX_PATH_LEN];
            if ( pCAPath )
            {
                if ( getValidPath( achCAPath, pCAPath, "CA Certificate path" ) != 0 )
                    break;
                pCAPath = achCAPath;
            }
            if ( pCAFile )
            {
                if ( getValidFile( achCAFile, pCAFile, "CA Certificate file" ) != 0 )
                    break;
                pCAFile = achCAFile;
            }
            int cv = getLongValue( pNode, "clientVerify", 0, 3, 0 );

            pListener = m_pServer->addSSLListener(pName, pAddr, achCert,
                                                  achKey, pCAFile, pCAPath, pCiphers, getLongValue( pNode, "certChain", 0, 1, 0 ), (cv!=0)  );
            if ( pListener == NULL )
            {
                LOG_ERR(( "[%s] failed to start SSL listener on address %s!",
                          getLogId(), pAddr ));
                break;
            }
            if ( cv > 0 )
            {
                SSLContext * pSSL = pListener->getVHostMap()->getSSLContext();
                if ( pSSL )
                {
                    pSSL->setClientVerify( cv, getLongValue( pNode, "verifyDepth", 1, INT_MAX, 1 ) );
                    pCAPath = pNode->getChildValue( "crlPath" );
                    pCAFile = pNode->getChildValue( "crlFile" );
                    if ( pCAPath )
                    {
                        if ( getValidPath( achCAPath, pCAPath, "CRL path" ) != 0 )
                            break;
                        pCAPath = achCAPath;
                    }
                    if ( pCAFile )
                    {
                        if ( getValidFile( achCAFile, pCAFile, "CRL file" ) != 0 )
                            break;
                        pCAFile = achCAFile;
                    }
                    if ( pCAPath || pCAFile )
                        pSSL->addCRL( achCAFile, achCAPath );
                }
            }
        }
        if ( HttpGlobals::s_children > 1 )
        {
            int binding = getLongValue( pNode, "binding", LONG_MIN, LONG_MAX, -1 );
            if ( (binding & ((1<<HttpGlobals::s_children) - 1)) == 0 )
            {
                LOG_WARN(( "[%s] This listener is not bound to any server process, "
                           "it is inaccessible." ));
            }
            pListener->setBinding( binding );
        }
        pListener->setName( pName );
        pListener->setAdmin( isAdmin );
        return 0;
    }

    return -1;
}

int HttpServerBuilder::initVirtualHostMapping(
    HttpListener * pListener, const XmlNode* pNode, const char * pVHostName)
{
    int add = 0;
    if (( pNode != NULL )&&( pListener ))
    {
        const XmlNodeList *pList = pNode->getChildren("vhostMap");
        if ( pList ) //old type has this mudle name
        {
            XmlNodeList::const_iterator iter;
            for ( iter = pList->begin(); iter != pList->end(); ++iter )
            {
                XmlNode * pListenerNode = *iter;
                if ( addVirtualHostMapping(  pListener, pListenerNode,
                                             pVHostName ) == 0 )
                    ++add;
            }
        }
        else
        {
            XmlNodeList list;
            pNode->getAllChildren(list);
            XmlNodeList::const_iterator iter;
            for( iter = list.begin(); iter != list.end(); ++iter )
            {
                if (strcasecmp((*iter)->getName(), "map") == 0)
                    if ( addVirtualHostMapping(  pListener, (*iter)->getValue(),
                                            pVHostName ) == 0 )
                ++add;
            }
        }
    }
        
    return add;
}

int HttpServerBuilder::addVirtualHostMapping(
    HttpListener * pListener, const char *value, const char * pVHostName )
{

    char pVHost[256] = {0};
    const char *p = strchr(value, ' ');
    if (!p)
        p = strchr(value, '\t');
    if (p) 
        memcpy(pVHost, value, p - value);
    else
        return -1;
    
    if (( pVHostName )&&( strcmp( pVHostName, pVHost ) != 0 ))
        return 1;

    if ( strcmp( pVHost, DEFAULT_ADMIN_SERVER_NAME ) == 0 )
    {
        LOG_ERR(( "[%s] can't bind administration server to normal listener %s, "
                  "instead, configure listeners for administration server in "
                  "$SERVER_ROOT/admin/conf/admin_config.xml",
                  getLogId(), pListener->getAddrStr() ));
        return -1;
    }

    //at least move ahead for one char position
    const char* pDomains = p;
    skipLeadingSpace(&pDomains);
    if ( pDomains == NULL )
    {
        LOG_ERR(( "[%s] missing <domain> in <vhostMap> - vhost = %s listener = %s",
                  getLogId(), pVHost, pListener ));
        return -1;
    }
    return m_pServer->mapListenerToVHost( pListener, pDomains, pVHost );
}

int HttpServerBuilder::addVirtualHostMapping(
    HttpListener * pListener, const XmlNode* pNode, const char * pVHostName )
{
    const char* pVHost = getTag( pNode, "vhost");
    if ( pVHost == NULL )
    {
        return -1;
    }
    
    if (( pVHostName )&&( strcmp( pVHostName, pVHost ) != 0 ))
        return 1;

    if ( strcmp( pVHost, DEFAULT_ADMIN_SERVER_NAME ) == 0 )
    {
        LOG_ERR(( "[%s] can't bind administration server to normal listener %s, "
                  "instead, configure listeners for administration server in "
                  "$SERVER_ROOT/admin/conf/admin_config.xml",
                  getLogId(), pListener->getAddrStr() ));
        return -1;
    }

    const char* pDomains = pNode->getChildValue( "domain" );
    if ( pDomains == NULL )
    {
        LOG_ERR(( "[%s] missing <domain> in <vhostMap> - vhost = %s listener = %s",
                  getLogId(), pVHost, pListener ));
        return -1;
    }
    return m_pServer->mapListenerToVHost( pListener, pDomains, pVHost );
}


int HttpServerBuilder::initListeners(
    const XmlNode* pRoot, int isAdmin )
{
    const XmlNode* pNode = pRoot->getChild("listenerList");
    if ( !pNode )
        pNode = pRoot;
    
    XmlNodeList list;
    int c = pNode->getAllChildren(list);
    int add = 0 ;
    for ( int i = 0 ; i < c ; ++ i )
    {
        XmlNode * pListenerNode = list[i];
        if ( addListener(  pListenerNode, isAdmin ) == 0 )
            ++add ;
    }
    return add;
}

int HttpServerBuilder::initListenerVHostMap(  const XmlNode* pRoot,
        const char * pVHostName )
{
    int confType = 0;
    const XmlNode* pNode = pRoot->getChild("listenerList");
    if ( !pNode )
    {
        pNode = pRoot;
        confType = 1;
    }
    const XmlNodeList *pList = pNode->getChildren("listener");
    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for ( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            const XmlNode * pListenerNode = *iter;
            const char * pName = pListenerNode->getChildValue( "name" );
            HttpListener * pListener = m_pServer->getListener( pName );
            if ( pListener )
            {
                if ( !pVHostName )
                    pListener->getVHostMap()->clear();
                if (( initVirtualHostMapping(  pListener,
                    ((confType == 0)?(pListenerNode->getChild( "vhostMapList" )):(pListenerNode)), pVHostName ) > 0 )&&
                        ( pVHostName ))
                    pListener->endConfig();

            }
        }
    }
    return 0;
}

static void initRLimits( const XmlNode * pNode, RLimits * pRLimits )
{
    if ( !pNode )
        return;
    pRLimits->setProcLimit(
        getLongValue( pNode, "procSoftLimit", 0, INT_MAX, 0 ),
        getLongValue( pNode, "procHardLimit", 0, INT_MAX, 0 ) );

    pRLimits->setCPULimit(
        getLongValue( pNode, "CPUSoftLimit", 0, INT_MAX, 0 ),
        getLongValue( pNode, "CPUHardLimit", 0, INT_MAX, 0 ) );
    long memSoft = getLongValue( pNode, "memSoftLimit", 0, INT_MAX, 0 );
    long memHard = getLongValue( pNode, "memHardLimit", 0, INT_MAX, 0 );
    if (( memSoft & (memSoft < 1024 * 1024 ))||
            ( memHard & (memHard < 1024 * 1024 )))
    {
        LOG_ERR(( "Memory limit is too low with %ld/%ld", memSoft, memHard ));
    }
    else
        pRLimits->setDataLimit( memSoft, memHard );
}

void HttpServerBuilder::initExtAppConfig( const XmlNode * pNode, ExtWorkerConfig * pConfig, int autoStart )
{
    const char * pValue;
    int iMaxConns = getLongValue( pNode, "maxConns", 1, 2000, 1 );
    int iRetryTimeout = getLongValue( pNode, "retryTimeout", 0, LONG_MAX, 10 );
    int iInitTimeout = getLongValue( pNode, "initTimeout", 1, LONG_MAX, 3 );
    int iBuffer = getLongValue( pNode, "respBuffer", 0, 2, 1 );
    int iKeepAlive = getLongValue( pNode, "persistConn", 0, 1, 1 );
    int iKeepAliveTimeout = getLongValue( pNode, "pcKeepAliveTimeout", -1, INT_MAX, INT_MAX );
    if ( iKeepAliveTimeout == -1 )
        iKeepAliveTimeout = INT_MAX;
    if ( iBuffer == 1 )
        iBuffer = 0;
    else if ( iBuffer == 0 )
        iBuffer = HEC_RESP_NOBUFFER;
    else if ( iBuffer == 2 )
        iBuffer = HEC_RESP_NPH;
    pConfig->setPersistConn( iKeepAlive );
    pConfig->setKeepAliveTimeout( iKeepAliveTimeout );
    pConfig->setMaxConns( iMaxConns );
    pConfig->setTimeout( iInitTimeout );
    pConfig->setRetryTimeout( iRetryTimeout );
    pConfig->setBuffering( iBuffer );
    pConfig->clearEnv();
    const XmlNodeList *pList = pNode->getChildren("env");
    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for ( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            pValue = (*iter)->getValue();
            if ( pValue )
            {
                pConfig->addEnv( (*iter)->getValue() );
            }
        }
    }
    if ( autoStart )
    {
        LocalWorkerConfig * pLWConfig = dynamic_cast< LocalWorkerConfig *>(pConfig);
        int backlog = getLongValue( pNode, "backlog", 1, 100, 10);
        int priority = getLongValue( pNode, "priority", -20, 20, HttpGlobals::s_priority + 1 );
        if ( priority > 20 )
            priority = 20;
        if ( priority < HttpGlobals::s_priority )
            priority = HttpGlobals::s_priority;
        long l = getLongValue( pNode, "extMaxIdleTime", -1, INT_MAX, INT_MAX );
        if ( l == -1 )
            l = INT_MAX;
        pLWConfig->setPriority( priority );
        pLWConfig->setBackLog( backlog );
        pLWConfig->setMaxIdleTime( l );

        pLWConfig->setRunOnStartUp( getLongValue( pNode, "runOnStartUp", 0, 1, 0 ) );

        RLimits limits;
        limits = *m_pRLimits;
        limits.setCPULimit( 0, 0 );
        initRLimits( pNode, &limits );
        pLWConfig->setRLimits( &limits );
        Env * pEnv = pLWConfig->getEnv();
        if ( pEnv->find( "PATH" ) == NULL )
        {
            pEnv->add( "PATH=/bin:/usr/bin" );
        }
    }
}


int HttpServerBuilder::addExtProcessor(  const XmlNode* pNode, const HttpVHost * pVHost )
{
    if ( strncasecmp( pNode->getName(), "extProcessor", 12 ) != 0 )
    {
        return 1;
    }

    char achName[256];
    const char * pName = getExpandedTag( pNode, "name", achName, 256 );
    if ( pName == NULL )
    {
        return 1;
    }

    LogIdTracker track;
    appendLogId( ":" );
    appendLogId( pName );

    const char * pType = getTag( pNode, "type" );
    if ( !pType )
        return 1;
    int iType;
    int role;
    iType = HandlerType::getHandlerType( pType, role );
    if (( iType < HandlerType::HT_FASTCGI )||
            ( iType >= HandlerType::HT_END ))
    {
        LOG_ERR(( "[%s] unknown external processor <type>: %s",
                    getLogId(), pType ));
        return 1;
    }
    if ( HandlerType::HT_LOADBALANCER == iType )
        return 1;
    iType -= HandlerType::HT_CGI;

    char achAddress[128];
    const char* pUri = getExpandedTag( pNode, "address", achAddress, 128 );
    if (( pUri == NULL )&&( iType != HandlerType::HT_LOGGER ))
    {
        return 1;
    }

//        const char * pValue;
    /*        int iMaxConns = getLongValue( pNode, "maxConns", 1, 2000, 1 );
            int iRetryTimeout = getLongValue( pNode, "retryTimeout", 0, LONG_MAX, 10 );
            int iInitTimeout = getLongValue( pNode, "initTimeout", 1, LONG_MAX, 3 );
            int iBuffer = getLongValue( pNode, "respBuffer", 0, 2, 1 );
            int iKeepAlive = getLongValue( pNode, "persistConn", 0, 1, 1 );
            int iKeepAliveTimeout = getLongValue( pNode, "pcKeepAliveTimeout", -1, INT_MAX, INT_MAX );
            if ( iKeepAliveTimeout == -1 )
                iKeepAliveTimeout = INT_MAX;*/
    //int iCompress = getLongValue( pNode, "forcedCompress", 0, 1, 1 );

    char buf[MAX_PATH_LEN];
    int iAutoStart = 0;
    const char* pPath = NULL;
    int instances = 1;
//        if ( iCompress )
//            iBuffer |= HEC_RESP_GZIP;

    if (( iType == EA_FCGI )||( iType == EA_LOGGER )||(iType == EA_LSAPI))
    {
        if ( iType == EA_LOGGER )
            iAutoStart = 1;
        else
        {
            iAutoStart = getLongValue( pNode, "autoStart", 0, 1, 0 );

            //if ( pValue )
            //    iAutoStart = atoi( pValue );
        }
        pPath = pNode->getChildValue( "path" );
        if (( iAutoStart )&&(( !pPath || !*pPath )))
        {
            LOG_ERR(( MISSING_TAG, getLogId(), "path" ));
            return 1;
        }

        if ( iAutoStart )
        {
            if ( getAbsoluteFile( buf, pPath ) != 0 )
                return 1;
            char * pCmd = buf;
            char * p = (char *)StringTool::strNextArg( pCmd );
            if ( p )
                *p = 0;
            if (access( pCmd, X_OK) == -1)
            {
                LOG_ERR(( "[%s] invalid path - %s, "
                            "it cannot be started by Web server!",
                            getLogId(), buf ));
                return 1;
            }
            if ( p )
                *p = ' ';
            if ( pCmd != buf )
                buf[m_sChroot.len()] = *buf;
            pPath = &buf[m_sChroot.len()];
            instances = getLongValue( pNode, "instances", 1, INT_MAX, 1 );
            if ( instances > 2000 )
                instances = 2000;
            if ( instances >
                    HttpServerConfig::getInstance().getMaxFcgiInstances() )
            {
                instances = HttpServerConfig::getInstance().getMaxFcgiInstances();
                LOG_WARN(( "[%s] <instances> is too large, use default max:%d",
                            getLogId(), instances ));
            }
        }
    }

    GSockAddr addr;
    if ( addr.set( pUri, NO_ANY ) )
    {
        LOG_ERR(( "[%s] failed to set socket address %s!", getLogId(),
                    pUri ));
        return 1;
    }

    ExtWorker * pWorker;
    ExtWorkerConfig * pConfig = NULL;
    int selfManaged = 0;

    pWorker = ExtAppRegistry::addApp( iType, pName );

    if ( !pWorker )
    {
        LOG_ERR(( "[%s] failed to add external processor!", getLogId()));
        return 1;
    }
    else
    {
        pConfig = pWorker->getConfigPointer();
        assert( pConfig );
        if ( pUri )
            if ( pWorker->setURL( pUri ) )
            {
                LOG_ERR(( "[%s] failed to set socket address to %s!", getLogId(),
                            pName ));
                return 1;
            }
        if ( pVHost )
        {
            pConfig->setVHost( pVHost );
        }
        pWorker->setRole( role );
    }

    initExtAppConfig( pNode, pConfig, iAutoStart );
    {
        static const char * instanceEnv[] =
            { "PHP_FCGI_CHILDREN",  "PHP_LSAPI_CHILDREN",
                "LSAPI_CHILDREN"
            };
        size_t i;
        Env * pEnv = pConfig->getEnv();
        const char * pEnvValue = NULL;
        for ( i = 0; i < sizeof( instanceEnv ) / sizeof( char *); ++i )
        {
            pEnvValue = pEnv->find( instanceEnv[i] );
            if ( pEnvValue != NULL )
                break;
        }


        if ( pEnvValue  )
        {
            int children = atol( pEnvValue );
            if (( children > 0 )&&(children * instances < pConfig->getMaxConns() ))
            {
                LOG_WARN(( "[%s] Improper configuration: the value of "
                            "%s should not be less than 'Max "
                            "connections', 'Max connections' is reduced to %d."
                            , getLogId(), instanceEnv[i], children * instances ));
                pConfig->setMaxConns( children * instances);
            }
            selfManaged = 1;
        }
        pEnv->add( 0, 0, 0, 0 );
    }

    if ( iAutoStart )
    {
        LocalWorker * pApp = dynamic_cast<LocalWorker *>( pWorker );
        if ( pApp )
        {
            LocalWorkerConfig& config = pApp->getConfig();
            config.setAppPath( pPath );
            config.setInstances( instances );
            config.setSelfManaged( selfManaged );

            if ( !pVHost ) {
                const char * pUser = pNode->getChildValue("extUser");
                const char * pGroup = pNode->getChildValue("extGroup");
                gid_t gid = -1;
                struct passwd * pw = getUGid( pUser, pGroup, gid );
                if ( pw )
                {
                    if ( gid == -1 )
                        gid = pw->pw_gid;
                    if ((iType != EA_LOGGER)&&( ( pw->pw_uid < HttpGlobals::s_uidMin )||
                        ( gid < HttpGlobals::s_gidMin )))
                    {
                        LOG_NOTICE(( "[%s] ExtApp suExec access denied,"
                        " UID or GID of VHost document root is smaller "
                        "than minimum UID, GID configured. ", getLogId() ));
                    }
                    else
                    {
                        /*
                        if ( config.getRunOnStartUp() == 2 )
                        {
                            char achEnv[1024];
                            snprintf( achEnv, 1024, "LSAPI_DEFAULT_UID=%d", pw->pw_uid );
                            pEnv->add( achEnv );
                            snprintf( achEnv, 1024, "LSAPI_DEFAULT_GID=%d", gid );
                            pEnv->add( achEnv );
                        }
                        else
                            */
                            config.setUGid( pw->pw_uid, gid );
                    }
                }
            }
            
            if (( instances != 1 )&&
                    ( config.getMaxConns() > instances ))
            {
                LOG_NOTICE(( "[%s] Possible mis-configuration: 'Instances'=%d, "
                                "'Max connections'=%d, unless one Fast CGI process is "
                                "capable of handling multiple connections, "
                                "you should set 'Instances' greater or equal to "
                                "'Max connections'.",
                                getLogId(), instances, config.getMaxConns() ));
                config.setMaxConns( instances );
            }
            RLimits * pLimits = config.getRLimits();
#if defined(RLIMIT_NPROC)
            int mini_nproc = (3 * config.getMaxConns() + 50) * HttpGlobals::s_children;
            struct rlimit  * pNProc = pLimits->getProcLimit();
            if ((( pNProc->rlim_cur > 0 )&&( (int)pNProc->rlim_cur < mini_nproc ))
                    ||(( pNProc->rlim_max > 0 )&&( (int)pNProc->rlim_max < mini_nproc )))
            {
                LOG_NOTICE(( "[%s]'Process Limit' probably is too low, "
                                "adjust the limit to: %d.",
                                getLogId(), mini_nproc ));
                pLimits->setProcLimit( mini_nproc, mini_nproc );
            }
#endif

        }
    }
    return 0;

}

int HttpServerBuilder::addLoadBalancer(  const XmlNode* pNode, const HttpVHost * pVHost )
{
    if ( strncasecmp( pNode->getName(), "extProcessor", 12 ) != 0 )
    {
        return 1;
    }

    char achName[256];
    const char * pName = getExpandedTag( pNode, "name", achName, 256 );
    if ( pName == NULL )
    {
        return 1;
    }

    LogIdTracker track;
    appendLogId( ":" );
    appendLogId( pName );

    const char * pType = getTag( pNode, "type" );
    if ( !pType )
        return 1;
    int iType;
    int role;
    iType = HandlerType::getHandlerType( pType, role );
    if ( HandlerType::HT_LOADBALANCER != iType )
        return 1;
    iType -= HandlerType::HT_CGI;

    LoadBalancer * pLB;

    pLB = (LoadBalancer *)ExtAppRegistry::addApp( iType, pName );

    if ( !pLB )
    {
        LOG_ERR(( "[%s] failed to add load balancer!", getLogId()));
        return 1;
    }
    else
    {
        pLB->clearWorkerList();
        if ( pVHost )
        {
            pLB->getConfigPointer()->setVHost( pVHost );
        }
        const char * pWorkers = pNode->getChildValue( "workers" );
        if ( pWorkers )
        {
            StringList workerList;
            workerList.split( pWorkers, pWorkers + strlen( pWorkers ), "," );
            StringList::const_iterator iter;
            for ( iter = workerList.begin(); iter != workerList.end(); ++iter )
            {
                const char * pType = (*iter)->c_str();
                char * pName = (char *)strstr( pType, "::" );
                if ( !pName )
                {
                    LOG_ERR(( "[%s] invalid worker syntax [%s].",
                                getLogId(), pType ));
                    continue;
                }
                *pName = 0;
                pName += 2;
                iType = HandlerType::getHandlerType( pType, role );
                if (( iType == HandlerType::HT_LOADBALANCER )||
                        ( iType == HandlerType::HT_LOGGER )||
                        ( iType < HandlerType::HT_CGI ))
                {
                    LOG_ERR(( "[%s] invalid handler type [%s] for load balancer worker.",
                                getLogId(), pType ));
                    continue;
                }
                const HttpHandler * pWorker = getHandler( pVHost, pType, pName );
                if ( pWorker )
                    pLB->addWorker( (ExtWorker *)pWorker );
            }
        }
    }
    return 0;
   
}


int HttpServerBuilder::initExtProcessors(  const XmlNode* pRoot, const HttpVHost * pVHost )
{
    const XmlNode* pNode = pRoot->getChild("extProcessorList");
    if ( !pNode )
        pNode = pRoot;

    XmlNodeList list;
    int c = pNode->getAllChildren(list);
    int add = 0 ;
    XmlNode * pExtAppNode;
    for ( int i = 0 ; i < c ; ++ i )
    {
        pExtAppNode = list[i];
        if ( addExtProcessor(  pExtAppNode, pVHost ) == 0 )
            add ++ ;
    }
    for ( int i = 0 ; i < c ; ++ i )
    {
        pExtAppNode = list[i];
        if ( addLoadBalancer(  pExtAppNode, pVHost ) == 0 )
            add ++ ;
    }

    return 0;

}



int HttpServerBuilder::initTuning(  const XmlNode* pRoot )
{
    LogIdTracker tracker( "config:server:tuning" );
    const XmlNode* pNode = pRoot->getChild("tuning");
    if ( pNode == NULL )
    {
        LOG_NOTICE(( "[%s] no tuning set up!" ));
        return -1;
    }
    //connections
    m_pServer->setMaxConns( getLongValue(pNode, "maxConnections", 1, 65535, 400));
    m_pServer->setMaxSSLConns(getLongValue(pNode, "maxSSLConnections", 0, 65535, 100 ));
    HttpListener::setSockSendBufSize(
        getLongValue( pNode, "sndBufSize", 0, 256 * 1024, 0 ));
    HttpListener::setSockRecvBufSize(
        getLongValue( pNode, "rcvBufSize", 0, 256 * 1024, 0 ));
    HttpServerConfig& config = HttpServerConfig::getInstance();
    config.setKeepAliveTimeout(
        getLongValue( pNode, "keepAliveTimeout", 1, 10000, 15 ) );
    config.setConnTimeOut(getLongValue( pNode, "connTimeout", 1, 10000, 30));
    config.setMaxKeepAliveRequests(
        getLongValue(pNode, "maxKeepAliveReq", 0, 32767, 100 ));
    config.setSmartKeepAlive( getLongValue( pNode, "smartKeepAlive", 0, 1, 0 ));
    //HTTP request/response
    config.setMaxURLLen( getLongValue( pNode, "maxReqURLLen", 100,
                                       MAX_URL_LEN , DEFAULT_URL_LEN + 20 ) );
    config.setMaxHeaderBufLen( getLongValue( pNode, "maxReqHeaderSize",
                               1024, MAX_REQ_HEADER_BUF_LEN, DEFAULT_REQ_HEADER_BUF_LEN ) );
    config.setMaxReqBodyLen( getLongValue( pNode, "maxReqBodySize",
                                           4096, MAX_REQ_BODY_LEN, DEFAULT_REQ_BODY_LEN ) );
    config.setMaxDynRespLen( getLongValue( pNode, "maxDynRespSize", 4096,
                                           MAX_DYN_RESP_LEN, DEFAULT_DYN_RESP_LEN ) );
    config.setMaxDynRespHeaderLen( getLongValue( pNode, "maxDynRespHeaderSize",
                                   200, MAX_DYN_RESP_HEADER_LEN, DEFAULT_DYN_RESP_HEADER_LEN ) );
    FileCacheDataEx::setTotalInMemCacheSize( getLongValue( pNode, "totalInMemCacheSize",
            0, LONG_MAX, DEFAULT_TOTAL_INMEM_CACHE ) );
    FileCacheDataEx::setTotalMMapCacheSize( getLongValue( pNode, "totalMMapCacheSize",
                                            0, LONG_MAX, DEFAULT_TOTAL_MMAP_CACHE ) );
    FileCacheDataEx::setMaxInMemCacheSize( getLongValue( pNode, "maxCachedFileSize",
                                           0, 16384, 4096 ) );
    FileCacheDataEx::setMaxMMapCacheSize( getLongValue( pNode, "maxMMapFileSize",
                                          0, LONG_MAX, 256 * 1024 ) );
    int val = getLongValue( pNode, "useSendfile", 0, 1, 0 );
    config.setUseSendfile( val );
    if ( val )
        FileCacheDataEx::setMaxMMapCacheSize( 0 );

    const char * pValue = pNode->getChildValue( "SSLCryptoDevice" );
    if ( SSLEngine::init( pValue ) == -1 )
    {
        LOG_WARN(( "[%s] Failed to initialize SSL Accelerator Device: %s,"
                   " SSL hardware acceleration is disabled!", getLogId(), pValue ));
    }

    // GZIP compression
    config.setGzipCompress( getLongValue( pNode, "enableGzipCompress",
                                          0, 1, 0 ) );
    config.setDynGzipCompress( getLongValue( pNode, "enableDynGzipCompress",
                               0, 1, 0 ) );
    config.setCompressLevel( getLongValue( pNode, "gzipCompressLevel",
                                           1, 9, 4 ) );
    HttpGlobals::getMime()->setCompressableByType(
        pNode->getChildValue( "compressibleTypes" ), NULL, getLogId() );
    StaticFileCacheData::setUpdateStaticGzipFile(
        getLongValue( pNode, "gzipAutoUpdateStatic", 0, 1, 0 ),
        getLongValue( pNode, "gzipStaticCompressLevel", 1, 9, 6 ),
        getLongValue( pNode, "gzipMinFileSize", 200, LONG_MAX, 300 ),
        getLongValue( pNode, "gzipMaxFileSize", 200, LONG_MAX, 1024 * 1024 )
    );
    pValue = pNode->getChildValue( "gzipCacheDir" );
    if ( !pValue )
    {
        pValue = HttpServer::getInstance().getSwapDir();
    }
    else
    {
        char achBuf[MAX_PATH_LEN];
        if ( getAbsolutePath( achBuf, pValue ) == -1 )
        {
            LOG_ERR(( "[%s] path of gzip cache is invalid, use default.", getLogId() ));
            pValue = m_pServer->getSwapDir();
        }
        else
        {
            if ( GPath::createMissingPath( achBuf, 0700 ) ==-1 )
            {
                LOG_ERR(( "[%s] Failed to create directory: %s .", getLogId(),
                            achBuf ));
                pValue = m_pServer->getSwapDir();
            }
            else
            {
                chown( achBuf, HttpGlobals::s_uid, HttpGlobals::s_gid );
                pValue = achBuf;
                pValue += m_sChroot.len();
            }
        }
    }
    StaticFileCacheData::setGzipCachePath( pValue );

    return 0;
}

int HttpServerBuilder::getLogFilePath( char * pBuf, const XmlNode * pNode )
{
    const char* pValue = getTag( pNode, "fileName");
    if ( pValue == NULL )
    {
        return 1;
    }
    if ( getAbsoluteFile( pBuf, pValue ) != 0 )
    {
        LOG_ERR(( INVAL_PATH, getLogId(), "log file",  pValue ));
        return 1;
    }
    if ( GPath::isWritable( pBuf ) == false )
    {
        LOG_ERR(( "[%s] log file is not writable - %s", getLogId(), pBuf ));
        return 1;
    }
    return 0;
}

#include <log4cxx/appender.h>

int HttpServerBuilder::initAccessLog( HttpLogSource& source, const XmlNode* pRoot,
                                      int setDebugLevel )
{
    LogIdTracker tracker;
    appendLogId( ":accesslog" );
    const XmlNode* p0 = pRoot->getChild("logging");
    if ( !p0 )
        p0 = pRoot;
    
    const XmlNode * pNode1 = p0->getChild("accessLog");
    if ( pNode1 == NULL )
    {
        if ( setDebugLevel )
        {
            LOG_ERR(( MISSING_TAG, getLogId(), "accessLog" ));
            return -1;
        }
    }
    else
    {
        off_t rollingSize = 1024 * 1024 * 1024;
        int useServer = getLongValue( pNode1, "useServer", 0, 2, 2 );
        source.enableAccessLog( useServer != 2 );

        if ( setDebugLevel )
        {
            const char * pFmt = pNode1->getChildValue( "logFormat" );
            if ( pFmt )
            {
                m_sDefaultLogFormat.setStr( pFmt );
            }
        }
        
        if ( setDebugLevel || useServer == 0 )
        {
            if ( initAccessLog( source, pNode1, &rollingSize ) != 0 )
            {
                LOG_ERR(( "[%s] failed to set up access log!", getLogId() ));
                return -1;
            }
        }
        const char * pByteLog = pNode1->getChildValue( "bytesLog" );
        if ( pByteLog )
        {
            char buf[MAX_PATH_LEN];
            if ( getAbsoluteFile( buf, pByteLog ) != 0 )
            {
                LOG_ERR(( INVAL_PATH, getLogId(), "log file",  pByteLog ));
                return -1;
            }
            if ( GPath::isWritable( buf ) == false )
            {
                LOG_ERR(( "[%s] log file is not writable - %s", getLogId(), buf ));
                return -1;
            }
            source.setBytesLogFilePath( buf, rollingSize );
        }
    }
    return 0;
}

int HttpServerBuilder::initAccessLog( HttpLogSource& logSource, const XmlNode* pNode,
                                      off_t * pRollingSize )
{
    char buf[MAX_PATH_LEN];
    int ret = -1;
    const char * pPipeLogger = pNode->getChildValue( "pipedLogger" );
    if ( pPipeLogger )
        ret = logSource.setAccessLogFile( pPipeLogger, 1 );
    if ( ret == -1 )
    {
        ret = getLogFilePath( buf, pNode );
        if ( ret )
            return ret;
        ret = logSource.setAccessLogFile(buf, 0);
    }
    if ( ret == 0 )
    {
        AccessLog * pLog = logSource.getAccessLog();
        const char * pValue = pNode->getChildValue( "keepDays" );
        if ( pValue )
            pLog->getAppender()->setKeepDays( atoi( pValue ) );
        pLog->setLogHeaders( getLongValue( pNode, "logHeaders", 0, 7, 3 ) );

        const char * pFmt = pNode->getChildValue( "logFormat" );
        if ( !pFmt )
            pFmt = m_sDefaultLogFormat.c_str();
        if ( pFmt )
            if ( pLog->setCustomLog( pFmt ) == -1 )
            {
                LOG_ERR(( "[%s] failed to setup custom log format [%s]", getLogId(), pFmt ));
            }
            pValue = pNode->getChildValue( "logReferer" );

        if ( pValue )
            pLog->accessLogReferer( atoi( pValue ) );
        pValue = pNode->getChildValue( "logUserAgent" );
        if ( pValue )
            pLog->accessLogAgent( atoi( pValue ) );
        pValue = pNode->getChildValue( "rollingSize" );
        if ( pValue )
        {
            off_t size = getLongValue( pValue );
            if (( size > 0 )&&( size < 1024 * 1024 ))
                size = 1024 * 1024;
            if ( pRollingSize )
                *pRollingSize = size;
            pLog->getAppender()->setRollingSize( size );
        }
        pLog->getAppender()->setCompress( getLongValue( pNode, "compressArchive", 0, 1, 0 ) );
    }
    return ret;
}


int HttpServerBuilder::initErrorLog2( HttpLogSource& logSource, const XmlNode* pNode,
                                     int setDebugLevel )
{
    char buf[MAX_PATH_LEN];
    int ret = getLogFilePath( buf, pNode );
    if ( ret )
        return ret;
    logSource.setErrorLogFile(buf);

    const char* pValue = getTag( pNode, "logLevel");
    if ( pValue != NULL )
        logSource.setLogLevel(pValue);
    off_t rollSize =
        getLongValue( pNode, "rollingSize", 1024*1024,
                      INT_MAX, 1024*10240 );
    int days = getLongValue( pNode, "keepDays", 0,
                     LLONG_MAX, 30 );
    logSource.setErrorLogRollingSize( rollSize, days );
    if ( setDebugLevel )
    {
        pValue = pNode->getChildValue("debugLevel");
        if ( pValue != NULL )
            HttpLog::setDebugLevel(atoi(pValue));
        if ( getLongValue( pNode, "enableStderrLog", 0, 1, 1 ) )
        {
            char * p = strrchr( buf, '/' );
            if ( p )
            {
                strcpy( p+1, "stderr.log" );
                HttpGlobals::getStdErrLogger()->setLogFileName( buf );
                HttpGlobals::getStdErrLogger()->getAppender()->
                    setRollingSize( rollSize );
            }
        }
        else
        {
            HttpGlobals::getStdErrLogger()->setLogFileName( NULL );
        }
    }
    return 0;
}

int HttpServerBuilder::initErrorLog( HttpLogSource& source, const XmlNode * pRoot,
                                int setDebugLevel )
{
    LogIdTracker tracker;
    appendLogId( ":errorLog" );
    int confType = 0;
    const XmlNode* p0 = pRoot->getChild("logging");
    if ( !p0 )
    {
        confType = 1;
        p0 = pRoot;
    }
    
    const XmlNode* pNode1 = NULL;
    if (confType == 0)
        pNode1 = p0->getChild("log");
    else
        pNode1 = p0->getChild("errorLog");
    
    if ( pNode1 == NULL )
    {
        if ( setDebugLevel )
        {
            LOG_ERR(( MISSING_TAG, getLogId(), "errorLog" ));
            return -1;
        }
    }
    else
    {
        int useServer = getLongValue( pNode1, "useServer", 0, 1, 1 );
        if ( setDebugLevel || useServer == 0 )
        {
            if ( initErrorLog2( source, pNode1, setDebugLevel ) != 0 )
            {
                LOG_ERR(( "[%s] failed to set up error log!", getLogId() ));
                return -1;
            }
        }
    }
    return 0;
}

int HttpServerBuilder::initAccessDeniedDir( const XmlNode* pNode )
{
    int add = 0;
    HttpGlobals::getDeniedDir()->clear();
    const XmlNodeList * pList = pNode->getChildren( "dir" );
    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for ( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            const XmlNode * pDir = *iter;
            if ( pDir->getValue() )
                if ( HttpGlobals::getDeniedDir()->addDir( pDir->getValue() ) == 0 )
                    add ++;
        }
    }
    return (add > 0);
}

int HttpServerBuilder::configCgid( CgidWorker * pWorker, const XmlNode * pNode1 )
{
    if ( !pWorker )
        return -1;
    int instances = getLongValue( pNode1,
                                  "maxCGIInstances", 1, 2000, 100 );
    pWorker->setMaxConns( instances );
    m_pRLimits = pWorker->getConfig().getRLimits();
    m_pRLimits->reset();
    initRLimits( pNode1, m_pRLimits );
    int priority = getLongValue( pNode1, "priority", -20, 20, HttpGlobals::s_priority + 1 );
    if ( priority > 20 )
        priority = 20;
    if ( priority < HttpGlobals::s_priority )
        priority = HttpGlobals::s_priority;
    char achSocket[128];
    const char * pValue = pNode1->getChildValue( "cgidSock" );
    if ( !pValue )
    {
        snprintf( achSocket, 128, "uds:/%s%sadmin/cgid/cgid.sock",
                  (m_sChroot.c_str())?m_sChroot.c_str():"",
                                                      m_sServerRoot.c_str() );
    }
    else
    {
        strcpy( achSocket, "uds:/" );
        if ( strncasecmp( "uds:/", pValue, 5 ) == 0 )
            pValue += 5;
        if (( m_sChroot.c_str() )&&
            ( strncmp( pValue, m_sChroot.c_str(), m_sChroot.len() ) == 0 ))
        {
            pValue += m_sChroot.len();
        }
        snprintf( achSocket, 128, "uds:/%s%s",
                  (m_sChroot.c_str())?m_sChroot.c_str():"",
                                                      pValue );
    }
    pWorker->getConfig().setSocket( achSocket );
    pWorker->getConfig().setPriority( priority );
    pWorker->getConfig().setMaxConns( instances );
    pWorker->getConfig().setRetryTimeout( 0 );
    pWorker->getConfig().setBuffering( HEC_RESP_NOBUFFER );
    pWorker->getConfig().setTimeout( HttpServerConfig::getInstance().getConnTimeout() );
    HttpGlobals::s_uidMin = getLongValue( pNode1, "minUID", 10, INT_MAX, 10 );
    HttpGlobals::s_gidMin = getLongValue( pNode1, "minGID", 5, INT_MAX, 5 );
    uid_t forcedGid = getLongValue( pNode1, "forceGID", 0, INT_MAX, 0 );
    if (( forcedGid > 0 )&&( forcedGid < HttpGlobals::s_gidMin ))
    {
        LOG_WARN(( "[%s] \"Force GID\" is smaller than \"Minimum GID\", turn it off.", getLogId() ));
    }
    else
        HttpGlobals::s_ForceGid = forcedGid;
    char achMIME[] = "application/x-httpd-cgi";
    HttpGlobals::getMime()->addMimeHandler( "", achMIME,
                                            HandlerFactory::getInstance(HandlerType::HT_CGI, NULL), NULL,  getLogId() );
    
    
    char achMIME_SSI[] = "application/x-httpd-shtml";
    HttpGlobals::getMime()->addMimeHandler( "", achMIME_SSI,
                                            HandlerFactory::getInstance(HandlerType::HT_SSI, NULL), NULL,  getLogId() );
    
    HttpGlobals::s_pidCgid = pWorker->start( m_sServerRoot.c_str(), m_sChroot.c_str(),
                                             HttpGlobals::s_uid, HttpGlobals::s_gid, HttpGlobals::s_priority );
    return HttpGlobals::s_pidCgid;
}



/*

int HttpServerBuilder::configCgid( CgidWorker * pWorker, const XmlNode * pNode1 )
{
    if ( !pWorker )
        return -1;
    int instances = getLongValue( pNode1,
                                  "maxCGIInstances", 1, 2000, 100 );
    pWorker->setMaxConns( instances );
    m_pRLimits = pWorker->getConfig().getRLimits();
    m_pRLimits->reset();
    initRLimits( pNode1, m_pRLimits );
    int priority = getLongValue( pNode1, "priority", -20, 20, HttpGlobals::s_priority + 1 );
    if ( priority > 20 )
        priority = 20;
    if ( priority < HttpGlobals::s_priority )
        priority = HttpGlobals::s_priority;
    char achSocket[128];
    const char * pValue = pNode1->getChildValue( "cgidSock" );
    if ( !pValue )
    {
        safe_snprintf( achSocket, 128, "uds:/%s%sadmin/conf/.cgid.sock",
                  (m_sChroot.c_str())?m_sChroot.c_str():"",
                  m_sServerRoot.c_str() );
    }
    else
    {
        strcpy( achSocket, "uds:/" );
        if ( strncasecmp( "uds:/", pValue, 5 ) == 0 )
            pValue += 5;
        if (( m_sChroot.c_str() )&&
                ( strncmp( pValue, m_sChroot.c_str(), m_sChroot.len() ) == 0 ))
        {
            pValue += m_sChroot.len();
        }
        safe_snprintf( achSocket, 128, "uds:/%s%s",
                  (m_sChroot.c_str())?m_sChroot.c_str():"",
                  pValue );
    }
    pWorker->getConfig().setSocket( achSocket );
    pWorker->getConfig().setPriority( priority );
    pWorker->getConfig().setMaxConns( instances );
    pWorker->getConfig().setRetryTimeout( 0 );
    pWorker->getConfig().setBuffering( HEC_RESP_NOBUFFER );
    pWorker->getConfig().setTimeout( 30 );
    HttpGlobals::s_uidMin = getLongValue( pNode1, "minUID", 10, INT_MAX, 10 );
    HttpGlobals::s_gidMin = getLongValue( pNode1, "minGID", 10, INT_MAX, 10 );
    uid_t forcedGid = getLongValue( pNode1, "forceGID", 0, INT_MAX, 0 );
    if (( forcedGid > 0 )&&( forcedGid < HttpGlobals::s_gidMin ))
    {
        LOG_WARN(( "[%s] \"Force GID\" is smaller than \"Minimum GID\", turn it off.", getLogId() ));
    }
    else
        HttpGlobals::s_ForceGid = forcedGid;
    return pWorker->start( m_sServerRoot.c_str(), m_sChroot.c_str(),
                           HttpGlobals::s_uid, HttpGlobals::s_gid, HttpGlobals::s_priority );

}
*/


int HttpServerBuilder::initSecurity(  const XmlNode* pRoot )
{
    LogIdTracker tracker( "config:server:security" );
    const XmlNode* pNode = pRoot->getChild("security");
    if ( pNode == NULL )
    {
        LOG_NOTICE(( "[%s] no <security> section at server level.", getLogId() ));
        return 1;
    }
    const XmlNode* pNode1 = pNode->getChild("accessDenyDir");
    if ( pNode1 != NULL )
    {
        initAccessDeniedDir( pNode1 );
    }
    HttpServerConfig& config = HttpServerConfig::getInstance();
    pNode1 = pNode->getChild( "fileAccessControl" );
    if ( !pNode1 )
        pNode1 = pNode;
    config.setFollowSymLink(
        getLongValue( pNode1, "followSymbolLink", 0, 2, 1) );
    config.checkDeniedSymLink( getLongValue( pNode1, "checkSymbolLink", 0, 1, 0));
    config.setRequiredBits(
        getLongValue( pNode1, "requiredPermissionMask", 0, 0177777, 004, 8) );
    config.setForbiddenBits(
        getLongValue( pNode1, "restrictedPermissionMask", 0, 0177777, 041111, 8));

    pNode1 = pNode->getChild( "perClientConnLimit" );
    if ( pNode1 )
    {
        setThrottleLimit( ThrottleControl::getDefault(), pNode1,
                          ThrottleControl::getDefault() );
        HttpIOLink::enableThrottle( (ThrottleControl::getDefault()->getOutputLimit() != INT_MAX) );
        HttpGlobals::getClientCache()->resetThrottleLimit();
        HttpGlobals::s_iConnsPerClientSoftLimit =
            getLongValue( pNode1, "softLimit", 1, INT_MAX, INT_MAX);
        HttpGlobals::s_iConnsPerClientHardLimit =
            getLongValue( pNode1, "hardLimit", 1, INT_MAX, INT_MAX);
        HttpGlobals::s_iOverLimitGracePeriod =
            getLongValue( pNode1, "gracePeriod", 1, 3600, 10 );
        HttpGlobals::s_iBanPeriod =
            getLongValue( pNode1, "banPeriod", 1, INT_MAX, 60 );
    }
    // CGI
    CgidWorker * pWorker = (CgidWorker *)ExtAppRegistry::addApp(
                               EA_CGID, LSCGID_NAME );
    pNode1 = pNode->getChild( "CGIRLimit" );
    if ( pNode1 )
    {
        configCgid( pWorker, pNode1);
    }
    appendLogId( ":accessControl" );
    if ( accessControlAvail( pNode ) )
    {
        initAccessControl( m_pServer->getAccessCtrl(), pNode );
        HttpGlobals::setAccessControl( m_pServer->getAccessCtrl() );
    }
    else
    {
        HttpGlobals::setAccessControl( NULL );
    }
    return 0;
}

int HttpServerBuilder::initMime(  const XmlNode* pRoot )
{
    const char * pValue = getTag( pRoot, "mime");
    if ( pValue != NULL )
    {
        char achBuf[MAX_PATH_LEN];
        if ( getValidFile(achBuf, pValue, "MIME config") != 0 )
            return -1;
        if ( HttpGlobals::getMime()->loadMime( achBuf ) == 0 )
            return 0;
        if ( HttpGlobals::getMime()->getDefault() == 0 )
            HttpGlobals::getMime()->initDefault();
    }
    return -1;
}


int HttpServerBuilder::denyAccessFiles( HttpVHost * pVHost, const char * pFile, int regex )
{
    HttpContext * pContext = new HttpContext();
    if ( pContext )
    {
        pContext->setFilesMatch( pFile, regex );
        pContext->addAccessRule( "*", 0 );
        if ( pVHost )
            pVHost->getRootContext().addFilesMatchContext( pContext );
        else
            m_pServer->getServerContext().addFilesMatchContext( pContext );
        return 0;
    }
    return -1;
}


int HttpServerBuilder::initServerBasic2(  const XmlNode* pRoot )
{
    while ( 1 )
    {
        const XmlNode* pNode;


        LogIdTracker tracker( "config:server:basic" );

        long inMemBufSize = getLongValue( pRoot, "inMemBufSize", 0,
                                          LONG_MAX, 20 * 1024 * 1024 );
        VMemBuf::setMaxAnonMapSize( inMemBufSize );
        const char * pValue = m_pRoot->getChildValue( "swappingDir" );
        if ( pValue )
        {
            m_pServer->setSwapDir( pValue );
        }


        setContextAutoIndex( &m_pServer->getServerContext(), pRoot );
        setContextDirIndex( &m_pServer->getServerContext(), pRoot );
        const char * pURI = getAutoIndexURI( pRoot );
        if ( pURI )
            m_sAutoIndexURI.setStr( pURI );

        int sv = getLongValue( pRoot, "showVersionNumber", 0, 1, 0 );
        HttpServerVersion::hideDetail( !sv );
        if ( !sv )
        {
            LOG_INFO(( "[%s] For better obscurity, server version number is hidden"
                       " in the response header.", getLogId() ));
        }

        denyAccessFiles( NULL, ".ht*", 0 );


        if ( initMime(  pRoot ) != 0 )
        {
            LOG_ERR(( "[%s] failed to load mime configure", getLogId() ));
            break;
        }
        pNode = pRoot->getChild( "expires" );
        if ( pNode )
        {
            initExpires( pNode, &m_pServer->getServerContext().getExpires(),
                         NULL, &m_pServer->getServerContext() );
            HttpGlobals::getMime()->setExpiresByType(
                pNode->getChildValue( "expiresByType" ), NULL, getLogId() );
        }
        initSecurity(  pRoot );
        return 0;
    }
    return -1;

}

int HttpServerBuilder::initMultiplexer()
{
    const char * pType = NULL;
    const XmlNode * pNode = m_pRoot->getChild("tuning");
    if ( pNode )
        pType = pNode->getChildValue( "eventDispatcher" );
    if ( m_pServer->initMultiplexer( pType ) == -1 )
    {
        LOG_ERR(( "[%s] Failed to initialize I/O event dispatcher type: %s, error: %s",
                  getLogId(), pType, strerror( errno ) ));
        if ( pType && (strcasecmp( pType, "poll" ) != 0 ) )
        {
            LOG_NOTICE(( "[%s] Fall back to I/O event dispatcher type: poll",
                         getLogId() ));
            return m_pServer->initMultiplexer( "poll" );
        }
        return -1;
    }
    return 0;
}




struct passwd * HttpServerBuilder::getUGid( const char * pUser, const char * pGroup,
                                            gid_t &gid )
{
    if ( !pUser || !pGroup )
        return NULL;
    struct passwd *pw;
    pw = getpwnam( pUser );
    if ( !pw )
    {
        if ( isdigit( *pUser ) )
        {
            uid_t uid = atoi( pUser );
            pw = getpwuid( uid );
        }
        if ( !pw )
        {
            LOG_ERR(( "[%s] Invalid user name:%s!", getLogId(), pUser ));
            return NULL;
        }
    }
    struct group * gr;
    gr = getgrnam( pGroup );
    if ( !gr )
    {
        if ( isdigit( *pGroup ) )
        {
            gid = atoi( pGroup );
        }
        else
        {
            LOG_ERR(( "[%s] Invalid group name:%s!", getLogId(), pGroup ));
            return NULL;
        }
    }
    else
        gid = gr->gr_gid;
    return pw;
}


int HttpServerBuilder::configServerBasics( int reconfig )
{
    LogIdTracker tracker( "config:server:basic" );
    while ( 1 )
    {
        if ( initErrorLog( *m_pServer, m_pRoot, 1 ) )
            break;
        const char * pValue = getTag( m_pRoot, "serverName");
        if ( pValue != NULL )
            setServerName( pValue );
        else
            setServerName( "anonymous" );
        if ( !reconfig )
        {
            pValue = m_pRoot->getChildValue("user");
            if ( pValue != NULL )
            {
                struct passwd *pw;
                setUser( pValue );
                pw = getpwnam( pValue );
                if ( !pw )
                {
                    LOG_ERR(( "[%s] Invalid user name:%s!", getLogId(), pValue ));
                    break;
                }
                HttpGlobals::s_uid = pw->pw_uid;
                m_pri_gid = pw->pw_gid;
            }
            pValue = m_pRoot->getChildValue("group");
            if ( pValue != NULL )
            {
                struct group * gr;
                setGroup( pValue );
                gr = getgrnam( pValue );
                if ( !gr )
                {
                    LOG_ERR(( "[%s] Invalid group name:%s!", getLogId(), pValue ));
                    break;
                }
                HttpGlobals::s_gid = gr->gr_gid;
            }
            if ( getuid() == 0 )
            {
                chown(  HttpLog::getErrorLogFileName(),
                        HttpGlobals::s_uid, HttpGlobals::s_gid );
                chown(  HttpLog::getAccessLogFileName(),
                        HttpGlobals::s_uid, HttpGlobals::s_gid );
            }
        }
        const char * pAdminEmails = m_pRoot->getChildValue( "adminEmails" );
        if ( !pAdminEmails )
        {
            pAdminEmails = "";
        }
        setAdminEmails( pAdminEmails );

        HttpGlobals::s_priority = getLongValue( m_pRoot, "priority", -20, 20, 0 );

        HttpGlobals::s_children = getLongValue( m_pRoot, "httpdWorkers", 1, 16, 1 );
        
        const char * pGDBPath = m_pRoot->getChildValue( "gdbPath" );
        if ( pGDBPath )
        {
            m_gdbPath = pGDBPath;
        }
        HttpGlobals::s_503AutoFix = getLongValue( m_pRoot, "AutoFix503", 0, 1, 1 );
        const char * pAutoRestart = m_pRoot->getChildValue( "autoRestart" );
        if ( pAutoRestart != NULL )
        {
            int t = atoi(pAutoRestart);
            if ( t )
                t = 1;
            //this value can only be set once when server start.
            if ( getCrashGuard() == 2 )
                setCrashGuard( t );
        }
        return 0;
    }

    return -1;
}

#define DEFAULT_ADMIN_CONFIG_FILE   "$VH_ROOT/conf/admin_config.xml"
#define ADMIN_CONFIG_NODE           "AdminConfigNode"
#define DEFAULT_ADMIN_FCGI_NAME     "AdminPHP"
#define DEFAULT_ADMIN_PHP_FCGI      "$VH_ROOT/fcgi-bin/admin_php"
#define DEFAULT_ADMIN_PHP_FCGI_URI  "UDS://tmp/lshttpd/admin_php.sock"

int HttpServerBuilder::loadAdminConfig( XmlNode* pRoot )
{
    LogIdTracker tracker( "config:admin" );
    const char * pAdminRoot = getTag( pRoot, "adminRoot" );
    if ( pAdminRoot == NULL )
    {
        return -1;
    }
    if ( getValidChrootPath( m_achVhRoot, pAdminRoot, "admin vhost root" ) != 0)
    {
        LOG_ERR(( "[%s] The value of root directory to "
                  "admin server is invalid - %s!",
                  getLogId(), pAdminRoot ));
        return -1;
    }
    char achConfFile[MAX_PATH_LEN];
    if ( getValidFile( achConfFile, DEFAULT_ADMIN_CONFIG_FILE,
                       "configuration file for admin vhost" ) != 0 )
    {
        LOG_ERR(( "[%s] missing configuration file for admin server: %s!",
                  getLogId(), achConfFile ));
        return -1;
    }
    XmlNode* pAdminConfNode = parseFile(achConfFile, "adminConfig");
    if ( pAdminConfNode == NULL )
    {
        LOG_ERR(( "[%s] cannot load configure file for admin server - %s !",
                  getLogId(), achConfFile ));
        return -1;
    }
    pRoot->addChild( ADMIN_CONFIG_NODE, pAdminConfNode );

    m_enableCoreDump = getLongValue( pAdminConfNode, "enableCoreDump", 0, 1, 0 );
    return 0;
}

int HttpServerBuilder::loadPlainConfigFile()
{
    plainconf::initKeywords();
    plainconf::setRootPath(getServerRoot());
    if ( !m_pRoot )
        m_pRoot = plainconf::parseFile(m_sPlainconfPath.c_str());

//#define OUTPUT_TEST_CONFIG_FILE
#ifdef OUTPUT_TEST_CONFIG_FILE
    char sPlainFile[512] = {0};
    strcpy(sPlainFile, pConfigFilePath);
    strcat(sPlainFile, ".txt");
    plainconf::testOutputConfigFile(m_pRoot, sPlainFile);
#endif
    return ( m_pRoot == NULL )?-1:0;  
}

int HttpServerBuilder::loadConfigFile( const char * pConfigFilePath)
{
    if (pConfigFilePath == NULL)
        pConfigFilePath = m_sConfigFilePath.c_str();
        
    if ( !m_pRoot )
        m_pRoot = parseFile(pConfigFilePath, "httpServerConfig");
    return ( m_pRoot == NULL )?-1:0;
}

void HttpServerBuilder::releaseConfigXmlTree()
{
    if ( m_pRoot )
    {
        delete m_pRoot;
        m_pRoot = NULL;
    }
}

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#include <sys/prctl.h>
#endif

#if defined(__FreeBSD__ ) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#include <sys/sysctl.h>
#endif
int HttpServerBuilder::configChroot()
{
    if (( getuid() == 0 )&&( getLongValue( m_pRoot, "enableChroot", 0, 1, 0 ) ))
    {
        const char *pValue = m_pRoot->getChildValue( "chrootPath" );
        if ( pValue )
        {
            char achTemp[512];
            char * pChroot = achTemp;
            strcpy( pChroot, pValue );
            int len = strlen( pChroot );
            len = GPath::checkSymLinks( pChroot, pChroot + len,
                                        pChroot + sizeof( achTemp ), pChroot, 1 );
            if ( len == -1 )
            {
                LOG_ERR(( "[%s] Invalid chroot path." ));
                return -1;
            }
            if ( *(pChroot + len - 1) != '/' )
            {
                *(pChroot + len++) = '/';
                *(pChroot + len ) = 0;
            }
            if (( *pChroot != '/' )
                    ||(access( pChroot, F_OK ) != 0 ))
            {
                LOG_ERR(( "[%s] chroot must be valid absolute path: %s",
                          getLogId(), pChroot ));
                strcpy( pChroot, "/");
                len = 1;
            }
            if ( strncmp( pChroot, m_sServerRoot.c_str(), len ) != 0 )
            {
                LOG_ERR(( "[%s] Server root: %s falls out side of chroot: %s, "
                          "disable chroot!", getLogId(), m_sServerRoot.c_str(),
                          pChroot ));
                strcpy( pChroot, "/");
            }
            if ( strcmp( pChroot, "/" ) != 0 )
            {
                *(pChroot + --len) = 0;
                m_sChroot = pChroot;
                HttpGlobals::s_psChroot = &m_sChroot;
                char achTemp[512];
                strcpy( achTemp, m_sServerRoot.c_str() + m_sChroot.len() );
                m_sServerRoot = achTemp;
                HttpGlobals::s_pServerRoot = m_sServerRoot.c_str();
            }
        }
    }
    else
        LOG_NOTICE(( "[%s] chroot is disabled.", getLogId() ));
    return 0;
}

int HttpServerBuilder::initGroups()
{
    char achBuf[256];
    if ( Daemonize::initGroups(
                getUser(), HttpGlobals::s_gid,
                m_pri_gid, achBuf, 256 ) )
    {
        LOG_ERR(( "[%s] %s",
                  getLogId(), achBuf  ));
        return -1;
    }
    return 0;
}

int HttpServerBuilder::changeUserChroot()
{
    if ( getuid() != 0 )
        return 0;
#if defined(__FreeBSD__ ) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    enableCoreDump();
#endif
    LOG_D(( "try to give up super user privilege!" ));
    char achBuf[256];
    if ( Daemonize::changeUserChroot( getUser(), HttpGlobals::s_uid,
                                      m_sChroot.c_str(), achBuf, 256 ) )
    {
        LOG_ERR(( "[%s] %s",
                  getLogId(), achBuf  ));
        return -1;
    }
    else
    {
        LOG_NOTICE(( "[child: %d] Successfully change current user to %s",
                     getpid(), getUser() ));
#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
        enableCoreDump();
#endif
        if ( m_sChroot.c_str() )
        {
            LOG_NOTICE(( "[child: %d] Successfully change root directory to %s",
                         getpid(), m_sChroot.c_str() ));
        }
    }
    return 0;
}

int HttpServerBuilder::startListeners()
{
    {
        LogIdTracker tracker( "config:admin:listener" );
        if ( initListeners(
                    m_pRoot->getChild( ADMIN_CONFIG_NODE ), 1 ) <= 0)
        {
            LOG_ERR(( "[%s] No listener is available for admin virtual host!",
                      getLogId() ));
            return -1;
        }
    }
    {
        LogIdTracker tracker( "config:server:listener" );
        if ( initListeners( m_pRoot, 0) <= 0 )
        {
            LOG_WARN(( "[%s] No listener is available for normal virtual host!",
                       getLogId() ));
        }
    }
    return 0;
}

void HttpServerBuilder::enableCoreDump()
{
#if defined(__FreeBSD__ ) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    int  mib[2];
    size_t len;

    len = 2;
    if ( sysctlnametomib("kern.sugid_coredump", mib, &len) == 0 )
    {
        len = sizeof(m_enableCoreDump);
        if (sysctl(mib, 2, NULL, 0, &m_enableCoreDump, len) == -1)
            LOG_WARN(( "sysctl: Failed to set 'kern.sugid_coredump', "
                       "core dump may not be available!"));
        else
        {
            int dumpable;
            if ( sysctl( mib, 2, &dumpable, &len, NULL, 0 ) != -1 )
            {
                LOG_NOTICE(( "Core dump is %s.",
                             (dumpable) ? "enabled" : "disabled" ));
            }
        }
    }


#endif

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    if (prctl(PR_SET_DUMPABLE, m_enableCoreDump) == -1)
        LOG_WARN(( "prctl: Failed to set dumpable, "
                   "core dump may not be available!"));
    {
        int dumpable = prctl(PR_GET_DUMPABLE);
        if ( dumpable == -1)
            LOG_WARN(( "prctl: get dumpable failed "));
        else
            LOG_NOTICE(( "[Child: %d] Core dump is %s.", getpid(),
                         (dumpable) ? "enabled" : "disabled" ));
    }
#endif
}

int HttpServerBuilder::configServer( int reconfig )
{
    int ret;
    if ( !reconfig )
    {
        initMultiplexer();
        m_pServer->recoverListeners();
        HttpGlobals::getStdErrLogger()->initLogger( HttpGlobals::getMultiplexer() );
        if ( configChroot() )
            return -1;
        int pri = getpriority( PRIO_PROCESS, 0 );
        setpriority( PRIO_PROCESS, 0, HttpGlobals::s_priority );
        int new_pri = getpriority( PRIO_PROCESS, 0 );
        LOG_INFO(( "old priority: %d, new priority: %d", pri, new_pri ));
    }
    ret = initServerBasic2(  m_pRoot );
    if ( ret )
        return ret;
    ret = loadAdminConfig( m_pRoot );
    if ( ret )
        return ret;
    initTuning( m_pRoot );
    if ( startListeners() )
        return -1;

    int maxconns = HttpGlobals::getConnLimitCtrl()->getMaxConns();
    unsigned long long maxfds = SystemInfo::maxOpenFile( maxconns * 3);
    LOG_NOTICE(( "The maximum number of file descriptor limit is set to %llu.",
                 maxfds ));
    if ( (unsigned long long)maxconns + 100 > maxfds )
    {
        LOG_WARN(( "[%s] Current per process file descriptor limit: %llu is too low comparing to "
                   "you currnet 'Max connections' setting: %d, consider to increase "
                   "your system wide file descriptor limit by following the instruction "
                   "in our HOWTO #1.", getLogId(), maxfds, maxconns ));
    }

    if ( initGroups() )
        return -1;

    ret = initAdmin(  m_pRoot->getChild( ADMIN_CONFIG_NODE ) );
    if ( ret )
    {
        LOG_ERR(( "[%s] Failed to setup the WEB administration interface!",
                  getLogId() ));
        return ret;
    }
    {
        LogIdTracker tracker( "config:server:epsr" );
        initExtProcessors( m_pRoot, NULL );
    }
    {
        LogIdTracker tracker( "config:server:rails" );
        loadRailsDefault( m_pRoot->getChild( "railsDefaults" ) );
    }
    
    int confType = 0;
    const XmlNode* p0 = m_pRoot->getChild("scriptHandlerList");
    if ( !p0 )
    {
        confType = 1;
        p0 = m_pRoot->getChild("scriptHandler");
    }
    
    if ( p0 != NULL )
    {
        if (confType == 0)
        {
            const XmlNodeList *pList = p0->getChildren( "scriptHandler" );
            if ( pList && pList->size() > 0 )
                configScriptHandler1( NULL, pList, NULL );
        }
        else 
        {
            const XmlNodeList *pList = p0->getChildren( "add" );
            if ( pList && pList->size() > 0 )
                configScriptHandler2( NULL, pList, NULL );
        }
    }
    initAccessLog( *m_pServer, m_pRoot, 1 );
    initVHosts( m_pRoot);
    initListenerVHostMap(  m_pRoot, NULL );
    initVHTemplates( m_pRoot );

    return ret;
}

void HttpServerBuilder::reconfigVHost( char * pVHostName )
{
    HttpVHost * pNew;
    if ( !reconfigVHost( pVHostName, pNew ) )
        if ( m_pServer->updateVHost( pVHostName, pNew ) )
            if ( pNew )
                delete pNew;
    reconfigVHostMappings( pVHostName );
}

int HttpServerBuilder::reconfigVHost( const char * pVHostName, HttpVHost * &pVHost )
{
    int ret;
    pVHost = NULL;
    ret = loadConfigFile( m_sConfigFilePath.c_str() );
    if ( ret )
        return ret;
    XmlNode* pNode = m_pRoot->getChild("virtualHostList");
    if ( !pNode )
        pNode = m_pRoot;
    
    const XmlNodeList *pList = pNode->getChildren( "virtualHost" );
    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for ( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            XmlNode * pVhostNode = *iter;
            m_achVhRoot[0] = 0;
            const char * pName = pVhostNode->getChildValue( "name" );
            if (( pName)&&( strcmp( pName, pVHostName ) == 0 ))
            {
                pVHost = configVHost( pVhostNode );
                break;
            }
        }
    }
    return 0;

}

int HttpServerBuilder::reconfigVHostMappings( const char * pVHostName )
{
    return initListenerVHostMap(  m_pRoot, pVHostName );
}

static void setupSUExec()
{
    HttpGlobals::s_pSUExec = new SUExec();
}

int HttpServerBuilder::initServer( int reconfig )
{
    int ret;
    if (strlen(m_sPlainconfPath.c_str()) > 0)
    {
        LOG_NOTICE(( "Loading configuration from plain file %s ...", m_sPlainconfPath.c_str() ));
        ret = loadPlainConfigFile();
    }
    else
    {
        LOG_NOTICE(( "Loading configuration from XML file %s ...", m_sConfigFilePath.c_str() ));
        ret = loadConfigFile();
    }
    if ( ret )
        return ret;
    if ( !reconfig )
    {
        setupSUExec();
    }
    ret = configServerBasics( reconfig );
    if ( ret )
        return ret;
    m_pServer->beginConfig();
    ret = configServer( reconfig );
    m_pServer->endConfig(ret);

    releaseConfigXmlTree();
    return ret;
}

int HttpServerBuilder::configRailsRunner( char * pRunnerCmd, int cmdLen, const char * pRubyBin )
{
    const char * rubyBin[2]={ "/usr/local/bin/ruby", "/usr/bin/ruby" };
    if (( pRubyBin )&&( access( pRubyBin, X_OK ) != 0 ))
    {
        LOG_ERR(( "[%s] Ruby path is not vaild: %s", getLogId(), pRubyBin ));
        pRubyBin = NULL;
    }
    if ( !pRubyBin )
    {
        for( int i = 0; i < 2; ++i )
        {
            if ( access( rubyBin[         i], X_OK ) == 0 )
            {
                pRubyBin = rubyBin[i];
                break;
            }
        }
    }
    if ( !pRubyBin )
    {
        LOG_NOTICE(( "[%s] Cannot find ruby interpreter, Rails easy configuration is turned off",
                    getLogId() ));
        return -1;
    }
    snprintf( pRunnerCmd, cmdLen, "%s %sfcgi-bin/RackRunner.rb", pRubyBin, m_sServerRoot.c_str() );
    return 0;
}

int HttpServerBuilder::loadRailsDefault( const XmlNode * pNode )
{
    const char * pRubyBin = NULL;
    m_iRailsEnv = 1;

    m_railsDefault = new LocalWorkerConfig();

    m_railsDefault->setPersistConn( 1 );
    m_railsDefault->setKeepAliveTimeout( 30 );
    m_railsDefault->setMaxConns( 1 );
    m_railsDefault->setTimeout( 120 );
    m_railsDefault->setRetryTimeout( 0 );
    m_railsDefault->setBuffering( 0 );
    m_railsDefault->setPriority( HttpGlobals::s_priority+1 );
    m_railsDefault->setBackLog( 10 );
    m_railsDefault->setMaxIdleTime( 300 );
    m_railsDefault->setRLimits( m_pRLimits );
    m_railsDefault->getRLimits()->setCPULimit( RLIM_INFINITY, RLIM_INFINITY );

    if ( !pNode )
        return -1;

    if ( getLongValue( pNode, "enableRailsHosting", 0, 1, 0 ) == 1  )
    {
        HttpGlobals::s_railsAppLimit = getLongValue( pNode, "railsAppLimit", 0, 20, 1 );
        HttpGlobals::s_rubyProcLimit = getLongValue( pNode, "rubyProcLimit", 0, 100, 10 );
    }
    else
        HttpGlobals::s_railsAppLimit = -1;

    m_iRailsEnv = getLongValue( pNode, "railsEnv", 0, 2, 1 );
    pRubyBin = pNode->getChildValue( "rubyBin" );
    char achBuf[4096];
    if ( configRailsRunner( achBuf, 4096, pRubyBin ) == -1 )
        return -1;
    m_sRailsRunner.setStr( achBuf );

    initExtAppConfig( pNode, m_railsDefault, 1 );
    return 0;
}

HttpContext * HttpServerBuilder::configRailsContext( const XmlNode * pNode,
            HttpVHost * pVHost, const char * contextUri, const char* appPath )
{

    int maxConns = getLongValue( pNode, "maxConns", 1, 2000,
                                m_railsDefault->getMaxConns() );
    const char * railsEnv[3] = { "development", "production", "staging" };
    int production = getLongValue( pNode, "railsEnv", 0, 2, m_iRailsEnv );
    const char * pRailsEnv = railsEnv[production];

    const char * pRubyPath = pNode->getChildValue( "rubyBin" );

    long maxIdle = getLongValue( pNode, "extMaxIdleTime", -1, INT_MAX,
                        m_railsDefault->getMaxIdleTime() );
    if ( maxIdle == -1 )
        maxIdle = INT_MAX;
    return configRailsContext( pVHost, contextUri, appPath,
            maxConns, pRailsEnv, maxIdle, NULL, pRubyPath );
}



LocalWorker * HttpServerBuilder::configRailsApp(
            HttpVHost * pVHost, const char * pAppName, const char* appPath,
            int maxConns, const char * pRailsEnv, int maxIdle, const Env * pEnv,
            int runOnStart, const char * pRubyPath )
{
    int  ret ;
    char achFileName[MAX_PATH_LEN];
    const char * pRailsRunner = m_sRailsRunner.c_str();

    if ( !m_railsDefault )
        return NULL;

    int pathLen = snprintf( achFileName, MAX_PATH_LEN, "%s", appPath );
    if ( pathLen > MAX_PATH_LEN - 20 )
    {
        LOG_ERR(( "[%s] path to Rack application is too long!", getLogId() ));
        return NULL;
    }

    if ( access( achFileName, F_OK ) != 0 )
    {
        LOG_ERR(( INVAL_PATH, getLogId(), "Rack application",  achFileName ));
        return NULL;
    }
/*
    strcpy( &achFileName[pathLen], "public/" );
    if ( access( achFileName, F_OK ) != 0 )
    {
        LOG_ERR(( INVAL_PATH, getLogId(), "public folder of Rails application",  achFileName ));
        return NULL;
    }
*/
    if ( !pRailsRunner )
    {
        LOG_ERR(( "[%s] 'Ruby path' is not set properly, Rack context is disabled!",
                getLogId() ));
        return NULL;
    }

    LocalWorker * pWorker;
    char achAppName[1024];
    char achName[MAX_PATH_LEN];
    snprintf( achAppName, 1024, "Rack:%s:%s", pVHost->getName(), pAppName );

    pWorker = (LocalWorker *)ExtAppRegistry::addApp( EA_LSAPI, achAppName );

    strcpy( &achFileName[pathLen], "tmp/sockets" );
    //if ( access( achFileName, W_OK ) == -1 )
    {
        snprintf( achName, MAX_PATH_LEN, "uds://tmp/lshttpd/%s:%s.sock",
                 pVHost->getName(), pAppName );
        for( char * p = &achName[18]; *p; ++p )
        {
            if (*p == '/' )
                *p = '_';
        }
    }
    //else
    //{
    //    snprintf( achName, MAX_PATH_LEN, "uds://%s/RailsRunner.sock", 
    //            &achFileName[m_sChroot.len()] );
    //}
    pWorker->setURL( achName );


    LocalWorkerConfig& config = pWorker->getConfig();
    config.setVHost( pVHost );
    config.setAppPath( pRailsRunner );
    config.setBackLog( m_railsDefault->getBackLog() );
    config.setSelfManaged( 1 );
    config.setStartByServer( 2 );
    config.setMaxConns( maxConns );
    config.setKeepAliveTimeout( m_railsDefault->getKeepAliveTimeout() );
    config.setPersistConn( 1 );
    config.setTimeout( m_railsDefault->getTimeout() );
    config.setRetryTimeout( m_railsDefault->getRetryTimeout() );
    config.setBuffering( m_railsDefault->getBuffering() );
    config.setPriority( m_railsDefault->getPriority() );
    config.setMaxIdleTime( maxIdle );
    config.setRLimits( m_railsDefault->getRLimits() );

    config.setInstances( 1 );
    config.clearEnv();

    if ( pEnv )
        config.getEnv()->add( pEnv );
    achFileName[pathLen] = 0;
    snprintf( achName, MAX_PATH_LEN, "RAILS_ROOT=%s", &achFileName[m_sChroot.len()] );
    config.addEnv( achName );
    if ( pRailsEnv )
    {
        snprintf( achName, MAX_PATH_LEN, "RAILS_ENV=%s", pRailsEnv );
        config.addEnv( achName );
    }

    if ( maxConns > 1 )
    {
        snprintf( achName, MAX_PATH_LEN, "LSAPI_CHILDREN=%d", maxConns );
        config.addEnv( achName );
    }
    else
        config.setSelfManaged( 0 );
    if ( maxIdle != INT_MAX )
    {
        snprintf( achName, MAX_PATH_LEN, "LSAPI_MAX_IDLE=%d", maxIdle );
        config.addEnv( achName );
        snprintf( achName, MAX_PATH_LEN, "LSAPI_PGRP_MAX_IDLE=%d", maxIdle );
        config.addEnv( achName );
    }
    config.getEnv()->add( m_railsDefault->getEnv() );
    config.addEnv( NULL );

    config.setRunOnStartUp( runOnStart );
    return pWorker;
}

HttpContext * addRailsContext( HttpVHost * pVHost, const char * pURI, const char * pLocation,
                    LocalWorker * pApp )
{
    return HttpGlobals::s_pBuilder->addRailsContext( pVHost, pURI, pLocation, pApp );

}
   
HttpContext * HttpServerBuilder::addRailsContext( HttpVHost * pVHost, const char * pURI,
                const char * pLocation, LocalWorker * pWorker )
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
        LOG_ERR(( "[%s] context URI is too long!", getLogId() ));
        return NULL;
    }
    char achBuf[MAX_PATH_LEN];
    snprintf( achBuf, MAX_PATH_LEN, "%spublic/", pLocation );
    HttpContext * pContext = configContext( pVHost, achURI, HandlerType::HT_NULL,
                            achBuf, NULL, 1 );
    if ( !pContext )
    {
        return NULL;
    }
    strcpy( &achURI[uriLen], "dispatch.rb" );
    HttpContext * pDispatch = addContext( pVHost, 0, achURI, HandlerType::HT_NULL,
                            NULL, NULL, 1 );
    if ( pDispatch )
        pDispatch->setHandler( pWorker );
/*
    strcpy( &achURI[uriLen], "dispatch.cgi" );
    pDispatch = addContext( pVHost, 0, achURI, HandlerType::HT_NULL,
                            NULL, NULL, 1 );
    if ( pDispatch )
        pDispatch->setHandler( pWorker );
    strcpy( &achURI[uriLen], "dispatch.fcgi" );
    pDispatch = addContext( pVHost, 0, achURI, HandlerType::HT_NULL,
                            NULL, NULL, 1 );
    if ( pDispatch )
        pDispatch->setHandler( pWorker );
*/
   
    strcpy( &achURI[uriLen], "dispatch.lsapi" );
    pDispatch = addContext( pVHost, 0, achURI, HandlerType::HT_NULL,
                            NULL, NULL, 1 );
    if ( pDispatch )
        pDispatch->setHandler( pWorker );
    pContext->setCustomErrUrls( "404", achURI );
    pContext->setRailsContext();
    return pContext;

}


HttpContext * HttpServerBuilder::configRailsContext(
            HttpVHost * pVHost, const char * contextUri, const char* appPath,
            int maxConns, const char * pRailsEnv, int maxIdle, const Env * pEnv, const char * pRubyPath )
{
    char achFileName[MAX_PATH_LEN];
    char achURI[MAX_URI_LEN];

    if ( !m_railsDefault )
        return NULL;
    int ret = getAbsolutePath( achFileName, appPath );
    if ( ret == -1 )
    {
        LOG_ERR(( "[%s] path to Rails application is invalid!", getLogId() ));
        return NULL;
    }


    LocalWorker * pWorker = configRailsApp( pVHost, contextUri, achFileName,
            maxConns, pRailsEnv, maxIdle, pEnv, m_railsDefault->getRunOnStartUp(), pRubyPath ) ;
    if ( !pWorker )
        return NULL;
    return addRailsContext( pVHost, contextUri, achFileName,
            pWorker );
}

static void setPHPHandler( HttpContext * pCtx, HttpHandler * pHandler,
                           char * pSuffix )
{
    char achMime[100];
    safe_snprintf( achMime, 100, "application/x-httpd-%s", pSuffix );
    pCtx->initMIME();
    pCtx->getMIME()->addMimeHandler( pSuffix, achMime,
                                     pHandler, NULL,  getLogId() );
}


LocalWorker * getPHPHandler( const char * pSuffix )
{

    char achMime[100];
    safe_snprintf( achMime, 100, "application/x-httpd-%s", pSuffix );
    const MIMESetting * pSetting = HttpGlobals::getMime()->getMIMESetting( achMime );
    if ( pSetting )
        return (LocalWorker *)ExtAppRegistry::getApp( EA_LSAPI,
                pSetting->getHandler()->getName() );
    return NULL;
}


static int addPHPSuExec( HttpVHost * pVHost, int maxConn,
                         char * pSuffix, LocalWorker * pProto )
{
    LocalWorker * pWorker;
    char achAppName[1024];
    char achName[MAX_PATH_LEN];
    safe_snprintf( achAppName, 1024, "%sSu%s:", pVHost->getName(), pSuffix );

    pWorker = (LocalWorker *)ExtAppRegistry::getApp( EA_LSAPI, achAppName );
    if ( !pWorker )
    {
        pWorker = (LocalWorker *)ExtAppRegistry::addApp( EA_LSAPI, achAppName );
        if ( !pWorker )
            return 0;
        safe_snprintf( achName, MAX_PATH_LEN, "uds://tmp/lshttpd/%s_Su%s.sock",
                  pVHost->getName(), pSuffix );
        pWorker->setURL( achName );
        LocalWorkerConfig& config = pWorker->getConfig();

        config.setAppPath( pProto->getConfig().getCommand() );
        config.setBackLog( pProto->getConfig().getBackLog() );
        config.setSelfManaged( 0 );
        config.setMaxConns( maxConn );
        config.setKeepAliveTimeout( pProto->getConfig().getKeepAliveTimeout() );
        config.setPersistConn( 1 );
        config.setTimeout( pProto->getConfig().getTimeout() );
        config.setRetryTimeout( pProto->getConfig().getRetryTimeout() );
        config.setBuffering( pProto->getConfig().getBuffering() );
        config.setPriority( pProto->getConfig().getPriority() );
        config.setMaxIdleTime( 300 );
        config.setRLimits(  pProto->getConfig().getRLimits() );
        config.clearEnv();
        config.addEnv( NULL );
        config.setVHost( pVHost );
    }
    setPHPHandler( &pVHost->getRootContext(), pWorker, pSuffix );
    return 0;
}


HttpContext * HttpServerBuilder::importWebApp( HttpVHost * pVHost,
        const char * contextUri, const char* appPath,
        const char * pWorkerName, int allowBrowse )
{
    char achFileName[MAX_PATH_LEN];

    int ret = getAbsolutePath( achFileName, appPath );
    if ( ret == -1 )
    {
        LOG_ERR(( "[%s] path to Web-App is invalid!", getLogId() ));
        return NULL;
    }

    int pathLen = strlen( achFileName );
    if ( pathLen > MAX_PATH_LEN - 20 )
    {
        LOG_ERR(( "[%s] path to Web-App is too long!", getLogId() ));
        return NULL;
    }

    if ( access( achFileName, F_OK ) != 0 )
    {
        LOG_ERR(( INVAL_PATH, getLogId(), "Web-App",  achFileName ));
        return NULL;
    }
    strcpy( &achFileName[pathLen], "WEB-INF/web.xml" );
    if ( access( achFileName, F_OK ) != 0 )
    {
        LOG_ERR(( INVAL_PATH, getLogId(), "Web-App configuration",  achFileName ));
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
        LOG_ERR(( "[%s] context URI is too long!", getLogId() ));
        return NULL;
    }

    XmlNode* pRoot = parseFile(achFileName, "web-app");
    if ( !pRoot )
    {
        LOG_ERR(( "[%s] invalid Web-App configuration file: %s."
                  , getLogId(), achFileName ));
        return NULL;
    }

    achFileName[pathLen] = 0;
    HttpContext * pContext = configContext( pVHost, achURI, HandlerType::HT_NULL,
                                            achFileName, NULL, allowBrowse );
    if ( !pContext )
    {
        delete pRoot;
        return NULL;
    }
    strcpy( &achURI[uriLen], "WEB-INF/" );
    strcpy( &achFileName[pathLen], "WEB-INF/" );
    HttpContext * pInnerContext;
    pInnerContext = configContext( pVHost, achURI, HandlerType::HT_NULL,
                                   achFileName, NULL, false );
    if ( !pInnerContext )
    {
        delete pRoot;
        return NULL;
    }
    const XmlNodeList * pList = pRoot->getChildren( "servlet-mapping" );
    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for ( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            const XmlNode * pMappingNode = *iter;
            const char *  pUrlPattern =
                pMappingNode->getChildValue( "url-pattern" );
            if ( pUrlPattern )
            {
                if ( *pUrlPattern == '/' )
                {
                    ++pUrlPattern;
                }
                strcpy( &achURI[uriLen], pUrlPattern );
                int patternLen = strlen( pUrlPattern );
                //remove the trailing '*'
                if ( *(pUrlPattern + patternLen - 1) == '*' )
                    achURI[uriLen + patternLen - 1] = 0;
                pInnerContext = configContext( pVHost, achURI, HandlerType::HT_SERVLET,
                                               NULL, pWorkerName, allowBrowse );
                if ( !pInnerContext )
                {
                    LOG_ERR(( "[%s] Failed to import servlet mapping for %s",
                              getLogId(), pMappingNode->getChildValue( "url-pattern" ) ));
                }
            }
        }
    }
    delete pRoot;
    return pContext;
}

static int detectIP( char family, char * str, char * pEnd)
{
    struct ifi_info * pHead = NICDetect::get_ifi_info( family, 1 );
    struct ifi_info * iter;
    char * pBegin = str;
    char temp[80];
    for ( iter = pHead; iter != NULL; iter = iter->ifi_next )
    {
        if ( iter->ifi_addr )
        {
            GSockAddr::ntop( iter->ifi_addr, temp, 80 );
            if ( family == AF_INET6 )
            {
                const struct in6_addr * pV6 = &((const struct sockaddr_in6 *)iter->ifi_addr)->sin6_addr;
                if (( !IN6_IS_ADDR_LINKLOCAL( pV6 ))&&
                        ( !IN6_IS_ADDR_SITELOCAL( pV6 ))&&
                        ( !IN6_IS_ADDR_MULTICAST( pV6 )))
                {
                    if ( pBegin != str )
                        *str++ = ',';
                    str += safe_snprintf( str, pEnd - str, "%s:[%s]", iter->ifi_name, temp );
                }
            }
            else
            {
                if ( pBegin != str )
                    *str++ = ',';
                str += safe_snprintf( str, pEnd - str, "%s:%s", iter->ifi_name, temp );
            }
        }
    }
    if ( pHead )
        NICDetect::free_ifi_info( pHead );
    return 0;
}

#define ADMIN_PHP_SESSION           "$SERVER_ROOT/admin/tmp"

int HttpServerBuilder::initAdmin(  const XmlNode * pNode )
{
    char achPHPBin[MAX_PATH_LEN];
    LogIdTracker tracker( "config:admin" );
    if ( getAbsoluteFile( achPHPBin, DEFAULT_ADMIN_PHP_FCGI ) ||
        ( access( achPHPBin, X_OK ) != 0 ))
    {
        LOG_ERR(( "[%s] missing PHP binary for admin server - %s!",
                  getLogId(), achPHPBin ));
        return -1;
    }
    const char * pURI;
    pURI = pNode->getChildValue( "phpFcgiAddr" );
    if ( pURI )
    {
        if (( strncasecmp( pURI, "UDS://", 6 ) != 0 )&&
            ( strncasecmp( pURI, "127.0.0.1:", 10 ) != 0 )&&
            ( strncasecmp( pURI, "localhost:", 10 ) != 0 ))
        {
            LOG_WARN(( "[%s] The PHP fast CGI for admin server must"
            " use localhost interface"
            " or unix domain socket, use default!",
            getLogId() ));
            pURI = DEFAULT_ADMIN_PHP_FCGI_URI;
        }
    }
    else
    {
        pURI = DEFAULT_ADMIN_PHP_FCGI_URI;
    }
    GSockAddr addr;
    if ( addr.set( pURI, NO_ANY ) )
    {
        LOG_ERR(( "[%s] failed to set socket address %s for %s!", getLogId(),
                  pURI, DEFAULT_ADMIN_FCGI_NAME ));
        return -1;
    }
    LocalWorker * pFcgiApp = (LocalWorker *)ExtAppRegistry::addApp(
        EA_LSAPI, DEFAULT_ADMIN_FCGI_NAME );
    assert( pFcgiApp );
    pFcgiApp->setURL( pURI );
    strcat( achPHPBin, " -c ../conf/php.ini" );
    pFcgiApp->getConfig().setAppPath( &achPHPBin[m_sChroot.len()] );
    pFcgiApp->getConfig().setBackLog( 10 );
    pFcgiApp->getConfig().setSelfManaged( 0 );
    pFcgiApp->getConfig().setStartByServer( 1 );
    pFcgiApp->setMaxConns( 4 );
    pFcgiApp->getConfig().setKeepAliveTimeout( 30 );
    pFcgiApp->getConfig().setInstances( 4 );
    pFcgiApp->getConfig().clearEnv();
    //pFcgiApp->getConfig().addEnv( "FCGI_WEB_SERVER_ADDRS=127.0.0.1" );
    pFcgiApp->getConfig().addEnv( "PHP_FCGI_MAX_REQUESTS=1000" );
    snprintf( achPHPBin, MAX_PATH_LEN, "LSWS_EDITION=LiteSpeed Web Server/%s/%s",
              "Open",
              VERSION  );
    pFcgiApp->getConfig().addEnv( achPHPBin );
    RLimits limits;
    limits.setDataLimit( 500*1024*1024, 500*1024*1024 );
    limits.setProcLimit( 1000, 1000 );
    pFcgiApp->getConfig().setRLimits( &limits );
    
    char pEnv[8192];
    //snprintf( pEnv, 2048, "HTTPD_SERVER_ROOT=%s", getServerRoot() );
    //pFcgiApp->getConfig().addEnv( pEnv );
    
    snprintf( pEnv, 2048, "LS_SERVER_ROOT=%s", getServerRoot() );
    pFcgiApp->getConfig().addEnv( pEnv );
    
    if ( m_sChroot.c_str() )
    {
        snprintf( pEnv, 2048, "LS_CHROOT=%s", m_sChroot.c_str() );
        pFcgiApp->getConfig().addEnv( pEnv );
    }
    snprintf( pEnv, 2048, "LS_PRODUCT=ows" );
    pFcgiApp->getConfig().addEnv( pEnv );
    
    snprintf( pEnv, 2048, "LS_PLATFORM=%s", LS_PLATFORM );
    pFcgiApp->getConfig().addEnv( pEnv );
    
    snprintf( pEnv, 2048, "LSWS_CHILDREN=%d", HttpGlobals::s_children );
    pFcgiApp->getConfig().addEnv( pEnv );
    if ( HttpGlobals::s_pAdminSock )
    {
        snprintf( pEnv, 2048, "LSWS_ADMIN_SOCK=%s", HttpGlobals::s_pAdminSock );
        pFcgiApp->getConfig().addEnv( pEnv );
    }
    
    strcpy( pEnv, "LSWS_IPV4_ADDRS=" );
    
    if ( detectIP( AF_INET, pEnv + strlen( pEnv ), &pEnv[8192] ) == 0 )
    {
        pFcgiApp->getConfig().addEnv( pEnv );
        //pFcgiApp->getConfig().addEnv( &pEnv[5] );
        m_sIPv4.setStr( &pEnv[16] );
    }
    strcpy( pEnv, "LSWS_IPV6_ADDRS=" );
    if ( detectIP( AF_INET6, pEnv + strlen( pEnv ), &pEnv[8192] ) == 0 )
    {
        pFcgiApp->getConfig().addEnv( pEnv );
    }
    pFcgiApp->getConfig().addEnv( "PATH=/bin:/usr/bin:/usr/local/bin" );
    pFcgiApp->getConfig().addEnv( NULL );
    m_pAdminEnv = pFcgiApp->getConfig().getEnv();
    //    if ( !pFcgiApp->isReady() )
    //    {
        //        if ( pFcgiApp->start() )
    //        {
        //            return -1;
    //        }
    //        pFcgiApp->stop();
    //    }
    
    
    HttpVHost* pVHostAdmin = m_pServer->getVHost( DEFAULT_ADMIN_SERVER_NAME );
    if ( !pVHostAdmin )
    {
        char achRootPath[MAX_PATH_LEN];
        pVHostAdmin = new HttpVHost(DEFAULT_ADMIN_SERVER_NAME);
        if ( !pVHostAdmin )
        {
            ERR_NO_MEM( "new HttpVHost()" );
            return -1;
        }
        if ( m_pServer->addVHost( pVHostAdmin ) != 0 )
        {
            LOG_ERR(( "[%s] failed to add admin virtual host!",
            getLogId() ));
            delete pVHostAdmin;
            return -1;
        }
        pFcgiApp->getConfig().setVHost( pVHostAdmin );
        
        strcpy( achPHPBin, m_achVhRoot );
        strcat( achPHPBin, "html/" );
        pVHostAdmin->setDocRoot( achPHPBin );
        HttpContext * pCtx = pVHostAdmin->addContext( "/", HandlerType::HT_NULL, achPHPBin, NULL, 1 );

        getAbsoluteFile( achRootPath, "$SERVER_ROOT/docs/" );
        HttpContext * pDocs = pVHostAdmin->addContext( "/docs/",
                                                        HandlerType::HT_NULL, &achRootPath[m_sChroot.len()], NULL, 1 );
        pVHostAdmin->addContext( pDocs );
        //pVHostAdmin->addContext( pSecure );
        pDocs = &pVHostAdmin->getRootContext();
        pDocs->addDirIndexes( "index.html, index.php" );
        /*
        CacheConfig * pConfig = pDocs->addCacheConfig( "" );
        if ( pConfig )
        {
            pConfig->setConfigBit( CACHE_ENABLED, 0 );
            pConfig->setConfigBit( CACHE_PRIVATE_ENABLED, 0 );
        }
        */
        char achPHPSuffix[10] = "php";
        setPHPHandler( pDocs, pFcgiApp, achPHPSuffix );
        
        char achMIME[] = "text/html";
        strcpy( achPHPSuffix, "html" );
        pDocs->getMIME()->addMimeHandler( achPHPSuffix, achMIME,
                                        HandlerFactory::getInstance(HandlerType::HT_NULL, NULL), NULL,  getLogId() );
        
        
        pVHostAdmin->enableScript( 1 );
        pVHostAdmin->followSymLink( 2 );
        pVHostAdmin->restrained( 1 );
        pVHostAdmin->getExpires().enable( 1 );
        pVHostAdmin->contextInherit();
        getAbsoluteFile( achRootPath, "$SERVER_ROOT/conf/" );
        pVHostAdmin->setUidMode( UID_DOCROOT );
        pVHostAdmin->updateUGid( getLogId(), &achRootPath[m_sChroot.len()] );
        
        chown( &HttpGlobals::s_pAdminSock[5], pVHostAdmin->getUid(), pVHostAdmin->getGid() );
        //chown( "/tmp/lshttpd/", -1, pVHostAdmin->getGid() );
        
        const char *pUserFile = "$SERVER_ROOT/admin/conf/htpasswd";
        if ( getValidFile(achPHPBin, pUserFile, "user DB") == 0 )
        {
            UserDir * pUserDir =
            pVHostAdmin->getFileUserDir( ADMIN_USERDB, achPHPBin,
                                        NULL );
            if ( !pUserDir )
            {
                LOG_ERR(( "[%s] Failed to create authentication DB.", getLogId() ));
            }
        }
        //pVHostAdmin->enableGzip( (HttpServerConfig::getInstance().getGzipCompress())?
        //                            1 : 0 );
    }
    ThrottleLimits * pTC = pVHostAdmin->getThrottleLimits();
#ifdef DEV_DEBUG
    pTC->setDynReqLimit( 100 );
    pTC->setStaticReqLimit( 3000 );
#else
    pTC->setDynReqLimit( 2 );
    pTC->setStaticReqLimit( 30 );
#endif
    if ( ThrottleControl::getDefault()->getOutputLimit() != INT_MAX )
    {
        pTC->setOutputLimit( 40960000 );
        pTC->setInputLimit( 20480 );
    }
    else
    {
        pTC->setOutputLimit( INT_MAX );
        pTC->setInputLimit( INT_MAX );
    }   
    
    
    configVHSecurity( pVHostAdmin, pNode );
    initErrorLog( *pVHostAdmin, pNode, 0 );
    initAccessLog( *pVHostAdmin, pNode, 0 );
    //pVHostAdmin->rotateLogFiles();
    //return( access( path, F_OK ) == 0 );
    //test if file $SERVER_ROOT/conf/disablewebconsole exist 
    //skip admin listener configuration
    char theWebConsolePathe[MAX_PATH_LEN];
    if(( getAbsoluteFile( theWebConsolePathe, "$SERVER_ROOT/conf/httpd_config.xml" ) )==0)
    {
        if ( access( theWebConsolePathe, F_OK ) == 0 )
        {
            if(( getAbsoluteFile( theWebConsolePathe, "$SERVER_ROOT/conf/disablewebconsole" ) )==0)
            {
                if ( access( theWebConsolePathe, F_OK ) == 0 )
                    return 0;
            }           
    }
    else
        return 0;
    } 
    else
        return 0;
    
    const XmlNode* pListenerNodes = pNode->getChild("listenerList");
    const XmlNodeList* pList = pListenerNodes->getChildren( "listener" );
    if ( pList )
    {
        XmlNodeList::const_iterator iter;
        for ( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            const XmlNode * pListenerNode = *iter;
            const char * pName = pListenerNode->getChildValue( "name" );
            HttpListener * pListener = m_pServer->getListener( pName );
            if ( pListener )
            {
                pListener->setBinding( 1 );
                pListener->getVHostMap()->clear();
                m_pServer->mapListenerToVHost( pName, "*", DEFAULT_ADMIN_SERVER_NAME );
            }
        }
    }
    
    return 0;
}

void HttpServerBuilder::setServerRoot( const char * pRoot )
{
    m_sServerRoot = pRoot;
    HttpGlobals::s_pServerRoot = m_sServerRoot.c_str();
}
SSLContext * HttpServerBuilder::newSSLContext( const XmlNode* pNode )
{

    const char* pCertFile = getTag( pNode,  "certFile" );
    if ( !pCertFile )
    {
        return NULL;
    }
    const char* pKeyFile = getTag( pNode, "keyFile" );
    if ( !pKeyFile )
    {
        return NULL;
    }
    char achCert[MAX_PATH_LEN];
    if ( getValidFile(achCert, pCertFile, "certificate file" ) != 0 )
        return NULL;
    char achKey[MAX_PATH_LEN];
    if ( getValidFile(achKey, pKeyFile, "key file") != 0 )
        return NULL;
    const char * pCiphers = pNode->getChildValue( "ciphers" );
    if ( !pCiphers )
    {
        pCiphers = "ALL:!ADH:!EXPORT56:RC4+RSA:+HIGH:+MEDIUM:!SSLv2:+EXP";
    }
    const char * pCAPath = pNode->getChildValue( "CACertPath" );
    const char * pCAFile = pNode->getChildValue( "CACertFile" );
    char achCAFile[MAX_PATH_LEN];
    char achCAPath[MAX_PATH_LEN];
    if ( pCAPath )
    {
        if ( getValidPath( achCAPath, pCAPath, "CA Certificate path" ) != 0 )
            return NULL;
        pCAPath = achCAPath;
    }
    if ( pCAFile )
    {
        if ( getValidFile( achCAFile, pCAFile, "CA Certificate file" ) != 0 )
            return NULL;
        pCAFile = achCAFile;
    }
    int cv = 0;
    SSLContext * pSSL = m_pServer->newSSLContext(NULL, achCert,
        achKey, pCAFile, pCAPath, pCiphers, getLongValue( pNode, "certChain", 0, 1, 0 ), (cv!=0),
        getLongValue( pNode, "regenProtection", 0, 1, 1 )  );
    if ( pSSL == NULL )
    {
        LOG_ERR(( "[%s] failed to create SSL Context with key: %s, Cert: %s!",
                getLogId(), achKey, achCert ));
        return NULL;
    }

    return pSSL;
}
