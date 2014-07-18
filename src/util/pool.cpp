/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

/***************************************************************************
    $Id: pool.cpp,v 1.1.1.1 2013/12/22 23:42:51 gwang Exp $
                         -------------------
    begin                : Wed Apr 30 2003
    author               : Gang Wang
    email                : gwang@litespeedtech.com
 ***************************************************************************/

#include <util/pool.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <new>

Pool::Pool( int max_unit_bytes )
    : m_pFreeLists( NULL )
    , m_pNodeList( NULL )
    , _S_start_free( NULL )
    , _S_end_free( NULL )
    , _S_heap_size( 0 )
{
    m_iMaxUnitBytes = roundUp( max_unit_bytes );
    size_t size = sizeof( _AllocLink *) * m_iMaxUnitBytes / _ALIGN;
    m_pFreeLists = (_AllocLink* volatile *) ::malloc( size );
    assert( m_pFreeLists );
    memset( (void *)m_pFreeLists, 0, size );
}

Pool::~Pool()
{
    //::free( (void *)m_pFreeLists );
    //m_pFreeLists = 0;
    //releaseAllBlocks();
}

//#if defined (_STANDARD_) || !defined(NDEBUG)
#define DEBUG_POOL
#define NO_POOL_NEW
//#endif

char * Pool::dupstr( const char * p )
{
    if ( p )
        return dupstr( p, strlen( p ) + 1 );
    return NULL;
}

char * Pool::dupstr( const char * p, int len )
{
    if ( p )
    {
        char * ps = (char *)allocate2( len );
        if ( ps )
            memmove( ps, p, len );
        return ps;
    }
    return NULL;
}
#ifndef NO_POOL_NEW
void *operator new( size_t sz, const std::nothrow_t& ) throw ()
{   return g_pool.allocate2( sz );   }
void *operator new( size_t sz ) throw (std::bad_alloc)
{   return g_pool.allocate2( sz );   }
#endif

void* Pool::allocate2(size_t num)
{
    num += 4;
    uint32_t * p = (uint32_t *)allocate( num );
    if ( p )
    {
        *p = num;
        return p + 1;
    }
    return NULL;
}

void*
Pool::allocate(size_t num)
{
    void* ret = 0;

    num = roundUp(num);
#ifdef DEBUG_POOL
        ret = ::malloc(num);
#else    
    if (num > (size_t) m_iMaxUnitBytes)
        ret = ::malloc(num);
    else
    {
        _AllocLink* volatile* my_free_list = m_pFreeLists
            + freeListIndex(num);

#ifdef _REENTRANT
        LockMutex lock( m_allocator_mutex );
#endif
        _AllocLink* result = *my_free_list;
        if (result == 0)
            ret = refill(num);
        else

        {
            *my_free_list = result -> m_pNext;
            ret = result;
        }
    }
#endif
    return ret;
}

#ifndef NO_POOL_NEW
void operator delete( void * p) throw ()
{   return g_pool.deallocate2( p );    }
void operator delete( void * p, const std::nothrow_t& ) throw ()
{   return g_pool.deallocate2( p );    }

#endif

void Pool::deallocate2(void* p)
{
    if ( p )
        deallocate( ((uint32_t *)p) - 1, *(((uint32_t *)p) - 1 ) );
}

// p may not be 0
void
Pool::deallocate(void* p, size_t num)
{
#ifdef DEBUG_POOL
        ::free(p);
#else
    if (num > (size_t) m_iMaxUnitBytes)
        ::free(p);
    else
    {
        _AllocLink* volatile*  my_free_list
            = m_pFreeLists + freeListIndex(num);
        _AllocLink* q = (_AllocLink*)p;

#ifdef _REENTRANT
        LockMutex lock( m_allocator_mutex );
#endif
        q -> m_pNext = *my_free_list;
        *my_free_list = q;

    }
#endif
}


