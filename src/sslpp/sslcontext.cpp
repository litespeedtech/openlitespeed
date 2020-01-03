/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#include "sslcontext.h"
#include <sslpp/sslconnection.h>
#include <sslpp/sslerror.h>
#include <sslpp/sslocspstapling.h>
#include <sslpp/sslutil.h>
#include <sslpp/sslcontextconfig.h>
#include <sslpp/sslasyncpk.h>
#include <sslpp/sslcertcomp.h>

#include <log4cxx/logger.h>

#include <assert.h>
#if __cplusplus <= 199711L && !defined(static_assert)
#define static_assert(a, b) _Static_assert(a, b)
#endif

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


SslContext *SslContext::config(SslContext *pContext, const char *pZcDomainName,
        const char * pKey, const char * pCert, const char * pBundle)
{
    int ret;
    if (( !pContext )
            // || ( !pContext->isZConf )
       )
    {
        // TODO: what should happen if old context was not zconf?
        // it will have irrelevant file system data in it?

        SslContext* pNewContext = new SslContext( SslContext::SSL_ALL );
        LS_DBG_L("[SSL] Create SSL context (ZConf)");
        if (!pNewContext)
        {
            LS_DBG_L("[SSL] Insufficient memory\n");
            return NULL;
        }
        
        if (pNewContext->init())
            return NULL;


        pContext = pNewContext;
    }
    if ((ret = SslUtil::loadPrivateKey(pContext->m_pCtx, (void*) pKey, strlen(pKey))) <= 1) {
        LS_ERROR( "[SSL] Config SSL Context (ZConf) with key failed.");
        // delete pNewContext;
        return NULL;
    }
    pContext->m_iKeyLen = ret;
    if ((ret = SslUtil::loadCert(pContext->m_pCtx, (void*)pCert, strlen(pCert))) != 1) {
        LS_ERROR( "[SSL] Config SSL Context (ZConf) with cert failed.");
        // delete pNewContext;
        return NULL;
    }

    if (!SslUtil::loadCA(pContext->m_pCtx, pBundle)) {
        LS_ERROR( "[SSL] Config SSL Context (ZConf) with bundle failed.");
        // delete pNewContext;
        return NULL;
    }

#ifdef SSL_ASYNC_PK
    ssl_ctx_enable_apk(pContext->m_pCtx);
#endif

#ifdef SSLCERTCOMP
    SslCertComp::enableCertComp(pContext->m_pCtx);
#endif
    return pContext;
}

