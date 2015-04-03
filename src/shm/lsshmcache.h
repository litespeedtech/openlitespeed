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

#include <shm/lsshmhash.h>
#include <shm/lsshmtypes.h>

/**
 * @file
 */


typedef uint32_t LsShmHKey;

//
//  @brief lsShm_hCacheData_s
//
typedef struct lsShm_hCacheData_s
{
    uint32_t        x_iMagic;
    //LsShmSize_t     x_iSize;
    int32_t         x_iExpireTime;
    int32_t         x_iExpireTimeMs; // we use Ms only
    uint8_t         x_iInited;       //
    uint8_t         x_iTracked;      // used by push and pop
    uint8_t         x_iExpired;      // expired item
    uint8_t         x_notused;
    LsShmOffset_t   x_iIteratorOffset;
    LsShmOffset_t   x_iNext;         // forward link
    LsShmOffset_t   x_iBack;         // backward link
    uint8_t         x_aData[0];
} lsShm_hCacheData_t;

//
//  @brief lsShm_hCacheHdr_t
//  @brief the cache container header block
//  @brief x_iFront is the Obj's top(first) offset
//  @brief x_iBack is the Obj's bottom(last) offset
//
typedef struct lsShm_hCacheHdr_s
{
    LsShmOffset_t   x_iFront;
    LsShmOffset_t   x_iBack;
    int             x_iNum;          // num of element
    //LsShmOffset_t   x_iLockOffset;
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
    typedef int (*udata_callback_fn)(lsShm_hCacheData_t *pObj, void *pUParam);
    typedef int (*udata_init_fn)(lsShm_hCacheData_t *pObj, void *pUParam);
    typedef int (*udata_remove_fn)(lsShm_hCacheData_t *pObj, void *pUParam);

public:
    LsShmCache(uint32_t   magic
               , const char     *cacheName
               , const char     *shmHashName
               , size_t          initHashSize = 101
               , LsShmSize_t     uDataSize = 0
               , LsShmHash::hash_fn hf = LsShmHash::hashBuf
               , LsShmHash::val_comp vc = LsShmHash::compBuf
               , udata_init_fn   udataInitCallback = NULL
               , udata_remove_fn udataRemoveCallback = NULL
              );

    ~LsShmCache()
    {
        if (m_pShmHash != NULL)
        {
            m_pShmHash->close();
            m_pShmHash = NULL;
            m_status = LSSHM_NOTREADY;
        }
    }

    //
    //  @brief push
    //  @brief push new object to the front of the queue.
    void push(lsShm_hCacheData_t *pObj);

    //
    //  @brief pop
    //  @brief pop an object from the back of the queue
    void pop();

    //
    //  @brief push_back
    //  @brief push new obj to the back of the queue
    void push_back(lsShm_hCacheData_t *pObj);

    //  @brief setFirst
    //  @brief move existing obj inside the queue to the front of the queue
    void setFirst(lsShm_hCacheData_t *pObj);

    //  @brief setLast
    //  @brief move existing obj inside the queue to the back of the queue
    void setLast(lsShm_hCacheData_t *pObj);

    //
    //  @brief removeObj
    //  @bried remove existing object from queue
    void removeObj(lsShm_hCacheData_t *pObj, void *pUParam = NULL)
    {
        disconnectObj(pObj);
        removeObjData(pObj, pUParam);
    }

    lsShm_hCacheData_t *findObj(const void *pKey, int keyLen)
    {
        LsShmHash::iteroffset iterOff;
        ls_str_pair_t parms;
        ls_str_set(&parms.key, (char *)pKey, keyLen);
        iterOff = m_pShmHash->findIterator(&parms);
        return (lsShm_hCacheData_t *)((iterOff != 0) ?
                m_pShmHash->offset2iteratorData(iterOff) : NULL);
    }

    //
    //  @brief getObj
    //  @brief (1) if the key already defined then return the obj
    //  @brief (2) create new obj and return the newly obj
    //
    lsShm_hCacheData_t *getObj(const void *pKey, int keyLen,
                               const void *pValue , int valueLen, void *pUParam = NULL);

    int isElementExpired(lsShm_hCacheData_t *pObj);

    //
    //  @brief remove2NonExpired -
    //  @brief remove and pop element if it marked expired.
    //  @brief remove and pop element if item's time is expired.
    //  @brief will stop check on first non expired element
    //
    int remove2NonExpired(void *pUParam = NULL);

    //
    //  @brief removeAllExpired -
    //  @brief remove and pop element if it marked expired.
    //  @brief remove and pop element if item's time is expired.
    //  @brief will search the entire queue.
    //
    int removeAllExpired(void *pUParam = NULL);

    LsShmStatus_t status() const
    {   return m_status; }

    LsShmOffset_t ptr2offset(const void *ptr) const
    {   return m_pShmHash->ptr2offset(ptr); }
    void *offset2ptr(LsShmOffset_t offset) const
    {   return m_pShmHash->offset2ptr(offset); }
    LsShmOffset_t alloc2(LsShmSize_t size, int &remapped)
    {   return m_pShmHash->alloc2(size, remapped); }
    void release2(LsShmOffset_t offset, LsShmSize_t size)
    {   m_pShmHash->release2(offset, size); }

    void stat(LsHashStat &stat, udata_callback_fn _cb);

    int loop(udata_callback_fn _cb, void *pUParam);

    int lock()
    {   return m_pShmHash->lock(); };
    int unlock()
    {   return m_pShmHash->unlock(); };

private:
    LsShmCache(const LsShmCache &other);
    LsShmCache &operator=(const LsShmCache &other);
    bool operator==(const LsShmCache &other);

    lsShm_hCacheHdr_t *getCacheHeader()
    {
        return (lsShm_hCacheHdr_t *)(m_pShmHash->offset2ptr(m_iHdrOffset));
    }

    //
    //  @brief removeObjData
    //  @brief (1) call user callback if defined.
    //  @brief (2) remove the hash entry of the item.
    //
    void removeObjData(lsShm_hCacheData_t *pObj, void *pUParam = NULL);

    //
    //  @brief disconnectObj
    //  @brief remove obj the from queue
    //
    void disconnectObj(lsShm_hCacheData_t *pObj);

#ifdef LSSHM_DEBUG_ENABLE
    friend class debugBase;
#endif

    LsShmStatus_t   m_status;
    LsShmHash      *m_pShmHash;
    LsShmOffset_t   m_iHdrOffset;
    uint32_t        m_iMagic;
    LsShmSize_t     m_iDataSize;

    udata_init_fn   m_dataInit_cb;      // callback on datainit
    udata_init_fn   m_dataRemove_cb;    // callback before remove
};

#endif /* LSSHMCACHE_H */
