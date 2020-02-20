/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2020  LiteSpeed Technologies, Inc.                 *
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

#include "quicengine.h"
#include <edio/eventreactor.h>
#include <edio/evtcbque.h>
#include <edio/multiplexer.h>
#include <http/clientcache.h>
#include <http/clientinfo.h>
#include <http/vhostmap.h>
#include <http/httpheader.h>
#include <http/httplistenerlist.h>
#include <socket/gsockaddr.h>
#include <log4cxx/logger.h>
#include <lsr/ls_str.h>
#include <http/ntwkiolink.h>

#include <quic/quicstream.h>
#include <quic/udplistener.h>
#include <quicshm.h>
#include <sslpp/sslcontext.h>
#include <sslpp/sslutil.h>
#include <shm/lsshm.h>

#include <lsquic.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif

#include <ls_sendmmsg.h>

#define QUIC_PENDING_OUT 1

#define N_STREAM_Qs 10
TDLinkQueue<QuicStream> QuicEngine::s_streamQueues[N_STREAM_Qs];

unsigned QuicEngine::s_active_conns = 0;


UdpListenerList::~UdpListenerList()
{
    release_objects();
}


int QuicEngine::getAltSvcVerStr(unsigned short port, char *alt_svc, size_t sz)
{
    const char *const *alpn;
    const char *gquic_versions;
    unsigned versions, off;
    int n;

    assert(sz > 0);

    off = 0;
    versions = lsquic_engine_quic_versions(m_pEngine);
    gquic_versions = lsquic_get_alt_svc_versions(versions);
    if (gquic_versions && gquic_versions[0])
    {
        n = snprintf(alt_svc + off, sz - off,
                "quic=\":%hu\"; ma=2592000; v=\"%s\"", port, gquic_versions);
        if (n < 0 || (size_t) n >= sz - off)
            return -1;
        off += (unsigned) n;
    }

    alpn = lsquic_get_h3_alpns(versions);
    if (!alpn)
    {
        alt_svc[off] = '\0';
        goto end;
    }

    for ( ; *alpn; ++alpn)
    {
        n = snprintf(alt_svc + off, sz - off,
                "%s%s=\":%hu\"; ma=2592000", off ? ", " : "", *alpn, port);
        if (n < 0 || (size_t) n >= sz - off)
        {
            alt_svc[off] = '\0';
            goto end;
        }
        off += (unsigned) n;
    }

  end:
    if (off > 0)
        return 0;
    else
        return -1;
}

UdpListener *QuicEngine::startUdpListener(void *pTcpPeer, VHostMap *pMap)
{
    UdpListener *pUdp = new UdpListener(this, pTcpPeer, pMap);
    pUdp->setAddr(pMap->getAddrStr()->c_str());
    int ret = pUdp->start();
    if (ret != LS_FAIL)
    {
        registerUdpListener(pUdp);
        LS_DBG_H("enableQuic addr: %s", pMap->getAddrStr()->c_str());
    }
    else
    {
        delete pUdp;
        pUdp = NULL;
        LS_DBG_H("Failed to enable QUIC for: %s", pMap->getAddrStr()->c_str());
    }
    return pUdp;
}


pid_t QuicEngine::s_pid = -1;

void  QuicEngine::setpid( pid_t pid)
{
    s_pid = pid;
    QuicShm::setPid(pid);
}


static int logger_log_buf (void *ctx, const char *buf, size_t len)
{
    log4cxx::Logger::s_logbuf(NULL, buf, len);

    return 0;
}


static const struct lsquic_logger_if logger_if = { logger_log_buf };



// void QuicEngine::cancel_events(void *event_ctx)
// {
//     QuicEngine *pEngine = (QuicEngine *)event_ctx;
//     if (pEngine->m_timerId != 0)
//     {
//         ModTimerList::getInstance().removeTimer(pEngine->m_timerId);
//         pEngine->m_timerId = 0;
//     }
// }

