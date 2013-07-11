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
#ifndef LOOPBUF_H
#define LOOPBUF_H


#include <string.h>

#define LOOPBUFUNIT 64

class IOVec;

class LoopBuf
{
    char  * m_pBuf;
    char  * m_pBufEnd;
    char  * m_pHead;
    char  * m_pEnd;
    int     m_iCapacity;

    int     allocate( int size );
    void    deallocate();

    LoopBuf(const LoopBuf & rhs );
    LoopBuf& operator=( const LoopBuf &rhs );
    
public:
    explicit LoopBuf(int size = LOOPBUFUNIT);
    ~LoopBuf();
    int available() const
    {   return ( m_pHead > m_pEnd ) ? m_pHead - m_pEnd - 1
                                    : m_pHead + m_iCapacity - m_pEnd - 1;
    }

    int contiguous() const;

    int blockSize() const
    {
        return ( m_pHead > m_pEnd ) ? m_pBufEnd - m_pHead
                                    : m_pEnd - m_pHead;
    }

    char * begin()      {   return m_pHead;    }
    char * end()        {   return m_pEnd;     }

    const char * begin() const  {   return m_pHead;  }
    const char * end() const    {   return m_pEnd;   }
    
    char * getPointer(int size) const
    {   return m_pBuf + ( m_pHead - m_pBuf + size ) % m_iCapacity ; }
    char * getPointer(int size)
    {   return m_pBuf + ( m_pHead - m_pBuf + size ) % m_iCapacity ; }

    int getOffset( const char * p ) const
    {
        return (( p < m_pBuf || p >= m_pBufEnd )? -1:
               ( p - m_pHead + m_iCapacity ) % m_iCapacity ) ;
    }

    void used( int size );

    void clear()
    {   m_pHead = m_pEnd = m_pBuf;    }

    int capacity() const         {   return m_iCapacity; }
    int size() const
    { return ( m_pEnd - m_pHead + m_iCapacity ) % m_iCapacity ;}

    int reserve( int size )
    {    return allocate(size) ;    }

    int append( const char * pBuf, int size );

    int append( const char * pBuf )
    {   return append( pBuf, strlen( pBuf ) ); }

    int guarantee(int size);

    char* inc(char * &pPos) const
    {
        if ( ++pPos == m_pBufEnd  )
            pPos = m_pBuf ;
        return pPos ;
    }

    const char* inc(const char * &pPos) const
        {
            if ( ++pPos == m_pBufEnd )
                pPos = m_pBuf ;
            return pPos ;
        }

    bool empty() const           {   return ( m_pHead == m_pEnd );  }
    bool full() const
        { return  (m_pEnd - m_pHead + 1) % m_iCapacity  == 0 ; }

    int moveTo( char * pBuf, int size );
    int pop_front( int size );
    int pop_back( int size );

    void getIOvec( IOVec &vect )   {   iov_append( vect );    }
    
    void iov_insert( IOVec &vect );
    void iov_append( IOVec &vect );

    void swap( LoopBuf& rhs );
};

#endif
