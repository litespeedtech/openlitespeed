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
#include "http/httprespheaders.h"
#include <arpa/inet.h>
#include <http/httpserverversion.h>
#include <http/httpver.h>
#include <log4cxx/logger.h>
#include <lshpack/lshpack.h>
#include <socket/gsockaddr.h>
#include <util/datetime.h>
#include <util/stringtool.h>
#include <h2/unpackedheaders.h>
#include <http/httpcgitool.h>
#include <lsqpack.h>

#include <ctype.h>
#include <util/stringlist.h>

//FIXME: need to update the comments
/*******************************************************************************
 *          Something  about lsxpack_header
 * keyOff is the real offset of the key/name,  valOff is the real offset of the value
 * chain_next_idx is 0 for sigle line casel
 * FOR MULTIPLE LINE CASE, chain_next_idx is relative with the next kvpair position
 * The chain_next_idx of the first KVpair of the multiple line is (the next kvpair position + 1)
 * Then in all the chlidren KVPair, the chain_next_idx is (the next kvpair position + 1) | 0x8000
 * The last one (does not have a child) is 0x80000000
 * ****************************************************************************/

#define HIGHEST_BIT_NUMBER                  0x8000
#define BYPASS_HIGHEST_BIT_MASK             0x7FFF

#ifdef _ENTERPRISE_
int  HttpRespHeaders::s_showServerHeader = 1;
#endif
static char s_sConnKeepAliveHeader[57] =
    "Connection: Keep-Alive\r\nKeep-Alive: timeout=5, max=100\r\n";
static char s_sConnCloseHeader[20] = "Connection: close\r\n";
static char s_sChunkedHeader[29] = "Transfer-Encoding: chunked\r\n";
static char s_sGzipEncodingHeader[48] =
    "content-encoding: gzip\r\nvary: Accept-Encoding\r\n";
static char s_sBrEncodingHeader[46] =
    "content-encoding: br\r\nvary: Accept-Encoding\r\n";
static char s_sCommonHeaders[66] =
    "date: Tue, 09 Jul 2013 13:43:01 GMT\r\nserver";
static char s_sTurboCharged[66] =
    "x-turbo-charged-by: ";

static int             s_commonHeadersCount = 2;
static http_header_t   s_commonHeaders[2];
static http_header_t   s_gzipHeaders[2];
static http_header_t   s_brHeaders[2];
static http_header_t   s_keepaliveHeader[2];
static http_header_t   s_chunkedHeader;
static http_header_t   s_concloseHeader;
static http_header_t   s_turboChargedBy;

const char *s_sHeaders[HttpRespHeaders::H_HEADER_END] =
{
    "Accept-Ranges",
    "Connection",
    "Content-Type",
    "Content-Length",
    "Content-Encoding",
    "Content-Range",
    "Content-Disposition",
    "Cache-Control",
    "Date",
    "Etag",
    "Expires",
    "Keep-Alive",
    "Last-Modified",
    "Location",
    "X-Litespeed-Location",
    "X-Litespeed-Cache-Control",
    "Pragma",
    "Proxy-Connection",
    "Server",
    "Set-Cookie",
    "Status",
    "Transfer-Encoding",
    "Vary",
    "Www-Authenticate",
    "X-Litespeed-Cache",
    "X-LiteSpeed-Purge",
    "X-LiteSpeed-Tag",
    "X-LiteSpeed-Vary",
    "Lsc-cookie",
    "X-Powered-By",
    "Link",
    "Version",
    "alt-svc",
    "X-LiteSpeed-Alt-Svc",
    "X-LSADC-Backend",
    "Upgrade",
    "X-LiteSpeed-Purge2",
};


const char **HttpRespHeaders::getNameList()
{
    return s_sHeaders;
}


int HttpRespHeaders::s_iHeaderLen[H_HEADER_END + 1] =
{
    13, 10, 12, 14, 16, 13, 19, 13, //cache-control
    4, 4, 7, 10, 13, 8, 20, 25, //x-litespeed-cache-control
    6, 16, 6, 10, 6, 17, 4, 16, 17, 17, 15, 16, 10, 12, //x-powered-by
    4, 7, 7, 19, 15, 7, 18, 0
};


HttpRespHeaders::HttpRespHeaders()
{
    m_lsxpack.alloc(32);
    reset();
}


HttpRespHeaders::~HttpRespHeaders()
{
}


inline lsxpack_header *HttpRespHeaders::newHdrEntry()
{
    assert(m_working == NULL);
    lsxpack_header *hdr = m_lsxpack.newObj();
    memset(hdr, 0, sizeof(*hdr));
    return hdr;
}


void HttpRespHeaders::reset2()
{
    if (m_KVPairindex[H_SET_COOKIE] == HRH_IDX_NONE)
    {
        reset();
        return;
    }
    if (m_iHeaderUniqueCount == 1)
        return;

    struct iovec iov[100], *pIov, *pIovEnd;
    int max = 100;
    max = getHeader(H_SET_COOKIE, iov, 100);
    AutoBuf tmp;
    tmp.swap(m_buf);
    reset();
    pIovEnd = &iov[max];
    for (pIov = iov; pIov < pIovEnd; ++pIov)
        add(H_SET_COOKIE, "Set-Cookie", 10, (char *)pIov->iov_base,
            pIov->iov_len, LSI_HEADER_ADD);
}


void HttpRespHeaders::reset()
{
    m_flags = 0;
    m_iHeaderUniqueCount = 0;
    m_buf.clear();
    memset(m_KVPairindex, 0xFF, sizeof(m_KVPairindex));
    memset(m_lsxpack.begin(), 0, sizeof(lsxpack_header));
    m_lsxpack.setSize(1);
    m_working = NULL;
    m_iHeaderRemovedCount = 1;  //compensate for the reserved header
    m_hLastHeaderKVPairIndex = -1;
    m_iHttpVersion = 0; //HTTP/1.1
    m_iHttpCode = SC_200;
    m_iKeepAlive = 0;
    m_iHeaderBuilt = 0;
    m_iHeadersTotalLen = 0;
}


void HttpRespHeaders::addCommonHeaders()
{   add(s_commonHeaders, s_commonHeadersCount);     }


void HttpRespHeaders::addGzipEncodingHeader()
{
    add(s_gzipHeaders, 2, LSI_HEADER_MERGE);
    updateEtag(ETAG_GZIP);
}


void HttpRespHeaders::addBrEncodingHeader()
{
    add(s_brHeaders, 2, LSI_HEADER_MERGE);
    updateEtag(ETAG_BROTLI);
}


void HttpRespHeaders::updateEtag(ETAG_ENCODING type)
{
    int etagLen;
    const char *pETag = getHeaderToUpdate(H_ETAG, &etagLen);
    char * pUpdate;
    if (!pETag)
        return;
    pUpdate = (char *)pETag + etagLen - 1;
    if (*pUpdate == '"')
        --pUpdate;
    --pUpdate;
    if (*(pUpdate - 1) == ';')
    {
        switch (type)
        {
        case ETAG_NO_ENCODE:
            *pUpdate++ = ';';
            *pUpdate++ = ';';
            break;
        case ETAG_GZIP:
            *pUpdate++ = 'g';
            *pUpdate++ = 'z';
            break;
        case ETAG_BROTLI:
            *pUpdate++ = 'b';
            *pUpdate++ = 'r';
            break;
        }
    }
}


void HttpRespHeaders::appendChunked()
{
    add(&s_chunkedHeader, 1);
}


void HttpRespHeaders::addTruboCharged()
{
    if (s_commonHeadersCount > 1)
        add(&s_turboChargedBy, 1);
}


void HttpRespHeaders::incKvPairs(int num)
{
    m_lsxpack.guarantee(num);
}


//Replace the value with new value in pKv, in this case must have enough space
void HttpRespHeaders::replaceHeader(lsxpack_header *pKv, const char *pVal,
                                    unsigned int valLen)
{
    char *pOldVal = getVal(pKv);
    memcpy(pOldVal, pVal, valLen);
    memset(pOldVal + valLen, ' ', pKv->val_len - valLen);
    pKv->val_len = valLen;
    lsxpack_header_mark_val_changed(pKv);
}

inline void appendLowerCase(char *dest, const char *src, int n)
{
    const char *end = src + n;
    while (src < end)
        *dest++ = tolower(*src++);
}


void HttpRespHeaders::appendKvPairIdx(lsxpack_header *exist, int kv_idx)
{
    while ((exist->chain_next_idx & BYPASS_HIGHEST_BIT_MASK) > 0)
        exist = getKvPair((exist->chain_next_idx & BYPASS_HIGHEST_BIT_MASK) - 1);

    exist->chain_next_idx = ((kv_idx + 1) | HIGHEST_BIT_NUMBER);
}


int HttpRespHeaders::addKvPairIdx(lsxpack_header *new_hdr)
{
    if ((new_hdr->flags & LSXPACK_APP_IDX)
        && new_hdr->app_index != H_UNKNOWN)
    {
        int idx = new_hdr->app_index;
        if (m_KVPairindex[idx] == HRH_IDX_NONE)
        {
            m_KVPairindex[idx] = new_hdr - m_lsxpack.begin();
        }
        else
        {
            appendKvPairIdx(m_lsxpack.get(m_KVPairindex[idx]),
                            new_hdr - m_lsxpack.begin());
            return 0;
        }
    }
    else
    {
        int ret = getHeaderKvOrder(lsxpack_header_get_name(new_hdr),
                                   new_hdr->name_len);
        if (ret != -1)
        {
            appendKvPairIdx(m_lsxpack.get(ret), new_hdr - m_lsxpack.begin());
            return 0;
        }
    }
    return 1;
}


