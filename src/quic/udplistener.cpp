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
#define DEBUG_SHM_OFFSETS 0

#if defined(__APPLE__)
#   define __APPLE_USE_RFC_3542 1
#endif

#include "quicshm.h"
#include "udplistener.h"
#include "quicengine.h"
#include "packetbuf.h"
#include "pbset.h"
#include "ls_sendmmsg.h"
#include "quiclog.h"
#include <errno.h>
#include <netinet/ip.h>
#include <sys/time.h>
#include <edio/multiplexer.h>
#include <edio/multiplexerfactory.h>
#include <edio/sigeventdispatcher.h>
#include <socket/coresocket.h>
#include <util/ghash.h>
#include <log4cxx/logger.h>
#include <lsr/xxhash.h>
#include <http/vhostmap.h>
#include <http/httplistener.h>
#include <sslpp/sslcontext.h>

/* XXX: What happens if LS_HAS_RTSIG is not defined?  Should UDP Listener
 * be compiled at all in this case?
 */

#include <fcntl.h>
#include <stdio.h>

#ifndef _NOT_USE_SHM_
#   define BATCH_SIZE   100
#endif

#if __linux__ && defined(IP_RECVORIGDSTADDR)
#   define DST_MSG_SZ sizeof(struct sockaddr_in)
#elif __linux__
#   define DST_MSG_SZ sizeof(struct in_pktinfo)
#else
#   define DST_MSG_SZ sizeof(struct sockaddr_in)
#endif

#if __linux__ || defined(__FreeBSD__)
#define ECN_SUPPORTED 1
#else
#define ECN_SUPPORTED 0
#endif

#if ECN_SUPPORTED
#define ECN_SZ CMSG_SPACE(sizeof(int))
#else
#define ECN_SZ 0
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) > (b) ? (b) : (a))

#define CTL_SZ CMSG_SPACE(MAX(DST_MSG_SZ, sizeof(struct in6_pktinfo)) +  ECN_SZ)

int UdpListener::s_rtsigNo = -1;


static hash_key_t hash_cid(const void *__s)
{
    const lsquic_cid_t *cid = (const lsquic_cid_t *) __s;
    if (sizeof(hash_key_t) == 8)
        return XXH64(cid->idbuf, cid->len, 0);
    else
        return XXH32(cid->idbuf, cid->len, 0);
}

static int compare_cids(const void *p1, const void *p2)
{
    const lsquic_cid_t *const cid_a = (const lsquic_cid_t *) p1,
                       *const cid_b = (const lsquic_cid_t *) p2;
    size_t len;
    int s;

    if (sizeof(uint64_t) == cid_a->len && cid_a->len == cid_b->len)
    {
        /* This is the expected case */
        return (* (uint64_t *) cid_a->idbuf > * (uint64_t *) cid_b->idbuf)
             - (* (uint64_t *) cid_a->idbuf < * (uint64_t *) cid_b->idbuf);
    }
    else if (cid_a->len == cid_b->len)
        return memcmp(cid_a->idbuf, cid_b->idbuf, cid_a->len);
    else
    {
        len = MIN(cid_a->len, cid_b->len);
        s = memcmp(cid_a->idbuf, cid_b->idbuf, len);
        if (s == 0)
            return cid_a->len < cid_b->len ? -1 : 1;
        else
            return s;
    }
}



#ifndef _NOT_USE_SHM_
static unsigned
low_threshold  (unsigned n_alloc) { return n_alloc / 2; }

static unsigned
high_threshold (unsigned n_alloc) { return n_alloc * 3; }

static unsigned handlePackets();
void UdpListener::onMyEvent(int signo, void *param)
{
    QuicEngine *pEngine = (QuicEngine *) param;
    unsigned count;

    assert(signo == s_rtsigNo);
    LS_DBG_L("[UdpListener::onMyEvent] pid %d.", QuicEngine::getpid());

    count = handlePackets();
    if (count > 0)
        pEngine->maybeProcessConns();

}
#else
void UdpListener::onMyEvent(int signo)
{
}
#endif


#define MAX_PACKET_SZ 1472

/* There are `n_alloc' elements in `vecs', `local_addresses', and
 * `peer_addresses' arrays.  `ctlmsg_data' is n_alloc * CTL_SZ.  Each packets
 * gets a single `vecs' element that points somewhere into `packet_data'.
 *
 * `n_alloc' is calculated at run-time based on the socket's receive buffer
 * size.
 */
struct packets_in
{
#ifndef _NOT_USE_SHM_
    packet_buf_t           **packet_bufs;
    lsquic_cid_t            *cids;        /* Copied into separate array for speed */
#else
    unsigned char           *packet_data;
    struct iovec            *vecs;
    struct sockaddr_storage *local_addresses,
                            *peer_addresses;
    unsigned char           *ctlmsg_data;
    unsigned                 data_sz;
#endif
    unsigned                 n_avail;   /* n_avail = n_alloc in non-SHM mode */
    unsigned                 n_alloc;
};


int UdpListener::setAddr(GSockAddr *pSockAddr)
{
    m_addr = *pSockAddr;
    return 0;
}

int UdpListener::setAddr(const char *pAddr)
{
    if (m_addr.set(pAddr, 0))
        return errno;
    return 0;
}


const char *UdpListener::buildLogId()
{
    char sId[256] = "UDP:";
    m_addr.toString(&sId[4], 200);
    appendLogId(sId);
    return m_logId.ptr;
}


struct CidInfo
{
    lsquic_cid_t        cid;
    UdpListener        *listener;
    ShmCidPidInfo       shmInfo;
};

static THash<CidInfo *> *s_sCidListenerMap;


void UdpListener::initCidListenerHt()
{
    if(!s_sCidListenerMap)
    {
        s_sCidListenerMap = new THash<CidInfo *>(10, hash_cid, compare_cids);
        assert(s_sCidListenerMap);
    }
}


void UdpListener::deleteCidListenerEntry(const lsquic_cid_t *cid)
{
    THash<CidInfo *>::iterator it;

    if (s_sCidListenerMap)
    {
        it = s_sCidListenerMap->find(cid);
        if (it)
        {
            CidInfo *pInfo = it.second();
            s_sCidListenerMap->erase(it);
            delete pInfo;
            LS_DBG_MC("removed CID %" CID_FMT
                                " from CID/Listener hash", CID_BITS(cid));
        }
    }
}


int UdpListener::start()
{
    //unsigned versions;
    int saved_errno;
    int fd;//, n;

    int ret = CoreSocket::bind(m_addr, SOCK_DGRAM, &fd);
    if (ret != 0)
        return -1;
    int val = 1;
    if (AF_INET == m_addr.get()->sa_family)
        ret = setsockopt(fd, IPPROTO_IP,
#if __linux__ && defined(IP_RECVORIGDSTADDR)
                         IP_RECVORIGDSTADDR,
#elif __linux__ || __APPLE__
                         IP_PKTINFO,
#else
                         IP_RECVDSTADDR,
#endif
                         &val, sizeof(val));
    else
        ret = setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &val, sizeof(val));
    if (ret != 0)
    {
        close(fd);
        return -1;
    }
#if (__linux__ && !defined(IP_RECVORIGDSTADDR)) || __APPLE__
    /* Need to set IP_PKTINFO for sending */
    if (AF_INET == m_addr.get()->sa_family)
    {
        val = 1;
        ret = setsockopt(fd, IPPROTO_IP, IP_PKTINFO, &val, sizeof(val));
        if (0 != ret)
        {
            saved_errno = errno;
            close(fd);
            errno = saved_errno;
            return -1;
        }
    }
