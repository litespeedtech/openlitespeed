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

#include <assert.h>
#include <stdint.h>

#include <shm/lsshmtypes.h>

#include "pbset.h"


struct SetOfPbBucket
{
    explicit SetOfPbBucket(struct quicshm_packet_buf *packet_buf)
    {
        sopb_slots          = 1;
        sopb_packet_bufs[0] = packet_buf;
    }
    
    STAILQ_ENTRY(SetOfPbBucket)    sopb_next_all;
    TAILQ_ENTRY(SetOfPbBucket)     sopb_next_avail;
    uint64_t                       sopb_slots;
    struct quicshm_packet_buf *    sopb_packet_bufs[64];
};


static unsigned
find_free_slot (uint64_t slots)
{
#if __GNUC__
    return __builtin_ffsll(~slots) - 1;
#else
    unsigned n;

    slots =~ slots;
    n = 0;

    if (0 == (slots & ((1ULL << 32) - 1))) { n += 32; slots >>= 32; }
    if (0 == (slots & ((1ULL << 16) - 1))) { n += 16; slots >>= 16; }
    if (0 == (slots & ((1ULL <<  8) - 1))) { n +=  8; slots >>=  8; }
    if (0 == (slots & ((1ULL <<  4) - 1))) { n +=  4; slots >>=  4; }
    if (0 == (slots & ((1ULL <<  2) - 1))) { n +=  2; slots >>=  2; }
    if (0 == (slots & ((1ULL <<  1) - 1))) { n +=  1; slots >>=  1; }
    return n;
#endif
}


static unsigned
find_used_slot (uint64_t slots)
{
    return find_free_slot(~slots);
}


SetOfPacketBuffers::~SetOfPacketBuffers()
{
    struct SetOfPbBucket *bucket, *next;
#if __GNUC__ && !defined(NDEBUG)
    unsigned count = 0;
#endif
    for (bucket = STAILQ_FIRST(&m_BucketsAll); bucket; bucket = next)
    {
        next = STAILQ_NEXT(bucket, sopb_next_all);
#if __GNUC__ && !defined(NDEBUG)
        count += __builtin_popcountl(bucket->sopb_slots);
#endif
        delete bucket;
    }
#if __GNUC__ && !defined(NDEBUG)
    assert(count == m_uCount);
#endif
}


struct quicshm_packet_buf *
SetOfPacketBuffers::removePacketBuf()
{
    struct SetOfPbBucket *bucket;
    struct quicshm_packet_buf *packet_buf;
    unsigned slot;
    
    bucket = STAILQ_FIRST(&m_BucketsAll);
    if (bucket)
    {
        assert(bucket->sopb_slots != 0);
        if (~0ULL == bucket->sopb_slots)
        {
            TAILQ_INSERT_HEAD(&m_BucketsAvail, bucket, sopb_next_avail);
            slot = 1;
        }
        else
            slot = find_used_slot(bucket->sopb_slots);
        packet_buf = bucket->sopb_packet_bufs[slot];
        bucket->sopb_slots &= ~(1ULL << slot);
        if (0 == bucket->sopb_slots)
        {
            STAILQ_REMOVE_HEAD(&m_BucketsAll, sopb_next_all);
            TAILQ_REMOVE(&m_BucketsAvail, bucket, sopb_next_avail);
            delete bucket;
        }
        --m_uCount;
        return packet_buf;
    }
    else
        return 0;
}


bool
SetOfPacketBuffers::insertPacketBuf(struct quicshm_packet_buf *packet_buf)
{
    struct SetOfPbBucket *bucket;
    unsigned slot;

    bucket = TAILQ_FIRST(&m_BucketsAvail);
    if (bucket)
    {
        assert(~0ULL != bucket->sopb_slots);
        slot = find_free_slot(bucket->sopb_slots);
        bucket->sopb_packet_bufs[slot] = packet_buf;
        bucket->sopb_slots |= 1ULL << slot;
        if (~0ULL == bucket->sopb_slots)
            TAILQ_REMOVE(&m_BucketsAvail, bucket, sopb_next_avail);
        ++m_uCount;
        return true;
    }
    else if ((bucket = new SetOfPbBucket(packet_buf)))
    {
        STAILQ_INSERT_HEAD(&m_BucketsAll, bucket, sopb_next_all);
        TAILQ_INSERT_HEAD(&m_BucketsAvail, bucket, sopb_next_avail);
        ++m_uCount;
        return true;
    }
    else
        return false;
}
