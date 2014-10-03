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
#include <lsr/lsr_aho.h>
#include <lsr/lsr_pool.h>

typedef struct lsr_aho_goto_node_s lsr_aho_goto_node_t;

struct lsr_aho_goto_node_s 
{
    unsigned char        m_label;
    lsr_aho_state_t     *m_state;
    lsr_aho_goto_node_t *m_next;
};

struct lsr_aho_state_s
{
    unsigned int         m_id;
    size_t               m_output;
    lsr_aho_state_t     *m_fail;
    lsr_aho_state_t     *m_next;
    lsr_aho_goto_node_t *m_first;
    unsigned int         m_goto_size;
    unsigned char       *m_childrenLabels;
    lsr_aho_state_t    **m_childrenStates;
};


static lsr_aho_state_t *lsr_aho_init_state();
static int              lsr_aho_goto_list_insert( lsr_aho_state_t *state, lsr_aho_goto_node_t *node );
static int              lsr_aho_goto_set( lsr_aho_state_t *pFromState,unsigned char sym, lsr_aho_state_t *pToState );
static lsr_aho_state_t *lsr_aho_goto_get( lsr_aho_state_t *state,unsigned char sym, int madeTree );
static int              lsr_aho_optimize( lsr_aho_state_t *state, int case_sensitive );
static void             lsr_aho_free_goto_node( lsr_aho_goto_node_t *node );
static void             lsr_aho_free_nodes( lsr_aho_state_t *state, int case_sensitive );


lsr_aho_t* lsr_aho_new( int case_sensitive )
{
    lsr_aho_t *pThis = lsr_palloc( sizeof( lsr_aho_t ) );
    if ( lsr_aho( pThis, case_sensitive ) == 0 )
    {
        if ( pThis )
            lsr_pfree( pThis );
        return NULL;
    }
    return pThis;
}

int lsr_aho( lsr_aho_t *pThis, int case_sensitive )
{
    if ( !pThis )
        return( 0 );
    pThis->m_next_state = 1;
    pThis->m_zero_state = lsr_aho_init_state();
    if ( pThis->m_zero_state == NULL ) 
        return( 0 );
    pThis->m_case_sensitive = case_sensitive;
    return( 1 );
}

void lsr_aho_d( lsr_aho_t *pThis )
{
    if ( pThis == NULL )
        return;
    if ( pThis->m_zero_state == NULL )
        return;
    lsr_aho_free_nodes( pThis->m_zero_state, pThis->m_case_sensitive );
}

void lsr_aho_delete( lsr_aho_t *pThis )
{
    lsr_aho_d( pThis );
    lsr_pfree( pThis );
}

int lsr_aho_add_non_unique_pattern( lsr_aho_t *pThis, const char *pattern, size_t size )
{
    lsr_aho_state_t *pState,*ptr = NULL;
    size_t j = 0;
    int ret;
    unsigned char ch;

    pState = pThis->m_zero_state;
    while( j != size )
    {
        ch = *(pattern + j);
        if ( !pThis->m_case_sensitive ) 
            ch = tolower( ch );
        ptr = lsr_aho_goto_get( pState, ch, 0 );
        if ( ptr == NULL )
                break;
        if ( ptr->m_output > 0 )
        {
#ifdef LSR_AHO_DEBUG
            printf( "-----pattern=%s, this pattern can be safely ignored - its prefix is a pattern\n", pattern );
#endif
            return( 1 );
        }
        pState = ptr;
        ++j;
    }
    if ( j == size ) 
    {
        if ( ptr->m_output == 0 )
        {
            ptr->m_output = j;
            lsr_aho_free_goto_node( ptr->m_first );
            ptr->m_first = NULL;
#ifdef LSR_AHO_DEBUG
            printf( "-----pattern=%s, this pattern discards previous patterns - it's a prefix of previous ones", pattern );
#endif
            return( 1 );
        }
        else if ( ptr->m_output == j )
        {
#ifdef LSR_AHO_DEBUG
            printf( "info:same pattern already exists\n" );
#endif
            return( 1 );
        }
#ifdef LSR_AHO_DEBUG
        else
        {
            printf( "warning: impossible, logic error somewhere\n" );
            return( 0 );
        }
#endif
    }
    
    while( j != size )
    {
        ptr = lsr_aho_init_state();
        if ( ptr == NULL ) 
            return( 0 );
        ptr->m_id = pThis->m_next_state++;
        ch = *(pattern + j);
        if ( !pThis->m_case_sensitive )     
            ch = tolower( ch );
        ret = lsr_aho_goto_set( pState, ch, ptr );
        if ( ret == 0 )
        {
            lsr_pfree( ptr );
            return( 0 );
        }
        pState = ptr;
        ++j;
    }
    ptr->m_output = size;
    return( 1 );
}

