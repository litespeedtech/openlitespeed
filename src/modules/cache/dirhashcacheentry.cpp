/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2014  LiteSpeed Technologies, Inc.                        *
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
#include <util/ni_fio.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

DirHashCacheEntry::DirHashCacheEntry()
    : CacheEntry()
    , m_lastCheck( -1 )
{
}


DirHashCacheEntry::~DirHashCacheEntry()
{
    if ( getFdStore() != -1 )
        close( getFdStore() );
    
}
//<"LSCH"><CeHeader><CacheKey><ResponseHeader><ResponseBody>
int DirHashCacheEntry::loadCeHeader()
{
    int fd = getFdStore();
    if ( fd == -1 )
    {
        errno = EBADF;
        return -1;
    }
    if ( nio_lseek( fd, getStartOffset(), SEEK_SET ) == -1 )
        return -1;
    char achBuf[4 + sizeof( CeHeader ) ];
    if ( (size_t)nio_read( fd, achBuf, 4 + sizeof( CeHeader ) ) 
                < 4 + sizeof( CeHeader ) )
        return -1;
//  if ( *( uint32_t *)achBuf != CE_ID )
//     return -1;
    if (memcmp(achBuf, CE_ID, 4) != 0)
        return -1;
    
    memmove( &getHeader(), &achBuf[4], sizeof( CeHeader ) );
    int len = getHeader().m_keyLen;
    if ( len > 0 )
    {
        char * p = getKey().resizeBuf( len+1 );
        if ( !p )   
            return -1;
        if ( nio_read( fd, p, len ) < len )
            return -1;
        *(p+len) = 0;
    }
    
    char tmpBUf[4096];
#ifdef CACHE_RESP_HEADER
    if (getHeader().m_valPart1Len < 4096 ) //< 4K
    {
        if ( nio_read( fd, tmpBUf, getHeader().m_valPart1Len ) < getHeader().m_valPart1Len )
            return -1;
        
        m_sRespHeader.append(tmpBUf, getHeader().m_valPart1Len);
    }
#endif

    //load part3 to buffer
    int part3offset = getHeaderSize() + getContentTotalLen();
    if ( nio_lseek( fd, part3offset, SEEK_SET ) != -1 )
    {
        while((len = nio_read( fd, tmpBUf, 4096 )) > 0)
            m_sPart3Buf.append(tmpBUf, len);
    }
    
    return 0;

}

int DirHashCacheEntry::saveCeHeader()
{
    int fd = getFdStore();
    if ( fd == -1 )
    {
        errno = EBADF;
        return -1;
    }
    if ( nio_lseek( fd, getStartOffset(), SEEK_SET ) == -1 )
        return -1;
    char achBuf[4 + sizeof( CeHeader ) ];
    //*( int *)achBuf = CE_ID;
    memcpy(achBuf, CE_ID, 4);
    memmove( &achBuf[4], &getHeader(), sizeof( CeHeader ) );
    if ( (size_t)nio_write( fd, achBuf, 4 + sizeof( CeHeader ) ) < 
                4 + sizeof( CeHeader ) )
        return -1;
    if ( getHeader().m_keyLen > 0 )
    {
        if ( nio_write( fd, getKey().c_str(), getHeader().m_keyLen ) < 
                getHeader().m_keyLen )
            return -1;
    }
    return 0;
}

int DirHashCacheEntry::allocate( int size )
{
    int fd = getFdStore();
    if ( fd == -1 )
    {
        errno = EBADF;
        return -1;
    }
    struct stat st;
    if ( fstat( fd, &st ) == -1 )
        return -1;
    if ( st.st_size < size )
    {
        if ( ftruncate( fd, size ) == -1 )
            return -1;
    }
    return 0;
}

int DirHashCacheEntry::releaseTmpResource()
{
    int fd = getFdStore();
    if ( fd != -1 )
    {
        close( fd );
        setFdStore( -1 );
    }
    return 0;
}



