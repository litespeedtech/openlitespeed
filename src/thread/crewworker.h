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
#ifndef CREWWORKER_H
#define CREWWORKER_H

#include <thread/worker.h>
#include <assert.h>
#include <util/gpointerlist.h>

class WorkCrew;

class CrewWorker : public Worker
{
public:
    CrewWorker(WorkCrew * wc, int32_t slot, workFn work = NULL)
        : m_wc(wc)
        , m_slot(slot)
    {
    }

    ~CrewWorker()
    {}

    int32_t getSlot() const     {   return m_slot;  }
    void setSlot(int slot)      {   m_slot = slot;  }

    const WorkCrew * getWorkCrew() const { return m_wc; }
    
    int start()
    {
        attrSetDetachState(PTHREAD_CREATE_DETACHED);
        return Worker::start(this); 
    }        

protected:
    virtual void thr_cleanup();
    virtual void * thr_main(void * arg);

private:
    WorkCrew                 *m_wc;
    int32_t                   m_slot;
    
    LS_NO_COPY_ASSIGN(CrewWorker);
};

#endif //CREWWORKER_H

