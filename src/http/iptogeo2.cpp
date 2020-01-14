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


#include <ls.h>
#include "iptogeo2.h"


#ifdef ENABLE_IPTOGEO2


#include <http/httplog.h>
#include <log4cxx/logger.h>
#include <main/configctx.h>
#include <util/ienv.h>
#include <util/xmlnode.h>
#include <lsr/ls_pool.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/errno.h>

#define MAX_KEY_LENGTH 256

IpToGeo2 *IpToGeo2::s_pIpToGeo2 = NULL;


struct  GeoIpData2::env_found_s
{
    const char *m_value; // Variable value string
};
typedef struct GeoIpData2::env_found_s env_found_t;

// An array that always matches the m_ndbs in IpToGeo2;
struct GeoIpData2::db_found_s
{
    MMDB_entry_data_list_s *m_list;      // The list of variable/values
    GeoIpData2::env_found_t  *m_env_found; // The environment list (pointed to)
};
typedef struct GeoIpData2::db_found_s db_found_t;

struct IpToGeo2::env_s {
    const char  *m_var;       // Variable name
    const char  *m_key;       // The 'key' in the maxminddb (from the user)
};
typedef struct IpToGeo2::env_s env_t;

struct IpToGeo2::dbs_s {
    int         m_open;
    const char *m_file;    // File name.
    const char *m_logical; // The logical name to be used for the DB in env vars
    MMDB_s     *m_mmdb;    // The open DB handle
    int         m_nenv;
    IpToGeo2::env_t *m_env;
};
typedef struct IpToGeo2::dbs_s dbs_t;

// I only have samples of the ASN, City and Country databases.  We can add
// more as we get more samples.
IpToGeo2::env_t s_env_ASN[] =
{
    { "GEOIP_ORGANIZATION", "/autonomous_system_organization" },
    { "GEOIP_ISP", "/autonomous_system_number" },
};

IpToGeo2::env_t s_env_City[] =
{
    { "GEOIP_COUNTRY_CODE", "/country/iso_code" },
    { "GEOIP_COUNTRY_NAME", "/country/names/en" },
    { "GEOIP_CONTINENT_CODE", "/continent/code" },
    { "GEOIP_COUNTRY_CONTINENT", "/continent/code" },
    { "GEOIP_REGION", "/subdivisions/0/iso_code" },
    { "GEOIP_REGION_NAME", "/subdivisions/0/names/en" },
    { "GEOIP_DMA_CODE", "/location/metro_code" },
    { "GEOIP_METRO_CODE", "/location/metro_code" },
    { "GEOIP_LATITUDE", "/location/latitude" },
    { "GEOIP_LONGITUDE", "/location/longitude" },
    { "GEOIP_POSTAL_CODE", "/postal/code" },
    { "GEOIP_CITY", "/city/names/en" },
};

IpToGeo2::env_t s_env_Country[] =
{
    { "GEOIP_COUNTRY_CODE", "/country/iso_code" },
    { "GEOIP_COUNTRY_NAME", "/country/names/en" },
    { "GEOIP_CONTINENT_CODE", "/continent/code" },
    { "GEOIP_COUNTRY_CONTINENT", "/continent/code" },
};

IpToGeo2::dbs_t s_db_default[] =
{
    { 0,
      "GeoLite2-ASN.mmdb",
      "ASN_DB",
      NULL,
      (int)(sizeof(s_env_ASN) / sizeof(IpToGeo2::env_t)),
      s_env_ASN },
    { 0,
      "GeoLite2-Country.mmdb",
      "COUNTRY_DB",
      NULL,
      (int)(sizeof(s_env_Country) / sizeof(IpToGeo2::env_t)),
      s_env_Country },
    { 0,
      "GeoLite2-City.mmdb",
      "CITY_DB",
      NULL,
      (int)(sizeof(s_env_City) / sizeof(IpToGeo2::env_t)),
      s_env_City }
};

GeoIpData2::GeoIpData2()
    : m_IpToGeo2(NULL)
    , m_db_found(NULL)
    , m_did_parse_env(0)
    , m_tried_parse_env(0)
{
}


GeoIpData2::~GeoIpData2()
{
    release();
}

void GeoIpData2::reset()
{
    release();
}

void GeoIpData2::release()
{
    m_did_parse_env = 0;
    m_tried_parse_env = 0;

    if (m_db_found && m_IpToGeo2)
    {
        for (int i = 0; i < m_IpToGeo2->m_ndbs; ++i)
        {
            if (m_db_found[i].m_env_found)
            {
                for (int e = 0; e < m_IpToGeo2->m_dbs[i].m_nenv; ++e)
                    if (m_db_found[i].m_env_found[e].m_value)
                        ls_pfree((void *)m_db_found[i].m_env_found[e].m_value);
                ls_pfree((void *)m_db_found[i].m_env_found);
            }
            if (m_db_found[i].m_list)
                MMDB_free_entry_data_list(m_db_found[i].m_list);
        }
    }
    ls_pfree(m_db_found);
    m_db_found = NULL;
    m_IpToGeo2 = NULL;
}


char *GeoIpData2::strutf8(MMDB_entry_data_s *entry_data, char *key)
{
    if (entry_data->data_size >= (MAX_KEY_LENGTH - 1))
    {
        memcpy(key, entry_data->utf8_string,
               (MAX_KEY_LENGTH - 1));
        key[(MAX_KEY_LENGTH - 1)] = 0;
    }
    else
    {
        memcpy(key, entry_data->utf8_string,
               entry_data->data_size);
        key[entry_data->data_size] = 0;
    }
    return key;
}


