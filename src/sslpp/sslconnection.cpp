/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#include "sslconnection.h"

#include <log4cxx/logger.h>
#include <lsr/ls_pool.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <sslpp/sslerror.h>

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>


const int RBIO_BUF_SIZE = 1024;
int32_t SslConnection::s_iConnIdx = -1;

SslConnection::SslConnection()
    : m_ssl(NULL)
    , m_pSessCache(NULL)
    , m_iStatus(DISCONNECTED)
    , m_iWant(0)
    , m_flag(0)
    , m_iUseRbio(0)
    , m_rbioBuf(NULL)
    , m_rbioBuffered(0)
{
}


SslConnection::~SslConnection()
{
    if (m_rbioBuf)
        ls_pfree(m_rbioBuf);
    if (m_ssl)
        release();
}


void SslConnection::setSSL(SSL *ssl)
{
    assert(!m_ssl);
    //m_iWant = 0;
    m_ssl = ssl;
    m_flag = 0;
    SSL_set_ex_data(ssl, s_iConnIdx, (void *)this);
}

void SslConnection::release()
{
    assert(m_ssl);
    if (m_iStatus != DISCONNECTED)
        shutdown(0);

    SSL_free(m_ssl);
    m_ssl = NULL;
    // All buffer counters must be set to 0 to reset everything.
    m_rbioBuffered = 0;
    m_iRFd = -1;

    if (m_rbioBuf)
    {
        ls_pfree(m_rbioBuf);
        m_rbioBuf = NULL;
    }
}


int SslConnection::installRbio(int rfd, int wfd)
{
    assert(m_ssl);
    BIO *rbio = BIO_new(BIO_s_mem());
    if (!rbio)
    {
        errno = ENOMEM;
        m_iUseRbio = 0;
        return -1;
    }
    if (!(m_rbioBuf = (char *)ls_palloc(RBIO_BUF_SIZE)))
    {
        BIO_free(rbio);
        errno = ENOMEM;
        m_iUseRbio = 0;
        return -1;
    }

    m_iRFd = rfd;
#ifndef OPENSSL_IS_BORINGSSL
    SSL_set_fd(m_ssl, rfd);
    m_saved_rbio = SSL_get_rbio(m_ssl);
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    BIO_up_ref(m_saved_rbio);
    SSL_set0_rbio(m_ssl, rbio);
#else
    m_ssl->rbio = rbio;
#endif //OPENSSL_VERSION_NUMBER >= 0x10100000L
#else
    SSL_set_bio(m_ssl, rbio, NULL);
    SSL_set_wfd(m_ssl, wfd);
#endif
    return 0;
}


int SslConnection::setfd(int fd)
{
    m_iWant = 0;

    if (m_iUseRbio && installRbio(fd,fd) == 0)
        return 1;
    if (SSL_set_fd(m_ssl, fd) == -1)
        ;
    return 1;
}


int SslConnection::setfd(int rfd, int wfd)
{
    m_iWant = 0;
    if (m_iUseRbio && installRbio(rfd,wfd) == 0)
        return 1;
    if (SSL_set_rfd(m_ssl, rfd) == -1 || SSL_set_wfd(m_ssl, wfd) == -1)
    {
        m_iStatus = DISCONNECTED;
        return 0;
    }
    return 1;
}


int SslConnection::readRbioClientHello()
{
    int ret;
    ssize_t dataRead;

    // Return if no data to service the next request.
    dataRead = recv(m_iRFd, &m_rbioBuf[m_rbioBuffered],
                    RBIO_BUF_SIZE - m_rbioBuffered, 0);
    if (dataRead == -1)
    {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
            return 0;
        }
        return -1;
    }
    else if (dataRead == 0)
    {
        errno = ECONNRESET; // Just something somewhat descriptive
        return -1;
    }

    ERR_clear_error();
    ret = BIO_write(SSL_get_rbio(m_ssl), &m_rbioBuf[m_rbioBuffered], dataRead);
    m_rbioBuffered += (int)dataRead;

    if (ret <= 0)
    {
        return -1;
    }
    return 1;
}


