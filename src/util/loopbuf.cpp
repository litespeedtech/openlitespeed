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
#include <util/loopbuf.h>
#include <util/iovec.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

LoopBuf::LoopBuf(int size)
{
    memset( this, 0, sizeof( LoopBuf ) );
    reserve( size );
}
LoopBuf::~LoopBuf()
{
    deallocate();
}

int LoopBuf::allocate( int size )
{
    if ( (size == 0) && (m_iCapacity != size) )
    {
        deallocate();
    }
    else
    {
        size = ( size + LOOPBUFUNIT  ) / LOOPBUFUNIT * LOOPBUFUNIT;
        if ( size > m_iCapacity )
        {
            char * pBuf = (char *)malloc( size );
            if ( pBuf == NULL )
            {
                errno = ENOMEM;
                return -1;
            }
            else
            {
                int len = 0;
                if ( m_pBuf != NULL )
                {
                    len = moveTo( pBuf, size-1 );
                    free(m_pBuf);
                }
                m_pBuf = m_pHead = pBuf ;
                m_pEnd = m_pBuf + len;
                m_iCapacity = size;
                m_pBufEnd = m_pBuf + size;
            }
        }
    }
    return 0;
}

void LoopBuf::deallocate()
{
    if ( m_pBuf != NULL )
        free(m_pBuf);
    memset( this, 0, sizeof( LoopBuf ) );
}

int LoopBuf::append( const char * pBuf, int size )
{
    if (( pBuf == NULL )||( size < 0 ))
    {
        errno = EINVAL;
        return -1;
    }

    if ( size )
    {
        if ( guarantee(size) < 0 )
            return -1 ;
        int len = contiguous();
        if ( len > size )
            len = size;

        memmove( m_pEnd, pBuf, len );
        pBuf += len ;
        len = size - len;

        if ( len )
        {
            memmove( m_pBuf, pBuf, len );
        }
        used(size);
    }
    return size;
}

int LoopBuf::moveTo( char * pBuf, int size )
{
    if (( pBuf == NULL )||( size < 0 ))
    {
        errno = EINVAL;
        return -1;
    }
    int len = this->size();
    if ( len > size )
        len = size ;
    if ( len > 0 )
    {
        size = m_pBufEnd - m_pHead ;
        if ( size > len )
            size = len ;
        memmove( pBuf, m_pHead, size );
        pBuf += size;
        size = len - size;
        if ( size )
        {
            memmove( pBuf, m_pBuf, size );
        }
        pop_front( len );
    }
    return len;
}

int LoopBuf::pop_front( int size )
{
    if ( size < 0 )
    {
        errno = EINVAL;
        return -1;
    }
    if (size )
    {
        int len = this->size() ;
        if ( size >= len )
        {
            size = len ;
            m_pHead = m_pEnd = m_pBuf;
        }
        else
        {
            m_pHead = m_pBuf + ( m_pHead - m_pBuf + size ) % m_iCapacity ;
        }
    }
    return size;
}

int LoopBuf::pop_back( int size )
{
    if ( size < 0 )
    {
        errno = EINVAL;
        return -1;
    }
    if ( size )
    {
        int len = this->size() ;
        if ( size >= len )
        {
            size = len ;
            m_pHead = m_pEnd = m_pBuf;
        }
        else
        {
            m_pEnd -= size ;
            if ( m_pEnd < m_pBuf )
                m_pEnd += m_iCapacity ;
        }
    }
    return size;
}

void LoopBuf::used(int size)
{
    assert( size <= available() );
    m_pEnd = m_pBuf + ( m_pEnd - m_pBuf + size ) % m_iCapacity ;
}

int LoopBuf::contiguous() const
{
    if ( m_pHead > m_pEnd )
        return ( m_pHead - m_pEnd - 1 );
    else
        return (m_pHead == m_pBuf)?( m_pBufEnd - m_pEnd - 1): m_pBufEnd - m_pEnd;
}


void LoopBuf::iov_insert( IOVec &vect )
{
    int size = this->size();
    if ( size > 0 )
    {
        int len = m_pBufEnd - m_pHead;
        if ( size > len )
        {
            vect.push_front( m_pBuf, size - len );
            vect.push_front( m_pHead, len );
        }
        else
        {
            vect.push_front( m_pHead, size );
        }
    }
}

void LoopBuf::iov_append( IOVec &vect )
{
    int size = this->size();
    if ( size > 0 )
    {
        int len = m_pBufEnd - m_pHead;
        if ( size > len )
        {
            vect.append( m_pHead, len );
            vect.append( m_pBuf, size - len );
        }
        else
        {
            vect.append( m_pHead, size );
        }
    }
}

//#include <util/swap.h>

void LoopBuf::swap( LoopBuf& rhs )
{
    char temp[sizeof( LoopBuf ) ];
    memmove( temp, this, sizeof( LoopBuf ) );
    memmove( this, &rhs, sizeof( LoopBuf ) );
    memmove( &rhs, temp, sizeof( LoopBuf ) );
    
//    char * pTemp;
//    int tmp;
//    GSWAP( m_pBuf, rhs.m_pBuf, pTemp );
//    GSWAP( m_pHead, rhs.m_pHead, pTemp );
//    GSWAP( m_pEnd, rhs.m_pEnd, pTemp );
//    GSWAP( m_pBufEnd, rhs.m_pBufEnd, pTemp );
//    GSWAP( m_iCapacity, rhs.m_iCapacity, tmp );    
}

int LoopBuf::guarantee(int size)
{
    int avail = available();
    if ( size <= avail )
        return 0;
    return reserve(size + this->size() + 1);
}
