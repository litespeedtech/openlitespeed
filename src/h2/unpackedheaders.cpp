#include "unpackedheaders.h"
#include <http/httpheader.h>
#include <http/httprespheaders.h>
#include <log4cxx/logger.h>
#include <lsr/ls_pool.h>
#include <util/stringtool.h>
#include <lshpack/lshpack.h>
#include <lsqpack.h>


UnpackedHeaders::UnpackedHeaders()
{
    LS_ZERO_FILL(m_url, m_methodLen);
    m_buf = new AutoBuf(1024);
    m_buf->append("\0\0\0\0", HEADER_BUF_PAD);
    m_lsxpack.alloc(32);
    m_lsxpack.setSize(UPK_PSDO_RESERVE);
    m_first_cookie_idx = UINT16_MAX;

}


UnpackedHeaders::~UnpackedHeaders()
{
    if (!m_shared_buf)
        delete m_buf;
    if (m_alloc_str)
    {
        if (m_url.ptr)
            ls_pfree(m_url.ptr);
        if (m_host.ptr)
            ls_pfree(m_host.ptr);
    }
}


void UnpackedHeaders::setSharedBuf(AutoBuf *buf)
{
    if (buf == m_buf)
        return;
    if (m_buf)
        delete m_buf;
    m_buf = buf;
    m_shared_buf = 1;
    m_last_shre_buf_size = m_buf->size();
}



void UnpackedHeaders::reset()
{
    if (m_alloc_str)
    {
        if (m_url.ptr)
            ls_pfree(m_url.ptr);
        if (m_host.ptr)
            ls_pfree(m_host.ptr);
    }
    LS_ZERO_FILL(m_url, m_methodLen);
    m_buf->resize(HEADER_BUF_PAD);
    m_lsxpack.clear();
    m_lsxpack.setSize(UPK_PSDO_RESERVE);
}


static const int hpack2appreq[LSHPACK_MAX_INDEX] = {
    HttpHeader::H_HOST,              //":authority"
    UPK_HDR_METHOD,                  //":method"
    UPK_HDR_METHOD,                  //":method"
    UPK_HDR_PATH,                    //":path"
    UPK_HDR_PATH,                    //":path"
    UPK_HDR_SCHEME,                  //":scheme"
    UPK_HDR_SCHEME,                  //":scheme"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    HttpHeader::H_ACC_CHARSET,       //"accept-charset"
    HttpHeader::H_ACC_ENCODING,      //"accept-encoding"
    HttpHeader::H_ACC_LANG,          //"accept-language"
    HttpHeader::H_UNKNOWN,           //"accept-ranges"
    HttpHeader::H_ACCEPT,            //"accept"
    HttpHeader::H_UNKNOWN,           //"access-control-allow-origin"
    HttpHeader::H_UNKNOWN,           //"age"
    HttpHeader::H_ALLOW,             //"allow"
    HttpHeader::H_AUTHORIZATION,     //"authorization"
    HttpHeader::H_CACHE_CTRL,        //"cache-control"
    HttpHeader::H_UNKNOWN,           //"content-disposition"
    HttpHeader::H_CONTENT_ENCODING,  //"content-encoding"
    HttpHeader::H_CONTENT_LANGUAGE,  //"content-language"
    HttpHeader::H_CONTENT_LENGTH,    //"content-length"
    HttpHeader::H_CONTENT_LOCATION,  //"content-location"
    HttpHeader::H_CONTENT_RANGE,     //"content-range"
    HttpHeader::H_CONTENT_TYPE,      //"content-type"
    HttpHeader::H_COOKIE,            //"cookie"
    HttpHeader::H_DATE,              //"date"
    HttpHeader::H_UNKNOWN,           //"etag"
    HttpHeader::H_EXPECT,            //"expect"
    HttpHeader::H_EXPIRES,           //"expires"
    HttpHeader::H_UNKNOWN,           //"from"
    HttpHeader::H_HOST,              //"host"
    HttpHeader::H_IF_MATCH,          //"if-match"
    HttpHeader::H_IF_MODIFIED_SINCE, //"if-modified-since"
    HttpHeader::H_IF_NO_MATCH,       //"if-none-match"
    HttpHeader::H_IF_RANGE,          //"if-range"
    HttpHeader::H_IF_UNMOD_SINCE,    //"if-unmodified-since"
    HttpHeader::H_LAST_MODIFIED,     //"last-modified"
    HttpHeader::H_UNKNOWN,           //"link"
    HttpHeader::H_UNKNOWN,           //"location"
    HttpHeader::H_MAX_FORWARDS,      //"max-forwards"
    HttpHeader::H_UNKNOWN,           //"proxy-authenticate"
    HttpHeader::H_PROXY_AUTH,        //"proxy-authorization"
    HttpHeader::H_RANGE,             //"range"
    HttpHeader::H_REFERER,           //"referer"
    HttpHeader::H_UNKNOWN,           //"refresh"
    HttpHeader::H_UNKNOWN,           //"retry-after"
    HttpHeader::H_UNKNOWN,           //"server"
    HttpHeader::H_UNKNOWN,           //"set-cookie"
    HttpHeader::H_UNKNOWN,           //"strict-transport-security"
    HttpHeader::H_TRANSFER_ENCODING, //"transfer-encoding"
    HttpHeader::H_USERAGENT,         //"user-agent"
    HttpHeader::H_UNKNOWN,           //"vary"
    HttpHeader::H_VIA,               //"via"
    HttpHeader::H_UNKNOWN,           //"www-authenticate"
};


inline int UnpackedHeaders::hpack2ReqIdx(int hpack_index)
{
    if (hpack_index > 0 && hpack_index <= LSHPACK_MAX_INDEX)
        return hpack2appreq[hpack_index - 1];
    return UPK_HDR_UNKNOWN;
};




