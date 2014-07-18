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
#ifndef LSSHM_H
#define LSSHM_H
/*
*   LsShm - LiteSpeed Shared memory
*/
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>

#include <util/hashstringmap.h>
#include <shm/lsshmtypes.h>
#include <shm/lsshmlock.h>

#ifdef LSSHM_DEBUG_ENABLE
class debugBase;// These two should be the same size...
#endif
class LsShmPool;
class LsShmHash;

typedef union {
    struct {
    uint16_t    major;
    uint8_t     minor;
    uint8_t     rel;
    } x;
    uint32_t    ver;
} lsShmVersion_t;

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
//  (3) should sizeof lsShmReg_t == lsShmRegBlkHdr_t
//
typedef struct {
    uint16_t            x_regNum;   // max 64k registry
    uint16_t            x_flag;     // 0x1 - assigned
    uint32_t            x_value;    // typical offset 
    uint8_t             x_name[LSSHM_MAXNAMELEN];
} lsShmReg_t;

typedef struct {
    uint16_t            x_regNum;
    uint16_t            x_flag;
    uint16_t            x_capacity; // max
    uint16_t            x_size;     // current size
    LsShm_offset_t      x_next;     // next block
    uint32_t            x_startNum; // start reg number
} lsShmRegBlkHdr_t;

typedef union {
    lsShmRegBlkHdr_t    x_hdr;
    lsShmReg_t          x_reg;
} lsShmRegElem_t;

// NOTE: offset element[0] == header
typedef struct {
#define LSSHM_MAX_REG_PERUNIT   (LSSHM_MINUNIT/sizeof(lsShmRegElem_t)) 
    lsShmRegElem_t x_entries[LSSHM_MAX_REG_PERUNIT];
} lsShmRegBlk_t;

typedef struct {
    uint32_t            x_magic;    
    lsShmVersion_t      x_version;
    uint8_t             x_name[LSSHM_MAXNAMELEN];
    LsShm_size_t        x_size;         /* the current max                      */
    LsShm_size_t        x_maxSize;      /* the file size                        */
    LsShm_size_t        x_unitSize ;    /* 1k */
    LsShm_offset_t      x_lockOffset;
    LsShm_offset_t      x_freeOffset;
    LsShm_offset_t      x_regBlkOffset; /* 1st reg Offset */
    LsShm_offset_t      x_regLastBlkOffset; /* last reg Offset */
    LsShm_offset_t      x_numRegPerBlk; /* num Reg Per Blk */
    LsShm_offset_t      x_maxRegNum;    /* num Reg Per Blk */
    
    LsShm_offset_t      x_xdataOffset;  /* the xdata offset                      */
    LsShm_size_t        x_xdataSize;    /* the xdata size                      */
    
} lsShmMap_t;

//
// Internal for free link
//
#define LSSHM_FREE_AMARKER 0x0123fedc
#define LSSHM_FREE_BMARKER 0xba984567
//
//  Two block to indicate a free block
//
typedef struct {
    LsShm_size_t        x_aMarker;
    LsShm_size_t        x_freeSize;     /* size of the freeblock */
    LsShm_offset_t      x_freeNext;
    LsShm_offset_t      x_freePrev;
} lsShmFreeTop_t;

typedef struct {
    LsShm_offset_t      x_freeOffset;   /* back to the begin */
    LsShm_size_t        x_bMarker;
} lsShmFreeBot_t;

/*
*   data struct all the library routines to access the map
*/
class LsShm : public ls_shm_s, public LsShmLock
{
    uint32_t                    m_magic;
    static lsShmVersion_t       s_version;
    static uint32_t             s_shmHdrSize;
    static uint32_t             s_pageSize;
    static LsShm_size_t         s_sysMaxShmSize;
    static const char *         s_dirBase;
    LsShm_size_t                m_unitSize ;
    LsShm_size_t                m_maxShmSize ;
    
    LsShm_status_t              m_status;
    char *                      m_fileName; // dir + mapName + ext
    char *                      m_mapName;
    int                         m_fd; 
    int                         m_removeFlag; // remove file 
    lsi_shmlock_t *             m_pShmLock;
    
    lsShmReg_t *                m_pShmReg;
    lsShmMap_t *                m_pShmMap;
    lsShmMap_t *                m_pShmMap_o;
    LsShm_size_t                m_maxSize_o;

    /* USER data start from here */
    void *                      m_pShmData;
    
public:
   static LsShm * get(const char * mapName, int initialSize);
   void unget() { if ( down_ref() == 0 ) delete this; }
   
private:   
   explicit LsShm(const char * mapName,
                  LsShm_size_t initialSize);
   ~LsShm();
   
public:
    // remove systemwise shm maxsize
   static void setSysShmMaxSize(LsShm_size_t size)
   {    s_sysMaxShmSize = size > LSSHM_MINSPACE ? size : LSSHM_MINSPACE; }
   static LsShm_size_t sysShmMaxSize()
   {    return s_sysMaxShmSize; }
   
   static LsShm_status_t setSysShmDir(const char * dirName);
   static const char * sysShmDir()
   {    return s_dirBase; }
   
   void setShmMaxSize(LsShm_size_t size)
   {    m_maxShmSize = size > LSSHM_MINSPACE ? size : LSSHM_MINSPACE; }
   LsShm_size_t shmMaxSize() const
   {    return m_maxShmSize; }
   
   
   LsShm_size_t maxSize() const
   { return getShmMap()->x_maxSize; }
   
   LsShm_size_t oldMaxSize() const
   { return m_maxSize_o; };
   
   const char * fileName() const
   { return m_fileName; }
   
   const char * name() const
   { return getShmMap() ? (char *)getShmMap()->x_name : NULL; }
   
