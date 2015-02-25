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
#include <stdlib.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <shm/lsshmpool.h>
#include <shm/lsshmhash.h>
#include <http/httplog.h>
#include <lsr/xxhash.h>
#if 0
#include <log4cxx/logger.h>
#endif

#ifdef DEBUG_RUN
using namespace LOG4CXX_NS;
#endif


const uint8_t LsShmHash::s_bitMask[] =
{
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};
const size_t LsShmHash::s_bitsPerChar =
  sizeof(LsShmHash::s_bitMask)/sizeof(LsShmHash::s_bitMask[0]);


enum { prime_count = 31    };
static const size_t s_primeList[prime_count] =
{
    7ul,          13ul,         29ul,
    53ul,         97ul,         193ul,       389ul,       769ul,
    1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
    49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
    1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
    50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
    1610612741ul, 3221225473ul, 4294967291ul
};


static int findRange(size_t sz)
{
    int i = 1;
    for (; i < prime_count - 1; ++i)
    {
        if (sz <= s_primeList[i])
            break;
    }
    return i;
}


static size_t roundUp(size_t sz)
{
    return s_primeList[findRange(sz)];
}


// Hash for 32bytes session id
LsShmHKey LsShmHash::hash32id(const void *__s, int len)
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


LsShmHKey LsShmHash::hashBuf(const void *__s, int len)
{
    LsShmHKey __h = 0;
    const uint8_t *p = (const uint8_t *)__s;
    uint8_t ch;

    // we will need a better hash key generator for buf key
    while (--len >= 0)
    {
        ch = *(const uint8_t *)p++;
        __h = __h * 31 + (ch);
    }
    return __h;
}

LsShmHKey LsShmHash::hashXXH32(const void *__s, int len)
{
    return XXH32(__s, len, 0);
}


int  LsShmHash::compBuf(const void *pVal1, const void *pVal2, int len)
{
    return memcmp((const char *)pVal1, (const char *)pVal2, len);
}


LsShmHKey LsShmHash::hashString(const void *__s, int len)
{
    LsShmHKey __h = 0;
    const char *p = (const char *)__s;
    char ch = *(const char *)p++;
    for (; ch ; ch = *((const char *)p++))
        __h = __h * 31 + (ch);

    return __h;
}


int LsShmHash::compString(const void *pVal1, const void *pVal2, int len)
{
    return strcmp((const char *)pVal1, (const char *)pVal2);
}


LsShmHKey LsShmHash::iHashString(const void *__s, int len)
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


int LsShmHash::iCompString(const void *pVal1, const void *pVal2, int len)
{
    return strncasecmp(
               (const char *)pVal1, (const char *)pVal2, strlen((const char *)pVal1));
}


LsShmHKey LsShmHash::hfIpv6(const void *pKey, int len)
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


int LsShmHash::cmpIpv6(const void *pVal1, const void *pVal2, int len)
{
    return memcmp(pVal1, pVal2, 16);
}


