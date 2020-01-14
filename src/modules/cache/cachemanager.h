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
#ifndef CACHEPURGE_H
#define CACHEPURGE_H

#include <util/hashstringmap.h>
#include <util/gpointerlist.h>
#include <util/autostr.h>
#include <util/pcregex.h>
#include <inttypes.h>

class AutoBuf;
class PurgeData2;
struct CacheKey;
class CacheEntry;
class PurgeData;
class PrivatePurgeData;
class CacheInfo;
class Str2Id;
class UrlVary;

#define PDF_MATCH   0
#define PDF_PREFIX  1
#define PDF_POSTFIX 2
#define PDF_REGEX   3
#define PDF_STALE   4
#define PDF_PURGE   8
#define PDF_SHARED  16
#define PDF_TAG     32
#define PDF_BLANK   64

#define CM_ID_FORMKEY       0
#define CM_ID_MESSAGES      9

#define CIF_VARY_HASH       4
#define CIF_PUB_TRACK_HASH  8
#define CIF_PRIV_TRACK_HASH 16
#define CIF_STALE_PURGE     32

#define CM_TRACK_LITEMAGE 1
#define CM_TRACK_PRIVATE  2

typedef struct shm_objtrack_s
{
    uint32_t    x_tmCreated;
    uint32_t    x_tmExpire;
    uint8_t     x_flag;
    uint8_t     x_hits;
    uint8_t     x_reserve[2];
}shm_objtrack_t;


typedef struct purgeinfo_s
{
    int32_t     tmSecs;
    int16_t     tmMsec;
    uint8_t     flags;
    uint8_t     idTag;
} purgeinfo_t;

typedef struct cachestats_s
{
    int32_t     created;
    int32_t     hits;
    int32_t     purged;
    int32_t     expired;
    int32_t     misses;

    int32_t     collisions;

} cachestats_t;


class CacheInfo
{
public:
    CacheInfo()
    {}
    ~CacheInfo()
    {}

    void clearStats()
    {
        memset(m_stats, 0, (char *)&m_tmLastHouseKeeping - (char*)m_stats);
    }

    int32_t getNextPrivateTagId()
    {   return ls_atomic_fetch_add(&m_iCurPrivateTagId, 1) + 1;    }

    int32_t getNextVaryId()
    {   return ls_atomic_fetch_add(&m_iCurVaryId, 1) + 1;          }

    void setPurgeTime(time_t curTime, int curTimeMs)
    {
        m_tmPurgeSecs = curTime;
        m_tmPurgeMsecs = curTimeMs;
    }

    int  shouldPurge(int32_t sec, int16_t msec)
    {
        return ((m_tmPurgeSecs > sec) ||
                ((m_tmPurgeSecs == sec) && (m_tmPurgeMsecs >= msec)));
    }

    cachestats_t *getPublicStats()
    {   return &m_stats[0];       }
    cachestats_t *getPrivateStats()
    {   return &m_stats[1];      }

    cachestats_t *getStats(int isPrivate)
    {   return &m_stats[isPrivate != 0];  }

    int32_t     getFullPageHits() const     {   return m_iPageHits[1];  }
    int32_t     getPartialPageHits() const  {   return m_iPageHits[0];  }

    void incFullPageHits(int isFull)
    {   ls_atomic_add(&m_iPageHits[isFull != 0], 1);     }

    void incSessionPurged(int count)
    {   ls_atomic_add(&m_iSessionPurged, count);     }
    int32_t getSessionPurged() const    {   return m_iSessionPurged;        }
    int32_t getLastHouseKeeping() const {   return m_tmLastHouseKeeping;    }
    char setLastHouseKeeping(int32_t tmOld, int32_t tmNow)
    {   return ls_atomic_cas32(&m_tmLastHouseKeeping, tmOld, tmNow);       }

    int32_t getLastCleanDiskCache() const   {   return m_tmLastCleanDiskCache;  }
    int32_t getLastCleanSessPurge() const   {   return m_iLastCleanSessPurge;   }

    char setLastCleanDiskCache(int32_t tmOld, int32_t tmNow)
    {
        char succ = ls_atomic_cas32(&m_tmLastCleanDiskCache, tmOld, tmNow);
        if (succ)
            m_iLastCleanSessPurge = m_iSessionPurged;
        return succ;
    }

    int32_t getNewPurgeCount() const
    {   return m_iSessionPurged - m_iLastCleanSessPurge;    }

