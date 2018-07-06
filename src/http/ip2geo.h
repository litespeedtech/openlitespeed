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
#ifndef IP2GEO_H
#define IP2GEO_H

#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <lsdef.h>
#include <util/ienv.h>
#include <util/xmlnode.h>

class Ip2Geo;

class GeoInfo
{
public:
    GeoInfo()  { }
    virtual ~GeoInfo() { };
    virtual const char *getGeoEnv(const char *pEnvName) = 0;
    virtual int addGeoEnv(IEnv *pEnv) = 0;
    
private:
    LS_NO_COPY_ASSIGN(GeoInfo);
};

class Ip2Geo
{
public:
    // Call the constructor AFTER having configured everything
    Ip2Geo() { }
    virtual ~Ip2Geo() { };
    
    virtual int config(const XmlNodeList *pList) = 0;

    // Called only after config and setEnv
    // Note that GeoInfo is allocated and returned from a successful lookup
    virtual GeoInfo *lookUp(uint32_t addr) = 0;
    virtual GeoInfo *lookUpV6(in6_addr addr) = 0;
    
private:    
    LS_NO_COPY_ASSIGN(Ip2Geo);
};

#endif
