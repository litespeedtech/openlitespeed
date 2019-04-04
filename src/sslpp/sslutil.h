// Author: Kevin Fwu
// Date: August 25, 2015

#ifndef SSLUTIL_H
#define SSLUTIL_H

#include <sslpp/ssldef.h>

class SslUtil
{
    static const char *s_pDefaultCAFile;
    static const char *s_pDefaultCAPath;

    SslUtil();
    ~SslUtil();
    SslUtil(const SslUtil &rhs);
    void operator=(const SslUtil &rhs);
public:
    enum
    {
        FILETYPE_PEM,
        FILETYPE_ASN1
    };

    /**
    * The cert callback is expected to return the following:
    * < 0 If the server needs to look for a certificate
    *  0  On error.
    *  1  If success, regardless of if a certificate was set or not.
    */
    enum
    {
        CERTCB_RET_WAIT = -1,
        CERTCB_RET_ERR  =  0,
        CERTCB_RET_OK   =  1
    };
    static void setUseStrongDH(int use);
    static int initDH(SSL_CTX *pCtx, const char *pFile, int iKeyLen);
    static int copyDH(SSL_CTX *pCtx, SSL_CTX *pSrcCtx);
    static int initECDH(SSL_CTX *pCtx);
    static int translateType(int type);
    static int loadPemWithMissingDash(const char *pFile, char *buf, int bufLen,
                                      char **pBegin);
    static int digestIdContext(SSL_CTX *pCtx, const void *pDigest,
                               size_t iDigestLen);
    static int loadCert(SSL_CTX *pCtx, const void *pCert, int iCertLen,
                        int loadChain = 0);
    static int loadPrivateKey(SSL_CTX *pCtx, const void *pKey, int iKeyLen);
    static int loadCertFile(SSL_CTX *pCtx, const char *pFile, int type);
    static int loadPrivateKeyFile(SSL_CTX *pCtx, const char *pFile, int type);
    static bool loadCA(SSL_CTX *pCtx, const char *pCAFile, const char *pCAPath,
                       int cv);
    static bool loadCA(SSL_CTX *pCtx, const char *pCAbuf);

    static int initDefaultCA(const char *pCAFile, const char *pCAPath);
    static const char *getDefaultCAFile()
    {
        if (!s_pDefaultCAFile)
            initDefaultCA(NULL, NULL);
        return s_pDefaultCAFile;
    }
    static const char *getDefaultCAPath()
    {
        if (!s_pDefaultCAPath)
            initDefaultCA(NULL, NULL);
        return s_pDefaultCAPath;
    }
    static int setCertificateChain(SSL_CTX *pCtx, BIO * bio);

    static int getSkid(SSL_CTX *pCtx, char *skid_buf, int buf_len);
    static int lookupCertSerial(X509 *pCert, char *pBuf, int len);

    static void initCtx(SSL_CTX *pCtx, int method, char renegProtect);
    static long setOptions(SSL_CTX *pCtx, long options);
    static long getOptions(SSL_CTX *pCtx);
    static int setCipherList(SSL_CTX *pCtx, const char *pList);
    static void updateProtocol(SSL_CTX *pCtx, int method);
    static int enableShmSessionCache(SSL_CTX *pCtx);
    static int enableSessionTickets(SSL_CTX *pCtx);
    static void disableSessionTickets(SSL_CTX *pCtx);

    static asyncCertFunc addAsyncCertLookup;
    static asyncCertFunc removeAsyncCertLookup;
    static void setAsyncCertFunc(asyncCertFunc add, asyncCertFunc remove)
    {
        if (add)
            addAsyncCertLookup = add;
        if (remove)
            removeAsyncCertLookup = remove;
    }
};

#endif
