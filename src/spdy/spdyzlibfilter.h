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
#ifndef SPDYZLIBFILTER_H
#define SPDYZLIBFILTER_H
#include  <netinet/in.h> 
#include  <iostream>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "spdyprotocol.h"
#include "zlib.h"
#include <sys/types.h>       /* basic system data types */
#include "util/autobuf.h"
#include "util/loopbuf.h"
#include "spdydebug.h"
/**
 * Context for zlib deflating and inflating.
 * Allows to use the same zlib stream on multiple frames. (Needed
 * for inflating multiple compressed headers on a SPDY stream.)
 */

class SpdyZlibFilter
{

private:
    z_stream m_stream;
    short    m_version;
    short    m_isInflator;

private:
    SpdyZlibFilter(const SpdyZlibFilter& other);
    SpdyZlibFilter& operator=(const SpdyZlibFilter& other);
    bool operator==(const SpdyZlibFilter& other) const;
    
public:
    SpdyZlibFilter();
    ~SpdyZlibFilter();
    int init( int isInflator, int verSpdy );
    int release();
    int decompress(char* pSource, uint32_t length, AutoBuf& bufInflate);
    int compress(char* pSource, uint32_t length, LoopBuf* pBuf, int flush);
};

#endif // SPDYZLIBFILTER_H
