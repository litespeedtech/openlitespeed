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
#include <shm/lsshm.h>
#include <http/httplog.h>
#if 0
#include <log4cxx/logger.h>
#endif

#define MIN_POOL_DATAUNITSIZE   8

#ifdef DEBUG_RUN 
using namespace LOG4CXX_NS;
#endif


LsShm_status_t LsShmPool::createStaticData(const char * name)
{
    register LsShm_offset_t    i;
    
    m_pool->x_magic = LSSHM_POOL_MAGIC;
    m_pool->x_size = sizeof(lsShmPool_t);
    strncpy((char *)m_pool->x_name, name, LSSHM_MAXNAMELEN);
    
    m_pool->x_lockOffset = 0;
    
    // Page
    m_pageMap->x_chunk.x_start = 0;
    m_pageMap->x_chunk.x_end = 0;
    m_pageMap->x_freeList = 0;
    m_pageMap->x_unitSize = m_pShm->getShmMap()->x_unitSize;
                                // may be no max here!
    m_pageMap->x_maxUnitSize = m_pageMap->x_unitSize * LSSHM_POOL_NUMBUCKET;
    m_pageMap->x_numFreeBucket = LSSHM_POOL_NUMBUCKET;
    for (i = 0; i < m_pageMap->x_numFreeBucket; i++)
        m_pageMap->x_freeBucket[i] = 0;
    
    // Data
    m_dataMap->x_chunk.x_start = 0;
    m_dataMap->x_chunk.x_end = 0;
    m_dataMap->x_freeList = 0;
    m_dataMap->x_unitSize = MIN_POOL_DATAUNITSIZE;      // 8 bytes
    m_dataMap->x_maxUnitSize = m_dataMap->x_unitSize * LSSHM_POOL_NUMBUCKET;
    m_dataMap->x_numFreeBucket = LSSHM_POOL_NUMBUCKET;
    for (i = 0; i < m_dataMap->x_numFreeBucket; i++)
        m_dataMap->x_freeBucket[i] = 0;
    
    return LSSHM_OK;
}

LsShm_status_t LsShmPool::loadStaticData(const char * name)
{
    // check the file
    if ((m_pool->x_magic != LSSHM_POOL_MAGIC)
            || (m_pool->x_size != sizeof(lsShmPool_t))
            || (strncmp((char *)m_pool->x_name, name, LSSHM_MAXNAMELEN))
       )
        return LSSHM_BADMAPFILE;
    else
        return LSSHM_OK;
}

void LsShmPool::remap()
{
    m_pShm->remap();
    if (m_pShm->getShmMap() != m_pShmMap)
    {
        m_pool = (lsShmPool_t *)m_pShm->offset2ptr (m_offset) ;
        m_pShmMap = m_pShm->getShmMap();
        m_pageMap = &m_pool->x_page;
        m_dataMap = &m_pool->x_data;
        
        if (m_pool->x_lockOffset)
            m_pShmLock = m_pShm->offset2pLock(m_pool->x_lockOffset);
    }
}

LsShmPool::LsShmPool( LsShm * shm , const char * name)
                                : m_magic(LSSHM_POOL_MAGIC)
                                , m_poolName(strdup(name))
                                , m_shmOwner(0)
                                , m_objBase (shm->getObjBase())
                                , m_ref(0)
{
    LsShm_offset_t extraOffset = 0;
    LsShm_size_t extraSize = 0;
    
    m_status = LSSHM_NOTREADY; // no good
    
    if ((!m_poolName) || (strlen(m_poolName) >= LSSHM_MAXNAMELEN) )
    {
        m_status = LSSHM_BADPARAM;
        return;
    }
    
    m_pageUnitSize = 0;
    m_dataUnitSize = 0;
    m_pShmMap = NULL;
    m_lockEnable = 1;
    m_pShm = shm;
    
    if ( !strcmp(name, LSSHM_SYSPOOL) )
    {
        /* The system pool */
        m_offset = shm->getShmMap()->x_xdataOffset ;
    }
    else
    {
        lsShmReg_t *    p_reg;
        p_reg = shm->findReg(name);
        if (!p_reg)
        {
            p_reg = shm->addReg(name);
            if (!p_reg)
            {
                m_status = LSSHM_BADMAPFILE;
                return ;
            }
            p_reg->x_flag = 1;
            p_reg->x_value = 0; /* not inited yet! */
        }
        if (!(m_offset = p_reg->x_value))
        {
            /* create memory for new Pool */
            /* allocate header from SYS POOL */
            LsShm_offset_t offset;
            int remapped;
            offset = shm->allocPage(shm->unitSize(), remapped);
            if (!offset)
            {
                m_status = LSSHM_BADMAPFILE;
                return;
            }
            ::memset(offset2ptr(offset), 0, sizeof(lsShmPool_t));
            m_offset = offset;
            p_reg->x_value = m_offset;
            
            extraOffset = sizeof(lsShmPool_t);
            extraSize = shm->unitSize() - sizeof(lsShmPool_t);
        }
    }
    
    remap();
    if ((m_pool->x_size) && (m_pool->x_magic == LSSHM_POOL_MAGIC))
    {
        m_status = loadStaticData(name);
    }
    else
    {
        m_status = createStaticData(name);
        // NOTE release extra to freeList
        if (extraOffset && (extraSize > m_dataMap->x_maxUnitSize))
        {
            releaseData( extraOffset, extraSize );
        }
    }
    syncData();
    mapLock();
    
    if (m_status == LSSHM_OK)
    {
        m_status = LSSHM_READY;
        m_ref = 1;
        
#ifdef DEBUG_RUN
        HttpLog::notice( 
             "LsShmPool::LsShmPool insert %s <%p>"
            , m_poolName, &m_objBase);
#endif        
        m_objBase.insert(m_poolName, this);
    }
}

