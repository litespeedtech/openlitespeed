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
#ifndef LSSHMLOCK_H
#define LSSHMLOCK_H

#include <shm/lsshmtypes.h>

#include <assert.h>
#include <stdint.h>

/**
 * @file
 * LsShm - LiteSpeed Shared Memory Lock
 */


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
typedef struct
{
    uint32_t            x_iMagic;
    LsShmCnt_t          x_iMaxElem;
    uint64_t            x_id;
    LsShmXSize_t        x_iMaxSize;      // the file size
    LsShmOffset_t       x_iFreeOffset;   // first free lock
} LsShmLockMap;

typedef union
{
    lsi_shmlock_t       x_lock;
    LsShmOffset_t       x_iNext;
} LsShmLockElem;

class LsShmLock : public ls_shmobject_s
{
    friend class LsShm;

public:
    LsShmLock();
    ~LsShmLock();

    static const char *getDefaultShmDir();


    LsShmLockMap *getShmLockMap() const
    {   return m_pShmLockMap; }

    lsi_shmlock_t *offset2pLock(LsShmOffset_t offset) const
    {
        assert(offset < getShmLockMap()->x_iMaxSize);
        return (lsi_shmlock_t *)(((uint8_t *)getShmLockMap()) + offset);
    }

    void *offset2ptr(LsShmOffset_t offset) const
    {
        assert(offset < getShmLockMap()->x_iMaxSize);
        return (void *)(((uint8_t *)getShmLockMap()) + offset);
    }

    LsShmOffset_t ptr2offset(const void *ptr) const
    {
        assert(ptr <
               ((uint8_t *)getShmLockMap()) + getShmLockMap()->x_iMaxSize);
        return (LsShmOffset_t)((uint8_t *)ptr - (uint8_t *)getShmLockMap());
    }

    LsShmOffset_t pLock2offset(lsi_shmlock_t *pLock)
    {
        assert((uint8_t *)pLock <
               ((uint8_t *)getShmLockMap()) + getShmLockMap()->x_iMaxSize);
        return (LsShmOffset_t)((uint8_t *)pLock - (uint8_t *)getShmLockMap());
    }

    LsShmStatus_t status() const
    {   return m_status; }

    int pLock2LockNum(lsi_shmlock_t *pLock) const
    {   return ((LsShmLockElem *)pLock - m_pShmLockElem); }

    LsShmLockElem *lockNum2pElem(const int num) const
    {
        return ((num < 0) || (num >= (int)getShmLockMap()->x_iMaxElem)) ?
               NULL : (m_pShmLockElem + num);
    }
    lsi_shmlock_t *lockNum2pLock(const int num) const
    {
        return (lsi_shmlock_t *)
               (((num < 0) || (num >= (int)getShmLockMap()->x_iMaxElem)) ?
                NULL : (m_pShmLockElem + num));
    }

    lsi_shmlock_t *allocLock();
    int freeLock(lsi_shmlock_t *pLock);

private:
    LsShmLock(const LsShmLock &other);
    LsShmLock &operator=(const LsShmLock &other);
    bool operator==(const LsShmLock &other);

private:
    // only use by physical mapping
    LsShmXSize_t  roundToPageSize(LsShmXSize_t size) const
    {   return ((size + s_iPageSize - 1) / s_iPageSize) * s_iPageSize; }

    LsShmStatus_t       expand(LsShmXSize_t size);
    LsShmStatus_t       expandFile(LsShmOffset_t from, LsShmXSize_t incrSize);

    void                cleanup();
    LsShmStatus_t       checkMagic(LsShmLockMap *mp) const;
    LsShmStatus_t       init(const char *pFile, int fd,
                             LsShmXSize_t size, uint64_t id);
    uint64_t            getId() const;
    LsShmStatus_t       map(LsShmXSize_t size);
    void                unmap();
    void                setupFreeList(LsShmOffset_t to);

    uint32_t            m_iMagic;
    static LsShmSize_t  s_iPageSize;
    static LsShmSize_t  s_iHdrSize;
    LsShmStatus_t       m_status;
    int                 m_iFd;
    LsShmLockMap       *m_pShmLockMap;
    LsShmLockElem      *m_pShmLockElem;
    LsShmXSize_t        m_iMaxSizeO;

    // for debug purpose
    friend class debugBase;
};

#endif