void SslConnection::restoreRbio()
{
#ifdef OPENSSL_IS_BORINGSSL
    SSL_set_rfd(m_ssl, m_iRFd);
#else
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    SSL_set0_rbio(m_ssl,m_saved_rbio);
#else
    BIO_free_all(m_ssl->rbio);
    m_ssl->rbio = m_saved_rbio;
#endif //OPENSSL_VERSION_NUMBER >= 0x10100000L
#endif
    ls_pfree(m_rbioBuf);
    m_rbioBuf = NULL;
    m_iUseRbio = 0;
}


int SslConnection::read(char *pBuf, int len)
{
    assert(m_ssl);
    m_iWant = 0;
    char *p = pBuf;
    char *pEnd = pBuf + len;
    int ret;
    while(p < pEnd)
    {
        ret = SSL_read(m_ssl, p, pEnd - p);
        if (ret > 0)
        {
            p += ret;
        }
        else
        {
            m_iWant = LAST_READ;
            if (p == pBuf)
                return checkError(ret);
            break;
        }
    }
    return p - pBuf;
}

int SslConnection::write(const char *pBuf, int len)
{
    assert(m_ssl);
    m_iWant = 0;
    if (len <= 0)
        return 0;
    int ret = SSL_write(m_ssl, pBuf, len);

    LS_DBG_M("SSL_write( %p, %p, %d) return %d, pending %d\n",
             m_ssl, pBuf, len, ret, wpending());

    if (ret > 0)
    {
        m_iWant &= ~LAST_WRITE;
        return ret;
    }
    else
    {
        LS_DBG_M("SslError:%s\n", SslError(SSL_get_error(m_ssl, ret)).what());
        m_iWant = LAST_WRITE;
        return checkError(ret);
    }
}


int SslConnection::writev(const struct iovec *vect, int count,
                          int *finished)
{
    int ret = 0;

    const struct iovec *pEnd = vect + count;
    const char *pBuf;
    int bufSize;
    int written;

    char *pBufEnd;
    char *pCurEnd;
    char achBuf[4096];
    pBufEnd = achBuf + 4096;
    pCurEnd = achBuf;
    for (; vect < pEnd ;)
    {
        pBuf = (const char *) vect->iov_base;
        bufSize = vect->iov_len;
        if (bufSize < 1024)
        {
            if (pBufEnd - pCurEnd > bufSize)
            {
                memmove(pCurEnd, pBuf, bufSize);
                pCurEnd += bufSize;
                ++vect;
                if (vect < pEnd)
                    continue;
            }
            pBuf = achBuf;
            bufSize = pCurEnd - pBuf;
            pCurEnd = achBuf;
        }
        else if (pCurEnd != achBuf)
        {
            pBuf = achBuf;
            bufSize = pCurEnd - pBuf;
            pCurEnd = achBuf;
        }
        else
            ++vect;
        written = write(pBuf, bufSize);
        if (written > 0)
        {
            ret += written;
            if (written < bufSize)
                break;
        }
        else if (!written || ret)
            break;
        else
            return -1;
    }
    if (finished)
        *finished = (vect == pEnd);
    return ret;
}


int SslConnection::wpending()
{
    BIO *pBIO = SSL_get_wbio(m_ssl);
    return BIO_wpending(pBIO);
}


int SslConnection::flush()
{
    BIO *pBIO = SSL_get_wbio(m_ssl);
    if (!pBIO)
        return 0;
    int ret = BIO_flush(pBIO);
    if (ret != 1)     //1 means BIO_flush succeed.
        return checkError(ret);
    return ret;
}


int SslConnection::shutdown(int bidirectional)
{
    assert(m_ssl);
    if (m_iStatus == ACCEPTING)
    {
        ERR_clear_error();
        m_iStatus = DISCONNECTED;
        return 0;
    }
    if (m_iStatus != DISCONNECTED)
    {
        m_iWant = 0;
        SSL_set_shutdown(m_ssl, SSL_RECEIVED_SHUTDOWN);
        //SSL_set_quiet_shutdown( m_ssl, !bidirectional );
        int ret = SSL_shutdown(m_ssl);
        if ((ret) ||
            ((ret == 0) && (!bidirectional)))
        {
            m_iStatus = DISCONNECTED;
            return 0;
        }
        else
            m_iStatus = SHUTDOWN;
        return checkError(ret);
    }
    return 0;
}


