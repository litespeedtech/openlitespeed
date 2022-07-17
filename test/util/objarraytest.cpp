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

#include <lsr/ls_xpool.h>
#include <util/objarray.h>

#include <stdio.h>
#include <string.h>
#include "unittest-cpp/UnitTest++.h"

typedef struct testpair_s
{
    int key;
    int val;
} testpair_t;

TEST(objArrayTest_test)
{
    int i;
    ObjArrayXpool *pArray = new ObjArrayXpool(sizeof(testpair_t));
    ls_xpool_t *pool = ls_xpool_new();

    CHECK(pArray->capacity() == 0);
    CHECK(pArray->size() == 0);
    CHECK(pArray->getArray() == NULL);
    pArray->guarantee(pool, 10);
    CHECK(pArray->capacity() == 10);
    CHECK(pArray->size() == 0);
    CHECK(pArray->getArray() != NULL);
    for (i = 0; i < 10; ++i)
    {
        testpair_t *buf = (testpair_t *)pArray->getNew();
        buf->key = buf->val = i + 1;
    }
    CHECK(pArray->size() == 10);
    CHECK(pArray->capacity() == 10);
    CHECK(pArray->getNew() == NULL);
    for (i = 0; i < 10; ++i)
    {
        testpair_t *buf = (testpair_t *)pArray->getObj(i);
        CHECK(buf->key == i + 1 && buf->val == i + 1);
    }
    CHECK(pArray->getObj(0) == pArray->getArray());
    CHECK(pArray->getObj(-1) == NULL);
    CHECK(pArray->getObj(11) == NULL);
    pArray->guarantee(pool, 20);
    CHECK(pArray->capacity() == 20);
    CHECK(pArray->size() == 10);
    CHECK(pArray->getArray() != NULL);
    CHECK(pArray->getObj(15) == NULL);
    for (i = 10; i < 20; ++i)
    {
        testpair_t *buf = (testpair_t *)pArray->getNew();
        buf->key = buf->val = i + 1;
    }
    for (i = 0; i < 20; ++i)
    {
        testpair_t *buf = (testpair_t *)pArray->getObj(i);
        CHECK(buf->key == i + 1 && buf->val == i + 1);
    }
    pArray->setCapacity(pool, 30);
    CHECK(pArray->capacity() == 30);
    CHECK(pArray->size() == 20);
    CHECK(pArray->getArray() != NULL);
    for (i = 20; i < 30; ++i)
    {
        testpair_t *buf = (testpair_t *)pArray->getNew();
        buf->key = buf->val = i - 1;
    }
    for (i = 0; i < 20; ++i)
    {
        testpair_t *buf = (testpair_t *)pArray->getObj(i);
        CHECK(buf->key == i + 1 && buf->val == i + 1);
    }
    for (i = 20; i < 30; ++i)
    {
        testpair_t *buf = (testpair_t *)pArray->getObj(i);
        CHECK(buf->key == i - 1 && buf->val == i - 1);
    }
    ls_xpool_delete(pool);
    //Test with global pool.
    pArray = new ObjArrayXpool(sizeof(testpair_t));

    CHECK(pArray->capacity() == 0);
    CHECK(pArray->size() == 0);
    CHECK(pArray->getArray() == NULL);
    pArray->guarantee(NULL, 10);
    CHECK(pArray->capacity() == 10);
    CHECK(pArray->size() == 0);
    CHECK(pArray->getArray() != NULL);
    for (i = 0; i < 10; ++i)
    {
        testpair_t *buf = (testpair_t *)pArray->getNew();
        buf->key = buf->val = i + 1;
    }
    CHECK(pArray->size() == 10);
    CHECK(pArray->capacity() == 10);
    CHECK(pArray->getNew() == NULL);
    for (i = 0; i < 10; ++i)
    {
        testpair_t *buf = (testpair_t *)pArray->getObj(i);
        CHECK(buf->key == i + 1 && buf->val == i + 1);
    }
    pArray->guarantee(NULL, 20);
    CHECK(pArray->capacity() == 20);
    CHECK(pArray->size() == 10);
    CHECK(pArray->getArray() != NULL);
    pArray->release(NULL);
}

TEST(TObjArrayTest_test)
{
    int i;
    TObjArrayXpool<testpair_t> *pArray = new TObjArrayXpool<testpair_t>();
    ls_xpool_t *pool = ls_xpool_new();

    CHECK(pArray->capacity() == 0);
    CHECK(pArray->size() == 0);
    CHECK(pArray->getArray() == NULL);
    pArray->guarantee(pool, 10);
    CHECK(pArray->capacity() == 10);
    CHECK(pArray->size() == 0);
    CHECK(pArray->getArray() != NULL);
    for (i = 0; i < 10; ++i)
    {
        testpair_t *buf = pArray->getNew();
        buf->key = buf->val = i + 1;
    }
    CHECK(pArray->size() == 10);
    CHECK(pArray->capacity() == 10);
    CHECK(pArray->getNew() == NULL);
    for (i = 0; i < 10; ++i)
    {
        testpair_t *buf = pArray->getObj(i);
        CHECK(buf->key == i + 1 && buf->val == i + 1);
    }
    CHECK(pArray->getObj(0) == pArray->getArray());
    CHECK(pArray->getObj(-1) == NULL);
    CHECK(pArray->getObj(11) == NULL);
    pArray->guarantee(pool, 20);
    CHECK(pArray->capacity() == 20);
    CHECK(pArray->size() == 10);
    CHECK(pArray->getArray() != NULL);
    CHECK(pArray->getObj(15) == NULL);
    for (i = 10; i < 20; ++i)
    {
        testpair_t *buf = pArray->getNew();
        buf->key = buf->val = i + 1;
    }
    for (i = 0; i < 20; ++i)
    {
        testpair_t *buf = pArray->getObj(i);
        CHECK(buf->key == i + 1 && buf->val == i + 1);
    }
    pArray->setCapacity(pool, 30);
    CHECK(pArray->capacity() == 30);
    CHECK(pArray->size() == 20);
    CHECK(pArray->getArray() != NULL);
    for (i = 20; i < 30; ++i)
    {
        testpair_t *buf = pArray->getNew();
        buf->key = buf->val = i - 1;
    }
    for (i = 0; i < 20; ++i)
    {
        testpair_t *buf = pArray->getObj(i);
        CHECK(buf->key == i + 1 && buf->val == i + 1);
    }
    for (i = 20; i < 30; ++i)
    {
        testpair_t *buf = pArray->getObj(i);
        CHECK(buf->key == i - 1 && buf->val == i - 1);
    }
    ls_xpool_delete(pool);
}





#endif