char *GeoIpData2::strint(MMDB_entry_data_s *entry_data, char *key)
{
    const char *fmt;
    uint64_t i;
    switch (entry_data->type)
    {
        case MMDB_DATA_TYPE_UINT16:
        case MMDB_DATA_TYPE_UINT32:
            fmt = "%lu";
            if (entry_data->type == MMDB_DATA_TYPE_UINT16)
                i = entry_data->uint16;
            else
                i = entry_data->uint32;
            break;
        case MMDB_DATA_TYPE_INT32:
            fmt = "%ld";
            i = entry_data->int32;
            break;
        case MMDB_DATA_TYPE_UINT64:
        default :
            fmt = "%lu";
            i = entry_data->uint64;
            break;
    }
    snprintf(key, MAX_KEY_LENGTH - 1, fmt, i);
    return(key);
}


char *GeoIpData2::strdouble(MMDB_entry_data_s *entry_data, char *key)
{
    snprintf(key, MAX_KEY_LENGTH - 1, "%7.5f",
             entry_data->double_value);
    return(key);
}


char *GeoIpData2::strboolean(MMDB_entry_data_s *entry_data, char *key)
{
    snprintf(key, MAX_KEY_LENGTH - 1, "%s", entry_data->boolean ? "1" : "0");
    return(key);
}


char *GeoIpData2::extractString(MMDB_entry_data_s *entry_data)
{
    LS_DBG("[GEO] extractString\n");
    char str[MAX_KEY_LENGTH];
    char *pstr = ls_pdupstr(strutf8(entry_data, str));
    LS_DBG("[GEO]    -> new string: %s\n", pstr);
    return pstr;
}


char *GeoIpData2::extractInt(MMDB_entry_data_s *entry_data)
{
    if ((entry_data->type != MMDB_DATA_TYPE_UINT16) &&
        (entry_data->type != MMDB_DATA_TYPE_UINT32) &&
        (entry_data->type != MMDB_DATA_TYPE_INT32) &&
        (entry_data->type != MMDB_DATA_TYPE_UINT64))
    {
        LS_DBG("[GEO] IpToGeo2::extractInt wrong type: %d\n", entry_data->type);
        return NULL;
    }
    char str[MAX_KEY_LENGTH];
    char *pstr;
    pstr = ls_pdupstr(strint(entry_data, str));
    return pstr;
}


char *GeoIpData2::extractDouble(MMDB_entry_data_s *entry_data)
{
    char str[MAX_KEY_LENGTH];
    char *pstr;
    pstr = ls_pdupstr(strdouble(entry_data, str));
    return pstr;
}

char *GeoIpData2::extractBoolean(MMDB_entry_data_s *entry_data)
{
    char str[MAX_KEY_LENGTH];
    char *pstr;
    pstr = ls_pdupstr(strboolean(entry_data, str));
    return pstr;
}

int GeoIpData2::simple_data_type(MMDB_entry_data_s *data)
{
    if ((data->type == MMDB_DATA_TYPE_UTF8_STRING) ||
        (data->type == MMDB_DATA_TYPE_DOUBLE) ||
        (data->type == MMDB_DATA_TYPE_INT32) ||
        (data->type == MMDB_DATA_TYPE_UINT16) ||
        (data->type == MMDB_DATA_TYPE_UINT32) ||
        (data->type == MMDB_DATA_TYPE_UINT64) ||
        (data->type == MMDB_DATA_TYPE_BOOLEAN))
        return 1;
    return 0;
}


char *GeoIpData2::extract_simple_data(MMDB_entry_data_s *data)
{
    if (data->type == MMDB_DATA_TYPE_UTF8_STRING)
        return extractString(data);
    if (data->type == MMDB_DATA_TYPE_DOUBLE)
        return extractDouble(data);
    if ((data->type == MMDB_DATA_TYPE_INT32) ||
        (data->type == MMDB_DATA_TYPE_UINT16) ||
        (data->type == MMDB_DATA_TYPE_UINT32) ||
        (data->type == MMDB_DATA_TYPE_UINT64))
        return extractInt(data);
    if (data->type == MMDB_DATA_TYPE_BOOLEAN)
        return extractBoolean(data);
    LS_WARN("[GEO] Extract of type %d not simple!\n", data->type);
    return NULL;
}


int GeoIpData2::process_map_key(int db, char *key,
                                MMDB_entry_data_list_s *entry_data_list)
{
    int nenv =  m_IpToGeo2->m_dbs[db].m_nenv;
    db_found_t *db_found = &m_db_found[db];
    for (int i = 0; i < nenv; ++i)
    {
        IpToGeo2::env_t const *env = &m_IpToGeo2->m_dbs[db].m_env[i];
        env_found_t *env_found = &db_found->m_env_found[i];

        if (!(strcasecmp(key, env->m_key)))
        {
            LS_DBG("[GEO] Found key %s, env: %s\n", key, env->m_var);
            struct MMDB_entry_data_s *data = &entry_data_list->next->entry_data;
            if (simple_data_type(data))
                env_found->m_value = extract_simple_data(data);
            else
            {
                LS_WARN("[GEO] Unexpected data type: %d in %s\n",
                        data->type,
                        m_IpToGeo2->m_dbs[db].m_logical);
                continue;
            }
            if (!env_found->m_value)
            {
                LS_ERROR("[GEO] Insufficient memory for env value of %s\n",
                         env->m_var);
                return -1;
            }
            else
            {
                LS_DBG("[GEO] Env found: db #%d %s=%s=%s\n", db, key, env->m_var,
                       env_found->m_value);
                break;
            }
        }
    }
    return 0;
}