int HttpRespHeaders::tryCommitHeader(INDEX idx, const char *pVal,
                                     unsigned int valLen)
{
    if (!m_working)
        return LS_FAIL;
    if (m_working->app_index == idx
        && pVal == lsxpack_header_get_value(m_working)
        && valLen == m_working->val_len)
    {
        commitWorkingHeader();
        return LS_OK;
    }
    return LS_FAIL;
}


void HttpRespHeaders::commitWorkingHeader()
{
    assert(m_working != NULL);
    m_buf.used(m_working->name_len + m_working->val_len + 4);
    m_iHeaderUniqueCount += addKvPairIdx(m_working);
    assert(m_working == m_lsxpack.end());
    m_lsxpack.setSize(m_lsxpack.size() + 1);
    m_working = NULL;
}


int HttpRespHeaders::appendHeader(lsxpack_header *pKv, int hdr_idx, const char *pName,
                                  unsigned int nameLen,
                                  const char *pVal, unsigned int valLen, int method)
{
    if (method == LSI_HEADER_SET)
        memset(pKv, 0, sizeof(lsxpack_header));

    if (m_buf.available() < (int)(pKv->val_len + nameLen * 2 + valLen + 9))
    {
        if (m_buf.grow(pKv->val_len + nameLen * 2 + valLen + 9))
            return -1;
    }

    //Be careful of the two Kv pointer
    lsxpack_header *pUpdKv = pKv;
    if (pKv->val_len > 0 && method == LSI_HEADER_ADD)
    {
        if (getFreeSpaceCount() <= 0)
        {
            int i = getKvIdx(pKv);
            incKvPairs(16);
            pKv = getKvPair(i);
        }
        int new_id = m_lsxpack.size();
        pUpdKv = newHdrEntry();
        appendKvPairIdx(pKv, new_id);
        m_hLastHeaderKVPairIndex = new_id;
        pUpdKv->chain_next_idx = HIGHEST_BIT_NUMBER;
        assert(pUpdKv->val_len == 0);
    }

    int len = nameLen;
    //StringTool::strnlower(pName, m_buf.end(), len);
    //memcpy(m_buf.end(), pName, len);
    pUpdKv->buf = m_buf.begin();
    pUpdKv->name_offset = m_buf.size();
    pUpdKv->name_len = len;
    pUpdKv->app_index = hdr_idx;
    if (hdr_idx != H_HEADER_END)
        pUpdKv->flags = (lsxpack_flag)(pUpdKv->flags | LSXPACK_APP_IDX);
    appendLowerCase(m_buf.end(), pName, len);
    m_buf.used(len);
    m_buf.append_unsafe(':');
    m_buf.append_unsafe(' ');

    if (pUpdKv->val_len > 0)  //only apply when append and merge
    {
        m_buf.append_unsafe(m_buf.begin() + pUpdKv->val_offset, pUpdKv->val_len);
        m_buf.append_unsafe(',');  //for append and merge case
        ++ pUpdKv->val_len;
    }
    pUpdKv->val_offset = pUpdKv->name_offset + nameLen + 2;
    m_buf.append_unsafe(pVal, valLen);
    m_buf.append_unsafe('\r');
    m_buf.append_unsafe('\n');
    pUpdKv->val_len +=  valLen;
    if (pUpdKv == pKv)
        lsxpack_header_mark_val_changed(pKv);
    return 0;
}


void HttpRespHeaders::verifyHeaderLength(INDEX headerIndex,
        const char *pName, unsigned int nameLen)
{
#ifndef NDEBUG
    assert(headerIndex == getIndex(pName));
    assert((int)nameLen == getNameLen(headerIndex));
#endif
}


//
static int hasValue(const char *existVal, int existValLen, const char *val,
                    int valLen)
{
    const char *p = existVal;
    const char *pEnd = existVal + existValLen;
    while (p < pEnd && pEnd - p >= valLen)
    {
        if (memcmp(p, val, valLen) == 0)
        {
            const char *p1 = p + valLen;
            while(p1 < pEnd && isspace(*p1))
                ++p1;
            if (p1 == pEnd || *p1 == ',')
                return 1;
        }
        p = (const char *)memchr((void *)p, ',', pEnd - p);
        if (p)
        {
            ++p;
            while(p < pEnd && isspace(*p))
                ++p;
        }
        else
            break;
    }
    return 0;
}


int HttpRespHeaders::add(INDEX headerIndex, const char *pVal,
                         unsigned int valLen,  int method)
{
    if ((int)headerIndex < 0 || headerIndex >= H_HEADER_END)
        return LS_FAIL;
    return add(headerIndex, s_sHeaders[headerIndex],
               getNameLen(headerIndex), pVal, valLen, method);
}


lsxpack_header *HttpRespHeaders::getExistKv(INDEX index,
                                         const char *name, int name_len)
{
    if (index == H_HEADER_END)   // || headerIndex == H_UNKNOWN )
    {
        if (name_len <= 0)
            return NULL;
        int ret = getHeaderKvOrder(name, name_len);
        if (ret != -1)
            getKvPair(ret);
    }
    else
    {
        if (m_KVPairindex[index] != HRH_IDX_NONE)
            return getKvPair(m_KVPairindex[index]);
    }
    return NULL;
}


//method: 0 replace,1 append, 2 merge, 3 add
int HttpRespHeaders::add(INDEX headerIndex, const char *pName, unsigned nameLen,
                         const char *pVal, unsigned int valLen, int method)
{
    if ((headerIndex > H_HEADER_END))
        return -1;

    int kvOrderNum = -1;
    lsxpack_header *pKv = NULL;

    if (headerIndex != H_HEADER_END)
    {
        if (pName == NULL)
        {
            pName = s_sHeaders[headerIndex];
            nameLen = getNameLen(headerIndex);
        }
        if (headerIndex == H_LINK)
        {
            if (memmem(pVal, valLen, "preload", 7) != NULL)
                m_flags |= HRH_F_HAS_PUSH;
        }
    }
    if (headerIndex == H_HEADER_END)   // || headerIndex == H_UNKNOWN )
    {
        if (nameLen <= 0)
            return -1;
        int ret = getHeaderKvOrder(pName, nameLen);
        if (ret != -1)
            kvOrderNum = ret;
        else
            kvOrderNum = m_lsxpack.size();
    }
    else
    {
        //verifyHeaderLength(headerIndex,  pName, nameLen);
        if (m_KVPairindex[headerIndex] == HRH_IDX_NONE)
            m_KVPairindex[headerIndex] = m_lsxpack.size();
        kvOrderNum = m_KVPairindex[headerIndex];
    }

    if (getFreeSpaceCount() <= 0)
        incKvPairs(16);
    if (kvOrderNum == m_lsxpack.size())
    {
        pKv = newHdrEntry();
        ++m_iHeaderUniqueCount;
    }
    else
        pKv = getKvPair(kvOrderNum);

    //enough space for replace, use the same keyoff, and update valoff, add padding, make it is the same leangth as before
    if (method == LSI_HEADER_SET && pKv->name_len == (int)nameLen
        && pKv->val_len >= (int)valLen)
    {
        assert(pKv->name_len == (int)nameLen);
        assert(strncasecmp(getName(pKv), pName, nameLen) == 0);
        replaceHeader(pKv, pVal, valLen);
        return 0;
    }

    if ((method == LSI_HEADER_MERGE || method == LSI_HEADER_ADD)
        && (pKv->val_len > 0))
    {
        if (headerIndex != H_SET_COOKIE || method != LSI_HEADER_ADD)
        {
            if (memchr(pVal, ',', valLen) != NULL)
            {
                AutoBuf new_val;
                StringList values;
                values.split(pVal, pVal + valLen, ",");
                StringList::iterator iter;
                for (iter = values.begin(); iter != values.end(); ++iter)
                {
                    if (!hasValue(getVal(pKv), pKv->val_len, (*iter)->c_str(),
                                 (*iter)->len()))
                    {
                        if (!new_val.empty())
                            new_val.append(",", 1);
                        new_val.append((*iter)->c_str(), (*iter)->len());
                    }
                }
                if (new_val.empty())
                    return 0;

                m_hLastHeaderKVPairIndex = kvOrderNum;
                //Under append situation, if has existing key and valLen > 0, then makes a hole
                if (pKv->val_len > 0 && method != LSI_HEADER_ADD)
                {
                    assert(pKv->name_len > 0);
                    m_flags |= HRH_F_HAS_HOLE;
                }
                return appendHeader(pKv, headerIndex, pName, nameLen,
                                    new_val.begin(), new_val.size(), method);
            }
            else if (hasValue(getVal(pKv), pKv->val_len, pVal, valLen))
                return 0;//if exist when merge, ignor, otherwise same as append
        }
    }

    m_hLastHeaderKVPairIndex = kvOrderNum;

    //Under append situation, if has existing key and valLen > 0, then makes a hole
    if (pKv->val_len > 0 && method != LSI_HEADER_ADD)
    {
        assert(pKv->name_len > 0);
        m_flags |= HRH_F_HAS_HOLE;
    }

    return appendHeader(pKv, headerIndex, pName, nameLen, pVal, valLen, method);
}


int HttpRespHeaders::add(const char *pName, int nameLen, const char *pVal,
                         unsigned int valLen, int method)
{
    INDEX headerIndex = getIndex(pName, nameLen);
    return add(headerIndex, pName, nameLen, pVal, valLen, method);

}