int lsr_aho_add_patterns_from_file( lsr_aho_t *pThis, const char *filename )
{
    char buf[LSR_AHO_MAX_STRING_LEN], *pStart, *pEnd;
    FILE *fp;
    int ret;

    fp = fopen( filename, "rb" );
    if ( fp == NULL )
    {
#ifdef LSR_AHO_DEBUG
        printf( "failed to open file: %s\n", filename );
#endif
        return( 0 );
    }
    /* no problem when a line exceed max length -- it'll be split into multiple lines */
    while( fgets( buf, LSR_AHO_MAX_STRING_LEN, fp ) != NULL ) 
    {
        /* trim whitespace */
        pStart = buf;
        while( isspace( *pStart ) && *pStart != 0 ) 
            ++pStart;
        /* ignore empty or comment line */
        if ( *pStart == 0 || *pStart == '#' )
            continue;
        pEnd = buf + strlen( buf ) - 1;
        while( isspace( *pEnd ) )
            --pEnd;
        *(++pEnd ) = 0;    /* needed later for strstr() */
        ret = lsr_aho_add_non_unique_pattern( pThis, pStart, pEnd - pStart );/* not include the last '\0' */
        if ( ret == 0 )
            return( 0 );
    }
    fclose( fp );
    return( 1 );
}

int lsr_aho_make_tree( lsr_aho_t *pThis )
{
    lsr_aho_state_t *pState, *ptr, *pIter, *pHead, *pTail;
    lsr_aho_goto_node_t *pNode;
    unsigned char sym;

    pHead = NULL;
    pTail = NULL;
    
    /* all goto_node under zero_state */
    for( pNode = pThis->m_zero_state->m_first; pNode != NULL; pNode = pNode->m_next )
    {
        ptr = pNode->m_state;
        if ( pHead == NULL )
        {
            pHead = ptr;
            pTail = ptr;
        }
        else
        {
            pTail->m_next = ptr;
            pTail = ptr;
        }
        ptr->m_fail = pThis->m_zero_state;
    }
    // following is the most tricky part
    // set fail() for depth > 0
    for( ; pHead != NULL; pHead = pHead->m_next )
    {
        pIter = pHead;
        //now pIter's all goto_node
        for( pNode = pIter->m_first; pNode != NULL; pNode = pNode->m_next )
        {
            ptr = pNode->m_state;
            sym = pNode->m_label;
            /* no need to set leaf's fail function -- only if each pattern is unique
              if non-unique pattern allowed, have to comment off following if statement -- otherwise, may cause segment fault */ 
            //if ( ptr->m_output == 0 ){    // or ptr->m_first == NULL 
            pTail->m_next = ptr;
            pTail = ptr;
            pState = pIter->m_fail;
            while( lsr_aho_goto_get( pState, sym, 1 ) == NULL )
                pState = pState->m_fail;
            ptr->m_fail = lsr_aho_goto_get( pState, sym, 1 );
            //}
        }
    }
    return( 1 );
}

int lsr_aho_optimize_tree( lsr_aho_t* pThis )
{
    return lsr_aho_optimize( pThis->m_zero_state, pThis->m_case_sensitive );
}

