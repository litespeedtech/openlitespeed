#include "unpackedheaders.h"
#include <http/httpheader.h>

#include <lsr/ls_pool.h>


UnpackedHeaders::UnpackedHeaders()
{
    LS_ZERO_FILL(m_url, m_methodLen);
    m_buf.append("\0\0\0\0", 4);
    m_entries.setCapacity(16);
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


int UnpackedHeaders::convertHpackIdx(int hpack_index)
{
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
    };

    if (hpack_index > 0 && hpack_index <= (int)(sizeof(lookup)/sizeof(int)))
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
            int index = -1;
            if (headers->key.len == 15
                && strcmp(headers->key.ptr, "accept-encoding") == 0)
                index = HttpHeader::H_ACC_ENCODING;
            if (appendHeader(index, headers->key.ptr, headers->key.len,
                                 headers->val.ptr, headers->val.len) == LS_FAIL)
                return LS_FAIL;
            ++headers;
        }
    }
    endHeader();
    return LS_OK;
}
