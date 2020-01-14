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
#include <thread/workcrew.h>

#include <edio/eventnotifier.h>
#include <log4cxx/logger.h>
#include <lsr/ls_lfqueue.h>
#include <lsr/ls_pool.h>

#ifndef LS_WORKCREW_LF
#include <thread/pthreadworkqueue.h>
#endif

#ifdef LS_WORKCREW_DEBUG
#include <string.h>
#define DPRINTF(...) fprintf(stderr, __VA_ARGS__)
#else
#define DPRINTF(...)
#endif

#include <new>


WorkCrew::WorkCrew(EventNotifier *en)
    : m_pProcess(NULL)
    , m_pFinishedQueue(NULL)
    , m_pNotifier(en)
    , m_crew()
    , m_emptySlots()
    , m_stateFutex(TO_START)
    , m_nice(0)
    , m_minIdle(1)
    , m_maxIdle(10)
    , m_maxWorkers(1)
    , m_idleWorkers(0)
    , m_iRunningWorkers(0)
    , m_cleanUp()
{
    init();
}


WorkCrew::WorkCrew(int maxWorkers, WorkCrewProcessFn  processor,
                   ls_lfqueue_t * finishedQueue, EventNotifier *en)
    : m_pProcess(processor)
    , m_pFinishedQueue(finishedQueue)
    , m_pNotifier(en)
    , m_crew()
    , m_emptySlots()
    , m_stateFutex(TO_START)
    , m_nice(0)
    , m_minIdle(1)
    , m_maxIdle(10)
    , m_maxWorkers(maxWorkers)
    , m_idleWorkers(0)
    , m_iRunningWorkers(0)
    , m_cleanUp()
{
    DPRINTF("WRKRW CNSTRCT %s\n", pStatus());
    init();
}


WorkCrew::WorkCrew(int32_t maxWorkers, WorkCrewProcessFn  processor,
                   ls_lfqueue_t * finishedQueue, EventNotifier *en,
                   int32_t minIdleWorkers, int32_t maxIdleWorkers)
    : m_pProcess(processor)
    , m_pFinishedQueue(finishedQueue)
    , m_pNotifier(en)
    , m_crew()
    , m_emptySlots()
    , m_stateFutex(TO_START)
    , m_nice(0)
    , m_minIdle(minIdleWorkers)
    , m_maxIdle(maxIdleWorkers)
    , m_maxWorkers(maxWorkers)
    , m_idleWorkers(0)
    , m_iRunningWorkers(0)
    , m_cleanUp()
{
    DPRINTF("WRKRW CNSTRCT %s\n", pStatus());
    init();
}


void WorkCrew::init()
{
    DPRINTF("WRKRW INIT %s\n", pStatus());

    ls_mutex_setup(&m_crewLock);
    ls_mutex_setup(&m_addWorker);
    sigemptyset(&m_sigBlock);
    sigaddset(&m_sigBlock, SIGCHLD);

    if (m_maxIdle < m_minIdle)
        m_maxIdle = m_minIdle;

#ifdef LS_WORKCREW_LF
    m_pJobQueue = ls_lfqueue_new();
#else
    m_pJobQueue = new PThreadWorkQueue();
#endif
}


void WorkCrew::blockSig(int signum)
{
    sigaddset(&m_sigBlock, signum);
}


WorkCrew::~WorkCrew()
{
    DPRINTF("WRKRW DSTRCT %s\n", pStatus());
    if (m_stateFutex < STOPPED) {
        stopProcessing(1);
    }
    ls_mutex_lock(&m_crewLock);
#ifdef LS_WORKCREW_LF
    ls_lfqueue_delete(m_pJobQueue);
#else
    delete m_pJobQueue;
#endif

    m_cleanUp.release_objects();
    ls_mutex_unlock(&m_crewLock);

    int count = ls_atomic_fetch_add(&m_iRunningWorkers, 0);
    if (count > 0)
    {
        while(count-- > 0)
            sched_yield();
        usleep(1000);
    }

}


int WorkCrew::getSlot()
{
    int32_t slot = LS_FAIL;

    ls_mutex_lock(&m_crewLock);
    if (size() < ls_atomic_fetch_add(&m_maxWorkers, 0))
    {
        if (m_emptySlots.size())
        {
            slot = m_emptySlots.pop_back();
        }
        else
        {
            slot = m_crew.size();
            m_crew.push_back((CrewWorker *) NULL);
        }
        assert(slot >= 0 && slot < m_crew.size());
        assert(m_crew[slot] == NULL);
    }
    ls_mutex_unlock(&m_crewLock);
    return slot;
}