void LsShmHash::remap()
{
    m_pPool->checkRemap();
    if ((m_pShmMap != m_pPool->getShmMap())
        || (m_iCapacity != m_pTable->x_iCapacity))
    {
        m_pTable = (LsShmHTable *)m_pPool->offset2ptr(m_iOffset);

#if 0
        HttpLog::notice(
            "LsShmHash::remap %6d %X %X SIZE %X %X size %d cap %d [%d]",
            getpid(), m_pShmMap,
            m_pPool->getShmMap(),
            m_pPool->getShmMapMaxSize(),
            m_pPool->getShmMapOldMaxSize(),
            m_pTable->x_iSize,
            m_pTable->x_iCapacity,
            m_iCapacity
        );
#endif
        m_pIdxStart = (LsShmHIdx *)m_pPool->offset2ptr(m_pTable->x_iTable);
        m_pIdxEnd = m_pIdxStart + m_pTable->x_iCapacity;
        m_pBitMap = (uint8_t *)m_pPool->offset2ptr(m_pTable->x_iBitMap);
        if (m_pTable->x_iLruOffset != 0)
        {
            m_pLru =
                (LsHashLruInfo *)m_pPool->offset2ptr(m_pTable->x_iLruOffset);
        }
        m_pShmMap = m_pPool->getShmMap();
        m_iCapacity = m_pTable->x_iCapacity;
        m_pShmLock = m_pPool->lockPool()->offset2pLock(m_pTable->x_iLockOffset);
        m_pShmGLock =
            m_pPool->lockPool()->offset2pLock(m_pTable->x_iGLockOffset);
    }
    else
    {
#ifdef DEBUG_RUN
        HttpLog::notice(
            "LsShmHash::remapXXX NOCHANGE %6d %X %X SIZE %X %X size %d cap %d",
            getpid(), m_pShmMap,
            m_pPool->getShmMap(),
            m_pPool->getShmMapMaxSize(),
            m_pPool->getShmMapOldMaxSize(),
            m_pTable->x_iSize,
            m_pTable->x_iCapacity
        );
#endif
    }
}


LsShmHash::LsShmHash(LsShmPool *pool, const char *name, size_t init_size,
                     LsShmHash::hash_fn hf, LsShmHash::val_comp vc)
    : m_iMagic(LSSHM_HASH_MAGIC)
    , m_pTable(NULL)
    , m_pIdxStart(NULL)
    , m_pIdxEnd(NULL)
    , m_pBitMap(NULL)
    , m_pLru(NULL)

    , m_pPool(pool)
    , m_iOffset(0)
    , m_pName(strdup(name))

    , m_hf(hf)
    , m_vc(vc)
{
    m_iRef = 0;
    m_iCapacity = 0;
    m_status = LSSHM_NOTREADY;
    m_pShmMap = NULL;
    m_pShmLock = NULL;
    m_iLockEnable = 1;

    if ((m_pName == NULL) || (strlen(m_pName) >= LSSHM_MAXNAMELEN))
        return;

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
    init_size = roundUp(init_size);

    LsShmReg *p_reg = m_pPool->findReg(name);
    if (p_reg == NULL)
    {
        if ((p_reg = m_pPool->addReg(name)) == NULL)
        {
            m_status = LSSHM_BADMAPFILE;
            return;
        }
    }
    m_iOffset = p_reg->x_iValue;
    if (m_iOffset == 0)
    {
        LsShmOffset_t regOffset = m_pPool->ptr2offset(p_reg);

        // Create new HASH Table
        int remapped = 0;

        // NOTE: system is not up yet... ignore remap here
        m_iOffset = m_pPool->alloc2(sizeof(LsShmHTable), remapped);
        if (m_iOffset == 0)
            return;
        LsShmOffset_t iOffset =
            m_pPool->alloc2(sz2TableSz(init_size), remapped);
        if (iOffset == 0)
        {
            m_pPool->release2(m_iOffset, sizeof(LsShmHTable));
            m_iOffset = 0;
            return;
        }
        LsShmOffset_t bOffset =
            m_pPool->alloc2(sz2BitMapSz(init_size), remapped);
        if (bOffset == 0)
        {
            m_pPool->release2(iOffset, sz2TableSz(init_size));
            m_pPool->release2(m_iOffset, sizeof(LsShmHTable));
            m_iOffset = 0;
            return;
        }

        // MAP in the system here...
        m_pShmMap = m_pPool->getShmMap();
        m_pTable = (LsShmHTable *)m_pPool->offset2ptr(m_iOffset);

        m_pTable->x_iMagic = LSSHM_HASH_MAGIC;
        m_pTable->x_iCapacity = init_size;
        m_pTable->x_iSize = 0;
        m_pTable->x_iFullFactor = 2;
        m_pTable->x_iGrowFactor = 2;
        m_pTable->x_iTable = iOffset;
        m_pTable->x_iBitMap = bOffset;
        m_pTable->x_iLockOffset = 0;
        m_pTable->x_iLruOffset = 0;

        m_pShmLock = m_pPool->lockPool()->allocLock();
        if (m_pShmLock == NULL)
        {
            m_pPool->release2(m_iOffset, sizeof(LsShmHTable));
            m_iOffset = 0;
            return;
        }
        m_pTable->x_iLockOffset = m_pPool->lockPool()->pLock2offset(m_pShmLock);

        m_pShmGLock = m_pPool->lockPool()->allocLock();
        if (m_pShmGLock == NULL)
        {
            m_pPool->release2(m_iOffset, sizeof(LsShmHTable));
            m_iOffset = 0;
            return;
        }
        m_pTable->x_iGLockOffset =
            m_pPool->lockPool()->pLock2offset(m_pShmGLock);

        m_pIdxStart = (LsShmHIdx *)m_pPool->offset2ptr(m_pTable->x_iTable);
        m_pIdxEnd = m_pIdxStart + m_pTable->x_iCapacity;
        m_pBitMap = (uint8_t *)m_pPool->offset2ptr(m_pTable->x_iBitMap);
        m_iCapacity = m_pTable->x_iCapacity;

        // clear all idx
        ::memset(m_pIdxStart, 0, sz2TableSz(m_pTable->x_iCapacity));
        ::memset(m_pBitMap, 0, sz2BitMapSz(m_pTable->x_iCapacity));

        LsShmReg *xp_reg = (LsShmReg *)m_pPool->offset2ptr(regOffset);

        xp_reg->x_iValue = m_iOffset;
        m_pTable->x_iMode = m_iMode;
        m_pTable->x_iLruMode = 0;
    }
    else
    {
        remap();

        // check the magic and mode
        if ((m_iMagic != m_pTable->x_iMagic) || (m_iMode != m_pTable->x_iMode))
            return;
    }

    // setup local and global lock
    m_pShmLock = m_pPool->lockPool()->offset2pLock(m_pTable->x_iLockOffset);
    m_pShmGLock = m_pPool->lockPool()->offset2pLock(m_pTable->x_iGLockOffset);
    if ((m_iOffset != 0) && (setupLock() == 0) && (setupGLock() == 0))
    {
        m_iRef = 1;
        m_status = LSSHM_READY;
#ifdef DEBUG_RUN
        HttpLog::notice("LsShmHash::LsShmHash insert %s <%p>",
                        m_pName, &m_objBase);
#endif
        m_pPool->getShm()->getObjBase().insert(m_pName, this);
    }
    else
        m_status = LSSHM_ERROR;
}


