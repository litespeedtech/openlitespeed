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
#ifndef XPOOL_H
#define XPOOL_H

#include <lsr/lsr_xpool.h>

#include <inttypes.h>

class XPool : private lsr_xpool_t
{
public:
    XPool()
    {
        lsr_xpool_init( this );
    }
    
    ~XPool()
    {
        lsr_xpool_reset( this );
    }
    
    void reset()
    {   lsr_xpool_reset( this );    }
    
    void *alloc( uint32_t size )
    {   return lsr_xpool_alloc( this, size );   }
    
    void *realloc( void *pOld, uint32_t new_size )
    {   return lsr_xpool_realloc( this, pOld, new_size);    }
    
    void free( void *data )
    {   lsr_xpool_free( this, data );   }
};

#endif
