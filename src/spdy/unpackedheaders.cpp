/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2020  LiteSpeed Technologies, Inc.                 *
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
#include "unpackedheaders.h"
#include <http/httpheader.h>

#include <lsr/ls_pool.h>


UnpackedHeaders::UnpackedHeaders()
{
    LS_ZERO_FILL(m_url, m_methodLen);
    m_buf.append("\0\0\0\0", 4);
    m_entries.alloc(16);
}


UnpackedHeaders::~UnpackedHeaders()
{
    if (m_alloc_str)
    {
        if (m_url.ptr)
            ls_pfree(m_url.ptr);
        if (m_host.ptr)
            ls_pfree(m_host.ptr);
    }
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
    m_buf.resize(4);
    m_entries.clear();
}


static int lookup[] =
{
    HttpHeader::H_HOST,             //":authority",
    UPK_HDR_METHOD,                 //":method",
    UPK_HDR_METHOD,                 //":method",
    UPK_HDR_PATH,                   //":path",
    UPK_HDR_PATH,                   //":path",
    UPK_HDR_SCHEME,                 //":scheme",
    UPK_HDR_SCHEME,                 //":scheme",
    UPK_HDR_STATUS,                 //":status",
    UPK_HDR_STATUS,                 //":status",
    UPK_HDR_STATUS,                 //":status",
    UPK_HDR_STATUS,                 //":status",
    UPK_HDR_STATUS,                 //":status",
    UPK_HDR_STATUS,                 //":status",
    UPK_HDR_STATUS,                 //":status",
    HttpHeader::H_ACC_CHARSET,      //"accept-charset",
    HttpHeader::H_ACC_ENCODING,     //"accept-encoding",
    HttpHeader::H_ACC_LANG,         //"accept-language",
    -5,                             //"accept-ranges",
    HttpHeader::H_ACCEPT,           //"accept",
    -6,                             //"access-control-allow-origin",
    -7,                             //"age",
    -8,                             //"allow",
    HttpHeader::H_AUTHORIZATION,    //"authorization",
    HttpHeader::H_CACHE_CTRL,       //"cache-control",
    -9,                             //"content-disposition",
    HttpHeader::H_CONTENT_ENCODING, //"content-encoding",
    -10,                            //"content-language",
    HttpHeader::H_CONTENT_LENGTH,   //"content-length",
    HttpHeader::H_CONTENT_LOCATION, //"content-location",
    HttpHeader::H_CONTENT_RANGE,    //"content-range",
    HttpHeader::H_CONTENT_TYPE,     //"content-type",
    HttpHeader::H_COOKIE,           //"cookie",
    HttpHeader::H_DATE,             //"date",
    -11,                            //"etag",
    HttpHeader::H_EXPECT,           //"expect",
    HttpHeader::H_EXPIRES,          //"expires",
    -12,                            //"from",
    HttpHeader::H_HOST,             //"host",
    HttpHeader::H_IF_MATCH,         //"if-match",
    HttpHeader::H_IF_MODIFIED_SINCE,//"if-modified-since",
    HttpHeader::H_IF_NO_MATCH,      //"if-none-match",
    HttpHeader::H_IF_RANGE,         //"if-range",
    HttpHeader::H_IF_UNMOD_SINCE,   //"if-unmodified-since",
    HttpHeader::H_LAST_MODIFIED,    //"last-modified",
    -12,                            //"link",
    -13,                            //"location",
    -14,                            //"max-forwards",
    -15,                            //"proxy-authenticate",
    HttpHeader::H_PROXY_AUTH,       //"proxy-authorization",
    HttpHeader::H_RANGE,            //"range",
    HttpHeader::H_REFERER,          //"referer",
    -16,                            //"refresh",
    -17,                            //"retry-after",
    -18,                            //"server",
    -19,                            //"set-cookie",
    -20,                            //"strict-transport-security",
    HttpHeader::H_TRANSFER_ENCODING,//"transfer-encoding",
    HttpHeader::H_USERAGENT,        //"user-agent",
    -21,                            //"vary",
    HttpHeader::H_VIA,              //"via",
    -22,                            //"www-authenticate",
    HttpHeader::H_HOST,             //":authority"
    UPK_HDR_PATH,                   //":path"
    -23,                            //"age"
    -24,                            //"content-disposition"
    HttpHeader::H_CONTENT_LENGTH,   //"content-length"
    HttpHeader::H_COOKIE,           //"cookie"
    HttpHeader::H_DATE,             //"date"
    -25,                            //"etag"
    HttpHeader::H_IF_MODIFIED_SINCE,//"if-modified-since"
    HttpHeader::H_IF_NO_MATCH,      //"if-none-match"
    HttpHeader::H_LAST_MODIFIED,    //"last-modified"
    -26,                            //"link"
    -27,                            //"location"
    HttpHeader::H_REFERER,          //"referer"
    -28,                            //"set-cookie"
    UPK_HDR_METHOD,                 //":method"
    UPK_HDR_METHOD,                 //":method"
    UPK_HDR_METHOD,                 //":method"
    UPK_HDR_METHOD,                 //":method"
    UPK_HDR_METHOD,                 //":method"
    UPK_HDR_METHOD,                 //":method"
    UPK_HDR_METHOD,                 //":method"
    UPK_HDR_SCHEME,                 //":scheme"
    UPK_HDR_SCHEME,                 //":scheme"
    UPK_HDR_STATUS,                 //":status"
    UPK_HDR_STATUS,                 //":status"
    UPK_HDR_STATUS,                 //":status"
    UPK_HDR_STATUS,                 //":status"
    UPK_HDR_STATUS,                 //":status"
    HttpHeader::H_ACCEPT,           //"accept"
    HttpHeader::H_ACCEPT,           //"accept"
    HttpHeader::H_ACC_ENCODING,     //"accept-encoding"
    -29,                            //"accept-ranges"
    -30,                            //"access-control-allow-headers"
    -31,                            //"access-control-allow-headers"
    -32,                            //"access-control-allow-origin"
    HttpHeader::H_CACHE_CTRL,       //"cache-control"
    HttpHeader::H_CACHE_CTRL,       //"cache-control"
    HttpHeader::H_CACHE_CTRL,       //"cache-control"
    HttpHeader::H_CACHE_CTRL,       //"cache-control"
    HttpHeader::H_CACHE_CTRL,       //"cache-control"
    HttpHeader::H_CACHE_CTRL,       //"cache-control"
    HttpHeader::H_CONTENT_ENCODING, //"content-encoding"
    HttpHeader::H_CONTENT_ENCODING, //"content-encoding"
    HttpHeader::H_CONTENT_TYPE,     //"content-type"
    HttpHeader::H_CONTENT_TYPE,     //"content-type"
    HttpHeader::H_CONTENT_TYPE,     //"content-type"
    HttpHeader::H_CONTENT_TYPE,     //"content-type"
    HttpHeader::H_CONTENT_TYPE,     //"content-type"
    HttpHeader::H_CONTENT_TYPE,     //"content-type"
    HttpHeader::H_CONTENT_TYPE,     //"content-type"
    HttpHeader::H_CONTENT_TYPE,     //"content-type"
    HttpHeader::H_CONTENT_TYPE,     //"content-type"
    HttpHeader::H_CONTENT_TYPE,     //"content-type"
    HttpHeader::H_CONTENT_TYPE,     //"content-type"
    HttpHeader::H_RANGE,            //"range"
    -33,                            //"strict-transport-security"
    -34,                            //"strict-transport-security"
    -35,                            //"strict-transport-security"
    -36,                            //"vary"
    -37,                            //"vary"
    -38,                            //"x-content-type-options"
    -39,                            //"x-xss-protection"
    UPK_HDR_STATUS,                 //":status"
    UPK_HDR_STATUS,                 //":status"
    UPK_HDR_STATUS,                 //":status"
    UPK_HDR_STATUS,                 //":status"
    UPK_HDR_STATUS,                 //":status"
    UPK_HDR_STATUS,                 //":status"
    UPK_HDR_STATUS,                 //":status"
    UPK_HDR_STATUS,                 //":status"
    UPK_HDR_STATUS,                 //":status"
    HttpHeader::H_ACC_LANG,         //"accept-language"
    -40,                            //"access-control-allow-credentials"
    -41,                            //"access-control-allow-credentials"
    -42,                            //"access-control-allow-headers"
    -43,                            //"access-control-allow-methods"
    -44,                            //"access-control-allow-methods"
    -45,                            //"access-control-allow-methods"
    -46,                            //"access-control-expose-headers"
    -47,                            //"access-control-request-headers"
    -48,                            //"access-control-request-method"
    -49,                            //"access-control-request-method"
    -50,                            //"alt-svc"
    HttpHeader::H_AUTHORIZATION,    //"authorization"
    -51,                            //"content-security-policy"
    -52,                            //"early-data"
    -53,                            //"expect-ct"
    -54,                            //"forwarded"
    HttpHeader::H_IF_RANGE,         //"if-range"
    -55,                            //"origin"
    -56,                            //"purpose"
    -57,                            //"server"
    -58,                            //"timing-allow-origin"
    -59,                            //"upgrade-insecure-requests"
    HttpHeader::H_USERAGENT,        //"user-agent"
    HttpHeader::H_X_FORWARDED_FOR,  //"x-forwarded-for"
    -61,                            //"x-frame-options"
    -62,                            //"x-frame-options"
};

