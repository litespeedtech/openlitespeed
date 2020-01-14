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
#ifndef WORKER_H
#define WORKER_H

#include <lsdef.h>
#include <thread/thread.h>
#include <lsr/ls_atomic.h>
#include <lsr/ls_lock.h>

#include <assert.h>
#include <lsr/ls_threadcheck.h>

typedef void *(*workFn)(void *arg);

class Worker;

class Worker : public Thread
{

public:
    enum
    {
        TO_START,
        RUNNING,
        TO_STOP,
        STOPPED
    };

    Worker(workFn work = NULL)
        : m_pWorkFn(work)
        , m_isWorking(TO_START)
    {
    }

    ~Worker()
    {   assert(m_isWorking != RUNNING);  }

    void setWorkFn(workFn work) {   m_pWorkFn = work; }

    int requestStop()
    {   return ls_atomic_casint(&m_isWorking, RUNNING, TO_STOP);  }

    int  isWorking() const
    {
        return ls_atomic_casint((volatile int *)&m_isWorking, RUNNING, RUNNING);
    }

    int start(void *arg)
    {
        if (ls_atomic_casint(&m_isWorking, TO_START, RUNNING))
            return Thread::start(arg);
        return LS_FAIL;
    }

    int run(void *arg)
    {
        if (!m_pWorkFn)
            return LS_FAIL;

        int ret = start(arg);
        assert(ret == 0);
        return ret;
    }

    virtual int join(void **pRetVal)
    {
        int ret = Thread::join(pRetVal);
        return ret;
    }

protected:
    void * thr_main(void * arg)
    {
        void *ret = NULL;
        ls_atomic_casint(&m_isWorking, TO_START, RUNNING);

        while (isWorking())
        {
            if ((ret = m_pWorkFn(this))) {
                // DESIGN CHANGE - ignore return code
                // and continue running
                //break;
            }
        }
        ls_atomic_setint(&m_isWorking, TO_START);
        return ret;
    }

private:
    workFn          m_pWorkFn;
    int             m_isWorking;

    LS_NO_COPY_ASSIGN(Worker);
};

#endif //WORKER_H