void WorkCrew::releaseSlot(int32_t slot)
{
    ls_mutex_lock(&m_crewLock);
    m_emptySlots.push_back(slot);
    ls_mutex_unlock(&m_crewLock);
}


int WorkCrew::addWorker()
{
    int ret = 0;
    int32_t slot;

    slot = getSlot();
    if (slot == -1)
        return LS_FAIL;
    CrewWorker *worker = new CrewWorker(this, slot);
    if (!worker)
        return LS_FAIL;
    // TODO see if this is a real issue - new should take care of it? LS_TH_NEWMEM(worker, sizeof(CrewWorker));
    worker->blockSigs(&m_sigBlock);
    m_crew[slot] = worker;

    if (worker)
    {
        ls_atomic_fetch_add(&m_iRunningWorkers, 1);
        if ( (ret = worker->start()) )
        {
            ls_atomic_fetch_add(&m_iRunningWorkers, -1);
            m_crew[slot] = NULL;
            delete worker;
        }
        else
        {
            if (m_crew[slot])
            {
                DPRINTF("ADDWRKR START Worker tid %lu in slot %d, %s\n",
                    worker->getHandle(), slot, pStatus());
            }
            return 0;
        }
    }
    releaseSlot(slot);
    return(LS_FAIL);
}


ls_lfnodei_t *WorkCrew::getJob(bool poll)
{
#ifdef LS_WORKCREW_LF
    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 250000000;
    return poll ? ls_lfqueue_get(m_pJobQueue) : ls_lfqueue_timedget(m_pJobQueue, &timeout);
#else
    int size = 1;
    ls_lfnodei_t *pWork;
    pWork = NULL;
    if (poll) {
        if (m_pJobQueue->get(&pWork, size) != 0)
            return NULL;
    }
    else {
        if (m_pJobQueue->get(&pWork, size, 250) != 0)
            return NULL;
    }
    return pWork;
#endif
}


int WorkCrew::startJobProcessor(int numWorkers,
        ls_lfqueue_t *pFinishedQueue,
        WorkCrewProcessFn processor)
{
    assert(processor && pFinishedQueue);
    m_pProcess = processor;
    m_pFinishedQueue = pFinishedQueue;
    m_maxWorkers = numWorkers;
    LS_DBG_H("WorkCrew::startJobProcessor(), Starting Processor.");
    return startProcessing();
}


int WorkCrew::startProcessing()
{
    assert(m_pProcess);
    if (!ls_atomic_casint(&m_stateFutex, TO_START, RUNNING))
    {
        LS_DBG_H("WorkCrew::startProcessing() WRONG STATE %d.", m_stateFutex);
        return LS_FAIL;
    }
    LS_DBG_H("WorkCrew::startProcessing(), Starting Processor.");
#ifndef LS_WORKCREW_LF
    m_pJobQueue->start();
#endif
    ls_mutex_lock(&m_addWorker);
    for (int32_t i = 0; i < m_minIdle; i++) {
        (void) addWorker();
    }
    ls_mutex_unlock(&m_addWorker);
    return 0;
}


void WorkCrew::stopProcessing(int disard)
{
    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 500000000;

    DPRINTF("STPPROC stopProcessing %s\n", pStatus());
    if (!ls_atomic_casint(&m_stateFutex, RUNNING, STOPPING))
    {
        LS_DBG_H("WorkCrew::stopProcessing() WRONG STATE %d.", m_stateFutex);
        DPRINTF("stopProcessing: WRONG STATE %d %s\n", m_stateFutex, pStatus());
        return;
    }

    stopAllWorkers(disard);
#ifndef LS_WORKCREW_LF
    m_pJobQueue->shutdown();
#endif

    // now wait for all to die
    while ( !disard && !isAllWorkerDead() ) {
        DPRINTF("STPPROC WAITDIE %s\n", pStatus());
        int ret = 0;
        //LS_TH_BENIGN(&m_stateFutex, "ok read");
        do {
            ret = ls_futex_wait_priv(&m_stateFutex, STOPPING, &timeout);
        } while (0 == ret && STOPPING == ls_atomic_fetch_add(&m_stateFutex, 0));
        //LS_TH_FLUSH();

        if (m_stateFutex == STOPPED) {
            assert(isAllWorkerDead());
            break;
        }

        if (!isAllWorkerDead()) {
            stopAllWorkers(0);
        }
        else {
            // all workers are gone but state didn't change ???
            DPRINTF("STPPROC WAITDIE workers gone but state WRONG %d %s\n", m_stateFutex, pStatus());
            ls_atomic_setint(&m_stateFutex, STOPPED);
            break;
        }
        sched_yield();
        usleep(5000);
    }


    LS_DBG_H("WorkCrew::stopProcessing(), Stopping Processor.");
    ls_atomic_setptr(&m_pFinishedQueue, NULL);
}


