/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
#ifndef LSR_MD5_H
#define LSR_MD5_H

#include <stdint.h>
#include <string.h>

/**
 * @file
 * This file provides an option for anyone who wishes to not use OpenSSL's version
 * of MD5.  To select implementations, either have or comment out USE_OPENSSL_MD5.
 * 
 * By default, OpenSSL's version is used.
 * 
 * To use, user must either:
 *      \li Call #lsr_md5
 *      \li Call #lsr_md5_init, #lsr_md5_update (May call this multiple times), #lsr_md5_final in that order.
 */

#define USE_OPENSSL_MD5


#define LSR_MD5_CHUNKSIZE 16
#define LSR_MD5_BUFSIZE 64

#ifdef __cplusplus
extern "C" {
#endif
    

#if defined( USE_OPENSSL_MD5 )
#include <openssl/md5.h>
typedef MD5_CTX lsr_md5_ctx_t;
#else
typedef struct lsr_md5_ctx_s
{
    uint32_t A, B, C, D,
            lo, hi;
    uint32_t chunk[LSR_MD5_CHUNKSIZE];
    unsigned char shiftBuf[LSR_MD5_BUFSIZE];
} lsr_md5_ctx_t;
#endif

/** @lsr_md5_init
 * @brief Initializes the MD5 ctx.  Must be called first if the user is
 * doing the calls him/herself.
 * 
 * @param[in] ctx - A pointer to an allocated ctx.
 */
int             lsr_md5_init( lsr_md5_ctx_t *ctx );

/** @lsr_md5_update
 * @brief Updates the MD5 with a new buffer.
 * 
 * @param[in] ctx - A pointer to an initialized ctx.
 * @param[in] p - A pointer to the buffer to add to the MD5.
 * @param[in] len - The length of the buffer.
 */
int             lsr_md5_update( lsr_md5_ctx_t *ctx, const void *p, size_t len );

/** @lsr_md5_final
 * @brief Calculates the result after finishing the MD5.
 * 
 * @param[out] ret - A pointer to the result of the hash.
 * @param[in] ctx - A pointer to an initialized ctx.
 */
int             lsr_md5_final( unsigned char *ret, lsr_md5_ctx_t *ctx );

/** @lsr_md5
 * @brief Calculates the \e p buffer and returns the result.
 * 
 * @param[in] p - A pointer to the buffer to calculate.
 * @param[in] len - The length of p.
 * @param[out] ret - A pointer to the result of the calculation.
 * @return Ret on success, NULL on failure.
 */
unsigned char  *lsr_md5( const unsigned char *p, size_t len, unsigned char *ret );

#ifdef __cplusplus
}
#endif

#endif // LSR_MD5_H
