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
#ifdef RUN_TEST

#include <shm/lsshmpool.h>
#include <shm/lsshmhash.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

static const char *g_pShmDirName = LsShmLock::getDefaultShmDir();
static const char *g_pShmName = "SHMXTEST";
static const char *g_pPool1Name = "XPOOL1";
static const char *g_pPool2Name = "XPOOL2";
static const char *g_pHashName = "XPOOLHASH";

#define SZ_TESTBCKT     32
#define SZ_TESTLIST     256
#define SZ_LISTMIN      256

typedef struct xxx_s {
    int x[10];
} xxx_t;


TEST(shmPerProcess_test)
{
    char achShmFileName[255];
    char achLockFileName[255];
    snprintf(achShmFileName, sizeof(achShmFileName), "%s/%s.shm",
             g_pShmDirName, g_pShmName);
    snprintf(achLockFileName, sizeof(achLockFileName), "%s/%s.lock",
             g_pShmDirName, g_pShmName);
    unlink(achShmFileName);
    unlink(achLockFileName);

    fprintf(stdout, "shmperprocesstest: [%s/%s,%s/%s]\n",
            g_pShmName, g_pPool1Name, g_pPool2Name, g_pHashName);

    LsShm *pShm;
    LsShmReg *pReg;
    LsShmPool *pGPool;
    LsShmPool *pPool1;
    LsShmPool *pPool2;
    LsShmHash *pHash1;
    LsShmHash *pHash2;
    LsShmOffset_t off0;
    LsShmOffset_t off1;
    LsShmOffset_t off2;
    LsShmOffset_t off3;
    const char *pMsg;

    CHECK((pShm = LsShm::open(g_pShmName, 0, "/etc")) == NULL);
    pMsg = LsShm::getErrMsg();
    CHECK(*pMsg != '\0');
    printf("pShm=%p, Expected Msg: [%s], stat=%d, errno=%d.\n",
           pShm, pMsg, LsShm::getErrStat(), LsShm::getErrNo());
    LsShm::clrErrMsg();
    CHECK(*pMsg == '\0');

    CHECK((pShm = LsShm::open(g_pShmName, 0, g_pShmDirName)) != NULL);
    if (unlink(achShmFileName) != 0)
        perror(achShmFileName);
    if (unlink(achLockFileName) != 0)
        perror(achLockFileName);
    if (pShm == NULL)
        return;
    CHECK(pShm->recoverOrphanShm() == 0);
    for (int i = 1; i <= 64; ++i)
    {
        char acName[12];
        snprintf(acName, sizeof(acName), "X%03dX", i);
        CHECK((pReg = pShm->addReg(acName)) != NULL);
        if (pReg == NULL)
            return;
        pReg->x_iValue = pReg->x_iRegNum + 1000;
    }
    CHECK(pShm->clrReg(33) == 0);
    CHECK(pShm->clrReg(33) == -1);
    CHECK(pShm->clrReg(90) == -1);
    CHECK((pReg = pShm->addReg((char *)"NEW33")) != NULL);
    pReg->x_iValue = 333;
    CHECK((pReg = pShm->addReg((char *)"NEW44")) != NULL);
    pReg->x_iValue = 4444;
    CHECK((pReg = pShm->getReg(0)) != NULL);
    CHECK((pReg = pShm->getReg(49)) != NULL);
    CHECK((pReg = pShm->getReg(50)) != NULL);
    CHECK(pShm->getReg(150) == NULL);
    CHECK((pReg = pShm->findReg((char *)"NEW33")) != NULL);
    CHECK(pReg->x_iRegNum == 33);
    CHECK(pReg->x_iValue == 333);

    CHECK((pGPool = pShm->getGlobalPool()) != NULL);
    if (pGPool == NULL)
        return;
    CHECK((pPool1 = pShm->getNamedPool(g_pPool1Name)) != NULL);
    if (pPool1 == NULL)
        return;
    CHECK((pPool2 = pShm->getNamedPool(g_pPool2Name)) != NULL);
    if (pPool2 == NULL)
        return;

    CHECK((pHash1 = pPool1->getNamedHash(g_pHashName, 0, NULL, NULL, LSSHM_LRU_NONE)) != NULL);
    if (pHash1 == NULL)
        return;
    const void *pKey = (const void *)0x11223344;
    int val = 0x01020304;
    CHECK(pHash1->insert(pKey, 0, (const void *)&val, sizeof(val)) != 0);
    CHECK(pHash1->insert(pKey, 0, (const void *)&val, sizeof(val)) == 0); // dup
    CHECK((pHash2 = pPool2->getNamedHash(g_pHashName, 0, NULL, NULL, LSSHM_LRU_NONE)) != NULL);
    if (pHash2 == NULL)
        return;

    // shmhash template test
    const char aKey[] = "tmplKey";
    const int iKeyLen = sizeof(aKey) - 1;
    xxx_t xxx;
    TShmHash<xxx_t> *pTHash;
    ls_str_pair_t parms;
    LsShmOffset_t off;
    int iValLen;
    int ret;

    pTHash = (TShmHash <xxx_t> *)pGPool->getNamedHash(
      "tmplHash", 0, LsShmHash::hashXXH32, LsShmHash::compBuf, LSSHM_LRU_NONE);
    xxx.x[0] = 0x1234;
    CHECK(pTHash->update(aKey, iKeyLen, &xxx) == 0);
    CHECK((off = pTHash->get(aKey, iKeyLen, &iValLen, &ret)) != 0);
    CHECK(iValLen == sizeof(xxx));
    CHECK(ret == LSSHM_FLAG_CREATED);
    CHECK(pTHash->update(aKey, iKeyLen, &xxx) == off);
    CHECK(pTHash->insert(aKey, iKeyLen, &xxx) == 0);
    CHECK(pTHash->find(aKey, iKeyLen, &ret) == off);
    CHECK(ret == sizeof(xxx));

    xxx.x[0] = 0x5678;
    ls_str_set(&parms.key, (char *)aKey, iKeyLen);
    ls_str_set(&parms.value, (char *)&xxx, sizeof(xxx));
    CHECK(pTHash->insertIterator(&parms) == 0);
    CHECK((off = pTHash->getIterator(&parms, &ret)) != 0);
    CHECK(ret == LSSHM_FLAG_NONE);
    if (off != 0)
    {
        TShmHash<xxx_t>::iterator it(pTHash->offset2iterator(off));
        CHECK(memcmp(it.first(), aKey, iKeyLen) == 0);
        CHECK(((xxx_t *)it.second())->x[0] == 0x1234);
        CHECK(pTHash->setIterator(&parms) == off);
        CHECK(pTHash->findIterator(&parms) == off);
        CHECK(((xxx_t *)it.second())->x[0] == 0x5678);
    }

    int remap = 0;
    CHECK((off0 = pGPool->alloc2(SZ_TESTBCKT, remap)) != 0);
    CHECK((off1 = pPool1->alloc2(SZ_TESTBCKT, remap)) != 0);
    CHECK((off2 = pPool1->alloc2(SZ_TESTBCKT, remap)) != 0);
    CHECK((off3 = pPool1->alloc2(SZ_TESTBCKT, remap)) != 0);
    if ((off0 == 0) || (off1 == 0) || (off2 == 0) || (off3 == 0))
        return;

    pPool2->release2(off1, SZ_TESTBCKT);
    pPool2->mvFreeBucket();
    CHECK((off0 = pGPool->alloc2(SZ_TESTBCKT, remap)) == off1);

    pPool2->release2(off0, SZ_TESTBCKT);
    pPool2->release2(off2, SZ_TESTBCKT);
    pPool2->mvFreeBucket();
    CHECK((off0 = pGPool->alloc2(SZ_TESTBCKT, remap)) == off2);

    pPool2->release2(off0, SZ_TESTBCKT);
    pPool2->release2(off3, SZ_TESTBCKT);
    pPool2->mvFreeBucket();
    CHECK((off0 = pGPool->alloc2(SZ_TESTBCKT, remap)) == off3);

    pPool1->mvFreeList();
    CHECK((off0 = pGPool->alloc2(SZ_TESTLIST, remap)) != 0);
    CHECK((off1 = pPool1->alloc2(SZ_TESTLIST, remap)) != 0);
    CHECK((off2 = pPool1->alloc2(SZ_TESTLIST, remap)) != 0);
    CHECK((off3 = pPool1->alloc2(SZ_TESTLIST, remap)) != 0);
    if ((off0 == 0) || (off1 == 0) || (off2 == 0) || (off3 == 0))
        return;

    pPool2->release2(off1, SZ_TESTLIST);
    pPool2->mvFreeList();

    CHECK(pGPool->alloc2(SZ_TESTLIST, remap) == off1);
    pPool2->release2(off2, SZ_TESTLIST);
    pPool2->release2(off3, SZ_TESTLIST);
    pPool2->mvFreeList();

    CHECK(pGPool->alloc2(SZ_TESTLIST, remap) == off3);

    CHECK(pShm->findReg(g_pPool1Name) != NULL);
    CHECK((off1 = pPool1->alloc2(SZ_TESTBCKT, remap)) != 0);
    CHECK(pShm->recoverOrphanShm() == 0);
    pPool1->destroyShm();
    CHECK(pShm->findReg(g_pPool1Name) == NULL);
    pPool1->release2(off1, SZ_TESTBCKT);
    pPool1->close();
    pPool2->close();
    CHECK(pShm->findReg(g_pPool2Name) == NULL);
    CHECK(pGPool->alloc2(SZ_TESTBCKT, remap) == off1);
    CHECK(pShm->recoverOrphanShm() == 0);

    // shm statistics
    int cnt;
    LsShmSize_t acnt;
    LsShmSize_t rcnt;
    off0 = pGPool->alloc2(1*LSSHM_SHM_UNITSIZE, remap);
    off1 = pGPool->alloc2(2*LSSHM_SHM_UNITSIZE+1, remap);
    off2 = pGPool->alloc2(2*LSSHM_SHM_UNITSIZE, remap);
    off3 = pGPool->alloc2(1*LSSHM_SHM_UNITSIZE, remap);
    acnt = pGPool->roundSize2pages(1*LSSHM_SHM_UNITSIZE)
        + pGPool->roundSize2pages(2*LSSHM_SHM_UNITSIZE+1)
        + pGPool->roundSize2pages(2*LSSHM_SHM_UNITSIZE)
        + pGPool->roundSize2pages(1*LSSHM_SHM_UNITSIZE);
    rcnt = 0;

    LsShmMapStat *pStat =
        (LsShmMapStat *)pShm->offset2ptr(pShm->getMapStatOffset());
    LsShmPoolMapStat *pStat2 =
        (LsShmPoolMapStat *)pShm->offset2ptr(pGPool->getPoolMapStatOffset());
    // in real life, should always use offset2ptr when accessing.
    // in this case, for simplicity, use same pointer, assume no remapping.

    cnt = pStat->m_iAllocated;
    pGPool->release2(off0, 1*LSSHM_SHM_UNITSIZE);
    pGPool->release2(off2, 2*LSSHM_SHM_UNITSIZE);
    rcnt += pGPool->roundSize2pages(3*LSSHM_SHM_UNITSIZE);
    CHECK(pStat->m_iReleased == 3);     // 3 blocks
    CHECK(pStat->m_iFreeListCnt == 2);  // 2 entries
    CHECK(pStat2->m_iShmAllocated == acnt);
    CHECK(pStat2->m_iShmReleased == rcnt);
    pGPool->release2(off1, 2*LSSHM_SHM_UNITSIZE+1);
    rcnt += pGPool->roundSize2pages(2*LSSHM_SHM_UNITSIZE+1);
    CHECK(pStat->m_iReleased == 6);     // 6 blocks
    CHECK(pStat->m_iFreeListCnt == 1);  // 1 entry
    CHECK(pStat2->m_iShmReleased == rcnt);
    pGPool->alloc2(2*LSSHM_SHM_UNITSIZE, remap); // alloc 2 blocks from shm freelist
    acnt += pGPool->roundSize2pages(2*LSSHM_SHM_UNITSIZE);
    cnt += 2;
    CHECK(pStat->m_iAllocated == (LsShmSize_t)cnt);
    CHECK(pStat->m_iFreeListCnt == 1);  // still 1 entry
    CHECK(pStat2->m_iShmAllocated == acnt);

    // shmpool freelist
    acnt = pStat2->m_iFlAllocated;      // bytes
    rcnt = pStat2->m_iFlReleased;
    off0 = pGPool->alloc2(2*SZ_TESTLIST, remap);
    acnt += (2*SZ_TESTLIST);
    int diff = (int)(pStat2->m_iFlAllocated - acnt);
    if ((diff > 0) && (diff < SZ_LISTMIN))  // might move residual to bucket
        acnt += diff;
    CHECK(pStat2->m_iFlAllocated == acnt);
    cnt = pStat2->m_iFlCnt;
    pGPool->release2(off0, 2*SZ_TESTLIST);
    rcnt += (2*SZ_TESTLIST);
    CHECK(pStat2->m_iFlReleased == rcnt);
    CHECK(pStat2->m_iFlCnt == (LsShmSize_t)(cnt + 1));
    pGPool->alloc2(SZ_TESTLIST, remap); // piece of a freelist block
    acnt += SZ_TESTLIST;
    CHECK(pStat2->m_iFlAllocated == acnt);
    CHECK(pStat2->m_iFlReleased == rcnt);
    CHECK(pStat2->m_iFlCnt == (LsShmSize_t)(cnt + 1));
    pGPool->alloc2(SZ_TESTLIST, remap); // remainder of freelist block
    acnt += SZ_TESTLIST;
    CHECK(pStat2->m_iFlAllocated == acnt);
    CHECK(pStat2->m_iFlCnt == (LsShmSize_t)cnt);

    // shmpool bucket
    acnt = pStat2->m_bckt[SZ_TESTBCKT/LSSHM_POOL_UNITSIZE].m_iBkAllocated;
    off0 = pGPool->alloc2(SZ_TESTBCKT, remap);
    ++acnt;
    CHECK(pStat2->m_bckt[SZ_TESTBCKT/LSSHM_POOL_UNITSIZE].m_iBkAllocated == acnt);
    rcnt = pStat2->m_bckt[SZ_TESTBCKT/LSSHM_POOL_UNITSIZE].m_iBkReleased;
    pGPool->release2(off0, SZ_TESTBCKT);
    ++rcnt;
    CHECK(pStat2->m_bckt[SZ_TESTBCKT/8].m_iBkReleased == rcnt);
}

#endif
