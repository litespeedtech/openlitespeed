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
#include <shm/lsshmpool.h>

#include <shm/lsshmhash.h>
#include <shm/lsshmlruhash.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>


LsShmStatus_t LsShmPool::createStaticData(const char *name)
{
    LsShmOffset_t i;
    LsShmPoolMem *pPool = getPool();

    pPool->x_iMagic = LSSHM_POOL_MAGIC;
    strncpy((char *)pPool->x_aName, name, LSSHM_MAXNAMELEN);
    pPool->x_iSize = sizeof(LsShmPoolMem);

    pPool->x_iLockOffset = 0;
    pPool->x_pid = (m_pParent ? getpid() : 0); // only if using global pool

    // Data
    LsShmPoolMap *pDataMap = &pPool->x_data;
    pDataMap->x_chunk.x_iStart = 0;
    pDataMap->x_chunk.x_iEnd = 0;
    pDataMap->x_iFreeList = 0;
    for (i = 0; i < LSSHM_POOL_NUMBUCKET; ++i)
        pDataMap->x_aFreeBucket[i] = 0;
    ::memset(&pDataMap->x_stat, 0, sizeof(LsShmPoolMapStat));

    return LSSHM_OK;
}


LsShmStatus_t LsShmPool::checkStaticData(const char *name)
{
    // check the file
    LsShmPoolMem *pPool = getPool();
    return ((pPool->x_iMagic == LSSHM_POOL_MAGIC)
            && (pPool->x_iSize == sizeof(LsShmPoolMem))
            && (strncmp((char *)pPool->x_aName, name, LSSHM_MAXNAMELEN) == 0)) ?
           LSSHM_OK : LSSHM_BADMAPFILE;
}


LsShmPool::LsShmPool(LsShm *shm, const char *name, LsShmPool *gpool)
    : m_iMagic(LSSHM_POOL_MAGIC)
    , m_pPoolName(NULL)
    , m_iShmOwner(0)
    , m_iRegNum(0)
    , m_pParent(gpool)
    , m_iRef(0)
{
    LsShmOffset_t extraOffset = 0;
    LsShmSize_t extraSize = 0;

    m_status = LSSHM_NOTREADY; // no good

    if ((name == NULL) || (strlen(name) >= LSSHM_MAXNAMELEN))
    {
        m_status = LSSHM_BADPARAM;
        return;
    }

    m_iLockEnable = 1;
    m_pShm = shm;

    if (strcmp(name, LSSHM_SYSPOOL) == 0)
    {
        m_iOffset = shm->xdataOffset();   // the system pool
    }
    else
    {
        if ((m_pPoolName = strdup(name)) == NULL)
        {
            m_status = LSSHM_ERROR;
            return;
        }
        LsShmReg *p_reg;
        if ((p_reg = shm->findReg(name)) == NULL)
        {
            if ((p_reg = shm->addReg(name)) == NULL)
            {
                m_status = LSSHM_BADMAPFILE;
                return;
            }
            if (m_pParent != NULL)
                p_reg->x_iFlag |= LSSHM_REGFLAG_PID;    // mark per process pool

            m_iRegNum = p_reg->x_iRegNum;
            p_reg->x_iValue = 0;    // not inited yet!
        }
        if ((m_iOffset = p_reg->x_iValue) == 0)
        {
            // create memory for new Pool
            // allocate header from SYS POOL
            LsShmOffset_t offset;
            int remapped;
            offset = shm->allocPage(LSSHM_SHM_UNITSIZE, remapped);
            if (offset == 0)
            {
                m_status = LSSHM_BADMAPFILE;
                return;
            }
            ::memset(offset2ptr(offset), 0, sizeof(LsShmPoolMem));
            m_iOffset = offset;
            p_reg->x_iValue = m_iOffset;

            extraOffset = offset + sizeof(LsShmPoolMem);
            extraSize = LSSHM_SHM_UNITSIZE - sizeof(LsShmPoolMem);
        }
    }

    if (getPool()->x_iSize != 0)
    {
        if ((m_status = checkStaticData(name)) != LSSHM_OK)
        {
            LsShm::setErrMsg(LSSHM_BADVERSION,
                "Invalid SHM Pool [%s], magic=%08X(%08X), MapFile [%s].",
                name, getPool()->x_iMagic, LSSHM_POOL_MAGIC, shm->fileName());
        }
    }
    else
    {
        m_status = createStaticData(name);
        // NOTE release extra to freeList
        if ((extraOffset != 0) && (extraSize > LSSHM_POOL_MAXBCKTSIZE))
            releaseData(extraOffset, extraSize);
    }
    mapLock();

    if (m_status == LSSHM_OK)
    {
        m_status = LSSHM_READY;
        m_iRef = 1;
        if (m_pPoolName != NULL)
        {
#ifdef DEBUG_RUN
            SHM_NOTICE("LsShmPool::LsShmPool insert %s <%p>",
                            m_pPoolName, &m_objBase);
#endif
            m_pShm->getObjBase().insert(m_pPoolName, this);
        }
    }
}


