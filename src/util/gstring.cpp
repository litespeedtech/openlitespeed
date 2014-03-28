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
#include <util/gstring.h>
#include <util/pool.h>
#include <assert.h>

static Pool* s_stringPool = &g_pool;

void GString::setStringPool( Pool * pPool )
{   s_stringPool = pPool;   }

GString::~GString()
{
    if ( m_pStr )
        s_stringPool->deallocate( m_pStr, m_iBufSize );
}

int GString::allocate( int size )
{
    int iBufSize = Pool::roundUp( size );
    if ( iBufSize > m_iBufSize )
    {
        char * pNewStr = (char *)s_stringPool->allocate( iBufSize );
        if ( pNewStr )
        {
            m_pStr = pNewStr;
            m_iBufSize = iBufSize;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

int GString::reallocate( int size )
{
    if ( m_iBufSize == 0 )
        return allocate( size );
    else
    {
        int iBufSize = Pool::roundUp( size + 1);
        if ( iBufSize > m_iBufSize )
        {
            char * pNewStr = (char *)s_stringPool->reallocate( m_pStr,
                    m_iBufSize, iBufSize );
            if ( pNewStr )
            {
                m_pStr = pNewStr;
                m_iBufSize = iBufSize;
            }
            else
            {
                return -1;
            }
        }
    }
    return 0;
}

void GString::deallocate()
{
    if ( m_pStr )
    {
        s_stringPool->deallocate( m_pStr, m_iBufSize );
        m_pStr = NULL;
        m_iBufSize = 0;
        m_iStrLen = 0;
    }
}

GString::GString( int bufLen )
    : m_pStr( NULL )
    , m_iStrLen( 0 )
    , m_iBufSize( 0 )
{
    if ( (int)bufLen > m_iBufSize )
    {
        allocate( bufLen );
    }
}


GString::GString( size_t bufLen, int val )
    : m_pStr( NULL )
    , m_iStrLen( 0 )
    , m_iBufSize( 0 )
{
    if ( (int)bufLen > m_iBufSize )
    {
        if ( !allocate( bufLen ) )
            memset( m_pStr, val, bufLen );
    }
}


GString::GString( const char * pStr )
    : m_pStr( NULL )
    , m_iStrLen( 0 )
    , m_iBufSize( 0 )
{
    if ( pStr )
    {
        int iStrLen = strlen( pStr );
        if ( !allocate( iStrLen + 1) )
        {
            if ( iStrLen )
                memmove( m_pStr, pStr, iStrLen + 1 );
            m_iStrLen = iStrLen;
        }
    }
}

GString::GString( const GString& rhs )
    : m_pStr( NULL )
    , m_iStrLen( 0 )
    , m_iBufSize( 0 )
{
    if ( rhs.m_iBufSize > 0 )
    {
        if ( !allocate( rhs.m_iBufSize ) )
        {
            memmove( m_pStr, rhs.m_pStr, rhs.m_iBufSize );
            m_iStrLen = rhs.m_iStrLen;
        }
    }
}

GString::GString( const char * pStr, int len )
    : m_pStr( NULL )
    , m_iStrLen( 0 )
    , m_iBufSize( 0 )
{
    assign( pStr, len );
}


GString& GString::operator=( const GString& rhs )
{
    if ( this != &rhs )
    {
        assign( rhs.m_pStr, rhs.m_iStrLen );
    }
    return *this;
}

GString & GString::assign( const char * pStr, int len )
{
    if ( pStr )
    {
        if ( m_pStr != pStr )
        {
            if ( len + 1 > m_iBufSize )
                if ( reallocate( len + 1 ) )
                    return *this;
            if ( len )
                memmove( m_pStr, pStr, len );
        }
        *( m_pStr + len ) = 0;
        m_iStrLen = len;
    }
    else
    {
        deallocate();
    }
    return *this;
}
/*
GString & GString::bassign( const char * pStr, int len )
{
    assert( pStr );
    if ( m_pStr != pStr )
    {
        if ( len > m_iBufSize )
            if ( reallocate( len ) )
                return *this;
        if ( len )
            memmove( m_pStr, pStr, len );
    }
    m_iStrLen = len;
    return *this;
}
*/

GString& GString::append( const char * pStr )
{
    if ( pStr )
    {
        int iLen = strlen( pStr );
        append( pStr, iLen );
    }
    return *this;
}
GString& GString::append( const char * pStr, int len )
{
    if (( pStr )&&( len > 0))
    {
        int iStrLen = m_iStrLen + len;
        if ( iStrLen + 1 > m_iBufSize )
        {
            char * pOldBuf = m_pStr;
            int oldBufSize = m_iBufSize;
            if ( allocate( iStrLen + 1 ) )
                return *this;
            ::memmove( m_pStr, pOldBuf, m_iStrLen );
            ::memmove( m_pStr + m_iStrLen, pStr, len );
            if ( pOldBuf )
                s_stringPool->deallocate( pOldBuf, oldBufSize );
        }
        else
            ::memmove( m_pStr + m_iStrLen, pStr, len );
        m_iStrLen = iStrLen;
        *( m_pStr + iStrLen ) = 0;
    }
    return *this;
}


