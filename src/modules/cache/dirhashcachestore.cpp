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
#include "dirhashcachestore.h"
#include "dirhashcacheentry.h"
#include "cachehash.h"

#include <util/datetime.h>
#include <util/stringtool.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DHCS_SOURCE_MATCH   1
#define DHCS_DEST_CHECK     2

DirHashCacheStore::DirHashCacheStore()
 : CacheStore()
{
}


DirHashCacheStore::~DirHashCacheStore()
{
    release_objects();
}

int DirHashCacheStore::clearStrage()
{
    //rename root directory
    //fork and delete root directory
    return 0;
}

int DirHashCacheStore::updateEntryState( DirHashCacheEntry * pEntry )
{
    struct stat st;
    if ( fstat( pEntry->getFdStore(), &st ) == -1 )
    {
        return -1;
    }
    pEntry->m_lastCheck = DateTime_s_curTime;
    pEntry->setLastAccess( DateTime_s_curTime );
    pEntry->m_lastMod   = st.st_mtime;
    pEntry->m_inode     = st.st_ino;
    pEntry->m_lSize     = st.st_size;
    return 0;
}

int DirHashCacheStore::isEntryExist( CacheHash& hash, const char * pSuffix,
        struct stat *pStat )
{
    char achBuf[4096];
    struct stat st;
    int n = buildCacheLocation( achBuf, 4096, hash );
    if ( pSuffix )
        strcpy( &achBuf[n], pSuffix );
    if ( !pStat )
        pStat = &st;
    if ( stat( achBuf, pStat ) == 0 )
    {
        return 1;
    }
    return 0;
}

int DirHashCacheStore::isEntryUpdating( CacheHash& hash )
{
    struct stat st;
    if (( isEntryExist( hash, ".tmp", &st ) == 1 )&&
        ( DateTime_s_curTime - st.st_mtime <= 300 ))
        return 1;
    return 0;
}

int DirHashCacheStore::isEntryStale( CacheHash& hash )
{
    struct stat st;
    if ( isEntryExist( hash, ".S", &st ) == 1 )
        return 1;
    return 0;
}


