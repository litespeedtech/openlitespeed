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
#include <util/vmembuf.h>
#include <util/blockbuf.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <util/ssnprintf.h>

#define _RELEASE_MMAP

size_t VMemBuf::s_iBlockSize = 8192;
size_t VMemBuf::s_iMinMmapSize = 8192;

int  VMemBuf::s_iMaxAnonMapBlocks = 1024 * 1024 * 10 / 8192;
int  VMemBuf::s_iCurAnonMapBlocks = 0;
int  VMemBuf::s_iKeepOpened = 0;
int  VMemBuf::s_fdSpare = -1; //open( "/dev/null", O_RDWR );
char VMemBuf::s_sTmpFileTemplate[256] = "/tmp/tmp-XXXXXX";
BufList * s_pAnonPool = NULL;

void VMemBuf::initAnonPool()
{
    s_pAnonPool = new BufList();
}

void VMemBuf::setMaxAnonMapSize( int sz )
{
    if ( sz >= 0 )
        s_iMaxAnonMapBlocks = sz / s_iBlockSize;
}

void VMemBuf::setTempFileTemplate( const char * pTemp )
{
    if ( pTemp != NULL )
    {
        strcpy( s_sTmpFileTemplate, pTemp );
        int len = strlen( pTemp );
        if (( len < 6 )||
            ( strcmp( pTemp + len - 6, "XXXXXX" ) != 0 ))
            strcat( s_sTmpFileTemplate, "XXXXXX" );
    }
}

VMemBuf::VMemBuf( int target_size )
    : m_bufList( 4 )
{
    reset();
}

VMemBuf::~VMemBuf()
{
    deallocate();
}

void VMemBuf::releaseBlocks()
{
    if ( m_type == VMBUF_ANON_MAP )
    {
        s_iCurAnonMapBlocks -= m_curTotalSize/s_iBlockSize;
        if ( m_noRecycle )
            m_bufList.release_objects();
        else
        {
            s_pAnonPool->push_back( m_bufList );
            m_bufList.clear();
        }
    }
    else
        m_bufList.release_objects();
    memset( &m_curWBlkPos, 0, (char *)(&m_pCurRPos + 1 ) - ( char * )&m_curWBlkPos );
    m_curTotalSize = 0;
}

void VMemBuf::deallocate()
{
    if ( VMBUF_FILE_MAP == m_type )
    {
        if ( m_fd != -1)
        {
            ::close( m_fd );
            m_fd = -1;
        }
        unlink( m_sFileName.c_str() );
    }
    releaseBlocks();    
}

void VMemBuf::recycle( BlockBuf * pBuf )
{
    if ( m_type == VMBUF_ANON_MAP )
    {
        if ( pBuf->getBlockSize() == s_iBlockSize )
        {
            if ( m_noRecycle )
                delete pBuf;
            else
                s_pAnonPool->push_back( pBuf );
            --s_iCurAnonMapBlocks;
            return;
        }
        s_iCurAnonMapBlocks -= pBuf->getBlockSize()/s_iBlockSize;
    }
    delete pBuf;
}


int VMemBuf::shrinkBuf( long size )
{
/*
    if ( m_type == VMBUF_FILE_MAP )
    {
        if ( s_iMaxAnonMapBlocks - s_iCurAnonMapBlocks > s_iMaxAnonMapBlocks / 10 )
        {
            deallocate();
            if ( set( VMBUF_ANON_MAP , getBlockSize() ) == -1 )
                return -1;
            return 0;
        }

    }
*/
    if ( size < 0 )
        size = 0;
    if (( m_type == VMBUF_FILE_MAP )&&( m_fd != -1 ))
    {
        if ( m_curTotalSize > (unsigned long) size )
            ftruncate( m_fd, size );
    }
    BlockBuf * pBuf;
    while( m_curTotalSize > (size_t)size )
    {
        pBuf = m_bufList.pop_back();
        m_curTotalSize -= pBuf->getBlockSize();
        recycle( pBuf );
    }
    if ( !m_bufList.empty())
    {
        m_pCurWBlock = m_pCurRBlock = m_bufList.begin();
    }
    return 0;
}

