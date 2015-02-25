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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <shm/lsshmpool.h>
#include <shm/lsshmhash.h>
#include <shm/lsshmlruhash.h>
#include <shm/lsshm.h>
#include <http/httplog.h>
#if 0
#include <log4cxx/logger.h>
#endif

#define MIN_POOL_DATAUNITSIZE   8

#ifdef DEBUG_RUN
using namespace LOG4CXX_NS;
#endif


LsShmStatus_t LsShmPool::createStaticData(const char *name)
{
    LsShmOffset_t i;

    m_pPool->x_iMagic = LSSHM_POOL_MAGIC;
    m_pPool->x_iSize = sizeof(LsShmPoolMem);
    strncpy((char *)m_pPool->x_aName, name, LSSHM_MAXNAMELEN);

    m_pPool->x_iLockOffset = 0;
    m_pPool->x_pid = (m_pParent ? getpid() : 0); // only if using global pool

    // Page
    m_pPageMap->x_chunk.x_iStart = 0;
    m_pPageMap->x_chunk.x_iEnd = 0;
    m_pPageMap->x_iFreeList = 0;
    m_pPageMap->x_iUnitSize = m_pShm->getShmMap()->x_iUnitSize;
    m_pPageMap->x_iMaxUnitSize = m_pPageMap->x_iUnitSize *
                                 LSSHM_POOL_NUMBUCKET;
    m_pPageMap->x_iNumFreeBucket = LSSHM_POOL_NUMBUCKET;
    for (i = 0; i < m_pPageMap->x_iNumFreeBucket; ++i)
        m_pPageMap->x_aFreeBucket[i] = 0;

    // Data
    m_pDataMap->x_chunk.x_iStart = 0;
    m_pDataMap->x_chunk.x_iEnd = 0;
    m_pDataMap->x_iFreeList = 0;
    m_pDataMap->x_iUnitSize = MIN_POOL_DATAUNITSIZE;      // 8 bytes
    m_pDataMap->x_iMaxUnitSize = m_pDataMap->x_iUnitSize *
                                 LSSHM_POOL_NUMBUCKET;
    m_pDataMap->x_iNumFreeBucket = LSSHM_POOL_NUMBUCKET;
    for (i = 0; i < m_pDataMap->x_iNumFreeBucket; ++i)
        m_pDataMap->x_aFreeBucket[i] = 0;

    return LSSHM_OK;
}


LsShmStatus_t LsShmPool::checkStaticData(const char *name)
{
    // check the file
    return ((m_pPool->x_iMagic == LSSHM_POOL_MAGIC)
            && (m_pPool->x_iSize == sizeof(LsShmPoolMem))
            && (strncmp((char *)m_pPool->x_aName, name, LSSHM_MAXNAMELEN) == 0)) ?
           LSSHM_OK : LSSHM_BADMAPFILE;
}


