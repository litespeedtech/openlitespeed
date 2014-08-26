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
#ifndef VMEMBUF_H
#define VMEMBUF_H



#include <util/autostr.h>
#include <util/gpointerlist.h>
#include <util/blockbuf.h>

#include <stddef.h>

#define VMBUF_MALLOC    0
#define VMBUF_ANON_MAP  1
#define VMBUF_FILE_MAP  2

typedef TPointerList<BlockBuf> BufList;
  
class VMemBuf
{
protected:
    static size_t   s_iBlockSize;
    static size_t   s_iMinMmapSize;
    static int      s_iKeepOpened;
    static int      s_fdSpare;
    static char     s_sTmpFileTemplate[256];
    static int      s_iMaxAnonMapBlocks;
    static int      s_iCurAnonMapBlocks;
    
private:    
    BufList     m_bufList;
    AutoStr2    m_sFileName;
    int         m_fd;
    size_t      m_curTotalSize;

    short       m_type;
    char        m_autoGrow;
    char        m_noRecycle;
    size_t      m_curWBlkPos;
    BlockBuf ** m_pCurWBlock;
    char      * m_pCurWPos;

    size_t      m_curRBlkPos;
    BlockBuf ** m_pCurRBlock;
    char      * m_pCurRPos;


    int mapNextWBlock();
    int mapNextRBlock();
    int appendBlock( BlockBuf * pBlock );
    int grow();
    void releaseBlocks();
    void reset();
    BlockBuf * getAnonMapBlock( int size );
    void recycle( BlockBuf * pBuf );

    int  remapBlock( BlockBuf * pBlock, int pos );

public:
    void deallocate();

    static size_t getBlockSize()    {   return s_iBlockSize;    }
	static int lowOnAnonMem()	
		{	return s_iMaxAnonMapBlocks - s_iCurAnonMapBlocks < s_iMaxAnonMapBlocks / 4; }
    static size_t getMinMmapSize()  {   return s_iMinMmapSize;  }
    static void setMaxAnonMapSize( int sz );
    static void setTempFileTemplate( const char * pTemp );
    static char * mapTmpBlock( int fd, BlockBuf &buf, size_t offset, int write = 0 );
    static void releaseBlock( BlockBuf * pBlock );

    explicit VMemBuf( int TargetSize = 0);
    ~VMemBuf();
    int set( int type, int size );
    int set( BlockBuf * pBlock );
    int set( const char * pFileName, int size );
    int setfd( const char * pFileName, int fd );
    char * getReadBuffer( size_t &size );
    char * getWriteBuffer( size_t &size )
    {
        if (( !m_pCurWBlock )||( m_pCurWPos >= (*m_pCurWBlock)->getBufEnd() ))
        {
            if ( mapNextWBlock( ) != 0 )
                return NULL;
        }
        size = (*m_pCurWBlock)->getBufEnd() - m_pCurWPos;
        return m_pCurWPos;
    }
    
    void readUsed( size_t len )     {   m_pCurRPos += len;      }
    void writeUsed( size_t len )    {   m_pCurWPos += len;      }
    char * getCurRPos() const       {   return m_pCurRPos;      }
    size_t getCurROffset() const    
    {   return (m_pCurRBlock)?( m_curRBlkPos - ((*m_pCurRBlock)->getBufEnd() - m_pCurRPos)):0;   }
    char * getCurWPos() const       {   return m_pCurWPos;      }
    size_t getCurWOffset() const    
    {   return m_pCurWBlock?(m_curWBlkPos - ((*m_pCurWBlock)->getBufEnd() - m_pCurWPos)):0;   }
    int write( const char * pBuf, int size );
    bool isMmaped() const {   return m_type >= VMBUF_ANON_MAP;  }
    //int  seekRPos( size_t pos );
    //int  seekWPos( size_t pos );
    void rewindWriteBuf();
    void rewindReadBuf();
    int setROffset( size_t offset );
    int getfd() const               {   return m_fd;            }
    size_t getCurFileSize() const   {   return m_curTotalSize;  }
    size_t getCurRBlkPos() const    {   return m_curRBlkPos;    }
    size_t getCurWBlkPos() const    {   return m_curWBlkPos;    }
    int empty() const
    {
        if ( m_curRBlkPos < m_curWBlkPos )
            return 0;
        if ( !m_pCurWBlock )
            return 1;
        return ( m_pCurRPos >= m_pCurWPos );
    }    
    long writeBufSize() const;
    int  reinit( int TargetSize = -1 );
    int  exactSize( long *pSize = NULL );
    int  shrinkBuf( long size );
    int  close();
    int  copyToFile( size_t startOff, size_t len, 
                            int fd, size_t destStartOff );
    const char * getTempFileName()  {    return m_sFileName.c_str();    }
        
    int convertInMemoryToFileBacked();

    int convertFileBackedToInMemory();
    static void initAnonPool();
    int eof( off_t offset );
    const char * acquireBlockBuf( off_t offset, int *size );
    void releaseBlockBuf( off_t offset );


};

class MMapVMemBuf : public VMemBuf
{
public:
    explicit MMapVMemBuf( int TargetSize = 0);
};


#endif
