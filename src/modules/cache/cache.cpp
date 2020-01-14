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
#include "cacheconfig.h"
#include "cachectrl.h"
#include "cacheentry.h"
#include "cachehash.h"
#include "dirhashcachestore.h"

#include <limits.h>
#include <ls.h>
#include <lsr/ls_confparser.h>
#include <util/autostr.h>
#include <util/datetime.h>
#include <util/stringtool.h>
#include <util/ni_fio.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <util/gpath.h>

#include <http/httpserverconfig.h>
#include <http/httpreq.h>
#include <http/httpheader.h>
#include <http/httpsession.h>
#include <http/httpvhost.h>
#include <http/httpmime.h>
#include <http/serverprocessconfig.h>
#include <util/autostr.h>
#include <sys/uio.h>
#include <zlib.h>



#define MAX_CACHE_CONTROL_LENGTH    128
#define MAX_RESP_HEADERS_NUMBER     50
#define MNAME                       cache
#define ModuleNameStr               "Module:Cache"
#define CACHEMODULEKEY              "_lsi_module_cache_handler__"
#define CACHEMODULEKEYLEN           (sizeof(CACHEMODULEKEY) - 1)
#define CACHEMODULEROOT             "cachedata/"

#define MODULE_VERSION_INFO         "1.62"

//The below info should be gotten from the configuration file
#define max_file_len        4096
#define VALMAXSIZE          4096
#define MAX_HEADER_LEN      16384
#define Z_BUF_SIZE          16384

/////////////////////////////////////////////////////////////////////////////
extern lsi_module_t MNAME;

static const char s_x_cached[] = "X-LiteSpeed-Cache";
static const char *s_hits[3] = { "hit", "hit,private", "hit,litemage" };
static int s_hitsLen[3] = { 3, 11, 12 };

static const char *cache_env_key = "cache-control";
static int icache_env_key = 13;

/**
 * if s_x_vary is set in resp header, ENV "vary=...." should be removed
 * then we should use this valus instead.
 */
static const char *s_x_vary = "X-LiteSpeed-Vary";

/* IF WANT TO USE THE RECV REQ HEADER HOOK
 * THEN DEFINE USE_RECV_REQ_HEADER_HOOK in cacheentry.h
 * But now it seems this will cuase some issues about using rewrite
 * pagespeed and wordpress and so on
 * We may remove it later
 * Right now, just disable it
 */
//#define USE_RECV_REQ_HEADER_HOOK


/***
 * 11/6/2017 changes: Make sure save the compressed copy to disk
 * Serv need to check if OK to serv with gzipped
 *
 */


enum
{
    CE_STATE_NOCACHE = 0,
    CE_STATE_HAS_PRIVATE_CACHE,
    CE_STATE_HAS_PUBLIC_CACHE,
    CE_STATE_UPDATE_STALE,
    CE_STATE_WILLCACHE,
    CE_STATE_CACHED,
    CE_STATE_CACHEFAILED,
};

enum HTTP_METHOD
{
    HTTP_UNKNOWN = 0,
    HTTP_OPTIONS,
    HTTP_GET ,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT ,
    HTTP_DELETE,
    HTTP_TRACE,
    HTTP_CONNECT,
    HTTP_MOVE,
    DAV_PROPFIND,
    DAV_PROPPATCH,
    DAV_MKCOL,
    DAV_COPY,
    DAV_LOCK,
    DAV_UNLOCK,
    DAV_VERSION_CONTROL,
    DAV_REPORT,
    DAV_CHECKIN,
    DAV_CHECKOUT,
    DAV_UNCHECKOUT,
    DAV_UPDATE,
    DAV_MKWORKSPACE,
    DAV_LABEL,
    DAV_MERGE,
    DAV_BASELINE_CONTROL,
    DAV_MKACTIVITY,
    DAV_BIND,
    DAV_SEARCH,
    HTTP_PURGE,
    HTTP_REFRESH,
    HTTP_METHOD_END,
};

struct MyMData
{
    CacheConfig    *pConfig;
    CacheEntry     *pEntry;
    char           *pOrgUri;

    /**
     * Save the list of rewrite ENV of "cache-vary:...."
     */
    AutoStr2        *pCacheCtrlVary;

    /**
     * Save the rewrite ENV of "cache-control: vary=...."
     */
    AutoStr2        *pCacheVary;

    unsigned char   iCacheState;
    unsigned char   iMethod;
    unsigned char   iHaveAddedHook;
    unsigned char   iCacheSendBody;
    CacheCtrl       cacheCtrl;
    CacheHash       cePublicHash;
    CacheHash       cePrivateHash;
    CacheKey        cacheKey;
    uint8_t         hkptIndex;
    uint8_t         hasCacheFrontend;
    uint8_t         reqCompressType; //0, no, 1: gzip, 2:br
    uint8_t         saveFailed;
    XXH64_state_t   contentState;
    z_stream       *zstream;
    off_t           orgFileLength;


};


static lsi_config_key_t paramArray[] =
{
    /***
     * The id is base on 0, added some just for reference
     */
    {"enableCache",             0, 0},  //0
    {"enablePrivateCache",      1, 0},
    {"checkPublicCache",        2, 0},
    {"checkPrivateCache",       3, 0}, //3
    {"qsCache",                 4, 0},
    {"reqCookieCache",          5, 0},

    {"ignoreReqCacheCtrl",      6, 0},
    {"ignoreRespCacheCtrl",     7, 0}, //7

    {"respCookieCache",         8, 0},

    {"expireInSeconds",         9, 0},
    {"privateExpireInSeconds",  10, 0},//10
    {"maxStaleAge",             11, 0},
    {"maxCacheObjSize",         12, 0},

    {"storagepath",             13, 0},//13 "cacheStorePath"

    //Below 5 are newly added
    {"noCacheDomain",           14, 0},//14
    {"noCacheUrl",              15, 0},//15

    {"no-vary",                 16, 0},//},16
    {"addEtag",                 17, 0},
    {"purgeUri",                18, 0},
    {"reqHeaderVary",           19, 0},

    {NULL, 0, 0} //Must have NULL in the last item
};

//Update permission of the dest to the same of the src
static void matchDirectoryPermissions(const char *dir)
{
    ServerProcessConfig &procConfig = ServerProcessConfig::getInstance();
    chown(dir, procConfig.getUid(), procConfig.getGid());
    chmod(dir, S_ISGID | 0770);
}


static int createCachePath(const char *path, int mode)
{
    struct stat st;
    if (stat(path, &st) == -1)
    {
        if (GPath::createMissingPath((char *)path, mode) == -1)
            return LS_FAIL;
    }
    return LS_OK;
}


static void house_keeping_cb(const void *p)
{
    DirHashCacheStore *pStore = (DirHashCacheStore *)p;
    if (pStore)
    {
        pStore->houseKeeping();
        pStore->cleanByTracking(100, 100);
        g_api->log(NULL, LSI_LOG_DEBUG, "[%s]house_keeping_cb with store %p.\n",
                   ModuleNameStr, pStore);
    }
}


static int parseStoragePath(CacheConfig *pConfig, const char *pValStr,
                            int valLen, int level, const char *name)
{
    if (level != LSI_CFG_CONTEXT)
    {
        char *pBak = new char[valLen + 1];
        if (!pBak)
            return -1;
        memcpy(pBak, pValStr, valLen);
        pBak[valLen] = 0x00;
        pValStr = pBak;

        char pTmp[max_file_len]  = {0};
        char cachePath[max_file_len]  = {0};

        //check if contains $
        if (strchr(pValStr, '$'))
        {
            int ret = g_api->expand_current_server_variable(level, pValStr, pTmp,
                      max_file_len);
            if (ret >= 0)
            {
                pValStr = pTmp;
                valLen = ret;
            }
            else
            {
                g_api->log(NULL, LSI_LOG_ERROR,
                           "[%s]parseConfig failed to expand_current_server_"
                           "variable[%s], default will be in use.\n",
                           ModuleNameStr, pValStr);

                delete []pBak;
                return -1;
            }
        }

        if (pValStr[0] != '/')
            lstrncpy(cachePath, g_api->get_server_root(), sizeof(cachePath));
        lstrncat(cachePath, pValStr, sizeof(cachePath));
        lstrncat(cachePath, "/", sizeof(cachePath));

        if (createCachePath(cachePath, 0770) == -1)
        {
            g_api->log(NULL, LSI_LOG_ERROR,
                       "[%s]parseConfig failed to create directory [%s].\n",
                       ModuleNameStr, cachePath);
        }
        else
        {
            matchDirectoryPermissions(cachePath);
            pConfig->setStore(new DirHashCacheStore);
            pConfig->getStore()->setStorageRoot(cachePath);
            pConfig->getStore()->initManager();
            pConfig->setOwnStore(1);
            g_api->set_timer(10*1000, 1, house_keeping_cb, pConfig->getStore());
            
            g_api->log(NULL, LSI_LOG_DEBUG,
                       "[%s]parseConfig setStoragePath [%s] for level %d[name: %s].\n",
                       ModuleNameStr, cachePath, level, name);
        }
        delete []pBak;
    }
    else
        g_api->log(NULL, LSI_LOG_INFO,
                   "[%s]context [%s] shouldn't have 'storagepath' parameter.\n",
                   ModuleNameStr, name);

    return 0;
}


static int parseNoCacheDomain(CacheConfig *pConfig, const char *pValStr,
                              int valLen, int level, const char *name)
{
    if (level != LSI_CFG_SERVER)
    {
        g_api->log(NULL, LSI_LOG_INFO,
                   "[%s][%s] Only SERVER level can have 'noCacheDomain' "
                   "parameter.\n", ModuleNameStr, name);
        return 0;
    }

    if (!pConfig->getVHostMapExclude())
        pConfig->setVHostMapExclude(new VHostMap());
    pConfig->getVHostMapExclude()->mapDomainList(
        HttpServerConfig::getInstance().getGlobalVHost(), pValStr);

    g_api->log(NULL, LSI_LOG_DEBUG, "[%s]noCacheDomain [%.*s] added.\n",
               ModuleNameStr, valLen, pValStr);

    return 0;
}


static int parseNoCacheUrl(CacheConfig *pConfig, const char *pValStr,
                           int valLen, int level, const char *name)
{
    if (level != LSI_CFG_SERVER && level != LSI_CFG_VHOST)
    {
        g_api->log(NULL, LSI_LOG_INFO,
                   "[%s][%s] Only SERVER and VHOST level can have 'noCacheUrl' "
                   "parameter.\n", ModuleNameStr, name);
        return 0;
    }

    if (valLen == 0)
        return 0;

    bool bClear = (valLen == 7 && strncasecmp(pValStr, "<clear>", 7) == 0);
    if (bClear)
        pConfig->setOnlyUseOwnUrlExclude(1);

    if (!pConfig->getUrlExclude())
        pConfig->setUrlExclude(new Aho(1));

    pConfig->getUrlExclude()->addPattern(pValStr, valLen, NULL);
    return 0;
}

void parseNoCacheUrlFinal(CacheConfig *pConfig)
{
    if (pConfig->getUrlExclude())
    {
        pConfig->getUrlExclude()->makeTree();
        pConfig->getUrlExclude()->optimizeTree();
    }
}


// Parses the key and value given.  If key is storagepath, returns 1, otherwise returns 0
static int parseLine(CacheConfig *pConfig, int param_id,
                     const char *val, int valLen)
{
    int i = param_id;
    int bit;
    int minValue = 0;
    int maxValue = 1;
    int defValue = 0;

    switch (i)
    {
    case 0:
        bit = CACHE_ENABLE_PUBLIC;
        break;
    case 1:
        bit = CACHE_ENABLE_PRIVATE;
        break;
    case 2:
        bit = CACHE_CHECK_PUBLIC;
        defValue = 1;
        break;
    case 3:
        bit = CACHE_CHECK_PRIVATE;
        defValue = 1;
        break;
    case 4:
        bit = CACHE_QS_CACHE;
        defValue = 1;
        break;
    case 5:
        bit = CACHE_REQ_COOKIE_CACHE;
        defValue = 1;
        break;
    case 6:
        bit = CACHE_IGNORE_REQ_CACHE_CTRL_HEADER;
        defValue = 1;
        break;
    case 7:
        bit = CACHE_IGNORE_RESP_CACHE_CTRL_HEADER;
        defValue = 0;
        break;
    case 8:
        bit = CACHE_RESP_COOKIE_CACHE;
        defValue = 1;
        break;
    case 9:
        bit = CACHE_MAX_AGE_SET;
        maxValue = INT_MAX;
        break;
    case 10:
        bit = CACHE_PRIVATE_AGE_SET;
        maxValue = INT_MAX;
        break;
    case 11:
        bit = CACHE_STALE_AGE_SET;
        maxValue = INT_MAX;
        break;
    case 12:
        bit = CACHE_MAX_OBJ_SIZE;
        maxValue = INT_MAX;
        defValue = 10000000; //10M
        break;

    case 13: //storagepath
    case 14:
    case 15:
    case 18:
    case 19:
        return i; //return the index for next step parsing

    case 16:
        bit = CACHE_NO_VARY;
        break;

    case 17:
        bit = CACHE_ADD_ETAG;
        maxValue = 2;
        defValue = 0;
        break;

    default:
        return 0;
    }

    int64_t value = strtoll(val, NULL, 10);

    if (value < minValue || value > maxValue)
        value = defValue;

    switch (bit)
    {
    case CACHE_MAX_AGE_SET:
        pConfig->setDefaultAge(value);
        break;
    case CACHE_STALE_AGE_SET:
        pConfig->setMaxStale(value);
        break;
    case CACHE_PRIVATE_AGE_SET:
        pConfig->setPrivateAge(value);
        break;
    case CACHE_MAX_OBJ_SIZE:
        pConfig->setMaxObjSize(value);
        break;
    case CACHE_ADD_ETAG:
        pConfig->setAddEtagType(value);
        break;
    default:
        break;
    }

    pConfig->setConfigBit(bit, value);
    return 0;
}


static void verifyStoreReady(CacheConfig *pConfig)
{
    if (pConfig->getStore())
        return ;

    pConfig->setStore(new DirHashCacheStore);
    if (pConfig->getStore())
    {
        char defaultCachePath[max_file_len];
        snprintf(defaultCachePath, sizeof(defaultCachePath), "%s%s",
                 g_api->get_server_root(), CACHEMODULEROOT);
        pConfig->getStore()->setStorageRoot((const char *)defaultCachePath);
        pConfig->getStore()->initManager();
        pConfig->setOwnStore(1);
        g_api->set_timer(10*1000, 1, house_keeping_cb, pConfig->getStore());
    }
    else
        g_api->log(NULL, LSI_LOG_ERROR,
                   "Cache verifyStoreReady failed to alloc memory.\n");
}