void LsShmPool::remap()
{
    m_pShm->remap();
    if (m_pShm->getShmMap() != m_pShmMap)
    {
        m_pPool = (LsShmPoolMem *)m_pShm->offset2ptr(m_iOffset);
        m_pShmMap = m_pShm->getShmMap();
        m_pPageMap = &m_pPool->x_page;
        m_pDataMap = &m_pPool->x_data;

        if (m_pPool->x_iLockOffset != 0)
            m_pShmLock = m_pShm->offset2pLock(m_pPool->x_iLockOffset);
    }
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

    m_iPageUnitSize = 0;
    m_iDataUnitSize = 0;
    m_pShmMap = NULL;
    m_iLockEnable = 1;
    m_pShm = shm;

    if (strcmp(name, LSSHM_SYSPOOL) == 0)
    {
        m_iOffset = shm->getShmMap()->x_iXdataOffset;   // the system pool
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
            offset = shm->allocPage(shm->unitSize(), remapped);
            if (offset == 0)
            {
                m_status = LSSHM_BADMAPFILE;
                return;
            }
            ::memset(offset2ptr(offset), 0, sizeof(LsShmPoolMem));
            m_iOffset = offset;
            p_reg->x_iValue = m_iOffset;

            extraOffset = offset + sizeof(LsShmPoolMem);
            extraSize = shm->unitSize() - sizeof(LsShmPoolMem);
        }
    }

    remap();
    if (m_pPool->x_iSize != 0)
    {
        if ((m_status = checkStaticData(name)) != LSSHM_OK)
        {
            LsShm::setErrMsg("Invalid SHM Pool [%s], magic=%08X(%08X), MapFile [%s].",
                name, m_pPool->x_iMagic, LSSHM_POOL_MAGIC, shm->fileName());
        }
    }
    else
    {
        m_status = createStaticData(name);
        // NOTE release extra to freeList
        if ((extraOffset != 0) && (extraSize > m_pDataMap->x_iMaxUnitSize))
            releaseData(extraOffset, extraSize);
    }
    syncData();
    mapLock();

    if (m_status == LSSHM_OK)
    {
        m_status = LSSHM_READY;
        m_iRef = 1;
        if (m_pPoolName != NULL)
        {
#ifdef DEBUG_RUN
            HttpLog::notice("LsShmPool::LsShmPool insert %s <%p>",
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
        HttpLog::notice("LsShmPool::~LsShmPool remove %s <%p>",
                        m_pPoolName, &m_objBase);
#endif
        m_pShm->getObjBase().remove(m_pPoolName);
        ::free(m_pPoolName);
        m_pPoolName = NULL;
    }
}


LsShmHash *LsShmPool::getNamedHash(const char *name,
                          size_t init_size, hash_fn hf, val_comp vc)
{
    LsShmHash *pObj;
    GHash::iterator itor;
    
    if (name == NULL)
        name = LSSHM_SYSHASH;
    
#ifdef DEBUG_RUN
    HttpLog::notice("LsShmPool::getNamedHash find %s <%p>",
                    name, &getObjBase());
#endif
    itor = getObjBase().find(name);
    if ((itor != NULL)
        && ((pObj = LsShmHash::checkHTable(itor, this, name, hf,
                                vc)) != (LsShmHash *)-1))
        return pObj;
   
    pObj = new LsShmHash(this, name, init_size, hf, vc);
    if (pObj != NULL)
    {
        if (pObj->getRef() != 0)
            return pObj;
        delete pObj;
    }
    return NULL;
}


LsShmHash *LsShmPool::getNamedLruHash(const char *name,
                          size_t init_size, hash_fn hf, val_comp vc)
{
    LsShmLruHash *pObj;
    GHash::iterator itor;
    
    if (name == NULL)
        name = LSSHM_SYSHASH;
    
#ifdef DEBUG_RUN
    HttpLog::notice("LsShmPool::getNamedLruHash find %s <%p>",
                    name, &getObjBase());
#endif
    itor = getObjBase().find(name);
    if ((itor != NULL)
        && ((pObj = (LsShmLruHash *)LsShmHash::checkHTable(itor, this, name, hf,
                                vc)) != (LsShmHash *)-1))
        return pObj;
   
    pObj = new LsShmLruHash(this, name, init_size, hf, vc);
    if (pObj != NULL)
    {
        if (pObj->getRef() != 0)
            return pObj;
        delete pObj;
    }
    return NULL;
}


LsShmHash *LsShmPool::getNamedXLruHash(const char *name,
                          size_t init_size, hash_fn hf, val_comp vc)
{
    LsShmXLruHash *pObj;
    GHash::iterator itor;
    
    if (name == NULL)
        name = LSSHM_SYSHASH;
    
#ifdef DEBUG_RUN
    HttpLog::notice("LsShmPool::getNamedXLruHash find %s <%p>",
                    name, &getObjBase());
#endif
    itor = getObjBase().find(name);
    if ((itor != NULL)
        && ((pObj = (LsShmXLruHash *)LsShmHash::checkHTable(itor, this, name, hf,
                                vc)) != (LsShmHash *)-1))
        return pObj;
   
    pObj = new LsShmXLruHash(this, name, init_size, hf, vc);
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
    remap();
    LsShmSize_t left;
    if ((left = m_pDataMap->x_chunk.x_iEnd - m_pDataMap->x_chunk.x_iStart) > 0)
        release2(m_pDataMap->x_chunk.x_iStart, left);
    mvFreeBucket();
    mvFreeList();
    m_pShm->clrReg(m_iRegNum);
    m_pShm->freeLock(m_pShmLock);
    m_pParent->release2(m_iOffset, sizeof(LsShmPoolMem));
    m_iOffset = m_pParent->m_iOffset;   // let parent/global take over
    m_iLockEnable = m_pParent->m_iLockEnable;
    if (owner != 0)
        m_pParent->close();
    m_pParent = NULL;
    m_pShmMap = NULL;               // force remap to parent/global
    remap();
}


LsShmOffset_t LsShmPool::alloc2(LsShmSize_t size, int &remapped)
{
    LsShmMap *map_o = m_pShm->getShmMap();
    LsShmOffset_t offset;

    if (size == 0)
        return 0;
    LSSHM_CHECKSIZE(size);

    remapped = 0;
    lock();
    remap();
    size = roundDataSize(size);
    if (size >= m_iPageUnitSize)
        offset = m_pShm->allocPage(size, remapped);
    else
    {
        if (size >= m_pDataMap->x_iMaxUnitSize)
            // allocate from FreeList
            offset = allocFromDataFreeList(size);
        else
            // allocate from bucket
            offset = allocFromDataBucket(size);
    }
    unlock();

    if ((map_o != m_pShm->getShmMap()) || (remapped != 0))
    {
        remap();
        remapped = 1;
    }
    return offset;
}


void LsShmPool::release2(LsShmOffset_t offset, LsShmSize_t size)
{
    lock();
    remap();
    size = roundDataSize(size);
    if (size >= m_iPageUnitSize)
        m_pShm->releasePage(offset, size);
    else
        releaseData(offset, size);
    unlock();
}


void LsShmPool::mvFreeList()
{
    if (m_pParent != NULL)
        m_pParent->addFreeList(&m_pDataMap->x_iFreeList);
    return;
}


void LsShmPool::addFreeList(LsShmOffset_t *pSrc)
{
    LsShmOffset_t listOffset;
    if ((listOffset = *pSrc) != 0)
    {
        lock();
        remap();
        LsShmFreeList *pFree;
        LsShmFreeList *pFreeList;
        LsShmOffset_t freeOffset;
        if ((freeOffset = m_pDataMap->x_iFreeList) != 0)
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
        m_pDataMap->x_iFreeList = listOffset;
        unlock();
        *pSrc = 0;
    }
    return;
}


void LsShmPool::mvFreeBucket()
{
    if (m_pParent != NULL)
        m_pParent->addFreeBucket(&m_pDataMap->x_aFreeBucket[0]);
    return;
}


void LsShmPool::addFreeBucket(LsShmOffset_t *pSrc)
{
    ++pSrc;
    int num = LSSHM_POOL_NUMBUCKET - 1;     // skip zero slot
    lock();
    remap();
    LsShmOffset_t bcktOffset;
    LsShmOffset_t *pDst = &m_pDataMap->x_aFreeBucket[1];
    while (--num >= 0)
    {
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
        }
        ++pSrc;
        ++pDst;
    }
    unlock();
    return;
}


