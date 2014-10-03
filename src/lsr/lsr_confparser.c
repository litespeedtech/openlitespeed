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

#include <lsr/lsr_confparser.h>
#include <lsr/lsr_pool.h>
#include <lsr/lsr_strtool.h>
#include <stddef.h>
#include <sys/types.h>

typedef void (*skipLeadingFn)( const char **p, const char *pEnd );

static void lsr_conf_reset_parser( lsr_confparser_t *pThis, int size );
static lsr_objarray_t *lsr_conf_parse( lsr_confparser_t *pThis, const char *pConf, const char *pConfEnd, skipLeadingFn skipFn );
static const char *lsr_get_param( const char *pBufBegin, const char *pBufEnd, const char **pParamEnd );
static const char *lsr_findEndQuote( const char *pBegin, const char *pEnd, char ch );
static void lsr_addToList( lsr_confparser_t *pThis, const char *pBegin, int len );

static inline void skipLeadingDelims( const char **p, const char *pEnd )
{
    while ( *p < pEnd && isspace(**p) )
        ++(*p);
}
static inline void skipLeadingWhiteSpace( const char **p, const char *pEnd )
{
    register char ch;
    while( *p < pEnd && ((( ch = **p ) == ' ')||(ch == '\t')))
        ++(*p);
}

void lsr_confparser( lsr_confparser_t *pThis )
{
    lsr_objarray_init( &pThis->m_pList, sizeof( lsr_str_t ));
    lsr_objarray_setcapacity( &pThis->m_pList, NULL, 5 );
    lsr_str( &pThis->m_pStr, NULL, 0 );
}

void lsr_confparser_d( lsr_confparser_t *pThis )
{
    lsr_objarray_release( &pThis->m_pList, NULL );
    lsr_str_d( &pThis->m_pStr );
}

inline lsr_objarray_t *lsr_conf_parse_line( lsr_confparser_t *pThis, const char *pLine, const char *pLineEnd )
{   
    return lsr_conf_parse( pThis, pLine, pLineEnd, skipLeadingWhiteSpace );
}

inline lsr_objarray_t *lsr_conf_parse_multi( lsr_confparser_t *pThis, const char *pBlock, const char *pBlockEnd )
{   
    return lsr_conf_parse( pThis, pBlock, pBlockEnd, skipLeadingDelims );
}

lsr_objarray_t *lsr_conf_parse_line_kv( lsr_confparser_t *pThis, const char *pLine, const char *pLineEnd )
{
    const char *pParam;
    const char *pParamEnd;
    
    lsr_strtrim2( &pLine, &pLineEnd );
    if ( pLine >= pLineEnd )
        return NULL;
    
    lsr_conf_reset_parser( pThis, pLineEnd - pLine + 1 );
    
    pParam = lsr_get_param( pLine, pLineEnd, &pParamEnd );
    lsr_addToList( pThis, pParam, pParamEnd - pParam );
    
    pParam = pParamEnd + 1;
    skipLeadingWhiteSpace( &pParam, pLineEnd );
    
    if ( pParam >= pLineEnd )
        pParam = NULL;
    else if ((*pParam == *( pLineEnd -1)) && (*pParam == '\"' || *pParam == '\''))
    {
        pParamEnd = lsr_findEndQuote( pParam + 1, pLineEnd, *pParam );
        if ( pParamEnd == pLineEnd - 1 )
        {
            ++pParam;
            --pLineEnd;
        }
    }
    
    lsr_addToList( pThis, pParam, pLineEnd - pParam );
    return &pThis->m_pList;
}

static lsr_objarray_t *lsr_conf_parse( lsr_confparser_t *pThis, const char *pConf, const char *pConfEnd, skipLeadingFn skipFn )
{
    lsr_strtrim2( &pConf, &pConfEnd );
    if ( pConf >= pConfEnd )
        return NULL;
    
    lsr_conf_reset_parser( pThis, pConfEnd - pConf + 1 );
    
    while( pConf < pConfEnd )
    {
        const char *pParam;
        const char *pParamEnd;
        
        skipFn( &pConf, pConfEnd );
        pParam = lsr_get_param( pConf, pConfEnd, &pParamEnd );
        if ( !pParam )
            return &pThis->m_pList;
        
#ifdef LSR_CONFPARSERDEBUG
        printf( "CONFPARSELINE: New Parameter: %.*s\n", pParamEnd - pParam, pParam );
#endif
        
        lsr_addToList( pThis, pParam, pParamEnd - pParam );
        pConf = pParamEnd + 1;
    }
    return &pThis->m_pList;
}

static void lsr_conf_reset_parser( lsr_confparser_t *pThis, int size )
{
    lsr_str_prealloc( &pThis->m_pStr, size );
    lsr_str_set_len( &pThis->m_pStr, 0 );
    lsr_objarray_clear( &pThis->m_pList );
}

static const char *lsr_get_param( const char *pBufBegin, const char *pBufEnd, const char **pParamEnd )
{
    switch( *pBufBegin )
    {
    case '\"': case '\'':
        *pParamEnd = lsr_findEndQuote( pBufBegin + 1, pBufEnd, *pBufBegin );
        ++pBufBegin;
        break;
    case '\r': case '\n':
        //This should only be an option for parse_line, in which case we should call this no param.
        return NULL;
    default:
        *pParamEnd = lsr_strnextarg( &pBufBegin, NULL );
        break;
    }
    if ( *pParamEnd == NULL )
    {
        *pParamEnd = pBufEnd;
        while( isspace((*pParamEnd)[-1]))
            --(*pParamEnd);
    }
    return pBufBegin;
}

static const char *lsr_findEndQuote( const char *pBegin, const char *pEnd, char ch )
{
    const char *p = memchr( pBegin, ch, pEnd - pBegin );
    if ( !p )
        return NULL;
    while( p && p < pEnd && *(p - 1) == '\\' )
    {
        ++p;
        p = memchr( p, ch, pEnd - p );
    }
    return p;
}

static void lsr_addToList( lsr_confparser_t *pThis, const char *pBegin, int len )
{
    lsr_str_t *p;
    char *pBuf = NULL;
    
    if ( lsr_objarray_getsize( &pThis->m_pList ) >= lsr_objarray_getcapacity( &pThis->m_pList ))
        lsr_objarray_guarantee( &pThis->m_pList, NULL, lsr_objarray_getcapacity( &pThis->m_pList ) + 10 );
    p = lsr_objarray_getnew( &pThis->m_pList );
    
    if ( pBegin )
    {
        pBuf = lsr_str_buf( &pThis->m_pStr ) + lsr_str_len( &pThis->m_pStr );
        
        memmove( pBuf, pBegin, len );
        *(pBuf + len) = '\0';
        lsr_str_set_len( &pThis->m_pStr, lsr_str_len( &pThis->m_pStr ) + len + 1 );
    }
    else
        len = 0;
    
    lsr_str_unsafe_set( p, pBuf, len );
}





