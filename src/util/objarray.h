/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
#ifndef OBJARRAY_H
#define OBJARRAY_H

#include <lsr/ls_objarray.h>

#include <string.h>

class ObjArray : private ls_objarray_t
{
private:
    ObjArray(const ObjArray &rhs);
    void operator=(const ObjArray &rhs);
public:
    ObjArray(int objSize)           {   ls_objarray_init(this, objSize); }
    ~ObjArray()                     {   release();          }

    void    init(int objSize)       {   ls_objarray_init(this, objSize); }
    void    release()               {   ls_objarray_release(this); }
    void    clear()                 {   sizenow = 0; }

    int     capacity() const        {   return sizemax;     }
    int     size() const            {   return sizenow;     }
    int     getSize() const         {   return sizenow;     }

    int     getObjSize() const      {   return objsize;     }
    void   *getArray()              {   return parray;      }
    const void *getArray() const    {   return parray;      }
    void   *getObj(int index) const {   return ls_objarray_getobj(this, index);}
    void   *getNew()
    {
        if (sizenow >= sizemax)
        {
            if (setCapacity(sizemax ? (sizemax << 1) : 4) == -1)
                return NULL;
        }
        return (void *)((char *)parray + (sizenow++ * objsize));
    }

    void    setSize(int size)       {   ls_objarray_setsize(this, size); }
    void    pop()
    {
        if (sizenow > 0)
            --sizenow;
    }

    int setCapacity(int numObj)
    {   return ls_objarray_setcapacity(this, numObj); }

    int alloc(int numObj)
    {   return setCapacity(numObj); }

    int guarantee(int numObj)
    {   return ls_objarray_guarantee(this, numObj);   }
};


class ObjArrayXpool : private ls_objarray_t
{
private:
    ObjArrayXpool(const ObjArrayXpool &rhs);
    void operator=(const ObjArrayXpool &rhs);
public:
    ObjArrayXpool(int objSize)          {   ls_objarray_init(this, objSize); }
    ~ObjArrayXpool() {};

    void    init(int objSize)           {   ls_objarray_init(this, objSize); }
    void    release(ls_xpool_t *pool)
    {   ls_objarray_release_xpool(this, pool); }
    void    clear()                     {   sizenow = 0; }

    int     capacity() const            {   return sizemax;     }
    int     size() const                {   return sizenow;     }
    int     getSize() const         {   return sizenow;     }
    int     getObjSize() const          {   return objsize;     }
    void   *getArray()                  {   return parray;      }
    const void *getArray() const        {   return parray;      }
    void   *getObj(int index) const     {   return ls_objarray_getobj(this, index);}
    void   *getNew()                    {   return ls_objarray_getnew(this); }

    void    setSize(int size)           {   ls_objarray_setsize(this, size); }

    int setCapacity(ls_xpool_t *pool, int numObj)
    {   return ls_objarray_setcapacity_xpool(this, pool, numObj); }

    int guarantee(ls_xpool_t *pool, int numObj)
    {   return ls_objarray_guarantee_xpool(this, pool, numObj);   }
};


template< class T >
class TObjArray : public ObjArray
{
private:
    TObjArray(const TObjArray &rhs);
    void operator=(const TObjArray &rhs);
public:
    TObjArray()
        : ObjArray(sizeof(T))
    {};
    ~TObjArray() {};

    typedef T* iterator;
    typedef const T* const_iterator;

    void    init()                      {   ObjArray::init(sizeof(T));   }
    T      *getArray()                  {   return (T *)ObjArray::getArray(); }
    const T*getArray() const            {   return (const T *)ObjArray::getArray(); }
    T      *getObj(int index) const     {   return (T *)ObjArray::getObj(index);  }
    T      *getNew()                    {   return (T *)ObjArray::getNew(); }
    T      *newObj()                    {   return getNew();    }

    T *begin()      {   return  getArray();    }
    T *end()        {   return (T *)getArray() + size();   }

    const T *begin() const     {   return  getArray();    }
    const T *end() const       {   return (const T *)getArray() + size();   }

    T *get(int index)
    {
        if (index >= 0 && index < size())
            return begin() + index;
        else
            return NULL;
    }

    const T *get(int index) const
    {
        if (index >= 0 && index < size())
            return begin() + index;
        else
            return NULL;
    }

    void deleteObj(T *iter)   {   iter->~T();                 }

    void releaseObjects()
    {
        T *b = begin();
        T *e = end();
        for (; b < e; ++b)
            deleteObj(b);
        clear();
    }

    void copy(const TObjArray &other)
    {
        setCapacity(other.capacity());
        setSize(other.size());
        memmove(getArray(), other.getArray(), size() * sizeof(T));
    }

    int guarantee(int numObj)
    {   return ObjArray::guarantee(numObj);   }


    T &operator[](size_t off)               {   return getArray()[off];   }
    const T &operator[](size_t off) const   {   return getArray()[off];   }
};


template< class T >
class TObjArrayXpool : public ObjArrayXpool
{
private:
    TObjArrayXpool(const TObjArrayXpool &rhs);
    void operator=(const TObjArrayXpool &rhs);
public:
    TObjArrayXpool()
        : ObjArrayXpool(sizeof(T))
    {};
    ~TObjArrayXpool() {};

    void    init()                      {   ObjArrayXpool::init(sizeof(T));   }
    T      *getArray()                  {   return (T *)ObjArrayXpool::getArray(); }
    const T*getArray() const            {   return (const T *)ObjArrayXpool::getArray(); }
    T      *getObj(int index) const     {   return (T *)ObjArrayXpool::getObj(index);  }
    T      *getNew()                    {   return (T *)ObjArrayXpool::getNew(); }
    T      *newObj()                    {   return getNew();    }

    T *begin()      {   return  getArray();    }
    T *end()        {   return (T *)getArray() + size();   }

    const T *begin() const     {   return  getArray();    }
    const T *end() const       {   return (const T *)getArray() + size();   }

    T *get(int index)
    {
        if (index >= 0 && index < size())
            return begin() + index;
        else
            return NULL;
    }

    const T *get(int index) const
    {
        if (index >= 0 && index < size())
            return begin() + index;
        else
            return NULL;
    }

    void copy(ObjArrayXpool &other, ls_xpool_t *pool)
    {
        setCapacity(pool, other.capacity());
        setSize(other.size());
        memmove(getArray(), other.getArray(), size() * sizeof(T));
    }

    int guarantee(ls_xpool_t *pool, int numObj)
    {   return ObjArrayXpool::guarantee(pool, numObj);   }

};

#endif //OBJARRAY_H
