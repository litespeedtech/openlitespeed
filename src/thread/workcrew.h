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
#ifndef WORKCREW_H
#define WORKCREW_H

#include <lsdef.h>
#include <thread/crewworker.h>
#include <util/objarray.h>
#include <util/gpointerlist.h>
#include <lsr/ls_lock.h>

//#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
//#define LS_WORKCREW_LF
//#else
class PThreadWorkQueue;
//#endif

//#define LS_WORKCREW_DEBUG
#ifdef LS_WORKCREW_DEBUG
#include <stdio.h>
#endif

/**
 * @file
 *
 * To use Work Crew:
 *
 * A class must have WorkCrew as a member.
 *      It must either contain or have access to a queue for storing finished items.
 *      It must implement a (static)processing function.
 *      Optionally inherit or have access to an Event Notifier.
 *
 * After WorkCrew construction, jobs may be added at any time.
 * Call startProcessing when ready to start processing.
 *
 * StopProcessing may be called at any time after construction.
 */


class EventNotifier;
typedef struct ls_lfnodei_s ls_lfnodei_t;
typedef struct ls_lfqueue_s ls_lfqueue_t;

/**
 * @typedef WorkCrewProcessFn
 * @brief The user must provide a processing function.
 * @details This function will be used when the worker receives
 * a job item from the job queue.
 *
 * @param[in] item - The job item to process.
 * @return NULL if successful, Not NULL if unsuccessful.  If unsuccessful,
 * the worker thread will stop, passing the return value back to the parent.
 */
typedef void *(*WorkCrewProcessFn)(ls_lfnodei_t *item);

class SlotList : private GPointerList
{
public:
    SlotList()
        : GPointerList()
    { assert(sizeof(long) <= sizeof(void *)); }
    virtual ~SlotList()     {}
    int push_back(long v)   {   return GPointerList::push_back((void *) v); }
    long pop_back()         {   return (long) GPointerList::pop_back(); }
    long operator[](size_t off)        {   return (long) GPointerList::operator[](off);   }
    long operator[](size_t off) const  {   return (long) GPointerList::operator[](off);   }
    ssize_t size() const    {   return GPointerList::size();    }
    bool empty() const      {   return GPointerList::empty();   }
    bool full() const       {   return GPointerList::full();    }
    void clear()            {   return GPointerList::clear();   }
};


class WorkCrew
{
    friend class CrewWorker; // allow access to workerDied, workerRoutine() - perhaps change those to public??
public:
    /** @WorkCrew
     * @brief The Work Crew Constructor. Initializes the members.
     * @details If provided, the Event Notifier will be notified when
     * a job is finished.
     *
     * @param[in] initWorkers - Initial requested members.
     * @param[in] finishedQueue - Queue for placing finished jobs.
     * @param[in] processor - Function for processing a job.
     * @param[in] en - An optional initialized event notifier.
     */
    WorkCrew(int initWorkers,
            WorkCrewProcessFn  processor,
            ls_lfqueue_t * finishedQueue,
            EventNotifier *en = NULL);

    WorkCrew(int32_t maxWorkers, WorkCrewProcessFn  processor,
             ls_lfqueue_t * finishedQueue, EventNotifier *en,
             int32_t minIdleWorkers, int32_t maxIdleWorkers);

    explicit WorkCrew(EventNotifier *en);

    /** @~WorkCrew
     * @brief The Work Crew Destructor.
     * @details This function will end any thread still running and delete the
     * job queue.  This function will \b not free anything left in the queue.
     */
    ~WorkCrew();

     enum
     {
        TO_START,
        RUNNING,
        STOPPING,
        STOPPED
     };

     int startJobProcessor(int numWorkers, ls_lfqueue_t *pFinishedQueue,
                           WorkCrewProcessFn processor);

    /** @startProcessing
     * @brief Starts the Workers to begin processing the jobs.
     *
     * @return 0 if successful, else -1 if not.
     *
     * @see WorkCrewProcessFn
     */
    int startProcessing();

    /** @stopProcessing
     * @brief Stops any worker threads and resets the finished queue.
     *
     * @return Void.
     */
    void stopProcessing(int discard = 0);

    /** @size
     * @brief Returns the current number of workers
     * @return current workers (total slots - empty slots)
     */
    int32_t size() const
    {    return ls_atomic_fetch_add((volatile int32_t *)&m_iRunningWorkers, 0);    }



