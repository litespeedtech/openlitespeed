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
#include "httpcontext.h"

#include <http/contextlist.h>
#include <http/contexttree.h>
#include <http/handlertype.h>
#include <http/htauth.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <http/httpmime.h>
#include <http/phpconfig.h>
#include <http/rewriterulelist.h>
#include <http/statusurlmap.h>
#include <http/urimatch.h>
#include <http/userdir.h>

#include <util/accesscontrol.h>
#include <util/pool.h>
#include <util/stringlist.h>
#include <util/stringtool.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <util/ssnprintf.h>


CtxInt HttpContext::s_defaultInternal =
{   NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, } ;

HttpContext::HttpContext()
    : m_iConfigBits( 0 )
    , m_pURIMatch( NULL )
    , m_pMatchList( NULL )
    , m_pFilesMatchStr( NULL )
    , m_pHandler( NULL )
    , m_pInternal( NULL )
    , m_bAllowBrowse( BIT_ALLOW_BROWSE | BIT_INCLUDES | BIT_INCLUDES_NOEXEC )
    , m_redirectCode( -1 )
    , m_iSetUidMode( ENABLE_SCRIPT )
    , m_iConfigBits2( BIT_URI_CACHEABLE )
    , m_iRewriteEtag( 0 )
//    , m_iFilesMatchCtx( 0 )
    , m_lHTALastMod( 0 )
    , m_pRewriteBase( NULL )
    , m_pRewriteRules( NULL )
    , m_pParent( NULL )
{
    m_pInternal = &s_defaultInternal;
    
}

HttpContext::~HttpContext()
{
    if ( m_pMatchList )
    {
        delete m_pMatchList;
    }
    
    if (m_pFilesMatchStr )
        delete m_pFilesMatchStr;
    if ( m_pURIMatch )
        delete m_pURIMatch;
    if ( m_pRewriteBase )
        delete m_pRewriteBase;
    releaseHTAConf();

    if (( m_iConfigBits & BIT_CTXINT ))
    {
        g_pool.deallocate( m_pInternal, sizeof( CtxInt ) );
    }

}

void HttpContext::releaseHTAConf()
{
    if (( m_pRewriteRules )&&( m_iConfigBits & BIT_REWRITE_RULE ))
    {
        delete m_pRewriteRules;
    }
    if (( m_iConfigBits & BIT_CTXINT ))
    {
        if (( m_iConfigBits & BIT_DIRINDEX )&&( m_pInternal->m_pIndexList ))
        {
            delete m_pInternal->m_pIndexList;
        }
        if (( m_iConfigBits & BIT_PHPCONFIG )&&( m_pInternal->m_pPHPConfig ))
            delete m_pInternal->m_pPHPConfig;
        if (( m_iConfigBits & BIT_ERROR_DOC )&&( m_pInternal->m_pCustomErrUrls ))
            delete m_pInternal->m_pCustomErrUrls;
        if (( m_iConfigBits & BIT_AUTH_REQ )&&( m_pInternal->m_pRequired ))
            delete m_pInternal->m_pRequired;
        if (( m_iConfigBits & BIT_FILES_MATCH )&&( m_pInternal->m_pFilesMatchList ))
            delete m_pInternal->m_pFilesMatchList;
        if (( m_iConfigBits & BIT_EXTRA_HEADER )&&( m_pInternal->m_pExtraHeader ))
            delete m_pInternal->m_pExtraHeader;
        releaseHTAuth();
        releaseAccessControl();
        releaseMIME();
        releaseDefaultCharset();
        
        memset( m_pInternal, 0 , sizeof( CtxInt ) );
        m_iConfigBits = BIT_CTXINT;
    }
    else
        clearConfigBit();
}



