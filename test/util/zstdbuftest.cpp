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

#ifdef USE_ZSTD

#include <util/zstdbuf.h>
#include <util/vmembuf.h>
#include "unittest-cpp/UnitTest++.h"


TEST(ZstdBufTest_testZstdFile)
{
    VMemBuf zstdFile;
    zstdFile.set("zstdbuftest.zst" , -1);
    ZstdBuf zstdBuf;
    CHECK(0 == zstdBuf.init(ZstdBuf::COMPRESSOR_COMPRESS, 6));

    char achBuf[8192];
    memset(achBuf, 'A', 4096);
    memset(achBuf + 4096, 'b', 4096);

    zstdBuf.setCompressCache(&zstdFile);
    CHECK(0 == zstdBuf.beginStream());
    CHECK(8192 == zstdBuf.write(achBuf, 8192));
    CHECK(0 == zstdBuf.endStream());
    zstdFile.exactSize();
    zstdFile.close();

    zstdBuf.reset();
    zstdFile.deallocate();
    zstdFile.set("zstdbuftest2.zst", -1);

    CHECK(0 == zstdBuf.beginStream());
    for (int i = 1; i < 1024; ++i)
        CHECK(i == zstdBuf.write(achBuf + 4096 - i / 2, i));
    CHECK(0 == zstdBuf.endStream());
    zstdFile.exactSize();
    zstdFile.close();

    zstdBuf.reset();
    zstdFile.deallocate();
    zstdFile.set("zstdbuftest3.zst", -1);

    CHECK(0 == zstdBuf.beginStream());
    int num;
    for (int i = 1; i < 20000; ++i)
    {
        num = rand();
        CHECK(4 == zstdBuf.write((char *)&num, 4));
    }
    CHECK(0 == zstdBuf.endStream());
    zstdFile.exactSize();
    zstdFile.close();
}

#endif
#endif
