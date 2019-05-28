/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
#include "cachehash.h"
#include <zlib.h>
#include <string.h>

CacheHash::CacheHash()
{
}


CacheHash::~CacheHash()
{
}

void CacheHash::update(XXH64_state_t *pState, const char *pBuf, int len)
{
    XXH64_update(pState, pBuf, len);
}

hash_key_t CacheHash::to_ghash_key(const void *__s)
{
    register hash_key_t __h = *((uint64_t *)__s);

    return __h;
}

int CacheHash::compare(const void *pVal1, const void *pVal2)
{   return  *((uint64_t *)pVal1) - *((uint64_t *)pVal2);    }


const char *CacheHash::to_str(char *buf) const
{
    return to_str(getKey(), buf);
}


const char *CacheHash::to_str(const unsigned char *key, char *buf)
{
    static char s_buf[17];
    if (buf == NULL)
        buf = s_buf;
    snprintf(buf, 17, "%02x%02x%02x%02x%02x%02x%02x%02x",
             key[0], key[1], key[2], key[3], key[4], key[5], key[6], key[7]);
    return buf;
}