int HttpContext::set( const char * pURI, const char * pLocation,
            const HttpHandler * pHandler, bool browse, int regex)
{
    if ( pURI == NULL )
        return EINVAL;
    if ( strncasecmp( pURI, "exp:", 4 ) == 0 )
    {
        regex = 1;
        pURI += 4;
        while( isspace( *pURI ) )
            ++pURI;
        if ( !*pURI )
            return EINVAL;
    }
    if ( regex )
    {
        int ret = setURIMatch( pURI, pLocation );
        if ( ret )
            return ret;
    }
    int isDir= 0;
    if (( pLocation )&&( *pLocation ))
    {
        int len = strlen( pLocation );
        m_sLocation.resizeBuf( len + 15 );
        strcpy( m_sLocation.buf(), pLocation );
        m_sLocation.setLen( len );
        if ( *(pLocation + len - 1 ) == '/' )
            isDir = 1;
//        if ( type < HandlerType::HT_FASTCGI )
//        {
//            // if context URI end with '/', then the m_sLocation must be end with '/'
//            if  (( '/' == *( getURI() + getURILen() - 1 ))&&
//                ( m_sLocation.at( m_sLocation.len() - 1 ) != '/' ))
//                m_sLocation += "/";
//        }
    }
    int len = strlen( pURI );
    m_sContextURI.resizeBuf( len + 8 );
    strcpy( m_sContextURI.buf(), pURI );
    if (( isDir && !regex )&&( *( pURI + len - 1 ) != '/' ))
    {
        *( m_sContextURI.buf() + len++ ) = '/';
        *( m_sContextURI.buf() + len ) = '\0';
    }
    m_sContextURI.setLen( len );
    m_pHandler = pHandler;
    allowBrowse(browse);
    return 0;
}

int HttpContext::setFilesMatch( const char * pURI, int regex )
{
    if ( !pURI )
        return -1;
    if ( regex )
    {
        //m_iFilesMatchCtx = 1;
        return setURIMatch( pURI, NULL );
    }
    else
    {
        if ( m_pFilesMatchStr )
        {
            //assert( m_iFilesMatchCtx );
            delete m_pFilesMatchStr;
        }
        m_pFilesMatchStr = StringTool::parseMatchPattern( pURI );
        //m_iFilesMatchCtx = 1;
        return ( m_pFilesMatchStr == NULL );
    }
    
}


int HttpContext::matchFiles( const char * pFile, int len ) const
{
    //if ( !m_iFilesMatchCtx )
    //    return 0;
    if ( m_pURIMatch )
    {
        return (m_pURIMatch->match( pFile, len ) > 0) ;
    }
    else if ( m_pFilesMatchStr )
    {
        return ( StringTool::strmatch( pFile, pFile + len,
                    m_pFilesMatchStr->begin(),
                    m_pFilesMatchStr->end(), 1 ) == 0 );
    }
    return 0;
}


const HttpContext * HttpContext::matchFilesContext( const char * pFile,
                    int len ) const
{
    //if ( !m_pInternal ||!m_pInternal->m_pFilesMatchList)
    if ( !m_pInternal->m_pFilesMatchList)
        return NULL;
    ContextList::iterator iter;
    for( iter = m_pInternal->m_pFilesMatchList->begin();
         iter != m_pInternal->m_pFilesMatchList->end();
         ++iter )
    {
        if ( (*iter)->matchFiles( pFile, len ) == 1 )
            return *iter;
    }
    return NULL;
}



int HttpContext::addFilesMatchContext( HttpContext * pContext )
{
    if (!( m_iConfigBits & BIT_FILES_MATCH ))
    {
        if ( allocateInternal() )
            return -1;
        ContextList * pList = new ContextList();
        if ( !pList )
            return -1;
        m_pInternal->m_pFilesMatchList = pList;
        m_iConfigBits |= BIT_FILES_MATCH;        
    }
    if ( m_pInternal->m_pFilesMatchList->add( pContext, 1 ) == -1 )
        return -1;
    pContext->setParent( this );
    return 0;
}

int HttpContext::setURIMatch( const char * pRegex, const char * pSubst )
{
    if ( m_pURIMatch )
        delete m_pURIMatch;
    m_pURIMatch = new URIMatch();
    if ( !m_pURIMatch )
        return ENOMEM;
    m_pURIMatch->set( pRegex, pSubst );
    if ( pRegex )
        m_sContextURI.setStr( pRegex );
    if ( pSubst )
        m_sLocation.setStr( pSubst );
    return 0;    
}

void HttpContext::setRoot( const char * pRoot )
{
    m_sLocation.setStr( pRoot, strlen( pRoot ) );
}

int HttpContext::allocateInternal()
{
    if ( !(m_iConfigBits & BIT_CTXINT ) )
    {
        m_pInternal = (CtxInt *)g_pool.allocate( sizeof( CtxInt ) );
        if ( !m_pInternal )
            return -1;
        memset( m_pInternal, 0, sizeof( CtxInt ) );
        m_iConfigBits |= BIT_CTXINT;
    }
    return 0;
}

