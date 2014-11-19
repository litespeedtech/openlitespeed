/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <shm/lsshmtypes.h>
#include <shm/lsshm.h>
#include <shm/lsshmlock.h>
#include <http/httplog.h>
#if 0
#include <log4cxx/logger.h>
#endif
#include <util/configctx.h>
#include <util/gpath.h>

extern "C" {
    int ls_expandfile(int fd, size_t fromsize, size_t incrsize);
};

#ifdef DEBUG_RUN 
using namespace LOG4CXX_NS;
#endif


lsShmVersion_t LsShm::s_version = {
    {LSSHM_VER_MAJOR, LSSHM_VER_MINOR, LSSHM_VER_REL}
};
uint32_t LsShm::s_pageSize = LSSHM_PAGESIZE; 
uint32_t LsShm::s_shmHdrSize = ((sizeof(lsShmMap_t)+0xf)&~0xf); // align 16
const char * LsShm::s_dirBase = NULL;
LsShm_size_t LsShm::s_sysMaxShmSize = 0;

//
//  house keeping stuff
//
// LsShm * LsShm::s_pBase = NULL;
HashStringMap< LsShm * > LsShm::s_base;

//
//  @brief setDir - allow configuration to set the path for SHM data
//
LsShm_status_t LsShm::setSysShmDir( const char* dirName )
{
    if (dirName && strlen(dirName))
    {
        struct stat mystat;
        if ( (!stat(dirName, &mystat)) 
                && ( S_ISDIR(mystat.st_mode) || S_ISLNK(mystat.st_mode) ))
        {
            char *newP;
            if ( (newP = strdup(dirName)) )
            {
                if (s_dirBase)
                    free ( (void *)(s_dirBase) );
                s_dirBase = newP;
                return LSSHM_OK;
            }
        }
    }
    return LSSHM_BADPARAM;
}

//
// @brief mapName - SHM mapName => dir / mapName . FILEEXT
// @brief return - should check status
//
LsShm::LsShm( const char * mapName,
              LsShm_size_t size)
                : LsShmLock( s_dirBase, mapName, LSSHM_MINLOCK)
                , m_magic(LSSHM_MAGIC)
                , m_unitSize(LSSHM_MINUNIT)
                , m_maxShmSize(s_sysMaxShmSize)
                , m_status(LSSHM_NOTREADY)
                , m_fileName(0)
                , m_mapName( strdup(mapName) )
                , m_fd(0)
                , m_removeFlag(0)
                , m_pShmLock(0)
                , m_pShmReg(0)
                , m_pShmMap(0)
                , m_pShmMap_o(0)
                , m_maxSize_o(0)
                , m_pShmData(0)
                , m_ref(0)
{
    const char * dirName;
    char buf[0x1000];
    
    // m_status = LSSHM_NOTREADY;
    
    // check to make sure the lock is ready to go
    if (LsShmLock::status() != LSSHM_READY)
    {
        if (m_mapName)
        {
            free (m_mapName);
            m_mapName = 0;
        }
        m_status = LSSHM_BADMAPFILE;
        return;
    }
    
    if ((!m_mapName) || (strlen(m_mapName) >= LSSHM_MAXNAMELEN))
    {
        m_status = LSSHM_BADPARAM;
        return;
    }
    
    if (!s_dirBase)
        dirName = getDefaultShmDir();
    else
        dirName = s_dirBase;
    
    snprintf(buf, 0x1000, "%s/%s.%s",
                 dirName,
                 mapName,
                 LSSHM_SYSSHM_FILE_EXT);
    m_fileName = strdup(buf);

#if 0
    m_unitSize = LSSHM_MINUNIT; // setup dummy first
    m_fd = 0;
    m_pShmMap = 0;
    m_pShmMap_o = 0;    // book keeping
    m_pShmData = 0;
    m_maxSize_o = 0;
    m_pShmLock = 0;     // the lock
#endif
    
#ifdef DELAY_UNMAP
    m_uCur = m_uArray;
    m_uEnd = m_uCur;
#endif
    
    /* set the size */
    size = ((size + s_pageSize - 1) / s_pageSize) * s_pageSize;
            
    if ( (!m_fileName ) || (!m_mapName) )
    {
        m_status = LSSHM_BADPARAM;
        return;
    }
    m_status = init(m_mapName, size);
    if (m_status == LSSHM_NOTREADY)
    {
        m_status = LSSHM_READY;
        m_ref = 1;
        
#ifdef DEBUG_RUN
        HttpLog::notice( "LsShm::LsShm insert %s <%p>"
            , m_mapName, &s_base);
#endif
        s_base.insert(m_mapName, this);
    }
#if 0
// destructor will clean this up
    else
    {
        cleanup();
    }
#endif
}

