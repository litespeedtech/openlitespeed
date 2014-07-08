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
#ifndef HTTPGLOBALS_H
#define HTTPGLOBALS_H


#include <sys/types.h>
#include <http/reqstats.h>
#include <util/tlinklist.h>
#include <../addon/include/ls.h>


class AccessControl;
class AutoStr2;
class ClientCache;
class ConnLimitCtrl;
class DeniedDir;
class EventDispatcher;
class HttpSession;
class HttpMime;
class HttpResourceManager;
class Multiplexer;
class RewriteEngine;
class ServerInfo;
class StaticFileCache;
class StdErrLogger;
class SUExec;
class CgidWorker;
class HttpVHost;
class HttpConfigLoader;

#ifdef USE_UDNS
class Adns;
#endif
class IpToGeo;
#define G_BUF_SIZE 16384
#define TIMER_PRECISION 10

class ModuleTimer : public LinkedObj
{
public:
    int                     m_iId;
    time_t                  m_tmExpire;
    time_t                  m_tmExpireUs;
    lsi_timer_callback_pf   m_TimerCb;
    void*                   m_pTimerCbParam;
};


class HttpGlobals
{
    //static EventDispatcher      * s_pDispatcher;
    static Multiplexer          * s_pMultiplexer;
    static AccessControl        * s_pAccessCtrl;
    static HttpMime             * s_pMime;
    static DeniedDir              s_deniedDir;
    static ConnLimitCtrl          s_connLimitCtrl;
    static ClientCache          * s_pClients;
    static HttpResourceManager    s_ResManager;
    static StaticFileCache        s_staticFileCache;
    static StdErrLogger           s_stdErrLogger;
    static ServerInfo           * s_pServerInfo;

    HttpGlobals() {};
    ~HttpGlobals() {};
public:
    static int                    s_iMultiplexerType;
    static int                    s_iConnsPerClientSoftLimit;
    static int                    s_iOverLimitGracePeriod;
    static int                    s_iConnsPerClientHardLimit;
    static int                    s_iBanPeriod;
    static uid_t                  s_uid;
    static gid_t                  s_gid;
    static uid_t                  s_uidMin;
    static gid_t                  s_gidMin;
    static uid_t                  s_ForceGid;
    static int                    s_priority;
    static pid_t                  s_pidCgid;
    static int                    s_umask;
    static AutoStr2             * s_psChroot;
    static const char           * s_pServerRoot;
    static const char           * s_pAdminSock;
    static SUExec               * s_pSUExec;
    static CgidWorker           * s_pCgid;
    
    static int                    s_iProcNo;
    static long                   s_lBytesRead;
    static long                   s_lBytesWritten;
    static long                   s_lSSLBytesRead;
    static long                   s_lSSLBytesWritten;
    static int                    s_iIdleConns;
    static ReqStats               s_reqStats;
    static int                    s_children;
    static int                    s_503Errors;
    static int                    s_503AutoFix;
    static int                    s_useProxyHeader;

    static int                    s_tmPrevToken;
    static int                    s_tmToken;
    static int                    s_dnsLookup;

    static RewriteEngine          s_RewriteEngine;
    static IpToGeo *              s_pIpToGeo;
    static HttpVHost *            s_pGlobalVHost;

    static int                    s_rubyProcLimit;
    static int                    s_railsAppLimit;

    
    //module timer linklist
    static TLinkList<ModuleTimer>    s_ModuleTimerList;
    
   
#ifdef USE_UDNS
    static Adns                   s_adns;
#endif

    static char g_achBuf[G_BUF_SIZE + 8];
    //static void setDispatcher( EventDispatcher * pDispatcher )
    //{   s_pDispatcher = pDispatcher;    }
    static void setMultiplexer( Multiplexer * pMultiplexer )
    {   s_pMultiplexer = pMultiplexer;  }
    static void setAccessControl( AccessControl * pControl )
    {   s_pAccessCtrl = pControl;   }
    static void setHttpMime( HttpMime * pMime )
    {   s_pMime = pMime;            }
        
    
    //static EventDispatcher * getDispatcher()
    //{   return s_pDispatcher;   }
    static Multiplexer * getMultiplexer()
    {   return s_pMultiplexer;      }
    static HttpResourceManager * getResManager()
    {   return &s_ResManager;       }
    static AccessControl * getAccessCtrl()
    {   return s_pAccessCtrl;       }
    static ClientCache * getClientCache()
    {   return s_pClients;          }
    static void setClientCache( ClientCache * p )
    {   s_pClients = p;      }
    
    static StaticFileCache *getStaticFileCache()
    {   return &s_staticFileCache;  }
    static HttpMime * getMime()
    {   return s_pMime;             }
    static ConnLimitCtrl* getConnLimitCtrl()
    {   return &s_connLimitCtrl;    }

    static DeniedDir* getDeniedDir()
    {   return &s_deniedDir;        }

    static StdErrLogger* getStdErrLogger()
    {   return &s_stdErrLogger;     }
    
    static ServerInfo * getServerInfo()
    {   return s_pServerInfo;        }
    static void setServerInfo( ServerInfo * pInfo )
    {   s_pServerInfo = pInfo;      }
    
    static const char * getCgidSecret();
    
};

#endif
