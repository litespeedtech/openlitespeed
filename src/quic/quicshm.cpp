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



#include "quicshm.h"
#include <lsr/xxhash.h>
#include <lsr/ls_str.h>
#include <shm/lsshmhash.h>
#include <main/lshttpdmain.h>
#include <liblsquic/lsquic_stock_shi.h>
#include <lsquic.h>
#include <log4cxx/logger.h>
#include <util/datetime.h>
#include "packetbuf.h"
#include "pbset.h"
#include "quiclog.h"

#ifndef _NOT_USE_SHM_


static pid_t s_pid = -1;

void QuicShm::setPid(pid_t pid)
{
    s_pid = pid;    
}


static LsShmHKey _self_hash(const void *__s, size_t len)
{
    LsShmHKey key = 0;
    memcpy(&key, __s, 4);
    return key;
}


LS_SINGLETON(QuicShm);


QuicShm::~QuicShm()
{
    if (m_pServerConf)
        m_pServerConf->close();

    if (m_pInternalShiCtx)
        stock_shared_hash_destroy((struct stock_shared_hash *)m_pInternalShiCtx);
    
    if (m_pCidMap)
        m_pCidMap->close();
    
    if (m_pPidPacketOffsetMap)
        m_pPidPacketOffsetMap->close();
}


int QuicShm::openShmPacketPool(const char *fileName, const char *pShmDir)
{
    LsShm *pShm;
    int attempts;
    int ret = -1;
    for (attempts = 0; attempts < 3; ++attempts)
    {
        pShm = LsShm::open(fileName, 4096, pShmDir, LSSHM_OPEN_STD);
        if (!pShm)
        {
            pShm = LsShm::open(fileName, 4096, pShmDir, LSSHM_OPEN_STD);
            if (!pShm)
                return -1;
        }

        m_pPacketPool = pShm->getGlobalPool();
        if (!m_pPacketPool)
        {
            pShm->deleteFile();
            pShm->close();
            continue;
        }
        m_pPacketPool->disableAutoLock();

//         /* FIXME: for testing only */ 
//         int remapped;
//         LsShmOffset_t off;
//         for (int i=0; i<10; ++i)
//         {
//             off = m_pPacketPool->alloc2(100 * 1024 * 1024, remapped);
//             if (off)
//             {
//                 m_pPacketPool->release2(off, 100 * 1024 * 1024);
//                 break;
//             }
//         }
//         /* FIXME: for testing only */ 
        
        ret = 0;
        break;
    }

    return ret;
}




int QuicShm::openShmMapPool(const char *fileName, const char *pShmDir)
{
    LsShm *pShm;
    int attempts;
    int ret = -1;
    for (attempts = 0; attempts < 3; ++attempts)
    {
        //FIXME: temorary work around for SHM large block boundary issue.
        pShm = LsShm::open(fileName, 4096, pShmDir, LSSHM_OPEN_STD);
        if (!pShm)
        {
            pShm = LsShm::open(fileName, 4096, pShmDir, LSSHM_OPEN_STD);
            if (!pShm)
                return -1;
        }

        m_pMapPool = pShm->getGlobalPool();
        if (!m_pMapPool)
        {
            pShm->deleteFile();
            pShm->close();
            continue;
        }

        m_pMapPool->disableAutoLock();
        m_pMapPool->lock();

        m_pServerConf = m_pMapPool->getNamedHash("SERVERCONF", 10,
                                            LsShmHash::hashXXH32, memcmp, 0);
        m_pCidMap = m_pMapPool->getNamedHash("CIDMAPPING", 1000, _self_hash, memcmp,
                                        LSSHM_FLAG_LRU);

        m_pPidPacketOffsetMap = m_pMapPool->getNamedHash("PIDOFFSET", 5,
                                                    _self_hash, memcmp, 0);
        
        m_pMapPool->unlock();
        m_pMapPool->enableAutoLock();

        if (!m_pServerConf || !m_pCidMap || !m_pPidPacketOffsetMap)
        {
            if (m_pServerConf)
                m_pServerConf->close();
            if (m_pCidMap)
                m_pCidMap->close();
            if (m_pPidPacketOffsetMap)
                m_pPidPacketOffsetMap->close();
            m_pMapPool->close();
            pShm->deleteFile();
            pShm->close();
        }
        else
        {
//             /* FIXME: for testing only */ 
//             int remapped;
//             LsShmOffset_t off;
//             for (int i=0; i<10; ++i)
//             {
//                 off = m_pMapPool->alloc2(1 * 1024 * 1024, remapped);
//                 if (off)
//                     m_pMapPool->release2(off, 1 * 1024 * 1024);
//                 
//                 off = m_pMapPool->alloc2(100 * 1024 * 1024, remapped);
//                 if (off)
//                 {
//                     m_pMapPool->release2(off, 100 * 1024 * 1024);
//                     break;
//                 }
//             }
//             /* FIXME: for testing only */ 
            m_pCidMap->disableAutoLock();
            m_pPidPacketOffsetMap->disableAutoLock();
            ret = 0;
            break;
        }
    }

    return ret;
}


int QuicShm::init(const char *pShmDir)
{
    /* TODO: check return values in the several function calls below: */
    m_pInternalShi = (struct lsquic_shared_hash_if *)&stock_shi;
    m_pInternalShiCtx = stock_shared_hash_new();
    if (openShmPacketPool(".quicshmdata", pShmDir) == LS_FAIL)
        return LS_FAIL;
    return openShmMapPool(".quicshmmap", pShmDir);
}


