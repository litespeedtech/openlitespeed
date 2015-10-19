/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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

#include <ls.h>
#include <lsr/ls_confparser.h>
#include <util/autostr.h>
#include <util/datetime.h>
#include <util/stringtool.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CACHE_CONTROL_LENGTH    128
#define MAX_RESP_HEADERS_NUMBER     50
#define MNAME                       cache
#define ModuleNameStr               "Module-Cache"
#define CACHEMODULEKEY              "_lsi_module_cache_handler__"
#define CACHEMODULEKEYLEN           (sizeof(CACHEMODULEKEY) - 1)
#define CACHEMODULEROOT             "cachedata"

//The below info should be gotten from the configuration file
#define max_file_len        4096
#define VALMAXSIZE          4096

/////////////////////////////////////////////////////////////////////////////
extern lsi_module_t MNAME;

static const char s_x_cached[] = "X-LiteSpeed-Cache: hit\r\n";
static const char s_x_cached_private[] =
    "X-LiteSpeed-Cache: hit,private\r\n";

DirHashCacheStore *s_pDefaultStore = NULL;

enum
{
    CE_STATE_NOCACHE = 0,
    CE_STATE_HAS_RIVATECACHE,
    CE_STATE_HAS_PUBLIC_CACHE,
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
    char *pOrgUri;
    DirHashCacheStore *pDirHashCacheStore;
    CacheEntry *pEntry;
    CacheConfig *pConfig;
    unsigned char iCacheState;
    unsigned char iMethod;
    CacheCtrl cacheCtrl;
    CacheHash cePublicHash;
    CacheHash cePrivateHash;
    int     iHaveAddedHook;
};

const int paramArrayCount = 11;
const char *paramArray[paramArrayCount + 1] =
{
    "enablecache",
    "qscache",
    "reqcookiecache",
    "respcookiecache",
    "ignorereqcachectrl",
    "ignorerespcachectrl",
    "expireinseconds",
    "maxstaleage",
    "enableprivatecache",
    "privateexpireinseconds",
    "storagepath",
    NULL //Must have NULL in the last item
};


// Parses the key and value given.  If key is storagepath, returns 1, otherwise returns 0
static int parseLine(CacheConfig *pConfig, const char *key, int keyLen,
                     const char *val, int valLen)
{
    int i;
    int bit;
    int minValue = 0;
    int maxValue = 1;
    int defValue = 0;
    for (i = 0; i < paramArrayCount; ++i)
    {
        if (strncasecmp(key, paramArray[i], keyLen) == 0)
            break;
    }

    switch (i)
    {
    case 0:
        bit = CACHE_ENABLED;
        break;
    case 1:
        bit = CACHE_QS_CACHE;
        break;
    case 2:
        bit = CACHE_REQ_COOKIE_CACHE;
        break;
    case 3:
        bit = CACHE_RESP_COOKIE_CACHE;
        break;
    case 4:
        bit = CACHE_IGNORE_REQ_CACHE_CTRL_HEADER;
        break;
    case 5:
        bit = CACHE_IGNORE_RESP_CACHE_CTRL_HEADER;
        break;
    case 6:
        bit = CACHE_MAX_AGE_SET;
        maxValue = INT_MAX;
        break;
    case 7:
        bit = CACHE_STALE_AGE_SET;
        maxValue = INT_MAX;
        break;
    case 8:
        bit = CACHE_PRIVATE_ENABLED;
        break;
    case 9:
        bit = CACHE_PRIVATE_AGE_SET;
        maxValue = INT_MAX;
        break;
    case 10:
        return 1; //Only case to return 1
    default:
        //Not a part of the list
        return 0;
    }

    int value = strtol(val, NULL, 10);
    if (!(value == 0 && *val != '0'))
    {
        if (value < minValue || value > maxValue)
            value = defValue;

        if (bit == CACHE_MAX_AGE_SET)
        {
            pConfig->setDefaultAge(value);
            value = 1;
        }
        else if (bit == CACHE_STALE_AGE_SET)
        {
            pConfig->setMaxStale(value);
            value = 1;
        }
        else if (bit == CACHE_PRIVATE_AGE_SET)
        {
            pConfig->setPrivateAge(value);
            value = 1;
        }

        pConfig->setConfigBit(bit, value);
    }
    return 0;
}


static int createCachePath(const char *path, int mode)
{
    struct stat st;
    if (stat(path, &st) == -1)
    {
        if (mkdir(path, mode) == -1)
            return LS_FAIL;
    }
    return LS_OK;
}


//Update permission of the dest to the same of the src
static void matchDirectoryPermissions(const char *src, const char *dest)
{
    struct stat st;
    if (stat(src, &st) != -1)
        chown(dest, st.st_uid, st.st_gid);
}


