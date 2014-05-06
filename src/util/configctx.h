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
#ifndef CONFIGCTX_H
#define CONFIGCTX_H

#include <util/ssnprintf.h>
#include <util/logidtracker.h>

//class HttpContext;
//class HttpVHost;
class XmlNode;
class HttpHandler;
#define MAX_PATH_LEN                4096
long long getLongValue( const char *pValue, int base = 10 );
class ConfigCtx
{
public:
    
    explicit ConfigCtx( const char * pAppendId1 = NULL, const char * pAppendId2 = NULL )
        : m_pParent( s_pCurConfigCtx )
    {
        s_pCurConfigCtx = this;
        if ( pAppendId1 )
        {
            m_logIdTracker.appendLogId( ":" );
            m_logIdTracker.appendLogId( pAppendId1 );
        }
        if ( pAppendId2 )
        {
            m_logIdTracker.appendLogId( ":" );
            m_logIdTracker.appendLogId( pAppendId2 );
        }
    }
    ~ConfigCtx()    
    {
        s_pCurConfigCtx = m_pParent; 
    }

    void vlog( int level, const char * pFmt, va_list args );
    void log_error( const char * pFmt, ... );
    void log_warn( const char * pFmt, ... );
    void log_notice( const char * pFmt, ... );
    void log_info( const char * pFmt, ... );
    void log_debug( const char * pFmt, ... );
    void log_errorPath( const char *pstr1,  const char *pstr2 );
    void log_errorInvalTag( const char *pstr1,  const char *pstr2);
    void log_errorMissingTag( const char *pstr1);
    const char * getTag( const XmlNode * pNode, const char * pName, int bKeyName = 0 );
    long long getLongValue( const XmlNode * pNode, const char * pTag,
            long long min, long long max, long long def, int base = 10 );
    int getRootPath ( const char *&pRoot, const char *&pFile );
    int expandVariable( const char * pValue, char * pBuf, int bufLen,
                        int allVariable = 0 );
    int getAbsolute( char * dest, const char * path, int pathOnly );
    int getAbsoluteFile(char* dest, const char* file);
    int getAbsolutePath(char* dest, const char* path);    
    int getLogFilePath( char * pBuf, const XmlNode * pNode );
    int getValidFile(char * dest, const char * file, const char * desc );
    int getValidPath(char * dest, const char * path, const char * desc );
    int getValidChrootPath(const char * path, const char * desc );
    char * getExpandedTag( const XmlNode * pNode,
                    const char * pName, char *pBuf, int bufLen, int bKeyName = 0 );    
    int expandDomainNames( const char *pDomainNames, 
                    char * achDomains, int len, char dilemma = ',' );  
    int checkAccess( char *pReal);
    int checkPath( char *pPath, const char *desc, int follow); 
    int convertToRegex( const char   *pOrg, char *pDestBuf, int bufLen );
    XmlNode *parseFile( const char *configFilePath, const char *rootTag );
    static const AutoStr2 * getVhName()               {   return &s_sVhName;          }
    static const AutoStr2 * getVhDomain()             {   return &s_sVhDomain;         }
    static const AutoStr2 * getVhAliases()            {   return &s_sVhAliases;        }
    static void setVhName(const char* pVhName)             { s_sVhName.setStr( pVhName );  }
    static void setVhDomain(const char* pvhDomain)         { s_sVhDomain.setStr( pvhDomain );  }
    static void setVhAliases(const char* pvhAliases)       { s_sVhDomain.setStr( pvhAliases );  }
    static const char * getVhRoot()         {   return s_achVhRoot;     }
    static void clearVhRoot()               {   s_achVhRoot[0] = 0;     }
    static ConfigCtx*   getCurConfigCtx()   {   return s_pCurConfigCtx; }
    //static void setCurConfigCtx( ConfigCtx* pConfigCtx ) { s_pCurConfigCtx = pConfigCtx; }
    static void clearDocRoot()              {   s_achDocRoot[0] = 0;    }
    static void setDocRoot(const char* pDocRoot)    
        { memcpy(s_achDocRoot, pDocRoot, strlen(pDocRoot) + 1); }
    static const char* getDocRoot()         {   return s_achDocRoot;    }
private:
    LogIdTracker    m_logIdTracker;
    ConfigCtx     * m_pParent;
    static AutoStr2        s_sVhName;
    static AutoStr2        s_sVhDomain;
    static AutoStr2        s_sVhAliases;
    static char            s_achVhRoot[MAX_PATH_LEN];
    static ConfigCtx*      s_pCurConfigCtx;
    static char            s_achDocRoot[MAX_PATH_LEN];
};

#endif // CONFIGCTX_H