#define QPACK_MAX_INDEX 99
static const int qpack2appreq[QPACK_MAX_INDEX] = {
    HttpHeader::H_HOST,              //":authority"
    UPK_HDR_PATH,                    //":path"
    HttpHeader::H_UNKNOWN,           //"age"
    HttpHeader::H_UNKNOWN,           //"content-disposition"
    HttpHeader::H_CONTENT_LENGTH,    //"content-length"
    HttpHeader::H_COOKIE,            //"cookie"
    HttpHeader::H_DATE,              //"date"
    HttpHeader::H_UNKNOWN,           //"etag"
    HttpHeader::H_IF_MODIFIED_SINCE, //"if-modified-since"
    HttpHeader::H_IF_NO_MATCH,       //"if-none-match"
    HttpHeader::H_LAST_MODIFIED,     //"last-modified"
    HttpHeader::H_UNKNOWN,           //"link"
    HttpHeader::H_UNKNOWN,           //"location"
    HttpHeader::H_REFERER,           //"referer"
    HttpHeader::H_UNKNOWN,           //"set-cookie"
    UPK_HDR_METHOD,                  //":method"
    UPK_HDR_METHOD,                  //":method"
    UPK_HDR_METHOD,                  //":method"
    UPK_HDR_METHOD,                  //":method"
    UPK_HDR_METHOD,                  //":method"
    UPK_HDR_METHOD,                  //":method"
    UPK_HDR_METHOD,                  //":method"
    UPK_HDR_SCHEME,                  //":scheme"
    UPK_HDR_SCHEME,                  //":scheme"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    HttpHeader::H_ACCEPT,            //"accept"
    HttpHeader::H_ACCEPT,            //"accept"
    HttpHeader::H_ACC_ENCODING,      //"accept-encoding"
    HttpHeader::H_UNKNOWN,           //"accept-ranges"
    HttpHeader::H_UNKNOWN,           //"access-control-allow-headers"
    HttpHeader::H_UNKNOWN,           //"access-control-allow-headers"
    HttpHeader::H_UNKNOWN,           //"access-control-allow-origin"
    HttpHeader::H_CACHE_CTRL,        //"cache-control"
    HttpHeader::H_CACHE_CTRL,        //"cache-control"
    HttpHeader::H_CACHE_CTRL,        //"cache-control"
    HttpHeader::H_CACHE_CTRL,        //"cache-control"
    HttpHeader::H_CACHE_CTRL,        //"cache-control"
    HttpHeader::H_CACHE_CTRL,        //"cache-control"
    HttpHeader::H_CONTENT_ENCODING,  //"content-encoding"
    HttpHeader::H_CONTENT_ENCODING,  //"content-encoding"
    HttpHeader::H_CONTENT_TYPE,      //"content-type"
    HttpHeader::H_CONTENT_TYPE,      //"content-type"
    HttpHeader::H_CONTENT_TYPE,      //"content-type"
    HttpHeader::H_CONTENT_TYPE,      //"content-type"
    HttpHeader::H_CONTENT_TYPE,      //"content-type"
    HttpHeader::H_CONTENT_TYPE,      //"content-type"
    HttpHeader::H_CONTENT_TYPE,      //"content-type"
    HttpHeader::H_CONTENT_TYPE,      //"content-type"
    HttpHeader::H_CONTENT_TYPE,      //"content-type"
    HttpHeader::H_CONTENT_TYPE,      //"content-type"
    HttpHeader::H_CONTENT_TYPE,      //"content-type"
    HttpHeader::H_RANGE,             //"range"
    HttpHeader::H_UNKNOWN,           //"strict-transport-security"
    HttpHeader::H_UNKNOWN,           //"strict-transport-security"
    HttpHeader::H_UNKNOWN,           //"strict-transport-security"
    HttpHeader::H_UNKNOWN,           //"vary"
    HttpHeader::H_UNKNOWN,           //"vary"
    HttpHeader::H_UNKNOWN,           //"x-content-type-options"
    HttpHeader::H_UNKNOWN,           //"x-xss-protection"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    UPK_HDR_STATUS,                  //":status"
    HttpHeader::H_ACC_LANG,          //"accept-language"
    HttpHeader::H_UNKNOWN,           //"access-control-allow-credentials"
    HttpHeader::H_UNKNOWN,           //"access-control-allow-credentials"
    HttpHeader::H_UNKNOWN,           //"access-control-allow-headers"
    HttpHeader::H_UNKNOWN,           //"access-control-allow-methods"
    HttpHeader::H_UNKNOWN,           //"access-control-allow-methods"
    HttpHeader::H_UNKNOWN,           //"access-control-allow-methods"
    HttpHeader::H_UNKNOWN,           //"access-control-expose-headers"
    HttpHeader::H_UNKNOWN,           //"access-control-request-headers"
    HttpHeader::H_UNKNOWN,           //"access-control-request-method"
    HttpHeader::H_UNKNOWN,           //"access-control-request-method"
    HttpHeader::H_UNKNOWN,           //"alt-svc"
    HttpHeader::H_AUTHORIZATION,     //"authorization"
    HttpHeader::H_UNKNOWN,           //"content-security-policy"
    HttpHeader::H_UNKNOWN,           //"early-data"
    HttpHeader::H_UNKNOWN,           //"expect-ct"
    HttpHeader::H_UNKNOWN,           //"forwarded"
    HttpHeader::H_IF_RANGE,          //"if-range"
    HttpHeader::H_UNKNOWN,           //"origin"
    HttpHeader::H_UNKNOWN,           //"purpose"
    HttpHeader::H_UNKNOWN,           //"server"
    HttpHeader::H_UNKNOWN,           //"timing-allow-origin"
    HttpHeader::H_UNKNOWN,           //"upgrade-insecure-requests"
    HttpHeader::H_USERAGENT,         //"user-agent"
    HttpHeader::H_X_FORWARDED_FOR,   //"x-forwarded-for"
    HttpHeader::H_UNKNOWN,           //"x-frame-options"
    HttpHeader::H_UNKNOWN,           //"x-frame-options"
};


int UnpackedHeaders::qpack2ReqIdx(int qpack_index)
{
    if (qpack_index >= 0 && qpack_index < QPACK_MAX_INDEX)
        return qpack2appreq[qpack_index];
    return UPK_HDR_UNKNOWN;
};


static const int s_appreq2hpack[46] = {
    14,    //":status"
     7,    //":scheme"
     5,    //":path"
     3,    //":method"
    19,    //"accept"
    15,    //"accept-charset"
    16,    //"accept-encoding"
    17,    //"accept-language"
    23,    //"authorization"
     0,    //"connection"
    31,    //"content-type"
    28,    //"content-length"
    32,    //"cookie"
     0,    //"cookie2"
    38,    //"host"
     0,    //"pragma"
    51,    //"referer"
    58,    //"user-agent"
    24,    //"cache-control"
    40,    //"if-modified-since"
    39,    //"if-match"
    41,    //"if-none-match"
    42,    //"if-range"
    43,    //"if-unmodified-since"
     0,    //"keep-alive"
    50,    //"range"
     0,    //"x-forwarded-for"
    60,    //"via"
    57,    //"transfer-encoding"
     0,    //"te"
    35,    //"expect"
    47,    //"max-forwards"
    49,    //"proxy-authorization"
     0,    //"proxy-connection"
    33,    //"date"
     0,    //"trailer"
     0,    //"upgrade"
     0,    //"warning"
    22,    //"allow"
    26,    //"content-encoding"
    27,    //"content-language"
    29,    //"content-location"
     0,    //"content-md5"
    30,    //"content-range"
    36,    //"expires"
    44,    //"last-modified"
};