void QuicShm::deleteExpiredItems (LsShmHash *pHash)
{
    LsShmHash::iterator iter;
    time_t t = time(NULL);
    LsShmHash::iteroffset iterOff;
    int autoLock = pHash->isAutoLock();
    if (autoLock)
    {
        pHash->disableAutoLock();
        pHash->lock();
    }
    for (iterOff = pHash->begin(); iterOff.m_iOffset != 0; iterOff = pHash->next(iterOff))
    {
        iter = pHash->offset2iterator(iterOff);
        if(iter)
        {
            time_t *data = (time_t *)iter->getVal();
            if (*data != 0 && *data  < t)
                pHash->eraseIterator(iterOff);
        }
    }
    if (autoLock)
    {
        pHash->unlock();
        pHash->enableAutoLock();
    }
}


/* Only store the pointer of the key and data, not deep copy */
int QuicShm::insertItem(void *hash_ctx, void *key, unsigned int key_sz, void *data,
               unsigned int data_sz, time_t expiry)
{
    struct lsquic_shared_hash_if *pInternalShi = QuicShm::getInstance().getInternalShi();
    void *pInternalShiCtx = QuicShm::getInstance().getInternalShiCtx();
    pInternalShi->shi_insert(pInternalShiCtx, key, key_sz, data, data_sz,
                             expiry);

    /* make deep copy to insert to SHM */
    int valLen = sizeof(time_t) + sizeof(unsigned int) * 2 + key_sz + data_sz;

    char *buf = new char[valLen];
    if (!buf)
        return -1;
    
    char *p = buf;
    memcpy(p, &expiry, sizeof(time_t));
    p += sizeof(time_t);
    
    memcpy(p, &key_sz, sizeof(unsigned int));
    p += sizeof(unsigned int);
    
    memcpy(p, &data_sz, sizeof(unsigned int));
    p += sizeof(unsigned int);
    
    memcpy(p, key, key_sz);
    p += key_sz;
    
    memcpy(p, data, data_sz);
    p += data_sz;

    LsShmHash *pHash = (LsShmHash *)hash_ctx;
    pHash->insert(key, key_sz, buf, valLen);
    delete [] buf;
    return 0;
}


int QuicShm::lookupItem(void *hash_ctx, const void *key, unsigned key_sz, void **data,
               unsigned *data_sz)
{
    struct lsquic_shared_hash_if *pInternalShi = QuicShm::getInstance().getInternalShi();
    void *pInternalShiCtx = QuicShm::getInstance().getInternalShiCtx();
    int ret = pInternalShi->shi_lookup(pInternalShiCtx, key, key_sz,
                                       data, data_sz);
    if (ret == 1)
        return 1; //found in memory

    LsShmHash *pHash = (LsShmHash *)hash_ctx;
    deleteExpiredItems(pHash);
    
    ls_strpair_t parms;
    ls_str_set(&parms.key, (char *)key, key_sz);
    LsShmHash::iteroffset iterOff = pHash->findIterator(&parms);
    if (iterOff.m_iOffset == 0)
        return 0;

    LsShmHash::iterator iter = pHash->offset2iterator(iterOff);
    assert(iter);
    
    int ValLen = iter->getValLen();
    uint8_t *buf = iter->getVal();
    uint8_t *p = buf;
    unsigned int key_len, data_len;
    time_t expiry;
    
    memcpy(&expiry, p, sizeof(time_t));
    p += sizeof(time_t);
    memcpy(&key_len, p, sizeof(unsigned int));
    p += sizeof(unsigned int);
    memcpy(&data_len, p, sizeof(unsigned int));
    p += sizeof(unsigned int);

    if ( key_sz != key_len || ValLen != p - buf + key_len + data_len
        || memcmp(key, p, key_len) != 0 )
    {
        deleteItem(hash_ctx, key, key_sz);
        return -1;
    }

    void *key_copy = malloc(key_len);
    memcpy(key_copy, p, key_len);
    p += key_len;
    
    *data = (char *)malloc(data_len);
    memcpy(*data, p, data_len);
    *data_sz = data_len;

    pInternalShi->shi_insert(pInternalShiCtx, key_copy, key_sz, *data,
                               data_len, expiry);
    return 1;
}


int QuicShm::deleteItem(void *hash_ctx, const void *key, unsigned key_sz)
{
    struct lsquic_shared_hash_if *pInternalShi = QuicShm::getInstance().getInternalShi();
    void *pInternalShiCtx = QuicShm::getInstance().getInternalShiCtx();
    pInternalShi->shi_delete(pInternalShiCtx, key, key_sz);
    
    LsShmHash *pHash = (LsShmHash *)hash_ctx;
    ls_strpair_t parms;
    ls_str_set(&parms.key, (char *)key, key_sz);
    pHash->disableAutoLock();
    pHash->lock();
    LsShmHash::iteroffset iterOff = pHash->findIterator(&parms);
    if (iterOff.m_iOffset > 0)
    {
        pHash->eraseIterator(iterOff);
    }
    pHash->unlock();
    pHash->enableAutoLock();
    return 0;
}


