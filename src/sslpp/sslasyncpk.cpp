/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#include <sslpp/sslasyncpk.h>

#ifdef SSL_ASYNC_PK

#include <log4cxx/logger.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

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

struct Offloader * s_offloader = NULL;

static int  s_ssl_apk_index = -1;
void ssl_apk_offload_release(ssl_apk_offload_t *t);

static void ssl_free_apk_data(void *parent, void *ptr, CRYPTO_EX_DATA *ad, int idx,
                        long argl, void *argp)
{
    DEBUG_MESSAGE("[SSL] [APK] freeing callback\n");
    assert(idx == s_ssl_apk_index);
    if (!ptr)
        return;
    if (!((ls_offload_t *)ptr)->is_canceled)
    {
        ((ls_offload_t *)ptr)->is_canceled = 1;
        LS_DBG_L("[SSL] [APK] free exdata %p, mark canceled.\n", ptr);
    }
    DEBUG_MESSAGE("[SSL] [APK] freeing data %p\n", ptr);
    ssl_apk_offload_release((ssl_apk_offload_t *)ptr);
}


static ssl_private_key_result_t AsyncPrivateKeySign(
    SSL *ssl, uint8_t *out, size_t *out_len, size_t max_out,
    uint16_t signature_algorithm, const uint8_t *in, size_t in_len);
static ssl_private_key_result_t AsyncPrivateKeyDecrypt(
    SSL *ssl, uint8_t *out, size_t *out_len, size_t max_out, const uint8_t *in,
    size_t in_len);
static ssl_private_key_result_t AsyncPrivateKeyComplete(
    SSL *ssl, uint8_t *out, size_t *out_len, size_t max_out);

static const SSL_PRIVATE_KEY_METHOD s_async_private_key_method = 
    { AsyncPrivateKeySign, AsyncPrivateKeyDecrypt, AsyncPrivateKeyComplete };


static ssl_private_key_result_t AsyncPrivateKeySign(
    SSL* ssl, uint8_t* out, size_t* out_len, size_t max_out, 
    uint16_t signature_algorithm, const uint8_t* in, size_t in_len)
{
    ssl_apk_offload_t *data = (ssl_apk_offload_t *)SSL_get_ex_data(ssl, s_ssl_apk_index);
    
    if (!data)
    {
        DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeySign method not defined\n", ssl);
        return ssl_private_key_failure;
    }
    if (data->m_header.is_canceled)
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

    DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeySign add job: %p, max_out: %ld, "
                  "in_len: %ld\n", ssl, data, max_out, in_len);
    if (offloader_enqueue(s_offloader, &data->m_header) == LS_FAIL)
    {
        ERROR_MESSAGE("[SSL: %p] AsyncPrivateKeySign SSL add of job of sign "
                      "failed\n", ssl);
        free((void*)data->m_sign);
        data->m_sign = NULL;
        data->m_in = NULL;
        ssl_apk_offload_release(data);
        return ssl_private_key_failure;
    }

    DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeySign retry\n", ssl);

    return ssl_private_key_retry;
}


static ssl_private_key_result_t AsyncPrivateKeyComplete(SSL *ssl, 
    uint8_t *out, size_t *out_len, size_t max_out) 
{
    ssl_apk_offload_t *data = (ssl_apk_offload_t *)SSL_get_ex_data(ssl, s_ssl_apk_index);

    SSL_set_private_key_method(ssl, NULL);
    ssl_private_key_result_t ret = ssl_private_key_failure;
    DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeyComplete\n", ssl);
    if (!data || data->m_header.is_canceled)
    {
        DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeySign canceled\n", ssl);
        return ret;
    }
    if (data->m_header.state < LS_OFFLOAD_FINISH_CB)
    {
        DEBUG_MESSAGE("[SSL: %p] Signing not finished yet, wait\n", ssl);
        return ssl_private_key_retry;
    }
    if (data->m_header.state == LS_OFFLOAD_FINISH_CB)
    {
        if (max_out < data->m_sign_size)
        {
            ERROR_MESSAGE("[SSL: %p] AsyncPrivateKeyComplete, max_out %ld < %ld\n",
                          ssl, max_out, data->m_sign_size);
        }
        else if (data->m_sign_size == 0)
        {
            DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeyComplete thread failed\n",
                          ssl);
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
    else if (data->m_header.state == LS_OFFLOAD_NOT_INUSE)
    {
        DEBUG_MESSAGE("[SSL: %p] AsyncPrivateKeyComplete not using async sync\n",
                      ssl);
    }
    data->m_header.is_canceled = 1;
    return ret;
}


static ssl_private_key_result_t AsyncPrivateKeyDecrypt(
    SSL *ssl, uint8_t *out, size_t *out_len, size_t max_out, const uint8_t *in,
    size_t in_len)
{
    RSA *rsa = EVP_PKEY_get0_RSA(SSL_get_privatekey(ssl));
    if (rsa == NULL)
    {
        ERROR_MESSAGE("AsyncPrivateKeyDecrypt called with incorrect key type.\n");
        return ssl_private_key_failure;
    }
    if (!RSA_decrypt(rsa, out_len, out, RSA_size(rsa), in, in_len,
        RSA_NO_PADDING))
    {
        ERROR_MESSAGE("RSA_decrypt error\n");
        return ssl_private_key_failure;
    }
    DEBUG_MESSAGE("AsyncPrivateKeyDecrypt ok\n");
    return ssl_private_key_success;
}


struct Offloader *ssl_apk_get_offloader()
{
    return s_offloader;
}


int ssl_apk_start_offloader(int worker)
{
    if (s_offloader != NULL)
        return 0;
    s_offloader = offloader_new("SSL_APK", worker);
    if (s_offloader == NULL)
        return -1;
    return 0;
}


static int ssl_apk_sign_ex(EVP_MD_CTX *ctx, ssl_apk_offload_t *data)
{
    EVP_PKEY_CTX *pctx = NULL;
    // Determine the hash.
    if (EVP_DigestSignInit(ctx, &pctx,
                           SSL_get_signature_algorithm_digest(data->m_sig_alg),
                           NULL, data->m_pkey) != 1)
    {
        ERROR_MESSAGE("[SSL] [APK] %p offload, DigestSignInit() failed\n",
                      data);
        return -1;
    }
    if (SSL_is_signature_algorithm_rsa_pss(data->m_sig_alg))
    {
        if (!EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PSS_PADDING) ||
            !EVP_PKEY_CTX_set_rsa_pss_saltlen(pctx, -1))
        {
            ERROR_MESSAGE("[SSL] [APK] %p offload, Bad additional parameters\n", data);
            return -1;
        }
    }
    if (EVP_DigestSign(ctx, (unsigned char *)data->m_sign,
                        &data->m_sign_size, data->m_in, data->m_in_len) != 1)
    {
        ERROR_MESSAGE("[SSL] [APK] %p offload thread, EVP_DigestSign() failed\n",
                      data);
        return -1;
    }
    return 0;
}


