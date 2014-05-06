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
#include <util/autostr.h>
#include <util/pool.h>

#include <string.h>

AutoStr::AutoStr()
    : m_pStr( NULL )
{
}
AutoStr::~AutoStr()
{
    if ( m_pStr )
        g_pool.deallocate2( m_pStr );
}

AutoStr::AutoStr( const AutoStr& rhs )
    : m_pStr( NULL )
{
    if ( rhs.c_str() )
        m_pStr = g_pool.dupstr( rhs.c_str() );
}

char * AutoStr::resizeBuf( int size )
{
    char *p = (char *)g_pool.reallocate2( m_pStr, size );
    if ( p )
        m_pStr = p;
    return p;
}


AutoStr::AutoStr( const char * pStr )
{
    m_pStr = g_pool.dupstr( pStr );
}

int AutoStr::setStr( const char * pStr )
{
    int len = strlen( pStr );
    return setStr( pStr, len );
}

int AutoStr::setStr( const char * pStr, int len )
{
    if ( m_pStr )
        g_pool.deallocate2( m_pStr );
    m_pStr = g_pool.dupstr( pStr, len + 1 );
    *(m_pStr + len) = 0;
    return len;
}

AutoStr& AutoStr::operator=( const char * pStr )
{
    setStr( pStr );
    return *this;
}

AutoStr2::AutoStr2()
    : m_iStrLen( 0 )
{}

AutoStr2::AutoStr2( const AutoStr2 & rhs)
    : AutoStr()
    , m_iStrLen( 0 )
{
    if ( rhs.c_str() )
        setStr( rhs.c_str(), rhs.len() );
}

int AutoStr2::setStr( const char * pStr )
{   return setStr( pStr, strlen( pStr ) );   }

void AutoStr2::append( const char * str, const int len )
{
    char *p = resizeBuf(m_iStrLen + len + 1);
    if ( p )
    {
        memcpy(p + m_iStrLen, str, len );
        m_iStrLen += len;
        *(p + m_iStrLen) = 0;
    }
}