SslContext *SslContext::config(SslContext *pContext, SslContextConfig *pConfig)
{
    int ret;
    SslContext* pNewContext = NULL;
    if (( !pContext )||
            ( pContext->isKeyFileChanged( pConfig->m_sKeyFile[0].c_str() )||
              pContext->isCertFileChanged( pConfig->m_sCertFile[0].c_str() )))
    {
        pNewContext = new SslContext( SslContext::SSL_ALL );
        LS_DBG_L("[SSL] Create SSL context.");
        if (!pNewContext)
        {
            LS_DBG_L("[SSL] Insufficient memory\n");
            return NULL;
        }
        if ( pConfig->m_iEnableMultiCerts )
        {
            ret = pNewContext->setMultiKeyCertFile(pConfig->m_sKeyFile[0].c_str(),
                            SslUtil::FILETYPE_PEM, pConfig->m_sCertFile[0].c_str(),
                            SslUtil::FILETYPE_PEM, pConfig->m_iCertChain);
            if (pConfig->m_sCAFile.c_str()
                && strcmp(pConfig->m_sCertFile[0].c_str(),
                            pConfig->m_sCAFile.c_str()) == 0)
            {
                pConfig->m_sCAFile.release();
            }
        }
        else
        {
            for(int i = 0; i <= pConfig->m_iKeyCerts; ++i )
            {
                ret = pNewContext->setKeyCertificateFile(pConfig->m_sKeyFile[i].c_str(),
                            SslUtil::FILETYPE_PEM, pConfig->m_sCertFile[i].c_str(),
                            SslUtil::FILETYPE_PEM, pConfig->m_iCertChain);
                if (pConfig->m_sCAFile.c_str()
                    && strcmp(pConfig->m_sCertFile[i].c_str(),
                              pConfig->m_sCAFile.c_str()) == 0)
                {
                    pConfig->m_sCAFile.release();
                }
            }
        }
        if ( !ret )
        {
            LS_ERROR( "[SSL] Config SSL Context with Certificate File: %s"
                    " and Key File:%s get SSL error: %s",
                    pConfig->m_sCertFile[0].c_str(), pConfig->m_sKeyFile[0].c_str(),
                    SslError().what());
            delete pNewContext;
            return NULL;
        }
        if (( pConfig->m_sCAFile.c_str() || pConfig->m_sCAPath.c_str() )
            && ( !pNewContext->setCALocation( pConfig->m_sCAFile.c_str(),
                            pConfig->m_sCAPath.c_str(), pConfig->m_iClientVerify )))
        {
            LS_ERROR( "[SSL] Failed to setup Certificate Authority "
                      "Certificate File: '%s', Path: '%s', SSL error: %s",
                      pConfig->m_sCAFile.c_str() ? pConfig->m_sCAFile.c_str() : "",
                      pConfig->m_sCAPath.c_str() ? pConfig->m_sCAPath.c_str() : "",
                      SslError().what());
            delete pNewContext;
            return NULL;
        }
        pContext = pNewContext;
    }
#ifdef OPENSSL_IS_BORINGSSL
    if (!pConfig->m_sCaChainFile.c_str()
        && pConfig->m_sCAFile.c_str())
    {
        //BoringSSL does not build the CA Chain with setCALocation
        //So, we do it explicitly
        pConfig->m_sCaChainFile.setStr(pConfig->m_sCAFile.c_str());
    }
#endif
    if (pConfig->m_sCaChainFile.c_str())
        if (pContext->setCertificateChainFile(
                 pConfig->m_sCaChainFile.c_str()) <= 0)
    {
        LS_ERROR("[SSL] Vhost %s: failed to set Certificate Chain file: %s"
                 , pConfig->m_sName.c_str(), pConfig->m_sCaChainFile.c_str());
    }

    pContext->setRenegProtect(!pConfig->m_iInsecReneg);

    pContext->setProtocol( pConfig->m_iProtocol );
    if ( pConfig->m_iEnableECDHE )
    {
        if ( pContext->initECDH() == LS_FAIL )
        {
            LS_ERROR("[SSL] Init ECDH failed.");
            if (pNewContext)
                delete pNewContext;
            return NULL;
        }
    }
    if ( pConfig->m_iEnableDHE )
    {
        if ( pContext->initDH( pConfig->m_sDHParam.c_str() ) == LS_FAIL )
        {
            LS_ERROR("[SSL] Init DH failed.");
            if (pNewContext)
                delete pNewContext;
            return NULL;
        }
    }

    if (pConfig->m_iEnableCache)
    {

        if (pContext->enableShmSessionCache() == LS_FAIL)
        {
            LS_ERROR("[SSL] Enable session cache failed.");
            if (pNewContext)
                delete pNewContext;
            return NULL;
        }
    }

    if (pConfig->m_iEnableTicket == 1)
    {
        if (pContext->enableSessionTickets() == LS_FAIL)
        {
            LS_ERROR("[SSL] Enable session ticket failed.");
            if (pNewContext)
                delete pNewContext;
            return NULL;
        }
    }
    else
        pContext->disableSessionTickets();

    if ( pConfig->m_iEnableSpdy != 0 )
    {
        if ( pContext->enableSpdy( pConfig->m_iEnableSpdy ) == -1 )
        {
            LS_ERROR("[SSL] SPDY/HTTP2 cannot be enabled [tried to set to %d].",
                        pConfig->m_iEnableSpdy);
            if (pNewContext)
                delete pNewContext;
            return NULL;
        }
    }
    pContext->setCipherList( pConfig->m_sCiphers.c_str() );

    if (pConfig->m_iEnableStapling)
    {
        pContext->configStapling(pConfig);
    }

#ifdef SSL_ASYNC_PK
    ssl_ctx_enable_apk(pContext->m_pCtx);
#endif

#ifdef SSLCERTCOMP
    SslCertComp::enableCertComp(pContext->m_pCtx);
#endif

#ifdef _ENTERPRISE_
    if (pConfig->m_iClientVerify)
    {
        pContext->setClientVerify(pConfig->m_iClientVerify,
                                  pConfig->m_iVerifyDepth);
    }
#endif

    return pContext;
}


int SslContext::configStapling(SslContextConfig *pConfig)
{
    SslOcspStapling *pSslOcspStapling = getStapling();
    if (pSslOcspStapling != NULL)
        return 0;
    pSslOcspStapling = new SslOcspStapling;
    if (!pSslOcspStapling)
    {
        LS_ERROR("[SSL] OCSP Stapling can't be enabled: Insufficient memory\n");
        return -1;
    }
    setStapling(pSslOcspStapling) ;
    pSslOcspStapling->setCertFile(pConfig->m_sCertFile[0].c_str());
    if (pConfig->m_sCAFile.c_str())
    {
        if (!pConfig->m_sCaChainFile.c_str())
            setCertificateChainFile(pConfig->m_sCAFile.c_str());
        pSslOcspStapling->setCAFile(pConfig->m_sCAFile.c_str());
    }
    else if (pConfig->m_sCaChainFile.c_str())
        pSslOcspStapling->setCAFile(pConfig->m_sCaChainFile.c_str());

    if (pConfig->m_iOcspMaxAge > 10)
    {
        if (pConfig->m_iOcspMaxAge > 3600 * 24 * 4)
            pConfig->m_iOcspMaxAge = 3600 * 24 * 4;
        pSslOcspStapling->setRespMaxAge(pConfig->m_iOcspMaxAge);
    }

    if (pConfig->m_sOcspResponder.c_str())
        pSslOcspStapling->setOcspResponder(pConfig->m_sOcspResponder.c_str());

    if (initStapling() == -1)
    {
        LS_ERROR("[SSL] OCSP Stapling can't be enabled [%s].",
                    getStaplingErrMsg());
        delete pSslOcspStapling;
        setStapling(NULL) ;

        return -1;
    }
    m_iEnableOcsp = 1;
    return 0;
}


