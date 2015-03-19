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
#include <shm/lsshm.h>

#include <shm/lsshmpool.h>
#include <util/gpath.h>

#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>


extern "C" {
    int ls_expandfile(int fd, size_t fromsize, size_t incrsize);
};


LsShmVersion LsShm::s_version =
{
    {LSSHM_VER_MAJOR, LSSHM_VER_MINOR, LSSHM_VER_REL}
};
uint32_t LsShm::s_iPageSize = LSSHM_PAGESIZE;
uint32_t LsShm::s_iShmHdrSize = ((sizeof(LsShmMap) + 0xf) & ~0xf); // align 16
const char *LsShm::s_pDirBase = NULL;
char LsShm::s_aErrMsg[LSSHM_MAXERRMSG] = { 0 };
LsShmSize_t LsShm::s_iSysMaxShmSize = 0;


//
//  house keeping stuff
//
// LsShm *LsShm::s_pBase = NULL;
static HashStringMap< LsShm * > * s_pBase = NULL;


static inline HashStringMap< LsShm * > * getBase()
{
    if ( s_pBase == NULL )
        s_pBase = new HashStringMap< LsShm * >( );
    return s_pBase;
}


//
//  @brief setDir - allow configuration to set the path for SHM data
//
LsShmStatus_t LsShm::setSysShmDir(const char *dirName)
{
    if ((dirName != NULL) && (*dirName != '\0'))
    {
        struct stat mystat;
        if ((stat(dirName, &mystat) == 0)
            && (S_ISDIR(mystat.st_mode) || S_ISLNK(mystat.st_mode)))
        {
            char *newP;
            if ((newP = strdup(dirName)) != NULL)
            {
                if (s_pDirBase != NULL)
                    free((void *)(s_pDirBase));
                s_pDirBase = newP;
                return LSSHM_OK;
            }
        }
    }
    return LSSHM_BADPARAM;
}


//
//  @brief setErrMsg - set the global static error message
//
LsShmStatus_t LsShm::setErrMsg(const char *fmt, ...)
{
    int ret;
    va_list va;

    va_start(va, fmt);
    ret = vsnprintf(s_aErrMsg, LSSHM_MAXERRMSG, fmt, va);
    va_end(va);
    if (ret >= LSSHM_MAXERRMSG)
        strcpy(&s_aErrMsg[LSSHM_MAXERRMSG - 4], "...");
    return LSSHM_OK;
}


// //
// // @brief mapName - SHM mapName => dir / mapName . FILEEXT
// // @brief return - should check status
// //
// LsShm::LsShm(const char *mapName, LsShmSize_t size)
//                 : LsShmLock(s_pDirBase, mapName, LSSHM_MINLOCK)
//                 , m_iMagic(LSSHM_MAGIC)
//                 , m_iUnitSize(LSSHM_MINUNIT)
//                 , m_iMaxShmSize(s_iSysMaxShmSize)
//                 , m_status(LSSHM_NOTREADY)
//                 , m_pFileName(0)
//                 , m_pMapName(strdup(mapName))
//                 , m_iFd(0)
//                 , m_iRemoveFlag(0)
//                 , m_pShmLock(0)
//                 , m_pRegLock(0)
//                 , m_pShmReg(0)
//                 , m_pShmMap(0)
//                 , m_pShmMapO(0)
//                 , m_iMaxSizeO(0)
//                 , m_pShmData(0)
//                 , m_iRef(0)
// {
//     char buf[0x1000];
//
//     // m_status = LSSHM_NOTREADY;
//
//     // check to make sure the lock is ready to go
//     if (LsShmLock::status() != LSSHM_READY)
//     {
//         if (m_pMapName != NULL)
//         {
//             free(m_pMapName);
//             m_pMapName = NULL;
//         }
//         m_status = LSSHM_BADMAPFILE;
//         return;
//     }
//
//     if ((m_pMapName == NULL) || (strlen(m_pMapName) >= LSSHM_MAXNAMELEN))
//     {
//         m_status = LSSHM_BADPARAM;
//         return;
//     }
//
//     snprintf(buf, sizeof(buf), "%s/%s.%s",
//       (s_pDirBase != NULL)? s_pDirBase: getDefaultShmDir(),
//       mapName, LSSHM_SYSSHM_FILE_EXT);
//     m_pFileName = strdup(buf);
//
// #if 0
//     m_iUnitSize = LSSHM_MINUNIT; // setup dummy first
//     m_iFd = 0;
//     m_pShmMap = 0;
//     m_pShmMapO = 0;    // book keeping
//     m_pShmData = 0;
//     m_iMaxSizeO = 0;
//     m_pShmLock = 0;     // the lock
//     m_pRegLock = 0;     // the reg lock
// #endif
//
// #ifdef DELAY_UNMAP
//     m_uCur = m_uArray;
//     m_uEnd = m_uCur;
// #endif
//
//     // set the size
//     size = ((size + s_iPageSize - 1) / s_iPageSize) * s_iPageSize;
//
//     if ((m_pFileName == NULL) || (m_pMapName == NULL))
//     {
//         m_status = LSSHM_BADPARAM;
//         return;
//     }
//     m_status = init(m_pMapName, size);
//     if (m_status == LSSHM_NOTREADY)
//     {
//         m_status = LSSHM_READY;
//         m_iRef = 1;
//
// #ifdef DEBUG_RUN
//         SHM_NOTICE("LsShm::LsShm insert %s <%p>", m_pMapName, &s_base);
// #endif
//         s_base.insert(m_pMapName, this);
//     }
// #if 0
// // destructor will clean this up
//     else
//     {
//         cleanup();
//     }
// #endif
// }