int GeoIpData2::process_array_entry(int db, char *key,
                                    MMDB_entry_data_s *entry_data)
{
    int nenv =  m_IpToGeo2->m_dbs[db].m_nenv;
    db_found_t *db_found = &m_db_found[db];
    LS_DBG("[GEO] process_array_index: %s\n", key);
    for (int i = 0; i < nenv; ++i)
    {
        IpToGeo2::env_t const *env = &m_IpToGeo2->m_dbs[db].m_env[i];
        env_found_t *env_found = &db_found->m_env_found[i];

        if (!(strcasecmp(key, env->m_key)))
        {
            env_found->m_value = extract_simple_data(entry_data);
            if (!env_found->m_value)
            {
                LS_ERROR("[GEO] Insufficient memory for env value of %s\n",
                         env->m_var);
                return -1;
            }
            else
            {
                LS_DBG("[GEO] Env found: db #%d %s=%s=%s\n", db, key, env->m_var,
                       env_found->m_value);
                break;
            }
        }
    }
    return 0;
}


int GeoIpData2::check_entry_data_list(
    int db,
    MMDB_entry_data_list_s *entry_data_list,
    MMDB_entry_data_list_s **entry_data_list_next, char *key_full)
{
    LS_DBG("[GEO] check_entry_data_list: key: %s\n", key_full);
    if (!entry_data_list)
    {
        LS_ERROR("[GEO] NULL entry data list while interpreting GEO result\n");
        return -1;
    }
    uint32_t initial_type = entry_data_list->entry_data.type;
    LS_DBG("[GEO] initial type: %d\n", initial_type);
    int ret;
    // Handle the aggregate types separately.
    if (initial_type == MMDB_DATA_TYPE_MAP)
    {
        uint32_t count = entry_data_list->entry_data.data_size;
        uint32_t initial_count = count;
        LS_DBG("[GEO] Map count: %d\n", count);
        entry_data_list = entry_data_list->next;
        while ((entry_data_list) && (count))
        {
            count--;
            if (MMDB_DATA_TYPE_UTF8_STRING != entry_data_list->entry_data.type) {
                LS_ERROR("[GEO] Expected a string type in a map (got %d)\n",
                         entry_data_list->entry_data.type);
                return -1;
            }
            char key[MAX_KEY_LENGTH];
            char key_child[MAX_KEY_LENGTH];
            strutf8(&entry_data_list->entry_data, key);
            snprintf(key_child, sizeof(key_child), "%s/%s",
                     key_full ? key_full : "", key);
            LS_DBG("[GEO] Map[%d of %d] key_child: %s\n", initial_count - count,
                   initial_count, key_child);
            if (process_map_key(db, key_child, entry_data_list) == -1)
                return -1;
            if (check_entry_data_list(db, entry_data_list->next,
                                      &entry_data_list, key_child) == -1)
                return -1;
        } // for
        if (count)
            LS_WARN("[GEO] Unexpected early exit from a map %d left of %d\n",
                    count, initial_count);
        (*entry_data_list_next) = entry_data_list;
        return 0; // advanced through recursion
    } // if map
    else if (initial_type == MMDB_DATA_TYPE_ARRAY)
    {
        uint32_t count = entry_data_list->entry_data.data_size;
        /* A customer found a bug where there might be multiple array elements
         * so we need to be able to deal with an array element (as is
         * documented.  */
        int implied_index = 0;
        if (key_full == NULL)
        {
            key_full = (char *)"0";
            implied_index = 1;
        }
        int index = 0;
        entry_data_list = entry_data_list->next;
        LS_DBG("[GEO] Array: Count: %d\n", count);
        while (count && entry_data_list)
        {
            char key_child[MAX_KEY_LENGTH];

            snprintf(key_child, sizeof(key_child), "%s/%u", key_full, index);
            LS_DBG("[GEO] Array[%d]:\n", index);
            if (simple_data_type(&entry_data_list->entry_data))
            {
                LS_DBG("[GEO] Array: SIMPLE, type: %d\n",
                       entry_data_list->entry_data.type);
                if (process_array_entry(db, key_child,
                                        &entry_data_list->entry_data) == -1)
                    return -1;
                entry_data_list = entry_data_list->next;
            }
            else
            {
                LS_DBG("[GEO] Array: Container, type: %d\n",
                       entry_data_list->entry_data.type);
                ret = check_entry_data_list(db, entry_data_list,
                                            &entry_data_list,
                                            key_child);
                if (ret)
                    return ret;
            }
            count--;
            index++;
        }
        (*entry_data_list_next) = entry_data_list;
        return 0; // advanced through recursion
    }

    if (entry_data_list)
        entry_data_list = entry_data_list->next;

    (*entry_data_list_next) = entry_data_list;

    return 0;
}


int GeoIpData2::parseEnv()
{
    LS_DBG("[GEO] parseEnv\n");

    m_tried_parse_env = 1;
    for (int i = 0; i < m_IpToGeo2->m_ndbs; ++i)
    {
        LS_DBG("[GEO] parseEnv, db: %s\n", m_IpToGeo2->m_dbs[i].m_logical);
        if (!m_db_found[i].m_env_found)
        {
            int size = sizeof(env_found_t) * m_IpToGeo2->m_dbs[i].m_nenv;
            m_db_found[i].m_env_found = (env_found_t *)ls_palloc(size);
            if (!m_db_found[i].m_env_found)
            {
                LS_ERROR("[GEO] Insufficient memory to create DB env\n");
                return LS_FAIL;
            }
            memset(m_db_found[i].m_env_found,0,size);
        }
        MMDB_entry_data_list_s *list = m_db_found[i].m_list;
        while (list)
        {

            if (check_entry_data_list(i, list, &list, NULL))
            {
                LS_DBG("[GEO] Error in check_entry_data_list\n");
                break;
            }
        }
        if (m_db_found[i].m_list)
        {
            MMDB_free_entry_data_list(m_db_found[i].m_list);
            m_db_found[i].m_list = NULL;
        }
    }
    m_did_parse_env = 1;
    return 0;
}


