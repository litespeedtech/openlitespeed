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
#include <shm/lsshmhash.h>

#include <lsr/xxhash.h>
#include <shm/lsshmpool.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct ls_shmhtable_s
{
    uint32_t        x_iMagic;
    LsShmSize_t     x_iCapacity;
    LsShmSize_t     x_iSize;
    LsShmSize_t     x_iFullFactor;
    LsShmSize_t     x_iGrowFactor;
    LsShmOffset_t   x_iHIdx;
    LsShmOffset_t   x_iBitMap;
    LsShmOffset_t   x_iHIdxNew; // in rehash()
    LsShmSize_t     x_iCapacityNew;
    LsShmOffset_t   x_iWorkIterOff;
    LsShmOffset_t   x_iBitMapSz;
    LsShmOffset_t   x_iLockOffset;
    uint8_t         x_iMode;
    uint8_t         x_iLruMode; // lru=1, wlru=2, xlru=3
    uint8_t         x_unused[2];
    LsShmHTableStat x_stat;         // hash statistics
    uint8_t         x_reserved[256];
} LsShmHTable;


#define x_lruInfo   x_reserved[sizeof(((LsShmHTable *)0)->x_reserved) \
                        - sizeof(LsHashLruInfo)]



const uint8_t s_bitMask[] =
{
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};
const size_t s_bitsPerChar =
    sizeof(s_bitMask) / sizeof(s_bitMask[0]);

// minimum element count for bloom bitmap (approx 1Mb table size)
#define MINSZ_FOR_BITMAP    (1024*1024/sizeof(LsShmHIdx))

const size_t s_bitsPerLsShmHIdx =
    s_bitsPerChar * sizeof(LsShmHIdx);


enum { prime_count = 31    };
static const LsShmSize_t s_primeList[prime_count] =
{
    7ul,          13ul,         29ul,
    53ul,         97ul,         193ul,       389ul,       769ul,
    1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
    49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
    1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
    50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
    1610612741ul, 3221225473ul, 4294967291ul
};


static int findRange(LsShmSize_t sz)
{
    int i = 1;
    for (; i < prime_count - 1; ++i)
    {
        if (sz <= s_primeList[i])
            break;
    }
    return i;
}

    static int sz2BitMapSz(LsShmSize_t sz)
    {
        return ((sz < MINSZ_FOR_BITMAP) ? 0 :
            (((sz + s_bitsPerLsShmHIdx - 1) / s_bitsPerLsShmHIdx)
            * sizeof(LsShmHIdx)));
    }


LsShmSize_t LsShmHash::roundUp(LsShmSize_t sz)
{
    return s_primeList[findRange(sz)];
}


// Hash for 32bytes session id
LsShmHKey LsShmHash::hash32id(const void *__s, size_t len)
{
    const uint32_t *lp = (const uint32_t *)__s;
    LsShmHKey __h = 0;

    if (len >= 8)
        __h = *lp ^ *(lp + 1);
    else
    {
        while (len >= 4)
        {
            __h ^= *lp++;
            len -= 4;
        }
        // ignore the leftover!
        // if (len)
    }
    return __h;
}


LsShmHKey LsShmHash::hashBuf(const void *__s, size_t len)
{
    LsShmHKey __h = 0;
    const uint8_t *p = (const uint8_t *)__s;
    const uint8_t *pEnd = p + len;
    uint8_t ch;

    // we will need a better hash key generator for buf key
    while (p < pEnd)
    {
        ch = *(const uint8_t *)p++;
        __h = __h * 31 + (ch);
    }
    return __h;
}


LsShmHKey LsShmHash::hashXXH32(const void *__s, size_t len)
{
    return XXH32(__s, len, 0);
}


LsShmHKey LsShmHash::hashString(const void *__s, size_t len)
{
    LsShmHKey __h = 0;
    const char *p = (const char *)__s;
    char ch = *(const char *)p++;
    for (; ch ; ch = *((const char *)p++))
        __h = __h * 31 + (ch);

    return __h;
}


int LsShmHash::compString(const void *pVal1, const void *pVal2, size_t len)
{
    return strcmp((const char *)pVal1, (const char *)pVal2);
}


LsShmHKey LsShmHash::iHashString(const void *__s, size_t len)
{
    LsShmHKey __h = 0;
    const char *p = (const char *)__s;
    char ch = *(const char *)p++;
    for (; ch ; ch = *((const char *)p++))
    {
        if (ch >= 'A' && ch <= 'Z')
            ch += 'a' - 'A';
        __h = __h * 31 + (ch);
    }
    return __h;
}


int LsShmHash::iCompString(const void *pVal1, const void *pVal2, size_t len)
{
    return strncasecmp(
               (const char *)pVal1, (const char *)pVal2, strlen((const char *)pVal1));
}


LsShmHKey LsShmHash::hfIpv6(const void *pKey, size_t len)
{
    LsShmHKey key;
    if (sizeof(LsShmHKey) == 4)
    {
        key = *((const LsShmHKey *)pKey) +
              *(((const LsShmHKey *)pKey) + 1) +
              *(((const LsShmHKey *)pKey) + 2) +
              *(((const LsShmHKey *)pKey) + 3);
    }
    else
    {
        key = *((const LsShmHKey *)pKey) +
              *(((const LsShmHKey *)pKey) + 1);
    }
    return key;
}


int LsShmHash::cmpIpv6(const void *pVal1, const void *pVal2, size_t len)
{
    return memcmp(pVal1, pVal2, 16);
}


inline LsShmSize_t LsShmHash::fullFactor() const
{   return getHTable()->x_iFullFactor;   }


inline LsShmSize_t LsShmHash::growFactor() const
{   return getHTable()->x_iGrowFactor;   }


inline LsShmHIdx *LsShmHash::getHIdx() const
{   return (LsShmHIdx *)m_pPool->offset2ptr(getHTable()->x_iHIdx);   }


inline uint8_t *LsShmHash::getBitMap() const
{
    LsShmHTable *pTable = getHTable();
    if (pTable->x_iBitMapSz == 0)
        return NULL;
    return (uint8_t *)m_pPool->offset2ptr(pTable->x_iBitMap);
}


