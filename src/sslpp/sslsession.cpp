/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#include <sslpp/sslsession.h>

#include <http/httplog.h>
#include <main/configctx.h>
#include <shm/lsshmhash.h>
#include <shm/lsshmtypes.h>
#include <sslpp/sslcontext.h>
#include <util/datetime.h>


#define shmSslCache "SSLCache"
#define shmSslHash  "SSL"
int32_t      SSLSession::s_idxContext = 0x0602;         // OpenSSL idx
uint32_t     SSLSession::s_shmMagic =   0x20140602;     // SHM magic
SSLSession *SSLSession::s_base =       NULL;
int SSLSession::m_numNew = 0;

#ifdef DEBUG_RUN
//
//  obj2string - convert to ascii
//
static char *obj2String(TSSLSession *pObj)
{
    static char buf[0x100];
    char *cp = buf;

    if ((pObj->x_cache.x_iMagic == SSLSession::shmMagic()))
        *cp++ = 'M';
    else
        *cp++ = ' ';

    int dt = (DateTime::s_curTime - pObj->x_cache.x_iExpireTime) * 1000 +
             (DateTime::s_curTimeUs / 1000) - pObj->x_cache.x_iExpireTimeMs;
    *cp = ' ';

    if (pObj->x_cache.x_iExpired || (dt > 0))
        *cp++ = 'X';
    else
        *cp++ = ' ';
    *cp++ = pObj->x_cache.x_iInited  ? 'I' : ' ';
    *cp++ = pObj->x_cache.x_iTracked ? 'T' : ' ';
    *cp++ = pObj->x_cache.x_iExpired ? 'E' : ' ';
    int nb = sprintf(cp, " [%8.3lf] %5d %5d", ((double) - dt) / 1000.0
                     , pObj->x_cache.x_iNext
                     , pObj->x_cache.x_iBack);

    cp += nb;

    *cp = 0;
    return buf;
}
#endif


int SSLSession::shmData_init(lsShm_hCacheData_t *p, void *pUParam)
{
    TSSLSession *pObj = (TSSLSession *)p;
    pObj->x_iValueLen = 0;
    return 0;
}


int SSLSession::shmData_remove(lsShm_hCacheData_t *p, void *pUParam)
{
    // Need to test this... We haven't been called by OpenSSL
    // TSSLSession * pObj = (TSSLSession *)p;
    return 0;
}


void SSLSession::add()
{
    m_next = s_base;
    s_base = this;
}


void SSLSession::remove()
{
    if (s_base == this)
    {
        s_base = next();
        return;
    }
    SSLSession *p;
    for (p = s_base; p->next(); p = p->next())
    {
        if (p->next() == this)
        {
            p->m_next = next();
            return;
        }
    }
}


void SSLSession::unget()
{
    HttpLog::notice("SSLSesion::unget <%p> %d %d context %p" , this, m_ready,
                    m_ref, m_context);
    --m_ref;
    if (m_ref == 0)
    {
        remove();
        // clear the hash
        delete this;
    }
}


SSLSession *SSLSession::get(SSLContext *pSSLContext
                            , const char *name
                            , int maxentries
                            , int expireSec)
{
    SSLSession *pSess;
    for (pSess = s_base; pSess; pSess = pSess->next())
    {
        if (pSess->m_context == pSSLContext)
        {
            pSess->m_ref++;
            return pSess;
        }
    }

    SSLSession *pSSLSession =  new SSLSession(pSSLContext
            , name
            , maxentries
            , expireSec);

    if (pSSLSession)
    {
        if (pSSLSession->ready())
        {
            pSSLSession->add();
            return pSSLSession;
        }
        delete pSSLSession;
    }
    return NULL;
}


SSLSession::SSLSession(SSLContext *pSSLContext
                       , const char *name
                       , int maxentries
                       , int expireSec)
    : m_ref(0)
    , m_ready(0)
    , m_next(0)
    , m_context(pSSLContext)
      // , m_pShmHash(0)
    , m_expireSec(expireSec)
    , m_pid(getpid())
    , m_base(0)
{
    m_base = new TSSLSessionPool(s_shmMagic
                                 , shmSslCache
                                 , name          /* shmSslHash */
                                 , maxentries
                                 , sizeof(TSSLSession)
                                 , LsShmHash::hash32id
                                 , LsShmHash::compBuf
                                 , shmData_init
                                 , shmData_remove
                                );
    if (m_base)
    {
        if (m_base->status() != LSSHM_READY)
        {
            delete m_base;
            m_base = NULL;
        }
        else
        {
            m_ready = 1;
            m_numNew = 0;
        }
    }
}


