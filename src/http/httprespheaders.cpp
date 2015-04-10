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
#include "http/httprespheaders.h"

#include <http/httpserverversion.h>
#include <http/httpstatusline.h>
#include <http/httpver.h>
#include <http/httpserverconfig.h>
#include <util/datetime.h>
#include <util/iovec.h>
#include <ctype.h>


#include <arpa/inet.h>

/*******************************************************************************
 *          Some comments about the resp_kvpair
 * keyOff is the real offset of the key/name,  valOff is the real offset of the value
 * next_index is 0 for sigle line case
 * FOR MULTIPLE LINE CASE, next_index is relative with the next kvpair position
 * The next_index of the first KVpair of the multiple line is (the next kvpair position + 1)
 * Then in all the chlidren KVPair, the next_index is (the next kvpair position + 1) | 0x80000000
 * The last one (does not have a child) is 0x80000000
 * ****************************************************************************/

#define HIGHEST_BIT_NUMBER                  0x80000000
#define BYPASS_HIGHEST_BIT_MASK             0x7FFFFFFF
#define MAX_RESP_HEADER_LEN                 8192



char HttpRespHeaders::s_sDateHeaders[30] = "Tue, 09 Jul 2013 13:43:01 GMT";
int             HttpRespHeaders::s_commonHeadersCount = 2;
http_header_t       HttpRespHeaders::s_commonHeaders[2];
http_header_t       HttpRespHeaders::s_gzipHeaders[2];
http_header_t       HttpRespHeaders::s_keepaliveHeader;
http_header_t       HttpRespHeaders::s_chunkedHeader;
http_header_t       HttpRespHeaders::s_concloseHeader;
http_header_t       HttpRespHeaders::s_acceptRangeHeader;

const char *HttpRespHeaders::m_sPresetHeaders[H_HEADER_END] =
{
    "accept-ranges",
    "connection",
    "content-type",
    "content-length",
    "content-encoding",
    "content-range",
    "content-disposition",
    "cache-control",
    "date",
    "etag",
    "expires",
    "keep-alive",
    "last-modified",
    "location",
    "x-litespeed-location",
    "x-litespeed-cache-control",
    "pragma",
    "proxy-connection",
    "server",
    "set-cookie",
    "status",
    "transfer-encoding",
    "vary",
    "www-authenticate",
    "x-powered-by"
};


int HttpRespHeaders::s_iHeaderLen[H_HEADER_END + 1] =
{
    13, 10, 12, 14, 16, 13, 19, 13, //cache-control
    4, 4, 7, 10, 13, 8, 20, 25, //x-litespeed-cache-control
    6, 16, 6, 10, 6, 17, 4, 16, 12, //x-powered-by
    0
};


HttpRespHeaders::HttpRespHeaders(ls_xpool_t *pool)
    : m_buf()
    , m_aKVPairs()
    , m_iHttpCode(SC_200)
{
    m_pool = pool;
    incKVPairs(16); //init 16 kvpair spaces
    memset(&m_hasHole, 0, &m_iKeepAlive + 1 - &m_hasHole);
}


void HttpRespHeaders::reset()
{
    if (m_hLastHeaderKVPairIndex == -1 && m_buf.size() == 0)
        return ;

    m_buf.clear();
    memset(m_KVPairindex, 0xFF, H_HEADER_END);
    m_iHttpCode = SC_200;
    memset(&m_hasHole, 0, &m_iKeepAlive + 1 - &m_hasHole);
    m_hLastHeaderKVPairIndex = -1;
    m_aKVPairs.init();
    m_aKVPairs.setSize(0);
}


inline void HttpRespHeaders::incKVPairs(int num)
{
    m_aKVPairs.guarantee(m_pool, m_aKVPairs.getSize() + num);
}


inline resp_kvpair *HttpRespHeaders::getKV(int index) const
{
    return m_aKVPairs.getObj(index);
}


inline resp_kvpair *HttpRespHeaders::getNewKV()
{
    resp_kvpair *tmp = m_aKVPairs.getNew();
    memset(tmp, 0, sizeof(resp_kvpair));
    return tmp;
}


//Replace the value with new value in pKv, in this case must have enough space
void HttpRespHeaders::replaceHeader(resp_kvpair *pKv, const char *pVal,
                                    unsigned int valLen)
{
    char *pOldVal = getVal(pKv);
    memcpy(pOldVal, pVal, valLen);
    memset(pOldVal + valLen, ' ', pKv->valLen - valLen);
}


