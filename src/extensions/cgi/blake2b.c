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
/* Compact portable BLAKE2b implementation, following RFC 7693. */
#include "blake2b.h"

#include <string.h>

static const uint64_t blake2b_iv[8] =
{
    0x6A09E667F3BCC908ULL, 0xBB67AE8584CAA73BULL,
    0x3C6EF372FE94F82BULL, 0xA54FF53A5F1D36F1ULL,
    0x510E527FADE682D1ULL, 0x9B05688C2B3E6C1FULL,
    0x1F83D9ABFB41BD6BULL, 0x5BE0CD19137E2179ULL
};

static const unsigned char blake2b_sigma[12][16] =
{
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 },
    {14,10, 4, 8, 9,15,13, 6, 1,12, 0, 2,11, 7, 5, 3 },
    {11, 8,12, 0, 5, 2,15,13,10,14, 3, 6, 7, 1, 9, 4 },
    { 7, 9, 3, 1,13,12,11,14, 2, 6, 5,10, 4, 0,15, 8 },
    { 9, 0, 5, 7, 2, 4,10,15,14, 1,11,12, 6, 8, 3,13 },
    { 2,12, 6,10, 0,11, 8, 3, 4,13, 7, 5,15,14, 1, 9 },
    {12, 5, 1,15,14,13, 4,10, 0, 7, 6, 3, 9, 2, 8,11 },
    {13,11, 7,14,12, 1, 3, 9, 5, 0,15, 4, 8, 6, 2,10 },
    { 6,15,14, 9,11, 3, 0, 8,12, 2,13, 7, 1, 4,10, 5 },
    {10, 2, 8, 4, 7, 6, 1, 5,15,11, 9,14, 3,12,13, 0 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 },
    {14,10, 4, 8, 9,15,13, 6, 1,12, 0, 2,11, 7, 5, 3 }
};

static uint64_t load64(const void *src)
{
    const unsigned char *p = (const unsigned char *)src;
    return (uint64_t)p[0] | ((uint64_t)p[1] << 8)
         | ((uint64_t)p[2] << 16) | ((uint64_t)p[3] << 24)
         | ((uint64_t)p[4] << 32) | ((uint64_t)p[5] << 40)
         | ((uint64_t)p[6] << 48) | ((uint64_t)p[7] << 56);
}

static void store64(void *dst, uint64_t value)
{
    unsigned char *p = (unsigned char *)dst;
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
    p[4] = (unsigned char)(value >> 32);
    p[5] = (unsigned char)(value >> 40);
    p[6] = (unsigned char)(value >> 48);
    p[7] = (unsigned char)(value >> 56);
}

static uint64_t rotr64(uint64_t value, unsigned int count)
{
    return (value >> count) | (value << (64 - count));
}

static void secure_zero(void *ptr, size_t len)
{
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--)
        *p++ = 0;
}

static void blake2b_increment(blake2b_ctx *ctx, uint64_t count)
{
    ctx->t[0] += count;
    ctx->t[1] += ctx->t[0] < count;
}

#define G(r, i, a, b, c, d) do {                                      \
    a = a + b + m[blake2b_sigma[r][2 * i]];                            \
    d = rotr64(d ^ a, 32);                                              \
    c += d;                                                             \
    b = rotr64(b ^ c, 24);                                              \
    a = a + b + m[blake2b_sigma[r][2 * i + 1]];                        \
    d = rotr64(d ^ a, 16);                                              \
    c += d;                                                             \
    b = rotr64(b ^ c, 63);                                              \
} while (0)

static void blake2b_compress(blake2b_ctx *ctx,
                             const unsigned char block[BLAKE2B_BLOCKBYTES])
{
    uint64_t m[16];
    uint64_t v[16];
    unsigned int i;
    unsigned int r;

    for (i = 0; i < 16; ++i)
        m[i] = load64(block + i * sizeof(uint64_t));
    for (i = 0; i < 8; ++i)
    {
        v[i] = ctx->h[i];
        v[i + 8] = blake2b_iv[i];
    }
    v[12] ^= ctx->t[0];
    v[13] ^= ctx->t[1];
    v[14] ^= ctx->f[0];
    v[15] ^= ctx->f[1];

    for (r = 0; r < 12; ++r)
    {
        G(r, 0, v[0], v[4], v[8],  v[12]);
        G(r, 1, v[1], v[5], v[9],  v[13]);
        G(r, 2, v[2], v[6], v[10], v[14]);
        G(r, 3, v[3], v[7], v[11], v[15]);
        G(r, 4, v[0], v[5], v[10], v[15]);
        G(r, 5, v[1], v[6], v[11], v[12]);
        G(r, 6, v[2], v[7], v[8],  v[13]);
        G(r, 7, v[3], v[4], v[9],  v[14]);
    }
    for (i = 0; i < 8; ++i)
        ctx->h[i] ^= v[i] ^ v[i + 8];
}