void SslContext::setUseStrongDH(int use)
{
    SslUtil::setUseStrongDH(use);
}


int SslContext::setSessionIdContext(unsigned char *sid, unsigned int len)
{
    return SSL_CTX_set_session_id_context(m_pCtx , sid, len);
}


long SslContext::setOptions(long options)
{
    return SSL_CTX_set_options(m_pCtx, options);
}


long SslContext::getOptions()
{
    return SSL_CTX_get_options(m_pCtx);
}


int SslContext::seedRand(int len)
{
    static int fd = open("/dev/urandom", O_RDONLY | O_NONBLOCK);
    char achBuf[2048];
    int ret;
    if (fd >= 0)
    {
        int toRead;
        do
        {
            toRead = sizeof(achBuf);
            if (len < toRead)
                toRead = len;
            ret = read(fd, achBuf, toRead);
            if (ret > 0)
            {
                RAND_seed(achBuf, ret);
                len -= ret;
            }
        }
        while ((len > 0) && (ret == toRead));
        fcntl(fd, F_SETFD, FD_CLOEXEC);
        //close( fd );
    }
    else
    {
#ifdef DEVRANDOM_EGD
        /* Use an EGD socket to read entropy from an EGD or PRNGD entropy
         * collecting daemon. */
        static const char *egdsockets[] = { "/var/run/egd-pool", "/dev/egd-pool", "/etc/egd-pool" };
        for (egdsocket = egdsockets; *egdsocket && n < len; egdsocket++)
        {

            ret = RAND_egd_bytes(*egdsocket, len);
            if (ret > 0)
                len -= ret;
        }
#endif
    }
    if (len == 0)
        return 0;
    if (len > (int)sizeof(achBuf))
        len = (int)sizeof(achBuf);
    gettimeofday((timeval *)achBuf, NULL);
    ret = sizeof(timeval);
    *(pid_t *)(achBuf + ret) = getpid();
    ret += sizeof(pid_t);
    *(uid_t *)(achBuf + ret) = getuid();
    ret += sizeof(uid_t);
    if (len > ret)
        memmove(achBuf + ret, achBuf + sizeof(achBuf) - len + ret, len - ret);
    RAND_seed(achBuf, len);
    return 0;
}


void SslContext::setProtocol(int method)
{
    if (method == m_iMethod)
        return;
    if ((method & SSL_ALL) == 0)
        return;
    m_iMethod = method;
    updateProtocol(method);
}


void SslContext::updateProtocol(int method)
{
    SslUtil::updateProtocol(m_pCtx, method);
}


int SslContext::initDH(const char *pFile)
{
    return SslUtil::initDH(m_pCtx, pFile, m_iKeyLen);
}


int SslContext::initECDH()
{
    return SslUtil::initECDH(m_pCtx);
}


static int s_ctx_ex_index = -1;

void SslContext::linkSslContext()
{
    if (s_ctx_ex_index == -1)
    {
        s_ctx_ex_index = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    }
    SSL_CTX_set_ex_data(m_pCtx, s_ctx_ex_index, this);
}


SslContext *SslContext::getSslContext(SSL_CTX *ctx)
{
    return (SslContext *)SSL_CTX_get_ex_data(ctx, s_ctx_ex_index);
}


int SslContext::init(int iMethod)
{
    if (m_pCtx != NULL)
        return 0;
    SSL_METHOD *meth;
    if (initSSL())
        return -1;
    
    m_iMethod = iMethod;
    m_iEnableSpdy = 0;
    m_iEnableOcsp = 0;
    meth = (SSL_METHOD *)SSLv23_method();
    m_pCtx = SSL_CTX_new(meth);
    if (m_pCtx)
    {
        LS_DBG_L("[SslCertComp] Create SSL_CTX: %p\n", m_pCtx);
        SslUtil::initCtx(m_pCtx, iMethod, m_iRenegProtect);
        //initDH( NULL );
        //initECDH();
        linkSslContext();
        return 0;
    }
    else
    {
        //FIXME: log ssl error
        return -1;
    }
}

