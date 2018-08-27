#include "unpackedheaders.h"
#include <http/httpheader.h>

UnpackedHeaders::UnpackedHeaders()
{
    LS_ZERO_FILL(m_url, m_host);
    m_buf.append("\0\0\0\0", 4);
    m_entries.setCapacity(16);
}


UnpackedHeaders::~UnpackedHeaders()
{

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


void UnpackedHeaders::appendHeader(int index, const char *name, int name_len,
                                const char *val, int val_len)
{
    req_header_entry * entry = m_entries.getNew();
    entry->name_index = index;
    entry->name_len = name_len;
    entry->val_len = val_len;
    entry->name_offset = m_buf.size();
    m_buf.append(name, name_len);
    m_buf.append(": ", 2);
    m_buf.append(val, val_len);
    m_buf.append("\r\n", 2);
}


void UnpackedHeaders::appendReqLine(const char *method, int method_len,
                                 const char *url, int url_len)
{
    m_buf.append(method, method_len);
    m_buf.append(" ", 1);
    m_buf.append(url, url_len);
    m_buf.append(" HTTP/1.1\r\n", 11);
}


void UnpackedHeaders::endHeader()
{
    m_buf.append("\r\n", 2);
}


void UnpackedHeaders::appendUrl(const char *ptr, int len)
{
    m_buf.append(ptr, len);
    m_buf.append(" HTTP/1.1\r\n", 11);
    if (m_host.ptr)
    {
        appendHeader(HttpHeader::H_HOST, "host", 4, m_host.ptr, m_host.len);
        m_host.ptr = NULL;
    }
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
    return 0;
}


void UnpackedHeaders::appendCookie(ls_str_t *cookies, int count, int total_len)
{
    req_header_entry * entry = m_entries.getNew();
    entry->name_index = HttpHeader::H_COOKIE;
    entry->name_len = 6;
    entry->val_len = total_len;
    entry->name_offset = m_buf.size();
    m_buf.guarantee(total_len + 10);

    m_buf.appendUnsafe("cookie: ", 8);

    ls_str_t *end = cookies + count;
    m_buf.appendUnsafe(cookies->ptr, cookies->len);
    ++cookies;
    while(cookies < end)
    {
        m_buf.appendUnsafe("; ", 2);
        m_buf.appendUnsafe(cookies->ptr, cookies->len);
        ++cookies;
    }
    m_buf.appendUnsafe("\r\n", 2);
}