void VMemBuf::releaseBlock( BlockBuf * pBlock )
{
    if ( pBlock->getBuf() )
    {
        munmap( pBlock->getBuf(), pBlock->getBlockSize() );
        pBlock->setBlockBuf( NULL, pBlock->getBlockSize() );
    }
    
}

int VMemBuf::remapBlock( BlockBuf * pBlock, int pos )
{
    char * pBuf = ( char*) mmap( NULL, s_iBlockSize, PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_FILE, m_fd, pos );
    if ( !pBuf )
    {
        perror( "mmap() failed in remapBlock" );
        return -1;
    }
    pBlock->setBlockBuf( pBuf, s_iBlockSize );
    return 0;
}


int VMemBuf::reinit(int TargetSize)
{
    if ( m_type == VMBUF_ANON_MAP )
    {
        if (( TargetSize >=
            (long)((s_iMaxAnonMapBlocks - s_iCurAnonMapBlocks) * s_iBlockSize ) )||
            ( TargetSize > 1024 * 1024 ))
        {
            releaseBlocks();
            if ( set( VMBUF_FILE_MAP , getBlockSize() ) == -1 )
                return -1;
        }
    }
    else if ( m_type == VMBUF_FILE_MAP )
    {
        if (( TargetSize < 1024 * 1024 )&&
            ( s_iMaxAnonMapBlocks - s_iCurAnonMapBlocks > s_iMaxAnonMapBlocks / 5 )&&
            ( (unsigned int) TargetSize < (s_iMaxAnonMapBlocks - s_iCurAnonMapBlocks) * s_iBlockSize ))
        {
            deallocate();
            if ( set( VMBUF_ANON_MAP , getBlockSize() ) == -1 )
                return -1;
        }

#ifdef _RELEASE_MMAP
        if ( !m_bufList.empty() )
        {
            if ( !(*m_bufList.begin())->getBuf() )
            {
                if ( remapBlock( *m_bufList.begin(), 0 ) == -1 )
                    return -1;
            }
            if (( m_pCurWBlock )&&
                ( m_pCurWBlock != m_bufList.begin() )&&
                ( m_pCurWBlock != m_pCurRBlock ))
            {
                releaseBlock( *m_pCurWBlock );
            }
            if (( m_pCurRBlock )&&( m_pCurRBlock != m_bufList.begin() ))
            {
                releaseBlock( *m_pCurRBlock );
            }
        }
#endif

    }
    
    if ( !m_bufList.empty() )
    {
        
        m_pCurWBlock = m_pCurRBlock = m_bufList.begin();
        if ( *m_pCurWBlock )
        {
            m_curWBlkPos = m_curRBlkPos = (*m_pCurWBlock)->getBlockSize();
            m_pCurRPos = m_pCurWPos = (*m_pCurWBlock)->getBuf();
        }
        else
        {
            m_curWBlkPos = m_curRBlkPos = 0;
            m_pCurRPos = m_pCurWPos = NULL;
        }
    }
    return 0;
}

void VMemBuf::reset()
{
    m_fd = -1;
    memset( &m_curTotalSize, 0, (char *)(&m_pCurRPos + 1 ) - ( char * )&m_curTotalSize );
    if (m_sFileName.buf() )
    {
        *m_sFileName.buf() = 0;
        m_sFileName.setLen(0);
    }
}