//
// @brief mapName - SHM mapName => dir / mapName . FILEEXT
// @brief return - should check status
//
LsShm::LsShm(const char *mapName, LsShmSize_t size, const char *pBaseDir)
    : LsShmLock(pBaseDir, mapName, LSSHM_MINLOCK)
    , m_iMagic(LSSHM_MAGIC)
    , m_iUnitSize(LSSHM_MINUNIT)
    , m_iMaxShmSize(s_iSysMaxShmSize)
    , m_status(LSSHM_NOTREADY)
    , m_pFileName(NULL)
    , m_pMapName(strdup(mapName))
    , m_iFd(0)
    , m_pShmLock(NULL)
    , m_pRegLock(NULL)
    , m_pShmReg(NULL)
    , m_pShmMap(NULL)
    , m_pShmMapO(NULL)
    , m_iMaxSizeO(0)
    , m_pShmData(NULL)
    , m_pGPool(NULL)
    , m_iRef(0)
{
    char buf[0x1000];

    // m_status = LSSHM_NOTREADY;

    if ((m_pMapName == NULL) || (strlen(m_pMapName) >= LSSHM_MAXNAMELEN))
    {
        m_status = LSSHM_BADPARAM;
        return;
    }

    snprintf(buf, sizeof(buf), "%s/%s.%s",
             (pBaseDir != NULL) ? pBaseDir : getDefaultShmDir(),
             mapName, LSSHM_SYSSHM_FILE_EXT);
    if ((m_pFileName = strdup(buf)) == NULL)
    {
        m_status = LSSHM_ERROR;
        return;
    }

    // check to make sure the lock is ready to go
    if (LsShmLock::status() != LSSHM_READY)
    {
        if (*getErrMsg() == '\0')       // if no error message, set generic one
            setErrMsg("Unable to set SHM LockFile for [%s].", buf);
        if (m_pMapName != NULL)
        {
            free(m_pMapName);
            m_pMapName = NULL;
        }
        m_status = LSSHM_BADMAPFILE;
        return;
    }

#if 0
    m_iUnitSize = LSSHM_MINUNIT; // setup dummy first
    m_iFd = 0;
    m_pShmMap = NULL;
    m_pShmMapO = NULL;     // book keeping
    m_pShmData = NULL;
    m_iMaxSizeO = 0;
    m_pShmLock = NULL;     // the lock
    m_pRegLock = NULL;     // the reg lock
#endif

#ifdef DELAY_UNMAP
    m_uCur = m_uArray;
    m_uEnd = m_uCur;
#endif

    // set the size
    size = ((size + s_iPageSize - 1) / s_iPageSize) * s_iPageSize;

    m_status = init(m_pMapName, size);
    if (m_status == LSSHM_NOTREADY)
    {
        m_status = LSSHM_READY;
        m_iRef = 1;

#ifdef DEBUG_RUN
        SHM_NOTICE("LsShm::LsShm insert %s <%p>", m_pMapName, s_pBase);
#endif
        getBase()->insert(m_pFileName, this);
    }
#if 0
// destructor will clean this up
    else
        cleanup();
#endif
}



LsShm::~LsShm()
{
    if (m_pFileName != NULL)
    {
#ifdef DEBUG_RUN
        SHM_NOTICE("LsShm::~LsShm remove %s <%p>", m_pFileName, s_pBase);
#endif
        getBase()->remove(m_pFileName);
    }
    cleanup();
}


LsShm *LsShm::open(const char *mapName, int initsize, const char *pBaseDir)
{
    LsShm *pObj;
    if (mapName == NULL)
        return NULL;

    char buf[0x1000];
    GHash::iterator itor;
    snprintf(buf, sizeof(buf), "%s/%s.%s",
             (pBaseDir != NULL) ? pBaseDir : getDefaultShmDir(),
             mapName, LSSHM_SYSSHM_FILE_EXT);

    itor = getBase()->find(buf);
#ifdef DEBUG_RUN
    SHM_NOTICE("LsShm::get find %s <%p>", buf, s_pBase);
#endif
    if (itor != NULL)
    {
#ifdef DEBUG_RUN
        SHM_NOTICE("LsShm::get find %s <%p> return <%p>",
                        buf, s_pBase, itor);
#endif
        pObj = (LsShm *)itor->second();
        pObj->upRef();
        return pObj;
    }

    pObj = new LsShm(mapName, initsize, pBaseDir);
    if (pObj != NULL)
    {
        if (pObj->status() == LSSHM_READY)
        {
            pObj->getGlobalPool();
            return pObj;
        }

//         SHM_NOTICE(
//             "ERROR: FAILED TO CREATE SHARED MEMORY %d MAPNAME [%s] DIR [%s] ",
//             pObj->status(), mapName,
//             (pBaseDir ? pBaseDir : getDefaultShmDir()));
        delete pObj;
    }
    if (*getErrMsg() == '\0')       // if no error message, set generic one
        setErrMsg("Unable to setup SHM MapFile [%s].", buf);

    return NULL;
}