SslContext::SslContext(int iMethod)
    : m_pCtx(NULL)
    , m_iMethod(iMethod)
    , m_iRenegProtect(1)
    , m_iEnableSpdy(0)
    , m_iEnableOcsp(0)
    , m_iKeyLen(1024)
    , m_tmLastAccess(0)
    , m_pStapling(NULL)
{
    memset(&m_stKey, 0, sizeof(m_stKey));
    memset(&m_stCert, 0, sizeof(m_stCert));
}


SslContext::~SslContext()
{
    release();
}


void SslContext::release()
{
#ifdef SSLCERTCOMP
    SslCertComp::disableCertCompDecomp(m_pCtx);
#endif        
    if (m_pCtx != NULL && m_pCtx != SSL_CTX_PENDING)
    {
        SSL_CTX *pCtx = m_pCtx;
        m_pCtx = NULL;
        SSL_CTX_free(pCtx);
    }
    if (m_pStapling)
    {
        delete m_pStapling;
        m_pStapling = NULL;
    }
}


SSL *SslContext::newSSL()
{
    init(m_iMethod);
    return SSL_new(m_pCtx);
}


static int isFileChanged(const char *pFile, const struct stat &stOld)
{
    struct stat st;
    if (::stat(pFile, &st) == -1)
        return false;
    return ((st.st_size != stOld.st_size) ||
            (st.st_ino != stOld.st_ino) ||
            (st.st_mtime != stOld.st_mtime));
}


int SslContext::isKeyFileChanged(const char *pKeyFile) const
{
    return isFileChanged(pKeyFile, m_stKey);
}


int SslContext::isCertFileChanged(const char *pCertFile) const
{
    return isFileChanged(pCertFile, m_stCert);
}


int SslContext::setKeyCertificateFile(const char *pFile, int iType,
                                       int chained)
{
    return setKeyCertificateFile(pFile, iType, pFile, iType, chained);
}


int SslContext::setKeyCertificateFile(const char *pKeyFile, int iKeyType,
                                       const char *pCertFile, int iCertType,
                                       int chained)
{
    if (!setCertificateFile(pCertFile, iCertType, chained))
        return false;
    if (!setPrivateKeyFile(pKeyFile, iKeyType))
        return false;
    return  SSL_CTX_check_private_key(m_pCtx) == 1;
}


static const int max_certs = 4;
static const int max_path_len = 512;
int SslContext::setMultiKeyCertFile(const char *pKeyFile, int iKeyType,
                                    const char *pCertFile, int iCertType,
                                    int chained)
{
    int i, iCertLen, iKeyLen, iLoaded = 0;
    char achCert[max_path_len], achKey[max_path_len];
    const char *apExt[max_certs] = {"", ".rsa", ".dsa", ".ecc"};
    char *pCertCur, *pKeyCur;

    iCertLen = snprintf(achCert, max_path_len, "%s", pCertFile);
    pCertCur = achCert + iCertLen;
    iKeyLen = snprintf(achKey, max_path_len, "%s", pKeyFile);
    pKeyCur = achKey + iKeyLen;
    for (i = 0; i < max_certs; ++i)
    {
        snprintf(pCertCur, max_path_len - iCertLen, "%s", apExt[i]);
        snprintf(pKeyCur, max_path_len - iKeyLen, "%s", apExt[i]);
        if ((access(achCert, F_OK) == 0) && (access(achKey, F_OK) == 0))
        {
            if (setKeyCertificateFile(achKey, iKeyType, achCert, iCertType,
                                      chained) == false)
            {
                LS_ERROR("Failed to load key file %s and cert file %s",
                         achKey, achCert);
                return false;
            }
            iLoaded = 1;
        }
    }
    return (iLoaded == 1);
}


int SslContext::setCertificateFile(const char *pFile, int type,
                                    int chained)
{
    if (!pFile)
        return false;
    ::stat(pFile, &m_stCert);
    if (init(m_iMethod))
        return false;
    if (chained)
        return SSL_CTX_use_certificate_chain_file(m_pCtx, pFile) == 1;
    else
    {
        int ret = SslUtil::loadCertFile(m_pCtx, pFile, type);
        if (ret == -1)
            return SSL_CTX_use_certificate_file(m_pCtx, pFile,
                                                SslUtil::translateType(type));
    }
    return 1;
}


int SslContext::setCertificateChainFile(const char *pFile)
{
    BIO *bio;
    if ((bio = BIO_new_file(pFile, "r")) == NULL)
        return -1;
    int ret =  SslUtil::setCertificateChain(m_pCtx, bio);
    BIO_free(bio);
    return ret;
}

