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
#ifndef LSSHMLOCK_H
#define LSSHMLOCK_H
/*
*   LsShm - LiteSpeed Shared memory Lock
*/
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>

#include "lsshmtypes.h"

class debugBase;

//
// LiteSpeed Shared Memory Lock Map
//
//  +------------------------------------------------
//  | HEADER
//  |    --> maxSize      --------------------------+
//  |    --> offset free  ----------------+         |
//  |                                     |         |
//  |----------------------------         |         |
//  |    --> Mutex                        |         |
//  |                        <------------+         |
//  |------------------------<----------------------+
//
typedef struct {
    uint32_t            x_magic;    
    uint8_t             x_name[LSSHM_MAXNAMELEN];
    LsShm_size_t        x_maxSize;      /* the file size   */
    LsShm_size_t        x_maxElem;
    int16_t             x_unitSize;     /* each of each lsi_shmlock_t */
    int16_t             x_elemSize;
    LsShm_offset_t      x_freeOffset;   /* first free lock */
} lsShmLockMap_t;

typedef union {
    lsi_shmlock_t       x_lock;
    LsShm_offset_t      x_next;
} lsShmLockElem_t;

class LsShmLock : public ls_shmobject_s
{
private:
    uint32_t                    m_magic;
    static uint32_t             s_pageSize;
    static uint32_t             s_hdrSize;
    LsShm_status_t              m_status;
    char *                      m_fileName;
    char *                      m_mapName;
    int                         m_fd; 
    int                         m_removeFlag; 
    lsShmLockMap_t *            m_pShmLockMap;
    lsShmLockElem_t *           m_pShmLockElem;
    LsShm_size_t                m_maxSize_o; 
    
public:
    explicit LsShmLock(
                  const char * dir,
                  const char * mapName,
                  LsShm_size_t initialSize);
    ~LsShmLock();

    static const char * getDefaultShmDir();

    const char * fileName() const
    { return m_fileName; }
    
    const char * mapName() const
    { return m_mapName; }
    
    lsShmLockMap_t * getShmLockMap() const
    {   return m_pShmLockMap; }

    const char * name() const
    {    return getShmLockMap() ? (char *)getShmLockMap()->x_name : NULL; }
   
    inline lsi_shmlock_t * offset2pLock( LsShm_offset_t offset ) const
    {   assert(offset < getShmLockMap()->x_maxSize);
        return (lsi_shmlock_t *)(((uint8_t*)getShmLockMap()) + offset); } 

    inline void * offset2ptr( LsShm_offset_t offset ) const
    {   assert(offset < getShmLockMap()->x_maxSize);
        return (void *)(((uint8_t*)getShmLockMap()) + offset); } 
   
    inline LsShm_offset_t ptr2offset( const void * ptr) const
    {   assert(ptr < ((uint8_t*)getShmLockMap()) + getShmLockMap()->x_maxSize);
        return (LsShm_offset_t) ((uint8_t*)ptr - (uint8_t*)getShmLockMap()); }

    inline LsShm_offset_t pLock2offset( lsi_shmlock_t * pLock)
    {   assert((uint8_t *)pLock < ((uint8_t*)getShmLockMap()) + getShmLockMap()->x_maxSize);
        return (LsShm_offset_t) ((uint8_t*)pLock - (uint8_t*)getShmLockMap()); }
   
    inline LsShm_status_t status () const
    {   return m_status; }

    inline int pLock2LockNum( lsi_shmlock_t * pLock ) const
    {
        return ( (lsShmLockElem_t*)pLock - m_pShmLockElem );
    }
    inline lsShmLockElem_t * lockNum2pElem( const int num ) const
    {
        return ((num < 0) || (num >= (int)getShmLockMap()-> x_maxElem)) ? 
                NULL : (m_pShmLockElem + num);
    }
    inline lsi_shmlock_t * lockNum2pLock( const int num ) const
    {
        return (lsi_shmlock_t *)(((num < 0) || (num >= (int)getShmLockMap()-> x_maxElem)) ? 
                NULL : (m_pShmLockElem + num) ) ;
    }

   
    lsi_shmlock_t * allocLock();
    int freeLock(lsi_shmlock_t * );

private:
    LsShmLock( ) ;
    LsShmLock( const LsShmLock & other );
    LsShmLock & operator=( const LsShmLock & other );
    bool operator==( const LsShmLock & other );
    
private:
    // only use by physical mapping
    inline LsShm_size_t roundToPageSize(LsShm_size_t size) const
    { return ((size + s_pageSize - 1) / s_pageSize) * s_pageSize; }
    
    LsShm_status_t      expand(LsShm_size_t size);
    LsShm_status_t      expandFile( LsShm_offset_t from, LsShm_offset_t incrSize);
    
    void                cleanup();
    LsShm_status_t      checkMagic(lsShmLockMap_t * mp, const char * mName) const;
    LsShm_status_t      init(const char * name, LsShm_size_t size);
    LsShm_status_t      map(LsShm_size_t);
    void                unmap();
    void                setupFreeList( LsShm_offset_t to );
    
    // for debug purpose
    friend class debugBase;
};

#endif