static void *ParseConfig(const char *param, int param_len,
                         void *_initial_config, int level, const char *name)
{
    const char *pLineBegin;
    const char *pLineEnd;
    const char *pParamEnd = param + param_len;
    ls_confparser_t cp;
    ls_objarray_t *pList;
    CacheConfig *pInitConfig = (CacheConfig *)_initial_config;
    CacheConfig *pConfig = new CacheConfig;
    if (!pConfig)
        return NULL;

    pConfig->inherit(pInitConfig);
    if (!param)
        return (void *)pConfig;

    ls_confparser(&cp);

    while ((pLineBegin = ls_getconfline(&param, pParamEnd, &pLineEnd)) != NULL)
    {
        pList = ls_confparser_linekv(&cp, pLineBegin, pLineEnd);
        if (!pList)
            continue;

        ls_str_t *pKey = (ls_str_t *)ls_objarray_getobj(pList, 0);
        ls_str_t *pVal = (ls_str_t *)ls_objarray_getobj(pList, 1);
        const char *pValStr = ls_str_cstr(pVal);
        if (pValStr == NULL)
            continue;

        int valLen = ls_str_len(pVal);
        if (valLen == 0)
            continue;

        if (parseLine(pConfig, ls_str_cstr(pKey), ls_str_len(pKey), pValStr,
                      valLen) == 1)
        {
            if (level != LSI_CFG_CONTEXT)
            {
                char *pBak = new char[valLen + 1];
                strncpy(pBak, pValStr, valLen);
                pBak[valLen] = 0x00;
                pValStr = pBak;

                char pTmp[max_file_len]  = {0};
                char cachePath[max_file_len]  = {0};
                char defaultCachePath[max_file_len]  = {0};

                //check if contains $
                if (strchr(pValStr, '$'))
                {
                    int ret = g_api->expand_current_server_varible(level, pValStr, pTmp,
                              max_file_len);
                    if (ret >= 0)
                    {
                        pValStr = pTmp;
                        valLen = ret;
                    }
                    else
                    {
                        g_api->log(NULL, LSI_LOG_ERROR,
                                   "[%s]parseConfig failed to expand_current_server_varible[%s], default will be in use.\n",
                                   ModuleNameStr, pValStr);

                        delete []pBak;
                        ls_confparser_d(&cp);
                        return (void *)pConfig;
                    }
                }

                if (pValStr[0] != '/')
                    strcpy(cachePath, g_api->get_server_root());
                strcpy(defaultCachePath, g_api->get_server_root());
                strncat(cachePath, pValStr, valLen);
                strncat(defaultCachePath, CACHEMODULEROOT, strlen(CACHEMODULEROOT));

                if (createCachePath(cachePath, 0755) == -1)
                {
                    g_api->log(NULL, LSI_LOG_ERROR,
                               "[%s]parseConfig failed to create directory [%s].\n",
                               ModuleNameStr, cachePath);
                }
                else
                {
                    matchDirectoryPermissions(defaultCachePath , cachePath);
                    pConfig->setStoragePath(cachePath, strlen(cachePath));
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
        }
    }

    ls_confparser_d(&cp);
    return (void *)pConfig;
}


static void FreeConfig(void *_config)
{
    delete(CacheConfig *)_config;
}


static int initDefaultCacheStore()
{
    if (!s_pDefaultStore )
    {
        s_pDefaultStore = new DirHashCacheStore;
    }
    return s_pDefaultStore != NULL;
}



void calcCacheHash(const char *pUri, int iUriLen, const char *pQS,
                   int iQSLen,
                   const char *pIp, int iIpLen, const char *pCookie,
                   int iCookieLen, CacheHash *pCeHash, CacheHash *pPrvCeHash)
{
    XXH64_state_t state;
    pCeHash->initHash(&state);
    pCeHash->updHash(&state, pUri, iUriLen);
    if (iQSLen > 0)
    {
        pCeHash->updHash(&state, "?", 1);
        pCeHash->updHash(&state, pQS, iQSLen);
    }
    pCeHash->saveHash(&state);

    if (pIp)
    {
        pPrvCeHash->updHash(&state, "@", 1);
        pPrvCeHash->updHash(&state, pIp, iIpLen);
        if (iCookieLen > 0)
        {
            pPrvCeHash->updHash(&state, "~", 1);
            pPrvCeHash->updHash(&state, pCookie, iCookieLen);
        }
        pPrvCeHash->saveHash(&state);
    }
}


short hasCache(lsi_param_t *rec, const char *uri, int uriLen,
               DirHashCacheStore *pDirHashCacheStore,
               CacheHash *cePublicHash, CacheHash *cePrivateHash,
               CacheConfig *pConfig, CacheEntry **pEntry)
{
    int cookieLen;
    int iQSLen;
    int ipLen;
    const char *pCookie = g_api->get_req_cookies(rec->session, &cookieLen);
    const char *pIp = g_api->get_client_ip(rec->session, &ipLen);
    const char *pQs = g_api->get_req_query_string(rec->session, &iQSLen);
    calcCacheHash(uri, uriLen, pQs, iQSLen, pIp, ipLen, pCookie, cookieLen,
                  cePublicHash, cePrivateHash);

    //Try private one first
    long lastCacheFlush = (long)g_api->get_module_data(rec->session, &MNAME,
                          LSI_DATA_IP);
    *pEntry = pDirHashCacheStore->getCacheEntry(*cePrivateHash, uri, uriLen,
              pQs, iQSLen, pIp, ipLen, pCookie, cookieLen, lastCacheFlush,
              pConfig->getMaxStale());
    if (*pEntry && (!(*pEntry)->isStale() || (*pEntry)->isUpdating()))
        return CE_STATE_HAS_RIVATECACHE;

    //Second, check if can use public one
    *pEntry = pDirHashCacheStore->getCacheEntry(*cePublicHash, uri, uriLen,
              pQs, iQSLen, NULL, 0, NULL, 0, 0, pConfig->getMaxStale());
    if (*pEntry && (!(*pEntry)->isStale() || (*pEntry)->isUpdating()))
        return CE_STATE_HAS_PUBLIC_CACHE;

    if (*pEntry)
        (*pEntry)->setStale(1);

    return CE_STATE_NOCACHE;
}


int httpRelease(void *data)
{
    MyMData *myData = (MyMData *)data;
    if (myData)
    {
        if (myData->pOrgUri)
            delete []myData->pOrgUri;
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
    };
    int8_t headersBypassLen[] = {  13, 4, 4, 14, 17, 16, };

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


void getRespHeader(lsi_session_t *session, int header_index, char **buf,
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


void clearHooks(lsi_session_t *session)
{
    int aEnableHkpts[4], iEnableCount = 0;
    MyMData *myData = (MyMData *) g_api->get_module_data(session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData)
    {
        if (myData->iHaveAddedHook)
        {
            if (myData->iHaveAddedHook == 2)
                aEnableHkpts[iEnableCount++] = LSI_HKPT_RCVD_RESP_BODY;
            aEnableHkpts[iEnableCount++] = LSI_HKPT_RCVD_RESP_HEADER;
            aEnableHkpts[iEnableCount++] = LSI_HKPT_HANDLER_RESTART;
            aEnableHkpts[iEnableCount++] = LSI_HKPT_HTTP_END;
            g_api->enable_hook(session, &MNAME, 0,
                                                aEnableHkpts, iEnableCount);
            myData->iHaveAddedHook = 0;
        }
        g_api->free_module_data(session, &MNAME, LSI_DATA_HTTP,
                                httpRelease);
    }
}


static int cancelCache(lsi_param_t *rec)
{
    MyMData *myData = (MyMData *)g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData != NULL && myData->iCacheState == CE_STATE_WILLCACHE)
        myData->pDirHashCacheStore->cancelEntry(myData->pEntry, 1);
    clearHooks(rec->session);
    g_api->log(rec->session, LSI_LOG_DEBUG, "[%s]cache ended.\n",
               ModuleNameStr);
    return 0;
}


static int createEntry(lsi_param_t *rec)
{
    MyMData *myData = (MyMData *)g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData == NULL)
    {
        clearHooks(rec->session);
        g_api->log(rec->session, LSI_LOG_ERROR,
                   "[%s]internal error during createEntry.\n", ModuleNameStr);
        return 0;
    }

    //Error page won't be stored to cache
    int code = g_api->get_status_code(rec->session);
    if (code != 200)
    {
        clearHooks(rec->session);
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]cacheTofile to be cancelled for error page, code=%d.\n",
                   ModuleNameStr, code);
        return 0;
    }

    const char *phandlerType = g_api->get_req_handler_type(rec->session);
    if (phandlerType && memcmp("static", phandlerType, 6) == 0)
    {
        clearHooks(rec->session);
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]cacheTofile to be cancelled for static file type.\n",
                   ModuleNameStr);
        return 0;
    }

    if (!myData->pConfig->isSet(CACHE_RESP_COOKIE_CACHE))
    {
        struct iovec iov[1] = {{NULL, 0}};
        int iovCount = g_api->get_resp_header(rec->session,
                                              LSI_RSPHDR_SET_COOKIE, NULL,
                                              0, iov, 1);
        if (iov[0].iov_len > 0 && iovCount == 1)
        {
            clearHooks(rec->session);
            g_api->log(rec->session, LSI_LOG_DEBUG,
                       "[%s]cacheTofile to be cancelled for having respcookie.\n",
                       ModuleNameStr);
            return 0;
        }
    }

    int iCookieLen;
    int iQsLen;
    int ipLen;
    int iUriLen = strlen(myData->pOrgUri);
    const char *pCookie = g_api->get_req_cookies(rec->session, &iCookieLen);
    const char *pIp = g_api->get_client_ip(rec->session, &ipLen);
    const char *pQs = g_api->get_req_query_string(rec->session, &iQsLen);
    int errorcode;
    if (myData->cacheCtrl.isPrivateCacheable())
        myData->pEntry = myData->pDirHashCacheStore->createCacheEntry(
                             myData->cePrivateHash, myData->pOrgUri, iUriLen,
                             pQs, iQsLen, pIp, ipLen,
                             pCookie, iCookieLen, 1, &errorcode);
    else// if (myData->cacheCtrl.isPublicCacheable())
        myData->pEntry = myData->pDirHashCacheStore->createCacheEntry(
                             myData->cePublicHash, myData->pOrgUri, iUriLen,
                             pQs, iQsLen, NULL, 0, NULL,
                             0, 1, &errorcode);

    if (myData->pEntry == NULL)
    {
        clearHooks(rec->session);
        g_api->log(rec->session, LSI_LOG_ERROR,
                   "[%s] createEntry failed, code [%d].\n", ModuleNameStr,
                   errorcode);
        return 0;
    }

    //Now we can store it
    myData->iCacheState = CE_STATE_WILLCACHE;
    int iEnableHkpt = LSI_HKPT_RCVD_RESP_BODY;
    g_api->enable_hook(rec->session, &MNAME, 1,
                                        &iEnableHkpt, 1);
    myData->iHaveAddedHook = 2;
    return 0;
}