static int
lookup_appreq2hpack (int idx)
{
    idx -= -4;
    if (idx >= 0 && (unsigned) idx < sizeof( s_appreq2hpack ) / sizeof( s_appreq2hpack[0]))
        return s_appreq2hpack[idx];
    else
        return 0;
}


static const int s_appreq2qpack[46] = {
    71,    //":status"
    23,    //":scheme"
     1,    //":path"
    21,    //":method"
    30,    //"accept"
    -1,    //"accept-charset"
    31,    //"accept-encoding"
    72,    //"accept-language"
    84,    //"authorization"
    -1,    //"connection"
    54,    //"content-type"
     4,    //"content-length"
     5,    //"cookie"
    -1,    //"cookie2"
    -1,    //"host"
    -1,    //"pragma"
    13,    //"referer"
    95,    //"user-agent"
    41,    //"cache-control"
     8,    //"if-modified-since"
    -1,    //"if-match"
     9,    //"if-none-match"
    89,    //"if-range"
    -1,    //"if-unmodified-since"
    -1,    //"keep-alive"
    55,    //"range"
    96,    //"x-forwarded-for"
    -1,    //"via"
    -1,    //"transfer-encoding"
    -1,    //"te"
    -1,    //"expect"
    -1,    //"max-forwards"
    -1,    //"proxy-authorization"
    -1,    //"proxy-connection"
     6,    //"date"
    -1,    //"trailer"
    -1,    //"upgrade"
    -1,    //"warning"
    -1,    //"allow"
    43,    //"content-encoding"
    -1,    //"content-language"
    -1,    //"content-location"
    -1,    //"content-md5"
    -1,    //"content-range"
    -1,    //"expires"
    10,    //"last-modified"
};

static int
lookup_appreq2qpack (int idx)
{
    idx -= -4;
    if (idx >= 0 && (unsigned) idx < sizeof( s_appreq2qpack ) / sizeof( s_appreq2qpack[0]))
        return s_appreq2qpack[idx];
    else
        return -1;
}


static const int s_qpack2hpack[99] = {
     1, //":authority"
     5, //":path"
    21, //"age"
    25, //"content-disposition"
    28, //"content-length"
    32, //"cookie"
    33, //"date"
    34, //"etag"
    40, //"if-modified-since"
    41, //"if-none-match"
    44, //"last-modified"
    45, //"link"
    46, //"location"
    51, //"referer"
    55, //"set-cookie"
     3, //":method"
     3, //":method"
     3, //":method"
     3, //":method"
     3, //":method"
     3, //":method"
     3, //":method"
     7, //":scheme"
     7, //":scheme"
    14, //":status"
    14, //":status"
    14, //":status"
    14, //":status"
    14, //":status"
    19, //"accept"
    19, //"accept"
    16, //"accept-encoding"
    18, //"accept-ranges"
     0, //"access-control-allow-headers"
     0, //"access-control-allow-headers"
    20, //"access-control-allow-origin"
    24, //"cache-control"
    24, //"cache-control"
    24, //"cache-control"
    24, //"cache-control"
    24, //"cache-control"
    24, //"cache-control"
    26, //"content-encoding"
    26, //"content-encoding"
    31, //"content-type"
    31, //"content-type"
    31, //"content-type"
    31, //"content-type"
    31, //"content-type"
    31, //"content-type"
    31, //"content-type"
    31, //"content-type"
    31, //"content-type"
    31, //"content-type"
    31, //"content-type"
    50, //"range"
    56, //"strict-transport-security"
    56, //"strict-transport-security"
    56, //"strict-transport-security"
    59, //"vary"
    59, //"vary"
     0, //"x-content-type-options"
     0, //"x-xss-protection"
    14, //":status"
    14, //":status"
    14, //":status"
    14, //":status"
    14, //":status"
    14, //":status"
    14, //":status"
    14, //":status"
    14, //":status"
    17, //"accept-language"
     0, //"access-control-allow-credentials"
     0, //"access-control-allow-credentials"
     0, //"access-control-allow-headers"
     0, //"access-control-allow-methods"
     0, //"access-control-allow-methods"
     0, //"access-control-allow-methods"
     0, //"access-control-expose-headers"
     0, //"access-control-request-headers"
     0, //"access-control-request-method"
     0, //"access-control-request-method"
     0, //"alt-svc"
    23, //"authorization"
     0, //"content-security-policy"
     0, //"early-data"
     0, //"expect-ct"
     0, //"forwarded"
    42, //"if-range"
     0, //"origin"
     0, //"purpose"
    54, //"server"
     0, //"timing-allow-origin"
     0, //"upgrade-insecure-requests"
    58, //"user-agent"
     0, //"x-forwarded-for"
     0, //"x-frame-options"
     0, //"x-frame-options"
};

int UnpackedHeaders::qpack2hpack (int idx)
{
    if (idx >= 0 && (unsigned) idx < sizeof(s_qpack2hpack) / sizeof(s_qpack2hpack[0]))
        return s_qpack2hpack[idx];
    else
        return 0;
}


static const int s_hpack2qpack[61] = {
     0, //":authority"
    21, //":method"
    21, //":method"
     1, //":path"
     1, //":path"
    23, //":scheme"
    23, //":scheme"
    71, //":status"
    71, //":status"
    71, //":status"
    71, //":status"
    71, //":status"
    71, //":status"
    71, //":status"
    -1, //"accept-charset"
    31, //"accept-encoding"
    72, //"accept-language"
    32, //"accept-ranges"
    30, //"accept"
    35, //"access-control-allow-origin"
     2, //"age"
    -1, //"allow"
    84, //"authorization"
    41, //"cache-control"
     3, //"content-disposition"
    43, //"content-encoding"
    -1, //"content-language"
     4, //"content-length"
    -1, //"content-location"
    -1, //"content-range"
    54, //"content-type"
     5, //"cookie"
     6, //"date"
     7, //"etag"
    -1, //"expect"
    -1, //"expires"
    -1, //"from"
    -1, //"host"
    -1, //"if-match"
     8, //"if-modified-since"
     9, //"if-none-match"
    89, //"if-range"
    -1, //"if-unmodified-since"
    10, //"last-modified"
    11, //"link"
    12, //"location"
    -1, //"max-forwards"
    -1, //"proxy-authenticate"
    -1, //"proxy-authorization"
    55, //"range"
    13, //"referer"
    -1, //"refresh"
    -1, //"retry-after"
    92, //"server"
    14, //"set-cookie"
    58, //"strict-transport-security"
    -1, //"transfer-encoding"
    95, //"user-agent"
    60, //"vary"
    -1, //"via"
    -1, //"www-authenticate"
};


int UnpackedHeaders::hpack2qpack (int idx)
{
    idx -= 1;
    if (idx >= 0 && (unsigned) idx < sizeof(s_hpack2qpack) / sizeof(s_hpack2qpack[0]))
        return s_hpack2qpack[idx];
    else
        return -1;
}