//
// Internal release2
//
void LsShmPool::releaseData(LsShmOffset_t offset, LsShmSize_t size)
{
    if (size >= m_pDataMap->x_iMaxUnitSize)
    {
        // release to FreeList
        LsShmFreeList *pFree;
        LsShmFreeList *pFreeList;
        LsShmOffset_t freeOffset;

        // setup FreeList block
        pFree = (LsShmFreeList *)offset2ptr(offset);
        pFree->x_iSize = size;

        if ((freeOffset = m_pDataMap->x_iFreeList) != 0)
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
        m_pDataMap->x_iFreeList = offset;
    }
    else
    {
        // release to DataBucket
        LsShmSize_t bucketNum = dataSize2Bucket(size);
        LsShmOffset_t *pBucket;
        LsShmOffset_t *pData;

        pData = (LsShmOffset_t *)offset2ptr(offset);
        pBucket = &m_pDataMap->x_aFreeBucket[bucketNum];
        *pData = *pBucket;
        *pBucket = offset;
    }
}


//
// @brige syncData - load static SHM data to object memory
//
void LsShmPool::syncData()
{
    m_iPageUnitSize = m_pPageMap->x_iUnitSize;
    m_iDataUnitSize = m_pDataMap->x_iUnitSize;
}


//
// @brief map the lock into Object space
//
void LsShmPool::mapLock()
{
    if (m_pPool->x_iLockOffset == 0)
    {
        if ((m_pShmLock = m_pShm->allocLock()) == NULL)
            m_status = LSSHM_ERROR;
        else
            m_pPool->x_iLockOffset = m_pShm->pLock2offset(m_pShmLock);
    }
    else
        m_pShmLock = m_pShm->offset2pLock(m_pPool->x_iLockOffset);
    setupLock();
}


//
//  @brief allocFromDataFreeList - only for size >= data.x_iMaxUnitSize
//  @brief linear search the FreeList for it.
//  @brief if no match from FreeList then allocate from data.x_chunk
//
LsShmOffset_t LsShmPool::allocFromDataFreeList(LsShmSize_t size)
{
    LsShmOffset_t offset;
    LsShmFreeList *pFree;
    offset = m_pDataMap->x_iFreeList;
    while (offset != 0)
    {
        pFree = (LsShmFreeList *)offset2ptr(offset);
        if (pFree->x_iSize >= size)
        {
            LsShmSize_t left = pFree->x_iSize - size;
            pFree->x_iSize = left;
            if (left < m_pDataMap->x_iMaxUnitSize)
                mvDataFreeListToBucket(pFree, offset);
            return offset + left;
        }
        offset = pFree->x_iNext;
    }
    LsShmSize_t num = 1;
    return allocFromDataChunk(size, num);
}