int HttpRespHeaders::appendHeader(resp_kvpair *pKv, const char *pName,
                                  unsigned int nameLen, const char *pVal,
                                  unsigned int valLen, int method)
{
    if ( nameLen + valLen > MAX_RESP_HEADER_LEN
        || m_buf.size() >= HttpServerConfig::getInstance().getMaxDynRespHeaderLen())
        return LS_FAIL;

    if (method == LSI_HEADER_SET)
        memset(pKv, 0, sizeof(resp_kvpair));

    if (m_buf.available() < (int)(pKv->valLen + nameLen * 2 + valLen + 9))
    {
        if (m_buf.grow(pKv->valLen + nameLen * 2 + valLen + 9))
            return LS_FAIL;
    }

    //Be careful of the two Kv pointer
    resp_kvpair *pUpdKv = pKv;
    if (pKv->valLen > 0 && method == LSI_HEADER_ADD)
    {
        int new_id = getTotalCount();
        if (getFreeSpaceCount() <= 0)
            incKVPairs(10);
        if (pKv->next_index == 0)
            pKv->next_index = new_id + 1;
        else
        {
            while ((pKv->next_index & BYPASS_HIGHEST_BIT_MASK) > 0)
                pKv = getKV((pKv->next_index & BYPASS_HIGHEST_BIT_MASK) - 1);

            pKv->next_index = ((new_id + 1) | HIGHEST_BIT_NUMBER);
        }
        pUpdKv = getNewKV();
        assert(new_id == m_aKVPairs.getSize() - 1);
        pUpdKv->next_index = HIGHEST_BIT_NUMBER;
        assert(pUpdKv->valLen == 0);
    }

    pUpdKv->keyOff = m_buf.size();
    pUpdKv->keyLen = nameLen;

    m_buf.appendUnsafe(pName, nameLen);
    m_buf.used(-1 * nameLen);
    //Convert name to lower case to store it
    char *pCur = (char *)m_buf.end();
    char *pNameEnd = pCur + nameLen;
    while (pCur < pNameEnd)
    {
        *pCur = tolower(*pCur);
        ++pCur;
    }
    m_buf.used(nameLen);
    m_buf.appendUnsafe(": ", 2);

    if (pUpdKv->valLen > 0)  //only apply when append and merge
    {
        m_buf.appendUnsafe(m_buf.begin() + pUpdKv->valOff, pUpdKv->valLen);
        m_buf.appendUnsafe(",", 1);  //for append and merge case
        ++ pUpdKv->valLen;
    }
    m_buf.appendUnsafe(pVal, valLen);
    m_buf.appendUnsafe("\r\n", 2);
    pUpdKv->valOff = pUpdKv->keyOff + nameLen + 2;
    pUpdKv->valLen +=  valLen;
    return 0;
}


void HttpRespHeaders::verifyHeaderLength(HEADERINDEX headerIndex,
        const char *pName, unsigned int nameLen)
{
#ifndef NDEBUG
    assert(headerIndex == getRespHeaderIndex(pName));
    assert((int)nameLen == getHeaderStringLen(headerIndex));
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
        if (memcmp(p, val, valLen) == 0 &&
            ((pEnd - p == valLen) ? 1 : (*(p + valLen) == ',')))
            return 1;

        p = (const char *)memchr((void *)p, ',', pEnd - p);
        if (p)
            ++p;
        else
            break;
    }
    return 0;
}


//method: 0 replace,1 append, 2 merge, 3 add
int HttpRespHeaders::_add(int kvOrderNum, const char *pName, int nameLen,
                          const char *pVal, unsigned int valLen, int method)
{
    assert(kvOrderNum >= 0);
    resp_kvpair *pKv;
    if (kvOrderNum == getTotalCount())
    {
        //Add a new header
        if (getFreeSpaceCount() == 0)
            incKVPairs(10);
        pKv = getNewKV();
        ++m_iHeaderUniqueCount;
    }
    else
        pKv = getKV(kvOrderNum);

    //enough space for replace, use the same keyoff, and update valoff, add padding, make it is the same length as before
    if (method == LSI_HEADER_SET && pKv->keyLen == (int)nameLen
        && pKv->valLen >= (int)valLen)
    {
        replaceHeader(pKv, pVal, valLen);
        return 0;
    }

    if ((method == LSI_HEADER_MERGE) && (pKv->valLen > 0))
    {
        if (hasValue(getVal(pKv), pKv->valLen, pVal, valLen))
            return 0;//if exist when merge, ignor, otherwise same as append
    }

    m_hLastHeaderKVPairIndex = kvOrderNum;

    //Under append situation, if has existing key and valLen > 0, then makes a hole
    if (pKv->valLen > 0 && method != LSI_HEADER_ADD)
    {
        assert(pKv->keyLen > 0);
        m_hasHole = 1;
    }

    return appendHeader(pKv, pName, nameLen, pVal, valLen, method);
}