int UnpackedHeaders::appendHeaders(const char *pStr, int len)
{
    if (!pStr)
        return -1;
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

            int idx = HttpHeader::getIndex(pName, nameLen);
            if (appendHeader(idx, pName, nameLen, pVal, valLen) == -1)
                return -1;
        }
        else
            pLineBegin = pLineEnd + 1;
    }
    return 0;
}


inline void appendLowerCase(char *dest, const char *src, int n)
{
    const char *end = src + n;
    while (src < end)
        *dest++ = tolower(*src++);
}


int UnpackedHeaders::appendHeader(int index, const char *name, int name_len,
                                const char *val, int val_len)
{
    lsxpack_header *entry = m_lsxpack.newObj();
    if (!entry)
        return LS_FAIL;
    if (m_buf->guarantee(name_len + val_len + 6) == LS_FAIL)
    {
        m_lsxpack.pop();
        return LS_FAIL;
    }
    memset(entry, 0, sizeof(*entry));
    entry->app_index = index;
    if (index != HttpHeader::H_UNKNOWN && index != UPK_HDR_UNKNOWN)
        entry->flags = (lsxpack_flag)(entry->flags | LSXPACK_APP_IDX);

    entry->buf = m_buf->begin();
    entry->name_len = name_len;
    entry->val_len = val_len;
    entry->name_offset = m_buf->size();
    entry->val_offset = entry->name_offset + name_len + 2;
    if (name == m_buf->end())
    {
        m_buf->used(name_len + val_len + 4);
    }
    else
    {
        appendLowerCase(m_buf->end(), name, name_len);
        m_buf->used(name_len);
        m_buf->append_unsafe(':');
        m_buf->append_unsafe(' ');
        m_buf->append_unsafe(val, val_len);
        m_buf->append_unsafe('\r');
        m_buf->append_unsafe('\n');
    }
    if (index >= 0 && index < HttpHeader::H_TE)
    {
        if (m_commonHeaderPos[index] == 0)
            m_commonHeaderPos[index] = entry - m_lsxpack.begin();
    }
    return LS_OK;
}


int UnpackedHeaders::appendExistHeader(int index, int name_offset, int name_len,
                                  int val_offset, int val_len)
{
    lsxpack_header *entry = m_lsxpack.newObj();
    if (!entry)
        return LS_FAIL;
    memset(entry, 0, sizeof(*entry));
    if (index == UPK_HDR_UNKNOWN)
        index = HttpHeader::H_UNKNOWN;
    entry->app_index = index;
    if (index != HttpHeader::H_UNKNOWN)
        entry->flags = (lsxpack_flag)(entry->flags | LSXPACK_APP_IDX);

    entry->buf = m_buf->begin();
    entry->name_len = name_len;
    entry->val_len = val_len;
    entry->name_offset = name_offset;
    entry->val_offset = val_offset;
    if (index >= 0 && index < HttpHeader::H_TE)
    {
        if (m_commonHeaderPos[index] == 0)
            m_commonHeaderPos[index] = entry - m_lsxpack.begin();
    }
    return entry - m_lsxpack.begin();
}


int UnpackedHeaders::appendReqLine(const char *method, int method_len,
                                 const char *url, int url_len)
{
    if (m_buf->guarantee(method_len + url_len + 17) == LS_FAIL)
        return LS_FAIL;
    assert(m_buf->size() == HEADER_BUF_PAD);
    m_methodLen = method_len;
    m_url.len = url_len;
    m_buf->append_unsafe(method, method_len);
    m_buf->append_unsafe(' ');
    m_url_offset = m_buf->size();
    m_buf->append_unsafe(url, url_len);
    m_buf->append_unsafe(" HTTP/1.1\r\n", 11);
    return LS_OK;
}


int UnpackedHeaders::setReqLine(const char *method, int method_len,
                                    const char *uri, int uri_len,
                                    const char *qs, int qs_len)
{
    if (m_buf->guarantee(method_len + uri_len + qs_len + 18) == LS_FAIL)
        return LS_FAIL;
    if (m_buf->size() == HEADER_BUF_PAD)
    {
        m_buf->append_unsafe(method, method_len);
        m_buf->append_unsafe(' ');
    }
    else
    {
        assert(method_len < (int)(m_methodLen + m_url.len + 11));
        memmove(m_buf->get_ptr(HEADER_BUF_PAD), method, method_len);
    }
    m_methodLen = method_len;
    m_url.len = uri_len;
    m_url_offset = m_buf->size();
    m_buf->append_unsafe(uri, uri_len);
    if (qs_len > 0)
    {
        m_buf->append_unsafe('?');
        m_buf->append_unsafe(qs, qs_len);
        m_url.len += qs_len + 1;
    }
    m_buf->append_unsafe(" HTTP/1.1\r\n", 11);
    return LS_OK;
}


void UnpackedHeaders::endHeader()
{
    m_buf->append_unsafe('\r');
    m_buf->append_unsafe('\n');
}


int UnpackedHeaders::setMethod(const char *ptr, int len)
{
    if (m_buf->size() != HEADER_BUF_PAD)
        return LS_FAIL;
    m_buf->append(ptr, len);
    m_methodLen = len;
    m_buf->append(" ", 1);
    if (m_url.ptr)
    {
        appendUrl(m_url.ptr, m_url.len);
        if (m_alloc_str)
            ls_pfree(m_url.ptr);
        m_url.ptr = NULL;
    }
    return 0;
}


int UnpackedHeaders::setMethod2(lsxpack_header *hdr)
{
    if (m_methodLen)
        return LS_FAIL;
    if (m_buf->size() != HEADER_BUF_PAD)
    {
        int diff = hdr->val_len - 6;
        if (diff != 0)
        {
            m_buf->guarantee(m_buf->size() + hdr->val_len);
            char *ptr = m_buf->get_ptr(HEADER_BUF_PAD + hdr->val_len + 1);
            memmove(ptr, m_buf->get_ptr(HEADER_BUF_PAD + 7),
                    m_buf->end() - ptr);
            m_buf->used(diff);
            if (m_url_offset)
                m_url_offset += diff;
            if (m_host_offset)
            {
                m_host_offset += diff;
                m_lsxpack.get(3)->val_offset += diff;

            }
        }
        memmove(m_buf->get_ptr(HEADER_BUF_PAD), m_buf->get_ptr(hdr->val_offset),
                hdr->val_len);
    }
    else
    {
        m_buf->append(m_buf->get_ptr(hdr->val_offset), hdr->val_len);
    }
    m_methodLen = hdr->val_len;
    return 0;
}


