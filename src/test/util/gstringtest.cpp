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
 
#include "gstringtest.h"
#include <util/gstring.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"


TEST( GStringTest_test)
{
    const char * pInit = "initial value";
    int len = strlen( pInit );
    GString init( pInit );
    GString copyconstruct( init );
    GString initn( pInit, 7 );
    CHECK( init.c_str() != NULL );
    CHECK( init.len() == len );
    CHECK( init.compare( pInit ) == 0 );
    CHECK( copyconstruct.len() == len );
    CHECK( copyconstruct.compare( pInit ) == 0 );
    CHECK( initn.len() == 7 );
    CHECK( strncmp( initn, pInit, 7 ) == 0 );
    CHECK( strncmp( initn, pInit, 8 ) != 0 );

    // initialize with NULL, empty String
    GString empty1;
    GString empty2( empty1 );
    GString empty3( (char *)NULL );
    GString empty4( (char *)NULL, 5 );
    GString empty5( "" );
    CHECK( empty1.c_str() == NULL );
    CHECK( empty1.len() == 0 );
    CHECK( empty2.c_str() == NULL );
    CHECK( empty2.len() == 0 );
    CHECK( empty3.c_str() == NULL );
    CHECK( empty3.len() == 0 );
    CHECK( empty4.c_str() == NULL );
    CHECK( empty4.len() == 0 );
    CHECK( empty5.c_str() != NULL );
    CHECK( empty5.len() == 0 );

    //test assignment
    GString s1;
    s1.assign( pInit, len );
    CHECK( s1.len() == len );
    CHECK( s1.compare( pInit ) == 0 );

    s1.assign( NULL, -1 );
    CHECK( s1.len() == 0 );
    CHECK( s1.c_str() == NULL );

    s1 = pInit;
    CHECK( s1.len() == len );
    CHECK( s1.compare( pInit ) == 0 );

    s1 = NULL;
    CHECK( s1.len() == 0 );
    CHECK( s1.c_str() == NULL );

    s1 = init;
    CHECK( s1.len() == len );
    CHECK( s1.compare( pInit ) == 0 );
    CHECK( s1.c_str() != init.c_str() );

    //assign to itself
    const char * pOldBuf = s1.c_str();
    s1.assign( pOldBuf, s1.len() );
    CHECK( s1.c_str() == pOldBuf );
    s1 = s1;
    CHECK( s1.c_str() == pOldBuf );
    s1 = pOldBuf;
    CHECK( s1.c_str() == pOldBuf );
    int oldLen = s1.len();
    s1.assign( pOldBuf, oldLen / 2 );
    CHECK( s1.c_str() == pOldBuf );
    CHECK( s1.len() == oldLen / 2 );
    CHECK( (int)strlen( s1.c_str() ) == oldLen / 2 );
    
    //test append
    GString s2;
    s2.append( NULL, 100 );
    CHECK( s2.len() == 0 );
    CHECK( s2.c_str() == NULL );

    s2.append( pInit, len );
    CHECK( s2.len() == len );
    CHECK( s2.c_str() != NULL );

    pOldBuf = s2.c_str();
    s2.append( NULL, -134904 );
    CHECK( s2.len() == len );
    CHECK( s2.c_str() == pOldBuf );

    s2.append( "", 0 );
    CHECK( s2.len() == len );
    CHECK( s2.c_str() == pOldBuf );

    s2.append( s2.c_str(), s2.len() );
    CHECK( s2.len() == len * 2 );
    CHECK( strncmp( s2.c_str(), pInit, len ) == 0 );
    CHECK( strncmp( s2.c_str() + len, pInit, len ) == 0 );

    pOldBuf = s2.c_str();
    s2 = "";
    CHECK( s2.c_str() == pOldBuf );
    CHECK( s2.len() == 0 );
    s2 += pInit;
    CHECK( s2.c_str() == pOldBuf );
    CHECK( s2.len() == len );
    s2 += s2;
    CHECK( s2.len() == len * 2 );
    CHECK( strncmp( s2.c_str(), pInit, len ) == 0 );
    CHECK( strncmp( s2.c_str() + len, pInit, len ) == 0 );
    

}

#endif