LsShmPool::~LsShmPool()
{
    if (m_pParent != NULL)
        destroyShm();
    if (m_pPoolName != NULL)
    {
#ifdef DEBUG_RUN
        SHM_NOTICE("LsShmPool::~LsShmPool remove %s <%p>",
                        m_pPoolName, &m_objBase);
#endif
        m_pShm->getObjBase().remove(m_pPoolName);
        ::free(m_pPoolName);
        m_pPoolName = NULL;
    }
}


LsShmHash *LsShmPool::getNamedHash(const char *name,
                          size_t init_size, hash_fn hf, val_comp vc, int lru_mode)
{
    LsShmHash *pObj;
    GHash::iterator itor;

    if (name == NULL)
        name = LSSHM_SYSHASH;

#ifdef DEBUG_RUN
    SHM_NOTICE("LsShmPool::getNamedHash find %s <%p>",
                    name, &getObjBase());
#endif
    itor = getObjBase().find(name);
    if ((itor != NULL)
        && ((pObj = LsShmHash::checkHTable(itor, this, name, hf,
                                vc)) != (LsShmHash *)-1))
        return pObj;

    if (lru_mode == LSSHM_LRU_MODE2)
        pObj = new LsShmWLruHash(this, name, init_size, hf, vc);
    else if (lru_mode == LSSHM_LRU_MODE3)
        pObj = new LsShmXLruHash(this, name, init_size, hf, vc);
    else
        pObj = new LsShmHash(this, name, init_size, hf, vc, lru_mode);
    if (pObj != NULL)
    {
        if (pObj->getRef() != 0)
            return pObj;
        delete pObj;
    }
    return NULL;
}


void LsShmPool::close()
{
    LsShm *p = NULL;
    if (m_iShmOwner != 0)
    {
        m_iShmOwner = 0;
        p = m_pShm; // delayed remove
    }
    if ((m_pPoolName != NULL) && (downRef() == 0))
        delete this;
    if (p != NULL)
        p->close();
}


void LsShmPool::destroyShm()
{
    int8_t owner;
    if (m_pParent != NULL)
        owner = 0;
    else
    {
        if ((m_pParent = m_pShm->getGlobalPool()) == NULL)
            return;
        owner = 1;
    }
    LsShmSize_t left;
    LsShmPoolMap *pDataMap = getDataMap();
    if ((left = pDataMap->x_chunk.x_iEnd - pDataMap->x_chunk.x_iStart) > 0)
        release2(pDataMap->x_chunk.x_iStart, left);
    mvFreeBucket();
    mvFreeList();
    m_pShm->clrReg(m_iRegNum);
    m_pShm->freeLock(m_pShmLock);
    m_pParent->release2(m_iOffset, sizeof(LsShmPoolMem));
    m_iOffset = m_pParent->m_iOffset;   // let parent/global take over
    m_pShmLock = m_pShm->offset2pLock(getPool()->x_iLockOffset);
    m_iLockEnable = m_pParent->m_iLockEnable;
    if (owner != 0)
        m_pParent->close();
    m_pParent = NULL;
}