pid_t QuicShm::lookupCidPid(const lsquic_cid_t *cid, ShmCidPidInfo *pInfo)
{
    pid_t pid;
    ls_strpair_t parms;
    ls_str_set(&parms.key, (char *) cid->idbuf, cid->len);
    LsShmHash::iteroffset iterOff;
    LsShmHash::iterator iter;
    
    iterOff = m_pCidMap->findIterator(&parms);
    if (iterOff.m_iOffset == 0)
    {
        if (m_pCidMap->size() >= m_uMaxCids)
        {
            LS_DBG_H("[QuicShm::lookupCidPid] max map size is reached: %u",
                                                            m_pCidMap->size());
            return 0;
        }
        pid = s_pid;
        ls_str_set(&parms.val, (char *)&pid, sizeof(pid));
        iterOff = m_pCidMap->insertIterator(&parms);
        if (iterOff.m_iOffset != 0)
        {
            LS_DBG_LC("[QuicShm::lookupCidPid]: insert CID: %" CID_FMT
                    ", PID: %d", CID_BITS(cid), pid);
            iter = m_pCidMap->offset2iterator(iterOff);
        }
        else
        {
            LS_DBG_H("[QuicShm::lookupCidPid] insertion failed");
            return 0;       /* Error */
        }
    }
    else
    {
        iter = m_pCidMap->offset2iterator(iterOff);
        assert(iter);

        memcpy(&pid, iter->getVal(), sizeof(pid));
        if (pid > 0 && iter->getLruLasttime() != DateTime::s_curTime) 
        {
            LS_DBG_LC("[QuicShm::lookupCidPid]: update LRU for CID: %" CID_FMT,
                     CID_BITS(cid));
            m_pCidMap->touchLru(iterOff);
        }
    }
    pInfo->shmCidIter = iter;
    pInfo->shmCidIterOffset = iterOff;

    return pid;
}


/* After the call, any entries in `pids' that have value 0 could not be
 * inserted into SHM.
 */
void QuicShm::lookupCidPids(const lsquic_cid_t *cids, ShmCidPidInfo *pids,
                                unsigned count)
{
    unsigned n, n_failures = 0;
    pid_t pid;

    maybeCleanShm();

    m_pCidMap->lock();

    for (n = 0; n < count; ++n)
        if (pids[n].pid == 0)
        {
            pid = lookupCidPid(&cids[n], &pids[n]);
            pids[n].pid = pid;
            n_failures += pid == 0;
        }

    m_pCidMap->unlock();

    if (n_failures > 0)
        LS_WARN("could not insert %u CID/PID mapping%s", n_failures,
            n_failures == 1 ? "" : "s");
}


void QuicShm::markBadCidItems(const lsquic_cid_t *cids, unsigned count, 
                              pid_t pid)
{
    LsShmHash::iteroffset iterOff;
    ls_strpair_t parms;
    unsigned n;

    m_pCidMap->lock();

    for (n = 0; n < count; ++n)
    {
        ls_str_set(&parms.key, (char *) cids[n].idbuf, cids[n].len);
        iterOff = m_pCidMap->findIterator(&parms);
        if (iterOff.m_iOffset > 0)
        {
            LS_DBG_LC("[QuicShm::markBadCidItems]: mark CID %" CID_FMT
                    " with PID: %d", CID_BITS(&cids[n]), pid);
            LsShmHash::iterator iter = m_pCidMap->offset2iterator(iterOff);
            assert(iter);

            assert(iter->getValLen() == sizeof(pid_t));
            memcpy(iter->getVal(), &pid, sizeof(pid_t));
            if (iter->getLruLasttime() != DateTime::s_curTime) 
                m_pCidMap->touchLru(iterOff);
        }
    }

    m_pCidMap->unlock();

    maybeCleanShm();
}


void QuicShm::touchCidItems(const lsquic_cid_t *cids, unsigned count)
{
    LsShmHash::iteroffset iterOff;
    ls_strpair_t parms;
    unsigned n;

    m_pCidMap->lock();

    for (n = 0; n < count; ++n)
    {
        ls_str_set(&parms.key, (char *) cids[n].idbuf, cids[n].len);
        iterOff = m_pCidMap->findIterator(&parms);
        if (iterOff.m_iOffset > 0)
        {
            LsShmHash::iterator iter = m_pCidMap->offset2iterator(iterOff);
            assert(iter);
            if (iter && iter->getLruLasttime() != DateTime::s_curTime)
            {
                LS_DBG_LC("[QuicShm::touchSCIDs]: touch CID %" CID_FMT
                    " (last touched %ld seconds ago)", CID_BITS(&cids[n]),
                    (long) DateTime::s_curTime - (long) iter->getLruLasttime());
                m_pCidMap->touchLru(iterOff);
            }
        }
    }

    m_pCidMap->unlock();

    maybeCleanShm();
}


void QuicShm::deleteCidItems(const lsquic_cid_t *cids, unsigned count)
{
    LsShmHash::iteroffset iterOff;
    ls_strpair_t parms;
    unsigned n;

    m_pCidMap->lock();

    for (n = 0; n < count; ++n)
    {
        ls_str_set(&parms.key, (char *) cids[n].idbuf, cids[n].len);
        iterOff = m_pCidMap->findIterator(&parms);
        if (iterOff.m_iOffset > 0)
        {
            LS_DBG_LC("[QuicShm::deleteCidItems]: remove entry for "
                                        "CID %" CID_FMT, CID_BITS(&cids[n]));
            m_pCidMap->eraseIterator(iterOff);
        }
    }

    m_pCidMap->unlock();

    maybeCleanShm();
}


static void iter2cid(LsShmHash::iterator iter, lsquic_cid_t *cid)
{
    const uint8_t *buf;

    cid->len = (unsigned) iter->getKeyLen();
    if (cid->len <= sizeof(cid->idbuf))
    {
        buf = iter->getKey();
        memcpy(cid->idbuf, buf, cid->len);
    }
    else
    {
        assert(0);
        cid->len = 0;
    }
}