#elif IP_RECVDSTADDR != IP_SENDSRCADDR
    /* On FreeBSD, IP_RECVDSTADDR is the same as IP_SENDSRCADDR, but I do not
     * know about other BSD systems.
     */
    if (AF_INET == m_addr.get()->sa_family)
    {
        val = 1;
        ret = setsockopt(fd, IPPROTO_IP, IP_SENDSRCADDR, &val, sizeof(val));
        if (0 != ret)
        {
            saved_errno = errno;
            close(fd);
            errno = saved_errno;
            return -1;
        }
    }
#endif

#if ECN_SUPPORTED
    val = 1;
    if (AF_INET == m_addr.get()->sa_family)
        ret = setsockopt(fd, IPPROTO_IP, IP_RECVTOS, &val, sizeof(val));
    else
        ret = setsockopt(fd, IPPROTO_IPV6, IPV6_RECVTCLASS, &val, sizeof(val));
    if (0 != ret)
    {
        saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }
#endif

    val = 1 * 1024 * 1024;
    ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &val, sizeof(val));
    if (ret != 0)
    {
        LS_WARN(this, "cannot set receive buffer to %d bytes: %s", val,
                  strerror(errno));
    }

    ::fcntl(fd, F_SETFD, FD_CLOEXEC);
    ::fcntl(fd, F_SETFL, MultiplexerFactory::getMultiplexer()->getFLTag());

    setfd(fd);

#ifndef  _NOT_USE_SHM_
    UdpListener::initCidListenerHt();
#if defined(LS_HAS_RTSIG)
    if (s_rtsigNo == -1)
        s_rtsigNo = SigEventDispatcher::getInstance().
                        registerRtsig(onMyEvent, m_pEngine, true);
#endif

#endif
    initPacketsIn();

    ret = registEvent();
    return ret;
}


/* Replace IP address part of `sa' with that provided in ancillary messages
 * in `msg'.
 */
static void
proc_ancillary (struct msghdr *msg, struct sockaddr_storage *storage
#if ECN_SUPPORTED
                                                            , uint8_t *ecn
#endif
    )
{
    const struct in6_pktinfo *in6_pkt;
    struct cmsghdr *cmsg;

    for (cmsg = CMSG_FIRSTHDR(msg); cmsg; cmsg = CMSG_NXTHDR(msg, cmsg))
    {
        if (cmsg->cmsg_level == IPPROTO_IP &&
            cmsg->cmsg_type  ==
#if __linux__ && defined(IP_RECVORIGDSTADDR)
                                IP_ORIGDSTADDR
#elif __linux__ || __APPLE__
                                IP_PKTINFO
#else
                                IP_RECVDSTADDR
#endif
                                              )
        {
#if __linux__ && defined(IP_RECVORIGDSTADDR)
            memcpy(storage, CMSG_DATA(cmsg), sizeof(struct sockaddr_in));
#elif __linux__ || __APPLE__
            const struct in_pktinfo *in_pkt;
            in_pkt = (struct in_pktinfo *) CMSG_DATA(cmsg);
            ((struct sockaddr_in *) storage)->sin_addr = in_pkt->ipi_addr;
#else
            memcpy(&((struct sockaddr_in *) storage)->sin_addr,
                            CMSG_DATA(cmsg), sizeof(struct in_addr));
#endif
        }
        else if (cmsg->cmsg_level == IPPROTO_IPV6 &&
                 cmsg->cmsg_type  == IPV6_PKTINFO)
        {
            in6_pkt = (struct in6_pktinfo *) CMSG_DATA(cmsg);
            ((struct sockaddr_in6 *) storage)->sin6_addr =
                                                    in6_pkt->ipi6_addr;
        }
#if ECN_SUPPORTED
        else if ((cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TOS)
                 || (cmsg->cmsg_level == IPPROTO_IPV6
                                            && cmsg->cmsg_type == IPV6_TCLASS))
        {
            int tos;
            memcpy(&tos, CMSG_DATA(cmsg), sizeof(tos));
            *ecn = tos & IPTOS_ECN_MASK;
        }
#ifdef __FreeBSD__
        else if (cmsg->cmsg_level == IPPROTO_IP
                                            && cmsg->cmsg_type == IP_RECVTOS)
        {
            unsigned char tos;
            memcpy(&tos, CMSG_DATA(cmsg), sizeof(tos));
            *ecn = tos & IPTOS_ECN_MASK;
        }
#endif
#endif
    }
}


#ifndef _NOT_USE_SHM_
/* Returns number of packets fed to lsquic engine */
static unsigned handlePackets()
{
    packet_buf_t *packet_buf;
    lsquic_cid_t cid;
    THash<CidInfo *>::iterator it;
    UdpListener *pListener;
    QuicShm::PacketBufIter iter =
                        QuicShm::getInstance().getPendingPacketBuffers();

    int ret;
    unsigned count = 0;
    while ((packet_buf = iter.nextPacketBuf()))
    {
        /* The second time around, the CID is expected to be there, as we
         * have already checked the packet.
         */
        cid.len = 0;
        (void) lsquic_cid_from_packet(packet_buf->data,
                                                packet_buf->data_len, &cid);
        it = s_sCidListenerMap->find(&cid);

        if (it == NULL)
        {   /* This is normal and can happen in one of two ways:
             *  I.  Packets are reordered in this batch and CONNECTION_CLOSE
             *      gets ahead of some packets that should precede it.
             * II.  Connection has been closed during previous iteration, but
             *      other processes appended to the queue while previous batch
             *      was being processed.
             */
            LS_DBG_LC("%s() caller pid %d: cannot "
                "find CID %" CID_FMT " in local cache", __func__,
                QuicEngine::getpid(), CID_BITS(&cid));
            QuicShm::getInstance().releasePacketBuffer(packet_buf);
        }
        else
        {
            LS_DBG_LC("%s() caller pid %d CID %" CID_FMT, __func__,
                QuicEngine::getpid(), CID_BITS(&cid));
            pListener = it.second()->listener;
            ret = lsquic_engine_packet_in(pListener->getEngine()->getEngine(),
                        packet_buf->data, packet_buf->data_len,
                        (const struct sockaddr *) &packet_buf->local_addr,
                        (const struct sockaddr *) &packet_buf->peer_addr,
                        pListener, packet_buf->ecn);
            count += ret == 0;
            if (ret < 0)
            {
                LS_WARN("handlePackets lsquic_engine_packet_in failed: %s",
                                                            strerror(errno));
            }
            if (pListener->m_availPacketBufs.insertPacketBuf(packet_buf))
                pListener->maybeReturnSomePacketsToSHM();
            else
                QuicShm::getInstance().releasePacketBuffer(packet_buf);
        }
    }

    return count;
}
#endif


enum ctl_what
{
    CW_SENDADDR     = 1 << 0,
#if ECN_SUPPORTED
    CW_ECN          = 1 << 1,
#endif
};


