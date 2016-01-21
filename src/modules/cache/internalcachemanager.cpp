/*
 *
 */

#include "internalcachemanager.h"
#include "cacheentry.h"

#include <assert.h>
#include <ctype.h>



class PurgeData
{
public:

    PurgeData()
    {
        memset(this, 0, sizeof(*this));
    }

    ~PurgeData()
    {
    }


    void setFlag(int flag)        {   m_purgeinfo.flags = flag;         }
    int16_t getFlag()   const       {   return m_purgeinfo.flags;         }
    void setExpireTime(int32_t sec, int16_t msec)
    {   m_purgeinfo.tmSecs = sec; m_purgeinfo.tmMsec = msec;       }

    int shouldExpire(int32_t sec, int16_t msec)
    {
        return ((m_purgeinfo.tmSecs > sec) ||
                ((m_purgeinfo.tmSecs == sec) && (m_purgeinfo.tmMsec >= msec)));
    }

    int getTagId() const    {   return m_purgeinfo.idTag;   }
    void setTagId(int id)   {   m_purgeinfo.idTag = id;     }

private:
    purgeinfo_t   m_purgeinfo;
};

class PurgeData2 : public PurgeData
{
public:
    PurgeData2()
    {
        memset(this, 0, sizeof(*this));
    }

    ~PurgeData2()
    {}

    const char *getKey() const             {   return m_sKey.c_str(); }

    void setKey(const char *pKey, int len)
    {
        m_sKey.setStr(pKey, len);
    }

    int match(const char *pKey, int len);

private:
    AutoStr2   m_sKey;
};

class Str2Id
{
public:
    Str2Id(const char *pString, int len, int id)
        : m_str(pString, len)
        , m_id(id)
    {}
    ~Str2Id()
    {}

    const AutoStr2 *getStr() const      {   return &m_str;      }
    int getId() const                   {   return m_id;        }
    void setId(int id)                 {   m_id = id;          }

private:
    AutoStr2    m_str;
    int         m_id;
};


class PrivatePurgeData
{
public:
    PrivatePurgeData()
        : m_tmLast(0)
        , m_msLast(0)
    {}
    ~PrivatePurgeData()
    {   m_listTags.release_objects();    }

    void setId(const char *pId, int len) {   m_sId.setStr(pId, len);   }
    const AutoStr2 *getId() const          {   return &m_sId;              }
    PurgeData *addUpdate(int idTag, int flag, int32_t sec, int16_t msec);
    int shouldPurge(int idTag, int32_t sec, int16_t msec);
    void setLastPurgeTime(int32_t sec, int16_t msec)
    {   m_tmLast = sec; m_msLast = msec;        }
private:
    AutoStr2    m_sId;
    TPointerList< PurgeData >    m_listTags;
    int32_t    m_tmLast;
    int32_t    m_msLast;

};


// int PurgeData::setRegex( const char *pPattern )
// {
//     if ( !m_pRegex )
//         m_pRegex = new Pcregex();
//     if ( m_pRegex->compile( pPattern, REG_EXTENDED ) == -1 )
//     {
//         delete m_pRegex ;
//         m_pRegex = NULL;
//         return -1;
//     }
//     return 0;
// }

int PurgeData2::match(const char *pKey, int len)
{
    if ((len == m_sKey.len())
        && (memcmp(pKey, m_sKey.c_str(), len) == 0))
        return 1;
    return 0;
}


PurgeData *PrivatePurgeData::addUpdate(int idTag, int flag, int32_t sec,
                                       int16_t msec)
{
    if (m_tmLast < sec)
        m_tmLast = sec;
    PurgeData *p = NULL;
    TPointerList< PurgeData >::iterator iter = m_listTags.begin();
    while (iter < m_listTags.end())
    {
        if ((*iter)->getTagId() == idTag)
        {
            p = *iter;
            break;
        }
        ++iter;
    }

    if (!p)
    {
        p = new PurgeData();
        if (!p)
            return NULL;
        p->setTagId(idTag);
        m_listTags.push_back(p);
    }
    p->setExpireTime(sec, msec);
    p->setFlag(flag);
    return p;
}


int PrivatePurgeData::shouldPurge(int idTag, int32_t sec, int16_t msec)
{
    if (sec > m_tmLast)    //newer than all entries in current list
        return 0;
    TPointerList< PurgeData >::iterator iter = m_listTags.begin();
    while (iter < m_listTags.end())
    {
        if ((*iter)->getTagId() == idTag && (*iter)->shouldExpire(sec, msec))
            return (*iter)->getFlag();
        ++iter;
    }
    return 0;
}




InternalCacheManager::InternalCacheManager()
    : m_pInfo(NULL)
{
    m_pInfo = new CacheInfo();
    if (m_pInfo)
        memset(m_pInfo, 0, sizeof(*m_pInfo));
}


InternalCacheManager::~InternalCacheManager()
{
    m_privateList.release_objects();
    m_hashTags.release_objects();
    m_str2IdHash.release_objects();
    m_urlVary.release_objects();
    if (m_pInfo != NULL)
        delete m_pInfo;
}

