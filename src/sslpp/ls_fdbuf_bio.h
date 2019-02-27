/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#ifndef __LS_FDBUF_BIO_H__
#define __LS_FDBUF_BIO_H__

#ifdef __cplusplus 
extern "C" {
#endif

/**
 * @file ls_fdbuf_bio.h
 * @brief Include file for BIO library to separate BIO from SSL so that porting
 * it to user-land sockets is cleaner and so that buffered BIO is cleaner.
 */

typedef struct ls_fdbio_data {
    char        *m_rbioBuf;
    uint16_t     m_rbioBuffered;
    uint16_t     m_rbioIndex;
    uint16_t     m_rbioBufSz;
    uint8_t      m_rbioClosed;
    uint8_t      m_rbioWaitEvent;
} ls_fdbio_data;


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


#ifdef __cplusplus 
}
#endif

#endif // Protection define
