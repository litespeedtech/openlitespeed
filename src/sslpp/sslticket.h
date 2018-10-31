// Author: Kevin Fwu
// Date: August 6, 2015

#ifndef SSLTICKET_H
#define SSLTICKET_H

#include <stdint.h>
#include <stddef.h>

#define SSLTICKET_NUMKEYS 3

class LsShmHash;
class AutoStr2;
typedef struct STShmData_s STShmData_t;
typedef uint32_t LsShmOffset_t;
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
typedef struct hmac_ctx_st HMAC_CTX;

typedef struct STKey_s
{
    unsigned char aName[16];
    unsigned char aAes[16];
    unsigned char aHmac[16];
    long          expireSec;
} STKey_t;

typedef struct rotatkeys_s
{
    STKey_t       m_aKeys[SSLTICKET_NUMKEYS];
    short         m_idxPrev;
    short         m_idxCur ;
    short         m_idxNext;
} RotateKeys_t;

class SslTicket 
{
    static LsShmHash      *m_pKeyStore;
    static AutoStr2       *m_pFile;
    static LsShmOffset_t   m_iOff;
    static RotateKeys_t    m_keys;
    static long            m_iLifetime;

    SslTicket();
    ~SslTicket();
    SslTicket(const SslTicket &rhs);
    void *operator=(const SslTicket &rhs);

    static int  initShm(int uid, int gid);
    static int  checkShmExpire(STShmData_t *pShmData);
public:

    static int  ticketCb(SSL *pSSL, unsigned char aName[16], unsigned char *iv,
                         EVP_CIPHER_CTX *ectx, HMAC_CTX *hctx, int enc);
    static int         init(const char *pFileName, long timeout, int uid, int gid);
    static int         onTimer();

    static inline int  isKeyStoreEnabled()  {   return m_pKeyStore != NULL; }
};

#endif

