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

#include <lsr/ls_lfqueue.h>
#include <lsr/ls_pool.h>
#include <thread/workcrew.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include "unittest-cpp/UnitTest++.h"

#define WORKCREWTEST_NUMWORKERS 16
#define DOWORKCREWTEST


#ifdef DOWORKCREWTEST

typedef struct
{
    ls_lfnodei_t m_node;
    int         m_oval;
    int         m_val;
} workcrewtest_t;

static void *workCrewTest(ls_lfnodei_t *item)
{
    workcrewtest_t *wct = (workcrewtest_t *)((char *)item - offsetof(
                workcrewtest_t, m_node));
    if (0 == (random() % 11)) {
        // occasional fast return
        wct->m_val += 3;
        return NULL;
    }
    usleep(random() % 2000000);
    wct->m_val++;
    usleep(random() % 2000000);
    wct->m_val++;
    usleep(random() % 2000000);
    wct->m_val++;
    return NULL;
}

typedef struct
{
    pthread_t   m_thread;
    int         m_exit;
    int         m_id;
    int         m_error;
    WorkCrew *  m_pwc;
} myThread_t;
static myThread_t threadMap[1000000];

static int s_start = 0;

static void *makeJobs(void *o)
{
    ls_futex_wait_priv(&s_start, 0, NULL);

    myThread_t *p = (myThread_t *)o;
    WorkCrew * wc = p->m_pwc;
    int i,j;
    for (i = 0; i < 5; ++i)
    {
        for (j = 0; j < 10; ++j)
        {
            workcrewtest_t *wct = (workcrewtest_t *)ls_palloc(sizeof(workcrewtest_t));
            wct->m_node.next = NULL;
            wct->m_oval = p->m_id*1000 + 10*i +j;
            wct->m_val = p->m_id*1000 + 10*i +j;
            CHECK(wc->addJob((ls_lfnodei_t *)((char *)wct + offsetof(workcrewtest_t,
                                m_node))) == 0);
        }
#ifdef GATE_BY_COMPLETION
        // this waits for batches to finish before creating more jobs - uses
        // less workers (the crew grows less) but total completion is slower
        if (i < 5) {
            printf("makeoJobs worker thread %d waitForCompletion\n", (int)(pthread_self() & 0xFFFF));
            wc->waitForCompletion(NULL);
        }
#endif
    }
    return NULL;
}

TEST(THREAD_WORKCREW_TEST)
{
    printf("Start work crew test!\n");
    int i;
    long ret;
    ls_lfqueue_t *pFinishedQueue = ls_lfqueue_new();
    WorkCrew *wc = new WorkCrew(7, workCrewTest, pFinishedQueue, 0);

    CHECK(0 == wc->size());

    ret = wc->maxWorkers(40);

    //CHECK(1 == ret); // default

    ret = wc->adjustIdle(3, 10);
    CHECK(0 == ret);

    ret = wc->maxWorkers(1);
    CHECK(-1 == ret); // lower than min workers, rejected

    ret = wc->maxWorkers(WORKCREWTEST_NUMWORKERS);
    CHECK(40 == ret);

    for (i = 0; i < 50; ++i)
    {
        workcrewtest_t *wct = (workcrewtest_t *)ls_palloc(sizeof(workcrewtest_t));
        wct->m_node.next = NULL;
        wct->m_oval = i;
        wct->m_val = i;
        CHECK(wc->addJob((ls_lfnodei_t *)((char *)wct + offsetof(workcrewtest_t,
                            m_node))) == 0);
    }

    CHECK(wc->startProcessing() == 0);
    ret = wc->maxWorkers(WORKCREWTEST_NUMWORKERS << 1);

    //CHECK(WORKCREWTEST_NUMWORKERS <= wc->size());

    for (; i < 100; ++i)
    {
        workcrewtest_t *wct = (workcrewtest_t *)ls_palloc(sizeof(workcrewtest_t));
        wct->m_node.next = NULL;
        wct->m_oval = i;
        wct->m_val = i;
        CHECK(wc->addJob((ls_lfnodei_t *)((char *)wct + offsetof(workcrewtest_t,
                            m_node))) == 0);
    }

    CHECK(WORKCREWTEST_NUMWORKERS <= wc->size());


    for (i = 0; i < 100; ++i)
    {
        ls_lfnodei_t *pNode = NULL;
        workcrewtest_t *wct = NULL;
        while ((pNode = ls_lfqueue_get(pFinishedQueue)) == NULL) {
        }
        wct = (workcrewtest_t *)((char *)pNode - offsetof(workcrewtest_t, m_node));
        LS_TH_IGN_RD_BEG();
        // thread sanitizer identifies a race here, but the code
        // only checks wct after pulling it from the finished queue.
        // TODO: determine if there is a memory reuse issue, or at least
        // that there is no possible real race between work-routine incrementing
        // the value and the CHECK reading it
        CHECK(wct->m_oval == wct->m_val-3);
        LS_TH_IGN_RD_END();
        ls_pfree(wct);
    }
    CHECK(i == 100);

    wc->maxWorkers(4);
    wc->stopProcessing();
    delete wc;
    ls_lfqueue_delete(pFinishedQueue);
    printf("End work crew test!\n");
}

