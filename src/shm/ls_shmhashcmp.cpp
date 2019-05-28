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

#include <shm/lsshmpool.h>
#include <shm/lsshmhash.h>

#include <util/stringtool.h>

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *g_pShmDefaultDirName = "/dev/shm/lslb";

static const char *g_pShmDirName1 = g_pShmDefaultDirName;
static const char *g_pShmName1 = NULL;
static const char *g_pHashName1 = NULL;

static const char *g_pShmDirName2 = g_pShmDefaultDirName;
static const char *g_pShmName2 = NULL;
static const char *g_pHashName2 = NULL;

static int g_iMaxFail = 10;

static bool g_dataCmp = false;

typedef struct
{
    LsShmHash *pHash;
    LsShmSize_t totalSize;
} MyStat;


char *argv0 = NULL;

#include <util/pool.h>
Pool g_pool;


LsShm *getShm(const char *pFile, const char *pDir);
LsShmHash *verifyHash(LsShm *pShm, const char *pHashName, int &mode, int &flags);
int cmpHashStat(LsShmHash *p1, LsShmHash *p2);
int cmpHashIter(LsShmHash *p1, LsShmHash *p2);
int cmpHashData(LsShmHash *p1, LsShmHash *p2);


void usage()
{
    fprintf(stderr,
            "usage: ls_shmhashcmp -shm1 shmfile -hash1 hashtable [ -dir1 dirname ] \
            -shm2 shmfile -hash2 hashtable [ -dir2 dirname ] [-c #] [--data]\n \
            -c indicates number of failures to print. Default 10. 0 for unlimited.\n \
            --data indicates doing a data comparison rather than iterating.\n \
            When doing a data comparison, the hash function used is XXHASH32, compare is memcmp.\n");
    return;
}


int getOptions(int ac, char *av[])
{
    int opt;
    int ret = 0;

    int iLongIdx;
    static struct option astLongOptions[] = 
    {
        {"shm1", required_argument, NULL, 's'},
        {"shm2", required_argument, NULL, 's'},
        {"dir1", required_argument, NULL, 'd'},
        {"dir2", required_argument, NULL, 'd'},
        {"hash1", required_argument, NULL, 'h'},
        {"hash2", required_argument, NULL, 'h'},
        {"data", no_argument,       NULL, 't'},
        {NULL, 0, NULL, 0}
    };
    while ((opt = getopt_long(ac, av, "c:", astLongOptions, &iLongIdx)) != -1)
    {
        switch (opt)
        {
        case 'c':
                g_iMaxFail = StringTool::atoll(optarg);
                break;
        case 'd':
            if (iLongIdx & 1)
                g_pShmDirName2 = optarg;
            else
                g_pShmDirName1 = optarg;
            break;
        case 'h':
            if (iLongIdx & 1)
                g_pHashName2 = optarg;
            else
                g_pHashName1 = optarg;
            break;
        case 's':
            if (iLongIdx & 1)
                g_pShmName2 = optarg;
            else
                g_pShmName1 = optarg;
            break;
        case 't':
            g_dataCmp = true;
            break;
        default:
            usage();
            return -1;
        }
    }
    if (g_pShmName1 == NULL || g_pShmName2 == NULL)
    {
        fprintf(stderr, "Missing mandatory shmfile.\n");
        ret = -1;
    }
    if (g_pHashName1 == NULL || g_pHashName2 == NULL)
    {
        fprintf(stderr, "Missing mandatory hashtable.\n");
        ret = -1;
    }
    if (ret < 0)
        usage();
    return ret;
}


