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
#ifndef AUTOBUF_H
#define AUTOBUF_H


#include <lsr/lsr_buf.h>

#include <string.h>

class AutoBuf : private lsr_buf_t
{
private:
    AutoBuf( const AutoBuf &rhs );
    void operator=( const AutoBuf &rhs );
public:
    explicit AutoBuf( int size = 1024 )
    {   lsr_buf( this, size );  }
    //NOTICE: Any buf allocated by xpool must be destroyed by xDestroy.
    explicit AutoBuf( lsr_xpool_t *pool, int size = 1024 )
    {   lsr_buf_x( this, size, pool );  }
    ~AutoBuf()
    {   lsr_buf_d( this );  }
    
    int     available() const                       {   return m_pBufEnd - m_pEnd; }

    char   *begin() const                           {   return m_pBuf; }
    char   *end() const                             {   return m_pEnd; }

    void    used( int size )                        {   m_pEnd += size;     }
    void    clear()                                 {   m_pEnd = m_pBuf;    }

    int     capacity() const                        {   return m_pBufEnd - m_pBuf; }
    int     size()   const                          {   return m_pEnd - m_pBuf;   }

    int     reserve( int size )                     {   return lsr_buf_reserve( this, size );    }
    int     xReserve( int size, lsr_xpool_t *pool ) {   return lsr_buf_xreserve( this, size, pool );    }
    void    resize( int size )                      {   lsr_buf_resize( this, size );    }
    int     grow( int size )                        {   return lsr_buf_grow( this, size );  }
    int     xGrow( int size, lsr_xpool_t *pool )    {   return lsr_buf_xgrow( this, size, pool );   }
    
    bool    empty()  const                          {   return ( m_pBuf == m_pEnd );  }
    bool    full() const                            {   return ( m_pEnd == m_pBufEnd ); }
    int     get_offset( const char * p ) const      {   return p - m_pBuf;    }
    char   *getp( int offset ) const                {   return m_pBuf + offset; }
    void    swap( AutoBuf& rhs )                    {   lsr_buf_swap( this, &rhs ); }
    
    int     pop_front_to( char * pBuf, int size )   {   return lsr_buf_pop_front_to( this, pBuf, size );}
    int     pop_front( int size )                   {   return lsr_buf_pop_front( this, size ); }
    int     pop_end( int size )                     {   return lsr_buf_pop_end( this, size );   }

    int     append( const char * pBuf, int size )   {   return lsr_buf_append2( this, pBuf, size ); }
    int     append( const char * pBuf )             {   return lsr_buf_append( this, pBuf ); }
    void    append_unsafe( char ch )                {   *m_pEnd++ = ch; }
    
    int append_unsafe( const char * pBuf, int size )
    {
        memmove( end(), pBuf, size );
        used( size );
        return size;
    }
    
    int xAppend( const char * pBuf, int size, lsr_xpool_t *pool ) 
    {   
        return lsr_buf_xappend2( this, pBuf, size, pool );
    }
    
    int xAppend( const char * pBuf, lsr_xpool_t *pool ) 
    {   
        return lsr_buf_xappend( this, pBuf, pool );
    }
    static void xDestroy( AutoBuf *p, lsr_xpool_t *pool )
    {
        lsr_buf_xd( p, pool );
    }
};

class XAutoBuf : private lsr_xbuf_t
{
private:
    XAutoBuf( const XAutoBuf &rhs );
    void operator=( const XAutoBuf &rhs );
public:
    explicit XAutoBuf( lsr_xpool_t * pPool, int size = 1024 )
    {   lsr_xbuf( this, size, pPool );  }
    ~XAutoBuf()
    {   lsr_xbuf_d( this );  }
    
    int     available() const                       {   return m_buf.m_pBufEnd - m_buf.m_pEnd; }

    char   *begin() const                           {   return m_buf.m_pBuf; }
    char   *end() const                             {   return m_buf.m_pEnd; }

    void    used( int size )                        {   m_buf.m_pEnd += size;     }
    void    clear()                                 {   m_buf.m_pEnd = m_buf.m_pBuf;    }

    int     capacity() const                        {   return m_buf.m_pBufEnd - m_buf.m_pBuf; }
    int     size()   const                          {   return m_buf.m_pEnd - m_buf.m_pBuf;   }

    int     reserve( int size )                     {   return lsr_xbuf_reserve( this, size );    }
    void    resize( int size )                      {   lsr_xbuf_resize( this, size );   }
    int     grow( int size )                        {   return lsr_xbuf_grow( this, size );  }
    
    bool    empty()  const                          {   return ( m_buf.m_pBuf == m_buf.m_pEnd );  }
    bool    full() const                            {   return ( m_buf.m_pEnd == m_buf.m_pBufEnd ); }
    int     get_offset( const char * p ) const      {   return p - m_buf.m_pBuf;    }
    char   *getp( int offset ) const                {   return m_buf.m_pBuf + offset; }
    void    swap( XAutoBuf& rhs )                   {   lsr_xbuf_swap( this, &rhs ); }
    
    int     pop_front_to( char * pBuf, int size )   {   return lsr_xbuf_pop_front_to( this, pBuf, size );}
    int     pop_front( int size )                   {   return lsr_xbuf_pop_front( this, size ); }
    int     pop_end( int size )                     {   return lsr_xbuf_pop_end( this, size );   }

    int     append( const char * pBuf, int size )   {   return lsr_xbuf_append2( this, pBuf, size ); }
    int     append( const char * pBuf )             {   return lsr_xbuf_append( this, pBuf ); }
    void    append_unsafe( char ch )                {   *m_buf.m_pEnd++ = ch; }
    
    int append_unsafe( const char * pBuf, int size )
    {
        memmove( end(), pBuf, size );
        used( size );
        return size;
    }
};

#endif

