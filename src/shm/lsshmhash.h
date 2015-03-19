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
#ifndef LSSHMHASH_H
#define LSSHMHASH_H

#ifdef LSSHM_DEBUG_ENABLE
class debugBase;
#endif

#include <lsdef.h>
#include <shm/lsshm.h>
#include <shm/lsshmpool.h>
#include <util/ghash.h>
#include <lsr/ls_str.h>


/**
 * @file
 *  HASH element
 *
 *  +--------------------
 *  | Element info
 *  | hkey
 *  +--------------------  <--- start of variable data
 *  | key len
 *  | key
 *  +--------------------
 *  | LRU info (optional)
 *  +--------------------  <--- value offset
 *  | value len
 *  | value
 *  +--------------------
 *
 *  It is reasonable to support max 32k key string.
 *  Otherwise we should change to uint32_t
 */


typedef uint32_t     LsShmHKey;
typedef int32_t      LsShmHElemLen_t;
typedef uint32_t     LsShmHElemOffs_t;
typedef int32_t      LsShmHRkeyLen_t;
typedef int32_t      LsShmHValueLen_t;


typedef struct
{
    LsShmOffset_t        x_iLinkNext;     // offset to next in linked list
    LsShmOffset_t        x_iLinkPrev;     // offset to prev in linked list
    time_t               x_lasttime;      // last update time (sec)
} LsShmHElemLink;


typedef struct ls_vardata_s
{
    int32_t         x_size;
    uint8_t         x_data[0];
} ls_vardata_t;


typedef struct lsShm_hElem_s
{
    LsShmHElemLen_t      x_iLen;          // element size
    uint32_t             x_iValOff;
    LsShmOffset_t        x_iNext;         // offset to next in element list
    LsShmHKey            x_hkey;          // the key itself
    uint8_t              x_aData[0];

    uint8_t         *getKey() const
    { return ((ls_vardata_t *)x_aData)->x_data; }
    int32_t          getKeyLen() const
    { return ((ls_vardata_t *)x_aData)->x_size; }
    uint8_t         *getVal() const
    { return ((ls_vardata_t *)(x_aData + x_iValOff))->x_data; }
    int32_t          getValLen() const
    { return ((ls_vardata_t *)(x_aData + x_iValOff))->x_size; }
    void             setKeyLen(int32_t len)
    { ((ls_vardata_t *)x_aData)->x_size = len; }
    void             setValLen(int32_t len)
    { ((ls_vardata_t *)(x_aData + x_iValOff))->x_size = len; }

    LsShmHElemLink  *getLruLinkPtr() const
    { return ((LsShmHElemLink *)(x_aData + x_iValOff) - 1); }
    LsShmOffset_t    getLruLinkNext() const
    { return getLruLinkPtr()->x_iLinkNext; }
    LsShmOffset_t    getLruLinkPrev() const
    { return getLruLinkPtr()->x_iLinkPrev; }
    time_t           getLruLasttime() const
    { return getLruLinkPtr()->x_lasttime; }
    void             setLruLinkNext(LsShmOffset_t off)
    { getLruLinkPtr()->x_iLinkNext = off; }
    void             setLruLinkPrev(LsShmOffset_t off)
    { getLruLinkPtr()->x_iLinkPrev = off; }

    const uint8_t   *first() const   { return getKey(); }
    uint8_t         *second() const  { return getVal(); }

    // real value len
    int32_t          realValLen() const
    {
        return (x_iLen - sizeof(struct lsShm_hElem_s) - x_iValOff - sizeof(ls_vardata_t));
    }
} LsShmHElem;

typedef struct
{
    LsShmOffset_t   x_iOffset;   // offset location of hash element
} LsShmHIdx;

