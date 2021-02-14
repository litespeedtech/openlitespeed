/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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

#include <sslpp/sslocspstapling.h>
#include <sslpp/sslerror.h>
#include <sslpp/sslcontext.h>

#include <util/httpfetch.h>
#include <util/vmembuf.h>
#include <util/stringtool.h>
#include <util/datetime.h>
#include <log4cxx/logger.h>

#include <assert.h>
#if __cplusplus <= 199711L && !defined(static_assert)
#define static_assert(a, b) _Static_assert(a, b)
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>
#ifndef OPENSSL_IS_BORINGSSL
#include <openssl/ocsp.h>
#else
#include <sslpp/ocsp/ocsp.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static char * s_pOcspCachePath = NULL;

void SslOcspStapling::setCachePath(const char *pPath)
{
    if (s_pOcspCachePath)
        free(s_pOcspCachePath);
    s_pOcspCachePath = strdup(pPath);
}


const char *SslOcspStapling::getCachePath()
{
    return s_pOcspCachePath;
}


static AutoStr2 s_ErrMsg;
const char *getStaplingErrMsg() { return s_ErrMsg.c_str(); }

static void setLastErrMsg(const char *format, ...)
{
    const unsigned int MAX_LINE_LENGTH = 4096;
    char s[MAX_LINE_LENGTH] = {0};
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(s, MAX_LINE_LENGTH, format, ap);
    va_end(ap);

    if ((unsigned int)ret > MAX_LINE_LENGTH)
        ret = MAX_LINE_LENGTH;
    s_ErrMsg.setLen(0);
    s_ErrMsg.setStr(s, ret);
}


static X509 *load_cert(const char *pPath)
{
    X509 *pCert;
    BIO *bio = BIO_new_file(pPath, "r");
    if (bio == NULL)
        return NULL;
    pCert = PEM_read_bio_X509_AUX(bio, NULL, NULL, NULL);
    BIO_free(bio);
    return pCert;
}

static int OcspRespCb(void *pArg, HttpFetch *pHttpFetch)
{
    SslOcspStapling *pSslOcspStapling = (SslOcspStapling *)pArg;
    pSslOcspStapling->processResponse(pHttpFetch);
    return 0;
}

int SslOcspStapling::callback(SSL *ssl)
{
    int     iResult;
    iResult = SSL_TLSEXT_ERR_NOACK;
    if (m_RespTime == UINT_MAX)
        return iResult;
    unsigned char *resp_data = m_pRespData;
    update();
    if (m_pRespData && m_iDataLen > 0)
    {
#ifdef OPENSSL_IS_BORINGSSL
        if (m_pRespData != resp_data)
            SSL_set_ocsp_response(ssl, m_pRespData, m_iDataLen);
#else
        unsigned char *pOcspResp;
        /*OpenSSL will free pOcspResp by itself */
        pOcspResp = (unsigned char *)malloc(m_iDataLen);
        if (pOcspResp == NULL)
            return SSL_TLSEXT_ERR_NOACK;
        memcpy(pOcspResp, m_pRespData, m_iDataLen);
        SSL_set_tlsext_status_ocsp_resp(ssl, pOcspResp, m_iDataLen);
#endif
        iResult = SSL_TLSEXT_ERR_OK;
    }
    return iResult;
}

int SslOcspStapling::processResponse(HttpFetch *pHttpFetch)
{
    struct stat        st;
    const char          *pRespContentType;
    //assert( pHttpFetch == m_pHttpFetch );
    int istatusCode = m_pHttpFetch->getStatusCode() ;
    pRespContentType = m_pHttpFetch->getRespContentType();
    if (istatusCode == 200 && pRespContentType != NULL
        && strcasecmp(pRespContentType, "application/ocsp-response") == 0)
    {
        if (verifyRespFile(1) != 0)
        {
            m_pCtx->disableOscp();
            m_RespTime = UINT_MAX;
        }
    }
    else
    {
        setLastErrMsg("[OCSP] %s: Received bad OCSP response. ReponderUrl=%s, "
                      "StatusCode=%d, ContentType=%s\n",
                      m_sCertfile.c_str(),
                      m_sOcspResponder.c_str(), istatusCode,
                      ((pRespContentType) ? (pRespContentType) : ("")));
        //printf("%s\n", s_ErrMsg.c_str());
        m_pHttpFetch->writeLog(s_ErrMsg.c_str());
        m_pCtx->disableOscp();
        m_RespTime = UINT_MAX;
    }

    if (::stat(m_sRespfileTmp.c_str(), &st) == 0)
        unlink(m_sRespfileTmp.c_str());
    pHttpFetch->releaseResult();
    return 0;
}

