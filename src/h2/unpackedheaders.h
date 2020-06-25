#ifndef HTTPREQHEADERS_H
#define HTTPREQHEADERS_H

#include <lsdef.h>
#include <lsr/ls_str.h>
#include <util/autobuf.h>
#include <util/objarray.h>
#include <util/autostr.h>
#include <http/httpdefs.h>
#include <http/httpheader.h>
#include <lsxpack_header.h>
#include <h2/lsxpack_error_code.h>

#define UPK_HDR_METHOD      (251)
#define UPK_HDR_PATH        (252)
#define UPK_HDR_SCHEME      (253)
#define UPK_HDR_STATUS      (254)
#define UPK_HDR_UNKNOWN     (255)

#define UPK_PSDO_RESERVE    3

class LogSession;

class UnpackedHeaders
{
    friend class UpkdHdrBuilder;
public:
    UnpackedHeaders();
    ~UnpackedHeaders();

    static int hpack2ReqIdx(int hpack_index);
    static int qpack2ReqIdx(int qpack_index);
    static int hpack2qpack(int idx);
    static int qpack2hpack(int idx);

    int setMethod(const char *ptr, int len);
    int setMethod2(lsxpack_header *hdr);

    int setUrl(const char *ptr, int len)
    {
        if (m_url.len)
            return LS_FAIL;
        if (m_buf->size() != HEADER_BUF_PAD)
            appendUrl(ptr, len);
        else
            m_url.ptr = (char *)ptr;
        m_url.len = len;
        return 0;
    }

    int setUrl2(lsxpack_header *hdr);
//     {
//         if (m_url.len)
//             return LS_FAIL;
//         if (m_buf->size() != HEADER_BUF_PAD)
//             appendUrl(ptr, len);
//         else
//         {
//             ls_str(&m_url, ptr, len);
//             m_alloc_str = 1;
//         }
//         m_url.len = len;
//         return 0;
//     }

    int appendUrl(const char *ptr, int len);

    int setHost(const char *host, int len);
    int setHost2(lsxpack_header *hdr);

    const AutoBuf  *getBuf() const          {   return m_buf;      }
    AutoBuf  *getBuf()                      {   return m_buf;      }

    int  appendReqLine(const char *method, int method_len,
                       const char *url, int url_len);
    int  setReqLine(const char *method, int method_len,
                       const char *uri, int uri_len,
                       const char *qs, int qs_len);

    int appendExistHeader(int index, int name_offset, int name_len,
                          int val_offset, int val_len);

    int appendHeader(int app_index, const char *name, int name_len,
                     const char *val, int val_len);
    int appendHeaders(const char *pStr, int len);
    void endHeader();

    bool isComplete() const
    {   return !m_host.ptr && m_host.len;   }

    const char *getMethod() const
    {   return m_buf->begin() + HEADER_BUF_PAD;   }
    int getMethodLen() const    {   return m_methodLen;     }

    const char *getUrl() const
    {   return m_buf->begin() + (int)m_url_offset;    }
    int getUrlLen() const       {   return m_url.len;       }

    const char *getHost() const
    {   return m_buf->begin() + (int)m_host_offset;         }
    int getHostLen() const      {   return m_host.len;      }
    void clearHostLen()         {   m_host.len = 0;         }

    int updateHost(const char *host, int len);
    void setSharedBuf(AutoBuf *buf);

    const char *getHeader(int name_off) const
    {   return m_buf->begin() + name_off;    }

    int set(ls_str_t *method, ls_str_t* url,
            ls_str_t* host, ls_strpair_t* headers);

    const lsxpack_header *begin() const
    {   return m_lsxpack.begin();   }
    const lsxpack_header *req_hdr_begin() const
    {   return m_lsxpack.get(UPK_PSDO_RESERVE); }
    const lsxpack_header *end() const
    {   return m_lsxpack.end();     }

    static void dump(LogSession *ls, const lsxpack_header *hdr,
                     const char *msg);

    void reset();
    int copy(const UnpackedHeaders &rhs);
    lsxpack_header *getFirstCookieHdr();
    lsxpack_header *getEntry(uint16_t i)
    {   assert(i < m_lsxpack.size());
        return m_lsxpack.get(i);    }

