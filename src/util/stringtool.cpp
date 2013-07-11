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
#include <util/stringtool.h>
#include <util/autostr.h>
#include <util/stringlist.h>
#include <ctype.h>
#include <string.h>
#include <openssl/md5.h>

StringTool::StringTool(){
}
StringTool::~StringTool(){
}

char * StringTool::strupper( const char * pSrc, char * pDest )
{
    if ( pSrc && pDest)
    {
        char * p1 = pDest;
        while( *pSrc )
            *p1++ = toupper( *pSrc++ );
        *p1 = 0;
        return pDest;
    }
    else
        return NULL;
}

char * StringTool::strnupper( const char * pSrc, char * pDest, int &n )
{
    if ( pSrc && pDest)
    {
        char * p1 = pDest;
        char * p2 = pDest + n;
        while(( *pSrc )&&( p1 < p2 ))
            *p1++ = toupper( *pSrc++ );
        if ( p1 < p2 )
            *p1 = 0;
        n = p1 - pDest;
        return pDest;
    }
    else
        return NULL;
}


char * StringTool::strlower( const char * pSrc, char * pDest )
{
    if ( pSrc && pDest)
    {
        char * p1 = pDest;
        while( *pSrc )
            *p1++ = tolower( *pSrc++ );
        *p1 = 0;
        return pDest;
    }
    else
        return NULL;
}

char * StringTool::strnlower( const char * pSrc, char * pDest, int &n )
{
    if ( pSrc && pDest)
    {
        char * p1 = pDest;
        char * p2 = pDest + n;
        while(( *pSrc )&&( p1 < p2 ))
            *p1++ = tolower( *pSrc++ );
        if ( p1 < p2 )
            *p1 = 0;
        n = p1 - pDest;
        return pDest;
    }
    else
        return NULL;
}

char * StringTool::strtrim( char * p )
{
    if ( p )
    {
        while(( *p )&&( isspace( *p )))
            ++p;
        if ( *p )
        {
            char *p1 = p + strlen( p ) - 1;
            while(( p1 > p )&&( isspace( *p1 )))
                --p1;
            *( p1 + 1 ) = 0;
        }
    }
    return p;
}

int StringTool::strtrim( const char *&pBegin, const char *&pEnd )
{
    if ( pBegin && pEnd > pBegin )
    {
        while(( pBegin < pEnd )&&( isspace( *pBegin )))
            ++pBegin;
        while(( pBegin < pEnd )&&( isspace( *( pEnd - 1 ))))
            --pEnd;
        return pEnd - pBegin;        
    }
    return 0;
}

const char StringTool::s_hex[17] = "0123456789abcdef";

int StringTool::hexEncode( const char * pSrc, int len, char * pDest )
{
    const char * pEnd = pSrc + len;
    if ( pDest == pSrc )
    {
        char * pDestEnd = pDest + ( len << 1 );
        *pDestEnd-- = 0;
        while( (--pEnd) >= pSrc )
        {
            *pDestEnd-- = s_hex[ *pEnd & 0xf ];
            *pDestEnd-- = s_hex[ ((unsigned char )(*pEnd)) >> 4 ];
        }
    }
    else
    {
        while( pSrc < pEnd )
        {
            *pDest++ = s_hex[ ((unsigned char )(*pSrc)) >> 4 ];
            *pDest++ = s_hex[ *pSrc++ & 0xf ];
        }
        *pDest = 0;
    }
    return len << 1;
}

int StringTool::hexDecode( const char * pSrc, int len, char * pDest )
{
    const char * pEnd = pSrc + len - 1;
    while( pSrc < pEnd )
    {
        *pDest++ = ( hexdigit( *pSrc ) << 4) + hexdigit( *(pSrc+1) );
        pSrc += 2; 
    }
    return len / 2;
}

int StringTool::str_off_t( char * pBuf, int len, off_t val )
{
    char * p = pBuf;
    char * p1;
    char * pEnd = pBuf + len;
    p1 = pEnd;
    if (val < 0) 
    {
        *p++ = '-';
        val = -val;
    }

    do 
    {
        *--p1 = '0' + (val % 10);
        val = val / 10;
    }while (val > 0);

    if ( p1 != p )
        memmove( p, p1, pEnd - p1 );
    p += pEnd - p1;
    *p = 0;
    return p - pBuf;

}


const char * StrParse::parse()
{
    const char * pStrBegin;
    if ( !m_pBegin || !m_delim || !(*m_delim) )
        return NULL;
    if ( m_pBegin < m_pEnd )
    {
        pStrBegin = m_pBegin;
        if ( !(*(m_delim + 1) ) )
            m_pStrEnd = (const char *)memchr( m_pBegin, *m_delim, m_pEnd - m_pBegin );
        else
            m_pStrEnd = strpbrk( m_pBegin, m_delim );
        if ( !m_pStrEnd )
            m_pBegin = m_pStrEnd = m_pEnd;
        else
            m_pBegin = m_pStrEnd + 1;
    }
    else
        pStrBegin = NULL;
    return pStrBegin;
}


