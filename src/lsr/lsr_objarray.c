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

#include <lsr/lsr_objarray.h>
#include <lsr/lsr_pool.h>


static void *lsr_objarray_alloc( lsr_objarray_t *pThis, lsr_xpool_t *pool, int numObj )
{
    return ( pool?
        lsr_xpool_realloc( pool,
          lsr_objarray_getarray( pThis ), numObj * lsr_objarray_getobjsize( pThis ) ):
        lsr_prealloc(
          lsr_objarray_getarray( pThis ), numObj * lsr_objarray_getobjsize( pThis ) ) );
}

static void lsr_objarray_releasearray( lsr_objarray_t *pThis, lsr_xpool_t *pool )
{
    if ( pool )
        lsr_xpool_free( pool, lsr_objarray_getarray( pThis ) );
    else
        lsr_pfree( lsr_objarray_getarray( pThis ) );
}

void lsr_objarray_release( lsr_objarray_t* pThis, lsr_xpool_t* pool )
{
    if ( pThis->m_pArray )
        lsr_objarray_releasearray( pThis, pool );
    pThis->m_capacity = 0;
    pThis->m_size = 0;
    pThis->m_objSize = 0;
    pThis->m_pArray = NULL;
}

void lsr_objarray_setcapacity( lsr_objarray_t *pThis, lsr_xpool_t *pool, int numObj )
{
    if ( pThis->m_capacity < numObj )
        pThis->m_pArray = lsr_objarray_alloc( pThis, pool, numObj );
    pThis->m_capacity = numObj;
}





