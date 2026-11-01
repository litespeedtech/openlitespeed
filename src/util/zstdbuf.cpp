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
#include <util/zstdbuf.h>

#ifdef USE_ZSTD

#include <util/vmembuf.h>

#include <lsr/ls_fileio.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>



ZstdBuf::ZstdBuf()
    : m_pEncoder(NULL)
    , m_iAvailIn(0)
    , m_iAvailOut(0)
    , m_pNextIn(NULL)
    , m_pNextOut(NULL)
    , m_iLevel(ZSTD_CLEVEL_DEFAULT)
    , m_iLastRet(0)
{
}

ZstdBuf::ZstdBuf(int type, int level)
    : m_pEncoder(NULL)
    , m_iAvailIn(0)
    , m_iAvailOut(0)
    , m_pNextIn(NULL)
    , m_pNextOut(NULL)
    , m_iLevel(ZSTD_CLEVEL_DEFAULT)
    , m_iLastRet(0)
{
    init(type, level);
}

ZstdBuf::~ZstdBuf()
{
    release();
}

int ZstdBuf::release()
{
    if (m_iType == COMPRESSOR_DECOMPRESS)
    {
        if (m_pDecoder)
            ZSTD_freeDStream(m_pDecoder);
    }
    else
    {
        if (m_pEncoder)
            ZSTD_freeCStream(m_pEncoder);
    }
    m_pEncoder = NULL;
    return 0;
}


int ZstdBuf::getMaxLevel()
{
    return ZSTD_maxCLevel();
}


int ZstdBuf::getDefaultLevel()
{
    return ZSTD_CLEVEL_DEFAULT;
}


int ZstdBuf::init(int type, int level)
{
    if (type == COMPRESSOR_DECOMPRESS)
        m_iType = COMPRESSOR_DECOMPRESS;
    else
        m_iType = COMPRESSOR_COMPRESS;

    if (level <= 0)
        level = ZSTD_CLEVEL_DEFAULT;
    else if (level > ZSTD_maxCLevel())
        level = ZSTD_maxCLevel();
    m_iLevel = level;

    if (m_iType == COMPRESSOR_COMPRESS)
    {
        m_pEncoder = ZSTD_createCStream();
        if (m_pEncoder != NULL)
        {
            ZSTD_CCtx_setParameter(m_pEncoder, ZSTD_c_compressionLevel,
                                    m_iLevel);
            //Enable content size and checksum in the frame header, this
            //matches the behavior of the reference `zstd` CLI tool and
            //allows generic zstd-aware clients/proxies to validate the
            //stream.
            ZSTD_CCtx_setParameter(m_pEncoder, ZSTD_c_checksumFlag, 1);
        }
        return (m_pEncoder ? LS_OK : LS_FAIL);
    }
    else
    {
        m_pDecoder = ZSTD_createDStream();
        if (m_pDecoder != NULL)
            ZSTD_initDStream(m_pDecoder);
        return (m_pDecoder ? LS_OK : LS_FAIL);
    }
}

int ZstdBuf::reinit()
{
    m_iStreamStarted = 1;
    return reset();
}


int ZstdBuf::beginStream()
{
    if (!m_pCompressCache)
        return LS_FAIL;
    size_t size;

    m_pNextIn = NULL;
    m_iAvailIn = 0;

    m_pNextOut = m_pCompressCache->getWriteBuffer(size);
    m_iAvailOut = size;
    if (!m_pNextOut)
        return LS_FAIL;
    m_iLastRet = 0;
    m_iStreamStarted = 1;
    return 0;
}

int ZstdBuf::shouldFlush()
{
    //There is still buffered/pending output that the last
    //compress/decompress call was not able to fully drain into the
    //output buffer it was given.
    return (m_iLastRet > 0);
}

int ZstdBuf::compress(const char *pBuf, int len)
{
    if (!m_iStreamStarted)
        return LS_FAIL;
    m_pNextIn = pBuf;
    m_iAvailIn = len;
    return process(ZSTD_e_continue);
}

