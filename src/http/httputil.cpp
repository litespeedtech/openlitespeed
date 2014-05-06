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
#include "httputil.h"
#include <util/stringtool.h>

#include <ctype.h>
#include <string.h>

int HttpUtil::unescape_inplace( char * pDest, int& uriLen,
                                const char * &pOrgSrc  )
{
    register const char * pSrc = pOrgSrc;
    register const char * pEnd = pOrgSrc + uriLen;
    register char * p = pDest;
    
    while ( pSrc < pEnd )
    {
        register char c = *pSrc++;
        switch(c)
        {
            case '%':
            {
                register char x1, x2;
                x1 = *pSrc++;
                if (!isxdigit(x1))
                    return -1;
                x2 = *pSrc++;
                if (!isxdigit(x2))
                    return -1;
                c = (hexdigit(x1) << 4) + hexdigit(x2);
                break;
            }
            case '?':
                uriLen = p - pDest;
                *p++ = 0;
                if ( pOrgSrc != pDest )
                {
                    pOrgSrc = p;
                    memmove( p, pSrc, pEnd - pSrc );
                    p+= pEnd - pSrc;
                    *p++ = 0;
                }
                else
                {
                    pOrgSrc = pSrc;
                    p = (char *)pEnd + 1;
                }
                return p - pDest;
        }
        switch( c )
        {
            case '.':
                if ( *(p - 1) == '/' )
                    return -1;
                *p++ = c;
                break;
            case '/':
                //get rid of duplicate '/'s.
                if ( *pSrc == '/' )
                    break;
                //fall through
            default:
                *p++ = c;
        }
    }
    pOrgSrc = p;
    uriLen = p - pDest;
    *p++ = 0;
    return p - pDest;
}


int HttpUtil::unescape( char * pDest, int& uriLen,
                        const char * &pOrgSrc  )
{
    register const char * pSrc = pOrgSrc;
    register const char * pEnd = pOrgSrc + uriLen;
    register char * p = pDest;

    while ( pSrc < pEnd )
    {
        register char c = *pSrc++;
        switch(c)
        {
        case '%':
        {
            register char x1, x2;
            x1 = *pSrc++;
            if (!isxdigit(x1))
            {
                *p++ = '%';
                c = x1;
                break;
            }
            x2 = *pSrc++;
            if (!isxdigit(x2))
            {
                *p++ = '%';
                *p++ = x1;
                c = x2;
                break;
            }
            c = (hexdigit(x1) << 4) + hexdigit(x2);
            break;
        }
        case '?':
            uriLen = p - pDest;
            *p++ = 0;
            if ( pOrgSrc != pDest )
            {
                pOrgSrc = p;
                memmove( p, pSrc, pEnd - pSrc );
                p+= pEnd - pSrc;
                *p++ = 0;
            }
            else
            {
                pOrgSrc = pSrc;
                p = (char *)pEnd + 1;
            }
            return p - pDest;
        }
        /*
        switch( c )
        {
        case '.':
            if ( *(p - 1) == '/' )
                return -1;
            *p++ = c;
            break;
        case '/':
            //get rid of duplicate '/'s.
            if ( *pSrc == '/' )
                break;
            //fall through
        default:
            *p++ = c;
        }
        */
        *p++ = c;
    }
    pOrgSrc = p;
    uriLen = p - pDest;
    *p++ = 0;
    return p - pDest;
}


//int HttpUtil::unescape( const char * pSrc, char * pDest )
//{
//    register char c;
//    register char * p = pDest;
//    while ((c = *pSrc++) != 0)
//    {
//        switch(c)
//        {
//        case '%':
//        {
//            register char x1, x2;
//            x1 = *pSrc++;
//            if (!isxdigit(x1))
//                    return -1;
//            x2 = *pSrc++;
//            if (!isxdigit(x2))
//                    return -1;
//            *p++ = (hexdigit(x1) << 4) + hexdigit(x2);
//            break;
//        }
//        case '/':
//            //get rid of duplicate '/'s.
//            if ( *pSrc == '/' )
//                break;
//        default:
//            *p++ = c;
//        }
//    }
//    *p = 0;
//    return p - pDest;
//}

