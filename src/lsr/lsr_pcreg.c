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

#include <lsr/lsr_pcreg.h>
#include <lsr/lsr_pool.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>

#ifndef PCRE_STUDY_JIT_COMPILE
#define PCRE_STUDY_JIT_COMPILE 0
#endif

lsr_pcre_result_t *lsr_pcre_result_new()
{
    lsr_pcre_result_t *pThis = (lsr_pcre_result_t *)lsr_palloc( sizeof( lsr_pcre_result_t ) );
    return lsr_pcre_result( pThis );
}

lsr_pcre_result_t *lsr_pcre_result( lsr_pcre_result_t *pThis )
{
    if ( !pThis )
        return NULL;
    pThis->m_pBuf = NULL;
    pThis->m_matches = 0;
    return pThis;
}

void lsr_pcre_result_d( lsr_pcre_result_t *pThis )
{
    memset( pThis, 0, sizeof( lsr_pcre_result_t ) );
    //Since I did not allocate the buffer, I will not deallocate the buffer.
    pThis->m_pBuf = NULL;
}

void lsr_pcre_result_delete(lsr_pcre_result_t *pThis )
{
    lsr_pcre_result_d( pThis );
    lsr_pfree( pThis );
}

lsr_pcre_t *lsr_pcre_new()
{
    lsr_pcre_t *pThis = (lsr_pcre_t *)lsr_palloc( sizeof( lsr_pcre_t ) );
    return lsr_pcre( pThis );
}

lsr_pcre_t *lsr_pcre( lsr_pcre_t *pThis )
{
    if ( !pThis )
        return NULL;
    pThis->m_regex = NULL;
    pThis->m_extra = NULL;
    pThis->m_iSubStr = 0;
    return pThis;

}

void lsr_pcre_d( lsr_pcre_t *pThis )
{
    lsr_pcre_release( pThis );
    pThis->m_iSubStr = 0;
}

void lsr_pcre_delete( lsr_pcre_t *pThis )
{
    lsr_pcre_d( pThis );
    lsr_pfree( pThis );
}

lsr_pcre_sub_t *lsr_pcre_sub_new()
{
    lsr_pcre_sub_t *pThis = (lsr_pcre_sub_t *)lsr_palloc( sizeof( lsr_pcre_sub_t ) );
    return lsr_pcre_sub( pThis );
}

lsr_pcre_sub_t *lsr_pcre_sub( lsr_pcre_sub_t *pThis )
{
    if ( !pThis )
        return NULL;
    pThis->m_parsed = NULL;
    pThis->m_pList = NULL;
    pThis->m_pListEnd = NULL;
    return pThis;
}

lsr_pcre_sub_t *lsr_pcre_sub_copy( lsr_pcre_sub_t *dest, const lsr_pcre_sub_t *src )
{
    assert( dest && src );
    char *p = lsr_pdupstr2( src->m_parsed, (char *)src->m_pListEnd - src->m_parsed );
    if ( !p )
        return NULL;
    dest->m_parsed = p;
    dest->m_pList = (lsr_pcre_sub_entry_t *)(p + ((char *)src->m_pList - src->m_parsed));
    dest->m_pListEnd = dest->m_pList + ( src->m_pListEnd - src->m_pList);
    
    return dest;
}

void lsr_pcre_sub_d( lsr_pcre_sub_t *pThis )
{
    lsr_pfree( pThis->m_parsed );
    pThis->m_pList = NULL;
    pThis->m_pListEnd = NULL;
}

void lsr_pcre_sub_delete( lsr_pcre_sub_t *pThis )
{
    lsr_pcre_sub_d( pThis );
    lsr_pfree( pThis );
}


int lsr_pcre_result_get_substring( const lsr_pcre_result_t *pThis, int i, char **pValue )
{
    assert( pValue );
    if ( i < pThis->m_matches )
    {
        const int * pParam = &pThis->m_ovector[ i << 1 ];
        *pValue = (char *)pThis->m_pBuf + *pParam;
        return *(pParam + 1) - *pParam;
    }
    return 0;
}

#ifndef PCRE_STUDY_JIT_COMPILE
#define PCRE_STUDY_JIT_COMPILE 0
#endif

#ifdef _USE_PCRE_JIT_
#if !defined(__sparc__) && !defined(__sparc64__)
static int s_jit_key_inited = 0;
static pthread_key_t s_jit_stack_key;

void lsr_pcre_init_jit_stack()
{
    s_jit_key_inited = 1;
    pthread_key_create( &s_jit_stack_key, lsr_pcre_release_jit_stack );
}

void lsr_pcre_release_jit_stack( void * pValue)
{
    pcre_jit_stack_free((pcre_jit_stack *) pValue );
}

pcre_jit_stack * lsr_pcre_get_jit_stack()
{
    pcre_jit_stack *jit_stack;

    if ( !s_jit_key_inited )
        lsr_pcre_init_jit_stack();
    jit_stack = (pcre_jit_stack *)pthread_getspecific( s_jit_stack_key );
    if ( !jit_stack )
    {
        jit_stack = (pcre_jit_stack *)pcre_jit_stack_alloc(32*1024, 512*1024);
        pthread_setspecific( s_jit_stack_key, jit_stack );
    }
    return jit_stack;
}
#endif
#endif