LsShmHash::~LsShmHash()
{
    if (m_pName != NULL)
    {
        if (m_iRef == 0)
        {
#ifdef DEBUG_RUN
            HttpLog::notice("LsShmHash::~LsShmHash remove %s <%p>",
                            m_pName, &m_objBase);
#endif
            m_pPool->getShm()->getObjBase().remove(m_pName);
        }
        free(m_pName);
        m_pName = NULL;
    }
}


LsShmHash *LsShmHash::checkHTable(GHash::iterator itor, LsShmPool *pool,
                                  const char *name, LsShmHash::hash_fn hf, LsShmHash::val_comp vc)
{
    LsShmHash *pObj;
    if (((pObj = (LsShmHash *)itor->second()) == NULL)
        || (pObj->m_iMagic != LSSHM_HASH_MAGIC)
        || (pObj->m_hf != hf)
        || (pObj->m_vc != vc))
        return NULL;    // bad: parameters not matching

    if (pObj->m_pPool != pool)
        return (LsShmHash *) - 1; // special case: different pools
    pObj->upRef();
    return pObj;
}


void LsShmHash::close()
{
    LsShmPool *p = NULL;
    if (m_iPoolOwner != 0)
    {
        m_iPoolOwner = 0;
        p = m_pPool;
    }
    if (downRef() == 0)
        delete this;
    if (p != NULL)
        p->close();
}


