/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
*                                              U General Public License       *
*    along with this program. If not, see http://www.gnu.org/licenses/.      *
*****************************************************************************/
#ifndef SSLSESSION_H
#define SSLSESSION_H

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <stdint.h>
#include <unistd.h>

#include <shm/lsshmcache.h>

class LsShmHash;
class SSLContext;

//
//  Storage for SSL_SESSION
//
typedef struct shmSslData_s
{
    lsShm_hCacheData_t  x_cache;
    uint32_t            x_valueLen;
    u_int8_t            x_sessionData[0];
} shmSslData_t ;
typedef shmSslData_t    TSSLSession;
typedef LsShmCache      TSSLSessionPool;

//
//  SSLSession 
//
class SSLSession
{
    int                     m_ref;
    int8_t                  m_ready;
    SSLSession *            m_next;
    SSLContext *            m_context;
    int32_t                 m_expireSec;
    pid_t                   m_pid;
    static int              m_numNew;
    
    TSSLSessionPool *       m_base;
    static int shmData_init(lsShm_hCacheData_t*, void * pUParam);
    static int shmData_remove(lsShm_hCacheData_t*, void * pUParam);
    
public:
    static uint32_t shmMagic()
                { return s_shmMagic; }
    
    static int watchCtx( SSLContext * pSSLContext
                , const char * shmSslName
                , int maxentries
                , SSL_CTX * pCtx );
    static SSLSession * get(SSLContext * 
                , const char * shmSslName 
                , int maxentries
                , int expireSec);
    void unget();
    inline int ready() 
                { return m_ready; }
    SSLContext * SSLCcontext()
                { return m_context; }
    
    int sessionFlush();
    static int sessionFlushAll();
    
    int stat();
    
private:
    explicit SSLSession(SSLContext *
                , const char *shmName
                , int maxentries
                , int expireSec);
    ~SSLSession();
    
    SSLSession( const SSLSession& other );
    SSLSession& operator=( const SSLSession& other );
    bool operator==( const SSLSession& other );
    

private:    
    // OpenSSL callbacks
    static SSL_SESSION *    sess_get_cb(SSL * pSSL
                                  , unsigned char * id
                                  , int len
                                  , int * ref);
    static void             sess_remove_cb(SSL_CTX * ctx
                                  , SSL_SESSION * pSess);
    static int              sess_new_cb(SSL * pSSL
                                  , SSL_SESSION * pSess);
    
    static void             show_SSLSessionId(const char * tag
                                  , SSLContext * pSSLContext
                                  , unsigned char * buf
                                  , int    size );
    // Compatible method 
    SSL_SESSION *           sess_get_func(SSL * pSSL
                                  , unsigned char * id
                                  , int len
                                  , int * ref);
    void                    sess_remove_func(SSL_SESSION * pSess);
    int                     sess_new_func(SSL * pSSL
                                  , SSL_SESSION * pSess);
    
private:
    void                    add();
    void                    remove();
    SSLSession *            next() { return m_next; }
    static SSLSession *     s_base;
    static uint32_t         s_shmMagic;
    static int32_t          s_idxContext;
};

#endif // SSLSESSION_H