SslOcspStapling::SslOcspStapling()
    : m_pHttpFetch(NULL)
    , m_pReqData(NULL)
    , m_iDataLen(0)
    , m_iocspRespMaxAge(3600 * 24)
    , m_pRespData(NULL)
    , m_notBefore(NULL)
    , m_pCtx(NULL)
    , m_RespTime(0)
    , m_statTime(0)
    , m_nextUpdate(0)
    , m_pCertId(NULL)
{
}

SslOcspStapling::~SslOcspStapling()
{
    releaseRespData();
    if (m_pHttpFetch != NULL)
        delete m_pHttpFetch;
    if (m_pReqData)
        free(m_pReqData);
    if (m_pCertId != NULL)
        OCSP_CERTID_free(m_pCertId);
    if (m_notBefore)
        ASN1_STRING_free(m_notBefore);
}

int SslOcspStapling::init(SslContext *pSslCtx)
{
    int             iResult;
    X509           *pCert;

    m_pCtx = pSslCtx;
    pCert = NULL;
    iResult = -1;

    //SSL_CTX_set_default_verify_paths( m_pCtx );

    pCert = load_cert(m_sCertfile.c_str());
    if (pCert == NULL)
    {
        setLastErrMsg("[OCSP] %s: Failed to load certificate file!\n",
                      m_sCertfile.c_str());
        return -1;
    }

    if ((getCertId(pCert) == 0) && (getResponder(pCert) == 0))
    {
        //m_addrResponder.setHttpUrl(m_sOcspResponder.c_str(),
        //                           m_sOcspResponder.len());
        iResult = 0;
        //update();
        const ASN1_TIME *not_before;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L || defined(OPENSSL_IS_BORINGSSL)
        not_before = X509_get0_notBefore(pCert);
#else
        not_before = X509_get_notBefore(pCert);
#endif
        ASN1_TIME_to_generalizedtime((ASN1_TIME *)not_before, &m_notBefore);
    }
    X509_free(pCert);
    return iResult;
}

int SslOcspStapling::update()
{
    int ret;
    struct stat st;
    if (m_RespTime == UINT_MAX)
        return 0;
    //NOTE: test code , to test clearing out expired OCSP response
    //if (m_pRespData && m_statTime + 10 <= DateTime::s_curTime)
    //    m_nextUpdate = DateTime::s_curTime;
    if (m_pRespData && m_nextUpdate <= DateTime::s_curTime)
    {
        LS_DBG("[OCSP] %s: OCSP response expired, cur: %ld, resp_create: %ld, expire: %ld.\n",
                    m_sRespfile.c_str(), DateTime::s_curTime, m_RespTime, m_nextUpdate);
        releaseRespData();
        if (m_statTime != DateTime::s_curTime)
        {
            m_statTime = DateTime::s_curTime;
            goto TRY_CREATE_REQ;
        }
        else
            return 0;
    }
    if (m_statTime != 0 && m_statTime + 60 >= DateTime::s_curTime)
        return 0;

    m_statTime = DateTime::s_curTime;

    if (::stat(m_sRespfile.c_str(), &st) == 0)
    {
        if (st.st_mtime + m_iocspRespMaxAge < DateTime::s_curTime)
        {
            releaseRespData();
            unlink(m_sRespfile.c_str());
        }
        else
        {
            if (m_RespTime == st.st_mtime)
                return 0;

            LS_DBG("[OCSP] %s: file timestamp updated, current: %ld, file: %ld\n",
                      m_sRespfile.c_str(), m_RespTime, st.st_mtime);

            if (verifyRespFile(0) == LS_OK)
            {
                m_RespTime = st.st_mtime;
                return 0;
            }
        }
    }

TRY_CREATE_REQ:
    ret = ::stat(m_sRespfileTmp.c_str(), &st);
    if (ret != 0 || st.st_mtime + 30 < DateTime::s_curTime)
    {
        if (ret == 0)
            unlink(m_sRespfileTmp.c_str());
        createRequest();
    }
    return 0;
}