char*
Pool::chunkAlloc(size_t size,
                       int& nobjs)
{
    char* result;

    size_t total_bytes = size * nobjs;
    size_t bytes_left = _S_end_free - _S_start_free;

    if (bytes_left >= total_bytes)
    {
        result = _S_start_free;
        _S_start_free += total_bytes;
        return(result);
    }
    else if (bytes_left >= size)
    {
        nobjs = (int)(bytes_left/size);
        total_bytes = size * nobjs;
        result = _S_start_free;
        _S_start_free += total_bytes;
        return(result);
    }
    else
    {
        size_t bytes_to_get =
            2 * total_bytes + roundUp(_S_heap_size >> 4);
        //printf( "allocate %d bytes!\n", bytes_to_get );
        // make use of the left-over piece.
        if (bytes_left > 0)
        {
            _AllocLink* volatile* my_free_list =
                m_pFreeLists + freeListIndex(bytes_left);

            ((_AllocLink*)_S_start_free) -> m_pNext = *my_free_list;
            *my_free_list = (_AllocLink*)_S_start_free;
        }
        _S_start_free = (char*)::malloc(bytes_to_get + sizeof( _AllocLink *)+ ( sizeof( long ) - 4));
        if (0 == _S_start_free)
        {
            size_t i;
            _AllocLink* volatile* my_free_list;
            _AllocLink* p;

            i = size;
            for (; i <= (size_t) m_iMaxUnitBytes; i += (size_t) _ALIGN)
            {
                my_free_list = m_pFreeLists + freeListIndex(i);
                p = *my_free_list;
                if (0 != p)
                {
                    *my_free_list = p -> m_pNext;
                    _S_start_free = (char*)p;

                    _S_end_free = _S_start_free + i;
                    return(chunkAlloc(size, nobjs));
                }
            }
            _S_end_free = 0;    // In case of exception.
            _S_start_free = (char*)::malloc(bytes_to_get + sizeof( _AllocLink *)+ ( sizeof( long ) - 4));
        }
        if ( _S_start_free )
        {
            ((_AllocLink *)_S_start_free)-> m_pNext = m_pNodeList;
            m_pNodeList = (_AllocLink *)_S_start_free;
            _S_start_free += sizeof( _AllocLink *);
        }
        _S_heap_size += bytes_to_get;
        _S_end_free = _S_start_free + bytes_to_get;
        return(chunkAlloc(size, nobjs));
    }
}

void Pool::releaseAllBlocks()
{
    while( m_pNodeList )
    {
        _AllocLink * pBlock = m_pNodeList;
        m_pNodeList = m_pNodeList-> m_pNext;
        ::free( pBlock );
    }
}


void*
Pool::refill(size_t num)
{
    int nobjs = 20;
    char* chunk = chunkAlloc(num, nobjs);
    _AllocLink* volatile* my_free_list;
    _AllocLink* result;
    _AllocLink* currentFreeList;
    _AllocLink* nextFreeList;
    int i;

    if (1 == nobjs) return(chunk);
    my_free_list = m_pFreeLists + freeListIndex(num);

    result = (_AllocLink*)chunk;
    *my_free_list = nextFreeList = (_AllocLink*)(chunk + num);
    for (i = 1; ; i++) {
        currentFreeList = nextFreeList;
        nextFreeList = (_AllocLink*)((char*)nextFreeList + num);
        if (nobjs - 1 == i) {
            currentFreeList -> m_pNext = 0;
            break;
        } else {
            currentFreeList -> m_pNext = nextFreeList;
        }
    }
    return(result);
}

void* Pool::reallocate2(void* p, size_t new_sz)
{
    if ( p )
    {
        uint32_t * pNew = ((uint32_t *)p) - 1;
        uint32_t old_sz = *pNew;
        new_sz += 4;
        if ( old_sz >= new_sz )
            return p;
        pNew = (uint32_t *)reallocate( pNew, old_sz, new_sz );
        if ( pNew )
        {
            *pNew = new_sz;
            return pNew + 1;
        }
        else
            return NULL;
    }
    else
        return allocate2( new_sz );
}


void*
Pool::reallocate(void* p,
                       size_t old_sz,
                       size_t new_sz)
{
    void* result;
    size_t copy_sz;
    old_sz = roundUp(old_sz);
    new_sz = roundUp(new_sz);
    if (old_sz == new_sz) return(p);
    if ((old_sz > (size_t) m_iMaxUnitBytes)&&(new_sz > (size_t) m_iMaxUnitBytes)) {
        return(::realloc(p, new_sz));
    }
    result = allocate(new_sz);
    if ( result )
    {
        copy_sz = new_sz > old_sz? old_sz : new_sz;
        if ( copy_sz )
            ::memmove(result, p, copy_sz);
        if ( old_sz )
            deallocate(p, old_sz);
    }
    return(result);
}





