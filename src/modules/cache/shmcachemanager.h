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
#ifndef SHMCACHEMANAGER_H
#define SHMCACHEMANAGER_H

#include <shm/lsshmtypes.h>
#include "cachemanager.h"

class LsShmHash;
class LsShmPool;
struct CacheKey;
class CacheEntry;
class ShmPrivatePurgeData;

template<class T> class TShmHash;


class ShmCacheManager : public CacheManager
{
public:
    ShmCacheManager()
        : m_pPublicPurge(NULL)
        , m_pSessions(NULL)
        , m_pPubTracker(NULL)
        , m_pPrivTracker(NULL)
        , m_pStr2IdHash(NULL)
        , m_pUrlVary(NULL)
        , m_pId2VaryStr(NULL)
        , m_CacheInfoOff(0)
        , m_attempts(0)
    {}

    ~ShmCacheManager();

    int init(const char *pStoreDir);

    int isPurged(CacheEntry *pEntry, CacheKey *pKey, bool isCheckPrivate);
    int processPurgeCmd(const char *pValue, int iValLen, time_t curTime,
                        int curTimeMS, int stale)
    {
        return processPurgeCmdEx(NULL, pValue, iValLen, curTime, curTimeMS, stale);
    }
    int processPrivatePurgeCmd(CacheKey *pKey, const char *pValue, int iValLen,
                               time_t curTime, int curTimeMS, int stale);

    
    
    void *getPrivateCacheInfo(const char *pPrivateId, int len, int force);

    int setPrivateTagFlag(void *pPrivatePurgeData, purgeinfo_t *pInfo);
    purgeinfo_t *getPrivateTagInfo(void *pPrivatePurgeData, int tagId);
    int isFetchAll(void *pPrivatePurgeData);

    int setVerifyKey(void *pPrivatePurgeData, const char *pVerifyKey, int len);
    const char *getVerifyKey(void *pPrivatePurgeData, int *len);

    virtual int addUrlVary(const char *pUrl, int len, int32_t id);
    
    virtual int delUrlVary(const char *pUrl, int len);
    
    virtual int32_t getUrlVaryId(const char *pUrl, int len);

    virtual int getVaryId(const char *pVary, int varyLen);

    virtual const AutoStr2 *getVaryStrById(uint id);

    virtual const AutoStr2 *getUrlVary(const char *pUrl, int len);


    CacheInfo *getCacheInfo();

    int findTagId(const char *pTag, int len);
    int getTagId(const char *pTag, int len);
    void incStats(int isPrivate, int offset);
    virtual int getPrivateSessionCount() const;

    virtual int houseKeeping();

    virtual int shouldCleanDiskCache();
    
    
    int  addTracking(CacheEntry * pEntry);
    int  removeTracking(const char *pKey, int keyLen, int isPrivate);
    int  trimExpiredByTracking(int isPrivate, int maxCnt, int (*removeEntry)(void *, void *), void *param);
    int  isInTracker(const unsigned char* getKey, int keyLen, int isPrivate);


private:
    LsShmHash               *m_pPublicPurge;
    LsShmHash               *m_pSessions;
    LsShmHash               *m_pPubTracker;
    LsShmHash               *m_pPrivTracker;
    LsShmHash               *m_pStr2IdHash;
    TShmHash<int32_t>       *m_pUrlVary;
    LsShmHash               *m_pId2VaryStr;
    TPointerList<AutoStr2>   m_id2StrList;
    LsShmOffset_t            m_CacheInfoOff;
    int                      m_attempts;


    LsShmOffset_t getSession(const char *pId, int len);
    
    LsShmOffset_t addUpdate(const char *pKey, int keyLen, int flag,
                            int32_t sec, int16_t msec);
    LsShmOffset_t addUpdate(LsShmHash *pHash, const char *pKey, int keyLen,
                            purgeinfo_t *pData);
    
    
    int shouldPurge(const char *pKey, int keyLen, int32_t sec, int16_t msec);

    int findSession(CacheKey *pKey, ShmPrivatePurgeData *pData);
    int isPurgedByTag(const char *pTag, CacheEntry *pEntry, CacheKey *pKey,
                      bool isCheckPrivate);
    int processPurgeCmdEx(ShmPrivatePurgeData *pPrivate, const char *pValue,
                          int iValLen, time_t curTime, int curTimeMS, int stale);

    int           getNextVaryId();
    int           getNextPrivateTagId();
    const AutoStr2 *addId2StrList(int id, const char *pVary, int varyLen);
    void logShmError();
    int  initCacheInfo(LsShmPool *pPool);
    int  initTables(LsShmPool *pPool);
    
    
    void cleanupExpiredSessions();
    int  cleanDiskCache();
    
    int  addTracking2(CacheEntry * pEntry, LsShmHash *pTracker);
    LsShmHash *getTracker(int isPrivate)
    {
        return isPrivate ? m_pPrivTracker : m_pPubTracker;
    }
    

};

#endif // SHMCACHEMANAGER_H
