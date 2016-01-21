#ifndef INTERNALCACHEMANAGER_H
#define INTERNALCACHEMANAGER_H

#include <util/hashstringmap.h>
#include <util/gpointerlist.h>
#include <util/autostr.h>
#include <util/pcregex.h>
#include "cachemanager.h"

class PurgeData2;
struct CacheKey;
class CacheEntry;
class PurgeData;
class PrivatePurgeData;
class CacheInfo;
class Str2Id;
class UrlVary;



class InternalCacheManager : public CacheManager
{
public:
    InternalCacheManager();
    ~InternalCacheManager();

    int init(const char *pStoreDir);

    void *getPrivateCacheInfo(const char *pPrivateId, int len, int force);

    int setPrivateTagFlag(void *pPrivatePurgeData, int tagId, uint8_t flag);
    purgeinfo_t *getPrivateTagInfo(void *pPrivatePurgeData, int tagId);

    int setVerifyKey(void *pPrivatePurgeData, const char *pVerifyKey, int len);
    const char *getVerifyKey(void *pPrivatePurgeData, int *len);


    int isPurged(CacheEntry *pEntry, CacheKey *pKey);
    int processPurgeCmd(const char *pValue, int iValLen, time_t curTime,
                        int curTimeMS)
    {
        return processPurgeCmdEx(NULL, pValue, iValLen, curTime, curTimeMS);
    }
    int processPrivatePurgeCmd(CacheKey *pKey, const char *pValue, int iValLen,
                               time_t curTime, int curTimeMS);

    int addUrlVary(const char *pUrl, int len, int id);

    int getVaryId(const char *pVary, int varyLen);

    const AutoStr2 *getVaryStrById(uint id)
    {
        if (id >= (uint)m_id2StrList.size())
            return NULL;
        return m_id2StrList[id];
    }

    const AutoStr2 *getUrlVary(const char *pUrl, int len);

    int findTagId(const char *pTag, int len);

    int getTagId(const char *pTag, int len);

    virtual CacheInfo *getCacheInfo()
    {   return m_pInfo;     }

    virtual int getPrivateSessionCount() const
    {   return m_privateList.size();    }

    virtual int houseKeeping() {    return 0;   };


private:
    HashStringMap<PrivatePurgeData *>   m_privateList;
    HashStringMap<PurgeData2 *>         m_hashTags;
    HashStringMap<Str2Id *>             m_str2IdHash;
    HashStringMap<UrlVary *>            m_urlVary;
    TPointerList<AutoStr2>              m_id2StrList;
    CacheInfo                          *m_pInfo;

    int processPurgeCmdEx(PrivatePurgeData *pPrivate, const char *pValue,
                          int iValLen, time_t curTime, int curTimeMS);
    PrivatePurgeData *getPrivate(const char *pId, int len);
    PrivatePurgeData *findPrivate(CacheKey *pKey);
    PurgeData *addUpdate(const char *pKey, int keyLen, int flag, int32_t sec,
                         int16_t msec);
    int isPurgedByTag(const char *pTag, CacheEntry *pEntry, CacheKey *pKey);
    int shouldPurge(const char *pKey, int keyLen, int32_t sec, int16_t msec);


};


#endif // INTERNALCACHEMANAGER_H
