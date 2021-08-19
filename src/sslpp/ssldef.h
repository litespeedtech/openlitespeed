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

#ifndef __SSL_DEF_H__
#define __SSL_DEF_H__

#include <assert.h>
#if __cplusplus <= 199711L && !defined(static_assert)
#define static_assert(a, b) _Static_assert(a, b)
#endif
#include <openssl/ssl.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct bio_st  BIO;
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct x509_st X509;
typedef struct ssl_cipher_st SSL_CIPHER;
typedef struct ssl_session_st SSL_SESSION;

typedef int (*AsyncCertDoneCb)(void *arg, const char *pDomain);
typedef int (*AsyncCertFunc)(AsyncCertDoneCb cb, void *pParam,
                             const char *pDomain, int iDomainLen);

#define asyncCertDoneCb AsyncCertDoneCb
#define asyncCertFunc AsyncCertFunc

#ifdef OPENSSL_IS_BORINGSSL
#define SSL_ASYNC_PK    // The determiner in ALL the code which uses this class.
#endif

typedef int (*ssl_async_pk_resume_cb)(void *link);

void *async_pk_prepare(SSL *ssl, ssl_async_pk_resume_cb on_resume, void *param);

#ifdef __cplusplus
}
#endif

#endif // Protection define
