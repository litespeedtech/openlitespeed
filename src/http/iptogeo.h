/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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

#include <config.h>
#ifdef ENABLE_IPTOGEO


#include <inttypes.h>

#include <lsdef.h>
#include <http/ip2geo.h>
#include "GeoIP.h"
#include "GeoIPCity.h"

//class HttpReq;
class IEnv;
class IpToGeo;
class XmlNodeList;
class ConfigCtx;

class GeoIpData : public GeoInfo
{
    friend class IpToGeo;
public:
    GeoIpData();
    ~GeoIpData();
    const char *getGeoEnv(const char *pEnvName);
    int addGeoEnv(IEnv *pEnv);
    //void addGeoEnv( HttpReq * pReq );
    void reset();
    void release();

private:

    int          m_netspeed;
    int          m_countryId;

    GeoIPRegion *m_pRegion;
    GeoIPRecord *m_pCity;

    char *m_pOrg;
    char *m_pIsp;
    LS_NO_COPY_ASSIGN(GeoIpData);
};

class IpToGeo : public Ip2Geo
{

public:
    IpToGeo();
    ~IpToGeo();

    int setGeoIpDbFile(const char *pFile, const char *cacheMode);

    GeoInfo *lookUp(uint32_t addr);
    GeoInfo *lookUpV6(in6_addr addr);
    int config(const XmlNodeList *pList);

private:
    int loadGeoIpDbFile(const char *pFile, int flag);
    int testGeoIpDbFile(const char *pFile, int flag);

    int     m_locDbType;
    GeoIP *m_pLocation;
    GeoIP *m_pOrg;
    GeoIP *m_pIsp;
    GeoIP *m_pNetspeed;
    LS_NO_COPY_ASSIGN(IpToGeo);
};

#endif  //ENABLE_IPTOGEO
#endif