//
//  The only way to remove the Shared Memory
//
void LsShmHash::destroy()
{
    if (m_iOffset != 0)
    {
        // all elements
        clear();
        if (m_pTable->x_iTable != 0)
        {
            m_pPool->release2(
                m_pTable->x_iTable, sz2TableSz(m_pTable->x_iCapacity));
        }
        if (m_pTable->x_iBitMap != 0)
        {
            m_pPool->release2(
                m_pTable->x_iBitMap, sz2BitMapSz(m_pTable->x_iCapacity));
        }
        if (m_pTable->x_iLruOffset != 0)
            m_pPool->release2(m_pTable->x_iLruOffset, sizeof(LsHashLruInfo));
        m_pPool->release2(m_iOffset, sizeof(LsShmHTable));

        // remove from regMap
        LsShmReg *p_reg = m_pPool->findReg(m_pName);
        p_reg->x_iValue = 0;
        m_iOffset = 0;
        m_pTable = NULL;
        m_pIdxStart = NULL;
        m_pIdxEnd = NULL;
        m_pBitMap = NULL;
        m_pLru = NULL;
    }
}


int LsShmHash::rehash()
{
    remap();
    int range = findRange(m_pTable->x_iCapacity);
    int newSize = s_primeList[range + m_pTable->x_iGrowFactor];

#ifdef DEBUG_RUN
    HttpLog::notice("LsShmHash::rehash %6d %X %X size %d cap %d NEW %d",
                    getpid(), m_pShmMap, m_pPool->getShmMap(),
                    m_pTable->x_iSize,
                    m_pTable->x_iCapacity,
                    newSize
                   );
#endif
    int remapped = 0;
    LsShmOffset_t newIdxOff = m_pPool->alloc2(sz2TableSz(newSize), remapped);
    if (newIdxOff == 0)
        return LS_FAIL;
    LsShmOffset_t newBitOff = m_pPool->alloc2(sz2BitMapSz(newSize), remapped);
    if (newBitOff == 0)
    {
        m_pPool->release2(newIdxOff, sz2TableSz(newSize));
        return LS_FAIL;
    }

    // if (remapped)
    remap();
    LsShmHIdx *pIdx = (LsShmHIdx *)m_pPool->offset2ptr(newIdxOff);
    ::memset(pIdx, 0, sz2TableSz(newSize));
    uint8_t *pBitMap = (uint8_t *)m_pPool->offset2ptr(newBitOff);
    ::memset(pBitMap, 0, sz2BitMapSz(newSize));

    iterator iterNext = begin();
    while (iterNext != end())
    {
        iterator iter = iterNext;
        iterNext = next(iter);

        LsShmHIdx *npIdx = pIdx + getIndex(iter->x_hkey, newSize);
        assert(npIdx->x_iOffset < m_pPool->getShmMap()->x_iMaxSize);
        iter->x_iNext = npIdx->x_iOffset;
        npIdx->x_iOffset = m_pPool->ptr2offset(iter);
    }
    m_pPool->release2(
        m_pTable->x_iTable, sz2TableSz(m_pTable->x_iCapacity));
    m_pPool->release2(
        m_pTable->x_iBitMap, sz2BitMapSz(m_pTable->x_iCapacity));

    m_pTable->x_iTable = newIdxOff;
    m_pTable->x_iBitMap = newBitOff;
    m_pTable->x_iCapacity = newSize;
    m_iCapacity = newSize;
    m_pIdxStart = (LsShmHIdx *)m_pPool->offset2ptr(m_pTable->x_iTable);
    m_pIdxEnd = m_pIdxStart + m_pTable->x_iCapacity;
    m_pBitMap = (uint8_t *)m_pPool->offset2ptr(m_pTable->x_iBitMap);
    remap();
    return 0;
}


int LsShmHash::release_hash_elem(LsShmHash::iterator iter, void *pUData)
{
    LsShmHash *pThis = (LsShmHash *)pUData;
    pThis->m_pPool->release2(pThis->m_pPool->ptr2offset(iter), iter->x_iLen);
    return 0;
}


