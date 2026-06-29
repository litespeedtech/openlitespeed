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


#include "statusurlmaptest.h"
#include <http/statusurlmap.h>
#include <util/autostr.h>
#include "unittest-cpp/UnitTest++.h"

static const AutoStr2 *getMappedUrl(const StatusUrlMap &map, int code)
{
    int index = HttpStatusCode::getInstance().codeToIndex(code);
    if (index <= 0)
        return NULL;
    return map.getUrl(index);
}

TEST(StatusUrlMapTest_test)
{
    StatusUrlMap map1;
    int i;
    for (i = SC_300; i < SC_END; ++i)
        CHECK(map1.getUrl(i) == NULL);
    for (i = 300; i <= 308; ++i)
        CHECK(map1.setStatusUrlMap(i, "/url3xx") == 0);
    for (i = 400; i <= 451; ++i)
        CHECK(map1.setStatusUrlMap(i, "/url4xx") == 0);
    for (i = 500; i <= 510; ++i)
        CHECK(map1.setStatusUrlMap(i, "/url5xx") == 0);

    for (i = 300; i <= 308; ++i)
        CHECK(strcmp(getMappedUrl(map1, i)->c_str(), "/url3xx") == 0);
    for (i = 400; i <= 451; ++i)
        CHECK(strcmp(getMappedUrl(map1, i)->c_str(), "/url4xx") == 0);
    for (i = 500; i <= 510; ++i)
        CHECK(strcmp(getMappedUrl(map1, i)->c_str(), "/url5xx") == 0);
    CHECK(map1.setStatusUrlMap(100, "/url100") == 0);
    CHECK(map1.setStatusUrlMap(299, "/url299") == -1);
    CHECK(map1.setStatusUrlMap(309, "/url309") == -1);
    CHECK(map1.setStatusUrlMap(399, "/url308") == -1);
    CHECK(map1.setStatusUrlMap(425, "/url425") == 0);
    CHECK(map1.setStatusUrlMap(499, "/url499") == -1);
    CHECK(map1.setStatusUrlMap(511, "/url506") == -1);

}

#endif