void SslConnection::toAccept()
{
    m_iStatus = ACCEPTING;
    m_iWant = READ;
}


int SslConnection::accept()
{
    int ret;
    assert(m_ssl);

    if (m_iUseRbio && (m_iWant & READ))
    {
        ret = readRbioClientHello();
        if (ret < 0)
        {
            return ret;
        }
    }
    m_iWant = 0;
    ret = SSL_accept(m_ssl);
    if (ret == 1)
    {
        m_iStatus = CONNECTED;
        m_iWant = READ;
    }
    else
    {
        ret = checkError(ret);
    }
    if (m_iUseRbio && ret != -1 && m_rbioBuffered > 100)
    {
        restoreRbio();
    }
    return ret;
}


int SslConnection::checkError(int ret)
{
    int err = SSL_get_error(m_ssl, ret);
    //printf( "SslError:%s\n", SslError(err).what() );
    switch (err)
    {
    case SSL_ERROR_ZERO_RETURN:
        errno = ECONNRESET;
        break;
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        if (err == SSL_ERROR_WANT_READ)
            m_iWant |= READ;
        else
            m_iWant |= WRITE;
        ERR_clear_error();
        return 0;
    case SSL_ERROR_WANT_X509_LOOKUP:
        LS_DBG_H("[SSLSNI] need to pause SSL accept to fetch cert.");
        m_iStatus = WAITINGCERT;
        ERR_clear_error();
        return 0; // This will trigger a setSSLAgain(), which will disable read/write.
    case SSL_ERROR_SSL:
        if (m_iWant == LAST_WRITE)
        {
            m_iWant |= WRITE;
            ERR_clear_error();
            return 0;
        }
        //fall through
    case SSL_ERROR_SYSCALL:
        {
            int ret = ERR_get_error();
            if (ret == 0)
            {
                errno = ECONNRESET;
                break;
            }
        }
    default:
        LS_DBG_H("SslError:%s\n", SslError(err).what());
        errno = EIO;
    }
    return -1;
}


int SslConnection::connect()
{
    assert(m_ssl);
    m_iStatus = CONNECTING;
    m_iWant = 0;
    int ret = SSL_connect(m_ssl);
    if (ret == 1)
    {
        m_iStatus = CONNECTED;
        return 1;
    }
    else
        return checkError(ret);
}


int SslConnection::tryagain()
{
    assert(m_ssl);
    switch (m_iStatus)
    {
    case CONNECTING:
        return connect();
    case ACCEPTING:
        return accept();
    case SHUTDOWN:
        return shutdown(1);
    }
    return 0;
}


int SslConnection::updateOnGotCert()
{
    assert(m_iStatus == WAITINGCERT);
    m_iStatus = GOTCERT;
    return 0;
}


X509 *SslConnection::getPeerCertificate() const
{   return SSL_get_peer_certificate(m_ssl);   }


long SslConnection::getVerifyResult() const
{   return SSL_get_verify_result(m_ssl);      }


int SslConnection::getVerifyMode() const
{   return SSL_get_verify_mode(m_ssl);        }


const char *SslConnection::getCipherName() const
{   return SSL_get_cipher_name(m_ssl);    }


const SSL_CIPHER *SslConnection::getCurrentCipher() const
{   return SSL_get_current_cipher(m_ssl); }


SSL_SESSION *SslConnection::getSession() const
{   return SSL_get_session(m_ssl);        }


int SslConnection::setSession(SSL_SESSION *session) const
{   return SSL_set_session(m_ssl, session);     }


int SslConnection::isSessionReused() const
{   return SSL_session_reused(m_ssl);       }


const char *SslConnection::getVersion() const
{   return SSL_get_version(m_ssl);        }


static const char NPN_SPDY_PREFIX[] = { 's', 'p', 'd', 'y', '/' };
int SslConnection::getSpdyVersion()
{
    int v = 0;

    unsigned int             len = 0;
    const unsigned char     *data = NULL;

#ifdef TLSEXT_TYPE_application_layer_protocol_negotiation
    SSL_get0_alpn_selected(m_ssl, &data, &len);
#endif

#ifdef TLSEXT_TYPE_next_proto_neg
    if (!data)
        SSL_get0_next_proto_negotiated(m_ssl, &data, &len);
#endif
    if (len > sizeof(NPN_SPDY_PREFIX) &&
        strncasecmp((const char *)data, NPN_SPDY_PREFIX,
                    sizeof(NPN_SPDY_PREFIX)) == 0)
    {
        v = data[ sizeof(NPN_SPDY_PREFIX) ] - '1';
        if ((v == 2) && (len >= 8) && (data[6] == '.') && (data[7] == '1'))
            v = 3;
        return v;
    }

    //h2: http2 version is 4
    if (len >= 2 && data[0] == 'h' && data[1] == '2')
        return 4;

    return v;
}