LsShmPool::~LsShmPool()
{
    if (m_poolName)
    {
#ifdef DEBUG_RUN
        HttpLog::notice( 
             "LsShmPool::~LsShmPool remove %s <%p>"
            , m_poolName, &m_objBase);
#endif

        m_objBase.remove(m_poolName);
        ::free (m_poolName);
        m_poolName = NULL;
    }
}

//
// @brief - get the default system pool from SHM
//
LsShmPool* LsShmPool::get( )
{
    LsShm * shm = LsShm::get(LSSHM_SYSSHM, LSSHM_INITSIZE);
    if (shm)
    {
        LsShmPool * me = get(shm, LSSHM_SYSPOOL);
        if (me)
        {
            me->m_shmOwner = 1;
            return(me);
        }
        else
        {
            shm->unget();
        }
    }
    return NULL;
}

//
// @brief - create a pool by myself
//
LsShmPool* LsShmPool::get( const char* name )
{
    if ((! name ) || ( !strcmp(LSSHM_SYSPOOL, name) ))
        return get(); // return the system default pool

    // get a SHM by itself
    LsShm * shm = LsShm::get(name, LSSHM_INITSIZE);
    if (shm)
    {
        LsShmPool * me = get(shm, name);
        if (me)
        {
            me->m_shmOwner = 1;
            return(me);
        }
        else
        {
            shm->unget();
        }
    }
    return NULL;
}

LsShmPool* LsShmPool::get( LsShm* shm, const char* name )
{
    register LsShmPool * pObj;
    GHash::iterator iter;
    
    if ( !name )
        name = LSSHM_SYSPOOL;
    
    if (!shm)
        return get(name);
    
#ifdef DEBUG_RUN
    HttpLog::notice( 
             "LsShmPool::get find %s <%p>", name, &shm->getObjBase());
#endif

    iter = shm->getObjBase().find(name);
    if (iter)
    {
        
#ifdef DEBUG_RUN
        HttpLog::notice( 
             "LsShmPool::get %s <%p> return %p "
            , name, &shm->getObjBase(), iter);
#endif
    
        pObj = (LsShmPool*) iter->second();
        if (pObj->m_magic != LSSHM_POOL_MAGIC)
        {
            return NULL;
        }
        pObj->up_ref();
        return pObj;
    }
    pObj = new LsShmPool(shm, name);
    if (pObj)
    {
        if (pObj->m_ref)
            return pObj;
        delete pObj;
    }
    return NULL;
}

void    LsShmPool::unget()
{
    LsShm * p = NULL;
    if (m_shmOwner)
    {
        m_shmOwner = 0;
        p = m_pShm; // delayed remove
    }
    if (down_ref() == 0)
    {
        delete this;
    }
    if (p) 
        p->unget();
}

//
// @brige syncData - load static SHM data to object memory
//
void LsShmPool::syncData()
{
    m_pageUnitSize = m_pageMap->x_unitSize;
    m_dataUnitSize = m_dataMap->x_unitSize;
}