static int logCleanedCid(LsShmHash::iterator iter, void *arg)
{
    lsquic_cid_t cid;
    iter2cid(iter, &cid);
    LS_DBG_LC("[QuicShm::cleanExpiredCidItem]: remove expired "
             "CID %" CID_FMT, CID_BITS(&cid));
    return 1;
}


void QuicShm::maybeUpdateLruCidItems(const ShmCidPidInfo *pInfos,
                                                            unsigned count)
{
    lsquic_cid_t cid;
    unsigned n, n_to_up, to_update[count];

    n_to_up = 0;
    for (n = 0; n < count; ++n)
        if (pInfos[n].pid > 0 && pInfos[n].shmCidIter->getLruLasttime() !=
                                                        DateTime::s_curTime)
            to_update[ n_to_up++ ] = n;

    if (n_to_up > 0)
    {
        m_pCidMap->lock();
        for (n = 0; n < n_to_up; ++n)
        {
            m_pCidMap->touchLru(pInfos[ to_update[n] ].shmCidIterOffset);
            if (LS_LOG_ENABLED(log4cxx::Level::DBG_LESS))
            {
                iter2cid(pInfos[ to_update[n] ].shmCidIter, &cid);
                LS_DBG_LC("[QuicShm::updateLruCidItem]: update LRU for CID "
                    "%" CID_FMT, CID_BITS(&cid));
            }
        }
        m_pCidMap->unlock();
    }
}


void QuicShm::cleanExpiredCidItem()
{
    LsShmHash::TrimCb cb = NULL;
    if (log4cxx::Level::isEnabled(log4cxx::Level::DEBUG))
        cb = logCleanedCid;
    m_pCidMap->lock();
    m_pCidMap->trim(DateTime::s_curTime - 90, cb, NULL);
    m_pCidMap->unlock();
}


void QuicShm::markClosedCidItem(const lsquic_cid_t *cid)
{
    markBadCidItems(cid, 1, -1);
}


void QuicShm::maybeCleanShm()
{
    if (m_tLastCleaned + m_uCleanPeriod < DateTime::s_curTime)
    {
        cleanupOldShmData();
        m_tLastCleaned = DateTime::s_curTime;
    }
}


void QuicShm::cleanupOldShmData()
{
    unsigned n_pids, n;
    LsShmHash::iteroffset iterOff;
    LsShmHash::iterator iter;
    pid_t pids[200 /* This should be large enough */];
    const pid_t my_pid = s_pid;

    /* Collects all pids into `pids' array */
    m_pPidPacketOffsetMap->lock();
    for (iterOff = m_pPidPacketOffsetMap->begin(), n_pids = 0;
            iterOff.m_iOffset != 0 && n_pids < sizeof(pids) / sizeof(pids[0]);
                iterOff = m_pPidPacketOffsetMap->next(iterOff))
    {
        iter = m_pPidPacketOffsetMap->offset2iterator(iterOff);
        if (iter)
            memcpy(&pids[n_pids++], iter->getKey(), sizeof(pids[n_pids]));
    }
    m_pPidPacketOffsetMap->unlock();

    /* Warn in case our array does not hold enough PIDs.  This is unlikely. */
    if (iterOff.m_iOffset && n_pids == sizeof(pids) / sizeof(pids[0]))
        LS_WARN("[QuicShm::cleanupOldShmData]: more pids than elements (%zu)",
            sizeof(pids) / sizeof(pids[0]));

    /* Identify dead processes and clean up after them: */
    for (n = 0; n < n_pids; ++n)
        if (pids[n] != my_pid &&
                0 != kill(pids[n], 0) &&
                    (errno == ESRCH || errno == EPERM))
        {
            LS_INFO("[PID %d] [QuicShm::cleanupOldShmData]: cleaning up dead process "
                    "SHM (pid: %d)", s_pid, pids[n]);
            cleanupPidShm(pids[n]);
        }
        else
            LS_DBG_L("[QuicShm::cleanupOldShmData]: PID %d is alive", pids[n]);
}


#define CHECK_QUEUES_FOR_SANITY 0

#ifndef NDEBUG

void
QuicShm::queueSanityCheck(const QueueHead &queue)
{
    const packet_buf_t *cur, *prev;
    LsShmOffset_t off;

    off = queue.qh_firstOffset;
    prev = NULL;
    while (off)
    {
        cur = packetOffset2ptr(off);
        assert(cur);
        assert(cur->qpb_off == off);
        if (prev)
        {
            assert(prev->qpb_off == cur->prev_offset);
            assert(prev->next_offset == cur->qpb_off);
        }
        else
            assert(cur->prev_offset == 0);
        off = cur->next_offset;
        prev = cur;
    }

    if (prev)
    {
        assert(prev->next_offset == 0);
        assert(queue.qh_lastOffset == prev->qpb_off);
    }
    else
        assert(queue.qh_lastOffset == 0);
}


void
QuicShm::pidInfoSanityCheck(pid_t pid)
{
    PidInfo info;
    GpiStatus status;
    LsShmHash::iterator iter;

    // TODO: how does one add an assert like this?
    // assert(m_pPidPacketOffsetMap->isLocked());

    status = getPidInfo(pid, info, iter);
    assert(GPIS_FOUND == status);

    queueSanityCheck(info.pi_freeQueue);
    queueSanityCheck(info.pi_pendQueue);
}

#define PID_INFO_SANITY_CHECK(pid) do {                         \
    if (CHECK_QUEUES_FOR_SANITY)                                \
        pidInfoSanityCheck(pid);                                \
} while (0)

