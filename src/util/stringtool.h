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
#ifndef STRINGTOOL_H
#define STRINGTOOL_H


#include <lsr/lsr_strtool.h>

#include <stddef.h>
#include <sys/types.h>


class StrParse : private lsr_parse_t
{
public:
    StrParse( const char *pBegin, const char *pEnd, const char *delim )
    {   lsr_parse( this, pBegin, pEnd, delim );   }
    ~StrParse()
    {   lsr_parse_d( this );   }
    
    int isEnd() const               {   return m_pEnd <= m_pBegin;   }
    const char * getStrEnd() const  {   return m_pStrEnd;   }
    
    const char * parse()
    {   return lsr_parse_parse( this );   }

    const char * trim_parse()
    {   return lsr_parse_trim_parse( this );   }
};

class StringList;
class AutoStr2;

class StringTool
{
    StringTool() {}
    ~StringTool() {}
public:
    static const char s_hex[17];
    static char * strupper( const char *pSrc, char *pDest )
    {   return lsr_strupper( pSrc, pDest );   }
    static char * strnupper( const char *pSrc, char *pDest, int &n )
    {   return lsr_strnupper( pSrc, pDest, &n );   }
    static char * strlower( const char *pSrc, char *pDest )
    {   return lsr_strlower( pSrc, pDest );   }
    static char * strnlower( const char *pSrc, char *pDest, int &n )
    {   return lsr_strnlower( pSrc, pDest, &n );   }
    static char * strtrim( char *p )
    {   return lsr_strtrim( p );   }
    static int    strtrim( const char *&pBegin, const char *&pEnd )
    {   return lsr_strtrim2( &pBegin, &pEnd );   }
    static int    hexEncode( const char *pSrc, int len, char *pDest )
    {   return lsr_hexencode( pSrc, len, pDest );   }
    static int    hexDecode( const char *pSrc, int len, char *pDest )
    {   return lsr_hexdecode( pSrc, len, pDest );   }
    static int    strmatch( const char *pSrc, const char *pEnd,
                    AutoStr2 *const*begin, AutoStr2 *const*end, int case_sens )
    {
        return lsr_strmatch( pSrc, pEnd,
          (lsr_str_t *const*)begin, (lsr_str_t *const*)end, case_sens );
    }
    static StringList * parseMatchPattern( const char *pPattern );
    static const char * strNextArg( const char *&s, const char *pDelim = NULL )
    {   return lsr_strnextarg( &s, pDelim );   }
    static char * strNextArg( char *&s, const char *pDelim = NULL )
    {   return (char *)lsr_strnextarg( (const char **)&s, pDelim );   }
    static const char * getLine( const char *pBegin, const char *pEnd  )
    {   return lsr_getline( pBegin, pEnd );   }
    static int    parseNextArg( const char *&pRuleStr, const char *pEnd,
                    const char *&argBegin, const char *&argEnd, const char *&pError )
    {   return lsr_parsenextarg( &pRuleStr, pEnd, &argBegin, &argEnd, &pError );   }
    static char * convertMatchToReg( char *pStr, char *pBufEnd )
    {   return lsr_convertmatchtoreg( pStr, pBufEnd );   }
    static const char * findCloseBracket(const char *pBegin, const char *pEnd,
                    char chOpen, char chClose )
    {   return lsr_findclosebracket( pBegin, pEnd, chOpen, chClose );   }
    static const char * findCharInBracket( const char *pBegin, const char *pEnd,
                    char searched, char chOpen, char chClose )
    {   return lsr_findcharinbracket( pBegin, pEnd, searched, chOpen, chClose );   }
    static int str_off_t( char *pBuf, int len, off_t val )
    {   return lsr_str_off_t( pBuf, len, val );   }
    static int unescapeQuote( char *pBegin, char *pEnd, int ch )
    {   return lsr_unescapequote( pBegin, pEnd, ch );   }
    static const char * lookupSubString(const char *pInput, const char *pEnd,
                    const char *key, int keyLen, int *retLen, char sep, char comp )
    {   return lsr_lookupsubstring( pInput, pEnd, key, keyLen, retLen, sep, comp );   }
    static const char * mempbrk( const char *pInput, int iSize, const char *accept, int acceptLen )
    {   return lsr_mempbrk( pInput, iSize, accept, acceptLen );   }
    static size_t memspn(const char *pInput, int iSize, const char *accept, int acceptLen )
    {   return lsr_memspn( pInput, iSize, accept, acceptLen );   }
    static size_t memcspn(const char *pInput, int iSize, const char *accept, int acceptLen )
    {   return lsr_memcspn( pInput, iSize, accept, acceptLen );   }
    
    static void * memmem(const char* haystack, size_t haystacklen, const char* needle, size_t needleLength );
 
    static void getMd5(const char *src, int len, unsigned char *dstBin )
    {   return lsr_getmd5( src, len, dstBin );   }
    
private:
    static const int NUM_CHAR = 256;
};

#endif