int WorkCrew::putFinishedItem(ls_lfnodei_t *item)
{
    int ret;
    ls_lfqueue_t *q = ls_atomic_fetch_add(&m_pFinishedQueue, 0);
    ret = ls_lfqueue_put(q, item);
    if (m_pNotifier)
    {
        LS_DBG_H("WorkCrew::putFinishedItem(), Notifying Notifier.");
        m_pNotifier->notify();
    }
    return ret;
}


int WorkCrew::addJob(ls_lfnodei_t *item)
{
    int ret;
    DPRINTF("ADDJOB new job from thread %ld %s\n",
            pthread_self(), pStatus());
    if (ls_atomic_fetch_add(&m_stateFutex, 0) >= STOPPING)
    {
        return LS_FAIL;
    }
#ifdef LS_WORKCREW_LF
    ret =  ls_lfqueue_put(m_pJobQueue, item);
#else
    ret = m_pJobQueue->append(&item, 1);
#endif
    if (ls_atomic_fetch_add(&m_idleWorkers, 0) >= ls_atomic_fetch_add(&m_minIdle, 0)
        || ls_mutex_trylock(&m_addWorker) != 0 )
    {
        return ret;
    }
    int i = 0;
    while(ls_atomic_fetch_or(&m_stateFutex, 0) == RUNNING && size() < ls_atomic_fetch_add(&m_maxWorkers, 0))
    {
        int32_t idleWorkers = ls_atomic_fetch_add(&m_idleWorkers, 0);
        if (ls_atomic_fetch_add(&idleWorkers, 0) >= ls_atomic_fetch_add(&m_minIdle, 0))
            break;
        DPRINTF("ADDJOB GROW %s\n", pStatus());
        addWorker();
        if (i++ > ls_atomic_fetch_add(&m_maxIdle, 0))
            break;
    }
    ls_mutex_unlock(&m_addWorker);
    return ret;
}


void WorkCrew::workerDied(CrewWorker *pWorker)
{
    ls_atomic_fetch_add(&m_iRunningWorkers, -1);
    ls_mutex_lock(&m_crewLock);
    int32_t slot = pWorker->getSlot();
    if (slot == -1)
        return;
    if (slot < 0 || slot >= m_crew.size() || !m_crew[slot])
    {
        printf("Race condition!  slot: %d, size: %ld, m_crew[%d]: %p\n",
               slot, m_crew.size(), slot, m_crew[slot]);
        assert(0);
    }
    assert(slot >= 0 && slot < m_crew.size() && m_crew[slot]);
    m_emptySlots.push_back(slot);
    m_crew[slot] = NULL;
    DPRINTF("WKRDIED UL slot %d %s\n", slot, pStatus());

    for (int i = 0; i < m_cleanUp.size(); i++)
    {
        CleanUp * cp = m_cleanUp[i];
        if (cp->routine)
        {
            cp->routine(cp->arg);
        }
    }

    if (ls_atomic_casint(&m_stateFutex, STOPPING, STOPPING))
    {
        // we are still in running state
        // already pushed our slot, so if we were last
        // worker running, size() is now 0 and noMoreWorkers is true
        if ( isAllWorkerDead() )
        {
            //LS_TH_BENIGN(&m_stateFutex, "ok set");
            ls_atomic_setint(&m_stateFutex, STOPPED);
            ls_futex_wakeall_priv(&m_stateFutex);
            //LS_TH_FLUSH();
        }
    }
    ls_mutex_unlock(&m_crewLock);
}


