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
#include "autobuf.h"

#include <errno.h>
#include <stdlib.h>

AutoBuf::AutoBuf( int size )
{
    memset( this, 0, sizeof( AutoBuf ) );
    allocate( size );
}
AutoBuf::~AutoBuf()
{
    if ( m_pBuf )
    {
        free( m_pBuf );
        m_pBuf = NULL;
    }
}

int AutoBuf::allocate( int size )
{
    char * pBuf;
    if ( m_pBufEnd - m_pBuf == size )
        return size;
    pBuf = (char *)realloc( m_pBuf, size );
    if (( pBuf != NULL )||( size == 0 ))
    {
        m_pEnd = pBuf + ( m_pEnd - m_pBuf );
        m_pBuf = pBuf;
        m_pBufEnd = pBuf + size;
        if ( m_pEnd > m_pBufEnd )
            m_pEnd = m_pBufEnd;
        return 0;
    }
    return -1;
}

void AutoBuf::deallocate()
{
    allocate( 0 );
}

int AutoBuf::grow( int size )
{
    size = (( size + 511 ) >> 9 ) << 9  ;
    return reserve( capacity() + size );
}

int AutoBuf::append( const char * pBuf, int size )
{
    if (( pBuf == NULL )||( size < 0 ))
    {
        errno = EINVAL;
        return -1;
    }
    if ( size == 0 )
        return 0;
    if ( size > available() )
    {
        if ( grow( size - available() ) == -1 )
            return -1;
    }
    memmove( end(), pBuf, size );
    used( size );
    return size;
}

int AutoBuf::moveTo( char * pBuf, int sz )
{
    if ( !empty() )
    {
        int copysize = ( size() < sz )? size() : sz ;
        memmove( pBuf, m_pBuf, copysize );
        pop_front( copysize );
        return copysize;
    }
    return 0;
}

int AutoBuf::pop_front( int sz )
{
    if ( sz >= size() )
    {
        m_pEnd = m_pBuf;
    }
    else
    {
        memmove( m_pBuf, m_pBuf + sz, size() - sz );
        m_pEnd -= sz;
    }
    return sz;
}

int AutoBuf::pop_end( int sz )
{
    if ( sz > size() )
        m_pEnd = m_pBuf;
    else
        m_pEnd -= sz;
    return sz;
}
#define SWAP( a, b, t )  do{ t = a; a = b; b = t;  }while(0)
void AutoBuf::swap( AutoBuf& rhs )
{
    char * pTemp;
    SWAP( m_pBuf, rhs.m_pBuf, pTemp );

    SWAP( m_pBufEnd, rhs.m_pBufEnd, pTemp );
    SWAP( m_pEnd, rhs.m_pEnd, pTemp );
}
