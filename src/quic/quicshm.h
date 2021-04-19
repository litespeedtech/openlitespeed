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

 /***
  * FOR TESTING only *
  * You can define _NOT_USE_SHM_ to disable share memory using,
  * it will break in multi processes mode.
  * FOR TESTING only *
  */
//#define _NOT_USE_SHM_



#ifndef QUICSHM_H
#define QUICSHM_H

#include <util/tsingleton.h>
#include <shm/lsshmtypes.h>
#include <shm/lsshmhash.h>
#include <sys/socket.h>

#include "lsquic_types.h"

#ifndef _NOT_USE_SHM_

class LsShmHash;
class LsShmPool;
class SetOfPacketBuffers;
struct sockaddr_storage;
struct quicshm_packet_buf;

struct ShmCidPidInfo
{
    pid_t                   pid;
    LsShmHash::iteroffset   shmCidIterOffset;
    LsShmHash::iterator     shmCidIter;
};

#ifdef RUN_TEST
namespace SuiteQuicShm {
    class TestCidPidMapping;
    class TestBufferData;
};
#endif

#define QUIC_SHM_PACKET_SIZE 1500

class QuicShm : public TSingleton<QuicShm>
{
    friend class TSingleton<QuicShm>;
#ifdef RUN_TEST
    friend class SuiteQuicShm::TestCidPidMapping;
    friend class SuiteQuicShm::TestBufferData;
#endif

public:
    class PacketBufIter
    {
    private:
        LsShmOffset_t   m_lastOffset, m_nextOffset;
    public:
        explicit PacketBufIter(LsShmOffset_t firstOffset, LsShmOffset_t lastOffset)
            : m_lastOffset(lastOffset)
            , m_nextOffset(firstOffset)
            {}
        struct quicshm_packet_buf *
        nextPacketBuf();
    };
private:
    struct QueueHead
    {
        LsShmOffset_t   qh_firstOffset,
                        qh_lastOffset;
    };

    /* This is the maximum number of packet buffers allowed to accumulate
     * in a pending queue.  After this limit is reached, no packet buffers
     * are appeneded to the queue, effectively dropping packet contents on
     * the floor.
     */
    static const unsigned MAX_PEND_QUEUE_BUFFER_COUNT = 2000;

    struct PidInfo
    {
        QueueHead       pi_pendQueue,
                        pi_freeQueue;
        time_t          pi_pendQueueSignalTime;
        unsigned        pi_pendQueueCount;
        unsigned        pi_pendQueueDropped;
        void reset() { memset(this, 0, sizeof(*this)); }
    };
    enum GpiStatus { GPIS_FOUND, GPIS_NOT_FOUND, GPIS_ERROR, };
    enum atpq { ATPQ_OK, ATPQ_SIGNAL, ATPQ_SIGNAL_REL, ATPQ_ERROR,
                                                        ATPQ_DROP_REL, };

private:
    LsShmHash               *m_pServerConf;
    LsShmHash               *m_pCidMap;  //cid-->pid
    LsShmHash               *m_pPidPacketOffsetMap;
    LsShmPool               *m_pMapPool;
    LsShmPool               *m_pPacketPool;
    LsShmHash::iterator      m_pMyPidInfoIter;

    struct lsquic_shared_hash_if * m_pInternalShi;
    void *m_pInternalShiCtx;

    time_t                   m_tLastCleaned;
    unsigned                 m_uCleanPeriod;    /* How often to invoke SHM cleaner, in seconds */
    unsigned                 m_uMaxCids;

    int openShmMapPool(const char *fileName, const char *pShmDir);
    int openShmPacketPool(const char *fileName, const char *pShmDir);

public:
    QuicShm() : m_pServerConf(NULL)
            , m_pCidMap(NULL)
            , m_pPidPacketOffsetMap(NULL)
            , m_pMapPool(NULL)
            , m_pPacketPool(NULL)
            , m_pMyPidInfoIter(NULL)
            , m_pInternalShi(NULL)
            , m_pInternalShiCtx(NULL)
            , m_tLastCleaned(0)
            , m_uCleanPeriod(120)
            , m_uMaxCids(800000)
            {};

    ~QuicShm();


    int init(const char *pShmDir);

    static void setPid(int pid);

    void *getHashCtx()  { return (void *)m_pServerConf;    }
    struct lsquic_shared_hash_if *getInternalShi() {  return m_pInternalShi; }
    void *getInternalShiCtx()  { return m_pInternalShiCtx;    }

public:
    /* CID/PID related methods: */
    void lookupCidPids(const lsquic_cid_t *cid, ShmCidPidInfo *, unsigned);
    void markClosedCidItem(const lsquic_cid_t *cid);
    void markBadCidItems(const lsquic_cid_t *cid, unsigned count, pid_t pid);
    void deleteCidItems(const lsquic_cid_t *cid, unsigned count);
    void touchCidItems(const lsquic_cid_t *cid, unsigned count);
    void maybeUpdateLruCidItems(const ShmCidPidInfo *pInfos, unsigned count);
    void cleanExpiredCidItem();