int32_t WorkCrew::adjustIdle(int32_t min, int32_t max)
{
    DPRINTF("ADJIDLE PRE MIN:%d MAX:%d %s\n", min, max, pStatus());
    if (min < 0 || max > m_maxWorkers || min > max)
    {
        return LS_FAIL;
    }

    ls_atomic_setint(&m_minIdle, min);
    ls_atomic_setint(&m_maxIdle, max);
    return LS_OK;
}


int32_t WorkCrew::maxWorkers(int32_t num)
{
    DPRINTF("MAXWRKRS PRE %d %s\n", num, pStatus());
    if (num < 0 || (num > 0 && num < m_minIdle))
    {
        return LS_FAIL;
    }

    int32_t oldMaxWorkers = m_maxWorkers;

    ls_atomic_setint(&m_maxWorkers, num);

    return oldMaxWorkers;
}


void WorkCrew::stopAllWorkers(int discard)
{
    DPRINTF("STPALL stopAllWorkers %s\n", pStatus());
    ls_mutex_lock(&m_crewLock);
    for (int32_t i = 0; i < m_crew.size(); i++)
    {
        if (m_crew[i])
        {
            if (discard)
                m_crew[i]->setSlot(-1);
            m_crew[i]->requestStop();
        }
    }
    ls_atomic_setint(&m_minIdle, 0);
    ls_atomic_setint(&m_maxIdle, 0);
    ls_mutex_unlock(&m_crewLock);
}


void *WorkCrew::workerRoutine(CrewWorker * pWorker)
{
    int32_t tidles;
    // sanity check from prior code - optional?
    if (!m_pProcess) {
        DPRINTF("GAPJ WRKBAD slot %d can't run %s\n", pWorker->getSlot(), pStatus());
        return NULL;
    }

    if (m_nice)
    {
        tidles = nice(m_nice);
        DPRINTF("WRKRTN slot %d drop prioirity by %d to %d\n",
                pWorker->getSlot(), m_nice, tidles);
    }

    while (pWorker->isWorking())
    {
        DPRINTF("WRKRTN slot %d running %s\n", pWorker->getSlot(), pStatus());

        ls_lfnodei_t * job = getJob(true);
        if (!job)
        {
            DPRINTF("WRKRTN poll job failed slot %d %s\n", pWorker->getSlot(), pStatus());
            int32_t idle = ls_atomic_fetch_add(&m_idleWorkers, 1);
            DPRINTF("WRKRTN IDLE slot %d %s\n", pWorker->getSlot(), pStatus());
            // timed get
            job = getJob();
            idle = ls_atomic_sub_fetch(&m_idleWorkers, 1);
            assert(idle >= 0);
            DPRINTF("WRKRTN NOT IDLE slot %d %s\n", pWorker->getSlot(), pStatus());
            DPRINTF("WRKRTN %s job slot %d %s\n", (job ? "GOT" : "NO"), pWorker->getSlot(), pStatus());
        }
        if (job)
        {
            DPRINTF("WRKRTN processing job slot %d %s\n", pWorker->getSlot(), pStatus());
            m_pProcess(job);
            if (ls_atomic_fetch_add(&m_pFinishedQueue, 0))
                putFinishedItem(job);

            DPRINTF("WRKRTN DONE processing job slot %d ret %p %s\n", pWorker->getSlot(), ret, pStatus());
        }

        //LS_TH_BENIGN(&m_idleWorkers, "ok read");
        //LS_TH_BENIGN(&m_maxWorkers, "ok read");
        //LS_TH_BENIGN(&m_maxIdle, "ok read");
        tidles = idles();
        int32_t tmaxWorkers = maxWorkers();
        int32_t tmaxIdle = maxIdle();
        //LS_TH_FLUSH();
        if (tidles >= tmaxIdle || size() > tmaxWorkers)
        {
            DPRINTF("WRKRTN SHRINK slot %d %s\n", pWorker->getSlot(), pStatus());
            break;
        }
    }
    DPRINTF("WRKRTN stopping slot %d %s\n", pWorker->getSlot(), pStatus());
    return NULL;
}


void WorkCrew::pushCleanup(void (*routine)(void *), void * arg)
{
    CleanUp * cp = new CleanUp(routine, arg);
    m_cleanUp.push_back(cp);
}
