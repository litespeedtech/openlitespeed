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
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>


/*
*   ls_expandfile - expanding current file
*   return 0 if file expanded
*   otherwise return -1 
* 
*   if incrsize < 0 the file will be reduced.
*/
int ls_expandfile(int fd, size_t fromsize, size_t incrsize)
{
    register size_t fromloc;
    register int pagesize = getpagesize();
    register size_t newsize = fromsize + incrsize;
    
    if ( ftruncate(fd, newsize) )
        return -1 ;

    if (newsize <= fromsize)
        return 0;

    fromloc = fromsize++;
    do
    {
        if (pwrite(fd, "", 1, fromsize) != 1)
        {
            ftruncate(fd, fromloc);
            return -1;
        }
        fromsize += pagesize;
    }
    while (fromsize < newsize) ;
    return 0;
}