typedef struct
{
    uint32_t        x_iMagic;
    LsShmSize_t     x_iCapacity;
    LsShmSize_t     x_iSize;
    LsShmSize_t     x_iFullFactor;
    LsShmSize_t     x_iGrowFactor;
    LsShmOffset_t   x_iTable;
    LsShmOffset_t   x_iBitMap;
    LsShmOffset_t   x_iLockOffset;
    LsShmOffset_t   x_iGLockOffset;
    LsShmOffset_t   x_iLruOffset;
    uint8_t         x_iMode;
    uint8_t         x_iLruMode; // lru wlru=1, xlru=2
    uint8_t         x_unused[2];
} LsShmHTable;

class LsShmHash : public ls_shmhash_s, ls_shmobject_s
{
public:
    typedef LsShmHElem *iterator;
    typedef const LsShmHElem *const_iterator;
    typedef LsShmOffset_t iteroffset;

    typedef LsShmHKey(*hash_fn)(const void *pVal, int len);
    typedef int (*val_comp)(const void *pVal1, const void *pVal2, int len);
    typedef int (*for_each_fn)(iteroffset iterOff);
    typedef int (*for_each_fn2)(iteroffset iterOff, void *pUData);

    static LsShmHKey hashString(const void *__s, int len);
    static int compString(const void *pVal1, const void *pVal2, int len);

    static LsShmHKey hash32id(const void *__s, int len);
    static LsShmHKey hashBuf(const void *__s, int len);
    static LsShmHKey hashXXH32(const void *__s, int len);
    static int compBuf(const void *pVal1, const void *pVal2, int len);

    static LsShmHKey iHashString(const void *__s, int len);
    static int iCompString(const void *pVal1, const void *pVal2, int len);

    static int cmpIpv6(const void *pVal1, const void *pVal2, int len);
    static LsShmHKey hfIpv6(const void *pKey, int len);

    LsShmStatus_t status()
    { return m_status; }

public:
    LsShmHash(LsShmPool *pool,
              const char *name, size_t init_size, hash_fn hf, val_comp vc);
    virtual ~LsShmHash();

    static LsShmHash *checkHTable(GHash::iterator itor, LsShmPool *pool,
                                  const char *name, LsShmHash::hash_fn hf, LsShmHash::val_comp vc);

    LsShmPool *getPool() const
    {   return m_pPool;     }

    ls_attr_inline LsShmOffset_t ptr2offset(const void *ptr) const
    {   return m_pPool->ptr2offset(ptr); }

    ls_attr_inline void *offset2ptr(LsShmOffset_t offset) const
    {   return m_pPool->offset2ptr(offset); }

    ls_attr_inline iterator offset2iterator(iteroffset offset) const
    {   return (iterator)m_pPool->offset2ptr(offset); }

    ls_attr_inline void *offset2iteratorData(iteroffset offset) const
    {   return ((iterator)m_pPool->offset2ptr(offset))->getVal(); }

    LsShmOffset_t alloc2(LsShmSize_t size, int &remapped)
    {   return m_pPool->alloc2(size, remapped); }

    void release2(LsShmOffset_t offset, LsShmSize_t size)
    {   m_pPool->release2(offset, size); }

    int round4(int x) const
    {   return (x + 0x3) & ~0x3; }

    void *getIterDataPtr(iterator iter) const
    {   return iter->getVal(); }

    const char *name() const
    {   return m_pName; }

    ls_attr_inline void checkRemap()
    {
        if (m_pShmMap != m_pPool->getShmMap())
            remap(); 
    }

    void close();

    void clear();

    ls_str_pair_t *setParms(ls_str_pair_t *pParms,
        const void *pKey, int keyLen, const void *pValue, int valueLen)
    {
        ls_str_unsafeset(&pParms->key, (char *)pKey, keyLen);
        ls_str_unsafeset(&pParms->value, (char *)pValue, valueLen);
        return pParms;
    }

    void remove(const void *pKey, int keyLen)
    {
        iteroffset iterOff;
        ls_str_pair_t parms;
        ls_str_unsafeset(&parms.key, (char *)pKey, keyLen);
        if ((iterOff = findIterator(&parms)) != 0)
            eraseIterator(iterOff);
    }