/**
 * return -1 error, ootherwise an index,
 * standard one is less than ot equal to HttpHeader::H_TRANSFER_ENCODING
 */
static int getVaryKeyIndex(CacheConfig *pConfig, const char *key, int len)
{
    int id = HttpHeader::getIndex(key, len);
    if (id > HttpHeader::H_TRANSFER_ENCODING)
    {
        id = -1; //ERROR case
        int offset = 0;
        StringList::iterator iter;
        if (pConfig->getVaryList())
        {
            for (iter = pConfig->getVaryList()->begin();
                 iter != pConfig->getVaryList()->end();
                 ++iter)
            {
                ++offset;
                if ((*iter)->len() == len
                    && strncasecmp(key, (*iter)->c_str(), len) == 0)
                {
                    id = HttpHeader::H_TRANSFER_ENCODING + offset;
                    break;
                }
            }
        }
    }
    return id;
}


static int setVaryList(CacheConfig *pConfig, const char *val, int valLen)
{
    StringList * pStringList = new StringList;
    if (!pStringList)
        return -1;

    pStringList->split(val, val + valLen, ",");

    int count = pStringList->size();
    if (count > 0)
    {
        /**
         * remove standard header from the list before save to config
         */
        StringList::iterator iter;
        for (iter = pStringList->begin(); iter != pStringList->end();)
        {
            int id = HttpHeader::getIndex((*iter)->c_str(), (*iter)->len());
            if (id <= HttpHeader::H_TRANSFER_ENCODING)
            {
                g_api->log(NULL, LSI_LOG_INFO, "[%s] needn't to add \"%.*s\" to"
                           " config which is standard request header.\n",
                           ModuleNameStr, (*iter)->len(), (*iter)->c_str());
                pStringList->erase(iter);
            }
            else
                 ++iter;
        }
        if (pStringList->size() > 0)
        {
            pConfig->setVaryList(pStringList);
            if (HttpHeader::H_TRANSFER_ENCODING + pStringList->size() > 32)
            {
                /**
                 * We use int32(32 bits) to indicate the flag of each bit, if
                 * total is more than 32 flags, it will lose those indexes
                 * over 32 flags
                 */
                g_api->log(NULL, LSI_LOG_ERROR, "[%s] config too many customised"
                            " varies in reqHeaderVary.\n", ModuleNameStr);
            }
            return 0;
        }
    }

    delete pStringList;
    pConfig->setVaryList(NULL);
    return 0;
}

static void *ParseConfig(module_param_info_t *param, int param_count,
                         void *_initial_config, int level, const char *name)
{
    CacheConfig *pInitConfig = (CacheConfig *)_initial_config;
    CacheConfig *pConfig = new CacheConfig;
    if (!pConfig)
        return NULL;

    pConfig->setLevel(level);
    pConfig->inherit(pInitConfig);
    if (!param || param_count == 0)
    {
        verifyStoreReady(pConfig);
        return (void *)pConfig;
    }

    for (int i=0 ;i<param_count; ++i)
    {
        int ret = parseLine(pConfig, param[i].key_index,
                            param[i].val, param[i].val_len);

        //Only server level need to match CHECKXXXX seetings
        if (ret == 2 && level == LSI_CFG_SERVER)
            pConfig->setConfigBit(CACHE_CHECK_PUBLIC,
                                  pConfig->isSet(CACHE_ENABLE_PUBLIC));
        else if (ret == 3 && level == LSI_CFG_SERVER)
            pConfig->setConfigBit(CACHE_CHECK_PRIVATE,
                              pConfig->isSet(CACHE_ENABLE_PRIVATE));
        else if (ret == 13)
            parseStoragePath(pConfig, param[i].val, param[i].val_len, level, name);
        else if (ret == 15)
            parseNoCacheUrl(pConfig, param[i].val, param[i].val_len, level, name);
        else if (ret == 14)
            parseNoCacheDomain(pConfig, param[i].val, param[i].val_len, level, name);
        else if (ret == 18)
            pConfig->setPurgeUri(param[i].val, param[i].val_len);
        else if (ret == 19)
            setVaryList(pConfig, param[i].val, param[i].val_len);

    }

    parseNoCacheUrlFinal(pConfig);
    verifyStoreReady(pConfig);
    return (void *)pConfig;
}


static void FreeConfig(void *_config)
{
    delete(CacheConfig *)_config;
}


/**
 * Comments: tag at least must contains 16 bytes
 * needQS should be 0 when the purge is from header, the uri already have the QS
 * and it should be 1 when it is from a PURGE / REFRESH request
 */
static void uriToTag(const lsi_session_t *session,
                     const char *uri, int uriLen,
                     int needQS, char *tag)
{
    const int maxUrlLength = 16384;
    char url[maxUrlLength] = { 0 };
    int urlLen = 0;
    char httpstr[4] = {0};
        
    int len = g_api->get_req_var_by_id(session, LSI_VAR_HTTPS,
                                        httpstr, 4);
    if (len == 2 && httpstr[1] == 'n')
    {
        memcpy(url + urlLen, "https://", 8);
        urlLen += 8;
    }
    else
    {
        memcpy(url + urlLen, "http://", 7);
        urlLen += 7;
    }   
    
    len = g_api->get_req_var_by_id(session, LSI_VAR_SERVER_NAME,
                                    url + urlLen, maxUrlLength - urlLen);
    urlLen += len;
    
    memcpy(url + urlLen, ":", 1);
    ++urlLen;
    
    len = g_api->get_req_var_by_id(session, LSI_VAR_SERVER_PORT,
                                    url + urlLen, maxUrlLength - urlLen);
    urlLen += len;
    
    if (maxUrlLength - urlLen > uriLen)
    {
        /**
            * The copy will copy the URI and the QS
            */
        memcpy(url + urlLen, uri, uriLen);
        urlLen += uriLen;
    }
    else
    {
        g_api->log(session, LSI_LOG_ERROR, "uriToTag() Url length error.");
        return ;
    }

    if (needQS)
    {
        const char *pQs = g_api->get_req_query_string(session, &len);
        if (len > 0)
        {
            *(url + urlLen) = '?';
            ++urlLen;
            memcpy(url + urlLen, pQs, len);
            urlLen += len;
        }
    }

    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, url, urlLen);
    unsigned long long md = XXH64_digest(&state);
    StringTool::hexEncode((const char *)&md, 8, tag);
    tag[16] = 0x00;  //Must add null terminate
}


static int testFlagWithShm(const lsi_session_t *session,
                           MyMData *myData, int32_t flag)
{
    CacheManager *pManager = myData->pConfig->getStore()->getManager();
    int32_t id = pManager->getUrlVaryId(myData->pOrgUri, strlen(myData->pOrgUri));

    g_api->log(session, LSI_LOG_DEBUG,
               "[%s]testFlagWithShm() flag %d, id in shm %d.\n",
               ModuleNameStr, flag, id);
    /**
     * For not exist case (-1), treat as 0
     */
    if (id == -1)
        id = 0;

    if (flag == id)
        return 0;

    if (flag <= 0)
        pManager->delUrlVary(myData->pOrgUri, strlen(myData->pOrgUri));
    else
        pManager->addUrlVary(myData->pOrgUri, strlen(myData->pOrgUri), flag);
    return 1;
}



static int32_t getVaryFlag(const lsi_session_t *session, CacheConfig *pConfig)
{
    int32_t flag = 0;
    struct iovec iov[1] = {{NULL, 0}};
    int count = g_api->get_resp_header(session, LSI_RSPHDR_VARY, NULL,
                                       0, iov,1);
    if (count == 1 && iov[0].iov_len > 0)
    {
        StringList stringList;
        stringList.split((char *)iov[0].iov_base,
                         (char *)iov[0].iov_base + iov[0].iov_len, ",");

        StringList::iterator iter;
        for (iter = stringList.begin(); iter != stringList.end(); ++iter)
        {
            int index = getVaryKeyIndex(pConfig, (*iter)->c_str(),
                                        (*iter)->len());
            if (index <= -1)
            {
                g_api->log(session, LSI_LOG_WARN, "[%s] vary request header"
                            " \"%.*s\" not defined!\n",
                           ModuleNameStr, (*iter)->len(), (*iter)->c_str());
                return LS_FAIL;
            }
            flag |= (1 << index);
        }
    }

    return flag;
}



static int isUrlExclude(const lsi_session_t *session, CacheConfig *pConfig,
                        const char *url, int urlLen)
{
    Aho *ahos[2] = {pConfig->getUrlExclude(), pConfig->getParentUrlExclude()};
    int count = 2;
    if (pConfig->isOnlyUseOwnUrlExclude())
        count = 1;

    for (int i = 0; i < count ; ++i)
    {
        if (ahos[i])
        {
            size_t out_start, out_end;
            AhoState *out_last_state;
            int ret = ahos[i]->search(NULL, url, urlLen, 0,
                                      &out_start, &out_end,
                                      &out_last_state, NULL);
            if (ret)
            {
                g_api->log(session, LSI_LOG_INFO, "Cache excluded by URL: %.*s\n",
                           urlLen, url);
                return 1;
            }
        }
    }
    return 0;
}


static int uninitZstream(z_stream *zstream, bool compress)
{
    if (compress)
        deflateEnd(zstream);
    else
        inflateEnd(zstream);
    delete zstream;
    return 0;
}


/**
 * return 0 for OK, -1 for error
 */
