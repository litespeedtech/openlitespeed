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
#ifndef IPTOGEO_H
#define IPTOGEO_H


#include <inttypes.h>

#include "GeoIP.h"
#include "GeoIPCity.h"

//class HttpReq;
class IEnv;
class IpToGeo;
class XmlNodeList;
class ConfigCtx;

class GeoInfo
{
    friend class IpToGeo;
public:
    GeoInfo();
    ~GeoInfo();
    const char * getGeoEnv( const char * pEnvName );
    int addGeoEnv( IEnv * pEnv );
    //void addGeoEnv( HttpReq * pReq );
    void reset();
    void release();

private:

    int          m_netspeed;
    int          m_countryId;

    GeoIPRegion* m_pRegion;
    GeoIPRecord* m_pCity;

    char * m_pOrg;
    char * m_pIsp;
};
  
class IpToGeo
{

public: 
    IpToGeo();
    ~IpToGeo();

    int setGeoIpDbFile( const char * pFile, const char * cacheMode  );
    void setUseProxyHeaders( int i )    {   m_useProxyHeaders = i;      }
    int getUseProxyHeaders() const      {   return m_useProxyHeaders;   }
    
    int lookUp( uint32_t addr, GeoInfo * pInfo );
    int lookUp( const char * pIP, GeoInfo * pInfo );
    int lookUpV6( in6_addr addr, GeoInfo * pInfo );
    int config( const XmlNodeList *pList );
    
private:

    int     m_locDbType;
    int     m_useProxyHeaders;
    GeoIP * m_pLocation;
    GeoIP * m_pOrg;
    GeoIP * m_pIsp;
    GeoIP * m_pNetspeed;
    
};

#endif
