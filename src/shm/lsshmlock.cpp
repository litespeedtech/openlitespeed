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
#include <shm/lsshmlock.h>

#include <shm/lsshm.h>
#include <util/gpath.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

#ifdef x_iMaxSize
#undef x_iMaxSize
#endif

extern "C" {
    int ls_expandfile(int fd, size_t fromsize, size_t incrsize);
};

uint32_t LsShmLock::s_iPageSize = LSSHM_PAGESIZE;
uint32_t LsShmLock::s_iHdrSize  = ((sizeof(LsShmLockMap) + 0xf) &
                                   ~0xf); // align 16

LsShmLock::LsShmLock(const char *dirName, const char *mapName,
                     LsShmSize_t size)
    : m_iMagic(LSSHM_LOCK_MAGIC)
    , m_status(LSSHM_NOTREADY)
    , m_pFileName(NULL)
    , m_pMapName(NULL)
    , m_iFd(0)
    , m_pShmLockMap(NULL)
    , m_pShmLockElem(NULL)
    , m_iMaxSizeO(0)
{
    char buf[0x1000];
    if (dirName == NULL)
        dirName = getDefaultShmDir();
    if (mapName == NULL)
        mapName = LSSHM_SYSSHM_FILENAME;

    snprintf(buf, sizeof(buf), "%s/%s.%s",
             dirName, mapName, LSSHM_SYSLOCK_FILE_EXT);

    m_pFileName = strdup(buf);
    m_pMapName = strdup(mapName);

#if 0
    m_status = LSSHM_NOTREADY;
    m_iFd = 0;
    m_pShmLockMap = NULL;
    m_pShmLockElem = NULL;
    m_iMaxSizeO = 0;
#endif

    // set the size
    size = ((size + s_iPageSize - 1) / s_iPageSize) * s_iPageSize;
    if ((m_pFileName == NULL)
        || (m_pMapName == NULL)
        || (strlen(m_pMapName) >= LSSHM_MAXNAMELEN))
    {
        m_status = LSSHM_BADPARAM;
        return;
    }

    m_status = init(m_pMapName, size);

    if (m_status == LSSHM_NOTREADY)
        m_status = LSSHM_READY;
#if 0
    // destructor should take care of this
    else
        cleanup();
#endif
}


LsShmLock::~LsShmLock()
{
    cleanup();
}


void LsShmLock::deleteFile()
{
    if (m_pFileName != NULL)
    {
        unlink(m_pFileName);
    }
}


const char *LsShmLock::getDefaultShmDir()
{
    static int isDirTected = 0;
    if (isDirTected == 0)
    {
        const char *pathname = "/dev/shm";
        struct stat sb;
        isDirTected =
            ((stat(pathname, &sb) == 0) && S_ISDIR(sb.st_mode)) ? 1 : 2;
    }

    return (isDirTected == 1) ? LSSHM_SYSSHM_DIR1 : LSSHM_SYSSHM_DIR2;
}


lsi_shmlock_t *LsShmLock::allocLock()
{
    LsShmLockElem *pElem;
    LsShmOffset_t offset;
    if ((offset = getShmLockMap()->x_iFreeOffset) == 0)
        return NULL;

    pElem = m_pShmLockElem + offset;
    m_pShmLockMap->x_iFreeOffset = pElem->x_iNext;
    return (lsi_shmlock_t *)pElem;
}


int LsShmLock::freeLock(lsi_shmlock_t *pLock)
{
    int num = pLock2LockNum(pLock);
    LsShmLockElem *pElem;

    pElem = m_pShmLockElem + num;
    pElem->x_iNext = getShmLockMap()->x_iFreeOffset;
    getShmLockMap()->x_iFreeOffset = num;
    return 0;
}


LsShmStatus_t LsShmLock::expand(LsShmSize_t size)
{
    LsShmSize_t xsize = getShmLockMap()->x_iMaxSize;

    if (expandFile(xsize, size) != LSSHM_OK)
        return LSSHM_ERROR;
    unmap();
    if (map(xsize + size) != LSSHM_OK)
        return LSSHM_ERROR;
    getShmLockMap()->x_iMaxSize = xsize + size;

    return LSSHM_OK;
}


LsShmStatus_t LsShmLock::expandFile(LsShmOffset_t from,
                                    LsShmOffset_t incrSize)
{
    return (ls_expandfile(m_iFd, from,
                          incrSize) < 0) ?  LSSHM_ERROR : LSSHM_OK;
}


void LsShmLock::cleanup()
{
    unmap();
    if (m_iFd > 0)
    {
        close(m_iFd);
        m_iFd = 0;
    }
    if (m_pFileName != NULL)
    {
        free(m_pFileName);
        m_pFileName = NULL;
    }
    if (m_pMapName == NULL)
    {
        free(m_pMapName);
        m_pMapName = NULL;
    }
}


LsShmStatus_t LsShmLock::checkMagic(LsShmLockMap *mp,
                                    const char *mName) const
{
    return ((mp->x_iMagic != m_iMagic)
            || (strncmp((char *)mp->x_aName, mName, LSSHM_MAXNAMELEN) != 0)) ?
           LSSHM_BADMAPFILE : LSSHM_OK;
}


