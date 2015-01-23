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
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <shm/lsshmtypes.h>
#include <shm/lsshmlock.h>
#include <shm/lsshm.h>
#include <util/gpath.h>

extern "C" {
    int ls_expandfile(int fd, size_t fromsize, size_t incrsize);
};

uint32_t LsShmLock::s_pageSize = LSSHM_PAGESIZE;
uint32_t LsShmLock::s_hdrSize  = ((sizeof(lsShmLockMap_t)+0xf)&~0xf); // align 16

LsShmLock::LsShmLock( const char * dirName,
                const char * mapName,
                LsShm_size_t size) 
                    : m_magic(LSSHM_LOCK_MAGIC)
                    , m_status(LSSHM_NOTREADY)
                    , m_fileName(NULL)
                    , m_mapName(NULL)
                    , m_fd(0)
                    , m_removeFlag(0)
                    , m_pShmLockMap(0)
                    , m_pShmLockElem(0)
                    , m_maxSize_o(0)
{
    char buf[0x1000];
    if (!dirName)
        dirName = getDefaultShmDir();
    if (!mapName)
        mapName = LSSHM_SYSSHM_FILENAME;
    
    snprintf(buf, 0x1000, "%s/%s.%s",
             dirName,
             mapName,
             LSSHM_SYSLOCK_FILE_EXT);
    
    m_fileName = strdup(buf);
    m_mapName = strdup(mapName);
    
#if 0
    m_status = LSSHM_NOTREADY;
    m_fd = 0;
    m_pShmLockMap = 0;
    m_pShmLockElem = 0;
    m_maxSize_o = 0;
#endif
    
    /* set the size */
    size = ((size + s_pageSize - 1) / s_pageSize) * s_pageSize;
    if ( (!m_fileName) 
        || (!m_mapName)
        || (strlen(m_mapName) >= LSSHM_MAXNAMELEN)
    )
    {
        m_status = LSSHM_BADPARAM;
        return;
    }
    
    m_status = init(m_mapName, size);
    
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

LsShm_status_t LsShmLock::checkMagic(lsShmLockMap_t * mp, const char * mName) const
{
    return (  (mp->x_magic != m_magic)
                || strncmp((char *)mp->x_name, mName, LSSHM_MAXNAMELEN) )
            ? LSSHM_BADMAPFILE : LSSHM_OK;
}

void LsShmLock::cleanup()
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
            unlink(m_fileName);
            m_removeFlag = 0;
        }
        free(m_fileName);
        m_fileName = 0;
    }
    if (m_mapName)
    {
        free(m_mapName);
        m_mapName = 0;
    }
}

LsShm_status_t LsShmLock::expandFile( LsShm_offset_t from,  LsShm_offset_t incrSize )
{
    // Just truncate it... 
    // if (ftruncate(m_fd, from + incrSize))
    if (ls_expandfile(m_fd, from, incrSize))
        return LSSHM_ERROR;
    // we may want to memset it to zero... here /
    return LSSHM_OK;
}

LsShm_status_t LsShmLock::init(const char * name, LsShm_size_t size)
{
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
        /* creating a new map */
        if (expandFile(0, roundToPageSize( size)) || map(size))
            return LSSHM_ERROR;
        getShmLockMap()->x_magic = m_magic;
        getShmLockMap()->x_maxSize = size;
        getShmLockMap()->x_freeOffset = 0;
        getShmLockMap()->x_maxElem = 0;
        getShmLockMap()->x_unitSize = sizeof(lsi_shmlock_t);
        getShmLockMap()->x_elemSize = sizeof(lsShmRegElem_t);
        strcpy((char *)getShmLockMap()->x_name, name);
        
        setupFreeList( size );
        
        m_removeFlag = 0;
    }
    else
    {
        if (mystat.st_size < s_hdrSize)
            return LSSHM_BADMAPFILE;
            
        // only map the header size to ensure the file is good.
        if (map(s_hdrSize))
            return LSSHM_ERROR;
        
        if ((getShmLockMap()->x_maxSize != mystat.st_size)
                || checkMagic(getShmLockMap(), name))
            return LSSHM_BADMAPFILE;

        // expand the file if needed... wont shrink
        if (size > getShmLockMap()->x_maxSize)
        {
            int maxSize = getShmLockMap()->x_maxSize;
            if (expandFile(maxSize, size))
                return LSSHM_ERROR;
            
            
            getShmLockMap()->x_maxSize = size;
            unmap();
            if (map(size))
                return LSSHM_ERROR;
            setupFreeList(  size );
        }
    }
    return LSSHM_OK;
}

