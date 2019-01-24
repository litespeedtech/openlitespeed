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
#ifndef IPTOGEO2_H
#define IPTOGEO2_H

#include <config.h>
#ifdef ENABLE_IPTOGEO2

#include <http/ip2geo.h>
#include "maxminddb.h"

//class HttpReq;
class IEnv;
class IpToGeo2;
class XmlNodeList;
class ConfigCtx;

class IpToGeo2;

class GeoIpData2 : public GeoInfo
{
    friend class IpToGeo2;
public:
    GeoIpData2();
    ~GeoIpData2();
    const char *getGeoEnv(const char *pEnvName);
    int addGeoEnv(IEnv *pEnv);
    void reset();
    void release();
    struct env_found_s;
    typedef struct env_found_s env_found_t;
    struct db_found_s;
    typedef struct db_found_s db_found_t;

private:
    IpToGeo2 *m_IpToGeo2;    
    db_found_t *m_db_found;
    int parseEnv();
    int m_did_parse_env;
    int m_tried_parse_env;
    
    char *strutf8(MMDB_entry_data_s *entry_data, char *key);
    char *strint(MMDB_entry_data_s *entry_data, char *key);
    char *strdouble(MMDB_entry_data_s *entry_data, char *key);
    char *strboolean(MMDB_entry_data_s *entry_data, char *key);
    // The 'extract' functions work assuming that a lookup was done
    char *extractString(MMDB_entry_data_s *entry_data);
    char *extractInt(MMDB_entry_data_s *entry_data);
    char *extractDouble(MMDB_entry_data_s *entry_data);
    char *extractBoolean(MMDB_entry_data_s *entry_data);
    int process_map_key(int db, char *key, 
                        MMDB_entry_data_list_s *entry_data_list);
    int process_array_entry(int db, char *key,  
                            MMDB_entry_data_s *entry_data);
    int check_entry_data_list(int db, MMDB_entry_data_list_s *entry_data_list, 
                              MMDB_entry_data_list_s **entry_data_list_next, 
                              char *key_full);
    int simple_data_type(MMDB_entry_data_s *entry_data);
    char *extract_simple_data(MMDB_entry_data_s *entry_data);
    LS_NO_COPY_ASSIGN(GeoIpData2);
};

class IpToGeo2 : public Ip2Geo
{
    friend class GeoIpData2;
public:
    IpToGeo2();
    ~IpToGeo2();

    // Call for ALL databases before you do ANY lookups.
    int setGeoIpDbFile(const char *pFile, const char *pDbLogical);

    // configOldKeywordNewFile is just for IpToGeo2
    int configOldKeywordNewFile(const XmlNodeList *pList);
    // 1st config file is for compatibility
    int config(const XmlNodeList *pList); // Compatible with old configs ONLY!
    // New config functions can be called multiple times
    int configSetFile(const char *pAliasFileName);
        // pAliasFileName is EVERYTHING after 'MaxMindDBFile <whitespace>'
    int configSetEnv(const char *pEnvAliasMap);
        // pEnvAliasMap is EVERYTHING after MaxMindDBEnv

    // Called only after config and setEnv
    GeoInfo *lookUp(uint32_t addr);
    GeoInfo *lookUpV6(in6_addr addr);
    // Just for IpToGeo2 
    GeoInfo *lookUp(const char *pIP);
    GeoInfo *lookUpV6(const char *pIP);

    static void setIpToGeo2(IpToGeo2 *pItg)
    {   s_pIpToGeo2 = pItg;  }
    static IpToGeo2 *getIpToGeo2()
    {   return s_pIpToGeo2;  }

private:
    GeoInfo *lookUpSA(struct sockaddr *sa);
    void release_all();
    void normalize_address(uint32_t addr, struct sockaddr_in *sa);
    void normalize_address(in6_addr addr, struct sockaddr_in6 *sa);
    int loadGeoIpDbFile(const char *pFile, const char *pDbLogical);
    int testGeoIpDbFile(const char *pFile);
    const char *getDefaultLogicalName(const char *fileName);
    int parseEnvLine(const char *pEnvAliasMap,
                     char *variable, int var_len,
                     char *database, int database_len,
                     char *map, int map_len);
    int validateEnv(const char *pEnvAliasMap, const char *variable, 
                    const char *database, const char *map, int *db, int *found);
    int addEnv(const char *pEnvAliasMap, const char *variable, 
               const char *database, const char *map, const int db);
    int defaultEnv();
public:
    struct env_s;
    typedef struct env_s env_t;
    struct dbs_s;
    typedef struct dbs_s dbs_t;
private:    
    int    m_ndbs;
    dbs_t *m_dbs;
    int    m_did_add_env;
    int    m_did_add_env_default;
    static IpToGeo2 *s_pIpToGeo2;
    LS_NO_COPY_ASSIGN(IpToGeo2);
};

#endif
#endif // ENABLE_IPTOGEO2