int cacheTofile(lsi_param_t *rec)
{
    MyMData *myData = (MyMData *)g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData == NULL)
    {
        g_api->log(rec->session, LSI_LOG_ERROR,
                   "[%s]internal error during cacheTofile.\n", ModuleNameStr);
        return 0;
    }
//     if (myData->pEntry == NULL || myData->iCacheState != CE_STATE_WILLCACHE)
//     {
//         g_api->log(rec->_session, LSI_LOG_ERROR, "[%s]cacheTofile error, code[%p %d].\n",
//                            ModuleNameStr, myData->pEntry, myData->iCacheState);
//         clearHooks(rec->_session);
//         return 0;
//     }

    myData->pEntry->setMaxStale(myData->pConfig->getMaxStale());
    g_api->log(rec->session, LSI_LOG_DEBUG,
               "[%s]save to %s cachestore, uri:%s\n", ModuleNameStr,
               ((myData->cacheCtrl.isPrivateCacheable()) ? "private" : "public"),
               myData->pOrgUri);

    int fd = myData->pEntry->getFdStore();
    char *sLastMod = NULL;
    char *sETag = NULL;
    int nLastModLen = 0;
    int nETagLen = 0;
    int headersBufSize = 0;
    CeHeader &CeHeader = myData->pEntry->getHeader();
    CeHeader.m_tmCreated = (int32_t)DateTime_s_curTime;
    CeHeader.m_tmExpire = CeHeader.m_tmCreated + myData->cacheCtrl.getMaxAge();
    getRespHeader(rec->session, LSI_RSPHDR_LAST_MODIFIED, &sLastMod,
                  &nLastModLen);
    if (sLastMod)
        CeHeader.m_tmLastMod = DateTime::parseHttpTime(sLastMod);

    getRespHeader(rec->session, LSI_RSPHDR_ETAG, &sETag, &nETagLen);
    if (sETag && nETagLen > 0)
    {
        if (nETagLen > VALMAXSIZE)
            nETagLen = VALMAXSIZE;
        CeHeader.m_iOffETag = 0;
        CeHeader.m_iLenETag = nETagLen;
    }
    else
        CeHeader.m_iLenETag = 0;

    CeHeader.m_iStatusCode = g_api->get_status_code(rec->session);

    //Right now, set wrong numbers, and this will be fixed when publish
    myData->pEntry->setContentLen(0, 0);

    if (g_api->is_resp_buffer_gzippped(rec->session))
        myData->pEntry->markReady(1);

    myData->pEntry->saveCeHeader();

    if (CeHeader.m_iLenETag > 0)
    {
        write(fd, sETag, CeHeader.m_iLenETag);

#ifdef CACHE_RESP_HEADER
        myData->m_pEntry->m_sRespHeader.append(sETag, CeHeader.m_lenETag);
#endif
    }

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
#ifdef CACHE_RESP_HEADER
            headersBufSize += writeHttpHeader(fd, &(myData->m_pEntry->m_sRespHeader),
                                              iov_key[i].iov_base, iov_key[i].iov_len,
                                              iov_val[i].iov_base, iov_val[i].iov_len);