CacheEntry * DirHashCacheStore::getCacheEntry( CacheHash& hash,
                const char * pURI, int iURILen, 
                const char * pQS, int iQSLen,
                const char * pIP, int ipLen,
                const char * pCookie, int cookieLen,
                int32_t lastCacheFlush, int maxStale  )
{
    char achBuf[4096] = "";
    int fd;
    //FIXME: look up cache entry in memory, then on disk
    CacheStore::iterator iter = find( hash.getKey() );
    CacheEntry * pEntry = NULL;
    int dispose = 0;
    int stale = 0;
    int pathLen = 0;
    
    if ( iter != end() )
    {
        pEntry = iter.second();
        int lastCheck = ((DirHashCacheEntry *)pEntry)->m_lastCheck;
        
        if (( DateTime_s_curTime != lastCheck )
            ||( lastCheck == -1 ))  //This entry is being written to disk
        {
            pathLen = buildCacheLocation( achBuf, 4096, hash );
            if ( isChanged( (DirHashCacheEntry *)pEntry, achBuf, pathLen ) )
            {
                //updated by another process, do not remove current object on disk
                erase( iter );
                addToDirtyList( pEntry );
                pEntry = NULL;
                iter = end();
            }
        }
    }
    if ( ( pEntry == NULL) || (pEntry->getFdStore() == -1) )
    {
        if ( !pathLen )
            pathLen = buildCacheLocation( achBuf, 4096, hash );

        fd = ::open( achBuf, O_RDONLY, 0600 );
        if ( fd == -1 )
        {
            strcpy( &achBuf[pathLen], ".S" );
            fd = ::open( achBuf, O_RDONLY, 0600 );
            achBuf[pathLen] = 0;
            if ( fd == -1 )
            {
                if ( errno != ENOENT )
                {
                    strcpy( &achBuf[pathLen], ": open() failed" );
                    perror( achBuf );
                }
                if ( pEntry )
                    CacheStore::dispose( iter, 1 );
                return NULL;
            }
            stale = 1;
        }
        ::fcntl( fd, F_SETFD, FD_CLOEXEC );
        if ( pEntry )
        {
            pEntry->setFdStore( fd );
            pEntry->setFilePath(achBuf);
        }
        //LOG_INFO(( "getCacheEntry(), open fd: %d, entry: %p", fd, pEntry ));
    }
    if ( !pEntry )
    {
        pEntry = new DirHashCacheEntry();
        pEntry->setFdStore( fd );    //Should check fd == -1 case???
        pEntry->setFilePath(achBuf);   //Same as above
        pEntry->setHashKey( hash );
        //pEntry->setKey( hash, pURI, iURILen, pQS, iQSLen, pIP, ipLen, pCookie, cookieLen );
        pEntry->loadCeHeader();
        //assert( pEntry->verifyHashKey() == 0 );
        updateEntryState( (DirHashCacheEntry * )pEntry );
        if ( stale )
            pEntry->setStale( 1 );
        pEntry->setMaxStale( maxStale );
    }
    if ( pEntry->isStale() || DateTime_s_curTime > pEntry->getExpireTime() )
    {
        if ( DateTime_s_curTime - pEntry->getExpireTime() > pEntry->getMaxStale() )
            dispose = 1;
        else
        {
            if ( !pEntry->isStale() )
            {
                pEntry->setStale( 1 );
                if ( !pathLen )
                    pathLen = buildCacheLocation( achBuf, 4096, hash );
                if ( renameDiskEntry( pEntry, achBuf, NULL, ".S",
                         DHCS_SOURCE_MATCH|DHCS_DEST_CHECK ) != 0 )
                {
                    dispose = 1;
                }
            }
            if ( !pEntry->isUpdating() )
            {
                if ( isEntryUpdating( hash ) )
                    pEntry->setUpdating( 1 );
            }
        }
    }
    if ( pEntry->getHeader().m_tmCreated <= lastCacheFlush )
        dispose = 1;
    if ( dispose )
    {
        if ( iter != end() )
        {
            CacheStore::dispose( iter, 1 );
        }
        else
        {
            delete pEntry;
            if ( !achBuf[0] )
                buildCacheLocation( achBuf, 4096, hash );
            unlink( achBuf );
        }
        return NULL;
    }

    if ( pEntry->verifyKey( pURI, iURILen, pQS, iQSLen, pIP, ipLen, pCookie, cookieLen ) != 0 )
    {
        if ( iter == end() )
            delete pEntry;
        return NULL;
    }
    if ( iter == end() )
        insert( pEntry->getHashKey().getKey(), pEntry );
    return pEntry;
    
}

int DirHashCacheStore::buildCacheLocation( char *pBuf, int len, const CacheHash& hash )
{
    const char * achHash = hash.getKey();
    int n = snprintf( pBuf, len, "%s/%x/%x/%x/", getRoot().c_str(), ((unsigned char)achHash[0])>>4, achHash[0]&0xf, ((unsigned char)achHash[1])>>4 );
    StringTool::hexEncode( achHash, HASH_KEY_LEN, &pBuf[n] );
    n += 2 * HASH_KEY_LEN;
    return n;
}


