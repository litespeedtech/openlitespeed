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

#include <lsr/lsr_map.h>
#include <lsr/lsr_internal.h>
#include <lsr/lsr_xpool.h>

//#define GMAP_DEBUG

class GMap : private lsr_map_t
{
private:
    GMap( const GMap &rhs );
    void operator=( const GMap &rhs );
public:
    
    class GMapNode : private lsr_mapnode_t
    {
        friend class GMap;
        
        //Forbidden functions
        GMapNode& operator++();
        GMapNode operator++( int );
        GMapNode& operator--();
        GMapNode operator--( int );
    public:
        const void *getKey() const      {   return m_pKey;  }
        void       *getValue() const    {   return m_pValue;}
    };
    
    typedef GMapNode *iterator;
    typedef const GMapNode *const_iterator;
    
    typedef lsr_map_val_comp val_comp;
    typedef lsr_map_for_each_fn for_each_fn;
    typedef lsr_map_for_each2_fn for_each_fn2;
    
public:
    GMap( val_comp vc, lsr_xpool_t *pool = NULL )
    {   lsr_map( this, vc, pool );    }
    
    ~GMap()
    {   clear();    }
    
    void        clear()                 {   lsr_map_clear( this );  }
    void        swap( GMap & rhs )      {   lsr_map_swap( this, &rhs ); }
    bool        empty() const           {   return m_size == 0; }
    size_t      size() const            {   return m_size;  }
    val_comp    get_val_comp() const    {   return m_vc;    }
    
    iterator        begin()             {   return (iterator)lsr_map_begin( this ); }
    iterator        end()               {   return (iterator)lsr_map_end( this );   }
    const_iterator  begin() const       {   return ((GMap *)this)->begin(); }
    const_iterator  end() const         {   return ((GMap *)this)->end();   }
    
    iterator find( const void *pKey )
    {  
        return (iterator)(*m_find)( this, pKey );
    }
    
    int insert( const void *pKey, void *pValue )
    {   
        return (*m_insert)( this, pKey, pValue );
    }

    void *update( const void *pKey, void *pValue, iterator node = NULL )
    {   
        return (*m_update)( this, pKey, pValue, node );
    }
    
    void *deleteNode( iterator node )
    {   
        return lsr_map_delete_node( this, node );
    }
    
    iterator next( iterator iter )
    {   
        return (iterator)lsr_map_next( this, iter );
    }
    
    const_iterator next( const_iterator iter ) const
    {   
        return ((GMap *)this)->next( iter );
    }
    
    int for_each( iterator beg, iterator end, for_each_fn fun )
    {   
        return lsr_map_for_each( this, beg, end, fun );
    }
    
    int for_each2( iterator beg, iterator end, for_each_fn2 fun, void *pUData )
    {   
        return lsr_map_for_each2( this, beg, end, fun, pUData );
    }
    
#ifdef LSR_MAP_DEBUG
    static void printTree( GMap *pThis )    
    {   lsr_map_printTree( pThis );  }
#endif
};



template< class T >
class TMap
    : public GMap
{
private:
    TMap( const TMap &rhs );
    void operator=( const TMap &rhs );
public:
    class iterator
    {
        GMap::iterator m_iter;
    public:
        iterator()
        {}

        iterator( GMap::iterator iter ) : m_iter( iter )
        {}
//         iterator( GMap::const_iterator iter )
//             : m_iter( (GMap::iterator)iter )
//         {}

        iterator( const iterator& rhs ) : m_iter( rhs.m_iter )
        {}
        
        const void *first() const
        {  return  m_iter->getKey();   }

        T second() const
        {   return (T)( m_iter->getValue() );   }

        operator GMap::iterator ()
        {   return m_iter;  }

    };
    typedef iterator const_iterator;

    TMap( GMap::val_comp cf )
        : GMap( cf )
        {};
    ~TMap() {};
    
    iterator insert( const void *pKey, const T& val )
    {   return GMap::insert( pKey, (void *)val );  }

    iterator update( const void *pKey, const T& val, TMap::iterator node = NULL )
    {   return GMap::update( pKey, (void *)val, (GMap::iterator)node );  }

    iterator find( const void *pKey )
    {   return GMap::find( pKey );   }

    const_iterator find( const void *pKey ) const
    {   return GMap::find( pKey );   }
    
    iterator begin()
    {   return GMap::begin();        }
    
    static int deleteObj( TMap::iterator iter/* GMap::iterator iter */ )
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


