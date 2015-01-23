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
#ifndef LSSHMCACHE_H
#define LSSHMCACHE_H

#ifdef LSSHM_DEBUG_ENABLE
class debugBase;
#endif

#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#include <http/httplog.h>
#include <util/datetime.h>
#include <shm/lsi_shm.h>
#include <shm/lsshmtypes.h>
#include <shm/lsshmhash.h>

typedef uint32_t LsShm_hkey_t;

//
//  @brief lsShm_hCacheData_s
//
typedef struct lsShm_hCacheData_s
{
    uint32_t        x_magic;
    // LsShm_size_t    x_size;     
    int32_t         x_expireTime;
    int32_t         x_expireTimeMs; // we use Ms only
    uint8_t         x_inited;       // 
    uint8_t         x_tracked;      // used by push and pop
    uint8_t         x_expired;      // expired item
    uint8_t         x_notused;      // expired item
    LsShm_offset_t  x_iteratorOffset;
    LsShm_offset_t  x_next;         // forward link
    LsShm_offset_t  x_back;         // backward link
    uint8_t         x_udata[0];
} lsShm_hCacheData_t;

//
//  @brief lsShm_hCacheHdr_t
//  @brief the cache container header block
//  @brief x_front is the Obj's top(first) offset 
//  @brief x_back is the Obj's bottom(last) offset 
//
typedef struct lsShm_hCacheHdr_s
{
    LsShm_offset_t  x_front;
    LsShm_offset_t  x_back;
    int             x_num;      /* num of element */
    /* LsShm_offset_t  x_lockOffset; */
} lsShm_hCacheHdr_t;

//
//  @brief LsShmCachePool
//  @brief In general, this is a queue; but it allows used to alter the ordering.
//  @brief  setFirst()  - place to the front of the queue
//  @brief  setLast()   - place to the last of the queue
//  @brief  push()      - place the object to the front of queue
//  @brief  pop()       - place the object to the end of queue
//
class LsShmCache
{
typedef int (*udata_callback_fn) ( lsShm_hCacheData_t * pObj, void * pUParam);
typedef int (*udata_init_fn) ( lsShm_hCacheData_t * pObj, void * pUParam);
typedef int (*udata_remove_fn) ( lsShm_hCacheData_t * pObj, void * pUParam);

    LsShm_status_t  m_status;
    LsShmHash *     m_pShmHash;
    LsShm_offset_t  m_hdrOffset;
    uint32_t        m_magic;
    LsShm_size_t    m_uDataSize;
    
    udata_init_fn   m_dataInit_cb;      // callback on datainit
    udata_init_fn   m_dataRemove_cb;    // callback before remove
    
public:
    LsShmCache (uint32_t magic
                , const char *  cacheName
                , const char *  shmHashName
                , size_t        initHashSize = 101
                , LsShm_size_t  uDataSize = 0
                , LsShmHash::hash_fn hf = LsShmHash::hash_buf
                , LsShmHash::val_comp vc = LsShmHash::comp_buf
                , udata_init_fn udataInitCallback = NULL
                , udata_remove_fn udataRemoveCallback = NULL
                );
    
    ~LsShmCache()
    { 
        if (m_pShmHash)
        {
            m_pShmHash->unget();
            m_pShmHash = NULL;
            m_status = LSSHM_NOTREADY;
        }
    }
    
    //
    //  @brief push 
    //  @brief push new object to the front of the queue.
    void push( lsShm_hCacheData_t * pObj);
    
    //
    //  @brief pop
    //  @brief pop an object from the back of the queue
    void pop();
    
    //
    //  @brief push_back
    //  @brief push new obj to the back of the queue
    void push_back( lsShm_hCacheData_t * pObj);
    
    //  @brief setFirst
    //  @brief move existing obj inside the queue to the front of the queue
    void setFirst( lsShm_hCacheData_t * pObj );
    
    //  @brief setLast
    //  @brief move existing obj inside the queue to the back of the queue
    void setLast( lsShm_hCacheData_t * pObj );
    
    //
    //  @brief removeObj
    //  @bried remove existing object from queue
    void removeObj( lsShm_hCacheData_t * pObj, void * pUParam = NULL)
    {
        disconnectObj( pObj );
        removeObjData( pObj, pUParam);
    }
    
