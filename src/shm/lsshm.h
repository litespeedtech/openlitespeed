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
#ifndef LSSHM_H
#define LSSHM_H

#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/types.h>

#include <util/hashstringmap.h>
#include <shm/lsshmtypes.h>
#include <shm/lsshmlock.h>

/**
 * @file
 * LsShm - LiteSpeed Shared Memory
 */


#ifdef LSSHM_DEBUG_ENABLE
class debugBase;// These two should be the same size...
#endif
class LsShmPool;
class LsShmHash;

typedef union
{
    struct
    {
        uint16_t    m_iMajor;
        uint8_t     m_iMinor;
        uint8_t     m_iRel;
    } x;
    uint32_t    m_iVer;
} LsShmVersion;

//
// LiteSpeed Shared Memory Map
//
//  +------------------------------------------------
//  | HEADER
//  |    --> maxSize      --------------------------+
//  |    --> offset free  ----------------+         |
//  |    --> offset xdata ------+         |         |
//  |           size of xdata   |         |         |
//  |-------------------------- |         |         |
//  | xdata area          <-----+         |         |
//  |-------------------------            |         |
//  | Free area           <---------------+         |
//  |-------------------------  <-------------------+
//
// struct mutex_t;


//
//  SHM Reg
// NOTE: ALL UNIT 0 of each block is for book keeping purpose
//  Simple key/value pair
//  But this is expandable... Not a good idea to have more than 64 entries...
//  (1) Should be reference via regNumber
//  (2) Can be reference via name (but slow).
//  (3) should sizeof LsShmReg == LsShmRegBlkHdr
//
typedef struct
{
    uint16_t          x_iRegNum;   // max 64k registry
    uint16_t          x_iFlag;     // 0x1 - assigned
    uint32_t          x_iValue;    // typical offset
    uint8_t           x_aName[LSSHM_MAXNAMELEN];
} LsShmReg;

typedef struct
{
    uint16_t          x_iRegNum;
    uint16_t          x_iFlag;
    uint16_t          x_iCapacity; // max
    uint16_t          x_iSize;     // current size
    LsShmOffset_t     x_iNext;     // next block
    uint32_t          x_iStartNum; // start reg number
} LsShmRegBlkHdr;

typedef union
{
    LsShmRegBlkHdr    x_hdr;
    LsShmReg          x_reg;
} LsShmRegElem;

// x_iFlag
#define LSSHM_REGFLAG_CLR       0x0000
#define LSSHM_REGFLAG_SET       0x0001
#define LSSHM_REGFLAG_PID       0x0002

// NOTE: offset element[0] == header
typedef struct
{
#define LSSHM_MAX_REG_PERUNIT   (LSSHM_MINUNIT/sizeof(LsShmRegElem))
    LsShmRegElem x_aEntries[LSSHM_MAX_REG_PERUNIT];
} LsShmRegBlk;

typedef struct
{
    uint32_t          x_iMagic;
    LsShmVersion      x_version;
    uint8_t           x_aName[LSSHM_MAXNAMELEN];
    LsShmSize_t       x_iSize;              // the current max
    LsShmSize_t       x_iMaxSize;           // the file size
    LsShmSize_t       x_iUnitSize ;         // 1k
    LsShmOffset_t     x_iLockOffset[2];     // 0=Shm, 1=Reg
    LsShmOffset_t     x_iFreeOffset;
    LsShmOffset_t     x_iRegBlkOffset;      // 1st reg Offset
    LsShmOffset_t     x_iRegLastBlkOffset;  // last reg Offset
    LsShmOffset_t     x_iRegPerBlk;         // num Reg Per Blk
    LsShmOffset_t     x_iMaxRegNum;         // current max Reg number

    LsShmOffset_t     x_iXdataOffset;       // the xdata offset
    LsShmSize_t       x_iXdataSize;         // the xdata size
} LsShmMap;

//
// Internal for free link
//
#define LSSHM_FREE_AMARKER 0x0123fedc
#define LSSHM_FREE_BMARKER 0xba984567
//
//  Two block to indicate a free block
//
typedef struct
{
    LsShmSize_t      x_iAMarker;
    LsShmSize_t      x_iFreeSize;           // size of the freeblock
    LsShmOffset_t    x_iFreeNext;
    LsShmOffset_t    x_iFreePrev;
} LShmFreeTop;

typedef struct
{
    LsShmOffset_t    x_iFreeOffset;         // back to the begin
    LsShmSize_t      x_iBMarker;
} LShmFreeBot;