    /* Buffer-related methods: */

    /* Get `count' brand-new SHM packet buffers from SHM and insert them
     * into `set'.  They can be returned to SHM using any of the
     * releasePacketBuffer* family of functions.
     *
     * Returns the number of new SHM packet buffers allocated.
     */
    unsigned getNewPacketBuffers(SetOfPacketBuffers &set, unsigned count);

    /* Release a single packet buffer */
    void releasePacketBuffer(struct quicshm_packet_buf *);

    /* Release a number of packet buffers.  Use this version if you have
     * more than one packet to release, as it is more efficient because
     * it only takes one lock.
     */
    void releasePacketBuffers(struct quicshm_packet_buf *const *, unsigned);

    /* Removes `count' (or fewer, if `set' does not have `count' elements)
     * arbitrary packet buffers from `set' and releases them back into SHM
     * pool.
     */
    void releasePacketBuffers(SetOfPacketBuffers &set, unsigned count);

    /* Alien packets are those that belong to other processes.  `pids' and
     * `packet_bufs' are arrays with `count' items each.  Packet buffer
     * in `packet_bufs[n]' belongs to process with PID `pids[n]'.
     *
     * The function populates array `pids_to_signal' of size
     * `*n_pids_to_signal' with PIDs that are to be signaled.  The final
     * number of the PIDs in this array is returned to the caller via the
     * same `n_pids_to_signal' variable.  The caller is expected to provide
     * array large enough to hold all unique pids specified in `pids'.
     */
    unsigned distributeAlienPackets(ShmCidPidInfo * const*pids,
        struct quicshm_packet_buf * const* packet_bufs, unsigned count,
        pid_t *pids_to_signal, unsigned *n_pids_to_signal);

    /* Pending packet buffers are those assigned to current by other
     * processes using distributeAlienPackets().  Calling this function
     * removes all packet buffers from the pending queue and appends it
     * to the free queue.
     */
    PacketBufIter getPendingPacketBuffers();

    /* Find and release SHM resources used by process `pid'.  Used to clean
     * up after crashed processes.
     */
    void cleanupPidShm(pid_t pid);

    /* For the shi_shm functions, only use for SCFG right now*/
    static void deleteExpiredItems (LsShmHash *pHash);
    static int insertItem(void *hash_ctx, void *key, unsigned key_sz, void *data,
               unsigned data_sz, time_t expiry);

    static int lookupItem(void *hash_ctx, const void *key, unsigned key_sz, void **data,
               unsigned *data_sz);

    static int deleteItem(void *hash_ctx, const void *key, unsigned key_sz);

private:
    void releasePacketBufferOff(LsShmOffset_t);
    struct quicshm_packet_buf *packetOffset2ptr(LsShmOffset_t offset);
    enum GpiStatus getPidInfo(pid_t pid, QuicShm::PidInfo &, LsShmHash::iterator &);
    int getMyPidInfo(QuicShm::PidInfo &);
    void updatePidInfo(const QuicShm::PidInfo &, LsShmHash::iterator &);
    int createPidInfo(pid_t pid, const QuicShm::PidInfo &);
    pid_t lookupCidPid(const lsquic_cid_t *, ShmCidPidInfo *pInfo);
    enum atpq appendToPendQueue(pid_t pid, struct quicshm_packet_buf *, time_t);
    struct quicshm_packet_buf * getNewPacketBuffer();
    struct quicshm_packet_buf * getPacketBufferSt(LsShmOffset_t offset)
    {    return (quicshm_packet_buf *)(packetOffset2ptr(offset));  }
    int appendPacketsToFreeQueue(struct quicshm_packet_buf *begin,
                                                    struct quicshm_packet_buf *end);
    int appendPacketsToFreeQueue(LsShmOffset_t firstOffset, LsShmOffset_t lastOffset);
    int removeFromFreeQueue(struct quicshm_packet_buf *);
    void releasePacketsFromQueue(const QueueHead &);
    void cleanupOldShmData();
    void maybeCleanShm();
#ifndef NDEBUG
    void pidInfoSanityCheck(pid_t pid);
    void queueSanityCheck(const QueueHead &queue);
#endif
#ifdef RUN_TEST
    LsShmOffset_t popFirstPendingOffset(pid_t pid);
    void lockCidMap();
    void unlockCidMap();
#endif
};

LS_SINGLETON_DECL(QuicShm);

#endif // QUICSHM_H
#endif //_NOT_USE_SHM_
