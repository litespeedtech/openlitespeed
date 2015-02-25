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
#include "ceheader.h"
#include "dirhashcacheentry.h"
#include <lsr/ls_fileio.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

DirHashCacheEntry::DirHashCacheEntry()
    : CacheEntry()
    , m_iLastCheck(-1)
{
}


DirHashCacheEntry::~DirHashCacheEntry()
{
    if (getFdStore() != -1)
        close(getFdStore());

}
//<"LSCH"><CeHeader><CacheKey><ResponseHeader><ResponseBody>
int DirHashCacheEntry::loadCeHeader()
{
    int fd = getFdStore();
    if (fd == -1)
    {
        errno = EBADF;
        return LS_FAIL;
    }
    if (ls_fio_lseek(fd, getStartOffset(), SEEK_SET) == -1)
        return LS_FAIL;
    char achBuf[4 + sizeof(CeHeader) ];
    if ((size_t)ls_fio_read(fd, achBuf, 4 + sizeof(CeHeader))
        < 4 + sizeof(CeHeader))
        return LS_FAIL;
//  if ( *( uint32_t *)achBuf != CE_ID )
//     return LS_FAIL;
    if (memcmp(achBuf, CE_ID, 4) != 0)
        return LS_FAIL;

    memmove(&getHeader(), &achBuf[4], sizeof(CeHeader));
    int len = getHeader().m_iKeyLen;
    if (len > 0)
    {
        char *p = getKey().prealloc(len + 1);
        if (!p)
            return LS_FAIL;
        if (ls_fio_read(fd, p, len) < len)
            return LS_FAIL;
        *(p + len) = 0;
    }

    char tmpBUf[4096];
#ifdef CACHE_RESP_HEADER
    if (getHeader().m_valPart1Len < 4096)  //< 4K
    {
        if (ls_fio_read(fd, tmpBUf,
                        getHeader().m_valPart1Len) < getHeader().m_valPart1Len)
            return LS_FAIL;

        m_sRespHeader.append(tmpBUf, getHeader().m_valPart1Len);
    }
#endif

    //load part3 to buffer
    int part3offset = getHeaderSize() + getContentTotalLen();
    if (ls_fio_lseek(fd, part3offset, SEEK_SET) != -1)
    {
        while ((len = ls_fio_read(fd, tmpBUf, 4096)) > 0)
            m_sPart3Buf.append(tmpBUf, len);
    }

    return 0;

}

int DirHashCacheEntry::saveCeHeader()
{
    int fd = getFdStore();
    if (fd == -1)
    {
        errno = EBADF;
        return LS_FAIL;
    }
    if (ls_fio_lseek(fd, getStartOffset(), SEEK_SET) == -1)
        return LS_FAIL;
    char achBuf[4 + sizeof(CeHeader) ];
    //*( int *)achBuf = CE_ID;
    memcpy(achBuf, CE_ID, 4);
    memmove(&achBuf[4], &getHeader(), sizeof(CeHeader));
    if ((size_t)ls_fio_write(fd, achBuf, 4 + sizeof(CeHeader)) <
        4 + sizeof(CeHeader))
        return LS_FAIL;
    if (getHeader().m_iKeyLen > 0)
    {
        if (ls_fio_write(fd, getKey().c_str(), getHeader().m_iKeyLen) <
            getHeader().m_iKeyLen)
            return LS_FAIL;
    }
    return 0;
}

int DirHashCacheEntry::allocate(int size)
{
    int fd = getFdStore();
    if (fd == -1)
    {
        errno = EBADF;
        return LS_FAIL;
    }
    struct stat st;
    if (fstat(fd, &st) == -1)
        return LS_FAIL;
    if (st.st_size < size)
    {
        if (ftruncate(fd, size) == -1)
            return LS_FAIL;
    }
    return 0;
}

int DirHashCacheEntry::releaseTmpResource()
{
    int fd = getFdStore();
    if (fd != -1)
    {
        close(fd);
        setFdStore(-1);
    }
    return 0;
}