lsquic_conn_ctx_t *QuicEngine::onNewConn(void *stream_if_ctx,
                                             lsquic_conn_t *c)
{
    const sockaddr *pPeer, *pLocal;
    ClientInfo *pClientInfo;

    LS_DBG("QuicEngine::onNewConn (%p)\n", c);

    if (0 != lsquic_conn_get_sockaddr(c, &pLocal, &pPeer))
    {
        LS_WARN("[QuicEngine::onNewConn] could not get socket addresses for "
                                                        "the new connection");
        lsquic_conn_abort(c);
        return NULL;
    }

    pClientInfo = ClientCache::getInstance().getClientInfo((sockaddr *)pPeer);
    if (!pClientInfo ||
        (int) pClientInfo->getConns() >= pClientInfo->getPerClientHardLimit())
    {
        lsquic_conn_abort(c);
        return NULL;
    }
    pClientInfo->incConn();
    ++s_active_conns;
    LS_DBG_H("[%s] [CLC] QUIC engine increase connection count 1, current: %u.",
             pClientInfo->getAddrString(), s_active_conns);
//     HttpGlobals::incCurConns();
//     LS_DBG_H("[%s] [CLC] QUIC engine increase connection count 1, total available: %d.",
//              pClientInfo->getAddrString(),
//              HttpGlobals::getConnLimitCtrl()->getAvailConn());
    return (lsquic_conn_ctx_t *)pClientInfo;
}

void QuicEngine::onConnEstablished(lsquic_conn_t *c)
{
}


static int s_isDestruct = 0;

#ifndef _NOT_USE_SHM_
const struct lsquic_shared_hash_if s_quic_shi_if =
{
    QuicShm::insertItem,
    QuicShm::deleteItem,
    QuicShm::lookupItem,
};


void QuicEngine::touchSCIDs(void *ctx, void **peer_ctx,
                                    const lsquic_cid_t *cids, unsigned count)
{
    LS_DBG("QuicEngine::touchSCIDs\n");
    QuicShm::getInstance().touchCidItems(cids, count);
}


void QuicEngine::removeOldSCIDs(void *ctx, void **peer_ctx,
                                    const lsquic_cid_t *cids, unsigned count)
{
    LS_DBG("QuicEngine::removeOldSCIDs\n");
    QuicShm::getInstance().markBadCidItems(cids, count, -1);
    const lsquic_cid_t *const end = cids + count;
    for( ; cids < end; ++cids)
        UdpListener::deleteCidListenerEntry(cids);

}


void QuicEngine::addNewSCIDs(void *ctx, void **peer_ctx,
                                    const lsquic_cid_t *cids, unsigned count)
{
    UdpListener *pUdpListener;
    unsigned n;
    ShmCidPidInfo pids[count];

    LS_DBG("QuicEngine::addNewSCIDs\n");
    memset(pids, 0, sizeof(pids));  /* Set .pid to 0 to signify unknown pids */
    QuicShm::getInstance().lookupCidPids(cids, pids, count);

    for(n = 0; n < count; ++n)
        if (pids[n].pid > 0)
        {
            pUdpListener = (UdpListener *) peer_ctx[n];
            pUdpListener->updateCidListenerMap(&cids[n], &pids[n]);
        }
}


void QuicEngine::onConnClosed(lsquic_conn_t *c)
{
    const lsquic_cid_t *cid;
    lsquic_conn_ctx_t *h = lsquic_conn_get_ctx(c);
    LS_DBG("QuicEngine::onConnClosed (%p)\n", c);
    ClientInfo *pClientInfo = (ClientInfo *)h;
    if (pClientInfo)
        pClientInfo->decConn();
    s_active_conns--;
    LS_DBG_H("[%s] [CLC] QUIC engine decrease connection count 1, current: %u.",
             pClientInfo ? pClientInfo->getAddrString() : "N/A", s_active_conns);
//     HttpGlobals::decCurConns();
//     LS_DBG_H("[%s] [CLC] QUIC engine decrease connection count 1, total available: %d.",
//              pClientInfo->getAddrString(),
//              HttpGlobals::getConnLimitCtrl()->getAvailConn());
    if (s_isDestruct)
        return;
    cid = lsquic_conn_id(c);
    QuicShm::getInstance().markClosedCidItem(cid);
    UdpListener::deleteCidListenerEntry(cid);
}


