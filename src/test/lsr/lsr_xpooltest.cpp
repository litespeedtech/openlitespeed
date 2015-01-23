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
#ifdef RUN_TEST
 
#include <lsr/lsr_xpool.h>

#include <stdio.h>
#include <string.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

//#define LSR_XPOOL_DEBUG
#define LSR_XPOOL_SB_SIZE 4096

TEST( lsr_XPoolTest_test)
{
#ifdef LSR_XPOOL_DEBUG
    printf( "Starting XPool Test\n" );
#endif
    static int sizes[] =
      { 24, 3, 32, 40, 8, 24, 46,
          LSR_XPOOL_SB_SIZE-8-16, LSR_XPOOL_SB_SIZE-8-15, 1024, 1024, 2048, 800 };
    
    lsr_xpool_t pool;
    char *ptr, cmp[256];
    memset( cmp, 0, 256 );
    lsr_xpool_init( &pool );
    
    ptr = (char *)lsr_xpool_alloc( &pool, LSR_XPOOL_SB_SIZE );
    CHECK( pool.m_pSB == NULL );
    CHECK( pool.m_pBblk != NULL );
    CHECK( lsr_xpool_is_empty( &pool ) == 0 );
    char *ptr2 = (char *)lsr_xpool_alloc( &pool, LSR_XPOOL_SB_SIZE );
    char *ptr3 = (char *)lsr_xpool_alloc( &pool, LSR_XPOOL_SB_SIZE );
    lsr_xpool_free( &pool, (void *)ptr2 );
    lsr_xpool_free( &pool, (void *)ptr3 );
    lsr_xpool_free( &pool, (void *)ptr );
    CHECK( pool.m_pSB == NULL );
    CHECK( pool.m_pBblk == NULL );
    CHECK( lsr_xpool_is_empty( &pool ) == 1 );
    
    ptr = (char *)lsr_xpool_alloc( &pool, 1032 );
    ptr2 = (char *)lsr_xpool_alloc( &pool, 1032+16 );
    lsr_xpool_free( &pool, (void *)ptr );
    lsr_xpool_free( &pool, (void *)ptr2 );
    lsr_xpool_alloc( &pool, 4000 );
    lsr_xpool_alloc( &pool, 4072 );
    ptr3 = (char *)lsr_xpool_alloc( &pool, 1032 );
    lsr_xpool_free( &pool, (void *)ptr3 );
    ptr3 = (char *)lsr_xpool_alloc( &pool, 1032+32 );
    lsr_xpool_free( &pool, (void *)ptr3 );
 
    for( int i = 0; i < 50; i++ )
    {
        ptr = (char *)lsr_xpool_alloc( &pool, 253 );
        memset( ptr, 0, 253 );
        CHECK( memcmp( ptr, cmp, 253 ) == 0 );
    }
    
    ptr = (char *)lsr_xpool_realloc( &pool, ptr, 510 );
    CHECK( memcmp( ptr, cmp, 253 ) == 0 );
    
    ptr = (char *)lsr_xpool_alloc( &pool, 5000 );
    memset( ptr, 0, 5000 );
    for( int i = 0; i < 5000; ++i )
        CHECK( *(ptr + i) == 0 );
    
    ptr = (char *)lsr_xpool_realloc( &pool, ptr, 5500 );
    for( int i = 0; i < 5000; ++i )
        CHECK( *(ptr + i) == 0 );
    
    int *psizes = sizes;;
    for( int i = 0; i < (int)(sizeof(sizes)/sizeof(sizes[0])); i++ )
    {
        ptr = (char *)lsr_xpool_alloc( &pool, *psizes );
        memset( ptr, 0x55, *psizes++ );
        lsr_xpool_free( &pool, (void *)ptr );
    }
    
    ptr = (char *)lsr_xpool_alloc( &pool, 96 - 8 );
    lsr_xpool_free( &pool, (void *)ptr );   /* goes into min 96 bucket */
    ptr2 = (char *)lsr_xpool_alloc( &pool, 96 - 8 - 15  );
    CHECK( ptr == ptr2 );                   /* takes from min 96 bucket */
    lsr_xpool_free( &pool, (void *)ptr2 );  /* and keeps original size */
    ptr = (char *)lsr_xpool_alloc( &pool, 96 - 8 );
    CHECK( ptr == ptr2 );
    lsr_xpool_free( &pool, (void *)ptr );
    
    ptr = (char *)lsr_xpool_alloc( &pool, 1024 );
    for( int i = 0; i < 1024; ++i )
        *(ptr + i) = (i & 0xff);
    lsr_xpool_free( &pool, (void *)ptr );
    ptr2 = (char *)lsr_xpool_alloc( &pool, 1024 );
    CHECK( ptr == ptr2 );
    lsr_xpool_free( &pool, (void *)ptr2 );
    ptr2 = (char *)lsr_xpool_alloc( &pool, 1024 );
    CHECK( ptr == ptr2 );
    lsr_xpool_free( &pool, (void *)ptr2 );
    
    CHECK( lsr_xpool_is_empty( &pool ) == 0 );
    (char *)lsr_xpool_alloc( &pool, 8*1024 );
    (char *)lsr_xpool_alloc( &pool, 64*1024+1 );
    (char *)lsr_xpool_alloc( &pool, 8*1024 );
    lsr_xpool_destroy( &pool );
    CHECK( lsr_xpool_is_empty( &pool ) == 1 );
}

#endif