// LsShm *LsShm::get(const char *mapName, int initsize)
// {
//     LsShm *pObj;
//     if (mapName == NULL)
//        return NULL;
//
//     GHash::iterator itor;
// #ifdef DEBUG_RUN
//     SHM_NOTICE("LsShm::get find %s <%p>", mapName, &s_base);
// #endif
//     itor = s_base.find(mapName);
//     if (itor != NULL)
//     {
// #ifdef DEBUG_RUN
//         SHM_NOTICE("LsShm::get find %s <%p> return <%p>",
//           mapName, &s_base, itor);
// #endif
//         pObj = (LsShm *)itor->second();
//         pObj->upRef();
//         return pObj;
//     }
//
//     pObj = new LsShm(mapName, initsize);
//     if (pObj != NULL)
//     {
//         if (pObj->status() == LSSHM_READY)
//             return pObj;
//
//         SHM_NOTICE(
//           "ERROR: FAILED TO CREATE SHARE MEMORY %d MAPNAME [%s] DIR [%s] ",
//           pObj->status(), mapName,
//           (s_pDirBase? s_pDirBase: getDefaultShmDir()));
//         delete pObj;
//     }
//     return NULL;
// }


LsShmStatus_t LsShm::checkMagic(LsShmMap *mp, const char *mName) const
{
    return ((mp->x_iMagic != m_iMagic)
            || (mp->x_version.m_iVer != s_version.m_iVer)
            || (strncmp((char *)mp->x_aName, mName, LSSHM_MAXNAMELEN) != 0)) ?
           LSSHM_BADMAPFILE : LSSHM_OK;
}


void LsShm::deleteFile()
{
    if (m_pFileName != NULL)
    {
        unlink(m_pFileName);
        LsShmLock::deleteFile();
    }
}

void LsShm::cleanup()
{
    unmap();
    if (m_iFd != 0)
    {
        ::close(m_iFd);
        m_iFd = 0;
    }
    if (m_pFileName != NULL)
    {
        free(m_pFileName);
        m_pFileName = NULL;
    }
    if (m_pMapName != NULL)
    {
        free(m_pMapName);
        m_pMapName = NULL;
    }
}


//
// expand the file to a particular size
//
LsShmStatus_t LsShm::expandFile(LsShmOffset_t from,
                                LsShmOffset_t incrSize)
{
    if (m_iMaxShmSize && ((from + incrSize) > m_iMaxShmSize))
        return LSSHM_BADMAXSPACE;

    if (ls_expandfile(m_iFd, from, incrSize) < 0)
    {
        setErrMsg("Unable to expand [%s], incr=%d, %s.",
                  m_pFileName, incrSize, strerror(errno));
        return LSSHM_BADNOSPACE;
    }
    return LSSHM_OK;
}


LsShmStatus_t LsShm::init(const char *name, LsShmSize_t size)
{
    LsShmReg *p_reg;
    struct stat mystat;

    if ((m_iFd = ::open(m_pFileName, O_RDWR | O_CREAT, 0750)) < 0)
    {
        if ((GPath::createMissingPath(m_pFileName, 0750) < 0)
          || ((m_iFd = ::open(m_pFileName, O_RDWR | O_CREAT, 0750)) < 0))
        {
            setErrMsg("Unable to open/create [%s], %s.", m_pFileName, strerror(errno));
            return LSSHM_BADMAPFILE;
        }
    }
    if (stat(m_pFileName, &mystat) < 0)
    {
        setErrMsg("Unable to stat [%s], %s.", m_pFileName, strerror(errno));
        return LSSHM_BADMAPFILE;
    }

    ::fcntl( m_iFd, F_SETFD, FD_CLOEXEC );

    if (mystat.st_size == 0)
    {
        // New File!!!
        //  creating a new map
        if (size < s_iPageSize)
            size = s_iPageSize;
        if ((expandFile(0, roundToPageSize(size)) != LSSHM_OK)
            || (map(size) != LSSHM_OK))
            return LSSHM_ERROR;
        getShmMap()->x_iMagic = m_iMagic;
        getShmMap()->x_version.m_iVer = s_version.m_iVer;
        getShmMap()->x_iUnitSize = LSSHM_MINUNIT;   // 1 k
        getShmMap()->x_iSize = getShmMap()->x_iUnitSize;
        getShmMap()->x_iMaxSize = size;
        getShmMap()->x_iXdataOffset = s_iShmHdrSize;
        getShmMap()->x_iXdataSize =
            getShmMap()->x_iUnitSize - getShmMap()->x_iXdataOffset;

        getShmMap()->x_iFreeOffset = 0;     // no free list yet

        getShmMap()->x_iRegPerBlk = LSSHM_MAX_REG_PERUNIT;
        getShmMap()->x_iRegBlkOffset = 0;
        getShmMap()->x_iRegLastBlkOffset = 0;
        getShmMap()->x_iMaxRegNum = 0;
        getShmMap()->x_iLockOffset[0] = 0;
        getShmMap()->x_iLockOffset[1] = 0;

        if (((m_pShmLock = allocLock()) == NULL)
            || ((m_pRegLock = allocLock()) == NULL)
            || (setupLocks() != LSSHM_OK))
            return LSSHM_ERROR;
        getShmMap()->x_iLockOffset[0] = pLock2offset(m_pShmLock);
        getShmMap()->x_iLockOffset[1] = pLock2offset(m_pRegLock);

        strncpy((char *)getShmMap()->x_aName, name, strlen(name));

        if ((LsShm::allocRegBlk(NULL) == NULL)
            || ((p_reg = addReg(name)) == NULL))
            return LSSHM_ERROR;
        p_reg->x_iFlag = LSSHM_REGFLAG_SET;
        assert(p_reg->x_iRegNum == 0);
    }
    else
    {
        // Old File
        if (mystat.st_size < s_iShmHdrSize)
        {
            setErrMsg("Bad SHM file format [%s], size=%lld.",
                      m_pFileName, (uint64_t)mystat.st_size);
            return LSSHM_BADMAPFILE;
        }

        // only map the header size to ensure the file is good.
        if (map(s_iShmHdrSize) != LSSHM_OK)
            return LSSHM_ERROR;

        if ((getShmMap()->x_iMaxSize != mystat.st_size)
            || (checkMagic(getShmMap(), name) != LSSHM_OK))
        {
            setErrMsg("Bad SHM file format [%s], size=%lld, magic=%08X(%08X).",
                m_pFileName, (uint64_t)mystat.st_size, getShmMap()->x_iMagic, m_iMagic);
            return LSSHM_BADMAPFILE;
        }

        // expand the file if needed... won't shrink
        if (size > getShmMap()->x_iMaxSize)
        {
            if (expandFile(getShmMap()->x_iMaxSize, size) != LSSHM_OK)
                return LSSHM_ERROR;
            getShmMap()->x_iMaxSize = size;
        }
        else
            size = mystat.st_size;

        unmap();
        if (map(size) != LSSHM_OK)
            return LSSHM_ERROR;

        m_pShmLock = offset2pLock(getShmMap()->x_iLockOffset[0]);
        m_pRegLock = offset2pLock(getShmMap()->x_iLockOffset[1]);
    }

    syncData2Obj();

    return LSSHM_OK;
}