#else

void QuicEngine::onConnClosed(lsquic_conn_t *c, lsquic_conn_ctx_t *h)
{
    ClientInfo *pClientInfo = (ClientInfo *)h;
    if (pClientInfo)
        pClientInfo->decConn(1);

    HttpGlobals::s_iCurConns--;
}

#endif

// int HttpListener::checkAccess(struct conn_data *pData)
// {
//     struct sockaddr *pPeer = (struct sockaddr *) pData->achPeerAddr;
//     if ((AF_INET6 == pPeer->sa_family) &&
//         (IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)pPeer)->sin6_addr)))
//     {
//         pPeer->sa_family = AF_INET;
//         memmove(&((struct sockaddr_in *)pPeer)->sin_addr.s_addr,
//                 &pData->achPeerAddr[20], 4);
//     }
//     ClientInfo *pInfo = ClientCache::getClientCache()->getClientInfo(pPeer);
//     pData->pInfo = pInfo;
//
//     LS_DBG_H(this, "New connection from %s:%d.", pInfo->getAddrString(),
//              ntohs(((struct sockaddr_in *)pPeer)->sin_port));
//
//     return pInfo->checkAccess();
// }

static void * createUnpackedHeaders(void *hsi_ctx,
                                          int is_push_promise)
{
    return new QuicUpkdHdrs();
}


static enum lsquic_header_status endUnpackedHeader(QuicUpkdHdrs *qhdrs)
{
    if (!qhdrs->scheme || !qhdrs->headers.isComplete())
        return LSQUIC_HDR_ERR_INCOMPL_REQ_PSDO_HDR;

    if (qhdrs->cookie_count > 0)
    {
        qhdrs->total_cookie_size += ((qhdrs->cookie_count - 1) << 1) ;
        qhdrs->total_size += qhdrs->total_cookie_size + 6 + 4;
        if (qhdrs->total_size >= 65535)
            return LSQUIC_HDR_ERR_HEADERS_TOO_LARGE;
        qhdrs->headers.appendCookieHeader(qhdrs->cookies, qhdrs->cookie_count,
                                    qhdrs->total_cookie_size);
    }

    qhdrs->headers.endHeader();
    return LSQUIC_HDR_OK;
}


