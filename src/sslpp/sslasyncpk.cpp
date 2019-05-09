/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#include <sslpp/sslasyncpk.h>

#ifdef SSL_ASYNC_PK
#include <log4cxx/logger.h>
#include <lsr/ls_internal.h>
#include <lsr/ls_pool.h>
#include <lsr/ls_lfqueue.h>
#include <assert.h>
#if __cplusplus <= 199711L && !defined(static_assert)
#define static_assert(a, b) _Static_assert(a, b)
#endif
#include <openssl/bio.h>
#include <openssl/err.h>

#include <sslpp/sslcontext.h>

#include <thread/workcrew.h>

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

#define DEBUGGING

#ifdef DEBUGGING

#if TEST_PGM
#define DEBUG_MESSAGE(...)     printf(__VA_ARGS__);
#define ERROR_MESSAGE(...)     fprintf(stderr, __VA_ARGS__);
#else
#define DEBUG_MESSAGE(...)     LS_DBG_L(__VA_ARGS__)
#define ERROR_MESSAGE(...)     LS_ERROR(__VA_ARGS__)
#endif

#else

#define DEBUG_MESSAGE(...)
#define ERROR_MESSAGE(...)

#endif

SslApkWorker *SslApkWorker::s_instance = NULL;


SslApkWorker::SslApkWorker()
    : m_pFinishedQueue(NULL)
    , m_crew(NULL)
{ }


SslApkWorker::~SslApkWorker()
{
    if (m_crew)
        delete m_crew;
    if (m_pFinishedQueue)
        ls_lfqueue_delete(m_pFinishedQueue);
}


int SslApkWorker::prepare()
{
    if (s_instance)
        return 0;
    s_instance = new SslApkWorker();
    if (s_instance->init() == -1)
    {
        delete s_instance;
        s_instance = NULL;
        return -1;
    }
    return 0;
}


int SslApkWorker::init()
{
    if (!m_pFinishedQueue)
    {
        m_pFinishedQueue = ls_lfqueue_new();
        if (!m_pFinishedQueue)
        {
            LS_NOTICE("[SSL] [ApkWorrker] init Unable to create Finished queue\n");
            return -1;
        }
    }
    if (!m_crew)
    {
        LS_DBG_L("[SSL] [ApkWorrker] init create WorkCrew!\n");
        m_crew = new WorkCrew(this);
        if (!m_crew)
        {
            LS_NOTICE("[SSL] [ApkWorrker] init create WorkCrew memory error!\n");
            return -1;
        }

        m_crew->dropPriorityBy(1);

        //m_crew->blockSig(SIGCHLD);
        //m_crew->startProcessing();
    }
    return 0;
}


int SslApkWorker::addJob(ls_lfnodei_t  *data)
{
    int rc;
    
    LS_DBG_L("[SSL] [ApkWorker] addJob: %p!\n", data);
    rc = m_crew->addJob(data);
    return rc;
}


int SslApkWorker::startProcessor(int workers)
{
    return m_crew->startJobProcessor(workers, m_pFinishedQueue,
                                     SslAsyncPk::privateKeyThread);
}


int SslApkWorker::start(Multiplexer *pMplx, int workers)
{
    if (prepare() != 0)
        return -1;
    if (s_instance->initNotifier(pMplx) == -1)
        return -1;
    return s_instance->startProcessor(workers);
}


int SslApkWorker::onNotified(int count)
{
    SslAsyncPk *event;
    LS_DBG_L("[SSL] [ApkWorker] onNotified: %d!\n", count);
    while (!ls_lfqueue_empty(m_pFinishedQueue))
    {
        event = (SslAsyncPk *)ls_lfqueue_get(m_pFinishedQueue);
        void *link = event->m_resume_param;
        event->release();
        if (link)
        {
            LS_DBG_L("[SSL] [ApkWorker] onNotified event: %p, resume!\n",
                     event);
            event->on_resume_cb(link);
        }
        else
        {
            LS_DBG_L("[SSL] [ApkWorker] onNotified event: %p, canceled!\n",
                     event);

        }
    }
    return LS_OK;
    
}