static int initZstream(z_stream *zstream, bool compress)
{
    zstream->opaque = Z_NULL;
    zstream->zalloc = Z_NULL;
    zstream->zfree = Z_NULL;
    zstream->avail_in = 0;
    zstream->next_in = Z_NULL;
    if (compress)
    {
        if (deflateInit2(zstream, Z_BEST_COMPRESSION, Z_DEFLATED,
                16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        {
            return -1;
        }
    }
    else
    {
        if (inflateInit2(zstream, 32 + MAX_WBITS) != Z_OK)
        {
            return -1;
        }
    }

    return 0;
}


static int compressbuf(z_stream *zstream, bool compress, unsigned char *buff,
                    int buffLen, int fd, int eof)
{
    unsigned char buf[Z_BUF_SIZE];
    zstream->next_in = buff;
    zstream->avail_in = buffLen;
    int ret = 0;
    int z_ret;

    do
    {
        zstream->next_out = buf;
        zstream->avail_out = Z_BUF_SIZE;

        if (compress)
            z_ret = deflate(zstream, eof ? Z_FINISH : Z_SYNC_FLUSH);
        else
            z_ret = inflate(zstream, eof ? Z_FINISH : Z_PARTIAL_FLUSH);

        if (z_ret == Z_STREAM_ERROR)
            return LS_FAIL;

        if (z_ret == Z_BUF_ERROR)
            z_ret = Z_OK;

        if (z_ret >= Z_OK)
            ret += write(fd, buf, Z_BUF_SIZE - zstream->avail_out);
    } while (z_ret != Z_STREAM_END);

    return ret;
}

static int isDomainExclude(const lsi_session_t *session, CacheConfig *pConfig)
{
    if (pConfig->getVHostMapExclude())
    {
        HttpSession *pSession = (HttpSession *)session;
        HttpReq *pReq = pSession->getReq();
        const HttpVHost *pVHost = pReq->matchVHost(pConfig->getVHostMapExclude());
        if (pVHost)
        {
            g_api->log(session, LSI_LOG_INFO, "Cache excluded by domain.\n");
            return 1;
        }
    }
    return 0;
}


static char *copyCookie0(char *p, char *pDestEnd, const char *pCookie, int n)
{
    if (n > (pDestEnd - p - 1))
        n = pDestEnd - p - 1;
    if (n > 0)
    {
        memmove(p, pCookie, n);
        p += n;
        *p++ = ';';
    }
    return p;
}


static char *copyCookie(char *p, char *pDestEnd,
                        const cookieval_t *pIndex, const char *pCookie)
{
    int n = pIndex->valLen + pIndex->valOff - pIndex->keyOff;
    return copyCookie0(p, pDestEnd, pCookie, n);
}


int getPrivateCacheCookie(HttpReq *pReq, char *pDest, char *pDestEnd)
{
    const cookieval_t *pIndex;
    pReq->parseCookies();
    if (pReq->getCookieList().getSize() == 0)
    {
        *pDest = 0;
        return 0;
    }

    char *p = pDest;
    if (pReq->getCookieList().isSessIdxSet())
    {
        pIndex = pReq->getCookieList().getObj(pReq->getCookieList().getSessIdx());
        //assert( (pIndex->flag & COOKIE_FLAG_SESSID) != 0 );
        if (!pIndex)
        {
            g_api->log(NULL, LSI_LOG_ERROR, "[%s]CookieList error, idx %d sizenow %d, objsize %d\n",
                       ModuleNameStr,
                       pReq->getCookieList().getSessIdx(),
                       pReq->getCookieList().getSize(),
                       pReq->getCookieList().getObjSize());
            *pDest = 0;
            return 0;
        }

        p = copyCookie(p, pDestEnd, pIndex,
                       pReq->getHeaderBuf().getp(pIndex->keyOff));
        *p = 0;
        return p - pDest;
    }
    pIndex = pReq->getCookieList().begin();
    const char *pCookie;
    int skip;
    while ((pIndex < pReq->getCookieList().end()) && (p < pDestEnd))
    {
        pCookie = pReq->getHeaderBuf().getp(pIndex->keyOff);
        skip = 0;
        if ((*pCookie == '_') && (*(pCookie + 1) == '_'))
            skip = 1;
        else if ((strncmp("has_js=", pCookie, 7) == 0)
                 || (strncmp("_lscache_vary", pCookie, 13) == 0)
                 || (strncmp("bb_forum_view=", pCookie, 14) == 0))
            skip = 1;
        else if (strncmp("frontend=", pCookie, 9) == 0)
        {
            int n = pIndex->valLen + pIndex->keyLen + 2;
            if ((p - pDest >= n)
                && (memcmp(pCookie, p - n, n - 1) == 0))
                skip = 1;
        }
        if (!skip)
            p = copyCookie(p, pDestEnd, pIndex, pCookie);
        ++pIndex;
    }
    *p = '\0';
    return p - pDest;
}


char *appendVaryCookie(HttpReq *pReq, const char *pCookeName, int len,
                       char *pDest, char *pDestEnd)
{
    cookieval_t *pIndex = pReq->getCookie(pCookeName, len);
    if (pIndex)
    {
        const char *pCookie = pReq->getHeaderBuf().getp(pIndex->keyOff);
        pDest = copyCookie(pDest, pDestEnd, pIndex, pCookie);
    }
    else
    {
        pDest = copyCookie0(pDest, pDestEnd, pCookeName, len);
    }
    return pDest;
}


char *scanVaryOnList(HttpReq *pReq, const char *pListBegin,
                     const char *pListEnd, char *pDest, char *pDestEnd)
{
    while (pListBegin < pListEnd)
    {
        const char *pVary = pListBegin;
        while (pVary < pListEnd && isspace(*pVary))
            ++pVary;
        if (strncasecmp(pVary, "cookie=", 7) == 0)
            pVary += 7;
        const char *pVaryEnd = strchr(pVary, ',');
        if (!pVaryEnd)
        {
            pVaryEnd = pListEnd;
            pListBegin = pListEnd;
        }
        else
            pListBegin = pVaryEnd + 1;
        if (pVaryEnd - pVary > 0)
        {
            pDest = appendVaryCookie(pReq, pVary, pVaryEnd - pVary,
                                     pDest, pDestEnd);
        }
    }
    return pDest;
}


int getCacheVaryCookie(const lsi_session_t *session, HttpReq *pReq,
                       char *pDest, char *pDestEnd)
{
    pReq->parseCookies();

    char *p = pDest;
    p = appendVaryCookie(pReq, "_lscache_vary", 13, p, pDestEnd);

    MyMData *myData = (MyMData *)g_api->get_module_data(session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData)
    {
        const char *pBegin = NULL;
        const char *pEnd = NULL;
        if (myData->pCacheCtrlVary)
        {
            pBegin = myData->pCacheCtrlVary->c_str();
            pEnd = myData->pCacheCtrlVary->c_str() + myData->pCacheCtrlVary->len();
            p = scanVaryOnList(pReq, pBegin, pEnd, p, pDestEnd);
        }
        if (myData->pCacheVary)
        {
            pBegin = myData->pCacheVary->c_str();
            pEnd = myData->pCacheVary->c_str() + myData->pCacheVary->len();
            p = scanVaryOnList(pReq, pBegin, pEnd, p, pDestEnd);
        }
    }

    *p = '\0';
    return p - pDest;
}


void calcCacheHash2(const lsi_session_t *session, CacheKey *pKey,
                    CacheHash *pHash, CacheHash *pPrivateHash)
{
//    CACHE_PRIVATE_KEY;
    XXH64_state_t state;
    int len;
    const char *host = g_api->get_req_header_by_id(session, LSI_HDR_HOST,
                       &len);
    XXH64_reset(&state, 0);
    XXH64_update(&state, host, len);

    char port[12] = ":";
    g_api->get_req_var_by_id(session, LSI_VAR_SERVER_PORT, port + 1, 10);
    XXH64_update(&state, port, strlen(port));

//     len = g_api->get_req_var_by_id(session, LSI_VAR_SSL_VERSION, env, 12);
//     if (len > 3)  //SSL
//         XXH64_update(&state, "!", 1);

    XXH64_update(&state, pKey->m_pUri, pKey->m_iUriLen);

    if (pKey->m_iQsLen > 0)
    {
        XXH64_update(&state, "?", 1);
        XXH64_update(&state, pKey->m_pQs, pKey->m_iQsLen);
    }

    if (pKey->m_iCookieVary > 0)
    {
        XXH64_update(&state, "#", 1);
        XXH64_update(&state, pKey->m_sCookie.c_str(), pKey->m_iCookieVary);
    }
    pHash->setKey(XXH64_digest(&state));



    if (pKey->m_iCookiePrivate > 0)
    {
        XXH64_update(&state, "~", 1);
        XXH64_update(&state, pKey->m_sCookie.c_str() + pKey->m_iCookieVary,
                        pKey->m_iCookiePrivate);
    }
    if (pKey->m_pIP)
    {
        XXH64_update(&state, "@", 1);
        XXH64_update(&state, pKey->m_pIP, pKey->m_ipLen);
    }
    pPrivateHash->setKey(XXH64_digest(&state));
}


// void calcCacheHash(const lsi_session_t *session, CacheKey *pKey,
//                    CacheHash *pHash, CacheHash *pPrivateHash)
// {
//     HttpSession *pSession = (HttpSession *)session;
//     HttpReq *pReq = pSession->getReq();
//     if (pReq->getContextState(CACHE_PRIVATE_KEY))
//         return;
//     if ((!pKey->m_pIP) && (pReq->getContextState(CACHE_KEY)))
//         return;
//
//     calcCacheHash2(session, pKey, pHash, pPrivateHash);
//     if (pKey->m_pIP)
//         pReq->orContextState(CACHE_PRIVATE_KEY | CACHE_KEY);
//     else
//         pReq->orContextState(CACHE_KEY);
// }


int appBufToCacheKey(const lsi_session_t *session, const char *buf,
                     int len, char *buffer, int maxLen)
{
    if (buf ==NULL || len <= 0)
        return 0;

    if (len <= maxLen)
        memcpy(buffer, buf, len);
    else
    {
        g_api->log(session, LSI_LOG_ERROR,
                   "[CACHE] buildCacheKey exceed limit of CookieBuf.\n");
        return -1;
    }

    return 0;
}


void buildCacheKey(const lsi_session_t *session, const char *uri, int uriLen,
                   int noVary, CacheKey *pKey, int useCurQS)
{
    int iQSLen;
    int ipLen;
    char pCookieBuf[MAX_HEADER_LEN] = {0};
    char *pCookieBufEnd = pCookieBuf + MAX_HEADER_LEN;
    const char *pIp = g_api->get_client_ip(session, &ipLen);
    const char *pQs = useCurQS ? g_api->get_req_query_string(session, &iQSLen) : "";
    HttpSession *pSession = (HttpSession *)session;
    HttpReq *pReq = pSession->getReq();
    CacheConfig *pConfig = (CacheConfig *)g_api->get_config(session, &MNAME);

    pKey->m_pIP = pIp;
    pKey->m_ipLen = ipLen;
    if (noVary)
        pKey->m_iCookieVary = 0;
    else
    {
        pKey->m_iCookieVary = getCacheVaryCookie(session, pReq, pCookieBuf,
                              pCookieBufEnd);

        int len;
        const char *buf;
        CacheManager *pManager = pConfig->getStore()->getManager();
        int32_t flag = pManager->getUrlVaryId(uri, uriLen);

        if (flag > 0)
        {
            if (flag % (1 << HttpHeader::H_TRANSFER_ENCODING) != 0)
            {
                for (int id = 0; id < HttpHeader::H_TRANSFER_ENCODING; ++id)
                {
                    if (flag & 1 << id)
                    {
                        //HttpHeader::getHeaderName(id)
                        buf = g_api->get_req_header_by_id(session, id, &len);
                        if (len > 0 && buf)
                        {
                            int ret = appBufToCacheKey(session,
                                                       buf,
                                                       len,
                                                       pCookieBuf + pKey->m_iCookieVary,
                                                       MAX_HEADER_LEN - pKey->m_iCookieVary);
                            if (ret)
                            {
                                /**
                                 * If not enough space, bypass the part and continue
                                 */
                                break;
                            }
                            else
                                pKey->m_iCookieVary += len;
                        }
                    }
                }
            }

            if (pConfig->getVaryList() && flag >> HttpHeader::H_TRANSFER_ENCODING != 0)
            {
                int id = HttpHeader::H_TRANSFER_ENCODING;
                StringList::iterator iter;
                for (iter = pConfig->getVaryList()->begin();
                     iter != pConfig->getVaryList()->end();
                     ++iter)
                {
                    if (flag & 1 << id)
                    {
                        buf = g_api->get_req_header(session, (*iter)->c_str(),
                                                            (*iter)->len(), &len);
                        int ret = appBufToCacheKey(session, buf,len,
                                                   pCookieBuf + pKey->m_iCookieVary,
                                                   MAX_HEADER_LEN - pKey->m_iCookieVary);
                        if (ret)
                        {
                            /**
                             * If not enough space, bypass the part and continue
                             */
                            break;
                        }
                        else
                            pKey->m_iCookieVary += len;
                    }

                    ++id;
                }
            }
        }
    }

    if (pIp)
        pKey->m_iCookiePrivate = getPrivateCacheCookie(pReq,
                                 &pCookieBuf[pKey->m_iCookieVary],
                                 pCookieBufEnd);
    else
        pKey->m_iCookiePrivate = 0;

    pKey->m_pUri = uri;
    pKey->m_iUriLen = uriLen;
    pKey->m_pQs = pQs;
    pKey->m_iQsLen = iQSLen;
    pKey->m_sCookie.setStr(pCookieBuf);
}

static void dumpCacheKey(const lsi_session_t *session, const CacheKey *pKey)
{
    g_api->log(session, LSI_LOG_DEBUG,
           "[CACHE] CacheKey data: URI [%.*s], "
           "QS [%.*s], Vary Cookie [%.*s], "
           "Private Cookie [%.*s], IP [%s]\n",
           pKey->m_iUriLen, pKey->m_pUri,
           pKey->m_iQsLen,  pKey->m_pQs ? pKey->m_pQs : "",
           pKey->m_iCookieVary, pKey->m_sCookie.c_str() ? pKey->m_sCookie.c_str() : "",
           pKey->m_iCookiePrivate,
           pKey->m_sCookie.c_str() ? pKey->m_sCookie.c_str() + pKey->m_iCookieVary : "",
           pKey->m_pIP);
}


static void dumpCacheHash(const lsi_session_t *session, const char *desc,
                          const CacheHash *pHash)
{
    g_api->log(session, LSI_LOG_DEBUG,
           "[CACHE] %s, hash: [%02x%02x%02x%02x%02x%02x%02x%02x]\n",
           desc,
           pHash->getKey()[0], pHash->getKey()[1],
           pHash->getKey()[2], pHash->getKey()[3],
           pHash->getKey()[4], pHash->getKey()[5],
           pHash->getKey()[6], pHash->getKey()[7]
          );
}

off_t getEntryContentLength(MyMData *myData)
{
    int part1offset = myData->pEntry->getPart1Offset();
    int part2offset = myData->pEntry->getPart2Offset();
    if (part2offset - part1offset > 0)
    {
        off_t length = myData->pEntry->getContentTotalLen() -
                       (part2offset - part1offset);
        return length;
    }
    else
        return -1;
}



short lookUpCache(lsi_param_t *rec, MyMData *myData, int no_vary,
                  const char *uri, int uriLen,
                  DirHashCacheStore *pDirHashCacheStore,
                  CacheHash *cePublicHash, CacheHash *cePrivateHash,
                  CacheConfig *pConfig, CacheEntry **pEntry, bool doPublic)
{
    buildCacheKey(rec->session, uri, uriLen, no_vary, &myData->cacheKey, 1);
    dumpCacheKey(rec->session, &myData->cacheKey);
    calcCacheHash2(rec->session, &myData->cacheKey, cePublicHash,
                  cePrivateHash);
    dumpCacheHash(rec->session, "Public hash", cePublicHash);
    dumpCacheHash(rec->session, "Private hash", cePrivateHash);

    long lastCacheFlush = (long)g_api->get_module_data(rec->session, &MNAME,
                          LSI_DATA_IP);

    *pEntry = pDirHashCacheStore->getCacheEntry(*cePrivateHash,
              &myData->cacheKey, pConfig->getMaxStale(), lastCacheFlush);
    if (*pEntry && (!(*pEntry)->isStale() || (*pEntry)->isUpdating())
        && !(*pEntry)->isUnderConstruct())
        return CE_STATE_HAS_PRIVATE_CACHE;

    if (doPublic)
    {
        //Second, check if can use public one
        //Attemp to set the ipLen to negative number for checking public cache
        int savedIpLen = myData->cacheKey.m_ipLen;
        myData->cacheKey.m_ipLen = 0 - savedIpLen;
        *pEntry = pDirHashCacheStore->getCacheEntry(*cePublicHash,
                  &myData->cacheKey, pConfig->getMaxStale(), -1);
        myData->cacheKey.m_ipLen = savedIpLen;
        if (*pEntry)
        {
            if ((*pEntry)->isStale() && !(*pEntry)->isUpdating())
            {
                CacheEntry *pNewEntry = myData->pConfig->getStore()->
                                        createCacheEntry(myData->cePublicHash,
                                        &myData->cacheKey);
                if (pNewEntry)
                {
                    myData->pEntry = pNewEntry;
                    return CE_STATE_UPDATE_STALE;
                }
                else
                {

                    g_api->log(rec->session, LSI_LOG_ERROR,
                               "[%s] createEntry failed for update stale.\n",
                               ModuleNameStr);
                }
            }
            
            if (!(*pEntry)->isUnderConstruct())
                return CE_STATE_HAS_PUBLIC_CACHE;
            else
                return CE_STATE_NOCACHE;
        }
    }

    if (*pEntry)
        (*pEntry)->setStale(1);

    return CE_STATE_NOCACHE;
}


static int releaseMData(void *data)
{
    MyMData *myData = (MyMData *)data;
    if (myData)
    {
        if (myData->pOrgUri)
            delete []myData->pOrgUri;

        if (myData->zstream)
        {
            deflateEnd(myData->zstream);
            delete myData->zstream;
        }
        if (myData->pCacheCtrlVary)
            delete myData->pCacheCtrlVary;

        if (myData->pCacheVary)
            delete myData->pCacheVary;
        memset(myData, 0, sizeof(MyMData));
        delete myData;
    }
    return 0;
}


int checkBypassHeader(const char *header, int len)
{
    const char *headersBypass[] =
    {
        "last-modified",
        "etag",
        "date",
        "content-length",
        "transfer-encoding",
        "content-encoding",
        "set-cookie",
        "x-litespeed-cache", //bypass itself, otherwise always value is 'miss'
    };
    int8_t headersBypassLen[] = {  13, 4, 4, 14, 17, 16, 10, 17, };

    int count = sizeof(headersBypass) / sizeof(const char *);
    for (int i = 0; i < count ; ++i)
    {
        if (len == headersBypassLen[i]
            && strncasecmp(headersBypass[i], header, headersBypassLen[i]) == 0)
            return 1;
    }

    return 0;
}


int writeHttpHeader(int fd, AutoStr2 *str, const char *key, int key_len,
                    const char *val, int val_len)
{
    write(fd, key, key_len);
    write(fd, ": ", 2);
    if (val_len > 0)
        write(fd, val, val_len);
    write(fd, "\r\n", 2);

#ifdef CACHE_RESP_HEADER
    str->append(key, key_len);
    str->append(": ", 2);
    str->append(val, val_len);
    str->append("\r\n", 2);

#endif

    return key_len + val_len + 4;
}

static void parseEnv(const lsi_session_t *session, CacheCtrl &cacheCtrl)
{
    char cacheEnv[MAX_CACHE_CONTROL_LENGTH] = {0};
    int cacheEnvLen = g_api->get_req_env(session, cache_env_key,
                                         icache_env_key,
                                         cacheEnv,
                                         MAX_CACHE_CONTROL_LENGTH);
    if (cacheEnvLen > 0)
        cacheCtrl.parse((const char *)cacheEnv, cacheEnvLen);

}

void getRespHeader(const lsi_session_t *session, int header_index, char **buf,
                   int *length)
{
    struct iovec iov[1] = {{NULL, 0}};
    int iovCount = g_api->get_resp_header(session, header_index, NULL, 0, iov,
                                          1);
    if (iovCount == 1)
    {
        *buf = (char *)iov[0].iov_base;
        *length = iov[0].iov_len;
    }
    else
    {
        *buf = NULL;
        *length = 0;
    }
}

static int bypassUrimapHook(lsi_param_t *rec, MyMData *myData)
{
#ifdef USE_RECV_REQ_HEADER_HOOK
    if (g_api->get_hook_level(rec) == LSI_HKPT_RCVD_REQ_HEADER)
    {
        int disableHkpt = LSI_HKPT_URI_MAP;
        g_api->enable_hook(rec->session, &MNAME, 0, &disableHkpt, 1);
    }
#endif

    if (myData)
        g_api->free_module_data(rec->session, &MNAME, LSI_DATA_HTTP, releaseMData);
    return 0;
}

static void disableRcvdRespHeaderFilter(const lsi_session_t *session)
{
    int disableHkpt = LSI_HKPT_RCVD_RESP_HEADER;
    g_api->enable_hook(session, &MNAME, 0, &disableHkpt, 1);
}


void clearHooksOnly(const lsi_session_t *session)
{
    int aHkpts[4], iHkptCount = 0;
    MyMData *myData = (MyMData *) g_api->get_module_data(session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData)
    {
        if (myData->iHaveAddedHook == 1 || myData->iHaveAddedHook == 2)
        {
            if (myData->iHaveAddedHook == 2)
                aHkpts[iHkptCount++] = myData->hkptIndex;
            aHkpts[iHkptCount++] = LSI_HKPT_RCVD_RESP_HEADER;
            g_api->enable_hook(session, &MNAME, 0, aHkpts, iHkptCount);
            myData->iHaveAddedHook = 0;
        }
    }
}


void clearHooks(const lsi_session_t *session)
{
    clearHooksOnly(session);
    g_api->free_module_data(session, &MNAME, LSI_DATA_HTTP, releaseMData);
}


static int cancelCache(lsi_param_t *rec)
{
    MyMData *myData = (MyMData *)g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData != NULL)
    {
        if (myData->iCacheState == CE_STATE_WILLCACHE
            || myData->iCacheState == CE_STATE_UPDATE_STALE)
        {
            g_api->log(rec->session, LSI_LOG_DEBUG, "[%s]cache cancelled.\n",
                       ModuleNameStr);
            myData->pConfig->getStore()->cancelEntry(myData->pEntry, 1);
        }
        if (myData->zstream)
        {
            deflateEnd(myData->zstream);
            delete myData->zstream;
            myData->zstream = NULL;
        }
    }
    clearHooks(rec->session);
    return 0;
}

//Return the byts number wriiten to file fd
static int deflateBufAndWriteToFile(MyMData *myData, unsigned char *pBuf,
                                    int len, int eof, int fd)
{
    unsigned char buf[Z_BUF_SIZE];
    int ret = 0, rc;
    if (myData->zstream)
    {
        myData->zstream->avail_in = len;
        myData->zstream->next_in = pBuf;

        do
        {
            myData->zstream->avail_out = Z_BUF_SIZE;
            myData->zstream->next_out = buf;
            int z_ret = deflate(myData->zstream, eof ? Z_FINISH : Z_SYNC_FLUSH);
            if ((z_ret == Z_OK) || (z_ret == Z_STREAM_END))
            {
                rc = write(fd, buf, Z_BUF_SIZE - myData->zstream->avail_out);
                if (rc > 0)
                    ret += rc;
                else if (rc < 0)
                {
                    ret = -1;
                    break;
                }
                    
            }
        } while (myData->zstream->avail_out == 0);
    }
    else
    {
        rc = write(fd, pBuf, len);
        if (rc > 0)
            ret += rc;
        else if (rc < 0)
            ret = -1;
    }

    return ret;
}


static int endCache(lsi_param_t *rec)
{
    MyMData *myData = (MyMData *)g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData)
    {
        if (myData->hkptIndex)
        {
            //check if static file not optmized, or 0 byte content
            if (myData->orgFileLength == 0 ||
                myData->saveFailed ||
                (myData->pEntry->getHeader().m_lenStxFilePath > 0 &&
                 myData->orgFileLength == myData->pEntry->getHeader().m_lSize))
            {
                //Check if file optimized, if not, do not store it
                cancelCache(rec);
                g_api->log(rec->session, LSI_LOG_DEBUG,
                           "[%s]cache cancelled due to error occurred or no optimization.\n",
                           ModuleNameStr);
            }
            else if (myData->pEntry && myData->iCacheState == CE_STATE_WILLCACHE)
            {
                int fd = myData->pEntry->getFdStore();
                deflateBufAndWriteToFile(myData, NULL, 0, 1, fd);

                if (myData->pConfig->getAddEtagType() == 2)
                {
                    char s[17] = {0};
                    snprintf(s, 17, "%llx",
                             (long long)XXH64_digest(&myData->contentState));
                    //Update the HASH64

                    /**
                     * Add 1 to offset because Hash has a \" in the beginning
                     */
                    nio_lseek(fd, myData->pEntry->getPart1Offset() + 1,
                              SEEK_SET);
                    write(fd, s, 16);
                }

                myData->pConfig->getStore()->publish(myData->pEntry);
                myData->pConfig->getStore()->getManager()->addTracking(myData->pEntry);
                myData->iCacheState = CE_STATE_CACHED;  //Succeed
                g_api->log(NULL, LSI_LOG_DEBUG,
                           "[%s]published %s, content length %ld.\n",
                           ModuleNameStr, myData->pOrgUri,
                           myData->orgFileLength);
            }
        }
        return cancelCache(rec);
    }
    else
        return 0;
}