BlockBuf * VMemBuf::getAnonMapBlock(int size)
{
    BlockBuf *pBlock;
    if ( ( unsigned int )size == s_iBlockSize )
    {
        if ( !s_pAnonPool->empty() )
        {
            pBlock = s_pAnonPool->pop_back();
            ++s_iCurAnonMapBlocks;
            return pBlock;
        }
    }
    int blocks = ( size + s_iBlockSize - 1 ) / s_iBlockSize;
    size = blocks * s_iBlockSize;
    char * pBuf =( char*) mmap( NULL, size, PROT_READ | PROT_WRITE,
        MAP_ANON | MAP_SHARED, -1, 0 );
    if ( pBuf == MAP_FAILED )
    {
        perror( "Anonymous mmap() failed" );
        return NULL;
    }
    pBlock = new MmapBlockBuf( pBuf, size );
    if ( !pBlock )
    {
        perror( "new MmapBlockBuf failed in getAnonMapBlock()" );
        munmap( pBuf, size );
        return NULL;
    }
    s_iCurAnonMapBlocks += blocks;
    return pBlock;
}


int VMemBuf::set( int type, int size )
{
    if (( type > VMBUF_FILE_MAP) || ( type < VMBUF_MALLOC ))
        return -1;

    char *pBuf;
    BlockBuf * pBlock = NULL;
    m_type = type;
    switch( type )
    {
    case VMBUF_MALLOC:
        if ( size > 0 )
        {
            size = (( size + 511 ) / 512) * 512;
            pBuf = ( char *)malloc( size );
            if ( !pBuf )
                return -1;
            pBlock = new MallocBlockBuf( pBuf, size );
            if ( !pBlock )
            {
                free( pBuf );
                return -1;
            }
        }
        break;
    case VMBUF_ANON_MAP:
        m_autoGrow = 1;
        if ( size > 0 )
        {
            if ( ( pBlock = getAnonMapBlock( size ) ) == NULL )
                return -1;
        }
        break;
    case VMBUF_FILE_MAP:
        m_autoGrow = 1;
        m_sFileName.setStr( s_sTmpFileTemplate );
        m_fd = mkstemp( m_sFileName.buf());
        if (m_fd == -1 )
        {
            char achBuf[1024];
            safe_snprintf( achBuf, 1024, "Failed to create swap file with mkstemp( %s ), please check 'Swap Directory' and permission", m_sFileName.c_str() );
            perror( achBuf );
            *m_sFileName.buf() = 0;
            m_sFileName.setLen(0);
            return -1;
        }
        fcntl( m_fd, F_SETFD, FD_CLOEXEC );
        break;
    }
    if ( pBlock )
    {
        appendBlock( pBlock );
        m_pCurRPos = m_pCurWPos = pBlock->getBuf();
        m_curTotalSize = m_curRBlkPos = m_curWBlkPos = pBlock->getBlockSize();
        m_pCurRBlock = m_pCurWBlock = m_bufList.begin();
    }
    return 0;
}

int VMemBuf::appendBlock( BlockBuf * pBlock )
{
    if ( m_bufList.full() )
    {
        BlockBuf ** pOld = m_bufList.begin();
        m_bufList.push_back( pBlock );
        if ( m_pCurWBlock )
            m_pCurWBlock = m_pCurWBlock - pOld + m_bufList.begin();
        if ( m_pCurRBlock )
            m_pCurRBlock = m_pCurRBlock - pOld + m_bufList.begin();
        return 0;    
    }
    else
    {
        m_bufList.safe_push_back( pBlock );
        return 0;
    }
    
}


int VMemBuf::convertFileBackedToInMemory()
{
    BlockBuf * pBlock;
    if ( m_type != VMBUF_FILE_MAP )
        return -1;
    if ( m_pCurWPos == (*m_pCurWBlock)->getBuf() )
    {
        if ( *m_pCurWBlock == m_bufList.back() )
        {
            m_bufList.pop_back();
            recycle( *m_pCurWBlock );
            if ( !m_bufList.empty() )
            {
                m_noRecycle = 1;
                s_iCurAnonMapBlocks += m_bufList.size();
                int i = 0;
                while( i < m_bufList.size())
                {
                    pBlock = m_bufList[i];
                    if ( !pBlock->getBuf() )
                    {
                        if ( remapBlock( pBlock, i * s_iBlockSize ) == -1 )
                            return -1;
                    }
                    ++i;
                }
            }
            unlink( m_sFileName.c_str() );
            ::close( m_fd );
            m_fd = -1;
            m_type = VMBUF_ANON_MAP;
            if ( !(pBlock = getAnonMapBlock( s_iBlockSize ) ))
                return -1;
            appendBlock( pBlock );
            if ( m_pCurRPos == m_pCurWPos )
                m_pCurRPos = (*m_pCurWBlock)->getBuf();
            m_pCurWPos = (*m_pCurWBlock)->getBuf();
            return 0;
        }
        
    }
    return -1;
}

