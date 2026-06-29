/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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

#include <lsr/ls_base64.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "unittest-cpp/UnitTest++.h"

TEST(ls_Base64Test_test)
{
#ifdef LSR_BASE64_DEBUG
    printf("Start LSR Base64 Test\n");
#endif
    const char *pEncoded = "QWxhZGRpbjpvcGVuIHNlc2FtZQ==";
    char achDecoded[50];
    const char *pResult = "Aladdin:open sesame";
    CHECK(-1 != ls_base64_decode(pEncoded, strlen(pEncoded), achDecoded,
                                  sizeof(achDecoded)));
    CHECK(0 == strcmp(pResult, achDecoded));

    const char *pDecoded = "Aladdin:open sesame";
    char achEncoded[50];
    unsigned int len = ls_base64_encode(pDecoded, strlen(pDecoded),
                                        achEncoded, sizeof(achEncoded));
    achEncoded[len] = 0;

    CHECK(len == strlen(pEncoded));
    CHECK(0 == memcmp(achEncoded, pEncoded, len));


}


TEST(ls_Base64Test_checksOutputSize)
{
    char achDecoded[4];
    char achSmall[2];
    char achEncoded[8];
    char achSmallEncoded[7];
    int len;

    errno = 0;
    CHECK(ls_base64_decode("QWxh", 4, achSmall, sizeof(achSmall)) == -1);
    CHECK(errno == ENOSPC);

    len = ls_base64_decode("QWxh", 4, achDecoded, sizeof(achDecoded));
    CHECK(len == 3);
    CHECK(memcmp(achDecoded, "Ala", len) == 0);
    CHECK(achDecoded[len] == 0);

    errno = 0;
    CHECK(ls_base64_encode("test", 4, achSmallEncoded,
                           sizeof(achSmallEncoded)) == -1);
    CHECK(errno == ENOSPC);

    len = ls_base64_encode("test", 4, achEncoded, sizeof(achEncoded));
    CHECK(len == 8);
}


TEST(ls_Base64Test_ignoresHighBitBytes)
{
    char achEncoded[] = { 'Q', 'W', (char)0x80, 'x', 'h', 0 };
    char achDecoded[8];

    CHECK(ls_base64_decode(achEncoded, 5, achDecoded,
                           sizeof(achDecoded)) == 3);
    CHECK(strcmp(achDecoded, "Ala") == 0);
}


TEST(ls_Base64Test_rejectsInvalidInputs)
{
    char achBuf[8];

    errno = 0;
    CHECK(ls_base64_decode(NULL, 1, achBuf, sizeof(achBuf)) == -1);
    CHECK(errno == EINVAL);

    errno = 0;
    CHECK(ls_base64_decode("QQ==", -1, achBuf, sizeof(achBuf)) == -1);
    CHECK(errno == EINVAL);

    errno = 0;
    CHECK(ls_base64_decode("QQ==", 4, NULL, 1) == -1);
    CHECK(errno == EINVAL);

    errno = 0;
    CHECK(ls_base64_encode(NULL, 1, achBuf, sizeof(achBuf)) == -1);
    CHECK(errno == EINVAL);

    errno = 0;
    CHECK(ls_base64_encode("A", -1, achBuf, sizeof(achBuf)) == -1);
    CHECK(errno == EINVAL);

    errno = 0;
    CHECK(ls_base64_encode("A", 1, NULL, 1) == -1);
    CHECK(errno == EINVAL);
}

#endif