//This will only append value to the last item
int HttpRespHeaders::appendLastVal(const char *pVal, int valLen)
{
    if (m_hLastHeaderKVPairIndex == -1 || valLen <= 0)
        return -1;

    assert(m_hLastHeaderKVPairIndex < m_lsxpack.size());
    lsxpack_header *pKv = getKvPair(m_hLastHeaderKVPairIndex);

    //check if it is the end
    if (m_buf.size() != pKv->val_offset + pKv->val_len + 2)
        return -1;

    if (pKv->name_len == 0)
        return -1;

    if (m_buf.available() < valLen)
    {
        if (m_buf.grow(valLen))
            return -1;
    }

    //Only update the valLen of the kvpair
    pKv->val_len += valLen;
    m_buf.used(-2);  //move back two char, to replace the \r\n at the end
    m_buf.append_unsafe(pVal, valLen);
    m_buf.append_unsafe("\r\n", 2);

    return 0;
}


int HttpRespHeaders::add(http_header_t *headerArray, int size, int method)
{
    int ret = 0;
    for (int i = 0; i < size; ++i)
    {
        if (0 != add(headerArray[i].index, headerArray[i].name,
                     headerArray[i].nameLen,
                     headerArray[i].val, headerArray[i].valLen, method))
        {
            ret = -1;
            break;
        }
    }

    return ret;
}


void HttpRespHeaders::_del(int kvOrderNum)
{
    if (kvOrderNum <= -1)
        return;

    lsxpack_header *pKv = getKvPair(kvOrderNum);
    if ((pKv->chain_next_idx & BYPASS_HIGHEST_BIT_MASK) > 0)
        _del((pKv->chain_next_idx & BYPASS_HIGHEST_BIT_MASK) - 1);

    memset(pKv, 0, sizeof(lsxpack_header));
    m_flags |= HRH_F_HAS_HOLE;
    ++m_iHeaderRemovedCount;
}


//del( const char * pName, int nameLen ) is lower than  del( int headerIndex )
//
int HttpRespHeaders::del(const char *pName, int nameLen)
{
    assert(nameLen > 0);
    size_t idx = getIndex(pName, nameLen);
    if (idx == H_HEADER_END)
    {
        int kvOrderNum = getHeaderKvOrder(pName, nameLen);
        _del(kvOrderNum);
        --m_iHeaderUniqueCount;
    }
    else
        del((INDEX)idx);
    return 0;
}


//del() will make some {0,0,0,0} kvpair in the list and make hole
int HttpRespHeaders::del(INDEX headerIndex)
{
    if (headerIndex < 0 || headerIndex >= H_HEADER_END)
        return -1;

    if (m_KVPairindex[headerIndex] != HRH_IDX_NONE)
    {
        _del(m_KVPairindex[headerIndex]);
        m_KVPairindex[headerIndex] = HRH_IDX_NONE;
        --m_iHeaderUniqueCount;
    }
    return 0;
}


char *HttpRespHeaders::getContentTypeHeader(int &len)
{
    int index = m_KVPairindex[HttpRespHeaders::H_CONTENT_TYPE];
    if (index == HRH_IDX_NONE)
    {
        len = -1;
        return NULL;
    }

    lsxpack_header *pKv = getKvPair(index);
    len = pKv->val_len;
    return getVal(pKv);
}


//The below calling will not maintance the kvpaire when regular case
int HttpRespHeaders::parseAdd(const char *pStr, int len, int method)
{
    if (!pStr)
        return -1;
    //When SPDY format, we need to parse it and add one by one
    const char *pName = NULL;
    int nameLen = 0;
    const char *pVal = NULL;
    int valLen = 0;

    const char *pBEnd = pStr + len;
    const char *pMark = NULL;
    const char *pLineEnd = NULL;
    const char *pLineBegin  = pStr;

    while (pBEnd > pLineBegin)
    {
        pLineEnd  = (const char *)memchr(pLineBegin, '\n',
                        pBEnd - pLineBegin);
        if (pLineEnd == NULL)
            pLineEnd = pBEnd;

        pMark = (const char *)memchr(pLineBegin, ':', pLineEnd - pLineBegin);
        if (pMark != NULL)
        {
            pName = pLineBegin;
            pVal = pMark + 1;
            while (pMark > pLineBegin && isspace(*(pMark - 1)))
                --pMark;
            nameLen = pMark - pLineBegin; //Should - 1 to remove the ':' position
            pLineBegin = pLineEnd + 1;

            while (pVal < pLineEnd && isspace(*(pLineEnd - 1)))
                --pLineEnd;
            while (pVal < pLineEnd && isspace(*pVal))
                ++pVal;
            valLen = pLineEnd - pVal;

            //This way, all the value use APPEND as default
            if (add(pName, nameLen, pVal, valLen, method) == -1)
                return -1;
        }
        else
            pLineBegin = pLineEnd + 1;
    }
    return 0;
}


int HttpRespHeaders::getHeaderKvOrder(const char *pName,
                                      unsigned int nameLen)
{
    lsxpack_header *pKv;

    for (pKv = m_lsxpack.begin(); pKv < m_lsxpack.end(); ++pKv)
    {
        if (pKv->name_len == (int)nameLen &&
            strncasecmp(pName, getName(pKv), nameLen) == 0)
        {
            return pKv - m_lsxpack.begin();
        }
    }
    return -1;
}


int HttpRespHeaders::getHeader(int kvOrderNum, int *idx, struct iovec *name,
                                struct iovec *iov, int maxIovCount)
{
    int count = 0;
    lsxpack_header *pKv;
    if (name)
    {
        name->iov_base = NULL;
        name->iov_len = 0;
    }
    while (kvOrderNum >= 0 && count < maxIovCount)
    {
        pKv = getKvPair(kvOrderNum);
        if (pKv->name_len > 0)
        {
            if (name && 0 == name->iov_len)
            {
                name->iov_base = getName(pKv);
                name->iov_len = pKv->name_len;
                if (idx != NULL)
                    *idx = pKv->app_index;
            }
            iov[count].iov_base = getVal(pKv);
            iov[count++].iov_len = pKv->val_len;
        }
        kvOrderNum = (pKv->chain_next_idx & BYPASS_HIGHEST_BIT_MASK) - 1;
    }
    return count;
}


int  HttpRespHeaders::getHeader(INDEX index, struct iovec *iov,
                                int maxIovCount)
{
    int kvOrderNum = -1;
    if (index >= H_HEADER_END)
        return 0;
    if (m_KVPairindex[index] != HRH_IDX_NONE)
        kvOrderNum = m_KVPairindex[index];

    return getHeader(kvOrderNum, NULL, NULL, iov, maxIovCount);
}


int HttpRespHeaders::getHeader(const char *pName, int nameLen,
                               struct iovec *iov, int maxIovCount)
{
    INDEX idx = getIndex(pName, nameLen);
    if (idx != H_HEADER_END)
        return getHeader(idx, iov, maxIovCount);

    int kvOrderNum = getHeaderKvOrder(pName, nameLen);
    return getHeader(kvOrderNum, NULL, NULL, iov, maxIovCount);
}


const char *HttpRespHeaders::getHeader(INDEX index, int *valLen) const
{
    const lsxpack_header *pKv;
    if (index >= H_HEADER_END)
        return 0;
    if (m_KVPairindex[index] == HRH_IDX_NONE)
        return NULL;
    pKv = getKvPair(m_KVPairindex[index]);
    *valLen = pKv->val_len;
    return getVal(pKv);
}


char *HttpRespHeaders::getHeaderToUpdate(INDEX index, int *valLen)
{
    lsxpack_header *pKv;
    if (index >= H_HEADER_END)
        return 0;
    if (m_KVPairindex[index] == HRH_IDX_NONE)
        return NULL;
    pKv = getKvPair(m_KVPairindex[index]);
    lsxpack_header_mark_val_changed(pKv);
    *valLen = pKv->val_len;
    return getVal(pKv);
}


int HttpRespHeaders::getHeader(const char *pName, int nameLen, const char **val,
                               int &valLen)
{
    struct iovec iov[1];
    if (getHeader(pName, nameLen, iov, 1) == 1)
    {
        *val = (char *)iov[0].iov_base;
        valLen = iov[0].iov_len;
        return 0;
    }
    else
        return -1;
}