int SslOcspStapling::getResponder(X509 *pCert)
{
    char                    *pUrl;
    X509                    *pCAt;
    STACK_OF(X509)          *pXchain;
    int                     i;
    int                     n;

#if OPENSSL_VERSION_NUMBER >= 0x10000003L
    STACK_OF(OPENSSL_STRING)  *strResp;
#else
    STACK                    *strResp;
#endif
    if (m_sOcspResponder.c_str())
        return 0;
    strResp = X509_get1_ocsp(pCert);
    if (strResp == NULL)
    {
#ifndef OPENSSL_IS_BORINGSSL
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
        SSL_CTX_get0_chain_certs(m_pCtx->get(), &pXchain);
#else        
        pXchain = m_pCtx->get()->extra_certs;
#endif        
#else
        SSL_CTX_get0_chain_certs(m_pCtx->get(), &pXchain);
#endif
        n = sk_X509_num(pXchain);
        for (i = 0; i < n; i++)
        {
            pCert = sk_X509_value(pXchain, i);
            strResp = X509_get1_ocsp(pCert);
            if (strResp)
                break;
        }
    }
    if (strResp == NULL)
    {
        if (m_sCAfile.c_str() == NULL)
            return -1;
        pCAt = load_cert(m_sCAfile.c_str());
        if (pCAt == NULL)
        {
            setLastErrMsg("[OCSP] %s: Failed to load CA file!\n",
                          m_sCAfile.c_str());
            return -1;
        }

        strResp = X509_get1_ocsp(pCAt);
        X509_free(pCAt);
        if (strResp == NULL)
        {
            setLastErrMsg("[OCSP] %s: Failed to get responder from CA!\n",
                          m_sCAfile.c_str());
            return -1;
        }
    }
#if OPENSSL_VERSION_NUMBER >= 0x1000004fL
    pUrl = (char *)sk_OPENSSL_STRING_value(strResp, 0);
#else
    pUrl = (char *)sk_value(strResp, 0);
#endif
    if (pUrl)
    {
        m_sOcspResponder.setStr(pUrl);
        X509_email_free(strResp);
        return 0;
    }
    X509_email_free(strResp);
    setLastErrMsg("[OCSP] %s: Failed to get responder Url!\n",
                  m_sCAfile.c_str());
    return -1;
}

int SslOcspStapling::getRequestData(unsigned char **pReqData)
{
    int             len = -1;
    OCSP_REQUEST    *ocsp;
    OCSP_CERTID     *id;

    if (m_pCertId == NULL)
        return -1;
    ocsp = OCSP_REQUEST_new();
    if (ocsp == NULL)
        return -1;

    id = OCSP_CERTID_dup(m_pCertId);
    if (OCSP_request_add0_id(ocsp, id) != NULL)
    {
        len = i2d_OCSP_REQUEST(ocsp, NULL);
        if (len > 0)
        {
            unsigned char *buf = (unsigned char *)malloc(len + 1);
            *pReqData = buf;
            len = i2d_OCSP_REQUEST(ocsp, &buf);
        }
    }
    OCSP_REQUEST_free(ocsp);
    return  len;
}