LsShmOffset_t LsShmPool::alloc2(LsShmSize_t size, int &remapped)
{
    LsShmMap *map_o = m_pShm->getShmMap();
    LsShmOffset_t offset;

    if (size == 0)
        return 0;
    LSSHM_CHECKSIZE(size);

    remapped = 0;
    size = roundDataSize(size);
    if (size >= LSSHM_SHM_UNITSIZE)
    {
        if ((offset = m_pShm->allocPage(size, remapped)) != 0)
        {
            lock();
            incrCheck(&getDataMap()->x_stat.m_iShmAllocated, roundSize2pages(size));
            unlock();
        }
    }
    else
    {
        lock();
        if (size >= LSSHM_POOL_MAXBCKTSIZE)
            // allocate from FreeList
            offset = allocFromDataFreeList(size);
        else
            // allocate from bucket
            offset = allocFromDataBucket(size);
        if (offset != 0)
            getDataMap()->x_stat.m_iPoolInUse += size;
        unlock();
    }
    if (map_o != m_pShm->getShmMap())
        remapped = 1;

    return offset;
}


void LsShmPool::release2(LsShmOffset_t offset, LsShmSize_t size)
{
    size = roundDataSize(size);
    if (size >= LSSHM_SHM_UNITSIZE)
    {
        m_pShm->releasePage(offset, size);
        lock();
        incrCheck(&getDataMap()->x_stat.m_iShmReleased, roundSize2pages(size));
        unlock();
    }
    else
    {
        lock();
        releaseData(offset, size);
        getDataMap()->x_stat.m_iPoolInUse -= size;
        unlock();
    }
}


void LsShmPool::mvFreeList()
{
    if (m_pParent != NULL)
    {
        lock();
        m_pParent->addFreeList(getDataMap());
        unlock();
    }
    return;
}


void LsShmPool::addFreeList(LsShmPoolMap *pSrcMap)
{
    LsShmOffset_t listOffset;
    if ((listOffset = pSrcMap->x_iFreeList) != 0)
    {
        lock();
        LsShmFreeList *pFree;
        LsShmFreeList *pFreeList;
        LsShmOffset_t freeOffset;
        if ((freeOffset = getDataMap()->x_iFreeList) != 0)
        {
            pFreeList = (LsShmFreeList *)offset2ptr(freeOffset);
            LsShmOffset_t next = listOffset;
            LsShmOffset_t last;
            do
            {
                last = next;
                pFree = (LsShmFreeList *)offset2ptr(last);
                next = pFree->x_iNext;
            }
            while (next != 0);
            pFree->x_iNext = freeOffset;
            pFreeList->x_iPrev = last;
        }
        LsShmSize_t cnt = pSrcMap->x_stat.m_iFlReleased
            - pSrcMap->x_stat.m_iFlAllocated;
        getDataMap()->x_iFreeList = listOffset;
        incrCheck(&getDataMap()->x_stat.m_iFlReleased, cnt);
        getDataMap()->x_stat.m_iFlCnt += pSrcMap->x_stat.m_iFlCnt;
        unlock();
        pSrcMap->x_iFreeList = 0;
        incrCheck(&pSrcMap->x_stat.m_iFlAllocated, cnt);
        pSrcMap->x_stat.m_iFlCnt = 0;
    }
    return;
}


void LsShmPool::mvFreeBucket()
{
    if (m_pParent != NULL)
    {
        lock();
        m_pParent->addFreeBucket(getDataMap());
        unlock();
    }
    return;
}


