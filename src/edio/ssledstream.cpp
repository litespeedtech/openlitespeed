#include "ssledstream.h"
#include <edio/multiplexerfactory.h>
#include <lsr/ls_str.h>
#include <log4cxx/logger.h>
#include <sslpp/sslconnection.h>
#include <sslpp/sslcontext.h>
#include <sslpp/sslerror.h>

SslEdStream::~SslEdStream()
{
    if (m_ssl)
        delete m_ssl;
}


void SslEdStream::takeover(EdStream *old, SslConnection *ssl)
{
    assert(old->getfd() >= 0);
    assert(ssl);
    if (m_ssl && m_ssl != ssl)
        delete m_ssl;
    m_ssl = ssl;
    setState(SS_CONNECTED);
    EdStream::takeover(old);
}


void SslEdStream::tobeClosed()
{
    if (getState() < SS_SHUTDOWN)
        setState(SS_CLOSING);
}


void SslEdStream::setSslAgain()
{
    if (m_ssl->wantRead() || getFlag(SS_FLAG_WANT_READ))
        getCurMplex()->continueRead(this);
    else
        getCurMplex()->suspendRead(this);

    if (m_ssl->wantWrite() || m_ssl->wpending() > 0 || getFlag(SS_FLAG_WANT_WRITE))
        getCurMplex()->continueWrite(this);
    else
        getCurMplex()->suspendWrite(this);
}


static SSL *getSslConn()
{
    static SslContext *s_pProxyCtx = NULL;
    if (!s_pProxyCtx)
    {
        s_pProxyCtx = new SslContext();
        if (s_pProxyCtx)
        {
            s_pProxyCtx->enableClientSessionReuse();
            s_pProxyCtx->setRenegProtect(0);
            //NOTE: Turn off TLSv13 for now, need to figure out 0-RTT
            s_pProxyCtx->setProtocol(14);
            //s_pProxyCtx->setCipherList();
        }
        else
            return NULL;
    }
    return s_pProxyCtx->newSSL();
}


int SslEdStream::initSsl(SslContext *ctx, ls_str_t *sni, bool enable_h2,
                         SslClientSessCache *cache)
{
    if (m_ssl)
    {
        if (m_ssl->getSSL())
            m_ssl->release();
    }
    else
        m_ssl = new SslConnection();
    if (!m_ssl)
    {
        LS_ERROR(getLogSession(), "[SSL] Insufficient memory creating connection");
        return -1;
    }
    if (!m_ssl->getSSL())
    {
        m_ssl->setSSL(ctx ? ctx->newSSL() : getSslConn());
        if (!m_ssl->getSSL())
        {
            LS_ERROR(getLogSession(), "[SSL] Failed to create connection: %s",
                     SslError().what());
            return -1;
        }
        LS_DBG_L(getLogSession(), "[SSL] get new connection.");
        m_ssl->setfd(getfd());
        if (sni && sni->ptr)
        {
            char ch = *(sni->ptr + sni->len);
            *(sni->ptr + sni->len) = 0;
            m_ssl->setTlsExtHostName(sni->ptr);
            *(sni->ptr + sni->len) = ch;
        }
        if (enable_h2)
            SSL_set_alpn_protos(m_ssl->getSSL(),
                                (const uint8_t*)"\x02h2\x08http/1.1", 12);

        m_ssl->setClientSessCache(cache);
        if (sni)
            m_ssl->tryReuseCachedSession(sni->ptr, sni->len);
        else
            m_ssl->tryReuseCachedSession(NULL, 0);

    }
    return m_ssl != NULL;
}


int SslEdStream::connectSsl()
{
    assert(m_ssl->getSSL());
    int ret = m_ssl->connect();
    switch (ret)
    {
    case 0:
        setSslAgain();
        LS_DBG_L(getLogSession(), "[SSL] handshake is in progress.");
        break;
    case 1:
        LS_DBG_L(getLogSession(), "[SSL] connected, version: %s, %s, session reuse: %d, ALPN: %d.\n",
                 m_ssl->getVersion(), m_ssl->getCipherName(),
                 m_ssl->isSessionReused(), m_ssl->getAlpnResult());
        break;
    default:
        if (errno == EIO)
        {
            LS_DBG_L(getLogSession(), "SSL_connect() failed!: %s ", SslError().what());
        }
        break;
    }

    return ret;
}