int HttpRespHeaders::add(HEADERINDEX headerIndex, const char *pVal,
                         unsigned int valLen, int method)
{
    if ((int)headerIndex < 0 || headerIndex >= H_HEADER_END)
        return LS_FAIL;

    if (m_KVPairindex[headerIndex] == 0xFF)
        m_KVPairindex[headerIndex] = getTotalCount();
    return _add(m_KVPairindex[headerIndex], m_sPresetHeaders[headerIndex],
                s_iHeaderLen[headerIndex], pVal, valLen, method);
}


int HttpRespHeaders::add(const char *pName, int nameLen, const char *pVal,
                         unsigned int valLen, int method)
{
    int kvOrderNum = -1;
    HEADERINDEX headerIndex = getRespHeaderIndex(pName);
    if (headerIndex != H_HEADER_END)
    {
        assert(s_iHeaderLen[headerIndex] == nameLen);
        assert(strncasecmp(m_sPresetHeaders[headerIndex], pName, nameLen) == 0);
        return add(headerIndex, pVal, valLen, method);
    }

    int ret = getHeaderKvOrder(pName, nameLen);
    if (ret != -1)
        kvOrderNum = ret;
    else
        kvOrderNum = getTotalCount();

    return _add(kvOrderNum, pName, nameLen, pVal, valLen, method);
}


//This will only append value to the last item
int HttpRespHeaders::appendLastVal(const char *pVal, int valLen)
{
    if (m_hLastHeaderKVPairIndex == -1 || valLen <= 0)
        return LS_FAIL;

    assert(m_hLastHeaderKVPairIndex < getTotalCount());
    resp_kvpair *pKv = getKV(m_hLastHeaderKVPairIndex);

    //check if it is the end
    if (m_buf.size() != pKv->valOff + pKv->valLen + 2)
        return LS_FAIL;

    if (pKv->keyLen == 0)
        return LS_FAIL;

    if (m_buf.available() < valLen)
    {
        if (m_buf.grow(valLen))
            return LS_FAIL;
    }

    //Only update the valLen of the kvpair
    pKv->valLen += valLen;
    m_buf.used(-2);  //move back two char, to replace the \r\n at the end
    m_buf.appendUnsafe(pVal, valLen);
    m_buf.appendUnsafe("\r\n", 2);

    return 0;
}