enum
{
    APK_NOT_INUSE,
    APK_REQUEST,
    APK_SIGNED,
    APK_FAILED,
};


static int  s_iSslAsyncPk_index = -1;

static void freeAsyncPkData(void *parent, void *ptr, CRYPTO_EX_DATA *ad, int idx,
                        long argl, void *argp)
{
    DEBUG_MESSAGE("[SSL] [AsyncPk] freeing callback\n");
    assert(idx == s_iSslAsyncPk_index);
    if (!ptr)
        return;
    if (((SslAsyncPk *)ptr)->m_resume_param)
    {
        ls_atomic_setptr(&((SslAsyncPk *)ptr)->m_resume_param, NULL);
        LS_DBG_L("[SSL] [AsyncPk: %p] free exdata, SslAsyncPk canceled!\n",
                    ptr);
    }
    DEBUG_MESSAGE("[SSL] [AsyncPk] freeing data %p\n", ptr);
    ((SslAsyncPk *)ptr)->release();
}


SslAsyncPk::SslAsyncPk()
    : m_resume_param(NULL)
    , on_resume_cb(NULL)
    , m_refCounter(1)
    , m_state(APK_NOT_INUSE)
    , m_sig_alg(0)
    , m_in(NULL)
    , m_in_len(0)
    , m_sign(NULL)
    , m_sign_size(0)
{
}


SslAsyncPk::~SslAsyncPk()
{
}


void *async_pk_prepare(SSL *ssl, ssl_async_pk_resume_cb on_resume, void *param)
{
    if (s_iSslAsyncPk_index == -1)
        s_iSslAsyncPk_index = SSL_get_ex_new_index(0, NULL, NULL, NULL, freeAsyncPkData);
    SslAsyncPk *data = new SslAsyncPk();
    if (!data)
    {
        ERROR_MESSAGE("[SSL: %p] Error creating SslAsyncPk structure\n", ssl);
        SSL_set_private_key_method(ssl, NULL);
        return NULL;
    }
    SSL_set_ex_data(ssl, s_iSslAsyncPk_index, (void *)data);
    
    data->m_resume_param = param;
    data->on_resume_cb = on_resume;
    return data;
}


void SslAsyncPk::cancel(SSL *ssl)
{
    SslAsyncPk *asyncPk = (SslAsyncPk *)SSL_get_ex_data(ssl, s_iSslAsyncPk_index);
    if (!asyncPk)
    {
        DEBUG_MESSAGE("[SSL: %p] Attempt to cancel with no class assigned\n",
                      ssl);
        return;
    }
    LS_DBG_L("[SSL: %p] [AsyncPk: %p] cancel by application\n", ssl, asyncPk);
    ls_atomic_setptr(&asyncPk->m_resume_param, NULL);
}


void SslAsyncPk::releaseAsyncPk(SSL *ssl)
{
    SslAsyncPk *asyncPk = (SslAsyncPk *)SSL_get_ex_data(ssl, s_iSslAsyncPk_index);
    if (!asyncPk)
    {
        DEBUG_MESSAGE("[SSL: %p] [AsyncPk] Attempt to release with no data assigned\n",
                      ssl);
        return;
    }
    DEBUG_MESSAGE("[SSL: %p] [AsyncPk: %p] release by application\n", ssl, asyncPk);
    asyncPk->release();
    SSL_set_ex_data(ssl, s_iSslAsyncPk_index, NULL);
}