    LsShmOffset_t find(const void *pKey, int keyLen, int *valLen)
    {
        iteroffset iterOff;
        ls_str_pair_t parms;
        ls_str_unsafeset(&parms.key, (char *)pKey, keyLen);
        if ((iterOff = findIterator(&parms)) == 0)
        {
            *valLen = 0;
            return 0;
        }
        iterator iter = offset2iterator(iterOff);
        *valLen = iter->getValLen();
        return ptr2offset(iter->getVal());
    }

    LsShmOffset_t get(
        const void *pKey, int keyLen, int *valLen, int *pFlag)
    {
        iteroffset iterOff;
        ls_str_pair_t parms;
        if ((iterOff = getIterator(
            setParms(&parms, pKey, keyLen, NULL, *valLen), pFlag)) == 0)
        {
            *valLen = 0;
            return 0;
        }
        iterator iter = offset2iterator(iterOff);
        *valLen = iter->getValLen();
        return ptr2offset(iter->getVal());
    }

    LsShmOffset_t insert(
        const void *pKey, int keyLen, const void *pValue, int valueLen)
    {
        ls_str_pair_t parms;
        iteroffset iterOff = insertIterator(
            setParms(&parms, pKey, keyLen, pValue, valueLen));
        return (iterOff == 0) ?
            0 : ptr2offset(offset2iteratorData(iterOff));
    }

    LsShmOffset_t set(
        const void *pKey, int keyLen, const void *pValue, int valueLen)
    {
        ls_str_pair_t parms;
        iteroffset iterOff = setIterator(
            setParms(&parms, pKey, keyLen, pValue, valueLen));
        return (iterOff == 0) ?
            0 : ptr2offset(offset2iteratorData(iterOff));
    }

    LsShmOffset_t update(
        const void *pKey, int keyLen, const void *pValue, int valueLen)
    {
        ls_str_pair_t parms;
        iteroffset iterOff = updateIterator(
            setParms(&parms, pKey, keyLen, pValue, valueLen));
        return (iterOff == 0) ?
            0 : ptr2offset(offset2iteratorData(iterOff));
    }

    void eraseIterator(iteroffset iterOff)
    {
        autoLock();
        checkRemap();
        eraseIteratorHelper(offset2iterator(iterOff));
        autoUnlock();
    }

    //
    //  Note - iterators should not be saved.
    //         use ptr2offset(iterator) to save the offset
    //
    iteroffset findIterator(ls_str_pair_t *pParms)
    {
        autoLock();
        checkRemap();
        iteroffset iterOff = (*m_find)(this, pParms);
        autoUnlock();
        return iterOff;
    }

    iteroffset getIterator(ls_str_pair_t *pParms, int *pFlag)
    {
        autoLock();
        checkRemap();
        iteroffset iterOff = (*m_get)(this, pParms, pFlag);
        autoUnlock();
        return iterOff;
    }

    iteroffset insertIterator(ls_str_pair_t *pParms)
    {
        autoLock();
        checkRemap();
        iteroffset iterOff = (*m_insert)(this, pParms);
        autoUnlock();
        return iterOff;
    }

    iteroffset setIterator(ls_str_pair_t *pParms)
    {
        autoLock();
        checkRemap();
        iteroffset iterOff = (*m_set)(this, pParms);
        autoUnlock();
        return iterOff;
    }

    iteroffset updateIterator(ls_str_pair_t *pParms)
    {
        autoLock();
        checkRemap();
        iteroffset iterOff = (*m_update)(this, pParms);
        autoUnlock();
        return iterOff;
    }

    hash_fn getHashFn() const   {   return m_hf;    }
    val_comp getValComp() const {   return m_vc;    }

    void setFullFactor(int f);
    void setGrowFactor(int f);

    
    
    bool empty() const
    {   
        return getHTable()->x_iSize == 0; 
    }

    size_t size() const             
    {   
        return getHTable()->x_iSize;      
    }
    size_t capacity() const         
    {   
        return getHTable()->x_iCapacity;  
    }
    LsShmOffset_t lruHdrOff() const 
    {   
        return getHTable()->x_iLruOffset; 
    }

