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
#ifndef LOOPBUF_H
#define LOOPBUF_H

#include <lsr/lsr_loopbuf.h>

#include <string.h>
#include <stdlib.h>

#define LOOPBUFUNIT 64

class IOVec;

class LoopBuf : private lsr_loopbuf_t
{
private:
    LoopBuf( const LoopBuf &rhs );
    void operator=( const LoopBuf &rhs );
public:
    explicit LoopBuf( int size = LOOPBUFUNIT )
    {   lsr_loopbuf( this, size );  }
    
    //NOTICE: Any buf allocated by xpool must be destroyed by xDestroy.
    explicit LoopBuf( lsr_xpool_t *pool, int size = LOOPBUFUNIT )
    {   lsr_loopbuf_x( this, size, pool );  }
    
    ~LoopBuf()
    {   lsr_loopbuf_d( this );  }

    // return the size of free memory block, could be smaller than available()
    //   as it will start use the free space at the beginning once 
    //   reach the end. 
    char   *begin() const                               {   return m_pHead; }
    char   *end()   const                               {   return m_pEnd;  }
    int     capacity() const                            {   return m_iCapacity; }
    bool    empty() const                               {   return ( m_pHead == m_pEnd );   }
    bool    full() const                                {   return  size() == m_iCapacity - 1;  }

    void    clear()                                     {   m_pHead = m_pEnd = m_pBuf;  }
    void    swap( LoopBuf& rhs )                        {   lsr_loopbuf_swap( this, &rhs ); }
    int     contiguous()                                {   return lsr_loopbuf_contiguous( this );  }
    void    used( int size )                            {   return lsr_loopbuf_used( this, size );  }
    
    int     reserve( int size )                         {   return lsr_loopbuf_reserve( this, size );   }
    int     xReserve( int size, lsr_xpool_t *pool )     {   return lsr_loopbuf_xreserve( this, size, pool );}
    int     guarantee( int size )                       {   return lsr_loopbuf_guarantee( this, size ); }
    int     xGuarantee( int size, lsr_xpool_t *pool )   {   return lsr_loopbuf_xguarantee( this, size, pool );  }
    void    straight()                                  {   lsr_loopbuf_straight( this );   }
    void    xStraight( lsr_xpool_t *pool )              {   lsr_loopbuf_xstraight( this, pool );    }
    int     moveTo( char *pBuf, int size )              {   return lsr_loopbuf_move_to( this, pBuf, size ); }
    int     pop_front( int size )                       {   return lsr_loopbuf_pop_front( this, size ); }
    int     pop_back( int size )                        {   return lsr_loopbuf_pop_back( this, size );  }
    int     append( const char *pBuf, int size )        {   return lsr_loopbuf_append( this, pBuf, size );  }
    
    int xAppend( const char *pBuf, int size, lsr_xpool_t *pool )
    {
        return lsr_loopbuf_xappend( this, pBuf, size, pool );
    }
    
    // NOTICE: no boundary check for maximum performance, should make sure 
    //         buffer has available space before calling this function.
    void append( char ch ) 
    {
        *m_pEnd++ = ch;
        if ( m_pEnd == m_pBufEnd )
            m_pEnd = m_pBuf;
    }
    
    int size() const
    { 
        register int ret = m_pEnd - m_pHead;
        if ( ret >= 0 )
            return ret;
        return ret + m_iCapacity; 
    }
    
    int available() const
    {   
        register int ret = m_pHead - m_pEnd - 1;
        if ( ret >= 0 )
            return ret;
        return ret + m_iCapacity;
    }
    
    // return the size of memory block, could be smaller than size()
    //   as it will start use the free space at the beginning once 
    //   reach the end. 
    int blockSize() const
    {
        return ( m_pHead > m_pEnd ) ? m_pBufEnd - m_pHead
                                    : m_pEnd - m_pHead;
    }
    
    char *getPointer( int size ) const
    {   
        return m_pBuf + ( m_pHead - m_pBuf + size ) % m_iCapacity ;
    }

    int getOffset( const char *p ) const
    {
        return (( p < m_pBuf || p >= m_pBufEnd )? -1:
               ( p - m_pHead + m_iCapacity ) % m_iCapacity ) ;
    }
    
    char *inc( char * &pPos ) const
    {
        if ( ++pPos == m_pBufEnd  )
            pPos = m_pBuf ;
        return pPos ;
    }
    
    void update( int offset, const char *pBuf, int size )
    {   
        lsr_loopbuf_update( this, offset, pBuf, size );
    }
    
    char *search( int offset, const char *accept, int acceptLen )
    {   
        return lsr_loopbuf_search( this, offset, accept, acceptLen );   
    }
    
    void getIOvec( IOVec &vect ) const  {   iov_append( vect );    }
    