int SslOcspStapling::createRequest()
{
    int             len;
    if (m_pHttpFetch != NULL )
    {
        if (m_pHttpFetch->isInUse())
        {
            if (DateTime::s_curTime - m_pHttpFetch->getTimeStart() < 30)
                return 0;
            LS_DBG("[OCSP] %s: HTTP fetch timed out, state: %d\n",
                      m_sRespfileTmp.c_str(), m_pHttpFetch->getReqState());
        }
        delete m_pHttpFetch;
        m_pHttpFetch = NULL;
    }
    if (m_pReqData)
    {
        free(m_pReqData);
        m_pReqData = NULL;
    }
    len = getRequestData(&m_pReqData);
    if (len <= 0)
        return -1;
    
    if (*(m_sOcspResponder.c_str() + m_sOcspResponder.len() - 1) != '/')
    {
        m_sOcspResponder.append("/", 1);
    }
    
    m_pHttpFetch = new HttpFetch();
    m_pHttpFetch->setCallBack(OcspRespCb, this);
    m_pHttpFetch->setTimeout(30);  //Set Req timeout as 30 seconds
    m_pHttpFetch->startReq(m_sOcspResponder.c_str(), 1, 1, 
                           (const char *)m_pReqData, len,
                           m_sRespfileTmp.c_str(), "application/ocsp-request", 
                           NULL);
    LS_DBG("[OCSP] %s: %p, len = %d\n", m_sCertfile.c_str(),
            m_pHttpFetch, len);
    //printf("%s\n", s_ErrMsg.c_str());
    return 0;
}


void SslOcspStapling::releaseRespData()
{
    if (m_pRespData != NULL)
    {
        delete [] m_pRespData;
        m_pRespData = NULL;
    }
}


int SslOcspStapling::updateRespData(OCSP_RESPONSE *pResponse)
{
    unsigned char *pOcspResp;
    m_iDataLen = i2d_OCSP_RESPONSE(pResponse, NULL);
    if (m_iDataLen > 0)
    {
        if (m_pRespData != NULL)
        {
            releaseRespData();
        }

        m_pRespData = new unsigned char[m_iDataLen];
        pOcspResp = m_pRespData;
        m_iDataLen = i2d_OCSP_RESPONSE(pResponse, &(pOcspResp));
        if (m_iDataLen <= 0)
        {
            releaseRespData();
            return -1;
        }
#ifdef OPENSSL_IS_BORINGSSL
        else if (m_pCtx)
            SSL_CTX_set_ocsp_response(m_pCtx->get(), m_pRespData, m_iDataLen);
#endif
        return 0;
    }
    return -1;
}


time_t ASN1_TIME_to_time_t(const ASN1_TIME * asn1)
{
    int days = 0, seconds = 0;
    if (!ASN1_TIME_diff(&days, &seconds, NULL, asn1))
        return -1;
    time_t t = time(NULL);
    t += days * 86400 + seconds;
    return t;
}


