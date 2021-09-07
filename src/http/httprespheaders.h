/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2021  LiteSpeed Technologies, Inc.                 *
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
#ifndef HTTPRESPHEADERS_H
#define HTTPRESPHEADERS_H

#include <util/autobuf.h>
#include <http/httpheader.h>
#include <util/iovec.h>
#include <util/objarray.h>
#include <http/httpstatusline.h>
#include <http/httpreq.h>
#include <ls.h>
#include <lsxpack_header.h>
#include <h2/lsxpack_error_code.h>

enum ETAG_ENCODING
{
    ETAG_NO_ENCODE,
    ETAG_BROTLI,
    ETAG_GZIP,
};


struct http_header_t;

#define HRH_F_HAS_HOLE  1
#define HRH_F_HAS_PUSH  2
#define HRH_F_LC_NAME   4

#define HRH_IDX_NONE    0xFFFF

class HttpRespHeaders
{
    friend class UpkdRespHdrBuilder;
public:
    enum INDEX
    {
        // most common response-header
        H_ACCEPT_RANGES = LSI_RSPHDR_ACCEPT_RANGES,
        H_CONNECTION = LSI_RSPHDR_CONNECTION,
        H_CONTENT_TYPE = LSI_RSPHDR_CONTENT_TYPE,
        H_CONTENT_LENGTH = LSI_RSPHDR_CONTENT_LENGTH,
        H_CONTENT_ENCODING = LSI_RSPHDR_CONTENT_ENCODING,
        H_CONTENT_RANGE = LSI_RSPHDR_CONTENT_RANGE,
        H_CONTENT_DISPOSITION = LSI_RSPHDR_CONTENT_DISPOSITION,
        H_CACHE_CTRL = LSI_RSPHDR_CACHE_CTRL,
        H_DATE = LSI_RSPHDR_DATE,
        H_ETAG = LSI_RSPHDR_ETAG,
        H_EXPIRES = LSI_RSPHDR_EXPIRES,
        H_KEEP_ALIVE = LSI_RSPHDR_KEEP_ALIVE,
        H_LAST_MODIFIED = LSI_RSPHDR_LAST_MODIFIED,
        H_LOCATION = LSI_RSPHDR_LOCATION,
        H_LITESPEED_LOCATION = LSI_RSPHDR_LITESPEED_LOCATION,
        H_LITESPEED_CACHE_CONTROL = LSI_RSPHDR_LITESPEED_CACHE_CONTROL,
        H_PRAGMA = LSI_RSPHDR_PRAGMA,
        H_PROXY_CONNECTION = LSI_RSPHDR_PROXY_CONNECTION,
        H_SERVER = LSI_RSPHDR_SERVER,
        H_SET_COOKIE = LSI_RSPHDR_SET_COOKIE,
        H_CGI_STATUS = LSI_RSPHDR_CGI_STATUS,
        H_TRANSFER_ENCODING = LSI_RSPHDR_TRANSFER_ENCODING,
        H_VARY = LSI_RSPHDR_VARY,
        H_WWW_AUTHENTICATE = LSI_RSPHDR_WWW_AUTHENTICATE,
        H_X_LITESPEED_CACHE = LSI_RSPHDR_LITESPEED_CACHE,
        H_X_LITESPEED_PURGE = LSI_RSPHDR_LITESPEED_PURGE,
        H_X_LITESPEED_TAG = LSI_RSPHDR_LITESPEED_TAG,
        H_X_LITESPEED_VARY = LSI_RSPHDR_LITESPEED_VARY,
        H_LSC_COOKIE = LSI_RSPHDR_LSC_COOKIE,
        H_X_POWERED_BY = LSI_RSPHDR_X_POWERED_BY,
        H_LINK         = LSI_RSPHDR_LINK,
        H_HTTP_VERSION = LSI_RSPHDR_VERSION,
        H_ALT_SVC      = LSI_RSPHDR_ALT_SVC,
        H_X_LITESPEED_ALT_SVC = LSI_RSPHDR_LITESPEED_ALT_SVC,
        H_LSADC_BACKEND = LSI_RSPHDR_LSADC_BACKEND,
        H_UPGRADE = LSI_RSPHDR_UPGRADE,
        H_HEADER_END = LSI_RSPHDR_END,
        H_UNKNOWN = LSI_RSPHDR_UNKNOWN


        //not commonly used headers.
//         H_AGE,
//         H_PROXY_AUTHENTICATE,
//         H_RETRY_AFTER,
//         H_SET_COOKIE2,
//
//
//         // general-header
//         H_TRAILER,
//         H_WARNING,
//
//         // entity-header
//         H_ALLOW,
//         H_CONTENT_LANGUAGE,
//         H_CONTENT_LOCATION,
//         H_CONTENT_MD5,
    };

public:
    HttpRespHeaders();
    ~HttpRespHeaders();

