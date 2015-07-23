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

#include <stdio.h>
#include <string.h>
#include <unistd.h>


static const char *g_pShmDirName = LsShm::getDefaultShmDir();
static const char *g_pShmName = NULL;


char *argv0 = NULL;


void doStatShm(LsShm *pShm);
void doStatShmPool(LsShmPool *pPool);



LsShmSize_t blks2kbytes(LsShmSize_t blks)
{   return blks * LSSHM_SHM_UNITSIZE / 1024;  }


void usage()
{
    fprintf(stderr, "usage: ls_shmstat -f shmname [ -d dirname ]\n");
    return;
}


int getOptions(int ac, char *av[])
{
    int opt;
    while ((opt = getopt(ac, av, "f:d:")) != -1)
    {
        switch (opt)
        {
        case 'f':
            g_pShmName = optarg;
            break;
        case 'd':
            g_pShmDirName = optarg;
            break;
        default:
            usage();
            return -1;
        }
    }
    if (g_pShmName == NULL)
    {
        fprintf(stderr, "Missing mandatory shmname.\n");
        usage();
        return -1;
    }
    return 0;
}


int main(int ac, char *av[])
{
    LsShm *pShm;
    LsShmPool *pGPool;
    char buf[2048];

    if (getOptions(ac, av) < 0)
        return 1;

    snprintf(buf, sizeof(buf), "%s/%s.%s",
             g_pShmDirName, g_pShmName, LSSHM_SYSSHM_FILE_EXT);
    if (access(buf, R_OK | W_OK) < 0)
    {
        fprintf(stderr, "Unable to access [%s], %s.\n", buf, strerror(errno));
        return 2;
    }
    if ((pShm = LsShm::open(g_pShmName, 0, g_pShmDirName)) == NULL)
    {
        fprintf(stderr, "LsShm::open(%s/%s) FAILED!\n",
                g_pShmDirName, g_pShmName);
        fprintf(stderr, "%s\nstat=%d, errno=%d.\n",
                (char *)LsShm::getErrMsg(), LsShm::getErrStat(), LsShm::getErrNo());
        LsShm::clrErrMsg();
        return 1;
    }
    if ((pGPool = pShm->getGlobalPool()) == NULL)
    {
        fprintf(stderr, "getGlobalPool() FAILED!\n");
        return 2;
    }

    doStatShm(pShm);

    doStatShmPool(pGPool);

    return 0;
}


void doStatShm(LsShm *pShm)
{
    LsShmMapStat *pStat =
        (LsShmMapStat *)pShm->offset2ptr(pShm->getMapStatOffset());

    fprintf(stdout, "SHM [%s] (in kbytes unless otherwise specified)\n\
shm filesize:               %lu\n\
currently used:             %lu\n\
available before expanding: %lu\n\
total allocated:            %lu\n\
total released:             %lu\n\
freelist elements (count):  %lu\n",
            pShm->name(),
            (unsigned long)(pStat->m_iFileSize / 1024),
            (unsigned long)(pStat->m_iUsedSize / 1024),
            (unsigned long)((pStat->m_iFileSize - pStat->m_iUsedSize) / 1024),
            (unsigned long)blks2kbytes(pStat->m_iAllocated),
            (unsigned long)blks2kbytes(pStat->m_iReleased),
            (unsigned long)pStat->m_iFreeListCnt
           );
    return;
}


void doStatShmPool(LsShmPool *pPool)
{
    LsShmPoolMapStat *pStat =
        (LsShmPoolMapStat *)pPool->offset2ptr(pPool->getPoolMapStatOffset());

    fprintf(stdout, "GLOBAL POOL (in kbytes unless otherwise specified)\n\
direct shm allocated (>1K):      %lu\n\
direct shm released  (>1K):      %lu\n\
allocated from freelist (bytes): %lu\n\
released to freelist (bytes):    %lu\n\
freelist elements (count):       %lu\n\
buckets (<256):  allocated         released             free(count)\n",
            (unsigned long)pStat->m_iShmAllocated,
            (unsigned long)pStat->m_iShmReleased,
            (unsigned long)pStat->m_iFlAllocated,
            (unsigned long)pStat->m_iFlReleased,
            (unsigned long)pStat->m_iFlCnt
           );
    LsShmSize_t poolFree = pStat->m_iFlReleased - pStat->m_iFlAllocated
                           + pStat->m_iFreeChunk;
    LsShmSize_t bcktsz = 0;
    struct LsShmPoolMapStat::bcktstat *p = &pStat->m_bckt[0];
    int i;
    for (i = 0; i < LSSHM_POOL_NUMBUCKET;
         ++i, ++p, bcktsz += LSSHM_POOL_BCKTINCR)
    {
        int num;
        if ((p->m_iBkAllocated == 0) && (p->m_iBkReleased == 0))
            continue;
        fprintf(stdout, "%5d %20lu %16lu",
                (unsigned int)bcktsz,
                (unsigned long)p->m_iBkAllocated,
                (unsigned long)p->m_iBkReleased
               );
        if ((num = (p->m_iBkReleased - p->m_iBkAllocated)) > 0)
        {
            fprintf(stdout, " %16u\n", num);
            poolFree += (num * bcktsz);
        }
        else
            fputc('\n', stdout);
    }
    fprintf(stdout, "\
current direct shm pool used (kbytes): %lu\n\
current total pool used (bytes): %lu\n\
current total pool free (bytes): %lu\n",
            (unsigned long)(pStat->m_iShmAllocated - pStat->m_iShmReleased),
            (unsigned long)pStat->m_iPoolInUse,
            (unsigned long)poolFree
           );
    return;
}

