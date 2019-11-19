/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#ifndef SSLCONTEXT_H
#define SSLCONTEXT_H


/**
  *@author George Wang
  */
#include <sys/stat.h>
#include <stdint.h>

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

class SslContext;
class SslOcspStapling;
class SslContextConfig;

typedef SslContext *(*SslSniLookupCb)(void *arg, const char *pName);

#define SSL_CTX_PENDING ((SSL_CTX *)-1L)

struct bio_st;

class SslContext
{
public:
    enum
    {
        SSL_v3      = 1,
        SSL_TLSv1   = 2,
        SSL_TLSv11  = 4,
        SSL_TLSv12  = 8,
        SSL_TLSv13  = 16,
        SSL_TLS_SAFE = 28,
        SSL_TLS     = 30,
        SSL_ALL     = 31
    };
    explicit SslContext(int method = SSL_ALL);
    ~SslContext();

    static SslContext *config(SslContext *pContext,
                              SslContextConfig *pConfig);

    static SslContext *config(SslContext *pContext, const char *pZcDomainName,
        const char * pKey, const char * pCert, const char * pBundle);

    // public because called from SslContextHash::addIP:
    int setKeyCertificateFile(const char *pKeyFile, int iKeyType,
                               const char *pCertFile, int iCertType,
                               int chained);
    int setCALocation(const char *pCAFile, const char *pCAPath, int cv);

    void enableClientSessionReuse();

    void setRenegProtect(int p)   {   m_iRenegProtect = p;    }
    static void setUseStrongDH(int use);

    long getLastAccess() const  {   return m_tmLastAccess;  }
    void setLastAccess(long t)  {   m_tmLastAccess = t;     }

    SSL_CTX *get() const        {   return m_pCtx;          }
    void set(SSL_CTX * ctx)     {   m_pCtx = ctx;           }
    
    bool checkPrivateKey();

    SSL *newSSL();
    void setProtocol(int method);

    int initSNI(void *param);
    static int servername_cb(SSL *pSSL, void *arg);

    static int  initSSL();

    static int  publickey_encrypt(const unsigned char *pPubKey, int keylen,
                                  const char *content,
                                  int len, char *encrypted, int bufLen);
    static int  publickey_decrypt(const unsigned char *pPubKey, int keylen,
                                  const char *encrypted,
                                  int len, char *decrypted, int bufLen);

#ifdef _ENTERPRISE_
    void setClientVerify(int mode, int depth);
    int addCRL(const char *pCRLFile, const char *pCRLPath);
#endif

    unsigned char getEnableSpdy() const      {   return m_iEnableSpdy;   }
    /**
     * Check if OCSP is configured for the current context and if so,
     * update the OCSP response if needed.
     */
    int initOCSP();
    void updateOcsp();
    void disableOscp()              {   m_iEnableOcsp = 0;      }
    static void setSniLookupCb(SslSniLookupCb pCb);
    static SslContext *getSslContext(SSL_CTX *ctx);
    static void setAlpnCb(SSL_CTX *ctx, void *arg);

private:
    SSL_CTX    *m_pCtx;
    char        m_iMethod;
    char        m_iRenegProtect;
    char        m_iEnableSpdy;
    char        m_iEnableOcsp;
    int         m_iKeyLen;
    long        m_tmLastAccess;
    struct stat m_stKey;
    struct stat m_stCert;

    SslOcspStapling *m_pStapling;

    SslContext(const SslContext &rhs);
    void operator=(const SslContext &rhs);

    void release();
    int init(int method = SSL_TLS);
    void linkSslContext();

    static int seedRand(int len);
    void updateProtocol(int method);

    int setKeyCertificateFile(const char *pKeyCertFile, int iType,
                               int chained);
    int  setMultiKeyCertFile(const char *pKeyFile, int iKeyType,
                             const char *pCertFile, int iCertType, int chained);
    int setCertificateFile(const char *pFile, int type, int chained);
    int setCertificateChainFile(const char *pFile);
    // int setCertificateChainFile(bio_st *pBio);
    int setPrivateKeyFile(const char *pFile, int type);
    //Ron int checkPrivateKey();
    int  setSessionIdContext(unsigned char *sid, unsigned int len);
    long setOptions(long options);
    long getOptions();
    int setCipherList(const char *pList);
    int isKeyFileChanged(const char *pKeyFile) const;
    int isCertFileChanged(const char *pCertFile) const;

    int enableSpdy(int level);
    SslOcspStapling *getStapling()  {   return m_pStapling;     }
    void setStapling(SslOcspStapling *pSslOcspStapling) {  m_pStapling = pSslOcspStapling;}
    int  initStapling();
    int  initECDH();
    int  initDH(const char *pFile);
    int  enableShmSessionCache();
    int  enableSessionTickets();
    void disableSessionTickets();
    int  configStapling(SslContextConfig *pConfig);
};



#endif