    iteroffset begin();
    iteroffset end()                {   return 0;   }
    iteroffset next(iteroffset iterOff);
    int for_each(iteroffset beg, iteroffset end, for_each_fn fun);
    int for_each2(iteroffset beg, iteroffset end, for_each_fn2 fun, void *pUData);

    void destroy();

    //  @brief stat
    //  @brief gether statistic on HashTable
    int stat(LsHashStat *pHashStat, for_each_fn2 fun);

    // Special section for registries entries
#ifdef notdef
    LsShmReg *getReg(int num)
    {   return m_pool->getReg(num); }
#endif

    LsShmReg *findReg(const char *name)
    {   return m_pPool->findReg(name); }

    LsShmReg *addReg(const char *name)
    {   return m_pPool->addReg(name); }



//     void enableManualLock()
//     {   m_pPool->disableLock(); disableLock(); }
//     void disableManualLock()
//     {   m_pPool->enableLock(); enableLock(); }

    void enableLock()
    {   m_iLockEnable = 1; };

    void disableLock()
    {   m_iLockEnable = 0; };

    int lock()
    {   return m_iLockEnable ? 0 : lsi_shmlock_lock(m_pShmLock); }

    int trylock()
    {   return m_iLockEnable ? 0 : lsi_shmlock_trylock(m_pShmLock); }

    int unlock()
    {   return m_iLockEnable ? 0 : lsi_shmlock_unlock(m_pShmLock); }

    int getRef()     { return m_iRef; }
    int upRef()      { return ++m_iRef; }
    int downRef()    { return --m_iRef; }

protected:
    typedef iteroffset (*hash_find)(LsShmHash *pThis, ls_str_pair_t *pParms);
    typedef iteroffset (*hash_get)(LsShmHash *pThis, ls_str_pair_t *pParms, int *pFlag);
    typedef iteroffset (*hash_insert)(LsShmHash *pThis, ls_str_pair_t *pParms);
    typedef iteroffset (*hash_set)(LsShmHash *pThis, ls_str_pair_t *pParms);
    typedef iteroffset (*hash_update)(LsShmHash *pThis, ls_str_pair_t *pParms);

    uint32_t getIndex(uint32_t k, uint32_t n)
    {   return k % n ; }

    int getBitMapEnt(uint32_t indx)
    {
        return m_pBitMap[indx / s_bitsPerChar] & s_bitMask[indx % s_bitsPerChar];
    }
    void setBitMapEnt(uint32_t indx)
    {
        m_pBitMap[indx / s_bitsPerChar] |= s_bitMask[indx % s_bitsPerChar];
    }
    void clrBitMapEnt(uint32_t indx)
    {
        m_pBitMap[indx / s_bitsPerChar] &= ~(s_bitMask[indx % s_bitsPerChar]);
    }

    int sz2TableSz(LsShmSize_t sz)
    {   return (sz * sizeof(LsShmHIdx)); }

    int sz2BitMapSz(LsShmSize_t sz)
    {   return ((sz + s_bitsPerChar - 1) / s_bitsPerChar); }

    int         rehash();
    iteroffset  find2(LsShmHKey key, ls_str_pair_t *pParms);
    iteroffset  insert2(LsShmHKey key, ls_str_pair_t *pParms);

    static iteroffset findNum(LsShmHash *pThis, ls_str_pair_t *pParms);
    static iteroffset getNum(LsShmHash *pThis, ls_str_pair_t *pParms, int *pFlag);
    static iteroffset insertNum(LsShmHash *pThis, ls_str_pair_t *pParms);
    static iteroffset setNum(LsShmHash *pThis, ls_str_pair_t *pParms);
    static iteroffset updateNum(LsShmHash *pThis, ls_str_pair_t *pParms);

    static iteroffset findPtr(LsShmHash *pThis, ls_str_pair_t *pParms);
    static iteroffset getPtr(LsShmHash *pThis, ls_str_pair_t *pParms, int *pFlag);
    static iteroffset insertPtr(LsShmHash *pThis,ls_str_pair_t *pParms);
    static iteroffset setPtr(LsShmHash *pThis, ls_str_pair_t *pParms);
    static iteroffset updatePtr(LsShmHash *pThis, ls_str_pair_t *pParms);