static enum lsquic_header_status processUnpackedHeader(
    void *hdr_set, unsigned name_idx, const char *name, unsigned name_len,
    const char *val, unsigned val_len)
{
    QuicUpkdHdrs *qhdrs = (QuicUpkdHdrs *)hdr_set;
    if (name == NULL)
    {
        return endUnpackedHeader(qhdrs);
    }

    int idx = UnpackedHeaders::convertHorQpackIdx(name_idx);

    if (name_len > 2 && name[0] == ':')
    {
        if (qhdrs->regular_header)
            return LSQUIC_HDR_ERR_MISPLACED_PSDO_HDR;
        if (val_len == 0)
            return LSQUIC_HDR_ERR_HEADERS_TOO_LARGE;   //invalid size
        if (idx == UPK_HDR_UNKNOWN)
        {
            switch(name[1])
            {
            case 'a':
                if (name_len == 10 && memcmp(name, ":authority", 10) == 0)
                    idx = HttpHeader::H_HOST;
                break;
            case 'm':
                if (name_len == 7 && memcmp(name, ":method", 7) == 0)
                    idx = UPK_HDR_METHOD;
                break;
            case 'p':
                if (name_len == 5 && memcmp(name, ":path", 5) == 0)
                    idx = UPK_HDR_PATH;
                break;
            case 's':
                if (name_len == 7 && memcmp(name, ":scheme", 7) == 0)
                    idx = UPK_HDR_SCHEME;
                break;
            default:
                return LSQUIC_HDR_ERR_UNNEC_REQ_PSDO_HDR;
            }
        }
        switch(idx)
        {
        case HttpHeader::H_HOST:         //":authority",
            if (qhdrs->headers.setHost2(val, val_len) == LS_FAIL)
                return LSQUIC_HDR_ERR_DUPLICATE_PSDO_HDR;
            break;
        case UPK_HDR_METHOD:  //":method"
            //If second time have the :method, ERROR
            if (qhdrs->headers.setMethod(val, val_len) == LS_FAIL)
                return LSQUIC_HDR_ERR_DUPLICATE_PSDO_HDR;
            break;
        case UPK_HDR_PATH:  //":path"
            //If second time have the :path, ERROR
            if (qhdrs->headers.setUrl2(val, val_len) == LS_FAIL)
                return LSQUIC_HDR_ERR_DUPLICATE_PSDO_HDR;
            break;
        case UPK_HDR_SCHEME:  //":scheme"
            if (qhdrs->scheme)
                return LSQUIC_HDR_ERR_DUPLICATE_PSDO_HDR;
            qhdrs->scheme = true;
            //Do nothing
            //We set to (char *)"HTTP/1.1"
            break;
        case UPK_HDR_STATUS:    //":status"
            return LSQUIC_HDR_ERR_UNNEC_REQ_PSDO_HDR;
        default:
            return LSQUIC_HDR_ERR_UNNEC_REQ_PSDO_HDR;
        }
    }
    else
    {
        if (!qhdrs->regular_header)
        {
            if (!qhdrs->headers.isComplete())
                return LSQUIC_HDR_ERR_INCOMPL_REQ_PSDO_HDR;
            qhdrs->regular_header = true;
        }
        if (idx == UPK_HDR_UNKNOWN)
        {
            for(const char *p = name; p < name + name_len; ++p)
                if (isupper(*p))
                    return LSQUIC_HDR_ERR_UPPERCASE_HEADER;
            if (name_len == 10 && memcmp(name, "connection", 10) == 0)
                return LSQUIC_HDR_ERR_BAD_REQ_HEADER;
            else if (name_len == 6 && memcmp(name, "cookie", 6) == 0)
                idx = HttpHeader::H_COOKIE;
        }
        if (idx == HttpHeader::H_COOKIE)
        {
            if (qhdrs->cookie_count < 256)
            {
                if (ls_str(&qhdrs->cookies[qhdrs->cookie_count], val,
                            val_len) == NULL)
                    return LSQUIC_HDR_ERR_NOMEM;
                qhdrs->total_cookie_size += val_len;
                ++qhdrs->cookie_count;
            }
            return LSQUIC_HDR_OK;
        }
        else if (name_len == 2 && memcmp(name, "te", 2) == 0)
        {
            if (val_len != 8 || strncasecmp("trailers", val, 8) != 0)
                return LSQUIC_HDR_ERR_BAD_REQ_HEADER;
        }
        qhdrs->total_size += name_len + val_len + 4;
        if (qhdrs->total_size >= 65535)
            return LSQUIC_HDR_ERR_HEADERS_TOO_LARGE;
        qhdrs->headers.appendHeader(idx, name, name_len, val, val_len);
    }
    return LSQUIC_HDR_OK;
}


static void releaseUnpackedHeaders(void *hdr_set)
{
    delete (QuicUpkdHdrs *)hdr_set;
}


static struct lsquic_hset_if s_quic_hpack_hdr_callback =
{
    createUnpackedHeaders,
    processUnpackedHeader,
    releaseUnpackedHeaders,
};