int SslContext::setCALocation(const char *pCAFile, const char *pCAPath,
                               int cv)
{
    if (init(m_iMethod))
        return false;
    return SslUtil::loadCA(m_pCtx, pCAFile, pCAPath, cv);
}

int SslContext::setPrivateKeyFile(const char *pFile, int type)
{
    int ret;
    if (!pFile)
        return false;
    if (::stat(pFile, &m_stKey) == -1)
        return false;
    if (init(m_iMethod))
        return false;
//     if (loadPrivateKeyFile(pFile, type) == -1)
    if ((ret = SslUtil::loadPrivateKeyFile(m_pCtx, pFile, type)) <= 1)
    {
        return SSL_CTX_use_PrivateKey_file(m_pCtx, pFile,
                                           SslUtil::translateType(type)) == 1;
    }
    m_iKeyLen = ret;
    return 1;
}

bool SslContext::checkPrivateKey()
{
    if (m_pCtx)
        return SSL_CTX_check_private_key(m_pCtx) == 1;
    return false;
}

int SslContext::setCipherList(const char *pList)
{
    if (!m_pCtx)
        return false;
    return SslUtil::setCipherList(m_pCtx, pList);
}



/*
SL_CTX_set_verify(ctx, nVerify,  ssl_callback_SSLVerify);
    SSL_CTX_sess_set_new_cb(ctx,      ssl_callback_NewSessionCacheEntry);
    SSL_CTX_sess_set_get_cb(ctx,      ssl_callback_GetSessionCacheEntry);
    SSL_CTX_sess_set_remove_cb(ctx,   ssl_callback_DelSessionCacheEntry);
    SSL_CTX_set_tmp_rsa_callback(ctx, ssl_callback_TmpRSA);
    SSL_CTX_set_tmp_dh_callback(ctx,  ssl_callback_TmpDH);
    SSL_CTX_set_info_callback(ctx,    ssl_callback_LogTracingState);
*/

int SslContext::initSSL()
{
    SSL_load_error_strings();
    SSL_library_init();
#ifndef SSL_OP_NO_COMPRESSION
    /* workaround for OpenSSL 0.9.8 */
    sk_SSL_COMP_zero(SSL_COMP_get_compression_methods());
#endif

    SslConnection::initConnIdx();

    return seedRand(512);
}

/*
static RSA *load_key(const char *file, char *pass, int isPrivate )
{
    BIO * bio_err = BIO_new_fp( stderr, BIO_NOCLOSE );
    BIO *bp = NULL;
    EVP_PKEY *pKey = NULL;
    RSA *pRSA = NULL;

    bp=BIO_new(BIO_s_file());
    if (bp == NULL)
    {
        return NULL;
    }
    if (BIO_read_filename(bp,file) <= 0)
    {
        BIO_free( bp );
        return NULL;
    }
    if ( !isPrivate )
        pKey = PEM_read_bio_PUBKEY( bp, NULL, NULL, pass);
    else
        pKey = PEM_read_bio_PrivateKey( bp, NULL, NULL, pass );
    if ( !pKey )
    {
        ERR_print_errors( bio_err );
    }
    else
    {
        pRSA = EVP_PKEY_get1_RSA( pKey );
        EVP_PKEY_free( pKey );
    }
    if (bp != NULL)
        BIO_free(bp);
    if ( bio_err )
        BIO_free( bio_err );
    return(pRSA);
}
*/

static RSA *load_key(const unsigned char *key, int keyLen, char *pass,
                     int isPrivate)
{
    BIO *bio_err = BIO_new_fp(stderr, BIO_NOCLOSE);
    BIO *bp = NULL;
    EVP_PKEY *pKey = NULL;
    RSA *pRSA = NULL;

    bp = BIO_new_mem_buf((void *)key, keyLen);
    if (bp == NULL)
        return NULL;
    if (!isPrivate)
        pKey = PEM_read_bio_PUBKEY(bp, NULL, NULL, pass);
    else
        pKey = PEM_read_bio_PrivateKey(bp, NULL, NULL, pass);
    if (!pKey)
        ERR_print_errors(bio_err);
    else
    {
        pRSA = EVP_PKEY_get1_RSA(pKey);
        EVP_PKEY_free(pKey);
    }
    if (bp != NULL)
        BIO_free(bp);
    if (bio_err)
        BIO_free(bio_err);
    return (pRSA);
}


int  SslContext::publickey_encrypt(const unsigned char *pPubKey,
                                   int keylen,
                                   const char *content, int len, char *encrypted, int bufLen)
{
    int ret;
    initSSL();
    RSA *pRSA = load_key(pPubKey, keylen, NULL, 0);
    if (pRSA)
    {
        if (bufLen < (int)RSA_size(pRSA))
            return -1;
        ret = RSA_public_encrypt(len, (unsigned char *)content,
                                 (unsigned char *)encrypted, pRSA, RSA_PKCS1_OAEP_PADDING);
        RSA_free(pRSA);
        return ret;
    }
    else
        return -1;

}

