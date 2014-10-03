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
#include <lsr/lsr_str.h>
#include <lsr/lsr_strtool.h>
#include <lsr/lsr_strlist.h>

#include <ctype.h>
#include <string.h>
#include <openssl/md5.h>


char * lsr_strupper( const char *pSrc, char *pDest )
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

char * lsr_strnupper( const char *pSrc, char *pDest, int *pCnt )
{
    if ( pSrc && pDest)
    {
        char * p1 = pDest;
        char * p2 = pDest + *pCnt;
        while(( *pSrc )&&( p1 < p2 ))
            *p1++ = toupper( *pSrc++ );
        if ( p1 < p2 )
            *p1 = 0;
        *pCnt = p1 - pDest;
        return pDest;
    }
    else
        return NULL;
}


char * lsr_strlower( const char *pSrc, char *pDest )
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

char * lsr_strnlower( const char *pSrc, char *pDest, int *pCnt )
{
    if ( pSrc && pDest)
    {
        char * p1 = pDest;
        char * p2 = pDest + *pCnt;
        while(( *pSrc )&&( p1 < p2 ))
            *p1++ = tolower( *pSrc++ );
        if ( p1 < p2 )
            *p1 = 0;
        *pCnt = p1 - pDest;
        return pDest;
    }
    else
        return NULL;
}

char * lsr_strtrim( char *p )
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

int lsr_strtrim2( const char **pBegin, const char **pEnd )
{
    const char * p1 = *pBegin;
    const char * p2 = *pEnd;
    if ( p1 && p2 > p1 )
    {
        while(( p1 < p2 )&&( isspace( *p1 )))
            ++p1;
        while(( p1 < p2 )&&( isspace( *( p2 - 1 ))))
            --p2;
        *pBegin = p1;
        *pEnd = p2;
        return p2 - p1;        
    }
    return 0;
}

const char lsr_s_hex[17] = "0123456789abcdef";

int lsr_hexencode( const char *pSrc, int len, char *pDest )
{
    const char * pEnd = pSrc + len;
    if ( pDest == pSrc )
    {
        char * pDestEnd = pDest + ( len << 1 );
        *pDestEnd-- = 0;
        while( (--pEnd) >= pSrc )
        {
            *pDestEnd-- = lsr_s_hex[ *pEnd & 0xf ];
            *pDestEnd-- = lsr_s_hex[ ((unsigned char )(*pEnd)) >> 4 ];
        }
    }
    else
    {
        while( pSrc < pEnd )
        {
            *pDest++ = lsr_s_hex[ ((unsigned char )(*pSrc)) >> 4 ];
            *pDest++ = lsr_s_hex[ *pSrc++ & 0xf ];
        }
        *pDest = 0;
    }
    return len << 1;
}

int lsr_hexdecode( const char *pSrc, int len, char *pDest )
{
    const char * pEnd = pSrc + len - 1;
    while( pSrc < pEnd )
    {
        *pDest++ = ( hexdigit( *pSrc ) << 4) + hexdigit( *(pSrc+1) );
        pSrc += 2; 
    }
    return len / 2;
}

int lsr_str_off_t( char *pBuf, int len, off_t val )
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

const char * lsr_parse_parse( lsr_parse_t *pThis )
{
    const char * pStrBegin;
    if ( !pThis->m_pBegin || !pThis->m_delim || !(*pThis->m_delim) )
        return NULL;
    if ( pThis->m_pBegin < pThis->m_pEnd )
    {
        pStrBegin = pThis->m_pBegin;
        if ( !(*(pThis->m_delim + 1) ) )
        {
            pThis->m_pStrEnd = (const char *)memchr( pThis->m_pBegin,
              *pThis->m_delim, pThis->m_pEnd - pThis->m_pBegin );
        }
        else
            pThis->m_pStrEnd = strpbrk( pThis->m_pBegin, pThis->m_delim );
        if ( !pThis->m_pStrEnd )
            pThis->m_pBegin = pThis->m_pStrEnd = pThis->m_pEnd;
        else
            pThis->m_pBegin = pThis->m_pStrEnd + 1;
    }
    else
        pStrBegin = NULL;
    return pStrBegin;
}

lsr_strlist_t * lsr_parsematchpattern( const char *pPattern )
{
    char * pBegin;
    char ch;
    lsr_strlist_t * pList = lsr_strlist_new( 0 );
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
                lsr_strlist_add( pList, achBuf, pBegin - achBuf );
            pBegin = achBuf;
            while( 1 )
            {
                *pBegin++ = ch;
                if ( *pPattern != ch )
                    break;
                ++pPattern;
            }
            if ( ch == '*' )
                lsr_strlist_add( pList, achBuf, 1 );
            else
                lsr_strlist_add( pList, achBuf, pBegin - achBuf );
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
        lsr_strlist_add( pList, achBuf, pBegin - achBuf );
    return pList;
}