// Handles both HPACK and QPACK indexes.  HPACK indexes are in the range
// 0 - 60, while QPACK indexes are in the range 61 - 159.
//
int UnpackedHeaders::convertHorQpackIdx(int hpack_index)
{
    if (hpack_index > 0 && hpack_index <= (int)(sizeof(lookup)/sizeof(int)))
        return lookup[hpack_index - 1];
    return UPK_HDR_UNKNOWN;
};


#define HPACK_MAX_INDEX 61
int UnpackedHeaders::convertHpackIdx(int hpack_index)
{
    if (hpack_index > 0 && hpack_index <= HPACK_MAX_INDEX)
        return lookup[hpack_index - 1];
    return UPK_HDR_UNKNOWN;
};


int UnpackedHeaders::appendHeader(int index, const char *name, int name_len,
                                const char *val, int val_len)
{
    req_header_entry * entry = m_entries.newObj();
    if (!entry)
        return LS_FAIL;
    if (m_buf.guarantee(name_len + val_len + 6) == LS_FAIL)
    {
        m_entries.pop();
        return LS_FAIL;
    }
    entry->name_index = index;
    entry->name_len = name_len;
    entry->val_len = val_len;
    entry->name_offset = m_buf.size();

    m_buf.append_unsafe(name, name_len);
    m_buf.append_unsafe(':');
    m_buf.append_unsafe(' ');
    m_buf.append_unsafe(val, val_len);
    m_buf.append_unsafe('\r');
    m_buf.append_unsafe('\n');
    return LS_OK;
}