int VMemBuf::set( BlockBuf * pBlock )
{
    assert( pBlock );
    m_type = VMBUF_MALLOC;
    appendBlock( pBlock );
    m_pCurRBlock = m_pCurWBlock = m_bufList.begin();
    m_pCurRPos = m_pCurWPos = pBlock->getBuf();
    m_curTotalSize = m_curRBlkPos = m_curWBlkPos = pBlock->getBlockSize();
    return 0;
}

int VMemBuf::set(const char * pFileName, int size)
{
    if ( size < 0 )
        size = s_iMinMmapSize;
    m_type = VMBUF_FILE_MAP;
    m_autoGrow = 1;
    if ( pFileName )
    {
        m_sFileName = pFileName;
        m_fd = ::open( pFileName, O_RDWR | O_CREAT | O_TRUNC, 0600 );
        if ( m_fd < 0 )
        {
            perror( "Failed to open temp file for swapping" );
            return -1;
        }
        
        fcntl( m_fd, F_SETFD, FD_CLOEXEC );

    }
    return 0;
}

int VMemBuf::setfd( const char * pFileName, int fd )
{
    if ( pFileName )
        m_sFileName = pFileName;
    m_fd = fd;
    fcntl( m_fd, F_SETFD, FD_CLOEXEC );
    m_type = VMBUF_FILE_MAP;
    m_autoGrow = 1;
    return 0;
}


void VMemBuf::rewindWriteBuf()
{
    if ( m_pCurRBlock )
    {
        if( m_pCurWBlock != m_pCurRBlock )
        {
            m_pCurWBlock = m_pCurRBlock;
            m_curWBlkPos = m_curRBlkPos;
        }
        m_pCurWPos = m_pCurRPos;
    }
    else
    {
        m_pCurRBlock = m_pCurWBlock;
        m_curRBlkPos = m_curWBlkPos;
        m_pCurRPos = m_pCurWPos;
    }

}

void VMemBuf::rewindReadBuf()
{
#ifdef _RELEASE_MMAP
    if ( m_type == VMBUF_FILE_MAP)
    {
        if ( !(*m_bufList.begin())->getBuf() )
        {
            if ( remapBlock( *m_bufList.begin(), 0 ) == -1 )
                return;
        }
        if (( m_pCurRBlock )&&
            ( m_pCurRBlock != m_bufList.begin() )&& 
            ( m_pCurRBlock != m_pCurWBlock))
        {
            releaseBlock( *m_pCurRBlock );
        }
    }
#endif
    m_pCurRBlock = m_bufList.begin();
    m_curRBlkPos = (*m_pCurRBlock)->getBlockSize();
    m_pCurRPos = (*m_pCurRBlock)->getBuf();
}

int VMemBuf::setROffset( size_t offset )
{
    if ( offset > m_curTotalSize )
        return -1;
    rewindReadBuf();
    while( offset >= (*m_pCurRBlock)->getBlockSize() )
    {
        offset -= (*m_pCurRBlock)->getBlockSize();
        mapNextRBlock();
    }
    m_pCurRPos += offset;
    return 0;
}