unsigned int lsr_aho_search( lsr_aho_t *pThis, lsr_aho_state_t *start_state, const char *string, 
                             size_t size, size_t startpos, size_t *out_start, size_t *out_end, 
                             lsr_aho_state_t **out_last_state )
{
    lsr_aho_state_t *pZero = pThis->m_zero_state,
                    *aAccept[ LSR_AHO_MAX_FIRST_CHARS ];
    unsigned char uc;
    const char *pInputPtr;
    size_t iStringIter;
    int i, 
        iChildPtr = pZero->m_goto_size, 
        iNumChildren = pZero->m_goto_size;
    
    if ( !start_state )
        start_state = pZero;
    
    memset( aAccept, 0, sizeof( aAccept ) );
    for( i = 0; i < iNumChildren; ++i )
        aAccept[ pZero->m_childrenLabels[i] ] = pZero->m_childrenStates[i];

    for( iStringIter = startpos; iStringIter < size ; ++iStringIter )
    {
        if ( (iChildPtr == iNumChildren) && ( start_state == pZero) )
        {
            for( pInputPtr = string + iStringIter; pInputPtr < string + size; ++pInputPtr )
                if ( ( start_state = aAccept[ (int)*pInputPtr ]) != 0 )
                    break;
            if ( pInputPtr >= string + size )
            {
                *out_start = -1;
                *out_end = -1;
                *out_last_state = start_state;
                return( 0 );
            }
            iStringIter = pInputPtr - string + 1;
        }
        uc = *(string + iStringIter);
        do
        {
            iNumChildren = start_state->m_goto_size;
            for( iChildPtr = 0; iChildPtr < iNumChildren; ++iChildPtr )
                if ( uc == start_state->m_childrenLabels[ iChildPtr ] )
                    break;
            if ( iChildPtr == iNumChildren )
            {
                start_state = start_state->m_fail;
                if ( start_state == pZero )
                    break;
            }
        }
        while( iChildPtr == iNumChildren );
        if ( (iChildPtr != iNumChildren) || ( start_state != pZero) )
        {
            start_state = start_state->m_childrenStates[ iChildPtr ];
            if ( start_state->m_output != 0 ) 
            {
                *out_start = iStringIter - start_state->m_output + 1;
                *out_end = iStringIter +1;
                *out_last_state = start_state;
                return start_state->m_id;
            }
        }
    }
    *out_start = -1;
    *out_end = -1;
    *out_last_state = start_state;
    return( 0 );
}

lsr_aho_state_t *lsr_aho_init_state()
{
    lsr_aho_state_t *pThis = (lsr_aho_state_t *)lsr_palloc( sizeof( lsr_aho_state_t ) );
    if ( !pThis )
        return NULL;
    pThis->m_id = 0;
    pThis->m_output = 0;
    pThis->m_fail = NULL;
    pThis->m_next = NULL;
    pThis->m_first = NULL;
    pThis->m_goto_size = 0;
    pThis->m_childrenLabels = NULL;
    pThis->m_childrenStates = NULL;
    
    return pThis;
}

static int lsr_aho_goto_list_insert( lsr_aho_state_t *state, lsr_aho_goto_node_t *node )
{
    lsr_aho_goto_node_t *ptr, *ptr2;
    unsigned char sym = node->m_label;

    ptr = state->m_first;
    if ( ptr == NULL )
    {
        state->m_first = node;
        state->m_goto_size = 1;
        node->m_next = NULL;
        return( 1 );
    }
    if ( ptr->m_label > sym )
    {
        state->m_first = node;
        ++state->m_goto_size;
        node->m_next = ptr;
        return( 1 );
    }
    for( ptr2 = ptr; ptr != NULL && ptr->m_label < sym; ptr = ptr->m_next )
        ptr2 = ptr;
#ifdef LSR_AHO_DEBUG
    if ( p != NULL && p->m_label == sym )
    {
        printf( "impossible! fatal error\n" );
        return( 0 );
    }
#endif
    //insert between ptr2 and ptr
    ptr2->m_next = node;
    node->m_next = ptr;
    ++state->m_goto_size;
    return( 1 );
}