static int getControlFlag(CacheConfig *pConfig)
{
    int flag = CacheCtrl::max_age | CacheCtrl::max_stale;
    if (pConfig->isSet(CACHE_ENABLE_PUBLIC))
        flag |= CacheCtrl::cache_public;
    if (pConfig->isSet(CACHE_ENABLE_PRIVATE))
        flag |= CacheCtrl::cache_private;
    if (pConfig->isSet(CACHE_NO_VARY))
        flag |= CacheCtrl::no_vary;

    if ((flag & CacheCtrl::cache_public) == 0 &&
        (flag & CacheCtrl::cache_private) == 0)
        flag |= CacheCtrl::no_cache;

    return flag;
}


static void processPurge(const lsi_session_t *session,
                         const char *pValue, int valLen);
static int createEntry(lsi_param_t *rec)
{
    //If have special cache headers, handle them here even if myData is NULL.
    struct iovec iov[5];
    int count = g_api->get_resp_header(rec->session,
                                       LSI_RSPHDR_LITESPEED_PURGE,
                                       NULL, 0, iov, 5);
    for (int i = 0; i < count; ++i)
    {
        int valLen = iov[i].iov_len;
        const char *pVal = (const char *)iov[i].iov_base;
        if (pVal && valLen > 0)
            processPurge(rec->session, pVal, valLen);
    }

    MyMData *myData = (MyMData *)g_api->get_module_data(rec->session, &MNAME,
                                                        LSI_DATA_HTTP);
    if (myData == NULL || myData->iHaveAddedHook == 0)
    {
        clearHooks(rec->session);
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]createEntry quit, code 2.\n", ModuleNameStr);
        return 0;
    }

    if (count > 0 && myData->hasCacheFrontend == 0)
        g_api->remove_resp_header(rec->session, LSI_RSPHDR_LITESPEED_PURGE,
                                  NULL, 0);


    CacheConfig *pContextConfig = (CacheConfig *)g_api->get_config(
                                      rec->session, &MNAME);
    if ((pContextConfig != NULL) && pContextConfig != myData->pConfig)
    {
        int flag = getControlFlag(pContextConfig);
        myData->cacheCtrl.update(flag, pContextConfig->getDefaultAge(),
                                 pContextConfig->getMaxStale());
        myData->pConfig = pContextConfig;
    }


    count = g_api->get_resp_header(rec->session,
                    LSI_RSPHDR_LITESPEED_CACHE_CONTROL, NULL, 0, iov, 3);
    for (int i = 0; i < count; ++i)
        myData->cacheCtrl.parse((char *)iov[i].iov_base, iov[i].iov_len);

    /**
     * If no LSI_RSPHDR_LITESPEED_CACHE_CONTROL, then we can check
     * expireInSeconds = 0 case and check the normal cache-contrl max-age
     * and s-maxage
     */
    if (count == 0 && myData->cacheCtrl.getMaxAge() == 0)
    {
        count = g_api->get_resp_header(rec->session,
                    LSI_RSPHDR_CACHE_CTRL, NULL, 0, iov, 3);
        for (int i = 0; i < count; ++i)
            myData->cacheCtrl.parse((char *)iov[i].iov_base, iov[i].iov_len);
    }

    if (myData->cacheCtrl.isCacheOff())
    {
        clearHooks(rec->session);
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]createEntry abort, code 1.\n", ModuleNameStr);
        return 0;
    }



    //For a HEAD request, do not save it
    if (myData->iMethod == HTTP_HEAD)
    {
        clearHooks(rec->session);
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]cacheTofile to be cancelled for HEAD request.\n",
                   ModuleNameStr);
        return 0;
    }

    //if no LSI_RSPHDR_LITESPEED_CACHE_CONTROL and not 200, do nothing
    //if 304, do nothing
    int code = g_api->get_status_code(rec->session);
    if (code == 304 || (code != 200 && count == 0))
    {
        clearHooks(rec->session);
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]cacheTofile to be cancelled for error page, code=%d.\n",
                   ModuleNameStr, code);
        return 0;
    }

    if (count && myData->hasCacheFrontend == 0)
    {
        g_api->remove_resp_header(rec->session,
                                  LSI_RSPHDR_LITESPEED_CACHE_CONTROL,
                                  NULL,
                                  0);
    }

    /**
     * TODO  should we do all case for test vary?
     */
    bool needRebuildCacheKey = false;
    int32_t iVaryFlag = getVaryFlag(rec->session, myData->pConfig);
    if (iVaryFlag == LS_FAIL)
    {
        /**
         * will stop the hook chain and cause 500 error
         */
        g_api->set_status_code(rec->session, 500);
        clearHooks(rec->session);
        g_api->log(rec->session, LSI_LOG_DEBUG,
                       "[%s]createEntry set 500 error and returned.\n",
                       ModuleNameStr);
        return 0;
    }

    count = g_api->get_resp_header(rec->session, -1, s_x_vary,
                                   sizeof(s_x_vary) - 1, iov, 1);
    if (iov[0].iov_len > 0 && count == 1)
    {
        g_api->set_req_env(rec->session, cache_env_key, icache_env_key,
                           (char *)iov[0].iov_base, iov[0].iov_len);
        needRebuildCacheKey = true;
    }

    /**
     * testFlagWithShm must to be called to update the SHM
     * even needRebuildCacheKey is TRUE
     */
    if (testFlagWithShm(rec->session, myData, iVaryFlag))
    {
        needRebuildCacheKey = true;
    }

    count = g_api->get_resp_header(rec->session, LSI_RSPHDR_SET_COOKIE, NULL,
                                   0, iov, 1);
    if (iov[0].iov_len > 0 && count == 1)
    {
        if (!myData->pConfig->isSet(CACHE_RESP_COOKIE_CACHE))
        {
            clearHooks(rec->session);
            g_api->log(rec->session, LSI_LOG_DEBUG,
                       "[%s]cacheTofile to be cancelled for having respcookie.\n",
                       ModuleNameStr);
            return 0;
        }
        else
            needRebuildCacheKey = true;
    }

    if (needRebuildCacheKey)
    {
        //since have set-cookie header, re-calculate the hash keys
        buildCacheKey(rec->session, myData->cacheKey.m_pUri,
                      myData->cacheKey.m_iUriLen,
                      myData->cacheCtrl.getFlags() & CacheCtrl::no_vary,
                      &myData->cacheKey, 1);
        calcCacheHash2(rec->session, &myData->cacheKey,
                      &myData->cePublicHash, &myData->cePrivateHash);

        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]CacheKey and hash updated.\n", ModuleNameStr);
        dumpCacheHash(rec->session, "Updated Public hash",
                      &myData->cePublicHash);
        dumpCacheHash(rec->session, "Updated Private hash",
                      &myData->cePrivateHash);
    }


    myData->hkptIndex = LSI_HKPT_RCVD_RESP_BODY;
    const char *phandlerType = g_api->get_req_handler_type(rec->session);
    if (phandlerType && strlen(phandlerType) == 6
        && memcmp("static", phandlerType, 6) == 0)
    {
        int flag1 = g_api->get_hook_flag(rec->session, LSI_HKPT_RECV_RESP_BODY);
        int flag2 = g_api->get_hook_flag(rec->session, LSI_HKPT_SEND_RESP_BODY);
        if (flag2 & (LSI_FLAG_TRANSFORM | LSI_FLAG_PROCESS_STATIC))
            myData->hkptIndex = LSI_HKPT_SEND_RESP_BODY;
        else  if (flag1 & (LSI_FLAG_TRANSFORM | LSI_FLAG_PROCESS_STATIC))
            myData->hkptIndex = LSI_HKPT_RCVD_RESP_BODY;
        else
        {
            clearHooks(rec->session);
            g_api->log(rec->session, LSI_LOG_DEBUG,
                       "[%s]cacheTofile to be cancelled for static file type.\n",
                       ModuleNameStr);
            return 0;
        }
    }


    if (myData->cacheCtrl.isCacheOff())
    {
        clearHooks(rec->session);
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s] createEntry abort due to cache is off.\n", ModuleNameStr);
        return 0;
    }

    if (myData->cacheCtrl.isPrivateCacheable())
        myData->pEntry = myData->pConfig->getStore()->createCacheEntry(
                             myData->cePrivateHash, &myData->cacheKey);
    else if (myData->cacheCtrl.isPublicCacheable())
    {
        if (myData->iCacheState != CE_STATE_UPDATE_STALE)
        {
            myData->pEntry = myData->pConfig->getStore()->createCacheEntry(
                                 myData->cePublicHash, &myData->cacheKey);
        }
    }

    if (myData->pEntry == NULL)
    {
        clearHooks(rec->session);
        g_api->log(rec->session, LSI_LOG_ERROR,
                   "[%s] createEntry failed.\n", ModuleNameStr);
        return 0;
    }

    dumpCacheKey(rec->session, &myData->cacheKey);
    dumpCacheHash(rec->session, "Create cache with hash",
                  &myData->pEntry->getHashKey());

    //Now we can store it
    myData->iCacheState = CE_STATE_WILLCACHE;

    int ids = myData->hkptIndex;
    g_api->enable_hook(rec->session, &MNAME, 1, &ids, 1);
    myData->iHaveAddedHook = 2;
    //When will cache store it, add miss header
    g_api->set_resp_header(rec->session, LSI_RSPHDR_UNKNOWN,
                           s_x_cached, sizeof(s_x_cached) - 1,
                           "miss", 4, LSI_HEADEROP_SET);

    return 0;
}