int lsr_pcre_compile( lsr_pcre_t* pThis, const char* regex, int options, 
                      int matchLimit, int recursionLimit )
{
    const char * error;
    int          erroffset;
    if ( pThis->m_regex )
        lsr_pcre_release( pThis );
    pThis->m_regex = pcre_compile( regex, options, &error, &erroffset, NULL );
    if ( pThis->m_regex == NULL )
        return -1;
    pThis->m_extra = pcre_study( pThis->m_regex, 
#if defined( _USE_PCRE_JIT_)&&!defined(__sparc__) && !defined(__sparc64__) && defined( PCRE_CONFIG_JIT )
                         PCRE_STUDY_JIT_COMPILE, 
#else
                         0,
#endif
                         &error);
    if ( matchLimit > 0 )
    {
        pThis->m_extra->match_limit = matchLimit;
        pThis->m_extra->flags |= PCRE_EXTRA_MATCH_LIMIT;
    }
    if ( recursionLimit > 0 )
    {
        pThis->m_extra->match_limit_recursion = recursionLimit;
        pThis->m_extra->flags |= PCRE_EXTRA_MATCH_LIMIT_RECURSION;
    }
    pcre_fullinfo( pThis->m_regex, pThis->m_extra, 
                   PCRE_INFO_CAPTURECOUNT, &(pThis->m_iSubStr));
    ++(pThis->m_iSubStr);
    return 0;
}

void lsr_pcre_release( lsr_pcre_t* pThis )
{
    if ( pThis->m_regex )
    {
        if ( pThis->m_extra )
        {
        #if defined( _USE_PCRE_JIT_)&&!defined(__sparc__) && !defined(__sparc64__) && defined( PCRE_CONFIG_JIT )
            pcre_free_study( pThis->m_extra );
        #else
            pcre_free( pThis->m_extra );
        #endif
            pThis->m_extra = NULL;
        }
        pcre_free( pThis->m_regex );
        pThis->m_regex = NULL;
    }
}

void lsr_pcre_sub_release( lsr_pcre_sub_t* pThis )
{
    if ( pThis->m_parsed )
        lsr_pfree( pThis->m_parsed );
    pThis->m_parsed = NULL;
    pThis->m_pList = NULL;
    pThis->m_pListEnd = NULL;
}

int lsr_pcre_sub_compile( lsr_pcre_sub_t *pThis, const char *rule )
{
    if ( !rule )
        return -1;
    const char     *p = rule;
    register char   c;
    int             entries = 0;
    while( (c = *p++) != '\0' )
    {
        if ( c == '&' )
            ++entries;
        else if ( c == '$' && isdigit(*p))
        {
            ++p;
            ++entries;
        }
        else if ( c == '\\' && (*p == '$' || *p == '&'))
            ++p;
    }
    ++entries;
    int bufSize = strlen( rule ) + 1;
    bufSize = ((bufSize + 7) >>3 ) << 3;
    if ((pThis->m_parsed = lsr_prealloc( pThis->m_parsed, 
        bufSize + entries * sizeof( lsr_pcre_sub_entry_t ) )) == NULL )
        return -1;
    pThis->m_pList = (lsr_pcre_sub_entry_t *)( pThis->m_parsed + bufSize );
    memset( pThis->m_pList, 0xff, entries * sizeof( lsr_pcre_sub_entry_t ) );
    
    char *pDest = pThis->m_parsed;
    p = rule;
    lsr_pcre_sub_entry_t * pEntry = pThis->m_pList;
    pEntry->m_strBegin = 0;
    pEntry->m_strLen = 0;
    while( ( c = *p++ ) != '\0' )
    {
        if ( c == '&' )
        {
            pEntry->m_param = 0;
        }
        else if ( c == '$' && isdigit( *p ))
        {
            pEntry->m_param = *p++ - '0';
        }
        else
        {
            if ( c == '\\' && (*p == '$' || *p == '&' ))
            {
                c = *p++;
            }
            *pDest++ = c;
            ++(pEntry->m_strLen);
            continue;
        }
        ++pEntry;
        pEntry->m_strBegin = pDest - pThis->m_parsed;
        pEntry->m_strLen = 0;
    }
    *pDest = 0;
    if ( pEntry->m_strLen == 0 )
    {
        --entries;
    }
    else
        ++pEntry;
    pThis->m_pListEnd = pEntry;
    assert( pEntry - pThis->m_pList == entries );
    return 0;
}

int lsr_pcre_sub_exec( lsr_pcre_sub_t* pThis, const char* input, const int* ovector, int ovec_num, char* output, int* length )
{
    
    lsr_pcre_sub_entry_t * pEntry = pThis->m_pList;
    char *p = output;
    assert( length );
    char *pBufEnd = output + *length;
    while( pEntry < pThis->m_pListEnd )
    {
        if ( pEntry->m_strLen > 0 )
        {
            if ( p + pEntry->m_strLen < pBufEnd )
                memmove( p, pThis->m_parsed + pEntry->m_strBegin, pEntry->m_strLen );
            p += pEntry->m_strLen;
        }
        if (( pEntry->m_param >= 0 )&&( pEntry->m_param < ovec_num ))
        {
            const int * pParam = ovector + ( pEntry->m_param << 1 );
            int len = *(pParam + 1) - *pParam;
            if ( len > 0 )
            {
                if ( p + len < pBufEnd )
                    memmove( p, input + *pParam , len );
                p += len;
            }
        }
        ++pEntry;
    }
    if ( p < pBufEnd )
        *p = 0;
    *length = p - output;
    return ( p < pBufEnd ) ? 0 : -1;
}