void LsShm::syncData2Obj()
{
    m_iUnitSize = getShmMap()->x_iUnitSize;
}


// will not shrink at the current moment
LsShmStatus_t LsShm::expand(LsShmSize_t size)
{
    LsShmSize_t xsize = getShmMap()->x_iMaxSize;

    if (expandFile(xsize, size) != LSSHM_OK)
        return LSSHM_ERROR;

#ifdef DELAY_UNMAP
    unmapLater();
#else
    unmap();
#endif
    if (map(xsize + size) != LSSHM_OK)
        return LSSHM_ERROR;
    getShmMap()->x_iMaxSize = xsize + size;

    return LSSHM_OK;
}


LsShmStatus_t LsShm::map(LsShmSize_t size)
{
    uint8_t *p =
        (uint8_t *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_iFd, 0);
    if (p == MAP_FAILED)
    {
        setErrMsg("Unable to mmap [%s], size=%d, %s.",
                  m_pFileName, size, strerror(errno));
        return LSSHM_ERROR;
    }

    m_pShmMap = (LsShmMap *)p;
    m_pShmData = (uint8_t *)p + s_iShmHdrSize;
    m_iMaxSizeO = size;
    return LSSHM_OK;
}


LsShmStatus_t LsShm::remap()
{
    if (getShmMap()->x_iMaxSize != m_iMaxSizeO)
    {
#ifdef DEBUG_RUN
        SHM_NOTICE("LsShm::remap %6d %X %X %X",
                        getpid(), getShmMap(), getShmMap()->x_iMaxSize, m_iMaxSizeO);
#endif

        LsShmSize_t size = getShmMap()->x_iMaxSize;
#ifdef DELAY_UNMAP
        unmapLater();
#else
        unmap();
#endif
        return map(size);
    }
    return LSSHM_OK;
}


void LsShm::unmap()
{
    if (m_pShmMap != NULL)
    {
        m_pShmMapO = m_pShmMap;
        munmap(m_pShmMap, m_iMaxSizeO);
        m_pShmMap = NULL;
        m_pShmData = NULL;
        m_iMaxSizeO = 0;
    }
}


LsShmOffset_t LsShm::getFromFreeList(LsShmSize_t size)
{
    LsShmOffset_t offset;
    LShmFreeTop *ap;

    offset = getShmMap()->x_iFreeOffset;
    while (offset != 0)
    {
        ap = (LShmFreeTop *)offset2ptr(offset);
        if (ap->x_iFreeSize >= size)
        {
            LsShmSize_t left = ap->x_iFreeSize - size;
            reduceFreeFromBot(ap, offset, left);
            return offset + left;
        }
        offset = ap->x_iFreeNext;
    }
    return 0; // no match
}


