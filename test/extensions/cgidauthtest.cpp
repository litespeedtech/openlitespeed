/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2026  LiteSpeed Technologies, Inc.                       *
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
*    along with this program. If not, see <https://www.gnu.org/licenses/>.   *
*****************************************************************************/
#ifdef RUN_TEST

#include <extensions/cgi/blake2b.h>
#include <extensions/cgi/cgidreq.h>

#include "unittest-cpp/UnitTest++.h"

#include <string.h>


TEST(CgidAuthTest_keyedBlake2bVector)
{
    const unsigned char expected[LSCGID_MAC_LEN] =
    {
        0xcc, 0x61, 0x1a, 0x26, 0x73, 0xca, 0x66, 0xe9,
        0xf7, 0xfc, 0x64, 0xb9, 0x95, 0xc5, 0x46, 0x7c
    };
    unsigned char key[LSCGID_SECRET_LEN];
    unsigned char output[LSCGID_MAC_LEN];
    unsigned int i;

    for (i = 0; i < sizeof(key); ++i)
        key[i] = i;
    CHECK_EQUAL(0, blake2b(output, sizeof(output), key, sizeof(key),
                          "abc", 3));
    CHECK(memcmp(output, expected, sizeof(expected)) == 0);
}


TEST(CgidAuthTest_senderMatchesStreamingReceiver)
{
    CgidReq req;
    blake2b_ctx ctx;
    char secret[LSCGID_SECRET_LEN];
    unsigned char supplied[LSCGID_MAC_LEN];
    unsigned char expected[LSCGID_MAC_LEN];
    unsigned int i;
    lscgid_req *header;

    for (i = 0; i < sizeof(secret); ++i)
        secret[i] = (char)(0xa0 + i);
    CHECK_EQUAL(12, req.add("request-body", 12));
    CHECK_EQUAL(0, req.finalize(42, secret, LSCGID_TYPE_CGI));
    header = req.getCgidReq();
    memcpy(supplied, header->m_md5, sizeof(supplied));
    memset(header->m_md5, 0, sizeof(header->m_md5));

    CHECK_EQUAL(0, blake2b_init_key(&ctx, sizeof(expected), secret,
                                   sizeof(secret)));
    CHECK_EQUAL(0, blake2b_update(&ctx, header, sizeof(*header)));
    CHECK_EQUAL(0, blake2b_update(&ctx, req.get() + sizeof(*header),
                                 header->m_szData));
    CHECK_EQUAL(0, blake2b_final(&ctx, expected, sizeof(expected)));
    CHECK(memcmp(supplied, expected, sizeof(expected)) == 0);

    const_cast<char *>(req.get())[req.size() - 1] ^= 1;
    CHECK_EQUAL(0, blake2b(expected, sizeof(expected), secret, sizeof(secret),
                          req.get(), req.size()));
    CHECK(memcmp(supplied, expected, sizeof(expected)) != 0);
}

#endif