int InternalCacheManager::init(const char *pStoreDir)
{
    populatePrivateTag();
    return 0;
}



PrivatePurgeData *InternalCacheManager::getPrivate(const char *pKey,
        int len)
{
    HashStringMap< PrivatePurgeData *>::iterator iter = m_privateList.find(
                pKey);
    if (iter != m_privateList.end())
        return iter.second();
    PrivatePurgeData *pData = new PrivatePurgeData();
    if (pData)
    {
        pData->setId(pKey, len);
        m_privateList.insert(pData->getId()->c_str(), pData);
    }
    return pData;
}


PrivatePurgeData *InternalCacheManager::findPrivate(CacheKey *pKey)
{
    char achKey[ 8192 ];
    int len;
    len = pKey->getPrivateId(achKey, &achKey[8192]);
    if (len > 0)
    {
        HashStringMap< PrivatePurgeData *>::iterator iter = m_privateList.find(
                    achKey);
        if (iter != m_privateList.end())
            return iter.second();
    }
    return NULL;
}


void *InternalCacheManager::getPrivateCacheInfo(const char *pPrivateId,
        int len, int force)
{
    return NULL;
}


int InternalCacheManager::setPrivateTagFlag(void *pPrivatePurgeData,
        int tagId, uint8_t flag)
{
    return LS_FAIL;
}


purgeinfo_t *InternalCacheManager::getPrivateTagInfo(
    void *pPrivatePurgeData, int tagId)
{
    return NULL;
}


int InternalCacheManager::setVerifyKey(void *pPrivatePurgeData,
                                       const char  *pVerifyKey, int len)
{
    return LS_FAIL;
}


const char *InternalCacheManager::getVerifyKey(void *pPrivatePurgeData,
        int *len)
{
    return NULL;
}


int InternalCacheManager::findTagId(const char *pTag, int len)
{
    HashStringMap<Str2Id *>::iterator iter;
    iter = m_str2IdHash.find(pTag);
    if (iter != m_str2IdHash.end())
        return iter.second()->getId();
    return -1;
}


int InternalCacheManager::getTagId(const char *pTag, int len)
{
    HashStringMap<Str2Id *>::iterator iter;
    iter = m_str2IdHash.find(pTag);
    if (iter != m_str2IdHash.end())
        return iter.second()->getId();
    else
    {
        int id = m_pInfo->getNextPrivateTagId();
        Str2Id *pStr2Id = new Str2Id(pTag, len, id);
        iter = m_str2IdHash.insert(pStr2Id->getStr()->c_str(), pStr2Id);
        if (iter == m_str2IdHash.end())
            return -1;
        return id;
    }
}


PurgeData *InternalCacheManager::addUpdate(const char *pKey, int keyLen,
        int flag, int32_t sec, int16_t msec)
{
    PurgeData2 *pData = NULL;
    HashStringMap< PurgeData2 *>::iterator iter = m_hashTags.find(pKey);
    if (iter != m_hashTags.end())
        pData = iter.second();

    if (!pData)
    {
        pData = new PurgeData2();
        if (!pData)
            return NULL;
        pData->setKey(pKey, keyLen);
        m_hashTags.insert(pData->getKey(), pData);
    }
    pData->setExpireTime(sec, msec);
    pData->setFlag(flag);
    return pData;
}


// x-litespeed-purge: private, *
// x-litespeed-purge: *
// x-litespeed-purge: private, tag=abc*
// x-litespeed-purge: tag=abc*
// x-litespeed-purge: /my/url/*
// x-litespeed-purge: private, /my/url/*

int InternalCacheManager::processPrivatePurgeCmd(CacheKey *pKey,
        const char *pValue, int iValLen, time_t curTime, int curTimeMS)
{
    char achKey[ 8192 ];
    int len;
    len = pKey->getPrivateId(achKey, &achKey[8192]);
    if (len <= 0)
        return -1;
    PrivatePurgeData *pData = getPrivate(achKey, len);
    return processPurgeCmdEx(pData, pValue, iValLen, curTime, curTimeMS);
}


int InternalCacheManager::processPurgeCmdEx(PrivatePurgeData *pPrivate,
        const char *pValue, int iValLen, time_t curTime, int curTimeMs)
{
    int flag;
    const char *pValueEnd, *pNext;
    const char *pEnd = pValue + iValLen;
    while (pValue < pEnd)
    {
        if (isspace(*pValue))
        {
            ++pValue;
            continue;
        }
        pValueEnd = (const char *)memchr(pValue, ',', pEnd - pValue);
        if (!pValueEnd)
            pValueEnd = pNext = pEnd;
        else
            pNext = pValueEnd + 1;

        while (isspace(pValueEnd[-1]))
            --pValueEnd;

        flag = 0;
        if (strncmp(pValue, "tag=", 4) == 0)
        {
            pValue += 4;
            flag |= PDF_TAG;
        }
        if (*pValue == '*')
        {
            if (pValue == pValueEnd - 1)
            {
                //only a "*".
                if (pPrivate)
                    pPrivate->setLastPurgeTime(curTime, curTimeMs);
                else
                    m_pInfo->setPurgeTime(curTime, curTimeMs);
                pValue = pNext;
                continue;
            }
            else
            {
                //prefix
                flag |= PDF_PREFIX;
            }
        }
        else if (pValueEnd[-1] == '*')
            flag |= PDF_POSTFIX;
        if (pPrivate)
        {
            int id;
            char *pEnd = (char *)pValueEnd;
            char ch = *pEnd;

            *pEnd = 0;
            id = getTagId(pValue, pValueEnd - pValue);
            *pEnd = ch;
            pPrivate->addUpdate(id, flag, curTime, curTimeMs);
        }
        else
            addUpdate(pValue, pValueEnd - pValue, flag, curTime, curTimeMs);
        pValue = pNext;
    }
    return 0;
}


