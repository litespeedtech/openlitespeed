/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#ifndef SSLCONNECTION_H
#define SSLCONNECTION_H
#include <lsdef.h>
#include <sslpp/ssldef.h>
#include <sslpp/hiocrypto.h>
#include <sslpp/ls_fdbuf_bio.h>

class SslClientSessCache;

class SslConnection : public HioCrypto
{
public:
    enum
    {
        DISCONNECTED,
        CONNECTING,
        ACCEPTING,
        CONNECTED,
        SHUTDOWN
    };
    enum
    {
        WANT_READ = 1,
        WANT_WRITE = 2,
        LAST_READ = 4,
        LAST_WRITE = 8,
        WANT_CERT = 16
    };

    enum
    {
        F_HANDSHAKE_DONE    = 1,
        F_DISABLE_HTTP2     = 2,
        F_ASYNC_CERT        = 4,
        F_ASYNC_PK          = 8,
        F_ASYNC_CERT_FAIL   = 16,
    };

    char wantRead() const   {   return m_iWant & WANT_READ;     }
    char wantWrite() const  {   return m_iWant & WANT_WRITE;    }
    char lastRead() const   {   return m_iWant & LAST_READ;     }
    char lastWrite() const  {   return m_iWant & LAST_WRITE;    }
    char wantCert() const   {   return m_iWant & WANT_CERT;     }
    void clearWantCert()    {   m_iWant &= ~WANT_CERT;          }
    void setAllowWrite()    {   ls_fdbio_clear_wblock(&m_bio);  }

    int  getFlag(int v) const   {   return m_flag & v;     }
    void setFlag(int f, int v)  {   m_flag = (m_flag & ~f) | (v ? f : 0);  }
    bool isWaitAsync() const    {   return m_flag & (F_ASYNC_CERT | F_ASYNC_PK);   }

    SslConnection();
    ~SslConnection();

    void setSSL(SSL *ssl);
    SSL *getSSL() const    {   return m_ssl;   }

    void release();
    int setfd(int fd);
    //int setfd(int rfd, int wfd);

    void toAccept();

    int accept();
    int connect();
    int read(char *pBuf, int len);
    int wpending();
    int write(const char *pBuf, int len);
    int writev(const struct iovec *vect, int count, int *finished);
    int flush();
    int shutdown(int bidirectional);
    int checkError(int ret);
    bool isConnected()      {   return m_iStatus == CONNECTED;  }
    int tryagain();
    void setWriteBuffering(int buffering);

    int asyncFetchCert(asyncCertDoneCb cb, void *pParam);
    void cancelAsyncFetchCert(asyncCertDoneCb cb, void *pParam);

    char getStatus() const   {   return m_iStatus;   }

    X509 *getPeerCertificate() const;
    long getVerifyResult() const;
    int  getVerifyMode() const;
    int  isVerifyOk() const;
    int  buildVerifyErrorString(char *pBuf, int len) const;

    virtual int getEnv(HioCrypto::ENV id, char *&val,int maxValLen);

    const char *getCipherName() const;

    const SSL_CIPHER *getCurrentCipher() const;

    SSL_SESSION *getSession() const;
    int setSession(SSL_SESSION *session) const;
    int isSessionReused() const;
    void setClientSessCache(SslClientSessCache *cache)
    {   m_pSessCache = cache;     }
    int cacheClientSession(SSL_SESSION* session, const char *pHost, int iHostLen);
    void tryReuseCachedSession(const char *pHost, int iHostLen);

    const char *getVersion() const;

    int setTlsExtHostName(const char *pName);

    const char *getTlsExtHostName();

    int getSpdyVersion();

    int updateOnGotCert();
    
    void enableRbio() {};

    static void initConnIdx();
    static SslConnection *get(const SSL *ssl);

    static int getCipherBits(const SSL_CIPHER *pCipher, int *algkeysize);
    static int isClientVerifyOptional(int i);
    
    // Can only be called after the first failed accept or read, to obtain the
    // raw data which can be used in a redirect (see ntwkiolink.cpp).
    char *getRawBuffer(int *len);
    bool hasPendingIn() const
    {   return m_bio.m_rbuf_used > m_bio.m_rbuf_read;   }

    void releaseIdleBuffer();

    bool isWaitingAsyncCert() const
    {   return (getFlag(F_ASYNC_CERT | F_ASYNC_CERT_FAIL) == F_ASYNC_CERT); }
    bool wantAsyncCtx(SSL_CTX *&pInput);

private:
    SSL    *m_ssl;
    SslClientSessCache *m_pSessCache;
    short   m_flag;
    char    m_iStatus;
    char    m_iWant;
    static int32_t s_iConnIdx;
    ls_fdbio_data m_bio;

    LS_NO_COPY_ASSIGN(SslConnection);
};

#endif
