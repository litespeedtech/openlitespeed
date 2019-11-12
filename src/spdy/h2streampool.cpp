/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
#include "h2streampool.h"
#include "h2stream.h"
#include "unpackedheaders.h"

typedef ObjPool<H2Stream>       Pool;
static Pool s_stream_pool;

void H2StreamPool::recycle(H2Stream *pStream)
{
    pStream->reset();
    s_stream_pool.recycle(pStream);
}


H2Stream *H2StreamPool::getH2Stream()
{
    H2Stream *p;
    p = s_stream_pool.get();
    return p;
}


void H2StreamPool::recycle(H2Stream **pStream, int n)
{
    for(int i = 0; i < n; ++i)
        pStream[i]->reset();
    s_stream_pool.recycle((void **)pStream, n);
}


int H2StreamPool::getH2Streams(H2Stream **pStream, int n)
{
    int ret = s_stream_pool.get(pStream, n);
    return ret;
}

