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
#include "configctx.h"
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <http/denieddir.h>
#include <http/handlertype.h>
#include <util/gpath.h>
#include <util/xmlnode.h>
#include <util/ni_fio.h>


#include <log4cxx/level.h>
#include "main/mainserverconfig.h"
#include "main/plainconf.h"

#include <errno.h>
#include <limits.h>
#include <unistd.h>

#define MAX_URI_LEN  1024

#define VH_ROOT     "VH_ROOT"
#define DOC_ROOT    "DOC_ROOT"
#define SERVER_ROOT "SERVER_ROOT"
#define VH_NAME "VH_NAME"

//static const char *MISSING_TAG = "missing <%s>";
static const char *MISSING_TAG_IN;// = "missing <%s> in <%s>";
//static const char *INVAL_TAG = "<%s> is invalid: %s";
//static const char * INVAL_TAG_IN = "[%s] invalid tag <%s> within <%s>!";
static const char *INVAL_PATH = "Path for %s is invalid: %s";
static const char *INACCESSIBLE_PATH = "Path for %s is not accessible: %s";
AutoStr2    ConfigCtx::s_sVhName;
AutoStr2    ConfigCtx::s_sVhDomain( "" );
AutoStr2    ConfigCtx::s_sVhAliases( "" );
char        ConfigCtx::s_achVhRoot[MAX_PATH_LEN];
char        ConfigCtx::s_achDocRoot[MAX_PATH_LEN];
ConfigCtx*  ConfigCtx::s_pCurConfigCtx = NULL;


long long getLongValue( const char *pValue, int base )
{
    long long l = strlen( pValue );
    long long m = 1;
    char ch = * ( pValue + l - 1 );

    if ( ch == 'G' || ch == 'g' )
    {
        m = 1024 * 1024 * 1024;
    }
    else
        if ( ch == 'M' || ch == 'm' )
        {
            m = 1024 * 1024;
        }
        else
            if ( ch == 'K' || ch == 'k' )
            {
                m = 1024;
            }

    return strtoll( pValue, ( char ** ) NULL, base ) * m;
}

void ConfigCtx::vlog( int level, const char *pFmt, va_list args )
{
    char achBuf[8192];

    int len = safe_vsnprintf( achBuf, sizeof( achBuf ) - 1, pFmt, args );
    achBuf[len] = 0;
    HttpLog::log( level, "[%s] %s", m_logIdTracker.getLogId(), achBuf );
}

void ConfigCtx::log_error( const char *pFmt, ... )
{
    if ( HttpLog::isEnabled( NULL, LOG4CXX_NS::Level::ERROR ) )
    {
        va_list ap;
        va_start( ap, pFmt );
        vlog( LOG4CXX_NS::Level::ERROR, pFmt, ap );
        va_end( ap );
    }
}

void ConfigCtx::log_errorPath( const char *pstr1,  const char *pstr2)
{
    if ( HttpLog::isEnabled( NULL, LOG4CXX_NS::Level::ERROR ) )
    {
        HttpLog::log( LOG4CXX_NS::Level::ERROR, "[%s] Path for %s is invalid: %s", m_logIdTracker.getLogId(), pstr1, pstr2 );
    }
}
void ConfigCtx::log_errorInvalTag( const char *pstr1,  const char *pstr2)
{
    if ( HttpLog::isEnabled( NULL, LOG4CXX_NS::Level::ERROR ) )
    {
        HttpLog::log( LOG4CXX_NS::Level::ERROR, "[%s] <%s> is invalid: %s", m_logIdTracker.getLogId(), pstr1, pstr2 );
    }
}

void ConfigCtx::log_errorMissingTag( const char *pstr1)
{
    if ( HttpLog::isEnabled( NULL, LOG4CXX_NS::Level::ERROR ) )
    {
        HttpLog::log( LOG4CXX_NS::Level::ERROR, "[%s] missing <%s>", m_logIdTracker.getLogId(), pstr1);
    }
}
void ConfigCtx::log_warn( const char *pFmt, ... )
{
    if ( HttpLog::isEnabled( NULL, LOG4CXX_NS::Level::WARN ) )
    {
        va_list ap;
        va_start( ap, pFmt );
        vlog( LOG4CXX_NS::Level::WARN, pFmt, ap );
        va_end( ap );
    }
}
void ConfigCtx::log_notice( const char *pFmt, ... )
{
    if ( HttpLog::isEnabled( NULL, LOG4CXX_NS::Level::NOTICE ) )
    {
        va_list ap;
        va_start( ap, pFmt );
        vlog( LOG4CXX_NS::Level::NOTICE, pFmt, ap );
        va_end( ap );
    }
}

