/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2026  LiteSpeed Technologies, Inc.                 *
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
#ifdef RUN_TEST

#include <h2/unpackedheaders.h>
#include <http/chunkinputstream.h>
#include <http/httpmethod.h>
#include <http/httprange.h>
#include <http/httpstatuscode.h>
#include <lsdef.h>
#include <lsr/ls_xpool.h>

#include "unittest-cpp/UnitTest++.h"

#include <string.h>
#include <sys/uio.h>

#include <string>
#include <vector>

class SecurityTestIS : public CacheableIS
{
    std::vector<std::string> m_data;

public:
    void addData(const char *pData, int len)
    {
        m_data.push_back(std::string(pData, len));
    }

    int read(char *pBuf, int size)
    {
        while (!m_data.empty() && m_data[0].empty())
            m_data.erase(m_data.begin());
        if (m_data.empty())
            return LS_FAIL;

        int len = size;
        if ((int)m_data[0].size() < len)
            len = m_data[0].size();
        memmove(pBuf, m_data[0].c_str(), len);
        m_data[0].erase(0, len);
        return len;
    }

    int readv(struct iovec *vector, int count)
    {
        int total = 0;
        const struct iovec *pEnd = vector + count;
        while (vector < pEnd)
        {
            if (vector->iov_len == 0)
            {
                ++vector;
                continue;
            }
            int ret = read((char *)vector->iov_base, vector->iov_len);
            if (ret > 0)
                total += ret;
            if (ret == (int)vector->iov_len)
            {
                ++vector;
                continue;
            }
            return total ? total : ret;
        }
        return total;
    }

    int readLine(char *pBuf, int size)
    {
        int ret = read(pBuf, size);
        if (ret <= 0)
            return ret;
        char *p = (char *)memchr(pBuf, '\n', ret);
        if (p)
            return p - pBuf + 1;
        return ret;
    }

    int cacheInput(int preferredLen)
    {
        return 0;
    }
};

static int readChunkCorpus(const char *data, int len, char *out, int outLen)
{
    SecurityTestIS testIS;
    ChunkInputStream chunkIS;
    testIS.addData(data, len);
    chunkIS.setStream(&testIS);
    chunkIS.open();

    int total = 0;
    while (total < outLen)
    {
        int ret = chunkIS.read(out + total, outLen - total);
        if (ret > 0)
            total += ret;
        if (ret < 0 || chunkIS.eos())
            break;
        if (ret == 0)
            break;
    }
    chunkIS.close();
    return total;
}

static lsxpack_err_code processH2Header(UpkdHdrBuilder &builder,
                                        const char *name, unsigned nameLen,
                                        const char *val, unsigned valLen)
{
    lsxpack_header *hdr = builder.prepareDecode(NULL, nameLen + valLen + 4);
    CHECK(hdr != NULL);
    if (!hdr)
        return LSXPACK_ERR_NOMEM;

    char *buf = hdr->buf;
    unsigned off = hdr->name_offset;
    memcpy(buf + off, name, nameLen);
    memcpy(buf + off + nameLen, ": ", 2);
    memcpy(buf + off + nameLen + 2, val, valLen);
    memcpy(buf + off + nameLen + 2 + valLen, "\r\n", 2);

    lsxpack_header_set_offset2(hdr, buf, off, nameLen,
                               off + nameLen + 2, valLen);
    return builder.process(hdr);
}

static void addH2RequiredPseudoHeaders(UpkdHdrBuilder &builder)
{
    CHECK_EQUAL(LSXPACK_OK,
                processH2Header(builder, ":method", 7, "GET", 3));
    CHECK_EQUAL(LSXPACK_OK,
                processH2Header(builder, ":scheme", 7, "https", 5));
    CHECK_EQUAL(LSXPACK_OK,
                processH2Header(builder, ":authority", 10,
                                "example.com", 11));
    CHECK_EQUAL(LSXPACK_OK, processH2Header(builder, ":path", 5, "/", 1));
}

static lsxpack_err_code processH2RegularHeader(const char *name,
                                               unsigned nameLen,
                                               const char *val,
                                               unsigned valLen)
{
    UnpackedHeaders *headers = new UnpackedHeaders();
    UpkdHdrBuilder builder(headers, false);
    addH2RequiredPseudoHeaders(builder);
    return processH2Header(builder, name, nameLen, val, valLen);
}

static bool isH2LowerTokenChar(unsigned char ch)
{
    if (ch == '\0')
        return false;
    if (ch >= 'a' && ch <= 'z')
        return true;
    if (ch >= '0' && ch <= '9')
        return true;
    return strchr("!#$%&'*+-.^_`|~", ch) != NULL;
}

