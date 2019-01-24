/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#include "sslcert.h"

#include <openssl/ssl.h>

SslCert::SslCert()
    : m_cert(NULL)
    , m_pSubjectName(NULL)
    , m_pIssuer(NULL)
{
}
SslCert::SslCert(X509 *pCert)
    : m_cert(pCert)
    , m_pSubjectName(NULL)
    , m_pIssuer(NULL)
{}

SslCert::~SslCert()
{
    release();
}

void SslCert::release()
{
    if (m_cert)
    {
        X509 *pCert = m_cert;
        m_cert = NULL;
        X509_free(pCert);
        if (m_pSubjectName)
        {
            char *p = m_pSubjectName;
            m_pSubjectName = NULL;
            free(p);
        }
        if (m_pIssuer)
        {
            char *p = m_pIssuer;
            m_pIssuer = NULL;
            free(p);
        }
    }
}

void SslCert::operator=(X509 *pCert)
{
    release();
    m_cert = pCert;
}

const char *SslCert::getSubjectName()
{
    if (!m_pSubjectName)
    {
        if (m_cert)
            m_pSubjectName = X509_NAME_oneline(
                                 X509_get_subject_name(m_cert), 0, 0);
    }
    return m_pSubjectName;
}

const char *SslCert::getIssuer()
{
    if (!m_pIssuer)
    {
        if (m_cert)
            m_pIssuer = X509_NAME_oneline(
                            X509_get_issuer_name(m_cert), 0, 0);
    }
    return m_pIssuer;
}

int SslCert::PEMWriteCert(X509 *pCert, char *pBuf, int len)
{
    int n;
    BIO *bio;
    if (!pCert)
        return -1;
    bio = BIO_new(BIO_s_mem());
    if (bio == NULL)
        return -1;
    PEM_write_bio_X509(bio, pCert);
    n = BIO_pending(bio);
    if (pBuf && len >= n)
        n = BIO_read(bio, pBuf, n);
    BIO_free(bio);
    return n;
}