SSLSession::~SSLSession()
{
    HttpLog::notice("~SSLSesion <%p> %d %d context %p" , this, m_ready, m_ref,
                    m_context);
    if (m_ready)
    {
        remove();
        // m_pShmHash->unget();
        // m_pShmHash = NULL;
        m_ready = 0;
    }
}


//
// OPEN SSL interface
//
int SSLSession::watchCtx(SSLContext *pSSLContext
                         , const char *name
                         , int maxentries
                         , SSL_CTX *pCtx)
{
    SSLSession *pSSLSession;

    long oldTimeout = SSL_CTX_get_timeout(pCtx);
    long timeout;

#ifdef DEBUG_RUN
    maxentries = 3101;
    maxentries = 10;
    timeout = 10; // set to lower number so I will able to catch it...
    oldTimeout = pSSLContext->setSessionTimeout(timeout);
#else
    // dont change the timeout setting... context should do this one.
    timeout = oldTimeout;
#endif

    pSSLSession = get(pSSLContext, name, maxentries, timeout);
    if (!pSSLSession)
    {
        ConfigCtx::getCurConfigCtx()->logNotice(
            "FAILED TO SETUP EXTERNAL STORAGE SSL CTX %p <%p> ID %s CACHE %d"
            , pSSLContext, pCtx, name
            , maxentries
        );
        // NO MEMORY!
        // problem to create new context!
        // pSSLContext->setContextExData(s_idxContext, NULL);
        return LS_FAIL;
    }

    pSSLContext->setContextExData(s_idxContext, pSSLSession);

    long oldCacheSize = 0;
    long oldCacheMode = 0;
    long cachemodes = 0;

#if 0
    // According to openSSL spec, we should call SSL_CTX_set_session_id_context
    // if we will re-use session.
    // But this does not work...
    int idReturn = SSL_CTX_set_session_id_context(pCtx,
                   (unsigned char *)name, strlen(name)) ;
#endif
    //
    oldCacheSize = pSSLContext->setSessionCacheSize(maxentries);
    cachemodes = SSL_SESS_CACHE_SERVER | SSL_SESS_CACHE_NO_INTERNAL ;
    oldCacheMode = pSSLContext->setSessionCacheMode(cachemodes);

    /*
     * This options force the openSSL to activate callback
     * regardless if the user have ticker or not!
     * keep the server stateless...
     *
     * SSL_set_options - may be we should set this too...
     *
     * These two options should be called carefully.
     * We should consult the user configuration setting on this.
     * May be we should move this code to sslcontext.cpp.
     */
    long oldOptions = pSSLContext->getOptions();
    long options;
    options = oldOptions | SSL_OP_NO_TICKET;
    oldOptions = pSSLContext->setOptions(options);

    // Can't call HttpLog - because http is not up yet!
    ConfigCtx::getCurConfigCtx()->logNotice("SET OPENSSL CTX %p <%p> MAP %s CACHE %d [%d] TIMEOUT %d [%d] Options %X [%X] Mode %X [%X]",
                                            pSSLContext, pCtx, name
                                            , maxentries, oldCacheSize
                                            , timeout, oldTimeout
                                            , options, oldOptions
                                            , cachemodes, oldCacheMode
                                           );

    //add SHM session cache callbacks
    SSL_CTX_sess_set_new_cb(pCtx, sess_new_cb);
    SSL_CTX_sess_set_remove_cb(pCtx, sess_remove_cb);
    SSL_CTX_sess_set_get_cb(pCtx, sess_get_cb);


    return 0; // always good!
}


static inline int id2ascii(unsigned char *frombuf, int size, char *tobuf,
                           int maxsize)
{
    int oldsize = maxsize;
    uint32_t *lp;

    lp = (uint32_t *)frombuf;
    while ((size > 4) && (maxsize > 0))
    {
        snprintf(tobuf, maxsize, "%08X", *lp++);
        tobuf += 4;
        maxsize -= 8;
        size -= 4;
    }
    unsigned char *cp;
    cp = (unsigned char *)lp;
    while (size && (maxsize > 0))
    {
        snprintf(tobuf, maxsize, "%02X", (uint32_t) *cp++);
        tobuf += 2;
        maxsize -= 2;
        size--;
    }
    if (maxsize > 0)
    {
        // maxsize--;
        *cp = 0;
    }
    return oldsize - maxsize;
}


