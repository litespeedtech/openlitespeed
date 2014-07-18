/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

/***************************************************************************
    $Id: pool.h,v 1.1.1.1 2013/12/22 23:42:51 gwang Exp $
                         -------------------
    begin                : Wed Apr 30 2003
    author               : Gang Wang
    email                : gwang@litespeedtech.com
 ***************************************************************************/

#ifndef POOL_H
#define POOL_H


/**
  *@author Gang Wang
  */

#ifdef _REENTRENT
#include <thread/tmutex.h>
#endif

#include <stddef.h>

#define MAX_UNIT_SIZE    256

class Pool
{
public:

    enum {_ALIGN = 4 };
    static size_t
    roundUp(size_t bytes)
    { return (((bytes) + (size_t) _ALIGN-1) & ~((size_t) _ALIGN - 1)); }

private:
    int m_iMaxUnitBytes;
    struct _AllocLink
    {
        struct _AllocLink*    m_pNext;
    };

    _AllocLink* volatile *   m_pFreeLists;
    _AllocLink* volatile     m_pNodeList;
    // Chunk allocation state.
    char*         _S_start_free;
    char*         _S_end_free;
    size_t        _S_heap_size;
#ifdef _REENTRANT
    //Mutex m_allocator_mutex;
#endif

    static size_t
    freeListIndex(size_t bytes)
    { return (((bytes) + (size_t)_ALIGN-1)/(size_t)_ALIGN - 1); }

    // Returns an object of size num, and optionally adds to size num
    // free list.
    void*
    refill(size_t num);

    // Allocates a chunk for nobjs of size size.  nobjs may be reduced
    // if it is inconvenient to allocate the requested number.
    char*
    chunkAlloc(size_t size, int& nobjs);

    void releaseAllBlocks();

public:

    explicit Pool( int max_unit_bytes = 256 );
    ~Pool();

    // num must be > 0
    void * allocate(size_t num);

    // p may not be 0
    void deallocate(void* p, size_t num);

    void * reallocate(void* p, size_t old_sz, size_t new_sz);

    void * allocate2(size_t num);
    void   deallocate2(void* p);
    void * reallocate2(void* p, size_t new_sz);
    char * dupstr( const char * p );
    char * dupstr( const char * p, int len );
};

extern Pool g_pool;

#endif