LsShm_status_t LsShmLock::expand(LsShm_size_t size)
{
    LsShm_size_t xsize = getShmLockMap()->x_maxSize;
    
    if ( expandFile(xsize, size) )
        return LSSHM_ERROR;
    unmap();
    if ( map(xsize + size) )
        return LSSHM_ERROR;
    getShmLockMap()->x_maxSize = xsize + size;
    
    return LSSHM_OK;
}

LsShm_status_t LsShmLock::map(LsShm_size_t size)
{
    uint8_t    *p;
    
    // p = (uint8_t *)mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_POPULATE, m_fd, 0);
    p = (uint8_t *)mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, m_fd, 0);
    if ( p != MAP_FAILED )
    {
        m_pShmLockMap = (lsShmLockMap_t *)p;
        m_pShmLockElem = (lsShmLockElem_t *) ((uint8_t *)p + s_hdrSize);
        m_maxSize_o = size;
        return LSSHM_OK;
    }
    // bad mmap - should touch the return handle anymore
    return LSSHM_ERROR;
}

void LsShmLock::unmap()
{
    if (m_pShmLockMap)
    {
        munmap(m_pShmLockMap, m_maxSize_o);
        m_pShmLockMap = 0;
        m_pShmLockElem = 0;
        m_maxSize_o = 0;
    }
}

//
//  Note - LockNum 0 is for myself...
//
void LsShmLock::setupFreeList( LsShm_offset_t to )
{
    register lsShmLockElem_t * p_elem = m_pShmLockElem + getShmLockMap()->x_maxElem;
    LsShm_offset_t from = ptr2offset( p_elem );
    LsShm_size_t size = to - from;
    int numAdd = size / sizeof(lsShmLockElem_t);
    
    int curNum = getShmLockMap()->x_maxElem;
    int lastNum = curNum + numAdd;
    int x_lastNum = lastNum;    // save it for later
    
    p_elem = m_pShmLockElem + --lastNum; // Can't call lockNum2pElem off bounded!
    // p_elem = lockNum2pElem(--lastNum);
    p_elem->x_next = getShmLockMap()-> x_freeOffset;
    while (--numAdd >= 1)
    {
        --p_elem;
        p_elem->x_next = lastNum--;
    }
    
    if (lastNum == 0)
        lastNum++;      // keep this for myself
    getShmLockMap()->x_freeOffset = lastNum;
    getShmLockMap()->x_maxElem = x_lastNum;
}

lsi_shmlock_t* LsShmLock::allocLock()
{
    register lsShmLockElem_t * pElem;
    LsShm_offset_t offset;
    if ( (offset = getShmLockMap()->x_freeOffset) )
    {
        pElem = m_pShmLockElem + offset;
        m_pShmLockMap->x_freeOffset = pElem->x_next;
        // return pElem->x_lock;
        return (lsi_shmlock_t *)pElem; // save some code...
    }
    return NULL;
}

int LsShmLock::freeLock( lsi_shmlock_t * pLock)
{
    int num = pLock2LockNum(pLock);
    lsShmLockElem_t * pElem;

    pElem = m_pShmLockElem + num;
    pElem->x_next = getShmLockMap()->x_freeOffset;
    return 0;
}

const char * LsShmLock::getDefaultShmDir()
{
    static int isDirTected = 0;
    if( isDirTected == 0 )
    {
        const char *pathname = "/dev/shm";
        struct stat sb;
        if( stat(pathname, &sb) == 0 && S_ISDIR(sb.st_mode) )
            isDirTected = 1;
        else
            isDirTected = 2;
    }
    
    if ( isDirTected == 1 )
        return LSSHM_SYSSHM_DIR1;
    else
        return LSSHM_SYSSHM_DIR2;
}


