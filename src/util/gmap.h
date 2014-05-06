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

#ifndef GMAP_H
#define GMAP_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//#define GMAP_DEBUG

class GMap
{
public:
    class GMapNode
    {
        friend class GMap;
        enum { red, black } m_color;
        const void *m_pKey;
        void *m_pValue;
        GMapNode *m_left,
                *m_right,
                *m_parent;
        
        //Forbidden functions
        GMapNode& operator++();
        GMapNode operator++( int );
        GMapNode& operator--();
        GMapNode operator--( int );
        
    public:
        const void* getKey() const  {   return m_pKey;  }
        void * getValue() const      {   return m_pValue; }
        
        const void* first() const  {   return m_pKey;  }
        void * second() const      {   return m_pValue; }

    };
    
    typedef GMapNode *iterator;
    typedef const GMapNode* const_iterator;
    
    typedef int (*val_comp) ( const void * pVal1, const void * pVal2 );
    typedef int (*for_each_fn)( iterator iter );
    typedef int (*for_each_fn2)( iterator iter, void *pUData );
    
    
private:
    
    typedef int (*node_insert)( GMap * pThis, const void * pKey, void * pValue );
    typedef void *(*node_update)( GMap * pThis, const void * pKey, void * pValue, iterator node );
    typedef iterator (*node_find)  ( GMap * pThis, const void * pKey );
    
    size_t m_size;
    GMapNode *m_root;
    val_comp m_vc;
    node_insert m_insert;
    node_update m_update;
    node_find   m_find;
    
    static iterator getGrandparent( iterator node );
    static iterator getUncle( iterator node );
    static void rotateLeft( GMap *pThis, iterator node );
    static void rotateRight( GMap *pThis, iterator node );
    static void fixTree( GMap *pThis, iterator node );
    
    static int insertNode( GMap * pThis, const void * pKey, void * pValue );
    static int insertIntoTree( iterator pCurrent, iterator pNew, val_comp vc );
    static void *updateNode( GMap * pThis, const void * pKey, void * pValue, iterator node );
    static iterator findNode( GMap * pThis, const void * pKey );
    static iterator removeNodeFromTree( GMap * pThis, iterator node );
    static iterator removeEndNode( GMap * pThis, iterator node, char nullify );
    static void releaseNodes( iterator node );
public:
    GMap( val_comp vc );
    ~GMap();
    void clear();
    
    void swap( GMap & rhs );
    void *deleteNode( iterator node );
        
    bool empty() const              {   return m_size == 0; }
    size_t size() const             {   return m_size;      }
    val_comp get_val_comp() const   {   return m_vc;    }
    
    iterator find( const void * pKey )
    {   return (*m_find)( this, pKey );  }
    
    int insert( const void * pKey, void * pValue )
    {   return (*m_insert)( this, pKey, pValue );   }

    void *update( const void * pKey, void * pValue, iterator node = NULL )
    {   return (*m_update)( this, pKey, pValue, node );   }
    
    iterator begin();
    iterator end();
    const_iterator begin() const    {   return ((GMap*)this)->begin(); }
    const_iterator end() const      {   return ((GMap*)this)->end();   }
    
    iterator next( iterator iter );
    const_iterator next( const_iterator iter ) const
    {   return ((GMap *)this)->next((iterator)iter); }
    int for_each( iterator beg, iterator end, for_each_fn fun );
    int for_each2( iterator beg, iterator end, for_each_fn2 fun, void * pUData );
    
#ifdef GMAP_DEBUG
    static void printTree( GMap *pThis )    {   print( pThis->m_root, 0 );  }
    static void print( iterator node, int layer );
#endif
};

template< class T >
class TMap
    : public GMap
{
public:
    class iterator
    {
        GMap::iterator m_iter;
    public:
        iterator()
        {}

        iterator( GMap::iterator iter ) : m_iter( iter )
        {}
        iterator( GMap::const_iterator iter )
            : m_iter( (GMap::iterator)iter )
        {}

        iterator( const iterator& rhs ) : m_iter( rhs.m_iter )
        {}
        
        const void * first() const
        {  return  m_iter->first();   }

        T second() const
        {   return (T)( m_iter->second() );   }

        operator GMap::iterator ()
        {   return m_iter;  }

    };
    typedef iterator const_iterator;

    TMap( GMap::val_comp cf )
        : GMap( cf )
        {};
    ~TMap() {};
    
    iterator insert( const void * pKey, const T& val )
    {   return GMap::insert( pKey, (void *)val );  }

    iterator update( const void * pKey, const T& val, TMap::iterator node = NULL )
    {   return GMap::update( pKey, (void *)val, (GMap::iterator)node );  }

    iterator find( const void * pKey )
    {   return GMap::find( pKey );   }

    const_iterator find( const void * pKey ) const
    {   return GMap::find( pKey );   }
    
    iterator begin()
    {   return GMap::begin();        }
    
    static int deleteObj( GMap::iterator iter )
    {
        delete (T)( iter->second() );
        return 0;
    }

    void release_objects()
    {
        GMap::for_each( begin(), end(), deleteObj );
        GMap::clear();
    }

};

#endif // GMAP_H