#else
            headersBufSize += writeHttpHeader(fd, NULL,
                                              (char *)iov_key[i].iov_base, iov_key[i].iov_len,
                                              (char *)iov_val[i].iov_base, iov_val[i].iov_len);
#endif
    }

#ifdef CACHE_RESP_HEADER
    if (myData->m_pEntry->m_sRespHeader.len() > 4096)
        myData->m_pEntry->m_sRespHeader.prealloc(0);
#endif

    myData->pEntry->setContentLen(CeHeader.m_iLenETag + headersBufSize, 0);

    long iCahcedSize = 0;
    off_t offset = 0;
    const char *pBuf;
    int len = 0;
    void *pRespBodyBuf = g_api->get_resp_body_buf(rec->session);

    while (!g_api->is_body_buf_eof(pRespBodyBuf, offset))
    {
        pBuf = g_api->acquire_body_buf_block(pRespBodyBuf, offset, &len);
        if (!pBuf)
            break;
        write(fd, pBuf, len);
        g_api->release_body_buf_block(pRespBodyBuf, offset);
        offset += len;
        iCahcedSize += len;
    }

    int part1Len = myData->pEntry->getContentTotalLen();
    myData->pEntry->setContentLen(part1Len, iCahcedSize);

    if (myData->pEntry->m_sPart3Buf.len() > 8)
        write(fd, myData->pEntry->m_sPart3Buf.c_str(),
              myData->pEntry->m_sPart3Buf.len());

    myData->pDirHashCacheStore->publish(myData->pEntry);
    myData->iCacheState = CE_STATE_CACHED;  //Succeed
    g_api->free_module_data(rec->session, &MNAME, LSI_DATA_HTTP,
                            httpRelease);

    g_api->log(rec->session, LSI_LOG_DEBUG,
               "[%s:cacheTofile] stored, size %ld\n",
               ModuleNameStr, offset);
    return 0;
}


