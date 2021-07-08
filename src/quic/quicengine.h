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
#ifndef QUICENGINE_H
#define QUICENGINE_H

#include <lsdef.h>
#include <lsquic.h>
#include <util/gpointerlist.h>
#include <util/dlinkqueue.h>

class Multiplexer;
class QuicStream;
class UdpListener;
class HttpListener;
class VHostMap;


typedef struct ssl_ctx_st *(*ssl_ctx_lookup_func)(struct ssl_ctx_st *, const char *);

class UdpListenerList: public TPointerList<UdpListener>
{
public:
    UdpListenerList()
    {}
    ~UdpListenerList();

    LS_NO_COPY_ASSIGN(UdpListenerList);
};

class QuicEngine
{
public:
    QuicEngine();
    ~QuicEngine();
    lsquic_engine_settings *getConfig()
    {   return &m_config;       }

    int init(Multiplexer * pMplx, const char *pShmDir,
             const struct lsquic_engine_settings *, const char *quicLogLevel);

    void startCooldown();

    Multiplexer *getMultiplexer() const     {   return m_pMultiplexer;  }
    lsquic_engine_t *getEngine() const      {   return m_pEngine;       }

    //UdpListener *startUdpListener(HttpListener *pTcpPeer, VHostMap *pMap);
    int registerUdpListener(UdpListener *pListener);
    UdpListener *getListener(int index) const;
    void onTimer();
    int nextEventTime();
    void maybeProcessConns();
    void sendUnsentPackets();

    void processEvents()
    {
        maybeProcessConns();
        recycleStreams();
    }

    static int sendPackets(void *pCtx,
                           const struct lsquic_out_spec *packets,
                           unsigned count);
    static int sendPacketsMmsg(void *pCtx,
                               const struct lsquic_out_spec  *packets,
                               unsigned count);
    static struct ssl_ctx_st * sniCb(void *pCtx, const sockaddr *pLocal,
                                     const char *sni);
    static struct ssl_ctx_st * getSslCtxCb(void *peer_ctx, const sockaddr *);

    static lsquic_conn_ctx_t *onNewConn(void *stream_if_ctx,
                                             lsquic_conn_t *c);
    static void onConnEstablished(lsquic_conn_t *c);
    static void onConnClosed(lsquic_conn_t *c);
    static void addNewSCIDs(void *ctx, void **peer_ctx,
                            const lsquic_cid_t *cids, unsigned count);
    static void touchSCIDs(void *ctx, void **peer_ctx,
                            const lsquic_cid_t *cids, unsigned count);
    static void removeOldSCIDs(void *ctx, void **peer_ctx,
                            const lsquic_cid_t *cids, unsigned count);

    static lsquic_stream_ctx_t *onNewStream(void *stream_if_ctx,
                                                 lsquic_stream_t *s);
    static void onStreamRead(lsquic_stream_t *s, lsquic_stream_ctx_t *h);
    static void onStreamWrite(lsquic_stream_t *s, lsquic_stream_ctx_t *h);
    static void onStreamClose(lsquic_stream_t *s, lsquic_stream_ctx_t *h);
    static void onStreamTimer();

    static void setDebugLog(int is_enable);

    static pid_t getpid()           {   return s_pid;   }
    static void  setpid( pid_t pid);
    int getAltSvcVerStr(unsigned short port, char *, size_t);

    static unsigned activeConnsCount(void)
    {   return s_active_conns;  }

    static void detectBusyLoop(int);

private:
    static void recycleStreams();

    lsquic_engine_t        *m_pEngine;
    Multiplexer            *m_pMultiplexer;
    lsquic_engine_settings  m_config;
    UdpListenerList         m_udpListeners;
    static unsigned         s_active_conns;

    static TDLinkQueue<QuicStream> s_streamQueues[10];
    static TPointerList<QuicStream> s_streamsTobeRecycled;

    static ssl_ctx_lookup_func s_extraLookup;

    static pid_t            s_pid;

    LS_NO_COPY_ASSIGN(QuicEngine);
};

#endif // QUICENGINE_H
