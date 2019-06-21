/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#ifndef SSLASYNCPK_H
#define SSLASYNCPK_H

#include <sslpp/ssldef.h>

#ifdef SSL_ASYNC_PK
#include <edio/eventnotifier.h> 
#include <lsr/ls_node.h>

typedef struct evp_pkey_st EVP_PKEY;
typedef struct ls_lfqueue_s ls_lfqueue_t;
typedef struct ls_lfnodei_s ls_lfnodei_t;

class WorkCrew;


class SslApkWorker : public EventNotifier
{
    ls_lfqueue_t *m_pFinishedQueue;
    WorkCrew     *m_crew;
    static SslApkWorker *s_instance;

public:
    SslApkWorker();

    ~SslApkWorker();

    static int prepare();
    static int start(Multiplexer *pMplx, int workers);

    static void setSslAsyncPkNotifier(SslApkWorker *m_not)
    {   s_instance = m_not; }
    
    static SslApkWorker *getSslAsyncPkNotifier()
    {   return s_instance;  }
    
    int init();
    
    int startProcessor(int workers = 1);

    int  addJob(ls_lfnodei_t  *conn);

    int onNotified(int count);
};


class SslAsyncPk : public ls_lfnodei_t
{
public:

    SslAsyncPk();
    ~SslAsyncPk();
    void release();

    static void enableApk(SSL_CTX *ctx);
    static void cancel(SSL *ssl);
    static void releaseAsyncPk(SSL *ssl);

    bool isCanceled()                   {   return (bool)(m_resume_param == NULL); }
    
    void         *m_resume_param;
    ssl_async_pk_resume_cb on_resume_cb;
    int           m_refCounter;
    char          m_state;
    uint16_t      m_sig_alg;
    uint8_t      *m_in;
    size_t        m_in_len;    
    char         *m_sign;
    size_t        m_sign_size;
    EVP_PKEY     *m_pkey;


    static void *privateKeyThread(ls_lfnodei_t *item);
    
    LS_NO_COPY_ASSIGN(SslAsyncPk);
};

#endif
#endif
