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
#include "chunkoutputstream.h"

#include <util/iovec.h>

#include <assert.h>
#include <stdio.h>
#include <sys/uio.h>
#include <util/ssnprintf.h>

static const char* s_CRLF = "\r\n" ;
static const char* s_CHUNK_END = "0\r\n\r\n";

ChunkOutputStream::ChunkOutputStream()
    : m_pOS( NULL )
    , m_pIov( NULL )
    , m_iCurSize( 0 )
    , m_iBuffering( 0 )
    , m_iTotalPending( 0 )
{
}

ChunkOutputStream::~ChunkOutputStream()
{}


int ChunkOutputStream::buildIovec( IOVec& iovec, const char * pBuf, int size )
{
    int total = safe_snprintf( m_headerBuf,
                    CHUNK_HEADER_SIZE, "%x\r\n", size + m_iCurSize );
    iovec.append( m_headerBuf, total );
    if ( m_iCurSize )
    {
        iovec.append( m_bufChunk, m_iCurSize );
        total += m_iCurSize;
    }
    if ( pBuf )
    {
        iovec.append( (void *)pBuf, size );
        total += size;
    }
    iovec.append( s_CRLF, 2 );
    return total + 2;
}

int ChunkOutputStream::fillChunkBuf( const char * pBuf, int size )
{
    memmove( m_bufChunk + m_iCurSize, pBuf, size );
    m_iCurSize += size;
    return size;
}


int ChunkOutputStream::chunkedWrite( IOVec &iov, int &headerTotal,
                            const char * pBuf, int size)
{
    //printf( "****ChunkOutputStream::chunkedWrite()\n" );
    IOVec::iterator iter = iov.end();
    int bytes = buildIovec( iov, pBuf, size );
    int total = bytes + headerTotal;
    int ret = m_pOS->writev( iov, total );
    assert( m_iCurSize == 0 );
    if ( ret >= total )
    {
        iov.clear();
        headerTotal = 0;
        m_iCurSize = 0;
        return size;
    }
    else if ( ret >= 0 )
    {
        if( ret >= headerTotal )
        {
            if ( ret == headerTotal )
            {
                iov.clear();
            }
            else
            {
                ret -= headerTotal;
                m_iTotalPending = bytes - ret;
                iov.setBegin( iter );
                iov.finish( ret );
                m_pLastBufBegin = pBuf;
                m_iLastBufLen = size;
            }
            headerTotal = 0;
        }
        else
        {
            if ( ret )
            {
                headerTotal -= ret;
                iov.finish( ret );
            }
            iov.setEnd( iter );
        }
        return 0;
    }
    else
        iov.setEnd( iter );
    return ret;
}

int ChunkOutputStream::chunkedWrite( IOVec &iov, const char * pBuf, int size)
{
    //printf( "****ChunkOutputStream::chunkedWrite()\n" );
    m_iTotalPending = buildIovec( iov, pBuf, size );
    int ret = flush2();
    switch( ret )
    {
    case 0:
        m_iLastBufLen = 0;
        return size;
    case 1:
        m_pLastBufBegin = pBuf;
        m_iLastBufLen = size;
        return 0;
    default:
        return ret;
    }
}

int ChunkOutputStream::write( IOVec& iov, int &headerTotal,
                              const char * pBuf, int size )
{
    //printf( "****ChunkOutputStream::write()\n" );
    int ret, left = size;
    if (( pBuf == NULL )||( size <= 0 ))
        return 0;
    assert( m_pOS != NULL );
    assert( m_pIov == &iov );
    if ( m_iTotalPending )
    {
        ret = flush2();
        switch( ret )
        {
        case 0:
            left -= m_iLastBufLen;
            pBuf += m_iLastBufLen;
            if ( !left )
                return size;
            break;
        case 1:
            return 0;
        default:
            return ret;
        }
    }
    int chunkSize;
    do
    {
        if ( left + m_iCurSize > MAX_CHUNK_SIZE )
            chunkSize = MAX_CHUNK_SIZE - m_iCurSize;
        else
            chunkSize = left;
        if ( headerTotal )
            ret = chunkedWrite( iov, headerTotal, pBuf, chunkSize );
        else
        {
            if ((m_iBuffering)&&( chunkSize + m_iCurSize < CHUNK_BUFSIZE ))
            {
                ret = fillChunkBuf( pBuf, chunkSize );
                return size;
            }
            ret = chunkedWrite( iov, pBuf, chunkSize );
        }
        if ( ret > 0 )
        {
            left -= ret;
            if ( !left )
                return size;
            pBuf += ret;
            assert( left > 0 );
        }
        else if ( !ret )
        {
            return size - left;
        }
        else
            return ret;
    }while( 1 );
}


void ChunkOutputStream::reset()
{
    memset( &m_iCurSize, 0, (char *)&m_iLastBufLen - (char*)&m_iCurSize );
}

int ChunkOutputStream::flush2()
{
    int ret = m_pOS->writev( *m_pIov, m_iTotalPending );
    if ( ret >= m_iTotalPending )
    {
        m_pIov->clear();
        m_iCurSize = 0;
        m_iTotalPending = 0;
        return 0;
    }
    else if ( ret >= 0 )
    {
        if ( ret )
        {
            m_iTotalPending -= ret;
            m_pIov->finish( ret );
        }
        return 1;
    }
    return -1;
}

int ChunkOutputStream::close()
{
    IOVec iov;
    return close( iov, 0 );
}

int ChunkOutputStream::close( IOVec &iov, int total)
{
    if ( m_iCurSize > 0 )
    {
        m_iTotalPending += buildIovec( *m_pIov, NULL, 0 );
    }
    m_iTotalPending += total + 5;
    m_pIov->append( s_CHUNK_END, 5 );
    return flush2();
}
