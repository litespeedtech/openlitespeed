#ifndef HTTPREQHEADERS_H
#define HTTPREQHEADERS_H

#include <lsdef.h>
#include <lsr/ls_str.h>
#include <util/autobuf.h>
#include <util/objarray.h>
#include <util/autostr.h>

#define UPK_HDR_METHOD      (-1)
#define UPK_HDR_PATH        (-2)
#define UPK_HDR_SCHEME      (-3)
#define UPK_HDR_STATUS      (-4)
#define UPK_HDR_UNKNOWN     (-100)

typedef struct
{
    int16_t     name_index;
    uint16_t    name_offset;
    uint16_t    name_len;
    uint16_t    val_len;
} req_header_entry;


class UnpackedHeaders
{
public:
    UnpackedHeaders();
    ~UnpackedHeaders();

    static int convertHpackIdx(int hpack_index);

    int setMethod(const char *ptr, int len);

    int setUrl(const char *ptr, int len)
    {
        if (m_url.len)
            return LS_FAIL;
        if (m_buf.size() != 4)
            appendUrl(ptr, len);
        else
            m_url.ptr = (char *)ptr;
        m_url.len = len;
        return 0;
    }

    int setUrl2(const char *ptr, int len)
    {
        if (m_url.len)
            return LS_FAIL;
        if (m_buf.size() != 4)
            appendUrl(ptr, len);
        else
        {
            ls_str(&m_url, ptr, len);
            m_alloc_str = 1;
        }
        m_url.len = len;
        return 0;
    }

    int appendUrl(const char *ptr, int len);

    int setHost(const char *host, int len);
    int setHost2(const char *host, int len);

    const AutoBuf  *getBuf() const          {   return &m_buf;      }
    AutoBuf  *getBuf()                      {   return &m_buf;      }

    int  appendReqLine(const char *method, int method_len,
                       const char *url, int url_len);
    int appendHeader(int hpack_index, const char *name, int name_len,
                     const char *val, int val_len);
    int appendCookieHeader(ls_str_t *cookies, int count, int total_len);

    void endHeader();

    bool isComplete() const
    {   return !m_host.ptr && m_host.len;   }

    int getMethodLen() const    {   return m_methodLen;     }
    int getUrlLen() const       {   return m_url.len;       }

    int set(ls_str_t *method, ls_str_t* url,
            ls_str_t* host, ls_strpair_t* headers);

    const req_header_entry *getEntryBegin() const
    {   return m_entries.begin();   }
    const req_header_entry *getEntryEnd() const
    {   return m_entries.end();     }

private:
    AutoBuf                     m_buf;
    TObjArray<req_header_entry> m_entries;
    ls_str_t                    m_url;
    ls_str_t                    m_host;
    int16_t                     m_alloc_str;
    int16_t                     m_methodLen;

    LS_NO_COPY_ASSIGN(UnpackedHeaders);
};

#endif // HTTPREQHEADERS_H