//
//  @brief show_SSLSessionId
//  @brief print out the session id
//
void SSLSession::show_SSLSessionId(const char *tag
                                   , SSLContext *pSSLContext
                                   , unsigned char *session_id
                                   , int session_id_length)
{
    char abuf[0x100];
    int len;

    len = id2ascii(session_id, session_id_length, abuf, 0x100 - 1);
    HttpLog::notice("%6d %3d %s <%p>: ID [%s] size %d len %d"
                    , getpid(), m_numNew
                    , tag , pSSLContext
                    , abuf , session_id_length, len);
    ;
}


SSL_SESSION *SSLSession::sess_get_cb(SSL *pSSL, unsigned char *id, int len,
                                     int *ref)
{
    SSL_CTX *pCtx = SSL_get_SSL_CTX(pSSL);
    SSLSession *pSSLSession = (SSLSession *)SSL_CTX_get_ex_data(pCtx,
                              s_idxContext);
    // no need to check pSSLSession since this routines shouldn't be called
    // if the SSLSession is not ready...
    return pSSLSession->sess_get_func(pSSL, id, len, ref);
}


SSL_SESSION *SSLSession::sess_get_func(SSL *pSSL, unsigned char *id,
                                       int len, int *ref)
{
    SSL_SESSION *pSess = NULL;

#ifdef DEBUG_SHOW_MORE
    show_SSLSessionId("SESS GET", SSLCcontext(), id, len);
#endif

    // according to internet doc... we should set this to zero???
    *ref = 0;

    // lock
    m_base->lock();
#ifdef DEBUG_RUN
    int numTry = 0;
again:
#endif

    TSSLSession *pObj = (TSSLSession *)m_base->findObj(id, len);
    if (pObj)
    {
        if (pObj->x_cache.x_iMagic == s_shmMagic)
        {
            if ((!pObj->x_cache.x_iExpired))
            {
                const unsigned char *cp;
                cp = (const unsigned char *) pObj->x_sessionData;
                if ((pSess = d2i_SSL_SESSION(NULL,
                                             (const unsigned char **) &cp,
                                             pObj->x_iValueLen)))
                {
#ifdef DEBUG_SHOW_MORE
                    show_SSLSessionId("SESS RET"
                                      , SSLCcontext()
                                      , pSess->session_id
                                      , pSess->session_id_length);
#endif
                    ;
                }
                else
                {
                    show_SSLSessionId("SESS GET-FAILED TO CREATE SSL SESSION"
                                      , SSLCcontext()
                                      , id
                                      , len);
                }
            }
            else
            {
                // Expired Session
                show_SSLSessionId("SESS GET-EXPIRED"
                                  , SSLCcontext()
                                  , id
                                  , len);
            }
        }
    }
    else
    {
        // Failed to allocated SSL SESSION
        show_SSLSessionId("SESS GET-FAILED TO FIND SSL SESSION"
                          , SSLCcontext()
                          , id
                          , len);
#ifdef DEBUG_RUN
        HttpLog::notice("SLEEP FOR 10 SECONDS %d [%d]", getpid(), ++numTry);
        sleep(10);
        goto again; /* for testing purpose... waiting for my attach*/
#endif
    }

    // unlock
    m_base->unlock();
    return pSess;
}


int SSLSession::sess_new_cb(SSL *pSSL, SSL_SESSION *pSess)
{
    SSL_CTX *pCtx = SSL_get_SSL_CTX(pSSL);
    SSLSession *pSSLSession = (SSLSession *)SSL_CTX_get_ex_data(pCtx,
                              s_idxContext);
    // no need to check pSSLSession since this routines shouldn't be called
    // if the SSLSession is not ready...
    return pSSLSession->sess_new_func(pSSL, pSess);
}


