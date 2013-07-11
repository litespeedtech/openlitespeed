/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
#ifndef GPOINTERLIST_H
#define GPOINTERLIST_H



  
#include <stddef.h>
#include <string.h>

typedef int (*gpl_for_each_fn)( void *);

class GPointerList
{
    void ** m_pStore;
    void ** m_pStoreEnd;
    void ** m_pEnd;
    
    int allocate( int capacity );
    GPointerList( const GPointerList & rhs );
    void operator=( const GPointerList& rhs );
public: 
    typedef void ** iterator;
    typedef void *const* const_iterator;
    GPointerList();
    explicit GPointerList( size_t initSize );
    ~GPointerList();
    size_t size() const     {   return m_pEnd - m_pStore;   }
    bool empty() const      {   return m_pEnd == m_pStore;  }
    bool full() const       {   return m_pEnd == m_pStoreEnd;   }
    size_t capacity() const {   return m_pStoreEnd - m_pStore;  }
    void clear()            {   m_pEnd = m_pStore;          }
    iterator begin()        {   return m_pStore;            }
    iterator end()          {   return m_pEnd;              }
    const_iterator begin() const    {   return m_pStore;    }
    const_iterator end() const      {   return m_pEnd;      }
    void * back() const             {   return *(m_pEnd - 1);   }
    int reserve( size_t sz )        {   return allocate( sz );  }
    int resize( size_t sz );
    int grow( size_t sz )   {   return allocate( sz + capacity() ); }
    
    iterator erase( iterator iter )
    {   *iter = *(--m_pEnd );  return iter; }
    void safe_push_back( void *pPointer )   {   *m_pEnd++ = pPointer;   }
    void safe_push_back( void **pPointer, int n );
    void safe_pop_back( void **pPointer, int n );
    int  push_back( void * pPointer );
    int  push_back( const void * pPointer )
    {   return push_back( (void *)pPointer );    }
    int  push_back( const GPointerList& list );
    void *pop_back()                        {   return *(--m_pEnd);         }
    int pop_front( void **pPointer, int n )
    {   if (n > size() ) 
        {
            n = size();
        }
        memmove( pPointer, m_pStore, n * sizeof( void *) );
        if ( n >= size() )
            clear();
        else
        {
            memmove( m_pStore, m_pStore + n, sizeof( void *) * ( m_pEnd - m_pStore - n ) );
            m_pEnd -= n;
        }
        return n;
    }
    void *& operator[]( size_t off )        {   return *(m_pStore + off);   }
    void *& operator[]( size_t off ) const  {   return *(m_pStore + off);   }
    void sort( int(*compare)(const void *, const void *) );
    void swap( GPointerList & rhs );
    
    const_iterator lower_bound( const void * pKey,
             int(*compare)(const void *, const void *) ) const ;
    const_iterator bfind( const void * pKey,
             int(*compare)(const void *, const void *) ) const;
    int for_each( iterator beg, iterator end, gpl_for_each_fn fn );

};

template< typename T >
class TPointerList : public GPointerList
{
    void operator=( const TPointerList& rhs );
    TPointerList( const TPointerList & rhs );
public:
    typedef T ** iterator;
    typedef T *const* const_iterator;
    TPointerList() {}
    
    explicit TPointerList( size_t initSize )
        : GPointerList( initSize )
        {}
    iterator begin()        {   return (iterator)GPointerList::begin();   }
    iterator end()          {   return (iterator)GPointerList::end();    }
    const_iterator begin() const
    {   return (const_iterator)GPointerList::begin();    }
    const_iterator end() const
    {   return (const_iterator)GPointerList::end();      }

    T *pop_back()
    {   return (T*)GPointerList::pop_back();   }
    T *&operator[]( size_t off ) 
    {   return (T*&)GPointerList::operator[]( off ); }

    T *&operator[](size_t off ) const
    {   return (T*&)GPointerList::operator[]( off ); }
    
    T * back() const
    {   return (T*)GPointerList::back();    }

    iterator erase( iterator iter )
    {   return (iterator)GPointerList::erase( (GPointerList::iterator)iter );    }
    
    void release_objects()
    {   for( iterator iter = begin(); iter < end(); ++iter )
            if ( *iter ) delete *iter;
        GPointerList::clear();
    }

    int copy( const TPointerList& rhs )
    {
        release_objects();
        for( const_iterator iter = rhs.begin(); iter < rhs.end(); ++iter )
            push_back( new T( **iter ) );
        return size();
    }

    int append( const TPointerList& rhs )
    {
        for( const_iterator iter = rhs.begin(); iter < rhs.end(); ++iter )
            if ( find( *iter ) == end() )
                push_back( new T( **iter ) );
        return size();
    }

    const_iterator find( const T* obj ) const
    {   for( const_iterator iter = begin(); iter < end(); ++iter )
            if ( **iter == *obj )
                return iter;
        return end();
    }
        
    
    iterator lower_bound( const void * pKey,
             int(*compare)(const void *, const void *) ) const
    {   return (iterator)GPointerList::lower_bound( pKey, compare );   }
    
    iterator bfind( const void * pKey,
             int(*compare)(const void *, const void *) ) const
    {   return (iterator)GPointerList::bfind( pKey, compare );  }

    int for_each( iterator beg, iterator end, gpl_for_each_fn fn )
    {   return GPointerList::for_each( (void**)beg, (void**)end, fn );    }
};


#endif