#else
#   define PID_INFO_SANITY_CHECK(pid)
#endif


packet_buf_t *
QuicShm::getNewPacketBuffer()
{
    LsShmOffset_t offset;
    packet_buf_t *packet_buf;

    int remapped = 0;
    offset = m_pPacketPool->alloc2(PACKETBUFSZIE, remapped);
    if (!offset)
    {
        LS_WARN("[QuicShm::getNewPacketBuffer] alloc2 failed");
        return NULL;
    }

    packet_buf = packetOffset2ptr(offset);
    if (!packet_buf)
    {
        LS_DBG_L("[QuicShm::getNewPacketBuffer] packetOffset2ptr(%u) failed",
                                                                        offset);
        m_pPacketPool->release2(offset, PACKETBUFSZIE);
        return NULL;
    }

    packet_buf->qpb_off = offset;
    packet_buf->prev_offset = 0;
    packet_buf->next_offset = 0;
    LS_DBG_L("[PID: %d] [QuicShm::getNewPacketBuffer] offset %d, packet %p", 
             s_pid, offset, packet_buf);
    return packet_buf;
}


unsigned QuicShm::getNewPacketBuffers(SetOfPacketBuffers &set, unsigned count)
{
    unsigned n;
    packet_buf_t *packet_bufs[count];

    m_pPacketPool->lock();

    for (n = 0; n < count; ++n)
    {
        packet_bufs[n] = getNewPacketBuffer();
        if (!packet_bufs[n])
            break;
        if (!set.insertPacketBuf(packet_bufs[n]))
        {
            m_pPacketPool->release2(packet_bufs[n]->qpb_off, PACKETBUFSZIE);
            break;
        }
        if (n > 0)
        {
            packet_bufs[n - 1]->next_offset = packet_bufs[n]->qpb_off;
            packet_bufs[n]->prev_offset = packet_bufs[n - 1]->qpb_off;
        }
    }

    m_pPacketPool->unlock();

    if (n)
    {
        m_pPidPacketOffsetMap->lock();
        appendPacketsToFreeQueue(packet_bufs[0], packet_bufs[n - 1]);
        /* XXX: handle failure of the call above? */
        PID_INFO_SANITY_CHECK(s_pid);
        m_pPidPacketOffsetMap->unlock();
    }

    return n;
}


void QuicShm::releasePacketBuffer(packet_buf_t *packet_buf)
{
    releasePacketBuffers(&packet_buf, 1);
}


/* Lock m_pPacketPool before calling this function */
void QuicShm::releasePacketBufferOff(LsShmOffset_t offset)
{
    LS_DBG_L("[PID: %d] [QuicShm::releasePacketBufferOff] offset %d.", s_pid, offset);

    m_pPacketPool->release2(offset, PACKETBUFSZIE);
}


void
QuicShm::releasePacketBuffers(class SetOfPacketBuffers &set, unsigned count)
{
    packet_buf_t *packet_bufs[count];
    unsigned n;

    for (n = 0; n < count; ++n)
    {
        packet_bufs[n] = set.removePacketBuf();
        if (!packet_bufs[n])
            break;
    }

    if (n)
        releasePacketBuffers(packet_bufs, n);
}



void QuicShm::releasePacketBuffers(packet_buf_t * const* packet_bufs,
                                   unsigned count)
{
    unsigned n;

    m_pPidPacketOffsetMap->lock();
    for (n = 0; n < count; ++n)
        if (0 != removeFromFreeQueue(packet_bufs[n]))
        {   /* Do not release packets that have not been removed from the
             * free queue:
             */
            count = n;
            break;
        }
    PID_INFO_SANITY_CHECK(s_pid);
    m_pPidPacketOffsetMap->unlock();

    if (count)
    {
        m_pPacketPool->lock();
        for (n = 0; n < count; ++n)
            releasePacketBufferOff(packet_bufs[n]->qpb_off);
        m_pPacketPool->unlock();
    }
}


struct quicshm_packet_buf *
QuicShm::packetOffset2ptr(LsShmOffset_t offset)
{
    int lastByteOffset = PACKETBUFSZIE - 1;
    uint8_t *pLastByte = (uint8_t *)m_pPacketPool->offset2ptr(offset + lastByteOffset);
    if (pLastByte)
        return (struct quicshm_packet_buf *) (pLastByte - lastByteOffset);
    else
        return NULL;
}


void
QuicShm::updatePidInfo(const QuicShm::PidInfo &info, LsShmHash::iterator &iter)
{
    memcpy(iter->getVal(), &info, sizeof(info));
}


int
QuicShm::createPidInfo(pid_t pid, const QuicShm::PidInfo &info)
{
    return m_pPidPacketOffsetMap->insert(&pid, sizeof(pid),
                                        &info, sizeof(info)) > 0 ? 0 : -1;
}


enum QuicShm::GpiStatus
QuicShm::getPidInfo(pid_t pid, QuicShm::PidInfo &info,
                                                LsShmHash::iterator &iter)
{
    ls_strpair_t parms;
    ls_str_set(&parms.key, (char *)&pid, sizeof(pid_t));
    LsShmHash::iteroffset iterOff = m_pPidPacketOffsetMap->findIterator(&parms);
    if (iterOff.m_iOffset == 0)
    {
        LS_DBG_H("%s: pid %d not found", __func__, pid);
        return GPIS_NOT_FOUND;
    }
    
    iter = m_pPidPacketOffsetMap->offset2iterator(iterOff);
    assert(iter);
    if (iter)
    {
        assert(iter->getValLen() == sizeof(info));
        memcpy(&info, iter->getVal(), sizeof(info));
        LS_DBG_H("%s: pid %d found", __func__, pid);
        return GPIS_FOUND;
    }
    else
    {
        LS_WARN("%s(%d): could not convert offset %" PRIu32" to iterator",
            __func__, pid, iterOff.m_iOffset);
        return GPIS_ERROR;
    }
}