static int lsr_aho_goto_set( lsr_aho_state_t *pFromState, unsigned char sym, lsr_aho_state_t *pToState )
{
    lsr_aho_goto_node_t *pEdge;
    pEdge = (lsr_aho_goto_node_t *)lsr_palloc( sizeof( lsr_aho_goto_node_t ) );
    if ( pEdge == NULL )
        return( 0 );
    
    pEdge->m_label = sym;
    pEdge->m_state = pToState;
    pEdge->m_next = NULL;

    lsr_aho_goto_list_insert( pFromState, pEdge );
    return( 1 );
}

static lsr_aho_state_t *lsr_aho_goto_get( lsr_aho_state_t *state, unsigned char sym, int madeTree )
{
    lsr_aho_goto_node_t *pNode;
    unsigned char uc;

    pNode = state->m_first;
    while( pNode != NULL ) 
    {
        uc = pNode->m_label; 
        if ( uc == sym )
            return pNode->m_state;
        else if ( uc > sym )
            break;
        
        pNode = pNode->m_next;
    }
    if ( madeTree && state->m_id == 0 )
        return state;
    else
        return NULL;
}

static int lsr_aho_optimize( lsr_aho_state_t *state, int case_sensitive )
{
    lsr_aho_goto_node_t *pNextChild, *pChild = state->m_first;
    int i = 0;
    if ( state->m_first == NULL )
        return 1;
    
    if ( !case_sensitive )
        state->m_goto_size <<= 1;

    if ( (state->m_childrenLabels = (unsigned char *)lsr_palloc( 
                sizeof( unsigned char ) * (state->m_goto_size) )) == NULL )
        return 0;
    if ( (state->m_childrenStates = (lsr_aho_state_t **)lsr_palloc( 
                sizeof( lsr_aho_state_t * ) * (state->m_goto_size) )) == NULL )
        return 0;
    state->m_first = NULL;
    while( pChild != NULL )
    {
        if ( case_sensitive )
        {
            state->m_childrenLabels[i] = pChild->m_label;
            state->m_childrenStates[i] = pChild->m_state;
        }
        else
        {
            state->m_childrenLabels[i] = toupper( pChild->m_label );
            state->m_childrenStates[i++] = pChild->m_state;
            state->m_childrenLabels[i] = tolower( pChild->m_label );
            state->m_childrenStates[i] = pChild->m_state;
        }
        
        if ( lsr_aho_optimize( pChild->m_state, case_sensitive ) == 0 )
            return 0;
        ++i;
        pNextChild = pChild->m_next;
        lsr_pfree( pChild );
        pChild = pNextChild;
    }
    return 1;
}

static void lsr_aho_free_goto_node( lsr_aho_goto_node_t *node )
{
    lsr_aho_state_t *ptr;
    lsr_aho_goto_node_t *pNextNode;

    pNextNode = node->m_next;
    ptr = node->m_state;
    if ( ptr->m_first != NULL )
        lsr_aho_free_goto_node( ptr->m_first );
    lsr_pfree( ptr );
    if ( pNextNode != NULL )
        lsr_aho_free_goto_node( pNextNode );
    lsr_pfree( node );
}

static void lsr_aho_free_nodes( lsr_aho_state_t *state, int case_sensitive )
{
    // Goto nodes are deallocated in optimize.
    size_t i;
    if ( state->m_childrenStates != NULL )
    {
        for( i = 0; i < state->m_goto_size; ++i )
        {
            lsr_aho_free_nodes( state->m_childrenStates[i], case_sensitive );
            state->m_childrenStates[i] = NULL;
            if ( !case_sensitive ) //if case insensitive, skip one to prevent double freeing
                state->m_childrenStates[++i] = NULL;
        }
        lsr_pfree( state->m_childrenLabels );
        state->m_childrenLabels = NULL;
        lsr_pfree( state->m_childrenStates );
        state->m_childrenStates = NULL;
    }
    lsr_pfree( state );
}



