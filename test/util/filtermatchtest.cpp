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

#include "filtermatchtest.h"

#include <util/filtermatch.h>
#include "unittest-cpp/UnitTest++.h"

#include <string.h>
#include <iostream>
#include <fstream>
#include <iomanip>

#define QUOTE(TXT) #TXT
#define EQ(TXT) QUOTE(TXT)

SUITE(FilterMatchTest)
{
#undef _UT_TNAME_
#define _UT_TNAME_ filtermatch_strcmp
    TEST(_UT_TNAME_)
    {
        std::cerr << std::endl << "START " << EQ(_UT_TNAME_) << std::endl;
        FilterMatch fm("sexactMatch");
        CHECK(false == fm.match("notreally"));
        CHECK(false == fm.match("exactMatchn"));
        CHECK(false == fm.match("exactMatc"));
        CHECK(false == fm.match("exactMatcH"));
        CHECK(true == fm.match("exactMatch"));
        std::cerr << std::endl << "END " << EQ(_UT_TNAME_) << std::endl;
    }

#undef _UT_TNAME_
#define _UT_TNAME_ filtermatch_negstrcmp
    TEST(_UT_TNAME_)
    {
        std::cerr << std::endl << "START " << EQ(_UT_TNAME_) << std::endl;
        FilterMatch fm("!sexactMatch");
        CHECK(true == fm.match("notreally"));
        CHECK(true == fm.match("exactMatchn"));
        CHECK(true == fm.match("exactMatc"));
        CHECK(true == fm.match("exactMatcH"));
        CHECK(false == fm.match("exactMatch"));
        std::cerr << std::endl << "END " << EQ(_UT_TNAME_) << std::endl;
    }

#undef _UT_TNAME_
#define _UT_TNAME_ filtermatch_strcasecmp
    TEST(_UT_TNAME_)
    {
        std::cerr << std::endl << "START " << EQ(_UT_TNAME_) << std::endl;
        FilterMatch fm("iexactMatch");
        CHECK(false == fm.match("notreally"));
        CHECK(false == fm.match("exactMatchn"));
        CHECK(false == fm.match("exactMatc"));
        CHECK(true == fm.match("exactMatcH"));
        CHECK(true == fm.match("exactMatch"));
        std::cerr << std::endl << "END " << EQ(_UT_TNAME_) << std::endl;
    }

#undef _UT_TNAME_
#define _UT_TNAME_ filtermatch_negstrcasecmp
    TEST(_UT_TNAME_)
    {
        std::cerr << std::endl << "START " << EQ(_UT_TNAME_) << std::endl;
        FilterMatch fm("!iexactMatch");
        CHECK(true == fm.match("notreally"));
        CHECK(true == fm.match("exactMatchn"));
        CHECK(true == fm.match("exactMatc"));
        CHECK(false == fm.match("exactMatcH"));
        CHECK(false == fm.match("exactMatch"));
        std::cerr << std::endl << "END " << EQ(_UT_TNAME_) << std::endl;
    }

#undef _UT_TNAME_
#define _UT_TNAME_ filtermatch_fnmatch
    TEST(_UT_TNAME_)
    {
        std::cerr << std::endl << "START " << EQ(_UT_TNAME_) << std::endl;
        FilterMatch *fm = new FilterMatch("f0exactMatch");
        CHECK(false == fm->match("notreally"));
        CHECK(false == fm->match("exactMatchn"));
        CHECK(false == fm->match("exactMatc"));
        CHECK(false == fm->match("exactMatcH"));
        CHECK(true == fm->match("exactMatch"));
        delete fm;
        fm = new FilterMatch("f0exa*tch");
        CHECK(false == fm->match("notreally"));
        CHECK(false == fm->match("exactMatchn"));
        CHECK(false == fm->match("exactMatc"));
        CHECK(false == fm->match("exactMatcH"));
        CHECK(true == fm->match("exactMatch"));
        delete fm;
        fm = new FilterMatch("f0exact?atch");
        CHECK(false == fm->match("exactMatcH"));
        CHECK(true == fm->match("exactMatch"));
        std::cerr << std::endl << "END " << EQ(_UT_TNAME_) << std::endl;
        delete fm;
    }

#undef _UT_TNAME_
#define _UT_TNAME_ filtermatch_fnmatch_casefold
    TEST(_UT_TNAME_)
    {
        std::cerr << std::endl << "START " << EQ(_UT_TNAME_) << std::endl;
        FilterMatch *fm = new FilterMatch("g0exactMatch");
        CHECK(false == fm->match("notreally"));
        CHECK(false == fm->match("exactMatchn"));
        CHECK(false == fm->match("exactMatc"));
        CHECK(true == fm->match("exactMatcH"));
        CHECK(true == fm->match("exactMatch"));
        delete fm;
        fm = new FilterMatch("g0exa*tch");
        CHECK(false == fm->match("notreally"));
        CHECK(false == fm->match("exactMatchn"));
        CHECK(false == fm->match("exactMatc"));
        CHECK(true == fm->match("exactMatcH"));
        CHECK(true == fm->match("exactMatch"));
        delete fm;
        fm = new FilterMatch("g0exact?atch");
        CHECK(true == fm->match("exactMatcH"));
        CHECK(true == fm->match("exactMatch"));
        std::cerr << std::endl << "END " << EQ(_UT_TNAME_) << std::endl;
        delete fm;
    }


#undef _UT_TNAME_
#define _UT_TNAME_ filtermatch_negfnmatch
    TEST(_UT_TNAME_)
    {
        std::cerr << std::endl << "START " << EQ(_UT_TNAME_) << std::endl;
        FilterMatch *fm = new FilterMatch("!f0exactMatch");
        CHECK(false != fm->match("notreally"));
        CHECK(false != fm->match("exactMatchn"));
        CHECK(false != fm->match("exactMatc"));
        CHECK(false != fm->match("exactMatcH"));
        CHECK(true != fm->match("exactMatch"));
        delete fm;
        fm = new FilterMatch("!f0exa*tch");
        CHECK(false != fm->match("notreally"));
        CHECK(false != fm->match("exactMatchn"));
        CHECK(false != fm->match("exactMatc"));
        CHECK(false != fm->match("exactMatcH"));
        CHECK(true != fm->match("exactMatch"));
        delete fm;
        fm = new FilterMatch("!f0exact?atch");
        CHECK(false != fm->match("exactMatcH"));
        CHECK(true != fm->match("exactMatch"));
        std::cerr << std::endl << "END " << EQ(_UT_TNAME_) << std::endl;
        delete fm;
    }

#undef _UT_TNAME_
#define _UT_TNAME_ filtermatch_negfnmatch_casefold
    TEST(_UT_TNAME_)
    {
        std::cerr << std::endl << "START " << EQ(_UT_TNAME_) << std::endl;
        FilterMatch *fm = new FilterMatch("!g0exactMatch");
        CHECK(false != fm->match("notreally"));
        CHECK(false != fm->match("exactMatchn"));
        CHECK(false != fm->match("exactMatc"));
        CHECK(true != fm->match("exactMatcH"));
        CHECK(true != fm->match("exactMatch"));
        delete fm;
        fm = new FilterMatch("!g0exa*tch");
        CHECK(false != fm->match("notreally"));
        CHECK(false != fm->match("exactMatchn"));
        CHECK(false != fm->match("exactMatc"));
        CHECK(true != fm->match("exactMatcH"));
        CHECK(true != fm->match("exactMatch"));
        delete fm;
        fm = new FilterMatch("!g0exact?atch");
        CHECK(true != fm->match("exactMatcH"));
        CHECK(true != fm->match("exactMatch"));
        std::cerr << std::endl << "END " << EQ(_UT_TNAME_) << std::endl;
        delete fm;
    }

#undef _UT_TNAME_
#define _UT_TNAME_ filtermatch_pcre
    TEST(_UT_TNAME_)
    {
        std::cerr << std::endl << "START " << EQ(_UT_TNAME_) << std::endl;
        FilterMatch *fm = new FilterMatch("r^exactMatch$");
        CHECK(false == fm->match("notreally"));
        CHECK(false == fm->match("exactMatchn"));
        CHECK(false == fm->match("exactMatc"));
        CHECK(false == fm->match("exactMatcH"));
        CHECK(true == fm->match("exactMatch"));
        delete fm;
        fm = new FilterMatch("r^(?i)exactMatch(?-i)$");
        CHECK(false == fm->match("notreally"));
        CHECK(false == fm->match("exactMatchn"));
        CHECK(false == fm->match("exactMatc"));
        CHECK(true == fm->match("exactMatcH"));
        CHECK(true == fm->match("exactMatch"));
        delete fm;
        fm = new FilterMatch("rcase.*(?i)exactMatch(?-i).*case");
        CHECK(false == fm->match("notreally"));
        CHECK(false == fm->match("exactMatchn"));
        CHECK(false == fm->match("exactMatc"));
        CHECK(true == fm->match("case exactMatcH case"));
        CHECK(true == fm->match("caseexactMatchcase"));
        CHECK(false == fm->match("Case exactMatcH case"));
        CHECK(false == fm->match("caseexactMatchCase"));
        delete fm;
        std::cerr << std::endl << "END " << EQ(_UT_TNAME_) << std::endl;
    }


}

#endif