int SslOcspStapling::certVerify(OCSP_RESPONSE *pResponse,
                                OCSP_BASICRESP *pBasicResp, X509_STORE *pXstore)
{
    int                 n, iResult = -1;
    STACK_OF(X509)      *pXchain;
    ASN1_GENERALIZEDTIME  *pThisupdate, *pNextupdate;

#ifndef OPENSSL_IS_BORINGSSL
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
        SSL_CTX_get0_chain_certs(m_pCtx->get(), &pXchain);
#else        
    pXchain = m_pCtx->get()->extra_certs;
#endif    
#else
    SSL_CTX_get0_chain_certs(m_pCtx->get(), &pXchain);
#endif
    if (OCSP_basic_verify(pBasicResp, pXchain,
                          pXstore, /* OCSP_TRUSTOTHER */ OCSP_NOVERIFY) == 1)
    {
        if (m_pCertId == NULL)
        {
            LS_NOTICE("[OCSP] %s: cert ID is NULL\n", m_sCertfile.c_str());
            return 1;
        }
        int find_status = OCSP_resp_find_status(pBasicResp, m_pCertId, &n,
                                      NULL, NULL, &pThisupdate, &pNextupdate);
        int validate = -100;

        if (find_status == 1 && n == V_OCSP_CERTSTATUS_GOOD)
        {
            int day, sec;
            //NOTE: get around an issue with response have wrong nextupdate time.
            //      make sure it is later than thisupdate.
            if (pThisupdate && pNextupdate)
            {
                if (ASN1_TIME_diff(&day, &sec, pThisupdate, pNextupdate) == 0
                    || (day <= 0 && sec <= 0))
                {
                    LS_NOTICE("[OCSP] %s: Bad next_update time day: %d, sec: %d, "
                                "set it to NULL\n", m_sCertfile.c_str(), day, sec);
                    pNextupdate = NULL;
                }
            }
            if (m_notBefore && ASN1_TIME_diff(&day, &sec, m_notBefore, pThisupdate)
                && (day > 0 || sec > 0))
                validate = OCSP_check_validity(pThisupdate, pNextupdate, 300, -1);
        }

        if (validate == 1)
        {
            m_nextUpdate = ASN1_TIME_to_time_t(pNextupdate);
            iResult = 0;
        }
        else
        {
            LS_NOTICE("[OCSP] %s: verify failed, find_status: %d, status: %d, validate: %d\n",
                      m_sCertfile.c_str(), find_status, n, validate);
        }
    }
    else
    {
        LS_NOTICE("[OCSP] %s: OCSP_basic_verify() failed: %s\n",
                  m_sCertfile.c_str(), SslError().what());
        m_pCtx->disableOscp();
        m_RespTime = UINT_MAX;

    }
    if (iResult)
    {
        setLastErrMsg("[OCSP] %s: OCSP_basic_verify() error: %s",
                      m_sCertfile.c_str(), SslError().what());
        ERR_clear_error();
        if (m_pHttpFetch)
            m_pHttpFetch->writeLog(s_ErrMsg.c_str());

    }
    return iResult;
}

int SslOcspStapling::verifyRespFile(int is_new_resp)
{
    int                 iResult = -1;
    BIO                 *pBio;
    OCSP_RESPONSE       *pResponse;
    OCSP_BASICRESP      *pBasicResp;
    uint8_t             *pBuf = NULL;
    const unsigned char *pCopy;
    size_t              ulSize;
    X509_STORE *pXstore;
    const char *file_name;
    if (is_new_resp)
        file_name = m_sRespfileTmp.c_str();
    else
        file_name = m_sRespfile.c_str();

    pBio = BIO_new_file(file_name, "r");
    if (pBio == NULL)
    {
        LS_ERROR("[OCSP] %s: Failed to open file.\n", file_name);
        return -1;
    }

#ifdef OPENSSL_IS_BORINGSSL
    if (1 != BIO_read_asn1(pBio, &pBuf, &ulSize, 100 * 1024))
    {
        BIO_free(pBio);
        return -1;
    }
    BIO_free(pBio);

    pCopy = pBuf;
    pResponse = d2i_OCSP_RESPONSE(NULL, &pCopy, ulSize);
    OPENSSL_free(pBuf);
#else
    pResponse = d2i_OCSP_RESPONSE_bio(pBio, NULL);
#endif
    if (pResponse == NULL)
    {
        LS_ERROR("[OCSP] %s: Failed to load OCSP RESPONSE.\n", file_name);
        return -1;
    }

    if (OCSP_response_status(pResponse) == OCSP_RESPONSE_STATUS_SUCCESSFUL)
    {
        pBasicResp = OCSP_response_get1_basic(pResponse);
        if (pBasicResp != NULL)
        {
            pXstore = SSL_CTX_get_cert_store(m_pCtx->get());
            if (pXstore)
            {
                iResult = certVerify(pResponse, pBasicResp, pXstore);
                if (iResult == 0 && is_new_resp)
                {
                    LS_DBG("[OCSP] %s: Update OCSP response file\n",
                        m_sRespfile.c_str());
                    unlink(m_sRespfile.c_str());
                    rename(m_sRespfileTmp.c_str(), m_sRespfile.c_str());
                    struct stat st;
                    if (::stat(m_sRespfile.c_str(), &st) == 0)
                        m_RespTime = st.st_mtime;
                }
            }
            OCSP_BASICRESP_free(pBasicResp);
        }
        if (iResult == 0)
        {
            iResult = updateRespData(pResponse);
            LS_DBG("[OCSP] %s: Update OCSP response data: %s\n",
                   file_name, (iResult == 0) ? "succeed" : "fail");
        }
    }
    OCSP_RESPONSE_free(pResponse);
    return iResult;
}

