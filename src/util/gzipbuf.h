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
#ifndef GZIPBUF_H
#define GZIPBUF_H



#include <inttypes.h>
#include <zlib.h>
  
class VMemBuf;

class GzipBuf
{
    z_stream        m_zstr;
    //uint32_t        m_crc;
    uint32_t        m_lastFlush;
    uint32_t        m_flushWindowSize;
    short           m_type;
    short           m_streamStarted;
    VMemBuf       * m_pCompressCache;

    int process( int finish );
    int compress( const char * pBuf, int len );
    int decompress( const char * pBuf, int len );
public: 
    enum 
    {
        GZIP_UNKNOWN,
        GZIP_DEFLATE,
        GZIP_INFLATE
    };
    GzipBuf();
    ~GzipBuf();
    
    explicit GzipBuf( int type, int level );

    int getType() const {   return m_type;   }

    int init( int type, int level);
    int reinit();
    int beginStream();
    int write( const char * pBuf, int len )
    {   return ( compress( pBuf, len ) < 0 )? -1 : len;  }
    int shouldFlush()
    {   return m_zstr.total_in - m_lastFlush > m_flushWindowSize;       }
    int flush()
    {   m_lastFlush = m_zstr.total_in; return process( Z_SYNC_FLUSH );  }
    int endStream();
    int reset()
    {   return deflateReset (&m_zstr);  }

    void setFlushWindowSize( unsigned long size )
    {   m_flushWindowSize = size;       }
        
    int release();

    void setCompressCache( VMemBuf * pCache )
    {   m_pCompressCache = pCache;  }
    VMemBuf * getCompressCache() const
    {   return m_pCompressCache;    }
    int resetCompressCache();
    const char * getLastError() const
    {   return m_zstr.msg;          }

    int processFile( int type, const char * pFileName, const char * pCompressFileName );

    int compressFile( const char * pFileName, const char * pCompressFileName )
    {   return processFile( GZIP_DEFLATE, pFileName, pCompressFileName );       }
    int decompressFile( const char * pFileName, const char * pDecompressFileName )
    {   return processFile( GZIP_INFLATE, pFileName, pDecompressFileName );       }
    int isStreamStarted() const {   return m_streamStarted;     }
};

#endif