int HttpRespHeaders::add(http_header_t *headerArray, int size, int method)
{
    int ret = 0;
    for (int i = 0; i < size; ++i)
    {
        if (0 != add(headerArray[i].index, headerArray[i].val,
                     headerArray[i].valLen, method))
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

    resp_kvpair *pKv = getKV(kvOrderNum);
    if ((pKv->next_index & BYPASS_HIGHEST_BIT_MASK) > 0)
        _del((pKv->next_index & BYPASS_HIGHEST_BIT_MASK) - 1);

    memset(pKv, 0, sizeof(resp_kvpair));
    m_hasHole = 1;
    ++m_iHeaderRemovedCount;
}


//del( const char * pName, int nameLen ) is lower than  del( int headerIndex )
//
int HttpRespHeaders::del(const char *pName, int nameLen)
{
    assert(nameLen > 0);
    size_t idx = getRespHeaderIndex(pName);
    if (idx == H_HEADER_END)
    {
        int kvOrderNum = getHeaderKvOrder(pName, nameLen);
        _del(kvOrderNum);
        --m_iHeaderUniqueCount;
    }
    else
        del((HEADERINDEX)idx);
    return 0;
}


//del() will make some {0,0,0,0} kvpair in the list and make hole
int HttpRespHeaders::del(HEADERINDEX headerIndex)
{
    if (headerIndex < 0)
        return LS_FAIL;

    if (m_KVPairindex[headerIndex] != 0xFF)
    {
        _del(m_KVPairindex[headerIndex]);
        m_KVPairindex[headerIndex] = 0xFF;
        --m_iHeaderUniqueCount;
    }
    return 0;
}


char *HttpRespHeaders::getContentTypeHeader(int &len)
{
    int index = m_KVPairindex[HttpRespHeaders::H_CONTENT_TYPE];
    if (index == 0xFF)
    {
        len = -1;
        return NULL;
    }

    resp_kvpair *pKv = getKV(index);
    len = pKv->valLen;
    return getVal(pKv);
}


//The below calling will not maintance the kvpaire when regular case
int HttpRespHeaders::parseAdd(const char *pStr, int len, int method)
{
    //When SPDY format, we need to parse it and add one by one
    const char *pName = NULL;
    int nameLen = 0;
    const char *pVal = NULL;
    int valLen = 0;

    const char *pBEnd = pStr + len;
    const char *pMark = NULL;
    const char *pLineEnd = NULL;
    const char *pLineBegin  = pStr;

    while ((pLineEnd  = (const char *)memchr(pLineBegin, '\n',
                        pBEnd - pLineBegin)) != NULL)
    {
        pMark = (const char *)memchr(pLineBegin, ':', pLineEnd - pLineBegin);
        if (pMark != NULL)
        {
            pName = pLineBegin;
            nameLen = pMark - pLineBegin; //Should - 1 to remove the ':' position
            pVal = pMark + 2;
            if (*pVal == ' ')
                ++ pVal;
            valLen = pLineEnd - pVal;
            if (*(pLineEnd - 1) == '\r')
                -- valLen;

            //This way, all the value use APPEND as default
            if (add(pName, nameLen, pVal, valLen, method) == -1)
                return LS_FAIL;
        }

        pLineBegin = pLineEnd + 1;
        if (pBEnd <= pLineBegin + 1)
            break;
    }
    return 0;
}


int HttpRespHeaders::getHeaderKvOrder(const char *pName,
                                      unsigned int nameLen)
{
    int index = -1;
    int total = getTotalCount();
    resp_kvpair *pKv = NULL;

    for (int i = 0; i < total; ++i)
    {
        pKv = getKV(i);
        if (pKv->keyLen == (int)nameLen &&
            strncasecmp(pName, getName(pKv), nameLen) == 0)
        {
            index = i;
            break;
        }
    }

    return index;
}


int HttpRespHeaders::_getHeader(int kvOrderNum, char **pName, int *nameLen,
                                struct iovec *iov, int maxIovCount)
{
    int count = 0;
    resp_kvpair *pKv;
    if (nameLen)
        *nameLen = 0;
    while (kvOrderNum >= 0 && count < maxIovCount)
    {
        pKv = getKV(kvOrderNum);
        if (pKv->keyLen > 0 && pKv->valLen > 0)
        {
            if (pName && nameLen && 0 == *nameLen)
            {
                *pName = getName(pKv);
                *nameLen = pKv->keyLen;
            }
            iov[count].iov_base = getVal(pKv);
            iov[count++].iov_len = pKv->valLen;
        }
        kvOrderNum = (pKv->next_index & BYPASS_HIGHEST_BIT_MASK) - 1;
    }
    return count;
}


int  HttpRespHeaders::getHeader(HEADERINDEX index, struct iovec *iov,
                                int maxIovCount)
{
    int kvOrderNum = -1;
    if (m_KVPairindex[index] != 0xFF)
        kvOrderNum = m_KVPairindex[index];

    return _getHeader(kvOrderNum, NULL, NULL, iov, maxIovCount);
}


int HttpRespHeaders::getHeader(const char *pName, int nameLen,
                               struct iovec *iov, int maxIovCount)
{
    HEADERINDEX idx = getRespHeaderIndex(pName);
    if (idx != H_HEADER_END)
        return getHeader(idx, iov, maxIovCount);

    int kvOrderNum = getHeaderKvOrder(pName, nameLen);
    return _getHeader(kvOrderNum, NULL, NULL, iov, maxIovCount);
}


const char *HttpRespHeaders::getHeader(HEADERINDEX index,
                                       int *valLen) const
{
    resp_kvpair *pKv;
    if (m_KVPairindex[index] == 0xFF)
        return NULL;
    pKv = getKV(m_KVPairindex[index]);
    *valLen = pKv->valLen;
    return getVal(pKv);
}


int HttpRespHeaders::getFirstHeader(const char *pName, int nameLen,
                                    char **val, int &valLen)
{
    struct iovec iov[1];
    if (getHeader(pName, nameLen, iov, 1) == 1)
    {
        *val = (char *)iov[0].iov_base;
        valLen = iov[0].iov_len;
        return 0;
    }
    else
        return LS_FAIL;
}


HttpRespHeaders::HEADERINDEX HttpRespHeaders::getRespHeaderIndex(
    const char *pHeader)
{
    HEADERINDEX idx = H_HEADER_END;

    switch (*pHeader++ | 0x20)
    {
    case 'a':
        if (strncasecmp(pHeader, "ccept-ranges", 12) == 0)
            idx = H_ACCEPT_RANGES;
        break;
    case 'c':
        if (strncasecmp(pHeader, "onnection", 9) == 0)
            idx = H_CONNECTION;
        else if (strncasecmp(pHeader, "ontent-type", 11) == 0)
            idx = H_CONTENT_TYPE;
        else if (strncasecmp(pHeader, "ontent-length", 13) == 0)
            idx = H_CONTENT_LENGTH;
        else if (strncasecmp(pHeader, "ontent-encoding", 15) == 0)
            idx = H_CONTENT_ENCODING;
        else if (strncasecmp(pHeader, "ontent-range", 12) == 0)
            idx = H_CONTENT_RANGE;
        else if (strncasecmp(pHeader, "ontent-disposition", 18) == 0)
            idx = H_CONTENT_DISPOSITION;
        else if (strncasecmp(pHeader, "ache-control", 12) == 0)
            idx = H_CACHE_CTRL;
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
        if (strncasecmp(pHeader, "ast-modified", 12) == 0)
            idx = H_LAST_MODIFIED;
        else if (strncasecmp(pHeader, "ocation", 7) == 0)
            idx = H_LOCATION;
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
        break;
    case 'w':
        if (strncasecmp(pHeader, "ww-authenticate", 15) == 0)
            idx = H_WWW_AUTHENTICATE;
        break;
    case 'x':
        if (strncasecmp(pHeader, "-powered-by", 11) == 0)
            idx = H_X_POWERED_BY;
        else if (strncasecmp(pHeader, "-litespeed-location", 19) == 0)
            idx = H_LITESPEED_LOCATION;
        else if (strncasecmp(pHeader, "-litespeed-cache-control", 24) == 0)
            idx = H_LITESPEED_CACHE_CONTROL;
        break;
    default:
        break;
    }
    return idx;
}


int HttpRespHeaders::nextHeaderPos(int pos)
{
    int ret = -1;
    int total = getTotalCount();
    for (int i = pos + 1; i < total; ++i)
    {
        if (getKV(i)->keyLen > 0
            && ((getKV(i)->next_index & HIGHEST_BIT_NUMBER) == 0))
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
    int total = getTotalCount();
//     if (withStatusLine)
//     {
//         const StatusLineString& statusLine = HttpStatusLine::getStatusLine( m_iHttpVersion, m_iHttpCode );
//         iov[count].iov_base = (void *)statusLine.get();
//         iov[count].iov_len = statusLine.getLen();
//         ++count;
//     }

    for (int i = 0; i < total && count < maxIovCount; ++i)
    {
        resp_kvpair *pKv = getKV(i);
        if (pKv->keyLen > 0)
        {
            iov_key[count].iov_base = m_buf.begin() + pKv->keyOff;
            iov_key[count].iov_len = pKv->keyLen;
            iov_val[count].iov_base = m_buf.begin() + pKv->valOff;
            iov_val[count].iov_len = pKv->valLen;
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
    resp_kvpair *pKv;

    for (int i = 0; (pKv = getKV(i)) != NULL; ++i)
    {
        if ((pKv->keyLen > 0)
            && ((pKv->keyLen != nameLen)
                || (strncasecmp(m_buf.begin() + pKv->keyOff, pName, nameLen))))
        {
            iovec->appendCombine(m_buf.begin() + pKv->keyOff,
                                 pKv->keyLen + pKv->valLen + 4);
            total += pKv->keyLen + pKv->valLen + 4;
        }
    }
    return total;
}


int HttpRespHeaders::appendToIov(IOVec *iovec) const
{
    int total = 0;
    if (m_hasHole == 0)
    {
        if (m_buf.size() > 0)
            iovec->append(m_buf.begin(), m_buf.size());
        total = m_buf.size();
    }
    else
        total = appendToIovExclude(iovec, NULL, 0);
    return total;
}


/***
 *
 * The below functions may be moved to another classes
 */
int HttpRespHeaders::outputNonSpdyHeaders(IOVec *iovec)
{
    if (m_iKeepAlive)
    {
        if (m_iHttpVersion != HTTP_1_1)
            add(&HttpRespHeaders::s_keepaliveHeader, 1);
    }
    else
        add(&HttpRespHeaders::s_concloseHeader, 1);

    iovec->clear();
    const StatusLineString &statusLine =
            HttpStatusLine::getInstance().getStatusLine(m_iHttpVersion,
                                                        m_iHttpCode);
    iovec->push_front(statusLine.get(), statusLine.getLen());
    m_iHeadersTotalLen = statusLine.getLen();
    m_iHeadersTotalLen += appendToIov(iovec);

    iovec->append("\r\n", 2);
    m_iHeadersTotalLen += 2;
    m_iHeaderBuilt = 1;
    return 0;
}


void HttpRespHeaders::buildCommonHeaders()
{
    HttpRespHeaders::s_commonHeaders[0].index    = HttpRespHeaders::H_DATE;
    HttpRespHeaders::s_commonHeaders[0].val      =
        HttpRespHeaders::s_sDateHeaders;
    HttpRespHeaders::s_commonHeaders[0].valLen   = 29;

    HttpRespHeaders::s_commonHeaders[1].index    = HttpRespHeaders::H_SERVER;
    HttpRespHeaders::s_commonHeaders[1].val      =
        HttpServerVersion::getVersion();
    HttpRespHeaders::s_commonHeaders[1].valLen   =
        HttpServerVersion::getVersionLen();


    HttpRespHeaders::s_gzipHeaders[0].index    =
        HttpRespHeaders::H_CONTENT_ENCODING;
    HttpRespHeaders::s_gzipHeaders[0].val      = "gzip";
    HttpRespHeaders::s_gzipHeaders[0].valLen   = 4;

    HttpRespHeaders::s_gzipHeaders[1].index    = HttpRespHeaders::H_VARY;
    HttpRespHeaders::s_gzipHeaders[1].val      = "accept-encoding";
    HttpRespHeaders::s_gzipHeaders[1].valLen   = 15;

    HttpRespHeaders::s_keepaliveHeader.index    =
        HttpRespHeaders::H_CONNECTION;
    HttpRespHeaders::s_keepaliveHeader.val      = "keep-alive";
    HttpRespHeaders::s_keepaliveHeader.valLen   = 10;

    HttpRespHeaders::s_concloseHeader.index    = HttpRespHeaders::H_CONNECTION;
    HttpRespHeaders::s_concloseHeader.val      = "close";
    HttpRespHeaders::s_concloseHeader.valLen   = 5;

    HttpRespHeaders::s_chunkedHeader.index    =
        HttpRespHeaders::H_TRANSFER_ENCODING;
    HttpRespHeaders::s_chunkedHeader.val      = "chunked";
    HttpRespHeaders::s_chunkedHeader.valLen   = 7;


    HttpRespHeaders::s_acceptRangeHeader.index    =
        HttpRespHeaders::H_ACCEPT_RANGES;
    HttpRespHeaders::s_acceptRangeHeader.val      = "bytes";
    HttpRespHeaders::s_acceptRangeHeader.valLen   = 5;

    updateDateHeader();
}


void HttpRespHeaders::updateDateHeader()
{
    char achDateTime[60];
    char *p = DateTime::getRFCTime(DateTime::s_curTime, achDateTime);
    if (p)
        memcpy(HttpRespHeaders::s_sDateHeaders, achDateTime, 29);
}


void HttpRespHeaders::hideServerSignature(int hide)
{
    if (hide == 2) //hide all
        HttpRespHeaders::s_commonHeadersCount = 1;
    else
    {
        HttpServerVersion::hideDetail(!hide);
        HttpRespHeaders::s_commonHeaders[1].valLen   =
            HttpServerVersion::getVersionLen();
    }

}