TEST(SecurityRegression_H2HeaderCharacterCorpus)
{
    for (int i = 0; i < 256; ++i)
    {
        char name = (char)i;
        lsxpack_err_code err = processH2RegularHeader(&name, 1, "v", 1);
        if (i >= 'A' && i <= 'Z')
            CHECK_EQUAL(LSXPACK_ERR_UPPERCASE_HEADER, err);
        else if (isH2LowerTokenChar((unsigned char)i))
            CHECK_EQUAL(LSXPACK_OK, err);
        else
            CHECK_EQUAL(LSXPACK_ERR_BAD_REQ_HEADER, err);
    }

    for (int i = 0; i < 256; ++i)
    {
        char val = (char)i;
        lsxpack_err_code err = processH2RegularHeader("x-test", 6, &val, 1);
        if (i == '\t' || i >= 0x20)
        {
            if (i == 0x7f)
                CHECK_EQUAL(LSXPACK_ERR_BAD_REQ_HEADER, err);
            else
                CHECK_EQUAL(LSXPACK_OK, err);
        }
        else
            CHECK_EQUAL(LSXPACK_ERR_BAD_REQ_HEADER, err);
    }
}

TEST(SecurityRegression_H2PseudoHeaderRewriteCorpus)
{
    {
        UnpackedHeaders *headers = new UnpackedHeaders();
        UpkdHdrBuilder builder(headers, false);
        CHECK_EQUAL(LSXPACK_OK, processH2Header(builder, ":path", 5, "/", 1));
        CHECK_EQUAL(LSXPACK_OK,
                    processH2Header(builder, ":scheme", 7, "https", 5));
        CHECK_EQUAL(LSXPACK_OK,
                    processH2Header(builder, ":authority", 10,
                                    "example.com", 11));
        CHECK_EQUAL(LSXPACK_OK, processH2Header(builder, ":method", 7,
                                                "A", 1));
        CHECK_EQUAL(1, headers->getMethodLen());
        CHECK_EQUAL(0, memcmp(headers->getMethod(), "A", 1));
        CHECK_EQUAL(1, headers->getUrlLen());
        CHECK_EQUAL(0, memcmp(headers->getUrl(), "/", 1));
    }

    {
        UnpackedHeaders *headers = new UnpackedHeaders();
        UpkdHdrBuilder builder(headers, false);
        CHECK_EQUAL(LSXPACK_OK, processH2Header(builder, ":path", 5, "/", 1));
        std::string method(HttpMethod::MAX_METHOD_LEN + 1, 'A');
        CHECK_EQUAL(LSXPACK_ERR_BAD_REQ_HEADER,
                    processH2Header(builder, ":method", 7,
                                    method.data(), method.size()));
    }

    {
        static const char pathLf[] = { '/', '\n', 'x' };
        static const char valueLf[] = { 'a', '\n', 'b' };
        UnpackedHeaders *headers = new UnpackedHeaders();
        UpkdHdrBuilder builder(headers, false);
        CHECK_EQUAL(LSXPACK_ERR_BAD_REQ_HEADER,
                    processH2Header(builder, ":path", 5, pathLf,
                                    sizeof(pathLf)));
        CHECK_EQUAL(LSXPACK_ERR_BAD_REQ_HEADER,
                    processH2RegularHeader("x-test", 6, valueLf,
                                           sizeof(valueLf)));
    }
}

TEST(SecurityRegression_ChunkInputCorpus)
{
    char out[64];
    static const char valid[] = "1\r\nA\r\n0\r\n\r\n";
    CHECK_EQUAL(1, readChunkCorpus(valid, sizeof(valid) - 1, out, sizeof(out)));
    CHECK_EQUAL('A', out[0]);

    static const char *invalid[] =
    {
        "\n",
        "\r\n",
        "80000000\r\n",
        "ffffffffffffffffffffffffffffffff\r\n",
        "4g\r\nbody\r\n",
        "1\r\nA",
    };

    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        memset(out, 0, sizeof(out));
        int ret = readChunkCorpus(invalid[i], strlen(invalid[i]),
                                  out, sizeof(out));
        CHECK(ret >= 0);
        CHECK(ret <= 1);
    }
}

TEST(SecurityRegression_HttpRangeCorpus)
{
    ls_xpool_t *pool = ls_xpool_new();
    CHECK(pool != NULL);

    HttpRange range(2000);
    std::string huge = "bytes=" + std::string(80, '9') + "-";
    CHECK_EQUAL(SC_416, range.parse(huge.c_str(), pool));

    CHECK_EQUAL(SC_200, range.parse("bytes=0-1999,0-1999", pool));
    CHECK_EQUAL(SC_400, range.parse("bytes=--10", pool));
    CHECK_EQUAL(SC_400, range.parse("bytes=1-2,,", pool));

    CHECK_EQUAL(0, range.parse("bytes=0-0,2-3", pool));
    range.beginMultipart();
    char full[256];
    int fullLen = range.getPartHeader(0, "text/plain", full, sizeof(full));
    CHECK(fullLen > 0);
    char small[256];
    CHECK_EQUAL(LS_FAIL, range.getPartHeader(0, "text/plain",
                                             small, fullLen - 1));

    ls_xpool_delete(pool);
}

#endif