    //
    //  @brief eraseIterator_helper
    //  @brief  should only be called after SHM-HASH-LOCK has been acquired.
    //
    void eraseIteratorHelper(iterator iter);
    
    ls_attr_inline LsShmHTable * getHTable() const
    {   return (LsShmHTable *)m_pPool->offset2ptr(m_iOffset);   }
    
    static inline iteroffset doGet(
        LsShmHash *pThis, iteroffset iterOff, LsShmHKey key, ls_str_pair_t *pParms, int *pFlag)
    {
        if (iterOff != 0)
        {
            iterator iter = pThis->offset2iterator(iterOff);
            pThis->linkSetTop(iter);
            *pFlag = LSSHM_FLAG_NONE;
            return iterOff;
        }
        LSSHM_CHECKSIZE(ls_str_len(&pParms->value));
        iterOff = pThis->insert2(key, pParms);
        if (iterOff != 0)
        {
            if (*pFlag & LSSHM_FLAG_INIT)
            {
                // initialize the memory
                ::memset(pThis->offset2iteratorData(iterOff),
                         0, ls_str_len(&pParms->value));
            }
            *pFlag = LSSHM_FLAG_CREATED;
            pThis->lruSpecial(iterOff);
        }
        return iterOff;
    }

    static inline iteroffset doInsert(
        LsShmHash *pThis, iteroffset iterOff, LsShmHKey key, ls_str_pair_t *pParms)
    {
        if (iterOff != 0)
            return 0;
        LSSHM_CHECKSIZE(ls_str_len(&pParms->value));
        return pThis->insert2(key, pParms);
    }
    
    static inline iteroffset doSet(
        LsShmHash *pThis, iteroffset iterOff, LsShmHKey key, ls_str_pair_t *pParms)
    {
        LSSHM_CHECKSIZE(ls_str_len(&pParms->value));
        if (iterOff != 0)
        {
            iterator iter = pThis->offset2iterator(iterOff);
            if (iter->realValLen() >= ls_str_len(&pParms->value))
            {
                iter->setValLen(ls_str_len(&pParms->value));
                pThis->setIterData(iter, ls_str_buf(&pParms->value));
                pThis->linkSetTop(iter);
                return iterOff;
            }
            else
            {
                // remove the iter and install new one
                pThis->eraseIteratorHelper(iter);
            }
        }
        return pThis->insert2(key, pParms);
    }
    
    static inline iteroffset doUpdate(
        LsShmHash *pThis, iteroffset iterOff, LsShmHKey key, ls_str_pair_t *pParms)
    {
        if (iterOff == 0)
            return 0;
        LSSHM_CHECKSIZE(ls_str_len(&pParms->value));
        iterator iter = pThis->offset2iterator(iterOff);
        if (iter->realValLen() >= ls_str_len(&pParms->value))
        {
            iter->setValLen(ls_str_len(&pParms->value));
            pThis->setIterData(iter, ls_str_buf(&pParms->value));
            pThis->linkSetTop(iter);
        }
        else
        {
            // remove the iter and install new one
            pThis->eraseIteratorHelper(iter);
            iterOff = pThis->insert2(key, pParms);
        }
        return iterOff;
    }
    
    void setIterData(iterator iter, const void *pValue)
    {
        if (pValue != NULL)
            ::memcpy(iter->getVal(), pValue, iter->getValLen());
    }

    void setIterKey(iterator iter, const void *pKey)
    {   ::memcpy(iter->getKey(), pKey, iter->getKeyLen()); }

    void remap();
    static int release_hash_elem(iteroffset iterOff, void *pUData);

    int autoLock()
    {   return m_iLockEnable && lsi_shmlock_lock(m_pShmLock); }

    int autoUnlock()
    {   return m_iLockEnable && lsi_shmlock_unlock(m_pShmLock); }