int ZstdBuf::decompress(const char *pBuf, int len)
{
    return compress(pBuf, len);
}

int ZstdBuf::process(ZSTD_EndDirective op)
{
    do
    {
        size_t size;
        size_t ret;
        if (!m_iAvailOut)
        {
            m_pNextOut = m_pCompressCache->getWriteBuffer(size);
            m_iAvailOut = size;
            if (!m_pNextOut)
                return LS_FAIL;
        }

        ZSTD_inBuffer input = { m_pNextIn, m_iAvailIn, 0 };
        ZSTD_outBuffer output = { m_pNextOut, m_iAvailOut, 0 };

        if (m_iType == COMPRESSOR_COMPRESS)
            ret = ZSTD_compressStream2(m_pEncoder, &output, &input, op);
        else
            ret = ZSTD_decompressStream(m_pDecoder, &output, &input);

        if (ZSTD_isError(ret))
        {
            m_iLastRet = (int)ret;
            return LS_FAIL;
        }

        m_iLastRet = (int)ret;

        m_pNextIn  += input.pos;
        m_iAvailIn -= input.pos;
        m_pNextOut += output.pos;
        m_iAvailOut -= output.pos;

        m_pCompressCache->writeUsed(output.pos);

        //For compression: keep looping while there is still input to
        //consume, or (for flush/end) while the encoder still has data
        //buffered that needs to be drained (ret > 0 means "call again").
        //For decompression: keep looping while there is still input.
        if (m_iType == COMPRESSOR_COMPRESS)
        {
            if (m_iAvailIn == 0 &&
                (op == ZSTD_e_continue || m_iLastRet == 0))
                return 0;
        }
        else
        {
            if (m_iAvailIn == 0)
                return 0;
        }
    }
    while (true);
}


int ZstdBuf::isStreamFinished()
{
    if (m_iType == COMPRESSOR_COMPRESS)
        return (m_iLastRet == 0) ? LS_OK : LS_FAIL;
    //A return value of 0 from ZSTD_decompressStream indicates a full
    //zstd frame has been flushed and there is no more buffered output
    //pending.
    return (m_iLastRet == 0 && m_iAvailIn == 0) ? LS_OK : LS_FAIL;
}


int ZstdBuf::endStream()
{
    if (m_iType == COMPRESSOR_COMPRESS)
    {
        m_pNextIn = NULL;
        m_iAvailIn = 0;
        process(ZSTD_e_end);
    }
    m_iStreamStarted = 0;
    if (isStreamFinished() != LS_OK)
        return LS_FAIL;
    return 0;
}

int ZstdBuf::reset()
{
    if (m_iType == COMPRESSOR_COMPRESS)
    {
        if (m_pEncoder != NULL)
            ZSTD_CCtx_reset(m_pEncoder, ZSTD_reset_session_only);
        else
            m_pEncoder = ZSTD_createCStream();
        if (m_pEncoder)
            ZSTD_CCtx_setParameter(m_pEncoder, ZSTD_c_compressionLevel,
                                    m_iLevel);
        m_iLastRet = 0;
        return (m_pEncoder ? LS_OK : LS_FAIL);
    }
    else
    {
        if (m_pDecoder != NULL)
            ZSTD_DCtx_reset(m_pDecoder, ZSTD_reset_session_only);
        else
            m_pDecoder = ZSTD_createDStream();
        m_iLastRet = 0;
        return (m_pDecoder ? LS_OK : LS_FAIL);
    }
}

int ZstdBuf::resetCompressCache()
{
    m_pCompressCache->rewindReadBuf();
    m_pCompressCache->rewindWriteBuf();
    size_t size;
    m_pNextOut = m_pCompressCache->getWriteBuffer(size);
    m_iAvailOut = size;
    return 0;
}


const char *ZstdBuf::getLastError() const
{
    if (m_iType == COMPRESSOR_COMPRESS)
        return NULL;
    if (ZSTD_isError((size_t)m_iLastRet))
        return ZSTD_getErrorName((size_t)m_iLastRet);
    return NULL;
}


#endif // USE_ZSTD