void LsShmHash::lockChkRehash()
{
    if ((lock() < 0) && (getHTable()->x_iHIdx != getHTable()->x_iHIdxNew))
        rehash();
}


void LsShmHash::autoLockChkRehash()
{
    if ((autoLock() < 0) && (getHTable()->x_iHIdx != getHTable()->x_iHIdxNew))
        rehash();
}


LsHashLruInfo *LsShmHash::getLru() const
{   return (LsHashLruInfo *)&(getHTable()->x_lruInfo);  }


LsShmOffset_t LsShmHash::alloc2(LsShmSize_t size, int &remapped)
{
    LsShmOffset_t ret = m_pPool->alloc2(size, remapped);
    if (ret != 0)
        getHTable()->x_stat.m_iHashInUse += LsShmPool::size2roundSize(size);
    return ret;
}


void LsShmHash::release2(LsShmOffset_t offset, LsShmSize_t size)
{
    m_pPool->release2(offset, size);
    getHTable()->x_stat.m_iHashInUse -= LsShmPool::size2roundSize(size);
}


bool LsShmHash::empty() const
{
    return getHTable()->x_iSize == 0;
}


LsShmSize_t LsShmHash::size() const
{
    return getHTable()->x_iSize;
}


void LsShmHash::LsShmHash::incrTableSize()
{
    ++(getHTable()->x_iSize);
}


void LsShmHash::decrTableSize()
{
    --(getHTable()->x_iSize);
}


LsShmSize_t LsShmHash::capacity() const
{
    return getHTable()->x_iCapacity;
}


inline int LsShmHash::getBitMapEnt(uint32_t indx)
{
    uint8_t *pBitMap = getBitMap();
    return ((pBitMap == NULL) ? 1 :
        pBitMap[indx / s_bitsPerChar] & s_bitMask[indx % s_bitsPerChar]);
}


inline void LsShmHash::setBitMapEnt(uint32_t indx)
{
    uint8_t *pBitMap;
    if ((pBitMap = getBitMap()) != NULL)
        pBitMap[indx / s_bitsPerChar] |= s_bitMask[indx % s_bitsPerChar];
}


inline void LsShmHash::clrBitMapEnt(uint32_t indx)
{
    uint8_t *pBitMap;
    if ((pBitMap = getBitMap()) != NULL)
        pBitMap[indx / s_bitsPerChar] &= ~(s_bitMask[indx % s_bitsPerChar]);
}


LsShmOffset_t LsShmHash::getHTableStatOffset() const
{   return (LsShmOffset_t)(long) & ((LsShmHTable *)(long)m_iOffset)->x_stat; }

LsShmOffset_t LsShmHash::getHTableReservedOffset() const
{   return (LsShmOffset_t)(long) & ((LsShmHTable *)(long)m_iOffset)->x_reserved; }


int LsShmHash::chkHashTable(LsShm *pShm, LsShmReg *pReg, int *pMode, int *pLruMode)
{
    if (pReg->x_iValue == 0)
        return -1;
    LsShmHTable *pTable = (LsShmHTable *)pShm->offset2ptr(pReg->x_iValue);
    if (pTable->x_iMagic != LSSHM_HASH_MAGIC)
        return -1;
    *pMode = pTable->x_iMode;
    *pLruMode = pTable->x_iLruMode;
    return 0;
}


LsShmHash::LsShmHash(LsShmPool *pool, const char *name,
                     LsShmHasher_fn hf, LsShmValComp_fn vc, int lru_mode)
    : m_iMagic(LSSHM_HASH_MAGIC)
    , m_pPool(pool)
    , m_iOffset(0)
    , m_hf(hf)
    , m_vc(vc)
    , m_iterExtraSpace(0)
    , m_dataExtraSpace(0)
    , m_iLruMode(lru_mode)
    , m_pLruAddon(NULL)
{
    obj.m_pName = strdup(name);
    m_iRef = 0;
    m_status = LSSHM_NOTREADY;
    m_pShmLock = NULL;
    m_iLockEnable = 1;      // enableLock()

    if (m_hf != NULL)
    {
        assert(m_vc);
        m_insert = insertPtr;
        m_update = updatePtr;
        m_set = setPtr;
        m_find = findPtr;
        m_get = getPtr;
        m_iMode = 1;
    }
    else
    {
        m_insert = insertNum;
        m_update = updateNum;
        m_set = setNum;
        m_find = findNum;
        m_get = getNum;
        m_iMode = 0;
    }
}


LsShmOffset_t LsShmHash::allocHTable(LsShmPool * pPool, int init_size, 
                                     int iMode, int iLruMode, 
                                     LsShmOffset_t lockOffset)
{
    int remapped;
    // NOTE: system is not up yet... ignore remap here
    LsShmOffset_t offset = pPool->alloc2(sizeof(LsShmHTable), remapped);
    if (offset == 0)
    {
        return 0;
    }
    
    init_size = roundUp(init_size);
    int szTable = sz2TableSz(init_size);
    int szBitMap = sz2BitMapSz(init_size);
    LsShmOffset_t iBase = pPool->alloc2(szTable + szBitMap, remapped);

    if (iBase == 0)
    {
        pPool->release2(offset, sizeof(LsShmHTable));
        return 0;
    }
    LsShmHTable *pTable = (LsShmHTable *)pPool->offset2ptr(offset);

    ::memset(pTable, 0, sizeof(*pTable));
    ::memset(pPool->offset2ptr(iBase), 0, szTable + szBitMap);

    pTable->x_iMagic = LSSHM_HASH_MAGIC;
    pTable->x_iCapacity = init_size;
    pTable->x_iFullFactor = 2;
    pTable->x_iGrowFactor = 2;
    pTable->x_iBitMap = iBase;
    pTable->x_iHIdx = iBase + szBitMap;
    pTable->x_iHIdxNew = pTable->x_iHIdx;

    pTable->x_iBitMapSz = szBitMap;
    pTable->x_stat.m_iHashInUse = LsShmPool::size2roundSize(sizeof(LsShmHTable))
                                  + LsShmPool::size2roundSize(szTable + szBitMap);

    pTable->x_iLockOffset = lockOffset;

    pTable->x_iMode = iMode;
    pTable->x_iLruMode = iLruMode;
    
    return offset;
    
}


