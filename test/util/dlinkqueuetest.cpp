/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2020  LiteSpeed Technologies, Inc.                 *
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

#include "dlinkqueuetest.h"
#include <util/dlinkqueue.h>
#include "unittest-cpp/UnitTest++.h"


SUITE(DLinkQueueTest)
{
    TEST(testDLinkQueue)
    {
        DLinkQueue q1;

        CHECK(q1.empty());
        CHECK(0 == q1.size());

        CHECK(q1.begin() == q1.head());
        CHECK(q1.end() == q1.head());

        CHECK(q1.rbegin() == q1.head());
        CHECK(q1.rend() == q1.head());

        DLinkedObj obj1, obj2, obj3;

        q1.append(&obj1);
        CHECK(!q1.empty());
        CHECK(1 == q1.size());

        CHECK(q1.begin() == &obj1);
        CHECK(q1.end() == q1.head());

        CHECK(q1.rbegin() == &obj1);
        CHECK(q1.rend() == q1.head());

        CHECK(obj1.next() == q1.head());
        CHECK(obj1.prev() == q1.head());

        q1.append(&obj2);
        CHECK(!q1.empty());
        CHECK(2 == q1.size());

        CHECK(q1.begin() == &obj1);
        CHECK(q1.end() == q1.head());

        CHECK(obj1.next() == &obj2);
        CHECK(obj1.prev() == q1.head());

        CHECK(obj2.next() == q1.head());
        CHECK(obj2.prev() == &obj1);

        CHECK(q1.rbegin() == &obj2);
        CHECK(q1.rend() == q1.head());

        q1.insert_after(&obj1, &obj3);
        CHECK(!q1.empty());
        CHECK(3 == q1.size());

        CHECK(q1.begin() == &obj1);
        CHECK(q1.end() == q1.head());

        CHECK(obj1.next() == &obj3);
        CHECK(obj1.prev() == q1.head());

        CHECK(obj3.next() == &obj2);
        CHECK(obj3.prev() == &obj1);

        CHECK(obj2.next() == q1.head());
        CHECK(obj2.prev() == &obj3);

        CHECK(q1.rbegin() == &obj2);
        CHECK(q1.rend() == q1.head());

    }

    TEST(testDLinkQueueChainAppend)
    {
        DLinkedObj arr1[10], arr2[5];
        DLinkQueue tgt, src;

        for (size_t i = 0; i < sizeof(arr1)/sizeof(DLinkedObj); i++)
        {
            tgt.append(&arr1[i]);
        }

        for (size_t i = 0; i < sizeof(arr2)/sizeof(DLinkedObj); i++)
        {
            src.append(&arr2[i]);
        }

        tgt.append(&src);

        CHECK(arr1[9].next() == &arr2[0]);
        CHECK(arr2[0].prev() == &arr1[9]);
        CHECK(arr2[4].next() == tgt.head());
        CHECK(src.empty());
        CHECK(0 == src.size());
        CHECK(15 == tgt.size());
        CHECK(tgt.head()->prev()==&arr2[4]);
    }

    TEST(testDLinkQueueChainPushFront)
    {
        DLinkedObj arr1[10], arr2[5];
        DLinkQueue tgt, src;

        for (size_t i = 0; i < sizeof(arr1)/sizeof(DLinkedObj); i++)
        {
            tgt.append(&arr1[i]);
        }

        for (size_t i = 0; i < sizeof(arr2)/sizeof(DLinkedObj); i++)
        {
            src.append(&arr2[i]);
        }

        tgt.push_front(&src);

        CHECK(tgt.head()->next()==&arr2[0]);
        CHECK(arr2[4].next() == &arr1[0]);
        CHECK(arr1[0].prev() == &arr2[4]);
        CHECK(arr2[0].prev() == tgt.head());
        CHECK(src.empty());
        CHECK(0 == src.size());
        CHECK(15 == tgt.size());
    }

}

#endif
