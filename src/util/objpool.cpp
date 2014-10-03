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
#include <util/objpool.h>

GObjPool::GObjPool( int chunkSize )
    : m_chunkSize( chunkSize )
    , m_poolSize( 0 )
{
}

int GObjPool::allocate( int size )
{
    if ( (int)m_freeList.capacity() < m_poolSize + size )
        if ( m_freeList.reserve( m_poolSize + size ) )
            return -1;
    int i = 0;
    try
    {
        for( ; i < size; ++i )
        {
            void * pObj = newObj();
            if ( pObj )
            {
                m_freeList.unsafe_push_back( pObj );
                ++m_poolSize;
            }
            else
            {
                return -1;
            }
        }
    }
    catch (...)
    {
        return -1;
    }
    return 0;
}