int LsShmHash::init(LsShmOffset_t offset)
{
    m_iOffset = offset;
    LsShmHTable *pTable = getHTable();

    // check the magic and mode
    if ((m_iMagic != pTable->x_iMagic)
        || (m_iMode != pTable->x_iMode)
        || (m_iLruMode != pTable->x_iLruMode))
        return LS_FAIL;
    m_pShmLock = m_pPool->lockPool()->offset2pLock(pTable->x_iLockOffset);

    if (m_iLruMode & LSSHM_LRU)
        m_iterExtraSpace = sizeof(LsShmHElemLink);
    //if ((m_iLruMode == LSSHM_LRU_MODE2) || (m_iLruMode == LSSHM_LRU_MODE3))
    //    m_iLockEnable = 0;  // shmlru routines lock manually on higher level
    m_iRef = 1;
    m_status = LSSHM_READY;
    return LS_OK;
}


LsShmHash::~LsShmHash()
{
    if (obj.m_pName != NULL)
    {
        if (m_iRef == 0)
        {
#ifdef DEBUG_RUN
            SHM_NOTICE("LsShmHash::~LsShmHash remove %s <%p>",
                       m_pName, &m_objBase);
#endif
            m_pPool->getShm()->getObjBase().remove(obj.m_pName);
        }
        free(obj.m_pName);
        obj.m_pName = NULL;
    }
}


LsShmHash *LsShmHash::open(
    const char *pShmName, const char *pHashName, int init_size, int lruMode)
{
    LsShm *pShm;
    LsShmPool *pPool = NULL;
    LsShmHash *pHash = NULL;
    int attempts;

    for (attempts = 0; attempts < 2; ++attempts)
    {
        if ((pShm = LsShm::open(pShmName, 0)) == NULL)
        {
            if ((pShm = LsShm::open(pShmName, 0)) == NULL)
                return NULL;
        }
        if ((pPool = pShm->getGlobalPool()) == NULL)
        {
            pShm->deleteFile();
            pShm->close();
            continue;
        }

        if ((pHash = pPool->getNamedHash(pHashName, init_size,
            LsShmHash::hashXXH32, memcmp, lruMode)) == NULL)
        {
            pPool->close();
            pShm->deleteFile();
            pShm->close();
        }
        else
            break;
    }
    return pHash;
}


LsShmHash *LsShmHash::checkHTable(GHash::iterator itor, LsShmPool *pool,
                                  const char *name, LsShmHasher_fn hf, LsShmValComp_fn vc)
{
    LsShmHash *pObj;
    if (((pObj = (LsShmHash *)(ls_shmhash_t *)itor->second()) == NULL)
        || (pObj->m_iMagic != LSSHM_HASH_MAGIC)
        || (pObj->m_hf != hf)
        || (pObj->m_vc != vc))
        return NULL;    // bad: parameters not matching

    if (pObj->m_pPool != pool)
        return (LsShmHash *) - 1; // special case: different pools
    pObj->upRef();
    return pObj;
}


void LsShmHash::setFullFactor(int f)
{
    if (f > 0)
        getHTable()->x_iFullFactor = f;
}


void LsShmHash::setGrowFactor(int f)
{
    if (f > 0)
        getHTable()->x_iGrowFactor = f;
}


void LsShmHash::releaseHTableShm()
{
    if (m_iOffset != 0)
    {
        LsShmHTable *pTable = getHTable();
        if (pTable->x_iBitMap != 0)
        {
            m_pPool->release2(pTable->x_iBitMap,
                pTable->x_iBitMapSz + sz2TableSz(pTable->x_iCapacity));
        }
        if (m_pShmLock != NULL)
        {
            m_pPool->lockPool()->freeLock(m_pShmLock);
            m_pShmLock = NULL;
        }
        m_pPool->release2(m_iOffset, sizeof(LsShmHTable));
        m_iOffset = 0;
    }
}


void LsShmHash::close()
{
//     LsShmPool *p = NULL;
//     if (m_iPoolOwner != 0)
//     {
//         m_iPoolOwner = 0;
//         p = m_pPool;
//     }
    if (downRef() == 0)
        delete this;
//     if (p != NULL)
//         p->close();
}


//
//  The only way to remove the Shared Memory
//
void LsShmHash::destroy()
{
    if (m_iOffset != 0)
    {
        // remove from regMap
        m_pPool->getShm()->delReg(obj.m_pName);

        // all elements
        clear();

        releaseHTableShm();
    }
}