static void
setup_control_msg (struct msghdr *msg, int cw, int ecn,
    const struct sockaddr *local_sockaddr, unsigned char *buf, size_t bufsz)
{
    struct cmsghdr *cmsg;
    struct sockaddr_in *local_sa;
    struct sockaddr_in6 *local_sa6;
#if __linux__ || __APPLE__
    struct in_pktinfo info;
#endif
    struct in6_pktinfo info6;
    size_t ctl_len;

    msg->msg_control    = buf;
    msg->msg_controllen = bufsz;

    /* Need to zero the buffer due to a bug(?) in CMSG_NXTHDR.  See
     * https://stackoverflow.com/questions/27601849/cmsg-nxthdr-returns-null-even-though-there-are-more-cmsghdr-objects
     */
    memset(buf, 0, bufsz);

    ctl_len = 0;
    for (cmsg = CMSG_FIRSTHDR(msg); cw && cmsg; cmsg = CMSG_NXTHDR(msg, cmsg))
    {
        if (cw & CW_SENDADDR)
        {
	    if (AF_INET == local_sockaddr->sa_family)
	    {
		local_sa = (struct sockaddr_in *) local_sockaddr;
#if __linux__ || __APPLE__
		memset(&info, 0, sizeof(info));
		info.ipi_spec_dst = local_sa->sin_addr;
		cmsg->cmsg_level    = IPPROTO_IP;
		cmsg->cmsg_type     = IP_PKTINFO;
		cmsg->cmsg_len      = CMSG_LEN(sizeof(info));
                ctl_len += CMSG_SPACE(sizeof(info));
		memcpy(CMSG_DATA(cmsg), &info, sizeof(info));
#else
		cmsg->cmsg_level    = IPPROTO_IP;
		cmsg->cmsg_type     = IP_SENDSRCADDR;
		cmsg->cmsg_len      = CMSG_LEN(sizeof(local_sa->sin_addr));
                ctl_len += CMSG_SPACE(sizeof(local_sa->sin_addr));
		memcpy(CMSG_DATA(cmsg), &local_sa->sin_addr,
						    sizeof(local_sa->sin_addr));
#endif
	    }
	    else
	    {
		local_sa6 = (struct sockaddr_in6 *) local_sockaddr;
		memset(&info6, 0, sizeof(info6));
		info6.ipi6_addr = local_sa6->sin6_addr;
		cmsg->cmsg_level    = IPPROTO_IPV6;
		cmsg->cmsg_type     = IPV6_PKTINFO;
		cmsg->cmsg_len      = CMSG_LEN(sizeof(info6));
                ctl_len += CMSG_SPACE(sizeof(info6));
		memcpy(CMSG_DATA(cmsg), &info6, sizeof(info6));
	    }
            cw &= ~CW_SENDADDR;
        }
#if ECN_SUPPORTED
        else if (cw & CW_ECN)
        {
            if (AF_INET == local_sockaddr->sa_family)
            {
                const
#if defined(__FreeBSD__)
                      unsigned char
#else
                      int
#endif
                            tos = ecn;
                cmsg->cmsg_level = IPPROTO_IP;
                cmsg->cmsg_type  = IP_TOS;
                cmsg->cmsg_len   = CMSG_LEN(sizeof(tos));
                memcpy(CMSG_DATA(cmsg), &tos, sizeof(tos));
                ctl_len += CMSG_SPACE(sizeof(tos));
            }
            else
            {
                const int tos = ecn;
                cmsg->cmsg_level = IPPROTO_IPV6;
                cmsg->cmsg_type  = IPV6_TCLASS;
                cmsg->cmsg_len   = CMSG_LEN(sizeof(tos));
                memcpy(CMSG_DATA(cmsg), &tos, sizeof(tos));
                ctl_len += CMSG_SPACE(sizeof(tos));
            }
            cw &= ~CW_ECN;
        }
#endif
        else
            assert(0);
    }

    msg->msg_controllen = ctl_len;
}


ssize_t UdpListener::sendPacket(struct iovec *iov, size_t iovlen,
            const struct sockaddr *src, const struct sockaddr *dest, int ecn)
{
    struct msghdr msg;
    union {
        /* cmsg(3) recommends union for proper alignment */
        unsigned char buf[ CMSG_SPACE(
                                MAX(
#if __linux__
                                      sizeof(struct in_pktinfo)
#else
                                      sizeof(struct in_addr)
#endif
                                        , sizeof(struct in6_pktinfo))
#if ECN_SUPPORTED
            + ECN_SZ
#endif
                                                                )];
        struct cmsghdr cmsg;
    } ancil;
    socklen_t socklen;
    int cw;

    socklen = AF_INET == dest->sa_family ?
                    sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
    msg.msg_name        = (void *) dest;
    msg.msg_namelen     = socklen;
    msg.msg_iov         = iov;
    msg.msg_iovlen      = iovlen;
    msg.msg_flags       = 0;

    if (src->sa_family)
        cw = CW_SENDADDR;
    else
        cw = 0;
#if ECN_SUPPORTED
    if (ecn)
        cw |= CW_ECN;
#endif
    if (cw)
        setup_control_msg(&msg, cw, ecn, src, ancil.buf, sizeof(ancil.buf));
    else
    {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
    }

    return sendmsg(getfd(), &msg, 0);
}


ssize_t UdpListener::send(struct iovec *iov, size_t iovlen,
            const struct sockaddr *src, const struct sockaddr *dest, int ecn)
{
    if (getEvents() & POLLOUT)
        return -1;
    ssize_t ret = sendPacket(iov, iovlen, src, dest, ecn);
    if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS))
    {
        MultiplexerFactory::getMultiplexer()->continueWrite(this);
    }
    return ret;
}


int UdpListener::sendPackets(const struct lsquic_out_spec *spec,
                             unsigned count)
{
#if __linux__
    const struct lsquic_out_spec *const end = spec + count;
    unsigned i;
    int cw;
    struct mmsghdr mmsgs[1024];
    union {
        /* cmsg(3) recommends union for proper alignment */
        unsigned char buf[ CMSG_SPACE(
            MAX(
#if __linux__
                                        sizeof(struct in_pktinfo)
#else
                                        sizeof(struct in_addr)
#endif
                                        , sizeof(struct in6_pktinfo))
#if ECN_SUPPORTED
            + ECN_SZ
#endif
                                                                  ) ];
        struct cmsghdr cmsg;
    } ancil [ sizeof(mmsgs) / sizeof(mmsgs[0]) ];

    if (getEvents() & POLLOUT)
        return -1;

    for (i = 0; spec < end && i < sizeof(mmsgs) / sizeof(mmsgs[0]); ++i, ++spec)
    {
        mmsgs[i].msg_hdr.msg_name       = (void *) spec->dest_sa;
        mmsgs[i].msg_hdr.msg_namelen    = (AF_INET == spec->dest_sa->sa_family ?
                                            sizeof(struct sockaddr_in) :
                                            sizeof(struct sockaddr_in6)),
        mmsgs[i].msg_hdr.msg_iov        = spec->iov;
        mmsgs[i].msg_hdr.msg_iovlen     = spec->iovlen;
        mmsgs[i].msg_hdr.msg_flags      = 0;
        if (spec->local_sa->sa_family)
            cw = CW_SENDADDR;
        else
            cw = 0;
#if ECN_SUPPORTED
        if (spec->ecn)
            cw |= CW_ECN;
#endif
        if (cw)
            setup_control_msg(&mmsgs[i].msg_hdr, cw, spec->ecn, spec->local_sa,
                                    ancil[i].buf, sizeof(ancil[i].buf));
        else
        {
            mmsgs[i].msg_hdr.msg_control = NULL;
            mmsgs[i].msg_hdr.msg_controllen = 0;
        }
    }

    int ret = ls_sendmmsg(getfd(), mmsgs, i, 0);
    if (ret < (int) count)
    {
        if (errno == EPERM)
            ret = count;
        else if (errno != EMSGSIZE)
        {
            int err = errno;
            MultiplexerFactory::getMultiplexer()->continueWrite(this);
            if (ret > 0)
                errno = EAGAIN;
            else
                errno = err;
        }
    }
    return ret;
#else
    return -1;
#endif //__linux__
}


struct read_iter
{
    unsigned                 ri_idx;    /* Current element */
#ifdef _NOT_USE_SHM_
    unsigned                 ri_off;    /* Offset into packet_data */
#endif
};


struct read_ctx
{
    struct read_iter    rc_riter;
    bool                rc_have_own;
};