LsShm::~LsShm()
{
    if  (m_mapName)
    {
#ifdef DEBUG_RUN
        HttpLog::notice( "LsShm::~LsShm remove %s <%p>"
            , m_mapName, &s_base);
#endif
        s_base.remove(m_mapName);
    }
    cleanup();
}

LsShm* LsShm::get( const char* mapName, int initsize)
{
    register LsShm * pObj;
    if (!mapName)
       return NULL;
        
    GHash::iterator itor;
#ifdef DEBUG_RUN
    HttpLog::notice( "LsShm::get find %s <%p>"
            , mapName, &s_base);
#endif
    itor = s_base.find(mapName);
    if ( itor )
    {
#ifdef DEBUG_RUN
        HttpLog::notice( "LsShm::get find %s <%p> return <%p>"
            , mapName, &s_base, itor);
#endif
        pObj = (LsShm *) itor->second();
        pObj->up_ref();
        return pObj;
    }
    
    pObj = new LsShm(mapName, initsize);
    if (pObj)
    {
        if (pObj->status() == LSSHM_READY)
            return pObj;
        
        HttpLog::notice( "ERROR: FAILED TO CREATE SHARE MEMORY %d MAPNAME [%s] DIR [%s] "
            , pObj->status()
            , mapName
            , s_dirBase ? s_dirBase : getDefaultShmDir()
            );
        
        delete pObj;
    }
    return NULL;
}

LsShm_status_t LsShm::checkMagic(lsShmMap_t * mp, const char * mName) const
{
    return (  (mp->x_magic != m_magic)
                || (mp->x_version.ver != s_version.ver) 
                || strncmp((char *)mp->x_name, mName, LSSHM_MAXNAMELEN) ) 
            ? LSSHM_BADMAPFILE : LSSHM_OK;
}

void LsShm::cleanup()
{
    unmap();
    if (m_fd)
    {
        close(m_fd);
        m_fd = 0;
    }
    if (m_fileName)
    {
        if (m_removeFlag)
        {
            m_removeFlag = 0;
            unlink(m_fileName);
        }
        free (m_fileName);
        m_fileName = 0;
    }
    if (m_mapName)
    {
        free (m_mapName);
        m_mapName = 0;
    }
}

//
// expand the file to a particula size
//
LsShm_status_t LsShm::expandFile( LsShm_offset_t from,  LsShm_offset_t incrSize )
{
    if ( m_maxShmSize && ((from + incrSize) > m_maxShmSize) )
        return LSSHM_BADMAXSPACE;
    // Just truncate it... 
    // if (ftruncate(m_fd, from + incrSize))
    if (ls_expandfile(m_fd, from, incrSize))
        return LSSHM_BADNOSPACE;
    else
        return LSSHM_OK;
}