int UnpackedHeaders::appendUrl(const char *ptr, int len)
{
    if (m_buf->guarantee(len + m_host.len + 17) == LS_FAIL)
        return LS_FAIL;
    m_url_offset = m_buf->size();
    m_buf->append(ptr, len);
    m_buf->append(" HTTP/1.1\r\n", 11);
    if (m_host.ptr)
    {
        m_host_offset = m_buf->size() + 6;
        appendHeader(HttpHeader::H_HOST, "host", 4, m_host.ptr, m_host.len);
        if (m_alloc_str)
            ls_pfree(m_host.ptr);
        m_host.ptr = NULL;
    }
    return LS_OK;
}


int UnpackedHeaders::setHost(const char *host, int len)
{
    if (m_host.len)
        return LS_FAIL;
    if (m_url.len && !m_url.ptr)
    {
        m_host_offset = m_buf->size() + 6;
        appendHeader(HttpHeader::H_HOST, "host", 4, host, len);
    }
    else
        m_host.ptr = (char *)host;
    m_host.len = len;
    return LS_OK;
}


int UnpackedHeaders::setHost2(lsxpack_header *hdr)
{
    if (m_host.len)
        return LS_FAIL;
    if (m_url.len && !m_url.ptr)
    {
        assert(hdr - m_lsxpack.begin() == 3);
        assert(hdr->buf == m_buf->begin() && m_buf->size() == hdr->name_offset);
        m_buf->append_unsafe("host: ", 6);
        assert(hdr->val_offset - m_buf->size() == 6);
        memmove(m_buf->end(), m_buf->get_ptr(hdr->val_offset), hdr->val_len + 2);
        m_buf->used(hdr->val_len + 2);
        hdr->val_offset -= 6;
        m_host_offset = hdr->val_offset;
    }
    else
    {
        ls_str(&m_host, m_buf->get_ptr(hdr->val_offset), hdr->val_len);
        m_alloc_str = 1;
    }
    hdr->name_offset = 0;
    hdr->name_len = 4;
    hdr->app_index = HttpHeader::H_HOST;
    hdr->flags = (lsxpack_flag)(hdr->flags | LSXPACK_APP_IDX);
    if (m_commonHeaderPos[HttpHeader::H_HOST] == 0)
        m_commonHeaderPos[HttpHeader::H_HOST] = 3;
    m_host.len = hdr->val_len;
    return LS_OK;
}


int UnpackedHeaders::setUrl2(lsxpack_header *hdr)
{
    if (m_url.len)
        return LS_FAIL;
    if (m_buf->size() != HEADER_BUF_PAD)
    {
        assert(m_buf->size() == hdr->name_offset);
        m_buf->append_unsafe(' ');
        hdr->val_offset -= 6;
        memmove(m_buf->end(), m_buf->end() + 6, hdr->val_len);
        m_buf->used(hdr->val_len);
    }
    else
    {
        assert(hdr->val_offset == HEADER_BUF_PAD + 7);
        memset(m_buf->end(), ' ', 7);
        m_buf->used(hdr->name_len + hdr->val_len + 2);
    }
    m_buf->append(" HTTP/1.1\r\n", 11);
    m_url_offset = hdr->val_offset;
    m_url.len = hdr->val_len;

    if (m_host.ptr)
    {
        if (m_buf->guarantee(m_host.len + 8) == LS_FAIL)
            return LS_FAIL;
        m_host_offset = m_buf->size() + 6;
        hdr = m_lsxpack.get(3);
        hdr->val_offset = m_host_offset;
        m_buf->append_unsafe("host: ", 6);
        m_buf->append_unsafe(m_host.ptr, m_host.len);
        m_buf->append_unsafe('\r');
        m_buf->append_unsafe('\n');
        if (m_alloc_str)
            ls_pfree(m_host.ptr);
        m_host.ptr = NULL;
    }

    return 0;
}


lsxpack_header *UnpackedHeaders::getFirstCookieHdr()
{   return (m_first_cookie_idx == UINT16_MAX) ? NULL
                : m_lsxpack.get(m_first_cookie_idx);  }


int UnpackedHeaders::updateHeader(int app_index, const char *val, int val_len)
{
    lsxpack_header *iter = NULL;
//     for(iter = m_lsxpack.begin() + UPK_PSDO_RESERVE;
//         iter < m_lsxpack.end(); ++iter)
//     {
//         if ((iter->flags & LSXPACK_APP_IDX) && iter->app_index == app_index)
//         {
//             if (iter->val_len >= val_len)
//             {
//                 if (iter->buf != m_buf->begin())
//                     iter->buf = m_buf->begin();
//                 memmove((void *)lsxpack_header_get_value(iter), val, val_len);
//                 iter->val_len = val_len;
//                 lsxpack_header_mark_val_changed(iter);
//                 return LS_OK;
//             }
//             else
//                 break;
//         }
//     }
    if (app_index >= 0 && app_index < HttpHeader::H_TE
        && m_commonHeaderPos[app_index] != 0)
    {
        iter = getEntry(m_commonHeaderPos[app_index]);
        if (iter)
        {
            assert(iter->app_index == app_index);
            lsxpack_header_mark_val_changed(iter);
            if (iter->val_len >= val_len)
            {
                if (iter->buf != m_buf->begin())
                    iter->buf = m_buf->begin();
                memmove((void *)lsxpack_header_get_value(iter), val, val_len);
                iter->val_len = val_len;
                return LS_OK;
            }
        }
    }
    if (!iter)
    {
        return appendHeader(app_index,
                            HttpHeader::getHeaderNameLowercase(app_index),
                            HttpHeader::getHeaderStringLen(app_index),
                            val, val_len);
    }

    iter->val_offset = m_buf->size();
    if (m_buf->append(val, val_len) < val_len)
        return LS_FAIL;
    iter->val_len = val_len;
    return LS_OK;
}


int UnpackedHeaders::dropHeader(int app_index, int val_offset)
{
    int count = 0;
    lsxpack_header *iter;
    for(iter = m_lsxpack.begin() + UPK_PSDO_RESERVE;
        iter < m_lsxpack.end(); ++iter)
    {
        if ((iter->flags & LSXPACK_APP_IDX) && iter->app_index == app_index)
        {
            if (val_offset > 0)
            {
                if (iter->val_offset == val_offset)
                {
                    iter->buf = LSXPACK_DEL;
                    return 1;
                }
            }
            else
            {
                iter->buf = LSXPACK_DEL;
                ++count;
            }
        }
    }
    return count;
}


int UnpackedHeaders::updateHost(const char *host, int len)
{
    m_host_offset = m_buf->size();
    m_buf->append(host, len);
    m_host.len = len;
    return LS_OK;
}


