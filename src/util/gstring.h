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
#ifndef GSTRING_H
#define GSTRING_H


#include <stddef.h>
#include <string.h>

class Pool;

class GString
{
    char *  m_pStr;
    int     m_iStrLen;
    int     m_iBufSize;
    
    int     allocate( int size );
    int     reallocate( int size );
    void    deallocate();
public: 
    GString()
        : m_pStr( NULL )
        , m_iStrLen( 0 )
        , m_iBufSize( 0 )
        {};
    explicit GString( int bufLen );
    GString( const char * pStr );
    GString( const GString& rhs );
    GString( size_t bufLen, int val );
    GString( const char * pStr, int len );
    
    ~GString();

    const char * c_str()    const   {   return m_pStr;  }
    operator const char *() const   {   return m_pStr;  }
    char * buf()                    {   return m_pStr;  }
    int bufLen()                    {   return m_iBufSize;  }
    int len() const     {   return m_iStrLen;   }
    int size() const    {   return m_iStrLen;   }
    void clear()        {   m_iStrLen = 0;  if ( m_pStr )  *m_pStr = 0;    }
    int  resizeBuf( int size )
    {   return reallocate( size ); }
    void setLen( int len )      {   m_iStrLen = len;   }
    
    GString & assign( const char * pStr, int len );
    //GString & bassign( const char * pStr, int len );
    GString & operator=( const char * pStr )
    {   return assign( pStr, (pStr)? strlen( pStr ): 0 );  }
    GString & operator=( const GString& rhs );

    GString & append( const char * pStr, int len );
    GString & append( const char * pStr );
    GString & operator+= ( const GString& rhs )
    {   return append( rhs.m_pStr, rhs.m_iStrLen ); }
    
    GString & operator+= ( const char * pString )
    {   return append( pString );                   }

    int compare( const char * pString ) const
    {   return ::strcmp( m_pStr, pString );         }

    int case_compare( const char *pString ) const
    {   return ::strcasecmp( m_pStr, pString );     }

    char at( int offset ) const
    {   return (offset < m_iStrLen )? *(m_pStr + offset) : 0 ;  }

    static void setStringPool( Pool * pPool );
    
};

inline bool operator<( const GString& s1, const GString& s2 )
{
    return ::strcmp( s1.c_str(), s2.c_str() ) < 0;
}

inline bool operator>( const GString& s1, const GString& s2 )
{
    return ::strcmp( s1.c_str(), s2.c_str() ) > 0;
}

inline bool operator>=( const GString& s1, const GString& s2 )
{
    return ::strcmp( s1.c_str(), s2.c_str() ) >= 0;
}

inline bool operator<=( const GString& s1, const GString& s2 )
{
    return ::strcmp( s1.c_str(), s2.c_str() ) <= 0;
}

inline bool operator==( const GString& s1, const GString& s2 )
{
    return ::strcmp( s1.c_str(), s2.c_str() ) == 0;
}

inline bool operator!=( const GString& s1, const GString& s2 )
{
    return ::strcmp( s1.c_str(), s2.c_str() ) != 0;
}

#endif