int  SslContext::publickey_decrypt(const unsigned char *pPubKey,
                                   int keylen,
                                   const char *encrypted, int len, char *decrypted, int bufLen)
{
    int ret;
    initSSL();
    RSA *pRSA = load_key(pPubKey, keylen, NULL, 0);
    if (pRSA)
    {
        if (bufLen < (int)RSA_size(pRSA))
            return -1;
        ret = RSA_public_decrypt(len, (unsigned char *)encrypted,
                                 (unsigned char *)decrypted, pRSA, RSA_PKCS1_PADDING);
        RSA_free(pRSA);
        return ret;
    }
    else
        return -1;
}

/*
int  SslContext::privatekey_encrypt( const char * pPrivateKeyFile, const char * content,
                    int len, char * encrypted, int bufLen )
{
    int ret;
    initSSL();
    RSA * pRSA = load_key( pPrivateKeyFile, NULL, 1 );
    if ( pRSA )
    {
        if ( bufLen < RSA_size( pRSA) )
            return -1;
        ret = RSA_private_encrypt(len, (unsigned char *)content,
            (unsigned char *)encrypted, pRSA, RSA_PKCS1_PADDING );
        RSA_free( pRSA );
        return ret;
    }
    else
        return -1;
}
int  SslContext::privatekey_decrypt( const char * pPrivateKeyFile, const char * encrypted,
                    int len, char * decrypted, int bufLen )
{
    int ret;
    initSSL();
    RSA * pRSA = load_key( pPrivateKeyFile, NULL, 1 );
    if ( pRSA )
    {
        if ( bufLen < RSA_size( pRSA) )
            return -1;
        ret = RSA_private_decrypt(len, (unsigned char *)encrypted,
            (unsigned char *)decrypted, pRSA, RSA_PKCS1_OAEP_PADDING );
        RSA_free( pRSA );
        return ret;
    }
    else
        return -1;
}
*/

/*
    ASSERT (options->ca_file || options->ca_path);
    if (!SSL_CTX_load_verify_locations (ctx, options->ca_file, options->ca_path))
    msg (M_SSLERR, "Cannot load CA certificate file %s path %s (SSL_CTX_load_verify_locations)", options->ca_file, options->ca_path);

    // * Set a store for certs (CA & CRL) with a lookup on the "capath" hash directory * /
    if (options->ca_path) {
        X509_STORE *store = SSL_CTX_get_cert_store(ctx);

        if (store)
        {
            X509_LOOKUP *lookup = X509_STORE_add_lookup(store, X509_LOOKUP_hash_dir());
            if (!X509_LOOKUP_add_dir(lookup, options->ca_path, X509_FILETYPE_PEM))
                X509_LOOKUP_add_dir(lookup, NULL, X509_FILETYPE_DEFAULT);
            else
                msg(M_WARN, "WARNING: experimental option --capath %s", options->ca_path);
#if OPENSSL_VERSION_NUMBER >= 0x00907000L
            X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
#else
#warn This version of OpenSSL cannot handle CRL files in capath
            msg(M_WARN, "WARNING: this version of OpenSSL cannot handle CRL files in capath");
#endif
        }
        else
            msg(M_SSLERR, "Cannot get certificate store (SSL_CTX_get_cert_store)");
    }
*/


static SslSniLookupCb s_sniLookup = NULL;

void SslContext::setSniLookupCb(SslSniLookupCb pCb)
{   s_sniLookup = pCb;      }


static int verifyProtocol(SSL *pSSL, long newCtxOptions)
{
    int ver = SSL_version(pSSL);
    switch(ver)
    {
    case SSL3_VERSION:
        if (newCtxOptions & SSL_OP_NO_SSLv3)
            return SslUtil::CERTCB_RET_ERR;
        break;
    case TLS1_VERSION:
        if (newCtxOptions & SSL_OP_NO_TLSv1)
            return SslUtil::CERTCB_RET_ERR;
        break;
    case TLS1_1_VERSION:
        if (newCtxOptions & SSL_OP_NO_TLSv1_1)
            return SslUtil::CERTCB_RET_ERR;
        break;
    }
    return SslUtil::CERTCB_RET_OK;
}


int newClientSessionCb(SSL * ssl, SSL_SESSION * session)
{
    const char *pHostName = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    if (NULL == pHostName)
        pHostName = "_";
    SslConnection *c = SslConnection::get(ssl);
    return c->cacheClientSession(session, pHostName, strlen(pHostName));
}