HttpRespHeaders::INDEX HttpRespHeaders::getIndex(const char *pHeader)
{
    INDEX idx = H_HEADER_END;

    switch (*pHeader++ | 0x20)
    {
    case 'a':
        if (strncasecmp(pHeader, "ccept-ranges", 12) == 0)
            idx = H_ACCEPT_RANGES;
        else if (strncasecmp(pHeader, "lt-svc", 6) == 0)
            idx = H_ALT_SVC;
        break;
    case 'c':
        switch(*(pHeader+7) | 0x20)
        {
        case 'o':
            if (strncasecmp(pHeader, "onnection", 9) == 0)
                idx = H_CONNECTION;
            break;
        case 't':
            if (strncasecmp(pHeader, "ontent-type", 11) == 0)
                idx = H_CONTENT_TYPE;
            break;
        case 'l':
            if (strncasecmp(pHeader, "ontent-length", 13) == 0)
                idx = H_CONTENT_LENGTH;
            break;
        case 'e':
            if (strncasecmp(pHeader, "ontent-encoding", 15) == 0)
                idx = H_CONTENT_ENCODING;
            break;
        case 'r':
            if (strncasecmp(pHeader, "ontent-range", 12) == 0)
                idx = H_CONTENT_RANGE;
            break;
        case 'd':
            if (strncasecmp(pHeader, "ontent-disposition", 18) == 0)
                idx = H_CONTENT_DISPOSITION;
            break;
        case 'n':
            if (strncasecmp(pHeader, "ache-control", 12) == 0)
                idx = H_CACHE_CTRL;
            break;
        }
        break;
    case 'd':
        if (strncasecmp(pHeader, "ate", 3) == 0)
            idx = H_DATE;
        break;
    case 'e':
        if (strncasecmp(pHeader, "tag", 3) == 0)
            idx = H_ETAG;
        else if (strncasecmp(pHeader, "xpires", 6) == 0)
            idx = H_EXPIRES;
        break;
    case 'k':
        if (strncasecmp(pHeader, "eep-alive", 9) == 0)
            idx = H_KEEP_ALIVE;
        break;
    case 'l':
        switch(*pHeader | 0x20)
        {
        case 'a':
            if (strncasecmp(pHeader, "ast-modified", 12) == 0)
                idx = H_LAST_MODIFIED;
            break;
        case 'o':
            if (strncasecmp(pHeader, "ocation", 7) == 0)
                idx = H_LOCATION;
            break;
        case 's':
            if (strncasecmp(pHeader, "sc-cookie", 9) == 0)
                idx = H_LSC_COOKIE;
            break;
        case 'i':
            if (strncasecmp(pHeader, "ink", 3) == 0)
                idx = H_LINK;
            break;
        }
        break;
    case 'p':
        if (strncasecmp(pHeader, "ragma", 5) == 0)
            idx = H_PRAGMA;
        else if (strncasecmp(pHeader, "roxy-connection", 15) == 0)
            idx = H_PROXY_CONNECTION;
        break;
    case 's':
        if (strncasecmp(pHeader, "erver", 5) == 0)
            idx = H_SERVER;
        else if (strncasecmp(pHeader, "et-cookie", 9) == 0)
            idx = H_SET_COOKIE;
        else if (strncasecmp(pHeader, "tatus", 5) == 0)
            idx = H_CGI_STATUS;
        break;
    case 't':
        if (strncasecmp(pHeader, "ransfer-encoding", 16) == 0)
            idx = H_TRANSFER_ENCODING;
        break;
    case 'v':
        if (strncasecmp(pHeader, "ary", 3) == 0)
            idx = H_VARY;
        else if (strncasecmp(pHeader, "ersion", 6) == 0)
            idx = H_HTTP_VERSION;
        break;
    case 'w':
        if (strncasecmp(pHeader, "ww-authenticate", 15) == 0)
            idx = H_WWW_AUTHENTICATE;
        break;
    case 'x':
        if (strncasecmp(pHeader, "-powered-by", 11) == 0)
            idx = H_X_POWERED_BY;
        else if (strncasecmp(pHeader, "-lsadc-backend", 14) == 0)
            idx = H_LSADC_BACKEND;
        else if (strncasecmp(pHeader, "-litespeed-", 11) == 0)
        {
            pHeader += 11;
            switch(*pHeader | 0x20)
            {
            case 'l':
                if (strncasecmp(pHeader, "location", 8) == 0)
                    idx = H_LITESPEED_LOCATION;
                break;
            case 'c':
                if (strncasecmp(pHeader, "cache-control", 13) == 0)
                    idx = H_LITESPEED_CACHE_CONTROL;
                else if (strncasecmp(pHeader, "cache", 5) == 0)
                    idx = H_X_LITESPEED_CACHE;
                break;
            case 't':
                if (strncasecmp(pHeader, "tag", 3) == 0)
                    idx = H_X_LITESPEED_TAG;
                break;
            case 'p':
                if (strncasecmp(pHeader, "purge", 5) == 0)
                {
                    if (*(pHeader + 5) == '2')
                        idx = H_X_LITESPEED_PURGE2;
                    else
                        idx = H_X_LITESPEED_PURGE;
                }
                break;
            case 'v':
                if (strncasecmp(pHeader, "vary", 4) == 0)
                    idx = H_X_LITESPEED_VARY;
                break;
            case 'a':
                if (strncasecmp(pHeader, "alt-svc", 7) == 0)
                    idx = H_X_LITESPEED_ALT_SVC;
                break;
            }
        }
        break;
    case 'u':
        if (strncasecmp(pHeader, "pgrade", 6) == 0)
            idx = H_UPGRADE;
        break;
    case ':': //only SPDY 3
        if (strncasecmp(pHeader, "status", 6) == 0)
            idx = H_CGI_STATUS;
        else if (strncasecmp(pHeader, "version", 7) == 0)
            idx = H_HTTP_VERSION;
        break;

    default:
        break;
    }
    return idx;
}


HttpRespHeaders::INDEX HttpRespHeaders::getIndex(const char *pHeader, int len)
{
    INDEX idx = getIndex(pHeader);
    if (idx != H_HEADER_END)
    {
        if (getNameLen(idx) != len)
            idx = H_HEADER_END;
    }
    return idx;
}


int HttpRespHeaders::nextHeaderPos(int pos)
{
    int ret = -1;
    for (int i = pos + 1; i < m_lsxpack.size(); ++i)
    {
        if (getKvPair(i)->name_len > 0
            && ((getKvPair(i)->chain_next_idx & HIGHEST_BIT_NUMBER) == 0))
        {
            ret = i;
            break;
        }
    }
    return ret;
}


int HttpRespHeaders::getAllHeaders(struct iovec *iov_key,
                                   struct iovec *iov_val, int maxIovCount)
{
    int count = 0;
//     if (withStatusLine)
//     {
//         const StatusLineString& statusLine = HttpStatusLine::getStatusLine( m_iHttpVersion, m_iHttpCode );
//         iov[count].iov_base = (void *)statusLine.get();
//         iov[count].iov_len = statusLine.getLen();
//         ++count;
//     }

    for (int i = 0; i < m_lsxpack.size() && count < maxIovCount; ++i)
    {
        lsxpack_header *pKv = getKvPair(i);
        if (pKv->name_len > 0)
        {
            iov_key[count].iov_base = m_buf.begin() + pKv->name_offset;
            iov_key[count].iov_len = pKv->name_len;
            iov_val[count].iov_base = m_buf.begin() + pKv->val_offset;
            iov_val[count].iov_len = pKv->val_len;
            ++count;
        }
    }

//     if (addEndLine && count < maxIovCount)
//     {
//         iov[count].iov_base = (void *)"\r\n";
//         iov[count].iov_len = 2;
//         ++count;
//     }

    return count;
}


int HttpRespHeaders::appendToIovExclude(IOVec *iovec, const char *pName,
                                        int nameLen) const
{
    int total = 0;
    const lsxpack_header *pKv = m_lsxpack.begin();
    const lsxpack_header *pKvEnd = m_lsxpack.end();

    while (pKv < pKvEnd)
    {
        if ((pKv->name_len > 0)
            && ((pKv->name_len != nameLen)
                || (strncasecmp(m_buf.begin() + pKv->name_offset, pName, nameLen))))
        {
            iovec->appendCombine(m_buf.begin() + pKv->name_offset,
                                 pKv->name_len + pKv->val_len + 4);
            total += pKv->name_len + pKv->val_len + 4;
        }
        ++pKv;
    }
    return total;
}


int HttpRespHeaders::mergeAll()
{
    int total = 0;
    lsxpack_header *pKv = m_lsxpack.begin();
    lsxpack_header *pKvEnd = m_lsxpack.end();
    AutoBuf tmp(m_buf.size());

    while (pKv < pKvEnd)
    {
        if (pKv->name_len > 0)
        {
            int len = pKv->name_len + pKv->val_len + 2;
            tmp.append(m_buf.begin() + pKv->name_offset, len);
            tmp.append_unsafe('\r');
            tmp.append_unsafe('\n');
            int diff = total - pKv->name_offset;
            pKv->name_offset = total;
            pKv->val_offset += diff;
            total += len + 2;
        }
        ++pKv;
    }
    m_buf.swap(tmp);
    return total;
}


void HttpRespHeaders::dump(LogSession *pILog, int dump_header)
{
    LS_DBG_H(pILog, "Resp headers, total: %d, removed: %d, unique:%d, "
                    "has hole: %d, buffer size: %d",
        (int)m_lsxpack.size(), (int)m_iHeaderRemovedCount,
        (int)m_iHeaderUniqueCount, (int)m_flags,
        (int)m_buf.size() );
    lsxpack_header *pKv = m_lsxpack.begin();
    lsxpack_header *pKvEnd = m_lsxpack.end();

    while (pKv < pKvEnd)
    {
        if (pKv->name_len > 0)
        {
            int len = pKv->name_len + pKv->val_len + 4;
            LS_DBG_H(pILog, "    %.*s", len, m_buf.begin() + pKv->name_offset);
        }
        ++pKv;
    }

}


int HttpRespHeaders::appendToIov(IOVec *iovec, int &addCrlf)
{
    int total;

    if (m_flags & HRH_F_HAS_HOLE)
        mergeAll();
    total = m_buf.size();
    if (addCrlf)
    {
        if (m_buf.available() >= 2)
        {
            char *p = m_buf.end();
            *p++ = '\r';
            *p++ = '\n';
            total += 2;
        }
        else
            addCrlf = 0;
    }
    if (total > 0)
        iovec->append(m_buf.begin(), total);

    return total;
}


/***
 *
 * The below functions may be moved to another classes
 */
