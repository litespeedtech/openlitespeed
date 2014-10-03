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
 
#include <lsr/lsr_str.h>
#include <lsr/lsr_strlist.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

#include <stdio.h>
#include <string.h>

/*
 * note the difference between
 * lsr_strlist_{push,pop,insert,erase} and lsr_strlist_{add,remove,clear,_d}.
 * {push,pop,...} expect user allocated (constructed) lsr_str_t objects.
 * {add,remove,...} allocate lsr_str_t objects given char *s,
 * and deallocate the objects when called to. do *NOT* mix these mechanisms.
 *
 */
TEST( lsr_strlisttest_test )
{
    int size;
    unsigned int capacity;
    lsr_strlist_t * pglist1;
    lsr_strlist_t glist3;
    lsr_str_t * pautostr1;
    lsr_str_t autostr3;
    lsr_str_t * pstr;
    static const char *myptrs[] = { "ptr333", "ptr22", "ptr4444", "ptr1" };
    char mybuf[256];
    char *p;
    int ret;
    int left;

    pautostr1 = lsr_str_new( "mystr1", 6 );
    CHECK( pautostr1 != NULL );
    lsr_str( &autostr3, "mystr2", 6 );

    capacity = 15;
    pglist1 = lsr_strlist_new( 0 );
    CHECK( pglist1 != NULL );
    CHECK( lsr_strlist_capacity( pglist1 ) == 0 );
    lsr_strlist_reserve( pglist1, capacity );
    lsr_strlist_push_back( pglist1, pautostr1 );
    lsr_strlist_push_back( pglist1, &autostr3 );
    CHECK( lsr_strlist_capacity( pglist1 ) == capacity );
    CHECK( lsr_strlist_size( pglist1 ) == 2 );
    lsr_strlist_pop_front( pglist1, &pstr, 1 );
    CHECK( pstr == pautostr1 );
    lsr_strlist_unsafe_pop_backn( pglist1, &pstr, 1 );
    CHECK( pstr == &autostr3 );
    CHECK( lsr_strlist_size( pglist1 ) == 0 );

    for ( size = 0; size < (int)(sizeof(myptrs)/sizeof(myptrs[0])); ++size )
        lsr_strlist_add( pglist1, myptrs[size], strlen( myptrs[size] ) );
    CHECK( lsr_strlist_capacity( pglist1 ) == capacity );
    CHECK( lsr_strlist_size( pglist1 ) == size );
    lsr_strlist_copy( &glist3, (const lsr_strlist_t *)pglist1 );
    CHECK( lsr_strlist_size( &glist3 ) == size );
    CHECK( lsr_strlist_capacity( &glist3 ) >= (unsigned)size );
    lsr_strlist_clear( pglist1 );
    CHECK( lsr_strlist_capacity( pglist1 ) == capacity );
    CHECK( lsr_strlist_size( pglist1 ) == 0 );
 
    lsr_strlist_push_back( &glist3, pautostr1 );
    ++size;
    CHECK( lsr_strlist_capacity( &glist3 ) >= (unsigned)size );
    CHECK( lsr_strlist_pop_back( &glist3 ) == pautostr1 );
    --size;
    CHECK( lsr_strlist_size( &glist3 ) == size );
    CHECK( sizeof(myptrs)/sizeof(myptrs[0]) == size );
 
    lsr_strlist_sort( &glist3 );
    pstr = lsr_strlist_bfind( &glist3, myptrs[0] );
    CHECK( pstr != NULL );
    CHECK( pstr == lsr_strlist_find( &glist3, myptrs[0] ) );
    if ( pstr )
    {
        CHECK( strncmp( lsr_str_c_str( (const lsr_str_t *)pstr ), myptrs[0],
          lsr_str_len( (const lsr_str_t *)pstr ) ) == 0 );
    }
    lsr_strlist_remove( &glist3, myptrs[0] );
    --size;
    CHECK( lsr_strlist_size( &glist3 ) == size );
    CHECK( lsr_strlist_find( &glist3, myptrs[0] ) == NULL );
 
    p = mybuf;
    left = sizeof(mybuf);
    for ( size = 0; size < (int)(sizeof(myptrs)/sizeof(myptrs[0])); ++size )
    {
        ret = snprintf( p, left, "%s, ", myptrs[size] );
        p += ret;
        left -= ret;
    }
    CHECK( lsr_strlist_split( pglist1, (const char *)mybuf, (const char *)p, "," )
      == sizeof(myptrs)/sizeof(myptrs[0]) );
    while ( size > 0 )
    {
        CHECK( lsr_strlist_size( pglist1 ) == size );
        --size;
        CHECK( lsr_strlist_find( pglist1, myptrs[size] ) != NULL );
        lsr_strlist_remove( pglist1, myptrs[size] );
        CHECK( lsr_strlist_find( pglist1, myptrs[size] ) == NULL );
    }
 
    lsr_str_d( &autostr3 );
    lsr_str_delete( pautostr1 );
    lsr_strlist_d( &glist3 );
    lsr_strlist_delete( pglist1 );
}

#endif