int SslEdStream::verifySni(ls_str_t *sni)
{
    if (!m_ssl || !m_ssl->isConnected())
        return LS_OK;
    const char *pSniName = m_ssl->getTlsExtHostName();
    if (pSniName
        && ((strlen(pSniName) != sni->len)
            || (memcmp(pSniName, sni->ptr, sni->len) != 0)))
    {
        LS_DBG_L(getLogSession(), "[SSL] SNI Mismatch, close and reconnect.");
        close();
        return LS_FAIL;
    }
    return LS_OK;
}


void SslEdStream::continueRead()
{
    LS_DBG_L(getLogSession(), "SslEdStream::continueRead(), fd: %d, mask: %hu",
             getfd(), getRegEvents());
    setFlag(SS_FLAG_WANT_READ, 1);
    EdStream::continueRead();
    if (m_ssl && m_ssl->isConnected())
    {
        if (!getFlag(SS_EVENT_PROCESSING) && m_ssl->hasPendingIn())
            onRead();
    }
}


void SslEdStream::suspendRead()
{
    LS_DBG_L(getLogSession(), "SslEdStream::suspendRead(), fd: %d, mask: %hu",
             getfd(), getRegEvents());
    setFlag(SS_FLAG_WANT_READ, 0);
    EdStream::suspendRead();
}


int SslEdStream::read(char* buf, int len)
{
    if (m_ssl)
    {
        if (m_ssl->isConnected())
            return m_ssl->read(buf, len);
        else
            return 0;
    }
    else
        return EdStream::read(buf, len);
}


int SslEdStream::readv(iovec* vector, int count)
{
    if (m_ssl)
    {
        if (!m_ssl->isConnected())
            return 0;
        int total = 0;
        int ret;
        iovec *end = vector + count;
        while (vector < end)
        {
            ret = m_ssl->read((char *)vector->iov_base, vector->iov_len);
            if (ret > 0)
                total += ret;
            if (ret < 0)
                return -1;
            if (ret < (int)vector->iov_len)
                break;
            ++vector;
        }
        return total;
    }
    else
        return EdStream::readv(vector, count);
}


void SslEdStream::continueWrite()
{
    LS_DBG_L(getLogSession(), "SslEdStream::continueWrite(), fd: %d, mask: %hu",
             getfd(), getRegEvents());
    setFlag(SS_FLAG_WANT_WRITE, 1);
    getCurMplex()->continueWrite(this);
}


void SslEdStream::suspendWrite()
{
    LS_DBG_L(getLogSession(), "SslEdStream::suspendWrite(), fd: %d, mask: %hu",
             getfd(), getRegEvents());
    setFlag(SS_FLAG_WANT_WRITE, 0);
    if (m_ssl && m_ssl->getSSL())
    {
        if (m_ssl->wantWrite() || m_ssl->wpending() > 0)
        {
            LS_DBG_L(this, "Pending SSL data, cannot suspend write.");
            return;
        }
    }
    getCurMplex()->suspendWrite(this);
}


int SslEdStream::write(const char* buf, int len)
{
    if (m_ssl)
    {
        if (m_ssl->isConnected())
            return m_ssl->write(buf, len);
        else
            return 0;
    }
    else
        return EdStream::write(buf, len);
}


int SslEdStream::writev(const iovec* iov, int count)
{
    if (m_ssl)
    {
        if (m_ssl->isConnected())
            return m_ssl->writev(iov, count, NULL);
        else
            return 0;
    }
    else
        return EdStream::writev(iov, count);
}


int SslEdStream::writev(IOVec& vector)
{
    if (m_ssl)
    {
        if (m_ssl->isConnected())
            return m_ssl->writev(vector.get(), vector.len(), NULL);
        else
            return 0;
    }
    else
        return EdStream::writev(vector);

}


int SslEdStream::writev(IOVec& vector, int total)
{
    if (m_ssl)
    {
        if (m_ssl->isConnected())
            return m_ssl->writev(vector.get(), vector.len(), NULL);
        else
            return 0;
    }
    else
        return EdStream::writev(vector, total);

}


int SslEdStream::close()
{
    if (m_ssl)
    {
        LS_DBG_L(this, "[SSL] Shutdown ...");
        m_ssl->release();
    }
    return EdStream::close();
}