void LsShmPool::addFreeBucket(LsShmPoolMap *pSrcMap)
{
    int num = 1;     // skip zero slot
    LsShmOffset_t *pSrc = &pSrcMap->x_aFreeBucket[1];
    LsShmOffset_t *pDst = &getDataMap()->x_aFreeBucket[1];
    lock();
    while (num < LSSHM_POOL_NUMBUCKET)
    {
        LsShmOffset_t bcktOffset;
        if ((bcktOffset = *pSrc) != 0)
        {
            LsShmOffset_t freeOffset;
            LsShmOffset_t *pFree;
            if ((freeOffset = *pDst) != 0)
            {
                LsShmOffset_t next = bcktOffset;
                do
                {
                    pFree = (LsShmOffset_t *)offset2ptr(next);
                }
                while ((next = *pFree) != 0);
                *pFree = freeOffset;
            }
            *pDst = bcktOffset;
            *pSrc = 0;
            LsShmSize_t cnt = pSrcMap->x_stat.m_bckt[num].m_iBkReleased
                - pSrcMap->x_stat.m_bckt[num].m_iBkAllocated;
            incrCheck(&pSrcMap->x_stat.m_bckt[num].m_iBkAllocated, cnt);
            incrCheck(&getDataMap()->x_stat.m_bckt[num].m_iBkReleased, cnt);
        }
        ++pSrc;
        ++pDst;
        ++num;
    }
    unlock();
    return;
}


//
// Internal release2
//
void LsShmPool::releaseData(LsShmOffset_t offset, LsShmSize_t size)
{
    LsShmPoolMap *pDataMap = getDataMap();
    if (size >= LSSHM_POOL_MAXBCKTSIZE)
    {
        // release to FreeList
        LsShmFreeList *pFree;
        LsShmFreeList *pFreeList;
        LsShmOffset_t freeOffset;

        // setup FreeList block
        pFree = (LsShmFreeList *)offset2ptr(offset);
        pFree->x_iSize = size;

        if ((freeOffset = pDataMap->x_iFreeList) != 0)
        {
            pFreeList = (LsShmFreeList *)offset2ptr(freeOffset);
            pFree->x_iNext = freeOffset;
            pFree->x_iPrev = 0;
            pFreeList->x_iPrev = offset;
        }
        else
        {
            pFree->x_iNext = 0;
            pFree->x_iPrev = 0;
        }
        pDataMap->x_iFreeList = offset;
        incrCheck(&pDataMap->x_stat.m_iFlReleased, size);
        ++pDataMap->x_stat.m_iFlCnt;
    }
    else
    {
        // release to DataBucket
        LsShmSize_t bucketNum = dataSize2Bucket(size);
        LsShmOffset_t *pBucket;
        LsShmOffset_t *pData;

        pData = (LsShmOffset_t *)offset2ptr(offset);
        pBucket = &pDataMap->x_aFreeBucket[bucketNum];
        *pData = *pBucket;
        *pBucket = offset;
        incrCheck(&pDataMap->x_stat.m_bckt[bucketNum].m_iBkReleased, 1);
    }
}


//
// @brief map the lock into Object space
//
void LsShmPool::mapLock()
{
    LsShmPoolMem *pPool = getPool();
    if (pPool->x_iLockOffset == 0)
    {
        if ((m_pShmLock = m_pShm->allocLock()) == NULL)
            m_status = LSSHM_ERROR;
        else
        {
            pPool->x_iLockOffset = m_pShm->pLock2offset(m_pShmLock);
            //FIX FOR BELOW BUG
            setupLock();
        }

    }
    else
        m_pShmLock = m_pShm->offset2pLock(pPool->x_iLockOffset);
    //BUG: reset lock for existing lock, could be locked by another process.
    //setupLock();
}


//
//  @brief allocFromDataFreeList - only for size >= LSSHM_POOL_MAXBCKTSIZE
//  @brief linear search the FreeList for it.
//  @brief if no match from FreeList then allocate from data.x_chunk
//
LsShmOffset_t LsShmPool::allocFromDataFreeList(LsShmSize_t size)
{
    LsShmOffset_t offset;
    LsShmFreeList *pFree;
    int left;
    offset = getDataMap()->x_iFreeList;
    while (offset != 0)
    {
        pFree = (LsShmFreeList *)offset2ptr(offset);
        if ((left = (int)(pFree->x_iSize - size)) >= 0)
        {
            pFree->x_iSize = (LsShmSize_t)left;
            incrCheck(&getDataMap()->x_stat.m_iFlAllocated, size);
            if ((LsShmSize_t)left < LSSHM_POOL_MAXBCKTSIZE)
                mvDataFreeListToBucket(pFree, offset);
            return offset + left;
        }
        offset = pFree->x_iNext;
    }
    LsShmSize_t num = 1;
    if ((offset = allocFromDataChunk(size, num)) != 0)
    {
        incrCheck(&getDataMap()->x_stat.m_iFlReleased, size);
        incrCheck(&getDataMap()->x_stat.m_iFlAllocated, size);
    }
    return offset;
}