int main(int ac, char *av[])
{
    LsShm *pShm1, *pShm2;
    LsShmHash *pHash1, *pHash2;
    int mode1, mode2, flag1, flag2;

    if (getOptions(ac, av) < 0)
        return 1;

    pShm1 = getShm(g_pShmName1, g_pShmDirName1);
    if (NULL == pShm1)
        return 2;

    pShm2 = getShm(g_pShmName2, g_pShmDirName2);
    if (NULL == pShm2)
        return 3;

    pHash1 = verifyHash(pShm1, g_pHashName1, mode1, flag1);
    if (NULL == pHash1)
        return 4;

    pHash2 = verifyHash(pShm2, g_pHashName2, mode2, flag2);
    if (NULL == pHash2)
        return 5;

    if (mode1 != mode2 || flag1 != flag2)
    {
        fprintf(stderr, "Mode [1: %d, 2: %d] or flag [1: %d, 2: %d] did not match.\n",
                mode1, mode2, flag1, flag2);
        return 6;
    }

    cmpHashStat(pHash1, pHash2);

    pHash1->lockEx();

    if (g_dataCmp)
    {
        if (0 == cmpHashData(pHash1, pHash2))
        {
            fprintf(stdout, "Everything in hash 1 is in hash 2.\n");
        }
        pHash1->unlockEx();
        pHash2->lockEx();
        if (0 == cmpHashData(pHash2, pHash1))
        {
            fprintf(stdout, "Everything in hash 2 is in hash 1.\n");
        }
    }
    else
    {
        pHash2->lockEx();
        if (0 == cmpHashIter(pHash1, pHash2))
        {
            fprintf(stdout, "Hashes are equivalent.\n");
        }
        pHash1->unlockEx();
    }

    pHash2->unlockEx();

    return 0;
}

LsShm *getShm(const char *pFile, const char *pDir)
{
    LsShm *pShm;
    char buf[2048];

    snprintf(buf, sizeof(buf), "%s/%s.%s", pDir, pFile, LSSHM_SYSSHM_FILE_EXT);
    if (access(buf, R_OK | W_OK) < 0)
    {
        fprintf(stderr, "Unable to access [%s], %s.\n", buf, strerror(errno));
        return NULL;
    }
    if ((pShm = LsShm::open(pFile, 0, pDir)) == NULL)
    {
        fprintf(stderr, "LsShm::open(%s/%s) FAILED!\n",
                pDir, pFile);
        fprintf(stderr, "%s\nstat=%d, errno=%d.\n",
                (char *)LsShm::getErrMsg(), LsShm::getErrStat(), LsShm::getErrNo());
        LsShm::clrErrMsg();
        return NULL;
    }
    return pShm;
}


LsShmHash *verifyHash(LsShm *pShm, const char *pHashName, int &mode, int &flags)
{
    LsShmPool *pGPool;
    LsShmReg *pReg;
    LsShmHash *pHash;
    if ((pGPool = pShm->getGlobalPool()) == NULL)
    {
        fprintf(stderr, "getGlobalPool() FAILED!\n");
        return NULL;
    }
    if ((pReg = pShm->findReg(pHashName)) == NULL)
    {
        fprintf(stderr, "Unable to find [%s] in registry!\n", pHashName);
        return NULL;
    }
    if (LsShmHash::chkHashTable(pShm, pReg, &mode, &flags) < 0)
    {
        fprintf(stderr, "Not a Hash Table [%s]!\n", pHashName);
        return NULL;
    }
    pHash = pGPool->getNamedHash(pHashName, 0,
                                      LsShmHash::hashXXH32,
                                      memcmp,
                                      flags);

    if (NULL == pHash)
    {
        fprintf(stderr, "No Hash Table [%s] found!\n", pHashName);
    }
    return pHash;
}


int cmpHashStat(LsShmHash *p1, LsShmHash *p2)
{
    LsShmHTableStat
        *pStat1 = (LsShmHTableStat *)p1->offset2ptr(p1->getHTableStatOffset()),
        *pStat2 = (LsShmHTableStat *)p2->offset2ptr(p2->getHTableStatOffset());

    if (pStat1->m_iHashInUse != pStat2->m_iHashInUse)
    {
        fprintf(stderr, "SHMHASH Current total hash memory mismatch %s:%u, %s:%u!\n",
                p1->name(), pStat1->m_iHashInUse, p2->name(), pStat2->m_iHashInUse);
        return 1;
    }
    fprintf(stdout, "SHMHASH current total hash memory: %u\n",
            pStat1->m_iHashInUse);
    return 0;
}