//
// @brief map the lock into Object space
//
void LsShmPool::mapLock()
{
    if (!m_pool->x_lockOffset)
    {
        if (!(m_pShmLock = m_pShm->allocLock()))
            m_status = LSSHM_ERROR;
        else
            m_pool->x_lockOffset = m_pShm->pLock2offset(m_pShmLock);
    }
    else
        m_pShmLock = m_pShm->offset2pLock(m_pool->x_lockOffset);
    setupLock();
}

LsShm_offset_t LsShmPool::alloc2( LsShm_size_t size, int & remapped )
{
    lsShmMap_t * map_o = m_pShm->getShmMap();
    register LsShm_offset_t offset;

    if (size == 0)
        return 0;
    LSSHM_CHECKSIZE(size);
        
    remapped = 0;
    lock();
    remap();
    size = roundDataSize(size);
    if ( size >= m_pageUnitSize )
    //if ( size >= m_pageMap->x_unitSize )
    {
        offset = m_pShm->allocPage(size, remapped);
    }
    else
    {
        if (size >= m_dataMap->x_maxUnitSize)
            // allocate from FreeList
            offset = allocFromDataFreeList(size);
        else
            // allocate from bucket
            offset = allocFromDataBucket(size);
    }
    unlock();
    
    if ( (map_o != m_pShm->getShmMap()) || remapped )
    {
        remap();
        remapped = 1;
    }
    return offset;
}

void LsShmPool::release2( LsShm_offset_t offset, LsShm_size_t size )
{
    lock();
    remap();
    size = roundDataSize(size);
    if ( size >= m_pageUnitSize )
    //if ( size >= m_pageMap->x_unitSize )
    {
        m_pShm->releasePage (offset, size);
    }
    else
    {
        releaseData(offset, size);
    }
    unlock();
}
    
//
// Internal release2
//
void LsShmPool::releaseData( LsShm_offset_t offset, LsShm_size_t size )
{
    if (size >= m_dataMap->x_maxUnitSize)
    {
        // release to FreeList
        lsShmFreeList_t * pFree;
        lsShmFreeList_t * pFreeList;
        LsShm_offset_t freeOffset;
        
        // setup FreeList block
        pFree = (lsShmFreeList_t *)offset2ptr(offset);
        pFree->x_size = size;
        
        if ( (freeOffset = m_dataMap->x_freeList) )
        {
            pFreeList = (lsShmFreeList_t *) offset2ptr( freeOffset );
            pFree->x_next = freeOffset;
            pFree->x_prev = 0;
            pFreeList->x_prev = offset;
        }
        else
        {
            pFree->x_next = 0;
            pFree->x_prev = 0;
        }
        m_dataMap->x_freeList = offset;
    }
    else
    {
        // release to DataBucket
        LsShm_size_t bucketNum = dataSize2Bucket(size);
        LsShm_offset_t * pBucket;
        LsShm_offset_t * pData;
        
        pData = (LsShm_offset_t *) offset2ptr(offset);
        pBucket = &m_dataMap->x_freeBucket[bucketNum];
        *pData = *pBucket;
        *pBucket = offset;
    }
}

//
//  @brief allocFromBucket - only for size >= data.x_maxUnitSize
//  @brief linear search the FreeList for it.
//  @brief if no match from FreeList then allocate from data.x_chunk
//
LsShm_offset_t LsShmPool::allocFromDataFreeList( LsShm_size_t size )
{
    register LsShm_offset_t offset;
    register lsShmFreeList_t * pFree;
    for ( offset = m_dataMap->x_freeList ; offset; )
    {
        pFree = (lsShmFreeList_t *) offset2ptr(offset);
        if (pFree->x_size >= size)
        {
            LsShm_size_t left = pFree->x_size - size;
            pFree->x_size = left;
            if (left < m_dataMap->x_maxUnitSize)
                mvDataFreeListToBucket(pFree, offset);
            return offset + left;
        }
        offset = pFree->x_next;
    }
    
    LsShm_size_t num = 1;
    return allocFromDataChunk(size, num);
}

void LsShmPool::rmFromDataFreeList( lsShmFreeList_t* pFree )
{
    register lsShmFreeList_t* xp;
    if (pFree->x_prev == 0)
    {
        m_dataMap->x_freeList = pFree->x_next;
    }
    else
    {
        xp = (lsShmFreeList_t*) offset2ptr ( pFree->x_prev );
        xp->x_next = pFree->x_next;
    }
    if (pFree->x_next)
    {
        xp = (lsShmFreeList_t*) offset2ptr ( pFree->x_next );
        xp->x_prev = pFree->x_prev;
    }
}