static int AsyncSign(EVP_MD_CTX *ctx, SslAsyncPk *data)
{
    EVP_PKEY_CTX *pctx = NULL;
    // Determine the hash.
    if (EVP_DigestSignInit(ctx, &pctx,
                           SSL_get_signature_algorithm_digest(data->m_sig_alg),
                           NULL, data->m_pkey) != 1)
    {
        ERROR_MESSAGE("[SSL] [AsyncPk: %p] privateKeyThread, Can't do DigestSignInit\n",
                      data);
        return APK_FAILED;
    }
    if (SSL_is_signature_algorithm_rsa_pss(data->m_sig_alg))
    {
        if (!EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PSS_PADDING) ||
            !EVP_PKEY_CTX_set_rsa_pss_saltlen(pctx, -1))
        {
            ERROR_MESSAGE("[SSL] [AsyncPk: %p] privateKeyThread, Bad additional "
                            "parameters\n", data);
            return APK_FAILED;
        }
    }
    if (EVP_DigestSign(ctx, (unsigned char *)data->m_sign,
                        &data->m_sign_size, data->m_in, data->m_in_len) != 1)
    {
        ERROR_MESSAGE("[SSL] [AsyncPk: %p] privateKeyThread, Error in EVP_DigestSign"
                        " for real\n", data);
        return APK_FAILED;
    }
    return APK_SIGNED;
}


void* SslAsyncPk::privateKeyThread(ls_lfnodei_t* item)
{
    SslAsyncPk *data = (SslAsyncPk *)item;
    if (data->isCanceled())
    {
        LS_DBG_L("[SSL] [AsyncPk: %p] privateKeyThread, SslAsyncPk canceled!\n",
                  data);
        return NULL; // No known connection for private key
    }
    if (!data->m_in || !data->m_sign)
    {
        ERROR_MESSAGE("[SSL] [AsyncPk: %p] privateKeyThread, no input data!\n",
                      data);
        return NULL; // No known connection for private key
    }
    DEBUG_MESSAGE("[SSL] [AsyncPk: %p] privateKeyThread sign item: "
                  "%p, PKEY: %p, using alg: %d\n",
                  data, data, data->m_pkey,
                  data->m_sig_alg);

    EVP_MD_CTX ctx;
    EVP_MD_CTX_init(&ctx);
    data->m_state = AsyncSign(&ctx, data);
    EVP_MD_CTX_cleanup(&ctx);

    DEBUG_MESSAGE("[SSL] [AsyncPk: %p] privateKeyThread sign done, ok: %d\n",
                  data, data->m_state);
    return NULL;
}


static ssl_private_key_result_t AsyncPrivateKeySign(
    SSL *ssl, uint8_t *out, size_t *out_len, size_t max_out,
    uint16_t signature_algorithm, const uint8_t *in, size_t in_len);
static ssl_private_key_result_t AsyncPrivateKeyComplete(
    SSL *ssl, uint8_t *out, size_t *out_len, size_t max_out);
static const SSL_PRIVATE_KEY_METHOD s_async_private_key_method = 
    { AsyncPrivateKeySign, NULL, AsyncPrivateKeyComplete };


static ssl_private_key_result_t AsyncPrivateKeySign(
    SSL* ssl, uint8_t* out, size_t* out_len, size_t max_out, 
    uint16_t signature_algorithm, const uint8_t* in, size_t in_len)
{
    SslAsyncPk *data = (SslAsyncPk *)SSL_get_ex_data(ssl, s_iSslAsyncPk_index);
    
    if (!data)
    {
        DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeySign method not defined\n", ssl);
        return ssl_private_key_failure;
    }
    if (data->isCanceled())
    {
        DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeySign canceled\n", ssl);
        return ssl_private_key_failure;
    }
    if (data->m_in)
    {
        ERROR_MESSAGE("[SSL: %p] AsyncPrivateKeySign called twice for same data,"
                      " wait\n", ssl);
        return ssl_private_key_retry;
    }

    data->m_pkey = SSL_get_privatekey(ssl);
    if (EVP_PKEY_id(data->m_pkey) !=
            SSL_get_signature_algorithm_key_type(signature_algorithm))
    {
        ERROR_MESSAGE("[SSL: %p] Bad key type!\n", ssl);
        return ssl_private_key_failure;
    }

    DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeySign, alg: %d in_len: %ld \n",
                  ssl, signature_algorithm, in_len);
    data->m_sig_alg = signature_algorithm;
    data->m_sign = (char *)malloc(in_len + max_out);
    if (!data->m_sign)
    {
        ERROR_MESSAGE("[SSL: %p] AsyncPrivateKeySign unable to allocate memory:"
                      " %ld bytes\n", ssl, in_len + max_out);
        return ssl_private_key_failure;
    }    
    data->m_in = (uint8_t *)data->m_sign + max_out;
    memcpy(data->m_in, in, in_len);
    data->m_in_len = in_len;
    data->m_sign_size = max_out;
    data->m_state = APK_REQUEST;
    ls_atomic_add_fetch(&data->m_refCounter, 1);

    DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeySign add job: %p, max_out: %ld, "
                  "in_len: %ld\n", ssl, data, max_out, in_len);
    if (SslApkWorker::getSslAsyncPkNotifier()->addJob(data) == LS_FAIL)
    {
        ERROR_MESSAGE("[SSL: %p] AsyncPrivateKeySign SSL add of job of sign "
                      "failed\n", ssl);
        free((void*)data->m_sign);
        data->m_sign = NULL;
        data->m_in = NULL;
        data->m_state = APK_NOT_INUSE;
        data->release();
        return ssl_private_key_failure;
    }

    DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeySign retry\n", ssl);

    return ssl_private_key_retry;
}


