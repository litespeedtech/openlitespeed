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
#ifndef LSSHMPOOL_H
#define LSSHMPOOL_H
#include <shm/lsshm.h>

#ifdef LSSHM_DEBUG_ENABLE
class debugBase;
#endif

/**
 * @file
 * replicated the gpool idea.
 *  lsShmPool
 *  (0) Page memory is handled by LsShm.
 *  (1) Maintain fixed size bucket for fast allocation (4-256, 8 byte interval).
 *  (2) Anything bigger than max bucket will get from the freelist allocation.
 *
 *  LsShmPoolMem -> the physical map structure
 *
 *  all map members start with x_
 */


// runtime free memory for the pool
typedef struct
{
    LsShmOffset_t  x_iStart;
    LsShmOffset_t  x_iEnd;
} ShmMapChunk;

typedef struct
{
    LsShmOffset_t  x_iNext;
    LsShmOffset_t  x_iPrev;
    LsShmSize_t    x_iSize;
} LsShmFreeList;

#define LSSHM_POOL_NUMBUCKET    0x20
#define LSSHM_MAX_BUCKET_SLOT   0x8 // max bucket slot allocation

#if 0
// useful for test purpose
#define LSSHM_POOL_NUMBUCKET    0x4
#define LSSHM_MAX_BUCKET_SLOT   0x4 // max bucket slot allocation
#endif

typedef struct
{
    LsShmSize_t     m_iShmAllocated;    // total direct shm allocated (bytes)
    LsShmSize_t     m_iShmReleased;     // total direct shm released (bytes)
    LsShmSize_t     m_iFromChunk;       // allocated for pool (bytes)
    LsShmSize_t     m_iFlAllocated;     // allocated from free list (bytes)
    LsShmSize_t     m_iFlReleased;      // released to free list (bytes)
    LsShmSize_t     m_iFlCnt;           // entries on free list
    struct
    {
        LsShmSize_t m_iBkAllocated;     // total allocations from bucket (count)
        LsShmSize_t m_iBkReleased;      // total releases to bucket (count)
    } m_bckt[LSSHM_POOL_NUMBUCKET];
} LsShmPoolMapStat;

typedef struct
{
    LsShmSize_t     x_iUnitSize;
    LsShmSize_t     x_iMaxUnitSize;
    LsShmSize_t     x_iNumFreeBucket;
    ShmMapChunk     x_chunk;             // unused after alloc2
    LsShmOffset_t   x_iFreeList;         // the big free list
    LsShmOffset_t   x_aFreeBucket[LSSHM_POOL_NUMBUCKET];
    LsShmPoolMapStat    x_stat;          // map statistics
} LsShmPoolMap;

typedef struct
{
    uint32_t        x_iMagic;
    uint8_t         x_aName[LSSHM_MAXNAMELEN];
    uint32_t        x_iSize;
    LsShmPoolMap    x_data;
    LsShmOffset_t   x_iLockOffset;
    pid_t           x_pid;
} LsShmPoolMem;

class LsShmPool : public ls_shmpool_s, ls_shmobject_s
{
public:
    explicit LsShmPool(LsShm *shm, const char *name, LsShmPool *gpool);
    ~LsShmPool();

public:
    typedef uint32_t     LsShmHKey;
    typedef LsShmHKey(*hash_fn)(const void *pVal, int len);
    typedef int (*val_comp)(const void *pVal1, const void *pVal2, int len);
    LsShmHash *getNamedHash(const char *name, size_t init_size,
                            hash_fn hf, val_comp vc, int lru_mode);
    void close();
    void destroyShm();

    uint32_t getMagic() const   {   return m_iMagic;    }
    LsShm *getShm() const       {   return m_pShm;      }

    const char *name() const
    {   return (const char *)getPool()->x_aName; };

    LsShmStatus_t status() const
    {   return m_status; };

    ls_attr_inline void *offset2ptr(LsShmOffset_t offset) const  
    {   return (void *)m_pShm->offset2ptr(offset); }

    ls_attr_inline LsShmOffset_t ptr2offset(const void *ptr) const
    {   return m_pShm->ptr2offset(ptr); }

    LsShmOffset_t  alloc2(LsShmSize_t size, int &remapped);
    void  release2(LsShmOffset_t offset, LsShmSize_t size);
    void  mvFreeList();
    void  addFreeList(LsShmPoolMap *pSrcMap);
    void  mvFreeBucket();
    void  addFreeBucket(LsShmPoolMap *pSrcMap);

    void enableLock()
    {   m_iLockEnable = 1; }

    void disableLock()
    {   m_iLockEnable = 0; }

#ifdef notdef
    LsShmReg *getReg(int num)
    {   return m_pShm->getReg(num); }
#endif
    LsShmReg *findReg(const char *name)
    {   return m_pShm->findReg(name); }
    LsShmReg *addReg(const char *name)
    {   return m_pShm->addReg(name); }