   const char * mapName() const
   { return m_mapName; }
   
   lsShmMap_t * getShmMap() const
   { return m_pShmMap; }
   
   LsShm_offset_t allocPage(LsShm_size_t pagesize, int & remapped);
   void releasePage(LsShm_offset_t offset, LsShm_size_t pagesize);
   
   LsShm_size_t unitSize() const
   { return m_unitSize; }
   
   // dangerous... no check fast access
   inline void * offset2ptr( LsShm_offset_t offset ) const
   {   if (!offset) return NULL;
       assert(offset < getShmMap()->x_maxSize);
       return (void *)(((uint8_t*)getShmMap()) + offset); }  /* the map size */
   
   inline LsShm_offset_t ptr2offset( const void * ptr) const
   {   if (!ptr) return 0;
       assert( (ptr < (((uint8_t*)getShmMap()) + getShmMap()->x_maxSize) )
                && (ptr > (const void *)getShmMap()) );
       return (LsShm_offset_t) ((uint8_t*)ptr - (uint8_t*)getShmMap()); }
   
   inline LsShm_size_t avail() const
   { return getShmMap()->x_maxSize - getShmMap()->x_size; }
   
   inline LsShm_status_t status () const
   { return m_status; }
   
   // xdata
   inline LsShm_offset_t xdataOffset () const
   {   return getShmMap()->x_xdataOffset; };
   
   inline LsShm_offset_t xdataSize () const
   {   return getShmMap()->x_xdataSize; };
   
   inline LsShmLock * lockPool()
   {    return this; };
   
   LsShm_status_t remap();
   
   //
   //   registry
   //
   lsShmReg_t *     getReg(int regNum); // user should check name and x_flag
   lsShmReg_t *     findReg(const char * name);
   lsShmReg_t *     addReg(const char * name);
   int              expandReg(int newRegTo); // expand the current reg
   
   inline int   lockShm()
   {   return lock(); }
   inline int   trylockShm()
   {   return trylock(); }
   inline int   unlockShm()
   {   return unlock(); }
   
   inline int ref() const
   { return m_ref; }
   
private:  
   void syncData2Obj(); // sync data to Object memory 
   
    lsShmRegBlk_t *  allocRegBlk(lsShmRegBlkHdr_t * p_from);
    lsShmReg_t *     findRegBlk(lsShmRegBlkHdr_t * p_from, int num);

    LsShm_status_t         expand(LsShm_size_t newsize);
    
    inline int lock()
    {   return lsi_shmlock_lock( m_pShmLock );   }
   
    inline int trylock()
    {   return lsi_shmlock_trylock( m_pShmLock );   }
    
    inline int unlock()
    {   return lsi_shmlock_unlock( m_pShmLock );   }

    inline LsShm_status_t setupLock()
    {   return lsi_shmlock_setup( m_pShmLock) ? LSSHM_ERROR : LSSHM_OK; }
    
private:
    LsShm( ) ;
    LsShm( const LsShm& other );
    LsShm& operator=( const LsShm& other );
    bool operator==( const LsShm& other );
    
    // only use by physical mapping
    inline LsShm_size_t roundToPageSize(LsShm_size_t size) const
    { return ((size + s_pageSize - 1) / s_pageSize) * s_pageSize; }
    
    inline LsShm_size_t roundUnitSize(LsShm_size_t pagesize) const
    { return ((pagesize + unitSize() - 1) / unitSize()) * unitSize(); }
    
    inline void used ( LsShm_size_t size)
    {   getShmMap()->x_size += size; };
    
    bool isFreeBlockAbove(LsShm_offset_t offset, LsShm_size_t size, int joinFlag);
    bool isFreeBlockBelow(LsShm_offset_t offset, LsShm_size_t size, int joinFlag);
    
    LsShm_offset_t getFromFreeList(LsShm_size_t pagesize);
    void joinFreeList(LsShm_offset_t, LsShm_size_t);
    
    void reduceFreeFromBot(lsShmFreeTop_t * ap, 
                    LsShm_offset_t offset,
                    LsShm_size_t newsize);
    void disconnectFromFree(lsShmFreeTop_t * ap, lsShmFreeBot_t * bp);
    inline void markTopUsed(lsShmFreeTop_t * ap)
    { ap->x_aMarker = 0; }
    
private:
    void        cleanup();
    LsShm_status_t    checkMagic(lsShmMap_t * mp, const char * mName) const;
    LsShm_status_t    init(const char * name, LsShm_size_t size);
    LsShm_status_t    map(LsShm_size_t);
    LsShm_status_t    expandFile(LsShm_offset_t from, LsShm_size_t incrSize);
    void              unmap();
    
#ifdef DELAY_UNMAP    
    void              unmapLater();
    void              unmapHelper(lsShmMap_t*, LsShm_size_t);
    void              unmapCheck();
    
typedef struct {
    lsShmMap_t * p;
    LsShm_size_t size;
    timeval      tv;
} unmapStat_t;
#define MAX_UNMAP 0x10
    unmapStat_t       m_uArray[MAX_UNMAP];
    unmapStat_t *     m_uCur;
    unmapStat_t *     m_uEnd;
#endif
    
   // house keeping
   static HashStringMap < LsShm * > s_base;
   int               m_ref;
   inline int up_ref() {   return ++m_ref; }
   inline int down_ref() {   return --m_ref; }
   
   // various objects within the SHM
   HashStringMap < lsi_shmobject_t *> m_objBase;
   
public:
   inline HashStringMap < lsi_shmobject_t *> & getObjBase()
        { return m_objBase ; }
   
#ifdef LSSHM_DEBUG_ENABLE
    // for debug purpose
    friend class debugBase;
#endif
};

#endif
