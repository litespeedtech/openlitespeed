/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#include <sslpp/sslcertcomp.h>

#ifdef SSLCERTCOMP
#include <log4cxx/logger.h>
#include <assert.h>
#if __cplusplus <= 199711L && !defined(static_assert)
#define static_assert(a, b) _Static_assert(a, b)
#endif

#include <lsr/ls_pool.h>

#include <util/brotlibuf.h>
#include <util/vmembuf.h>

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

#include <openssl/bytestring.h>
#include <openssl/pool.h>
#include <openssl/span.h>

#define DEBUGGING

#ifdef DEBUGGING

#if TEST_PGM
#define DEBUG_MESSAGE(...)     printf(__VA_ARGS__)
#define ERROR_MESSAGE(...)     fprintf(stderr, __VA_ARGS__)
#define INFO_MESSAGE(...)      printf(__VA_ARGS__)
#else
#define DEBUG_MESSAGE(...)     LS_DBG_L(__VA_ARGS__)
#define ERROR_MESSAGE(...)     LS_ERROR(__VA_ARGS__)
#define INFO_MESSAGE(...)      LS_INFO(__VA_ARGS__)
#endif

#else

#define DEBUG_MESSAGE(...)
#define ERROR_MESSAGE(...)

#endif


static bool s_activate_comp = false;
static bool s_activate_decomp = false;
static int  s_iBrCompressLevel = 6;
static int  s_iSSL_CTX_index = -1;

static void freeCtxData(void *parent, void *ptr, CRYPTO_EX_DATA *ad, int idx, 
                        long argl, void *argp)
{
    assert(idx == s_iSSL_CTX_index);
    if (ptr)
    {
        DEBUG_MESSAGE("[SSLCertComp] freeing data\n");
        ls_pfree(ptr);
    }
}


static SslCertComp::comp_cache_t *cache_data(SSL_CTX *ctx, 
                                             SslCertComp::comp_cache_t *cache, 
                                             int in_size,
                                             char *readBuffer, size_t len)
{
    if (!cache)
    {
        cache = (SslCertComp::comp_cache_t *)ls_palloc(len + sizeof(int) * 2);
        if (!cache)
            return NULL;
        cache->m_input_len = in_size;
        cache->m_len = len;
        memcpy(cache->m_comp, readBuffer, len);    
    }
    else
    {
        SslCertComp::comp_cache_t *recache;
        recache = (SslCertComp::comp_cache_t *)ls_prealloc(cache, cache->m_len + 
                                                           sizeof(int) * 2 + len);
        if (recache)
        {
            memcpy(&recache->m_comp[recache->m_len], readBuffer, len);
            recache->m_len += len;    
            cache = recache;
        }
        else 
        {
            return NULL;
        }    
    }
    SSL_CTX_set_ex_data(ctx, s_iSSL_CTX_index, (void *)cache);
    return cache;
}