int ssl_apk_sign(ls_offload* item)
{
    ssl_apk_offload_t *data = (ssl_apk_offload_t *)item;
    DEBUG_MESSAGE("[SSL] [APK] %p ssl_apk_sign, PKEY: %p, using alg: %d\n",
                  data, data->m_pkey, data->m_sig_alg);

    EVP_MD_CTX ctx;
    EVP_MD_CTX_init(&ctx);
    int ret = ssl_apk_sign_ex(&ctx, data);
    EVP_MD_CTX_cleanup(&ctx);

    if (ret == -1)
        data->m_sign_size = 0;
    DEBUG_MESSAGE("[SSL] [APK] %p ssl_apk_sign done, ok: %d\n", data, ret);
    return ret;
}


void ssl_apk_offload_release(ssl_apk_offload_t *task)
{
    DEBUG_MESSAGE("[SSL] [APK] %p release, ref: %d\n", task,
                  task->m_header.ref_cnt);
    if (--task->m_header.ref_cnt > 0)
        return;
    DEBUG_MESSAGE("[SSL] [APK] no reference now, release\n");
    if (task->m_sign)
        free(task->m_sign);
    free(task);
    DEBUG_MESSAGE("[SSL] [APK] release done\n");
}


void ssl_apk_release(SSL *ssl)
{
    ssl_apk_offload_t *offload = (ssl_apk_offload_t *)SSL_get_ex_data(ssl, s_ssl_apk_index);
    if (!offload)
    {
        DEBUG_MESSAGE("[SSL: %p] [APK] Attempt to release with no data assigned\n",
                      ssl);
        return;
    }
    DEBUG_MESSAGE("[SSL: %p] [APK] %p release by application\n", ssl, offload);
    ssl_apk_offload_release(offload);
    SSL_set_ex_data(ssl, s_ssl_apk_index, NULL);
}


void ssl_apk_set_api(ls_offload_api *api, void (*done_cb)(void *param))
{
    if (s_ssl_apk_index == -1)
        s_ssl_apk_index = SSL_get_ex_new_index(0, NULL, NULL, NULL,
                                               ssl_free_apk_data);
    api->perform = ssl_apk_sign;
    api->release = (void (*)(struct ls_offload *))ssl_apk_offload_release;
    api->on_task_done = done_cb;
}


ssl_apk_offload_t *ssl_apk_prepare(SSL *ssl, ls_offload_api *api, void *param)
{
    assert (s_ssl_apk_index != -1);
    ssl_apk_offload_t *data = (ssl_apk_offload_t *)malloc(sizeof(ssl_apk_offload_t));
    DEBUG_MESSAGE("[SSL: %p] ssl_apk_prepare entry, ssl_apk_offload_t: %p\n", ssl,
                  data);
    if (!data)
    {
        ERROR_MESSAGE("[SSL: %p] Error creating ssl_apk_offload_t\n", ssl);
        SSL_set_private_key_method(ssl, NULL);
        return NULL;
    }

    memset(data, 0, sizeof(ssl_apk_offload_t));
    data->m_header.ref_cnt = 1;

    SSL_set_ex_data(ssl, s_ssl_apk_index, (void *)data);

    data->m_header.param_task_done = param;
    data->m_header.api = api;
    return data;
}


void ssl_ctx_enable_apk(SSL_CTX *ctx)
{
    DEBUG_MESSAGE("[SSL_CTX] newContext: %p\n", ctx);
    if (s_ssl_apk_index != -1)
    {
        SSL_CTX_set_private_key_method(ctx, &s_async_private_key_method);
        DEBUG_MESSAGE("[SSL_CTX] private_key_method enabled\n");
    }
    else
        DEBUG_MESSAGE("[SSL_CTX] private_key_method not enabled\n");
}


#endif //SSL_ASYNC_PK