void LsShmPool::mvDataFreeListToBucket( lsShmFreeList_t* pFree,
                                        LsShm_offset_t  offset )
{
    rmFromDataFreeList( pFree );

    LsShm_offset_t bucketNum = dataSize2Bucket(pFree->x_size);
    LsShm_offset_t * np;
    np = (LsShm_offset_t*) pFree; // cast to offset
    *np = m_dataMap->x_freeBucket[bucketNum];
    m_dataMap->x_freeBucket[bucketNum] = offset;
}

LsShm_offset_t LsShmPool::allocFromDataBucket( LsShm_size_t size )
{
    LsShm_offset_t offset;
    LsShm_offset_t * np, * pBucket;
    
    LsShm_offset_t num = dataSize2Bucket(size);
    pBucket = &m_dataMap->x_freeBucket[num];
    if ( (offset = *pBucket) )
    {
        np = (LsShm_offset_t*)offset2ptr(offset);
        *pBucket = *np;
        return offset;
    }
    
    return fillDataBucket(num, size);
}

//
// @brief fillDataBucket allocates memory from Chunk storage.
// @brief This may cause remap, only use offset...
// @brief bucketNum and size should be logical connected. 
// @brief passing both to avoid a extra calulation.
// @brief the freebucket[bucketNum] will set to the new allocated pool.
// @brief return the offset of the newly allocated memory.
//
LsShm_offset_t LsShmPool::fillDataBucket( LsShm_offset_t bucketNum,
                                          LsShm_size_t size )
{
    LsShm_size_t num;
    // allocated according to data size
    num = (m_dataMap->x_maxUnitSize << 2) / size;
    
    register LsShm_offset_t xoffset, offset;
    xoffset = offset = allocFromDataChunk(size, num);
    
    if (!num)
        return offset;  // big problem no more memory
    
    // take the first one - save the rest
    xoffset += size;
    if (--num)
        m_dataMap->x_freeBucket[bucketNum] = xoffset;
    
    register uint8_t * xp;
    xp = (uint8_t *) offset2ptr(offset);
    while (num--)
    {
        xp += size;
        xoffset += size;
        *((LsShm_offset_t *)xp) = num ? xoffset : 0;
    }
    return offset;
}

//
// @brief allocFromDataChunk 
// @breif num suggested the number of bucket to allocate.
// 
// @brief could reduce the number if it is too many, currently it set max to 0x20 regradless.
// @brief fill the bucket will all available space.
// @brief return offset and num of bucket.
// @brief could cause remap.
// @brief always check return num and offset.
//
LsShm_offset_t LsShmPool::allocFromDataChunk( LsShm_size_t size, LsShm_size_t & num )
{
    register LsShm_offset_t offset;
    register LsShm_size_t numAvail;
    register LsShm_size_t avail;
    
    if (num > LSSHM_MAX_BUCKET_SLOT)
        num = LSSHM_MAX_BUCKET_SLOT;
    
    avail = m_dataMap->x_chunk.x_end - m_dataMap->x_chunk.x_start;
    numAvail = avail / size;
    if (numAvail)
    {
        if (numAvail < num) // shrink the num
            num = numAvail;
        offset = m_dataMap->x_chunk.x_start;
        m_dataMap->x_chunk.x_start += (num * size);
        return offset;
    }
    
    LsShm_offset_t releaseOffset;
    LsShm_size_t releaseSize;
    if ( avail )
    {
        offset = m_dataMap->x_chunk.x_start;
        m_dataMap->x_chunk.x_start += avail;
        // release all Chunk memory
        // releaseData(offset, avail);
        releaseOffset = offset;
        releaseSize = avail;
    } else {
        releaseOffset = 0;
        releaseSize = 0;
    }
    
    // figure pagesize needed - round to SHM page unit to avoid waste
    register LsShm_size_t needed = roundPageSize( size * num);
    if ( needed < m_pageUnitSize )
        needed = m_pageUnitSize;
    
    int remapKey = 0;
    offset = m_pShm->allocPage(needed, remapKey);
    if (remapKey)
        remap();
    
    if (releaseOffset)
    {
        // merging leftover and newly allocated memory
        if  (releaseOffset + releaseSize == offset)
        {
            offset = releaseOffset;
            needed += releaseSize;
        }
        else
        {
            releaseData(releaseOffset, releaseSize);
        }
    }
    
    // NOTE: must after remap...
    if ( offset )
    {
        m_dataMap->x_chunk.x_start = offset;
        m_dataMap->x_chunk.x_end = offset + needed;
        return allocFromDataChunk( size, num );
    }
    num = 0;
    return offset; // in trouble
}


