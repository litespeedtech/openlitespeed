/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#ifndef SSLASYNCPK_H
#define SSLASYNCPK_H

#include <sslpp/ssldef.h>

#ifdef SSL_ASYNC_PK

typedef struct evp_pkey_st EVP_PKEY;

#include <lsr/ls_offload.h>
typedef struct ssl_apk_offload
{
    ls_offload_t  m_header;
    uint16_t      m_sig_alg;
    uint8_t      *m_in;
    size_t        m_in_len;
    char         *m_sign;
    size_t        m_sign_size;
    EVP_PKEY     *m_pkey;
}ssl_apk_offload_t;

int ssl_apk_start_offloader(int worker);
struct Offloader *ssl_apk_get_offloader();

void ssl_apk_set_api(ls_offload_api *api, void (*done_cb)(void *param));
ssl_apk_offload_t *ssl_apk_prepare(SSL *ssl, ls_offload_api *api, void *param);

void ssl_apk_release(SSL *ssl);

void ssl_ctx_enable_apk(SSL_CTX *ctx);


#endif
#endif