int LsShmHash::rehash()
{
    LsShmSize_t oldSize = capacity();
    LsShmSize_t newSize;;
    LsShmOffset_t newIdxOff;
    LsShmOffset_t newBitOff;
    LsShmHIdx *pIdxOld;
    LsShmHIdx *pIdxNew;
    LsShmHIdx *opIdx;
    LsShmHIdx *npIdx;
    iterator iter;
    iteroffset iterOff;
    iteroffset iterNextOff;
    int szTable;
    int szBitMap;
#ifdef DEBUG_RUN
    SHM_NOTICE("LsShmHash::rehash %6d %X size %d cap %d NEW %d",
               getpid(), m_pPool->getShmMap(),
               size(),
               oldSize,
               s_primeList[findRange(oldSize) + growFactor()]
              );
#endif
    LsShmHTable *pTable = getHTable();
    pIdxOld = (LsShmHIdx *)m_pPool->offset2ptr(pTable->x_iHIdx);
    if (pTable->x_iHIdx != pTable->x_iHIdxNew)          // rehash in progress
    {
        newSize = pTable->x_iCapacityNew;
        newIdxOff = pTable->x_iHIdxNew;
        pIdxNew = (LsShmHIdx *)m_pPool->offset2ptr(newIdxOff);
        if ((iterOff = pTable->x_iWorkIterOff) != 0)    // iter in progress
        {
            iter = offset2iterator(iterOff);
            npIdx = pIdxNew + getIndex(iter->x_hkey, newSize);
            if (npIdx->x_iOffset != iterOff)            // not there yet
            {
                opIdx = pIdxOld + getIndex(iter->x_hkey, oldSize);
                if (opIdx->x_iOffset == iterOff)
                    opIdx->x_iOffset = iter->x_iNext;   // remove from old
                iter->x_iNext = npIdx->x_iOffset;
                npIdx->x_iOffset = iterOff;
            }

        }
    }
    else
    {
        int remapped;
        newSize = s_primeList[findRange(oldSize) + growFactor()];
        szTable = sz2TableSz(newSize);
        szBitMap = sz2BitMapSz(newSize);
        if ((newBitOff = alloc2(szTable + szBitMap, remapped)) == 0)
            return LS_FAIL;
        uint8_t *ptr = (uint8_t *)offset2ptr(newBitOff);
        ::memset(ptr, 0, szTable + szBitMap);
        newIdxOff = newBitOff + szBitMap;
        pIdxNew = (LsShmHIdx *)(ptr + szBitMap);
        pTable = getHTable();
        pTable->x_iBitMap = newBitOff;
        pTable->x_iBitMapSz = szBitMap;
        pTable->x_iCapacityNew = newSize;
        pTable->x_iHIdxNew = newIdxOff;
    }

    pIdxOld = (LsShmHIdx *)m_pPool->offset2ptr(pTable->x_iHIdx);
    for (iterOff = begin(); iterOff != end();)
    {
        uint32_t hashIndx;
        iter = offset2iterator(iterOff);
        iterNextOff = next(iterOff);
        hashIndx = getIndex(iter->x_hkey, newSize);
        npIdx = pIdxNew + hashIndx;
        setBitMapEnt(hashIndx);
        pTable->x_iWorkIterOff = iterOff;
        (pIdxOld + getIndex(iter->x_hkey, oldSize))->x_iOffset = iter->x_iNext;
        iter->x_iNext = npIdx->x_iOffset;
        npIdx->x_iOffset = iterOff;
        iterOff = iterNextOff;
    }
    pTable->x_iWorkIterOff = 0;

    szTable = sz2TableSz(oldSize);
    szBitMap = sz2BitMapSz(oldSize);
    release2(pTable->x_iHIdx - szBitMap, szTable + szBitMap);
    pTable->x_iCapacity = newSize;
    pTable->x_iHIdx = newIdxOff;
    return 0;
}


int LsShmHash::release_hash_elem(LsShmHash::iteroffset iterOff,
                                 void *pUData)
{
    LsShmHash *pThis = (LsShmHash *)pUData;
    LsShmHash::iterator iter = pThis->offset2iterator(iterOff);
    pThis->release2(iterOff, (LsShmSize_t)iter->x_iLen);
    return 0;
}


void LsShmHash::clear()
{
    LsShmHTable *pTable = getHTable();
    int n = for_each2(begin(), end(), release_hash_elem, this);
    assert(n == (int)size());

    ::memset(offset2ptr(pTable->x_iBitMap), 0,
        pTable->x_iBitMapSz + sz2TableSz(pTable->x_iCapacity));
    pTable->x_iSize = 0;
    if (m_iLruMode != LSSHM_LRU_NONE)
    {
        LsHashLruInfo *pLru = getLru();
        pLru->linkFirst = 0;
        pLru->linkLast = 0;
        pLru->nvalset = 0;
        pLru->ndataset = 0;
    }
}


LsShmHash::iteroffset LsShmHash::doGet(
    iteroffset iterOff, LsShmHKey key, ls_strpair_t *pParms,
    int *pFlag)
{
    if (iterOff != 0)
    {
        iterator iter = offset2iterator(iterOff);
        if (m_iLruMode != LSSHM_LRU_NONE)
            linkSetTop(iter);
        *pFlag = LSSHM_FLAG_NONE;
        return iterOff;
    }
    LSSHM_CHECKSIZE(ls_str_len(&pParms->value));
    iterOff = insert2(key, pParms);
    if (iterOff != 0)
    {
        if (*pFlag & LSSHM_FLAG_INIT)
        {
            // initialize the memory
            ::memset(offset2iteratorData(iterOff),
                        0, ls_str_len(&pParms->value));
        }
        // some special lru initialization
//         if (m_iLruMode == LSSHM_LRU_MODE2)
//         {
//             shmlru_data_t *pData = (shmlru_data_t *)offset2iteratorData(
//                                         iterOff);
//             pData->maxsize = 0;
//             pData->offiter = iterOff;
//         }
//         else if (m_iLruMode == LSSHM_LRU_MODE3)
//             ((shmlru_val_t *)offset2iteratorData(iterOff))->offdata = 0;
        *pFlag = LSSHM_FLAG_CREATED;
    }
    return iterOff;
}


LsShmHash::iteroffset LsShmHash::doInsert(
            iteroffset iterOff, LsShmHKey key, ls_strpair_t *pParms)
{
    if (iterOff != 0)
        return 0;
    LSSHM_CHECKSIZE(ls_str_len(&pParms->value));
    return insert2(key, pParms);
}

LsShmHash::iteroffset LsShmHash::doSet(
            iteroffset iterOff, LsShmHKey key, ls_strpair_t *pParms)
{
    LSSHM_CHECKSIZE(ls_str_len(&pParms->value));
    if (iterOff != 0)
    {
        iterator iter = offset2iterator(iterOff);
        if (iter->realValLen() >= (LsShmSize_t)ls_str_len(&pParms->value))
        {
            iter->setValLen(ls_str_len(&pParms->value));
            setIterData(iter, ls_str_buf(&pParms->value));
            if (m_iLruMode != LSSHM_LRU_NONE)
                linkSetTop(iter);
            return iterOff;
        }
        else
        {
            // remove the iter and install new one
            eraseIteratorHelper(iter);
        }
    }
    return insert2(key, pParms);
}

LsShmHash::iteroffset LsShmHash::doUpdate(iteroffset iterOff, LsShmHKey key, ls_strpair_t *pParms)
{
    if (iterOff == 0)
        return 0;
    LSSHM_CHECKSIZE(ls_str_len(&pParms->value));
    iterator iter = offset2iterator(iterOff);
    if (iter->realValLen() >= (LsShmSize_t)ls_str_len(&pParms->value))
    {
        iter->setValLen(ls_str_len(&pParms->value));
        setIterData(iter, ls_str_buf(&pParms->value));
        if (m_iLruMode != LSSHM_LRU_NONE)
            linkSetTop(iter);
    }
    else
    {
        // remove the iter and install new one
        eraseIteratorHelper(iter);
        iterOff = insert2(key, pParms);
    }
    return iterOff;
}


