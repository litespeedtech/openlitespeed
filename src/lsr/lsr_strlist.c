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
#include <lsr/lsr_strlist.h>
#include <lsr/lsr_str.h>
#include <lsr/lsr_strtool.h>
#include <stdlib.h>
#include <string.h>

void lsr_strlist_copy( lsr_strlist_t *pThis, const lsr_strlist_t *pRhs )
{
    lsr_strlist( pThis, 0 );
    lsr_const_strlist_iter iter =
      (lsr_const_strlist_iter)lsr_strlist_begin( (lsr_strlist_t *)pRhs );
    for( ; iter != lsr_strlist_end( (lsr_strlist_t *)pRhs ); ++iter )
    {
        lsr_strlist_add( pThis,
          lsr_str_c_str( *iter ), lsr_str_len( *iter ) );
    }
}

void lsr_strlist_d( lsr_strlist_t *pThis )
{
    lsr_strlist_release_objects( pThis );
    lsr_ptrlist_d( pThis );
}

const lsr_str_t * lsr_strlist_add(
  lsr_strlist_t *pThis, const char *pStr, int len )
{
    lsr_str_t * pTemp = lsr_str_new( pStr, len );
    if ( !pTemp )
        return NULL;
    lsr_strlist_push_back( pThis, pTemp );
    return pTemp;
}

void lsr_strlist_remove( lsr_strlist_t *pThis, const char *pStr )
{
    lsr_strlist_iter iter = lsr_strlist_begin( pThis );
    for( ; iter != lsr_strlist_end( pThis ); ++iter )
    {
        if ( strcmp ( lsr_str_c_str( *iter ), pStr ) == 0 )
        {
            lsr_str_delete( *iter );
            lsr_strlist_erase( pThis, iter );
            break;
        }
    }
}

void lsr_strlist_clear( lsr_strlist_t *pThis )
{
    lsr_strlist_release_objects( pThis );
}

static int compare( const void *val1, const void *val2 )
{
    return strcmp( lsr_str_c_str( *(const lsr_str_t **)val1 ),
                   lsr_str_c_str( *(const lsr_str_t **)val2 ) );
}

void lsr_strlist_sort( lsr_strlist_t *pThis )
{
    qsort( lsr_strlist_begin( pThis ),
      lsr_strlist_size( pThis ), sizeof(lsr_str_t *), compare );
}


void lsr_strlist_insert( lsr_strlist_t *pThis, lsr_str_t *pDir )
{
    lsr_strlist_push_back( pThis, pDir );
    lsr_strlist_sort( pThis );
}

const lsr_str_t * lsr_strlist_find(
  const lsr_strlist_t *pThis, const char *pStr )
{
    if ( pStr )
    {
        lsr_const_strlist_iter iter =
          (lsr_const_strlist_iter)lsr_strlist_begin( (lsr_strlist_t *)pThis );
        for( ; iter != lsr_strlist_end( (lsr_strlist_t *)pThis ); ++iter )
        {
            if ( strcmp( lsr_str_c_str( *iter ), pStr ) == 0 )
                return *iter;
        }
    }
    return NULL;
}

lsr_const_strlist_iter lsr_strlist_lower_bound(
  const lsr_strlist_t *pThis, const char *pStr )
{
    if ( !pStr )
        return NULL;
    lsr_const_strlist_iter e =
      (lsr_const_strlist_iter)lsr_strlist_end( (lsr_strlist_t *)pThis );
    lsr_const_strlist_iter b =
      (lsr_const_strlist_iter)lsr_strlist_begin( (lsr_strlist_t *)pThis );
    while( b < e )
    {
        lsr_const_strlist_iter m = b + (e - b) / 2;
        int c = strcmp( pStr, lsr_str_c_str( *m ) );
        if ( c == 0 )
            return m;
        else if ( c < 0 )
            e = m;
        else
            b = m+1;
    }
    return b;
}

lsr_str_t * lsr_strlist_bfind(
  const lsr_strlist_t *pThis, const char *pStr )
{
    lsr_const_strlist_iter p = lsr_strlist_lower_bound( (lsr_strlist_t *)pThis, pStr );
    if ( ( p != lsr_strlist_end( (lsr_strlist_t *)pThis ) )
      && ( strcmp( pStr, lsr_str_c_str( *p ) ) == 0 ) )
        return *p;
    return NULL;
}

int lsr_strlist_split(
  lsr_strlist_t *pThis, const char *pBegin, const char *pEnd, const char *delim )
{
    const char * p;
    lsr_parse_t strparse;
    lsr_parse( &strparse, pBegin, pEnd, delim );
    while( !lsr_parse_isend( &strparse ) )
    {
        p = lsr_parse_trim_parse( &strparse );
        if ( !p )
            break;
        if ( p != lsr_parse_getstrend( &strparse ) )
        {
            if ( !lsr_strlist_add(
              pThis, p, lsr_parse_getstrend( &strparse ) - p ) )
                break;
        }
    }
    return lsr_strlist_size( pThis );
}