StringList * StringTool::parseMatchPattern( const char * pPattern )
{
    char * pBegin;
    char ch;
    StringList * pList = new StringList();
    if ( !pList )
        return NULL;
    char achBuf[2048];
    pBegin = achBuf;
    *pBegin++ = 0;
    while( (ch = *pPattern++) )
    {
        switch( ch )
        {
        case '*':
        case '?':
            if ( pBegin - 1 != achBuf )
                pList->add( achBuf, pBegin - achBuf );
            pBegin = achBuf;
            while( 1 )
            {
                *pBegin++ = ch;
                if ( *pPattern != ch )
                    break;
                ++pPattern;
                
            }
            if ( ch == '*' )
                pList->add( achBuf, 1 );
            else
                pList->add( achBuf, pBegin - achBuf );
            pBegin = achBuf;
            *pBegin++ = 0;
            break;

        case '\\':
            ch = *pPattern++;
            //fall through
        default:
            if ( ch )
                *pBegin++ = ch;
            break;
                        
        }
    }
    if ( pBegin - 1 != achBuf )
        pList->add( achBuf, pBegin - achBuf );
    return pList;
}


//  return value:
//          0 : match
//         != 0: not match
//
int StringTool::strmatch( const char * pSrc, const char * pSrcEnd,
                AutoStr2 * const *begin, AutoStr2 *const *end, int case_sens )
{
    if ( !pSrcEnd  )
        pSrcEnd = pSrc + strlen( pSrc );
    char ch;
    int c = -1;
    int len;
    const char * p = (*begin)->c_str();
    
    while(( begin < end )&&(*((p = (*begin)->c_str())) != '*' ))
    {
        len = (*begin)->len();
        if ( !*p )
        {
            ++p; --len;
            if ( len > pSrcEnd - pSrc )
                return -1;
            if ( case_sens )
                c = strncmp( pSrc, p, len );
            else
                c = strncasecmp( pSrc, p, len );
            if ( c )
                return 1;
        }
        else // must be '?'
        {
            if ( len > pSrcEnd - pSrc )
                return -1;
        }
        pSrc += len;
        ++begin;
    }
    
    while(( begin < end )&&( *(p = (*(end-1))->c_str()) != '*' ))
    {
        len = (*(end-1))->len();
        if ( !*p )
        {
            ++p; --len;
            if ( len > pSrcEnd - pSrc )
                return -1;
            if ( case_sens )
                c = strncmp( pSrcEnd - len, p, len );
            else
                c = strncasecmp( pSrcEnd - len, p, len );
            if ( c )
                return 1;
        }
        else // must be '?'
        {
            if ( len > pSrcEnd - pSrc )
                return -1;
        }
        pSrcEnd -= len;
        --end;
    }
    if ( end - begin == 1 )  //only a "*" left
        return 0;
    if ( end - begin == 0 )  //nothing left in pattern
        return pSrc != pSrcEnd;
    while( begin < end )
    {
        p = (*begin)->c_str();
        if (( *p != '*' )&&( *p != '?' ))
            break;
        if ( *p == '?' )
        {
            pSrc += (*begin)->len();
        }
        ++begin;
    }
    if ( end == begin )
        return pSrc != pSrcEnd;
    len = (*begin)->len() - 1;
    ch = *(++p );
    char search[4];
    if ( !case_sens )
    {
        search[0] = tolower( ch );
        search[1] = toupper( ch );
        search[2] = 0;
    }
    while( pSrcEnd - pSrc >= len )
    {
        if ( case_sens )
            pSrc = ( const char *)memchr( pSrc, ch, pSrcEnd - pSrc );
        else
            pSrc = strpbrk( pSrc, search );
        if ( !pSrc || (pSrcEnd - pSrc < len) )
            return -1;
        int ret = strmatch( pSrc, pSrcEnd, begin, end, case_sens );
        if ( ret != 1 )
            return ret;
        ++pSrc;
    }
    return -1;
}

char * StringTool::convertMatchToReg( char * pStr, char * pBufEnd )
{
    char * pEnd = pStr + strlen( pStr );
    char * p = pStr;
    while(1)
    {
        switch( *p )
        {
        case 0:
            return pStr;
        case '?':
            *p = '.';
            break;
        case '*':
            if ( pEnd == pBufEnd )
                return NULL;
            memmove( p+1, p, ++pEnd - p );
            *p++ = '.';
            break;
        }
        ++p;
    }
    return pStr;
}