const char* GeoIpData2::getGeoEnv(const char* pEnvName)
{
    LS_DBG("[GEO] getGeoEnv: %s\n", pEnvName);
    if (!m_IpToGeo2)
    {
        LS_ERROR("[GEO] Attempt to get GeoEnv without first doing lookup\n");
        return NULL;
    }
    if ((!m_did_parse_env) && (m_tried_parse_env))
    {
        LS_ERROR("[GEO] Attempt to get GeoEnv without environment variables set\n");
        return NULL;
    }
    if (!m_did_parse_env)
        if (parseEnv() == -1)
            return NULL;

    if (!m_db_found)
    {
        LS_ERROR("[GEO] Attempt to get GeoEnv without DB info collected\n");
        return NULL;
    }
    for (int i = 0; i < m_IpToGeo2->m_ndbs; ++i)
    {
        if ((m_db_found) && (m_db_found[i].m_env_found))
        {
            for (int e = 0; e < m_IpToGeo2->m_dbs[i].m_nenv; e++)
                if (!strcasecmp(m_IpToGeo2->m_dbs[i].m_env[e].m_var, pEnvName))
                {
                    LS_DBG("[GEO]     -> %s\n",
                           m_db_found[i].m_env_found[e].m_value);
                    if (!m_db_found[i].m_env_found[e].m_value)
                    {
                        LS_DBG("[GEO] NULL value so keep looking (db #%d)\n", i);
                        break; // out of envs to next db
                    }
                    else
                        return m_db_found[i].m_env_found[e].m_value;
                }
        }
    }
    LS_DBG("[GEO]     -> %s\n", "NOT FOUND");
    return NULL;
}

int GeoIpData2::addGeoEnv(IEnv *pEnv)
{
    int count = 0;
    LS_DBG("[GEO] addGeoEnv\n");
    if (!m_IpToGeo2)
    {
        LS_ERROR("[GEO] Attempt to add GeoEnv without first doing lookup\n");
        return -1;
    }
    if (!m_did_parse_env)
        if (parseEnv() == -1)
            return -1;

    if (!m_db_found)
    {
        LS_ERROR("[GEO] Attempt to add GeoEnv without DB info collected\n");
        return -1;
    }
    for (int db = 0; db < m_IpToGeo2->m_ndbs; ++db)
    {
        if (!m_db_found[db].m_env_found)
        {
            LS_ERROR("[GEO] Unexpected missing GeoInfo2 env data\n");
            return -1;
        }
        for (int env = 0; env < m_IpToGeo2->m_dbs[db].m_nenv; env++)
        {
            if (m_db_found[db].m_env_found[env].m_value)
            {
                LS_DBG("[GEO] Found env %s=%s\n",
                       m_IpToGeo2->m_dbs[db].m_env[env].m_var,
                       m_db_found[db].m_env_found[env].m_value);
                pEnv->add(m_IpToGeo2->m_dbs[db].m_env[env].m_var,
                          strlen(m_IpToGeo2->m_dbs[db].m_env[env].m_var),
                          m_db_found[db].m_env_found[env].m_value,
                          strlen(m_db_found[db].m_env_found[env].m_value));
                count++;
            }
        }
    }
    return count;
}


IpToGeo2::IpToGeo2()
    : m_ndbs(0)
    , m_dbs(NULL)
    , m_did_add_env(0)
    , m_did_add_env_default(0)
{
}

IpToGeo2::~IpToGeo2()
{
    release_all();
}


void IpToGeo2::release_all()
{
    LS_DBG("[GEO] release_all\n");
    for (int i = 0; i < m_ndbs; ++i)
    {
        if (m_dbs[i].m_env)
        {
            for (int s = 0; s < m_dbs[i].m_nenv; ++s)
            {
                if (m_dbs[i].m_env[s].m_var)
                    ls_pfree((void *)m_dbs[i].m_env[s].m_var);
                if (m_dbs[i].m_env[s].m_key)
                    ls_pfree((void *)m_dbs[i].m_env[s].m_key);
            }
            ls_pfree(m_dbs[i].m_env);
        }
        if (m_dbs[i].m_file)
            ls_pfree((void *)m_dbs[i].m_file);
        if (m_dbs[i].m_logical)
            ls_pfree((void *)m_dbs[i].m_logical);
        if (m_dbs[i].m_open)
            MMDB_close(m_dbs[i].m_mmdb);
        if (m_dbs[i].m_mmdb)
            ls_pfree(m_dbs[i].m_mmdb);
    }
    m_ndbs = 0;
    ls_pfree(m_dbs);
    m_dbs = NULL;
    m_did_add_env = 0;
    m_did_add_env_default = 0;
}


void IpToGeo2::normalize_address(uint32_t addr, struct sockaddr_in *sa)
{
    memset(sa, 0, sizeof(struct sockaddr_in));
    sa->sin_family = AF_INET;
    sa->sin_addr.s_addr = addr;
}


void IpToGeo2::normalize_address(in6_addr addr, struct sockaddr_in6 *sa)
{
    memset(sa, 0, sizeof(struct sockaddr_in6));
    sa->sin6_family = AF_INET6;
    memcpy(&sa->sin6_addr, &addr, sizeof(in6_addr));
}