//
//  reduceFreeFromBot - reduce to size or unlink myself from FreeList
//
void LsShm::reduceFreeFromBot(
    LShmFreeTop *ap, LsShmOffset_t offset, LsShmSize_t newsize)
{
    if (newsize == 0)
    {
        // remove myself from freelist
        markTopUsed(ap);
        disconnectFromFree(ap,
                           (LShmFreeBot *)offset2ptr(ap->x_iFreeSize - sizeof(LShmFreeBot)));
        return;
    }

    //
    // NOTE: reduce size from bottom
    //
    LShmFreeBot *bp =
        (LShmFreeBot *)offset2ptr(offset + newsize - sizeof(LShmFreeBot));

    bp->x_iFreeOffset = offset;
    bp->x_iBMarker = LSSHM_FREE_BMARKER;
    ap->x_iFreeSize = newsize;
}


void LsShm::disconnectFromFree(LShmFreeTop *ap, LShmFreeBot *bp)
{
    LsShmOffset_t myNext, myPrev;
    myNext = ap->x_iFreeNext;
    myPrev = ap->x_iFreePrev;
    LShmFreeTop *xp;

    if (myPrev != 0)
    {
        xp = (LShmFreeTop *)offset2ptr(myPrev);
        xp->x_iFreeNext = myNext;
    }
    else
        getShmMap()->x_iFreeOffset = myNext;
    if (myNext != 0)
    {
        xp = (LShmFreeTop *)offset2ptr(myNext);
        xp->x_iFreePrev = myPrev;
    }
}


//
//  @brief isFreeBlockAbove - is the space above this block "freespace".
//  @return true if the block above is free otherwise false
//
bool LsShm::isFreeBlockAbove(
    LsShmOffset_t offset, LsShmSize_t size,  int joinFlag)
{
    LsShmSize_t aboveOffset = offset - sizeof(LShmFreeBot);
    if (aboveOffset < getShmMap()->x_iUnitSize)
        return false;

    LShmFreeBot *bp = (LShmFreeBot *)offset2ptr(aboveOffset);
    if ((bp->x_iBMarker == LSSHM_FREE_BMARKER)
        && (bp->x_iFreeOffset >= getShmMap()->x_iUnitSize)
        && (bp->x_iFreeOffset < aboveOffset))
    {
        LShmFreeTop *ap = (LShmFreeTop *)offset2ptr(bp->x_iFreeOffset);
        if ((ap->x_iAMarker == LSSHM_FREE_AMARKER)
            && ((ap->x_iFreeSize + bp->x_iFreeOffset) == offset))
        {
            if (joinFlag != 0)
            {
                // join above block
                ap->x_iFreeSize += size;

                LShmFreeBot *xp =
                    (LShmFreeBot *)offset2ptr(offset + size - sizeof(LShmFreeBot));
                xp->x_iBMarker = LSSHM_FREE_BMARKER;
                xp->x_iFreeOffset = bp->x_iFreeOffset;

                markTopUsed((LShmFreeTop *)offset2ptr(offset));
            }
            return true;
        }
    }
    return false;
}


//
// Check if this is a good free block
//
bool LsShm::isFreeBlockBelow(
    LsShmOffset_t offset, LsShmSize_t size, int joinFlag)
{
    LsShmOffset_t belowOffset = offset + size;

    LShmFreeTop *ap = (LShmFreeTop *)offset2ptr(belowOffset);
    if (ap->x_iAMarker == LSSHM_FREE_AMARKER)
    {
        // the bottom free offset
        LsShmOffset_t e_offset = belowOffset + ap->x_iFreeSize;
        if ((e_offset < getShmMap()->x_iUnitSize)
            || (e_offset > getShmMap()->x_iMaxSize))
            return false;

        e_offset -= sizeof(LShmFreeBot);
        LShmFreeBot *bp = (LShmFreeBot *)offset2ptr(e_offset);
        if ((bp->x_iBMarker == LSSHM_FREE_BMARKER)
            && (bp->x_iFreeOffset == belowOffset))
        {
            if (joinFlag != 0)
            {
                markTopUsed(ap);
                if (joinFlag == 2)
                {
                    disconnectFromFree(ap, bp);
                    // merge to top
                    LShmFreeTop *xp = (LShmFreeTop *)offset2ptr(offset);
                    xp->x_iFreeSize += ap->x_iFreeSize;
                    bp->x_iFreeOffset = offset;
                    return true;
                }

                // setup myself as free block
                LShmFreeTop *np = (LShmFreeTop *)offset2ptr(offset);
                np->x_iAMarker = LSSHM_FREE_AMARKER;
                np->x_iFreeSize = size + ap->x_iFreeSize;
                np->x_iFreeNext = ap->x_iFreeNext;
                np->x_iFreePrev = ap->x_iFreePrev;

                bp->x_iFreeOffset = offset;

                LShmFreeTop *xp;
                if (np->x_iFreeNext != 0)
                {
                    xp = (LShmFreeTop *)offset2ptr(np->x_iFreeNext);
                    xp->x_iFreePrev = offset;
                }
                if (np->x_iFreePrev != 0)
                {
                    xp = (LShmFreeTop *)offset2ptr(np->x_iFreePrev);
                    xp->x_iFreeNext = offset;
                }
                else
                    getShmMap()->x_iFreeOffset = offset;
            }
            return true;
        }
    }
    return false;
}


