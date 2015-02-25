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
#ifndef GHASH_H
#define GHASH_H



#include <stddef.h>

typedef unsigned long hash_key_t;


class GHash
{
public:

    class HashElem
    {
        friend class GHash;
        HashElem     *m_pNext;
        const void   *m_pKey;
        void         *m_pData;
        hash_key_t    m_hkey;

        void setKey(const void *pKey)    {m_pKey = pKey;  }

        //Forbidden functions
        HashElem &operator++();
        HashElem operator++(int);
        HashElem &operator--();
        HashElem operator--(int);


    public:
        const void *getKey() const  {   return m_pKey;  }
        void *getData() const      {   return m_pData; }
        hash_key_t getHKey() const  {   return m_hkey;  }
        HashElem *getNext() const  {   return m_pNext; }

        void setData(void *p)    {   m_pData = p;    }

        const void *first() const  {   return m_pKey;  }
        void *second() const       {   return m_pData; }
    };

    /*
    class iterator
    {
        HashElem * m_pElem;

        iterator& operator++();
        iterator operator++( int );
        iterator& operator--();
        iterator operator--( int );
    public:
        iterator( HashElem * pElem )
            : m_pElem( pElem )
            {}
        iterator( const iterator& rhs )
            : m_pElem( rhs.m_pElem )
            {}
        int compare( const iterator& rhs ) const
        {   return m_pElem - rhs.m_pElem;   }
        HashElem * operator->() {   return
    };
    typedef const iterator const_iterator;
    */
    typedef HashElem *iterator;
    typedef const HashElem *const_iterator;

    typedef hash_key_t (*hash_fn)(const void *);
    typedef int (*val_comp)(const void *pVal1, const void *pVal2);
    typedef int (*for_each_fn)(iterator iter);
    typedef int (*for_each_fn2)(iterator iter, void *pUData);

    static hash_key_t hash_string(const void *__s);
    static int  comp_string(const void *pVal1, const void *pVal2);

    static hash_key_t i_hash_string(const void *__s);
    static int  i_comp_string(const void *pVal1, const void *pVal2);

    static int  cmp_ipv6(const void *pVal1, const void *pVal2);
    static hash_key_t hf_ipv6(const void *pKey);



    //strcmp

private:

    typedef iterator(*hash_insert)(GHash *pThis, const void *pKey,
                                   void *pValue);
    typedef iterator(*hash_update)(GHash *pThis, const void *pKey,
                                   void *pValue);
    typedef iterator(*hash_find)(GHash *pThis, const void *pKey);

    HashElem  **m_table;
    HashElem  **m_tableEnd;
    size_t      m_capacity;
    size_t      m_size;
    int         m_full_factor;
    hash_fn     m_hf;
    val_comp    m_vc;
    int         m_grow_factor;
    hash_insert m_insert;
    hash_update m_update;
    hash_find   m_find;

    void        rehash();
    iterator    find2(const void *pKey, hash_key_t key);
    iterator    insert2(const void *pKey, void *pValue, hash_key_t key);
    static iterator insert_num(GHash *pThis, const void *pKey, void *pValue);
    static iterator update_num(GHash *pThis, const void *pKey, void *pValue);
    static iterator find_num(GHash *pThis, const void *pKey);
    static iterator insert_p(GHash *pThis, const void *pKey, void *pValue);
    static iterator update_p(GHash *pThis, const void *pKey, void *pValue);
    static iterator find_p(GHash *pThis, const void *pKey);

public:

    GHash(size_t init_size, hash_fn hf, val_comp vc);
    ~GHash();
    void clear();
    void erase(iterator iter);

    void swap(GHash &rhs);

    iterator find(const void *pKey)
    {   return (*m_find)(this, pKey);  }
    const_iterator find(const void *pKey) const
    {   return ((GHash *)this)->find(pKey);   }

    iterator insert(const void *pKey, void *pValue)
    {   return (*m_insert)(this, pKey, pValue);   }

    iterator update(const void *pKey, void *pValue)
    {   return (*m_update)(this, pKey, pValue);   }

    hash_fn get_hash_fn() const     {   return m_hf;    }
    val_comp get_val_comp() const   {   return m_vc;    }

    void set_full_factor(int f)   {   if (f > 0)    m_full_factor = f;  }
    void set_grow_factor(int f)   {   if (f > 0)    m_grow_factor = f;  }

    bool empty() const              {   return m_size == 0; }
    size_t size() const             {   return m_size;      }
    size_t capacity() const         {   return m_capacity;  }
    iterator begin();
    iterator end()                  {   return NULL;    }
    const_iterator begin() const    {   return ((GHash *)this)->begin(); }
    const_iterator end() const      {   return ((GHash *)this)->end();   }

    iterator next(iterator iter);
    const_iterator next(const_iterator iter) const
    {   return ((GHash *)this)->next((iterator)iter); }
    int for_each(iterator beg, iterator end, for_each_fn fun);
    int for_each2(iterator beg, iterator end, for_each_fn2 fun, void *pUData);
};

//bool operator!=( const GHash::iterator& lhs, const GHash::iterator& rhs )
//{   return lhs.compare( rhs ) != 0 ;    }
//bool operator==( const GHash::iterator& lhs, const GHash::iterator& rhs )
//{   return lhs.compare( rhs ) == 0 ;    }

template< class T >
class THash
    : public GHash
{
public:
    class iterator
    {
        GHash::iterator m_iter;
    public:
        iterator()
        {}

        iterator(GHash::iterator iter) : m_iter(iter)
        {}
        iterator(GHash::const_iterator iter)
            : m_iter((GHash::iterator)iter)
        {}

        iterator(const iterator &rhs) : m_iter(rhs.m_iter)
        {}

        const void *first() const
        {  return  m_iter->first();   }

        T second() const
        {   return (T)(m_iter->second());   }

        operator GHash::iterator()
        {   return m_iter;  }

    };
    typedef iterator const_iterator;

    THash(int initsize, GHash::hash_fn hf, GHash::val_comp cf)
        : GHash(initsize, hf, cf)
    {};
    ~THash() {};

    iterator insert(const void *pKey, const T &val)
    {   return GHash::insert(pKey, (void *)val);  }

    iterator update(const void *pKey, const T &val)
    {   return GHash::update(pKey, (void *)val);  }

    iterator find(const void *pKey)
    {   return GHash::find(pKey);   }

    const_iterator find(const void *pKey) const
    {   return GHash::find(pKey);   }

    iterator begin()
    {   return GHash::begin();        }

    static int deleteObj(GHash::iterator iter)
    {
        delete(T)(iter->second());
        return 0;
    }

    void release_objects()
    {
        GHash::for_each(begin(), end(), deleteObj);
        GHash::clear();
    }

};


#endif