CacheEntry * DirHashCacheStore::createCacheEntry( const CacheHash& hash,
                const char * pURI, int iURILen, 
                const char * pQS, int iQSLen, 
                const char * pIP, int ipLen,
                const char * pCookie, int cookieLen,
                int force, int* errorcode )
{
    char achBuf[4096];
    int n = buildCacheLocation( achBuf, 4096, hash );
    struct stat st;

//    if ( stat( achBuf, &st ) == 0 )
//    {
//        if ( !force )
//            return NULL;
//        //FIXME: rename the cache entry to make it dirty
//    }
//    else 
    {
        strcpy( &achBuf[n], ".tmp" );
        if ( stat( achBuf, &st ) == 0 )
        {
            //in progress
            if ( DateTime_s_curTime - st.st_mtime > 120 )
                unlink( achBuf );
            else
            {
                *errorcode = -1;
                return NULL;
            }
        }
    }
    achBuf[n - 2 * HASH_KEY_LEN - 1] = 0;
    if (( stat( achBuf, &st ) == -1 )&&( errno == ENOENT ))
    {
        achBuf[ n - 2 * HASH_KEY_LEN - 3 ] = 0;
        if (( stat( achBuf, &st ) == -1 )&&( errno == ENOENT ))
        {
            achBuf[ n - 2 * HASH_KEY_LEN - 5 ] = 0;
            if (( stat( achBuf, &st ) == -1 )&&( errno == ENOENT ))
            {
                if (( mkdir( achBuf, 0700 ) == -1 )&&( errno != EEXIST ))
                {
                    *errorcode = -2;
                    return NULL;
                }
            }
            achBuf[ n - 2 * HASH_KEY_LEN - 5 ] = '/';
            if ( mkdir( achBuf, 0700 ) == -1 )
            {
                *errorcode = -3;
                return NULL;
            }
        }
        achBuf[ n - 2 * HASH_KEY_LEN - 3 ] = '/';
        if ( mkdir( achBuf, 0700 ) == -1 )
        {
            *errorcode = -4;
            return NULL;
        }
    }
    achBuf[n - 2 * HASH_KEY_LEN - 1 ] = '/';

    int fd = ::open( achBuf, O_RDWR | O_CREAT | O_TRUNC | O_EXCL, 0600 );
    if ( fd == -1 )
    {
        *errorcode = -5;
        return NULL;
    }
    ::fcntl( fd, F_SETFD, FD_CLOEXEC );
    //LOG_INFO(( "createCacheEntry(), open fd: %d", fd ));

    CacheEntry * pEntry = new DirHashCacheEntry();
    pEntry->setFdStore( fd );
    pEntry->setFilePath(achBuf);
    pEntry->setKey( hash, pURI, iURILen, pQS, iQSLen, pIP, ipLen,
                pCookie, cookieLen );
    if ( pIP )
        pEntry->getHeader().m_flag |= CeHeader::CEH_PRIVATE;
    pEntry->saveCeHeader();

    //update current entry 
    CacheStore::iterator iter = find( hash.getKey() );
    if ( iter != end() )
        iter.second()->setUpdating( 1 );
    *errorcode = 0;
    return pEntry;
}

// remove:  0  do not remove temp file
//          1  remove temp file without checking
//          -1 check temp file inode then remove
void DirHashCacheStore::cancelEntry( CacheEntry * pEntry, int remove )
{
    char achBuf[4096];
    CacheStore::iterator iter = find( pEntry->getHashKey().getKey() );
    if ( iter != end() )
        iter.second()->setUpdating( 0 );
    if ( remove )
    {
        int n = buildCacheLocation( achBuf, 4096, pEntry->getHashKey() );
        strcpy( &achBuf[n], ".tmp" );
        if ( (pEntry->getFdStore() != -1) && remove == -1 )
        {
            struct stat stFd;
            struct stat stDir;
            fstat( pEntry->getFdStore(), &stFd );
            if (( stat( achBuf, &stDir ) != 0 )||
                ( stFd.st_ino != stDir.st_ino ))  //tmp has been modified by someone else
                remove = 0;
        }
        if ( remove )
            unlink( achBuf );
    }
    close( pEntry->getFdStore() );
    pEntry->setFdStore(-1);
}


CacheEntry * DirHashCacheStore::getCacheEntry( const char * pKey, 
                int keyLen )
{
    return NULL;
}

CacheEntry * DirHashCacheStore::getWriteEntry( const char * pKey, 
                    int keyLen, const char * pHash )
{
    return NULL;
}

int DirHashCacheStore::saveEntry( CacheEntry * pEntry )
{
    
    return 0;
}

void DirHashCacheStore::removePermEntry( CacheEntry * pEntry )
{
    char achBuf[4096];
    buildCacheLocation( achBuf, 4096, pEntry->getHashKey() );
    unlink( achBuf );
}
/*
void DirHashCacheStore::renameDiskEntry( CacheEntry * pEntry )
{
    char achBuf[4096];
    char achBufNew[4096];
    buildCacheLocation( achBuf, 4096, pEntry->getHashKey() );
    strcpy( achBufNew, achBuf );
    strcat( achBufNew, ".d" );
    rename( achBuf, achBufNew );
    //unlink( achBuf );
}
*/

