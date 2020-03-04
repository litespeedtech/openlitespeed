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
#ifndef UDPLISTENER_H
#define UDPLISTENER_H

#include <lsquic.h>
#include <edio/eventreactor.h>
#include <log4cxx/logsession.h>
#include <socket/gsockaddr.h>
#include <util/autostr.h>
#include <quic/pbset.h>
#include <quic/quicshm.h>   /* For _NOT_USE_SHM_ definition */
#include <lsr/reuseport.h>

class QuicEngine;
class VHostMap;
class GHash;
class HttpListener;
struct CidInfo;
struct packets_in;
struct read_ctx;
struct read_iter;
struct quicshm_packet_buf;
enum rop { ROP_OK, ROP_NOROOM, ROP_ERROR, };    /* ROP: Read One Packet */

class UdpListener : public EventReactor, public LogSession
{
public:
    UdpListener()
        : EventReactor(-1)
        , m_pEngine(NULL)
        , m_pTcpPeer(NULL)
        , m_id(-1)
        , m_pPacketsIn(NULL)
    {}

    explicit UdpListener(QuicEngine *pEngine, HttpListener *pTcpPeer)
        : EventReactor(-1)
        , m_pEngine(pEngine)
        , m_pTcpPeer(pTcpPeer)
        , m_id(-1)
        , m_pPacketsIn(NULL)
    {}
        
    ~UdpListener();

    int setAddr(const char *pAddr);
    int setAddr(GSockAddr *);
    
    int start();
    
    ssize_t sendPacket(struct iovec *, size_t iovlen, const struct sockaddr *,
                                        const struct sockaddr *, int ecn);
    ssize_t send(struct iovec *, size_t iovlen, const struct sockaddr *src,
                                        const struct sockaddr *dest, int ecn);
    int sendPackets(const struct lsquic_out_spec *specs, unsigned count);

    virtual int handleEvents(short int event);

    int onRead();
    
    static void onMyEvent(int signo, void *param);
    
    QuicEngine *getEngine() { return m_pEngine; }

    virtual const char *buildLogId();

public:
    static void initCidListenerHt();
    static void deleteCidListenerEntry(const lsquic_cid_t *);
    CidInfo *updateCidListenerMap(const lsquic_cid_t *, ShmCidPidInfo *pShmInfo);
    
    void setId(int id)      {   m_id = id;        }
    int  getId() const      {   return m_id;      }
    HttpListener *getTcpPeer() const { return m_pTcpPeer; }
    unsigned short getPort() const { return m_addr.getPort(); }

    struct ssl_ctx_st *getSslContext() const;
    
    int setSockOptions(int fd);
    
    
    void initShmCidMapping();
    int beginServe(QuicEngine *pEngine);
    int bind2(int flag);
    int bind();
    
    
    int getFdCount(int* maxfd);
    int passFds(int target_fd, const char *addr_str);
    void addReusePortSocket(int fd, const char *addr_str);
    int registEvent();
    int activeReusePort(int seq, const char *addr_str);
    int bindReusePort(int s_children, const char* getAddrStr);

#ifndef _NOT_USE_SHM_
public: /* These need to be public because we access them from handlePackets */
    SetOfPacketBuffers    m_availPacketBufs;
    void releaseSomePacketsToSHM();
    void maybeReturnSomePacketsToSHM();
private:
    void allocatePacketsFromSHM();
    void replenishPacketBatchBuffers();
    int feedOwnedPacketToEngine(CidInfo *, unsigned);
    int feedOwnedPacketToEngine(UdpListener *,
                                            const struct quicshm_packet_buf *);
#endif

private:
    QuicEngine     *m_pEngine;
    HttpListener   *m_pTcpPeer;
    GSockAddr       m_addr;
    int             m_id;
    ReusePortFds        m_reusePortFds;

    static int      s_rtsigNo;

    struct packets_in
                   *m_pPacketsIn;
    int initPacketsIn();
    void cleanupPacketsIn();

    enum rop readOnePacket(struct read_iter *);
    void startReading(struct read_ctx *);
    void processPacketsInBatch(struct read_ctx *);
    void finishReading(struct read_ctx *);
};

#endif // UDPLISTENER_H