LsShmOffset_t LsShmPool::allocFromDataBucket(LsShmSize_t size)
{
    LsShmOffset_t offset;
    LsShmOffset_t *np, *pBucket;

    LsShmOffset_t num = dataSize2Bucket(size);
    pBucket = &getDataMap()->x_aFreeBucket[num];
    if ((offset = *pBucket) != 0)
    {
        np = (LsShmOffset_t *)offset2ptr(offset);
        *pBucket = *np;
    }
    else if ((offset = fillDataBucket(num, size)) == 0)
    {
        return 0;
    }
    incrCheck(&getDataMap()->x_stat.m_bckt[num].m_iBkAllocated, 1);
    return offset;
}


LsShmOffset_t LsShmPool::allocFromGlobalBucket(
    LsShmOffset_t bucketNum, LsShmSize_t &num)
{
    LsShmOffset_t first;
    LsShmOffset_t next;
    LsShmOffset_t *np;

    if (getDataMap()->x_aFreeBucket[bucketNum] == 0)
        return 0;

    LsShmSize_t cnt = 0;
    lock();
    np = &getDataMap()->x_aFreeBucket[bucketNum];
    next = first = *np;
    while (next != 0)
    {
        np = (LsShmOffset_t *)offset2ptr(next);
        next = *np;
        if (++cnt >= num)
            break;
    }
    getDataMap()->x_aFreeBucket[bucketNum] = next;
    *np = 0;
    incrCheck(&getDataMap()->x_stat.m_bckt[bucketNum].m_iBkAllocated, cnt);
    unlock();
    num = cnt;
    return first;
}


//
// @brief fillDataBucket allocates memory from a Global pool else Chunk storage.
// @brief This may cause a remap; only use offset...
// @brief bucketNum and size should be logically connected.
// @brief passing both to avoid an extra calulation.
// @brief freeBucket[bucketNum] will be set to the new allocated pool.
// @brief return the offset of the newly allocated memory.
//
LsShmOffset_t LsShmPool::fillDataBucket(
    LsShmOffset_t bucketNum, LsShmSize_t size)
{
    LsShmSize_t num;
    // allocated according to data size
    num = (LSSHM_POOL_MAXBCKTSIZE << 2) / size;
    if (num > LSSHM_MAX_BUCKET_SLOT)
        num = LSSHM_MAX_BUCKET_SLOT;

    LsShmOffset_t xoffset, offset;
    if ((m_pParent != NULL)
        && ((offset = m_pParent->allocFromGlobalBucket(bucketNum, num)) != 0))
    {
        if (num > 1)
            getDataMap()->x_aFreeBucket[bucketNum] = offset + size;
        incrCheck(&getDataMap()->x_stat.m_bckt[bucketNum].m_iBkReleased, num);
        return offset;
    }
    xoffset = offset = allocFromDataChunk(size, num);   // might remap
    if (num == 0)
        return offset;  // big problem, no more memory
    incrCheck(&getDataMap()->x_stat.m_bckt[bucketNum].m_iBkReleased, num);

    // take the first one - save the rest
    xoffset += size;
    if (--num != 0)
        getDataMap()->x_aFreeBucket[bucketNum] = xoffset;

    uint8_t *xp;
    xp = (uint8_t *)offset2ptr(offset);
    while (num-- != 0)
    {
        xp += size;
        xoffset += size;
        *((LsShmOffset_t *)xp) = num ? xoffset : 0;
    }
    return offset;
}


