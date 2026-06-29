/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#include <lsr/ls_base64.h>

#include <errno.h>
#include <limits.h>

static const unsigned char s_ls_decodeTable[128] =
{
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* 00-0F */
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, /* 10-1F */
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62, 255, 255, 255, 63, /* 20-2F */
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255, 255, 255, 255, 255, 255, /* 30-3F */
    255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,          /* 40-4F */
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 255, 255, 255, 255, 255, /* 50-5F */
    255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 60-6F */
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 255, 255, 255, 255, 255 /* 70-7F */
};

static const unsigned char s_ls_encodeTable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


static int ls_base64_decodedlen(int size)
{
    return (size / 4) * 3 + ((size % 4) * 3) / 4;
}


int ls_base64_decode(const char *encoded, int size, char *decoded,
                     int decodedLen)
{
    if ((size < 0) || (decodedLen < 0) || ((size > 0) && !encoded)
        || ((decodedLen > 0) && !decoded))
    {
        errno = EINVAL;
        return -1;
    }
    if (size == 0)
    {
        if (decodedLen > 0)
            *decoded = 0;
        return 0;
    }
    int needLen = ls_base64_decodedlen(size);
    if (decodedLen < needLen)
    {
        errno = ENOSPC;
        return -1;
    }
    if (needLen == 0)
    {
        if (decodedLen > 0)
            *decoded = 0;
        return 0;
    }
    const char    *pEncoded = encoded;
    unsigned char  e1,
             prev_e = 0;
    char           phase = 0;
    unsigned char *pDecoded = (unsigned char *)decoded;
    unsigned char *pDecodedEnd = pDecoded + decodedLen;
    const char    *pEnd = encoded + size;

    while (pEncoded < pEnd)
    {
        unsigned char ch = *pEncoded++;
        if (ch >= sizeof(s_ls_decodeTable))
            continue;
        e1 = s_ls_decodeTable[ch];
        if (e1 != 255)
        {
            switch (phase)
            {
            case 0:
                break;
            case 1:
                if (pDecoded >= pDecodedEnd)
                {
                    errno = ENOSPC;
                    return -1;
                }
                *pDecoded++ = ((prev_e << 2) | ((e1 & 0x30) >> 4));
                break;
            case 2:
                if (pDecoded >= pDecodedEnd)
                {
                    errno = ENOSPC;
                    return -1;
                }
                *pDecoded++ =
                    (((prev_e & 0xf) << 4) | ((e1 & 0x3c) >> 2));
                break;
            case 3:
                if (pDecoded >= pDecodedEnd)
                {
                    errno = ENOSPC;
                    return -1;
                }
                *pDecoded++ = (((prev_e & 0x03) << 6) | e1);
                phase = -1;
                break;
            }
            ++phase;
            prev_e = e1;
        }
    }
    if (pDecoded < pDecodedEnd)
        *pDecoded = 0;
    return pDecoded - (unsigned char *)decoded;
}


int ls_base64_encode(const char *decoded, int size, char *encoded,
                     int encodedLen)
{
    if ((size < 0) || (encodedLen < 0)
        || ((size > 0) && !decoded) || ((encodedLen > 0) && !encoded))
    {
        errno = EINVAL;
        return -1;
    }
    if (size > (INT_MAX / 4) * 3 - 2)
    {
        errno = EINVAL;
        return -1;
    }
    if (encodedLen < ls_base64_encodelen(size))
    {
        errno = ENOSPC;
        return -1;
    }
    if (size == 0)
        return 0;
    const unsigned char *pDecoded = (const unsigned char *)decoded;
    const unsigned char *pEnd = (const unsigned char *)decoded + size;
    char                *pEncoded = encoded;
    unsigned char        ch;

    size %= 3;
    pEnd -= size;

    while (pEnd > pDecoded)
    {
        *pEncoded++ = s_ls_encodeTable[((ch = *pDecoded++) >> 2) & 0x3f];
        *pEncoded++ = s_ls_encodeTable[((ch & 0x03) << 4) | (*pDecoded >> 4)];
        ch = *pDecoded++;
        *pEncoded++ = s_ls_encodeTable[((ch & 0x0f) << 2) | (*pDecoded >> 6)];
        *pEncoded++ = s_ls_encodeTable[*pDecoded++ & 0x3f];
    }

    if (size > 0)
    {
        *pEncoded++ = s_ls_encodeTable[((ch = *pDecoded++) >> 2) & 0x3f];

        if (size == 1)
        {
            *pEncoded++ = s_ls_encodeTable[(ch & 0x03) << 4];
            *pEncoded++ = '=';
        }
        else
        {
            *pEncoded++ =
                s_ls_encodeTable[((ch & 0x03) << 4) | (*pDecoded >> 4)];
            *pEncoded++ = s_ls_encodeTable[(*pDecoded & 0x0f) << 2];
        }
        *pEncoded++ = '=';
    }

    return pEncoded - encoded;
}