int HttpRespHeaders::outputNonSpdyHeaders(IOVec *iovec)
{
    if (m_iHttpCode == SC_100)
    {
        iovec->clear(1);
        const StatusLineString &statusLine = HttpStatusLine::getInstance().getStatusLine(
                m_iHttpVersion, m_iHttpCode);
        iovec->push_front(statusLine.get(), statusLine.getLen());
        m_iHeadersTotalLen = statusLine.getLen() + 2;
        iovec->append("\r\n", 2);
        return 0;
    }
    if (m_iKeepAlive)
    {
        add(s_keepaliveHeader, 1);
    }
    else
        add(&s_concloseHeader, 1);

    iovec->clear(1);
    const StatusLineString &statusLine = HttpStatusLine::getInstance().getStatusLine(
            m_iHttpVersion, m_iHttpCode);
    iovec->push_front(statusLine.get(), statusLine.getLen());
    m_iHeadersTotalLen = statusLine.getLen();
    int addCrlf = 1;
    m_iHeadersTotalLen += appendToIov(iovec, addCrlf);
    if (!addCrlf)
    {
        iovec->append("\r\n", 2);
        m_iHeadersTotalLen += 2;
    }
    m_iHeaderBuilt = 1;
    return 0;
}


void HttpRespHeaders::buildCommonHeaders()
{
    s_commonHeaders[0].index    = HttpRespHeaders::H_DATE;
    s_commonHeaders[0].name     = s_sCommonHeaders;
    s_commonHeaders[0].nameLen  = 4;
    s_commonHeaders[0].val      = s_sCommonHeaders + 6;
    s_commonHeaders[0].valLen   = 29;

//     s_commonHeaders[1].index    =
//         HttpRespHeaders::H_ACCEPT_RANGES;
//     s_commonHeaders[1].name     = s_sCommonHeaders + 37;
//     s_commonHeaders[1].nameLen  = 13;
//     s_commonHeaders[1].val      = s_sCommonHeaders + 52;
//     s_commonHeaders[1].valLen   = 5;

    s_commonHeaders[1].index    = HttpRespHeaders::H_SERVER;
    s_commonHeaders[1].name     = s_sCommonHeaders + 37;
    s_commonHeaders[1].nameLen  = 6;
    s_commonHeaders[1].val      = HttpServerVersion::getVersion();
    s_commonHeaders[1].valLen   = HttpServerVersion::getVersionLen();

    s_turboChargedBy.index      = HttpRespHeaders::H_UNKNOWN;
    s_turboChargedBy.name       = s_sTurboCharged;
    s_turboChargedBy.nameLen    = 18;
    s_turboChargedBy.val        = HttpServerVersion::getVersion();
    s_turboChargedBy.valLen     = HttpServerVersion::getVersionLen();

    s_gzipHeaders[0].index    = HttpRespHeaders::H_CONTENT_ENCODING;
    s_gzipHeaders[0].name     = s_sGzipEncodingHeader;
    s_gzipHeaders[0].nameLen  = 16;
    s_gzipHeaders[0].val      = s_sGzipEncodingHeader + 18;
    s_gzipHeaders[0].valLen   = 4;

    s_gzipHeaders[1].index    = HttpRespHeaders::H_VARY;
    s_gzipHeaders[1].name     = s_sGzipEncodingHeader + 24;
    s_gzipHeaders[1].nameLen  = 4;
    s_gzipHeaders[1].val      = s_sGzipEncodingHeader + 30;
    s_gzipHeaders[1].valLen   = 15;


    s_brHeaders[0].index    = HttpRespHeaders::H_CONTENT_ENCODING;
    s_brHeaders[0].name     = s_sBrEncodingHeader;
    s_brHeaders[0].nameLen  = 16;
    s_brHeaders[0].val      = s_sBrEncodingHeader + 18;
    s_brHeaders[0].valLen   = 2;

    s_brHeaders[1].index    = HttpRespHeaders::H_VARY;
    s_brHeaders[1].name     = s_sBrEncodingHeader + 22;
    s_brHeaders[1].nameLen  = 4;
    s_brHeaders[1].val      = s_sBrEncodingHeader + 28;
    s_brHeaders[1].valLen   = 15;

    s_keepaliveHeader[0].index    = HttpRespHeaders::H_CONNECTION;
    s_keepaliveHeader[0].name     = s_sConnKeepAliveHeader;
    s_keepaliveHeader[0].nameLen  = 10;
    s_keepaliveHeader[0].val      = s_sConnKeepAliveHeader +
            12;
    s_keepaliveHeader[0].valLen   = 10;

    s_keepaliveHeader[1].index    = HttpRespHeaders::H_KEEP_ALIVE;
    s_keepaliveHeader[1].name     = s_sConnKeepAliveHeader +
            24;
    s_keepaliveHeader[1].nameLen  = 10;
    s_keepaliveHeader[1].val      = s_sConnKeepAliveHeader +
            36;
    s_keepaliveHeader[1].valLen   = 18;

    s_concloseHeader.index    = HttpRespHeaders::H_CONNECTION;
    s_concloseHeader.name     = s_sConnCloseHeader;
    s_concloseHeader.nameLen  = 10;
    s_concloseHeader.val      = s_sConnCloseHeader + 12;
    s_concloseHeader.valLen   = 5;

    s_chunkedHeader.index    = HttpRespHeaders::H_TRANSFER_ENCODING;
    s_chunkedHeader.name     = s_sChunkedHeader;
    s_chunkedHeader.nameLen  = 17;
    s_chunkedHeader.val      = s_sChunkedHeader + 19;
    s_chunkedHeader.valLen   = 7;

    updateDateHeader();
}


void HttpRespHeaders::updateDateHeader()
{
    char achDateTime[60];
    DateTime::getRFCTime(DateTime::s_curTime, achDateTime);
    assert(strlen(achDateTime) == 29);
    memcpy(s_sCommonHeaders + 6, achDateTime, 29);
}


void HttpRespHeaders::hideServerSignature(int hide)
{
    if (hide == 2) //hide all
        s_commonHeadersCount = 1;
    else
    {
        HttpServerVersion::hideDetail(!hide);
        s_commonHeaders[1].valLen   =
            HttpServerVersion::getVersionLen();
    }

}


void HttpRespHeaders::convertLscCookie()
{
    if (m_KVPairindex[H_LSC_COOKIE] == HRH_IDX_NONE)
        return;
    int kvOrderNum = m_KVPairindex[H_LSC_COOKIE];
    lsxpack_header * pKv = NULL;

    while (kvOrderNum >= 0)
    {
        pKv = getKvPair(kvOrderNum);
        char *pName = getName(pKv);
        assert(strncasecmp(pName, "Lsc-", 4) == 0);
        *pName++ = 's';
        *pName++ = 'e';
        *pName++ = 't';
        assert(pKv->app_index == H_LSC_COOKIE);
        pKv->app_index = H_SET_COOKIE;
        pKv->flags = (lsxpack_flag)(pKv->flags &
                ~(LSXPACK_NAME_HASH|LSXPACK_NAMEVAL_HASH));
        kvOrderNum = (pKv->chain_next_idx & BYPASS_HIGHEST_BIT_MASK) - 1;
    }

    if (m_KVPairindex[H_SET_COOKIE] != HRH_IDX_NONE)
    {
        assert(pKv != NULL);
        pKv->chain_next_idx = ((m_KVPairindex[H_SET_COOKIE] + 1)
                        | (pKv->chain_next_idx & HIGHEST_BIT_NUMBER));
        pKv = getKvPair(m_KVPairindex[H_SET_COOKIE]);
        pKv->chain_next_idx |= HIGHEST_BIT_NUMBER;
        --m_iHeaderUniqueCount;
    }
    m_KVPairindex[H_SET_COOKIE] = m_KVPairindex[H_LSC_COOKIE];
    m_KVPairindex[H_LSC_COOKIE] = HRH_IDX_NONE;
}


void HttpRespHeaders::dropConnectionHeaders()
{
    del(H_CONNECTION);
    del(H_KEEP_ALIVE);
    del(H_PROXY_CONNECTION);
    del(H_TRANSFER_ENCODING);
    //del(H_UPGRADE);
}


void HttpRespHeaders::copy(const HttpRespHeaders &headers)
{
    struct iovec iov[100], *pIov, *pIovEnd;
    int max = 100;
    max = getHeader(H_SET_COOKIE, iov, 100);
    if (max > 0)
    {
        AutoBuf tmp;
        tmp.swap(m_buf);
        copyEx(headers);
        pIovEnd = &iov[max];
        for (pIov = iov; pIov < pIovEnd; ++pIov)
            add(H_SET_COOKIE, "Set-Cookie", 10, (char *)pIov->iov_base,
                pIov->iov_len, LSI_HEADER_ADD);
    }
    else
        copyEx(headers);
}


void HttpRespHeaders::copyEx(const HttpRespHeaders &headers)
{
    m_flags |= (headers.m_flags & HRH_F_HAS_PUSH);
    m_buf.clear();
    m_buf.append(headers.m_buf.begin(), headers.m_buf.size());

    m_lsxpack.guarantee(headers.m_lsxpack.size() + 5 - m_lsxpack.size());
    m_lsxpack.copy(headers.m_lsxpack);

    memmove(&m_KVPairindex[0], &headers.m_KVPairindex[0],
            (char *)(&m_iHeadersTotalLen + 1) - (char *)&m_KVPairindex[0]);
}