void printMismatch(int iter, const char *pKey, const char *pStr1, int iStr1Len,
        const char *pStr2, int iStr2Len)
{
    char achDump1[(iStr1Len+1)*2];
    char achDump2[(iStr2Len+1)*2];

    int iDump1Len = StringTool::hexEncode(pStr1, iStr1Len, achDump1);
    int iDump2Len = StringTool::hexEncode(pStr2, iStr2Len, achDump2);

    fprintf(stdout, "[iter:%d] %s mismatch, hex dump 1:\n%.*s\n\nhex dump 2:\n%.*s\n",
            iter, pKey, iDump1Len, achDump1, iDump2Len, achDump2);
}


int cmpHashIter(LsShmHash *p1, LsShmHash *p2)
{
    int failed, failures = 0;
    int n = 0;
    LsShmHash::iteroffset iterNext1 = p1->begin(), iterNext2 = p2->begin();
    LsShmHash::iteroffset iterOff1, iterOff2;
    LsShmHash::iterator iter1, iter2;
    while (iterNext1.m_iOffset != 0 && iterNext2.m_iOffset != 0)
    {
        failed = 0;
        iterOff1 = iterNext1;
        iterOff2 = iterNext2;
        iterNext1 = p1->next(iterNext1);
        iterNext2 = p2->next(iterNext2);

        if (iterOff1.m_iOffset != iterOff2.m_iOffset)
        {
            fprintf(stdout, "[iter:%d] Offsets do not match. 1: %u, 2: %u\n",
                    n, iterOff1.m_iOffset, iterOff2.m_iOffset);
            failed = 1;
        }

        iter1 = p1->offset2iterator(iterOff1);
        iter2 = p2->offset2iterator(iterOff2);

        if ((iter1->getKeyLen() != iter2->getKeyLen())
            || memcmp(iter1->getKey(), iter2->getKey(), iter1->getKeyLen()))
        {
            printMismatch(n, "key", (const char *)iter1->getKey(), iter1->getKeyLen(),
                    (const char *)iter2->getKey(), iter2->getKeyLen());
            failed = 1;
        }
        if ((iter1->getValLen() != iter2->getValLen())
            || memcmp(iter1->getVal(), iter2->getVal(), iter1->getValLen()))
        {
            printMismatch(n, "val", (const char *)iter1->getVal(), iter1->getValLen(),
                    (const char *)iter2->getVal(), iter2->getValLen());
            failed = 1;
        }

        if (iter1->getLruLinkNext().m_iOffset != iter2->getLruLinkNext().m_iOffset)
        {
            fprintf(stdout, "[iter:%d] LRU Next mismatch 1: %u, 2: %u\n", n,
                    iter1->getLruLinkNext().m_iOffset,
                    iter2->getLruLinkNext().m_iOffset);
            failed = 1;
        }
        if (iter1->getLruLinkPrev().m_iOffset != iter2->getLruLinkPrev().m_iOffset)
        {
            fprintf(stdout, "[iter:%d] LRU Prev mismatch 1: %u, 2: %u\n", n,
                    iter1->getLruLinkPrev().m_iOffset,
                    iter2->getLruLinkPrev().m_iOffset);
            failed = 1;
        }
        if (iter1->getLruLasttime() != iter2->getLruLasttime())
        {
            fprintf(stdout, "[iter:%d] LRU Lasttime mismatch 1: %lu, 2: %lu\n", n,
                    iter1->getLruLasttime(),
                    iter2->getLruLasttime());
            failed = 1;
        }

        if (failed)
            ++failures;

        if (g_iMaxFail > 0 && failures >= g_iMaxFail)
        {
            fprintf(stderr, "[iter:%d] Too many failures, stop iterating.\n", n);
            return failures;
        }
        ++n;
    }
    if ((iterNext1.m_iOffset == 0) != (iterNext2.m_iOffset == 0))
    {
        fprintf(stdout, "[iter:%d] One hash is done, the other is not. 1: %u, 2: %u\n",
                n, iterOff1.m_iOffset, iterOff2.m_iOffset);
        ++failures;
    }
    return failures;
}