int UnpackedHeaders::finalizeCookieHeader(int count, uint16_t first_index,
                                          const char *value, int total_len)
{
    if (m_buf->guarantee(total_len + 12) == LS_FAIL)
    {
        return LS_FAIL;
    }
    int name_offset = m_buf->size();
    m_buf->append_unsafe("cookie: ", 8);

    int cookie_offset = m_buf->size();
    m_buf->append_unsafe(value, total_len);
    m_buf->append_unsafe("\r\n", 2);
    m_first_cookie_idx = first_index;
    int next_idx = first_index;
    do
    {
        assert(next_idx < m_lsxpack.size());
        lsxpack_header *hdr = m_lsxpack.get(next_idx);
        next_idx = hdr->chain_next_idx;
        assert(hdr->name_offset == 0);
        hdr->buf = m_buf->begin();
        hdr->name_offset = name_offset;
        hdr->name_len = 6;
        hdr->val_offset += cookie_offset;
        --count;
    } while (next_idx);
    assert(count == 0);
    return LS_OK;
}


int UnpackedHeaders::set(ls_str_t *method, ls_str_t* url,
                         ls_str_t* host, ls_strpair_t* headers)
{
    if (appendReqLine(method->ptr, method->len, url->ptr, url->len) == LS_FAIL)
        return LS_FAIL;
    m_host_offset = m_buf->size() + 6;
    if (appendHeader(HttpHeader::H_HOST, "host", 4,
                     host->ptr, host->len) == LS_FAIL)
        return LS_FAIL;

    if (headers)
    {
        while(headers->key.ptr)
        {
            int index = UPK_HDR_UNKNOWN;
            if (headers->key.len == 15
                && strcmp(headers->key.ptr, "accept-encoding") == 0)
                index = HttpHeader::H_ACC_ENCODING;
            else if(headers->key.len == 10
                && strcmp(headers->key.ptr, "user-agent") == 0)
                index = HttpHeader::H_USERAGENT;
            if (appendHeader(index, headers->key.ptr, headers->key.len,
                                 headers->val.ptr, headers->val.len) == LS_FAIL)
                return LS_FAIL;
            ++headers;
        }
    }
    endHeader();
    return LS_OK;
}


int UnpackedHeaders::copy(const UnpackedHeaders &rhs)
{
    m_buf->clear();
    if (m_buf->append(rhs.m_buf->begin(), rhs.m_buf->size()) == LS_FAIL)
        return LS_FAIL;
    m_lsxpack.copy(rhs.m_lsxpack);
    lsxpack_header *hdr;
    for(hdr = m_lsxpack.begin() + UPK_PSDO_RESERVE;
        hdr < m_lsxpack.end(); ++hdr)
    {
        hdr->buf = m_buf->begin();
    }
    memcpy(&m_url, &rhs.m_url,
           (char *)(&m_first_cookie_idx + 1) - (char *)&m_url);
    m_url.ptr = NULL;
    m_host.ptr = NULL;
    m_alloc_str = 0;
    return LS_OK;
}


static void buildQpackIdx(lsxpack_header *hdr)
{
    if (hdr->flags & LSXPACK_QPACK_IDX)
        return;
    int idx = -1;
    if (hdr->hpack_index)
        idx = UnpackedHeaders::hpack2qpack(hdr->hpack_index);
    if (idx == -1 && (hdr->flags & LSXPACK_APP_IDX)
        && hdr->app_index != HttpHeader::H_UNKNOWN)
        idx = lookup_appreq2qpack(hdr->app_index);
    if (idx != -1)
    {
        hdr->qpack_index = idx;
        hdr->flags = (lsxpack_flag)(hdr->flags | LSXPACK_QPACK_IDX);
    }
}


void UnpackedHeaders::buildQpackPsuedo()
{
    lsxpack_header *hdr = m_lsxpack.get(UPK_PSDO_RESERVE);

    assert(hdr->app_index == HttpHeader::H_HOST);
    hdr->qpack_index = LSQPACK_TNV_AUTHORITY;
    hdr->flags = (lsxpack_flag)(hdr->flags | LSXPACK_QPACK_IDX);
    hdr->name_len = 0;
    if (m_host.len)
    {
        hdr->val_offset = m_host_offset;
        hdr->val_len = m_host.len;
        lsxpack_header_mark_val_changed(hdr);
    }
    else
        assert(m_host_offset == 0 || hdr->val_offset == m_host_offset);
    hdr->buf = m_buf->begin();

    hdr = m_lsxpack.get(0);
    lsxpack_header_set_qpack_idx(hdr, is_http()?
                           LSQPACK_TNV_SCHEME_HTTP : LSQPACK_TNV_SCHEME_HTTPS,
                           "https", 5 - is_http());
    hdr->flags = (lsxpack_flag)(hdr->flags | LSXPACK_VAL_MATCHED);
    ++hdr;

    const char *method = getMethod();
    int len = getMethodLen();
    lsxpack_header_set_qpack_idx(hdr, LSQPACK_TNV_METHOD_GET, method, len);
    hdr->flags = (lsxpack_flag)(hdr->flags | LSXPACK_VAL_MATCHED);
    int m = HttpMethod::parse2(method);
    switch(m)
    {
    case HttpMethod::HTTP_OPTIONS:
        hdr->qpack_index = LSQPACK_TNV_METHOD_OPTIONS;
        break;
    case HttpMethod::HTTP_GET:
        hdr->qpack_index = LSQPACK_TNV_METHOD_GET;
        break;
    case HttpMethod::HTTP_HEAD:
        hdr->qpack_index = LSQPACK_TNV_METHOD_HEAD;
        break;
    case HttpMethod::HTTP_POST:
        hdr->qpack_index = LSQPACK_TNV_METHOD_POST;
        break;
    case HttpMethod::HTTP_PUT:
        hdr->qpack_index = LSQPACK_TNV_METHOD_PUT;
        break;
    case HttpMethod::HTTP_DELETE:
        hdr->qpack_index = LSQPACK_TNV_METHOD_DELETE;
        break;
    case HttpMethod::HTTP_CONNECT:
        hdr->qpack_index = LSQPACK_TNV_METHOD_CONNECT;
        break;
    default:
        hdr->qpack_index = LSQPACK_TNV_METHOD_CONNECT;
        hdr->flags = (lsxpack_flag)(hdr->flags & ~LSXPACK_VAL_MATCHED);
        break;
    }
    ++hdr;

    lsxpack_header_set_qpack_idx(hdr, LSQPACK_TNV_PATH, getUrl(), m_url.len);
    if (m_url.len == 1 )
        hdr->flags = (lsxpack_flag)(hdr->flags | LSXPACK_VAL_MATCHED);

}


