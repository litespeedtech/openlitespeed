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

// Minimal entry point for the low-level unit-test binaries (lsr/util/edio/
// thread/socket). Unlike unittest_main.cpp it does not pull in HttpLog or the
// LsiApi module hooks, so these tests link only the libraries they actually
// reference instead of the whole server. Tests that need that infrastructure
// use unittest_main.cpp (the FULL tier in test/CMakeLists.txt) instead.

#include "unittest-cpp/UnitTest++.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

char *argv0 = NULL;

int main(int argc, char *argv[])
{
    argv0 = argv[0];
    umask(022);
    return UnitTest::RunAllTests();
}
