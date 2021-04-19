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

#ifndef QUIC_PACKETBUF_H
#define QUIC_PACKETBUF_H

#include <stdint.h>
#include <shm/lsshmtypes.h>

/***
 * Because alloc2 will always get 2048 bytes from the pool,
 * we needn't to save memory here.
 * Packet data format in the SHM pool
 * 40 bytes reserve data
 * prev packet offset  4 bytes
 * next packet offset  4 bytes
 * current packet length 4 bytes
 * struct sockaddr_storage local_addr 128 bytes (sizeof(struct sockaddr_storage))
 * struct sockaddr_storage peer_addr  128 bytes
 * ecn  1 byte
 * packet data (current packet length bytes)
 * 
 * PACKETBUFSZIE = 1683 (hold 40 + 4 + 4 + 4 + sizeof(struct sockaddr_storage) * 2 + QUIC_SHM_PACKET_SIZE + 1)
 */
#define  PACKETBUFRESERVEHEAD   40
typedef struct quicshm_packet_buf
{
    uint8_t reserve[PACKETBUFRESERVEHEAD];
    /* qpb_off is populated when packet buffer if allocated.  There is no
     * need to access this value outside of quicshm.cpp.
     */
    LsShmOffset_t qpb_off;
    LsShmOffset_t prev_offset;
    LsShmOffset_t next_offset;
    int32_t  data_len;
    struct sockaddr_storage local_addr;
    struct sockaddr_storage peer_addr;
    uint8_t ecn;
    uint8_t data[
#ifndef _NOT_USE_SHM_
                 QUIC_SHM_PACKET_SIZE
#else
                 1472
#endif
                                     ];
} packet_buf_t;
#define  PACKETBUFSZIE (sizeof(packet_buf_t))

#endif