int lsr_strmatch( const char *pSrc, const char *pSrcEnd,
  lsr_str_t *const *begin, lsr_str_t *const *end, int case_sens )
{
    if ( !pSrcEnd  )
        pSrcEnd = pSrc + strlen( pSrc );
    char ch;
    int c = -1;
    int len;
    const char * p = lsr_str_c_str( *begin );
    
    while(( begin < end ) && (*( p = lsr_str_c_str( *begin ) ) != '*' ))
    {
        len = lsr_str_len( *begin );
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
        else /* must be '?' */
        {
            if ( len > pSrcEnd - pSrc )
                return -1;
        }
        pSrc += len;
        ++begin;
    }
    
    while(( begin < end ) && (*( p = lsr_str_c_str( *(end-1) ) ) != '*' ))
    {
        len = lsr_str_len( *(end-1) );
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
        else /* must be '?' */
        {
            if ( len > pSrcEnd - pSrc )
                return -1;
        }
        pSrcEnd -= len;
        --end;
    }
    if ( end - begin == 1 )  /* only a "*" left */
        return 0;
    if ( end - begin == 0 )  /* nothing left in pattern */
        return pSrc != pSrcEnd;
    while( begin < end )
    {
        p = lsr_str_c_str( *begin );
        if (( *p != '*' ) && ( *p != '?' ))
            break;
        if ( *p == '?' )
        {
            pSrc += lsr_str_len( *begin );
        }
        ++begin;
    }
    if ( end == begin )
        return pSrc != pSrcEnd;
    len = lsr_str_len( *begin ) - 1;
    ch = *(++p);
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
        int ret = lsr_strmatch( pSrc, pSrcEnd, begin, end, case_sens );
        if ( ret != 1 )
            return ret;
        ++pSrc;
    }
    return -1;
}

char * lsr_convertmatchtoreg( char *pStr, char *pBufEnd )
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

const char * lsr_strnextarg( const char **pStr, const char *pDelim )
{
    if ( !pDelim )
        pDelim = " \t\r\n";
    const char * s = *pStr;
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
    *pStr = s;
    return p;       /* points past the end of the arg (delim or quote) */
}

const char * lsr_getline( const char *pBegin, const char *pEnd  )
{
    if ( pEnd <= pBegin )
        return NULL;
    const char * p = (const char *)memchr( pBegin, '\n', pEnd - pBegin );
    if ( !p )
        return pEnd;
    else
        return p;
}

const char *lsr_get_conf_line( const char **pParseBegin, const char *pParseEnd, const char **pLineEnd )
{
    const char *pLineBegin;
    
    lsr_strtrim2( pParseBegin, &pParseEnd );
    if ( *pParseBegin >= pParseEnd )
        return NULL;
    
    pLineBegin = *pParseBegin;
    *pLineEnd = (const char *)memchr( pLineBegin, '\n', pParseEnd - pLineBegin );
    if ( !(*pLineEnd))
        *pLineEnd = pParseEnd;
    
    *pParseBegin = *pLineEnd + 1;
    skipTrailingSpace( pLineEnd );
    return pLineBegin;
}

int lsr_parsenextarg( const char **pRuleStr, const char *pEnd,
  const char **pArgBegin, const char **pArgEnd, const char **pError )
{
    const char * ruleStr = *pRuleStr;
    int quoted = 0;
    while(( ruleStr < pEnd )&&( isspace( *ruleStr )))
        ++ruleStr;
    if (( *ruleStr == '"' )||( *ruleStr == '\'' ))
    {
        quoted = *ruleStr;
        ++ruleStr;
    }
    *pArgBegin = *pArgEnd = ruleStr;
    while( ruleStr < pEnd )
    {
        char ch = *ruleStr;
        if ( ch == '\\' )
        {
            ruleStr += 2;
        }
        else if (((quoted)&&( ch == quoted ))||
            ((!quoted)&&( isspace( ch ))))
        {
            *pArgEnd = ruleStr++;
            *pRuleStr = ruleStr;
            return 0;
        }
        else
            ++ruleStr;
    }
    if ( quoted )
    {
        *pArgEnd = pEnd;
        *pRuleStr = ruleStr;
        return 0;
    }
    if ( ruleStr > pEnd )
    {
        *pError = "pre-mature end of line";
        *pRuleStr = ruleStr;
        return -1;
    }
    *pArgEnd = ruleStr;
    *pRuleStr = ruleStr;
    return 0;
}