void LsShmHash::clear()
{
    int n = for_each2(begin(), end(), release_hash_elem, this);
    assert(n == (int)m_pTable->x_iSize);

    ::memset(m_pIdxStart, 0, sz2TableSz(m_pTable->x_iCapacity));
    ::memset(m_pBitMap, 0, sz2BitMapSz(m_pTable->x_iCapacity));
    m_pTable->x_iSize = 0;
}


//
// @brief erase - remove iter from the SHM pool.
// @brief will destroy the link to itself if any!
//
void LsShmHash::eraseIteratorHelper(iterator iter)
{
    if (iter == NULL)
        return;

    LsShmOffset_t iterOffset = m_pPool->ptr2offset(iter);
    LsShmHIdx *pIdx =
        m_pIdxStart + getIndex(iter->x_hkey, m_pTable->x_iCapacity);
    LsShmOffset_t offset = pIdx->x_iOffset;
    LsShmHElem *pElem;

#ifdef DEBUG_RUN
    if (offset == 0)
    {
        HttpLog::notice(
            "LsShmHash::eraseIteratorHelper %6d %X %X size %d cap %d",
            getpid(), m_pShmMap, m_pPool->getShmMap(),
            m_pTable->x_iSize,
            m_pTable->x_iCapacity
        );
        sleep(10);
    }
#endif

    if (offset == iterOffset)
    {
        if ((pIdx->x_iOffset = iter->x_iNext) == 0) // last one
            clrBitMapEnt(iter->x_hkey);
    }
    else
    {
        do
        {
            pElem = (LsShmHElem *)m_pPool->offset2ptr(offset);
            if (pElem->x_iNext == iterOffset)
            {
                pElem->x_iNext = iter->x_iNext;
                break;
            }
            // next offset...
            offset = pElem->x_iNext;
        }
        while (offset != 0);
    }

    m_pPool->release2(iterOffset, iter->x_iLen);
    --m_pTable->x_iSize;
}


LsShmHash::iterator LsShmHash::find2(
    const void *pKey, int keyLen, LsShmHKey key)
{
    if (getBitMapEnt(key) == 0)     // quick check
        return end();
    LsShmHIdx *pIdx = m_pIdxStart + getIndex(key, m_pTable->x_iCapacity);

#ifdef DEBUG_RUN
    HttpLog::notice("LsShmHash::find %6d %X %X size %d cap %d <%p> %d",
                    getpid(), m_pShmMap, m_pPool->getShmMap(),
                    m_pTable->x_iSize,
                    m_pTable->x_iCapacity,
                    pIdx,
                    getIndex(key, m_pTable->x_iCapacity)
                   );
#endif
    LsShmOffset_t offset = pIdx->x_iOffset;
    LsShmHElem *pElem;

    while (offset != 0)
    {
        pElem = (LsShmHElem *)m_pPool->offset2ptr(offset);
        if ((pElem->x_hkey == key)
            && (pElem->getKeyLen() == keyLen)
            && ((*m_vc)(pKey, pElem->getKey(), keyLen) == 0))
            return pElem;
        offset = pElem->x_iNext;
        remap();
    }
    return end();
}


