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

#include <extensions/extworkerconfig.h>
#include <socket/gsockaddr.h>
#include "unittest-cpp/UnitTest++.h"

#include <string.h>
#include <sys/un.h>


TEST(ExtWorkerConfigTest_altServerAddrReplacesUnixSuffix)
{
    const char *url = "uds:///tmp/lshttpd/example.sock";
    ExtWorkerConfig config("alt-uds-test");
    char basePath[sizeof(((sockaddr_un *)0)->sun_path)];
    char firstPath[sizeof(basePath)];
    char firstUrl[sizeof(basePath) + 8];
    size_t basePathLen;
    size_t baseUrlLen = strlen(url);

    CHECK(config.setURL(url) == 0);
    CHECK(config.updateServerAddr(url) == 0);
    strcpy(basePath, config.getServerAddrUnixSock());
    basePathLen = strlen(basePath);

    config.altServerAddr();
    strcpy(firstPath, config.getServerAddrUnixSock());
    strcpy(firstUrl, config.getURL());
    CHECK(strlen(firstPath) == basePathLen + 4);
    CHECK(strlen(firstUrl) == baseUrlLen + 4);
    CHECK(strncmp(firstPath, basePath, basePathLen) == 0);
    CHECK(strncmp(firstUrl, url, baseUrlLen) == 0);

    config.altServerAddr();
    CHECK(strlen(config.getServerAddrUnixSock()) == basePathLen + 4);
    CHECK(strlen(config.getURL()) == baseUrlLen + 4);
    CHECK(strncmp(config.getServerAddrUnixSock(), basePath, basePathLen) == 0);
    CHECK(strncmp(config.getURL(), url, baseUrlLen) == 0);

    CHECK(config.setURL(url) == 0);
    config.altServerAddr();
    CHECK(strlen(config.getServerAddrUnixSock()) == basePathLen + 4);
    CHECK(strlen(config.getURL()) == baseUrlLen + 4);
    CHECK(strncmp(config.getServerAddrUnixSock(), basePath, basePathLen) == 0);
    CHECK(strncmp(config.getURL(), url, baseUrlLen) == 0);
}

#endif
