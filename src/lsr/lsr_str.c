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
#include <lsr/lsr_pool.h>
#include <lsr/lsr_xpool.h>

#include <ctype.h>

static lsr_str_t* lsr_iStr_new( const char* pStr, int len, lsr_xpool_t *pool );
static lsr_str_t *lsr_iStr( lsr_str_t *pThis, const char *pStr, int len, lsr_xpool_t *pool );
static void lsr_iStr_d( lsr_str_t* pThis, lsr_xpool_t *pool );
static int lsr_str_iSetStr( lsr_str_t *pThis, const char *pStr, int len, lsr_xpool_t *pool );
static void lsr_str_iAppend( lsr_str_t *pThis, const char *pStr, const int len, lsr_xpool_t *pool );

lsr_inline void *lsr_str_do_alloc( lsr_xpool_t *pool, size_t size )
{
    return ( pool? lsr_xpool_alloc( pool, size ): lsr_palloc( size ) );
}

lsr_inline void lsr_str_do_free( lsr_xpool_t *pool, void *ptr )
{
    if ( pool )
        lsr_xpool_free( pool, ptr );
    else
        lsr_pfree( ptr );
    return;
}

lsr_str_t* lsr_str_new( const char* pStr, int len )
{
    return lsr_iStr_new( pStr, len, NULL );
}

lsr_str_t *lsr_str( lsr_str_t *pThis, const char *pStr, int len )
{
    return lsr_iStr( pThis, pStr, len, NULL );
}

lsr_str_t *lsr_str_copy( lsr_str_t *dest, const lsr_str_t *src )
{
    assert( dest && src );
    return lsr_str( dest, lsr_str_c_str( src ), lsr_str_len( src ) );
}

void lsr_str_d( lsr_str_t* pThis )
{
    lsr_iStr_d( pThis, NULL );
}

void lsr_str_delete( lsr_str_t *pThis )
{
    lsr_str_d( pThis );
    lsr_pfree( pThis );
}

char *lsr_str_prealloc( lsr_str_t *pThis, int size )
{
    char *p = (char *)lsr_prealloc( pThis->m_pStr, size );
    if ( p )
        pThis->m_pStr = p;
    return p;
}

int lsr_str_set_str( lsr_str_t *pThis, const char *pStr, int len )
{
    return lsr_str_iSetStr( pThis, pStr, len, NULL );
}

void lsr_str_append( lsr_str_t *pThis, const char *pStr, const int len )
{
    lsr_str_iAppend( pThis, pStr, len, NULL );
}

int lsr_str_cmp( const void *pVal1, const void *pVal2 )
{   
    return strncmp( 
                    ((lsr_str_t *)pVal1)->m_pStr, 
                    ((lsr_str_t *)pVal2)->m_pStr, 
                    ((lsr_str_t *)pVal1)->m_iStrLen 
                  );    
}

lsr_hash_key_t lsr_str_hf( const void *pKey )
{
    register lsr_hash_key_t __h = 0;
    register const char *p = ((lsr_str_t *)pKey)->m_pStr;
    register char ch = *(const char *)p;
    register int i = ((lsr_str_t *)pKey)->m_iStrLen;
    for ( ; i > 0 ; --i )
    {
        ch = *((const char *)p++);
        __h = __h * 31 + ( ch );
    }
    return __h;
}

int lsr_str_ci_cmp( const void *pVal1, const void *pVal2 )
{
    return strncasecmp( 
                    ((lsr_str_t *)pVal1)->m_pStr, 
                    ((lsr_str_t *)pVal2)->m_pStr, 
                    ((lsr_str_t *)pVal1)->m_iStrLen 
                  );  
}

lsr_hash_key_t lsr_str_ci_hf( const void *pKey )
{
    register lsr_hash_key_t __h = 0;
    register const char *p = ((lsr_str_t *)pKey)->m_pStr;
    register char ch = *(const char *)p;
    register int i = ((lsr_str_t *)pKey)->m_iStrLen;
    for ( ; i > 0 ; --i )
    {
        ch = *((const char *)p++);
        __h = __h * 31 + toupper( ch );
    }
    
    return __h;
}

lsr_str_t* lsr_str_new_xp( const char* pStr, int len, lsr_xpool_t *pool )
{
    return lsr_iStr_new( pStr, len, pool );
}

