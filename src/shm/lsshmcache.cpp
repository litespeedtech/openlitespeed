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

#include <shm/lsshmcache.h>

#include <http/httplog.h>
#include <shm/lsshmpool.h>
#include <util/datetime.h>

#include <string.h>


//
//  @brief LsShmCache
//  @brief Shared Memory Hash Cache Container
//
LsShmCache::LsShmCache(
    uint32_t              magic,
    const char           *cacheName,
    const char           *shmHashName,
    size_t                initHashSize,
    LsShmSize_t           uDataSize,
    LsShmHash::hash_fn    hf,
    LsShmHash::val_comp   vc,
    udata_init_fn         udataInitCallback,
    udata_remove_fn       udataRemoveCallback
)
    : m_status(LSSHM_NOTREADY)
    , m_pShmHash(NULL)
    , m_iHdrOffset(0)
    , m_iMagic(magic)
    , m_iDataSize(uDataSize)
    , m_dataInit_cb(udataInitCallback)
    , m_dataRemove_cb(udataRemoveCallback)
{
    LsShm *pShm;
    LsShmPool *pPool;

    if ((pShm = LsShm::open(shmHashName, 0)) == NULL)
        return;
    if ((pPool = pShm->getGlobalPool()) == NULL)
        return;
    if ((m_pShmHash = pPool->getNamedHash(shmHashName, initHashSize, hf, vc, LSSHM_LRU_NONE)) != NULL)
    {
        LsShmReg *pReg;
        if ((pReg = m_pShmHash->findReg(cacheName)) == NULL)
            pReg = m_pShmHash->addReg(cacheName);
        if (pReg != NULL)
        {
            if ((m_iHdrOffset = pReg->x_iValue) == 0)
            {
                int remapped;
                m_iHdrOffset =
                    m_pShmHash->alloc2(sizeof(lsShm_hCacheHdr_t), remapped);
                if (m_iHdrOffset == 0)
                {
                    // trouble no memory
                    m_status = LSSHM_ERROR;
                }
                else
                {
                    pReg->x_iValue = m_iHdrOffset;
                }
            }
        }
        if (m_status == LSSHM_NOTREADY)
            m_status = LSSHM_READY;
        m_pShmHash->disableLock(); // we will be responsible for the lock
    }
}