LsShmHash::iterator LsShmHash::insert2(
    const void *pKey, int keyLen, const void *pValue, int valueLen,
    LsShmHKey key)
{
    uint32_t valueOff = sizeof(ls_vardata_t) + round4(keyLen);
    valueSetup(&valueOff, &valueLen);
    int elementSize = sizeof(LsShmHElem) + valueOff
      + sizeof(ls_vardata_t) + round4(valueLen);
    int remapped = 0;
    LsShmOffset_t offset = m_pPool->alloc2(elementSize, remapped);
    if (offset == 0)
        return end();

    remap();
    if (m_pTable->x_iSize * m_pTable->x_iFullFactor > m_pTable->x_iCapacity)
    {
        if (rehash() < 0)
        {
            if (m_pTable->x_iSize == m_pTable->x_iCapacity)
                return end();
        }
    }
    LsShmHElem *pNew = (LsShmHElem *)m_pPool->offset2ptr(offset);

    pNew->x_iLen = elementSize;
    pNew->x_iValOff = valueOff;
    // pNew->x_iNext = 0;
    pNew->x_hkey = key;
    pNew->setKeyLen(keyLen);
    pNew->setValLen(valueLen);

    setIterKey(pNew, pKey);
    setIterData(pNew, pValue);
    linkHElem(pNew, offset);

    LsShmHIdx *pIdx = m_pIdxStart + getIndex(key, m_pTable->x_iCapacity);
    pNew->x_iNext = pIdx->x_iOffset;
    pIdx->x_iOffset = offset;
    setBitMapEnt(key);

#ifdef DEBUG_RUN
    HttpLog::notice("LsShmHash::insert %6d %X %X size %d cap %d <%p> %d",
                    getpid(), m_pShmMap, m_pPool->getShmMap(),
                    m_pTable->x_iSize,
                    m_pTable->x_iCapacity,
                    pIdx,
                    getIndex(key, m_pTable->x_iCapacity)
                   );
#endif
    ++m_pTable->x_iSize;

    return pNew;
}


LsShmHash::iterator LsShmHash::insertNum(LsShmHash *pThis,
        const void *pKey, int keyLen, const void *pValue, int valueLen)
{
    iterator iter = findNum(pThis, pKey, sizeof(LsShmHKey));
    return (iter != pThis->end()) ?
           pThis->end() :
           pThis->insert2(&pKey, sizeof(LsShmHKey), pValue, valueLen, (LsShmHKey)(long)pKey);
}


LsShmHash::iterator LsShmHash::setNum(LsShmHash *pThis,
                                      const void *pKey, int keyLen, const void *pValue, int valueLen)
{
    LSSHM_CHECKSIZE(valueLen);
    iterator iter = findNum(pThis, pKey, sizeof(LsShmHKey));
    if (iter != pThis->end())
    {
        if (iter->realValLen() >= valueLen)
        {
            iter->setValLen(valueLen);
            pThis->setIterData(iter, pValue);
            return iter;
        }
        else
            pThis->eraseIteratorHelper(iter);
    }
    return pThis->insert2(
               &pKey, sizeof(LsShmHKey), pValue, valueLen, (LsShmHKey)(long)pKey);
}


LsShmHash::iterator LsShmHash::updateNum(LsShmHash *pThis,
        const void *pKey, int keyLen, const void *pValue, int valueLen)
{
    LSSHM_CHECKSIZE(valueLen);
    iterator iter = findNum(pThis, pKey, sizeof(LsShmHKey));
    if (iter == pThis->end())
        return pThis->end();

    if (iter->realValLen() >= valueLen)
    {
        iter->setValLen(valueLen);
        pThis->setIterData(iter, pValue);
        return iter;
    }
    pThis->eraseIteratorHelper(iter);
    return pThis->insert2(
               &pKey, sizeof(LsShmHKey), pValue, valueLen, (LsShmHKey)(long)pKey);
}


LsShmHash::iterator LsShmHash::findNum(LsShmHash *pThis,
                                       const void *pKey, int keyLen)
{
    if (pThis->getBitMapEnt((LsShmHKey)(long)pKey) == 0)     // quick check
        return pThis->end();
    LsShmHIdx *pIdx = pThis->m_pIdxStart +
                      pThis->getIndex((LsShmHKey)(long)pKey, pThis->m_pTable->x_iCapacity);
    LsShmOffset_t offset = pIdx->x_iOffset;
    LsShmHElem *pElem;

    while (offset != 0)
    {
        pElem = (LsShmHElem *)pThis->m_pPool->offset2ptr(offset);
        // check to see if the key is the same
        if ((pElem->x_hkey == (LsShmHKey)(long)pKey)
            && ((*(LsShmHKey *)pElem->getKey()) == (LsShmHKey)(long)pKey))
            return pElem;
        offset = pElem->x_iNext;
    }
    return pThis->end();
}


