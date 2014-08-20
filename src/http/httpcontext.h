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
#ifndef HTTPCONTEXT_H
#define HTTPCONTEXT_H


#include <util/autostr.h>
#include <http/expiresctrl.h>
#include <http/httphandler.h>
#include "socket/gsockaddr.h"
#include <stddef.h>

class AccessControl;
class AuthRequired;
class ContextList;
class HTAuth;
//class HTACache;
class HttpMime;
class MIMESetting;
class PHPConfig;
class ReqHandler;
class RewriteRuleList;
class StringList;
class URIMatch;
class StatusUrlMap;
class AutoBuf;
class SSIConfig;

#define UID_SERVER          0
#define UID_FILE            1
#define UID_DOCROOT         2

#define UID_MASK            3

#define CHROOT_SERVER       0
#define CHROOT_VHROOT       4
#define CHROOT_PATH         8

#define CHROOT_MASK         12

#define CTX_GEOIP_ON        16
#define ENABLE_SCRIPT       32
#define CHANG_UID_ONLY      64
#define USE_CANONICAL       128

#define BIT_FORCE_TYPE      (1<<0)
#define BIT_AUTH            (1<<1)
#define BIT_ACCESS          (1<<2)
#define BIT_ALLOW_OVERRIDE  (1<<3)
#define BIT_ENABLE_EXPIRES  (1<<4)
#define BIT_EXPIRES_DEFAULT (1<<5)
#define BIT_ERROR_DOC       (1<<6)
#define BIT_SUEXEC          (1<<7)
#define BIT_CHROOT          (1<<8)
#define BIT_SETUID          (1<<9)
#define BIT_ALLOW_SETUID    (1<<10)
#define BIT_DEF_CHARSET     (1<<11)
#define BIT_MIME            (1<<12)
#define BIT_REWRITE_ENGINE  (1<<13)
#define BIT_REWRITE_RULE    (1<<14)
#define BIT_REWRITE_INHERIT (1<<15)
#define BIT_SATISFY         (1<<16)
#define BIT_SATISFY_ANY     (1<<17)
#define BIT_AUTOINDEX       (1<<18)
#define BIT_AUTOINDEX_ON    (1<<19)
#define BIT_AUTHORIZER      (1<<20)
#define BIT_DIRINDEX        (1<<21)
#define BIT_PHPCONFIG       (1<<22)
#define BIT_CTXINT          (1<<23)
#define BIT_AUTH_REQ        (1<<24)
#define BIT_FILES_MATCH     (1<<25)
#define BIT_EXTRA_HEADER    (1<<26)
#define BIT_ENABLE_SCRIPT   (1<<27)
#define BIT_GEO_IP          (1<<28)
#define BIT_GSOCKADDR       (1<<29)


#define REWRITE_OFF         0
#define REWRITE_ON          1
#define REWRITE_INHERIT     2
#define REWRITE_MASK        3

#define ETAG_NONE           0
#define ETAG_INODE          4
#define ETAG_MTIME          8
#define ETAG_SIZE           16
#define ETAG_ALL            (ETAG_INODE|ETAG_MTIME|ETAG_SIZE)
#define ETAG_MOD_INODE      32
#define ETAG_MOD_MTIME      64
#define ETAG_MOD_SIZE       128
#define ETAG_MOD_ALL        (ETAG_MOD_INODE|ETAG_MOD_MTIME|ETAG_MOD_SIZE)
#define ETAG_MASK           (ETAG_ALL|ETAG_MOD_ALL)
#define BIT_FILES_ETAG          (1<<12)

#define BIT_ALLOW_BROWSE        (1<<0)
#define BIT_INCLUDES            (1<<4)
#define BIT_INCLUDES_NOEXEC     (1<<5)
#define BIT_XBIT_HACK_FULL      (1<<7)
#define BIT_URI_CACHEABLE       (1<<8)

#define BIT_RAILS_CONTEXT       (1<<6)


/**********************************************************************
*   m_sContextURI: is the root URI the context starts
*
* Type of Http Contexts are
*   STATIC FILE:
*     m_sRoot is the root directory of the static file
*     m_iHandlerType is HT_STATIC
*
*   CGI:
*     m_sRoot is the root directory of the CGI
*     m_iHandlerType is HT_CGI
*   FCGI:
*     m_sRoot   is the URI of the Fast CGI, the fast CGI application
*               can be accessed through a TCP port or unix domain socket
*               if TCP, the URI is in format "['TCP:']<IPorHostname>:<Port>",
*               if domain socket, the format is "['UDS:']<filepath>"
*
*
***********************************************************************/