//
// @brief allocFromDataChunk
// @brief num suggested the number of buckets to allocate.
//
// @brief could reduce the number if it is too many,
//  currently it set max to 0x20 regardless.
// @brief fill the bucket will all available space.
// @brief return offset and num of buckets.
// @brief could cause remap.
// @brief always check return num and offset.
//
LsShmOffset_t LsShmPool::allocFromDataChunk(LsShmSize_t size,
        LsShmSize_t  &num)
{
    LsShmOffset_t offset;
    LsShmSize_t numAvail;
    LsShmSize_t avail;
    LsShmPoolMap *pDataMap = getDataMap();

    avail = pDataMap->x_chunk.x_iEnd - pDataMap->x_chunk.x_iStart;
    numAvail = avail / size;
    if (numAvail)
    {
        if (numAvail < num) // shrink the num
            num = numAvail;
        size *= num;
        offset = pDataMap->x_chunk.x_iStart;
        pDataMap->x_chunk.x_iStart += size;
        pDataMap->x_stat.m_iFreeChunk -= size;
        return offset;
    }

    LsShmOffset_t releaseOffset;
    LsShmSize_t releaseSize;
    if (avail != 0)
    {
        offset = pDataMap->x_chunk.x_iStart;
        pDataMap->x_chunk.x_iStart += avail;
        // release all Chunk memory
        // releaseData(offset, avail);
        releaseOffset = offset;
        releaseSize = avail;
    }
    else
    {
        releaseOffset = 0;
        releaseSize = 0;
    }

    // figure pagesize needed - round to SHM page unit to avoid waste
    LsShmSize_t needed = roundPageSize(size * num);
    if (needed < LSSHM_SHM_UNITSIZE)
        needed = LSSHM_SHM_UNITSIZE;

    int remapped = 0;
    if ((offset = m_pShm->allocPage(needed, remapped)) == 0)
    {
        num = 0;
        return 0;
    }
    if (remapped != 0)
        pDataMap = getDataMap();
    pDataMap->x_stat.m_iFreeChunk += needed;
    
    if (releaseOffset != 0)
    {
        // merging leftover and newly allocated memory
        if (releaseOffset + releaseSize == offset)
        {
            offset = releaseOffset;
            needed += releaseSize;
        }
        else
        {
            releaseData(releaseOffset, releaseSize);
            pDataMap->x_stat.m_iFreeChunk -= releaseSize;
        }
    }
    pDataMap->x_chunk.x_iStart = offset;
    pDataMap->x_chunk.x_iEnd = offset + needed;
    return allocFromDataChunk(size, num);
}


void LsShmPool::mvDataFreeListToBucket(
    LsShmFreeList *pFree, LsShmOffset_t offset)
{
    rmFromDataFreeList(pFree);
    if (pFree->x_iSize > 0)
    {
        incrCheck(&getDataMap()->x_stat.m_iFlAllocated, pFree->x_iSize);
        LsShmOffset_t bucketNum = dataSize2Bucket(pFree->x_iSize);
        LsShmOffset_t *np;
        np = (LsShmOffset_t *)pFree; // cast to offset
        *np = getDataMap()->x_aFreeBucket[bucketNum];
        getDataMap()->x_aFreeBucket[bucketNum] = offset;
        incrCheck(&getDataMap()->x_stat.m_bckt[bucketNum].m_iBkReleased, 1);
    }
}


void LsShmPool::rmFromDataFreeList(LsShmFreeList *pFree)
{
    LsShmFreeList *xp;
    if (pFree->x_iPrev == 0)
        getDataMap()->x_iFreeList = pFree->x_iNext;
    else
    {
        xp = (LsShmFreeList *) offset2ptr(pFree->x_iPrev);
        xp->x_iNext = pFree->x_iNext;
    }
    if (pFree->x_iNext != 0)
    {
        xp = (LsShmFreeList *)offset2ptr(pFree->x_iNext);
        xp->x_iPrev = pFree->x_iPrev;
    }
    --getDataMap()->x_stat.m_iFlCnt;
}