//
// @brief erase - remove iter from the SHM pool.
// @brief will destroy the link to itself if any!
//
void LsShmHash::eraseIteratorHelper(iterator iter)
{
    if (iter == NULL)
        return;

    iteroffset iterOff = m_pPool->ptr2offset(iter);
    uint32_t hashIndx = getIndex(iter->x_hkey, capacity());
    LsShmHIdx *pIdx = getHIdx() + hashIndx;
    LsShmOffset_t offset = pIdx->x_iOffset;
    LsShmHElem *pElem;

#ifdef DEBUG_RUN
    if (offset == 0)
    {
        SHM_NOTICE(
            "LsShmHash::eraseIteratorHelper %6d %X size %d cap %d",
            getpid(), m_pPool->getShmMap(),
            size(),
            capacity()
        );
        sleep(10);
    }
#endif

    if (m_iLruMode != LSSHM_LRU_NONE)
        unlinkHElem(iter);
    if (offset == iterOff)
    {
        if ((pIdx->x_iOffset = iter->x_iNext) == 0) // last one
            clrBitMapEnt(hashIndx);
    }
    else
    {
        do
        {
            pElem = (LsShmHElem *)m_pPool->offset2ptr(offset);
            if (pElem->x_iNext == iterOff)
            {
                pElem->x_iNext = iter->x_iNext;
                break;
            }
            // next offset...
            offset = pElem->x_iNext;
        }
        while (offset != 0);
    }

    release2(iterOff, (LsShmSize_t)iter->x_iLen);
    decrTableSize();
}


LsShmHash::iteroffset LsShmHash::find2(LsShmHKey key,
                                       ls_strpair_t *pParms)
{
    uint32_t hashIndx = getIndex(key, capacity());
    if (getBitMapEnt(hashIndx) == 0)     // quick check
        return 0;
    LsShmHIdx *pIdx = getHIdx() + hashIndx;

#ifdef DEBUG_RUN
    SHM_NOTICE("LsShmHash::find %6d %X size %d cap %d <%p> %d",
               getpid(), m_pPool->getShmMap(),
               size(),
               capacity(),
               pIdx,
               hashIndx
              );
#endif
    LsShmOffset_t offset = pIdx->x_iOffset;
    LsShmHElem *pElem;

    while (offset != 0)
    {
        pElem = (LsShmHElem *)m_pPool->offset2ptr(offset);
        if ((pElem->x_hkey == key)
            && (pElem->getKeyLen() == ls_str_len(&pParms->key))
            && ((*m_vc)(ls_str_buf(&pParms->key), pElem->getKey(),
                        ls_str_len(&pParms->key)) == 0))
            return offset;
        offset = pElem->x_iNext;
    }
    return 0;
}


LsShmHash::iteroffset LsShmHash::insert2(LsShmHKey key,
        ls_strpair_t *pParms)
{
    LsShmHElemOffs_t valueOff = sizeof(ls_vardata_t) + round4(ls_str_len(
                                    &pParms->key)) + m_iterExtraSpace;
    int valLen = ls_str_len(&pParms->value) + m_dataExtraSpace;
    LsShmHElemLen_t elementSize = sizeof(LsShmHElem) + valueOff
                      + sizeof(ls_vardata_t) + round4(valLen);
    int remapped;
    LsShmOffset_t offset = alloc2(elementSize, remapped);
    if (offset == 0)
        return 0;

    if (size() * fullFactor() > capacity())
    {
        if (rehash() < 0)
        {
            if (size() == capacity())
                return 0;
        }
    }
    LsShmHElem *pNew = (LsShmHElem *)m_pPool->offset2ptr(offset);

    pNew->x_iLen = elementSize;
    pNew->x_iValOff = valueOff;
    // pNew->x_iNext = 0;
    pNew->x_hkey = key;
    pNew->setKeyLen(ls_str_len(&pParms->key));
    pNew->setValLen(valLen);

    setIterKey(pNew, ls_str_buf(&pParms->key));
    setIterData(pNew, ls_str_buf(&pParms->value));
    if (m_iLruMode != LSSHM_LRU_NONE)
        linkHElem(pNew, offset);

    uint32_t hashIndx = getIndex(key, capacity());
    LsShmHIdx *pIdx = getHIdx() + hashIndx;
    pNew->x_iNext = pIdx->x_iOffset;
    pIdx->x_iOffset = offset;
    setBitMapEnt(hashIndx);

#ifdef DEBUG_RUN
    SHM_NOTICE("LsShmHash::insert %6d %X size %d cap %d <%p> %d",
               getpid(), m_pPool->getShmMap(),
               size(),
               capacity(),
               pIdx,
               hashIndx
              );
#endif
    incrTableSize();

    return offset;
}


LsShmHash::iteroffset LsShmHash::findNum(LsShmHash *pThis, ls_strpair_t *pParms)
{
    LsShmHKey key = (LsShmHKey)(long)ls_str_buf(&pParms->key);
    uint32_t hashIndx = pThis->getIndex(key, pThis->capacity());
    if (pThis->getBitMapEnt(hashIndx) == 0)     // quick check
        return 0;
    LsShmHIdx *pIdx = pThis->getHIdx() + hashIndx;
    LsShmOffset_t offset = pIdx->x_iOffset;
    LsShmHElem *pElem;

    while (offset != 0)
    {
        pElem = (LsShmHElem *)pThis->m_pPool->offset2ptr(offset);
        // check to see if the key is the same
        if ((pElem->x_hkey == key) && ((*(LsShmHKey *)pElem->getKey()) == key))
            return offset;
        offset = pElem->x_iNext;
    }
    return 0;
}


LsShmHash::iteroffset LsShmHash::getNum(LsShmHash *pThis,
                                        ls_strpair_t *pParms, int *pFlag)
{
    iteroffset iterOff = findNum(pThis, pParms);
    char *keyptr = ls_str_buf(&pParms->key);
    ls_strpair_t nparms;
    ls_str_set(&nparms.key, (char *)&keyptr, sizeof(LsShmHKey));
    nparms.value = pParms->value;

    return pThis->doGet(iterOff, (LsShmHKey)(long)keyptr, &nparms, pFlag);
}


