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
#ifdef RUN_TEST


#include <lsr/lsr_objpool.h>
#include <lsr/lsr_pool.h>

#include <stdio.h>
#include <string.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"


typedef struct lsr_objpooltest_s
{
    int key;
    int val;
} lsr_objpooltest_t;

void *getNew()
{
    return lsr_palloc( sizeof( lsr_objpooltest_t ));
}

void releaseObj( void *pObj )
{
    lsr_pfree( pObj );
}

TEST( lsr_objpooltest )
{
    printf( "START LSR OBJPOOL TEST\n" );
    lsr_objpool_t op;
    const int chunk = 10;
    int i;
    lsr_objpooltest_t *pObjs[chunk] = { NULL };
    lsr_objpool( &op, chunk, getNew, releaseObj );
    CHECK( lsr_objpool_size( &op ) == 0 );
    CHECK( lsr_objpool_capacity( &op ) == 0 );
    CHECK( lsr_objpool_poolSize( &op ) == 0 );
    CHECK( lsr_objpool_begin( &op ) == lsr_objpool_end( &op ));
    
    for( i = 0; i < chunk; ++i )
    {
        pObjs[i] = (lsr_objpooltest_t *)lsr_objpool_get( &op );
        CHECK( pObjs[i] != NULL );
        CHECK( lsr_objpool_size( &op ) == chunk - i - 1 );
        CHECK( lsr_objpool_capacity( &op ) == chunk );
        CHECK( lsr_objpool_poolSize( &op ) == chunk );
    }
    CHECK( lsr_objpool_begin( &op ) == lsr_objpool_end( &op ));
    lsr_objpooltest_t *p = (lsr_objpooltest_t *)lsr_objpool_get( &op );
    CHECK( p != NULL );
    CHECK( lsr_objpool_size( &op ) == chunk - 1 );
    CHECK( lsr_objpool_capacity( &op ) == chunk * 2 );
    CHECK( lsr_objpool_poolSize( &op ) == chunk * 2 );
    CHECK( lsr_objpool_begin( &op ) != lsr_objpool_end( &op ));
    
    lsr_objpooltest_t *pObjs2[chunk] = { NULL };
    lsr_objpool_getMulti( &op, (void **)pObjs2, chunk );
    CHECK( lsr_objpool_size( &op ) == chunk - 1 );
    CHECK( lsr_objpool_capacity( &op ) == chunk * 3 );
    CHECK( lsr_objpool_poolSize( &op ) == chunk * 3 );
    
    for( i = 0; i < chunk; ++i )
    {
        CHECK( pObjs2[i] != NULL );
        /* For the sake of reducing repetition, I put the following
        * checks in the same for loop.  These are separate tests 
        * by nature, but I didn't want to have back to back for loops.
        */
        lsr_objpool_recycle( &op, pObjs[i] );
        CHECK( lsr_objpool_size( &op ) == chunk + i );
        CHECK( lsr_objpool_capacity( &op ) == chunk * 3 );
        CHECK( lsr_objpool_poolSize( &op ) == chunk * 3 );
    }
    lsr_objpool_recycle( &op, p );
    lsr_objpool_shrinkTo( &op, chunk );
    CHECK( lsr_objpool_size( &op ) == chunk );
    CHECK( lsr_objpool_capacity( &op ) == chunk * 3 );
    CHECK( lsr_objpool_poolSize( &op ) == chunk * 2 );
    CHECK( lsr_objpool_begin( &op ) + chunk == lsr_objpool_end( &op ));
    
    lsr_objpool_recycleMulti( &op, (void **)pObjs2, chunk );
    CHECK( lsr_objpool_size( &op ) == chunk * 2 );
    CHECK( lsr_objpool_capacity( &op ) == chunk * 3 );
    CHECK( lsr_objpool_poolSize( &op ) == chunk * 2 );
    CHECK( lsr_objpool_begin( &op ) + (2*chunk) == lsr_objpool_end( &op ));
    
    lsr_objpool_getMulti( &op, (void **)pObjs, chunk );
    CHECK( lsr_objpool_size( &op ) == chunk );
    CHECK( lsr_objpool_capacity( &op ) == chunk * 3 );
    CHECK( lsr_objpool_poolSize( &op ) == chunk * 2 );
    
    lsr_objpool_getMulti( &op, (void **)pObjs2, chunk );
    CHECK( lsr_objpool_size( &op ) == 0 );
    CHECK( lsr_objpool_capacity( &op ) == chunk * 3 );
    CHECK( lsr_objpool_poolSize( &op ) == chunk * 2 );
    
    p = (lsr_objpooltest_t *)lsr_objpool_get( &op );
    CHECK( lsr_objpool_size( &op ) == chunk - 1 );
    CHECK( lsr_objpool_capacity( &op ) == chunk * 3 );
    CHECK( lsr_objpool_poolSize( &op ) == chunk * 3 );
    
    lsr_objpool_recycle( &op, p );
    lsr_objpool_recycleMulti( &op, (void **)pObjs, chunk );
    lsr_objpool_recycleMulti( &op, (void **)pObjs2, chunk );
    CHECK( lsr_objpool_size( &op ) == chunk * 3 );
    CHECK( lsr_objpool_capacity( &op ) == chunk * 3 );
    CHECK( lsr_objpool_poolSize( &op ) == chunk * 3 );
    
    
    
    lsr_objpool_d( &op );
}

#endif