int SSLSession::sess_new_func(SSL *pSSL, SSL_SESSION *pSess)
{
    m_numNew++;
#ifdef DEBUG_TEST_SYS
    static int flushkey = 0;
    static int maxKeep = 10;
    static int modStat = 5;

    if (flushkey >= maxKeep)
    {
        stat();
        sessionFlush();
        HttpLog::notice("AFTER FLUSH STATISTIC <%p> " , this);
        stat();
        flushkey = 0;
    }
    else if ((flushkey % modStat) == (modStat - 1))
        stat();
    flushkey++;
#endif
    // flush out expired data
    if (!(m_numNew & 0x400))
        sessionFlush();

    // session id len
    int session_len = i2d_SSL_SESSION(pSess, NULL);

    // lock
    m_base->lock();
    TSSLSession *pObj = (TSSLSession *) m_base->getObj(pSess->session_id
                        , pSess->session_id_length
                        , NULL
                        , sizeof(TSSLSession) + session_len
                        , (void *)this);
    if (pObj)
    {
        if (pObj->x_iValueLen)
        {
            // This will never happened - guarantee by openssl
            goto out;
            ;;
        }
        pObj->x_iValueLen = session_len;
        unsigned char *cp = (unsigned char *)pObj->x_sessionData;
        i2d_SSL_SESSION(pSess, &cp);

        pObj->x_cache.x_iExpireTime += m_expireSec ;
        pObj->x_cache.x_iInited = 1;
        if (!pObj->x_cache.x_iTracked)
            m_base->push((lsShm_hCacheData_t *)pObj);
#ifdef DEBUG_SHOW_MORE
        show_SSLSessionId("SESS NEW"
                          , SSLCcontext()
                          , pSess->session_id
                          , pSess->session_id_length);
#endif
    }
    else
    {
        show_SSLSessionId("SESS NEW-NO-HASH-MEMORY"
                          , SSLCcontext()
                          , pSess->session_id
                          , pSess->session_id_length);
    }

out:
    // unlock
    m_base->unlock();

    return 1 ; //session will be cached
    // return 0 ; //session will not be cached
}


//
//  @brief sess_remove_cb
//  @brief     NOTE: this code has not been tested; because openssl never called this?.
//
void SSLSession::sess_remove_cb(SSL_CTX *pCtx, SSL_SESSION *pSess)
{
    SSLSession *pSSLSession = (SSLSession *)SSL_CTX_get_ex_data(pCtx,
                              s_idxContext);
    // no need to check pSSLSession since this routines shouldn't be called
    // if the SSLSession is not ready...
    pSSLSession->sess_remove_func(pSess);
}


void SSLSession::sess_remove_func(SSL_SESSION *pSess)
{
    show_SSLSessionId("SESS RM "
                      , SSLCcontext()
                      , pSess->session_id
                      , pSess->session_id_length);

    // lock
    m_base->lock();
    TSSLSession *pObj = (TSSLSession *)m_base->findObj(pSess->session_id,
                        pSess->session_id_length);
    if (pObj)
    {
        if (pObj->x_cache.x_iMagic == s_shmMagic)
            pObj->x_cache.x_iExpired = 1;
    }
    //   remove the file according to session id
    // unlock
    m_base->unlock();

    sessionFlush(); // flush out the SHM expired sessions
}


//
//  SessionCache Cleanup here
//
int SSLSession::sessionFlushAll()
{
    int        num = 0;
    SSLSession *pSSLSession;

    for (pSSLSession = s_base;
         pSSLSession; pSSLSession = pSSLSession->next())
        num += pSSLSession->sessionFlush();
    return num;
}


static int checkStatElement(lsShm_hCacheData_t *p , void *pUParam)
{
#ifdef DEBUG_RUN
    TSSLSession *pObj = (TSSLSession *)p;
    LsHashStat *pHashStat = (LsHashStat *)pUParam;
    HttpLog::notice("ELEMENT %d %s", pHashStat->num, obj2String(pObj));
#endif
    return 0;
}


int SSLSession::sessionFlush()
{
    // lock
    m_base->lock();
    int num = m_base->remove2NonExpired(this);
    // unlock
    m_base->unlock();
    return num;
}


int SSLSession::stat()
{
    LsHashStat stat;

    // lock
    m_base->lock();
    m_base->stat(stat, checkStatElement);
    HttpLog::notice("NEWSESSION STATISTIC <%p> " , this);
    HttpLog::notice("HASH STATISTIC NUM %3d DUP %3d EXPIRED %d IDX [%d %d %d]"
                    , stat.num
                    , stat.numDup
                    , stat.numExpired
                    , stat.numIdx
                    , stat.numIdxOccupied
                    , stat.maxLink);
    HttpLog::notice("HASH STATISTIC TOP %d %d %d %d %d [%d %d %d %d %d]"
                    , stat.top[0]
                    , stat.top[1]
                    , stat.top[2]
                    , stat.top[3]
                    , stat.top[4]
                    , stat.top[5]
                    , stat.top[6]
                    , stat.top[7]
                    , stat.top[8]
                    , stat.top[9]);
    // unlock
    m_base->unlock();
    return 0;
}
