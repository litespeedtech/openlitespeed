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
#ifndef TLINKLIST_H
#define TLINKLIST_H


#include <util/linkedobj.h>

template< typename T>
class TLinkList
{
    LinkedObj m_first;
public:
    TLinkList() {};
    ~TLinkList() {};
    TLinkList( const TLinkList& rhs )
    {
        T * p = rhs.begin();
        LinkedObj * pNext = &m_first;
        T * pObj;
        while( p )
        {
            pObj = new T( *p );
            if ( !pObj )
                break;
            pNext->addNext( pObj );
            pNext = pObj;
            p = (T *)p->next();
        }
    }
    LinkedObj * head()                  {   return &m_first;    }
    const LinkedObj * head() const      {   return &m_first;    }
    T * begin() const          {   return (T*)(m_first.next()); }
    LinkedObj * tail()
    {
        LinkedObj * p = m_first.next();
        while( p && p->next() )
            p = p->next();
        return p;
    }

    LinkedObj * prev( LinkedObj * obj )
    {
        LinkedObj * p = m_first.next();
        if ( p == obj )
            p = NULL;
        while( p && p->next() != obj )
            p = p->next();
        return p;
    }

    LinkedObj * remove_back()
    {
        LinkedObj * p = &m_first;
        while(( p->next() )&&(p->next()->next()))
            p = p->next();
        p = p->removeNext();
        return p;
    }
    void release_objects()
    {
        LinkedObj * pDel;
        while( (pDel = m_first.removeNext()) != NULL )
        {
            delete (T *)pDel;
        }
    }
    void clear()
    {
        m_first.setNext( NULL );
    }

    void append( T * p )
    {
        LinkedObj * pNext = tail();
        if ( !pNext )
            pNext = &m_first;
        pNext->addNext( p );
    }

    void insert_front( T * p )
    {
        m_first.addNext( p );
    }
    
    int insert_front( LinkedObj * pHead, LinkedObj * pTail )
    {
        if ( !pHead || !pTail || pTail->next() )
            return -1;
        LinkedObj * pNext = m_first.next();
        m_first.setNext( pHead );
        pTail->setNext( pNext );
        return 0;
    }
    void swap(TLinkList& rhs )
    {
        LinkedObj tmp;
        tmp = m_first;
        m_first = rhs.m_first;
        rhs.m_first = tmp;
    }
};


#endif
