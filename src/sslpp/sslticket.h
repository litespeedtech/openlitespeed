/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
    static void dumpKeyExpires(const char *msg, const RotateKeys_t *keys);
public:

    static int  ticketCb(SSL *pSSL, unsigned char aName[16], unsigned char *iv,
                         EVP_CIPHER_CTX *ectx, HMAC_CTX *hctx, int enc);
    static int         init(const char *pFileName, long timeout, int uid, int gid);
    static int         onTimer();

    static inline int  isKeyStoreEnabled()  {   return m_pKeyStore != NULL; }
};

#endif