    int updateHeader(int app_index, const char *val, int val_len);
    int dropHeader(int app_index, int val_offset);

    void setSecheme(bool http)  {   m_scheme_http = http;   }
    int is_http() const         {   return m_scheme_http;   }
    void prepareSendXpack(bool is_qpack);
    void prepareSendHpack();
    void prepareSendQpack();

    void setHeaderPos(uint32_t hdr_idx, uint16_t pos)
    {
        assert(hdr_idx < HttpHeader::H_TE);
        m_commonHeaderPos[hdr_idx] = pos;
    }

    uint16_t getHeaderPos(uint32_t hdr_idx)
    {
        assert(hdr_idx < HttpHeader::H_TE);
        return m_commonHeaderPos[hdr_idx];
    }

    void markValueUpdated(uint32_t hdr_idx)
    {
        assert(hdr_idx < HttpHeader::H_TE);
        if (m_commonHeaderPos[hdr_idx] != 0)
        {
            assert(m_commonHeaderPos[hdr_idx] < m_lsxpack.size());
            lsxpack_header_mark_val_changed(getEntry(m_commonHeaderPos[hdr_idx]));
        }
    }

private:
    int finalizeCookieHeader(int count, uint16_t first_idx,
                             const char *value, int total_len);
    void buildHpackPsuedo();
    void buildQpackPsuedo();

private:
    AutoBuf                    *m_buf;
    TObjArray<lsxpack_header>   m_lsxpack;
    ls_str_t                    m_url;
    ls_str_t                    m_host;
    uint16_t                    m_commonHeaderPos[HttpHeader::H_TE];
    uint16_t                    m_host_offset;
    uint16_t                    m_url_offset;
    uint16_t                    m_last_shre_buf_size;
    uint8_t                     m_alloc_str:1;
    uint8_t                     m_shared_buf:1;
    uint8_t                     m_scheme_http:1;
    uint8_t                     m_methodLen;
    uint16_t                    m_first_cookie_idx;

    LS_NO_COPY_ASSIGN(UnpackedHeaders);
};


#define DUMP_LSXPACK(a, b, c)  \
    do { \
        if ( log4cxx::Level::isEnabled( log4cxx::Level::DBG_LESS ) ) \
            UnpackedHeaders::dump(a, b, c); \
    }while(0)


class UpkdHdrBuilder
{
public:
    explicit UpkdHdrBuilder(UnpackedHeaders *hdr, bool qpack)
        : headers(hdr)
    {
        tmp_buf = fixed_buf;
        tmp_max = sizeof(fixed_buf);
        tmp_used = 0;
        LS_ZERO_FILL(working, scheme);
        this->is_qpack = qpack;
        first_cookie_idx = UINT16_MAX;
    }

    ~UpkdHdrBuilder()
    {
        if (headers)
            delete headers;
        if (tmp_buf != fixed_buf)
            free(tmp_buf);
    }

    void setHeaders(UnpackedHeaders *hdrs)
    {
        assert(headers == NULL);
        headers = hdrs;
    }
    UnpackedHeaders *getHeaders() const     {   return headers;     }
    UnpackedHeaders *retrieveHeaders()
    {
        UnpackedHeaders * h = headers;
        headers = NULL;
        return h;
    }

    lsxpack_header_t *prepareDecode(lsxpack_header_t *hdr,
                                    size_t mini_buf_size);
    lsxpack_header_t *getWorking()  {   return working; }

    lsxpack_err_code process(lsxpack_header_t *hdr);
    lsxpack_err_code processHpack(lsxpack_header_t *hdr);
    lsxpack_err_code end();

private:
    int guarantee(int size);


private:
    UnpackedHeaders    *headers;
    char               *tmp_buf;
    int                 tmp_used;
    int                 tmp_max;
    lsxpack_header_t   *working;
    int                 cookie_count;
    int                 total_size;
    uint16_t            first_cookie_idx;
    uint16_t            last_cookie_idx;
    bool                regular_header;
    bool                scheme;
    bool                is_qpack;
    char                fixed_buf[4000];
    LS_NO_COPY_ASSIGN(UpkdHdrBuilder);
};


#endif // HTTPREQHEADERS_H