/*****
 * save to part3 data
 * Format is
 * 4 bytes of length of the envKey1 + envKey1
 * 4 bytes of length of the value1 + value1
 * ...
 * ...
 * 4 bytes of length of the envKeyN + envKeyN
 * 4 bytes of length of the valueN + valueN
 */
int setCacheUserData(lsi_param_t *rec)
{
    const char *p = (char *)rec->ptr1;
    int lenTotal = rec->len1;
    const char *pEnd = p + lenTotal;

    if (lenTotal < 8)
        return LS_FAIL;


    //Check the kvPair length matching
    int len;
    while (p < pEnd)
    {
        memcpy(&len, p, 4);
        if (len < 0 || len > lenTotal)
            return LS_FAIL;

        lenTotal -= (4 + len);
        p += (4 + len);
    }

    if (lenTotal != 0)
        return LS_FAIL;


    MyMData *myData = (MyMData *)g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData == NULL) //|| myData->iCacheState != CE_STATE_CACHED)
        return LS_FAIL;

    if (rec->len1 < 8)
        return LS_FAIL;

    myData->pEntry->m_sPart3Buf.append((char *)rec->ptr1, rec->len1);

    g_api->log(rec->session, LSI_LOG_DEBUG,
               "[%s:setCacheUserData] written %d\n", ModuleNameStr,
               rec->len1);
    return 0;
}


//get the part3 data
int getCacheUserData(lsi_param_t *rec)
{
    MyMData *myData = (MyMData *)g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);
    if (myData == NULL)
        return LS_FAIL;


//     char **buf = (char **)rec->_param;
//     int *len = (int *)(long)rec->_param_len;
//     *buf = (char *)myData->pEntry->m_sPart3Buf.c_str();
//     *len = myData->pEntry->m_sPart3Buf.len();

    return 0;
}


