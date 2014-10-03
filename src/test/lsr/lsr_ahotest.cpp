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
#ifdef RUN_TEST
 
#include "lsr_ahotest.h"

#include <stdio.h>
#include <lsr/lsr_pool.h>
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

TEST( lsr_AhoTest_test )
{
    int iNumTests = 9;
    size_t iOutStart = 0, iOutEnd = 0;
    lsr_aho_t *pThis;
    lsr_aho_state_t **outlast = (lsr_aho_state_t **)lsr_palloc( sizeof( lsr_aho_state_t ** ) );
#ifdef LSR_AHO_DEBUG
    printf( "Start lsr Aho Test" );
#endif
    for( int i = 0; i < iNumTests; ++i )
    {
        pThis = lsr_aho_initTree( lsr_aho_TestAccept[i], lsr_aho_TestAcceptLen[i], lsr_aho_Sensitive[i] );
        if ( pThis == NULL )
        {
            printf( "Init Tree failed." );
            lsr_pfree( outlast );
            return;
        }
        
        lsr_aho_search( pThis, NULL, lsr_aho_TestInput[i], lsr_aho_TestInputLen[i], 0, &iOutStart, &iOutEnd, outlast );
        
        CHECK( iOutStart == lsr_aho_OutStartRes[i] && iOutEnd == lsr_aho_OutEndRes[i] );
        iOutStart = 0;
        iOutEnd = 0;
        lsr_aho_delete( pThis );
    }
    lsr_pfree( outlast );
    
}

lsr_aho_t *lsr_aho_initTree( const char *acceptBuf[], int bufCount, int sensitive )
{
    int i;
    lsr_aho_t *pThis;
    
    if ( (pThis = lsr_aho_new( sensitive )) == NULL )
    {
        printf( "Init failed." );
        return NULL;
    }
    for( i = 0; i < bufCount; ++i )
    {
        if ( (lsr_aho_add_non_unique_pattern( pThis, acceptBuf[i], strlen( acceptBuf[i] ) )) == 0 )
        {
            printf( "Add patterns failed." );
            lsr_aho_delete( pThis );
            return NULL; 
        }
    }
    if ( (lsr_aho_make_tree( pThis )) == 0 )
    {
        printf( "Make tree failed." );
        lsr_aho_delete( pThis );
        return NULL; 
    }
    if ( (lsr_aho_optimize_tree( pThis )) == 0 )
    {
        printf( "Optimize failed." );
        lsr_aho_delete( pThis );
        return NULL; 
    }
    return pThis;
}

#endif