    int32_t minIdle() const { return ls_atomic_fetch_add((volatile int32_t *)&m_minIdle, 0);        }
    int32_t maxIdle() const { return ls_atomic_fetch_add((volatile int32_t *)&m_maxIdle, 0);        }
    int32_t adjustIdle(int32_t min, int32_t max);
    int32_t idles() const   { return ls_atomic_fetch_add((volatile int32_t *)&m_idleWorkers, 0);    }

    int32_t maxWorkers() const { return ls_atomic_fetch_add((volatile int32_t *)&m_maxWorkers, 0); };
    int32_t maxWorkers(int32_t num);

    /** @addJob
     * @brief Adds a job to the job queue.
     * @details This function adds the item to the job queue for the workers
     * to work on.  The item will be passed in to the
     * \link #WorkCrewProcessFn processing function.\endlink
     *
     * @param[in] item - The job item to pass into the processing function.
     * @return 0 if successful, else -1 if unsuccessful.
     */
    int addJob(ls_lfnodei_t *item);


    /** @blockSig / blockSigs
     * @brief add to or set the signal blocking mask for new workers
     * @details NOTE only applies to workers created AFTER the
     * call completes
     *
     * @param[in] sigset - the sigset_t to block
     */
    void blockSig(int signum);
    void blockSigs(const sigset_t &block);

    void pushCleanup(void (*routine)(void *), void * arg);

    void dropPriorityBy(int n)  {   m_nice = n;     }

private:
    class CleanUp
    {
    public:
        void (*routine)(void *);
        void *arg;
        CleanUp( void (*routine)(void *), void *arg)
            : routine(routine), arg(arg)
        {
        }
    };
    void markNoCleanup();

#ifdef LS_WORKCREW_LF
    ls_lfqueue_t               *m_pJobQueue;
#else
    PThreadWorkQueue           *m_pJobQueue;
#endif
    WorkCrewProcessFn           m_pProcess;
    ls_lfqueue_t               *m_pFinishedQueue;
    EventNotifier              *m_pNotifier;
    TPointerList<CrewWorker>    m_crew;
    ls_mutex_t                  m_addWorker;
    ls_mutex_t                  m_crewLock;
    SlotList                    m_emptySlots;
    int                         m_stateFutex;
    int                         m_nice;
    int32_t                     m_minIdle;
    int32_t                     m_maxIdle;
    int32_t                     m_maxWorkers;
    int32_t                     m_idleWorkers;
    int32_t                     m_iRunningWorkers;
    sigset_t                    m_sigBlock;
    TPointerList<CleanUp>       m_cleanUp;


#ifdef LS_WORKCREW_DEBUG
    const char * pStatus() {
        __thread static char msg[256] = "";

#if defined(USE_THREADCHECK) || ( defined(__has_feature) && __has_feature(thread_sanitizer) )
#ifndef ENABLE_PSTATUS_TSAN
        return msg;
#endif
#endif

        ls_mutex_lock(&m_crewLock);
        long csize=m_crew.size(), empty=m_emptySlots.size(), sz=size(),
             idle=idles(),  minw=minIdle(), maxw=maxWorkers();
        int state=ls_atomic_fetch_add(&m_stateFutex, 0);
        ls_mutex_unlock(&m_crewLock);
        snprintf(msg, 255, "crew %ld empty %ld size %ld idle %ld state %d min %ld max %ld",
                csize, empty, sz, idle, state, minw, maxw);
        return msg;
    }
#endif

    // NOT USEDstatic void *workCrewWorkerRoutine( void * pArg); // for thread
    void * workerRoutine(CrewWorker * pWorker); // to do work

    /** @getJob
     * @internal This function attempts to get a job from the job queue.
     */
    ls_lfnodei_t *getJob(bool poll = false);

    /** @workerDied
     * @brief signals WorkCrew that a worker has died
     * @details Worker slot in m_crew should be NULL'd
     * to make sure no pointers to the old / dead worker remain
     *
     * @param[in] pWorker - the pointer to the worker
     */
    void workerDied(CrewWorker *pWorker);


    /** @putFinishedItem
     * @internal This function puts an item into the finished queue and
     * notifies the Event Notifier if there is one.
     */
    int putFinishedItem(ls_lfnodei_t *item);

    int32_t  getSlot();
    void releaseSlot(int32_t slot);

    void init();

    int addWorker();
    void stopAllWorkers(int discard);

    bool isAllWorkerDead() const { return  0 == size(); }

    LS_NO_COPY_ASSIGN(WorkCrew);
};




#endif //WORKCREW_H