LsShm_status_t LsShm::init(const char * name, LsShm_size_t size)
{
    lsShmReg_t * p_reg;
    struct stat  mystat;
    
    if ((m_fd = open(m_fileName, O_RDWR | O_CREAT, 0750)) < 0)
    {
        if (GPath::createMissingPath(m_fileName, 0750))
            return LSSHM_BADMAPFILE;
        else if ((m_fd = open(m_fileName, O_RDWR | O_CREAT, 0750)) < 0)
            return LSSHM_BADMAPFILE;
    }
    if (stat (m_fileName, &mystat))
    {
        return LSSHM_BADMAPFILE;
    }
    
    if (!mystat.st_size)
    {
        m_removeFlag = 1;
        // New File!!!
        /* creating a new map */
        if (size < s_pageSize)
            size = s_pageSize;
        if (expandFile(0, roundToPageSize( size)) || map(size))
            return LSSHM_ERROR;
        getShmMap()->x_magic = m_magic;
        getShmMap()->x_version.ver = s_version.ver;
        getShmMap()->x_unitSize = LSSHM_MINUNIT; /* 1 k */
        getShmMap()->x_size = getShmMap()->x_unitSize;
        getShmMap()->x_maxSize = size;
        getShmMap()->x_xdataOffset = s_shmHdrSize;
        getShmMap()->x_xdataSize = s_pageSize - getShmMap()->x_xdataOffset;
            
        getShmMap()->x_xdataOffset = s_shmHdrSize;
        getShmMap()->x_xdataSize = getShmMap()->x_unitSize - getShmMap()->x_xdataOffset;
            
        getShmMap()->x_freeOffset = 0; /* no free list yet */
            
        getShmMap()->x_numRegPerBlk = LSSHM_MAX_REG_PERUNIT;
        getShmMap()->x_regBlkOffset = 0;
        getShmMap()->x_regLastBlkOffset = 0;
        getShmMap()->x_maxRegNum = 0;
        getShmMap()->x_lockOffset = 0;
            
        if (!(m_pShmLock = allocLock()))
            return LSSHM_ERROR;
        getShmMap()->x_lockOffset = pLock2offset(m_pShmLock);
            
        strncpy((char *)getShmMap()->x_name, name, strlen(name));
            
        if (! LsShm::allocRegBlk( NULL ) )
            return LSSHM_ERROR;
            
        if (!(p_reg = addReg(name)))
        {
            return LSSHM_ERROR;
        }
        p_reg->x_flag = 0x1;
        assert(p_reg->x_regNum == 0);
    }
    else
    {
        // OLD FILE
        if (mystat.st_size < s_shmHdrSize)
            return LSSHM_BADMAPFILE;
                
        // only map the header size to ensure the file is good.
        if (map(s_shmHdrSize))
            return LSSHM_ERROR;
        
        if ((getShmMap()->x_maxSize != mystat.st_size)
                || checkMagic(getShmMap(), name))
            return LSSHM_BADMAPFILE;

        // expand the file if needed... wont shrink
        if (size > getShmMap()->x_maxSize)
        {
            if (expandFile(getShmMap()->x_maxSize, size))
                return LSSHM_ERROR;
            getShmMap()->x_maxSize = size;
        } else {
            size = mystat.st_size;
        }
        
        unmap();
        
        // note... use size since we may expanded the File already
        if (map(size))
            return LSSHM_ERROR;
    }
    
    // load important data
    syncData2Obj();
    m_pShmLock = offset2pLock(getShmMap()->x_lockOffset);
    if ( setupLock() )
        return LSSHM_ERROR;
    
    m_removeFlag = 0;
    return LSSHM_OK;
}

void LsShm::syncData2Obj()
{
    m_unitSize = getShmMap()->x_unitSize;
}

// will not shrink at the current moment
LsShm_status_t LsShm::expand(LsShm_size_t size)
{
    LsShm_size_t xsize = getShmMap()->x_maxSize;
    
    if ( expandFile(xsize, size) )
        return LSSHM_ERROR;
    
#ifdef DELAY_UNMAP
    unmapLater();
#else
    unmap();
#endif
    if ( map(xsize + size) )
        return LSSHM_ERROR;
    getShmMap()->x_maxSize = xsize + size;
    
    return LSSHM_OK;
}

LsShm_status_t LsShm::map(LsShm_size_t size)
{
    uint8_t    *p;
    
    // p = (uint8_t *)mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_POPULATE, m_fd, 0);
    p = (uint8_t *)mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, m_fd, 0);
    if ( p != MAP_FAILED )
    {
        m_pShmMap = (lsShmMap_t *)p;
        m_pShmData = (uint8_t *)p + s_shmHdrSize;
        m_maxSize_o = size;
        return LSSHM_OK;
    }
    return LSSHM_ERROR;
}

LsShm_status_t LsShm::remap()
{
    if (getShmMap()->x_maxSize != m_maxSize_o)
    {
#ifdef DEBUG_RUN
HttpLog::notice("LsShm::remap %6d %X %X %X", getpid()
                , getShmMap()
                , getShmMap()->x_maxSize
                , m_maxSize_o);
#endif
        
        LsShm_size_t size = getShmMap()->x_maxSize;
#ifdef DELAY_UNMAP
        unmapLater();
#else
        unmap();
#endif
        return map(size);
    }
    return LSSHM_OK;
}

