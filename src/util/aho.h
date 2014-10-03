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

#ifndef AHO_H
#define AHO_H

#include <lsr/lsr_aho.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>



#define MAX_STRING_LEN 8192
#define MAX_FIRST_CHARS 256


#ifdef __cplusplus
extern "C"
{
#endif
    
typedef lsr_aho_state_t AhoState;

class Aho : private lsr_aho_t
{
private:
    Aho( const Aho &rhs );
    void operator=( const Aho& rhs );
public:
    Aho( int case_insensitive )
    {   lsr_aho( this, case_insensitive );    }
    
    ~Aho()
    {   lsr_aho_d( this );  }
    
    AhoState *getZeroState()
    {   return m_zero_state;    }
    
    int addNonUniquePattern( const char *pattern, size_t size )
    {   return lsr_aho_add_non_unique_pattern( this, pattern, size );   }
    
    int addPatternsFromFile( const char *filename )
    {   return lsr_aho_add_patterns_from_file( this, filename );    }
    
    int makeTree()
    {   return lsr_aho_make_tree( this );   }
    
    int optimizeTree()
    {   return lsr_aho_optimize_tree( this );    }
    
    /* search for matches in an aho corasick tree. */
    unsigned int search( AhoState *start_state, const char *string, size_t size, size_t startpos, 
                         size_t *out_start, size_t *out_end, AhoState **out_last_state )
    {   
        return lsr_aho_search( this, start_state, string, size, startpos, 
            out_start, out_end, out_last_state );
    }
};

#ifdef __cplusplus
}
#endif


#endif