//
//  Simple bucket type allocation and release
//
LsShmOffset_t LsShm::allocPage(LsShmSize_t pagesize, int &remap)
{
    LsShmOffset_t offset;
    LsShmSize_t availSize;

    LSSHM_CHECKSIZE(pagesize);

    pagesize = roundUnitSize(pagesize);
    remap = 0;

    //
    //  MUTEX SHOULD BE HERE for multi process/thread environment
    // Only use lock when m_status is Ready.
    if (m_status == LSSHM_READY)
        if (lock())
            return 0; // no lock acquired...

    // Allocate from free space
    if ((offset = getFromFreeList(pagesize)) != 0)
        goto out;

    // Allocate from heap space
    availSize = avail();
    if (pagesize > availSize)
    {
        LsShmSize_t needSize;
        // min 16 unit at a time
#define MIN_NUMUNIT_SHIFT 4
        needSize = ((pagesize - availSize) >
                    (getShmMap()->x_iUnitSize << MIN_NUMUNIT_SHIFT)) ?
                   (pagesize - availSize) :
                   (getShmMap()->x_iUnitSize << MIN_NUMUNIT_SHIFT);
        // allocate 8 pages more!
        if (expand(roundToPageSize(needSize)) != LSSHM_OK)
        {
            offset = 0;
            goto out;
        }
        if (getShmMap() != m_pShmMapO)
            remap = 1;
    }
    offset = getShmMap()->x_iSize;
    markTopUsed((LShmFreeTop *)offset2ptr(offset));
    used(pagesize);
out:
    if (m_status == LSSHM_READY)
        unlock();
    return offset;
}


void LsShm::releasePage(LsShmOffset_t offset, LsShmSize_t pagesize)
{
    //
    //  MUTEX SHOULD BE HERE for multi process/thread environment
    //
    lock();

    pagesize = roundUnitSize(pagesize);
    if ((offset + pagesize) == getShmMap()->x_iSize)
    {
        getShmMap()->x_iSize -= pagesize;
        goto out;
    }
    if (isFreeBlockAbove(offset, pagesize, 1))
    {
        LShmFreeTop *ap;
        offset = ((LShmFreeBot *)offset2ptr(
                      offset + pagesize - sizeof(LShmFreeBot)))->x_iFreeOffset;
        ap = (LShmFreeTop *)offset2ptr(offset);
        isFreeBlockBelow(offset, ap->x_iFreeSize, 2);
        goto out;
    }
    if (isFreeBlockBelow(offset, pagesize, 1))
        goto out;
    joinFreeList(offset, pagesize);
out:
    unlock();
}


void LsShm::joinFreeList(LsShmOffset_t offset, LsShmSize_t size)
{
    // setup myself as free block
    LShmFreeTop *np = (LShmFreeTop *)offset2ptr(offset);
    np->x_iAMarker = LSSHM_FREE_AMARKER;
    np->x_iFreeSize = size;
    np->x_iFreeNext = getShmMap()->x_iFreeOffset;
    np->x_iFreePrev = 0;

    // join myself to freeList
    getShmMap()->x_iFreeOffset = offset;

    if (np->x_iFreeNext != 0)
    {
        LShmFreeTop *xp = (LShmFreeTop *)offset2ptr(np->x_iFreeNext);
        xp->x_iFreePrev = offset;
    }

    LShmFreeBot *bp =
        (LShmFreeBot *)offset2ptr(offset + size - sizeof(LShmFreeBot));
    bp->x_iFreeOffset = offset;
    bp->x_iBMarker = LSSHM_FREE_BMARKER;
}


//
//  SHM Reg sub interface
//
LsShmRegBlk *LsShm::allocRegBlk(LsShmRegBlkHdr *pFrom)
{
    LsShmOffset_t offset;
    LsShmRegBlkHdr *p_regBlkHdr;

    int remap;
    offset = allocPage(sizeof(LsShmRegElem) * getShmMap()->x_iRegPerBlk,
                       remap);
    if (offset == 0)
        return NULL;
    p_regBlkHdr = (LsShmRegBlkHdr *)offset2ptr(offset);
    p_regBlkHdr->x_iCapacity = getShmMap()->x_iRegPerBlk;
    p_regBlkHdr->x_iSize = 1;
    p_regBlkHdr->x_iNext = 0;

    if (pFrom != NULL)
    {
        p_regBlkHdr->x_iStartNum = pFrom->x_iStartNum + pFrom->x_iCapacity - 1;
        pFrom->x_iNext = offset;
    }
    else
    {
        getShmMap()->x_iRegBlkOffset = offset;
        p_regBlkHdr->x_iStartNum = 0;
    }

    getShmMap()->x_iRegLastBlkOffset = offset;
    getShmMap()->x_iMaxRegNum += (getShmMap()->x_iRegPerBlk - 1);

    // initialize the block data
    LsShmReg *p_reg = (LsShmReg *)(((LsShmRegElem *)p_regBlkHdr) + 1);
    int regnum = p_regBlkHdr->x_iStartNum;
    int i = p_regBlkHdr->x_iCapacity;
    while (--i > 0)
    {
        p_reg->x_iRegNum = regnum;
        ++p_reg;
        ++regnum;
    }

    // note LsShmRegBlk == LsShmRegBlkHdr == LsShmReg[0]
    return (LsShmRegBlk *)p_regBlkHdr;
}


