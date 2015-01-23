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

#include <lsr/lsr_dlinkq.h>
#include <lsr/lsr_link.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"


SUITE( lsr_dlinkqtest )
{
TEST( lsr_testlinkq )
{
    lsr_linkq_t * pqobj;
    lsr_link_t obj1, obj2, obj3;
    pqobj = lsr_linkq_new();
    CHECK( pqobj != NULL );
    lsr_link( &obj1, NULL );
    lsr_link( &obj2, NULL );
    lsr_link( &obj3, NULL );

    CHECK( lsr_linkq_size( pqobj ) == 0 );
    CHECK( lsr_linkq_begin( pqobj ) == NULL );
    CHECK( lsr_linkq_pop( pqobj ) == NULL );

    lsr_linkq_push( pqobj, &obj1 );
    CHECK( lsr_linkq_size( pqobj ) == 1 );
    CHECK( lsr_linkq_begin( pqobj ) == &obj1 );

    lsr_linkq_addnext( pqobj, lsr_linkq_head( pqobj ), &obj2 );
    CHECK( lsr_linkq_size( pqobj ) == 2 );
    CHECK( lsr_linkq_begin( pqobj ) == &obj2 );

    CHECK( lsr_linkq_pop( pqobj ) == &obj2 );
    CHECK( lsr_linkq_size( pqobj ) == 1 );
    CHECK( lsr_linkq_begin( pqobj ) == &obj1 );

    lsr_linkq_push( pqobj, &obj3 );
    lsr_linkq_push( pqobj, &obj2 );
    CHECK( lsr_linkq_size( pqobj ) == 3 );
    CHECK( lsr_linkq_begin( pqobj ) == &obj2 );

    CHECK( lsr_linkq_removenext( pqobj, lsr_linkq_head( pqobj ) )
      == &obj2 );
    CHECK( lsr_linkq_pop( pqobj ) == &obj3 );
    CHECK( lsr_linkq_pop( pqobj ) == &obj1 );
    CHECK( lsr_linkq_size( pqobj ) == 0 );
    CHECK( lsr_linkq_begin( pqobj ) == NULL );
    CHECK( lsr_linkq_pop( pqobj ) == NULL );

    lsr_linkq_delete( pqobj );
}

TEST( lsr_testdlinkq )
{
    lsr_dlinkq_t * pqobj;
    lsr_dlink_t obj1, obj2, obj3;
    pqobj = lsr_dlinkq_new();
    CHECK( pqobj != NULL );
    lsr_dlink( &obj1, NULL, NULL );
    lsr_dlink( &obj2, NULL, NULL );
    lsr_dlink( &obj3, NULL, NULL );

    CHECK( lsr_dlinkq_size( pqobj ) == 0 );
    CHECK( lsr_dlinkq_empty( pqobj ) == true );
    CHECK( lsr_dlinkq_begin( pqobj ) == lsr_dlinkq_end( pqobj ) );
    CHECK( lsr_dlinkq_begin( pqobj ) != NULL );
    CHECK( lsr_dlinkq_pop_front( pqobj ) == NULL );

    lsr_dlinkq_append( pqobj, &obj1 );
    CHECK( lsr_dlinkq_size( pqobj ) == 1 );
    CHECK( lsr_dlinkq_empty( pqobj ) == false );
    CHECK( lsr_dlinkq_begin( pqobj ) != lsr_dlinkq_end( pqobj ) );
    CHECK( lsr_dlinkq_begin( pqobj ) == &obj1 );

    lsr_dlinkq_append( pqobj, &obj2 );
    CHECK( lsr_dlinkq_size( pqobj ) == 2 );
    CHECK( lsr_dlinkq_begin( pqobj ) == &obj1 );

    lsr_dlinkq_push_front( pqobj, &obj3 );
    CHECK( lsr_dlinkq_size( pqobj ) == 3 );
    CHECK( lsr_dlinkq_begin( pqobj ) == &obj3 );

    lsr_dlinkq_remove( pqobj, &obj1 );
    CHECK( lsr_dlinkq_size( pqobj ) == 2 );
    CHECK( lsr_dlinkq_begin( pqobj ) == &obj3 );

    lsr_dlinkq_append( pqobj, &obj1 );
    CHECK( lsr_dlinkq_size( pqobj ) == 3 );
    CHECK( lsr_dlinkq_begin( pqobj ) == &obj3 );

    CHECK( lsr_dlinkq_pop_front( pqobj ) == &obj3 );
    CHECK( lsr_dlinkq_size( pqobj ) == 2 );
    CHECK( lsr_dlinkq_begin( pqobj ) == &obj2 );

    CHECK( lsr_dlinkq_pop_front( pqobj ) == &obj2 );
    CHECK( lsr_dlinkq_size( pqobj ) == 1 );
    CHECK( lsr_dlinkq_begin( pqobj ) == &obj1 );

    CHECK( lsr_dlinkq_pop_front( pqobj ) == &obj1 );
    CHECK( lsr_dlinkq_size( pqobj ) == 0 );
    CHECK( lsr_dlinkq_empty( pqobj ) == true );
    CHECK( lsr_dlinkq_begin( pqobj ) == lsr_dlinkq_end( pqobj ) );
    CHECK( lsr_dlinkq_begin( pqobj ) != NULL );
    CHECK( lsr_dlinkq_pop_front( pqobj ) == NULL );

    lsr_dlinkq_delete( pqobj );
}
}

#endif