int SslOcspStapling::getCertId(X509 *pCert)
{
    int     i, n;
    X509                *pXissuer;
    X509_STORE          *pXstore;
    STACK_OF(X509)      *pXchain;
    X509_STORE_CTX      *pXstore_ctx;


#ifndef OPENSSL_IS_BORINGSSL
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    SSL_CTX_get0_chain_certs(m_pCtx->get(), &pXchain);
#else        
    pXchain = m_pCtx->get()->extra_certs;
#endif    
#else
    SSL_CTX_get0_chain_certs(m_pCtx->get(), &pXchain);
#endif
    n = sk_X509_num(pXchain);
    for (i = 0; i < n; i++)
    {
        pXissuer = sk_X509_value(pXchain, i);
        if (X509_check_issued(pXissuer, pCert) == X509_V_OK)
        {
#ifndef OPENSSL_IS_BORINGSSL
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
            X509_up_ref(pXissuer);
#else            
            CRYPTO_add(&pXissuer->references, 1, CRYPTO_LOCK_X509);
#endif            
#else
            X509_up_ref(pXissuer);
#endif
            m_pCertId = OCSP_cert_to_id(NULL, pCert, pXissuer);
            X509_free(pXissuer);
            return 0;
        }
    }
    pXstore = SSL_CTX_get_cert_store(m_pCtx->get());
    if (pXstore == NULL)
    {
        setLastErrMsg("[OCSP] %s: SSL_CTX_get_cert_store failed!",
                      m_sCertfile.c_str());
        return -1;
    }
    pXstore_ctx = X509_STORE_CTX_new();
    if (pXstore_ctx == NULL)
    {
        setLastErrMsg("[OCSP] %s: X509_STORE_CTX_new failed!",
                      m_sCertfile.c_str());
        return -1;
    }
    if (X509_STORE_CTX_init(pXstore_ctx, pXstore, NULL, NULL) == 0)
    {
        setLastErrMsg("[OCSP] %s: X509_STORE_CTX_init failed!",
                      m_sCertfile.c_str());
        return -1;
    }
    n = X509_STORE_CTX_get1_issuer(&pXissuer, pXstore_ctx, pCert);
    X509_STORE_CTX_free(pXstore_ctx);
    if ((n == -1) || (n == 0))
    {
        setLastErrMsg("[OCSP] %s: X509_STORE_CTX_get1_issuer failed!",
                      m_sCertfile.c_str());
        return -1;
    }
    m_pCertId = OCSP_cert_to_id(NULL, pCert, pXissuer);
    X509_free(pXissuer);
    return 0;
}

void SslOcspStapling::setCertFile(const char *Certfile)
{
    char RespFile[4096], *pExt;
    unsigned char md5[16];
    char md5Str[35] = {0};
    int iLen;
    m_sCertfile.setStr(Certfile);
    StringTool::getMd5(Certfile, strlen(Certfile), md5);
    StringTool::hexEncode((const char *)md5, 16, md5Str);
    md5Str[32] = 0;
    iLen = snprintf(RespFile, 4095, "%s/R%s", s_pOcspCachePath, md5Str);
    pExt = RespFile + iLen;
    snprintf(pExt, 5, ".rsp");
    m_sRespfile.setStr(RespFile);
    snprintf(pExt, 5, ".tmp");
    m_sRespfileTmp.setStr(RespFile);
}
