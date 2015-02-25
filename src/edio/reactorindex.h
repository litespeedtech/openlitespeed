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
#ifndef REACTORINDEX_H
#define REACTORINDEX_H


#include <stddef.h>

#define MAX_FDINDEX 100000
class EventReactor;
class ReactorIndex
{
public:
    EventReactor **m_pIndexes;
    unsigned int    m_capacity;
    unsigned int    m_iUsed;

    int deallocate();

public:
    ReactorIndex();
    ~ReactorIndex();

    unsigned int getUsed() const        {   return m_iUsed;         }
    unsigned int getCapacity() const    {   return m_capacity;      }

    int allocate(int capacity);

    EventReactor *get(int fd) const
    {   return ((unsigned)fd <= m_iUsed) ? m_pIndexes[fd] : NULL;  }

    int set(int fd, EventReactor *pReactor)
    {
        if ((unsigned)fd >= m_capacity)
        {
            if ((unsigned)fd > MAX_FDINDEX)
                return -1;
            int new_cap = m_capacity * 2;
            if (new_cap <= fd)
                new_cap = fd + 1;
            if (allocate(new_cap) == -1)
                return -1;
        }
        if ((unsigned)fd > m_iUsed)
            m_iUsed = fd;
        m_pIndexes[fd] = pReactor;
        return 0;
    }
    void timerExec();
    int verify(int fd, EventReactor *pReactor)
    {
        if (((unsigned)fd <= m_iUsed) && (m_pIndexes[fd] == pReactor))
            return 0;
        return -1;
    }
};

#endif
