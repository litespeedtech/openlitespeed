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
#ifndef PBSET_H
#define PBSET_H

#include <stddef.h>
#include <sys/queue.h>

struct quicshm_packet_buf;


class SetOfPacketBuffers
{
    private:
        STAILQ_HEAD(, SetOfPbBucket)        m_BucketsAll;
        TAILQ_HEAD(, SetOfPbBucket)         m_BucketsAvail;
        unsigned                            m_uCount;

    public:
        SetOfPacketBuffers()
            :m_uCount(0)
        {
            STAILQ_INIT(&m_BucketsAll);
            TAILQ_INIT(&m_BucketsAvail);
        }

        virtual ~SetOfPacketBuffers();

        /* Insert packet buffer into the set.  A failure is returned if
         * memory could not be allocated.
         */
        bool insertPacketBuf(struct quicshm_packet_buf *);

        /* If the set is not empty, an arbitrary packet buffer is removed
         * from the set and is returned.  If the set is empty, the return
         * value is NULL;
         */
        struct quicshm_packet_buf * removePacketBuf();

        unsigned count() const
        {
            return m_uCount;
        }
};

#endif
