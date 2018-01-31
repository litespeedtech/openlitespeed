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
#ifndef LINKEDOBJ_H
#define LINKEDOBJ_H

#include <lsdef.h>

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <lsr/ls_swap.h>

class LinkedObj
{
    LinkedObj *m_pNext;

    LinkedObj(const LinkedObj &rhs);
public:
    LinkedObj() : m_pNext(NULL) {}
    explicit LinkedObj(LinkedObj *next)
        : m_pNext(next)
    {}
    void operator=(const LinkedObj &rhs)
    {
        m_pNext = rhs.m_pNext;
    }

    LinkedObj *next() const
    {   return m_pNext;     }

    void setNext(LinkedObj *pNext)
    {   m_pNext = pNext;        }

    void addNext(LinkedObj *pNext)
    {
        LinkedObj *pTemp = m_pNext;
        setNext(pNext);
        pNext->setNext(pTemp);
    }


    void addNext(LinkedObj *pFirst, LinkedObj * pLast)
    {
        assert(pFirst);
        assert(pLast);
        assert(pFirst != pLast);
        LinkedObj *pTemp = next();

        setNext(pFirst);
        pLast->setNext(pTemp);
    }

    LinkedObj *removeNext()
    {
        LinkedObj *pNext = m_pNext;
        if (pNext)
        {
            setNext(pNext->m_pNext);
            pNext->setNext(NULL);
        }
        return pNext;
    }

    void swap(LinkedObj &rhs)
    {
        LinkedObj *temp;
        GSWAP(m_pNext, rhs.m_pNext, temp);
    }
};


class DLinkedObj : public LinkedObj
{
    DLinkedObj *m_pPrev;

public:

    DLinkedObj(): m_pPrev(NULL) {}
    DLinkedObj(DLinkedObj *prev, DLinkedObj *next)
        : LinkedObj(next), m_pPrev(prev)
    {}

    DLinkedObj *prev() const
    {   return m_pPrev;     }

    void setPrev(DLinkedObj *p)
    {   m_pPrev = p;        }

    DLinkedObj *next() const
    {   return (DLinkedObj *)LinkedObj::next();     }

    void addNext(DLinkedObj *pNext)
    {
        assert(pNext);
        DLinkedObj *pTemp = next();
        LinkedObj::addNext(pNext);
        pNext->m_pPrev = this;
        if (pTemp)
            pTemp->m_pPrev = pNext;
    }

    void addNext(DLinkedObj *pFirst, DLinkedObj * pLast)
    {
        assert(pFirst);
        assert(pLast);
        assert(pFirst != pLast);
        DLinkedObj *pTemp = next();

        LinkedObj::addNext(pFirst, pLast);
        pFirst->setPrev(this);
        if (pTemp)
            pTemp->setPrev(pLast);
    }


    void addPrev(DLinkedObj *pFirst, DLinkedObj * pLast)
    {
        assert(pFirst && "NULL pFirst");
        assert(pLast && "NULL pLast");
        assert(pFirst != pLast);
        DLinkedObj *pTemp = prev();

        if (pTemp)
        {
            pTemp->addNext(pFirst, pLast);
            return;
        }

        // first in the chain (should not happen if DlinkQueue)
        pFirst->setPrev(NULL);
        pLast->setNext(this);
        setPrev(pLast);
    }


    DLinkedObj *removeNext()
    {
        DLinkedObj *pNext = next();
        if (pNext)
        {
            setNext(pNext->next());
            if (next())
                next()->m_pPrev = this;
            memset(pNext, 0, sizeof(DLinkedObj));
        }
        return pNext;
    }

    void addPrev(DLinkedObj *pPrev)
    {
        assert(pPrev);
        DLinkedObj *pTemp;
        pTemp = m_pPrev;
        m_pPrev = pPrev;
        pPrev->m_pPrev = pTemp;
        pPrev->setNext(this);
        if (pTemp)
            pTemp->setNext(pPrev);
    }

    DLinkedObj *removePrev()
    {
        DLinkedObj *pPrev = m_pPrev;
        if (m_pPrev)
        {
            m_pPrev = pPrev->m_pPrev;
            if (m_pPrev)
                m_pPrev->setNext(this);
            memset(pPrev, 0, sizeof(DLinkedObj));
        }
        return pPrev;
    }

    DLinkedObj *remove()
    {
        DLinkedObj *pNext = next();
        if (pNext)
            pNext->m_pPrev = m_pPrev;
        if (m_pPrev)
            m_pPrev->setNext(next());
        memset(this, 0, sizeof(DLinkedObj));
        return pNext;
    }

    void swap(DLinkedObj &rhs)
    {
        DLinkedObj *temp;
        LinkedObj::swap(rhs);
        GSWAP(m_pPrev, rhs.m_pPrev, temp);
    }


    LS_NO_COPY_ASSIGN(DLinkedObj);
};
#endif