#ifndef _NOT_USE_SHM_
#if DEBUG_SHM_OFFSETS
static void
pbs2offstr (const packet_buf_t *const *packet_bufs, unsigned count, char *buf,
             size_t bufsz)
{
    unsigned n, off;
    int w;

    for (n = 0, off = 0; n < count; ++n, off += w)
    {
        w = snprintf(buf + off, bufsz - off, "%u%s", packet_bufs[n]->qpb_off,
                                                    n < count - 1 ? ", " : "");
        if (w < 0)
            break;
    }
}
#endif


void UdpListener::maybeReturnSomePacketsToSHM()
{
    if (m_pPacketsIn->n_avail + m_availPacketBufs.count() >
                                        high_threshold(m_pPacketsIn->n_alloc))
        releaseSomePacketsToSHM();
}


void UdpListener::releaseSomePacketsToSHM()
{
    unsigned count_before;
    count_before = m_availPacketBufs.count();
    QuicShm::getInstance().releasePacketBuffers(m_availPacketBufs,
                                                    m_pPacketsIn->n_alloc);
    LS_DBG_M("[UdpListener::releaseSomePacketsToSHM] released %u packets "
                    "back to SHM",  m_availPacketBufs.count() - count_before);
}


void UdpListener::allocatePacketsFromSHM()
{
    unsigned n;
    n = QuicShm::getInstance().getNewPacketBuffers(m_availPacketBufs,
                                                        m_pPacketsIn->n_alloc);
    LS_DBG_M("[UdpListener::allocatePacketsFromSHM] allocated %u packet "
            "buffers from SHM out of %u requested", n, m_pPacketsIn->n_alloc);
}


void UdpListener::replenishPacketBatchBuffers()
{
    if (m_pPacketsIn->n_alloc - m_pPacketsIn->n_avail >
                                                m_availPacketBufs.count())
        allocatePacketsFromSHM();

    while (m_pPacketsIn->n_avail < m_pPacketsIn->n_alloc)
    {
        m_pPacketsIn->packet_bufs[m_pPacketsIn->n_avail] =
                                        m_availPacketBufs.removePacketBuf();
        if (!m_pPacketsIn->packet_bufs[m_pPacketsIn->n_avail])
            break;
        ++m_pPacketsIn->n_avail;
    }
}
#endif


void UdpListener::startReading(struct read_ctx *rctx)
{
    memset(rctx, 0, sizeof(*rctx));
}


#ifndef _NOT_USE_SHM_
/* Returns 0 if packet was given to a real connection object, -1 othersise */
int UdpListener::feedOwnedPacketToEngine(UdpListener *pListener,
                                const struct quicshm_packet_buf *packet_buf)
{
    int s;

    s = lsquic_engine_packet_in(pListener->getEngine()->getEngine(),
            packet_buf->data,
            packet_buf->data_len,
            (struct sockaddr *) &packet_buf->local_addr,
            (struct sockaddr *) &packet_buf->peer_addr,
            pListener, packet_buf->ecn);

    switch (s)
    {
    case 0:
        return 0;
    default:
        LS_WARN("unexpected return value from lsquic: %d", s);
        // fallthru
    case  1:
    case -1:
        return -1;
    }
}
#endif


CidInfo *UdpListener::updateCidListenerMap(const lsquic_cid_t *cid, ShmCidPidInfo *pShmInfo)
{
    THash<CidInfo *>::iterator it;
    CidInfo *pInfo;

    pInfo = new CidInfo();
    pInfo->listener = this;
    pInfo->cid = *cid;
    pInfo->shmInfo = *pShmInfo;
    it = s_sCidListenerMap->insert(&pInfo->cid, pInfo);
    if (it != s_sCidListenerMap->end())
        LS_DBG_MC("saved cid %" CID_FMT " -> %p", CID_BITS(cid), this);
    else
    {
        delete pInfo;
        it = s_sCidListenerMap->find(cid);
        assert(it != s_sCidListenerMap->end());
        pInfo = it.second();
        LS_DBG_MC("not saving CID %" CID_FMT " again, return %p",
                                                CID_BITS(cid), this);
    }

    return pInfo;
}


void UdpListener::finishReading(struct read_ctx *rctx)
{
    if (rctx->rc_have_own)
        m_pEngine->maybeProcessConns();
#ifndef _NOT_USE_SHM_
    maybeReturnSomePacketsToSHM();
#endif
}


#ifndef _NOT_USE_SHM_

/* When number of unique CIDs in a batch reaches this threshold, we give up
 * linear search in favor of sorting all CIDs and then compressing that list
 * into a set of unique CIDs.  Note that qsort(3) calls malloc.
 */
#define UCID_THRESH 128


struct cid_and_index
{
    lsquic_cid_t    cai_cid;
    unsigned        cai_idx;
};


static int
compare_cais (const void *ap, const void *bp)
{
    const struct cid_and_index *a = (struct cid_and_index *) ap;
    const struct cid_and_index *b = (struct cid_and_index *) bp;

    return compare_cids(&a->cai_cid, &b->cai_cid);
}