    void reset();
    void reset2();

    const lsxpack_header *begin() const {   return m_lsxpack.begin();   }
    const lsxpack_header *end() const   {   return m_lsxpack.end();     }

    lsxpack_header *begin()             {   return m_lsxpack.begin();   }
    lsxpack_header *end()               {   return m_lsxpack.end();     }

    void prepareSendXpack(bool is_qpack);
    void prepareSendQpack();
    void prepareSendHpack();

    int add(INDEX headerIndex, const char *pVal, unsigned int valLen,
            int method = LSI_HEADER_SET);
    int add(INDEX headerIndex, const char *pName, unsigned int nameLen,
            const char *pVal, unsigned int valLen, int method = LSI_HEADER_SET);
    int appendLastVal(const char *pVal, int valLen);
    int add(http_header_t *headerArray, int size, int method = LSI_HEADER_SET);
    int parseAdd(const char *pStr, int len, int method = LSI_HEADER_SET);

    int add(const char *pName, int nameLen, const char *pVal,
            unsigned int valLen, int method = LSI_HEADER_SET);
    int addKvPairIdx(lsxpack_header *new_hdr);
    void commitWorkingHeader();
    int tryCommitHeader(INDEX idx, const char *pVal, unsigned int valLen);
    //Special case
    void addStatusLine(int ver, int code, short iKeepAlive) {  m_iHttpVersion = ver; m_iHttpCode = code; m_iKeepAlive = iKeepAlive; }
    short getHttpVersion()  { return m_iHttpVersion; }
    short getHttpCode()     { return m_iHttpCode;   }

    int del(const char *pName, int nameLen);
    int del(INDEX headerIndex);

    int getUniqueCnt() const    {   return m_iHeaderUniqueCount;    }

    int  getCount() const
    {
        return m_lsxpack.size() - m_iHeaderRemovedCount;
    }

    int  getTotalCount() const  {   return m_lsxpack.size();   }

    char *getContentTypeHeader(int &len);

    //return number of header appended to iov
    int  getHeader(const char *pName, int nameLen, struct iovec *iov,
                   int maxIovCount);
    int  getHeader(INDEX index, struct iovec *iov, int maxIovCount);

    int  getHeader(const char *pName, int nameLen, const char **val, int &valLen);
    const char *getHeader(INDEX index, int *valLen) const;
    char *getHeaderToUpdate(INDEX index, int *valLen);

    int  isHeaderSet(INDEX index) const
    {   return m_KVPairindex[index] != HRH_IDX_NONE;    }

    //For LSIAPI using//return number of header appended to iov
    int getAllHeaders(struct iovec *iov_key,
                      struct iovec *iov_val, int maxIovCount);

    int  HeaderBeginPos()  {   return nextHeaderPos(-1); }
    int  HeaderEndPos()    {    return -1; }
    int  nextHeaderPos(int pos);

    int  getHeader(int pos, int *idx, struct iovec *name, struct iovec *iov,
                   int maxIovCount);

    void dump(LogSession *pILog, int dump_header);

public:

    //0: REGULAR, 1:SPDY2 2, SPDY3, 3, SPDY 4, .....
    int outputNonSpdyHeaders(IOVec *iovec);
    char isRespHeadersBuilt() const {   return m_iHeaderBuilt;      }
    int getTotalLen() const         {   return m_iHeadersTotalLen;  }
    int appendToIov(IOVec *iovec, int &addCrlf);
    int appendToIovExclude(IOVec *iovec, const char *pName, int nameLen) const;

    static INDEX getIndex(const char *pHeader);
    static INDEX getIndex(const char *pHeader, int len);
    static const char **getNameList();
    static int getNameLen(INDEX index)  {    return s_iHeaderLen[(int)index];  }

    static void buildCommonHeaders();
    static void updateDateHeader();
    static void hideServerSignature(int hide);

    void addGzipEncodingHeader();
    void addBrEncodingHeader();
    void updateEtag(ETAG_ENCODING type);
    void appendChunked();
    void addCommonHeaders();
    void addTruboCharged();

    static void hideServerHeader()  {   s_showServerHeader = 0;     }
    void clearSetCookieIndex()
    {   m_KVPairindex[H_SET_COOKIE] = HRH_IDX_NONE;    }
    void dropConnectionHeaders();
    void convertLscCookie();
    void dropCacheHeaders()
    {
        del(HttpRespHeaders::H_LITESPEED_CACHE_CONTROL);
        del(HttpRespHeaders::H_X_LITESPEED_TAG);
        del(HttpRespHeaders::H_X_LITESPEED_VARY);
    }
    void copy(const HttpRespHeaders &headers);
    unsigned char hasPush() const   {   return m_flags & HRH_F_HAS_PUSH;    }
    void addAcceptRanges()
    {   add(HttpRespHeaders::H_ACCEPT_RANGES, "bytes", 5);  }

