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
#ifndef MAINSERVERCONFIG_H
#define MAINSERVERCONFIG_H
#include <util/autostr.h>
#include <util/tsingleton.h>
class MainServerConfig: public TSingleton<MainServerConfig>
{
    friend class TSingleton<MainServerConfig>;
private:

    AutoStr         m_sServerName;
    AutoStr         m_sAdminEmails;
    AutoStr         m_gdbPath;
    AutoStr         m_sServerRoot;
    AutoStr         m_sGroup;
    AutoStr         m_sUser;
    AutoStr         m_sAutoIndexURI;
    AutoStr2        m_sChroot;
    int             m_iCrashGuard;
    int             m_iEnableCoreDump;    
    MainServerConfig()
        : m_sGroup( "nobody" )
        , m_sUser( "nobody" )
        , m_sAutoIndexURI( "/_autoindex/default.php" )
        , m_iCrashGuard( 2 )
        , m_iEnableCoreDump( 0 )
    {}
public:
    int  getCrashGuard() const          {   return m_iCrashGuard;   }
    void setCrashGuard( int guard )     {   m_iCrashGuard = guard;  }   
    void setServerName( const char * pServerName )  {   m_sServerName = pServerName;}
    void setAdminEmails( const char * pEmails)      {   m_sAdminEmails = pEmails;   }
    void setGDBPath( const char * pGdbPath)         {   m_gdbPath = pGdbPath;       }
    void setServerRoot( const char * pServerRoot )  {   m_sServerRoot = pServerRoot;}
    void setChroot( const char * pChroot )          {   m_sChroot = pChroot;        }
    void setGroup( const char * pGroup )            {   m_sGroup = pGroup;          }
    void setUser( const char * pUser )              {   m_sUser = pUser;            }
    void setAutoIndexURI(const char* pAutoIndexURI) {   m_sAutoIndexURI = pAutoIndexURI;}
    void setEnableCoreDump(int iEnableCoreDump)     {  m_iEnableCoreDump = iEnableCoreDump; } 
    const char * getServerName()    const   {   return m_sServerName.c_str();   }
    const char * getAdminEmails()   const   {   return m_sAdminEmails.c_str();  }
    const char * getGDBPath()       const   {   return m_gdbPath.c_str();       }
    const char * getServerRoot()    const   {   return m_sServerRoot.c_str();   }
    const char * getChroot()        const   {   return m_sChroot.c_str();       }
    const AutoStr2* getpsChroot()   const   {   return &m_sChroot;              }
    int    getChrootlen()     const         {   return m_sChroot.len();         }
    const char * getGroup()         const   {   return m_sGroup.c_str();        }
    const char * getUser()          const   {   return m_sUser.c_str();         }
    const char * getAutoIndexURI()  const   {   return m_sAutoIndexURI.c_str(); }
    int  getEnableCoreDump()  const         {   return m_iEnableCoreDump;       }
    
};

#endif // MAINSERVERCONFIG_H