LsShmHash::iteroffset LsShmHash::insertNum(LsShmHash *pThis,
        ls_strpair_t *pParms)
{
    iteroffset iterOff = findNum(pThis, pParms);
    char *keyptr = ls_str_buf(&pParms->key);
    ls_strpair_t nparms;
    ls_str_set(&nparms.key, (char *)&keyptr, sizeof(LsShmHKey));
    nparms.value = pParms->value;

    return pThis->doInsert(iterOff, (LsShmHKey)(long)keyptr, &nparms);
}


LsShmHash::iteroffset LsShmHash::setNum(LsShmHash *pThis, ls_strpair_t *pParms)
{
    iteroffset iterOff = findNum(pThis, pParms);
    char *keyptr = ls_str_buf(&pParms->key);
    ls_strpair_t nparms;
    ls_str_set(&nparms.key, (char *)&keyptr, sizeof(LsShmHKey));
    nparms.value = pParms->value;

    return pThis->doSet(iterOff, (LsShmHKey)(long)keyptr, &nparms);
}


LsShmHash::iteroffset LsShmHash::updateNum(LsShmHash *pThis,
        ls_strpair_t *pParms)
{
    iteroffset iterOff = findNum(pThis, pParms);
    char *keyptr = ls_str_buf(&pParms->key);
    ls_strpair_t nparms;
    ls_str_set(&nparms.key, (char *)&keyptr, sizeof(LsShmHKey));
    nparms.value = pParms->value;

    return pThis->doUpdate(iterOff, (LsShmHKey)(long)keyptr, &nparms);
}


LsShmHash::iteroffset LsShmHash::findPtr(LsShmHash *pThis,
        ls_strpair_t *pParms)
{
    return pThis->find2((*pThis->m_hf)(
                            ls_str_buf(&pParms->key), ls_str_len(&pParms->key)), pParms);
}


LsShmHash::iteroffset LsShmHash::getPtr(LsShmHash *pThis,
                                        ls_strpair_t *pParms, int *pFlag)
{
    LsShmHKey key = (*pThis->m_hf)(
                        ls_str_buf(&pParms->key), ls_str_len(&pParms->key));
    iteroffset iterOff = pThis->find2(key, pParms);

    return pThis->doGet(iterOff, key, pParms, pFlag);
}


LsShmHash::iteroffset LsShmHash::insertPtr(LsShmHash *pThis,
        ls_strpair_t *pParms)
{
    LsShmHKey key = (*pThis->m_hf)(
                        ls_str_buf(&pParms->key), ls_str_len(&pParms->key));
    iteroffset iterOff = pThis->find2(key, pParms);

    return pThis->doInsert(iterOff, key, pParms);
}


LsShmHash::iteroffset LsShmHash::setPtr(LsShmHash *pThis,
                                        ls_strpair_t *pParms)
{
    LsShmHKey key = (*pThis->m_hf)(
                        ls_str_buf(&pParms->key), ls_str_len(&pParms->key));
    iteroffset iterOff = pThis->find2(key, pParms);

    return pThis->doSet(iterOff, key, pParms);
}


LsShmHash::iteroffset LsShmHash::updatePtr(LsShmHash *pThis,
        ls_strpair_t *pParms)
{
    LsShmHKey key = (*pThis->m_hf)(
                        ls_str_buf(&pParms->key), ls_str_len(&pParms->key));
    iteroffset iterOff = pThis->find2(key, pParms);

    return pThis->doUpdate(iterOff, key, pParms);
}


#ifdef notdef
LsShmHash::iteroffset LsShmHash::doExpand(LsShmHash *pThis,
        iteroffset iterOff, LsShmHKey key, ls_strpair_t *pParms, uint16_t flags)
{
    iterator iter = pThis->offset2iterator(iterOff);
    int32_t lenExp = ls_str_len(&pParms->value);
    int32_t lenNew = iter->getValLen() + lenExp;
    LSSHM_CHECKSIZE(lenNew);
    if (iter->realValLen() >= (LsShmSize_t)lenNew)
    {
        iter->setValLen(lenNew);
        if (pThis->m_iLruMode != LSSHM_LRU_NONE)
            pThis->linkSetTop(iter);
    }
    else
    {
        // allocate a new iter, copy data, and remove the old
        ls_strpair_t parmsNew;
        iteroffset iterOffNew;
        iterator iterNew;
        iterOffNew = pThis->insert2(key,
                                    pThis->setParms(&parmsNew,
                                            ls_str_buf(&pParms->key), ls_str_len(&pParms->key),
                                            NULL, lenNew));
        iter = pThis->offset2iterator(iterOff);     // in case of insert remap
        iterNew = pThis->offset2iterator(iterOffNew);
        ::memcpy(iterNew->getVal(), iter->getVal(), iter->getValLen());
        pThis->eraseIteratorHelper(iter);
        iterOff = iterOffNew;
        iter = iterNew;
    }
    return iterOff;
}
#endif


LsShmHash::iteroffset LsShmHash::begin()
{
    if (size() == 0)
        return end();

    LsShmHIdx *p = getHIdx();
    LsShmHIdx *pIdxEnd = p + capacity();
    while (p < pIdxEnd)
    {
        if (p->x_iOffset != 0)
            return (iteroffset)p->x_iOffset;
        ++p;
    }
    return end();
}


LsShmHash::iteroffset LsShmHash::next(iteroffset iterOff)
{
    if (iterOff == end())
        return end();
    iterator iter = offset2iterator(iterOff);
    if (iter->x_iNext != 0)
        return (iteroffset)iter->x_iNext;

    LsShmHIdx *p = getHIdx();
    LsShmHIdx *pIdxEnd = p + capacity();
    p += (getIndex(iter->x_hkey, capacity()) + 1);
    while (p < pIdxEnd)
    {
        if (p->x_iOffset != 0)
        {
#ifdef DEBUG_RUN
            iterator xiter = (iterator)m_pPool->offset2ptr(p->x_iOffset);
            if (xiter != NULL)
            {
                if ((xiter->getKeyLen() == 0)
                    || (xiter->x_hkey == 0)
                    || (xiter->x_iLen == 0))
                {
                    SHM_NOTICE(
                        "LsShmHash::next PROBLEM %6d %X SLEEPING",
                        getpid(), m_pPool->getShmMap());
                    sleep(10);
                }
            }
#endif
            return (iteroffset)p->x_iOffset;
        }
        ++p;
    }
    return end();
}