int VMemBuf::mapNextWBlock()
{
    if ( !m_pCurWBlock || m_pCurWBlock+1 >= m_bufList.end() )
    {
        if ( !m_autoGrow || grow() )
            return -1;
    }

    if ( m_pCurWBlock )
    {
#ifdef _RELEASE_MMAP
        if ( m_type == VMBUF_FILE_MAP)
        {
            if ( !(*(m_pCurWBlock+1))->getBuf() )
            {
                if ( remapBlock( *(m_pCurWBlock+1), m_curWBlkPos ) == -1 )
                    return -1;
            }
            if ( m_pCurWBlock != m_pCurRBlock )
                releaseBlock( *m_pCurWBlock );
        }
#endif
        ++m_pCurWBlock;
    }
    else
    {
#ifdef _RELEASE_MMAP
        if (( m_type == VMBUF_FILE_MAP)&&( !(*m_bufList.begin())->getBuf() ))
        {
            if ( remapBlock( *m_bufList.begin(), 0 ) == -1 )
                return -1;
        }
#endif
        m_pCurWBlock = m_pCurRBlock = m_bufList.begin();
        m_curWBlkPos = 0;
        m_curRBlkPos = (*m_pCurWBlock)->getBlockSize();
        m_pCurRPos = (*m_pCurWBlock)->getBuf();
    }
    m_curWBlkPos += (*m_pCurWBlock)->getBlockSize();
    m_pCurWPos = (*m_pCurWBlock)->getBuf();
    return 0;
}

int VMemBuf::grow()
{
    char * pBuf;
    BlockBuf * pBlock;
    size_t oldPos = m_curTotalSize;
    switch( m_type )
    {
    case VMBUF_MALLOC:
        pBuf = ( char *)malloc( s_iBlockSize );
        if ( !pBuf )
            return -1;
        pBlock = new MallocBlockBuf( pBuf, s_iBlockSize );
        if ( !pBlock )
        {
            perror( "new MallocBlockBuf failed in grow()" );
            free( pBuf );
            return -1;
        }
        break;    
    case VMBUF_FILE_MAP:
        if ( ftruncate( m_fd, m_curTotalSize + s_iBlockSize ) == -1 )
        {
            perror( "Failed to increase temp file size with ftrancate()" );
            return -1;
        }
        pBuf = ( char*) mmap( NULL, s_iBlockSize, PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_FILE, m_fd, oldPos );
        if( pBuf == MAP_FAILED )
        {
            perror( "FS backed mmap() failed" );
            return -1;
        }
        pBlock = new MmapBlockBuf( pBuf, s_iBlockSize );
        if ( !pBlock )
        {
            perror( "new MmapBlockBuf failed in grow()" );
            munmap( pBuf, s_iBlockSize );
            return -1;
        }
        break;
    case VMBUF_ANON_MAP:
        if ( (pBlock = getAnonMapBlock( s_iBlockSize ) ))
            break;
	default:
		return -1;
    }
    appendBlock( pBlock );
    m_curTotalSize += s_iBlockSize;
    return 0;

}



char * VMemBuf::getReadBuffer( size_t &size )
{
    if (( !m_pCurRBlock )||( m_pCurRPos >= (*m_pCurRBlock)->getBufEnd() ))
    {
        size = 0;
        if ( mapNextRBlock() != 0 )
            return NULL;
    }
    if ( m_curRBlkPos == m_curWBlkPos )
    {
        size = m_pCurWPos - m_pCurRPos;
    }
    else
        size = (*m_pCurRBlock)->getBufEnd() - m_pCurRPos;
    return m_pCurRPos;
}

int VMemBuf::mapNextRBlock()
{
    if ( m_curRBlkPos >= m_curWBlkPos )
    {
        return -1;
    }
    if ( m_pCurRBlock )
    {
#ifdef _RELEASE_MMAP
        if ( m_type == VMBUF_FILE_MAP)
        {
            if ( !(*(m_pCurRBlock+1))->getBuf() )
            {
                if ( remapBlock( *(m_pCurRBlock+1), m_curRBlkPos ) == -1 )
                    return -1;
            }
            if ( m_pCurWBlock != m_pCurRBlock )
                releaseBlock( *m_pCurRBlock );
        }
#endif
        ++m_pCurRBlock;
    }
    else
        m_pCurRBlock = m_bufList.begin();
    m_curRBlkPos += (*m_pCurRBlock)->getBlockSize();
    m_pCurRPos = (*m_pCurRBlock)->getBuf();
    return 0;

}



