/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2026  LiteSpeed Technologies, Inc.                       *
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
*    along with this program. If not, see <https://www.gnu.org/licenses/>.   *
*****************************************************************************/
#ifndef BLAKE2B_H
#define BLAKE2B_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define BLAKE2B_BLOCKBYTES 128
#define BLAKE2B_OUTBYTES   64
#define BLAKE2B_KEYBYTES   64

typedef struct
{
    uint64_t h[8];
    uint64_t t[2];
    uint64_t f[2];
    unsigned char buf[BLAKE2B_BLOCKBYTES];
    size_t buflen;
    size_t outlen;
} blake2b_ctx;

int blake2b_init(blake2b_ctx *ctx, size_t outlen);
int blake2b_init_key(blake2b_ctx *ctx, size_t outlen,
                     const void *key, size_t keylen);
int blake2b_update(blake2b_ctx *ctx, const void *input, size_t input_len);
int blake2b_final(blake2b_ctx *ctx, void *output, size_t output_len);
int blake2b(void *output, size_t output_len,
            const void *key, size_t key_len,
            const void *input, size_t input_len);

#ifdef __cplusplus
}
#endif

#endif
