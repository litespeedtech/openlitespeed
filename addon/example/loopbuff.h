/*
Copyright (c) 2014, LiteSpeed Technologies Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met: 

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer. 
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution. 
    * Neither the name of the Lite Speed Technologies Inc nor the
      names of its contributors may be used to endorse or promote
      products derived from this software without specific prior
      written permission.  

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

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