int cmpHashData(LsShmHash *p1, LsShmHash *p2)
{
    int failed, failures = 0;
    int n = 0;
    LsShmHash::iteroffset iterNext = p1->begin();
    LsShmHash::iteroffset iterOff1, iterOff2;
    LsShmHash::iterator iter1, iter2;
    ls_strpair_t parms;
    while (iterNext.m_iOffset != 0)
    {
        failed = 0;
        iterOff1 = iterNext;
        iterNext = p1->next(iterNext);
        iter1 = p1->offset2iterator(iterOff1);

        parms.key.ptr = (char *)iter1->getKey();
        parms.key.len = iter1->getKeyLen();
        iterOff2 = p2->findIterator(&parms);

        if (0 == iterOff2.m_iOffset)
        {
            printMismatch(n, "Item in hash 1 not in hash 2.",
                    (const char *)iter1->getKey(), iter1->getKeyLen(),
                    (const char *)iter1->getVal(), iter1->getValLen());
            failed = 1;
        }
        else
        {
            iter2 = p2->offset2iterator(iterOff2);

            if ((iter1->getKeyLen() != iter2->getKeyLen())
                || memcmp(iter1->getKey(), iter2->getKey(), iter1->getKeyLen()))
            {
                printMismatch(n, "key", (const char *)iter1->getKey(), iter1->getKeyLen(),
                        (const char *)iter2->getKey(), iter2->getKeyLen());
                failed = 1;
            }
            if ((iter1->getValLen() != iter2->getValLen())
                || memcmp(iter1->getVal(), iter2->getVal(), iter1->getValLen()))
            {
                printMismatch(n, "val", (const char *)iter1->getVal(), iter1->getValLen(),
                        (const char *)iter2->getVal(), iter2->getValLen());
                failed = 1;
            }
            // With a data check, only care about key/val
            // if (iter1->getLruLinkNext().m_iOffset != iter2->getLruLinkNext().m_iOffset)
            // {
            //     fprintf(stdout, "[iter:%d] LRU Next mismatch 1: %u, 2: %u\n", n,
            //             iter1->getLruLinkNext().m_iOffset,
            //             iter2->getLruLinkNext().m_iOffset);
            //     failed = 1;
            // }
            // if (iter1->getLruLinkPrev().m_iOffset != iter2->getLruLinkPrev().m_iOffset)
            // {
            //     fprintf(stdout, "[iter:%d] LRU Prev mismatch 1: %u, 2: %u\n", n,
            //             iter1->getLruLinkPrev().m_iOffset,
            //             iter2->getLruLinkPrev().m_iOffset);
            //     failed = 1;
            // }
            // if (iter1->getLruLasttime() != iter2->getLruLasttime())
            // {
            //     fprintf(stdout, "[iter:%d] LRU Lasttime mismatch 1: %lu, 2: %lu\n", n,
            //             iter1->getLruLasttime(),
            //             iter2->getLruLasttime());
            //     failed = 1;
            // }
        }

        if (failed)
            ++failures;

        if (g_iMaxFail > 0 && failures >= g_iMaxFail)
        {
            fprintf(stderr, "[iter:%d] Too many failures, stop iterating.\n", n);
            return failures;
        }
        ++n;
    }

    return failures;
}