int HttpRespHeaders::toHpackIdx(int index)
{
    static int lookup[] =
    {
        LSHPACK_HDR_ACCEPT_RANGES,
        -1,                             //H_CONNECTION,
        LSHPACK_HDR_CONTENT_TYPE,
        LSHPACK_HDR_CONTENT_LENGTH ,
        LSHPACK_HDR_CONTENT_ENCODING,
        LSHPACK_HDR_CONTENT_RANGE,
        LSHPACK_HDR_CONTENT_DISPOSITION,
        LSHPACK_HDR_CACHE_CONTROL,
        LSHPACK_HDR_DATE,
        LSHPACK_HDR_ETAG,
        LSHPACK_HDR_EXPIRES,
        -2,                             //KEEP_ALIVE,
        LSHPACK_HDR_LAST_MODIFIED,
        LSHPACK_HDR_LOCATION,
        -3,                             //LITESPEED_LOCATION,
        -4,                             //LITESPEED_CACHE_CONTROL,
        -5,                             //LSI_RSPHDR_PRAGMA,
        -6,                             //PROXY_CONNECTION,
        LSHPACK_HDR_SERVER,
        LSHPACK_HDR_SET_COOKIE,
        -7,                             //CGI_STATUS,
        LSHPACK_HDR_TRANSFER_ENCODING,
        LSHPACK_HDR_VARY,
        LSHPACK_HDR_WWW_AUTHENTICATE,
        -8,                             //X_LITESPEED_CACHE,
        -9,                             //X_LITESPEED_PURGE,
        -10,                            //X_LITESPEED_TAG,
        -11,                            //X_LITESPEED_VARY,
        LSHPACK_HDR_SET_COOKIE,         //LSC_COOKIE ,
        -13,                            //X_POWERED_BY,
        LSHPACK_HDR_LINK,
        -14,                            //HTTP_VERSION,
        -15                             //H_HEADER_END
    };

    if (index >= 0 && index < (int)(sizeof(lookup)/sizeof(int)))
        return lookup[index];
    return -15;
};



static const int appresp2qpack[36] = {
    32,    //"accept-ranges"
    LS_RESP_HDR_DROP,    //"connection"
    54,    //"content-type"
     4,    //"content-length"
    43,    //"content-encoding"
    -1,    //"content-range"
     3,    //"content-disposition"
    41,    //"cache-control"
     6,    //"date"
     7,    //"etag"
    -1,    //"expires"
    LS_RESP_HDR_DROP,    //"keep-alive"
    10,    //"last-modified"
    12,    //"location"
    -1,    //"x-litespeed-location"
    -1,    //"x-litespeed-cache-control"
    -1,    //"pragma"
    LS_RESP_HDR_DROP,    //"proxy-connection"
    92,    //"server"
    14,    //"set-cookie"
    -1,    //"status"
    LS_RESP_HDR_DROP,    //"transfer-encoding"
    60,    //"vary"
    -1,    //"www-authenticate"
    -1,    //"x-litespeed-cache"
    -1,    //"x-litespeed-purge"
    -1,    //"x-litespeed-tag"
    -1,    //"x-litespeed-vary"
    -1,    //"lsc-cookie"
    -1,    //"x-powered-by"
    11,    //"link"
    -1,    //"version"
    83,    //"alt-svc"
    -1,    //"x-litespeed-alt-svc"
    -1,    //"x-lsadc-backend"
    LS_RESP_HDR_DROP,    //"upgrade"
};

static int
lookup_appresp2qpack (int idx)
{
    if (idx >= 0 && (unsigned) idx < sizeof(appresp2qpack) / sizeof(appresp2qpack[0]))
        return appresp2qpack[idx];
    else
        return -1;
}


static void buildQpackIdx(lsxpack_header *hdr)
{
    if (hdr->flags & LSXPACK_QPACK_IDX)
        return;
    int idx = -1;
    if (hdr->hpack_index)
        idx = UnpackedHeaders::hpack2qpack(hdr->hpack_index);
    if (idx == -1 && (hdr->flags & LSXPACK_APP_IDX)
        && hdr->app_index != HttpRespHeaders::H_HEADER_END)
        idx = lookup_appresp2qpack(hdr->app_index);
    if (idx != -1)
    {
        if (idx == LS_RESP_HDR_DROP)
            hdr->buf = NULL;
        else
        {
            hdr->qpack_index = idx;
            hdr->flags = (lsxpack_flag)(hdr->flags | LSXPACK_QPACK_IDX);
        }
    }

}


static const int appresp2hpack[36] = {
    18,    //"accept-ranges"
    LS_RESP_HDR_DROP,    //"connection"
    31,    //"content-type"
    28,    //"content-length"
    26,    //"content-encoding"
    30,    //"content-range"
    25,    //"content-disposition"
    24,    //"cache-control"
    33,    //"date"
    34,    //"etag"
    36,    //"expires"
    LS_RESP_HDR_DROP,    //"keep-alive"
    44,    //"last-modified"
    46,    //"location"
    0,     //"x-litespeed-location"
    0,     //"x-litespeed-cache-control"
    0,     //"pragma"
    LS_RESP_HDR_DROP,    //"proxy-connection"
    54,    //"server"
    55,    //"set-cookie"
    0,     //"status"
    LS_RESP_HDR_DROP,    //"transfer-encoding"
    59,    //"vary"
    61,    //"www-authenticate"
    0,     //"x-litespeed-cache"
    0,     //"x-litespeed-purge"
    0,     //"x-litespeed-tag"
    0,     //"x-litespeed-vary"
    0,     //"lsc-cookie"
    0,     //"x-powered-by"
    45,    //"link"
    0,     //"version"
    0,     //"alt-svc"
    0,     //"x-litespeed-alt-svc"
    0,     //"x-lsadc-backend"
    LS_RESP_HDR_DROP,     //"upgrade"
};


static int
lookup_appresp2hpack (int idx)
{
    if (idx >= 0 && (unsigned) idx < sizeof(appresp2hpack) / sizeof(appresp2hpack[0]))
        return appresp2hpack[idx];
    else
        return 0;
}


static void buildHpackIdx(lsxpack_header *hdr)
{
    if (hdr->hpack_index)
        return;
    int idx = LSHPACK_HDR_UNKNOWN;
    if (hdr->flags & LSXPACK_QPACK_IDX)
    {
        idx = UnpackedHeaders::qpack2hpack(hdr->qpack_index);
        //hdr->flags = (lsxpack_flag)(hdr->flags & ~LSXPACK_VAL_MATCHED);
    }
    if (idx == LSHPACK_HDR_UNKNOWN && (hdr->flags & LSXPACK_APP_IDX)
        && hdr->app_index != HttpRespHeaders::H_HEADER_END)
        idx = lookup_appresp2hpack(hdr->app_index);
    if (idx != LSHPACK_HDR_UNKNOWN)
    {
        if (idx == LS_RESP_HDR_DROP)
            hdr->buf = NULL;
        else
            hdr->hpack_index = idx;
    }
}


void HttpRespHeaders::prepareSendXpack(bool is_qpack)
{
    if (is_qpack)
        prepareSendQpack();
    else
        prepareSendHpack();
}


void HttpRespHeaders::prepareSendQpack()
{
    lsxpack_header *hdr = m_lsxpack.begin();
    memset(hdr, 0, sizeof(*hdr));
    hdr->flags = (lsxpack_flag)(LSXPACK_QPACK_IDX | LSXPACK_VAL_MATCHED);
    switch(getHttpCode())
    {
    case SC_100:
        hdr->qpack_index = LSQPACK_TNV_STATUS_100;
        break;
    case SC_200:
        hdr->qpack_index = LSQPACK_TNV_STATUS_200;
        break;
    case SC_204:
        hdr->qpack_index = LSQPACK_TNV_STATUS_204;
        break;
    case SC_206:
        hdr->qpack_index = LSQPACK_TNV_STATUS_206;
        break;
    case SC_302:
        hdr->qpack_index = LSQPACK_TNV_STATUS_302;
        break;
    case SC_304:
        hdr->qpack_index = LSQPACK_TNV_STATUS_304;
        break;
    case SC_400:
        hdr->qpack_index = LSQPACK_TNV_STATUS_400;
        break;
    case SC_403:
        hdr->qpack_index = LSQPACK_TNV_STATUS_403;
        break;
    case SC_404:
        hdr->qpack_index = LSQPACK_TNV_STATUS_404;
        break;
    case SC_421:
        hdr->qpack_index = LSQPACK_TNV_STATUS_421;
        break;
    case SC_425:
        hdr->qpack_index = LSQPACK_TNV_STATUS_425;
        break;
    case SC_500:
        hdr->qpack_index = LSQPACK_TNV_STATUS_500;
        break;
    case SC_503:
        hdr->qpack_index = LSQPACK_TNV_STATUS_503;
        break;
    default:
        hdr->qpack_index = LSQPACK_TNV_STATUS_103;
        hdr->flags = (lsxpack_flag)(LSXPACK_QPACK_IDX);
        break;
    }
    hdr->buf = (char *)HttpStatusCode::getInstance().getCodeString(getHttpCode());
    hdr->val_offset = 0;
    hdr->val_len = 3;

    ++hdr;
    for(; hdr < end(); ++hdr)
    {
        if (!hdr->buf)
            continue;
        if (hdr->buf != m_buf.begin())
        {
            hdr->buf = m_buf.begin();
        }
        assert(hdr->name_offset + hdr->name_len <= m_buf.size());
        assert(hdr->val_offset + hdr->val_len <= m_buf.size());

        buildQpackIdx(hdr);
    }
}