    // stat helper
    int statIdx(iteroffset iterOff, for_each_fn2 fun, void *pUData);

    int setupLock()
    {   return m_iLockEnable && lsi_shmlock_setup(m_pShmLock); }

    int setupGLock()
    {   return lsi_shmlock_setup(m_pShmGLock); }

    virtual void valueSetup(uint32_t *pValOff, int *pValLen)
    {   return; }
    virtual void linkHElem(LsShmHElem *pElem, LsShmOffset_t offElem)
    {   return; }
    virtual void unlinkHElem(LsShmHElem *pElem)
    {   return; }
    virtual void linkSetTop(LsShmHElem *pElem)
    {   return; }
    virtual void lruSpecial(iteroffset iterOff)
    {   return; }

protected:
    uint32_t            m_iMagic;
    LsShmHTable        *m_pTable;
    LsShmHIdx          *m_pIdxStart;
    LsShmHIdx          *m_pIdxEnd;
    uint8_t            *m_pBitMap;
    LsHashLruInfo      *m_pLru;

    LsShmPool          *m_pPool;
    LsShmOffset_t       m_iOffset;
    char               *m_pName;

    hash_fn             m_hf;
    val_comp            m_vc;
    hash_insert         m_insert;
    hash_update         m_update;
    hash_set            m_set;
    hash_find           m_find;
    hash_get            m_get;

    int8_t              m_iLockEnable;
    int8_t              m_iMode;        // mode 0=Num, 1=Ptr
    int8_t              m_notused;
    LsShmMap           *m_pShmMap;      // keep track of map
    lsi_shmlock_t      *m_pShmLock;     // local lock for Hash
    lsi_shmlock_t      *m_pShmGLock;    // global lock
    LsShmStatus_t       m_status;
    LsShmSize_t         m_iCapacity;

    // house keeping
    int m_iRef;

    static const uint8_t s_bitMask[];
    static const size_t s_bitsPerChar;

private:
    // disable the bad boys!
    LsShmHash(const LsShmHash &other);
    LsShmHash &operator=(const LsShmHash &other);

    void initErrCleanUp();

#ifdef LSSHM_DEBUG_ENABLE
    // for debug purpose - should debug this later
    friend class debugBase;
#endif
};


template< class T >
class TShmHash
    : public LsShmHash
{
private:
    TShmHash() {};
    TShmHash(const TShmHash &rhs);
    ~TShmHash() {};
    void operator=(const TShmHash &rhs);
public:
    class iterator
    {
        LsShmHash::iterator m_iter;
    public:
        iterator()
        {}

        iterator(LsShmHash::iterator iter) : m_iter(iter)
        {}
        iterator(LsShmHash::const_iterator iter)
            : m_iter((LsShmHash::iterator)iter)
        {}

        iterator(const iterator &rhs) : m_iter(rhs.m_iter)
        {}

        const void *first() const
        {  return  m_iter->first();   }

        T *second() const
        {   return (T *)(m_iter->second());   }

        operator LsShmHash::iterator()
        {   return m_iter;  }
    };
    typedef iterator const_iterator;

    LsShmOffset_t get(const void *pKey, int keyLen, int *valueLen, int *pFlag)
    {
        *valueLen = sizeof(T);
        return LsShmHash::get(pKey, keyLen, valueLen, pFlag);
    }

    LsShmOffset_t set(const void *pKey, int keyLen, T *pValue)
    {   return LsShmHash::set(pKey, keyLen, (const void *)pValue, sizeof(T));  }

    LsShmOffset_t insert(const void *pKey, int keyLen, T *pValue)
    {   return LsShmHash::insert(pKey, keyLen, (const void *)pValue, sizeof(T));  }

    LsShmOffset_t update(const void *pKey, int keyLen, T *pValue)
    {   return LsShmHash::update(pKey, keyLen, (const void *)pValue, sizeof(T));  }

    iteroffset begin()
    {   return LsShmHash::begin();  }

};

#endif // LSSHMHASH_H

