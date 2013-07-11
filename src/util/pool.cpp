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
#include <util/pool.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <new>

Pool::Pool( int max_unit_bytes )
{
}

Pool::~Pool()
{
    //::free( (void *)m_pFreeLists );
    //m_pFreeLists = 0;
    //releaseAllBlocks();
}

//#if !defined(NDEBUG)
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
        ret = ::malloc(num);
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
        ::free(p);
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
    old_sz = roundUp(old_sz);
    new_sz = roundUp(new_sz);
    if (old_sz == new_sz) return(p);
    return(::realloc(p, new_sz));
}