void HttpRespHeaders::prepareSendHpack()
{
    lsxpack_header *hdr = m_lsxpack.begin();
    memset(hdr, 0, sizeof(*hdr));
    hdr->flags = (lsxpack_flag)(LSXPACK_HPACK_VAL_MATCHED);
    switch(getHttpCode())
    {
    case SC_200:
        hdr->hpack_index = LSHPACK_HDR_STATUS_200;
        break;
    case SC_204:
        hdr->hpack_index = LSHPACK_HDR_STATUS_204;
        break;
    case SC_206:
        hdr->hpack_index = LSHPACK_HDR_STATUS_206;
        break;
    case SC_304:
        hdr->hpack_index = LSHPACK_HDR_STATUS_304;
        break;
    case SC_400:
        hdr->hpack_index = LSHPACK_HDR_STATUS_400;
        break;
    case SC_404:
        hdr->hpack_index = LSHPACK_HDR_STATUS_404;
        break;
    case SC_500:
        hdr->hpack_index = LSHPACK_HDR_STATUS_500;
        break;
    default:
        hdr->hpack_index = LSHPACK_HDR_STATUS_200;
        hdr->flags = (lsxpack_flag)(hdr->flags & ~LSXPACK_HPACK_VAL_MATCHED);
        break;
    }
    hdr->buf = (char *)HttpStatusCode::getInstance().getCodeString(getHttpCode());
    hdr->val_offset = 0;
    hdr->val_len = 3;

    ++hdr;
    for(; hdr < end(); ++hdr)
    {
        if (!hdr->buf)
            continue;
        if (hdr->buf != m_buf.begin())
        {
            hdr->buf = m_buf.begin();
        }
        assert(hdr->name_offset + hdr->name_len <= m_buf.size());
        assert(hdr->val_offset + hdr->val_len <= m_buf.size());

        buildHpackIdx(hdr);
    }
}


static const int hpack2appresp[LSHPACK_MAX_INDEX] = {
    UPK_HDR_UNKNOWN,                           //":authority"
    UPK_HDR_METHOD,                            //":method"
    UPK_HDR_METHOD,                            //":method"
    UPK_HDR_PATH,                              //":path"
    UPK_HDR_PATH,                              //":path"
    UPK_HDR_SCHEME,                            //":scheme"
    UPK_HDR_SCHEME,                            //":scheme"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_UNKNOWN,                           //"accept-charset"
    UPK_HDR_UNKNOWN,                           //"accept-encoding"
    UPK_HDR_UNKNOWN,                           //"accept-language"
    HttpRespHeaders::H_ACCEPT_RANGES,          //"accept-ranges"
    UPK_HDR_UNKNOWN,                           //"accept"
    UPK_HDR_UNKNOWN,                           //"access-control-allow-origin"
    UPK_HDR_UNKNOWN,                           //"age"
    UPK_HDR_UNKNOWN,                           //"allow"
    UPK_HDR_UNKNOWN,                           //"authorization"
    HttpRespHeaders::H_CACHE_CTRL,             //"cache-control"
    HttpRespHeaders::H_CONTENT_DISPOSITION,    //"content-disposition"
    HttpRespHeaders::H_CONTENT_ENCODING,       //"content-encoding"
    UPK_HDR_UNKNOWN,                           //"content-language"
    HttpRespHeaders::H_CONTENT_LENGTH,         //"content-length"
    UPK_HDR_UNKNOWN,                           //"content-location"
    HttpRespHeaders::H_CONTENT_RANGE,          //"content-range"
    HttpRespHeaders::H_CONTENT_TYPE,           //"content-type"
    UPK_HDR_UNKNOWN,                           //"cookie"
    HttpRespHeaders::H_DATE,                   //"date"
    HttpRespHeaders::H_ETAG,                   //"etag"
    UPK_HDR_UNKNOWN,                           //"expect"
    HttpRespHeaders::H_EXPIRES,                //"expires"
    UPK_HDR_UNKNOWN,                           //"from"
    UPK_HDR_UNKNOWN,                           //"host"
    UPK_HDR_UNKNOWN,                           //"if-match"
    UPK_HDR_UNKNOWN,                           //"if-modified-since"
    UPK_HDR_UNKNOWN,                           //"if-none-match"
    UPK_HDR_UNKNOWN,                           //"if-range"
    UPK_HDR_UNKNOWN,                           //"if-unmodified-since"
    HttpRespHeaders::H_LAST_MODIFIED,          //"last-modified"
    HttpRespHeaders::H_LINK,                   //"link"
    HttpRespHeaders::H_LOCATION,               //"location"
    UPK_HDR_UNKNOWN,                           //"max-forwards"
    UPK_HDR_UNKNOWN,                           //"proxy-authenticate"
    UPK_HDR_UNKNOWN,                           //"proxy-authorization"
    UPK_HDR_UNKNOWN,                           //"range"
    UPK_HDR_UNKNOWN,                           //"referer"
    UPK_HDR_UNKNOWN,                           //"refresh"
    UPK_HDR_UNKNOWN,                           //"retry-after"
    HttpRespHeaders::H_SERVER,                 //"server"
    HttpRespHeaders::H_SET_COOKIE,             //"set-cookie"
    UPK_HDR_UNKNOWN,                           //"strict-transport-security"
    HttpRespHeaders::H_TRANSFER_ENCODING,      //"transfer-encoding"
    UPK_HDR_UNKNOWN,                           //"user-agent"
    HttpRespHeaders::H_VARY,                   //"vary"
    UPK_HDR_UNKNOWN,                           //"via"
    HttpRespHeaders::H_WWW_AUTHENTICATE,       //"www-authenticate"
};


int HttpRespHeaders::hpack2RespIdx(int hpack_index)
{
    if (hpack_index > 0 && hpack_index <= LSHPACK_MAX_INDEX)
        return hpack2appresp[hpack_index - 1];
    return UPK_HDR_UNKNOWN;
};

#define QPACK_MAX_INDEX 99
static const int qpack2appresp[QPACK_MAX_INDEX] = {
    UPK_HDR_UNKNOWN,                           //":authority"
    UPK_HDR_PATH,                              //":path"
    UPK_HDR_UNKNOWN,                           //"age"
    HttpRespHeaders::H_CONTENT_DISPOSITION,    //"content-disposition"
    HttpRespHeaders::H_CONTENT_LENGTH,         //"content-length"
    UPK_HDR_UNKNOWN,                           //"cookie"
    HttpRespHeaders::H_DATE,                   //"date"
    HttpRespHeaders::H_ETAG,                   //"etag"
    UPK_HDR_UNKNOWN,                           //"if-modified-since"
    UPK_HDR_UNKNOWN,                           //"if-none-match"
    HttpRespHeaders::H_LAST_MODIFIED,          //"last-modified"
    HttpRespHeaders::H_LINK,                   //"link"
    HttpRespHeaders::H_LOCATION,               //"location"
    UPK_HDR_UNKNOWN,                           //"referer"
    HttpRespHeaders::H_SET_COOKIE,             //"set-cookie"
    UPK_HDR_METHOD,                            //":method"
    UPK_HDR_METHOD,                            //":method"
    UPK_HDR_METHOD,                            //":method"
    UPK_HDR_METHOD,                            //":method"
    UPK_HDR_METHOD,                            //":method"
    UPK_HDR_METHOD,                            //":method"
    UPK_HDR_METHOD,                            //":method"
    UPK_HDR_SCHEME,                            //":scheme"
    UPK_HDR_SCHEME,                            //":scheme"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_UNKNOWN,                           //"accept"
    UPK_HDR_UNKNOWN,                           //"accept"
    UPK_HDR_UNKNOWN,                           //"accept-encoding"
    HttpRespHeaders::H_ACCEPT_RANGES,          //"accept-ranges"
    UPK_HDR_UNKNOWN,                           //"access-control-allow-headers"
    UPK_HDR_UNKNOWN,                           //"access-control-allow-headers"
    UPK_HDR_UNKNOWN,                           //"access-control-allow-origin"
    HttpRespHeaders::H_CACHE_CTRL,             //"cache-control"
    HttpRespHeaders::H_CACHE_CTRL,             //"cache-control"
    HttpRespHeaders::H_CACHE_CTRL,             //"cache-control"
    HttpRespHeaders::H_CACHE_CTRL,             //"cache-control"
    HttpRespHeaders::H_CACHE_CTRL,             //"cache-control"
    HttpRespHeaders::H_CACHE_CTRL,             //"cache-control"
    HttpRespHeaders::H_CONTENT_ENCODING,       //"content-encoding"
    HttpRespHeaders::H_CONTENT_ENCODING,       //"content-encoding"
    HttpRespHeaders::H_CONTENT_TYPE,           //"content-type"
    HttpRespHeaders::H_CONTENT_TYPE,           //"content-type"
    HttpRespHeaders::H_CONTENT_TYPE,           //"content-type"
    HttpRespHeaders::H_CONTENT_TYPE,           //"content-type"
    HttpRespHeaders::H_CONTENT_TYPE,           //"content-type"
    HttpRespHeaders::H_CONTENT_TYPE,           //"content-type"
    HttpRespHeaders::H_CONTENT_TYPE,           //"content-type"
    HttpRespHeaders::H_CONTENT_TYPE,           //"content-type"
    HttpRespHeaders::H_CONTENT_TYPE,           //"content-type"
    HttpRespHeaders::H_CONTENT_TYPE,           //"content-type"
    HttpRespHeaders::H_CONTENT_TYPE,           //"content-type"
    UPK_HDR_UNKNOWN,                           //"range"
    UPK_HDR_UNKNOWN,                           //"strict-transport-security"
    UPK_HDR_UNKNOWN,                           //"strict-transport-security"
    UPK_HDR_UNKNOWN,                           //"strict-transport-security"
    HttpRespHeaders::H_VARY,                   //"vary"
    HttpRespHeaders::H_VARY,                   //"vary"
    UPK_HDR_UNKNOWN,                           //"x-content-type-options"
    UPK_HDR_UNKNOWN,                           //"x-xss-protection"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_STATUS,                            //":status"
    UPK_HDR_UNKNOWN,                           //"accept-language"
    UPK_HDR_UNKNOWN,                           //"access-control-allow-credentials"
    UPK_HDR_UNKNOWN,                           //"access-control-allow-credentials"
    UPK_HDR_UNKNOWN,                           //"access-control-allow-headers"
    UPK_HDR_UNKNOWN,                           //"access-control-allow-methods"
    UPK_HDR_UNKNOWN,                           //"access-control-allow-methods"
    UPK_HDR_UNKNOWN,                           //"access-control-allow-methods"
    UPK_HDR_UNKNOWN,                           //"access-control-expose-headers"
    UPK_HDR_UNKNOWN,                           //"access-control-request-headers"
    UPK_HDR_UNKNOWN,                           //"access-control-request-method"
    UPK_HDR_UNKNOWN,                           //"access-control-request-method"
    HttpRespHeaders::H_ALT_SVC,                //"alt-svc"
    UPK_HDR_UNKNOWN,                           //"authorization"
    UPK_HDR_UNKNOWN,                           //"content-security-policy"
    UPK_HDR_UNKNOWN,                           //"early-data"
    UPK_HDR_UNKNOWN,                           //"expect-ct"
    UPK_HDR_UNKNOWN,                           //"forwarded"
    UPK_HDR_UNKNOWN,                           //"if-range"
    UPK_HDR_UNKNOWN,                           //"origin"
    UPK_HDR_UNKNOWN,                           //"purpose"
    HttpRespHeaders::H_SERVER,                 //"server"
    UPK_HDR_UNKNOWN,                           //"timing-allow-origin"
    UPK_HDR_UNKNOWN,                           //"upgrade-insecure-requests"
    UPK_HDR_UNKNOWN,                           //"user-agent"
    UPK_HDR_UNKNOWN,                           //"x-forwarded-for"
    UPK_HDR_UNKNOWN,                           //"x-frame-options"
    UPK_HDR_UNKNOWN,                           //"x-frame-options"
};