static int certCompressFunc(SSL *ssl, CBB *out, const uint8_t *in_data, 
                            size_t in_size, 
#ifdef ZBUF_H                            
                            Zbuf *zbuf, 
#else
                            Compressor *zbuf,
#endif                            
                            int level)
{
    DEBUG_MESSAGE("[SSLCertComp] Compressing %ld bytes\n", in_size);
    SSL_CTX *ctx = SSL_get_SSL_CTX(ssl);
    
    SslCertComp::comp_cache_t *cache;
    cache = (SslCertComp::comp_cache_t *)SSL_CTX_get_ex_data(ctx, s_iSSL_CTX_index);
    if (cache)
    {
        if (cache->m_input_len == (int)in_size)
        {
            DEBUG_MESSAGE("[SSLCertComp] Using cached %d bytes\n", cache->m_len);
            if (CBB_add_bytes(out, (const uint8_t *)cache->m_comp, cache->m_len) == 0)
            {
                ERROR_MESSAGE("[SSLCertComp] Error adding cached data to compressed buffer\n");
                return false;
            }
            return true;
        }
        else
        {
            SSL_CTX_set_ex_data(ctx, s_iSSL_CTX_index, NULL);\
            DEBUG_MESSAGE("[SSLCertComp] input size changed, release old cache\n");
            ls_pfree(cache);
            cache = NULL;
        }
    }
    
    VMemBuf   vmembuf;
    
    if (vmembuf.set(VMBUF_MALLOC, 
#ifdef ZBUF_H                            
                    in_size
#else
                    (int)in_size
#endif                    
                   ) == LS_FAIL)
    {
        ERROR_MESSAGE("[SSLCertComp] Unable to set memory buffer\n");
        return false;
    }
    zbuf->setCompressCache(&vmembuf);
    if (zbuf->init(
#ifdef ZBUF_H                            
                   Zbuf::MODE_COMPRESS, 
#else
                   Compressor::COMPRESSOR_COMPRESS,
#endif                   
                   level
#ifdef ZBUF_H                            
                   , 0
#endif                   
                  ) == LS_FAIL)
    {
        ERROR_MESSAGE("[SSLCertComp] Unable to initialize compression\n");
        return false;
    }
    if (zbuf->beginStream() == LS_FAIL)
    {
        ERROR_MESSAGE("[SSLCertComp] Unable to begin stream\n");
        return false;
    }
    if (zbuf->write((char *)in_data, in_size) == LS_FAIL)
    {
        ERROR_MESSAGE("[SSLCertComp] Unable to write data\n");
        return false;
    }
    char *readBuffer;
    size_t len;
    if (zbuf->endStream() == LS_FAIL)
    {
        ERROR_MESSAGE("[SSLCertComp] Unable to complete the compression\n");
        return false;
    }
    while ((readBuffer = vmembuf.getReadBuffer(len)) && (len))
    {
        DEBUG_MESSAGE("[SSLCertComp] Got compressed buffer of %ld bytes\n", len);
        if (CBB_add_bytes(out, (const uint8_t *)readBuffer, len) == 0)
        {
            ERROR_MESSAGE("[SSLCertComp] Error adding data to compressed buffer\n");
            return false;
        }
        if (!(cache = cache_data(ctx, cache, in_size, readBuffer, len)))
            return false;
        vmembuf.readUsed(len);
    }
    if (CBB_flush(out) == 0)
    {
        ERROR_MESSAGE("[SSLCertComp] Error completing adding to compressed buffer\n");
        SslCertComp::disableCertCompDecomp(ctx);
        return false;
    }
    DEBUG_MESSAGE("[SSLCertComp] Final compression resulted in %ld bytes\n",
                  CBB_len(out));
    return true;
}


static int certCompressFuncBrotli(SSL *ssl, CBB *out, const uint8_t *in, 
                                  size_t in_len)
{
    BrotliBuf brotBuf;
    return certCompressFunc(ssl, out, in, in_len, &brotBuf, s_iBrCompressLevel);
}


static int certDecompressFunc(SSL *ssl, CRYPTO_BUFFER **out,
                              size_t uncompressed_len, const uint8_t *in, 
                              size_t in_len,
#ifdef ZBUF_H                            
                              Zbuf *zbuf, 
#else
                              Compressor *zbuf,
#endif                            
                              int level)
{
    DEBUG_MESSAGE("[SSLCertComp] Decompressing %ld bytes to %ld bytes\n", in_len,
                  uncompressed_len);

    VMemBuf   vmembuf;
    
    if (vmembuf.set(VMBUF_MALLOC, 
#ifdef ZBUF_H                            
                    in_len
#else
                    (int)in_len
#endif                    
                   ) == LS_FAIL)
    {
        ERROR_MESSAGE("[SSLCertComp] Decompress Unable to set memory buffer\n");
        return false;
    }
    zbuf->setCompressCache(&vmembuf);
    if (zbuf->init(
#ifdef ZBUF_H                            
                   Zbuf::MODE_DECOMPRESS, 
#else
                   Compressor::COMPRESSOR_DECOMPRESS,
#endif                   
                   level
#ifdef ZBUF_H                            
                   , 0
#endif                   
                  ) == LS_FAIL)
    {
        ERROR_MESSAGE("[SSLCertComp] Unable to initialize decompression\n");
        return false;
    }
    if (zbuf->beginStream() == LS_FAIL)
    {
        ERROR_MESSAGE("[SSLCertComp] Decompress; Unable to begin stream\n");
        return false;
    }
    if (zbuf->write((char *)in, in_len) == LS_FAIL)
    {
        ERROR_MESSAGE("[SSLCertComp] Decompress; Unable to write data\n");
        return false;
    }
    char *readBuffer;
    size_t len;
    if (zbuf->endStream() == LS_FAIL)
    {
        ERROR_MESSAGE("[SSLCertComp] Unable to complete the decompression\n");
        return false;
    }

    uint8_t *out_data;
    if (!((*out) = CRYPTO_BUFFER_alloc(&out_data, uncompressed_len)))
    {
        ERROR_MESSAGE("[SSLCertComp] Insufficient memory to allocate "
                      "decompression buffer\n");
        return false;
    }
    size_t pos = 0;
    while ((readBuffer = vmembuf.getReadBuffer(len)) && (len))
    {
        DEBUG_MESSAGE("[SSLCertComp] Got decompressed buffer of %ld bytes\n", len);
        if (len + pos > uncompressed_len)
        {
            ERROR_MESSAGE("[SSLCertComp] Decompression exceeded expected len "
                          "of %ld bytes (%ld bytes)\n", uncompressed_len, len + pos);
            CRYPTO_BUFFER_free(*out);
            return false;
        }
        memcpy(&out_data[pos], readBuffer, len);
        pos += len;
        vmembuf.readUsed(len);
    }
    if (len + pos != uncompressed_len)
    {
        ERROR_MESSAGE("[SSLCertComp] Decompression len error %ld != %ld\n",
                      pos, uncompressed_len);
        CRYPTO_BUFFER_free(*out);
        return false;
    }
    DEBUG_MESSAGE("[SSLCertComp] Decompression successful\n");
    return true;
}