const char * lsr_findclosebracket( const char *pBegin, const char *pEnd,
  char chOpen, char chClose )
{
    int dep = 1;    /* assumes already in a bracket */
    while( pBegin < pEnd )
    {
        char ch = *pBegin;
        if ( ch == chOpen )
            ++dep;
        else if ( ch == chClose )
        {
            --dep;
            if ( dep == 0 )
                break;
        }
        ++pBegin;
    }
    return pBegin;
}

const char * lsr_findcharinbracket( const char *pBegin, const char *pEnd,
  char searched, char chOpen, char chClose )
{
    int dep = 1;    /* assumes already in a bracket */
    while( pBegin < pEnd )
    {
        char ch = *pBegin;
        if ( ch == chOpen )
            ++dep;
        else if ( ch == chClose )
        {
            --dep;
            if ( dep == 0 )
                return NULL;
        }
        else if (( dep == 1 )&&( ch == searched ))
        {
            return pBegin;
        }
        ++pBegin;
    }
    return NULL;
}

int lsr_unescapequote( char *pBegin, char *pEnd, int ch )
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
            ++n; ++pBegin;  /* NOTE: the start of the result has moved */
            p = p+1;
        }
        else
            p = p + 2;
    }
    return n;
}

const char * lsr_lookupsubstring( const char *p, const char *pEnd,
  const char *key, int keyLen, int *retLen, char sep, char comp )
{
    const char *ptr;
    *retLen = 0;
    if ( keyLen <= 0 )
        return NULL;
    while ( p < pEnd )
    {
        while ( *p == ' ' && (pEnd - p) >= keyLen )
            ++p;
        if ( (pEnd - p) < keyLen )
            return NULL;
        if ( memcmp( p, key, keyLen ) == 0 )
        {
            ptr = p;
            p += keyLen;
            while ( p < pEnd && *p == ' ' )
                ++p;
            if ( p == pEnd || *p == sep )
            {
                if ( comp == '\0' )
                    return ptr;
                break;
            }
            if ( *p == comp && comp != '\0' )
            {
                ++p;
                /* check for spaces AFTER comparator */
                while ( p < pEnd && *p == ' ' )
                    ++p;
                if ( p == pEnd )
                {
                    *retLen = 0;
                    return p;
                }
                if ( *p == '\'' || *p == '"' )
                {
                    ptr = (const char *)memchr( p + 1, *p, pEnd - p );
                    ++p;
                    if (ptr == NULL)
                        ptr = pEnd;
                }
                else
                {
                    ptr = (const char *)memchr( p, sep, pEnd - p );
                    if (ptr == NULL)
                        ptr = pEnd;
                    while ( ptr > p && *( ptr - 1 ) == ' ' )
                        --ptr;
                }
                *retLen = ptr - p;
                return p;
            }
        }
        p = (const char *)memchr( p, sep, pEnd - p );
        if ( p == NULL )
            return NULL;
        ++p;
    }
    return NULL;
}

#define CACHE_SIZE  256

const char * lsr_mempbrk(
  const char *pInput, int iSize, const char *accept, int acceptLen )
{
    register const char *p, *min = NULL, *pAcceptPtr = accept, 
      *pAcceptEnd = accept + acceptLen;
    while( iSize > 0 )
    {
        while( pAcceptPtr < pAcceptEnd )
        {
            if ( ( p = (const char *)memchr( pInput, *pAcceptPtr,
              (iSize > CACHE_SIZE) ? CACHE_SIZE : iSize ) ) != NULL )
            {
                iSize = p - pInput;
                min = p;
            }
            ++pAcceptPtr;
        }
        if ( min != NULL )
            return min;
        pInput += CACHE_SIZE;
        iSize -= CACHE_SIZE;
        pAcceptPtr = accept;
    }
    return NULL;
}

#define NUM_CHAR    256

size_t lsr_memspn(
  const char *pInput, int iSize, const char *accept, int acceptLen )
{
    char aAcceptArray[NUM_CHAR];
    memset( aAcceptArray, 0, NUM_CHAR );
    int i;
    for( i = 0; i < acceptLen; ++i )
        aAcceptArray[ (unsigned char)accept[i] ] = 1;
    for( i = 0; i < iSize; ++i )
        if ( aAcceptArray[ (unsigned char)pInput[i] ] == 0 )
            return i;
    return iSize;
}

size_t lsr_memcspn(
  const char *pInput, int iSize, const char *accept, int acceptLen )
{
    register const char *ptr = NULL;
    while( acceptLen > 0 )
    {
        ptr = (const char *)memchr( pInput, *accept, iSize );
        if ( ptr != NULL )
            iSize = ptr - pInput;
        ++accept;
        --acceptLen;
    }
    return iSize;
}

void lsr_getmd5( const char *src, int len, unsigned char *dstBin )
{
    MD5_CTX ctx;
    MD5_Init( &ctx );
    MD5_Update( &ctx, src, len );
    MD5_Final( dstBin, &ctx );
}

