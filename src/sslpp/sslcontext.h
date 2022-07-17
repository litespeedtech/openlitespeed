/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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

#ifndef SSLCONTEXT_H
#define SSLCONTEXT_H


/**
  *@author George Wang
  */
#include <sys/stat.h>
#include <stdint.h>
#include <sslpp/ssldef.h>

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

class SslContext;
class SslOcspStapling;
class SslContextConfig;

typedef SslContext *(*SslSniLookupCb)(void *arg, const char *pName);
typedef SslContext *(*SslAsyncSniLookupCb)(void *arg, const char *name,
                                           int name_len,
                                           AsyncCertDoneCb cb, void *cb_param);


#define SNI_LOOKUP_PENDING  ((SslContext *)-1L)
#define SNI_LOOKUP_FAIL     ((SslContext *)-2L)

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

    static SslContext *configMultiCerts(SslContext *pContext,
                                        SslContextConfig *pConfig);
    static SslContext *configOneCert(SslContext *pContext, const char * key_file,
                                     const char * cert_file, const char * bundle_file,
                                     SslContextConfig *pConfig);

    int loadCA(const char *pBundle);
    int configOptions(SslContextConfig *pConfig);

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
    int initSniMultiCert(void *param);
    void enableSelectCertCb();
    int applyToSsl(SSL *pSsl);

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
    void setOcspStapling(int v)     {   m_iEnableOcsp = v;      }
    char getOcspStapling() const    {   return m_iEnableOcsp;   }
    int  configStapling(const char *name, int max_age, const char *responder);

    int selectCert(SSL *pSSL, const char *name, const void *cli_hello);
    SslContext *getEccCtx() const   {   return m_pEccCtx;       }

    static SslSniLookupCb setSniLookupCb(SslSniLookupCb pCb);
    static SslAsyncSniLookupCb setAsyncSniLookupCb(SslAsyncSniLookupCb pCb);
    static SslAsyncSniLookupCb getAsyncSniLookupCb();
    static SslContext *getSslContext(SSL_CTX *ctx);
    static void setAlpnCb(SSL_CTX *ctx, void *arg);

private:
    SSL_CTX    *m_pCtx;
    SslContext *m_pEccCtx;
    char        m_iMethod;
    char        m_iRenegProtect;
    char        m_iEnableSpdy;
    char        m_iEnableOcsp;
    short       m_iKeyLen;
    short       m_iKeyType;
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
    int setMultiKeyCertFile(const char *pKeyFile, int iKeyType,
                            const char *pCertFile, int iCertType,
                            int chained, int ecc_only);
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
};



#endif
