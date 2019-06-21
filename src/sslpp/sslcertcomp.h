/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#ifndef SSLCERTCOMP_H
#define SSLCERTCOMP_H

#include <assert.h>
#if __cplusplus <= 199711L && !defined(static_assert)
#define static_assert(a, b) _Static_assert(a, b)
#endif
#include <openssl/ssl.h>

#ifdef OPENSSL_IS_BORINGSSL
#define SSLCERTCOMP     // The determiner in ALL the code which uses this class.

#include <lsdef.h>

class SslCertComp 
{
    SslCertComp(); 
    ~SslCertComp();
    
public:
    typedef struct 
    {
        int     m_input_len;
        int     m_len;
        uint8_t m_comp[1];
    } comp_cache_t;
    
    static void activateComp(bool activate);      
    static void activateDecomp(bool activate);
    static void enableCertComp(SSL_CTX *ctx);
    static void enableCertDecomp(SSL_CTX *ctx);
    static void disableCertCompDecomp(SSL_CTX *ctx);
    static void setBrCompressLevel(int level);

    LS_NO_COPY_ASSIGN(SslCertComp);
};

#endif
#endif