LsShmHash::iterator LsShmHash::getNum(LsShmHash *pThis,
                                      const void *pKey, int keyLen, const void *pValue, int valueLen, int *pFlag)
{
    LSSHM_CHECKSIZE(valueLen);

    iterator iter = findNum(pThis, pKey, sizeof(LsShmHKey));
    if (iter != pThis->end())
    {
        *pFlag = LSSHM_FLAG_NONE;
        return iter;
    }

    iter = pThis->insert2(
               &pKey, sizeof(LsShmHKey), pValue, valueLen, (LsShmHKey)(long)pKey);
    if (iter != pThis->end())
    {
        if (*pFlag & LSSHM_FLAG_INIT)
        {
            // initialize the memory
            ::memset(iter->getVal(), 0, valueLen);
        }
        *pFlag = LSSHM_FLAG_CREATED;
    }
    else
    {
        ;   // dont need to set pFlag...
    }
    return iter;
}


LsShmHash::iterator LsShmHash::insertPtr(LsShmHash *pThis,
        const void *pKey, int keyLen, const void *pValue, int valueLen)
{
    LSSHM_CHECKSIZE(valueLen);
    LsShmHKey key = (*pThis->m_hf)(pKey, keyLen);
    iterator iter = pThis->find2(pKey, keyLen, key);
    return (iter != pThis->end()) ?
           pThis->end() :
           pThis->insert2(pKey, keyLen, pValue, valueLen, key);
}


LsShmHash::iterator LsShmHash::setPtr(LsShmHash *pThis,
                                      const void *pKey, int keyLen, const void *pValue, int valueLen)
{
    LSSHM_CHECKSIZE(valueLen);
    LsShmHKey key = (*pThis->m_hf)(pKey, keyLen);
    iterator iter = pThis->find2(pKey, keyLen, key);
    if (iter != pThis->end())
    {
        if (iter->realValLen() >= valueLen)
        {
            iter->setValLen(valueLen);
            pThis->setIterData(iter, pValue);
            return iter;
        }
        else
        {
            // remove the iter and install new one
            pThis->eraseIteratorHelper(iter);
        }
    }
    return pThis->insert2(pKey, keyLen, pValue, valueLen, key);
}


LsShmHash::iterator LsShmHash::updatePtr(LsShmHash *pThis,
        const void *pKey, int keyLen, const void *pValue, int valueLen)
{
    LSSHM_CHECKSIZE(valueLen);
    LsShmHKey key = (*pThis->m_hf)(pKey, keyLen);
    iterator iter = pThis->find2(pKey, keyLen, key);
    if (iter == pThis->end())
        return pThis->end();

    if (iter->realValLen() >= valueLen)
    {
        iter->setValLen(valueLen);
        pThis->setIterData(iter, pValue);
        return iter;
    }
    // remove the iter and install new one
    pThis->eraseIteratorHelper(iter);
    return pThis->insert2(pKey, keyLen, pValue, valueLen, key);
}


LsShmHash::iterator LsShmHash::findPtr(LsShmHash *pThis,
                                       const void *pKey, int keyLen)
{
    return pThis->find2(pKey, keyLen, (*pThis->m_hf)(pKey, keyLen));
}


LsShmHash::iterator LsShmHash::getPtr(LsShmHash *pThis,
                                      const void *pKey, int keyLen, const void *pValue, int valueLen, int *pFlag)
{
    LSSHM_CHECKSIZE(valueLen);

    LsShmHKey key = (*pThis->m_hf)(pKey, keyLen);
    iterator iter = pThis->find2(pKey, keyLen, key);

    if (iter != pThis->end())
    {
        *pFlag = LSSHM_FLAG_NONE;
        return iter;
    }

    iter = pThis->insert2(pKey, keyLen, pValue, valueLen, key);
    if (iter != pThis->end())
    {
        if (*pFlag & LSSHM_FLAG_INIT)
        {
            // initialize the memory
            ::memset(iter->getVal(), 0, valueLen);
        }
        *pFlag = LSSHM_FLAG_CREATED;
    }
    else
    {
        ;   // dont need to set pFlag...
    }
    return iter;
}