int
QuicShm::getMyPidInfo(PidInfo &info)
{
    int s;
    pid_t pid;

    if (!m_pMyPidInfoIter)
    {
        pid = s_pid;
        info.reset();
        s = createPidInfo(pid, info);
        if (s != 0)
            return -1;
        if (GPIS_FOUND != getPidInfo(pid, info, m_pMyPidInfoIter))
            return -1;
    }

    memcpy(&info, m_pMyPidInfoIter->getVal(), sizeof(info));
    return 0;
}


int
QuicShm::removeFromFreeQueue(packet_buf_t *packetBuf)
{
    QuicShm::PidInfo info;
    packet_buf_t *prevPacketBuf, *nextPacketBuf;

    LS_DBG_M("[PID: %d] removeFromFreeQueue packet %u (prev: %u, next: %u). "
        , s_pid, packetBuf->qpb_off,
        packetBuf->prev_offset, packetBuf->next_offset);
 
    if (packetBuf->prev_offset)
        prevPacketBuf = packetOffset2ptr(packetBuf->prev_offset);
    else
        prevPacketBuf = NULL;

    if (packetBuf->next_offset)
        nextPacketBuf = packetOffset2ptr(packetBuf->next_offset);
    else
        nextPacketBuf = NULL;

    if (prevPacketBuf && nextPacketBuf)
    {                                   /* Removed from middle of list */
        prevPacketBuf->next_offset = nextPacketBuf->qpb_off;
        nextPacketBuf->prev_offset = prevPacketBuf->qpb_off;
    }
    else if (0 == getMyPidInfo(info))
    {
        if (prevPacketBuf)              /* Removed last packet */
        {
            info.pi_freeQueue.qh_lastOffset = prevPacketBuf->qpb_off;
            prevPacketBuf->next_offset = 0;
        }
        else if (nextPacketBuf)         /* Removed first packet */
        {
            info.pi_freeQueue.qh_firstOffset = nextPacketBuf->qpb_off;
            nextPacketBuf->prev_offset = 0;
        }
        else                            /* Removed only packet */
        {
            info.pi_freeQueue.qh_firstOffset = 0;
            info.pi_freeQueue.qh_lastOffset = 0;
        }
        updatePidInfo(info, m_pMyPidInfoIter);
    }
    else
    {
        LS_WARN("could not find header for our pid info (pid: %d)", s_pid);
        return -1;
    }

    PID_INFO_SANITY_CHECK(s_pid);

    packetBuf->prev_offset = 0;
    packetBuf->next_offset = 0;
    return 0;
}


/* Append packet buffer to the end of the pending queue.
 * This method assumes m_pPidPacketOffsetMap is locked.
 *
 * If something other than ATPQ_ERROR is returned, it means that the packet
 * buffer has been removed from the free queue.
 */
enum QuicShm::atpq
QuicShm::appendToPendQueue(pid_t pid, struct quicshm_packet_buf *packet_buf,
                            time_t now)
{    
    QuicShm::PidInfo info;
    packet_buf_t *last_buf;
    LsShmHash::iterator iter;
    QuicShm::GpiStatus status;
    enum atpq retval;

    status = getPidInfo(pid, info, iter);
    if (GPIS_ERROR == status)
        return ATPQ_ERROR;

    /* Before this, we return an error, which means that the packet buffer
     * is still on the free queue.  After removing it from the free queue,
     * the packet buffer must either be appended to corresponding free queue
     * or released.
     */
    if (0 != removeFromFreeQueue(packet_buf))
        return ATPQ_ERROR;

    if (GPIS_NOT_FOUND == status)
        return ATPQ_DROP_REL;

    if (info.pi_pendQueueCount >= MAX_PEND_QUEUE_BUFFER_COUNT)
    {
        ++info.pi_pendQueueDropped;
        LS_DBG_L("[PID: %d] [QuicShm::appendToPendQueue] pending queue for pid %d "
            "has %u packet bufs; dropping packet buf #%u", s_pid, pid,
            info.pi_pendQueueCount, info.pi_pendQueueDropped);
        updatePidInfo(info, iter);
        return ATPQ_SIGNAL_REL;
    }

    if (info.pi_pendQueue.qh_firstOffset > 0)
    {
        last_buf = getPacketBufferSt(info.pi_pendQueue.qh_lastOffset);
        /* TODO: check if last_buf is NULL? */
        assert(last_buf);
        assert(0 == last_buf->next_offset);
        last_buf->next_offset = packet_buf->qpb_off;
        packet_buf->prev_offset = last_buf->qpb_off;
        info.pi_pendQueue.qh_lastOffset = packet_buf->qpb_off;
        assert(info.pi_pendQueueCount > 0);
        info.pi_pendQueueCount++;
        /* XXX 1000 is an arbitrary value.  What should it be?  We could even
         * calculate it dynamically based on history.
         */
        if (info.pi_pendQueueSignalTime + 1 < now ||
            (info.pi_pendQueueSignalTime != now /* This makes sure this is not
                in the same call */ && info.pi_pendQueueCount > 1000
                ))
        {
            info.pi_pendQueueSignalTime = now;
            retval = ATPQ_SIGNAL;
        }
        else
            retval = ATPQ_OK;
    }
    else
    {
        info.pi_pendQueue.qh_firstOffset = packet_buf->qpb_off;
        info.pi_pendQueue.qh_lastOffset = packet_buf->qpb_off;
        info.pi_pendQueueCount = 1;
        info.pi_pendQueueSignalTime = now;
        retval = ATPQ_SIGNAL;
    }

    LS_DBG_M("[PID: %d] %s appended packet %u (prev: %u, next: %u) to pend queue, "
        "new chain [%u ... %u]", s_pid, __func__, packet_buf->qpb_off,
        packet_buf->prev_offset, packet_buf->next_offset,
        info.pi_pendQueue.qh_firstOffset, info.pi_pendQueue.qh_lastOffset);
    assert(packet_buf->next_offset == 0);

    updatePidInfo(info, iter);
    return retval;
}


