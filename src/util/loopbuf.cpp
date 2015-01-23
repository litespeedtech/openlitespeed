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
#include <util/loopbuf.h>
#include <util/iovec.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>


void LoopBuf::iov_insert( IOVec &vect ) const
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

void LoopBuf::iov_append( IOVec &vect ) const
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

void XLoopBuf::iov_insert( IOVec &vect ) const
{
    int size = this->size();
    if ( size > 0 )
    {
        int len = m_loopbuf.m_pBufEnd - m_loopbuf.m_pHead;
        if ( size > len )
        {
            vect.push_front( m_loopbuf.m_pBuf, size - len );
            vect.push_front( m_loopbuf.m_pHead, len );
        }
        else
        {
            vect.push_front( m_loopbuf.m_pHead, size );
        }
    }
}

void XLoopBuf::iov_append( IOVec& vect ) const
{
    int size = this->size();
    if ( size > 0 )
    {
        int len = m_loopbuf.m_pBufEnd - m_loopbuf.m_pHead;
        if ( size > len )
        {
            vect.append( m_loopbuf.m_pHead, len );
            vect.append( m_loopbuf.m_pBuf, size - len );
        }
        else
        {
            vect.append( m_loopbuf.m_pHead, size );
        }
    }
}