int IpToGeo2::loadGeoIpDbFile(const char *pFile, const char *pDbLogical)
{
    int ret;
    LS_DBG("[GEO] loadGeoIpDbFile: %s, total dbs: %d\n", pFile, m_ndbs + 1);
    void *arr = ls_prealloc(m_dbs, ((m_ndbs + 1) * sizeof(dbs_t)));
    if (!arr)
    {
        LS_ERROR("[GEO] Insufficent memory allocationg GeoIP2 space\n");
        return LS_FAIL;
    }
    m_dbs = (dbs_t *)arr;
    memset(&m_dbs[m_ndbs],0,sizeof(dbs_t));
    m_dbs[m_ndbs].m_logical = ls_pdupstr(pDbLogical);
    if (!m_dbs[m_ndbs].m_logical)
    {
        LS_ERROR("[GEO] Insufficient memory allocating logical space\n");
        return LS_FAIL; // Free it all in the destructor
    }
    m_dbs[m_ndbs].m_file = ls_pdupstr(pFile);
    if (!m_dbs[m_ndbs].m_file)
    {
        LS_ERROR("[GEO] Insufficient memory allocating file name\n");
        return LS_FAIL; // Free it all in the destructor
    }
    m_dbs[m_ndbs].m_mmdb = (MMDB_s *)ls_palloc(sizeof(MMDB_s));
    if (!m_dbs[m_ndbs].m_mmdb)
    {
        LS_ERROR("[GEO] Insufficient memory allocating DB handle\n");
        return LS_FAIL; // Free it all in the destructor
    }
    ret = MMDB_open(pFile, 0, m_dbs[m_ndbs].m_mmdb);
    if (ret != 0)
    {
        LS_ERROR("[GEO] Error opening GeoIP2 DB file: %s: %s\n", pFile,
                 MMDB_strerror(ret));
        return LS_FAIL;
    }
    m_dbs[m_ndbs].m_open = 1;
    LS_DBG("[GEO] Geo file: %s now open, size: %ld\n",
           m_dbs[m_ndbs].m_mmdb->filename, m_dbs[m_ndbs].m_mmdb->file_size);
    m_ndbs++;

    return 0;
}


int IpToGeo2::testGeoIpDbFile(const char *pFile)
{
    // Was done in a fork, but did not work reliably that way.
    //pid_t pid = fork();
    //if (pid < 0)
    //{
    //    LS_ERROR("[GEO] testGeoIpDbFile fork failed, errno: %d\n", errno);
    //    return -1;
    //}
    //if (pid == 0)
    //{
        MMDB_s mmdb;
        int ret = MMDB_open(pFile, 0/*MMDB_MODE_MMAP*/, &mmdb);
        if (ret == 0)
        {
            unsigned char addr[4] = { 88, 252, 206, 167 };
            MMDB_lookup_result_s res;
            int dberr;
            struct sockaddr_in sa;
            normalize_address(*(int32_t *)addr, &sa);
            res = MMDB_lookup_sockaddr(&mmdb, (struct sockaddr *)&sa, &dberr);
            if (dberr != 0)
            {
                ret = dberr;
            }
            else if (!res.found_entry)
            {
                ret = 0xff;
            }
            else
            {
                // Found
            }
            MMDB_close(&mmdb);
        }
    //    exit(ret);
    //}
    //else
    //{
    //    int status;
    //    int wpid = waitpid(pid, &status, 0);
    //    if (wpid == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0)
    if (!ret)
            return 0;
    LS_ERROR("[GEO] GeoIP DB file test failed: '%s'.  ret: %d\n", pFile, ret);

    //    LS_ERROR("[GEO] GeoIP DB file test failed: '%s'.  Exited: %d, Status: %d\n",
    //             pFile, WIFEXITED(status), WEXITSTATUS(status));
    //}
    return -1;
}


int IpToGeo2::setGeoIpDbFile(const char *pFile, const char *pDbLogical)
{
    if ((!pFile) || (!pDbLogical))
    {
        LS_ERROR("[GEO] To add a DB file it must be named physically and "
                 "logically\n");
        return -1;
    }
    // Validate that neither pFile or pDbLogical have been seen before.
    for (int i = 0; i < m_ndbs; ++i)
    {
        if (!(strcmp(pFile, m_dbs[i].m_file)))
        {
            LS_ERROR("Specific file name can only be specified one time in "
                     "config: %s\n", pFile);
            return -1;
        }
        if (!(strcasecmp(pDbLogical, m_dbs[i].m_logical)))
        {
            LS_ERROR("Specific logical DB name can only be specified one time "
                     "in config: %s\n", pDbLogical);
            return -1;
        }
    }
    if (testGeoIpDbFile(pFile) != 0)
        return -1;
    return loadGeoIpDbFile(pFile, pDbLogical);
}


const char *IpToGeo2::getDefaultLogicalName(const char *fileName)
{
    int dblen = strlen(fileName);
    for (int i = 0; i < (int)(sizeof(s_db_default) / sizeof(dbs_t)); ++i)
    {
        int defDbLen = strlen(s_db_default[i].m_file);
        if ((dblen > defDbLen) &&
            (!(strcmp(s_db_default[i].m_file,
                      &fileName[dblen - defDbLen]))))
        {
            return s_db_default[i].m_logical;
        }
    }
    return NULL;
}


/**
 * config: With XmlNodeList
 * "GeoIP DB": Allows you a user to specify a NEW DB name in the OLD form.
 *             User interface nests environment variables within the XML
 *             definition.  Process the name, then the nested stuff and then
 *             do the defaults so that they will NOT overwrite the user's
 *             specifications.
 */