/* Append a chain of already-linked packet buffers to the free queue.
 * This method assumes m_pPidPacketOffsetMap is locked.
 */
int
QuicShm::appendPacketsToFreeQueue(struct quicshm_packet_buf *begin,
                                  struct quicshm_packet_buf *end)
{    
    QuicShm::PidInfo info;
    packet_buf_t *last_buf;

    if (0 != getMyPidInfo(info))
        return -1;

    if (info.pi_freeQueue.qh_firstOffset > 0)
    {
        last_buf = getPacketBufferSt(info.pi_freeQueue.qh_lastOffset);
        /* TODO: check if last_buf is NULL? */
        assert(last_buf);
        assert(0 == last_buf->next_offset);
        last_buf->next_offset = begin->qpb_off;
        begin->prev_offset = last_buf->qpb_off;
        info.pi_freeQueue.qh_lastOffset = end->qpb_off;
    }
    else
    {
        info.pi_freeQueue.qh_firstOffset = begin->qpb_off;
        info.pi_freeQueue.qh_lastOffset = end->qpb_off;
    }

    LS_DBG_M("[PID: %d] %s appended chain [%u ... %u] to free queue, new chain [%u ... %u]",
        s_pid, __func__, begin->qpb_off, end->qpb_off,
        info.pi_freeQueue.qh_firstOffset,
        info.pi_freeQueue.qh_lastOffset);

    updatePidInfo(info, m_pMyPidInfoIter);
    PID_INFO_SANITY_CHECK(s_pid);
    return 0;
}


int
QuicShm::appendPacketsToFreeQueue(LsShmOffset_t firstOffset,
                                    LsShmOffset_t lastOffset)
{
    packet_buf_t *firstBuf, *lastBuf;

    firstBuf = packetOffset2ptr(firstOffset);
    lastBuf = packetOffset2ptr(lastOffset);
    if (firstBuf && lastBuf)
        return appendPacketsToFreeQueue(firstBuf, lastBuf);
    else
        return -1;
}


unsigned
QuicShm::distributeAlienPackets(ShmCidPidInfo *const *pids,
                             packet_buf_t *const *packet_bufs, unsigned count,
                             pid_t *pids_to_signal, unsigned *n_pids_to_signal)
{
    lsquic_cid_t cid;
    unsigned n, max_n_to_sig, i, n_to_rel, n_to_sig;
    time_t now;
    enum atpq atpq;
    unsigned to_release[count];

    n_to_rel = 0;
    n_to_sig = 0;
    max_n_to_sig = *n_pids_to_signal;
    now = time(NULL);

    m_pPidPacketOffsetMap->lock();
    for (n = 0; n < count; ++n)
    {
        atpq = appendToPendQueue(pids[n]->pid, packet_bufs[n], now);

        if (atpq == ATPQ_SIGNAL_REL || atpq == ATPQ_DROP_REL)
            to_release[ n_to_rel++ ] = n;

        switch (atpq)
        {
        case ATPQ_SIGNAL_REL:
        case ATPQ_SIGNAL:
            /* Find out whether this PID has already been recorded to be
             * signaled.  If it has been, there is nothing to do.  The
             * linear search might not be the smartest thing to employ,
             * but this approach is robust.  The number of PIDs should
             * be pretty small.
             */
            for (i = 0; i < n_to_sig; ++i)
                if (pids_to_signal[i] == pids[n]->pid)
                    goto already_signaled;
            if (i < max_n_to_sig)
            {
                pids_to_signal[i] = pids[n]->pid;
                ++n_to_sig;
            }
            else
            {   /* This means that the caller did not supply enough
                 * elements in the array to hold all the pids to be
                 * signaled:
                 */
                assert(i < max_n_to_sig);
            }
  already_signaled:
            break;
        case ATPQ_OK:
            break;
        case ATPQ_DROP_REL:
            {
                //mark CID->PID mapping with -2, PID is dead.
                pid_t pid = -2;
                memcpy(pids[n]->shmCidIter->getVal(), &pid, sizeof(pid_t));
                if (LS_LOG_ENABLED(log4cxx::Level::DBG_LESS))
                {
                    iter2cid(pids[n]->shmCidIter, &cid);
                    LS_DBG_LC("[QuicShm::distributeAlienPackets]: pid %d is "
                        "dead, mark CID %" CID_FMT, pids[n]->pid,
                        CID_BITS(&cid));
                }
            }
            break;
        case ATPQ_ERROR:
            goto end;
        }
    }

  end:
    PID_INFO_SANITY_CHECK(s_pid);
    m_pPidPacketOffsetMap->unlock();

    if (n_to_rel > 0)
    {
        m_pPacketPool->lock();
        for (i = 0; i < n_to_rel; ++i)
            releasePacketBufferOff(packet_bufs[ to_release[i] ]->qpb_off);
        m_pPacketPool->unlock();
    }
    *n_pids_to_signal = n_to_sig;

    return n;
}


