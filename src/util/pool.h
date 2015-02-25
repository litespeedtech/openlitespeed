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
#ifndef POOL_H
#define POOL_H



#ifdef _REENTRENT
#include <thread/tmutex.h>
#endif

#include <stddef.h>

#define MAX_UNIT_SIZE    256

class Pool
{
public:

    enum {_ALIGN = 4 };
    static size_t
    roundUp(size_t bytes)
    { return (((bytes) + (size_t) _ALIGN - 1) & ~((size_t) _ALIGN - 1)); }

private:

public:

    explicit Pool(int max_unit_bytes = 256);
    ~Pool();

    // num must be > 0
    void *allocate(size_t num);

    // p may not be 0
    void deallocate(void *p, size_t num);

    void *reallocate(void *p, size_t old_sz, size_t new_sz);

    void *allocate2(size_t num);
    void   deallocate2(void *p);
    void *reallocate2(void *p, size_t new_sz);
    char *dupstr(const char *p);
    char *dupstr(const char *p, int len);
};

extern Pool g_pool;

#endif