// void LsShm::unmap(LsShm_size_t size)
void LsShm::unmap()
{
    if (m_pShmMap)
    {
        m_pShmMap_o = m_pShmMap;
        munmap(m_pShmMap, m_maxSize_o);
        m_pShmMap = 0;
        m_pShmData = 0;
        m_maxSize_o = 0;
    }
}

//
//  reduceFree - reduce to size or unlink myself from FreeList
//
void LsShm::reduceFreeFromBot( lsShmFreeTop_t * ap, 
                        LsShm_offset_t offset,
                        LsShm_size_t newsize )
{
    if (!newsize)
    {
        // remove myself from freelist
        markTopUsed(ap);
        disconnectFromFree(ap, (lsShmFreeBot_t *)
            offset2ptr(ap->x_freeSize - sizeof(lsShmFreeBot_t)));
        return;
    }
    
    //
    // NOTE: reduce size from bottom
    //
    lsShmFreeBot_t * bp = (lsShmFreeBot_t*)
            offset2ptr(offset + newsize - sizeof(lsShmFreeBot_t));
            
    bp->x_freeOffset = offset;
    bp->x_bMarker = LSSHM_FREE_BMARKER;
    ap->x_freeSize = newsize;
}

LsShm_offset_t LsShm::getFromFreeList( LsShm_size_t size )
{
    LsShm_offset_t offset;
    lsShmFreeTop_t * ap;
    
    for ( offset = getShmMap()->x_freeOffset ; offset; )
    {
        ap = (lsShmFreeTop_t*) offset2ptr(offset);
        if (ap->x_freeSize >= size)
        {
            LsShm_size_t left = ap->x_freeSize - size;
            reduceFreeFromBot(ap, offset, left);
            return offset + left;
        }
        offset = ap->x_freeNext;
    }
    return 0; // no match
}

void LsShm::disconnectFromFree( lsShmFreeTop_t* ap, lsShmFreeBot_t* bp )
{
    register LsShm_offset_t  myNext, myPrev;
    myNext = ap->x_freeNext;
    myPrev = ap->x_freePrev;
    register lsShmFreeTop_t  * xp;
    
    if (myPrev)
    {
        xp = (lsShmFreeTop_t *)offset2ptr(myPrev);
        xp->x_freeNext = myNext;
    }
    else
    {
        getShmMap()->x_freeOffset = myNext;
    }
    if (myNext)
    {
        xp = (lsShmFreeTop_t *)offset2ptr(myNext);
        xp->x_freePrev = myPrev;
    }
}

//
// Check if this is a good free block
//
bool LsShm::isFreeBlockBelow( LsShm_offset_t offset, LsShm_size_t size, int joinFlag)
{
   LsShm_offset_t belowOffset = offset + size;
    
    lsShmFreeTop_t *ap = (lsShmFreeTop_t*) offset2ptr(belowOffset);
    if (ap->x_aMarker == LSSHM_FREE_AMARKER)
    {
        // the bottom free offset
        LsShm_offset_t e_offset = belowOffset + ap->x_freeSize;
        if ((e_offset < getShmMap()->x_unitSize)
                || (e_offset > getShmMap()->x_maxSize))
            return false;
        
        e_offset -= sizeof(lsShmFreeBot_t);
        lsShmFreeBot_t *bp = (lsShmFreeBot_t*) offset2ptr(e_offset);
        if ((bp->x_bMarker == LSSHM_FREE_BMARKER)
                && (bp->x_freeOffset == belowOffset))
        {
            if (joinFlag)
            {
                markTopUsed(ap);
                
                if (joinFlag == 2)
                {
                    disconnectFromFree(ap, bp);
                    
                    // merge to top
                    lsShmFreeTop_t *xp = (lsShmFreeTop_t*) offset2ptr(offset);
                    xp->x_freeSize += ap->x_freeSize;
                    bp->x_freeOffset = offset;
                    return true;
                }
                
                // setup myself as free block
                lsShmFreeTop_t * np = (lsShmFreeTop_t *) offset2ptr(offset);
                np->x_aMarker = LSSHM_FREE_AMARKER;
                np->x_freeSize = size + ap->x_freeSize;
                np->x_freeNext = ap->x_freeNext;
                np->x_freePrev = ap->x_freePrev;
                
                bp->x_freeOffset = offset;
                
                lsShmFreeTop_t *xp;
                if (np->x_freeNext)
                {
                    xp = (lsShmFreeTop_t*) offset2ptr(np->x_freeNext);
                    xp->x_freePrev = offset;
                }
                if (np->x_freePrev)
                {
                    xp = (lsShmFreeTop_t*) offset2ptr(np->x_freePrev);
                    xp->x_freeNext = offset;
                }
                else
                {
                    getShmMap()->x_freeOffset = offset;
                }
            }
            return true;
        }
    }
    return false;
}

