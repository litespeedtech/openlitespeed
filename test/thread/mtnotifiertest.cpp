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
#include <unistd.h>

#include <thread/mtnotifier.h>

#include <pthread.h>
#include "unittest-cpp/UnitTest++.h"


bool didWait = false;
bool didSignal = false;

static void *waitThread(void *arg)
{
    MtNotifier *pmtNotifier = (MtNotifier *)arg;
    pmtNotifier->fullwait();
    didWait = true;
    return(NULL);
}

TEST(MTNOTIFIER_BLOCK_RELEASE_TEST)
{
    didWait = false;
    didSignal = false;
    MtNotifier mtNotifier;
    pthread_t waitThreadID;
    pthread_attr_t pthread_attrWait;

    pthread_attr_init(&pthread_attrWait);

    int ret = pthread_create(&waitThreadID, &pthread_attrWait, waitThread,
                             &mtNotifier);
    pthread_attr_destroy(&pthread_attrWait);
    CHECK(ret == 0);
    if (ret != 0)
        return;

    for (int i = 0; i < 2000 && mtNotifier.getWaiters() == 0; ++i)
        usleep(1000);

    CHECK(mtNotifier.getWaiters() > 0);
    if (mtNotifier.getWaiters() == 0)
    {
        pthread_cancel(waitThreadID);
        pthread_join(waitThreadID, NULL);
        return;
    }

    didSignal = (mtNotifier.notify() > 0);
    pthread_join(waitThreadID, NULL);
    CHECK(didWait);
    CHECK(didSignal);
    CHECK(mtNotifier.getWaiters() == 0);
}

#endif