int UnpackedHeaders::appendReqLine(const char *method, int method_len,
                                 const char *url, int url_len)
{
    if (m_buf.guarantee(method_len + url_len + 17) == LS_FAIL)
        return LS_FAIL;
    m_buf.append_unsafe(method, method_len);
    m_buf.append_unsafe(' ');
    m_buf.append_unsafe(url, url_len);
    m_buf.append_unsafe(" HTTP/1.1\r\n", 11);
    return LS_OK;
}


void UnpackedHeaders::endHeader()
{
    m_buf.append_unsafe('\r');
    m_buf.append_unsafe('\n');
}

int UnpackedHeaders::setMethod(const char *ptr, int len)
{
    if (m_buf.size() != 4)
        return LS_FAIL;
    m_buf.append(ptr, len);
    m_methodLen = len;
    m_buf.append(" ", 1);
    if (m_url.ptr)
    {
        appendUrl(m_url.ptr, m_url.len);
        if (m_alloc_str)
            ls_pfree(m_url.ptr);
        m_url.ptr = NULL;
    }
    return 0;
}


int UnpackedHeaders::appendUrl(const char *ptr, int len)
{
    if (m_buf.guarantee(len + m_host.len + 17) == LS_FAIL)
        return LS_FAIL;
    m_buf.append(ptr, len);
    m_buf.append(" HTTP/1.1\r\n", 11);
    if (m_host.ptr)
    {
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
        appendHeader(HttpHeader::H_HOST, "host", 4, host, len);
    else
        m_host.ptr = (char *)host;
    m_host.len = len;
    return LS_OK;
}


int UnpackedHeaders::setHost2(const char *host, int len)
{
    if (m_host.len)
        return LS_FAIL;
    if (m_url.len && !m_url.ptr)
        appendHeader(HttpHeader::H_HOST, "host", 4, host, len);
    else
    {
        ls_str(&m_host, host, len);
        m_alloc_str = 1;
    }
    m_host.len = len;
    return LS_OK;
}


int UnpackedHeaders::appendCookieHeader(ls_str_t *cookies, int count,
                                        int total_len)
{
    req_header_entry * entry = m_entries.newObj();
    if (!entry)
        return LS_FAIL;
    if (m_buf.guarantee(total_len + 12) == LS_FAIL)
    {
        m_entries.pop();
        return LS_FAIL;
    }
    entry->name_index = HttpHeader::H_COOKIE;
    entry->name_len = 6;
    entry->val_len = total_len;
    entry->name_offset = m_buf.size();

    m_buf.append_unsafe("cookie: ", 8);

    ls_str_t *end = cookies + count;
    m_buf.append_unsafe(cookies->ptr, cookies->len);
    ++cookies;
    while(cookies < end)
    {
        m_buf.append_unsafe("; ", 2);
        m_buf.append_unsafe(cookies->ptr, cookies->len);
        ++cookies;
    }
    m_buf.append_unsafe("\r\n", 2);
    return LS_OK;
}


int UnpackedHeaders::set(ls_str_t *method, ls_str_t* url,
                         ls_str_t* host, ls_strpair_t* headers)
{
    m_methodLen = method->len;
    m_url.len = url->len;
    if (appendReqLine(method->ptr, method->len, url->ptr, url->len) == LS_FAIL)
        return LS_FAIL;
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
    m_buf.clear();
    if (m_buf.append(rhs.m_buf.begin(), rhs.m_buf.size()) == LS_FAIL)
        return LS_FAIL;
    m_entries.copy(rhs.m_entries);
    m_url.ptr = NULL;
    m_url.len = rhs.m_url.len;
    m_host.ptr = NULL;
    m_host.len = rhs.m_host.len;
    m_alloc_str = 0;
    m_methodLen = rhs.m_methodLen;
    return LS_OK;
}