void SslContext::enableClientSessionReuse()
{
    init(m_iMethod);
    SSL_CTX_set_session_cache_mode(m_pCtx, SSL_SESS_CACHE_CLIENT);
    SSL_CTX_sess_set_new_cb(m_pCtx, newClientSessionCb);
}


int SslContext::servername_cb(SSL *pSSL, void *arg)
{
    SSL_CTX *pNewCtx;
    SslContext *pCtx = NULL;
    long newCtxOptions;
    const char *servername = SSL_get_servername(pSSL,
                             TLSEXT_NAMETYPE_host_name);
    if (!servername || !*servername)
        return SslUtil::CERTCB_RET_OK;
    if (s_sniLookup)
        pCtx = (*s_sniLookup)(arg, servername);
    if (!pCtx)
    {

        LS_DBG_H("SslContext::servername_cb() no ctx found.");
        return SslUtil::CERTCB_RET_OK;
    }
    pNewCtx = pCtx->get();
    if (pNewCtx == SSL_get_SSL_CTX(pSSL))
        return SslUtil::CERTCB_RET_OK;
#ifdef OPENSSL_IS_BORINGSSL
    // Check OCSP again when the context needs to be changed.
    pCtx->initOCSP();
#endif
    SSL_set_SSL_CTX(pSSL, pNewCtx);
    SSL_set_verify(pSSL, SSL_CTX_get_verify_mode(pNewCtx), NULL);
    SSL_set_verify_depth(pSSL, SSL_CTX_get_verify_depth(pNewCtx));

    newCtxOptions = SSL_CTX_get_options(pNewCtx);
    SSL_clear_options(pSSL,
                      SSL_get_options(pSSL) & ~newCtxOptions);
    SSL_set_options(pSSL, newCtxOptions);
    return verifyProtocol(pSSL, newCtxOptions);
}


int SslContext::initSNI(void *param)
{
    assert(s_sniLookup != NULL);
    SSL_CTX_set_cert_cb(m_pCtx, servername_cb, param);
    return 0;
}



#ifdef _ENTERPRISE_

/*!
    \fn SslContext::setClientVerify( int mode, int depth)
 */
void SslContext::setClientVerify(int mode, int depth)
{
    int req;
    switch (mode)
    {
    case 0:     //none
        req = SSL_VERIFY_NONE;
        break;
    case 1:     //optional
    case 3:     //no_ca
        req = SSL_VERIFY_PEER;
        break;
    case 2:     //required
    default:
        req = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT |
              SSL_VERIFY_CLIENT_ONCE;
    }
    SSL_CTX_set_verify(m_pCtx, req, NULL);
    SSL_CTX_set_verify_depth(m_pCtx, depth);
}


/*!
    \fn SslContext::addCRL( const char * pCRLFile, const char * pCRLPath)
 */
