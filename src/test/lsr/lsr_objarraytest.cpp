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
 
#include <lsr/lsr_objarray.h>
#include <lsr/lsr_xpool.h>

#include <stdio.h>
#include <string.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

typedef struct testpair_s
{
    int key;
    int val;
} testpair_t;


TEST( lsr_ObjArrayTest_test)
{
    int i;
    lsr_objarray_t array;
    lsr_xpool_t pool;
    lsr_objarray_init( &array, sizeof( testpair_t ));
    lsr_xpool_init( &pool );
    
    CHECK( lsr_objarray_getcapacity( &array ) == 0 );
    CHECK( lsr_objarray_getsize( &array ) == 0 );
    CHECK( lsr_objarray_getarray( &array ) == NULL );
    lsr_objarray_guarantee( &array, &pool, 10 );
    CHECK( lsr_objarray_getcapacity( &array ) == 10 );
    CHECK( lsr_objarray_getsize( &array ) == 0 );
    CHECK( lsr_objarray_getarray( &array ) != NULL );
    for( i = 0; i < 10; ++i )
    {
        testpair_t *buf = (testpair_t *)lsr_objarray_getnew( &array );
        buf->key = buf->val = i + 1;
    }
    CHECK( lsr_objarray_getsize( &array ) == 10 );
    CHECK( lsr_objarray_getcapacity( &array ) == 10 );
    CHECK( lsr_objarray_getnew( &array ) == NULL );
    for( i = 0; i < 10; ++i )
    {
        testpair_t *buf = (testpair_t *)lsr_objarray_getobj( &array, i );
        CHECK( buf->key == i + 1 && buf->val == i + 1 );
    }
    CHECK( lsr_objarray_getobj( &array, 0 ) == lsr_objarray_getarray( &array ));
    CHECK( lsr_objarray_getobj( &array, -1 ) == NULL );
    CHECK( lsr_objarray_getobj( &array, 11 ) == NULL );
    lsr_objarray_guarantee( &array, &pool, 20 );
    CHECK( lsr_objarray_getcapacity( &array ) == 20 );
    CHECK( lsr_objarray_getsize( &array ) == 10 );
    CHECK( lsr_objarray_getarray( &array ) != NULL );
    CHECK( lsr_objarray_getobj( &array, 15 ) == NULL );
    for( i = 10; i < 20; ++i )
    {
        testpair_t *buf = (testpair_t *)lsr_objarray_getnew( &array );
        buf->key = buf->val = i + 1;
    }
    for( i = 0; i < 20; ++i )
    {
        testpair_t *buf = (testpair_t *)lsr_objarray_getobj( &array, i );
        CHECK( buf->key == i + 1 && buf->val == i + 1 );
    }
    lsr_objarray_setcapacity( &array, &pool, 30 );
    CHECK( lsr_objarray_getcapacity( &array ) == 30 );
    CHECK( lsr_objarray_getsize( &array ) == 20 );
    CHECK( lsr_objarray_getarray( &array ) != NULL );
    for( i = 20; i < 30; ++i )
    {
        testpair_t *buf = (testpair_t *)lsr_objarray_getnew( &array );
        buf->key = buf->val = i - 1;
    }
    for( i = 0; i < 20; ++i )
    {
        testpair_t *buf = (testpair_t *)lsr_objarray_getobj( &array, i );
        CHECK( buf->key == i + 1 && buf->val == i + 1 );
    }
    for( i = 20; i < 30; ++i )
    {
        testpair_t *buf = (testpair_t *)lsr_objarray_getobj( &array, i );
        CHECK( buf->key == i - 1 && buf->val == i - 1 );
    }
    
    lsr_objarray_release( &array, &pool );
    lsr_xpool_destroy( &pool );
    
    lsr_objarray_init( &array, sizeof( testpair_t ));
    
    CHECK( lsr_objarray_getcapacity( &array ) == 0 );
    CHECK( lsr_objarray_getsize( &array ) == 0 );
    CHECK( lsr_objarray_getarray( &array ) == NULL );
    lsr_objarray_guarantee( &array, NULL, 10 );
    CHECK( lsr_objarray_getcapacity( &array ) == 10 );
    CHECK( lsr_objarray_getsize( &array ) == 0 );
    CHECK( lsr_objarray_getarray( &array ) != NULL );
    for( i = 0; i < 10; ++i )
    {
        testpair_t *buf = (testpair_t *)lsr_objarray_getnew( &array );
        buf->key = buf->val = i + 1;
    }
    CHECK( lsr_objarray_getsize( &array ) == 10 );
    CHECK( lsr_objarray_getcapacity( &array ) == 10 );
    CHECK( lsr_objarray_getnew( &array ) == NULL );
    for( i = 0; i < 10; ++i )
    {
        testpair_t *buf = (testpair_t *)lsr_objarray_getobj( &array, i );
        CHECK( buf->key == i + 1 && buf->val == i + 1 );
    }
    CHECK( lsr_objarray_getobj( &array, 0 ) == lsr_objarray_getarray( &array ));
    CHECK( lsr_objarray_getobj( &array, -1 ) == NULL );
    CHECK( lsr_objarray_getobj( &array, 11 ) == NULL );
    lsr_objarray_guarantee( &array, NULL, 20 );
    CHECK( lsr_objarray_getcapacity( &array ) == 20 );
    CHECK( lsr_objarray_getsize( &array ) == 10 );
    CHECK( lsr_objarray_getarray( &array ) != NULL );
    lsr_objarray_release( &array, NULL );
    
}

#endif