int cacheHeader(lsi_param_t *rec, MyMData *myData)
{
    myData->pEntry->setMaxStale(myData->pConfig->getMaxStale());
    g_api->log(rec->session, LSI_LOG_DEBUG,
               "[%s]save to %s cachestore by cacheHeader(), uri:%s\n", ModuleNameStr,
               ((myData->cacheCtrl.isPrivateCacheable()) ? "private" : "public"),
               myData->pOrgUri);

    int fd = myData->pEntry->getFdStore();
    char *sLastMod = NULL;
    char *sETag = NULL;
    int nLastModLen = 0;
    int nETagLen = 0;
    int headersBufSize = 0;
    char sTmpEtag[128] = { 0 };
    CeHeader &CeHeader = myData->pEntry->getHeader();
    CeHeader.m_tmCreated = (int32_t)DateTime_s_curTime;
    CeHeader.m_msCreated = (int32_t)DateTime_s_curTimeMs;
    CeHeader.m_tmExpire = CeHeader.m_tmCreated + myData->cacheCtrl.getMaxAge();


#ifdef USE_RECV_REQ_HEADER_HOOK
    char cacheEnv[MAX_CACHE_CONTROL_LENGTH] = {0};
    int cacheEnvLen = g_api->get_req_env(rec->session, "HAVE_REWITE", 11,
                                         cacheEnv, MAX_CACHE_CONTROL_LENGTH);
    if (cacheEnvLen >= 1 && strncasecmp(cacheEnv, "1", 1) ==  0){
        myData->pEntry->setNeedDelay(1);
    }
#endif

    getRespHeader(rec->session, LSI_RSPHDR_LAST_MODIFIED, &sLastMod,
                  &nLastModLen);
    if (sLastMod)
        CeHeader.m_tmLastMod = DateTime::parseHttpTime(sLastMod, nLastModLen);


    //Check if it is a static file caching
    char path[4096];
    int cur_uri_len;
    struct stat sb;
    const char *cur_uri = g_api->get_req_uri(rec->session, &cur_uri_len);
    int pathLen = g_api->get_file_path_by_uri(rec->session, cur_uri,
                  cur_uri_len, path, 4096);
    if (pathLen > 0 && stat(path, &sb) != -1)
        CeHeader.m_lenStxFilePath = pathLen;
    else
    {
        CeHeader.m_lenStxFilePath = 0;
        memset(&sb, 0, sizeof(struct stat));
    }

    CeHeader.m_lenETag = 0;
    CeHeader.m_offETag = 0;
    getRespHeader(rec->session, LSI_RSPHDR_ETAG, &sETag, &nETagLen);
    if (sETag && nETagLen > 0)
    {
        if (nETagLen > VALMAXSIZE)
            nETagLen = VALMAXSIZE;
        CeHeader.m_lenETag = nETagLen;
    }
    else if (myData->pConfig->getAddEtagType() == 1)  //size-mtime
    {
        if (CeHeader.m_lenStxFilePath > 0)
        {
            snprintf(sTmpEtag, 127, "\"%llx-%llx;;;\"",
                     (long long)sb.st_size, (long long)sb.st_mtime);
            sETag = sTmpEtag;
            nETagLen = strlen(sETag);
            CeHeader.m_lenETag = nETagLen;
        }
    }
    else if (myData->pConfig->getAddEtagType() == 2)  //XXH64_digest
    {
        sETag = (char *)"\"0000000000000000;;;\"";
        nETagLen = 16 + 5;
        CeHeader.m_lenETag = nETagLen;
        XXH64_reset(&myData->contentState, 0);
    }

    char *pKey = NULL;
    int keyLen;
    getRespHeader(rec->session, LSI_RSPHDR_LITESPEED_TAG, &pKey, &keyLen);
    char * tag = new char[(keyLen > 0 ? keyLen : 0) + 18];
    int offset = 0;
    if (pKey && keyLen > 0)
    {
        memcpy(tag, pKey, keyLen);
        memcpy(tag + keyLen, ",", 1);
        offset = keyLen + 1;
    }
   
   
    int uri_len = 0;
    const char *uri = g_api->get_req_uri(rec->session, &uri_len);
    uriToTag(rec->session, uri, uri_len, 1, tag + offset);
    
    myData->pEntry->setTag(tag, offset + 16);
    delete []tag;

    CeHeader.m_statusCode = g_api->get_status_code(rec->session);

    //Right now, set wrong numbers, and this will be fixed when publish
    myData->pEntry->setPart1Len(0); //->setContentLen(0, 0);
    myData->pEntry->setPart2Len(0);

    /**
     * If already gzipped, no need to gzip
     */
    int needGzip = (g_api->get_resp_buffer_compress_method(rec->session) == 0);

    /**
     * If the response not gzipped, and check if req need gzip,
     * if not needed, do not gzip it.
     */
    if (needGzip)
    {
        needGzip = (myData->reqCompressType == 1);
    }

    /**
     * If need to gzip but it is a small static file, no need
     * Or it is not compressible, no need
     */
    if (needGzip)
    {
        int contentTypelen;
        char *pContentType = NULL;
        getRespHeader(rec->session, LSI_RSPHDR_CONTENT_TYPE, &pContentType,
                      &contentTypelen);

        if (pContentType && contentTypelen > 0)
        {
            char ch = pContentType[contentTypelen];
            pContentType[contentTypelen] = 0;
            char compressible = HttpMime::getMime()->compressible(pContentType);
            pContentType[contentTypelen] = ch;
            if (!compressible)
                needGzip = false;
        }
    }

    const char *phandlerType = g_api->get_req_handler_type(rec->session);
    if (needGzip && phandlerType && strlen(phandlerType) == 6 &&
        memcmp("static", phandlerType, 6) == 0 &&
        sb.st_size > 0 && sb.st_size < 200)
        needGzip = false;

    /***
     * if it isn't a static file but has small size, no need to gzip
     */
    if (needGzip)
    {
        char *pVal = NULL;
        int valLen;
        getRespHeader(rec->session, LSI_RSPHDR_CONTENT_LENGTH, &pVal, &valLen);
        if (pVal && valLen > 0)
        {
            int len = 0;
            len = atoi(pVal);
            if (len >= 0 && len < 200)
                needGzip = false;
        }
    }

    if (needGzip)
    {
        myData->zstream = new z_stream;
        if (myData->zstream)
        {
            if (initZstream(myData->zstream, 1))
            {
                delete myData->zstream;
                myData->zstream = NULL;
                needGzip = false;
            }
        }
        else
            needGzip = false;
    }

    int compress_method = g_api->get_resp_buffer_compress_method(rec->session);
    if (compress_method == 0 && needGzip)
        compress_method  = 1;
    myData->pEntry->markReady(compress_method);

    myData->pEntry->saveCeHeader();

    if (CeHeader.m_lenETag > 0)
    {
        write(fd, sETag, CeHeader.m_lenETag);

#ifdef CACHE_RESP_HEADER
        myData->m_pEntry->m_sRespHeader.append(sETag, CeHeader.m_lenETag);
#endif
    }
    if (CeHeader.m_lenStxFilePath > 0)
        write(fd, path, CeHeader.m_lenStxFilePath);

    CeHeader.m_lSize = sb.st_size;
    CeHeader.m_inode = sb.st_ino;
    CeHeader.m_tmFileLastMod = sb.st_mtime;

    //Stat to write akll other headers

    int count = g_api->get_resp_headers_count(rec->session);
    if (count >= MAX_RESP_HEADERS_NUMBER)
        g_api->log(rec->session, LSI_LOG_WARN,
                   "[%s] too many resp headers [=%d]\n",
                   ModuleNameStr, count);

    struct iovec iov_key[MAX_RESP_HEADERS_NUMBER];
    struct iovec iov_val[MAX_RESP_HEADERS_NUMBER];
    count = g_api->get_resp_headers(rec->session, iov_key, iov_val,
                                    MAX_RESP_HEADERS_NUMBER);
    for (int i = 0; i < count; ++i)
    {
        //check if need to bypass
        if (!checkBypassHeader((const char *)iov_key[i].iov_base,
                               iov_key[i].iov_len))
        {
            //if it is lsc-cookie, then change to Set-Cookie
            const char *pKey = (const char *)iov_key[i].iov_base;
            if (iov_key[i].iov_len == 10 &&
                strncasecmp(pKey, "lsc-cookie", 10) == 0)
                pKey = "Set-Cookie";

#ifdef CACHE_RESP_HEADER
            headersBufSize += writeHttpHeader(fd, &(myData->m_pEntry->m_sRespHeader),
                                              pKey, iov_key[i].iov_len,
                                              iov_val[i].iov_base, iov_val[i].iov_len);
#else
            headersBufSize += writeHttpHeader(fd, NULL,
                                              pKey, iov_key[i].iov_len,
                                              (char *)iov_val[i].iov_base, iov_val[i].iov_len);
#endif
        }
    }

#ifdef CACHE_RESP_HEADER
    if (myData->m_pEntry->m_sRespHeader.len() > 4096)
        myData->m_pEntry->m_sRespHeader.prealloc(0);
#endif

    myData->pEntry->setPart1Len(CeHeader.m_lenETag + CeHeader.m_lenStxFilePath
                                +
                                headersBufSize);
    return 0;
}


int cacheTofile(lsi_param_t *rec)
{
    MyMData *myData = (MyMData *)g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);
    if (!myData)
        return 0;

    if (myData->iCacheSendBody == 0) //uninit
    {
        myData->iCacheSendBody = 1; //need cache
        cacheHeader(rec, myData);
    }

    int fd = myData->pEntry->getFdStore();

    long iCahcedSize = 0;
    off_t offset = 0;
    const char *pBuf;
    int len = 0;
    void *pRespBodyBuf = g_api->get_resp_body_buf(rec->session);
    long maxObjSz = myData->pConfig->getMaxObjSize();
    if (maxObjSz > 0 && g_api->get_body_buf_size(pRespBodyBuf) > maxObjSz)
    {
        cancelCache(rec);
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s:cacheTofile] cache cancelled, body buffer size %ld > maxObjSize %ld\n",
                   ModuleNameStr, g_api->get_body_buf_size(pRespBodyBuf), maxObjSz);
        return 0;
    }

    int ret;
    while (fd != -1 && !myData->saveFailed && !g_api->is_body_buf_eof(pRespBodyBuf, offset))
    {
        len = 0;
        pBuf = g_api->acquire_body_buf_block(pRespBodyBuf, offset, &len);
        if (!pBuf || len <= 0)
            break;

        ret = deflateBufAndWriteToFile(myData, (unsigned char *)pBuf, len, 0, fd);
        if (ret == -1)
        {
            myData->saveFailed = 1;
            break;
        }
        if (myData->pConfig->getAddEtagType() == 2)
            XXH64_update(&myData->contentState, pBuf, len);
        g_api->release_body_buf_block(pRespBodyBuf, offset);
        offset += len;
        iCahcedSize += ret;
        myData->orgFileLength += len;
    }

    if (!myData->saveFailed && fd != -1)
    {
        ret = deflateBufAndWriteToFile(myData, NULL, 0, 1, fd);
        iCahcedSize += ret;
        myData->pEntry->setPart2Len(iCahcedSize);
        g_api->log(rec->session, LSI_LOG_DEBUG,
               "[%s:cacheTofile] stored, size %ld\n",
               ModuleNameStr, offset);
    }
    else
        g_api->log(rec->session, LSI_LOG_ERROR,
               "[%s:cacheTofile] Failed due to write file error!\n",
               ModuleNameStr);
    endCache(rec);
    
    return 0;
}


int cacheTofileFilter(lsi_param_t *rec)
{
    char cacheEnv[MAX_CACHE_CONTROL_LENGTH] = {0};
    int cacheEnvLen = g_api->get_req_env(rec->session, "cache-control", 13,
                                         cacheEnv, MAX_CACHE_CONTROL_LENGTH);
    if (cacheEnvLen == 8
        && strncasecmp(cacheEnv, "no-cache", cacheEnvLen) ==  0)
        return rec->len1;

    //Because Pagespeed module uses the non-blocking way to check if can handle
    //The optimized cache, it will have an eventCb to set the reqVar to notice
    //cache module to start to cahce, So have to check it here
    MyMData *myData = (MyMData *)g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);
    if (!myData || myData->saveFailed)
        return rec->len1;

    if (myData->iCacheSendBody == 0) //uninit
    {
        myData->iCacheSendBody = 1; //need cache
        cacheHeader(rec, myData);
    }
    int fd = myData->pEntry->getFdStore();
    int part2Len = myData->pEntry->getPart2Len();
    int ret = g_api->stream_write_next(rec, (const char *) rec->ptr1,
                                       rec->len1);
    if (ret > 0)
    {
        long maxObjSz = myData->pConfig->getMaxObjSize();
        if (maxObjSz > 0 && part2Len + ret > maxObjSz)
        {
            cancelCache(rec);
            g_api->log(rec->session, LSI_LOG_DEBUG,
                       "[%s:cacheTofileFilter] cache cancelled, current size to cache %d > maxObjSize %ld\n",
                       ModuleNameStr, part2Len + ret, maxObjSz);
            return ret;
        }


        int len = deflateBufAndWriteToFile(myData, (unsigned char *)rec->ptr1, ret, 0, fd);
        if (len == -1)
        {
            myData->saveFailed = 1;
            g_api->log(rec->session, LSI_LOG_ERROR,
               "[%s:cacheTofileFilter] Failed due a write error!\n",
               ModuleNameStr);
            return rec->len1;
        }
        else
        {
            if (myData->pConfig->getAddEtagType() == 2)
                XXH64_update(&myData->contentState, rec->ptr1, ret);

            myData->pEntry->setPart2Len(part2Len + len);
            myData->orgFileLength += ret;
            g_api->log(rec->session, LSI_LOG_DEBUG,
                    "[%s:cacheTofileFilter] stored, size %d, now part2len %d\n",
                    ModuleNameStr, len, part2Len + len);
        }
    }
    return ret; //rec->len1;
}