lsr_str_t *lsr_str_xp( lsr_str_t *pThis, const char *pStr, int len, lsr_xpool_t *pool )
{
    return lsr_iStr( pThis, pStr, len, pool );
}

lsr_str_t *lsr_str_copy_xp( lsr_str_t *dest, const lsr_str_t *src, lsr_xpool_t *pool )
{
    assert( dest && src );
    return lsr_str_xp( dest, lsr_str_c_str( src ), lsr_str_len( src ), pool );
}

void lsr_str_d_xp( lsr_str_t* pThis, lsr_xpool_t *pool )
{
    lsr_iStr_d( pThis, pool );
}

void lsr_str_delete_xp( lsr_str_t *pThis, lsr_xpool_t *pool )
{
    lsr_str_d_xp( pThis, pool );
    lsr_xpool_free( pool, pThis );
}

char *lsr_str_prealloc_xp( lsr_str_t *pThis, int size, lsr_xpool_t *pool )
{
    char *p = (char *)lsr_xpool_realloc( pool, pThis->m_pStr, size );
    if ( p )
        pThis->m_pStr = p;
    return p;
}

int lsr_str_set_str_xp( lsr_str_t *pThis, const char *pStr, int len, lsr_xpool_t *pool )
{
    return lsr_str_iSetStr( pThis, pStr, len, pool );
}

void lsr_str_append_xp( lsr_str_t *pThis, const char *pStr, const int len, lsr_xpool_t *pool )
{
    lsr_str_iAppend( pThis, pStr, len, pool );
}

lsr_str_t* lsr_iStr_new( const char* pStr, int len, lsr_xpool_t *pool )
{
    lsr_str_t *pThis;
    if ( (pThis = (lsr_str_t *)lsr_str_do_alloc( pool, sizeof( lsr_str_t ) )) != NULL )
    {
        if ( lsr_iStr( pThis, pStr, len, pool ) == NULL )
        {
            lsr_str_do_free( pool, pThis );
            return NULL;
        }
    }
    return pThis;
}

void lsr_iStr_d( lsr_str_t* pThis, lsr_xpool_t *pool )
{
    if ( pThis )
    {
        if ( pThis->m_pStr )
            lsr_str_do_free( pool, pThis->m_pStr );
        lsr_str_blank( pThis );
    }
}

static char *lsr_str_do_dupstr( const char *pStr, int len, lsr_xpool_t *pool )
{
    char *p;
    if ( pool )
    {
        if ( (p = (char *)lsr_xpool_alloc( pool, len + 1 )) == NULL )
            return NULL;
        memmove( p, pStr, len );
    }
    else if ((p = lsr_pdupstr2( pStr, len + 1 )) == NULL )
        return NULL;
    *(p + len) = 0;
    return p;
}

lsr_str_t *lsr_iStr( lsr_str_t *pThis, const char *pStr, int len, lsr_xpool_t *pool )
{
    if ( pStr == NULL )
        lsr_str_blank( pThis );
    else if ( (pThis->m_pStr = lsr_str_do_dupstr( pStr, len, pool )) == NULL )
        pThis = NULL;
    else
        pThis->m_iStrLen = len;
    return pThis;
}

int lsr_str_iSetStr( lsr_str_t *pThis, const char *pStr, int len, lsr_xpool_t *pool )
{
    assert( pThis );
    lsr_str_do_free( pool, pThis->m_pStr );
    return (( lsr_iStr( pThis, pStr, len, pool ) == NULL )? 0: pThis->m_iStrLen );
}

void lsr_str_iAppend( lsr_str_t *pThis, const char *pStr, const int len, lsr_xpool_t *pool )
{
    assert( pThis && pStr );
    char *p;
    if ( pool )
        p = (char *)lsr_xpool_realloc( pool, pThis->m_pStr, pThis->m_iStrLen + len + 1 );
    else
        p = (char *)lsr_prealloc( pThis->m_pStr, pThis->m_iStrLen + len + 1 );
    if ( p )
    {
        memmove(p + pThis->m_iStrLen, pStr, len );
        pThis->m_iStrLen += len;
        *(p + pThis->m_iStrLen) = 0;
        pThis->m_pStr = p;
    }
}
