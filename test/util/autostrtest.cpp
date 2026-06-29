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

#include <util/autostr.h>
#include "unittest-cpp/UnitTest++.h"

#include <string.h>

TEST(AutoStrSelfReferentialSet)
{
    AutoStr str("prefix-value");

    CHECK(5 == str.setStr(str.c_str() + 7, 5));
    CHECK(0 == strcmp(str.c_str(), "value"));

    str = str;
    CHECK(0 == strcmp(str.c_str(), "value"));
}

#endif