LsShmHash::iterator LsShmHash::next(iterator iter)
{
    if (iter == end())
        return end();
    if (iter->x_iNext != 0)
        return (iterator)m_pPool->offset2ptr(iter->x_iNext);

    LsShmHIdx *p;
    p = m_pIdxStart + getIndex(iter->x_hkey, m_pTable->x_iCapacity) + 1;
    while (p < m_pIdxEnd)
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
                    HttpLog::notice(
                        "LsShmHash::next PROBLEM %6d %X %X SIZE %X SLEEPING",
                        getpid(), m_pShmMap,
                        m_pPool->getShmMap(),
                        m_pPool->getShmMapMaxSize());
                    sleep(10);
                }
            }
#endif
            return (iterator)m_pPool->offset2ptr(p->x_iOffset);
        }
        ++p;
    }
    return end();
}


int LsShmHash::for_each(iterator beg, iterator end, for_each_fn fun)
{
    if (fun == NULL)
    {
        errno = EINVAL;
        return LS_FAIL;
    }
    if (beg == NULL)
        return 0;
    int n = 0;
    iterator iterNext = beg;
    iterator iter;
    while ((iterNext != NULL) && (iterNext != end))
    {
        iter = iterNext;
        iterNext = next(iterNext);
        if (fun(iter) != 0)
            break;
        ++n;
    }
    return n;
}


int LsShmHash::for_each2(
    iterator beg, iterator end, for_each_fn2 fun, void *pUData)
{
    if (fun == NULL)
    {
        errno = EINVAL;
        return LS_FAIL;
    }
    if (beg == NULL)
        return 0;
    int n = 0;
    iterator iterNext = beg;
    iterator iter ;
    while ((iterNext != NULL) && (iterNext != end))
    {
        iter = iterNext;
        iterNext = next(iterNext);
        if (fun(iter, pUData) != 0)
            break;
        ++n;
    }
    return n;
}


//
//  @brief statIdx - helper function which return num of elements in this link
//
int LsShmHash::statIdx(iterator iter, for_each_fn2 fun, void *pUdata)
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
    while (iter != end())
    {
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
        fun(iter, pUdata);
        iter = (iterator)m_pPool->offset2ptr(iter->x_iNext);
    }

    pHashStat->numDup += curDup;
    return numInIdx;
}


//
//  @brief stat - populate the statistic of the hash table
//  @brief return num of elements searched.
//  @brief populate the pHashStat.
//
int LsShmHash::stat(LsHashStat *pHashStat, for_each_fn2 fun)
{
    if (pHashStat == NULL)
    {
        errno = EINVAL;
        return LS_FAIL;
    }
    ::memset(pHashStat, 0, sizeof(LsHashStat));

    // search each idx
    LsShmHIdx *p = m_pIdxStart;
    while (p < m_pIdxEnd)
    {
        ++pHashStat->numIdx;
        if (p->x_iOffset != 0)
        {
            ++pHashStat->numIdxOccupied;
            int num;
            if ((num = statIdx((iterator)m_pPool->offset2ptr(p->x_iOffset),
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
    return pHashStat->num;
}


LsShmHash::iterator LsShmHash::begin()
{
    if (m_pTable->x_iSize == 0)
        return end();

    LsShmHIdx *p = m_pIdxStart;
    while (p < m_pIdxEnd)
    {
        if (p->x_iOffset != 0)
            return (iterator)m_pPool->offset2ptr(p->x_iOffset);
        ++p;
    }
    return NULL;
}