static int checkAssignHandler(lsi_param_t *rec)
{
    char httpMethod[10] = {0};
    int uriLen = g_api->get_req_org_uri(rec->session, NULL, 0);
    if (uriLen <= 0)
    {
        g_api->log(rec->session, LSI_LOG_ERROR,
                   "[%s]checkAssignHandler error 1.\n", ModuleNameStr);
        return 0;
    }

    CacheConfig *pConfig = (CacheConfig *)g_api->get_config(
                               rec->session, &MNAME);
    if (!pConfig)
    {
        g_api->log(rec->session, LSI_LOG_ERROR,
                   "[%s]checkAssignHandler error 2.\n", ModuleNameStr);
        return 0;
    }

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
        {
            //Do not store method for handling, only store the time
            method = HTTP_POST;
            g_api->set_module_data(rec->session, &MNAME, LSI_DATA_IP,
                                   (void *)(long)DateTime_s_curTime);
        }
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

    if (method == HTTP_UNKNOWN || method == HTTP_POST)
    {
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]checkAssignHandler returned, method %s[%d].\n",
                   ModuleNameStr, httpMethod, method);
        return 0;
    }

    //If it is range request, quit
    int rangeRequestLen = 0;
    const char *rangeRequest = g_api->get_req_header_by_id(rec->session,
                               LSI_HDR_RANGE, &rangeRequestLen);
    if (rangeRequest && rangeRequestLen > 0)
    {
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]checkAssignHandler returned, not support rangeRequest [%s].\n",
                   ModuleNameStr, rangeRequest);
        return 0;
    }

    int iQSLen;
    const char *pQS = g_api->get_req_query_string(rec->session, &iQSLen);

    //If no env and rewrite-rule, then check
    char cacheEnv[MAX_CACHE_CONTROL_LENGTH] = {0};
    int cacheEnvLen = g_api->get_req_env(rec->session, "cache-control", 13,
                                         cacheEnv, MAX_CACHE_CONTROL_LENGTH);

    CacheCtrl cacheCtrl;
    if (method == HTTP_GET || method == HTTP_HEAD)
    {
        if (rec->ptr1 == NULL && cacheEnvLen == 0 &&
            (pConfig->isPublicPrivateEnabled() == 0 ||
             (!pConfig->isSet(CACHE_QS_CACHE) && pQS && iQSLen > 0)))
        {
            g_api->log(rec->session, LSI_LOG_DEBUG,
                       "[%s]checkAssignHandler returned, for cache disabled or has QS but qscache disabled.\n",
                       ModuleNameStr);
            clearHooks(rec->session);
            return 0;
        }

        //Now parse the req header
        int flag = CacheCtrl::max_age | CacheCtrl::max_stale | CacheCtrl::s_maxage;
        if (pConfig->isSet(CACHE_ENABLED))
            flag |= CacheCtrl::cache_public;
        if (pConfig->isSet(CACHE_PRIVATE_ENABLED))
            flag |= CacheCtrl::cache_private;
        cacheCtrl.init(flag, pConfig->getDefaultAge(), pConfig->getMaxStale());
        if (!pConfig->isSet(CACHE_IGNORE_REQ_CACHE_CTRL_HEADER))
        {
            int bufLen;
            const char *buf = g_api->get_req_header_by_id(rec->session,
                              LSI_HDR_CACHE_CTRL, &bufLen);
            if (buf)
                cacheCtrl.parse(buf, bufLen);
        }
        if (!pConfig->isSet(CACHE_IGNORE_RESP_CACHE_CTRL_HEADER))
        {
            iovec iov[3];
            int count = g_api->get_resp_header(rec->session,
                                               LSI_RSPHDR_CACHE_CTRL, NULL, 0, iov, 3);
            for (int i = 0; i < count; ++i)
                cacheCtrl.parse((char *)iov[i].iov_base, iov[i].iov_len);
        }

        //this go from env or rewrite-rule
        if (rec->ptr1 != NULL && rec->len1 > 0)
            cacheCtrl.parse((const char *)rec->ptr1, rec->len1);
        if (cacheEnvLen > 0)
            cacheCtrl.parse((const char *)cacheEnv, cacheEnvLen);
        if (cacheCtrl.isCacheOff())
        {
            g_api->log(rec->session, LSI_LOG_DEBUG,
                       "[%s]checkAssignHandler returned, for cache disabled.\n",
                       ModuleNameStr);
            clearHooks(rec->session);
            return 0;
        }
    }

    MyMData *myData = (MyMData *) g_api->get_module_data(rec->session, &MNAME,
                      LSI_DATA_HTTP);
    if (!myData)
    {
        char host[512] = {0};
        int hostLen = g_api->get_req_var_by_id(rec->session,
                                               LSI_VAR_SERVER_NAME, host, 512);
        char port[12] = {0};
        int portLen = g_api->get_req_var_by_id(rec->session,
                                               LSI_VAR_SERVER_PORT, port, 12);

        //host:port uri
        char *uri = new char[uriLen + hostLen + portLen + 2];
        strncpy(uri, host, hostLen);
        uri[hostLen] = ':';
        strncpy(uri + hostLen + 1, port, portLen);
        g_api->get_req_org_uri(rec->session, uri + hostLen + 1 + portLen,
                               uriLen + 1);
        uriLen += (hostLen + 1 + portLen); //Set the the right uriLen
        uri[uriLen] = 0x00; //NULL terminated

        myData = new MyMData;
        memset(myData, 0, sizeof(MyMData));
        myData->pConfig = pConfig;
        myData->pOrgUri = uri;
        myData->iMethod = method;

        myData->pDirHashCacheStore = s_pDefaultStore;

        char cachePath[max_file_len]  = {0};
        const char *pPath = myData->pConfig->getStoragePath();
        if (!pPath || strlen(pPath) == 0)
            pPath = CACHEMODULEROOT;

        if (pPath[0] != '/')
            strcpy(cachePath, g_api->get_server_root());

        strcat(cachePath, pPath);

        struct stat st;
        if (stat(cachePath, &st) == -1)
        {
            g_api->log(rec->session, LSI_LOG_ERROR,
                       "[%s]checkAssignHandler failed to create directory [%s].\n",
                       ModuleNameStr, cachePath);
            clearHooks(rec->session);
            return 0;
        }
        myData->pDirHashCacheStore->setStorageRoot(cachePath);

        myData->iCacheState = hasCache(rec, myData->pOrgUri, uriLen,
                                       myData->pDirHashCacheStore, &myData->cePublicHash, &myData->cePrivateHash,
                                       pConfig, &myData->pEntry);
        g_api->set_module_data(rec->session, &MNAME, LSI_DATA_HTTP,
                               (void *)myData);
    }
    //multiple calling of this function, Cachectrl may be updated
    myData->cacheCtrl = cacheCtrl;

    if (myData->iCacheState == CE_STATE_HAS_RIVATECACHE ||
        (myData->iCacheState == CE_STATE_HAS_PUBLIC_CACHE
         && cacheCtrl.isPublicCacheable()) ||
        myData->iMethod == HTTP_PURGE ||
        myData->iMethod == HTTP_REFRESH)
    {
        g_api->register_req_handler(rec->session, &MNAME, 0);
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]checkAssignHandler register_req_handler OK.\n", ModuleNameStr);
    }
    else if (myData->iMethod == HTTP_GET
             && myData->iCacheState == CE_STATE_NOCACHE)
    {
        //only GET need to store, HEAD won't
        int aEnableHkpt[] = {LSI_HKPT_RCVD_RESP_HEADER,
                             LSI_HKPT_HANDLER_RESTART
                            };
        g_api->enable_hook(rec->session, &MNAME, 1,
                                            aEnableHkpt, 2);
        myData->iHaveAddedHook = 1;

        //g_api->set_session_hook_flag( rec->_session, LSI_HKPT_RCVD_RESP_BODY, &MNAME, 1 );
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]checkAssignHandler Add Hooks.\n", ModuleNameStr);
    }
    else
    {
        g_api->free_module_data(rec->session, &MNAME, LSI_DATA_HTTP,
                                httpRelease);
        g_api->log(rec->session, LSI_LOG_DEBUG,
                   "[%s]checkAssignHandler won't do anything and quit.\n", ModuleNameStr);
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
    {LSI_HKPT_RECV_REQ_HEADER, checkAssignHandler, LSI_HOOK_LAST, LSI_FLAG_ENABLED},
    {LSI_HKPT_HTTP_END, cancelCache, LSI_HOOK_LAST, LSI_FLAG_ENABLED},
    {LSI_HKPT_RCVD_RESP_HEADER, createEntry, LSI_HOOK_LAST, 0}, //Disabled
    {LSI_HKPT_HANDLER_RESTART, cancelCache, LSI_HOOK_LAST, 0},  //Disabled
    {LSI_HKPT_RCVD_RESP_BODY, cacheTofile, LSI_HOOK_LAST, 0},   //Disabled
    LSI_HOOK_END   //Must put this at the end position
};


