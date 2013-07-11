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
#ifndef STRINGTOOL_H
#define STRINGTOOL_H

#include <stddef.h>
#include <sys/types.h>


template< typename T> class TPointerList;

inline void skipLeadingSpace( const char ** p )
{
    register char ch;
    while( (( ch = **p ) == ' ')||(ch == '\t'))
        ++(*p);
}

inline void skipTrailingSpace( const char ** p )
{
    register char ch;
    while( (( ch = (*p)[-1] ) == ' ')||(ch == '\t'))
        --(*p);
}

inline char hexdigit( char ch )
{
    return (((ch) <= '9') ? (ch) - '0' : ((ch) & 7) + 9);
}


class StrParse
{
    const char * m_pBegin;
    const char * m_pEnd;
    const char * m_delim;
    const char * m_pStrEnd;
public:
    StrParse( const char * pBegin, const char * pEnd, const char * delim )
        : m_pBegin( pBegin )
        , m_pEnd( pEnd )
        , m_delim( delim )
    {}
    ~StrParse() {}  
    int isEnd() const { return m_pEnd <= m_pBegin ;  }
    
    const char * parse();

    const char * trim_parse()
    {
        skipLeadingSpace( &m_pBegin );
        const char * p = parse();
        if ( p &&( p != m_pStrEnd ) )
            skipTrailingSpace( &m_pStrEnd );
        return p;
    }

    const char * getStrEnd() const  {   return m_pStrEnd;   }

};

class StringList;
class AutoStr2;

class StringTool
{
    StringTool();
    ~StringTool();
public:
    static const char s_hex[17];
    static char * strupper( const char * pSrc, char * pDest );
    static char * strnupper( const char * p, char * pDest, int &n );
    static char * strlower( const char * pSrc, char * pDest );
    static char * strnlower( const char * pSrc, char * pDest, int &n );
    static char * strtrim( char * p );
    static int    strtrim( const char *&pBegin, const char *&pEnd );
    static int    hexEncode( const char * pSrc, int len, char * pDest );
    static int    hexDecode( const char * pSrc, int len, char * pDest );
    static int    strmatch( const char * pSrc, const char * pEnd,
                   AutoStr2 *const*begin, AutoStr2 *const*end, int case_sens );
    static StringList * parseMatchPattern( const char * pPattern );
    static const char * strNextArg( const char * &s, const char * pDelim = NULL );
    static char * strNextArg( char * &s, const char * pDelim = NULL );
    static const char * getLine( const char * pBegin,
                        const char * pEnd  );
    static int parseNextArg( const char * &pRuleStr, const char * pEnd,
                const char * &argBegin, const char * &argEnd, const char * &pError );
    static char * convertMatchToReg( char * pStr, char * pBufEnd );
    static const char * findCloseBracket( const char * pBegin, 
                                          const char * pEnd );

    static const char * findCharInBracket( const char * pBegin, 
                                 const char * pEnd, char searched );
    static int str_off_t( char * pBuf, int len, off_t val );
    static int unescapeQuote( char * pBegin, char * pEnd, int ch );
};

#endif