void UdpListener::processPacketsInBatch(struct read_ctx *rctx)
{
    int s;
    unsigned n, n_uniq_cids, n_uniq_pids;
    const lsquic_cid_t *cid;
    packet_buf_t **const packet_bufs = m_pPacketsIn->packet_bufs;   /* Shorthand */
    lsquic_cid_t uniq_cids[rctx->rc_riter.ri_idx];
    int uniq_cid_idxs[rctx->rc_riter.ri_idx];   /* Dual-use: points into `pids', too */
#if DEBUG_SHM_OFFSETS
    char offstr[0x400];
#endif

#if DEBUG_SHM_OFFSETS
    LS_DBG_L(this, "%s: batch has %u packets, "
        "offsets: (%s)", __func__, rctx->rc_riter.ri_idx,
        (pbs2offstr(packet_bufs, rctx->rc_riter.ri_idx, offstr,
                                                sizeof(offstr)), offstr));
#endif

    /* Collects all unique CIDs into `uniq_cids': */
    n_uniq_cids = 0;
    for (n = 0; n < rctx->rc_riter.ri_idx && n_uniq_cids < UCID_THRESH; ++n)
    {
        uniq_cids[ n_uniq_cids ] = m_pPacketsIn->cids[n];
        for (cid = uniq_cids; !LSQUIC_CIDS_EQ(cid, &uniq_cids[ n_uniq_cids ]); ++cid)
            ;
        if (cid == &uniq_cids[ n_uniq_cids ])
            ++n_uniq_cids;
        uniq_cid_idxs[n] = cid - uniq_cids;
    }
    if (n < rctx->rc_riter.ri_idx)
    {
        /* Instead of linear search, which may become too slow, do something
         * more elaborate:
         */
        struct cid_and_index cais[rctx->rc_riter.ri_idx];
        for (n = 0; n < rctx->rc_riter.ri_idx; ++n)
        {
            cais[n].cai_cid = m_pPacketsIn->cids[n];
            cais[n].cai_idx = n;
        }
        qsort(cais, rctx->rc_riter.ri_idx, sizeof(cais[0]), compare_cais);
        n_uniq_cids = 0;
        for (n = 0; n < rctx->rc_riter.ri_idx; ++n)
        {
            if (n == 0 || !LSQUIC_CIDS_EQ(&cais[n].cai_cid, &uniq_cids[n_uniq_cids - 1]))
            {
                uniq_cids[ n_uniq_cids ] = cais[n].cai_cid;
                ++n_uniq_cids;
            }
            uniq_cid_idxs[ cais[n].cai_idx ] = n_uniq_cids - 1;
        }
    }
    LS_DBG_L(this, "%s: %u unique cid%.*s", __func__,
        n_uniq_cids, n_uniq_cids != 1, "s");

    ShmCidPidInfo pids[ n_uniq_cids ];
    pid_t uniq_pids[ n_uniq_cids ];
    unsigned cid2packet_counts[ n_uniq_cids ];
    signed char unknown_cids[ n_uniq_cids ];
    CidInfo *cid_infos[ n_uniq_cids ];
    int have_unknown_cids, have_cached_cids;

    memset(pids, 0, sizeof(pids));
    memset(cid2packet_counts, 0, sizeof(cid2packet_counts));
    have_unknown_cids = 0;
    have_cached_cids = 0;
    for (n = 0; n < n_uniq_cids; ++n)
    {
        THash<CidInfo *>::iterator iter = s_sCidListenerMap->find(&uniq_cids[n]);
        if (iter != s_sCidListenerMap->end())
        {
            /* This must mean that we are the owner.  If we a lucky, we will
             * not have to call lookupCIDPids().
             */
            assert(iter.second()->shmInfo.pid == QuicEngine::getpid());
            pids[n] = iter.second()->shmInfo;
            cid_infos[n] = iter.second();
            unknown_cids[n] = 0;
            ++have_cached_cids;
        }
        else
        {
            pids[n].pid = 0;    /* This signifies unknown pid to lookupCIDPids()
                                 * and to maybeUpdateLruCidItems().
                                 */
            cid_infos[n] = NULL;
            unknown_cids[n] = 1;
            have_unknown_cids = 1;
        }
    }
    /* Call maybeUpdateLruCidItems() before lookupCidPids(), as the latter
     * updates LRU of items it finds.
     */
    if (have_cached_cids)
        QuicShm::getInstance().maybeUpdateLruCidItems(pids, n_uniq_cids);
    if (have_unknown_cids)
        QuicShm::getInstance().lookupCidPids(uniq_cids, pids, n_uniq_cids);

    /* Get a list of unique pids.  Unlike CIDs, the number of PIDs is expected
     * to be relatively small, so we always use linear search.
     */
    const pid_t *pid;
    n_uniq_pids = 0;
    for (n = 0; n < n_uniq_cids; ++n)
    {
        uniq_pids[ n_uniq_pids ] = pids[n].pid;
        for (pid = uniq_pids; *pid != uniq_pids[ n_uniq_pids ]; ++pid)
            ;
        if (pid == &uniq_pids[ n_uniq_pids ])
            ++n_uniq_pids;
        rctx->rc_have_own |= pids[n].pid == QuicEngine::getpid();
    }
    LS_DBG_L(this, "%s: %u unique pid%.*s", __func__,
        n_uniq_pids, n_uniq_pids != 1, "s");

    unsigned alien_packet_idxs[rctx->rc_riter.ri_idx];
    ShmCidPidInfo *alien_pids[rctx->rc_riter.ri_idx];
    unsigned n_alien_packets;
    int maybe_have_bad_cids;

    /* Look through the packets: process packets that belong to us immedately
     * and record indexes of alien packets.
     */
    n_alien_packets = 0;
    maybe_have_bad_cids = 0;
    for (n = 0; n < rctx->rc_riter.ri_idx; ++n)
    {
        if (pids[ uniq_cid_idxs[n]].pid == QuicEngine::getpid())
        {
            if (cid_infos[uniq_cid_idxs[n]] == NULL)
            {
                if (!lsquic_is_valid_hs_packet(this->getEngine()->getEngine(),
                              packet_bufs[n]->data, packet_bufs[n]->data_len))
                {
                    maybe_have_bad_cids = 1;
                    continue;
                }
                cid_infos[uniq_cid_idxs[n]] = updateCidListenerMap(
                    &uniq_cids[ uniq_cid_idxs[n] ], &pids[uniq_cid_idxs[n]]);
            }
            s = feedOwnedPacketToEngine(cid_infos[uniq_cid_idxs[n]]->listener,
                                                                packet_bufs[n]);
            cid2packet_counts[ uniq_cid_idxs[n] ] += s == 0;
            maybe_have_bad_cids |= s != 0;
        }
        else if (pids[ uniq_cid_idxs[n] ].pid > 0)
        {
            alien_packet_idxs[ n_alien_packets ] = n;
            alien_pids[ n_alien_packets ] = &pids[ uniq_cid_idxs[n] ];
            ++n_alien_packets;
        }
        else
        {
            switch(pids[ uniq_cid_idxs[n] ].pid)
            {
            case -1:
                LS_DBG_MC(this, "%s: skip packet %u: "
                    "CID %" CID_FMT ", connection closed", __func__, n,
                    CID_BITS(&uniq_cids[ uniq_cid_idxs[n] ]));
                break;
            case -2:
                s = feedOwnedPacketToEngine(this, packet_bufs[n]);
                /* We let the lsquic engine generate and schedule stateless
                 * resets.
                 */
                LS_DBG_MC(this, "%s: handling process died for packet %u: "
                    "CID %" CID_FMT ", engine %s send stateless reset",
                    __func__, n, CID_BITS(&uniq_cids[ uniq_cid_idxs[n] ]),
                    s == 1 ? "will" : "will not");
                pids[ uniq_cid_idxs[n] ].pid = -3;
                break;
            case -3:
                LS_DBG_MC(this, "%s: skip packet %u: "
                    "CID %" CID_FMT ", already sent PUBLIC_RESET.", __func__,
                    n, CID_BITS(&uniq_cids[ uniq_cid_idxs[n] ]));
                break;
            default:
                /* Log this because there was an error inserting these CIDs: */
                LS_DBG_MC(this, "%s: skip packet %u: "
                    "CID %" CID_FMT " does not have an entry", __func__, n,
                    CID_BITS(&uniq_cids[ uniq_cid_idxs[n] ]));
                break;
            }
        }
    }

    /* If some of packets for presumably new connections did not refer to a
     * valid connection, we may have to clean up our record.  This is done
     * by looking through the packet per connection counts.  If new connection
     * entry has had zero valid packets, then there is no connection and we
     * remove it.
     */
    if (maybe_have_bad_cids)
    {
        lsquic_cid_t bad_cids[ n_uniq_cids ];
        unsigned n_bad_cids = 0;
        THash<CidInfo *>::iterator it;
        for (n = 0; n < n_uniq_cids; ++n)
        {
            if (cid2packet_counts[n] == 0
                    /* This means that this is never-before-seen cid: */
                && unknown_cids[n] && pids[n].pid == QuicEngine::getpid())
            {
                LS_DBG_LC(this, "%s: clean up bad CID %" CID_FMT, __func__,
                         CID_BITS(&uniq_cids[n]));
                if (cid_infos[n])
                {
                    it = s_sCidListenerMap->find(&uniq_cids[n]);
                    if (it)
                    {
                        CidInfo *pInfo = it.second();
                        s_sCidListenerMap->erase(it);
                        delete pInfo;
                    }
                    else
                    {
                        assert(it);
                    }
                }
                bad_cids[n_bad_cids++] = uniq_cids[n];
            }
        }
        if (n_bad_cids > 0)
            QuicShm::getInstance().deleteCidItems(bad_cids, n_bad_cids);
    }

    /* Move all alien offsets to the end of shm_offsets array:
     *
     *  shm_offsets: M M A M A M A A
     *
     *    becomes
     *
     *  shm_offsets: M M M M A A A A
     *
     * Since we've already processed all of our (or "my": M) packets, we do
     * not care about maintaining the ordering of M elements, while A elements
     * do stay in order.
     *
     * This allows us to do two things:
     *  1. Use the alien offsets subarray as argument to distributeAlienPackets();
     *  2. Truncate the list of available packets in the packet batch.
     *
     * WARNING: at the end of the loop, contents of alien_packet_idxs are
     * invalid.
     */
    if (n_alien_packets < m_pPacketsIn->n_avail && n_alien_packets > 0)
    {
        unsigned alien_idx = n_alien_packets;
        unsigned all_idx = m_pPacketsIn->n_avail;
        do
        {
            --alien_idx;
            --all_idx;
            if (alien_packet_idxs[ alien_idx ] < all_idx)
            {
                packet_buf_t *tmp_packet_buf;
                /* Swap */
                tmp_packet_buf = packet_bufs[all_idx];
                packet_bufs[all_idx] = packet_bufs[ alien_packet_idxs[ alien_idx ] ];
                packet_bufs[ alien_packet_idxs[ alien_idx ] ] = tmp_packet_buf;
            }
            else
            {
                assert(alien_packet_idxs[ alien_idx ] == all_idx);
            }
        }
        while (alien_idx > 0);
#if DEBUG_SHM_OFFSETS
        LS_DBG_L(this, "%s: offsets after alien shuffling: (%s)", __func__,
                 (pbs2offstr(packet_bufs, m_pPacketsIn->n_avail, offstr,
                             sizeof(offstr)), offstr));
#endif
    }

    pid_t pids_to_signal[n_uniq_pids];
    unsigned n_pids_to_signal;

    /* Distribute alien packets to responsible processes */
    if (n_alien_packets)
    {
#if DEBUG_SHM_OFFSETS
        LS_DBG_L(this, "%s: %u alien packets, offsets: (%s)", __func__,
                 n_alien_packets,
            (pbs2offstr(packet_bufs + m_pPacketsIn->n_avail - n_alien_packets,
                           n_alien_packets, offstr, sizeof(offstr)), offstr));
#endif
        n_pids_to_signal = sizeof(pids_to_signal) / sizeof(pids_to_signal[0]);
        n = QuicShm::getInstance().distributeAlienPackets(&alien_pids[0],
            packet_bufs + m_pPacketsIn->n_avail - n_alien_packets,
            n_alien_packets, pids_to_signal, &n_pids_to_signal);
        if (n < n_alien_packets)
        {
            LS_WARN(this, "%s: could only append %u out of %u packets",
                    __func__, n, n_alien_packets);
            if (n)
                memmove(
                    packet_bufs + m_pPacketsIn->n_avail - n_alien_packets,
                    packet_bufs + m_pPacketsIn->n_avail - n_alien_packets + n,
                    sizeof(packet_bufs[0]) * (n_alien_packets - n));
            n_alien_packets = n;
        }
    }
    else
        n_pids_to_signal = 0;

    /* Truncate available list of packets: */
    m_pPacketsIn->n_avail -= n_alien_packets;
#if DEBUG_SHM_OFFSETS
    LS_DBG_L(this, "%s: n_avail is now %u", __func__, m_pPacketsIn->n_avail);
#endif

    /* Notify other processes */
    for (n = 0; n < n_pids_to_signal; ++n)
        if (0 == kill(pids_to_signal[n], s_rtsigNo))
            LS_DBG_L(this, "%s: my pid %d successfully sent a sig to pid %d",
                     __func__, QuicEngine::getpid(), pids_to_signal[n]);
        else if (errno == ESRCH || errno == EPERM)
        {   /* EPERM is handled here because it is possible that a
             * process crashed and its PID was reused by another,
             * unrelated, process.
             */
            LS_DBG_L(this, "%s: pid %d seems to be dead, clean up after it",
                     __func__, pids_to_signal[n]);
            QuicShm::getInstance().cleanupPidShm(pids_to_signal[n]);
        }
}

