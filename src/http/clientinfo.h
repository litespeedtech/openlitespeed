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
#ifndef CLIENTINFO_H
#define CLIENTINFO_H


#include <http/throttlecontrol.h>
#include <lsiapi/lsimoduledata.h>
#include <util/autostr.h>

struct sockaddr;
class GeoInfo;
class ClientInfo
{
    char        m_achSockAddr[24];
    AutoStr2    m_sAddr;
    AutoStr2    m_sHostName;
    GeoInfo   * m_pGeoInfo;
    LsiModuleData   m_moduleData;
    
    size_t      m_iConns;
    time_t      m_tmOverLimit;
    short       m_sslNewConn;
    int         m_iHits;
    time_t      m_lastConnect;
    int         m_iAccess;
    
    ThrottleControl     m_ctlThrottle;
    //time_t
    //int       m_iBadReqs;
    //int       m_iGoodReqs;
    //int       m_iTimeoutConn;
    //int       m_iDynReqs;
    //int       m_iBytesReceived;
    //int       m_iBytesSent;
    //int       m_iExcessiveConnAttempts;
                        
public: 
    ClientInfo();
    ~ClientInfo();
    const struct sockaddr * getAddr() const
    {   return (struct sockaddr *)m_achSockAddr;         }

    void setAddr( const struct sockaddr * pAddr );
        
    const char * getAddrString() const  {   return m_sAddr.c_str();     }
    int          getAddrStrLen() const  {   return m_sAddr.len();       }

    void setHostName( const char * p )
    {   if ( p ) m_sHostName.setStr( p );
        else     m_sHostName.setStr( "", 0 );
    }
    const char * getHostName() const    {   return m_sHostName.c_str(); }
    int getHostNameLen() const          {   return m_sHostName.len();   }
    
    size_t incConn()                    {   return ++m_iConns;      }
    size_t decConn()                    {   return --m_iConns;      }
    size_t getConns() const             {   return m_iConns;        }

    void hit( time_t t)                 {   ++m_iHits; m_lastConnect = t;    }
    void resetHits()                    {   m_iHits = 0;            }
    int  getHits() const                {   return m_iHits;         }

    void setLastConnTime( time_t t )    {   m_lastConnect = t ;     }
    time_t getLastConnTime() const      {   return m_lastConnect;   }

    void setOverLimitTime( time_t t )   {   m_tmOverLimit = t ;     }
    time_t getOverLimitTime() const     {   return m_tmOverLimit;   }
    
    void setAccess( int access )        {   m_iAccess = access;     }
    int getAccess() const               {   return m_iAccess;       }

    int checkAccess();
    
    ThrottleControl& getThrottleCtrl()  {   return m_ctlThrottle;   }
    
    bool allowRead() const      {   return m_ctlThrottle.allowRead();   }
    bool allowWrite() const     {   return m_ctlThrottle.allowWrite();  }

    short incSslNewConn()   {   return ++m_sslNewConn;  }
    short getSslNewConn()   {   return m_sslNewConn;    }
    void  setSslNewConn( int n)   {   m_sslNewConn = n;    }
    GeoInfo * allocateGeoInfo();
    GeoInfo * getGeoInfo() const        {   return m_pGeoInfo;      }
    
    LsiModuleData* getModuleData()      {   return &m_moduleData;   }
};

#endif