lsquic_stream_ctx_t *QuicEngine::onNewStream(void *stream_if_ctx,
                                                    lsquic_stream_t *s)
{
    const sockaddr *pPeer, *pLocal;
    ClientInfo *pClientInfo;
    int token;
    lsquic_conn_t *c = lsquic_stream_conn(s);
    lsquic_conn_get_sockaddr(c, &pLocal, &pPeer);

    LS_DBG("QuicEngine::onNewStream (%p)\n", s);

    pClientInfo = (ClientInfo *)lsquic_conn_get_ctx(c);
    if (!pClientInfo || pClientInfo->checkAccess())
        return NULL;

    QuicStream *pStream = new QuicStream();
    ConnInfo * pConnInfo = (ConnInfo *)pStream->getConnInfo();
    UdpListener * pUdpListener = (UdpListener *)lsquic_conn_get_peer_ctx(c,
                                    pConnInfo->m_pServerAddrInfo->getAddr());
    pConnInfo->m_pCrypto = pStream;
    pConnInfo->m_pClientInfo = pClientInfo;
    pConnInfo->m_pServerAddrInfo = ServerAddrRegistry::getInstance().get(
             pLocal, pUdpListener->getTcpPeer());
    pConnInfo->m_remotePort = GSockAddr::getPort(pPeer);
    //pStream->setConnInfo(getStream()->getConnInfo());
    pStream->init(s);
    QuicUpkdHdrs *hdrs = (QuicUpkdHdrs *)lsquic_stream_get_hset(s);
    if (hdrs)
    {
        if (pStream->processUpkdHdrs(hdrs) == LS_FAIL)
        {
            delete pStream;
            return NULL;
        }
    }
    else
        pStream->continueRead();

    token = NtwkIOLink::getToken();
    assert(token < N_STREAM_Qs);
    s_streamQueues[token].append(pStream);
    return pStream->toHandler();
}


void QuicEngine::onStreamRead(lsquic_stream_t *s, lsquic_stream_ctx_t *h)
{
    QuicStream *pStream = (QuicStream *)h;
    pStream->onRead();
}


void QuicEngine::onStreamWrite(lsquic_stream_t *s, lsquic_stream_ctx_t *h)
{
    QuicStream *pStream = (QuicStream *)h;
    pStream->onWrite();
}


void QuicEngine::onStreamClose(lsquic_stream_t *s, lsquic_stream_ctx_t *h)
{
    LS_DBG("QuicEngine::onStreamClose (%p)\n", s);

    QuicStream *pStream = (QuicStream *)h;
    if (pStream)
    {
        pStream->onClose();
        pStream->remove();
        s_streamsTobeRecycled.push_back(pStream);
    }
}


TPointerList<QuicStream> QuicEngine::s_streamsTobeRecycled;

void QuicEngine::recycleStreams()
{
    QuicStream *pStream;
    while(!s_streamsTobeRecycled.empty())
    {
        pStream = s_streamsTobeRecycled.pop_back();
        if (pStream->getHandler())
            pStream->getHandler()->recycle();
        delete pStream;
    }
}


static struct lsquic_stream_if s_quic_if =
{
    QuicEngine::onNewConn,
    QuicEngine::onConnEstablished,
    QuicEngine::onConnClosed,

    /* If you need to initiate a connection, call lsquic_conn_make_new_stream().
     * This will cause `on_new_stream' callback to be called when appropriate
     * (this operation is delayed when maximum number of outgoing streams is
     * reached).
     *
     * After `on_close' is called, the stream is no longer accessible.
     */
    QuicEngine::onNewStream,
    QuicEngine::onStreamRead,
    QuicEngine::onStreamWrite,
    QuicEngine::onStreamClose,
    NULL,
    NULL,
    NULL,
};


struct ssl_ctx_st * QuicEngine::getSslCtxCb(void *peer_ctx)
{
    UdpListener *pUdpListener = (UdpListener *) peer_ctx;
    return pUdpListener->getSslContext();
}


struct ssl_ctx_st * QuicEngine::sniCb(void *pCtx, const sockaddr *pLocal,
                                      const char *sni)
{
    LS_DBG("QuicEngine::sniCb (%p)\n", sni);

    if (!sni)
        return NULL;
    SslContext *pSslCtx;
    //QuicEngine *pEngine = (QuicEngine *)pCtx;
    const VHostMap *pMap = NULL;
    if (pLocal)
    {
        const ServerAddrInfo * pAddrInfo = ServerAddrRegistry::getInstance().get(pLocal, NULL);
        if (pAddrInfo)
            pMap = pAddrInfo->getVHostMap();
    }
    if (pMap)
    {
        pSslCtx = VHostMapFindSslContext((void *)pMap, sni);
        if (!pSslCtx)
            pSslCtx = pMap->getSslContext();
        if (pSslCtx)
            return pSslCtx->get();
    }
    return NULL;
}