int IpToGeo2::config(const XmlNodeList *pList)
{
    XmlNodeList::const_iterator iter;
    int succ = 0;
    const char *logicalName = NULL;

    for (iter = pList->begin(); iter != pList->end(); ++iter)
    {
        XmlNode *p = *iter;
        const char *pFile = p->getValue();

        LS_DBG_M("[GEO] config test pList: name: %s, value: %s, has child: %d\n",
                 p->getName(), p->getValue(), p->hasChild());
        if (!(strcasecmp(p->getName(), "maxminddbenable")))
        {
            if (strcasecmp(p->getValue(), "off"))
            {
                LS_NOTICE("[GEO] Geolocation disabled by configuration\n");
                return -1;
            }
        }
        else if (!strcasecmp(p->getName(), "maxminddbfile"))
        {
            if (configSetFile(p->getValue()) == -1)
                return -1;
            succ = 1;
        }
        else if (!strcasecmp(p->getName(), "geoipdb"))
        {
            int hasEnv = 0;
            if (!pFile)
            {
                LS_ERROR("[GEO] for %s you must specify a valid file name\n",
                         p->getName());
                return -1;
            }
            LS_DBG_M("[GEO] geoipdb found, hasChild: %d\n", p->hasChild());
            if (p->hasChild())
            {
                XmlNodeList children;
                p->getAllChildren(children);
                XmlNodeList::const_iterator iterChild;

                for (iterChild = children.begin(); iterChild != children.end();
                     ++iterChild)
                {
                    XmlNode *parm = *iterChild;
                    LS_DBG_M("[GEO] check child name: %s, value: %s\n",
                             parm->getName(), parm->getValue());
                    if (!strcasecmp(parm->getName(), "geoipdbname"))
                    {
                        logicalName = parm->getValue();
                        LS_DBG_M("[GEO] geoipdb found logical name: %s\n",
                                 logicalName);
                    }
                    else if (!strcasecmp(parm->getName(),"maxminddbenv"))
                    {
                        hasEnv = 1;
                        LS_DBG_M("[GEO] geoipdb found env var for later\n");
                    }
                }
            }
            if (!logicalName)
            {
                // Use a default logical name?
                if (!(logicalName = getDefaultLogicalName(pFile)))
                    LS_ERROR("[GEO] no logical name specified for unknown "
                             "database %s\n", pFile);
                    return -1;
            }
            if (setGeoIpDbFile(pFile, logicalName) == -1)
                return -1;
            if (hasEnv)
            {
                XmlNodeList children;
                p->getAllChildren(children);
                XmlNodeList::const_iterator iterChild;

                for (iterChild = children.begin(); iterChild != children.end();
                     ++iterChild)
                {
                    XmlNode *parm = *iterChild;
                    LS_DBG_M("[GEO] check child name for env: %s, value: %s\n",
                             parm->getName(), parm->getValue());
                    if (!strcasecmp(parm->getName(),"maxminddbenv"))
                    {
                        LS_DBG_M("[GEO] process env override now\n");
                        if (configSetEnv(parm->getValue()))
                            return -1;
                    }
                }
            }
            succ = 1;
        }
        else if (!strcasecmp(p->getName(), "maxminddbenv"))
        {
            if (configSetEnv(p->getValue()))
                return -1;
        }
    }
    if (succ)
        IpToGeo2::setIpToGeo2(this);
    else
    {
        LS_WARN(ConfigCtx::getCurConfigCtx(),
                "Failed to setup a valid GeoIP2 DB file, Geolocation is disabled!");
        return LS_FAIL;
    }
    return 0;
}

int IpToGeo2::configSetFile(const char *pAliasFileName)
{
    /**
     * AliasFileName format is everything defined for 'MaxMindDBFile' AFTER
     * that keyword: <whitespace><db_logical><whitespace><file_name>
     **/
    const char *env_title = "MaxMindDBFile";
    int i;
    const char *pFileName;
    char alias[MAX_PATH_LEN];
    const char *whitespace = " \t=";
    const char *pAlias;
    /* skip whitespace.  */
    i = strspn(pAliasFileName,whitespace);
    pAliasFileName += i;
    if (!*pAliasFileName)
    {
        LS_ERROR("[GEO] %s has no value\n", env_title);
        return -1;
    }
    const char *beginWhitespace;
    beginWhitespace = strpbrk(pAliasFileName,whitespace);
    if (!beginWhitespace)
    {
        pFileName = pAliasFileName;
        pAlias = NULL;
    }
    else
    {
        int copy = (int)(beginWhitespace - pAliasFileName);
        if (copy >= (int)(sizeof(alias) - 1))
        {
            memcpy(alias, pAliasFileName, sizeof(alias) - 1);
            alias[sizeof(alias) - 1] = 0;
        }
        else
            memcpy(alias, pAliasFileName, copy);
        pAliasFileName += copy;
        pAlias = alias;
        i = strspn(pAliasFileName, whitespace);
        pAliasFileName += i;
        if (!*pAliasFileName)
        {
            // it's really all file name
            pFileName = alias;
            pAlias = NULL;
        }
        else
        {
            pFileName = pAliasFileName;
        }
    }
    if (setGeoIpDbFile(pFileName, pAlias) == -1)
    {
        if (!IpToGeo2::getIpToGeo2())
            LS_WARN(ConfigCtx::getCurConfigCtx(),
                    "Failed to setup a valid GeoIP DB file, Geolocation is disabled!");
        return LS_FAIL;
    }
    IpToGeo2::setIpToGeo2(this);
    return 0;
}