static int isReqCacheable(lsi_param_t *rec, CacheConfig *pConfig)
{
    if (!pConfig->isSet(CACHE_QS_CACHE))
    {
        int iQSLen;
        const char *pQS = g_api->get_req_query_string(rec->session, &iQSLen);
        if (pQS && iQSLen > 0)
        {
            g_api->log(rec->session, LSI_LOG_DEBUG,
                       "[%s]isReqCacheable return 0 for has QS but qscache disabled.\n",
                       ModuleNameStr);
            return 0;
        }
    }

    if (!pConfig->isSet(CACHE_REQ_COOKIE_CACHE))
    {
        int cookieLen;
        const char *pCookie = g_api->get_req_cookies(rec->session, &cookieLen);
        if (pCookie && cookieLen > 0)
        {
            g_api->log(rec->session, LSI_LOG_DEBUG,
                       "[%s]isReqCacheable return 0 for has reqcookie but reqcookie disabled.\n",
                       ModuleNameStr);
            return 0;
        }
    }

    return 1;
}


// int setCacheUserData(lsi_param_t *rec)
// {
//     const char *p = (char *)rec->ptr1;
//     int lenTotal = rec->len1;
//     const char *pEnd = p + lenTotal;
//
//     if (lenTotal < 8)
//         return LS_FAIL;
//
//
//     //Check the kvPair length matching
//     int len;
//     while (p < pEnd)
//     {
//         memcpy(&len, p, 4);
//         if (len < 0 || len > lenTotal)
//             return LS_FAIL;
//
//         lenTotal -= (4 + len);
//         p += (4 + len);
//     }
//
//     if (lenTotal != 0)
//         return LS_FAIL;
//
//
//     MyMData *myData = (MyMData *)g_api->get_module_data(rec->session, &MNAME,
//                       LSI_DATA_HTTP);
//     if (myData == NULL) //|| myData->iCacheState != CE_STATE_CACHED)
//         return LS_FAIL;
//
//     if (rec->len1 < 8)
//         return LS_FAIL;
//
//     myData->pEntry->m_sPart3Buf.append((char *)rec->ptr1, rec->len1);
//
//     g_api->log(rec->session, LSI_LOG_DEBUG,
//                "[%s:setCacheUserData] written %d\n", ModuleNameStr,
//                rec->len1);
//     return 0;
// }
//
//
// //get the part3 data
// int getCacheUserData(lsi_param_t *rec)
// {
//     MyMData *myData = (MyMData *)g_api->get_module_data(rec->session, &MNAME,
//                       LSI_DATA_HTTP);
//     if (myData == NULL)
//         return LS_FAIL;
//
//
// //     char **buf = (char **)rec->_param;
// //     int *len = (int *)(long)rec->_param_count;
// //     *buf = (char *)myData->pEntry->m_sPart3Buf.c_str();
// //     *len = myData->pEntry->m_sPart3Buf.len();
//
//     return 0;
// }


int getHttpMethod(lsi_param_t *rec, char *httpMethod)
{
    int methodLen = g_api->get_req_var_by_id(rec->session,
                    LSI_VAR_REQ_METHOD, httpMethod, 10);
    int method = HTTP_UNKNOWN;
    switch (methodLen)
    {
    case 3:
        if ((httpMethod[0] | 0x20) == 'g')   //"GET"
            method = HTTP_GET;
        break;
    case 4:
        if (strncasecmp(httpMethod, "HEAD", 4) == 0)
            method = HTTP_HEAD;
        else if (strncasecmp(httpMethod, "POST", 4) == 0)
            method = HTTP_POST;
        break;
    case 5:
        if (strncasecmp(httpMethod, "PURGE", 5) == 0)
            method = HTTP_PURGE;
        break;
    case 7:
        if (strncasecmp(httpMethod, "REFRESH", 7) == 0)
            method = HTTP_REFRESH;
        break;
    default:
        break;
    }

    return method;
}


static void checkFileUpdateWithCache(lsi_param_t *rec, MyMData *myData)
{
    CeHeader &CeHeader = myData->pEntry->getHeader();
    if (CeHeader.m_lenStxFilePath <= 0)
        return ;

    char path[4096] = {0};
    int fd = myData->pEntry->getFdStore();
    int ret = lseek(fd, myData->pEntry->getPart1Offset() + CeHeader.m_lenETag, 0);
    if (ret != -1)
        ret = read(fd, path, CeHeader.m_lenStxFilePath);
    if (ret != -1)
    {
        struct stat sb;
        if ((ret = stat(path, &sb)) != -1 &&
            CeHeader.m_lSize == sb.st_size &&
            CeHeader.m_inode == sb.st_ino &&
            CeHeader.m_tmFileLastMod == sb.st_mtime)
            return ;
    }
    myData->pConfig->getStore()->purge(myData->pEntry);
    g_api->log(rec->session, LSI_LOG_DEBUG,
               "[%s]cache destroyed for file [%s] %s.\n",
               ModuleNameStr, path, (ret == -1) ? "cache error" : "file changed");
    myData->iCacheState = CE_STATE_NOCACHE;
}

MyMData *createMData(lsi_param_t *rec)
{
    MyMData *myData = new MyMData;
    assert(myData);

    memset(myData, 0, sizeof(MyMData));
    g_api->set_module_data(rec->session, &MNAME, LSI_DATA_HTTP, (void *)myData);

    CacheConfig *pConfig = (CacheConfig *)g_api->get_config(
                               rec->session, &MNAME);
    int flag = getControlFlag(pConfig);
    myData->cacheCtrl.init(flag, pConfig->getDefaultAge(), pConfig->getMaxStale());
    myData->pConfig = pConfig;
    int uriLen = g_api->get_req_org_uri(rec->session, NULL, 0);
    if (uriLen > 0)
    {
        char host[512] = {0};
        int hostLen = g_api->get_req_var_by_id(rec->session,
                                               LSI_VAR_SERVER_NAME, host, 512);
        char port[12] = {0};
        int portLen = g_api->get_req_var_by_id(rec->session,
                                               LSI_VAR_SERVER_PORT, port, 12);

        //host:port uri
        char *uri = new char[uriLen + hostLen + portLen + 2];
        memcpy(uri, host, hostLen);
        uri[hostLen] = ':';
        memcpy(uri + hostLen + 1, port, portLen);
        g_api->get_req_org_uri(rec->session, uri + hostLen + 1 + portLen,
                               uriLen + 1);
        uriLen += (hostLen + 1 + portLen); //Set the the right uriLen
        uri[uriLen] = 0x00; //NULL terminated
        myData->pOrgUri = uri;
    }

    return myData;
}


static int sessionBegin(lsi_param_t *rec)
{
    MyMData *myData = (MyMData *) g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData)
    {
        g_api->free_module_data(rec->session, &MNAME, LSI_DATA_HTTP, releaseMData);
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s] sessionBegin released previous module data.\n",
                   ModuleNameStr);
    }
    return 0;
}

static int checkAssignHandler(lsi_param_t *rec)
{
    char val[3] = {0};
    if (g_api->get_req_env(rec->session, "staticcacheserve", 16, val, 1) > 0
        && strncasecmp(val, "1", 1) ==  0)
    {
        int aHkpts[] = {LSI_HKPT_URI_MAP, LSI_HKPT_HTTP_END,
                    LSI_HKPT_HANDLER_RESTART, LSI_HKPT_RCVD_RESP_HEADER};
        g_api->enable_hook(rec->session, &MNAME, 0, aHkpts, 4);

        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s] checkAssignHandler return since req served by internal cache.\n",
                   ModuleNameStr);
        return 0;
    }

    MyMData *myData = (MyMData *) g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);

    CacheConfig *pConfig = (CacheConfig *)g_api->get_config(
                               rec->session, &MNAME);
    if (!pConfig)
    {
        g_api->log(rec->session, LSI_LOG_ERROR,
                   "[%s]checkAssignHandler error 2.\n", ModuleNameStr);
        return bypassUrimapHook(rec, myData);
    }

    int uriLen = g_api->get_req_org_uri(rec->session, NULL, 0);
    if (uriLen <= 0)
    {
        g_api->log(rec->session, LSI_LOG_ERROR,
                   "[%s]checkAssignHandler error 1.\n", ModuleNameStr);
        return bypassUrimapHook(rec, myData);
    }

    int cur_uri_len;
    const char *cur_uri = g_api->get_req_uri(rec->session, &cur_uri_len);
    if (isUrlExclude(rec->session, pConfig, cur_uri, cur_uri_len))
        return bypassUrimapHook(rec, myData);

    if (isDomainExclude(rec->session, pConfig))
        return bypassUrimapHook(rec, myData);

    char httpMethod[10] = {0};
    int method = getHttpMethod(rec, httpMethod);
    if (method == HTTP_UNKNOWN || method == HTTP_POST)
    {
        CacheConfig *pConfig = (CacheConfig *)g_api->get_config(
                            rec->session, &MNAME);
        int flag = getControlFlag(pConfig);
        CacheCtrl cacheCtrl;
        cacheCtrl.init(flag, pConfig->getDefaultAge(), pConfig->getMaxStale());
        parseEnv(rec->session, cacheCtrl);

        //Do not store POST method for handling, only store the timestamp
        if (method == HTTP_POST && cacheCtrl.isPrivateAutoFlush())
        {
            g_api->set_module_data(rec->session, &MNAME, LSI_DATA_IP,
                                   (void *)(long)DateTime_s_curTime);
        }
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]checkAssignHandler returned, method %s[%d].\n",
                   ModuleNameStr, httpMethod, method);
        return bypassUrimapHook(rec, myData);
    }

    //If it is range request, quit
    int rangeRequestLen = 0;
    const char *rangeRequest = g_api->get_req_header_by_id(rec->session,
                               LSI_HDR_RANGE, &rangeRequestLen);
    if (rangeRequest && rangeRequestLen > 0)
    {
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]checkAssignHandler returned, not support rangeRequest [%.*s].\n",
                   ModuleNameStr, rangeRequestLen, rangeRequest);
        return bypassUrimapHook(rec, myData);
    }

    if (method == HTTP_GET || method == HTTP_HEAD)
    {
        if (!pConfig->isCheckPublic() && !pConfig->isPrivateCheck())
        {
            if (!isReqCacheable(rec, pConfig))
            {
                g_api->log(rec->session, LSI_LOG_DEBUG,
                           "[%s]checkAssignHandler returned, no check and no cache.\n",
                           ModuleNameStr);
                return 0;  //Do not bypass Urimap hook for may use it next
            }
        }
    }


    if (myData == NULL)
        myData = createMData(rec);

    myData->pConfig = pConfig;
    myData->iMethod = method;

    if (myData->iMethod == HTTP_PURGE || myData->iMethod == HTTP_REFRESH)
    {
        g_api->log(rec->session, LSI_LOG_DEBUG,
                       "[%s]checkAssignHandler get HTTP PURGE/REFRESH.\n",
                       ModuleNameStr);

        if (LS_OK != g_api->register_req_handler(rec->session, &MNAME, 0))
        {
            g_api->log(rec->session, LSI_LOG_WARN,
                       "[%s]checkAssignHandler register_req_handler failed.\n",
                       ModuleNameStr);
            g_api->free_module_data(rec->session, &MNAME, LSI_DATA_HTTP,
                                    releaseMData);
        }
        disableRcvdRespHeaderFilter(rec->session);
        return bypassUrimapHook(rec, NULL);
    }

    CacheCtrl &cacheCtrl = myData->cacheCtrl;
    parseEnv(rec->session, cacheCtrl);

    if (rec->ptr1 != NULL && rec->len1 > 0)
        cacheCtrl.parse((const char *)rec->ptr1, rec->len1);

    if (!pConfig->isSet(CACHE_IGNORE_REQ_CACHE_CTRL_HEADER))
    {
        int bufLen;
        const char *buf = g_api->get_req_header_by_id(rec->session,
                          LSI_HDR_CACHE_CTRL, &bufLen);
        if (buf && bufLen > 0)
            cacheCtrl.parse(buf, bufLen);
    }

    //Set to true but not the below just for not to re-check cache state or
    //re-store it
    //bool doPublic = cacheCtrl.isPublicCacheable() || myData->pConfig->isCheckPublic();
    bool doPublic = true;
    int encodingLen;
    const char *encoding = g_api->get_req_header_by_id(rec->session,
                                                       LSI_HDR_ACC_ENCODING,
                                                       &encodingLen);
    myData->reqCompressType = (encodingLen >= 4 && strcasestr(encoding, "gzip"));
    if (myData->reqCompressType == LSI_NO_COMPRESS &&
        encodingLen >= 2 && strcasestr(encoding, "br"))
        myData->reqCompressType = LSI_BR_COMPRESS;

    myData->iCacheState = lookUpCache(rec, myData,
                                   cacheCtrl.getFlags() & CacheCtrl::no_vary,
                                   myData->pOrgUri, strlen(myData->pOrgUri),
                                   myData->pConfig->getStore(),
                                   &myData->cePublicHash,
                                   &myData->cePrivateHash,
                                   myData->pConfig,
                                   &myData->pEntry,
                                   doPublic);

    g_api->log(rec->session, LSI_LOG_DEBUG,
               "[%s]checkAssignHandler lookUpCache, myData %p entry %p state %d.\n",
               ModuleNameStr, myData, myData->pEntry, myData->iCacheState);

    if (g_api->get_req_env(rec->session, "LSCACHE_FRONTEND", 16, val, 2) > 0
        && strncasecmp(val, "1", 1) ==  0)
    {
        myData->hasCacheFrontend = 1;
    }

    if (myData->iCacheState != CE_STATE_NOCACHE)
        checkFileUpdateWithCache(rec, myData);//may change state


    if (myData->iCacheState != CE_STATE_NOCACHE && myData->iCacheState != CE_STATE_UPDATE_STALE)
    {
        if (((myData->iCacheState == CE_STATE_HAS_PRIVATE_CACHE &&
             (myData->pConfig->isCheckPublic() || myData->pConfig->isPrivateCheck()))
            ||
            (myData->iCacheState == CE_STATE_HAS_PUBLIC_CACHE
             && myData->pConfig->isCheckPublic()))
#ifdef USE_RECV_REQ_HEADER_HOOK
            && myData->pEntry->getNeedDelay() == 0
#endif
            )
        {

            if (LS_OK != g_api->register_req_handler(rec->session, &MNAME, 0))
            {
                g_api->log(rec->session, LSI_LOG_WARN,
                           "[%s]checkAssignHandler register_req_handler failed.\n",
                           ModuleNameStr);
                g_api->free_module_data(rec->session, &MNAME, LSI_DATA_HTTP,
                                        releaseMData);
            }
            else
            {
                //Since it is served from cache, so bypass modsec checking
                //Becaue when cache entry created, it was checked.
                g_api->set_req_env(rec->session,  "modsecurity", 11, "off", 3);

                //myData->pEntry->incHits();
                myData->iHaveAddedHook = 3; //state of using handler
                g_api->log(rec->session, LSI_LOG_DEBUG,
                           "[%s]checkAssignHandler register_req_handler OK.\n",
                           ModuleNameStr);
            }
            disableRcvdRespHeaderFilter(rec->session);
            bypassUrimapHook(rec, NULL);
        }
        else
        {
            g_api->log(rec->session, LSI_LOG_INFO,
                       "[%s]checkAssignHandler found cachestate %d but set to not check.\n",
                       ModuleNameStr, myData->iCacheState);
#ifdef USE_RECV_REQ_HEADER_HOOK
            //may have another chance to check
            if (myData->pEntry->getNeedDelay())
                myData->pEntry->setNeedDelay(0);
#endif
        }
    }
    else if (myData->iMethod == HTTP_GET && isReqCacheable(rec, pConfig))
    {
        if (!myData->cacheCtrl.isCacheOff()
            || (myData->pConfig->isCheckPublic() || myData->pConfig->isPrivateCheck()))
        {
            myData->iHaveAddedHook = 1;

            //g_api->set_session_hook_flag( rec->_session, LSI_HKPT_RCVD_RESP_BODY, &MNAME, 1 );
            g_api->log(rec->session, LSI_LOG_DEBUG,
                       "[%s]checkAssignHandler Add Hooks.\n", ModuleNameStr);
            bypassUrimapHook(rec, NULL);
        }
        else
        {
#ifdef USE_RECV_REQ_HEADER_HOOK
            //Do not free myData because later may have cache-contrl env enable it
            myData->iHaveAddedHook = 0;
#else
            clearHooksOnly(rec->session);
            myData->iHaveAddedHook = 0;
            g_api->free_module_data(rec->session, &MNAME, LSI_DATA_HTTP,
                                        releaseMData);
#endif
        }
    }

    return 0;
}

