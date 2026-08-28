/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
#ifndef ZSTDBUF_H
#define ZSTDBUF_H

#include <config.h>

#ifdef USE_ZSTD
#include <lsdef.h>
#include <util/compressor.h>

#include <zstd.h>


class VMemBuf;

class ZstdBuf : public Compressor
{
    union {
        ZSTD_CStream   *m_pEncoder;
        ZSTD_DStream   *m_pDecoder;
    };
    size_t          m_iAvailIn;
    size_t          m_iAvailOut;
    const char     *m_pNextIn;
    char           *m_pNextOut;
    int             m_iLevel;
    int             m_iLastRet;     //last return code from ZSTD_compressStream2/decompressStream

    int process(ZSTD_EndDirective op);
    int compress(const char *pBuf, int len);
    int decompress(const char *pBuf, int len);

    int isStreamFinished();
public:
    ZstdBuf();
    ~ZstdBuf();

    explicit ZstdBuf(int type, int level);

    int getType() const {   return m_iType;   }

    int init(int type, int level);
    int reinit();
    int beginStream();
    int write(const char *pBuf, int len)
    {   return (compress(pBuf, len) < 0) ? -1 : len;  }
    int shouldFlush();
    int flush()
    {   return process(ZSTD_e_flush); }
    int endStream();
    int reset();

    int release();

    int resetCompressCache();
    const char *getLastError() const;

    static int getMaxLevel();
    static int getDefaultLevel();

    LS_NO_COPY_ASSIGN(ZstdBuf);
};

#endif // USE_ZSTD

#endif
