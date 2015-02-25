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

#include <thread/workcrew.h>

#include <new>

int WorkCrew::increaseTo(int numMembers)
{
    int i;
    m_crew.guarantee(NULL, numMembers);
    for (i = m_crew.getSize(); i < numMembers; ++i)
    {
        Worker *worker = new(m_crew.getNew()) Worker(startCrewWork);
        if (worker->run(this))
            return LS_FAIL;
#ifdef LS_WORKCREW_DEBUG
        printf("Worker %lx, %lu started\n", (unsigned long)worker,
               worker->getId());
#endif
    }
    return 0;
}

int WorkCrew::decreaseTo(int numMembers)
{
    void *retVal;
    int i;
    int iSize = m_crew.getSize();

    for (i = numMembers; i < iSize; ++i)
        m_crew.getObj(i)->setStop();

    for (i = numMembers; i < iSize; ++i)
        m_crew.getObj(i)->join(&retVal);

    m_crew.setSize(numMembers);
#ifdef LS_WORKCREW_DEBUG
    printf("%d workers left\n", m_crew.getSize());
#endif
    return 0;
}

void *WorkCrew::doWork()
{
    void *ret;
    ls_lfnodei_t *item = getJob();
    if (!item)
        return NULL;
    if ((ret = m_pProcess(item)) != NULL)
        return ret;
    putFinishedItem(item);
    return NULL;
}

int WorkCrew::resize(int numMembers)
{
    if (numMembers < 0)
        return LS_FAIL;
    else if (numMembers < LS_WORKCREW_MINWORKER)
        numMembers = LS_WORKCREW_MINWORKER;
    else if (numMembers > LS_WORKCREW_MAXWORKER)
        numMembers = LS_WORKCREW_MAXWORKER;
    if (numMembers == m_crew.getSize())
        return 0;
    return (numMembers > m_crew.getSize() ? increaseTo(numMembers) :
            decreaseTo(numMembers));
}