struct AAAData
{
    const HTAuth        * m_pHTAuth;
    const AuthRequired  * m_pRequired;
    const AccessControl * m_pAccessCtrl;
    const HttpHandler   * m_pAuthorizer;
};

typedef struct _CTX_INT
{
    const HTAuth        * m_pHTAuth;
    AuthRequired        * m_pRequired;
    AccessControl       * m_pAccessCtrl;
    const HttpHandler   * m_pAuthorizer;
    StringList          * m_pIndexList;
    AutoStr2            * m_pDefaultCharset;
    const MIMESetting   * m_pForceType;
    HttpMime            * m_pMIME;
    PHPConfig           * m_pPHPConfig;
    StatusUrlMap        * m_pCustomErrUrls;
    ContextList         * m_pFilesMatchList;
    AutoBuf             * m_pExtraHeader;
    SSIConfig           * m_pSSIConfig;
    GSockAddr             m_GSockAddr;
} CtxInt;

class HttpContext
{
    static CtxInt s_defaultInternal;
    
    AutoStr2            m_sContextURI;
    AutoStr2            m_sLocation;
    int                 m_iConfigBits;
    
    URIMatch            * m_pURIMatch;

    ContextList     * m_pMatchList;
    StringList      * m_pFilesMatchStr;

    const HttpHandler   * m_pHandler;
    CtxInt              * m_pInternal;
    ExpiresCtrl           m_expires;
    

    char                m_bAllowBrowse;
    char                m_bAllowOverride;
    char                m_redirectCode;
    char                m_iSetUidMode;
    short               m_iConfigBits2;
    unsigned char       m_iRewriteEtag;
    char                m_iDummy;
    long                m_lHTALastMod;
    AutoStr2            * m_pRewriteBase;
    RewriteRuleList     * m_pRewriteRules;
    const HttpContext   * m_pParent;

    void releaseHTAuth();
    void releaseAccessControl();
    void releaseDefaultCharset();
    void releaseMIME();


    HttpContext( const HttpContext& rhs ) {}
    void operator=( const HttpContext& rhs ) {}


public:
    HttpContext();
    ~HttpContext();

    int set( const char * pURI, const char * pRoot,
             const HttpHandler * pHandler, bool allowBrowse = true, int regex=0);

    int setURIMatch( const char * pRegex, const char * pSubst );

    int setFilesMatch( const char * pURI, int regex );
    int matchFiles( const char * pFile, int len ) const;
    const HttpContext * matchFilesContext( const char * pURI,
                    int iURILen ) const;
    int shouldMatchFiles() const
    {   return (m_pInternal->m_pFilesMatchList != NULL);   }
    
    const AutoStr2 * getContextURI() const {   return &m_sContextURI;   }
    const char * getURI() const     {   return m_sContextURI.c_str();   }
    int getURILen() const           {   return m_sContextURI.len();     }

    int  allocateInternal();
    void releaseHTAConf();

    void setCacheable( int c )      {   setConfigBit2( BIT_URI_CACHEABLE, c );  }
    short isCacheable() const       {   return m_iConfigBits2 & BIT_URI_CACHEABLE; }
    
    const char * getLocation() const {   return m_sLocation.c_str();    }
    int getLocationLen() const       {   return m_sLocation.len();      }

    const AutoStr2 * getRoot() const {   return &m_sLocation;           }
    void setRoot( const char * pRoot );

    int getHandlerType() const      {   return m_pHandler->getHandlerType();  }
    void setHandler( const HttpHandler * p )    {   m_pHandler = p;     }
    
    const HttpHandler * getHandler() const      {   return m_pHandler;  }

    char allowBrowse() const            {   return (m_bAllowBrowse & BIT_ALLOW_BROWSE);      }
    void allowBrowse( int browse )      
    {
        if(browse)
            m_bAllowBrowse |= BIT_ALLOW_BROWSE;
        else
            m_bAllowBrowse &= (~BIT_ALLOW_BROWSE);
    }
     
    ExpiresCtrl& getExpires()               {   return m_expires;   }
    const ExpiresCtrl& getExpires() const   {   return m_expires;   }
    
    //int match( const char * pURI ) const
    //{   return (strncmp( m_sContextURI, pURI, m_sContextURI.len() ) == 0 )
    //     ? m_sContextURI.len() : 0;   }

