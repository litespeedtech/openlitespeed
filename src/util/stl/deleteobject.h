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
#ifndef _STL_DELETE_OBJECT_
#define _STL_DELETE_OBJECT_

#include <stdlib.h>

namespace boost
{
template<class T> class shared_ptr;
};

struct DeleteObject
{
    template< typename T> void operator()(const T* ptr) const
    {   delete ptr; }
    template< typename T>
    void operator()( boost::shared_ptr<T> ) const
    {}
};

struct FreeMem
{
    template< typename T> void operator()( const T* ptr) const
    {   ::free( (void*)ptr );  }
};

struct DeletePair
{
    template< typename T>
    void operator()( T& pPair ) const
    {   delete pPair.first; delete pPair.second;   }
};

struct DeletePairFirst
{
    template< typename T>
    void operator()( T& pPair ) const
    {   delete pPair.first;   }
};

struct DeletePairSecond
{
    template< typename T>
    void operator()( T& pPair ) const
    {   delete pPair.second;   }
};

#endif