long VMemBuf::writeBufSize() const
{
    if ( m_pCurWBlock == m_pCurRBlock )
        return m_pCurWPos - m_pCurRPos;
    int diff = m_curWBlkPos - m_curRBlkPos;
    if ( m_pCurWBlock )
        diff += m_pCurWPos - (*m_pCurWBlock)->getBuf();
    if ( m_pCurRBlock )
        diff -= m_pCurRPos - (*m_pCurRBlock)->getBuf();
    return diff;
}

int VMemBuf::write( const char * pBuf, int size )
{
    const char * pCur = pBuf;
    if ( m_pCurWPos )
    {
        int len = (*m_pCurWBlock)->getBufEnd() - m_pCurWPos;
        if ( size <= len )
        {
            memmove( m_pCurWPos, pBuf, size );
            m_pCurWPos += size;
            return size;
        }
        else
        {
            memmove( m_pCurWPos, pBuf, len );
            pCur += len;
            size -= len;
        }
    }
    do
    {
        if ( mapNextWBlock() != 0 )
        {
            return pCur - pBuf;
        }
        int len  = (*m_pCurWBlock)->getBufEnd() - m_pCurWPos;
        if ( size <= len )
        {
            memmove( m_pCurWPos, pCur, size );
            m_pCurWPos += size;
            return pCur - pBuf + size;
        }
        else
        {
            memmove( m_pCurWPos, pCur, len );
            pCur += len;
            size -= len;
        }
    }while( true );
}

int VMemBuf::exactSize( long *pSize )
{
    long size = m_curTotalSize;
    if ( m_pCurWBlock )
        size -= (*m_pCurWBlock)->getBufEnd() - m_pCurWPos;
    if ( pSize )
        *pSize = size;
    if ( m_fd != -1)
    {
        return  ftruncate( m_fd, size );
    }
    return 0;
}

int VMemBuf::close()
{
    if ( m_fd != -1)
    {
        ::close( m_fd );
    }
    releaseBlocks();
    reset();
    return 0;
}

MMapVMemBuf::MMapVMemBuf( int TargetSize )
{
    int type = VMBUF_ANON_MAP;
    
    if (( lowOnAnonMem() )||( TargetSize > 1024 * 1024 )||
        ( TargetSize >=
            (long)((s_iMaxAnonMapBlocks - s_iCurAnonMapBlocks) * s_iBlockSize ) ))
        type = VMBUF_FILE_MAP;
    if ( set( type , getBlockSize() ) == -1 )
        abort(); //throw -1;
}

// int VMemBuf::setFd( int fd )
// {
//     struct stat st;
//     if ( fstat( fd, &st ) == -1 )
//         return -1;
//     m_fd = fd;
//     m_type = VMBUF_FILE_MAP;
//     m_curTotalSize = st.st_size;
//     
//     return 0;
// }

char * VMemBuf::mapTmpBlock( int fd, BlockBuf &buf, size_t offset, int write )
{
    size_t blkBegin = offset - offset % s_iBlockSize;
    char * pBuf = ( char*) mmap( NULL, s_iBlockSize, PROT_READ | write,
                MAP_SHARED | MAP_FILE, fd, blkBegin );
    if( pBuf == MAP_FAILED )
    {
        perror( "FS backed mmap() failed" );
        return NULL;
    }
    buf.setBlockBuf( pBuf, s_iBlockSize );
    return pBuf + offset % s_iBlockSize;
}


int VMemBuf::eof( off_t offset )
{
    int total = getCurWOffset(); 
    if ( offset >= total )
        return 1;
    else
        return 0;
}

