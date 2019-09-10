#ifndef QUICSTREAM_H
#define QUICSTREAM_H

#include <lsdef.h>
#include <http/hiostream.h>
#include <sslpp/hiocrypto.h>
#include <lsquic_types.h>
#include <util/linkedobj.h>
#include <spdy/unpackedheaders.h>

struct lsquic_stream_ctx {
    lsquic_stream_t *m_pStream;
};


struct QuicUpkdHdrs
{
    UnpackedHeaders headers;
    ls_str_t cookies[256];
    int cookie_count;
    int total_cookie_size;
    int total_size;
    bool regular_header;
    bool scheme;
public:
    QuicUpkdHdrs()
        : cookie_count(0)
        , total_cookie_size(0)
        , total_size(0)
        , regular_header(false)
        , scheme(false)
    {
        memset(&cookies, 0, sizeof(cookies));
    }

    ~QuicUpkdHdrs()
    {
        for(int i = 0; i< cookie_count; ++i)
        {
            ls_str_d(&cookies[i]);
        }
    }
};


class QuicStream : private lsquic_stream_ctx_t
                 , public HioStream
                 , public HioCrypto
                 , public DLinkedObj
{
public:
    QuicStream()
        {}
    ~QuicStream();
    int init(lsquic_stream_t *s);
    void setQuicStream(lsquic_stream_t *s)  {   m_pStream = s;  }
    int processUpkdHdrs(QuicUpkdHdrs *hdrs);
    lsquic_stream_ctx_t *toHandler()    {   return this;    }

    virtual int shutdown();
    virtual int onTimer();
    virtual void switchWriteToRead();
    virtual void continueWrite();
    virtual void suspendWrite();
    virtual void continueRead();
    virtual void suspendRead();
    virtual int sendRespHeaders(HttpRespHeaders *pHeaders, int isNoBody);

    virtual int sendfile(int fdSrc, off_t off, off_t size);
    virtual int readv(iovec *vector, size_t count);
    virtual int read(char *pBuf, int size);
    virtual int close();
    virtual int flush();
    virtual int writev(const iovec *vector, int count);
    virtual int write(const char *pBuf, int size);
    virtual const char *buildLogId();
    virtual int push(ls_str_t *pUrl, ls_str_t *pHost, 
                     ls_strpair_t *pExtraHeaders);

    virtual int getEnv(HioCrypto::ENV id, char *&val, int maxValLen);

    virtual int sendfile(int fdSrc, off_t off, size_t size) { return 0; }

    virtual uint16_t getEvents() const;
    virtual int isFromLocalAddr() const;
    virtual NtwkIOLink *getNtwkIoLink();
    
    void onRead();
    void onWrite();
    void onClose();
    
private:
    int checkReadRet(int ret);
};

#endif // QUICSTREAM_H