    lsShm_hCacheData_t * findObj( const void * pKey , int keyLen)
    {
        LsShmHash::iterator iter;
        iter = m_pShmHash->find_iterator(pKey, keyLen);
        if (iter)
            return (lsShm_hCacheData_t *) iter->p_value();
        else
            return NULL;
    }

    //
    //  @brief getObj
    //  @brief (1) if the key already defined then return the obj
    //  @brief (2) create new obj and return the newly obj
    //
    lsShm_hCacheData_t * getObj( const void * pKey , int keyLen
                , const void * pValue , int valueLen
                , void * pUParam = NULL);
    
    int isElementExpired(lsShm_hCacheData_t * pObj)
    {
        if (pObj->x_expired
                ||  ((((DateTime::s_curTime - pObj->x_expireTime) * 1000)
                     + ((DateTime::s_curTimeUs / 1000) - pObj->x_expireTimeMs)) > 0)
           )
            return 1;
        else
            return 0;
    }
    
    //
    //  @brief remove2NonExpired -
    //  @brief remove and pop element if it marked expired.
    //  @brief remove and pop element if item's time is expired.
    //  @brief will stop check on first non expired element
    //
    int remove2NonExpired(void * pUParam = NULL);
    
    //
    //  @brief removeAllExpired -
    //  @brief remove and pop element if it marked expired.
    //  @brief remove and pop element if item's time is expired.
    //  @brief will search the entire queue.
    //
    int removeAllExpired(void * pUParam = NULL);
    
    void remap()
        { m_pShmHash->checkRemap(); }
        
    LsShm_status_t  status() const
        { return m_status; }
    
    LsShm_offset_t ptr2offset( const void * ptr) const
    { return m_pShmHash->ptr2offset(ptr); }
    void * offset2ptr( LsShm_offset_t  offset) const
    { return m_pShmHash->offset2ptr(offset); }
    LsShm_offset_t alloc2(LsShm_size_t size, int & remapped)
    { return m_pShmHash->alloc2(size, remapped); }
    void release2(LsShm_offset_t offset, LsShm_size_t size)
    { m_pShmHash->release2(offset, size); }
    
    void stat(LsHash_stat_t & stat, udata_callback_fn _cb);
    
    int loop(udata_callback_fn _cb, void * pUParam);
    
    int lock ()
    {
        // int retValue =  m_pShmHash->g_lock();
        int retValue =  m_pShmHash->lock();
        m_pShmHash->checkRemap();
        return retValue;
    };
    int unlock ()
    {
        // return m_pShmHash->g_unlock();
        return m_pShmHash->unlock();
    };
    
private:
    LsShmCache( const LsShmCache & );
    LsShmCache & operator=( const LsShmCache & );
    bool operator==( const LsShmCache& );
    
    // 
    lsShm_hCacheHdr_t * getCacheHeader()
    {
        return (lsShm_hCacheHdr_t*)(m_pShmHash->offset2ptr(m_hdrOffset));
    }
    
    //
    //  @brief removeObjData
    //  @brief (1) call user callback if defined.
    //  @brief (2) remove the hash entry of the item.
    //
    void removeObjData( lsShm_hCacheData_t * pObj, void * pUParam = NULL)
    {
#ifdef DEBUG_RUN
        LsShmHash::iterator iter = m_pShmHash->offset2iterator( pObj->x_iteratorOffset ) ;
        HttpLog::notice("LsShmCache::removeObjData %6d iter <%p> obj <%p> size %d"
            , getpid(), iter, pObj, (long)pObj-(long)iter);
#endif

        if (m_dataRemove_cb)
            (*m_dataRemove_cb)(pObj, pUParam);
        m_pShmHash->erase_iterator_helper(
            m_pShmHash->offset2iterator( pObj->x_iteratorOffset ) );
    }
    
    //
    //  @brief disconnectObj
    //  @brief remove obj the from queue
    //
    void disconnectObj( lsShm_hCacheData_t * pObj);
    
#ifdef LSSHM_DEBUG_ENABLE
    friend class debugBase;
#endif
};

#endif /* LSSHMCACHE_H */

