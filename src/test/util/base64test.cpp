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
 
#include "base64test.h"
#include <util/base64.h>

#include <stdio.h>
#include <string.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

TEST( Base64Test_test)
{
    const char * pEncoded = "QWxhZGRpbjpvcGVuIHNlc2FtZQ==";
    char achDecoded[50];
    const char * pResult = "Aladdin:open sesame";
    CHECK( -1 != Base64::decode( pEncoded, 50, achDecoded ) );
    CHECK( 0 == strcmp( pResult, achDecoded ) );
    
    const char *pDecoded = "Aladdin:open sesame";
    char achEncoded[50];
    unsigned int len = Base64::encode(pDecoded, strlen(pDecoded), achEncoded);
    achEncoded[len] = 0;
    printf("%s\n", achEncoded);
    
    CHECK (len == strlen(pEncoded));
    CHECK (0 == memcmp(achEncoded, pEncoded, len));
    
    
}

#endif
