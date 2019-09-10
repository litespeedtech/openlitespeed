#ifndef UDPLISTENER_H
#define UDPLISTENER_H

#include <lsquic.h>
#include <edio/eventreactor.h>
#include <log4cxx/logsession.h>
#include <socket/gsockaddr.h>
#include <util/autostr.h>
#include <quic/pbset.h>
#include <quic/quicshm.h>   /* For _NOT_USE_SHM_ definition */

class QuicEngine;
class VHostMap;
class GHash;
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
        , m_pVHostMap(NULL)
        , m_id(-1)
        , m_pPacketsIn(NULL)
    {}

    explicit UdpListener(QuicEngine *pEngine, void *pTcpPeer, VHostMap *pMap)
        : EventReactor(-1)
        , m_pEngine(pEngine)
        , m_pTcpPeer(pTcpPeer)
        , m_pVHostMap(pMap)
        , m_id(-1)
        , m_pPacketsIn(NULL)
    {}
        
    ~UdpListener();

    int setAddr(const char *pAddr);

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
    void *getTcpPeer() const { return m_pTcpPeer; }
    unsigned short getPort() const { return m_addr.getPort(); }

    struct ssl_ctx_st *getSslContext() const;

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
    void *          m_pTcpPeer;
    VHostMap       *m_pVHostMap;
    GSockAddr       m_addr;
    int             m_id;

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
