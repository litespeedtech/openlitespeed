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

#ifndef __LS_FDBUF_BIO_H__
#define __LS_FDBUF_BIO_H__

#include <lsdef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file ls_fdbuf_bio.h
 * @brief Include file for BIO library to separate BIO from SSL so that porting
 * it to user-land sockets is cleaner and so that buffered BIO is cleaner.
 */

typedef struct ls_fdbio_data {
    uint8_t     *m_rbuf;
    uint16_t     m_rbuf_max_block;
    uint16_t     m_rbuf_used;
    uint16_t     m_rbuf_read;
    uint16_t     m_rbuf_size;

    uint8_t     *m_wbuf;
    int32_t      m_wbuf_size;
    int32_t      m_wbuf_used;
    int32_t      m_wbuf_sent;
    int32_t      m_flag;
} ls_fdbio_data;

enum LS_FDBIO_FLAG
{
    LS_FDBIO_WBLOCK = 1,
    LS_FDBIO_BUFFERING = 2,
    LS_FDBIO_CLOSED = 4,
    LS_FDBIO_NEED_READ_EVT = 8,
    LS_FDBIO_RBUF_ALLOC = 16
};

/**
 * @brief ls_fdbuf_bio_init called during the connection constructor,
 * initializes the BIO access.
 * @param[out] fdbio is initialized for later use.
 * @return None.
 */
void ls_fdbuf_bio_init(ls_fdbio_data *fdbio);


/**
 * @brief ls_fdbio_create initializes the use of BIOs.  Call once per
 * connection.
 * @param[in] fd The socket fd for the connection.
 * @param[out] fdbio ls_fdbuf_bio to be used as the BIO for the connection.
 * Allocated by caller.
 * @return NULL to indicate an error.
 */
struct bio_st *ls_fdbio_create(int fd, ls_fdbio_data *fdbio);

int ls_fdbio_flush(ls_fdbio_data *fdbio, int fd);
int ls_fdbio_buff_input(ls_fdbio_data *fdbio, int fd);

ls_inline void ls_fdbio_set_wbuff(ls_fdbio_data *fdbio, int dobuff)
{
    if (dobuff)
        fdbio->m_flag |= LS_FDBIO_BUFFERING;
    else
        fdbio->m_flag &= ~LS_FDBIO_BUFFERING;
}

void ls_fdbio_release_wbuff(ls_fdbio_data *fdbio);
void ls_fdbio_release_rbuff(ls_fdbio_data *fdbio);

ls_inline int ls_fdbio_is_wbuf_idle(ls_fdbio_data *fdbio)
{
    return fdbio->m_wbuf && fdbio->m_wbuf_used == 0;
}

ls_inline int ls_fdbio_is_rbuf_idle(ls_fdbio_data *fdbio)
{
    return (fdbio->m_flag & LS_FDBIO_RBUF_ALLOC)
            && fdbio->m_rbuf_used == fdbio->m_rbuf_read;
}

ls_inline void ls_fdbio_release_idle_buffer(ls_fdbio_data *fdbio)
{
    if (ls_fdbio_is_wbuf_idle(fdbio))
        ls_fdbio_release_wbuff(fdbio);
    if (ls_fdbio_is_rbuf_idle(fdbio))
        ls_fdbio_release_rbuff(fdbio);
}

ls_inline int ls_fdbio_is_wblock(ls_fdbio_data *fdbio)
{   return fdbio->m_flag & LS_FDBIO_WBLOCK;      }

ls_inline void ls_fdbio_clear_wblock(ls_fdbio_data *fdbio)
{    fdbio->m_flag &= ~LS_FDBIO_WBLOCK;    }

#ifdef __cplusplus
}
#endif

#endif // Protection define
