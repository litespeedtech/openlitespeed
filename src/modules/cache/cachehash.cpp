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
#include "cachehash.h"
#include <zlib.h>
#include <lsr/ls_crc64.h>
#include <string.h>

CacheHash::CacheHash()
{
}


CacheHash::~CacheHash()
{
}


#ifdef notdef
void CacheHash::init()
{
    memset(m_key, 0, 8);
}


void CacheHash::hash(const char *pBuf, int len)
{
    //*((uint64_t *)m_key) = ls_crc64( *((uint64_t *)m_key), (const unsigned char *)pBuf, len );
    uint64_t *pKey = (uint64_t *)m_key;
    *pKey = ls_crc64(*pKey, (const unsigned char *)pBuf, len);
}
#endif


hash_key_t CacheHash::to_ghash_key(const void *__s)
{
    hash_key_t __h = *((uint64_t *)__s);

    return __h;
}


int CacheHash::compare(const void *pVal1, const void *pVal2)
{   return  *((uint64_t *)pVal1) - *((uint64_t *)pVal2);    }