int QuicEngine::registerUdpListener(UdpListener *pListener)
{
    LS_DBG("QuicEngine::registerUdpListener (%p)\n", pListener);
    if (!pListener)
        return -1;
    int index = m_udpListeners.size();
    m_udpListeners.push_back(pListener);
    pListener->setId(index);
    return index;
}


UdpListener *QuicEngine::getListener(int index) const
{
    if (index < 0 || index >= m_udpListeners.size())
        return NULL;
    return m_udpListeners[index];
}


static size_t
spec_size(const struct lsquic_out_spec *packet)
{
    const struct iovec *iov = packet->iov;
    const struct iovec *const end = iov + packet->iovlen;
    size_t size;

    size = 0;
    while (iov < end)
    {
        size += iov->iov_len;
        ++iov;
    }

    return size;
}



int QuicEngine::sendPackets(
    void *pCtx, const struct lsquic_out_spec *specs, unsigned count)
{
    const struct lsquic_out_spec *spec, *end;
    ssize_t sent;

    for (spec = specs, end = specs + count; spec < end; ++spec)
    {
        UdpListener *pListener = (UdpListener *) spec->peer_ctx;
        LS_DBG("QuicEngine::sendPackets %zu bytes", spec_size(spec));
        sent = pListener->send(spec->iov, spec->iovlen, spec->local_sa,
                                            spec->dest_sa, spec->ecn);
        if (sent < 0)
        {
            if (spec == specs)
                return -1;
            else
                break;
        }
    }
    return spec - specs;
}


int QuicEngine::sendPacketsMmsg(
    void *pCtx, const struct lsquic_out_spec  *packets, unsigned count)
{
    UdpListener *pListener;
    if (count)
    {
        pListener = (UdpListener *)packets[0].peer_ctx;
        return pListener->sendPackets(packets, count);
    }
    else
        return 0;
}


void QuicEngine::onTimer()
{
    int token;

    onStreamTimer();
    token = NtwkIOLink::getToken();
    if (!token)
        QuicShm::getInstance().cleanExpiredCidItem();
}



void QuicEngine::onStreamTimer()
{
    int token;

    token = NtwkIOLink::getToken();
    TDLinkQueue<QuicStream> *pQue = &s_streamQueues[token];
    if (pQue->empty())
        return;
    QuicStream *pStream = pQue->begin();
    QuicStream *pNext;
    while(pStream && pStream != pQue->end())
    {
        pNext = (QuicStream *)pStream->next();
        pStream->onTimer();
        pStream = pNext;
    }
}



QuicEngine::QuicEngine()
    : m_pEngine(NULL)
    , m_pMultiplexer(NULL)
    , m_cooldown(0)
{
    memset(&m_config, 0, sizeof(m_config));
}


QuicEngine::~QuicEngine()
{
    s_isDestruct = 1;
    if (m_pEngine)
        lsquic_engine_destroy(m_pEngine);
    if (s_pid > 0)
        QuicShm::getInstance().cleanupPidShm(s_pid);
}