static ssl_private_key_result_t AsyncPrivateKeyComplete(SSL *ssl, 
    uint8_t *out, size_t *out_len, size_t max_out) 
{
    SslAsyncPk *data = (SslAsyncPk *)SSL_get_ex_data(ssl, s_iSslAsyncPk_index);

    ssl_private_key_result_t ret = ssl_private_key_failure;
    DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeyComplete\n", ssl);
    if (!data || data->isCanceled())
    {
        DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeySign canceled\n", ssl);
        return ret;
    }
    if (data->m_state == APK_REQUEST)
    {
        DEBUG_MESSAGE("[SSL: %p] has not been signed yet, wait\n", ssl);
        return ssl_private_key_retry;
    }
    if (data->m_state == APK_SIGNED)
    {
        if (max_out < data->m_sign_size)
        {
            ERROR_MESSAGE("[SSL: %p] AsyncPrivateKeyComplete, max_out %ld < %ld\n",
                          ssl, max_out, data->m_sign_size);
        }
        else
        {
            DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeyComplete ptr: %p %ld bytes",
                          ssl, data->m_sign, data->m_sign_size);
            memcpy(out, data->m_sign, data->m_sign_size);
            *out_len = data->m_sign_size;
            ret = ssl_private_key_success;
        }
    }
    else if (data->m_state == APK_FAILED)
    {
        DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeyComplete thread failed\n",
                      ssl);
    }
    else if (data->m_state == APK_NOT_INUSE)
    {
        DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeyComplete not using async sync\n",
                      ssl);
    }
    data->m_resume_param = NULL;
    return ret;
}


void SslAsyncPk::enableApk(SSL_CTX *ctx)
{
    DEBUG_MESSAGE("[SSL_CTX] newContext: %p\n", ctx);
    if (SslApkWorker::getSslAsyncPkNotifier())
    {
        SSL_CTX_set_private_key_method(ctx, &s_async_private_key_method);
        DEBUG_MESSAGE("[SSL_CTX] private_key_method enabled\n");
    }
    else
        DEBUG_MESSAGE("[SSL_CTX] private_key_method not enabled\n");
}

    
void SslAsyncPk::release()
{
    DEBUG_MESSAGE("[SSL] [AsyncPk: %p] release, ref: %d\n", this, m_refCounter);
    if (ls_atomic_add_fetch(&m_refCounter, -1) > 0)
        return;
    DEBUG_MESSAGE("[SSL] [AsyncPk] no reference now, release\n");
    m_sign_size = 0;
    if (m_sign)
        free(m_sign);
    delete this; 
    DEBUG_MESSAGE("[SSL] [AsyncPk] release done\n");
}

#endif // SSL_ASYNC_PK