void HttpContext::releaseMIME()
{
    if ((m_iConfigBits & BIT_MIME)&& m_pInternal->m_pMIME )
    {
        delete m_pInternal->m_pMIME;
        m_iConfigBits &= ~BIT_MIME;
        m_pInternal->m_pMIME = NULL;
    }
}

int HttpContext::initMIME()
{
    if (!(m_iConfigBits & BIT_MIME))
    {
        if ( allocateInternal() )
            return -1;
        HttpMime * pMIME = new HttpMime();
        if ( !pMIME )
            return -1;
        m_pInternal->m_pMIME = pMIME;
        m_iConfigBits |= BIT_MIME;
    }
    return 0;
}

static AutoStr2 * s_pDefaultCharset = NULL;

void HttpContext::releaseDefaultCharset()
{
    if ((m_iConfigBits & BIT_DEF_CHARSET)&& m_pInternal->m_pDefaultCharset
        &&( m_pInternal->m_pDefaultCharset != s_pDefaultCharset ))
    {
        delete m_pInternal->m_pDefaultCharset;
        m_iConfigBits &= ~BIT_DEF_CHARSET;
    }
    if ( m_pInternal )
        m_pInternal->m_pDefaultCharset = NULL;
}

void HttpContext::setDefaultCharset( const char * pCharset )
{
    releaseDefaultCharset();
    if ( pCharset )
    {
        if ( allocateInternal() )
            return ;
        char achBuf[256];
        safe_snprintf( achBuf, 255, "; charset=%s", pCharset );
        achBuf[255] = 0;
        m_pInternal->m_pDefaultCharset = new AutoStr2( achBuf );
    }
    m_iConfigBits |= BIT_DEF_CHARSET;
}

void HttpContext::setDefaultCharsetOn()
{
    releaseDefaultCharset();
    if ( !s_pDefaultCharset )
    {
        s_pDefaultCharset = new AutoStr2( "; charset=ISO-8859-1" );
        if ( !s_pDefaultCharset )
            return;
    }
    if ( allocateInternal() )
        return ;
    m_pInternal->m_pDefaultCharset = s_pDefaultCharset;
    m_iConfigBits |= BIT_DEF_CHARSET;
}


void HttpContext::setHTAuth(HTAuth* pHTAuth)
{
    releaseHTAuth();
    if ( allocateInternal() )
        return ;
    m_pInternal->m_pHTAuth = pHTAuth;
    m_iConfigBits |= BIT_AUTH;
}

int HttpContext::setExtraHeaders( const char * pLogId, const char * pHeaders, int len )
{
    if ( allocateInternal() )
        return -1;
    if ( !(m_iConfigBits & BIT_EXTRA_HEADER) )
    {
        m_pInternal->m_pExtraHeader = new AutoBuf( 512 );
        if ( !m_pInternal->m_pExtraHeader )
        {
            LOG_WARN(( "[%s] Failed to allocate buffer for extra headers", pLogId ));
            return -1;
        }
    }
    m_iConfigBits |= BIT_EXTRA_HEADER;
    if ( strcasecmp( pHeaders, "none" ) == 0 )
        return 0;
    
    const char * pEnd = pHeaders + len;
    const char * pLineBegin, *pLineEnd;
    pLineBegin = pHeaders;
    while( (pLineEnd = StringTool::getLine( pLineBegin, pEnd )) )
    {
        const char * pCurEnd = pLineEnd;
        StringTool::strtrim( pLineBegin, pCurEnd );
        if ( pLineBegin < pCurEnd )
        {
            const char * pHeaderNameEnd = strpbrk( pLineBegin, ": " );
            if (( pHeaderNameEnd )&&( pHeaderNameEnd > pLineBegin )&&
                    ( pHeaderNameEnd + 1 < pCurEnd ))
            {
                m_pInternal->m_pExtraHeader->append(
                    pLineBegin, pHeaderNameEnd - pLineBegin );
                m_pInternal->m_pExtraHeader->append( ':' );
                m_pInternal->m_pExtraHeader->append( pHeaderNameEnd + 1,
                            pCurEnd - pHeaderNameEnd - 1 );
                m_pInternal->m_pExtraHeader->append( "\r\n", 2 );
            }
            else
            {
                char * p = ( char * )pCurEnd;
                *p = '0';
                LOG_WARN(( "[%s] Invalid Header: %s", pLogId, pLineBegin ));
                *p = '\n';
            }
        }

        pLineBegin = pLineEnd;
        while( isspace( *pLineBegin ) )
            ++pLineBegin;
    }
    return 0;
}