void ConfigCtx::log_info( const char *pFmt, ... )
{
    if ( HttpLog::isEnabled( NULL, LOG4CXX_NS::Level::INFO ) )
    {
        va_list ap;
        va_start( ap, pFmt );
        vlog( LOG4CXX_NS::Level::INFO, pFmt, ap );
        va_end( ap );
    }
}

void ConfigCtx::log_debug( const char *pFmt, ... )
{
    if ( HttpLog::isEnabled( NULL, LOG4CXX_NS::Level::DEBUG ) )
    {
        va_list ap;
        va_start( ap, pFmt );
        vlog( LOG4CXX_NS::Level::DEBUG, pFmt, ap );
        va_end( ap );
    }
}
const char *ConfigCtx::getTag( const XmlNode *pNode, const char *pName, int bKeyName )
{
    const char *pRet = pNode->getChildValue( pName, bKeyName );
    if ( !pRet )
    {
        log_error( MISSING_TAG_IN, pName, pNode->getName() );
    }

    return pRet;
}

long long ConfigCtx::getLongValue( const XmlNode *pNode, const char *pTag,
                                   long long min, long long max, long long def, int base )
{
    const char *pValue = pNode->getChildValue( pTag );
    long long val;

    if ( pValue )
    {
        val = ::getLongValue( pValue, base );

        if ( ( max == INT_MAX ) && ( val > max ) )
            val = max;

        if ( ( ( min != LLONG_MIN ) && ( val < min ) ) ||
                ( ( max != LLONG_MAX ) && ( val > max ) ) )
        {
            log_warn( "invalid value of <%s>:%s, use default=%ld", pTag, pValue, def );
            return def;
        }

        return val;
    }
    else
        return def;

}