LsShmRegBlk *LsShm::findRegBlk(LsShmRegBlkHdr *pFrom, int num)
{
    if ((LsShmOffset_t)num >= pFrom->x_iStartNum)
    {
        if ((LsShmOffset_t)num < (pFrom->x_iStartNum + pFrom->x_iCapacity - 1))
            return (LsShmRegBlk *)pFrom;
        if (pFrom->x_iNext != 0)
            return findRegBlk((LsShmRegBlkHdr *)offset2ptr(pFrom->x_iNext), num);
    }
    return NULL;
}


LsShmReg *LsShm::getReg(int num)
{
    LsShmRegBlkHdr *pFrom;
    LsShmOffset_t regoff = getShmMap()->x_iRegBlkOffset;
    if ((regoff != 0)
        && (pFrom = (LsShmRegBlkHdr *)
                    findRegBlk((LsShmRegBlkHdr *)offset2ptr(regoff), num)) != NULL)
    {
        return (LsShmReg *)
               (((LsShmRegElem *)pFrom) + 1 + num - pFrom->x_iStartNum);
    }
    return NULL;
}


int LsShm::clrReg(int num)
{
    lockReg();
    LsShmRegBlkHdr *pFrom;
    LsShmOffset_t regoff = getShmMap()->x_iRegBlkOffset;
    if ((regoff != 0)
        && (pFrom = (LsShmRegBlkHdr *)
                    findRegBlk((LsShmRegBlkHdr *)offset2ptr(regoff), num)) != NULL)
    {
        LsShmRegElem *p_regElem =
            ((LsShmRegElem *)pFrom) + 1 + num - pFrom->x_iStartNum;
        if (p_regElem->x_reg.x_iFlag != LSSHM_REGFLAG_CLR)
        {
            p_regElem->x_reg.x_iFlag = LSSHM_REGFLAG_CLR; // clear it!
            --pFrom->x_iSize;
            unlockReg();
            return 0;
        }
    }
    unlockReg();
    return LS_FAIL;
}


int LsShm::expandReg(int maxRegNum)
{
    LsShmRegBlkHdr *p_regBlkHdr = NULL;

    if (maxRegNum <= 0)
        maxRegNum = getShmMap()->x_iMaxRegNum + getShmMap()->x_iRegPerBlk - 1;

    if (getShmMap()->x_iRegLastBlkOffset == 0)
        p_regBlkHdr = (LsShmRegBlkHdr *)allocRegBlk(0);
    else
    {
        p_regBlkHdr =
            (LsShmRegBlkHdr *)offset2ptr(getShmMap()->x_iRegLastBlkOffset);
    }
    while ((p_regBlkHdr != NULL)
           && (getShmMap()->x_iMaxRegNum <= (LsShmOffset_t)maxRegNum))
        p_regBlkHdr = (LsShmRegBlkHdr *)allocRegBlk(p_regBlkHdr);

    return getShmMap()->x_iMaxRegNum;
}


LsShmReg *LsShm::findReg(const char *name)
{
    if (name == NULL)
        return NULL;

    LsShmSize_t nameSize = (LsShmSize_t)strlen(name);
    if ((nameSize == 0)
        || (nameSize > (sizeof(LsShmReg) - sizeof(LsShmOffset_t))))
        return NULL;

    if (getShmMap()->x_iRegBlkOffset == 0)
        return NULL;

    LsShmRegBlkHdr *p_regBlkHdr;
    p_regBlkHdr = (LsShmRegBlkHdr *)offset2ptr(getShmMap()->x_iRegBlkOffset);
    while (p_regBlkHdr != NULL)
    {
        // search the block!
        LsShmRegElem *p_regElem = (LsShmRegElem *)p_regBlkHdr;
        int i = p_regBlkHdr->x_iCapacity;
        while (--i > 0)
        {
            ++p_regElem; // skip the first one
            if ((p_regElem->x_reg.x_iFlag != LSSHM_REGFLAG_CLR)
                && (memcmp(name, p_regElem->x_reg.x_aName, nameSize) == 0))
                return &p_regElem->x_reg;
        }
        if (p_regBlkHdr->x_iNext != 0)
            p_regBlkHdr = (LsShmRegBlkHdr *)offset2ptr(p_regBlkHdr->x_iNext);
        else
            return NULL;
    }
    return NULL;
}