    void iov_insert( IOVec &vect ) const;
    void iov_append( IOVec &vect ) const;
    
    
    static void xDestroy( LoopBuf *p, lsr_xpool_t *pool )
    {
        lsr_loopbuf_xd( p, pool );
    }
};

class XLoopBuf : private lsr_xloopbuf_t
{
private:
    XLoopBuf( const XLoopBuf &rhs );
    void operator=( const XLoopBuf &rhs );
public:
    explicit XLoopBuf( lsr_xpool_t *pPool, int size = LOOPBUFUNIT )
    {   lsr_xloopbuf( this, size, pPool );  }
    
    ~XLoopBuf()
    {   lsr_xloopbuf_d( this );  }

    // return the size of free memory block, could be smaller than available()
    //   as it will start use the free space at the beginning once 
    //   reach the end. 
    
    char   *begin() const                           {   return m_loopbuf.m_pHead;   }
    char   *end()   const                           {   return m_loopbuf.m_pEnd;    }
    int     capacity() const                        {   return m_loopbuf.m_iCapacity;   }
    bool    empty() const                           {   return (m_loopbuf.m_pHead == m_loopbuf.m_pEnd); }
    bool    full() const                            {   return size() == m_loopbuf.m_iCapacity - 1; }
    
    void    clear()                                 {   lsr_xloopbuf_clear( this );    }
    void    swap( XLoopBuf& rhs )                   {   lsr_xloopbuf_swap( this, &rhs ); }
    int     contiguous()                            {   return lsr_xloopbuf_contiguous( this );  }
    void    used( int size )                        {   return lsr_xloopbuf_used( this, size );  }
    
    int     reserve( int size )                     {    return lsr_xloopbuf_reserve( this, size );    }
    int     guarantee( int size )                   {   return lsr_xloopbuf_guarantee( this, size ); }
    void    straight()                              {   lsr_xloopbuf_straight( this );   }
    int     moveTo( char *pBuf, int size )          {   return lsr_xloopbuf_move_to( this, pBuf, size ); }
    int     pop_front( int size )                   {   return lsr_xloopbuf_pop_front( this, size ); }
    int     pop_back( int size )                    {   return lsr_xloopbuf_pop_back( this, size );  }
    int     append( const char *pBuf, int size )    {   return lsr_xloopbuf_append( this, pBuf, size );    }
    
    // NOTICE: no boundary check for maximum performance, should make sure 
    //         buffer has available space before calling this function.
    void    append( char ch )                       {   lsr_xloopbuf_append_unsafe( this, ch ); }
    
    int size() const
    { 
        register int ret = m_loopbuf.m_pEnd - m_loopbuf.m_pHead;
        if ( ret >= 0 )
            return ret;
        return ret + m_loopbuf.m_iCapacity; 
    }
    
    int available() const
    {   
        register int ret = m_loopbuf.m_pHead - m_loopbuf.m_pEnd - 1;
        if ( ret >= 0 )
            return ret;
        return ret + m_loopbuf.m_iCapacity;
    }
    
    // return the size of memory block, could be smaller than size()
    //   as it will start use the free space at the beginning once 
    //   reach the end. 
    int blockSize() const
    {
        return ( m_loopbuf.m_pHead > m_loopbuf.m_pEnd ) ? m_loopbuf.m_pBufEnd - m_loopbuf.m_pHead
                                    : m_loopbuf.m_pEnd - m_loopbuf.m_pHead;
    }
    
    char *getPointer(int size) const
    {   
        return m_loopbuf.m_pBuf + ( m_loopbuf.m_pHead - m_loopbuf.m_pBuf + size ) % m_loopbuf.m_iCapacity;
    }
    
    int getOffset( const char *p ) const
    {
        return (( p < m_loopbuf.m_pBuf || p >= m_loopbuf.m_pBufEnd )? -1:
               ( p - m_loopbuf.m_pHead + m_loopbuf.m_iCapacity ) % m_loopbuf.m_iCapacity ) ;
    }
    
    char *inc( char * &pPos ) const
    {
        if ( ++pPos == m_loopbuf.m_pBufEnd  )
            pPos = m_loopbuf.m_pBuf ;
        return pPos ;
    }
    
    void update( int offset, const char *pBuf, int size )
    {   
        lsr_xloopbuf_update( this, offset, pBuf, size );
    }
    
    char *search( int offset, const char *accept, int acceptLen )
    {   
        return lsr_xloopbuf_search( this, offset, accept, acceptLen );   
    }
    
    void getIOvec( IOVec &vect ) const      {   iov_append( vect );    }
    
    void iov_insert( IOVec &vect ) const;
    void iov_append( IOVec &vect ) const;
};
#endif