void UnpackedHeaders::buildHpackPsuedo()
{
    lsxpack_header *hdr = m_lsxpack.get(UPK_PSDO_RESERVE);
    assert(hdr->app_index == HttpHeader::H_HOST);
    hdr->hpack_index = LSHPACK_HDR_AUTHORITY;
    hdr->name_len = 0;
    if (m_host.len)
    {
        hdr->val_offset = m_host_offset;
        hdr->val_len = m_host.len;
        lsxpack_header_mark_val_changed(hdr);
    }
    else
        assert(m_host_offset == 0 || hdr->val_offset == m_host_offset);
    hdr->buf = m_buf->begin();

    hdr = m_lsxpack.get(0);
    lsxpack_header_set_idx(hdr, is_http()?
                           LSHPACK_HDR_SCHEME_HTTP : LSHPACK_HDR_SCHEME_HTTPS,
                           "https", 5 - is_http());
    hdr->flags = (lsxpack_flag)(hdr->flags | LSXPACK_HPACK_VAL_MATCHED);
    ++hdr;

    int hpack_idx = LSHPACK_HDR_METHOD_GET;
    const char *method = getMethod();
    int method_len = getMethodLen();
    if (method_len == 4 && strncmp("POST", method, 4) == 0)
        hpack_idx = LSHPACK_HDR_METHOD_POST;

    lsxpack_header_set_idx(hdr, hpack_idx, method, method_len);
    if (hpack_idx == LSHPACK_HDR_METHOD_POST
        || (3 == method_len && strncmp("GET", method, 3) == 0))
        hdr->flags = (lsxpack_flag)(hdr->flags | LSXPACK_HPACK_VAL_MATCHED);
    ++hdr;

    lsxpack_header_set_idx(hdr, LSHPACK_HDR_PATH, getUrl(), getUrlLen());

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
        && hdr->app_index != HttpHeader::H_HEADER_END)
        idx = lookup_appreq2hpack(hdr->app_index);
    if (idx != LSHPACK_HDR_UNKNOWN)
    {
        hdr->hpack_index = idx;
    }
}


void UnpackedHeaders::prepareSendXpack(bool is_qpack)
{
    if (is_qpack)
        prepareSendQpack();
    else
        prepareSendHpack();
}


void UnpackedHeaders::prepareSendQpack()
{
    buildQpackPsuedo();

    lsxpack_header *hdr = m_lsxpack.begin() + UPK_PSDO_RESERVE + 1;
    for(; hdr < end(); ++hdr)
    {
        if (!hdr->buf)
            continue;
        if (hdr->buf != m_buf->begin())
        {
            hdr->buf = m_buf->begin();
        }
        assert(hdr->name_offset + hdr->name_len <= m_buf->size());
        assert(hdr->val_offset + hdr->val_len <= m_buf->size());

#ifndef NDEBUG
        char *p = hdr->buf + hdr->name_offset;
        char *end = p + hdr->name_len;
        while (p < end)
        {
            assert(*p == tolower(*p));
            ++p;
        }
#endif
        buildQpackIdx(hdr);
    }
}


void UnpackedHeaders::prepareSendHpack()
{
    buildHpackPsuedo();

    lsxpack_header *hdr = m_lsxpack.begin() + UPK_PSDO_RESERVE + 1;
    for(; hdr < end(); ++hdr)
    {
        if (!hdr->buf)
            continue;
        if (hdr->buf != m_buf->begin())
        {
            hdr->buf = m_buf->begin();
        }
        assert(hdr->name_offset + hdr->name_len <= m_buf->size());
        assert(hdr->val_offset + hdr->val_len <= m_buf->size());

#ifndef NDEBUG
        char *p = hdr->buf + hdr->name_offset;
        char *end = p + hdr->name_len;
        while (p < end)
        {
            assert(*p == tolower(*p));
            ++p;
        }
#endif
        buildHpackIdx(hdr);
    }
}


int UpkdHdrBuilder::guarantee(int size)
{
    int avail = tmp_max - tmp_used;
    if (avail > size)
        return LS_OK;
    int require = tmp_max << 1;
    while(require < tmp_used + size)
        require <<= 1;
    if (require >= 65536)
        return LS_FAIL;
    char *buf = (char *)malloc(require);
    if (!buf)
        return LS_FAIL;
    memmove(buf, tmp_buf, tmp_used);
    if (tmp_buf != fixed_buf)
        free(tmp_buf);
    tmp_buf = buf;
    tmp_max = require;
    return LS_OK;
}


