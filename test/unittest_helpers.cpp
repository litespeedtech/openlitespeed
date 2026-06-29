/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2026  LiteSpeed Technologies, Inc.                 *
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
#ifdef RUN_TEST

#include <lsdef.h>

#include <string.h>
#include <unistd.h>

extern char *argv0;

const char *get_server_root(char *achServerRoot, ssize_t sz)
{
    if (*argv0 != '/')
    {
        if (getcwd(achServerRoot, sz))
            lstrncat(achServerRoot, "/", sz);
    }
    else
        achServerRoot[0] = 0;
    lstrncat(achServerRoot, argv0, sz);
    const char *pEnd = strrchr(achServerRoot, '/');
    if (pEnd)
    {
        --pEnd;
        while (pEnd > achServerRoot && *pEnd != '/')
            --pEnd;
        --pEnd;
        while (pEnd > achServerRoot && *pEnd != '/')
            --pEnd;
        ++pEnd;

        lstrncpy(&achServerRoot[pEnd - achServerRoot], "test/serverroot",
                 sz - (pEnd - achServerRoot));
    }
    else
        lstrncpy(achServerRoot, "/", sz);
    return achServerRoot;
}

#endif