const char * VMemBuf::acquireBlockBuf( off_t offset, int *size )
{
    BlockBuf * pSrcBlock = NULL;
    char * pSrcPos;
    int total = getCurWOffset(); 
    int blk = offset / s_iBlockSize;
    *size = 0;
    if ( blk >= m_bufList.size() )
        return NULL;
    if ( offset >= total )
        return "";
 
    int len = total - offset;
    pSrcBlock = m_bufList[blk];
    if ( !pSrcBlock )
        return NULL;

    if ( !pSrcBlock->getBuf() )
    {
        if ( remapBlock( pSrcBlock, blk * s_iBlockSize ) == -1 )
            return NULL;
    }
    pSrcPos = pSrcBlock->getBuf() + offset % s_iBlockSize;
    if ( len > pSrcBlock->getBufEnd() - pSrcPos )
        *size = pSrcBlock->getBufEnd() - pSrcPos;
    else
        *size = len;
    return pSrcPos;
}

void VMemBuf::releaseBlockBuf( off_t offset )
{
    BlockBuf * pSrcBlock = NULL;
    int blk = offset / s_iBlockSize;
    if (( m_type != VMBUF_FILE_MAP )||
        ( blk >= m_bufList.size() ))
        return ;
    pSrcBlock = m_bufList[blk];
    if ( pSrcBlock->getBuf()&&( pSrcBlock != *m_pCurRBlock )
         &&( pSrcBlock != *m_pCurWBlock ))
        releaseBlock( pSrcBlock );
}

int VMemBuf::copyToFile( size_t startOff, size_t len, 
                        int fd, size_t destStartOff )
{
    BlockBuf * pSrcBlock = NULL;
    char * pSrcPos;
    int mapped = 0;
    int blk = startOff / s_iBlockSize;
    if ( blk >= m_bufList.size() )
        return -1;
    struct stat st;
    if ( fstat( fd, &st ) == -1 )
        return -1;
    int destSize = destStartOff + len;
    //destSize -= destSize % s_iBlockSize;
    if ( st.st_size < destSize )
    {
        if ( ftruncate( fd, destSize ) == -1 )
            return -1;
    }
    BlockBuf destBlock;
    char * pPos = mapTmpBlock( fd, destBlock, destStartOff, PROT_WRITE );
    int ret = 0;
    if ( !pPos )
    {
        return -1;
    }
    while( !ret && (len > 0) )
    {
        if ( blk >= m_bufList.size() )
            break;
        pSrcBlock = m_bufList[blk];
        if ( !pSrcBlock )
        {
            ret = -1;
            break;
        }
        if ( !pSrcBlock->getBuf() )
        {
            if ( remapBlock( pSrcBlock, blk * s_iBlockSize ) == -1 )
                return -1;
            mapped = 1;
        }
        pSrcPos = pSrcBlock->getBuf() + startOff % s_iBlockSize;
        
        while( (len > 0 )&&( pSrcPos < pSrcBlock->getBufEnd() ) )
        {
            if ( pPos >= destBlock.getBufEnd() )
            {
                msync( destBlock.getBuf(), destBlock.getBlockSize(), MS_SYNC );
                releaseBlock( &destBlock );
                pPos = mapTmpBlock( fd, destBlock, destStartOff, PROT_WRITE );
                if ( !pPos )
                {
                    ret = -1;
                    break;
                }
            }
            int sz = len;
            if ( sz > pSrcBlock->getBufEnd() - pSrcPos )
                sz = pSrcBlock->getBufEnd() - pSrcPos;
            if ( sz > destBlock.getBufEnd() - pPos )
                sz = destBlock.getBufEnd() - pPos;
            memmove( pPos, pSrcPos, sz );
            pSrcPos += sz;
            pPos += sz;
            destStartOff += sz;
            len -= sz;
        }
        startOff = 0;
        if ( mapped )
        {
            releaseBlock( pSrcBlock );
            mapped = 0;
        }
        ++blk;
    }
    releaseBlock( &destBlock );
    return ret;
    
}