void HttpContext::releaseHTAuth()
{
    if ((m_iConfigBits & BIT_AUTH)&&( m_pInternal->m_pHTAuth ))
    {
        delete m_pInternal->m_pHTAuth;
        m_pInternal->m_pHTAuth = NULL;
    }
}


int HttpContext::setAuthRequired( const char * pRequired )
{
    if (!(m_iConfigBits & BIT_AUTH_REQ )||
        (!m_pInternal)||!m_pInternal->m_pRequired )
    {
        if ( allocateInternal() )
            return -1;
        if ( !m_pInternal->m_pRequired )
        {
            m_pInternal->m_pRequired = new AuthRequired();
            if ( !m_pInternal->m_pRequired )
                return -1;
        }
        m_iConfigBits |= BIT_AUTH_REQ;

    }
    return m_pInternal->m_pRequired->parse( pRequired );
}


int HttpContext::setAccessControl(AccessControl* pAccess)
{
    releaseAccessControl();
    if ( allocateInternal() )
        return -1;
    m_pInternal->m_pAccessCtrl = pAccess;
    m_iConfigBits |= BIT_ACCESS;
    return 0;
}

void HttpContext::releaseAccessControl()
{
    if ((m_iConfigBits & BIT_ACCESS)&& m_pInternal->m_pAccessCtrl )
    {
        delete m_pInternal->m_pAccessCtrl;
        m_pInternal->m_pAccessCtrl = NULL;
    }
}

int HttpContext::addAccessRule( const char * pRule, int allow )
{
    if ( !(m_iConfigBits & BIT_ACCESS) || !m_pInternal->m_pAccessCtrl )
    {
        AccessControl * pCtrl = new AccessControl();
        if (( !pCtrl )||( setAccessControl( pCtrl ) ))
            return -1;
    }
    m_pInternal->m_pAccessCtrl->addList( pRule, allow );
    return 0;
}

int HttpContext::setAuthorizer( const HttpHandler * pHandler )
{
    if ( allocateInternal() )
        return -1;
    m_pInternal->m_pAuthorizer = pHandler;
    m_iConfigBits |= BIT_AUTHORIZER;
    return 0;
}



static RewriteRuleList * getValidRewriteRules(
    const HttpContext * pContext )
{
    while( pContext )
    {
        if ( pContext->getRewriteRules() )
            return pContext->getRewriteRules();
        if ( !(pContext->rewriteEnabled() & REWRITE_INHERIT) )
            break;
        pContext = pContext->getParent();
    }
    return NULL;
}