#else
/* This is non-SHM version: */
void UdpListener::processPacketsInBatch(struct read_ctx *rctx)
{
    unsigned n, ok_count;
    int s;

    ok_count = 0;
    for (n = 0; n < rctx->rc_riter.ri_idx; ++n)
    {
        s = lsquic_engine_packet_in(this->getEngine()->getEngine(),
                (const unsigned char *) m_pPacketsIn->vecs[n].iov_base,
                m_pPacketsIn->vecs[n].iov_len,
                (struct sockaddr *) &m_pPacketsIn->local_addresses[n],
                (struct sockaddr *) &m_pPacketsIn->peer_addresses[n], this);
        ok_count += s >= 0;
    }
    rctx->rc_have_own = ok_count > 0;
}
#endif


enum rop UdpListener::readOnePacket(struct read_iter *iter)
{
    struct sockaddr_storage *local_addr;
    lsquic_cid_t cid;
    ssize_t nread;

    if (iter->ri_idx >= m_pPacketsIn->n_avail
#ifdef _NOT_USE_SHM_
        || iter->ri_off + MAX_PACKET_SZ > m_pPacketsIn->data_sz
#endif
                                                               )
    {
        LS_DBG_M(this, "%s: out of room in packets_in", __func__);
        return ROP_NOROOM;
    }

#ifndef _NOT_USE_SHM_
    unsigned char ctl_buf[CTL_SZ];
    packet_buf_t *const packet_buf = m_pPacketsIn->packet_bufs[iter->ri_idx];
    struct iovec iov;
    iov.iov_base = packet_buf->data;
    iov.iov_len  = sizeof(packet_buf->data);

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_name       = &packet_buf->peer_addr;
    msg.msg_namelen    = sizeof(packet_buf->peer_addr);
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_control    = ctl_buf;
    msg.msg_controllen = CTL_SZ;
    msg.msg_flags      = 0;
#else
    m_pPacketsIn->vecs[iter->ri_idx].iov_base = m_pPacketsIn->packet_data + iter->ri_off;
    m_pPacketsIn->vecs[iter->ri_idx].iov_len  = MAX_PACKET_SZ;
    unsigned char *ctl_buf = m_pPacketsIn->ctlmsg_data + iter->ri_idx * CTL_SZ;

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_name       = &m_pPacketsIn->peer_addresses[iter->ri_idx],
    msg.msg_namelen    = sizeof(m_pPacketsIn->peer_addresses[iter->ri_idx]),
    msg.msg_iov        = &m_pPacketsIn->vecs[iter->ri_idx],
    msg.msg_iovlen     = 1,
    msg.msg_control    = ctl_buf,
    msg.msg_controllen = CTL_SZ,
    msg.msg_flags      = 0,
#endif

    nread = recvmsg(getfd(), &msg, 0);
    if (-1 == nread) {
        if (!(EAGAIN == errno || EWOULDBLOCK == errno))
            LS_ERROR("recvmsg: %s", strerror(errno));
        return ROP_ERROR;
    }

    /* Drop packets that are obviously bad (we always expect to have the
     * connection ID:
     */
    if (0 != lsquic_cid_from_packet(
                (const unsigned char *) msg.msg_iov[0].iov_base,
                                                nread, &cid))
        return ROP_OK;

#ifndef _NOT_USE_SHM_
    local_addr = &packet_buf->local_addr;
#else
    local_addr = &m_pPacketsIn->local_addresses[iter->ri_idx];
#endif