    void setHTAuth(HTAuth* pHTAuth);
    const HTAuth * getHTAuth() const    {   return m_pInternal->m_pHTAuth;   }

    int  setAuthRequired( const char * pRequired );
    const AuthRequired * getAuthRequired() const
    {   return m_pInternal->m_pRequired;     }
    
    int   setAccessControl( AccessControl * pControl);
    AccessControl * getAccessControl() const
    {   return m_pInternal->m_pAccessCtrl;   }

    int addAccessRule( const char * pRule, int allow );

    void redirectCode( int code )   {   m_redirectCode = code;  }
    char redirectCode() const       {   return m_redirectCode;  }
    
    
    long getLastMod() const         {   return m_lHTALastMod;   }
    void setLastMod( long mod )     {   m_lHTALastMod = mod;    }
        
    void setParent( const HttpContext * pParent )
    {   m_pParent = pParent;    }
    
    const HttpContext* getParent() const {   return m_pParent;      }

    void setConfigBit( int bit, int enable )
    {   m_iConfigBits = (m_iConfigBits & (~bit) ) |((enable)? bit : 0); }
    int  getConfigBits() const      {   return m_iConfigBits;   }
    void clearConfigBit()           {   m_iConfigBits = 0;      }
    
    void inherit(const HttpContext * pRootContext);

    HttpMime * getMIME()            {   return m_pInternal->m_pMIME;}
    const HttpMime * getMIME() const{   return m_pInternal->m_pMIME;}
    int initMIME();
    int addMIME( const char * pValue );
    int setExpiresByType( const char * pValue );
    int setCompressByType( const char * pValue );

    const MIMESetting * getForceType() const
    {   return m_pInternal->m_pForceType;    }
    int setForceType( char * pValue, const char * pLogId );
    
    URIMatch *    getURIMatch()  const  {   return m_pURIMatch;     }
    ContextList * getMatchList() const  {   return m_pMatchList;    }

    int addMatchContext( HttpContext * pContext );
    const HttpContext * match( const char * pURI, int iURILen,
                  char * pBuf, int &bufLen ) const;
    const HttpContext * findMatchContext( const char * pURI, int useLocation = 0 ) const;

    const AutoStr2 * getDefaultCharset() const
    //{   return m_pInternal->m_pDefaultCharset;      }
    {   return (m_pInternal)?m_pInternal->m_pDefaultCharset:NULL;      }
    void setDefaultCharset( const char * pCharset );
    void setDefaultCharsetOn();
    int allowSetUID() const
    {   return m_iConfigBits & BIT_ALLOW_SETUID;    }

    void setUidMode( int a )
    {   m_iSetUidMode = (m_iSetUidMode & ~UID_MASK)| (a & UID_MASK);
        m_iConfigBits |= BIT_SUEXEC;                                            }
    char getSetUidMode() const      {   return m_iSetUidMode & UID_MASK;        }

    void setChrootMode( int a )
    {   m_iSetUidMode = (m_iSetUidMode & ~CHROOT_MASK) | (a & CHROOT_MASK);
        m_iConfigBits |= BIT_CHROOT;                                            }
    char getChrootMode() const      {   return m_iSetUidMode & CHROOT_MASK;     }

    void setChangeUidOnly()         {   m_iSetUidMode |= CHANG_UID_ONLY;        }
    char changeUidOnly() const      {   return m_iSetUidMode & CHANG_UID_ONLY;  }
    
    void enableScript( int a )
    {   m_iSetUidMode = (m_iSetUidMode & ~ENABLE_SCRIPT)|( (a)?ENABLE_SCRIPT:0 ); 
        m_iConfigBits |= BIT_ENABLE_SCRIPT;     }
    int  isScriptEnabled() const    {   return m_iSetUidMode & ENABLE_SCRIPT;   }
    
    const MIMESetting * determineMime( const char * pSuffix,
                                        char * pMimeType ) const;

    void setRailsContext()          {   setConfigBit2( BIT_RAILS_CONTEXT, 1 );   }
    char isRailsContext() const     {   return m_iConfigBits2 & BIT_RAILS_CONTEXT;   }

    const AutoStr2 * getRewriteBase() const
    {   return (m_pRewriteBase)?m_pRewriteBase:&m_sContextURI;  }
    void setRewriteBase( const char * p );
    