void HttpContext::inherit(const HttpContext * pRootContext )
{
    if ( !m_pParent )
        return;
    if ( !m_pHandler )
        m_pHandler = m_pParent->m_pHandler;
    if ( !(m_iConfigBits & BIT_CTXINT ) )
    {
        m_pInternal = m_pParent->m_pInternal;
    }
    else
    {
        if ( !(m_iConfigBits & BIT_AUTH) )
        {
            m_pInternal->m_pHTAuth = m_pParent->getHTAuth();
        }
        if ( !(m_iConfigBits & BIT_AUTH_REQ ) )
        {
            m_pInternal->m_pRequired = m_pParent->m_pInternal->m_pRequired;
        }
        if ( !(m_iConfigBits & BIT_ACCESS) )
        {
            m_pInternal->m_pAccessCtrl = m_pParent->getAccessControl();
        }
        if ( !(m_iConfigBits & BIT_DEF_CHARSET ) )
        {
            m_pInternal->m_pDefaultCharset = m_pParent->m_pInternal->m_pDefaultCharset;
        }
        if ( !(m_iConfigBits & BIT_MIME ) )
        {
            m_pInternal->m_pMIME = m_pParent->m_pInternal->m_pMIME;
        }
        else
        {
            if ( m_pInternal->m_pMIME )
            {
                m_pInternal->m_pMIME->inherit( HttpGlobals::getMime(), 1 );
                m_pInternal->m_pMIME->inherit( m_pParent->m_pInternal->m_pMIME, 0 );
            }
        }
        if ( !(m_iConfigBits & BIT_FORCE_TYPE ) )
        {
            m_pInternal->m_pForceType = m_pParent->m_pInternal->m_pForceType;
        }
        if ( !(m_iConfigBits & BIT_AUTHORIZER ) )
        {
            m_pInternal->m_pAuthorizer = m_pParent->m_pInternal->m_pAuthorizer;
        }
        if ( !(m_iConfigBits & BIT_DIRINDEX ))
        {
            m_pInternal->m_pIndexList = m_pParent->m_pInternal->m_pIndexList;
        }
        if ( !(m_iConfigBits & BIT_PHPCONFIG ))
            m_pInternal->m_pPHPConfig = m_pParent->m_pInternal->m_pPHPConfig;
        else
        {
            m_pInternal->m_pPHPConfig->merge( m_pParent->m_pInternal->m_pPHPConfig );
            m_pInternal->m_pPHPConfig->buildLsapiEnv();
        }
        if ( !(m_iConfigBits & BIT_ERROR_DOC ))
            m_pInternal->m_pCustomErrUrls = m_pParent->m_pInternal->m_pCustomErrUrls;
        else
        {
            m_pInternal->m_pCustomErrUrls->inherit(
                        m_pParent->m_pInternal->m_pCustomErrUrls );
        }
        if ( m_iConfigBits & BIT_FILES_MATCH )
        {
            ContextList::iterator iter;
            for( iter = m_pInternal->m_pFilesMatchList->begin();
                 iter != m_pInternal->m_pFilesMatchList->end(); ++iter )
            {
                (*iter)->inherit( pRootContext );
            }
            m_pInternal->m_pFilesMatchList->merge(
                    m_pParent->m_pInternal->m_pFilesMatchList, 0 );
        }
        else
            m_pInternal->m_pFilesMatchList = m_pParent->m_pInternal->m_pFilesMatchList;

        if ( !(m_iConfigBits & BIT_EXTRA_HEADER ))
        {
            m_pInternal->m_pExtraHeader = m_pParent->m_pInternal->m_pExtraHeader;
        }
    }

    if ( !(m_iConfigBits & BIT_ENABLE_EXPIRES) )
    {
        m_expires.enable( m_pParent->getExpires().isEnabled() );
    }
    if ( !(m_iConfigBits & BIT_EXPIRES_DEFAULT) )
    {
        m_expires.setBase( m_pParent->getExpires().getBase() );
        m_expires.setAge( m_pParent->getExpires().getAge() );
    }

    if ( !(m_iConfigBits & BIT_SETUID) )
    {
        setConfigBit( BIT_ALLOW_SETUID,
                    m_pParent->m_iConfigBits & BIT_ALLOW_SETUID);
    }
    if ( m_pParent->m_iSetUidMode & CHANG_UID_ONLY )
        m_iSetUidMode |= CHANG_UID_ONLY;
        
    if ( !(m_iConfigBits & BIT_SUEXEC ) )
    {
        m_iSetUidMode = (m_iSetUidMode & ~UID_MASK)|
                        (m_pParent->m_iSetUidMode & UID_MASK);
    }
    if ( !(m_iConfigBits & BIT_CHROOT ) )
    {
        m_iSetUidMode = (m_iSetUidMode & ~CHROOT_MASK) |
                        ( m_pParent->m_iSetUidMode & CHROOT_MASK);
    }
    if ( !(m_iConfigBits & BIT_GEO_IP ) )
    {
        m_iSetUidMode = (m_iSetUidMode & ~CTX_GEOIP_ON) |
                        ( m_pParent->m_iSetUidMode & CTX_GEOIP_ON);
    }
    if ( !(m_iConfigBits & BIT_ENABLE_SCRIPT ) )
    {
        m_iSetUidMode = (m_iSetUidMode & ~ENABLE_SCRIPT) |
                        ( m_pParent->m_iSetUidMode & ENABLE_SCRIPT);
    }
    
    if ( !(m_iConfigBits & BIT_REWRITE_ENGINE) )
    {
        m_iRewriteEtag = (m_iRewriteEtag & ~REWRITE_MASK) |
                    (( m_pParent->m_iRewriteEtag | REWRITE_INHERIT ) & REWRITE_MASK);
    }
    if ( !(m_iConfigBits2 & BIT_FILES_ETAG ) )
    {
        m_iRewriteEtag = (m_iRewriteEtag & ~ETAG_MASK) |
                    ( m_pParent->m_iRewriteEtag  & ETAG_MASK);
    }
    else
    {
        if ( m_iRewriteEtag & ETAG_MOD_ALL )
        {
            int tag = m_pParent->m_iRewriteEtag & ETAG_ALL;
            int mask = (m_iRewriteEtag & ETAG_MOD_ALL) >> 3;
            int val = ( m_iRewriteEtag & ETAG_ALL ) & mask;
            m_iRewriteEtag = (m_iRewriteEtag & ~ETAG_MASK)
                    |((tag & ~mask ) | val )|(m_iRewriteEtag & ETAG_MOD_ALL);
        }
    }
    
    
    
    if ( m_iConfigBits & BIT_REWRITE_INHERIT )
    {
        RewriteRuleList * pList =
                getValidRewriteRules( m_pParent );
        if ( !(m_iConfigBits & BIT_REWRITE_RULE ))
            m_pRewriteRules = pList;

        //FIXME: append rules from parent
    }
    
    if ( !(m_iConfigBits & BIT_SATISFY ) )
    {
        setConfigBit( BIT_SATISFY_ANY, m_pParent->isSatisfyAny() );
    }

    if ( !(m_iConfigBits & BIT_AUTOINDEX ) )
    {
        setConfigBit( BIT_AUTOINDEX_ON, m_pParent->isAutoIndexOn() );
    }

    if ( m_pMatchList )
    {
        ContextList::iterator iter;
        for( iter = m_pMatchList->begin(); iter != m_pMatchList->end(); ++iter )
        {
            (*iter)->inherit( pRootContext );
        }
    }
}

