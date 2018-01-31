/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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

#include "linkedobjtest.h"
#include <util/linkedobj.h>
#include "unittest-cpp/UnitTest++.h"


SUITE(LinkedObjTest)
{
    TEST(testLinkedObj)
    {
        LinkedObj obj1, obj2, obj3;
        CHECK(obj1.next() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj3.next() == NULL);

        obj1.addNext(&obj2);
        CHECK(obj1.next() == &obj2);
        CHECK(obj2.next() == NULL);

        obj1.addNext(&obj3);
        CHECK(obj1.next() == &obj3);
        CHECK(obj2.next() == NULL);
        CHECK(obj3.next() == &obj2);

        CHECK(obj2.removeNext() == NULL);
        CHECK(obj1.next() == &obj3);
        CHECK(obj2.next() == NULL);
        CHECK(obj3.next() == &obj2);

        CHECK(obj3.removeNext() == &obj2);
        CHECK(obj1.next() == &obj3);
        CHECK(obj2.next() == NULL);
        CHECK(obj3.next() == NULL);

        obj3.addNext(&obj2);
        CHECK(obj1.next() == &obj3);
        CHECK(obj2.next() == NULL);
        CHECK(obj3.next() == &obj2);

        CHECK(obj1.removeNext() == &obj3);
        CHECK(obj1.next() == &obj2);
        CHECK(obj2.next() == NULL);
        CHECK(obj3.next() == NULL);

        CHECK(obj1.removeNext() == &obj2);
        CHECK(obj1.next() == NULL);
        CHECK(obj2.next() == NULL);

    }

    TEST(testDLinkedObj)
    {
        DLinkedObj obj1, obj2, obj3;
        CHECK(obj1.next() == NULL);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == NULL);
        CHECK(obj3.next() == NULL);
        CHECK(obj3.prev() == NULL);

        obj1.addNext(&obj2);
        CHECK(obj1.next() == &obj2);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == &obj1);

        obj1.addNext(&obj3);
        CHECK(obj1.next() == &obj3);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == &obj3);
        CHECK(obj3.next() == &obj2);
        CHECK(obj3.prev() == &obj1);

        CHECK(obj2.removeNext() == NULL);
        CHECK(obj1.next() == &obj3);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == &obj3);
        CHECK(obj3.next() == &obj2);
        CHECK(obj3.prev() == &obj1);

        CHECK(obj3.removeNext() == &obj2);
        CHECK(obj1.next() == &obj3);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == NULL);
        CHECK(obj3.next() == NULL);
        CHECK(obj3.prev() == &obj1);

        obj3.addNext(&obj2);
        CHECK(obj1.next() == &obj3);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == &obj3);
        CHECK(obj3.next() == &obj2);
        CHECK(obj3.prev() == &obj1);

        CHECK(obj2.remove() == NULL);
        CHECK(obj1.next() == &obj3);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == NULL);
        CHECK(obj3.next() == NULL);
        CHECK(obj3.prev() == &obj1);

        obj3.addNext(&obj2);
        CHECK(obj1.next() == &obj3);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == &obj3);
        CHECK(obj3.next() == &obj2);
        CHECK(obj3.prev() == &obj1);

        CHECK(obj3.remove() == &obj2);
        CHECK(obj1.next() == &obj2);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.prev() == &obj1);
        CHECK(obj2.next() == NULL);
        CHECK(obj3.next() == NULL);
        CHECK(obj3.prev() == NULL);

        obj1.addNext(&obj3);
        CHECK(obj1.next() == &obj3);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == &obj3);
        CHECK(obj3.next() == &obj2);
        CHECK(obj3.prev() == &obj1);

        CHECK(obj1.removeNext() == &obj3);
        CHECK(obj1.next() == &obj2);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.prev() == &obj1);
        CHECK(obj2.next() == NULL);
        CHECK(obj3.next() == NULL);
        CHECK(obj3.prev() == NULL);

        CHECK(obj1.removeNext() == &obj2);
        CHECK(obj1.next() == NULL);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == NULL);
        CHECK(obj3.next() == NULL);
        CHECK(obj3.prev() == NULL);

        obj2.addPrev(&obj1);
        CHECK(obj1.next() == &obj2);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == &obj1);

        obj2.addPrev(&obj3);
        CHECK(obj1.next() == &obj3);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == &obj3);
        CHECK(obj3.next() == &obj2);
        CHECK(obj3.prev() == &obj1);

        CHECK(obj1.removePrev() == NULL);
        CHECK(obj1.next() == &obj3);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == &obj3);
        CHECK(obj3.next() == &obj2);
        CHECK(obj3.prev() == &obj1);

        CHECK(obj3.removePrev() == &obj1);
        CHECK(obj1.next() == NULL);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == &obj3);
        CHECK(obj3.next() == &obj2);
        CHECK(obj3.prev() == NULL);

        obj3.addPrev(&obj1);
        CHECK(obj1.next() == &obj3);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == &obj3);
        CHECK(obj3.next() == &obj2);
        CHECK(obj3.prev() == &obj1);

        CHECK(obj2.removePrev() == &obj3);
        CHECK(obj1.next() == &obj2);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.prev() == &obj1);
        CHECK(obj2.next() == NULL);
        CHECK(obj3.next() == NULL);
        CHECK(obj3.prev() == NULL);

        CHECK(obj2.removePrev() == &obj1);
        CHECK(obj1.next() == NULL);
        CHECK(obj1.prev() == NULL);
        CHECK(obj2.next() == NULL);
        CHECK(obj2.prev() == NULL);
        CHECK(obj3.next() == NULL);
        CHECK(obj3.prev() == NULL);
    }

    TEST(testLinkedObjChain2Next)
    {
        LinkedObj obj1, obj2, obj3;
        LinkedObj obj4, obj5;

        obj1.addNext(&obj2);
        obj2.addNext(&obj3);

        obj4.addNext(&obj5);

        obj1.addNext(&obj4, &obj5);

        CHECK(obj1.next() == &obj4);
        CHECK(obj4.next() == &obj5);
        CHECK(obj5.next() == &obj2);
        CHECK(obj2.next() == &obj3);
        CHECK(obj3.next() == NULL);
    }

    TEST(testLinkedObjChain3Next)
    {
        LinkedObj obj1, obj2, obj3;
        LinkedObj obj4, obj5, obj6;
        obj1.addNext(&obj2);
        obj2.addNext(&obj3);

        obj4.addNext(&obj5);
        obj5.addNext(&obj6);

        obj1.addNext(&obj4, &obj6);

        CHECK(obj1.next() == &obj4);
        CHECK(obj4.next() == &obj5);
        CHECK(obj5.next() == &obj6);
        CHECK(obj6.next() == &obj2);
        CHECK(obj2.next() == &obj3);
        CHECK(obj3.next() == NULL);
    }

    TEST(testDLinkedObjChain2Next)
    {
        DLinkedObj obj1, obj2, obj3;
        DLinkedObj obj4, obj5;
        obj1.addNext(&obj2);
        obj2.addNext(&obj3);

        obj4.addNext(&obj5);

        obj1.addNext(&obj4, &obj5);

        CHECK(obj1.next() == &obj4);
        CHECK(obj4.next() == &obj5);
        CHECK(obj5.next() == &obj2);
        CHECK(obj2.next() == &obj3);
        CHECK(obj3.next() == NULL);


        CHECK(obj3.prev() == &obj2);
        CHECK(obj2.prev() == &obj5);
        CHECK(obj5.prev() == &obj4);
        CHECK(obj4.prev() == &obj1);
        CHECK(obj1.prev() == NULL);
    }

    TEST(testDLinkedObjChain3Next)
    {
        DLinkedObj obj1, obj2, obj3;
        DLinkedObj obj4, obj5, obj6;
        obj1.addNext(&obj2);
        obj2.addNext(&obj3);

        obj4.addNext(&obj5);
        obj5.addNext(&obj6);

        obj1.addNext(&obj4, &obj6);

        CHECK(obj1.next() == &obj4);
        CHECK(obj4.next() == &obj5);
        CHECK(obj5.next() == &obj6);
        CHECK(obj6.next() == &obj2);
        CHECK(obj2.next() == &obj3);
        CHECK(obj3.next() == NULL);


        CHECK(obj3.prev() == &obj2);
        CHECK(obj2.prev() == &obj6);
        CHECK(obj6.prev() == &obj5);
        CHECK(obj5.prev() == &obj4);
        CHECK(obj4.prev() == &obj1);
        CHECK(obj1.prev() == NULL);

    }

    TEST(testDLinkedObjChain3Prev)
    {
        DLinkedObj obj1, obj2, obj3;
        DLinkedObj obj4, obj5, obj6;
        obj1.addNext(&obj2);
        obj2.addNext(&obj3);

        obj4.addNext(&obj5);
        obj5.addNext(&obj6);

        obj1.addPrev(&obj4, &obj6);

        CHECK(obj4.next() == &obj5);
        CHECK(obj5.next() == &obj6);
        CHECK(obj6.next() == &obj1);
        CHECK(obj1.next() == &obj2);
        CHECK(obj2.next() == &obj3);
        CHECK(obj3.next() == NULL);

        CHECK(obj3.prev() == &obj2);
        CHECK(obj2.prev() == &obj1);
        CHECK(obj1.prev() == &obj6);
        CHECK(obj6.prev() == &obj5);
        CHECK(obj5.prev() == &obj4);
        CHECK(obj4.prev() == NULL);
    }

}

#endif
