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
#ifndef CACHEHASH_H
#define CACHEHASH_H



#include <lsdef.h>
#include <lsr/xxhash.h>
#include <util/ghash.h>

#define HASH_KEY_LEN 8
class CacheHash
{
public:
    CacheHash();

    ~CacheHash();

    void init(const CacheHash &rhs)
    {
        *this = rhs;
    }

    void initHash(XXH64_state_t *pState)
    {
        XXH64_reset(pState, 0);
    }

    void updHash(XXH64_state_t *pState, const char *pBuf, int len)
    {
        XXH64_update(pState, pBuf, len);
    }

    void saveHash(XXH64_state_t *pState)
    {
        *((uint64_t *)m_key) = XXH64_digest(pState);
    }

    const char *getKey() const
    {
        return m_key;
    }

    static hash_key_t to_ghash_key(const void *__s);
    static int  compare(const void *pVal1, const void *pVal2);

private:
    char  m_key[HASH_KEY_LEN];

};

#endif