static int init(lsi_module_t *pModule)
{
    if (!initDefaultCacheStore())
        return LS_FAIL;

    g_api->init_module_data(pModule, httpRelease, LSI_DATA_HTTP);
    g_api->init_module_data(pModule, releaseIpCounter, LSI_DATA_IP);
    g_api->register_env_handler("cache-control", 13, checkAssignHandler);
    g_api->register_env_handler("setcachedata", 12, setCacheUserData);
    g_api->register_env_handler("getcachedata", 12, getCacheUserData);

    return 0;
}


int isModified(lsi_session_t *session, CeHeader &CeHeader, char *etag,
               int etagLen)
{
    int len;
    const char *buf = NULL;

    if (etagLen > 0)
    {
        buf = g_api->get_req_header(session, "If-None-Match", 13, &len);
        if (buf && len == etagLen && memcmp(etag, buf, etagLen) == 0)
            return 0;
    }

    buf = g_api->get_req_header(session, "If-Modified-Since", 17, &len);
    if (buf && len >= RFC_1123_TIME_LEN &&
        CeHeader.m_tmLastMod <= DateTime::parseHttpTime(buf))
        return 0;

    return 1;
}


static int handlerProcess(lsi_session_t *session)
{
    MyMData *myData = (MyMData *)g_api->get_module_data(session, &MNAME,
                      LSI_DATA_HTTP);
    if (!myData)
    {
        g_api->log(session, LSI_LOG_ERROR,
                   "[%s]internal error during handlerProcess.\n", ModuleNameStr);
        return 500;
    }

    if (myData->iMethod == HTTP_PURGE || myData->iMethod == HTTP_REFRESH)
    {
        int len;
        const char *pIP = g_api->get_client_ip(session, &len);
        if (!pIP || strncmp(pIP, "127.0.0.1", 9) != 0)
        {
            g_api->free_module_data(session, &MNAME, LSI_DATA_HTTP,
                                    httpRelease);
            return 405;
        }

        if (myData->pEntry)
        {
            if (myData->iMethod == HTTP_PURGE)
                myData->pDirHashCacheStore->purge(myData->pEntry);
            else
                myData->pDirHashCacheStore->refresh(myData->pEntry);
        }
        g_api->end_resp(session);
        g_api->free_module_data(session, &MNAME, LSI_DATA_HTTP,
                                httpRelease);
        return 511;
    }


    char tmBuf[RFC_1123_TIME_LEN + 1];
    int len;
    int fd = myData->pEntry->getFdStore();
    CeHeader &CeHeader = myData->pEntry->getHeader();

    //If has part3 data, cache will not handle it, broadcast to who can handle it
    if (myData->pEntry->m_sPart3Buf.len() >= 8)
    {
        const char *p = myData->pEntry->m_sPart3Buf.c_str();
        const char *pEnd = p + myData->pEntry->m_sPart3Buf.len();

        int envKeylen, envValueLen;
        while (p < pEnd)
        {
            memcpy(&envKeylen, p, 4);
            if (envKeylen < 0 || envKeylen > pEnd - p - 4)
                break;

            memcpy(&envValueLen, p + 4 + envKeylen, 4);
            if (envValueLen < 0 || envValueLen > pEnd - p - 8 - envKeylen)
                break;

            g_api->set_req_env(session, p + 4, envKeylen, p + 8 + envKeylen,
                               envValueLen);  // trigger the next handler cb

            p += 8 + envKeylen + envKeylen;
        }
    }

    char *buff = NULL;
    char *pBuffOrg = NULL;
    if (myData->pEntry->getPart2Offset() - myData->pEntry->getPart1Offset() >
        0)
    {
#ifdef CACHE_RESP_HEADER
        if (myData->m_pEntry->m_sRespHeader.len() > 0) //has it
            buff = (char *)(myData->m_pEntry->m_sRespHeader.c_str());
        else
#endif
        {
            buff  = (char *)mmap((caddr_t)0, myData->pEntry->getPart2Offset(),
                                 PROT_READ, MAP_SHARED, fd, 0);
            if (buff == (char *)(-1))
            {
                g_api->free_module_data(session, &MNAME, LSI_DATA_HTTP,
                                        httpRelease);
                return 500;
            }
            pBuffOrg = buff;
            buff += myData->pEntry->getPart1Offset();
        }

        if (CeHeader.m_iLenETag > 0)
        {
            g_api->set_resp_header(session, LSI_RSPHDR_ETAG, NULL, 0, buff,
                                   CeHeader.m_iLenETag, LSI_HEADEROP_SET);
            if (!isModified(session, CeHeader, buff, CeHeader.m_iLenETag))
            {
                if (myData->iCacheState == CE_STATE_HAS_RIVATECACHE)
                    g_api->set_resp_header2(session, s_x_cached_private,
                                            sizeof(s_x_cached_private) - 1, LSI_HEADEROP_SET);
                else
                    g_api->set_resp_header2(session, s_x_cached, sizeof(s_x_cached) - 1,
                                            LSI_HEADEROP_SET);

                g_api->set_status_code(session, 304);
                if (pBuffOrg)
                    munmap((caddr_t)pBuffOrg, myData->pEntry->getPart2Offset());
                g_api->free_module_data(session, &MNAME, LSI_DATA_HTTP,
                                        httpRelease);
                g_api->end_resp(session);
                return 0;
            }
        }

        buff += CeHeader.m_iLenETag;
        len = myData->pEntry->getPart2Offset() - myData->pEntry->getPart1Offset() -
              CeHeader.m_iLenETag;
        g_api->set_resp_header2(session, buff, len, LSI_HEADEROP_SET);
    }

    if (CeHeader.m_tmLastMod != 0)
    {
        g_api->set_resp_header(session, LSI_RSPHDR_LAST_MODIFIED, NULL, 0,
                               DateTime::getRFCTime(CeHeader.m_tmLastMod, tmBuf),
                               RFC_1123_TIME_LEN, LSI_HEADEROP_SET);
    }
    if (myData->iCacheState == CE_STATE_HAS_RIVATECACHE)
        g_api->set_resp_header2(session, s_x_cached_private,
                                sizeof(s_x_cached_private) - 1, LSI_HEADEROP_SET);
    else
        g_api->set_resp_header2(session, s_x_cached, sizeof(s_x_cached) - 1,
                                LSI_HEADEROP_SET);


//     time_t tmExpired = DateTime_s_curTime + 604800;
//     g_api->set_resp_header(session, "Expires", 7,
//                                     DateTime::getRFCTime(tmExpired, buf), RFC_1123_TIME_LEN);
//
//     g_api->set_resp_header(session, "Cache-Control", 13, "max-age=604800", 14);


//      CeHeader.m_tmExpire > (int32_t)DateTime_s_curTime
//      unlink file ???

    g_api->set_status_code(session, CeHeader.m_iStatusCode);

    int ret  = 0;
    if (myData->iMethod == HTTP_GET)
    {
        char filePath[max_file_len] = {0};
        myData->pEntry->getFilePath(filePath, max_file_len);
//        g_api->remove_resp_header(session, LSI_RSPHDR_TRANSFER_ENCODING, NULL, 0);
        off_t length = myData->pEntry->getContentTotalLen() -
                       (myData->pEntry->getPart2Offset() - myData->pEntry->getPart1Offset());
        off_t startPos = myData->pEntry->getPart2Offset();

        if (myData->pEntry->isGzipped())
        {
            g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_ENCODING,
                                   NULL, 0, "gzip", 4, LSI_HEADEROP_SET);
            g_api->log(session, LSI_LOG_DEBUG,
                       "[%s]set_resp_header [Content-Encoding: gzip].\n",
                       ModuleNameStr);
            g_api->set_resp_buffer_gzip_flag(session, 1);
        }
        else
            g_api->set_resp_buffer_gzip_flag(session, 0);

        g_api->set_resp_content_length(session, length);
        if (g_api->send_file(session, filePath, startPos, length) == 0)
            g_api->end_resp(session);
        else
            ret = 500;
    }
    else //HEAD
        g_api->end_resp(session);

    if (pBuffOrg)
        munmap((caddr_t)pBuffOrg, myData->pEntry->getPart2Offset());

    g_api->free_module_data(session, &MNAME, LSI_DATA_HTTP,
                            httpRelease);
    return ret;
}

lsi_reqhdlr_t cache_handler = { handlerProcess, NULL, NULL, NULL };
lsi_confparser_t cacheDealConfig = { ParseConfig, FreeConfig, paramArray };
lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init, &cache_handler,
                       &cacheDealConfig, "cache v1.3", serverHooks, {0}
                     };