int LsShmHash::for_each(iteroffset beg, iteroffset end, for_each_fn fun)
{
    if (fun == NULL)
    {
        errno = EINVAL;
        return LS_FAIL;
    }
    int n = 0;
    iteroffset iterNext = beg;
    iteroffset iterOff;
    while ((iterNext != 0) && (iterNext != end))
    {
        iterOff = iterNext;
        iterNext = next(iterNext);      // get next before fun
        if (fun(iterOff) != 0)
            break;
        ++n;
    }
    return n;
}


int LsShmHash::for_each2(
    iteroffset beg, iteroffset end, for_each_fn2 fun, void *pUData)
{
    if (fun == NULL)
    {
        errno = EINVAL;
        return LS_FAIL;
    }
    int n = 0;
    iteroffset iterNext = beg;
    iteroffset iterOff;
    while ((iterNext != 0) && (iterNext != end))
    {
        iterOff = iterNext;
        iterNext = next(iterNext);      // get next before fun
        if (fun(iterOff, pUData) != 0)
            break;
        ++n;
    }
    return n;
}


int LsShmHash::trim(time_t tmCutoff, int (*func)(iterator iter, void *arg),
                    void *arg)
{
    if (m_iLruMode == LSSHM_LRU_NONE)
        return LS_FAIL;
    int del = 0;
    LsShmHElem *pElem;
    LsShmOffset_t next;
    autoLockChkRehash();
    LsHashLruInfo *pLru = getLru();
    LsShmOffset_t offElem = pLru->linkLast;
    while (offElem != 0)
    {
        int ret = 1;
        pElem = (LsShmHElem *)offset2ptr(offElem);
        if (pElem->getLruLasttime() >= tmCutoff)
            break;

        if (func != NULL)
            ret = (*func)(pElem, arg);
        //int ret = clrdata(pElem->getVal());
        next = pElem->getLruLinkNext();
        eraseIteratorHelper(pElem);
        ++del;
        pLru->ndataexp += ret;
        pLru->ndataset -= ret;
        offElem = next;
    }
    pLru->nvalexp += del;
    autoUnlock();
    return del;
}


int LsShmHash::touchLru(iteroffset iterOff)
{
    if (m_iLruMode == LSSHM_LRU_NONE)
        return 0;
    
    autoLockChkRehash();
    linkSetTop(offset2iterator(iterOff));
    autoUnlock();

    return 0;
}


void LsShmHash::linkHElem(LsShmHElem *pElem, LsShmOffset_t offElem)
{
    LsHashLruInfo *pLru = getLru();
    LsShmHElemLink *pLink = pElem->getLruLinkPtr();
    if (pLru->linkFirst)
    {
        set_linkNext(pLru->linkFirst, offElem);
        pLink->x_iLinkPrev = pLru->linkFirst;
    }
    else
    {
        pLink->x_iLinkPrev = 0;
        pLru->linkLast = offElem;
    }
    pLink->x_iLinkNext = 0;
    pLink->x_lasttime = time((time_t *)NULL);
    pLru->linkFirst = offElem;
    ++pLru->nvalset;
}


void LsShmHash::unlinkHElem(LsShmHElem *pElem)
{
    LsHashLruInfo *pLru = getLru();
    LsShmHElemLink *pLink = pElem->getLruLinkPtr();
    if (pLink->x_iLinkNext)
        set_linkPrev(pLink->x_iLinkNext, pLink->x_iLinkPrev);
    else
        pLru->linkFirst = pLink->x_iLinkPrev;
    if (pLink->x_iLinkPrev)
        set_linkNext(pLink->x_iLinkPrev, pLink->x_iLinkNext);
    else
        pLru->linkLast = pLink->x_iLinkNext;
    --pLru->nvalset;
}

void LsShmHash::linkSetTop(LsShmHElem *pElem)
{
    LsShmHElemLink *pLink = pElem->getLruLinkPtr();
    LsShmOffset_t next = pLink->x_iLinkNext;
    if (next != 0)      // not top of list already
    {
        LsHashLruInfo *pLru = getLru();
        LsShmOffset_t offElem = ptr2offset(pElem);
        LsShmOffset_t prev = pLink->x_iLinkPrev;
        if (prev == 0)  // last one
            pLru->linkLast = next;
        else
            set_linkNext(prev, next);
        set_linkPrev(next, prev);
        pLink->x_iLinkNext = 0;
        pLink->x_iLinkPrev = pLru->linkFirst;
        set_linkNext(pLru->linkFirst, offElem);
        pLru->linkFirst = offElem;
    }
    pLink->x_lasttime = time((time_t *)NULL);
}



int LsShmHash::linkSetTopTime(LsShmOffset_t offset, time_t lasttime)
{
    if (m_iLruMode != LSSHM_LRU_NONE)
    {
        autoLockChkRehash();
        LsShmHElemLink *pLink = offset2iterator(offset)->getLruLinkPtr();
        if ((pLink->x_iLinkNext != 0) || (lasttime > time((time_t *)NULL)))
        {
            autoUnlock();
            return LS_FAIL;
        }
        LsShmOffset_t prev = pLink->x_iLinkPrev;
        if (prev != 0)
        {
            LsShmHElemLink *pPrev = offset2iterator(prev)->getLruLinkPtr();
            if (lasttime < pPrev->x_lasttime)
                lasttime = pPrev->x_lasttime;
        }
        pLink->x_lasttime = lasttime;
        autoUnlock();
    }
    return LS_OK;
}