//
// @brief - addReg add the given name into Registry.
// @brief   return the ptr of the registry
//
LsShmReg *LsShm::addReg(const char *name)
{
    int nameSize;
    if ((name == NULL) || (*name == '\0')
        || (getShmMap()->x_iRegBlkOffset == 0)
        || ((nameSize = strlen(name) + 1) > LSSHM_MAXNAMELEN))  // including null
        return NULL;

    lockReg();
    LsShmReg *p_reg;
    if ((p_reg = findReg(name)) != NULL)
    {
        unlockReg();
        return p_reg;
    }

    LsShmRegBlkHdr *p_regBlkHdr;
    p_regBlkHdr = (LsShmRegBlkHdr *)offset2ptr(getShmMap()->x_iRegBlkOffset);
    LsShmRegElem *p_regElem;
    while (p_regBlkHdr != NULL)
    {
        if (p_regBlkHdr->x_iSize < p_regBlkHdr->x_iCapacity)
        {
            // find the empty slot
            p_regElem = ((LsShmRegElem *)p_regBlkHdr) + 1;
            int i = p_regBlkHdr->x_iCapacity;
            while (--i > 0)
            {
                if (p_regElem->x_reg.x_iFlag == LSSHM_REGFLAG_CLR)
                {
                    memcpy((char *)p_regElem->x_reg.x_aName, name, nameSize);
                    if (nameSize < LSSHM_MAXNAMELEN)
                        p_regElem->x_reg.x_aName[nameSize] = '\0';
                    p_regElem->x_reg.x_iFlag = LSSHM_REGFLAG_SET; // assign it!
                    p_regElem->x_reg.x_iValue = 0;
                    ++p_regBlkHdr->x_iSize;
                    unlockReg();
                    return (LsShmReg *)p_regElem;
                }
                ++p_regElem;
            }
        }
        else
        {
            if (p_regBlkHdr->x_iNext != 0)
            {
                p_regBlkHdr =
                    (LsShmRegBlkHdr *)offset2ptr(p_regBlkHdr->x_iNext);
            }
            else
            {
                expandReg(0);
                p_regBlkHdr = ((p_regBlkHdr->x_iNext != 0) ?
                               (LsShmRegBlkHdr *)offset2ptr(p_regBlkHdr->x_iNext) : NULL);
            }
        }
    }
    unlockReg();
    return NULL;
}


int LsShm::recoverOrphanShm()
{
    if (getGlobalPool() == NULL)
        return 0;
    LsShmRegBlkHdr *pRegBlkHdr;
    LsShmOffset_t offRegBlkHdr = getShmMap()->x_iRegBlkOffset;
    int cnt = 0;
    while (offRegBlkHdr != 0)
    {
        pRegBlkHdr = (LsShmRegBlkHdr *)offset2ptr(offRegBlkHdr);
        chkRegBlk(pRegBlkHdr, &cnt);
        offRegBlkHdr = pRegBlkHdr->x_iNext;
    }
    return cnt;
}


void LsShm::chkRegBlk(LsShmRegBlkHdr *pRegBlkHdr, int *pCnt)
{
    uint16_t num = pRegBlkHdr->x_iCapacity;
    LsShmRegElem *pRegElem = (((LsShmRegElem *)pRegBlkHdr) + 1);
    while (--num > 0)
    {
        if (pRegElem->x_reg.x_iFlag & LSSHM_REGFLAG_PID)
        {
            LsShmPoolMem *pPool =
                (LsShmPoolMem *)offset2ptr(pRegElem->x_reg.x_iValue);
            if ((kill(pPool->x_pid, 0) < 0) && (errno == ESRCH))
            {
                m_pGPool->addFreeBucket(&pPool->x_data.x_aFreeBucket[0]);
                m_pGPool->addFreeList(&pPool->x_data.x_iFreeList);
                lockReg();
                pRegElem->x_reg.x_iFlag = LSSHM_REGFLAG_CLR; // clear it!
                --pRegBlkHdr->x_iSize;
                uint32_t offset = pRegElem->x_reg.x_iValue;
                unlockReg();
                m_pGPool->release2(offset, sizeof(LsShmPool));
                ++(*pCnt);
            }
        }
        ++pRegElem;
    }
    return;
}


LsShmPool *LsShm::getGlobalPool()
{
    if ((m_pGPool != NULL) && (m_pGPool->getMagic() == LSSHM_POOL_MAGIC))
        return m_pGPool;

    const char *name = LSSHM_SYSPOOL;
    m_pGPool = new LsShmPool(this, name, NULL);
    if (m_pGPool != NULL)
    {
        if (m_pGPool->getRef() != 0)
            return m_pGPool;

        delete m_pGPool;
        m_pGPool = NULL;
    }
    if (*getErrMsg() == '\0')       // if no error message, set generic one
        setErrMsg("Unable to get Global Pool, MapFile [%s].", m_pFileName);
    return NULL;
}


LsShmPool *LsShm::getNamedPool(const char *name)
{
    LsShmPool *pObj;
    GHash::iterator iter;

    if (name == NULL)
        return getGlobalPool();

#ifdef DEBUG_RUN
    SHM_NOTICE("LsShm::getNamedPool find %s <%p>",
                    name, &getObjBase());
#endif
    iter = getObjBase().find(name);
    if (iter != NULL)
    {
#ifdef DEBUG_RUN
        SHM_NOTICE("LsShm::getNamedPool %s <%p> return %p ",
                        name, &getObjBase(), iter);
#endif
        pObj = (LsShmPool *)iter->second();
        if (pObj->getMagic() != LSSHM_POOL_MAGIC)
            return NULL;
        pObj->upRef();
        return pObj;
    }
    LsShmPool *gpool;
    if ((gpool = getGlobalPool()) == NULL)
        return NULL;
    pObj = new LsShmPool(this, name, gpool);
    if (pObj != NULL)
    {
        if (pObj->getRef() != 0)
            return pObj;
        delete pObj;
    }
    if (*getErrMsg() == '\0')       // if no error message, set generic one
        setErrMsg("Unable to get SHM Pool [%s], MapFile [%s].",
                  name, m_pFileName);
    return NULL;
}