int HttpContext::addMatchContext( HttpContext * pContext )
{
    //assert( !m_iFilesMatchCtx );
    if ( !m_pMatchList )
    {
        m_pMatchList = new ContextList();
        if ( !m_pMatchList )
            return -1;
    }
    if ( m_pMatchList->add( pContext, 1 ) == -1 )
        return -1;
    pContext->setParent( this );
    return 0;
}

const HttpContext * HttpContext::match( const char * pURI, int iURILen,
                  char * pBuf, int &bufLen ) const
{
    //if ( !m_pMatchList || m_iFilesMatchCtx)
    if ( !m_pMatchList )
        return NULL;
    ContextList::iterator iter;
    for( iter = m_pMatchList->begin(); iter != m_pMatchList->end(); ++iter )
    {
        if ( (*iter)->getURIMatch()->match( pURI, iURILen, pBuf, bufLen ) == 0 )
            return *iter;
    }
    return NULL;
}

const HttpContext * HttpContext::findMatchContext( const char * pURI, int useLocation ) const
{
    //if ( !m_pMatchList || m_iFilesMatchCtx)
    if ( !m_pMatchList )
        return NULL;
    ContextList::iterator iter;
    for( iter = m_pMatchList->begin(); iter != m_pMatchList->end(); ++iter )
    {
        const char *p;
        if ( !useLocation )
            p = (*iter)->getURI();
        else
            p = (*iter)->getLocation();
        if ( p &&(strcmp( p, pURI ) == 0 ) )
            return *iter;
    }
    return NULL;
    
}



int HttpContext::addMIME( const char * pValue )
{
    if ( initMIME() )
        return -1;
    return m_pInternal->m_pMIME->addType( HttpGlobals::getMime(), pValue,
                                        m_sLocation.c_str() );

}

int HttpContext::setExpiresByType( const char * pValue )
{
    if ( initMIME() )
        return -1;
    return m_pInternal->m_pMIME->setExpiresByType(pValue, HttpGlobals::getMime(),
                                m_sLocation.c_str() );
}

int HttpContext::setCompressByType( const char * pValue )
{
    if ( initMIME() )
        return -1;
    return m_pInternal->m_pMIME->setCompressableByType( pValue, HttpGlobals::getMime(),
                                    m_sLocation.c_str() );
}

int HttpContext::setForceType( char * pValue, const char * pLogId )
{
    const MIMESetting * pSetting = NULL;
    if ( allocateInternal() )
        return -1;
    if ( strcasecmp( pValue, "none" ) != 0 )
    {
        if ( m_pInternal->m_pMIME )
            pSetting = m_pInternal->m_pMIME->getMIMESetting( pValue );
        if ( !pSetting )
            pSetting = HttpGlobals::getMime()->getMIMESetting( pValue );
        if ( !pSetting )
        {
            LOG_WARN(( "[%s] can't set 'Forced Type', undefined MIME Type %s",
                        pLogId, pValue ));
            return -1;
        }
    }
    m_pInternal->m_pForceType = pSetting;
    setConfigBit( BIT_FORCE_TYPE, 1 );
    return 0;
}