// return
//   0: flushed
//   1: more to flush
//  -1: something wrong
int SslEdStream::flush()
{
    if (!m_ssl || !m_ssl->getSSL() || getFlag(SS_EVENT_PROCESSING))
        return 0;
    int ret = flushSslWpending();
    switch (ret)
    {
    case 0:
        setSslAgain();
        continueWrite();
        return 1;
    case 1:
        return 0;
    case -1:
        tobeClosed();
        break;
    }
    return ret;
}


int SslEdStream::handleEventsSsl(short int event)
{
    int ret = 0;
    LS_DBG_L("SslEdStream::handleEventsSsl(), fd: %d, mask: %hd, event: %hd",
             getfd(), getEvents(), event);
    if (!m_ssl->isConnected() && (event & (POLLIN|POLLOUT)))
    {
        ret = connectSsl();
        if (ret != 1)
        {
            if (ret == -1)
            {
                setState(SS_CLOSING);
                onPeerClose();
            }
            return ret;
        }
    }
    setFlag(SS_EVENT_PROCESSING, 1);
    if (event & POLLIN)
    {
        //NOTE: force to allow flush output if unexpected POLLIN happens,
        //      flush could detect socket need to be closed.
        if ((getEvents() & POLLIN) == 0)
        {
            m_ssl->setAllowWrite();
            setFlag(SS_FLAG_PAUSE_WRITE, 0);
            if (m_ssl->bufferInput() == -1)
            {
                LS_INFO(this, "SslEdStream::handleEvents() fd: %d, mask=%hd, "
                        "events=%hd, SSL connection closed by peer!",
                        getfd(), getEvents(), event);
                if (getState() < SS_CLOSING)
                    setState(SS_CLOSING);
                ret = -1;
                goto EVENT_DONE;
            }
        }
        do
        {
            ret = onRead();
        } while(ret > 0 && getFlag(SS_FLAG_WANT_READ)
                && m_ssl->hasPendingIn());
    }
    if (event & (POLLHUP | POLLERR))
    {
        if (getState() < SS_CLOSING)
            setState(SS_CLOSING);
        ret = -1;
        goto EVENT_DONE;
    }
    if (event & POLLOUT)
    {
        setFlag(SS_FLAG_PAUSE_WRITE, 0);
        ret = onWrite();
        if (ret == -1 || !getAssignedRevent())
            goto EVENT_DONE;
    }

EVENT_DONE:
    setFlag(SS_EVENT_PROCESSING, 0);

    if (getState() == SS_CLOSING)
    {
        if (onPeerClose() == -1)
            return -1;
    }
    if (ret != -1)
    {
        ret = flushSslWpending();
        if (ret == 1)
            m_ssl->releaseIdleBuffer();
        if (ret != -1)
            onEventDone(event);
    }
    return 0;
}


int SslEdStream::handleEvents(short int event)
{
    if (m_ssl)
        return handleEventsSsl(event);
    return EdStream::handleEvents(event);
}


int SslEdStream::onPeerClose()
{
    setFlag(SS_FLAG_PEER_SHUTDOWN, 1);
    return close();
}


int SslEdStream::shutdown()
{
    if (m_ssl && m_ssl->isConnected())
        m_ssl->shutdown(0);
    return EdStream::shutdown();
}


//return
//   0: not flushed
//   1: flushed
int SslEdStream::flushSslWpending()
{
    LS_DBG_L(this, "SslEdStream::flushSslWpending()...");

    if (isPauseWrite())
        return 0;
    if (getFlag(SS_FLAG_DELAY_FLUSH))
        setFlag(SS_FLAG_DELAY_FLUSH, 0);
    int ret = 1;
    int pending = m_ssl->wpending();
    LS_DBG_L(this, "SSL wpending: %d", pending);
    if (pending > 0)
        ret = m_ssl->flush();
    if (!ret)
    {
        LS_DBG_L(this, "[SSL] more to flush, continue write.");
        MultiplexerFactory::getMultiplexer()->continueWrite(this);
        setFlag(SS_FLAG_PAUSE_WRITE, 1);
    }
    else if (ret == -1)
        tobeClosed();
    else
        if (!isWantWrite())
            MultiplexerFactory::getMultiplexer()->suspendWrite(this);
    return ret;
}