    memcpy(local_addr, m_addr.get(), AF_INET == m_addr.get()->sa_family ?
                    sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
    packet_buf->ecn = 0;
    proc_ancillary(&msg, local_addr
#if ECN_SUPPORTED
        , &packet_buf->ecn
#endif
    );


#ifndef _NOT_USE_SHM_
    packet_buf->data_len = nread;
    m_pPacketsIn->cids[iter->ri_idx] = cid;
#else
    m_pPacketsIn->vecs[iter->ri_idx].iov_len = nread;
    iter->ri_off += nread;
#endif
    LS_DBG_MC(this, "%s: read in packet for CID %" CID_FMT ", size: %zd",
            __func__, CID_BITS(&cid), nread);

    iter->ri_idx += 1;

    return ROP_OK;
}


int UdpListener::onRead()
{
    /* The code below assumes this value is smaller than one second */
#define MAX_USEC_PER_LOOP 25000

    struct read_ctx rctx;
    unsigned n_batches, n_packets;
    enum rop rop;
    struct timeval start, end;

    n_batches = 0, n_packets = 0;

    startReading(&rctx);
    gettimeofday(&start, NULL);

    do
    {
#ifdef _NOT_USE_SHM_
        rctx.rc_riter.ri_off = 0;
#else
        if (m_pPacketsIn->n_avail < low_threshold(m_pPacketsIn->n_alloc))
        {
            replenishPacketBatchBuffers();
            if (m_pPacketsIn->n_avail == 0)
            {
                LS_ERROR(this, "%s: no buffers to read into", __func__);
                return -1;
            }
        }
        LS_DBG_M(this, "%s: available packet buffers: %u", __func__,
                 m_pPacketsIn->n_avail);
#endif
        rctx.rc_riter.ri_idx = 0;

        do
            rop = readOnePacket(&rctx.rc_riter);
        while (ROP_OK == rop);

        LS_DBG_L(this, "%s: read %u packet%.*s", __func__,
            rctx.rc_riter.ri_idx, rctx.rc_riter.ri_idx != 1, "s");
        n_packets += rctx.rc_riter.ri_idx;
        n_batches += rctx.rc_riter.ri_idx > 0;

        if (rctx.rc_riter.ri_idx > 0)
            processPacketsInBatch(&rctx);

        gettimeofday(&end, NULL);
        if ((start.tv_sec == end.tv_sec &&
                start.tv_usec + MAX_USEC_PER_LOOP <= end.tv_usec) ||
            (start.tv_sec == end.tv_sec - 1 &&
                start.tv_usec + MAX_USEC_PER_LOOP <= end.tv_usec + 1000000) ||
            (start.tv_sec <  end.tv_sec - 1))
        {
            LS_DBG_M(this, "%s: take a breather reading packets "
                "after exceeding timer", __func__);
            break;
        }
    }
    while (ROP_NOROOM == rop);

    finishReading(&rctx);

    LS_DBG_L(this, "%s: done; in total, read %u packet%.*s in %u batch%.*s",
             __func__, n_packets, n_packets != 1,         "s",
                     n_batches, (n_batches != 1) << 1, "es");

    return 0;
}


int UdpListener::handleEvents(short int event)
{
    if (event & POLLIN)
    {
//         if (m_pEngine->isShutdown())
//             MultiplexerFactory::getMultiplexer()->suspendRead(this);
//         else
        onRead();
    }
    if (event & POLLOUT)
    {
        MultiplexerFactory::getMultiplexer()->suspendWrite(this);
        m_pEngine->sendUnsentPackets();
    }
    return 0;
}


int UdpListener::initPacketsIn(void)
{
    unsigned n_alloc;

#ifdef _NOT_USE_SHM_
    socklen_t opt_len;
    int recvsz;
    opt_len = sizeof(recvsz);
    if (0 != getsockopt(getfd(), SOL_SOCKET, SO_RCVBUF, &recvsz, &opt_len))
    {
        LS_ERROR(this, "%s: getsockopt failed: %s", __func__, strerror(errno));
        return -1;
    }
    n_alloc = (unsigned) recvsz / MAX_PACKET_SZ * 4;
    LS_INFO(this, "%s: socket buffer size: %d bytes; max # packets is set to %u",
            __func__, recvsz, n_alloc);
#else
    n_alloc = BATCH_SIZE;
#endif

#if DEBUG_SHM_OFFSETS
    n_alloc = MIN(10, BATCH_SIZE);  /* Small number to make it easy to track */
    LS_INFO(this, "%s: DEBUG_SHM_OFFSETS is on, allocate %u packets",
            __func__, n_alloc);
#endif

    m_pPacketsIn = (struct packets_in *) malloc(sizeof(struct packets_in));
    if (!m_pPacketsIn)
    {
        LS_ERROR(this, "%s: malloc failed", __func__);
        return -1;
    }

    m_pPacketsIn->n_alloc     = n_alloc;
#ifndef _NOT_USE_SHM_
    m_pPacketsIn->n_avail = 0;
    m_pPacketsIn->cids        = (lsquic_cid_t  *) calloc(n_alloc, sizeof(m_pPacketsIn->cids[0]));
    m_pPacketsIn->packet_bufs = (packet_buf_t **) calloc(n_alloc, sizeof(m_pPacketsIn->packet_bufs[0]));
#else
    m_pPacketsIn->data_sz = recvsz;
    m_pPacketsIn->packet_data = (unsigned char *) malloc(recvsz);
    m_pPacketsIn->ctlmsg_data = (unsigned char *) malloc(n_alloc * CTL_SZ);
    m_pPacketsIn->vecs = (struct iovec *)
                malloc(n_alloc * sizeof(m_pPacketsIn->vecs[0]));
    m_pPacketsIn->local_addresses = (struct sockaddr_storage *)
                malloc(n_alloc * sizeof(m_pPacketsIn->local_addresses[0]));
    m_pPacketsIn->peer_addresses = (struct sockaddr_storage *)
                malloc(n_alloc * sizeof(m_pPacketsIn->peer_addresses[0]));
    m_pPacketsIn->n_avail = n_alloc;
#endif

    if (
#ifndef _NOT_USE_SHM_
        m_pPacketsIn->packet_bufs &&
            m_pPacketsIn->cids
#else
        m_pPacketsIn->packet_data &&
            m_pPacketsIn->ctlmsg_data &&
                m_pPacketsIn->vecs &&
                    m_pPacketsIn->local_addresses &&
                        m_pPacketsIn->peer_addresses
#endif
                                                    )
    {
        LS_INFO(this, "%s: allocated %u packets", __func__, n_alloc);
        return 0;
    }
    else
        return -1;
}

void UdpListener::cleanupPacketsIn(void)
{
    if (m_pPacketsIn)
    {
#ifndef _NOT_USE_SHM_
        free(m_pPacketsIn->cids);
        free(m_pPacketsIn->packet_bufs);
#else
        free(m_pPacketsIn->packet_data);
        free(m_pPacketsIn->ctlmsg_data);
        free(m_pPacketsIn->vecs);
        free(m_pPacketsIn->local_addresses);
        free(m_pPacketsIn->peer_addresses);
#endif
        free(m_pPacketsIn);
        m_pPacketsIn = NULL;
    }
}


UdpListener::~UdpListener()
{
// #ifndef _NOT_USE_SHM_
//     if (m_pPacketsIn->n_avail)
//     {
//         QuicShm::getInstance().releasePacketBuffers(
//                     m_pPacketsIn->packet_bufs, m_pPacketsIn->n_avail);
//         m_pPacketsIn->n_avail = 0;
//     }
//
//     while (m_availPacketBufs.count() > 0)
//         releaseSomePacketsToSHM();
// #endif
    cleanupPacketsIn();
//    m_reusePortFds.releaseObjects();
}


struct ssl_ctx_st * UdpListener::getSslContext(void) const
{
    SslContext *pSslContext = NULL;
    VHostMap *map = m_pTcpPeer->getVHostMap();
    if (map)
    {
        pSslContext = map->getSslContext();
        if (pSslContext)
            return pSslContext->get();
    }