int QuicEngine::init(Multiplexer * pMplx, const char *pShmDir,
                             const struct lsquic_engine_settings *settings)
{
    LS_DBG_L("QuicEngine::init(), pid: %d.", getpid());
    if (m_pEngine)
        return LS_OK;

#if defined(__FreeBSD__) && !defined(_NOT_USE_SHM_)
    {
        const char *const name = "security.bsd.conservative_signals";
        int val;
        size_t sz = sizeof(val);
        if (0 == sysctlbyname(name, &val, &sz, NULL, 0))
        {
            if (0 != val)
            {
                LS_ERROR("cannot initialize QUIC engine, as it relies on "
                    "realtime signals.  Set %s to zero.  See kill(2) for "
                    "more information.", name);
                return LS_FAIL;
            }
            LS_DBG_L("%s is set to %d, OK", name, val);
        }
        else
            LS_WARN("sysctlbyname(%s): %s", name, strerror(errno));
    }
#endif

    if (settings)
        m_config = *settings;
    else
        lsquic_engine_init_settings(&m_config, LSENG_SERVER);
    m_config.es_max_header_list_size = 64 * 1024;
    m_config.es_rw_once = 1;
    m_config.es_send_prst = 1;

    lsquic_logger_init(&logger_if, log4cxx::Logger::getDefault(),
                                                    LLTS_YYYYMMDD_HHMMSSUS);
//     if (LS_LOG_ENABLED(log4cxx::Level::DBG_MEDIUM))
//         lsquic_set_log_level("debug");
//     else if (LS_LOG_ENABLED(log4cxx::Level::DBG_LESS))
//         lsquic_logger_lopt("event=debug,engine=debug");
//     lsquic_logger_lopt("pacer=info");
    bool isQuicLogEnable = (LS_LOG_ENABLED(log4cxx::Level::DBG_HIGH));
    setDebugLog(isQuicLogEnable);
    
    m_pMultiplexer = pMplx;

#ifndef _NOT_USE_SHM_
    if (0 != QuicShm::getInstance().init(pShmDir))
    {
        LS_WARN("could not initialize SHM: QUIC is disabled");
        return LS_FAIL;
    }
#else
    LS_DBG_H("[QuicEngine::init]_NOT_USE_SHM_ defined in quicshm.h, for testing only!!!!\n");
#endif //_NOT_USE_SHM_

    struct lsquic_engine_api api;
    memset(&api, 0, sizeof(api));
    api.ea_settings         = &m_config;
    api.ea_stream_if        = &s_quic_if;
    api.ea_stream_if_ctx    = this;
    api.ea_packets_out      = QuicEngine::sendPackets;
    api.ea_packets_out_ctx  = this;
    api.ea_get_ssl_ctx      = QuicEngine::getSslCtxCb;
    api.ea_lookup_cert      = QuicEngine::sniCb;
    api.ea_cert_lu_ctx      = this;
#ifndef _NOT_USE_SHM_
    api.ea_shi              = &s_quic_shi_if;
    api.ea_shi_ctx          = QuicShm::getInstance().getHashCtx();
#endif
    api.ea_new_scids        = QuicEngine::addNewSCIDs;
    api.ea_live_scids       = QuicEngine::touchSCIDs;
    api.ea_old_scids        = QuicEngine::removeOldSCIDs;
    api.ea_hsi_if           = &s_quic_hpack_hdr_callback;

    if (is_sendmmsg_available())
    {
        LS_DBG_H("[QuicEngine::init] enable sendmmsg().\n");
        api.ea_packets_out = QuicEngine::sendPacketsMmsg;
    }


    m_pEngine = lsquic_engine_new(LSENG_HTTP_SERVER, &api);
    if (!m_pEngine)
        return LS_FAIL;
    lsquic_global_init (LSQUIC_GLOBAL_SERVER);
    return LS_OK;
}


void QuicEngine::startCooldown()
{
    m_cooldown = 1;
    if (m_pEngine)
        lsquic_engine_cooldown(m_pEngine);
}


void QuicEngine::setDebugLog(int is_enable)
{
    if (is_enable)
    {
        lsquic_set_log_level("debug");
        lsquic_logger_lopt("pacer=info");
    }
    else
        lsquic_set_log_level("warn");
}


void QuicEngine::maybeProcessConns()
{
    int diff;
    if (lsquic_engine_earliest_adv_tick(m_pEngine, &diff))
        lsquic_engine_process_conns(m_pEngine);
}


void QuicEngine::sendUnsentPackets()
{
    lsquic_engine_send_unsent_packets(m_pEngine);
}


int QuicEngine::nextEventTime()
{
    int diff;
    if (lsquic_engine_earliest_adv_tick(m_pEngine, &diff))
    {
        if (diff >= 1000)
            return (diff + 500) / 1000;
        else if (diff >= 0)
            return diff > 0;
    }
    return -1;
}