static int checkVaryEnv(lsi_param_t *rec)
{
    MyMData *myData = (MyMData *) g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData == NULL)
        myData = createMData(rec);

    if (myData->pCacheVary == NULL)
        myData->pCacheVary = new AutoStr2;

    myData->pCacheVary->append((const char *)rec->ptr1, rec->len1);
    myData->pCacheVary->append(",", 1);
    return 0;
}



static int checkCtrlEnv(lsi_param_t *rec)
{
    MyMData *myData = (MyMData *) g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);

    if (myData == NULL)
        myData = createMData(rec);
    if (myData->pCacheCtrlVary == NULL)
        myData->pCacheCtrlVary = new AutoStr2;

    if (rec->len1 > 5 && strncasecmp((const char *)rec->ptr1, "vary=", 5) == 0)
    {
        myData->pCacheCtrlVary->setStr((const char *)rec->ptr1 + 5,
                                       rec->len1 - 5);
        return 0;
    }

    CacheCtrl &cacheCtrl = myData->cacheCtrl;
    if (rec->ptr1 != NULL && rec->len1 > 0)
        cacheCtrl.parse((const char *)rec->ptr1, rec->len1);

    if (cacheCtrl.isCacheOff() && myData->iHaveAddedHook == 1)
    {
        clearHooksOnly(rec->session);
        myData->iHaveAddedHook = 0;
    }
    else if (!cacheCtrl.isCacheOff() && myData->iHaveAddedHook == 0)
    {
        int hkpt = LSI_HKPT_RCVD_RESP_HEADER;
        g_api->enable_hook(rec->session, &MNAME, 1, &hkpt, 1);
        myData->iHaveAddedHook = 1;
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]checkEnv Add Hooks.\n", ModuleNameStr);
    }

    return 0;
}

int releaseIpCounter(void *data)
{
    //No malloc, needn't free, but functions must be presented.
    return 0;
}


static lsi_serverhook_t serverHooks[] =
{
    {LSI_HKPT_HTTP_BEGIN,       sessionBegin,       LSI_HOOK_FIRST,  LSI_FLAG_ENABLED},
#ifdef USE_RECV_REQ_HEADER_HOOK
    {LSI_HKPT_RCVD_REQ_HEADER,  checkAssignHandler, LSI_HOOK_EARLY,     LSI_FLAG_ENABLED},
#endif
    {LSI_HKPT_URI_MAP,          checkAssignHandler, LSI_HOOK_FIRST, LSI_FLAG_ENABLED},
    {LSI_HKPT_HTTP_END,         endCache,           LSI_HOOK_LAST + 1,  LSI_FLAG_ENABLED},
    {LSI_HKPT_HANDLER_RESTART,  cancelCache,        LSI_HOOK_LAST + 1,  LSI_FLAG_ENABLED},
    {LSI_HKPT_RCVD_RESP_HEADER, createEntry,        LSI_HOOK_LAST + 1,  LSI_FLAG_ENABLED},


    {LSI_HKPT_RCVD_RESP_BODY,   cacheTofile,        LSI_HOOK_LAST + 1,  0},
    {LSI_HKPT_SEND_RESP_BODY,   cacheTofileFilter,  LSI_HOOK_LAST + 1,  0},
    LSI_HOOK_END   //Must put this at the end position
};

static int init(lsi_module_t *pModule)
{
    g_api->init_module_data(pModule, releaseMData, LSI_DATA_HTTP);
    g_api->init_module_data(pModule, releaseIpCounter, LSI_DATA_IP);
    g_api->register_env_handler("cache-control", 13, checkCtrlEnv);
    g_api->register_env_handler("cache-vary",    10, checkVaryEnv);

//     g_api->register_env_handler("setcachedata", 12, setCacheUserData);
//     g_api->register_env_handler("getcachedata", 12, getCacheUserData);

    return 0;
}


int isModified(const lsi_session_t *session, CeHeader &CeHeader, char *etag,
               int etagLen, AutoStr2 &str)
{
    int len;
    const char *buf = NULL;

    if (etagLen > 0)
    {
        buf = g_api->get_req_header(session, "If-None-Match", 13, &len);
        if (buf && len == etagLen &&
            memcmp(etag, buf, etagLen > 3 ? etagLen - 3 : etagLen) == 0)
        {
            str.setStr(buf, len);
            return 0;
        }
    }

    buf = g_api->get_req_header(session, "If-Modified-Since", 17, &len);
    if (!*buf && len >= RFC_1123_TIME_LEN &&
        CeHeader.m_tmLastMod <= DateTime::parseHttpTime(buf, len))
        return 0;

    return 1;
}


static void purgeAllByTag(const lsi_session_t *session, MyMData *myData,
                          const char *pValue, int valLen, int stale)
{
    CacheStore *pStore = myData->pConfig->getStore();
    if (!pStore)
        return ;

    CacheKey key;
    int ipLen;
    key.m_pIP = g_api->get_client_ip(session, &ipLen);
    key.m_ipLen = ipLen;
    key.m_iCookieVary = 0;
    key.m_iCookiePrivate = 0;
    key.m_sCookie.setStr("");


    pStore->getManager()->processPrivatePurgeCmd(&key,
            pValue, valLen, DateTime::s_curTime, DateTime::s_curTimeUs / 1000, stale);
    g_api->log(session, LSI_LOG_DEBUG,
               "PURGE private cache for [%s]: %.*s\n",
               key.m_pIP, valLen, pValue);



    pStore->getManager()->processPurgeCmd(
        pValue, valLen, DateTime::s_curTime, DateTime::s_curTimeUs / 1000, stale);
    g_api->log(session, LSI_LOG_DEBUG,  "PURGE public cache: %.*s\n",
               valLen, pValue);
}

static void processPurge2(const lsi_session_t *session,
                         const char *pValue, int valLen)
{
    CacheStore *pStore = NULL;
    MyMData *myData = (MyMData *)g_api->get_module_data(session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData)
        pStore = myData->pConfig->getStore();
    else
    {
        CacheConfig *pContextConfig = (CacheConfig *)g_api->get_config(
                                          session, &MNAME);
        pStore = pContextConfig->getStore();
    }
    if (!pStore)
        return ;

    const char *pValueEnd = pValue + valLen;
    CacheKey key;
    bool isPriv = false;
    if (strncmp(pValue, "private,", 8) == 0)
    {
        int ipLen;
        char pCookieBuf[MAX_HEADER_LEN] = {0};
        char *pCookieBufEnd = pCookieBuf + MAX_HEADER_LEN;
        key.m_pIP = g_api->get_client_ip(session, &ipLen);
        key.m_ipLen = ipLen;
        key.m_iCookieVary = 0;
        
        HttpSession *pSession = (HttpSession *)session;
        HttpReq *pReq = pSession->getReq();
        key.m_iCookiePrivate = getPrivateCacheCookie(pReq,
                               &pCookieBuf[key.m_iCookieVary],
                               pCookieBufEnd);
        key.m_sCookie.setStr(pCookieBuf);

        pValue += 8;
        isPriv = true;
    }
    else
    {
        if (strncmp(pValue, "public,", 7) == 0)
            pValue += 7;
    }
    
    
    while (isspace(*pValue))
        ++pValue;

    int stale = 0;
    if (strncasecmp(pValue, "stale,", 6 ) == 0)
    {
        stale = 1;
        pValue += 6;
    }
    
    while (isspace(*pValue))
        ++pValue;

    valLen = pValueEnd - pValue;
    
    char tag[17] = {0};
    if (*pValue == '/')
    {
        uriToTag(session, pValue, valLen, 0, tag);
        pValue = tag;
        valLen = 16;
    }
        
    if (isPriv)
    {
        pStore->getManager()->processPrivatePurgeCmd(&key,
                pValue, valLen, DateTime::s_curTime, DateTime::s_curTimeUs / 1000, stale);
        g_api->log(session, LSI_LOG_DEBUG,
                   "PURGE private cache for [%s]: %.*s\n",
                   key.m_pIP, valLen, pValue);
    }
    else
    {
        pStore->getManager()->processPurgeCmd(
            pValue, valLen, DateTime::s_curTime, DateTime::s_curTimeUs / 1000, stale);
        g_api->log(session, LSI_LOG_DEBUG,  "PURGE public cache: %.*s\n",
                   valLen, pValue);
    }
    
}


static void processPurge(const lsi_session_t *session, const char *pValue,
                         int valLen)
{
    const char *pBegin = pValue;
    const char *pEnd = pValue + valLen;
    const char *p;
    while(pBegin < pEnd)
    {
        while(pBegin < pEnd && isspace(*pBegin))
            ++pBegin;
        p = (const char *)memchr(pBegin, ';', pEnd - pBegin);
        if (!p)
            p = pEnd;
        if (p > pBegin)
        {
            processPurge2(session, pBegin, p - pBegin);
        }
        pBegin = p+1;
    }
    g_api->log(session, LSI_LOG_DEBUG,  "processPurge: %.*s\n", valLen, pValue);
}

static void decref_and_free_data(MyMData *myData, const lsi_session_t *session)
{
    if (myData->pEntry)
        myData->pEntry->decRef();
    g_api->free_module_data(session, &MNAME, LSI_DATA_HTTP, releaseMData);
}


static void updateCacheEntry(CacheConfig *pConfig, CacheEntry *pEntry, int tmpfd,
                             char *tmppath, int compressType, off_t length)
{
    //length change
    pEntry->setPart2Len(length);

    //Update compressType
    pEntry->markReady(compressType);

    //Update fd
    close(pEntry->getFdStore());
    pEntry->setFdStore(tmpfd);

    //since the filepath is org path + .tmp, so just publish it to use it
    pConfig->getStore()->publish(pEntry);
}

static void closeTmpFile(int fd, const char *path)
{
    close(fd);
    unlink(path);
}