/*
 *   data struct all the library routines to access the map
 */
class LsShm : public ls_shm_s, public LsShmLock
{
public:
    static LsShm *open(const char *mapName, int initialSize,
                       const char *pBaseDir = NULL);
    void close()
    {
        if (downRef() == 0)
            delete this;
    }

private:
    explicit LsShm(const char *mapName, LsShmSize_t initialSize,
                   const char *pBaseDir = NULL);
    //explicit LsShm(const char *mapName, LsShmSize_t initialSize);
    ~LsShm();

public:
    // remove systemwise shm maxsize
    static void setSysShmMaxSize(LsShmSize_t size)
    {
        s_iSysMaxShmSize = size > LSSHM_MINSPACE ? size : LSSHM_MINSPACE;
    }
    static LsShmSize_t sysShmMaxSize()
    {
        return s_iSysMaxShmSize;
    }

    static LsShmStatus_t setSysShmDir(const char *dirName);
    static const char *sysShmDir()
    {
        return s_pDirBase;
    }

    static LsShmStatus_t setErrMsg(const char *fmt, ...);
    static const char *getErrMsg()
    {
        return s_aErrMsg;
    }
    static void clrErrMsg()
    {
        s_aErrMsg[0] = '\0';
    }
    
    void setShmMaxSize(LsShmSize_t size)
    {
        m_iMaxShmSize = size > LSSHM_MINSPACE ? size : LSSHM_MINSPACE;
    }
    LsShmSize_t shmMaxSize() const
    {
        return m_iMaxShmSize;
    }

    LsShmSize_t maxSize() const
    {
        return getShmMap()->x_iMaxSize;
    }

    LsShmSize_t oldMaxSize() const
    {
        return m_iMaxSizeO;
    };

    const char *fileName() const
    {
        return m_pFileName;
    }

    const char *name() const
    {
        return getShmMap() ? (char *)getShmMap()->x_aName : NULL;
    }

    const char *mapName() const
    {
        return m_pMapName;
    }

    LsShmMap *getShmMap() const
    {
        return m_pShmMap;
    }

    LsShmOffset_t allocPage(LsShmSize_t pagesize, int &remapped);
    void releasePage(LsShmOffset_t offset, LsShmSize_t pagesize);

    LsShmSize_t unitSize() const
    {
        return m_iUnitSize;
    }

    LsShmPool *getGlobalPool();
    LsShmPool *getNamedPool(const char *pName);


    // dangerous... no check fast access
    void *offset2ptr(LsShmOffset_t offset) const
    {
        if (offset == 0)
            return NULL;
        assert(offset < getShmMap()->x_iMaxSize);
        return (void *)(((uint8_t *)getShmMap()) + offset);
    }  // map size

    LsShmOffset_t ptr2offset(const void *ptr) const
    {
        if (ptr == NULL)
            return 0;
        assert((ptr < (((uint8_t *)getShmMap()) + getShmMap()->x_iMaxSize))
               && (ptr > (const void *)getShmMap()));
        return (LsShmOffset_t)((uint8_t *)ptr - (uint8_t *)getShmMap());
    }

    LsShmSize_t avail() const
    {
        return getShmMap()->x_iMaxSize - getShmMap()->x_iSize;
    }

    LsShmStatus_t status() const
    {
        return m_status;
    }

#ifdef notdef
    // xdata
    LsShmOffset_t xdataOffset() const
    {
        return getShmMap()->x_iXdataOffset;
    };

    LsShmOffset_t xdataSize() const
    {
        return getShmMap()->x_iXdataSize;
    };
#endif

    LsShmLock *lockPool()
    {
        return this;
    };

    LsShmStatus_t remap();

    //
    //   registry
    //
    LsShmReg      *getReg(int regNum); // user should check name and x_iFlag
    int            clrReg(int regNum);
    LsShmReg      *findReg(const char *name);
    LsShmReg      *addReg(const char *name);
    int            expandReg(int maxRegNum); // expand the current reg

    int            recoverOrphanShm();

    int lockShm()
    {
        return lock();
    }
    int trylockShm()
    {
        return trylock();
    }
    int unlockShm()
    {
        return unlock();
    }

    int ref() const
    {
        return m_iRef;
    }

    HashStringMap < lsi_shmobject_t *> &getObjBase()
    {
        return m_objBase;
    }

#ifdef LSSHM_DEBUG_ENABLE
    // for debug purpose
    friend class debugBase;
#endif

private:
    LsShm();
    LsShm(const LsShm &other);
    LsShm &operator=(const LsShm &other);
    bool operator==(const LsShm &other);