    return NULL;
}

void UdpListener::initShmCidMapping()
{
    //FIXME
}


int UdpListener::beginServe(QuicEngine *pEngine)
{
    m_pEngine = pEngine;
    m_pEngine->registerUdpListener(this);
    //return start();
#ifndef  _NOT_USE_SHM_
    UdpListener::initCidListenerHt();
#endif
    initPacketsIn();

    return registEvent();
}

int UdpListener::bind2(int flag)
{
    int fd;
    int ret = CoreSocket::bind(m_addr, SOCK_DGRAM, &fd, flag);
    if (ret != 0)
        return -1;
    ret = setSockOptions(fd);
    if (ret == -1)
    {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    ::fcntl(fd, F_SETFD, FD_CLOEXEC);
    ::fcntl(fd, F_SETFL, MultiplexerFactory::getMultiplexer()->getFLTag());
    return fd;
}


int UdpListener::bind()
{
    int fd = bind2(0);
    if (fd == -1)
        return -1;
    setfd(fd);
    return 0;
}


int UdpListener::bindReusePort(int count, const char* addr_str)
{
    int i, fd;
    m_reusePortFds.guarantee(count - m_reusePortFds.size());
    for(i = 0; i < count; ++i)
    {
        fd = bind2(LS_SOCK_REUSEPORT);
        if (fd == -1)
        {
            if (errno != EACCES)
                LS_NOTICE("[UDP %s] failed to start SO_REUSEPORT socket",
                           addr_str);
            return -1;
        }
        LS_NOTICE("[UDP %s] SO_REUSEPORT #%d socket started, fd: %d",
               addr_str, i + 1, fd);
        m_reusePortFds[i] = fd;
    }
    m_reusePortFds.setSize(count);
    return 0;

}


int UdpListener::setSockOptions(int fd)
{
    int ret = 0;
    int val = 1;
    if (AF_INET == m_addr.get()->sa_family)
        ret = setsockopt(fd, IPPROTO_IP,
#if __linux__ && defined(IP_RECVORIGDSTADDR)
                         IP_RECVORIGDSTADDR,
#elif __linux__
                         IP_PKTINFO,
#else
                         IP_RECVDSTADDR,
#endif
                         &val, sizeof(val));
    else
        ret = setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &val, sizeof(val));
    if (ret != 0)
        return -1;

#if __linux__
#if !defined(IP_RECVORIGDSTADDR)
    /* Need to set IP_PKTINFO for sending */
    if (AF_INET == m_addr.get()->sa_family)
    {
        val = 1;
        ret = setsockopt(fd, IPPROTO_IP, IP_PKTINFO, &val, sizeof(val));
        if (0 != ret)
        {
            return -1;
        }
    }
#endif
#elif IP_RECVDSTADDR != IP_SENDSRCADDR
    /* On FreeBSD, IP_RECVDSTADDR is the same as IP_SENDSRCADDR, but I do not
     * know about other BSD systems.
     */
    if (AF_INET == m_addr.get()->sa_family)
    {
        val = 1;
        ret = setsockopt(fd, IPPROTO_IP, IP_SENDSRCADDR, &val, sizeof(val));
        if (0 != ret)
        {
            return -1;
        }
    }
#endif

#if ECN_SUPPORTED
    val = 1;
    if (AF_INET == m_addr.get()->sa_family)
        ret = setsockopt(fd, IPPROTO_IP, IP_RECVTOS, &val, sizeof(val));
    else
        ret = setsockopt(fd, IPPROTO_IPV6, IPV6_RECVTCLASS, &val, sizeof(val));
    if (0 != ret)
    {
        return -1;
    }
#endif

    val = 1 * 1024 * 1024;
    ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &val, sizeof(val));
    if (ret != 0)
    {
        LS_WARN(this, "cannot set receive buffer to %d bytes: %s", val,
                  strerror(errno));
    }

    if (AF_INET == m_addr.get()->sa_family)
    {
#if __linux__
        val = IP_PMTUDISC_DO;
        ret = setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));
#elif WIN32
        val = 1;
        ret = setsockopt(fd, IPPROTO_IP, IP_DONTFRAGMENT, (char *) &val, sizeof(val));
#else
        val = 1;
        ret = setsockopt(fd, IPPROTO_IP, IP_DONTFRAG, &val, sizeof(val));
#endif
        if (0 != ret)
        {
            return -1;
        }
    }

    return ret;
}


int UdpListener::registEvent()
{
    return MultiplexerFactory::getMultiplexer()->add(this,
            POLLIN | POLLHUP | POLLERR);
}


int UdpListener::getFdCount(int* maxfd)
{
    if (getfd() > *maxfd)
        *maxfd = getfd();
    if (m_reusePortFds.size() <= 0)
        return getfd() != -1;
    return m_reusePortFds.getFdCount(maxfd);
}


int UdpListener::passFds(int target_fd, const char *addr_str)
{
    if (m_reusePortFds.size() > 0)
        return m_reusePortFds.passFds("UDP", addr_str, target_fd);
    if (getfd() == -1)
        return 0;
    --target_fd;
    LS_INFO("[UDP %s] pass listener, copy fd %d to %d.", addr_str,
        getfd(), target_fd);
    dup2(getfd(), target_fd);
    close(getfd());
    return 1;
}


void UdpListener::addReusePortSocket(int fd, const char *addr_str)
{
    if (m_reusePortFds.size() == 0)
    {
        *m_reusePortFds.newObj() = getfd();
    }
    *m_reusePortFds.newObj() = fd;
    LS_NOTICE("[UDP %s] Recovering server socket, SO_REUSEPORT #%d, fd: %d",
                addr_str, m_reusePortFds.size(), fd);
}


int UdpListener::activeReusePort(int seq, const char *addr_str)
{
    int n;
    int fd = m_reusePortFds.getActiveFd(seq, &n);
    if (fd != -1)
    {
        LS_NOTICE("[UDP %s] Worker #%d activates SO_REUSEPORT #%d socket, fd: %d",
                  addr_str, seq, n + 1, fd);
        setfd(fd);
        return 0;
    }
    return -1;
}


int UdpListener::startReusePortSocket(int start, int total, const char* addr_str)
{
    int i, fd;
    LS_NOTICE("[UDP %s] Add SO_REUSEPORT sockets, #%d to #%d",
                addr_str, start + 1, total);
    m_reusePortFds.guarantee(total - m_reusePortFds.size());

    if (start == 1)
        ls_setsockopt(m_reusePortFds[0], SOL_SOCKET, SO_REUSEPORT,
                      (char *)(&start), sizeof(start));

    for(i = start; i < total; ++i)
    {
        fd = bind2(LS_SOCK_REUSEPORT);
        if (fd == -1)
        {
            if (errno != EACCES)
                LS_NOTICE("[UDP %s] failed to start SO_REUSEPORT socket",
                           addr_str);
            return -1;
        }
        LS_DBG("[UDP %s] SO_REUSEPORT #%d socket started, fd: %d",
               addr_str, i + 1, fd);
        m_reusePortFds[i] = fd;
    }
    m_reusePortFds.setSize(total);
    return 0;
}


int UdpListener::adjustReusePortCount(int count, const char *addr_str)
{
    if (m_reusePortFds.size() == 0 && getfd() != -1)
        *m_reusePortFds.newObj() = getfd();
    if (count > m_reusePortFds.size())
        return startReusePortSocket(m_reusePortFds.size(), count, addr_str);
    else if (count < m_reusePortFds.size())
    {
        LS_NOTICE("[UDP %s] Shink SO_REUSEPORT socket count from %d to %d",
                    addr_str, m_reusePortFds.size(), count);
        return m_reusePortFds.shrink(count);
    }
    return 0;
}