int InternalCacheManager::shouldPurge(const char *pKey, int keyLen,
                                      int32_t sec, int16_t msec)
{
    PurgeData *pData = NULL;
    HashStringMap< PurgeData *>::iterator iter = m_hashTags.find(pKey);
    if (iter != m_hashTags.end())
    {
        pData = iter.second();
        if (pData->shouldExpire(sec, msec))
            return pData->getFlag();
    }
    return 0;
}

int InternalCacheManager::isPurgedByTag(const char *pTag,
                                        CacheEntry *pEntry, CacheKey *pKey)
{
    if (pKey->m_pIP)
    {
        //assert( "NO_PRIVATE_PURGE_YET" == NULL );
        PrivatePurgeData *pData = findPrivate(pKey);
        if (pData)
        {
            int tagId = findTagId(pTag, pEntry->getHeader().m_tagLen);
            if (tagId == -1)
                return 0;
            if (pData->shouldPurge(tagId, pEntry->getHeader().m_tmCreated,
                                   pEntry->getHeader().m_msCreated))
                return 1;
        }
    }
    else
    {
        if (shouldPurge(pTag, pEntry->getHeader().m_tagLen,
                        pEntry->getHeader().m_tmCreated, pEntry->getHeader().m_msCreated))
            return 1;

    }
    return 0;
}


int InternalCacheManager::isPurged(CacheEntry *pEntry, CacheKey *pKey)
{
    if (m_pInfo->shouldPurge(pEntry->getHeader().m_tmCreated,
                             pEntry->getHeader().m_msCreated))
        return 1;
    const char *pTag = pEntry->getTag().c_str();
    if (pTag)
        if (isPurgedByTag(pTag, pEntry, pKey))
            return 1;
    return 0;
}


class UrlVary : public Str2Id
{
public:
    UrlVary(const char *pUrl, int len, int id)
        : Str2Id(pUrl, len, id)
    {}

    ~UrlVary()
    {}

    void setVaryId(int32_t id)          {   Str2Id::setId(id);    }
    int32_t getVaryId() const           {   return getId();         }

    const AutoStr2 *getUrl() const     {   return getStr();        }

private:
};


int InternalCacheManager::addUrlVary(const char *pUrl, int len,
                                     int32_t varyId)
{
    HashStringMap<UrlVary *>::iterator iter;
    char *p = (char *)pUrl + len;
    char ch = *p;
    *p = 0;
    iter = m_urlVary.find(pUrl);
    *p = ch;
    if (iter != m_urlVary.end())
    {
        if (iter.second()->getVaryId() != varyId)
            iter.second()->setVaryId(varyId);
    }
    else
    {
        UrlVary *pUrlVary = new UrlVary(pUrl, len, varyId);
        iter = m_urlVary.insert(pUrlVary->getUrl()->c_str(), pUrlVary);
        if (iter == m_urlVary.end())
        {
            delete pUrlVary;
            return -1;
        }
    }
    return 0;
}


int InternalCacheManager::getVaryId(const char *pVary, int varyLen)
{
    HashStringMap<Str2Id *>::iterator iter;
    iter = m_str2IdHash.find(pVary);
    if (iter != m_str2IdHash.end())
        return iter.second()->getId();

    int id = m_pInfo->getNextVaryId();
    Str2Id *pStr2Id = new Str2Id(pVary, varyLen, id);
    iter = m_str2IdHash.insert(pStr2Id->getStr()->c_str(), pStr2Id);
    if (iter == m_str2IdHash.end())
        return -1;
    if (m_id2StrList.size() < id)
        m_id2StrList.resize(id);
    m_id2StrList.push_back(pStr2Id->getStr());
    return id;
}


const AutoStr2 *InternalCacheManager::getUrlVary(const char *pUrl, int len)
{
    HashStringMap<UrlVary *>::iterator iter;
    char *p = (char *)pUrl + len;
    char ch = *p;
    *p = 0;
    iter = m_urlVary.find(pUrl);
    *p = ch;
    if (iter == m_urlVary.end())
        return NULL;
    int id = iter.second()->getVaryId();
    return m_id2StrList[ id ];
}