TEST(THREAD_WORKCREW_TEST_MULTI)
{
    printf("Start work crew test multi\n");
    int i;
    int numThreads = 5;
    myThread_t *p;
    ls_lfqueue_t *pFinishedQueue = ls_lfqueue_new();
    WorkCrew *wc = new WorkCrew(WORKCREWTEST_NUMWORKERS, workCrewTest, pFinishedQueue, 0);

    // CHECK(WORKCREWTEST_NUMWORKERS == wc->size());

    for (i = 0; i < 50; ++i)
    {
        workcrewtest_t *wct = (workcrewtest_t *)ls_palloc(sizeof(workcrewtest_t));
        wct->m_node.next = NULL;
        wct->m_oval = i;
        wct->m_val = i;
        CHECK(wc->addJob((ls_lfnodei_t *)((char *)wct + offsetof(workcrewtest_t,
                            m_node))) == 0);
    }

    CHECK(wc->startProcessing() == 0);
    //int count = wc->size();
    //CHECK(WORKCREWTEST_NUMWORKERS <= count); Might be finished

    p = threadMap;

    for (i = 0; i < numThreads; i++, p++)
    {
        p->m_exit = 0;
        p->m_id = i;
        p->m_error = 0;
        p->m_pwc = wc;

        if (pthread_create(&p->m_thread, NULL, makeJobs, (void *)p))
        {
            std::cerr << "Thread creation failed, total threads completed " << i << std::endl;
            numThreads = i;
            break;
        }
    }
    s_start = 1;
    ls_futex_wakeall_priv(&s_start);


//     struct timespec t = { 0, 10 };
//     printf("main thread %d waitForCompletion 10 nSec\n", (int)(pthread_self() & 0xFFFF));
//     int wt = wc->waitForCompletion(&t);
//     CHECK(-1 == wt);
//
//     t.tv_sec = 75;
//     printf("main thread %d waitForCompletion 75 sec\n", (int)(pthread_self() & 0xFFFF));
//     do {
//         wt = wc->waitForCompletion(&t);
//         printf("main thread %d waitForCompletion LOOP 75 sec ret %d pending jobs %d\n",
//                 (int)(pthread_self() & 0xFFFF), wt, wc->pendingJobs());
//     } while (wt != 0 && wc->pendingJobs());
//
//     printf("main thread %d waitForCompletion DONE 75 sec ret %d pending jobs %d\n",
//             (int)(pthread_self() & 0xFFFF), wt, wc->pendingJobs());
//
//     CHECK(0 == wt);
//
//     while (wc->pendingJobs()) {
//         sched_yield();
//     }

    p = threadMap;
    for (i = 0; i < numThreads; i++, p++) {
        if (int err = pthread_join(p->m_thread, (void **)&p->m_exit)) {
            std::cerr << "FAILED to join thread " << p->m_thread << " error "
                << err << " exit " << p->m_exit << std::endl;
        };
    }

    wc->stopProcessing();
    CHECK(0 == wc->size());
    delete wc;

//     for (i = 0; i < (50 + 50 * numThreads); ++i)
//     {
//         ls_lfnodei_t *pNode = NULL;
//         workcrewtest_t *wct = NULL;
//         while ((pNode = ls_lfqueue_get(pFinishedQueue)) == NULL)
//         {
//         }
//         wct = (workcrewtest_t *)((char *)pNode - offsetof(workcrewtest_t, m_node));
//         CHECK(wct->m_oval == wct->m_val-3);
//         ls_pfree(wct);
//     }
//     CHECK(i == (50 + 50 * numThreads));
//
    ls_lfqueue_delete(pFinishedQueue);
    printf("End work crew test multi\n");
}
#endif

#endif
