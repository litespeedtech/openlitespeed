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

#include <lsr/lsr_pcreg.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"



TEST( lsr_pcregextest_test )
{
    lsr_pcre_t *reg = lsr_pcre_new();
    char achSub1[] = "/some/dir/abc.gif";
    char achSub2[] = "/some/dir/abc.jpg";
    int vector[30];
    int find;
    CHECK( lsr_pcre_compile( reg, "/(.*)\\.gif", 0, 0, 0 ) == 0 );
    find = lsr_pcre_exec( reg, achSub1 , sizeof( achSub1 ) - 1 , 0, 0, vector, 30 );
    CHECK( find == 2 );
    CHECK( vector[0] == 0 );
    CHECK( vector[1] == sizeof( achSub1 ) - 1 );
    CHECK( vector[2] == 1 );
    CHECK( vector[3] == sizeof( achSub1 ) - 5 );
    find = lsr_pcre_exec( reg, achSub2, sizeof( achSub2 ) - 1, 0, 0, vector, 30 );
    CHECK( find == PCRE_ERROR_NOMATCH );
    lsr_pcre_delete( reg );
}

TEST( lsr_pcregex_regsub_test )
{
    lsr_pcre_t *reg = lsr_pcre_new();
    int vector[30];
    int find;
    char achSub1[] = "/abcdefghijklmnopqrst";
    CHECK( lsr_pcre_compile( reg, "/(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)(k).*", 0, 0, 0 ) == 0 );
    find = lsr_pcre_exec( reg, achSub1, sizeof( achSub1 ) - 1, 0, 0, vector, 30 );
    CHECK( find == 0 );
    find = 10;

    
    lsr_pcre_sub_t regsub1, 
                       *regsub2 = lsr_pcre_sub_new(), 
                       *regsub3 = lsr_pcre_sub_new();
    lsr_pcre_sub( &regsub1 );
    CHECK( lsr_pcre_sub_compile( &regsub1, "/$9_$8_$7_$6_$5$4$3$2$1$0" ) == 0 );
    CHECK( lsr_pcre_sub_compile( regsub2, "/\\$$9\\&$8_$7_$11_&" ) == 0 );

    char expect1[] = "/i_h_g_f_edcba/abcdefghijklmnopqrst";
    char expect2[] = "/$i&h_g_a1_/abcdefghijklmnopqrst";
    char result[200];
    int len = sizeof( expect1 );
    int ret = lsr_pcre_sub_exec( &regsub1, achSub1, vector, find, result, &len );
    CHECK( ret == 0 );
    CHECK( strcmp( result, expect1 ) == 0 );
    CHECK( len == sizeof( expect1 ) - 1 );

    len = sizeof( expect1 ) - 1;
    ret = lsr_pcre_sub_exec( &regsub1, achSub1, vector, find, result, &len );
    CHECK( ret == -1 );
    CHECK( len == sizeof( expect1 ) - 1 );
    
    
    len = 200;
    ret = lsr_pcre_sub_exec( regsub2, achSub1, vector, find, result, &len );
    CHECK( ret == 0 );
    CHECK( strcmp( result, expect2 ) == 0 );
    CHECK( len == sizeof( expect2 ) - 1 );
    
    lsr_pcre_sub_d( &regsub1 );
    lsr_pcre_sub_delete( regsub2 );
    lsr_pcre_sub_delete( regsub3 );
    lsr_pcre_delete( reg );    
}


#endif
