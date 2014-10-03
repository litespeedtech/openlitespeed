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
#ifndef GHASH_H
#define GHASH_H

#include <lsr/lsr_hash.h>
#include <lsr/lsr_internal.h>
#include <lsr/lsr_xpool.h>
  
#include <stddef.h>

typedef unsigned long hash_key_t;

class GHash : private lsr_hash_t
{
private:
    GHash( const GHash &rhs );
    void operator=( const GHash &rhs );
public:
    class HashElem : private lsr_hashelem_t
    {
        friend class GHash;
        void setKey( const void * pKey )    
        {m_pKey = pKey;  }
        
        //Forbidden functions
        HashElem& operator++();
        HashElem operator++( int );
        HashElem& operator--();
        HashElem operator--( int );
            
    public:
        const void *getKey() const  {   return m_pKey;  }
        void       *getData() const {   return m_pData; }
        hash_key_t  getHKey() const {   return m_hkey;  }
        HashElem   *getNext() const {   return (HashElem *)m_pNext; }
        const void *first() const   {   return m_pKey;  }
        void       *second() const  {   return m_pData; }
    };
    
    typedef HashElem* iterator;
    typedef const HashElem* const_iterator;

    typedef hash_key_t (*hash_fn) ( const void * );
    typedef int   (*val_comp) ( const void * pVal1, const void * pVal2 );
    //typedef int (*for_each_fn)( iterator iter);
    //typedef int (*for_each_fn2)( iterator iter, void *pUData);
    typedef lsr_hash_for_each_fn for_each_fn;
    typedef lsr_hash_for_each2_fn for_each_fn2;
    
    static hash_key_t hash_string(const void * __s);
    static int  comp_string( const void * pVal1, const void * pVal2 );

    static hash_key_t i_hash_string(const void * __s);
    static int  i_comp_string( const void * pVal1, const void * pVal2 );

    static int  cmp_ipv6( const void * pVal1, const void * pVal2 );
    static hash_key_t hf_ipv6( const void * pKey );
    
    GHash( size_t init_size, hash_fn hf, val_comp vc, lsr_xpool_t *pool = NULL )
    {   lsr_hash( this, init_size, hf, vc, pool );    }
    
    ~GHash()
    {   
        if ( m_xpool )
            assert( !lsr_xpool_is_empty( m_xpool ) );
        lsr_hash_d( this ); 
    }
    
    void        clear()                     {   lsr_hash_clear( this ); }
    void        erase( iterator iter )      {   lsr_hash_erase( this, iter );   }
    void        swap( GHash & rhs )         {   lsr_hash_swap( this, &rhs );    }
    
    hash_fn     get_hash_fn() const         {   return m_hf;    }
    val_comp    get_val_comp() const        {   return m_vc;    }
    void        set_full_factor( int f )    {   if ( f > 0 )    m_full_factor = f;  }
    void        set_grow_factor( int f )    {   if ( f > 0 )    m_grow_factor = f;  }
    
    bool        empty() const               {   return m_size == 0; }
    size_t      size() const                {   return m_size;      }
    size_t      capacity() const            {   return m_capacity;  }
    
    iterator        begin()                 {   return (iterator)lsr_hash_begin( this );    }
    iterator        end()                   {   return NULL;    }
    const_iterator  begin() const           {   return ((GHash *)this)->begin(); }
    const_iterator  end() const             {   return ((GHash *)this)->end();   }
    
    iterator find( const void * pKey )
    {   
        return (iterator)(*m_find)( this, pKey );
    }
    
    const_iterator find( const void * pKey ) const
    {   
        return (const_iterator)((GHash *)this)->find( pKey );
    }

    iterator insert(const void * pKey, void * pValue)
    {   
        return (iterator)(*m_insert)( this, pKey, pValue );
    }

    iterator update(const void * pKey, void * pValue)
    {   
        return (iterator)(*m_update)( this, pKey, pValue );
    }
    
    iterator next( iterator iter )
    {   
        return (iterator)lsr_hash_next( this, iter );
    }
    
    const_iterator next( const_iterator iter ) const
    {   
        return ((GHash *)this)->next((iterator)iter);
    }
    
    int for_each( iterator beg, iterator end, for_each_fn fun )
    {   
        return lsr_hash_for_each( this, beg, end, fun );    
    }
    
    int for_each2( iterator beg, iterator end, for_each_fn2 fun, void * pUData )
    {   
        return lsr_hash_for_each2( this, beg, end, fun, pUData );   
    }

};

template< class T >
class THash
    : public GHash
{
private:
    THash( const THash &rhs );
    void operator=( const THash &rhs );
public:
    class iterator
    {
        GHash::iterator m_iter;
    public:
        iterator()
        {}

        iterator( GHash::iterator iter ) : m_iter( iter )
        {}
        iterator( GHash::const_iterator iter )
            : m_iter( (GHash::iterator)iter )
        {}

        iterator( const iterator& rhs ) : m_iter( rhs.m_iter )
        {}

        const void * first() const
        {  return  m_iter->first();   }

        T second() const
        {   return (T)( m_iter->second() );   }

        operator GHash::iterator ()
        {   return m_iter;  }

    };
    typedef iterator const_iterator;

    THash( int initsize, GHash::hash_fn hf, GHash::val_comp cf )
        : GHash( initsize, hf, cf )
        {};
    ~THash() {};

    iterator insert( const void * pKey, const T& val )
    {   return GHash::insert( pKey, (void *)val );  }

    iterator update( const void * pKey, const T& val )
    {   return GHash::update( pKey, (void *)val );  }

    iterator find( const void * pKey )
    {   return GHash::find( pKey );   }

    const_iterator find( const void * pKey ) const
    {   return GHash::find( pKey );   }
    
    iterator begin()
    {   return GHash::begin();        }
    
    static int deleteObj( const void *pKey, void *pData )
    {
        delete (T)(pData);
        return 0;
    }

    void release_objects()
    {
        GHash::for_each( begin(), end(), deleteObj );
        GHash::clear();
    }

};


#endif