//
//  @brief isFreeBlockAbove - return true if the space above this block is "freespace".
//  @return true if the block above is free otherwise false
//
bool LsShm::isFreeBlockAbove( LsShm_offset_t offset , LsShm_size_t size,  int joinFlag )
{
    LsShm_size_t aboveOffset = offset - sizeof(lsShmFreeBot_t);
    if (aboveOffset < getShmMap()->x_unitSize)
        return false;
    
    lsShmFreeBot_t *bp = (lsShmFreeBot_t*) offset2ptr(aboveOffset);
    if (bp->x_bMarker == LSSHM_FREE_BMARKER)
    {
        if ((bp->x_freeOffset < getShmMap()->x_unitSize) || (bp->x_freeOffset >= aboveOffset))
            return false;
        lsShmFreeTop_t *ap = (lsShmFreeTop_t*) offset2ptr(bp->x_freeOffset);
        if ((ap->x_aMarker == LSSHM_FREE_AMARKER) 
                && ((ap->x_freeSize + bp->x_freeOffset) == offset))
        {
            if (joinFlag)
            {
                // join above block
                ap->x_freeSize += size;
                
                lsShmFreeBot_t *xp = (lsShmFreeBot_t*) offset2ptr(offset + size - sizeof(lsShmFreeBot_t));
                xp->x_bMarker = LSSHM_FREE_BMARKER;
                xp->x_freeOffset = bp->x_freeOffset;
                
                markTopUsed((lsShmFreeTop_t*)offset2ptr(offset));
            }
            return true;
        }
    }
    return false;
}

//
//  Simple bucket type allocation and release
//
LsShm_offset_t LsShm::allocPage( LsShm_size_t pagesize , int & remap )
{
    LsShm_offset_t offset;
    LsShm_size_t availSize;
    
    LSSHM_CHECKSIZE(pagesize);
    
    pagesize = roundUnitSize(pagesize);
    remap = 0;

    //
    //  MUTEX SHOULD BE HERE for multi process/thread environment
    // Only use lock when m_status is Ready.
    if (m_status == LSSHM_READY)
        if (lock())
            return 0; // no lock aquired...
    
    // Allocate from free space
    if ( (offset = getFromFreeList( pagesize )) )
    {
        goto out;
    }
    
    // Allocate from heap space
    availSize = avail();
    if (pagesize > availSize)
    {
        LsShm_size_t needSize;
        // min 16 unit at a time
#define MIN_NUMUNIT_SHIFT 4
        needSize = (pagesize - availSize) > (getShmMap()->x_unitSize << MIN_NUMUNIT_SHIFT)
            ? (pagesize - availSize) 
            : (getShmMap()->x_unitSize << MIN_NUMUNIT_SHIFT) ;
        // allocate 8 pages more!
        if (expand(roundToPageSize(needSize)))
        {
            offset = 0;
            goto out;
        }
        if (getShmMap() != m_pShmMap_o)
            remap = 1;
    }
    offset = getShmMap()->x_size;
    markTopUsed((lsShmFreeTop_t *)offset2ptr(offset));
    used(pagesize);
    
out:
    if (m_status == LSSHM_READY)
        unlock();
    return offset;
}