static int certDecompressFuncBrotli(SSL *ssl, CRYPTO_BUFFER **out,
                                    size_t uncompressed_len, const uint8_t *in, 
                                    size_t in_len)
{
    BrotliBuf brotBuf;
    return certDecompressFunc(ssl, out, uncompressed_len, in, in_len, &brotBuf,
                              s_iBrCompressLevel);
}

SslCertComp::SslCertComp()
{
}


SslCertComp::~SslCertComp()
{
}


void SslCertComp::activateComp(bool activate)
{   
    DEBUG_MESSAGE("[SSLCertComp] %sactivating compression!\n", activate ? "" : "de");
    s_activate_comp = activate;  
    if ((activate) && (s_iSSL_CTX_index < 0))
        s_iSSL_CTX_index = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, freeCtxData);
}


void SslCertComp::activateDecomp(bool activate)
{   
    DEBUG_MESSAGE("[SSLCertComp] %sactivating decompression!\n", activate ? "" : "de");
    s_activate_decomp = activate;  
    if ((activate) && (s_iSSL_CTX_index < 0))
        s_iSSL_CTX_index = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, freeCtxData);
}


void SslCertComp::setBrCompressLevel(int level)
{ 
    s_iBrCompressLevel = level; 
}


void SslCertComp::disableCertCompDecomp(SSL_CTX *ctx)
{
}


void SslCertComp::enableCertComp(SSL_CTX *ctx)
{
    DEBUG_MESSAGE("[SSLCertComp] enableCertComp: %p\n", ctx);
    if (s_activate_comp)
    {
        if (s_iSSL_CTX_index < 0)
            s_iSSL_CTX_index = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, freeCtxData);
        
        if (!(SSL_CTX_add_cert_compression_alg(ctx, TLSEXT_cert_compression_brotli,
                                               certCompressFuncBrotli, NULL)))
            INFO_MESSAGE("[SSLCertComp] Requested cert compression but unable to "
                         "register Brotli\n");
    }
    else
        DEBUG_MESSAGE("[SSLCertComp] Cert compression not enabled\n");
}

    
void SslCertComp::enableCertDecomp(SSL_CTX *ctx)
{
    DEBUG_MESSAGE("[SSLCertComp] enableCertDecomp: %p\n", ctx);
    if (s_activate_decomp)
    {
        if (s_iSSL_CTX_index < 0)
            s_iSSL_CTX_index = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, freeCtxData);
        
        if (!(SSL_CTX_add_cert_compression_alg(ctx, TLSEXT_cert_compression_brotli,
                                               NULL, certDecompressFuncBrotli)))
            INFO_MESSAGE("[SSLCertComp] Requested cert decompression but unable to "
                         "register Brotli\n");
    }
    else
        DEBUG_MESSAGE("[SSLCertComp] Cert decompression not enabled\n");
}

    
#endif // SSLCERTCOMP