    void enableRewrite( int a )     {   m_iRewriteEtag = 
                                ( m_iRewriteEtag & ~REWRITE_MASK )|(a & REWRITE_MASK);
                                        m_iConfigBits |= BIT_REWRITE_ENGINE;    }
    unsigned char rewriteEnabled() const     {   return m_iRewriteEtag & REWRITE_MASK;    }
    void setRewriteInherit( int a ) {   setConfigBit( BIT_REWRITE_INHERIT, a ); }
    int  isRewriteInherit() const   {   return m_iConfigBits & BIT_REWRITE_INHERIT; }
    
    RewriteRuleList * getRewriteRules() const
    {   return m_pRewriteRules;                     }
    void setRewriteRules( RewriteRuleList * pList )
    {   m_pRewriteRules = pList;
        m_iConfigBits |= BIT_REWRITE_RULE;          }

    PHPConfig * getPHPConfig() const
    {   return m_pInternal->m_pPHPConfig;    }
    //{   return m_pInternal?m_pInternal->m_pPHPConfig : NULL;    }
    void setPHPConfig( PHPConfig * pConfig );
        
    void setSatisfyAny( int a )
    {   setConfigBit( BIT_SATISFY_ANY, a );
        m_iConfigBits |= BIT_SATISFY;               }
    int isSatisfyAny() const
    {   return m_iConfigBits & BIT_SATISFY_ANY;     }

    void setAutoIndex( int a )
    {   setConfigBit( BIT_AUTOINDEX_ON, a );
        m_iConfigBits |= BIT_AUTOINDEX;             }
        
    int isAutoIndexOn() const
    {   return m_iConfigBits & BIT_AUTOINDEX_ON;    }
    
    int setAuthorizer( const HttpHandler * pHandler );
    const HttpHandler * getAuthorizer() const
    {   return m_pInternal->m_pAuthorizer;          }
    
    void clearDirIndexes();
    void addDirIndexes( const char * pList );

    const StringList * getIndexFileList() const
    {   return m_pInternal->m_pIndexList;           }
    
    StringList * getIndexFileList()
    {   return m_pInternal->m_pIndexList;           }

    int  setCustomErrUrls(const char * pStatusCode, const char* url);
    const AutoStr2 * getErrDocUrl( int statusCode ) const;

    int addFilesMatchContext( HttpContext * pContext );

    void getAAAData( struct AAAData & data ) const;

    //HttpContext * dup() const;

    int setExtraHeaders( const char * pLogId, const char * pHeaders, int len );
    const AutoBuf * getExtraHeaders() const
    {   return m_pInternal->m_pExtraHeader;     }
    
    const GSockAddr *getWebSockAddr() const { return &m_pInternal->m_GSockAddr;   }
    void setWebSockAddr(GSockAddr &gsockAddr);
    
    void setGeoIP( int a )
    {   if ( a )
            m_iSetUidMode |= CTX_GEOIP_ON;
        else
            m_iSetUidMode &= ~CTX_GEOIP_ON;
        m_iConfigBits |= BIT_GEO_IP;
    }
    char isGeoIpOn() const          {   return m_iSetUidMode & CTX_GEOIP_ON;    }    
    SSIConfig * getSSIConfig() const
    {   return m_pInternal->m_pSSIConfig;   }   
    int isXbitHackFull() const
    {   return m_bAllowBrowse & BIT_XBIT_HACK_FULL; }   
    int isIncludesNoExec() const
    {   return (m_bAllowBrowse & (BIT_INCLUDES_NOEXEC | BIT_INCLUDES )) 
                    == BIT_INCLUDES_NOEXEC ;}  
    int hasRewriteConfig() const
    {   return  m_iConfigBits & (BIT_REWRITE_ENGINE | BIT_REWRITE_RULE 
                                | BIT_REWRITE_INHERIT );        }                    

    void setConfigBit2( short bit, int enable )
    {   m_iConfigBits2 = (m_iConfigBits2 & (~bit) ) |((enable)? bit : 0); }
    short getConfigBits2() const     {   return m_iConfigBits2;   }
     

    void setFileEtag( int a )       {   m_iRewriteEtag = 
                                ( m_iRewriteEtag & ~ETAG_MASK )|(a & ETAG_MASK);
                                        m_iConfigBits2 |= BIT_FILES_ETAG;            }     
     unsigned char getFileEtag() const        {   return m_iRewriteEtag & ETAG_MASK;  }
        
    int isIncludesOn() const
    {   return m_bAllowBrowse & (BIT_INCLUDES_NOEXEC | BIT_INCLUDES );       }  
      
};

extern void recycleContext( HttpContext * pContext );

#endif