void LsShm::releasePage( LsShm_offset_t offset, LsShm_size_t pagesize )
{
    //
    //  MUTEX SHOULD BE HERE for multi process/thread environment
    //
    lock();
    
    pagesize = roundUnitSize(pagesize);
    if ((offset + pagesize) == getShmMap()->x_size)
    {
        getShmMap()->x_size -= pagesize;
        goto out;
    }
    if (isFreeBlockAbove(offset, pagesize, 1))
    {
        lsShmFreeTop_t * ap;
        offset = ((lsShmFreeBot_t*)offset2ptr(offset + pagesize - sizeof(lsShmFreeBot_t)))->x_freeOffset;
        ap = (lsShmFreeTop_t*)offset2ptr(offset);
        isFreeBlockBelow(offset, ap->x_freeSize, 2);
        goto out;
    }
    if (isFreeBlockBelow(offset, pagesize, 1))
    {
        goto out;
    }
    joinFreeList(offset, pagesize);
    
out:
    unlock();
}

void LsShm::joinFreeList( LsShm_offset_t offset, LsShm_size_t size)
{
    // setup myself as free block
    lsShmFreeTop_t * np = (lsShmFreeTop_t *) offset2ptr(offset);
    np->x_aMarker = LSSHM_FREE_AMARKER;
    np->x_freeSize = size;
    np->x_freeNext = getShmMap()->x_freeOffset;
    np->x_freePrev = 0;
    
    // join myself to freeList
    getShmMap()->x_freeOffset = offset;
    
    if (np->x_freeNext)
    {
        lsShmFreeTop_t * xp = (lsShmFreeTop_t *) offset2ptr(np->x_freeNext);
        xp->x_freePrev = offset;
    }
    
    lsShmFreeBot_t * bp = (lsShmFreeBot_t *) offset2ptr(offset + size - sizeof(lsShmFreeBot_t));
    bp->x_freeOffset = offset;
    bp->x_bMarker = LSSHM_FREE_BMARKER;
}

//
//  SHM Reg sub interface
//
lsShmRegBlk_t * LsShm::allocRegBlk( lsShmRegBlkHdr_t * p_from )
{
    LsShm_offset_t offset;
    lsShmRegBlkHdr_t * p_regBlkHdr;
    
    int remap;
    offset = allocPage(sizeof(lsShmRegElem_t) * getShmMap()->x_numRegPerBlk, remap);
    if (!offset)
            return NULL;
    p_regBlkHdr = (lsShmRegBlkHdr_t *)offset2ptr(offset);
    p_regBlkHdr->x_capacity = getShmMap()->x_numRegPerBlk;
    p_regBlkHdr->x_size = 1;
    p_regBlkHdr->x_next = 0;
    
    if (p_from)
    {
        p_regBlkHdr->x_startNum = p_from->x_startNum + p_from->x_capacity - 1;
        p_from->x_next = offset;
    }
    else
    {
        getShmMap()->x_regBlkOffset = offset;
        p_regBlkHdr->x_startNum = 0;
    }
    
    getShmMap()->x_regLastBlkOffset = offset;
    getShmMap()->x_maxRegNum += (getShmMap()->x_numRegPerBlk-1);
    
    // initialize the block data
    register lsShmReg_t * p_reg;
    p_reg = (lsShmReg_t *)(((lsShmRegElem_t *) p_regBlkHdr) + 1);
    
    register int i;
    for (i = 0; i < (p_regBlkHdr->x_capacity-1); i++)
    {
        p_reg->x_regNum = p_regBlkHdr->x_startNum + i;
        p_reg++;
    }
    
    // note lsShmRegBlk_t == lsShmRegBlkHdr_t == lsShmReg_t[0]
    return (lsShmRegBlk_t *) p_regBlkHdr;
}
    
lsShmReg_t * LsShm::findRegBlk( lsShmRegBlkHdr_t * p_from, int num  )
{
    if ((LsShm_offset_t)num >= p_from->x_startNum) 
    {
        if ((LsShm_offset_t)num < (p_from->x_startNum + p_from->x_capacity - 1))
            return (lsShmReg_t *) (((lsShmRegElem_t *)p_from) + 1 + num - p_from->x_startNum ) ;
        if (p_from->x_next)
            return findRegBlk ((lsShmRegBlkHdr_t*)offset2ptr(p_from->x_next), num);
    }
    return NULL;
}

lsShmReg_t * LsShm::getReg( int num  )
{
    if (getShmMap()->x_regBlkOffset)
        return findRegBlk( (lsShmRegBlkHdr_t*) offset2ptr(getShmMap()->x_regBlkOffset), num );
    return NULL;
}

