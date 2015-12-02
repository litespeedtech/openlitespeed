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
#include <shm/lsshmtidmgr.h>


typedef struct
{
    uint64_t             x_tid;           // `transaction' id
} LsShmTidLink;

int LsShmTidMgr::init(LsShmHash *pHash, LsShmOffset_t off, bool blkinit)
{
    LsShmTidInfo *pTidInfo;
    m_pHash = pHash;
    m_iOffset = off;
    if (blkinit)
    {
#ifdef notdef
        m_pShmLock = m_pHash->getPool()->lockPool()->allocLock();
        if ((m_pShmLock == NULL) || (setupLock() != 0))
            return -1;
#endif
        pTidInfo = getTidInfo();
        ::memset((void *)pTidInfo, 0, sizeof(*pTidInfo));
#ifdef notdef
        pTidInfo->x_iLockOff =
            m_pHash->getPool()->lockPool()->pLock2offset(m_pShmLock);
#endif
    }
    else
    {
        pTidInfo = getTidInfo();
#ifdef notdef
        m_pShmLock =
            m_pHash->getPool()->lockPool()->offset2pLock(pTidInfo->x_iLockOff);
#endif
    }
    return 0;
}


void LsShmTidMgr::clrTidTbl()
{
    LsShmTidInfo *pTidInfo = getTidInfo();
    LsShmOffset_t off = pTidInfo->x_iTidTblCurOff;
    while (off != 0)
    {
        LsShmOffset_t prev;
        prev = ((LsShmTidTblBlk *)m_pHash->offset2ptr(off))->x_iPrev;
        m_pHash->release2(off, (LsShmSize_t)sizeof(LsShmTidTblBlk));
        off = prev;
    }
    pTidInfo->x_iTidTblStrtOff = 0;
    pTidInfo->x_iTidTblCurOff = 0;
    pTidInfo->x_iBlkLastNoti = 0;
    if (m_pHash->isTidMaster())
    {
        // load flush_all notification
        uint64_t tid = 0;
        setTidTblEnt((uint64_t)TIDDEL_FLUSHALL, &tid);
//         LS_DBG_M(m_pHash->getLogger(), "clrTidTbl FLUSHALL: tid=%lld\n", tid);
    }
    return;
}


void LsShmTidMgr::linkTid(LsShmHIterOff offElem, uint64_t *pTid)
{
    LsShmTidLink *pLink;
    uint64_t tid;
    LsShmHElem *pLinkElem;
    if (pTid == NULL)
    {
        tid = 0;
        pTid = &tid;
    }
    setTidTblIter(offElem, pTid);    // careful, may remap
    pLinkElem = m_pHash->offset2iterator(offElem);
    pLink = (LsShmTidLink *)pLinkElem->getExtraPtr(m_iIterOffset);
    pLink->x_tid = *pTid;
}


void LsShmTidMgr::unlinkTid(LsShmHElem *pElem)
{
    LsShmTidLink *pLink = (LsShmTidLink *)pElem->getExtraPtr(m_iIterOffset);
    uint64_t tid = pLink->x_tid;
    clrTidTblEnt(tid);
    // new delete tid only processed here in master;
    // slave updates tid table when processing transactions with specified tids.
    if (m_pHash->isTidMaster())
    {
        uint64_t tidNew = 0;
        setTidTblDel(tid, &tidNew);  // careful, may remap
    }
}


LsShmHIterOff LsShmTidMgr::doSet(const void* pKey, int iKeyLen,
                                 const void* pVal, int iValLen)
{
    ls_strpair_t parms;
    LsShmHIterOff offElem;
    LsShmHKey hkey;
    m_pHash->setParms(&parms, pKey, iKeyLen, pVal, iValLen);
    offElem = m_pHash->findIterator(&parms);
    if (offElem.m_iOffset != 0)
    {
        LsShmHElem *pElem = m_pHash->offset2iterator(offElem);
        if (iValLen < pElem->getValLen())
        {
            if (pVal != NULL)
                memmove(pElem->getVal(), pVal, iValLen);
            pElem->setValLen(iValLen);
            unlinkTid(pElem);
            return offElem;
        }
        else
            m_pHash->eraseIterator(offElem);
    }
    hkey = (*m_pHash->getHashFn())(pKey, iKeyLen);
    return m_pHash->insertCopy(hkey, &parms);
}


LsShmOffset_t LsShmTidMgr::setIter(const void* pKey, int iKeyLen,
                                const void* pVal, int iValLen, uint64_t* pTid)
{
    LsShmHIterOff offElem;

    offElem = doSet(pKey, iKeyLen, pVal, iValLen);
    if (offElem.m_iOffset != 0)
    {
        linkTid(offElem, pTid);
        return LS_OK;
    }
    return LS_FAIL;
}


void LsShmTidMgr::delIter(LsShmHIterOff off)
{
    m_pHash->eraseIterator(off);
}


void LsShmTidMgr::insertIterCb(LsShmHIterOff off)
{
    linkTid(off, NULL);
}


void LsShmTidMgr::eraseIterCb(LsShmHElem* pElem)
{
    LsShmTidLink *pLink = (LsShmTidLink *)pElem->getExtraPtr(m_iIterOffset);
    if (pLink->x_tid != 0)
    {
        unlinkTid(pElem);
        pLink->x_tid = 0;
    }
}