    ls_attr_inline LsShmMap *getShmMap() const
    {   return m_pShm->getShmMap(); }

    ls_attr_inline LsShmSize_t getShmMapMaxSize() const
    {   return m_pShm->maxSize(); }

    ls_attr_inline LsShmSize_t getShmMapOldMaxSize() const
    {   return m_pShm->oldMaxSize(); }

    LsShmOffset_t getPoolMapStatOffset() const
    { return (LsShmOffset_t)(long)&((LsShmPoolMem *)m_iOffset)->x_data.x_stat; }

    LsShmLock *lockPool()
    {   return m_pShm->lockPool(); }

#ifdef notdef
    void setMapOwner(int o)
    {   m_iShmOwner = o; }
#endif

    HashStringMap< lsi_shmobject_t *> &getObjBase()
    {   return m_pShm->getObjBase(); }

    int lock()
    {   return m_iLockEnable && lsi_shmlock_lock(m_pShmLock); }

    int trylock()
    {   return m_iLockEnable && lsi_shmlock_trylock(m_pShmLock); }

    int unlock()
    {   return m_iLockEnable && lsi_shmlock_unlock(m_pShmLock); }

    int getRef()  { return m_iRef; }
    int upRef()   { return ++m_iRef; }
    int downRef() { return --m_iRef; }

private:
    ls_attr_inline LsShmPoolMem *getPool() const
    {   return (LsShmPoolMem *)m_pShm->offset2ptr(m_iOffset);   }

    ls_attr_inline LsShmPoolMap *getDataMap() const
    {   return (LsShmPoolMap *)&(getPool()->x_data);   }

    int setupLock()
    {   return m_iLockEnable && lsi_shmlock_setup(m_pShmLock); }

    void syncData();
    void mapLock();

    // for internal purpose
    void  releaseData(LsShmOffset_t offset, LsShmSize_t size);

    LsShmStatus_t checkStaticData(const char *name);
    LsShmStatus_t createStaticData(const char *name);

    //
    // data related
    //
    LsShmOffset_t allocFromDataFreeList(LsShmSize_t size);
    LsShmOffset_t allocFromDataBucket(LsShmSize_t size);
    LsShmOffset_t allocFromGlobalBucket(
        LsShmOffset_t bucketNum, LsShmSize_t &num);
    LsShmOffset_t fillDataBucket(LsShmOffset_t bucketNum, LsShmOffset_t size);
    LsShmOffset_t allocFromDataChunk(LsShmSize_t size, LsShmSize_t &num);
    void mvDataFreeListToBucket(LsShmFreeList *pFree, LsShmOffset_t offset);
    void rmFromDataFreeList(LsShmFreeList *pFree);

    // NOTE m_pageUnitSize and m_dataUnitSize are set only at start up
#ifdef notdef
    LsShmOffset_t pageSize2Bucket(LsShmSize_t size) const
    {
        return ((size + m_pageUnitSize - 1)
                / m_pageUnitSize)
               * m_pageUnitSize;
    }
#endif

    LsShmSize_t roundPageSize(LsShmSize_t size) const
    {
        return ((size - 1 + m_iPageUnitSize)
                / m_iPageUnitSize) * m_iPageUnitSize;
    };

    LsShmOffset_t dataSize2Bucket(LsShmSize_t size) const
    {   return (size / m_iDataUnitSize); }

    LsShmSize_t roundDataSize(LsShmSize_t size) const
    {
        return ((size - 1 + m_iDataUnitSize)
                / m_iDataUnitSize) * m_iDataUnitSize;
    };

    void incrCheck(LsShmSize_t *ptr, LsShmSize_t size)
    {
        LsShmSize_t prev = *ptr;
        *ptr += size;
        if (*ptr < prev)    // cnt wrapped
            *ptr = (LsShmSize_t)-1;
        return;
    }

private:
    LsShmPool(const LsShmPool &other);
    LsShmPool &operator=(const LsShmPool &other);
    bool operator==(const LsShmPool &other);

    uint32_t            m_iMagic;
    char               *m_pPoolName;    // Name
    LsShm              *m_pShm;         // SHM handle
    LsShmStatus_t       m_status;       // Ready ...
    LsShmOffset_t       m_iOffset;      // find from SHM registry
    lsi_shmlock_t      *m_pShmLock;
    int8_t              m_iLockEnable;
    int8_t              m_iShmOwner;    // indicated if I own the SHM
    uint16_t            m_iRegNum;      // registry number
    LsShmSize_t         m_iPageUnitSize;
    LsShmSize_t         m_iDataUnitSize;
    LsShmPool          *m_pParent;      // parent global pool
    int                 m_iRef;
#ifdef LSSHM_DEBUG_ENABLE
    friend class debugBase;
#endif
};

#endif // LSSHMPOOL_H
