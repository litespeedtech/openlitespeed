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

#ifndef MTNOTIFIER_H
#define MTNOTIFIER_H

#include <lsdef.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <thread/pthreadmutex.h>
#include <thread/pthreadcond.h>



class MtNotifier
{
public:
    MtNotifier()
        : m_waiters(0)
    {}



    ~MtNotifier()
    {
        assert(m_waiters == 0);
    }

    void lock()
    {
        m_mutex.lock();
    }

    int wait()
    {
        ++m_waiters;
        m_cond.wait(m_mutex.get());
        return --m_waiters;
    }

    void unlock()
    {
        m_mutex.unlock();
    }

    int fullwait()
    {
        int remain_waiters;
        m_mutex.lock();
        ++m_waiters;
        m_cond.wait(m_mutex.get());
        remain_waiters = --m_waiters;
        m_mutex.unlock();
        return remain_waiters;
    }

    int notify()
    {
        int waiters;
        m_mutex.lock();
        if ((waiters = m_waiters) > 0)
            m_cond.signal();
        m_mutex.unlock();
        return waiters;
    }

    int32_t getWaiters() const  {   return m_waiters;   }

    void notifyEx()
    {
        m_cond.signal();
    }

    void broadcastEx()
    {
        m_cond.broadcast();
    }

    void broadcastTillNoWaiting()
    {
        while (true)
        {
            {
                LockMutex lock(m_mutex);
                if (m_waiters == 0)
                    break;
            }
            m_cond.broadcast();
            ::sched_yield();
        }
    }

private:
    PThreadMutex    m_mutex;
    PThreadCond     m_cond;
    int32_t         m_waiters;

    LS_NO_COPY_ASSIGN(MtNotifier);

};


#endif // MTNOTIFIER_H