int SslContext::addCRL(const char *pCRLFile, const char *pCRLPath)
{
    X509_STORE *store = SSL_CTX_get_cert_store(m_pCtx);
    X509_LOOKUP *lookup = X509_STORE_add_lookup(store, X509_LOOKUP_hash_dir());
    if (pCRLFile)
    {
        if (!X509_load_crl_file(lookup, pCRLFile, X509_FILETYPE_PEM))
            return -1;
    }
    if (pCRLPath)
    {
        if (!X509_LOOKUP_add_dir(lookup, pCRLPath, X509_FILETYPE_PEM))
            return -1;
    }
    X509_STORE_set_flags(store,
                         X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
    return 0;
}
#endif


/* This will neeed to be updated as the ID versions change.  Eventually
 * it will become simply "h3"
 */
#ifndef H3_ALPN
#define H3_ALPN "\x05h3-22"
#endif
#define H3_ALSZ (sizeof(H3_ALPN) - 1)

/**
 * We support h2-16, but if set to this value, firefox will not choose h2-16, so we have to use h2-14.
 */
static const char *NEXT_PROTO_STRING[16] =
{
    "\x08http/1.1",
    "\x06spdy/2\x08http/1.1",
    "\x08spdy/3.1\x06spdy/3\x08http/1.1",
    "\x08spdy/3.1\x06spdy/3\x06spdy/2\x08http/1.1",
    "\x02h2\x08http/1.1",
    "\x02h2\x06spdy/2\x08http/1.1",
    "\x02h2\x08spdy/3.1\x06spdy/3\x08http/1.1",
    "\x02h2\x08spdy/3.1\x06spdy/3\x06spdy/2\x08http/1.1",
    H3_ALPN "\x08http/1.1",
    H3_ALPN "\x06spdy/2\x08http/1.1",
    H3_ALPN "\x08spdy/3.1\x06spdy/3\x08http/1.1",
    H3_ALPN "\x08spdy/3.1\x06spdy/3\x06spdy/2\x08http/1.1",
    H3_ALPN "\x02h2\x08http/1.1",
    H3_ALPN "\x02h2\x06spdy/2\x08http/1.1",
    H3_ALPN "\x02h2\x08spdy/3.1\x06spdy/3\x08http/1.1",
    H3_ALPN "\x02h2\x08spdy/3.1\x06spdy/3\x06spdy/2\x08http/1.1",
};

static unsigned int NEXT_PROTO_STRING_LEN[16] =
{
    9, 16, 25, 32, 12, 19, 28, 35,
    9+H3_ALSZ, 16+H3_ALSZ, 25+H3_ALSZ, 32+H3_ALSZ, 12+H3_ALSZ, 19+H3_ALSZ, 28+H3_ALSZ, 35+H3_ALSZ,
};

//static const char NEXT_PROTO_STRING[] = "\x06spdy/2\x08http/1.1\x08http/1.0";
#if 0
static int SslConnection_ssl_npn_advertised_cb(SSL *pSSL,
        const unsigned char **out,
        unsigned int *outlen, void *arg)
{
    SslContext *pCtx = (SslContext *)arg;
    *out = (const unsigned char *)NEXT_PROTO_STRING[ pCtx->getEnableSpdy()];
    *outlen = NEXT_PROTO_STRING_LEN[ pCtx->getEnableSpdy() ];
    return SSL_TLSEXT_ERR_OK;
}
#endif


#ifdef TLSEXT_TYPE_application_layer_protocol_negotiation
static int SSLConntext_alpn_select_cb(SSL *pSSL, const unsigned char **out,
                                      unsigned char *outlen, const unsigned char *in,
                                      unsigned int inlen, void *arg)
{
    SslContext *pCtx = (SslContext *)arg;
    SslConnection *pConn = SslConnection::get(pSSL);
    if (pConn && pConn->getFlag(SslConnection::F_DISABLE_HTTP2))
        return SSL_TLSEXT_ERR_NOACK;
    unsigned char alpn_idx = pCtx->getEnableSpdy();
    if (pConn)
        alpn_idx &= ~8; // No HTTP/3 on TCP connection
    if (SSL_select_next_proto((unsigned char **) out, outlen,
                              (const unsigned char *)NEXT_PROTO_STRING[ alpn_idx ],
                              NEXT_PROTO_STRING_LEN[ alpn_idx ],
                              in, inlen)
        != OPENSSL_NPN_NEGOTIATED)
        return SSL_TLSEXT_ERR_NOACK;
    return SSL_TLSEXT_ERR_OK;
}
#endif


void SslContext::setAlpnCb(SSL_CTX *ctx, void *arg)
{
#ifdef TLSEXT_TYPE_application_layer_protocol_negotiation
    SSL_CTX_set_alpn_select_cb(ctx, SSLConntext_alpn_select_cb, arg);
#endif
}


int SslContext::enableSpdy(int level)
{
    m_iEnableSpdy = (level & 15);
    if (m_iEnableSpdy == 0)
        return 0;
#ifdef TLSEXT_TYPE_application_layer_protocol_negotiation
    SSL_CTX_set_alpn_select_cb(m_pCtx, SSLConntext_alpn_select_cb, this);
// #endif
// #ifdef TLSEXT_TYPE_next_proto_neg
//     SSL_CTX_set_next_protos_advertised_cb(m_pCtx,
//                                           SslConnection_ssl_npn_advertised_cb, this);
#else
#error "Openssl version is too low (openssl 1.0.2 or higher is required)!!!"
#endif
    return 0;
}

#ifndef OPENSSL_IS_BORINGSSL
static int sslCertificateStatus_cb(SSL *ssl, void *data)
{
    SslOcspStapling *pStapling = (SslOcspStapling *)data;
    return pStapling->callback(ssl);
}
#endif

int SslContext::initOCSP()
{
#ifdef OPENSSL_IS_BORINGSSL
    if (getStapling() == NULL) {
        return 0;
    }
    return getStapling()->update();
#else
    return 0;
#endif
}

int SslContext::initStapling()
{
    if (m_pStapling->init(this) == -1)
        return -1;
#ifndef OPENSSL_IS_BORINGSSL
    SSL_CTX_set_tlsext_status_cb(m_pCtx, sslCertificateStatus_cb);
    SSL_CTX_set_tlsext_status_arg(m_pCtx, m_pStapling);
#endif
    return 0;
}


void SslContext::updateOcsp()
{
    if (m_pStapling && m_iEnableOcsp)
        m_pStapling->update();
}


int  SslContext::enableShmSessionCache()
{
    return SslUtil::enableShmSessionCache(m_pCtx);
}


int SslContext::enableSessionTickets()
{
    return SslUtil::enableSessionTickets(m_pCtx);
}


void SslContext::disableSessionTickets()
{
    SslUtil::disableSessionTickets(m_pCtx);
}

