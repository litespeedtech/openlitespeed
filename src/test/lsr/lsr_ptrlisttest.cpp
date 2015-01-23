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
 
#include <lsr/lsr_ptrlist.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

#include <string.h>

static int s_mycmp( const void *p1, const void *p2 )
{
    return( strcmp( *(char **)p1, *(char **)p2 ) );
}

static int f_mycmp( const void *p1, const void *p2 )
{
    return( strcmp( (char *)p1, (char *)p2 ) );
}

TEST( lsr_ptrlisttest_test )
{
    int size;
    unsigned int capacity;
    lsr_ptrlist_t * pglist1;
    lsr_ptrlist_t * pglist2;
    lsr_ptrlist_t glist3;
    lsr_ptrlist_iter iter;
    static const char *myptrs[] = { "ptr3", "ptr2", "ptr4", "ptr1" };
    char *ptrs[sizeof(myptrs)/sizeof(myptrs[0])];

    capacity = 10;
    pglist2 = lsr_ptrlist_new( capacity );
    CHECK( pglist2 != NULL );
    CHECK( lsr_ptrlist_capacity( pglist2 ) == capacity );
    capacity *= 3;
    lsr_ptrlist_reserve( pglist2, capacity );
    CHECK( lsr_ptrlist_capacity( pglist2 ) == capacity );
    CHECK( lsr_ptrlist_size( pglist2 ) == 0 );
    lsr_ptrlist_unsafe_push_backn( pglist2, (void **)myptrs, 2 );
    CHECK( lsr_ptrlist_size( pglist2 ) == 2 );
    lsr_ptrlist_unsafe_pop_backn( pglist2, (void **)ptrs, 1 );
    CHECK( lsr_ptrlist_size( pglist2 ) == 1 );
    CHECK( ptrs[0] == myptrs[1] );
    lsr_ptrlist_clear( pglist2 );
    CHECK( lsr_ptrlist_capacity( pglist2 ) == capacity );
    CHECK( lsr_ptrlist_size( pglist2 ) == 0 );
    
    capacity = 15;
    pglist1 = lsr_ptrlist_new( 0 );
    CHECK( pglist1 != NULL );
    CHECK( lsr_ptrlist_capacity( pglist1 ) == 0 );
    lsr_ptrlist_reserve( pglist1, capacity );
    for ( size = 0; size < (int)(sizeof(myptrs)/sizeof(myptrs[0])); ++size )
        lsr_ptrlist_push_back( pglist1, (void *)myptrs[size] );
    CHECK( lsr_ptrlist_capacity( pglist1 ) == capacity );
    CHECK( lsr_ptrlist_size( pglist1 ) == size );
    lsr_ptrlist_copy( &glist3, (const lsr_ptrlist_t *)pglist1 );
    CHECK( lsr_ptrlist_capacity( &glist3 ) == (unsigned)size );
    CHECK( lsr_ptrlist_size( &glist3 ) == size );
    
    --size;
    lsr_ptrlist_pop_front( &glist3, (void **)ptrs, size );
    while ( --size >= 0 )
        CHECK( ptrs[size] == myptrs[size] );
    
    lsr_ptrlist_push_back2( &glist3, (const lsr_ptrlist_t *)pglist1 );
    size = sizeof(myptrs)/sizeof(myptrs[0]) + 1;
    CHECK( lsr_ptrlist_size( &glist3 ) == size );
    
    lsr_ptrlist_push_back( &glist3, (void *)myptrs[0] );
    ++size;
    CHECK( lsr_ptrlist_capacity( &glist3 ) >= (unsigned)size );
    lsr_ptrlist_pop_front( &glist3, (void **)ptrs, 1 );
    --size;
    CHECK( lsr_ptrlist_pop_back( &glist3 ) == (void *)myptrs[0] );
    --size;
    CHECK( lsr_ptrlist_size( &glist3 ) == size );
    CHECK( sizeof(myptrs)/sizeof(myptrs[0]) == size );
    
    lsr_ptrlist_sort( &glist3, s_mycmp );
    iter = (lsr_ptrlist_iter)lsr_ptrlist_bfind(
      &glist3, (const void *)myptrs[0], f_mycmp );
    CHECK( *(const char **)iter == myptrs[0] );
    lsr_ptrlist_erase( &glist3, iter );
    --size;
    CHECK( lsr_ptrlist_size( &glist3 ) == size );
    
    lsr_ptrlist_delete( pglist1 );
    lsr_ptrlist_delete( pglist2 );
    lsr_ptrlist_d( &glist3 );
}

#endif
