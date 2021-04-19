/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2021  LiteSpeed Technologies, Inc.                 *
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

#include <util/ni_fio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>


int nio_creat(const char *pathname, mode_t mode)
{
    int ret;
    while (1)
    {
        ret = creat(pathname, mode);
        if ((ret == -1) && (errno == EINTR))
            continue;
        return ret;
    }
}

int nio_open(const char *pathname, int flags, mode_t mode)
{
    int ret;
    while (1)
    {
        ret = open(pathname, flags, mode);
        if ((ret == -1) && (errno == EINTR))
            continue;
        return ret;
    }
}

int nio_close(int fd)
{
    int ret;
    while (1)
    {
        ret = close(fd);
        if ((ret == -1) && (errno == EINTR))
            continue;
        return ret;
    }
}

int nio_read(int fd, void *pBuf, int len)
{
    int ret;
    while (1)
    {
        ret = read(fd, pBuf, len);
        if ((ret == -1) && (errno == EINTR))
            continue;
        return ret;
    }
}

int nio_pread(int fd, void *pBuf, int len, off_t off)
{
    int ret;
    while (1)
    {
        ret = pread(fd, pBuf, len, off);
        if ((ret == -1) && (errno == EINTR))
            continue;
        return ret;
    }
}


int nio_write(int fd, const void *pBuf, int len)
{
    int ret;
    while (1)
    {
        ret = write(fd, pBuf, len);
        if ((ret == -1) && (errno == EINTR))
            continue;
        return ret;
    }
}

int nio_writev(int fd, const struct iovec *pIov, int count)
{
    int ret;
    while (1)
    {
        ret = writev(fd, pIov, count);
        if ((ret == -1) && (errno == EINTR))
            continue;
        return ret;
    }
}

int nio_pwrite(int fd, const void *pBuf, int len, off_t off)
{
    int ret;
    while (1)
    {
        ret = pwrite(fd, pBuf, len, off);
        if ((ret == -1) && (errno == EINTR))
            continue;
        return ret;
    }
}

off_t nio_lseek(int fd, off_t offset, int whence)
{
    off_t ret;
    while (1)
    {
        ret = lseek(fd, offset, whence);
        if ((ret == -1) && (errno == EINTR))
            continue;
        return ret;
    }
}

int nio_stat(const char *pathname, struct stat *st)
{
    int ret;
    while (1)
    {
        ret = stat(pathname, st);
        if ((ret == -1) && ((errno == EINTR) || (errno == EAGAIN)))
            continue;
        return ret;
    }
}


