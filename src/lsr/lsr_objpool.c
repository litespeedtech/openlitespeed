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

#include <lsr/lsr_objpool.h>
#include <lsr/lsr_pool.h>

int lsr_objpool_alloc( lsr_objpool_t *pThis, int size )
{
    if ((int)lsr_ptrlist_capacity( &pThis->m_freeList ) < pThis->m_poolSize + size )
        if ( lsr_ptrlist_reserve( &pThis->m_freeList, pThis->m_poolSize + size ))
            return -1;
    int i = 0;
    for( ; i < size; ++i )
    {
        void *pObj = pThis->m_getNew();
        if ( pObj )
        {
            lsr_ptrlist_unsafe_push_back( &pThis->m_freeList, pObj );
            ++pThis->m_poolSize;
        }
        else
            return -1;
    }
    return 0;
}

lsr_inline void lsr_objpool_clear( lsr_objpool_t *pThis )
{   lsr_ptrlist_clear( &pThis->m_freeList );    }

void lsr_objpool( lsr_objpool_t *pThis, int chunkSize,
                lsr_objpool_getNewFn getNewFn, lsr_objpool_releaseObjFn releaseFn )
{
    pThis->m_chunkSize = chunkSize <= 0 ? 10 : chunkSize;
    pThis->m_poolSize = 0;
    pThis->m_getNew = getNewFn;
    pThis->m_releaseObj = releaseFn;
    lsr_ptrlist( &pThis->m_freeList, 0 );
}

void lsr_objpool_d( lsr_objpool_t *pThis )
{
    lsr_objpool_shrinkTo( pThis, 0 );
    pThis->m_chunkSize = pThis->m_poolSize = 0;
    lsr_ptrlist_d( &pThis->m_freeList );
}

void *lsr_objpool_get( lsr_objpool_t *pThis )
{
    if ( lsr_ptrlist_empty( &pThis->m_freeList ))
        if ( lsr_objpool_alloc( pThis, pThis->m_chunkSize ))
            return NULL;
    return lsr_ptrlist_pop_back( &pThis->m_freeList );
}

int lsr_objpool_getMulti( lsr_objpool_t *pThis, void **pObj, int n )
{
    if ( (int)lsr_ptrlist_size( &pThis->m_freeList ) < n)
        if ( lsr_objpool_alloc( pThis, (n<pThis->m_chunkSize) ? pThis->m_chunkSize : n ))
            return 0;
    lsr_ptrlist_unsafe_pop_backn( &pThis->m_freeList, pObj, n );
    return n;
}