LsShmStatus_t LsShmLock::init(const char *name, LsShmSize_t size)
{
    struct stat mystat;

    if ((m_iFd = open(m_pFileName, O_RDWR | O_CREAT, 0750)) < 0)
    {
        if ((GPath::createMissingPath(m_pFileName, 0750) < 0)
            || ((m_iFd = open(m_pFileName, O_RDWR | O_CREAT, 0750)) < 0))
        {
            LsShm::setErrMsg(LSSHM_SYSERROR, "Unable to open/create [%s], %s.",
                             m_pFileName, strerror(errno));
            return LSSHM_BADMAPFILE;
        }
    }

    ::fcntl( m_iFd, F_SETFD, FD_CLOEXEC );

    if (stat(m_pFileName, &mystat) < 0)
    {
        LsShm::setErrMsg(LSSHM_SYSERROR, "Unable to stat [%s], %s.",
                         m_pFileName, strerror(errno));
        return LSSHM_BADMAPFILE;
    }

    if (mystat.st_size == 0)
    {
        // creating a new map
        if ((expandFile(0, roundToPageSize(size)) != LSSHM_OK)
            || (map(size) != LSSHM_OK))
            return LSSHM_ERROR;
        getShmLockMap()->x_iMagic = m_iMagic;
        getShmLockMap()->x_iMaxSize = size;
        getShmLockMap()->x_iFreeOffset = 0;
        getShmLockMap()->x_iMaxElem = 0;
        getShmLockMap()->x_iElemSize = sizeof(LsShmRegElem);
        strcpy((char *)getShmLockMap()->x_aName, name);

        setupFreeList(size);
    }
    else
    {
        if (mystat.st_size < s_iHdrSize)
        {
            LsShm::setErrMsg(LSSHM_BADMAPFILE, "Bad LockFile format [%s], size=%lld.",
                      m_pFileName, (uint64_t)mystat.st_size);
            return LSSHM_BADMAPFILE;
        }

        // only map the header size to ensure the file is good.
        if (map(s_iHdrSize) != LSSHM_OK)
            return LSSHM_ERROR;

        if ((getShmLockMap()->x_iMaxSize != mystat.st_size)
            || (checkMagic(getShmLockMap(), name) != LSSHM_OK))
        {
            LsShm::setErrMsg(LSSHM_BADVERSION,
                      "Bad LockFile format [%s], size=%lld, magic=%08X(%08X).",
                      m_pFileName, (uint64_t)mystat.st_size, getShmLockMap()->x_iMagic, m_iMagic);
            return LSSHM_BADVERSION;
        }

        // expand the file if needed... won't shrink
        if (size > getShmLockMap()->x_iMaxSize)
        {
            int maxSize = getShmLockMap()->x_iMaxSize;
            if (expandFile(maxSize, size) != LSSHM_OK)
                return LSSHM_ERROR;
            getShmLockMap()->x_iMaxSize = size;
            unmap();
            if (map(size) != LSSHM_OK)
                return LSSHM_ERROR;
            setupFreeList(size);
        }
    }
    return LSSHM_OK;
}


LsShmStatus_t LsShmLock::map(LsShmSize_t size)
{
    uint8_t *p =
        (uint8_t *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_iFd, 0);
    if (p == MAP_FAILED)
    {
        LsShm::setErrMsg(LSSHM_SYSERROR, "Unable to mmap [%s], size=%d, %s.",
                  m_pFileName, size, strerror(errno));
        return LSSHM_SYSERROR;
    }

    m_pShmLockMap = (LsShmLockMap *)p;
    m_pShmLockElem = (LsShmLockElem *)((uint8_t *)p + s_iHdrSize);
    m_iMaxSizeO = size;
    return LSSHM_OK;
}


void LsShmLock::unmap()
{
    if (m_pShmLockMap != NULL)
    {
        munmap(m_pShmLockMap, m_iMaxSizeO);
        m_pShmLockMap = NULL;
        m_pShmLockElem = NULL;
        m_iMaxSizeO = 0;
    }
}


//
//  Note - LockNum 0 is for myself...
//
void LsShmLock::setupFreeList(LsShmOffset_t to)
{
    LsShmLockElem *p_elem =
        m_pShmLockElem + getShmLockMap()->x_iMaxElem;
    LsShmOffset_t from = ptr2offset(p_elem);
    LsShmSize_t size = to - from;
    int numAdd = size / sizeof(LsShmLockElem);

    int curNum = getShmLockMap()->x_iMaxElem;
    int lastNum = curNum + numAdd;
    int x_lastNum = lastNum;    // save it for later

    p_elem = m_pShmLockElem + --lastNum; // Can't call lockNum2pElem
    p_elem->x_iNext = getShmLockMap()->x_iFreeOffset;
    while (--numAdd >= 1)
    {
        --p_elem;
        p_elem->x_iNext = lastNum--;
    }

    if (lastNum == 0)
        ++lastNum;      // keep this for myself
    getShmLockMap()->x_iFreeOffset = lastNum;
    getShmLockMap()->x_iMaxElem = x_lastNum;
}

