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
#ifndef LOOPBUFF_H
#define LOOPBUFF_H

#include <memory.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOOPBUFUNIT 64

typedef struct _LoopBuff
{
    char  * m_pBuf;
    char  * m_pBufEnd;
    char  * m_pHead;
    char  * m_pEnd;
    int     m_iCapacity;
} LoopBuff;


static inline  void _loopbuff_init(LoopBuff* loopbuff) 
{
    memset( (void *)loopbuff, 0, sizeof( LoopBuff ) );
}

/*return 0 ok, -1 means error occured */
int _loopbuff_alloc(LoopBuff* loopbuff, int size);

void _loopbuff_dealloc(LoopBuff* loopbuff);

static inline  int _loopbuff_getcapacity(LoopBuff* loopbuff)
{
    return loopbuff->m_iCapacity;
}

int _loopbuff_getdatasize(LoopBuff* loopbuff);

static inline  int _loopbuff_getfreespace(LoopBuff* loopbuff)
{
    return loopbuff->m_iCapacity - _loopbuff_getdatasize(loopbuff) -1;
}

int _loopbuff_copyto( LoopBuff* loopbuff, char * pBuf, int size );

/*return 0 ok, 1 means error occured */
int _loopbuff_append(LoopBuff* loopbuff, const char *buf, int len);

/*copy the data to buf, max size is bufLen, return the size copied */
static inline  int _loopbuff_getdata(LoopBuff* loopbuff, char *buf, int bufLen)
{
    return _loopbuff_copyto( loopbuff, buf, bufLen );
}

/*Return the first continue buffer size,  */
static inline  int _loopbuff_blockSize(LoopBuff* loopbuff)
{
    return ( loopbuff->m_pHead > loopbuff->m_pEnd ) ? loopbuff->m_pBufEnd - loopbuff->m_pHead
                                    : loopbuff->m_pEnd - loopbuff->m_pHead;
}

/*update the *buf and point to the data pointer, and update the len to correct length
return 0 means all data was return, return 1 means still has data
User should call _loopbuff_erasedata to remove this data block, 
and call this function again to get the left data*/
static inline  char* _loopbuff_getdataref(LoopBuff* loopbuff)
{
    return loopbuff->m_pHead;
}

static inline  char* _loopbuff_getdataref_end(LoopBuff* loopbuff)
{
    return loopbuff->m_pEnd;
}

/*return 0 ok, 1 means error occured */
int _loopbuff_erasedata(LoopBuff* loopbuff, int len);
void _loopbuff_cleardata(LoopBuff* loopbuff);

/*return 1 if has data */
static inline  int _loopbuff_hasdata(LoopBuff* loopbuff)
{
    return ( loopbuff->m_pHead != loopbuff->m_pEnd );
}



int _loopbuff_guarantee(LoopBuff* loopbuff, int size);
int _loopbuff_contiguous(LoopBuff* loopbuff);
void _loopbuff_used(LoopBuff* loopbuff, int size);

void _loopbuff_reorder(LoopBuff* loopbuff);


#ifdef __cplusplus
}
#endif

#endif