int HttpRespHeaders::qpack2RespIdx(int qpack_index)
{
    if (qpack_index >= 0 && qpack_index < QPACK_MAX_INDEX)
        return qpack2appresp[qpack_index];
    return UPK_HDR_UNKNOWN;
};


lsxpack_err_code UpkdRespHdrBuilder::process(lsxpack_header *hdr)
{
    if (hdr == NULL || hdr->buf == NULL)
        return end();

    if (hdr->name_len == 0)  //Ignore bad header without a name
        return LSXPACK_OK;

    int idx = UPK_HDR_UNKNOWN;
    if (!is_qpack)
        idx = HttpRespHeaders::hpack2RespIdx(hdr->hpack_index);
    else if (hdr->flags & LSXPACK_QPACK_IDX)
        idx = HttpRespHeaders::qpack2RespIdx(hdr->qpack_index);
    if (idx != UPK_HDR_UNKNOWN)
    {
        hdr->app_index = idx;
        hdr->flags = (lsxpack_flag)(hdr->flags | LSXPACK_APP_IDX);
    }
    const char *name = lsxpack_header_get_name(hdr);
    const char *val = lsxpack_header_get_value(hdr);
    if (*name == ':')
    {
        if (hdr->name_len == 7 && strncmp(name, ":status", 7) == 0)
        {
            int code = SC_200;
            if (!is_qpack)
            {
                switch(hdr->hpack_index)
                {
                case LSHPACK_HDR_STATUS_204:
                    code = SC_204;
                    break;
                case LSHPACK_HDR_STATUS_206:
                    code = SC_206;
                    break;
                case LSHPACK_HDR_STATUS_304:
                    code = SC_304;
                    break;
                case LSHPACK_HDR_STATUS_400:
                    code = SC_400;
                    break;
                case LSHPACK_HDR_STATUS_404:
                    code = SC_404;
                    break;
                case LSHPACK_HDR_STATUS_500:
                    code = SC_500;
                    break;
                default:
                    if (strncmp(val, "200", 3) != 0)
                    {
                        code = HttpStatusCode::getInstance().codeToIndex(val);
                        if (code == -1)
                            code = SC_200;
                    }
                    break;
                }
            }
            else
            {
                if (!(hdr->flags & LSXPACK_QPACK_IDX))
                    hdr->qpack_index = LSQPACK_TNV_STATUS_103;
                switch(hdr->qpack_index)
                {
                case LSQPACK_TNV_STATUS_100:
                    code = SC_100;
                    break;
                case LSQPACK_TNV_STATUS_200:
                    code = SC_200;
                    break;
                case LSQPACK_TNV_STATUS_204:
                    code = SC_204;
                    break;
                case LSQPACK_TNV_STATUS_206:
                    code = SC_206;
                    break;
                case LSQPACK_TNV_STATUS_302:
                    code = SC_302;
                    break;
                case LSQPACK_TNV_STATUS_304:
                    code = SC_304;
                    break;
                case LSQPACK_TNV_STATUS_400:
                    code = SC_400;
                    break;
                case LSQPACK_TNV_STATUS_403:
                    code = SC_403;
                    break;
                case LSQPACK_TNV_STATUS_404:
                    code = SC_404;
                    break;
                case LSQPACK_TNV_STATUS_421:
                    code = SC_421;
                    break;
                case LSQPACK_TNV_STATUS_425:
                    code = SC_425;
                    break;
                case LSQPACK_TNV_STATUS_500:
                    code = SC_500;
                    break;
                case LSQPACK_TNV_STATUS_503:
                    code = SC_503;
                    break;
                case LSQPACK_TNV_STATUS_103:
                default:
                    code = HttpStatusCode::getInstance().codeToIndex(val);
                    if (code == -1)
                        code = SC_200;
                    break;
                }
            }
            headers->addStatusLine(HTTP_1_1, code, 0);
            headers->m_working = NULL;
            if (connector)
                HttpCgiTool::processStatusCode(connector, code);
        }
        else
        {
            headers->m_working = NULL;
            return LSXPACK_ERR_UNNEC_REQ_PSDO_HDR;
        }
    }
    else
    {
        if (!regular_header)
        {
            if (!headers->getHttpCode())
            {
                headers->m_working = NULL;
                return LSXPACK_ERR_INCOMPL_REQ_PSDO_HDR;
            }
            regular_header = true;
        }
        if (idx == UPK_HDR_UNKNOWN)
        {
            idx = HttpRespHeaders::getIndex(name, hdr->name_len);
            for(const char *p = name; p < name + hdr->name_len; ++p)
                if (isupper(*p))
                {
                    headers->m_working = NULL;
                    return LSXPACK_ERR_UPPERCASE_HEADER;
                }
            if (idx == HttpRespHeaders::H_CONNECTION)
            {
                headers->m_working = NULL;
                // Ignore it silently is better way to handle it.
                return LSXPACK_OK;
                //return LSXPACK_ERR_BAD_REQ_HEADER;
            }
            if (idx != HttpRespHeaders::H_UNKNOWN)
            {
                hdr->app_index = idx;
                hdr->flags = (lsxpack_flag)(hdr->flags | LSXPACK_APP_IDX);
            }
        }
        total_size += hdr->name_len + hdr->val_len + 4;
        if (total_size >= 65535)
        {
            headers->m_working = NULL;
            return LSXPACK_ERR_HEADERS_TOO_LARGE;
        }
        if (!connector || HttpCgiTool::processHeaderLine2(connector, idx,
                name, hdr->name_len, val, hdr->val_len) == 1)
        {
            if (hdr == headers->m_working)
            {
                headers->commitWorkingHeader();
                if (hdr->app_index == HttpRespHeaders::H_LINK)
                {
                    if (memmem(val, hdr->val_len, "preload", 7) != NULL)
                        headers->m_flags |= HRH_F_HAS_PUSH;
                }
            }
            else
                headers->add((HttpRespHeaders::INDEX)idx, name, hdr->name_len,
                            val, hdr->val_len, LSI_HEADER_ADD);
        }
        else
        {
            headers->m_working = NULL;
        }
    }
    return LSXPACK_OK;
}


lsxpack_err_code UpkdRespHdrBuilder::end()
{
    if (!headers->getHttpCode())
        return LSXPACK_ERR_INCOMPL_REQ_PSDO_HDR;

    if (headers->m_working)
    {
        headers->m_working = NULL;
    }

    //headers->endHeader();
    return LSXPACK_OK;
}


lsxpack_header_t *UpkdRespHdrBuilder::prepareDecode(lsxpack_header_t *hdr,
                                                size_t mini_buf_size)
{
    assert(!hdr || !hdr->buf || hdr->buf == headers->m_buf.begin());
    if (mini_buf_size + headers->m_buf.size() >= MAX_BUF_SIZE)
    {
        if (hdr && hdr == headers->m_working)
            headers->m_working = NULL;
        return NULL;
    }
    if ((size_t)headers->m_buf.available() < mini_buf_size + 2)
    {
        size_t increase_to = (mini_buf_size + 2 + 255) & (~255);
        if (headers->m_buf.guarantee(increase_to) == -1)
            return NULL;
        assert((size_t)headers->m_buf.available() >= mini_buf_size + 2);
    }
    if (!hdr)
    {
        assert(headers->m_working == NULL);
        hdr = headers->m_working = headers->m_lsxpack.newObj();
        headers->m_lsxpack.pop();
        lsxpack_header_prepare_decode(hdr, headers->m_buf.begin(),
            headers->m_buf.size(), headers->m_buf.available() - 2);
    }
    else
    {
        if (!hdr->buf)
            lsxpack_header_prepare_decode(hdr, headers->m_buf.begin(),
                headers->m_buf.size(), headers->m_buf.available() - 2);
        else
        {
            if (hdr->buf != headers->m_buf.begin())
                hdr->buf = headers->m_buf.begin();
            hdr->val_len = headers->m_buf.available() - 2;
        }
    }
    assert(hdr->buf + hdr->name_offset + hdr->val_len <= headers->m_buf.buf_end());
    return hdr;
}


