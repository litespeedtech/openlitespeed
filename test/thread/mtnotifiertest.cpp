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
#include <stdio.h>
#include "unittest-cpp/UnitTest++.h"


bool didWait = false;
bool didSignal = false;

int waitFunction(void *arg) {
    return(0);
}

int signalFunction(void *arg) {
    return(0);
}

static void *waitThread(void *arg) {
    MtNotifier *pmtNotifier = (MtNotifier *)arg;
    int waiters;
    printf("Entered wait thread\n");
    waiters = pmtNotifier->fullwait();
    printf("Called wait, set something and broadcast\n");
    didWait = true;
    if (waiters > 0)
        pmtNotifier->notify();
    printf("Wait thread done\n");
    return(NULL);
}

static void *signalThread(void *arg) {
    MtNotifier *pmtNotifier = (MtNotifier *)arg;

    printf("Entered signal thread\n");
    pmtNotifier->notify();
    printf("Called signal, set something and broadcast\n");
    didSignal = true;
    printf("Signal thread done\n");
    return(NULL);
}

TEST(MTNOTIFIER_BLOCK_RELEASE_TEST)
{
    printf("Start first mtnotifier test\n");
    MtNotifier mtNotifier;
    pthread_t waitThreadID;
    pthread_t signalThreadID;
    pthread_attr_t pthread_attrWait, pthread_attrSignal;

    pthread_attr_init(&pthread_attrWait);
    pthread_attr_init(&pthread_attrSignal);

    pthread_create(&waitThreadID, &pthread_attrWait, waitThread, &mtNotifier);
    pthread_create(&signalThreadID, &pthread_attrSignal, signalThread, &mtNotifier);

    sleep(1); // Give it a second to finish.
    CHECK(didWait);
    CHECK(didSignal);
}

#endif



