/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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

//
// replicated the gpool idea.
//  lsShmPool
//  (0) Page memeory are handled by the LsShm.
//  (1) Maintain fixed size bucket for fast allocation (4-256 in 8 bytes interval).
//  (2) Anything bigger than max bucket size will get from the freelist allocation.
//
//  lsShmPool_t -> the physical map structure
//
//  all map memember start with x_
//

// runtime free memory for the pool
typedef struct {
    LsShm_offset_t  x_start;
    LsShm_offset_t  x_end;
} shmMapChunk_t;

typedef struct {
    LsShm_offset_t  x_next;
    LsShm_offset_t  x_prev;
    LsShm_size_t    x_size;
} lsShmFreeList_t;

#define LSSHM_POOL_NUMBUCKET    0x20
#define LSSHM_MAX_BUCKET_SLOT   0x8 // max bucket slot allocation

#if 0
// useful for test purpose
#define LSSHM_POOL_NUMBUCKET    0x4
#define LSSHM_MAX_BUCKET_SLOT   0x4 // max bucket slot allocation
#endif

typedef struct {
    LsShm_size_t    x_unitSize;
    LsShm_size_t    x_maxUnitSize;
    LsShm_size_t    x_numFreeBucket;
    shmMapChunk_t   x_chunk;            // unused after alloc2
    LsShm_offset_t  x_freeList;         // Save the big free list
    LsShm_offset_t  x_freeBucket[LSSHM_POOL_NUMBUCKET]; 
} lsShmPoolMap_t;

typedef struct {
    uint32_t        x_magic;
    uint8_t         x_name[LSSHM_MAXNAMELEN];
    uint32_t        x_size;
    lsShmPoolMap_t  x_page;
    lsShmPoolMap_t  x_data;
    LsShm_offset_t  x_lockOffset;
} lsShmPool_t;

class LsShmPool : public ls_shmpool_s , ls_shmobject_s
{
    uint32_t            m_magic;
    char *              m_poolName;     // Name
    LsShm *             m_pShm;         // SHM handle
    lsShmPool_t *       m_pool;
    lsShmPoolMap_t *    m_pageMap;
    lsShmPoolMap_t *    m_dataMap;
    LsShm_status_t      m_status;       // Ready ...
    LsShm_offset_t      m_offset;       // find from SHM registry
    lsShmMap_t *        m_pShmMap;     
    lsi_shmlock_t *     m_pShmLock;
    int8_t              m_lockEnable;
    int8_t              m_shmOwner;     // indicated if I own the SHM
    int8_t              m_notUsed[2];
    
    LsShm_size_t        m_pageUnitSize; 
    LsShm_size_t        m_dataUnitSize;

private:
    explicit LsShmPool (LsShm * , const char * name );
    ~LsShmPool();
    
public:    
    static LsShmPool * get();                   // return the default sys pool
    static LsShmPool * get(const char * name);  // return the pool by itself
    static LsShmPool * get(LsShm *, const char * name);
    void unget();
    
    const char * name() const
    { return (const char *)m_pool->x_name; };
    
    LsShm_status_t status() const
    { return m_status; };
    
    void * offset2ptr(LsShm_offset_t offset) const
    { return (void *)m_pShm->offset2ptr(offset); }
    
    LsShm_offset_t ptr2offset(const void * ptr) const
    { return m_pShm->ptr2offset(ptr); }
    
    LsShm_offset_t  alloc2(LsShm_size_t size, int & remapped);
    void  release2(LsShm_offset_t offset, LsShm_size_t size);

    void enableLock()
    { m_lockEnable = 1; }
    
    void disableLock()
    { m_lockEnable = 0; }
    
    // Special section for registries entries 
    inline lsShmReg_t *  getReg(int num)
    { return m_pShm->getReg(num); }
    inline lsShmReg_t *  findReg(const char * name)
    { return m_pShm->findReg(name); }
    inline lsShmReg_t *  addReg(const char * name)
    { return m_pShm->addReg(name); }
    
    inline lsShmMap_t * getShmMap() const
    { return m_pShm->getShmMap(); }
    
    inline LsShm_size_t getShmMap_maxSize() const
    { return m_pShm->maxSize(); }
    
    inline LsShm_size_t getShmMap_oldMaxSize() const
    { return m_pShm->oldMaxSize(); }
    
    inline LsShmLock * lockPool()
    { return m_pShm->lockPool(); }
    
#if 0
    inline int lockShm()
    { return m_pShm->lockShm(); }
    
    inline int unlockShm()
    { return m_pShm->unlockShm(); }
    
    inline int trylockShm()
    { return m_pShm->trylockShm(); }
#endif
    
    inline void setMapOwner(int o)
    { m_shmOwner = o; }
    
    inline HashStringMap< lsi_shmobject_t *> & getObjBase()
    { return m_pShm->getObjBase(); }
    
    inline int lock ()
    {   return m_lockEnable && lsi_shmlock_lock( m_pShmLock ); }

    inline int trylock ()
    {   return m_lockEnable && lsi_shmlock_trylock( m_pShmLock ); }
    
    inline int unlock ()
    {   return m_lockEnable && lsi_shmlock_unlock( m_pShmLock ); }
    
    inline void checkRemap()
    { remap(); }
private:
    int setupLock()
    {   return m_lockEnable && lsi_shmlock_setup( m_pShmLock ); }

    void syncData();   
    void mapLock();
    
    // for internal purpose
    void  releaseData(LsShm_offset_t offset, LsShm_size_t size);
   
    LsShm_status_t loadStaticData(const char * name);
    LsShm_status_t createStaticData(const char * name);
    void            remap(); 
    
    //
    // data related
    //
    LsShm_offset_t allocFromDataFreeList(LsShm_size_t size);
    LsShm_offset_t allocFromDataBucket(LsShm_size_t size);
    LsShm_offset_t allocFromDataChunk(LsShm_size_t size, LsShm_size_t & num);
    void mvDataFreeListToBucket(lsShmFreeList_t * pFree,
                            LsShm_offset_t offset );
    void rmFromDataFreeList(lsShmFreeList_t * pFree);
    
    LsShm_offset_t fillDataBucket(LsShm_offset_t bucketNum , LsShm_offset_t  size);
    
    // NOTE m_pageUnitSize and m_dataUnitSize are set only at start up
    inline LsShm_offset_t pageSize2Bucket(LsShm_size_t size) const
    { return ((size + m_pageUnitSize - 1) 
                    / m_pageUnitSize)
                    * m_pageUnitSize ;
    }
    
    inline LsShm_size_t roundPageSize(LsShm_size_t size) const
    { return ((size - 1 + m_pageUnitSize)
            / m_pageUnitSize) * m_pageUnitSize; };
    
    inline LsShm_offset_t dataSize2Bucket(LsShm_size_t size) const
    { return (size / m_dataUnitSize); }
    
    inline LsShm_size_t roundDataSize(LsShm_size_t size) const
    { return ((size - 1 + m_dataUnitSize)
            / m_dataUnitSize) * m_dataUnitSize; };
            
private:
    LsShmPool( const LsShmPool& other );
    LsShmPool& operator=( const LsShmPool& other );
    bool operator==( const LsShmPool& other );
    
    // house keeping
    HashStringMap< lsi_shmobject_t *> & m_objBase;
    int m_ref;
    inline int up_ref() { return ++m_ref; }
    inline int down_ref() { return --m_ref; }
    
#ifdef LSSHM_DEBUG_ENABLE
    friend class debugBase;
#endif    
};

#endif // LSSHMPOOL_H