int LsShmHash::linkMvTopTime(LsShmOffset_t offset, time_t lasttime)
{
    if (m_iLruMode != LSSHM_LRU_NONE)
    {
        autoLockChkRehash();
        LsShmHElemLink *pLink = offset2iterator(offset)->getLruLinkPtr();
        if ((pLink->x_iLinkNext != 0) || (lasttime <= 0))
            return LS_FAIL;
        pLink->x_lasttime = lasttime;
        LsShmOffset_t prev = pLink->x_iLinkPrev;
        if (prev == 0)  // only one
        {
            autoUnlock();
            return LS_OK;
        }
        LsShmHElemLink *pPrev = offset2iterator(prev)->getLruLinkPtr();
        if (pPrev->x_lasttime <= lasttime)  // prev is older or same time
        {
            autoUnlock();
            return LS_OK;
        }
        LsHashLruInfo *pLru = getLru();
        pPrev->x_iLinkNext = 0;
        pLru->linkFirst = prev;     // prev is now top
        while (1)
        {
            if (pPrev->x_iLinkPrev == 0)
            {
                pPrev->x_iLinkPrev = offset;
                pLink->x_iLinkNext = prev;
                pLink->x_iLinkPrev = 0;
                pLru->linkLast = offset;
                break;
            }
            prev = pPrev->x_iLinkPrev;
            pPrev = offset2iterator(prev)->getLruLinkPtr();
            if (pPrev->x_lasttime <= lasttime)
            {
                pLink->x_iLinkNext = pPrev->x_iLinkNext;
                pLink->x_iLinkPrev = prev;
                pPrev->x_iLinkNext = offset;
                set_linkPrev(pLink->x_iLinkNext, offset);
                break;
            }
        }
        autoUnlock();
    }
    return LS_OK;
}


LsShmHash::iteroffset LsShmHash::nextTmLruIterOff(time_t tmCutoff)
{
    if (m_iLruMode == LSSHM_LRU_NONE)
        return 0;
    iterator iter;
    iteroffset iterOff = getLru()->linkLast;
    if (tmCutoff > 0)
    {
        while (iterOff != 0)
        {
            iter = offset2iterator(iterOff);
            if (iter->getLruLasttime() > tmCutoff)
                break;
            iterOff = iter->getLruLinkNext();
        }
    }
    return iterOff;
}


LsShmHash::iteroffset LsShmHash::prevTmLruIterOff(time_t tmCutoff)
{
    if (m_iLruMode == LSSHM_LRU_NONE)
        return 0;
    iterator iter;
    iteroffset iterOff = getLru()->linkFirst;
    if (tmCutoff > 0)
    {
        while (iterOff != 0)
        {
            iter = offset2iterator(iterOff);
            if (iter->getLruLasttime() < tmCutoff)
                break;
            iterOff = iter->getLruLinkPrev();
        }
    }
    return iterOff;
}


int LsShmHash::check()
{
    if (m_iLruMode == LSSHM_LRU_NONE)
        return SHMLRU_BADINIT;

    int valcnt = 0;
    int datacnt = 0;
    int ret;
    LsShmHElem *pElem;
    autoLockChkRehash();
    LsHashLruInfo *pLru = getLru();
    LsShmOffset_t offElem = pLru->linkLast;
    while (offElem != 0)
    {
        ret = 1;
        pElem = (LsShmHElem *)offset2ptr(offElem);
        if (m_pLruAddon && (ret = m_pLruAddon->chkdata(pElem->getVal())) < 0)
        {
            autoUnlock();
            return ret;
        }
        ++valcnt;
        datacnt += ret;
        offElem = pElem->getLruLinkNext();
    }
    if (valcnt != pLru->nvalset)
        ret = SHMLRU_BADVALCNT;
    else if (datacnt != pLru->ndataset)
        ret = SHMLRU_BADDATACNT;
    else
        ret = SHMLRU_CHECKOK;
    autoUnlock();
    return ret;
}



//
//  @brief statIdx - helper function which return num of elements in this link
//
int LsShmHash::statIdx(iteroffset iterOff, for_each_fn2 fun, void *pUdata)
{
    LsHashStat *pHashStat = (LsHashStat *)pUdata;
#define NUM_SAMPLE  0x20
    typedef struct statKey_s
    {
        LsShmHKey    key;
        int          numDup;
    } statKey_t;
    statKey_t keyTable[NUM_SAMPLE];
    statKey_t *p_keyTable;
    int numKey = 0;
    int curDup = 0; // keep track the number of dup!

    int numInIdx = 0;
    while (iterOff != 0)
    {
        iterator iter = offset2iterator(iterOff);
        int i;
        p_keyTable = keyTable;
        for (i = 0; i < numKey; ++i, ++p_keyTable)
        {
            if (p_keyTable->key == iter->x_hkey)
            {
                ++p_keyTable->numDup;
                ++curDup;
                break;
            }
        }
        if ((i == numKey) && (i < NUM_SAMPLE))
        {
            p_keyTable->key = iter->x_hkey;
            p_keyTable->numDup = 0;
            ++numKey;
        }

        ++numInIdx;
        if (fun != NULL)
            fun(iterOff, pUdata);
        iterOff = iter->x_iNext;
    }

    pHashStat->numDup += curDup;
    return numInIdx;
}


//
//  @brief stat - populate the statistic of the hash table
//  @brief return num of elements searched.
//  @brief populate the pHashStat.
//
int LsShmHash::stat(LsHashStat *pHashStat, for_each_fn2 fun, void *pData)
{
    if (pHashStat == NULL)
    {
        errno = EINVAL;
        return LS_FAIL;
    }
    ::memset(pHashStat, 0, sizeof(LsHashStat));
    pHashStat->userData = pData;

    autoLockChkRehash();
    // search each idx
    LsShmHIdx *p = getHIdx();
    LsShmHIdx *pIdxEnd = p + capacity();
    while (p < pIdxEnd)
    {
        ++pHashStat->numIdx;
        if (p->x_iOffset != 0)
        {
            ++pHashStat->numIdxOccupied;
            int num;
            if ((num = statIdx((iteroffset)p->x_iOffset,
                               fun, (void *)pHashStat)) != 0)
            {
                pHashStat->num += num;
                if (num > pHashStat->maxLink)
                    pHashStat->maxLink = num;

                // top 10 listing
                int topidx;
                if (num <= 5)
                    topidx = num - 1;
                else if (num <= 10)
                    topidx = 5;
                else if (num <= 20)
                    topidx = 6;
                else if (num <= 50)
                    topidx = 7;
                else if (num <= 100)
                    topidx = 8;
                else
                    topidx = 9;
                ++pHashStat->top[topidx];
            }
        }
        ++p;
    }
    autoUnlock();
    return pHashStat->num;
}

