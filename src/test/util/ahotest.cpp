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
#ifdef RUN_TEST

#include <iostream>
#include "ahotest.h"
#include "test/unittest-cpp/UnitTest++/src/UnitTest++.h"

TEST( AhoTest )
{
    int iNumTests = 8;
    size_t iOutStart = 0, iOutEnd = 0;
    Aho *ac;
    AhoState **outlast = (AhoState **)malloc( sizeof( AhoState ** ) );
    
    std::cout << "Start Aho Test" << std::endl;
    
    for( int i = 0; i < iNumTests; ++i )
    {
        ac = getTree(aTestAccept[i], aTestAcceptLen[i], aSensitive[i]);
        if (ac == NULL)
        {
            std::cout << "Get Tree failed." << std::endl;
            return;
        }
        
        ac->search( NULL, aTestInput[i], aTestInputLen[i], 0, &iOutStart, &iOutEnd, outlast );
                
        CHECK( iOutStart == aOutStartRes[i] && iOutEnd == aOutEndRes[i] );
        iOutStart = 0;
        iOutEnd = 0;
        delete ac;
    }
    free( outlast );
}

Aho *getTree( const char *acceptBuf[], int bufCount, int sensitive )
{
    int i;
    Aho *ac = new Aho( sensitive );
    
    for( i = 0; i < bufCount; ++i )
    {
        if ((ac->addNonUniquePattern( acceptBuf[i], strlen( acceptBuf[i] ) )) == 0 )
        {
            std::cout << "Add patterns failed." << std::endl;
            delete( ac );
            return NULL; 
        }
    }
    
    if ( ac->makeTree() == 0 )
    {
        std::cout << "Make tree failed." << std::endl;
        delete( ac );
        return NULL; 
    }
    
    if ( ac->optimizeTree() == 0 )
    {
        std::cout << "Optimize failed." << std::endl;
        delete( ac );
        return NULL; 
    }

    return ac;
}


#endif