    uint32_t getFlags() const       {   return m_iFlags;        }
    void     setFlags(uint32_t f)   {   m_iFlags = f;        }
    
    void updateFlag(uint32_t f, uint32_t val)
    {
        uint32_t old;
        uint32_t new_val;
        do
        {
            old = m_iFlags;
            if (val)
                new_val = old | f;
            else
                new_val = old & ~f;
        }
        while(!ls_atomic_cas32(&m_iFlags, old, new_val));
    }

private:
    int32_t     m_tmPurgeSecs;
    int32_t     m_tmPurgeMsecs;
    int32_t     m_iCurVaryId;
    int32_t     m_iCurPrivateTagId;

    cachestats_t    m_stats[2];
    int32_t         m_iPageHits[2];
    int32_t         m_iSessionPurged;
    uint32_t        m_tmLastHouseKeeping;
    uint32_t        m_tmLastCleanDiskCache;
    uint32_t        m_iLastCleanSessPurge;
    uint32_t        m_iFlags;
    char            m_reserved[252] __attribute__ ((unused)); /* Padding, do not remove */
};


class CacheManager
{
public:
    CacheManager()
    {}
    virtual ~CacheManager()
    {}

    virtual int init(const char *pStoreDir) = 0;

    virtual void *getPrivateCacheInfo(const char *pPrivateId, int len,
                                      int force) = 0;

    virtual int setPrivateTagFlag(void *pPrivatePurgeData,
                                  purgeinfo_t *pInfo) = 0;
    virtual purgeinfo_t *getPrivateTagInfo(void *pPrivatePurgeData,
                                           int tagId) = 0;

    virtual int isFetchAll(void *pPrivatePurgeData)     {   return 0;  }

    virtual int setVerifyKey(void *pPrivatePurgeData, const char *pVerifyKey,
                             int len) = 0;
    virtual const char *getVerifyKey(void *pPrivatePurgeData, int *len) = 0;

    virtual int isPurged(CacheEntry *pEntry, CacheKey *pKey,
                         bool isCheckPrivate) = 0;
    virtual int processPurgeCmd(const char *pValue, int iValLen,
                                time_t curTime, int curTimeMS, int stale) = 0;
    virtual int processPrivatePurgeCmd(
        CacheKey *pKey, const char *pValue, int iValLen,
        time_t curTime, int curTimeMS, int stale) = 0;

    virtual int addUrlVary(const char *pUrl, int len, int32_t id) = 0;

    virtual int delUrlVary(const char *pUrl, int len) = 0;

    virtual int32_t getUrlVaryId(const char *pUrl, int len) = 0;

    virtual int getVaryId(const char *pVary, int varyLen) = 0;

    virtual const AutoStr2 *getVaryStrById(uint id) = 0;

    virtual const AutoStr2 *getUrlVary(const char *pUrl, int len) = 0;

    virtual int findTagId(const char *pTag, int len) = 0;
    virtual int getTagId(const char *pTag, int len) = 0;

    void incStats(int isPrivate, int offset)
    {
        cachestats_t *pInfo = getCacheInfo()->getStats(isPrivate);
        int32_t *pCounter = (int32_t *)((char *)pInfo + offset);
        ls_atomic_add(pCounter, 1);
    }

    void generateRpt(const char *name, AutoBuf *pBuf);
    virtual int getPrivateSessionCount() const = 0;

    void    incFullPageHits(int isFull)
    {   getCacheInfo()->incFullPageHits(isFull);     }

    virtual int houseKeeping() = 0;
    virtual int shouldCleanDiskCache() = 0;
      
    
    void updateStatsExpireByTracking(shm_objtrack_t* pData);
    
    
    virtual int  addTracking(CacheEntry * pEntry) = 0;
    virtual int  removeTracking(const char *pKey, int keyLen, int isPrivate) = 0;
    virtual int  trimExpiredByTracking(int isPrivate, int maxCnt, 
                                       int (*removeEntry)(void *, void *), 
                                       void *param) = 0;
    virtual int  isInTracker(const unsigned char* getKey, int keyLen, 
                             int isPrivate) = 0;

private:
    virtual CacheInfo *getCacheInfo() = 0;

protected:
    void populatePrivateTag();

private:
    TPointerList<AutoStr2>              m_id2StrList;


};


#endif // CACHEPURGE_H