int ConfigCtx::getRootPath( const char *&pRoot, const char *&pFile )
{
    int offset = 1;

    if ( *pFile == '$' )
    {
        if ( strncasecmp( pFile + 1, VH_ROOT, 7 ) == 0 )
        {
            if ( s_achVhRoot[0] )
            {
                pRoot = s_achVhRoot;
                pFile += 8;
            }
            else
            {
                log_error( "Virtual host root path is not available for %s.", pFile );
                return -1;
            }
        }
        else
            if ( strncasecmp( pFile + 1, DOC_ROOT, 8 ) == 0 )
            {
                if ( s_achDocRoot[0] )
                {
                    pRoot = getDocRoot();
                    pFile += 9;
                }
                else
                {
                    log_error( "Document root path is not available for %s.", pFile );
                    return -1;
                }
            }
            else
                if ( strncasecmp( pFile + 1, SERVER_ROOT, 11 ) == 0 )
                {
                    pRoot = MainServerConfig::getInstance().getServerRoot();
                    pFile += 12;
                }
    }
    else
    {
        offset = 0;

        if ( ( *pFile != '/' ) && ( s_achDocRoot[0] ) )
        {
            pRoot = getDocRoot();
        }
    }

    if ( ( offset ) && ( *pFile == '/' ) )
        ++pFile;

    return 0;
}
int ConfigCtx::expandVariable( const char *pValue, char *pBuf,
                                       int bufLen, int allVariable )
{
    const char *pBegin = pValue;
    char *pBufEnd = pBuf + bufLen - 1;
    char *pCur = pBuf;
    int len;

    while( *pBegin )
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
            if ( strncasecmp( pBegin + 1, VH_NAME, 7 ) == 0 )
            {
                pBegin += 8;
                int nameLen = s_sVhName.len();

                if ( nameLen > 0 )
                {
                    if ( nameLen > pBufEnd - pCur )
                        return -1;

                    memmove( pCur, s_sVhName.c_str(), nameLen );
                    pCur += nameLen;
                }
            }
            else
                if ( ( allVariable )
                        && ( ( strncasecmp( pBegin + 1, VH_ROOT, 7 ) == 0 )
                             || ( strncasecmp( pBegin + 1, DOC_ROOT, 8 ) == 0 )
                             || ( strncasecmp( pBegin + 1, SERVER_ROOT, 11 ) == 0 ) ) )
                {
                    const char *pRoot = "";
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
int ConfigCtx::getAbsolute( char *res, const char *path, int pathOnly )
{
    const char *pChroot = MainServerConfig::getInstance().getChroot();
    int iChrootLen = MainServerConfig::getInstance().getChrootlen();    
    const char *pRoot = "";
    const char *pPath = path;
    int ret;
    char achBuf[MAX_PATH_LEN];
    char *dest = achBuf;
    int len = MAX_PATH_LEN;

    if ( getRootPath( pRoot, pPath ) )
        return -1;

    if ( pChroot )
    {
        if ( ( *pRoot ) || ( strncmp( path, pChroot, 
            iChrootLen ) != 0 ) )
        {
            memmove( dest, pChroot, iChrootLen );
            dest += iChrootLen;
            len -= iChrootLen;
        }
    }

    if ( pathOnly )
        ret = GPath::getAbsolutePath( dest, len, pRoot, pPath );
    else
        ret = GPath::getAbsoluteFile( dest, len, pRoot, pPath );

    if ( ret )
    {
        log_error( "Failed to tanslate to absolute path with root=%s, "
                                    "path=%s!", pRoot, path );
    }
    else
    {
        // replace "$VH_NAME" with the real name of the virtual host.
        if ( expandVariable( achBuf, res, MAX_PATH_LEN ) < 0 )
        {
            log_notice( "Path is too long: %s", pPath );
            return -1;
        }
    }

    return ret;    
}
int ConfigCtx::getAbsoluteFile( char *dest, const char *file )
{
    return getAbsolute( dest, file, 0 );
}

int ConfigCtx::getAbsolutePath( char *dest, const char *path )
{
    return getAbsolute( dest, path, 1 );
}

char *ConfigCtx::getExpandedTag( const XmlNode *pNode,
        const char *pName, char *pBuf, int bufLen, int bKeyName )
{
    const char *pVal = getTag( pNode, pName, bKeyName );

    if ( !pVal )
        return NULL;
    //if ( expandVariable( pVal, pBuf, bufLen ) >= 0 )
    if ( expandVariable( pVal, pBuf, bufLen ) >= 0 )
        return pBuf;

    log_notice( "String is too long for tag: %s, value: %s, maxlen: %d",
                                 pName, pVal, bufLen );
    return NULL;
}


int ConfigCtx::getValidFile( char *dest, const char *file, const char *desc )
{
    if ( ( getAbsoluteFile( dest, file ) != 0 )
            || access( dest, F_OK ) != 0 )
    {
        log_error( INVAL_PATH, desc,  dest );
        return -1;
    }

    return 0;
}

int ConfigCtx::getValidPath( char *dest, const char *path, const char *desc )
{
    if ( getAbsolutePath( dest, path ) != 0 )
    {
        log_error( INVAL_PATH, desc,  path );
        return -1;
    }

    if ( access( dest, F_OK ) != 0 )
    {
        log_error( INACCESSIBLE_PATH, desc,  dest );
        return -1;
    }

    return 0;
}

int ConfigCtx::getValidChrootPath( const char *path, const char *desc )
{
    const char *pChroot = MainServerConfig::getInstance().getChroot();
    int iChrootLen = MainServerConfig::getInstance().getChrootlen();        
    if ( getValidPath( s_achVhRoot, path, desc ) == -1 )
        return -1;

    if ( ( pChroot) &&
            ( strncmp( s_achVhRoot, pChroot,  iChrootLen ) == 0 ) )
    {
        memmove( s_achVhRoot, s_achVhRoot + iChrootLen,
                 strlen( s_achVhRoot ) - iChrootLen + 1 );
    }
    return 0;
}

int ConfigCtx::getLogFilePath( char *pBuf, const XmlNode *pNode )
{
    const char *pValue = getTag( pNode, "fileName", 1 );

    if ( pValue == NULL )
    {
        return 1;
    }

    if ( getAbsoluteFile( pBuf, pValue ) != 0 )
    {
        log_error( "ath for %s is invalid: %s", "log file",  pValue );
        return 1;
    }

    if ( GPath::isWritable( pBuf ) == false )
    {
        log_error( "log file is not writable - %s", pBuf );
        return 1;
    }

    return 0;
}
int ConfigCtx::expandDomainNames( const char *pDomainNames,
        char *pDestDomains, int len, char dilemma )
{
    if ( !pDomainNames )
    {
        pDestDomains[0] = 0;
        return 0;
    }

    const char *p = pDomainNames;

    char *pDest = pDestDomains;

    char *pEnd = pDestDomains + len - 1;

    const char *pStr;

    int n;

    while( ( *p ) && ( pDest < pEnd ) )
    {
        n = strspn( p, " ," );

        if ( n )
            p += n;

        n = strcspn( p, " ," );

        if ( !n )
            continue;

        if ( ( strncasecmp( p, "$vh_domain", 10 ) == 0 ) &&
                ( 10 == n ) )
        {
            pStr = s_sVhDomain.c_str();
            len = s_sVhDomain.len();            
        }
        else
            if ( ( strncasecmp( p, "$vh_aliases", 11 ) == 0 ) &&
                    ( 11 == n ) )
            {
                pStr = s_sVhAliases.c_str();
                len = s_sVhAliases.len();                
            }
            else
            {
                pStr = p;
                len = n;
            }

        if ( ( pDest != pDestDomains ) && ( pDest < pEnd ) )
            *pDest++ = dilemma;

        if ( len > pEnd - pDest )
            len = pEnd - pDest;

        memmove( pDest, pStr, len );
        pDest += len;
        p += n;
    }

    *pDest = 0;
    *pEnd = 0;
    return pDest - pDestDomains;
}
int ConfigCtx::checkPath( char *pPath, const char *desc, int follow)
{
    char achOld[MAX_PATH_LEN];
    struct stat st;
    int ret = nio_stat( pPath, &st );

    if ( ret == -1 )
    {
        log_errorPath( desc,  pPath );
        return -1;
    }

    memccpy( achOld, pPath, 0, MAX_PATH_LEN - 1 );
    ret = GPath::checkSymLinks( pPath, pPath + strlen( pPath ),
                                pPath + MAX_PATH_LEN, pPath, follow );

    if ( ret == -1 )
    {
        if ( errno == EACCES )
            log_error( "Path of %s contains symbolic link or"
                                    " ownership does not match:%s",
                                    desc, pPath );
        else
            log_errorPath( desc,  pPath );

        return -1;
    }

    if ( S_ISDIR( st.st_mode ) )
    {
        if ( * ( pPath + ret - 1 ) != '/' )
        {
            * ( pPath + ret ) = '/';
            * ( pPath + ret + 1 ) = 0;
        }
    }

    if ( strcmp( achOld, pPath ) != 0 )
        if ( D_ENABLED( DL_LESS ) )
            log_debug( "the real path of %s is %s.", achOld, pPath );

    if ( HttpGlobals::s_psChroot )  //offset chroot
        pPath += HttpGlobals::s_psChroot->len();

    if ( checkAccess( pPath ) )
        return -1;

    return 0;
}
int ConfigCtx::checkAccess( char *pReal)
{
    if ( HttpGlobals::getDeniedDir()->isDenied( pReal ) )
    {
        log_error( "Path is in the access denied list:%s", pReal );
        return -1;
    }

    return 0;
}

int ConfigCtx::convertToRegex( const char   *pOrg, char *pDestBuf, int bufLen )
{
    const char *p = pOrg;
    char *pDest = pDestBuf;
    char *pEnd = pDest + bufLen - 1;
    int n;

    while( ( *p ) && ( pDest < pEnd - 3 ) )
    {
        n = strspn( p, " ," );

        if ( n )
            p += n;

        n = strcspn( p, " ," );

        if ( !n )
            continue;

        if ( ( pDest != pDestBuf ) && ( pDest < pEnd ) )
            *pDest++ = ' ';

        if ( ( strncasecmp( p, "REGEX[", 6 ) != 0 ) &&
                ( ( memchr( p, '?', n ) ) || ( memchr( p, '*', n ) ) ) &&
                ( pEnd - pDest > 10 ) )
        {
            const char *pB = p;
            const char *pE = p + n;
            memmove( pDest, "REGEX[", 6 );
            pDest += 6;

            while( ( pB < pE ) && ( pEnd - pDest > 3 ) )
            {
                if ( '?' == *pB )
                    *pDest++ = '.';
                else
                    if ( '*' == *pB )
                    {
                        *pDest++ = '.';
                        *pDest++ = '*';
                    }
                    else
                        if ( '.' == *pB )
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
                len = pEnd - pDest;

            memmove( pDest, p, len );
            pDest += len;
        }

        p += n;
    }

    *pDest = 0;
    *pEnd = 0;
    return pDest - pDestBuf;
}

XmlNode *ConfigCtx::parseFile( const char *configFilePath, const char *rootTag )
{
    char achError[4096];
    XmlTreeBuilder tb;
    XmlNode *pRoot = tb.parse( configFilePath, achError, 4095 );

    if ( pRoot == NULL )
    {
        log_error( "%s", achError );
        return NULL;
    }

    // basic validation
    if ( strcmp( pRoot->getName(), rootTag ) != 0 )
    {
        log_error( "%s: root tag expected: <%s>, real root tag : <%s>!\n",
                                configFilePath, rootTag, pRoot->getName() );
        delete pRoot;
        return NULL;
    }

#ifdef TEST_OUTPUT_PLAIN_CONF
    char sPlainFile[512] = {0};
    strcpy( sPlainFile, configFilePath );
    strcat( sPlainFile, ".txt" );
    plainconf::testOutputConfigFile( pRoot, sPlainFile );
#endif

    return pRoot;

}