const MIMESetting * HttpContext::determineMime( const char * pSuffix,
                                                char * pForcedType ) const
{
    const MIMESetting *pMimeType = NULL;
    if ( pForcedType )
    {
        if ( m_pInternal->m_pMIME )
            pMimeType = m_pInternal->m_pMIME->getMIMESetting( pForcedType );
        if ( !pMimeType )
            pMimeType = HttpGlobals::getMime()->getMIMESetting( pForcedType );
        if ( pMimeType )
            return pMimeType;
    }
    if ( m_pInternal->m_pForceType )
        return m_pInternal->m_pForceType;
    if ( pSuffix )
    {
		char achSuffix[256];
		int len = 256;
		StringTool::strnlower( pSuffix, achSuffix, len );
        if ( m_pInternal->m_pMIME )
        {
            pMimeType = m_pInternal->m_pMIME->getFileMimeByType( achSuffix );
            if ( pMimeType )
                return pMimeType;
        }
        pMimeType = HttpGlobals::getMime()->getFileMimeByType( achSuffix );
        if ( pMimeType )
            return pMimeType;
    }
    if ( m_pInternal->m_pMIME )
    {
        pMimeType = m_pInternal->m_pMIME->getDefault();
        if ( pMimeType )
            return pMimeType;
    }
    pMimeType = HttpGlobals::getMime()->getDefault();
    return pMimeType;
}


void HttpContext::setRewriteBase( const char * p )
{
    m_pRewriteBase = new AutoStr2( p );
}


void HttpContext::clearDirIndexes()
{
    if ((m_iConfigBits & BIT_DIRINDEX )&&( m_pInternal->m_pIndexList ))
        delete m_pInternal->m_pIndexList;
    if ( m_pInternal )
        m_pInternal->m_pIndexList = NULL;
    m_iConfigBits &= ~BIT_DIRINDEX;
}


void HttpContext::addDirIndexes( const char * pList )
{
    if ( !(m_iConfigBits & BIT_DIRINDEX ))
    {
        m_iConfigBits |= BIT_DIRINDEX;
        if ( allocateInternal() )
            return ;
    }
    if ( strcmp( pList, "-") == 0 )
        return;
    if ( !m_pInternal->m_pIndexList )
    {
        m_pInternal->m_pIndexList = new StringList();
        if (!m_pInternal->m_pIndexList )
            return;
    }
    char * p1 = strdup( pList );
    m_pInternal->m_pIndexList->split( p1, strlen( p1 ) + p1, ", " );
    free( p1 );
}


int  HttpContext::setCustomErrUrls(const char * pStatusCode, const char* url)
{
    int statusCode = atoi( pStatusCode );
    if (!(m_iConfigBits & BIT_ERROR_DOC ))
    {
        if ( allocateInternal() )
            return -1;
        m_pInternal->m_pCustomErrUrls = new StatusUrlMap();
        if ( !m_pInternal->m_pCustomErrUrls )
            return -1;
        m_iConfigBits |= BIT_ERROR_DOC;
    }
    return m_pInternal->m_pCustomErrUrls->setStatusUrlMap( statusCode, url );
}


const AutoStr2 * HttpContext::getErrDocUrl( int statusCode ) const
{   return m_pInternal->m_pCustomErrUrls?
            m_pInternal->m_pCustomErrUrls->getUrl( statusCode ): NULL;    }


void HttpContext::setPHPConfig( PHPConfig * pConfig )
{
    if ( !allocateInternal() )
    {
        m_pInternal->m_pPHPConfig = pConfig;
        m_iConfigBits |= BIT_PHPCONFIG;
    }
}

void HttpContext::getAAAData( struct AAAData & data ) const
{
    memmove( &data, &m_pInternal->m_pHTAuth, sizeof( AAAData ) );
}

void HttpContext::setWebSockAddr(GSockAddr &gsockAddr)
{
    if ( !allocateInternal() )
    {
        m_pInternal->m_GSockAddr = gsockAddr;
        m_iConfigBits |= BIT_GSOCKADDR;
    }
}