#undef G

static int blake2b_init_internal(blake2b_ctx *ctx, size_t outlen,
                                 size_t keylen)
{
    unsigned int i;

    if (!ctx || outlen == 0 || outlen > BLAKE2B_OUTBYTES
        || keylen > BLAKE2B_KEYBYTES)
        return -1;
    memset(ctx, 0, sizeof(*ctx));
    for (i = 0; i < 8; ++i)
        ctx->h[i] = blake2b_iv[i];
    ctx->h[0] ^= 0x01010000 ^ ((uint64_t)keylen << 8) ^ (uint64_t)outlen;
    ctx->outlen = outlen;
    return 0;
}

int blake2b_init(blake2b_ctx *ctx, size_t outlen)
{
    return blake2b_init_internal(ctx, outlen, 0);
}

int blake2b_init_key(blake2b_ctx *ctx, size_t outlen,
                     const void *key, size_t keylen)
{
    unsigned char block[BLAKE2B_BLOCKBYTES];
    int result;

    if (!key || keylen == 0)
        return -1;
    result = blake2b_init_internal(ctx, outlen, keylen);
    if (result != 0)
        return result;
    memset(block, 0, sizeof(block));
    memcpy(block, key, keylen);
    result = blake2b_update(ctx, block, sizeof(block));
    secure_zero(block, sizeof(block));
    return result;
}

int blake2b_update(blake2b_ctx *ctx, const void *input, size_t input_len)
{
    const unsigned char *in = (const unsigned char *)input;
    size_t fill;

    if (!ctx || (!input && input_len != 0))
        return -1;
    if (input_len == 0)
        return 0;

    fill = BLAKE2B_BLOCKBYTES - ctx->buflen;
    if (input_len > fill)
    {
        memcpy(ctx->buf + ctx->buflen, in, fill);
        blake2b_increment(ctx, BLAKE2B_BLOCKBYTES);
        blake2b_compress(ctx, ctx->buf);
        ctx->buflen = 0;
        in += fill;
        input_len -= fill;
        while (input_len > BLAKE2B_BLOCKBYTES)
        {
            blake2b_increment(ctx, BLAKE2B_BLOCKBYTES);
            blake2b_compress(ctx, in);
            in += BLAKE2B_BLOCKBYTES;
            input_len -= BLAKE2B_BLOCKBYTES;
        }
    }
    memcpy(ctx->buf + ctx->buflen, in, input_len);
    ctx->buflen += input_len;
    return 0;
}

int blake2b_final(blake2b_ctx *ctx, void *output, size_t output_len)
{
    unsigned char buffer[BLAKE2B_OUTBYTES];
    unsigned int i;

    if (!ctx || !output || output_len < ctx->outlen || ctx->f[0] != 0)
        return -1;
    blake2b_increment(ctx, (uint64_t)ctx->buflen);
    ctx->f[0] = UINT64_MAX;
    memset(ctx->buf + ctx->buflen, 0, BLAKE2B_BLOCKBYTES - ctx->buflen);
    blake2b_compress(ctx, ctx->buf);
    for (i = 0; i < 8; ++i)
        store64(buffer + sizeof(uint64_t) * i, ctx->h[i]);
    memcpy(output, buffer, ctx->outlen);
    secure_zero(buffer, sizeof(buffer));
    secure_zero(ctx, sizeof(*ctx));
    return 0;
}

int blake2b(void *output, size_t output_len,
            const void *key, size_t key_len,
            const void *input, size_t input_len)
{
    blake2b_ctx ctx;
    int result;

    if (key_len != 0)
        result = blake2b_init_key(&ctx, output_len, key, key_len);
    else
        result = blake2b_init(&ctx, output_len);
    if (result == 0)
        result = blake2b_update(&ctx, input, input_len);
    if (result == 0)
        result = blake2b_final(&ctx, output, output_len);
    if (result != 0)
        secure_zero(&ctx, sizeof(ctx));
    return result;
}

