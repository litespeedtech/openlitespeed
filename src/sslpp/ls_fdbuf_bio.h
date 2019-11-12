/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

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
    uint16_t     m_rbuf_used;
    uint16_t     m_rbuf_read;
    uint16_t     m_rbuf_size;
    uint8_t      m_is_closed;
    uint8_t      m_need_read_event;

    uint8_t     *m_wbuf;
    int32_t      m_wbuf_size;
    int32_t      m_wbuf_used;
    int32_t      m_wbuf_sent;
    int32_t      m_flag;
} ls_fdbio_data;

#define LS_FDBIO_WBLOCK         1
#define LS_FDBIO_BUFFERING      2


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
    return fdbio->m_rbuf && fdbio->m_rbuf_used == fdbio->m_rbuf_read;
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