void SslConnection::initConnIdx()
{
    if (s_iConnIdx < 0)
        s_iConnIdx = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);
}


int SslConnection::getCipherBits(const SSL_CIPHER *pCipher,
                                 int *algkeysize)
{
    return SSL_CIPHER_get_bits((SSL_CIPHER *)pCipher, algkeysize);
}


int SslConnection::isClientVerifyOptional(int i)
{   return i == SSL_VERIFY_PEER;    }


int SslConnection::isVerifyOk() const
{   return SSL_get_verify_result(m_ssl) == X509_V_OK;     }


int SslConnection::buildVerifyErrorString(char *pBuf, int len) const
{
    return snprintf(pBuf, len, "FAILED: %s", X509_verify_cert_error_string(
                        SSL_get_verify_result(m_ssl)));
}


int SslConnection::setTlsExtHostName(const char *pName)
{
    if (pName)
        return SSL_set_tlsext_host_name(m_ssl, pName);
    return  0;
}


const char *SslConnection::getTlsExtHostName()
{
    if (m_iStatus >= ACCEPTING)
        return SSL_get_servername(m_ssl, TLSEXT_NAMETYPE_host_name);
    return NULL;
}

#include <util/stringtool.h>
#include <sslcert.h>
#include "sslsesscache.h"
int SslConnection::getEnv(HioCrypto::ENV id, char *&pValue, int bufLen)
{
    SSL_SESSION *pSession;
    X509 *pClientCert;
    const SSL_CIPHER *pCipher;
    int algkeysize;
    int keysize;
    int ret = 0;

    switch(id)
    {
    case CRYPTO_VERSION:
        pValue = (char *)getVersion();
        ret = strlen(pValue);
        break;
    case SESSION_ID:
        pSession = getSession();
        if (pSession)
        {
            unsigned int idLen;
            const unsigned char *pId;
            pId = SSL_SESSION_get_id(pSession, &idLen);
            ret = idLen * 2;
            if (ret > bufLen)
                ret = bufLen;
            StringTool::hexEncode((char *)pId, ret / 2, pValue);
        }
        break;
    case CLIENT_CERT:
        pClientCert = getPeerCertificate();
        if (pClientCert)
            ret = SslCert::PEMWriteCert(pClientCert, pValue, bufLen);
        break;
    case CIPHER:
        pValue = (char *)SSL_get_cipher_name(m_ssl);
        ret = strlen(pValue);
        break;
    case CIPHER_USEKEYSIZE:
    case CIPHER_ALGKEYSIZE:
        pCipher = getCurrentCipher();
        keysize = SSL_CIPHER_get_bits((SSL_CIPHER *)pCipher, &algkeysize);
        if (id == CIPHER_USEKEYSIZE)
            ret = snprintf(pValue, 20, "%d", keysize);
        else //LSI_VAR_SSL_CIPHER_ALGKEYSIZE
            ret = snprintf(pValue, 20, "%d", algkeysize);
        break;
    default:
        break;
    }
    return ret;
}


char* SslConnection::getRawBuffer(int *len)
{
    *len = m_rbioBuffered;
    return m_rbioBuf;
}


int SslConnection::cacheClientSession(SSL_SESSION* session, const char *pHost, int iHostLen)
{
    if (m_pSessCache)
    {
        m_pSessCache->saveSession(pHost, iHostLen, session);
        return 1; //take ownership of ssession
    }
    return 0;
}


void SslConnection::tryReuseCachedSession(const char *pHost, int iHostLen)
{
    if (!m_pSessCache)
        return;
    SSL_SESSION *session = m_pSessCache->getSession(pHost, iHostLen);
    if (session)
        SSL_set_session(m_ssl, session);
}