    void syncData2Obj(); // sync data to Object memory

    LsShmRegBlk    *allocRegBlk(LsShmRegBlkHdr *pFrom);
    LsShmRegBlk    *findRegBlk(LsShmRegBlkHdr *pFrom, int num);
    void            chkRegBlk(LsShmRegBlkHdr *pFrom, int *pCnt);

    LsShmStatus_t   expand(LsShmSize_t newsize);

    int lock()
    {
        return lsi_shmlock_lock(m_pShmLock);
    }

    int trylock()
    {
        return lsi_shmlock_trylock(m_pShmLock);
    }

    int unlock()
    {
        return lsi_shmlock_unlock(m_pShmLock);
    }

    int lockReg()
    {
        return lsi_shmlock_lock(m_pRegLock);
    }

    int unlockReg()
    {
        return lsi_shmlock_unlock(m_pRegLock);
    }

    LsShmStatus_t setupLocks()
    {
        return (lsi_shmlock_setup(m_pShmLock) || lsi_shmlock_setup(m_pRegLock)) ?
               LSSHM_ERROR : LSSHM_OK;
    }

    // only use by physical mapping
    LsShmSize_t roundToPageSize(LsShmSize_t size) const
    {
        return ((size + s_iPageSize - 1) / s_iPageSize) * s_iPageSize;
    }

    LsShmSize_t roundUnitSize(LsShmSize_t pagesize) const
    {
        return ((pagesize + unitSize() - 1) / unitSize()) * unitSize();
    }

    void used(LsShmSize_t size)
    {
        getShmMap()->x_iSize += size;
    }

    bool isFreeBlockAbove(LsShmOffset_t offset, LsShmSize_t size,
                          int joinFlag);
    bool isFreeBlockBelow(LsShmOffset_t offset, LsShmSize_t size,
                          int joinFlag);
    void joinFreeList(LsShmOffset_t offset, LsShmSize_t size);

    LsShmOffset_t getFromFreeList(LsShmSize_t pagesize);
    void reduceFreeFromBot(LShmFreeTop *ap,
                           LsShmOffset_t offset, LsShmSize_t newsize);
    void disconnectFromFree(LShmFreeTop *ap, LShmFreeBot *bp);
    void markTopUsed(LShmFreeTop *ap)
    {
        ap->x_iAMarker = 0;
    }

    void            cleanup();
    LsShmStatus_t   checkMagic(LsShmMap *mp, const char *mName) const;
    LsShmStatus_t   init(const char *name, LsShmSize_t size);
    LsShmStatus_t   map(LsShmSize_t size);
    LsShmStatus_t   expandFile(LsShmOffset_t from, LsShmSize_t incrSize);
    void            unmap();

#ifdef DELAY_UNMAP
    void            unmapLater();
    void            unmapHelper(LsShmMap *, LsShmSize_t);
    void            unmapCheck();

    typedef struct
    {
        LsShmMap    *p;
        LsShmSize_t  size;
        timeval      tv;
    } unmapStat_t;
#define MAX_UNMAP 0x10
    unmapStat_t     m_uArray[MAX_UNMAP];
    unmapStat_t    *m_uCur;
    unmapStat_t    *m_uEnd;
#endif

    int upRef()
    {
        return ++m_iRef;
    }
    int downRef()
    {
        return --m_iRef;
    }


    // various objects within the SHM
    HashStringMap < lsi_shmobject_t *> m_objBase;

    uint32_t                m_iMagic;
    static LsShmVersion     s_version;
    static uint32_t         s_iShmHdrSize;
    static uint32_t         s_iPageSize;
    static LsShmSize_t      s_iSysMaxShmSize;
    static const char      *s_pDirBase;
    static char             s_aErrMsg[];
    LsShmSize_t             m_iUnitSize;
    LsShmSize_t             m_iMaxShmSize;

    LsShmStatus_t           m_status;
    char                   *m_pFileName;    // dir + mapName + ext
    char                   *m_pMapName;
    int                     m_iFd;
    int                     m_iRemoveFlag;  // remove file
    lsi_shmlock_t          *m_pShmLock;
    lsi_shmlock_t          *m_pRegLock;

    LsShmReg               *m_pShmReg;
    LsShmMap               *m_pShmMap;
    LsShmMap               *m_pShmMapO;
    LsShmSize_t             m_iMaxSizeO;

    void                   *m_pShmData;     // start of data
    LsShmPool              *m_pGPool;
    int                     m_iRef;
};

#endif