#ifdef RUN_TEST
LsShmOffset_t
QuicShm::popFirstPendingOffset(pid_t pid)
{
    packet_buf_t *first_buf;
    QuicShm::PidInfo info;
    LsShmOffset_t retvalOffset;
    LsShmHash::iterator iter;
    
    retvalOffset = 0;
    m_pPidPacketOffsetMap->lock();
    if (getPidInfo(pid, info, iter) != GPIS_FOUND)
        goto end;

    if (info.pi_pendQueue.qh_firstOffset > 0)
    {
        retvalOffset = info.pi_pendQueue.qh_firstOffset;
        if (info.pi_pendQueue.qh_firstOffset ==
                                        info.pi_pendQueue.qh_lastOffset)
        {
            info.pi_pendQueue.qh_firstOffset = 0;
            info.pi_pendQueue.qh_lastOffset = 0;
        }
        else
        {
            first_buf = getPacketBufferSt(info.pi_pendQueue.qh_firstOffset);
            /* TODO: check if first_buf is NULL? */
            info.pi_pendQueue.qh_firstOffset = first_buf->next_offset;
        }
        updatePidInfo(info, iter);
    }

  end:
    m_pPidPacketOffsetMap->unlock();
    if (retvalOffset)
    {
        m_pPacketPool->lock();
        releasePacketBufferOff(retvalOffset);
        m_pPacketPool->unlock();
    }
    return retvalOffset;
}
#endif


QuicShm::PacketBufIter
QuicShm::getPendingPacketBuffers()
{
    QuicShm::PidInfo info;
    LsShmOffset_t firstOffset, lastOffset;

    m_pPidPacketOffsetMap->lock();
    if (0 != getMyPidInfo(info))
    {
        firstOffset = 0;
        lastOffset = 0;
        goto end;
    }

    firstOffset = info.pi_pendQueue.qh_firstOffset;
    lastOffset = info.pi_pendQueue.qh_lastOffset;
    LS_DBG_L("[PID: %d] [QuicShm::getPendingPacketBuffers] firstOffset %"
        PRIu32", lastOffset %" PRIu32", got %u packets, %u were dropped",
        s_pid, firstOffset, lastOffset,
        info.pi_pendQueueCount, info.pi_pendQueueDropped);
    info.pi_pendQueue.qh_firstOffset = 0;
    info.pi_pendQueue.qh_lastOffset = 0;
    info.pi_pendQueueCount = 0;
    info.pi_pendQueueDropped = 0;
    info.pi_pendQueueSignalTime = 0;
    updatePidInfo(info, m_pMyPidInfoIter);

    if (firstOffset && lastOffset)
        appendPacketsToFreeQueue(firstOffset, lastOffset);

  end:
    PID_INFO_SANITY_CHECK(s_pid);
    m_pPidPacketOffsetMap->unlock();
    return QuicShm::PacketBufIter(firstOffset, lastOffset);
}


void QuicShm::cleanupPidShm(pid_t pid)
{
    PidInfo info;
    LsShmHash::iterator iter;
    GpiStatus status;

    LS_DBG_L("[PID %d] [QuicShm::cleanupPidShm] cleaning up after pid %d%s", 
             s_pid, pid, pid == s_pid ? " (ourselves)" : "");

    m_pPidPacketOffsetMap->lock();
    status = getPidInfo(pid, info, iter);
    if (GPIS_FOUND == status)
    {
        /* Remove head of the queue first */
        m_pPidPacketOffsetMap->remove(&pid, sizeof(pid));
        m_pPidPacketOffsetMap->unlock();

        /* Release packets from the both queues if there are any */
        if (info.pi_freeQueue.qh_firstOffset ||
                                        info.pi_pendQueue.qh_firstOffset)
        {
            m_pPacketPool->lock();
            releasePacketsFromQueue(info.pi_freeQueue);
            releasePacketsFromQueue(info.pi_pendQueue);
            m_pPacketPool->unlock();
        }
    }
    else
        m_pPidPacketOffsetMap->unlock();

}


void
QuicShm::releasePacketsFromQueue(const QueueHead &queue)
{
    const packet_buf_t *packet_buf;
    LsShmOffset_t off, nextOff;

    off = queue.qh_firstOffset;
    while (off)
    {
        packet_buf = packetOffset2ptr(off);
        if (packet_buf)
            nextOff = packet_buf->next_offset;
        else
            nextOff = 0;
        releasePacketBufferOff(off);
        if (off == queue.qh_lastOffset)
            break;  /* Possible corruption? */
        off = nextOff;
    }
}


struct quicshm_packet_buf *
QuicShm::PacketBufIter::nextPacketBuf()
{
    struct quicshm_packet_buf *packet_buf;
    if (0 == m_nextOffset)
        return NULL;
    packet_buf = QuicShm::getInstance().getPacketBufferSt(m_nextOffset);
    if (packet_buf)
    {
        if (m_lastOffset == m_nextOffset)
            m_nextOffset = 0;
        else
            m_nextOffset = packet_buf->next_offset;
        return packet_buf;
    }
    else
        return NULL;
}


#ifdef RUN_TEST
void QuicShm::lockCidMap() { m_pCidMap->lock(); }
void QuicShm::unlockCidMap() { m_pCidMap->unlock(); }
#endif  /* RUN TEST */

#endif //_NOT_USE_SHM_