char * StringTool::strNextArg( char * &s, const char * pDelim )
{
    const char * p = s;
    char * p1 = (char *)strNextArg( p, pDelim);
    s = (char *)p;
    return p1;
}


const char * StringTool::strNextArg( const char * &s, const char * pDelim )
{
    if ( !pDelim )
        pDelim = " \t\r\n";
    const char * p = s;
    if (( *s == '\"' )||( *s == '\'' ))
    {
        char ch = *s++;
        while( (p = strchr( p+1, ch )) != NULL )
        {
            const char * p1 = p;    
            while(( p1 > s )&&( '\\' == *(p1 - 1) ))
                --p1;
            if ( (p - p1) % 2 == 0 )
            {
                break;
            }            
        }
    }
    else
        p = strpbrk( s, pDelim );
    return p;
}

const char * StringTool::getLine( const char * pBegin, const char * pEnd  )
{
    if ( pEnd <= pBegin )
        return NULL;
    const char * p = (const char *)memchr( pBegin, '\n', pEnd - pBegin );
    if ( !p )
        return pEnd;
    else
        return p;
}

int StringTool::parseNextArg( const char * &pRuleStr, const char * pEnd,
                const char * &argBegin, const char * &argEnd, const char * &pError )
{
    int quoted = 0;
    while(( pRuleStr < pEnd )&&( isspace( *pRuleStr )))
        ++pRuleStr;
    if (( *pRuleStr == '"' )||( *pRuleStr == '\'' ))
    {
        quoted = *pRuleStr;
        ++pRuleStr;
    }
    argBegin = argEnd = pRuleStr;
    while( pRuleStr < pEnd )
    {
        char ch = *pRuleStr;
        if ( ch == '\\' )
        {
            pRuleStr += 2;
        }
        else if (((quoted)&&( ch == quoted ))||
            ((!quoted)&&( isspace( ch ))))
        {
            argEnd = pRuleStr++;
            return 0;
        }
        else
            ++pRuleStr;
    }
    if ( quoted )
    {
        argEnd = pEnd;
        return 0;
    }
    if ( pRuleStr > pEnd )
    {
        pError = "pre-mature end of line";
        return -1;
    }
    argEnd = pRuleStr;
    return 0;
}



const char * StringTool::findCloseBracket( const char * pBegin, const char * pEnd )
{
    int dep = 1;
    while( pBegin < pEnd )
    {
        char ch = *pBegin;
        if ( ch == '{' )
            ++dep;
        else if ( ch == '}' )
        {
            --dep;
            if ( dep == 0 )
                break;
        }
        ++pBegin;
    }
    return pBegin;
}

const char * StringTool::findCharInBracket( const char * pBegin, const char * pEnd, char searched )
{
    int dep = 1;
    while( pBegin < pEnd )
    {
        char ch = *pBegin;
        if ( ch == '{' )
            ++dep;
        else if ( ch == '}' )
        {
            --dep;
            if ( dep == 0 )
                return NULL;
        }
        else if ((dep == 1)&&( ch == searched ))
        {
            return pBegin;
        }
        ++pBegin;
    }
    return NULL;
}



//void testBuildArgv()
//{
//    char cmd[] = "/path/to/cmd\targ1  arg2\targ3\t \t";
//    TPointerList<char> argvs;
//    char * pDir;
//    StringTool::buildArgv( cmd, &pDir, &argvs );
//    assert( strcmp( pDir, "/path/to" ) == 0 );
//    assert( argvs.size() == 5 );
//    assert( strcmp( argvs[0], "cmd" ) == 0 );
//    assert( strcmp( argvs[1], "arg1" ) == 0 );
//    assert( strcmp( argvs[2], "arg2" ) == 0 );
//    assert( strcmp( argvs[3], "arg3" ) == 0 );
//    assert( NULL == argvs[4] );
//}

int StringTool::unescapeQuote( char * pBegin, char * pEnd, int ch )
{
    int n = 0;
    char * p = pBegin;
    while( p < pEnd - 1 )
    {
        p = (char *)memchr( p, '\\', pEnd - p - 1 );
        if ( !p )
            break;
        if ( *(p+1) == ch )
        {
            if ( p != pBegin )
                memmove( pBegin+1, pBegin, p - pBegin );
            ++n; ++pBegin;
            p = p+1;
        }
        else
            p = p + 2;
    }
    return n;
}

void StringTool::getMd5(const char *src, int len, unsigned char * dstBin)
{
    MD5_CTX ctx;
    MD5_Init( &ctx );
    MD5_Update( &ctx, src, len);
    MD5_Final( dstBin, &ctx );
}