int HttpUtil::unescape_n(const char *pSrc, char *pDest, int n)
{
    register char c;
    register char * p = pDest;

    while (n-- && ((c = *pSrc++) != 0))
    {
        switch(c)
        {
        case '%':
        {
            register char x1, x2;
            x1 = *pSrc++;
            if (!isxdigit(x1))
                    return -1;
            x2 = *pSrc++;
            if (!isxdigit(x2))
                    return -1;
            *p++ = (hexdigit(x1) << 4) + hexdigit(x2);
            n -= 2;
            break;
        }
        case '/':
            //get rid of duplicate '/'s.
            if ( *pSrc == '/' )
                break;
        default:
            *p++ = c;
        }
    }
    *p = 0;
    return p - pDest;
}

int HttpUtil::escape( const char * pSrc, char * pDest, int n )
{
    register char ch;
    register char * p = pDest;
    while ( --n&&((ch = *pSrc++) != 0))
    {
        switch (ch)
        {
//        case ';':
//        case '=':
//        case '$':
//        case ',':
//        case ':':
        case '%':
        case ' ':
        case '?':
        case '+':
        case '&':
            if ( n < 3 )
            {        
                *p++ = '%';
                *p++ = StringTool::s_hex[(ch >> 4) & 15];
                *p++ = StringTool::s_hex[ch & 15];
                n -=2;
            }
            else
                n = 1;
            break;
        default:
            *p++ = ch;
            break;
        }
    }
    *p = 0;
    return p - pDest;

}

int HttpUtil::unescape_n(const char *pSrc, int srcLen, char *pDest, int n)
{
    register char c;
    register char * p = pDest;
    register const char * pSrcEnd = pSrc + srcLen;

    while (n-- && (pSrc < pSrcEnd ))
    {
        c = *pSrc++;
        if ( c == '%' && pSrc + 2 < pSrcEnd )
        {
            register char x1, x2;
            x1 = *pSrc++;
            if (!isxdigit(x1))
                    return -1;
            x2 = *pSrc++;
            if (!isxdigit(x2))
                    return -1;
            *p++ = (hexdigit(x1) << 4) + hexdigit(x2);
            n -= 2;
        }
        else if ( c == '/' && pSrc < pSrcEnd && *pSrc == '/' )
        {
            //handle "://" case, keep them
            if (n + 2 <= srcLen && pSrc[-2] == ':') 
                *p++ = c;
            //else
                //; //Do nothing so that get rid of duplicate '/'s.
        }
        else
            *p++ = c;
    }
        
    return p - pDest;
}
int HttpUtil::escape( const char * pSrc, int len, char * pDest, int n )
{
    register char ch;
    register char * p = pDest;
    register const char * pEnd = pSrc + len;
    while ( --n&&(pSrc < pEnd ))
    {
        ch = *pSrc++;
        switch (ch)
        {
//        case ';':
//        case '=':
//        case '$':
//        case ',':
//        case ':':
        case '%':
        case ' ':
        case '?':
        case '+':
        case '&':
            if ( n < 3 )
            {        
                *p++ = '%';
                *p++ = StringTool::s_hex[(ch >> 4) & 15];
                *p++ = StringTool::s_hex[ch & 15];
                n -=2;
            }
            else
                n = 1;
            break;
        default:
            *p++ = ch;
            break;
        }
    }
    *p = 0;
    return p - pDest;

}
int HttpUtil::escapeHtml(const char *pSrc, const char * pSrcEnd, char * pDest, int n)
{
    char * pBegin = pDest;
    char * pEnd = pDest + n - 6;
    char ch;
    while( (pSrc < pSrcEnd )&&(ch = *pSrc)&&( pDest < pEnd ) )
    {
        switch( ch )
        {
        case '<':
            memmove( pDest, "&lt;", 4 );
            pDest += 4;
            break;
        case '>':
            memmove( pDest, "&gt;", 4 );
            pDest += 4;
            break;
        case '&':
            memmove( pDest, "&amp;", 5 );
            pDest += 5;
            break;
        case '"':
            memmove( pDest, "&quot;", 6 );
            pDest += 6;
            break;
        default:
            *pDest++ = ch;
            break;
        }
        ++pSrc;
    }
    *pDest = 0;
    return pDest - pBegin;
}