int IpToGeo2::parseEnvLine(const char *pEnvAliasMap,
                           char *variable, int var_len,
                           char *database, int database_len,
                           char *map, int map_len)
{
    /**
     * EnvAliasMap format is everything AFTER 'MaxMindDbEnv'
     * <whitespace><env_title><whitespace><database>/<hierarchical-map>
     *    <database> MUST match a database previously defined in MaxMindDBFile
     *               (or our defaults).
     **/
    const char *pEnvAliasMapInitial = pEnvAliasMap;
    const char *env_title = "MaxMindDBEnv";
    int i;
    const char *whitespace = " \t=";
    const char *pMap;
    LS_DBG("[GEO] parseEnvLine: %s\n", pEnvAliasMap);

    /* skip whitespace.  */
    i = strspn(pEnvAliasMap,whitespace);
    pEnvAliasMap += i;
    if (!*pEnvAliasMap)
    {
        LS_ERROR("[GEO] %s has no env. variable\n", env_title);
        return -1;
    }
    const char *beginWhitespace;
    beginWhitespace = strpbrk(pEnvAliasMap,whitespace);
    if (!beginWhitespace)
    {
        LS_ERROR("[GEO] %s for variable %s has no mapping\n", env_title,
                 pEnvAliasMap);
        return -1;
    }
    int copy = (int)(beginWhitespace - pEnvAliasMap);
    if (copy >= (int)(var_len - 1))
    {
        LS_ERROR("[GEO] %s environment title is too long\n", pEnvAliasMap);
        return -1;
    }
    else
    {
        memcpy(variable, pEnvAliasMap, copy);
        variable[copy] = 0;
        pEnvAliasMap += copy;
    }
    pMap = pEnvAliasMap;
    i = strspn(pMap, whitespace);
    pMap += i;
    if (!*pMap)
    {
        LS_ERROR("[GEO] %s for variable %s has no mapping\n", env_title,
                 variable);
        return -1;
    }
    // Mapping is two components <database>/<heirarchical-map>
    if (*pMap == '/')
    {
        LS_ERROR("[GEO] %s missing DB alias in %s", env_title,
                 pEnvAliasMapInitial);
        return -1;
    }
    const char *slash = strchr(pMap,'/');
    if (!slash)
    {
        LS_ERROR("[GEO] %s missing alias map in %s (%s)", env_title,
                 pEnvAliasMapInitial, pMap);
        return -1;
    }
    if (slash - pMap >= database_len)
    {
        LS_ERROR("[GEO] %s has too long of a database name in %s", env_title,
                 pEnvAliasMapInitial);
        return -1;
    }
    memcpy(database, pMap, slash - pMap);
    database[slash - pMap] = 0;
    lstrncpy(map, slash, map_len);
    return 0;
}


int IpToGeo2::validateEnv(const char *pEnvAliasMap, const char *variable,
                          const char *database, const char *map, int *db,
                          int *found)
{
    const char *env_title = "MaxMindDBEnv";
    *found = 0;
    for (int i = 0; i < m_ndbs; ++i)
    {
        if (!(strcasecmp(database, m_dbs[i].m_logical)))
        {
            if (!m_dbs[i].m_nenv)
            {
                LS_DBG("[GEO] FIRST env! %s=%s%s DB #%d\n", variable, database,
                       map, i);
                *db = i;
                return 0;
            }
            // Make sure the variable hasn't been defined.
            for (int e = 0; e < m_dbs[i].m_nenv; ++e)
            {
                if (strcasecmp(variable, m_dbs[i].m_env[e].m_var) == 0)
                {
                    LS_DBG_M("[GEO] In %s Environment variable %s 2nd definition"
                             "being ignored", env_title, variable);
                    *found = 1;
                    break;
                }
            }
            LS_DBG("[GEO] Env FOUND! %s=%s%s DB #%d\n", variable, database,
                   map, i);
            (*db) = i;
            return 0;
        }
    }
    LS_ERROR("[GEO] Environment variable database not found: %s\n",
             pEnvAliasMap);
    return -1;
}


int IpToGeo2::addEnv(const char *pEnvAliasMap, const char *variable,
                     const char *database, const char *map, const int db)
{
    const char *env_title = "MaxMindDBEnv";
    env_t *arr;

    if (db > m_ndbs)
    {
        LS_ERROR("[GEO] addEnv Unexpected db #%d, current num: %d\n", db, m_ndbs);
        return -1;
    }
    LS_DBG("[GEO] addEnv db #%d (of %d) n_env: %d, add %s=%s\n", db, m_ndbs,
           m_dbs[db].m_nenv, variable, map);
    arr = (env_t *)ls_prealloc(m_dbs[db].m_env,
                               sizeof(env_t) * (m_dbs[db].m_nenv + 1));
    if (!arr)
    {
        LS_ERROR("[GEO] for %s Insufficient memory to allocate space for "
                 "env: %s\n",env_title, pEnvAliasMap);
        return -1;
    }
    m_dbs[db].m_env = arr;
    int index = m_dbs[db].m_nenv;
    memset(&m_dbs[db].m_env[index], 0, sizeof(env_t));
    m_dbs[db].m_nenv++;
    m_dbs[db].m_env[index].m_var = ls_pdupstr(variable);
    m_dbs[db].m_env[index].m_key = ls_pdupstr(map);

    if ((!m_dbs[db].m_env[index].m_var) ||
        (!m_dbs[db].m_env[index].m_key))
    {
        LS_ERROR("[GEO] Insufficient memory to allocate space for env values: %s\n",
                 pEnvAliasMap);
        return -1;
    }
    return 0;
}


int IpToGeo2::configSetEnv(const char *pEnvAliasMap)
{
    char variable[MAX_KEY_LENGTH];
    char database[MAX_PATH_LEN];
    char map[MAX_PATH_LEN];
    int  db;
    int  found;
    if (parseEnvLine(pEnvAliasMap, variable, sizeof(variable),
                     database, sizeof(database), map, sizeof(map)) == -1)
        return -1;
    if (validateEnv(pEnvAliasMap, variable, database, map, &db, &found) == -1)
        return -1;
    if (!found)
    {
        if (addEnv(pEnvAliasMap, variable, database, map, db) == -1)
            return -1;
        m_did_add_env = 1;
    }
    return 0;
}


