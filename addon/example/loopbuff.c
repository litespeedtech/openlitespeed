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
#include "loopbuff.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int _loopbuff_alloc(LoopBuff* loopbuff, int size)
{  
    if ( (size == 0) && (loopbuff->m_iCapacity != 0) )
    {
        _loopbuff_dealloc(loopbuff);
    }
    else
    {
        size = ( size + LOOPBUFUNIT  ) / LOOPBUFUNIT * LOOPBUFUNIT;
        if ( size > loopbuff->m_iCapacity )
        {
            char * pBuf = (char *)malloc( size );
            if ( pBuf == NULL )
            {
                return -1;
            }
            else
            {
                int len = 0;
                if ( loopbuff->m_pBuf != NULL )
                {
                    len = _loopbuff_copyto( loopbuff, pBuf, size-1 );
                    if (len > 0)
                        _loopbuff_erasedata( loopbuff, len );
                    free(loopbuff->m_pBuf);
                }
                loopbuff->m_pBuf = loopbuff->m_pHead = pBuf ;
                loopbuff->m_pEnd = loopbuff->m_pBuf + len;
                loopbuff->m_iCapacity = size;
                loopbuff->m_pBufEnd = loopbuff->m_pBuf + size;
            }
        }
    }
    return 0;
}


void _loopbuff_dealloc(LoopBuff* loopbuff)
{
    if ( loopbuff->m_pBuf != NULL )
        free(loopbuff->m_pBuf);
    _loopbuff_init(loopbuff);
}

int _loopbuff_copyto( LoopBuff* loopbuff, char * pBuf, int size )
{
    if (( pBuf == NULL )||( size < 0 ))
    {
        return -1;
    }
    int len = _loopbuff_getdatasize(loopbuff);
    if ( len > size )
        len = size ;
    if ( len > 0 )
    {
        size = loopbuff->m_pBufEnd - loopbuff->m_pHead ;
        if ( size > len )
            size = len ;
        memmove( pBuf, loopbuff->m_pHead, size );
        pBuf += size;
        size = len - size;
        if ( size )
        {
            memmove( pBuf, loopbuff->m_pBuf, size );
        }
    }
    return len;    
    
}

int _loopbuff_getdatasize(LoopBuff* loopbuff)
{
    register int ret = loopbuff->m_pEnd - loopbuff->m_pHead;
    if ( ret >= 0 )
        return ret;
    return ret + loopbuff->m_iCapacity;
}

void _loopbuff_cleardata(LoopBuff* loopbuff)
{
    loopbuff->m_pHead = loopbuff->m_pEnd = loopbuff->m_pBuf;
}

int _loopbuff_erasedata(LoopBuff* loopbuff, int size)
{
    if ( size < 0 )
    {
        return -1;
    }
    if ( size )
    {
        int len = _loopbuff_getdatasize(loopbuff);
        if ( size >= len )
        {
            size = len ;
            loopbuff->m_pHead = loopbuff->m_pEnd = loopbuff->m_pBuf;
        }
        else
        {
            loopbuff->m_pHead += size;
            if ( loopbuff->m_pHead >= loopbuff->m_pBufEnd )
                loopbuff->m_pHead -= loopbuff->m_iCapacity;
        }
    }
    return size;
}


int _loopbuff_guarantee(LoopBuff* loopbuff, int size)
{
    int avail = _loopbuff_getfreespace(loopbuff);
    if ( size <= avail )
        return 0;
    return _loopbuff_alloc( loopbuff, size + _loopbuff_getdatasize(loopbuff) + 1);
}

int _loopbuff_contiguous(LoopBuff* loopbuff)
{
    if ( loopbuff->m_pHead > loopbuff->m_pEnd )
        return ( loopbuff->m_pHead - loopbuff->m_pEnd - 1 );
    else
        return (loopbuff->m_pHead == loopbuff->m_pBuf)?( loopbuff->m_pBufEnd - loopbuff->m_pEnd - 1): loopbuff->m_pBufEnd - loopbuff->m_pEnd;
}

void _loopbuff_used(LoopBuff* loopbuff, int size)
{
    register int avail = _loopbuff_getfreespace(loopbuff);
    if ( size > avail )
        size = avail;

    loopbuff->m_pEnd += size;
    if ( loopbuff->m_pEnd >= loopbuff->m_pBufEnd )
        loopbuff->m_pEnd -= loopbuff->m_iCapacity ;
}

int _loopbuff_append(LoopBuff* loopbuff, const char *buf, int size)
{
    if (( buf == NULL )||( size < 0 ))
    {
        return -1;
    }

    if ( size )
    {
        if ( _loopbuff_guarantee(loopbuff, size) < 0 )
            return -1 ;
        int len = _loopbuff_contiguous(loopbuff);
        if ( len > size )
            len = size;

        memmove( loopbuff->m_pEnd, buf, len );
        buf += len ;
        len = size - len;

        if ( len )
        {
            memmove( loopbuff->m_pBuf, buf, len );
        }
        _loopbuff_used(loopbuff, size);
    }
    return size;
    
}

void _loopbuff_reorder(LoopBuff* loopbuff)
{
    if ( loopbuff->m_pHead > loopbuff->m_pEnd )
    {   
        int size = _loopbuff_getdatasize(loopbuff);
        char *pBuf = (char *)malloc(size);
        _loopbuff_copyto( loopbuff, pBuf, size );
        free(loopbuff->m_pBuf);
        
        loopbuff->m_pBuf = loopbuff->m_pHead = pBuf ;
        loopbuff->m_pEnd = loopbuff->m_pBuf + size;
        loopbuff->m_iCapacity = size;
        loopbuff->m_pBufEnd = loopbuff->m_pBuf + loopbuff->m_iCapacity;
    }
}


/*
int main()
{
    int i;
    char s[52] = {0};
    int len = 0;
    LoopBuff loopbuff;
    
    _loopbuff_init(&loopbuff);
    _loopbuff_alloc(&loopbuff, 100);
    
    for (i=0; i<3; i++)
    {
        _loopbuff_append(&loopbuff, "12345678901234567890123456789012345678901234567890*", 51);
    }
    
    for (i=0; i<4; i++)
    {
        len = _loopbuff_getdata(&loopbuff, s, 51);
        _loopbuff_erasedata(&loopbuff, len);
        printf("#%d: Get %d data\n", i , len); 
    }
   
    
    printf ("Done!\n");
    return 0;
}
*/
