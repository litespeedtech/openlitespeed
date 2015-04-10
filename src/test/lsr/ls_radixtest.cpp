
/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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

#include <stdio.h>
#include <string.h>
#include <lsdef.h>
#include <lsr/ls_radix.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

static const char *pAdd[] =
{
    "123456789", // Add first
    "123446789", // split new before old
    "123486789", // insert end of children
    "123436789", // insert beginning of children
    "123466789", // insert middle of children
    "123556789", // split old before new
    "1234",      // Make the existing node have an object
    "123456",    // Cut off the end and make it a new child
    "223456789", // Insert new unrelated child.
    "123456789"  // Insert duplicate, should fail.
};

static int pResults[] =
{
    LS_OK,
    LS_OK,
    LS_OK,
    LS_OK,
    LS_OK,
    LS_OK,
    LS_OK,
    LS_OK,
    LS_OK,
    LS_FAIL
};

static const int outlen = 10;
TEST(ls_radix_test)
{
    int i, count = outlen;
    printf("*******BEGIN RADIX TEST********\n");
    ls_radix_t tree, tree2;
    ls_radix(&tree);
#ifdef LS_RADIX_DEBUG
    ls_radix_printtree(&tree);
#endif

    for (i = 0; i < count; ++i)
    {
#ifdef LS_RADIX_DEBUG
        printf("Inserting %s\n", pAdd[i]);
#endif
        CHECK(ls_radix_insert(&tree, pAdd[i], strlen(pAdd[i]), &tree)
                == pResults[i]);
#ifdef LS_RADIX_DEBUG
        ls_radix_printtree(&tree);
        printf("\n\n\n");
#endif
    }

    for (i = 0; i < count; ++i)
    {
        CHECK(ls_radix_find(&tree, pAdd[i], strlen(pAdd[i])) == &tree);
    }
    CHECK(ls_radix_find(&tree, "123", strlen("123")) == NULL);
    CHECK(ls_radix_find(&tree, "ABC", strlen("ABC")) == NULL);
    CHECK(ls_radix_find(&tree, "", strlen("")) == NULL);
    CHECK(ls_radix_find(&tree, "12345678", strlen("12345678")) == NULL);
    CHECK(ls_radix_find(&tree, "1234567890", strlen("1234567890")) == NULL);

    CHECK(ls_radix_update(&tree, "1234", strlen("1234"), NULL) == NULL);
    CHECK(ls_radix_update(&tree, "1234", strlen("1234"), &tree2) == &tree);

    ls_radix_d(&tree);

    printf("******* END RADIX TEST ********\n");
}

#endif