void LsShmCache::push(lsShm_hCacheData_t *pObj)
{
    if (pObj->x_iTracked != 0)
        return;

    lsShm_hCacheHdr_t *pHdr = getCacheHeader();
    LsShmOffset_t offset = m_pShmHash->ptr2offset(pObj);
    if (pHdr->x_iFront != 0)
    {
        lsShm_hCacheData_t *pFront =
            (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(pHdr->x_iFront);
        pFront->x_iNext = offset;
        pObj->x_iBack = pHdr->x_iFront;
    }
    else
    {
        // lonely child
        pHdr->x_iBack = offset;
        pObj->x_iBack = 0;
    }
    pHdr->x_iFront = offset;
    ++pHdr->x_iNum;

    pObj->x_iNext = 0;
    pObj->x_iTracked = 1;
}


void LsShmCache::pop()
{
    lsShm_hCacheHdr_t *pHdr = getCacheHeader();
    LsShmOffset_t offset = pHdr->x_iBack;

    if (offset != 0)
    {
        lsShm_hCacheData_t *pBack =
            (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(offset);
        pHdr->x_iBack = pBack->x_iNext;
        if (pHdr->x_iBack == 0)
        {
            // queue is empty after pop...
            pHdr->x_iFront = 0;
        }
        else
        {
            lsShm_hCacheData_t *pNext =
                (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(pHdr->x_iBack);
            pNext->x_iBack = 0;
        }
        --pHdr->x_iNum;
        pBack->x_iNext = 0;
        pBack->x_iTracked = 0;
    }
}


void LsShmCache::push_back(lsShm_hCacheData_t *pObj)
{
    if (pObj->x_iTracked != 0)
        return;

    lsShm_hCacheHdr_t *pHdr = getCacheHeader();
    LsShmOffset_t offset = m_pShmHash->ptr2offset(pObj);
    if (pHdr->x_iBack != 0)
    {
        lsShm_hCacheData_t *pBack =
            (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(pHdr->x_iBack);
        pBack->x_iBack = offset;
        pObj->x_iNext = pHdr->x_iBack;
    }
    else
    {
        // lonely child
        pHdr->x_iFront = offset;
        pObj->x_iNext = 0;
    }
    pHdr->x_iBack = offset;
    ++pHdr->x_iNum;
    pObj->x_iBack = 0; //ensure I am the last one
    pObj->x_iTracked = 1;
}


void LsShmCache::setFirst(lsShm_hCacheData_t *pObj)
{
    lsShm_hCacheHdr_t *pHdr = getCacheHeader();
    lsShm_hCacheData_t *pFirst =
        (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(pHdr->x_iFront);
    if (pObj == pFirst)
        return;
    disconnectObj(pObj);
    push(pObj);
}


void LsShmCache::setLast(lsShm_hCacheData_t *pObj)
{
    lsShm_hCacheHdr_t *pHdr = getCacheHeader();
    lsShm_hCacheData_t *pLast =
        (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(pHdr->x_iBack);
    if (pObj == pLast)
        return;
    disconnectObj(pObj);
    push_back(pObj);
}


void LsShmCache::removeObjData(lsShm_hCacheData_t *pObj, void *pUParam)
{
#ifdef DEBUG_RUN
    LsShmHash::iterator iter =
        m_pShmHash->offset2iterator(pObj->x_iIteratorOffset);
    HttpLog::notice(
        "LsShmCache::removeObjData %6d iter <%p> obj <%p> size %d",
        getpid(), iter, pObj, (long)pObj - (long)iter);
#endif
    if (m_dataRemove_cb != NULL)
        (*m_dataRemove_cb)(pObj, pUParam);
//     m_pShmHash->eraseIteratorHelper(
//         m_pShmHash->offset2iterator(pObj->x_iIteratorOffset));
    m_pShmHash->eraseIterator(pObj->x_iIteratorOffset);
}


//
//  @brief disconnectObj
//  @brief disconnect the forward and backward links to the queue
//
void LsShmCache::disconnectObj(lsShm_hCacheData_t *pObj)
{
    if (pObj->x_iTracked == 0)
        return;

    lsShm_hCacheHdr_t *pHdr = getCacheHeader();
    if (pObj->x_iNext != 0)
    {
        lsShm_hCacheData_t *pNext =
            (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(pObj->x_iNext);
        if (pObj->x_iBack != 0)
        {
            lsShm_hCacheData_t *pBack =
                (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(pObj->x_iBack);
            pBack->x_iNext = pObj->x_iNext;
            pNext->x_iBack = pObj->x_iBack;
        }
        else
        {
            pNext->x_iBack = 0;
            pHdr->x_iBack = pObj->x_iNext;
        }
    }
    else
    {
        if (pObj->x_iBack != 0)
        {
            lsShm_hCacheData_t *pBack =
                (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(pObj->x_iBack);
            pBack->x_iNext = 0;
            pHdr->x_iFront = pObj->x_iBack;
        }
        else
        {
            pHdr->x_iBack = 0;
            pHdr->x_iFront = 0;
        }
    }
    --pHdr->x_iNum;
    pObj->x_iNext = 0;
    pObj->x_iBack = 0;
    pObj->x_iTracked = 0; // not in the queue
}


lsShm_hCacheData_t *LsShmCache::getObj(const void *pKey, int keyLen,
                                       const void *pValue, int valueLen, void *pUParam)
{
    lsShm_hCacheData_t *pObj;

#if 0
    pObj = findObj(pKey, keyLen);
    if (pObj != NULL)
        return pObj;
#endif
    LsShmHash::iteroffset iterOff;
    ls_str_pair_t parms;
    if ((iterOff = m_pShmHash->insertIterator(
        m_pShmHash->setParms(&parms, pKey, keyLen, NULL, valueLen))) == 0)
        return NULL;
    pObj = (lsShm_hCacheData_t *)m_pShmHash->offset2iteratorData(iterOff);
    ::memset(pObj, 0, valueLen);
    pObj->x_iMagic = m_iMagic;
    pObj->x_iExpireTime = DateTime::s_curTime;
    pObj->x_iExpireTimeMs = DateTime::s_curTimeUs / 1000;
    pObj->x_iIteratorOffset = iterOff;
#if 0
    // memset take care of these already...
    pObj->x_iNext = 0;
    pObj->x_iBack = 0;
    pObj->x_iInited = 0;    // init indication
    pObj->x_iTracked = 0;   // push indication
#endif

#ifdef DEBUG_RUN
    HttpLog::notice("LsShmCache::getObj %6d iterOff <%d> obj <%p>",
                    ::getpid(), iterOff, pObj);
#endif

    if (m_dataInit_cb != NULL)
        (*m_dataInit_cb)(pObj, pUParam);
    return pObj;
}


int LsShmCache::isElementExpired(lsShm_hCacheData_t *pObj)
{
    return (pObj->x_iExpired
            || ((((DateTime::s_curTime - pObj->x_iExpireTime) * 1000)
                    + ((DateTime::s_curTimeUs / 1000) - pObj->x_iExpireTimeMs)) > 0)
            );
}


int LsShmCache::remove2NonExpired(void *pUParam)
{
    int numRemoved = 0;    // number of cache items removed
    lsShm_hCacheHdr_t *pHdr = getCacheHeader();
    LsShmOffset_t offset;

    while ((offset = pHdr->x_iBack) != 0)
    {
        lsShm_hCacheData_t *pObj;
        pObj = (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(offset);
        if (isElementExpired(pObj))
        {
            pop();
            // remove
            removeObjData(pObj, pUParam);
            ++numRemoved;
        }
        else
        {
            // stop on first non expired
            break;
        }
    }
    return numRemoved;
}


int LsShmCache::removeAllExpired(void *pUParam)
{
    int numRemoved = 0;   // number of cache items removed
    lsShm_hCacheHdr_t *pHdr = getCacheHeader();
    LsShmOffset_t offset = pHdr->x_iBack;

    numRemoved = remove2NonExpired(pUParam);
    if ((offset = pHdr->x_iBack) != 0)
    {
        // skip the first one!
        lsShm_hCacheData_t *pObj;
        pObj = (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(offset);
        while ((offset = pObj->x_iNext) != 0)
        {
            pObj = (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(offset);
            if (isElementExpired(pObj))
            {
                disconnectObj(pObj);
                removeObjData(pObj, pUParam);
                ++numRemoved;
            }
        }
    }
    return numRemoved;
}


void LsShmCache::stat(LsHashStat &stat, udata_callback_fn _cb)
{
    ::memset(&stat, 0, sizeof(LsHashStat));
    lsShm_hCacheHdr_t *pHdr = getCacheHeader();
    LsShmOffset_t offset = pHdr->x_iBack;
    while (offset != 0)
    {
        lsShm_hCacheData_t *pObj;
        pObj = (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(offset);
        if (isElementExpired(pObj))
            ++stat.numExpired;
        ++stat.num;

        if (_cb != NULL)
            (*_cb)(pObj, (void *)&stat);
        offset = pObj->x_iNext;
    }
}


int LsShmCache::loop(LsShmCache::udata_callback_fn _cb, void *pUParam)
{
    int num = 0;
    lsShm_hCacheHdr_t *pHdr = getCacheHeader();
    lsShm_hCacheData_t *pObj;
    int retValue;

    LsShmOffset_t offset = pHdr->x_iBack;
    while (offset != 0)
    {
        pObj = (lsShm_hCacheData_t *)m_pShmHash->offset2ptr(offset);
        if ((retValue = (*_cb)(pObj, pUParam)) != 0)
            return retValue;

        offset = pObj->x_iNext;
        ++num;
    }
    return num;
}