    static int toHpackIdx(int index);
    static int qpack2RespIdx(int qpack_index);
    static int hpack2RespIdx(int hpack_index);

private:
    AutoBuf             m_buf;
    TObjArray<lsxpack_header> m_lsxpack;
    lsxpack_header_t   *m_working;

    uint16_t            m_KVPairindex[H_HEADER_END];
    unsigned char       m_flags;
    unsigned char       m_iHeaderBuilt;
    short               m_iHeaderRemovedCount;
    short               m_iHeaderUniqueCount;
    short               m_hLastHeaderKVPairIndex;

    int8_t              m_iHttpVersion;
    int8_t              m_iKeepAlive;
    short               m_iHttpCode;

    int                 m_iHeadersTotalLen;

    lsxpack_header *newHdrEntry();
    int             getFreeSpaceCount() const
    {    return m_lsxpack.capacity() - m_lsxpack.size();   };
    void            incKvPairs(int num);
    lsxpack_header *getKvPair(int index)
    {   return m_lsxpack.get(index);    }

    const lsxpack_header *getKvPair(int index) const
    {   return m_lsxpack.get(index);    }

    int             getKvIdx(const lsxpack_header *kv) const
    {   return kv - m_lsxpack.begin(); }

    char           *getHeaderStr(int offset)
    { return m_buf.begin() + offset;  }
    const char     *getHeaderStr(int offset) const
    { return m_buf.begin() + offset;  }
    char           *getName(const lsxpack_header *pKv)
    { return getHeaderStr(pKv->name_offset); }
    char           *getVal(const lsxpack_header *pKv)
    { return getHeaderStr(pKv->val_offset); }
    const char     *getVal(const lsxpack_header *pKv) const
    { return getHeaderStr(pKv->val_offset); }


    void            _del(int kvOrderNum);
    void            replaceHeader(lsxpack_header *pKv, const char *pVal,
                                  unsigned int valLen);
    int             appendHeader(lsxpack_header *pKv, int hdr_idx, const char *pName,
                                 unsigned int nameLen, const char *pVal, unsigned int valLen, int);
    int             getHeaderKvOrder(const char *pName, unsigned int nameLen);
    void            verifyHeaderLength(INDEX headerIndex, const char *pName,
                                       unsigned int nameLen);
    int             mergeAll();
    void            copyEx(const HttpRespHeaders &headers);
    lsxpack_header *getExistKv(INDEX index, const char *name, int name_len);
    void            appendKvPairIdx(lsxpack_header *exist, int kv_idx);

    HttpRespHeaders(const HttpRespHeaders &other) {};

    static int      s_iHeaderLen[H_HEADER_END + 1];
    static int      s_showServerHeader;

};

struct http_header_t
{
    HttpRespHeaders::INDEX index;
    unsigned short nameLen;
    unsigned short valLen;
    const char *name;
    const char *val;
};


class HttpExtConnector;
class UpkdRespHdrBuilder
{
public:
    explicit UpkdRespHdrBuilder(HttpRespHeaders *hdr, bool qpack)
        : headers(hdr)
    {
        LS_ZERO_FILL(connector, regular_header);
        is_qpack = qpack;
    }

    ~UpkdRespHdrBuilder()
    {
    }

    void setHeaders(HttpRespHeaders *hdrs)
    {
        assert(headers == NULL);
        headers = hdrs;
    }

    HttpRespHeaders *getHeaders() const     {   return headers;     }
    HttpRespHeaders *retrieveHeaders()
    {
        HttpRespHeaders * h = headers;
        headers = NULL;
        return h;
    }

    void setConnector(HttpExtConnector *c)  {   connector = c;  }

    lsxpack_header_t *prepareDecode(lsxpack_header_t *hdr,
                                    size_t mini_buf_size);
    lsxpack_header_t *getWorking()  {   return headers->m_working; }

    lsxpack_err_code process(lsxpack_header_t *hdr);
    lsxpack_err_code end();

    void *get_aux_data() const      {   return aux_data;    }
    void set_aux_data(void *aux)    {   aux_data = aux;     }

private:
    HttpRespHeaders    *headers;
    HttpExtConnector   *connector;
    void               *aux_data;
    int                 total_size;
    bool                regular_header;
    bool                is_qpack;

    LS_NO_COPY_ASSIGN(UpkdRespHdrBuilder);
};

#endif // HTTPRESPHEADERS_H