/*
int DirHashCacheStore::dirty( const char * pKey, int keyLen )
{
    
}
*/

int DirHashCacheStore::isChanged( CacheEntry * pEntry, const char * pPath, int len )
{
    DirHashCacheEntry * pE = (DirHashCacheEntry *) pEntry;
    pE->m_lastCheck = DateTime_s_curTime;
    
    struct stat st;
    int ret = stat( (char *)pPath, &st );
    if ( ret == -1 )
    {
        strcpy( (char *)pPath+len, ".S" );
        ret = stat( (char *)pPath, &st );
        *( (char *)pPath + len ) = 0;
        if ( ret == -1 )
            return 1;
        pEntry->setStale( 1 );
        strcpy( (char *)pPath+len, ".tmp" );
        ret = stat( (char *)pPath, &st );
        *( (char *)pPath + len ) = 0;
        pEntry->setUpdating( ret == 0 );
        
    }
    if (( st.st_mtime != pE->m_lastMod )||
        ( st.st_ino  != pE->m_inode )||
        ( st.st_size != pE->m_lSize ))
    {
        return 1;
    }
    return 0;
}

int DirHashCacheStore::renameDiskEntry( CacheEntry * pEntry, char * pFrom,
                 const char * pFromSuffix, const char * pToSuffix, int validate )
{
    struct stat stFromFd;
    struct stat stFromDir;
    struct stat stTo;
    char achFrom[4096];
    char achTo[4096];
    int fd = pEntry->getFdStore();
    if ( !pFrom )
        pFrom = achFrom;
    int n = buildCacheLocation( pFrom, 4090, pEntry->getHashKey() );
    if ( n == -1 )
        return -1;
    memmove( achTo, pFrom, n + 1 );
    if ( pFromSuffix )
        strcat( &pFrom[n], pFromSuffix );
    if ( pToSuffix )
        strcat( &achTo[n], pToSuffix );
    if ( validate & DHCS_SOURCE_MATCH )
    {
        fstat( fd, &stFromFd );
        if ( stat( pFrom, &stFromDir ) == -1 )
            return -2;
        if ( stFromFd.st_ino != stFromDir.st_ino )  //tmp has been modified by someone else
            return -2;
    }
    if (( validate & DHCS_DEST_CHECK ) && (stat( achTo, &stTo ) != -1 ))   // old 
    {
        if ( stFromFd.st_mtime >= stTo.st_mtime )
            unlink( achTo );
        else
        {
            return -3;
        }
    }

    if ( rename( pFrom, achTo ) == -1 )
        return -1;
    
    return 0;

}

int DirHashCacheStore::publish( CacheEntry * pEntry )
{
    char achTmp[4096];
    int fd = pEntry->getFdStore();
    if ( fd == -1 )
    {
        errno = EBADF;
        return -1;
    }
    pEntry->getHeader().m_tmExpire += DateTime_s_curTime - pEntry->getHeader().m_tmCreated;
    if ( lseek( fd, pEntry->getStartOffset()+4, SEEK_SET ) == -1 )
        return -1;
    if ( (size_t)write( fd, &pEntry->getHeader(), sizeof( CeHeader ) ) < 
                sizeof( CeHeader ) )
        return -1;

    int ret = renameDiskEntry( pEntry, achTmp, ".tmp", NULL,
                DHCS_SOURCE_MATCH|DHCS_DEST_CHECK );
    if ( ret )
    {
        return ret;
    }

    int len = strlen( achTmp );
    achTmp[len - 3] = 'S';
    achTmp[len - 2] = 0;
    unlink( achTmp );

    CacheStore::iterator iter = find( pEntry->getHashKey().getKey() );
    if ( iter != end() )
        dispose( iter, 0 );

    updateEntryState( (DirHashCacheEntry * )pEntry );
    insert( pEntry->getHashKey().getKey(), pEntry );
    
    //Rename the record in pEntry
    achTmp[len - 4] = 0;
    pEntry->setFilePath(achTmp);
    return 0;
}