LsShmOffset_t LsShmPool::allocFromDataBucket(LsShmSize_t size)
{
    LsShmOffset_t offset;
    LsShmOffset_t *np, *pBucket;

    LsShmOffset_t num = dataSize2Bucket(size);
    pBucket = &m_pDataMap->x_aFreeBucket[num];
    if ((offset = *pBucket) != 0)
    {
        np = (LsShmOffset_t *)offset2ptr(offset);
        *pBucket = *np;
        return offset;
    }
    return fillDataBucket(num, size);
}


LsShmOffset_t LsShmPool::allocFromGlobalBucket(
    LsShmOffset_t bucketNum, LsShmSize_t &num)
{
    LsShmOffset_t first;
    LsShmOffset_t next;
    LsShmOffset_t *np;

    if (m_pDataMap->x_aFreeBucket[bucketNum] == 0)
        return 0;

    LsShmSize_t cnt = 0;
    lock();
    remap();
    np = &m_pDataMap->x_aFreeBucket[bucketNum];
    next = first = *np;
    while (next != 0)
    {
        np = (LsShmOffset_t *)offset2ptr(next);
        next = *np;
        if (++cnt >= num)
            break;
    }
    m_pDataMap->x_aFreeBucket[bucketNum] = next;
    *np = 0;
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
    num = (m_pDataMap->x_iMaxUnitSize << 2) / size;
    if (num > LSSHM_MAX_BUCKET_SLOT)
        num = LSSHM_MAX_BUCKET_SLOT;

    LsShmOffset_t xoffset, offset;
    if ((m_pParent != NULL)
        && ((offset = m_pParent->allocFromGlobalBucket(bucketNum, num)) != 0))
    {
        if (num > 1)
            m_pDataMap->x_aFreeBucket[bucketNum] = offset + size;
        return offset;
    }
    xoffset = offset = allocFromDataChunk(size, num);

    if (num == 0)
        return offset;  // big problem, no more memory

    // take the first one - save the rest
    xoffset += size;
    if (--num != 0)
        m_pDataMap->x_aFreeBucket[bucketNum] = xoffset;

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

    avail = m_pDataMap->x_chunk.x_iEnd - m_pDataMap->x_chunk.x_iStart;
    numAvail = avail / size;
    if (numAvail)
    {
        if (numAvail < num) // shrink the num
            num = numAvail;
        offset = m_pDataMap->x_chunk.x_iStart;
        m_pDataMap->x_chunk.x_iStart += (num * size);
        return offset;
    }

    LsShmOffset_t releaseOffset;
    LsShmSize_t releaseSize;
    if (avail != 0)
    {
        offset = m_pDataMap->x_chunk.x_iStart;
        m_pDataMap->x_chunk.x_iStart += avail;
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
    if (needed < m_iPageUnitSize)
        needed = m_iPageUnitSize;

    int remapKey = 0;
    offset = m_pShm->allocPage(needed, remapKey);
    if (remapKey != 0)
        remap();

    if (releaseOffset != 0)
    {
        // merging leftover and newly allocated memory
        if (releaseOffset + releaseSize == offset)
        {
            offset = releaseOffset;
            needed += releaseSize;
        }
        else
            releaseData(releaseOffset, releaseSize);
    }

    // NOTE: must after remap...
    if (offset != 0)
    {
        m_pDataMap->x_chunk.x_iStart = offset;
        m_pDataMap->x_chunk.x_iEnd = offset + needed;
        return allocFromDataChunk(size, num);
    }
    num = 0;
    return offset; // in trouble
}


void LsShmPool::mvDataFreeListToBucket(
    LsShmFreeList *pFree, LsShmOffset_t offset)
{
    rmFromDataFreeList(pFree);

    LsShmOffset_t bucketNum = dataSize2Bucket(pFree->x_iSize);
    LsShmOffset_t *np;
    np = (LsShmOffset_t *)pFree; // cast to offset
    *np = m_pDataMap->x_aFreeBucket[bucketNum];
    m_pDataMap->x_aFreeBucket[bucketNum] = offset;
}


void LsShmPool::rmFromDataFreeList(LsShmFreeList *pFree)
{
    LsShmFreeList *xp;
    if (pFree->x_iPrev == 0)
        m_pDataMap->x_iFreeList = pFree->x_iNext;
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
}