lsxpack_err_code UpkdHdrBuilder::process(lsxpack_header *hdr)
{
    if (hdr == NULL || hdr->buf == NULL)
        return end();

    int idx = UPK_HDR_UNKNOWN;
    if (!is_qpack)
        idx = UnpackedHeaders::hpack2ReqIdx(hdr->hpack_index);
    else if (hdr->flags & LSXPACK_QPACK_IDX)
        idx = UnpackedHeaders::qpack2ReqIdx(hdr->qpack_index);
    const char *name = lsxpack_header_get_name(hdr);
    char *val = hdr->buf + hdr->val_offset;
    char *p = val;
    while (p < val + hdr->val_len
        && (p = (char *)memchr(p, '\n', val + hdr->val_len - p)))
        *p++ = ' ';

    if ((idx >= UPK_HDR_METHOD && idx <= UPK_HDR_STATUS)
        || (hdr->name_len > 2 && name[0] == ':'))
    {
        if (regular_header)
            return LSXPACK_ERR_MISPLACED_PSDO_HDR;
        if (hdr->val_len == 0)
            return LSXPACK_ERR_HEADERS_TOO_LARGE;   //invalid size
        if (idx == UPK_HDR_UNKNOWN)
        {
            switch(name[1])
            {
            case 'a':
                if (hdr->name_len == 10 && memcmp(name, ":authority", 10) == 0)
                    idx = HttpHeader::H_HOST;
                break;
            case 'm':
                if (hdr->name_len == 7 && memcmp(name, ":method", 7) == 0)
                    idx = UPK_HDR_METHOD;
                break;
            case 'p':
                if (hdr->name_len == 5 && memcmp(name, ":path", 5) == 0)
                    idx = UPK_HDR_PATH;
                break;
            case 's':
                if (hdr->name_len == 7 && memcmp(name, ":scheme", 7) == 0)
                    idx = UPK_HDR_SCHEME;
                break;
            default:
                return LSXPACK_ERR_UNNEC_REQ_PSDO_HDR;
            }
        }
        switch(idx)
        {
        case HttpHeader::H_HOST:         //":authority",
            if (headers->setHost2(hdr) == LS_FAIL)
                return LSXPACK_ERR_DUPLICATE_PSDO_HDR;
            working = NULL;
            return LSXPACK_OK;
        case UPK_HDR_METHOD:  //":method"
            //If second time have the :method, ERROR
            if (headers->setMethod2(hdr) == LS_FAIL)
                return LSXPACK_ERR_DUPLICATE_PSDO_HDR;
            break;
        case UPK_HDR_PATH:  //":path"
            //If second time have the :path, ERROR
            if (headers->setUrl2(hdr) == LS_FAIL)
                return LSXPACK_ERR_DUPLICATE_PSDO_HDR;
            break;
        case UPK_HDR_SCHEME:  //":scheme"
            if (scheme)
                return LSXPACK_ERR_DUPLICATE_PSDO_HDR;
            scheme = true;
            //Do nothing
            //We set to (char *)"HTTP/1.1"
            break;
        case UPK_HDR_STATUS:    //":status"
            return LSXPACK_ERR_UNNEC_REQ_PSDO_HDR;
        default:
            return LSXPACK_ERR_UNNEC_REQ_PSDO_HDR;
        }
        headers->m_lsxpack.pop();
        working = NULL;
    }
    else
    {
        if (!regular_header)
        {
            if (!headers->isComplete())
                return LSXPACK_ERR_INCOMPL_REQ_PSDO_HDR;
            regular_header = true;
        }
        if (idx == UPK_HDR_UNKNOWN)
        {
            for(const char *p = name; p < name + hdr->name_len; ++p)
                if (isupper(*p))
                    return LSXPACK_ERR_UPPERCASE_HEADER;
            if (hdr->name_len == 10 && memcmp(name, "connection", 10) == 0)
                return LSXPACK_ERR_BAD_REQ_HEADER;
            else if (hdr->name_len == 6 && memcmp(name, "cookie", 6) == 0)
                idx = HttpHeader::H_COOKIE;
            else if (hdr->name_len == 17 && memcmp(name, "transfer-encoding", 17) == 0)
                idx = HttpHeader::H_TRANSFER_ENCODING;
            else if (hdr->name_len == 0)
            {
                //NOTE: skip blank header
                headers->m_lsxpack.pop();
                working = NULL;
                return LSXPACK_OK;
            }
        }
        if (idx == HttpHeader::H_COOKIE)
        {
            if (guarantee(hdr->val_len + 2) == LS_FAIL)
            {
                return LSXPACK_ERR_NOMEM;
            }
            if (cookie_count > 0)
            {
                memcpy(tmp_buf + tmp_used, "; ", 2);
                tmp_used += 2;
            }
            hdr->buf = NULL;
            hdr->app_index = idx;
            hdr->name_offset = 0;
            hdr->name_len = 0;
            hdr->flags = (lsxpack_flag)(hdr->flags | LSXPACK_APP_IDX);
            hdr->val_offset = tmp_used;
            assert(hdr == working);
            working = NULL;
            int hdr_idx = hdr - headers->m_lsxpack.begin() ;
            if (first_cookie_idx == UINT16_MAX)
            {
                first_cookie_idx = last_cookie_idx = hdr_idx;
            }
            else
            {
                lsxpack_header *prev = headers->m_lsxpack.get(last_cookie_idx);
                assert(prev && prev->chain_next_idx == 0);
                prev->chain_next_idx = hdr_idx;
                last_cookie_idx = hdr_idx;
            }
            memcpy(tmp_buf + tmp_used, val, hdr->val_len);
            tmp_used += hdr->val_len;
            ++cookie_count;
            return LSXPACK_OK;
        }
        else if (idx == HttpHeader::H_TRANSFER_ENCODING)
        {
            if (hdr->val_len == 7 && strncasecmp(val, "chunked", 7) == 0)
                return LSXPACK_ERR_BAD_REQ_HEADER;
        }
        else if (hdr->name_len == 2 && memcmp(name, "te", 2) == 0)
        {
            if (hdr->val_len != 8 || strncasecmp("trailers", val, 8) != 0)
                return LSXPACK_ERR_BAD_REQ_HEADER;
        }
        total_size += hdr->name_len + hdr->val_len + 4;
        if (total_size >= 65535)
            return LSXPACK_ERR_HEADERS_TOO_LARGE;
        if (hdr == working)
        {
            hdr->app_index = idx;
            headers->m_buf->used(hdr->name_len + hdr->val_len + 4);
            working = NULL;
        }
        else
            headers->appendHeader(idx, name, hdr->name_len, val, hdr->val_len);
    }
    return LSXPACK_OK;
}


lsxpack_err_code UpkdHdrBuilder::end()
{
    if (!scheme || !headers->isComplete())
        return LSXPACK_ERR_INCOMPL_REQ_PSDO_HDR;

    if (working)
    {
        headers->m_lsxpack.pop();
        working = NULL;
    }

    if (cookie_count > 0)
    {
        total_size += tmp_used + 6 + 4;
        if (total_size >= 65535)
            return LSXPACK_ERR_HEADERS_TOO_LARGE;
        headers->finalizeCookieHeader(cookie_count, first_cookie_idx,
                                      tmp_buf, tmp_used);
    }
    headers->endHeader();
    return LSXPACK_OK;
}


lsxpack_header_t *UpkdHdrBuilder::prepareDecode(lsxpack_header_t *hdr,
                                                size_t mini_buf_size)
{
    assert(!hdr || !hdr->buf || hdr->buf == headers->m_buf->begin());

    if ((size_t)headers->m_buf->available() < mini_buf_size + 2)
    {
        size_t increase_to = (mini_buf_size + 2 + 255) & (~255);
        if (increase_to + headers->m_buf->size() >= MAX_BUF_SIZE)
        {
            if (hdr && hdr == working)
            {
                working = NULL;
                headers->m_lsxpack.pop();
            }
            return NULL;
        }
        if (headers->m_buf->guarantee(increase_to) == -1)
            return NULL;
        assert((size_t)headers->m_buf->available() >= mini_buf_size + 2);
    }
    if (!hdr)
    {
        assert(working == NULL);
        hdr = working = headers->m_lsxpack.newObj();
        lsxpack_header_prepare_decode(hdr, headers->m_buf->begin(),
            headers->m_buf->size(), headers->m_buf->available() - 2);
    }
    else
    {
        if (!hdr->buf)
            lsxpack_header_prepare_decode(hdr, headers->m_buf->begin(),
                headers->m_buf->size(), headers->m_buf->available() - 2);
        else
        {
            if (hdr->buf != headers->m_buf->begin())
                hdr->buf = headers->m_buf->begin();
            hdr->val_len = headers->m_buf->available() - 2;
        }
    }
    assert(hdr->buf + hdr->name_offset + hdr->val_len <= headers->m_buf->buf_end());
    return hdr;
}


void UnpackedHeaders::dump(LogSession *ls, const lsxpack_header *hdr,
                           const char *message)
{
    LS_DBG_L(ls, "%s: (%02d/%02d/%02d) %02X %.*s: %.*s", message,
             hdr->app_index, hdr->hpack_index, hdr->qpack_index,
             hdr->flags, hdr->name_len,
             lsxpack_header_get_name(hdr),
             hdr->val_len, hdr->buf + hdr->val_offset);
}

