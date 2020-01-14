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
#include <thread/crewworker.h>
#include <thread/workcrew.h>
#include <log4cxx/logger.h>

void CrewWorker::thr_cleanup()
{
#ifdef LS_WORKCREW_DEBUG
    LS_DBG_H("CrewWorker from slot %d (* %p, id %ld) exiting\n",
            m_slot, this, getHandle());
#endif
    assert(m_wc);
    if (m_slot != -1)
        m_wc->workerDied(this);
    requestStop();
    delete this;
}

void * CrewWorker::thr_main(void * arg)
{
    assert(m_wc);
    return m_wc->workerRoutine(this);
}

