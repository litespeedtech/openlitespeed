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
#ifndef OBJARRAY_H
#define OBJARRAY_H

#include <lsr/lsr_objarray.h>

#include <string.h>

class ObjArray : private lsr_objarray_t
{
private:
    ObjArray( const ObjArray &rhs );
    void operator=( const ObjArray &rhs );
public:
    ObjArray( int objSize )                     {   lsr_objarray_init( this, objSize ); }
    ~ObjArray() {};
    
    void    init( int objSize )                 {   lsr_objarray_init( this, objSize ); }
    void    release( lsr_xpool_t *pool )        {   lsr_objarray_release( this, pool ); }
    void    clear()                             {   m_size = 0; }
    
    int     getCapacity() const                 {   return m_capacity;  }
    int     getSize() const                     {   return m_size;  }
    int     getObjSize() const                  {   return m_objSize;   }
    void   *getArray()                          {   return m_pArray;}
    void   *getObj( int index ) const           {   return lsr_objarray_getobj( this, index );}
    void   *getNew()                            {   return lsr_objarray_getnew( this ); }
    
    void    setSize( int size )                 {   lsr_objarray_setsize( this, size ); }
    
    void setCapacity( lsr_xpool_t *pool, int numObj ) 
    {   lsr_objarray_setcapacity( this, pool, numObj ); }
    
    void guarantee( lsr_xpool_t *pool, int numObj )     
    {   lsr_objarray_guarantee( this, pool, numObj );   }
};
    
template< class T >
class TObjArray : public ObjArray
{   
private:
    TObjArray( const TObjArray &rhs );
    void operator=( const TObjArray& rhs );
public:
    TObjArray()
        :ObjArray( sizeof( T ) )
    {};
    ~TObjArray(){};
    
    void    init()                      {   ObjArray::init( sizeof( T ));   }
    T      *getArray()                  {   return (T *)ObjArray::getArray(); }
    T      *getObj( int index ) const   {   return (T *)ObjArray::getObj( index );  }
    T      *getNew()                    {   return (T *)ObjArray::getNew(); }
};


#endif //OBJARRAY_H
