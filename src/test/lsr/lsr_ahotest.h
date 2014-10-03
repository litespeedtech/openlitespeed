/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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

#ifndef LSR_AHOTEST_H
#define LSR_AHOTEST_H

#include <sys/types.h>
#include <lsr/lsr_aho.h>




lsr_aho_t *lsr_aho_initTree( const char *acceptBuf[], int bufCount, int sensitive );

const int lsr_aho_TestOneLen = 86;
const char *lsr_aho_TestOne = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Quisque sagittis lectus erat.";

//Find first occurrence
const int lsr_aho_AcceptOneLen = 5;
const char *lsr_aho_AcceptOne[] = 
{
    "dolof",
    "ultricies",
    "mattis",
    "Quisque",
    "Mauris"
};

//Never find occurrence
const int lsr_aho_AcceptTwoLen = 5;
const char *lsr_aho_AcceptTwo[] = 
{
    "stuff",
    "blah",
    "extravagent",
    "excluded",
    "unused"
};

const int lsr_aho_TestTwoLen = 5;
const char *lsr_aho_TestTwo = "apple";

//Length of accept is longer than input
const int lsr_aho_AcceptThreeLen = 4;
const char *lsr_aho_AcceptThree[] =
{
    "alphabet",
    "octangular",
    "western",
    "parabolic"
};

//Input matches all but last character
const int lsr_aho_AcceptFourLen = 5;
const char *lsr_aho_AcceptFour[] =
{
    "appla",
    "applb",
    "applc",
    "appld",
    "apple"
};

const int lsr_aho_TestThreeLen = 21;
const char *lsr_aho_TestThree = "blahabdcdegfghjkstuff";

//Jump branches
const int lsr_aho_AcceptFiveLen = 4;
const char *lsr_aho_AcceptFive[] = 
{
    "abcdefg",
    "bdcdefgh",
    "degfghi",
    "egfghjk"
};

const int lsr_aho_TestFourLen = 21;
const char *lsr_aho_TestFour = "APPLESBANANASCHERRIES";

//Case insensitive and sensitive
const int lsr_aho_AcceptSixLen = 2;
const char *lsr_aho_AcceptSix[] = 
{
    "bAnAnas",
    "Apples"
};

const int lsr_aho_TestFiveLen = 10;
const char *lsr_aho_TestFive = "aaaaaaaaaa";

//Test null check
const int lsr_aho_AcceptSevenLen = 8;
const char *lsr_aho_AcceptSeven[] =
{
    "webZIP",
    "webCopier",
    "webStripper",
    "siteSnagger",
    "proWebWalker",
    "cheeseBot",
    "breakdown",
    "octangular"
};

//Test pattern shrinking
const int lsr_aho_AcceptEightLen = 5;
const char *lsr_aho_AcceptEight[] =
{
    "abcdefgh",
    "abcdefg",
    "abcdef",
    "abcde",
    "abcd"
};

const char *lsr_aho_TestInput[] = 
{
    lsr_aho_TestOne,
    lsr_aho_TestOne,
    lsr_aho_TestTwo,
    lsr_aho_TestTwo,
    lsr_aho_TestThree,
    lsr_aho_TestFour,
    lsr_aho_TestFour,
    lsr_aho_TestFive,
    lsr_aho_TestThree
};
int lsr_aho_TestInputLen[] = 
{
    lsr_aho_TestOneLen,
    lsr_aho_TestOneLen,
    lsr_aho_TestTwoLen,
    lsr_aho_TestTwoLen,
    lsr_aho_TestThreeLen,
    lsr_aho_TestFourLen,
    lsr_aho_TestFourLen,
    lsr_aho_TestFiveLen,
    lsr_aho_TestThreeLen
};
const char **lsr_aho_TestAccept[] = 
{
    lsr_aho_AcceptOne,
    lsr_aho_AcceptTwo,
    lsr_aho_AcceptThree,
    lsr_aho_AcceptFour,
    lsr_aho_AcceptFive,
    lsr_aho_AcceptSix,
    lsr_aho_AcceptSix,
    lsr_aho_AcceptSeven,
    lsr_aho_AcceptEight
};
int lsr_aho_TestAcceptLen[] = 
{
    lsr_aho_AcceptOneLen,
    lsr_aho_AcceptTwoLen,
    lsr_aho_AcceptThreeLen,
    lsr_aho_AcceptFourLen,
    lsr_aho_AcceptFiveLen,
    lsr_aho_AcceptSixLen,
    lsr_aho_AcceptSixLen,
    lsr_aho_AcceptSevenLen,
    lsr_aho_AcceptEightLen
};

size_t lsr_aho_OutStartRes[] = 
{
    57,
    (size_t)-1,
    (size_t)-1,
    0,
    9,
    (size_t)-1,
    0,
    (size_t)-1,
    (size_t)-1
};
size_t lsr_aho_OutEndRes[] = 
{
    64,
    (size_t)-1,
    (size_t)-1,
    5,
    16,
    (size_t)-1,
    6,
    (size_t)-1,
    (size_t)-1
};
int lsr_aho_Sensitive[] = 
{
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    1,
    1
};

#endif // LSR_AHOTEST_H

#endif