void LsShmTidMgr::updateIterCb(LsShmHElem* pElem, LsShmHIterOff off)
{
    LsShmTidLink *pLink = (LsShmTidLink *)pElem->getExtraPtr(m_iIterOffset);
    if (pLink->x_tid != 0)
    {
        unlinkTid(pElem);
        pLink->x_tid = 0;
    }
    linkTid(off, NULL);
}


void LsShmTidMgr::clearCb()
{
    clrTidTbl();
}


int LsShmTidMgr::setTidTblEnt(uint64_t tidVal, uint64_t *pTid)
{
    int indx;
    LsShmTidInfo *pTidInfo = getTidInfo();
    if (*pTid == 0)
        *pTid = ++(pTidInfo->x_tid);
    else if (*pTid > pTidInfo->x_tid)
        pTidInfo->x_tid = *pTid;
    else
        return -1;

    LsShmTidTblBlk *pBlk =
        (LsShmTidTblBlk *)m_pHash->offset2ptr(pTidInfo->x_iTidTblCurOff);
    indx = (*pTid % TIDTBLBLK_MAXSZ);
    if ((pBlk == NULL) || (*pTid >= (pBlk->x_tidBase + TIDTBLBLK_MAXSZ)))
    {
        int remapped;
        LsShmOffset_t blkOff;
        if ((blkOff = m_pHash->alloc2(sizeof(LsShmTidTblBlk), remapped)) == 0)
            return -1;
        if (remapped)
        {
            pTidInfo = getTidInfo();
            pBlk = (LsShmTidTblBlk *)m_pHash->offset2ptr(pTidInfo->x_iTidTblCurOff);
        }
        if (pBlk == NULL)
            pTidInfo->x_iTidTblStrtOff = blkOff;
        else
            pBlk->x_iNext = blkOff;
        pBlk = (LsShmTidTblBlk *)m_pHash->offset2ptr(blkOff);
        pBlk->x_tidBase = *pTid - indx;
        pBlk->x_iIterCnt = 0;
        pBlk->x_iDelCnt = 0;
        pBlk->x_iNext = 0;
        pBlk->x_iPrev = pTidInfo->x_iTidTblCurOff;
        ::memset((void *)pBlk->x_iTidVals, 0, sizeof(pBlk->x_iTidVals));
        pTidInfo->x_iTidTblCurOff = blkOff;
    }
    if (isTidValIterOff(tidVal))
        ++pBlk->x_iIterCnt;
    else
    {
        ++pBlk->x_iDelCnt;
    }
    pBlk->x_iTidVals[indx] = tidVal;
    return 0;
}


void LsShmTidMgr::clrTidTblEnt(uint64_t tid)
{
    LsShmTidTblBlk *pBlk;
    if ((pBlk = tid2tblBlk(tid, NULL)) != NULL)
    {
        pBlk->x_iTidVals[tid % TIDTBLBLK_MAXSZ] = 0;
        --pBlk->x_iIterCnt;
    }
    return;
}


LsShmTidTblBlk *LsShmTidMgr::tid2tblBlk(uint64_t tid, LsShmTidTblBlk *pBlk)
{
    if ((pBlk = nxtTblBlkStrt(pBlk)) == NULL)
        return NULL;
    while (1)
    {
        LsShmOffset_t off;
        if (tid < pBlk->x_tidBase)
            break;
        if (tid < (pBlk->x_tidBase + TIDTBLBLK_MAXSZ))
            return pBlk;
        if ((off = pBlk->x_iNext) == 0)
            break;
        pBlk = (LsShmTidTblBlk *)m_pHash->offset2ptr(off);
    }
    return NULL;
}


uint64_t *LsShmTidMgr::nxtTidTblVal(uint64_t *pTid, void **ppBlk)
{
    uint64_t *pVal;
    int indx;
    LsShmOffset_t off;
    LsShmTidTblBlk *pBlk;
    if ((pBlk = nxtTblBlkStrt(*(LsShmTidTblBlk **)ppBlk)) == NULL)
        return NULL;
    while (1)
    {
        if (*pTid < (pBlk->x_tidBase + TIDTBLBLK_MAXSZ))
        {
            indx = ((*pTid >= pBlk->x_tidBase) ?
                (*pTid % TIDTBLBLK_MAXSZ) + 1 : 0);
            if ((pVal = nxtValInBlk(pBlk, &indx)) != NULL)
                break;
        }
        if ((off = pBlk->x_iNext) == 0)
            return NULL;
        pBlk = (LsShmTidTblBlk *)m_pHash->offset2ptr(off);
    }
    *pTid = pBlk->x_tidBase + indx;
    *ppBlk = (void *)pBlk;
    return pVal;
}


uint64_t *LsShmTidMgr::nxtValInBlk(LsShmTidTblBlk *pBlk, int *pIndx)
{
    int indx = *pIndx;
    uint64_t *pVal = &pBlk->x_iTidVals[indx];
    while (indx < TIDTBLBLK_MAXSZ)
    {
        if (*pVal != 0)
        {
            *pIndx = indx;
            return pVal;
        }
        ++pVal;
        ++indx;
    }
    return NULL;
}

