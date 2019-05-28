/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#ifndef SSLCERT_H
#define SSLCERT_H


/**
  *@author George Wang
  */
#include <assert.h>
#if __cplusplus <= 199711L && !defined(static_assert)
#define static_assert(a, b) _Static_assert(a, b)
#endif

#include <openssl/x509.h>

class SslCert
{
private:
    X509 *m_cert;
    char *m_pSubjectName;
    char *m_pIssuer;
    void release();
    SslCert(const SslCert &rhs);
    void operator=(const SslCert &rhs);
public:
    SslCert();
    explicit SslCert(X509 *pCert);

    ~SslCert();
    void operator=(X509 *pCert);
    bool operator==(X509 *pCert) const
    {   return m_cert == pCert ;    }
    bool operator!=(X509 *pCert) const
    {   return m_cert != pCert ;    }
    X509 *get() const
    {   return m_cert;  }
    const char *getSubjectName();
    const char *getIssuer();
    static int PEMWriteCert(X509 *pCert, char *pBuf, int len);
};

#endif
