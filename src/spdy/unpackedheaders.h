#ifndef HTTPREQHEADERS_H
#define HTTPREQHEADERS_H

#include <lsdef.h>
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

    int setMethod(const char *ptr, int len)
    {
        if (m_buf.size() != 4)
            return LS_FAIL;
        m_buf.append(ptr, len);
        m_methodLen = len;
        m_buf.append(" ", 1);
        if (m_url.ptr)
            appendUrl(m_url.ptr, m_url.len);
        return 0;
    }

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

    void appendUrl(const char *ptr, int len);

    int setHost(const char *host, int len);

    const AutoBuf  *getBuf() const          {   return &m_buf;      }
    AutoBuf  *getBuf()                      {   return &m_buf;      }

    void appendReqLine(const char *method, int method_len,
                       const char *url, int url_len);
    void appendHeader(int hpack_index, const char *name, int name_len,
                      const char *val, int val_len);
    void appendCookie(ls_str_t *cookies, int count, int total_len);

    void endHeader();

    bool isComplete() const
    {   return !m_host.ptr && m_host.len;   }

    int getMethodLen() const    {   return m_methodLen;     }
    int getUrlLen() const       {   return m_url.len;       }

    const req_header_entry *getEntryBegin() const
    {   return m_entries.begin();   }
    const req_header_entry *getEntryEnd() const
    {   return m_entries.end();     }

private:
    AutoBuf                     m_buf;
    TObjArray<req_header_entry> m_entries;
    ls_str_t                    m_url;
    ls_str_t                    m_host;
    int                         m_methodLen;

    LS_NO_COPY_ASSIGN(UnpackedHeaders);
};

#endif // HTTPREQHEADERS_H