int IpToGeo2::defaultEnv()
{
    LS_DBG("[GEO] defaultEnv %d DBs, default DBs: %d\n", m_ndbs,
           (int)(sizeof(s_db_default) / sizeof(dbs_t)));
    m_did_add_env_default = 1;
    for (int i = 0; i < m_ndbs; ++i)
    {
        for (int defDb = 0; defDb < (int)(sizeof(s_db_default) / sizeof(dbs_t));
             ++defDb)
        {
            if (!(strcasecmp(s_db_default[defDb].m_logical, m_dbs[i].m_logical)))
            {
                LS_DBG("[GEO] defaultEnv - logical DB: %s\n", m_dbs[i].m_logical);
                for (int e = 0; e < s_db_default[defDb].m_nenv; ++e)
                {
                    char env[MAX_PATH_LEN * 3];
                    snprintf(env, sizeof(env), "%s %s%s",
                             s_db_default[defDb].m_env[e].m_var,
                             m_dbs[i].m_logical,
                             s_db_default[defDb].m_env[e].m_key);

                    LS_DBG("[GEO]    Try to add %s\n", env);
                    if (configSetEnv(env) == -1)
                        return -1;
                }
            }
        }
    }
    if (!m_did_add_env)
    {
        LS_ERROR("[GEO] No environment variables were added but files were set\n");
        release_all();
        return -1;
    }
    return 0;
}


GeoInfo * IpToGeo2::lookUpSA(struct sockaddr *sa)
{
    int one_worked = 0;
    if (sa->sa_family == AF_INET)
        LS_DBG("[GEO] LookUp of %u.%u.%u.%u\n",
               (unsigned char)((char *)&((struct sockaddr_in *)sa)->sin_addr)[0],
               (unsigned char)((char *)&((struct sockaddr_in *)sa)->sin_addr)[1],
               (unsigned char)((char *)&((struct sockaddr_in *)sa)->sin_addr)[2],
               (unsigned char)((char *)&((struct sockaddr_in *)sa)->sin_addr)[3]);
    else
        LS_DBG("[GEO] LookUp of IPv6\n");

    // Note that addr is already in line byte order
    if (!m_ndbs)
    {
        LS_ERROR("[GEO] No open GeoIP2 databases.  Must be specified in config "
                 "file\n");
        return NULL;
    }
    if ((m_ndbs) && (!m_did_add_env_default))
        if (defaultEnv() == -1)
        {
            LS_ERROR("[GEO] Error setting default ENV\n");
            return NULL;
        }

    GeoIpData2::db_found_t *db_found = (GeoIpData2::db_found_t *)ls_palloc(
        sizeof(GeoIpData2::db_found_t) * m_ndbs);
    if (!db_found)
    {
        LS_ERROR("[GEO] Insufficient memory to create DB info\n");
        return NULL;
    }
    memset(db_found,0,sizeof(GeoIpData2::db_found_t) * m_ndbs);
    GeoIpData2 *pInfo = new GeoIpData2();
    if (!pInfo)
    {
        LS_ERROR("[GEO] Can't allocate GeoIpData2\n");
        ls_pfree(db_found);
        return NULL;
    }
    pInfo->m_db_found = db_found;

    for (int i = 0; i < m_ndbs; ++i)
    {
        MMDB_lookup_result_s res;
        int dberr;

        pInfo->m_db_found[i].m_list = NULL;
        res = MMDB_lookup_sockaddr(m_dbs[i].m_mmdb, sa, &dberr);
        if (dberr != 0)
            LS_NOTICE("[GEO] IP lookup failed: %s\n", MMDB_strerror(dberr));
        else if (!res.found_entry)
            LS_NOTICE("[GEO] IP lookup failed, entry not found\n");
        else
        {
            // Found
            dberr = MMDB_get_entry_data_list(&res.entry,
                                             &pInfo->m_db_found[i].m_list);
            if (dberr != 0)
                LS_ERROR("[GEO] Get data entry list failed: %s\n",
                         MMDB_strerror(dberr));
            else
            {
                one_worked = 1;
                LS_DBG("[GEO] LookUp Worked!\n");
            }
        }
    }
    if (!one_worked)
    {
        LS_NOTICE("[GEO] No successful GeoIP2 lookups\n");
        pInfo->release();
        delete pInfo;
        return NULL;
    }
    pInfo->m_IpToGeo2 = this;
    return pInfo;
}

GeoInfo *IpToGeo2::lookUp(uint32_t addr)
{
    // Note that addr is already in line byte order
    struct sockaddr_in sa;
    normalize_address(addr, &sa);
    return(lookUpSA((struct sockaddr *)&sa));
}


GeoInfo *IpToGeo2::lookUpV6(in6_addr addr)
{
    struct sockaddr_in6 sa;
    normalize_address(addr, &sa);
    return(lookUpSA((struct sockaddr *)&sa));
}


GeoInfo *IpToGeo2::lookUp(const char *pIP)
{
    uint32_t addr = inet_addr(pIP);
    if (addr == INADDR_BROADCAST)
    {
        LS_NOTICE("[GEO] inet_addr %s returns the broadcast address, invalid\n", pIP);
        return NULL;
    }
    return lookUp(addr);
}


GeoInfo *IpToGeo2::lookUpV6(const char *pIP)
{
    struct in6_addr addr;
    if (inet_pton(AF_INET6, pIP, &addr) == 0)
    {
        LS_NOTICE("[GEO] %s is not a valid IP6 address\n", pIP);
        return NULL;
    }
    return lookUpV6(addr);
}

#endif //ENABLE_IPTOGEO2
