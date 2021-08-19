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
    static SSL_CTX *newCtx();
    static void freeCtx(SSL_CTX *pCtx);
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
    static void setDefaultCipherList(const char *pList);
    static const char *buildCipherList(char *buf, int iMaxBufLen, const char *pList);
    static int useCipherList(SSL_CTX *pCtx, const char *pList);
    static int setCipherList(SSL_CTX *pCtx, const char *pList);
    static void updateProtocol(SSL_CTX *pCtx, int method);
    static int enableShmSessionCache(SSL_CTX *pCtx);
    static int enableSessionTickets(SSL_CTX *pCtx);
    static void disableSessionTickets(SSL_CTX *pCtx);
    static int getLcaseServerName(SSL *pSsl, char *name, int len);
    static bool isEcdsaSupported(const uint8_t *ciphers, size_t len);

    static AsyncCertFunc addAsyncCertLookup;
    static AsyncCertFunc removeAsyncCertLookup;
    static void setAsyncCertFunc(AsyncCertFunc add, AsyncCertFunc remove)
    {
        if (add)
            addAsyncCertLookup = add;
        if (remove)
            removeAsyncCertLookup = remove;
    }
};

#endif