int LsShm::expandReg(int maxRegNum)
{
    register lsShmRegBlkHdr_t * p_regBlkHdr = 0;
    
    if (maxRegNum <= 0)
        maxRegNum = getShmMap()->x_maxRegNum + getShmMap()->x_numRegPerBlk - 1;
    
    if (!getShmMap()->x_regLastBlkOffset)
        p_regBlkHdr = (lsShmRegBlkHdr_t*) allocRegBlk(0);
    else
        p_regBlkHdr = (lsShmRegBlkHdr_t*)offset2ptr( getShmMap()->x_regLastBlkOffset );
    while ( p_regBlkHdr && ( getShmMap()->x_maxRegNum <= (LsShm_offset_t)maxRegNum ) )
        p_regBlkHdr = (lsShmRegBlkHdr_t *)allocRegBlk(p_regBlkHdr);
    
    return getShmMap()->x_maxRegNum;
}

lsShmReg_t * LsShm::findReg( const char * name )
{
    if (!name)
        return NULL;
        
    LsShm_size_t nameSize;
    nameSize = (LsShm_size_t)strlen(name);
    if ((!nameSize) || (nameSize > (sizeof(lsShmReg_t)-sizeof(LsShm_offset_t))))
        return NULL;
    
    lsShmRegBlkHdr_t * p_regBlkHdr;
    if (getShmMap()->x_regBlkOffset)
        p_regBlkHdr = (lsShmRegBlkHdr_t *) offset2ptr (getShmMap()->x_regBlkOffset);
    else
        return NULL;
    
    while (p_regBlkHdr)
    {
        register uint32_t i, last_blkNum; 
        last_blkNum = p_regBlkHdr->x_startNum + p_regBlkHdr->x_capacity;
        register lsShmRegElem_t * p_regElem = (lsShmRegElem_t *) p_regBlkHdr;
        
        // search the block!
        for (i = p_regBlkHdr->x_startNum + 1; i < last_blkNum; i++)
        {
            p_regElem++; // skip the first one
            if (!memcmp(name, p_regElem->x_reg.x_name, nameSize))
                return &p_regElem->x_reg;
        }
        if (p_regBlkHdr->x_next)
            p_regBlkHdr = (lsShmRegBlkHdr_t *)offset2ptr(p_regBlkHdr->x_next);
        else
            return NULL;
    }
    return NULL;
}

//
// @brief - addReg add the given name into Registry. 
// @brief   return the ptr of the registry
//
lsShmReg_t* LsShm::addReg( const char* name )
{
    if (!name)
        return NULL;
    int nameSize;
    nameSize = strlen(name)+1; // including null
    if ((!nameSize) || (nameSize > LSSHM_MAXNAMELEN))
        return NULL;
    
    register lsShmReg_t * p_reg;
    if ( (p_reg = findReg(name)) )
        return p_reg;
    
    lsShmRegBlkHdr_t * p_regBlkHdr;
    if (getShmMap()->x_regBlkOffset)
        p_regBlkHdr = (lsShmRegBlkHdr_t *) offset2ptr (getShmMap()->x_regBlkOffset);
    else
        return NULL;
    
    register lsShmRegElem_t * p_regElem;
    while (p_regBlkHdr)
    {
        if (p_regBlkHdr->x_size < p_regBlkHdr->x_capacity)
        {
            // find the empty slot
            p_regElem = ((lsShmRegElem_t *) p_regBlkHdr) + 1;
            for (int i =  1; i < p_regBlkHdr->x_capacity; i++, p_regElem++)
            {
                if (!p_regElem->x_reg.x_flag)
                {
                    memcpy((char *)p_regElem->x_reg.x_name, name, nameSize);
                    if (nameSize < LSSHM_MAXNAMELEN)
                        p_regElem->x_reg.x_name[nameSize] = 0;
                    p_regElem->x_reg.x_flag = 0x1; // assign it!
                    p_regBlkHdr->x_size++;
                    return (lsShmReg_t *)p_regElem;
                }
            }
        }
        else
        {
            if (p_regBlkHdr->x_next)
                p_regBlkHdr = (lsShmRegBlkHdr_t *)offset2ptr(p_regBlkHdr->x_next);
            else
                p_regBlkHdr = NULL;
        }
    }
    return NULL;
}