static void toggleGzipState(MyMData *myData, int needCompressType)
{
    if (needCompressType != LSI_NO_COMPRESS &&
        needCompressType != LSI_GZIP_COMPRESS)
        return ;

    /**
     * Too small content needn't to be compressed
     */
    if (needCompressType == LSI_GZIP_COMPRESS &&
        getEntryContentLength(myData) < 200)
        return ;

    /***
     * Because in child process, mydata maybe can not be accessed when it is
     * released, so save the config and entry now.
     */
    CacheConfig *pConfig = myData->pConfig;
    CacheEntry *pEntry = myData->pEntry;

    /***
     * Should not block the current processing
     */
    pid_t pid = fork();
    if (pid < 0)
    {
        g_api->log(NULL, LSI_LOG_ERROR, "[%s]toggleGzipState fork failed.\n",
               ModuleNameStr);
        return ;
    }

    if (pid > 0)
    {
        g_api->log(NULL, LSI_LOG_DEBUG,
               "[%s]toggleGzipState fork pid %d to processing.\n",
               ModuleNameStr, pid);
        return;
    }
    else //child process
    {
        char tmppath[4100] = {0};
        int len = 4096;
        pConfig->getStore()->getEntryFilePath(pEntry, tmppath, len);
        lstrncat(tmppath, ".tmp", sizeof(tmppath));

        /**
         * If the the file exists more than 10 minutes, should be something wrong
         * Even the state is updating, try to remove it
         */
        struct stat sb;
        if (stat(tmppath, &sb) != -1)
        {
            if((long)DateTime_s_curTime - (long)sb.st_ctime < 360)
                exit (0);
            else
            {
                g_api->log(NULL, LSI_LOG_DEBUG,
                           "[%s]toggleGzipState processing too long %ld seconds.\n",
                           ModuleNameStr,
                           (long)DateTime_s_curTime - (long)sb.st_ctime);
                unlink(tmppath);
            }
        }

        int tmpfd = ::open(tmppath, O_RDWR | O_CREAT | O_TRUNC | O_EXCL, 0760);
        if (tmpfd == -1)
        {
            g_api->log(NULL, LSI_LOG_DEBUG,
                           "[%s]toggleGzipState can not open file %s.\n",
                           ModuleNameStr, tmppath);
            exit (0);
        }

        ::fcntl(tmpfd, F_SETFD, FD_CLOEXEC);

        z_stream *zstream = new z_stream;
        if (!zstream)
        {
            g_api->log(NULL, LSI_LOG_ERROR, "[%s]toggleGzipState alloc"
                        " memory for zstream error.\n", ModuleNameStr);
            closeTmpFile(tmpfd, tmppath);
            exit (0);
        }

        bool compress = (needCompressType == LSI_GZIP_COMPRESS);
        if (initZstream(zstream, compress))
        {
            delete zstream;
            closeTmpFile(tmpfd, tmppath);
            g_api->log(NULL, LSI_LOG_ERROR, "[%s]toggleGzipState initZstream"
                        " error.\n", ModuleNameStr);
            exit (0);
        }

        int fd = pEntry->getFdStore();
        int part1offset = pEntry->getPart1Offset();
        int part2offset = pEntry->getPart2Offset();
        off_t length = pEntry->getContentTotalLen() -
                           (part2offset - part1offset);

        unsigned char *buff  = (unsigned char *)mmap((caddr_t)0,
                                                     part2offset + length,
                                                     PROT_READ, MAP_SHARED,
                                                     fd, 0);
        if (buff == (unsigned char *)(-1))
        {
            uninitZstream(zstream, compress);
            closeTmpFile(tmpfd, tmppath);
            g_api->log(NULL, LSI_LOG_ERROR, "[%s]toggleGzipState mmap"
                        " error.\n", ModuleNameStr);
            exit (0);
        }




        /**
         * Now set the updating flag
         */
        write(tmpfd, buff, part2offset);
        buff+= part2offset;

        off_t ret = compressbuf(zstream, compress, buff, length, tmpfd, 1);
        g_api->log(NULL, LSI_LOG_DEBUG,
                   "[%s]toggleGzipState write %lld bytes to file %s.\n",
                   ModuleNameStr, (long long)ret, tmppath);

        if (ret <= 0)
        {
            //failed
            closeTmpFile(tmpfd, tmppath);
            g_api->log(NULL, LSI_LOG_ERROR, "[%s]toggleGzipState compressbuf"
                        " error.\n", ModuleNameStr);
        }
        else
        {
            updateCacheEntry(pConfig, pEntry, tmpfd, tmppath, needCompressType, ret);
            g_api->log(NULL, LSI_LOG_DEBUG, "[%s]toggleGzipState updated"
                        " the cache entry.\n", ModuleNameStr);
        }
        uninitZstream(zstream, compress);

        munmap((caddr_t)buff, part2offset + length);
        exit(0);
    }
}


static int handlerProcess(const lsi_session_t *session)
{
    MyMData *myData = (MyMData *)g_api->get_module_data(session, &MNAME,
                      LSI_DATA_HTTP);
    if (!myData)
    {
        g_api->log(session, LSI_LOG_ERROR,
                   "[%s]internal error during handlerProcess.\n", ModuleNameStr);
        return 500;
    }

    //Disable RECV_RESP header here, to avoid cache myself
    disableRcvdRespHeaderFilter(session);
    
    if (myData->iMethod == HTTP_PURGE || myData->iMethod == HTTP_REFRESH)
    {
        /**
         * 1, if the context is protected by realm, need to pass the http Authorization
         * 2, if the context is not protected by realm, the IP must be defined as trust
         * 3, if the purgeUri is defined, and match the uri, purge
         * by the request purge header
         * 4, else purge all the entry with a tag which is caculated from the
         * url, no matter the qs, cookie and vary and so on.
         */
        
        //Comments: REFRESH is stale purge
        int stale = myData->iMethod == HTTP_REFRESH ? 1 : 0;
        char httpAuthEnv[3] = {0};
        int httpAuthEnvLen = g_api->get_req_env(session, "HttpAuth", 8,
                                                    httpAuthEnv, 2);
        if (httpAuthEnvLen <= 0 || *httpAuthEnv != '1')
        {
            int len;
            const char *pIP = g_api->get_client_ip(session, &len);
            if (g_api->get_client_access_level(session) != LSI_ACL_TRUST &&
                (!pIP || strncmp(pIP, "127.0.0.1", 9) != 0))
            {
                g_api->free_module_data(session, &MNAME, LSI_DATA_HTTP, releaseMData);
                g_api->log(session, LSI_LOG_DEBUG,
                   "[%s]Processed PURGE http AUTH error.\n", ModuleNameStr);
                return 405;
            }
        }


        bool bPurgeTags = false;
        char *pConfUri = myData->pConfig->getPurgeUri();
        int uri_len = 0;
        const char *uri = g_api->get_req_uri(session, &uri_len);
    
        if (pConfUri && strcasecmp(pConfUri, uri) == 0)
            bPurgeTags = true;
        
        if (bPurgeTags)
        {
            int valLen = 0;
            const char *pVal = g_api->get_req_header_by_id(session,
                                                            LSI_HDR_LITESPEED_PURGE,
                                                            &valLen);
            if (pVal && valLen > 0)
            {
                processPurge(session, pVal, valLen);
                g_api->set_resp_content_length(session, valLen + 24);
                g_api->append_resp_body(session, "Processed PURGE by \"", 20);
                g_api->append_resp_body(session, pVal, valLen);
                g_api->append_resp_body(session, "\".\r\n", 4);
            }
            else
                g_api->append_resp_body(session, "PURGE tag not defined.\n",
                                        23);
        }
        else
        {
            //g_api->append_resp_body(session, "PURGE uri not match.\n", 21);
            /**
             * Now, will puge all cache with this URL
             */
            
            char tag[17] = {0};
            uriToTag(session, uri, uri_len, 1, tag);
            purgeAllByTag(session, myData, tag, 16, stale);

            g_api->set_resp_content_length(session, strlen(myData->pOrgUri) + 24);
            g_api->append_resp_body(session, "Processed ", 10);
            g_api->append_resp_body(session,
                                    stale ? "REFRESH" : "PURGE",
                                    stale ? 7 : 5);
            
            g_api->append_resp_body(session, " by \"", 5);
            g_api->append_resp_body(session, myData->pOrgUri, 
                    strlen(myData->pOrgUri));
            g_api->append_resp_body(session, "\".\r\n", 4);
        }

        g_api->log(NULL, LSI_LOG_DEBUG,
                    "[%s]Did %s %s with tag: %d.\n", ModuleNameStr, 
                    stale ? "REFRESH" : "PURGE",
                    myData->pOrgUri, bPurgeTags);

        g_api->end_resp(session);
        g_api->free_module_data(session, &MNAME, LSI_DATA_HTTP, releaseMData);
        return 0;
    }

    myData->pEntry->incRef();
    myData->pEntry->setLastAccess(DateTime::s_curTime);
    
    char tmBuf[RFC_1123_TIME_LEN + 1];
    int len;
    int fd = myData->pEntry->getFdStore();
    CeHeader &CeHeader = myData->pEntry->getHeader();
    int compressType = myData->pEntry->getCompressType();

    int hitIdx = (myData->iCacheState == CE_STATE_HAS_PRIVATE_CACHE) ? 1 : 0;

    char *buff = NULL;
    char *pBuffOrg = NULL;
    int part1offset = myData->pEntry->getPart1Offset();
    int part2offset = myData->pEntry->getPart2Offset();
    if (part2offset - part1offset > 0)
    {
#ifdef CACHE_RESP_HEADER
        if (myData->m_pEntry->m_sRespHeader.len() > 0) //has it
            buff = (char *)(myData->m_pEntry->m_sRespHeader.c_str());
        else
#endif
        {
            buff  = (char *)mmap((caddr_t)0, part2offset,
                                 PROT_READ, MAP_SHARED, fd, 0);
            if (buff == (char *)(-1))
            {
                decref_and_free_data(myData, session);
                g_api->log(session, LSI_LOG_ERROR,
                           "[%s]handlerProcess return 500 due to cant alloc memory.\n",
                           ModuleNameStr);
                return 500;
            }
            pBuffOrg = buff;
            buff += part1offset;
        }

        if (CeHeader.m_lenETag > 0)
        {
            char *pEtag = buff;
            AutoStr2 str;
            if (!isModified(session, CeHeader, buff, CeHeader.m_lenETag, str))
            {
                if (CeHeader.m_lenETag > 0)
                    g_api->set_resp_header(session, LSI_RSPHDR_ETAG, NULL, 0,
                                           str.c_str(), CeHeader.m_lenETag,
                                           LSI_HEADEROP_SET);

                g_api->set_resp_header(session, LSI_RSPHDR_UNKNOWN,
                                       s_x_cached, sizeof(s_x_cached) - 1,
                                       s_hits[hitIdx], s_hitsLen[hitIdx],
                                       LSI_HEADEROP_SET);

                g_api->set_status_code(session, 304);
                if (pBuffOrg)
                    munmap((caddr_t)pBuffOrg, part2offset);
                g_api->end_resp(session);
                decref_and_free_data(myData, session);
                g_api->log(session, LSI_LOG_DEBUG,
                           "[%s]handlerProcess return 304.\n", ModuleNameStr);
                return 0;
            }


            str.setStr(buff, CeHeader.m_lenETag);
            pEtag = (char *)str.c_str();

            char *pUpdate = pEtag + str.len() - 4;
            if (*pUpdate++ == ';')
            {
                if (compressType == 0)
                {
                    *pUpdate++ = ';';
                    *pUpdate++ = ';';
                }
                else if (compressType == 1)
                {
                    *pUpdate++ = 'g';
                    *pUpdate++ = 'z';
                }
                else if (compressType == 2)
                {
                    *pUpdate++ = 'b';
                    *pUpdate++ = 'r';
                }
            }

            g_api->set_resp_header(session, LSI_RSPHDR_ETAG, NULL, 0, pEtag,
                                   CeHeader.m_lenETag, LSI_HEADEROP_SET);
            g_api->log(session, LSI_LOG_DEBUG,
                       "[%s]handlerProcess add etag %s.\n",
                       ModuleNameStr, pEtag);
        }

        buff += CeHeader.m_lenETag + CeHeader.m_lenStxFilePath;
        len = part2offset - part1offset -
              CeHeader.m_lenETag - CeHeader.m_lenStxFilePath;
        g_api->set_resp_header2(session, buff, len, LSI_HEADEROP_SET);


        if (myData->hasCacheFrontend == 0)
        {
            g_api->remove_resp_header(session, LSI_RSPHDR_LITESPEED_CACHE_CONTROL,
                                      NULL, 0);
            g_api->remove_resp_header(session, LSI_RSPHDR_LITESPEED_TAG,
                                      NULL, 0);
            g_api->remove_resp_header(session, LSI_RSPHDR_LITESPEED_VARY,
                                      NULL, 0);

        }
    }

    if (CeHeader.m_tmLastMod != 0)
    {
        g_api->set_resp_header(session, LSI_RSPHDR_LAST_MODIFIED, NULL, 0,
                               DateTime::getRFCTime(CeHeader.m_tmLastMod, tmBuf),
                               RFC_1123_TIME_LEN, LSI_HEADEROP_SET);
    }

    g_api->set_resp_header(session, LSI_RSPHDR_UNKNOWN,
                           s_x_cached, sizeof(s_x_cached) - 1,
                           s_hits[hitIdx], s_hitsLen[hitIdx],
                           LSI_HEADEROP_SET);

    g_api->set_status_code(session, CeHeader.m_statusCode);
    g_api->log(session, LSI_LOG_DEBUG,
                       "[%s]handlerProcess set_status_code %d.\n",
                       ModuleNameStr, CeHeader.m_statusCode);

    //assert(strcasestr(myData->pOrgUri, "fonts/ProximaNova-Regular.woff") == NULL);

    int ret  = 0;
    if (myData->iMethod == HTTP_GET)
    {
        off_t length = myData->pEntry->getContentTotalLen() -
                       (part2offset - part1offset);

        if (compressType == LSI_NO_COMPRESS)
        {
            if (myData->reqCompressType == LSI_GZIP_COMPRESS)
                myData->pEntry->incHits();
            else if (myData->reqCompressType == compressType)
                myData->pEntry->clearHits();
        }
        else if (compressType == LSI_GZIP_COMPRESS)
        {
            g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_ENCODING,
                                   NULL, 0, "gzip", 4, LSI_HEADEROP_SET);
            g_api->log(session, LSI_LOG_DEBUG,
                       "[%s]set_resp_header [Content-Encoding: gzip].\n",
                       ModuleNameStr);

            if (myData->reqCompressType == LSI_NO_COMPRESS)
                myData->pEntry->incHits();
            else if (myData->reqCompressType == compressType)
                myData->pEntry->clearHits();

        }
        else if (compressType == LSI_BR_COMPRESS)
        {
            g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_ENCODING,
                                   NULL, 0, "br", 2, LSI_HEADEROP_SET);
            g_api->log(session, LSI_LOG_DEBUG,
                       "[%s]set_resp_header [Content-Encoding: br].\n",
                       ModuleNameStr);
        }
        g_api->set_resp_buffer_compress_method(session, compressType);


        g_api->set_resp_content_length(session, length);
        int fd = myData->pEntry->getFdStore();

        g_api->log(session, LSI_LOG_DEBUG,
                   "[%s]handlerProcess fd %d, offset %d, length %ld\n",
                   ModuleNameStr, fd, part2offset, length);

        if (g_api->send_file2(session, fd, part2offset, length) == 0)
            g_api->end_resp(session);
        else
            ret = 500;

        /**
         * For testing, disable the code
         */
        if (myData->pEntry->getHits() >= 10)
        {
            g_api->log(session, LSI_LOG_DEBUG,
                       "[%s]handlerProcess check entry hit %ld times, "
                       "will change to compressType from %d to %d.\n",
                       ModuleNameStr, myData->pEntry->getHits(),
                       compressType, myData->reqCompressType);
            toggleGzipState(myData, myData->reqCompressType);
        }
    }
    else //HEAD
        g_api->end_resp(session);

    if (pBuffOrg)
        munmap((caddr_t)pBuffOrg, part2offset);
    decref_and_free_data(myData, session);
    return ret;
}

lsi_reqhdlr_t cache_handler = { handlerProcess, NULL, NULL, NULL,
                                NULL, NULL, NULL,  };
lsi_confparser_t cacheDealConfig = { ParseConfig, FreeConfig, paramArray };
lsi_module_t cache = { LSI_MODULE_SIGNATURE, init, &cache_handler,
                       &cacheDealConfig, MODULE_VERSION_INFO, serverHooks, {0}
                     };

