/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2026  LiteSpeed Technologies, Inc.                 *
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

#include <h2/unpackedheaders.h>
#include "unittest-cpp/UnitTest++.h"

#include <string.h>


static lsxpack_err_code processHeader(UpkdHdrBuilder &builder,
                                      const char *name, unsigned name_len,
                                      const char *val, unsigned val_len)
{
    lsxpack_header *hdr = builder.prepareDecode(NULL,
                                                name_len + val_len + 4);
    CHECK(hdr != NULL);
    if (hdr == NULL)
        return LSXPACK_ERR_NOMEM;

    char *buf = hdr->buf;
    unsigned off = hdr->name_offset;
    memcpy(buf + off, name, name_len);
    memcpy(buf + off + name_len, ": ", 2);
    memcpy(buf + off + name_len + 2, val, val_len);
    memcpy(buf + off + name_len + 2 + val_len, "\r\n", 2);

    lsxpack_header_set_offset2(hdr, buf, off, name_len,
                               off + name_len + 2, val_len);
    return builder.process(hdr);
}


static void addRequiredPseudoHeaders(UpkdHdrBuilder &builder)
{
    CHECK(LSXPACK_OK == processHeader(builder, ":method", 7, "GET", 3));
    CHECK(LSXPACK_OK == processHeader(builder, ":scheme", 7, "https", 5));
    CHECK(LSXPACK_OK == processHeader(builder, ":authority", 10,
                                      "example.com", 11));
    CHECK(LSXPACK_OK == processHeader(builder, ":path", 5, "/", 1));
}


static lsxpack_err_code processRealHeader(const char *name, unsigned name_len,
                                          const char *val, unsigned val_len)
{
    UnpackedHeaders *headers = new UnpackedHeaders();
    UpkdHdrBuilder builder(headers, false);

    addRequiredPseudoHeaders(builder);
    return processHeader(builder, name, name_len, val, val_len);
}


TEST(UnpackedHeadersTest_validFieldNameAndValue)
{
    static const char value[] = { 'a', 'l', 'p', 'h', 'a', '\t',
                                 (char)0x80, };

    CHECK(LSXPACK_OK == processRealHeader("x-test", 6, value,
                                          sizeof(value)));
}


TEST(UnpackedHeadersTest_invalidFieldValues)
{
    static const char method_cr[] = { 'G', 'E', '\r', 'T', };
    static const char path_lf[] = { '/', '\n', 'x', };
    static const char authority_nul[] = { 'e', 'x', '\0', 'a', };
    static const char value_cr[] = { 'a', '\r', 'b', };
    static const char value_lf[] = { 'a', '\n', 'b', };
    static const char value_nul[] = { 'a', '\0', 'b', };
    static const char cookie_lf[] = { 'a', '=', '\n', };

    {
        UnpackedHeaders *headers = new UnpackedHeaders();
        UpkdHdrBuilder builder(headers, false);
        CHECK(LSXPACK_ERR_BAD_REQ_HEADER ==
              processHeader(builder, ":method", 7, method_cr,
                            sizeof(method_cr)));
    }

    {
        UnpackedHeaders *headers = new UnpackedHeaders();
        UpkdHdrBuilder builder(headers, false);
        CHECK(LSXPACK_ERR_BAD_REQ_HEADER ==
              processHeader(builder, ":path", 5, path_lf, sizeof(path_lf)));
    }

    {
        UnpackedHeaders *headers = new UnpackedHeaders();
        UpkdHdrBuilder builder(headers, false);
        CHECK(LSXPACK_ERR_BAD_REQ_HEADER ==
              processHeader(builder, ":authority", 10, authority_nul,
                            sizeof(authority_nul)));
    }

    CHECK(LSXPACK_ERR_BAD_REQ_HEADER ==
          processRealHeader("x-test", 6, value_cr, sizeof(value_cr)));
    CHECK(LSXPACK_ERR_BAD_REQ_HEADER ==
          processRealHeader("x-test", 6, value_lf, sizeof(value_lf)));
    CHECK(LSXPACK_ERR_BAD_REQ_HEADER ==
          processRealHeader("x-test", 6, value_nul, sizeof(value_nul)));
    CHECK(LSXPACK_ERR_BAD_REQ_HEADER ==
          processRealHeader("cookie", 6, cookie_lf, sizeof(cookie_lf)));
}


TEST(UnpackedHeadersTest_invalidFieldNames)
{
    CHECK(LSXPACK_ERR_BAD_REQ_HEADER == processRealHeader("", 0, "v", 1));
    CHECK(LSXPACK_ERR_UPPERCASE_HEADER ==
          processRealHeader("X-Test", 6, "v", 1));
    CHECK(LSXPACK_ERR_BAD_REQ_HEADER ==
          processRealHeader("bad:name", 8, "v", 1));
    CHECK(LSXPACK_ERR_BAD_REQ_HEADER ==
          processRealHeader("bad name", 8, "v", 1));
    CHECK(LSXPACK_ERR_BAD_REQ_HEADER ==
          processRealHeader("bad@", 4, "v", 1));
}

#endif
