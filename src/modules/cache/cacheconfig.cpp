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
#include "ls.h"
#include <new>

#include "dirhashcachestore.h"
#include <http/httpvhost.h>


int CacheKeyModList::parseAppend(const char *pConfig, int len)
{
    int op = 0;
    if (len < 5)
        return -1;

    if ((*pConfig | 0x20) == 'c' && len >= 5)
    {
        if (strncasecmp(pConfig, "clear", 5) == 0)
        {
            clear();
            return 0;
        }
    }

    if (*pConfig != '-')
        return -1;

    const char *pEnd = pConfig + len;
    ++pConfig;
    if ((*pConfig | 0x20) == 'q' && (*(pConfig + 1) | 0x20) == 's'
        && *(pConfig + 2) == ':')
    {
        pConfig += 3;
        if (*(pEnd - 1) == '*')
        {
            op = CACHE_KEY_QS_DEL_PREFIX;
            --pEnd;
        }
        else
            op = CACHE_KEY_QS_DEL_EXACT;
        CacheKeyMod *obj = new(newObj()) CacheKeyMod();
        obj->m_operator = op;
        ls_str_dup(&obj->m_str, pConfig, pEnd - pConfig);
    }
    return 0;
}


CacheConfig::CacheConfig()
    : m_iCacheConfigBits(0)
    , m_iCacheFlag(0x0027C)   //0 0000 0010 0111 1100
    , m_defaultAge(3600)
    , m_privateAge(3600)
    , m_iMaxStale(200)
    , m_lMaxObjSize(10000000)
      //, m_iBypassPercentage(5)
    , m_iLevele(0)
    , m_iAddEtag(0)
    , m_iOnlyUseOwnUrlExclude(0)
    , m_iOwnStore(0)
    , m_iOwnPurgeUri(0)
    , m_iOwnVaryList(0)
    , m_pUrlExclude(NULL)
    , m_pParentUrlExclude(NULL)
    , m_pVHostMapExclude(NULL)
    , m_pStore(NULL)
    , m_pPurgeUri(NULL)
    , m_pVaryList(NULL)
    , m_pKeyModList(NULL)
{
}


CacheConfig::~CacheConfig()
{
    if (m_pUrlExclude)
        delete m_pUrlExclude;
    if (m_iLevele == LSI_CFG_SERVER && m_pVHostMapExclude)
        delete m_pVHostMapExclude;
    if (m_iOwnStore && m_pStore)
        delete m_pStore;
    if (m_iOwnPurgeUri && m_pPurgeUri)
        free(m_pPurgeUri);
    if (m_iOwnVaryList && m_pVaryList)
        delete m_pVaryList;
    if (m_pKeyModList && (m_iCacheConfigBits & CACHE_KEY_MOD_SET))
        delete m_pKeyModList;

    m_pUrlExclude = NULL;
    m_pVaryList = NULL;
    m_pPurgeUri = NULL;
    m_pParentUrlExclude = NULL;
    m_pVHostMapExclude = NULL;
    m_pStore = NULL;
    m_iOwnStore = 0;
    m_iOwnPurgeUri = 0;
    m_iOwnVaryList = 0;
}


void CacheConfig::inherit(const CacheConfig *pParent)
{
    if (pParent)
    {
        m_pKeyModList = pParent->m_pKeyModList;
        m_iCacheFlag = (m_iCacheFlag & m_iCacheConfigBits) |
                       (pParent->m_iCacheFlag & ~m_iCacheConfigBits);
        m_pParentUrlExclude = pParent->m_pUrlExclude;
        m_pUrlExclude = NULL;
        m_pVHostMapExclude = pParent->m_pVHostMapExclude;
        m_iOnlyUseOwnUrlExclude = 0;
        m_pStore = pParent->getStore();
        m_iOwnStore = 0;
        m_iAddEtag = pParent->getAddEtagType();
        m_pPurgeUri = pParent->getPurgeUri();
        m_iOwnPurgeUri = 0;
        m_pVaryList = pParent->getVaryList();
        m_iOwnVaryList = 0;
    }
}


void CacheConfig::apply(const CacheConfig *pParent)
{
    if (pParent)
    {
        if (pParent->m_iCacheConfigBits & CACHE_MAX_AGE_SET)
            m_defaultAge = pParent->m_defaultAge;
        if (pParent->m_iCacheConfigBits & CACHE_PRIVATE_AGE_SET)
            m_privateAge = pParent->m_privateAge;
        if (pParent->m_iCacheConfigBits & CACHE_STALE_AGE_SET)
            m_iMaxStale = pParent->m_iMaxStale;
        if (pParent->m_iCacheConfigBits & CACHE_MAX_OBJ_SIZE)
            m_lMaxObjSize = pParent->m_lMaxObjSize;

        m_iCacheFlag = (pParent->m_iCacheFlag & pParent->m_iCacheConfigBits) |
                       (m_iCacheFlag & ~pParent->m_iCacheConfigBits);
    }

}

#define CACHE_LITEMAGE_BITS (CACHE_QS_CACHE | CACHE_REQ_COOKIE_CACHE \
                             | CACHE_IGNORE_REQ_CACHE_CTRL_HEADER \
                             | CACHE_IGNORE_RESP_CACHE_CTRL_HEADER \
                             | CACHE_RESP_COOKIE_CACHE \
                             | CACHE_CHECK_PUBLIC  )

int CacheConfig::isLitemagReady()
{
    if ((m_iCacheFlag & (CACHE_LITEMAGE_BITS | CACHE_ENABLE_PUBLIC
                         | CACHE_ENABLE_PRIVATE)) ==  CACHE_LITEMAGE_BITS)
    {
        if (m_lMaxObjSize >= 500 * 1024)
            return 1;
    }
    return 0;
}


void CacheConfig::setLitemageDefault()
{
    setConfigBit(CACHE_LITEMAGE_BITS, 1);
    setConfigBit(CACHE_ENABLE_PUBLIC | CACHE_ENABLE_PRIVATE, 0);
    setMaxObjSize(1024 * 1024);
}

void CacheConfig::setVaryList(StringList *pList)
{
    if (m_iOwnVaryList && m_pVaryList)
        delete m_pVaryList;
    m_pVaryList = pList;
    m_iOwnVaryList = 1;
}


int CacheConfig::parseCacheKeyMod(const char *pConfig, int len)
{   
    if ((m_iCacheConfigBits & CACHE_KEY_MOD_SET) == 0)
    {
        CacheKeyModList *pParent = m_pKeyModList;
        m_pKeyModList = new CacheKeyModList();
        m_pKeyModList->setParent(pParent);
        m_iCacheConfigBits = m_iCacheConfigBits | CACHE_KEY_MOD_SET;
    }
    return m_pKeyModList->parseAppend(pConfig, len);
}
