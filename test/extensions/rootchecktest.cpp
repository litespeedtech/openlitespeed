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

#include <extensions/cgi/rootcheck.h>

#include "unittest-cpp/UnitTest++.h"


TEST(RootCheckTest_stripRuntimeStartupControls)
{
    const char *dangerous[] =
    {
        "LD_PRELOAD=/tmp/x.so", "LD_AUDIT=/tmp/x.so", "BASH_ENV=/tmp/x",
        "GLIBC_TUNABLES=x", "PERL5OPT=-Mx", "PERL5DB=x", "PYTHONPATH=/tmp",
        "PYTHONUSERBASE=/tmp", "PYTHONWARNINGS=ignore::x.Y",
        "NODE_OPTIONS=--require=/tmp/x", "RUBYOPT=-rx",
        "PHPRC=/tmp", "PHP_INI_SCAN_DIR=/tmp",
        "JAVA_TOOL_OPTIONS=-javaagent:/tmp/x.jar",
        "JDK_JAVA_OPTIONS=-javaagent:/tmp/x.jar",
        "_JAVA_OPTIONS=-javaagent:/tmp/x.jar",
    };
    for (unsigned i = 0; i < sizeof(dangerous) / sizeof(dangerous[0]); ++i)
        CHECK_EQUAL(1, rootcheck_env_is_dangerous(dangerous[i]));
}


TEST(RootCheckTest_keepOrdinaryEnvironment)
{
    const char *safe[] =
    {
        "PATH=/usr/bin:/bin", "HTTP_HOST=example.com", "SCRIPT_FILENAME=/x",
        "PHPRCX=/tmp", "HTTP_PHPRC=/tmp", "PHP_SELF=/index.php",
        "JAVA_HOME=/usr/lib/jvm", "PYTHONUNBUFFERED=1", "ENVIRONMENT=prod",
    };
    for (unsigned i = 0; i < sizeof(safe) / sizeof(safe[0]); ++i)
        CHECK_EQUAL(0, rootcheck_env_is_dangerous(safe[i]));
}

#endif
