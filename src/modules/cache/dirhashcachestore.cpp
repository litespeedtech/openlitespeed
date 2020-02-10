/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
#include <ls.h>
#include "dirhashcachestore.h"
#include "dirhashcacheentry.h"
#include "cachehash.h"

#include <util/datetime.h>
#include <util/stringtool.h>
#include <util/ni_fio.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
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

int DirHashCacheStore::updateEntryState(DirHashCacheEntry *pEntry)
{
    struct stat st;
    if (fstat(pEntry->getFdStore(), &st) == -1)
        return -1;
    pEntry->m_lastCheck = DateTime::s_curTime;
    pEntry->setLastAccess(DateTime::s_curTime);
    pEntry->m_lastMod   = st.st_mtime;
    pEntry->m_inode     = st.st_ino;
    pEntry->m_lSize     = st.st_size;
    return 0;
}

int DirHashCacheStore::isEntryExist(const CacheHash &hash, const char *pSuffix,
                                    struct stat *pStat, int isPrivate)
{
    char achBuf[4096];
    struct stat st;
    int n = buildCacheLocation(achBuf, 4096, hash.getKey(), isPrivate);
    if (pSuffix)
        lstrncpy(&achBuf[n], pSuffix, sizeof(achBuf) - n);
    if (!pStat)
        pStat = &st;
    if (nio_stat(achBuf, pStat) == 0)
        return 1;
    return 0;
}

int DirHashCacheStore::isEntryUpdating(const CacheHash &hash, int isPrivate)
{
    struct stat st;
    if ((isEntryExist(hash, ".tmp", &st, isPrivate) == 1) &&
        (DateTime::s_curTime - st.st_mtime <= 300))
        return 1;
    return 0;
}

int DirHashCacheStore::isEntryStale(const CacheHash &hash, int isPrivate)
{
    struct stat st;
    if (isEntryExist(hash, ".S", &st, isPrivate) == 1)
        return 1;
    return 0;
}

int DirHashCacheStore::processStale(CacheEntry *pEntry, char *pBuf,
                                    size_t maxBuf, int pathLen)
{
    int dispose = 0;
    if (DateTime::s_curTime - pEntry->getExpireTime() > pEntry->getMaxStale())
        {
        g_api->log(NULL, LSI_LOG_DEBUG, "[CACHE] [%p] has expired, dispose"
                   , pEntry);

        getManager()->incStats(pEntry->isPrivate(), offsetof(cachestats_t,
                               expired));

        dispose = 1;
    }
    else
    {
        if (!pEntry->isStale())
        {
            pEntry->setStale(1);
            if (!pathLen)
                pathLen = buildCacheLocation(pBuf, 4096, pEntry->getHashKey().getKey(),
                                             pEntry->isPrivate());
            if (renameDiskEntry(pEntry, pBuf, maxBuf, NULL, ".S",
                                DHCS_SOURCE_MATCH | DHCS_DEST_CHECK) != 0)
            {
                g_api->log(NULL, LSI_LOG_DEBUG, "[CACHE] [%p] is stale, [%s] mark stale"
                           , pEntry, pBuf);
                dispose = 1;
            }

        }
        if (!pEntry->isUpdating())
        {
            if (isEntryUpdating(pEntry->getHashKey(), pEntry->isPrivate()))
                pEntry->setUpdating(1);
        }
    }
    return dispose;
}


CacheEntry *DirHashCacheStore::getCacheEntry(CacheHash &hash,
        CacheKey *pKey, int maxStale, int32_t lastCacheFlush)
{
    char achBuf[4096] = "";
    int fd;
    //FIXME: look up cache entry in memory, then on disk
    CacheStore::iterator iter = find(hash.getKey());
    CacheEntry *pEntry = NULL;
    int dispose = 0;
    int stale = 0;
    int pathLen = 0;
    int ret = 0;

    if (iter != end())
    {
        pEntry = iter.second();

        debug_dump(pEntry, "found entry in hash");

        if (pEntry->isBuilding())
            return pEntry;

        int lastCheck = ((DirHashCacheEntry *)pEntry)->m_lastCheck;

        if ((DateTime::s_curTime != lastCheck)
            || (lastCheck == -1))   //This entry is being written to disk
        {
            pathLen = buildCacheLocation(achBuf, 4096, hash.getKey(),
                                         pEntry->isPrivate());
            if (isChanged((DirHashCacheEntry *)pEntry, achBuf, pathLen, sizeof(achBuf)))
            {
                g_api->log(NULL, LSI_LOG_DEBUG, "[CACHE] [%p] path [%s] has been modified "
                           "on disk, mark dirty", pEntry, achBuf);

                //updated by another process, do not remove current object on disk
                erase(iter);
                addToDirtyList(pEntry);
                pEntry = NULL;
                iter = end();
            }
        }
    }
    else
    {
        if (!getManager()->isInTracker(hash.getKey(), HASH_KEY_LEN, 
                                       pKey->m_pIP != NULL))
            return NULL;
    }
    
    if ((pEntry == NULL) || (pEntry->getFdStore() == -1))
    {
        if (!pathLen)
            pathLen = buildCacheLocation(achBuf, 4096, hash.getKey(),
                                         pKey->m_pIP != NULL);

        fd = ::open(achBuf, O_RDONLY);
        if (fd == -1)
        {
            lstrncpy(&achBuf[pathLen], ".S", sizeof(achBuf) - pathLen);
            fd = ::open(achBuf, O_RDONLY);
            achBuf[pathLen] = 0;
            if (fd == -1)
            {
                if (errno != ENOENT)
                {
                    lstrncpy(&achBuf[pathLen], ": open() failed",
                            sizeof(achBuf) - pathLen);
                    perror(achBuf);
                }
                if (pEntry)
                    CacheStore::dispose(iter, 1);

                getManager()->incStats(pKey->m_pIP != NULL, offsetof(cachestats_t,
                                       misses));

                return NULL;
            }
            stale = 1;
        }
        ::fcntl(fd, F_SETFD, FD_CLOEXEC);
        if (pEntry)
            pEntry->setFdStore(fd);
        //LS_INFO( "getCacheEntry(), open fd: %d, entry: %p", fd, pEntry ));
    }
    if (!pEntry)
    {
        pEntry = new DirHashCacheEntry();
        pEntry->setFdStore(fd);
        pEntry->setHashKey(hash);
        //pEntry->setKey( hash, pURI, iURILen, pQS, iQSLen, pIP, ipLen, pCookie, cookieLen );
        if (pEntry->loadCeHeader() == -1)
        {
            delete pEntry;
            return NULL;
        };
        //assert( pEntry->verifyHashKey() == 0 );

        debug_dump(pEntry, "load entry from disk");

        if (pEntry->isUnderConstruct())
        {
            if (isCreatorAlive(pEntry))
            {
                g_api->log(NULL, LSI_LOG_INFO, 
                           "[CACHE] %p [%s], is under construction by PID: %d",
                            pEntry, pEntry->getHashKey().to_str(NULL),
                            pEntry->getHeader().m_pidCreator);
                addToHash(pEntry);
            }
            else
            {
                removeDeadEntry(pEntry, hash, achBuf);
                pEntry = NULL;
            }    
            return pEntry;
        }
        
        updateEntryState((DirHashCacheEntry *)pEntry);
        if (stale)
        {
            g_api->log(NULL, LSI_LOG_DEBUG,
                       "[CACHE] [%p] [%s] found stale copy, mark stale", pEntry,
                       pEntry->getHashKey().to_str(NULL));
            pEntry->setStale(1);
        }
        pEntry->setMaxStale(maxStale);
    }

    if (pEntry->isStale() || DateTime::s_curTime > pEntry->getExpireTime())
    {
        dispose = processStale(pEntry, achBuf, sizeof(achBuf), pathLen);
    }
    g_api->log(NULL, LSI_LOG_DEBUG,
               "[CACHE] check [%p] against cache manager, tag: '%s' \n",
               pEntry, pEntry->getTag().c_str());

    if ( !dispose && pEntry->getHeader().m_tmCreated <= lastCacheFlush)
    {
        g_api->log(NULL, LSI_LOG_DEBUG,
                   "[CACHE] [%p] has been flushed, dispose.\n", pEntry);
        dispose = 1;
    }
    g_api->log(NULL, LSI_LOG_DEBUG,
               "[CACHE] check [%p] against cache manager, tag: '%s' \n",
               pEntry, pEntry->getTag().c_str());
    
    if (!dispose )
    {
        int flag = getManager()->isPurged(pEntry, pKey, (lastCacheFlush >= 0));
        if (flag)
        {
            g_api->log(NULL, LSI_LOG_DEBUG,
                       "[CACHE] [%p] has been purged by cache manager, %s",
                       pEntry, (flag & PDF_STALE) ? "stale" : "dispose");
            if (flag &PDF_STALE)
                dispose = processStale(pEntry, achBuf, sizeof(achBuf), pathLen);
            else
                dispose = 1;
        }
    }

    if (dispose)
    {
        if (iter != end())
            CacheStore::dispose(iter, 1);
        else
        {
            if (pEntry->isStale())
            {
                if (!achBuf[0])
                    buildCacheLocation(achBuf, 4096, hash.getKey(), pEntry->isPrivate());
                strcpy(&achBuf[pathLen], ".S");
            }
            removeDeadEntry(pEntry, hash, achBuf);
        }
        return NULL;
    }

    if ((ret = pEntry->verifyKey(pKey)) != 0)
    {
        g_api->log(NULL, LSI_LOG_DEBUG,
                   "[CACHE] [%p] does not match cache key, key confliction detect, do not use [ret=%d].\n"
                   , pEntry, ret);

        getManager()->incStats(pEntry->isPrivate(), offsetof(cachestats_t,
                               collisions));

        if (iter == end())
            delete pEntry;
        return NULL;
    }
    if (iter == end())
        addToHash(pEntry);
    return pEntry;

}


int DirHashCacheStore::isCreatorAlive(CacheEntry *pEntry)
{
    if (pEntry->getHeader().m_pidCreator == 0)
        return 0;
    return (kill(pEntry->getHeader().m_pidCreator, 0) != -1 
            || errno != ESRCH);
}


int DirHashCacheStore::buildCacheLocation(char *pBuf, int len,
        const unsigned char *pHashKey, int isPrivate)
{
    int n = getCacheDirPath(pBuf, len, pHashKey, isPrivate);
    StringTool::hexEncode((char *)pHashKey, HASH_KEY_LEN, &pBuf[n]);
    n += 2 * HASH_KEY_LEN;
    return n;
}

class TempUmask
{
public:
    TempUmask(int newumask)
    {
        m_umask = umask(newumask);
    }
    ~TempUmask()
    {
        umask(m_umask);
    }
private:
    int m_umask;
};


static int createMissingPath(char *pPath, char *pPathEnd, int isPrivate)
{
    struct stat st;
    pPathEnd[-2] = 0;
    if ((nio_stat(pPath, &st) == -1) && (errno == ENOENT))
    {
        pPathEnd[-4] = 0;
        if ((nio_stat(pPath, &st) == -1) && (errno == ENOENT))
        {
            if (isPrivate)
            {
                pPathEnd[-6] = 0;
                if ((nio_stat(pPath, &st) == -1) && (errno == ENOENT))
                {
                    if ((mkdir(pPath, 0770) == -1) && (errno != EEXIST))
                        return -1;
                }
                pPathEnd[-6] = '/';
            }
            if ((mkdir(pPath, 0770) == -1) && (errno != EEXIST))
                return -1;
        }
        pPathEnd[-4] = '/';
        if (mkdir(pPath, 0770) == -1)
            return -1;
    }
    pPathEnd[-2] = '/';
    if (mkdir(pPath, 0770) == -1)
        return -1;
    return 0;

}



int DirHashCacheStore::createCacheFile(const CacheHash *pHash, bool is_private)
{
    char achBuf[4096];
    char *pPathEnd;
    int n = buildCacheLocation(achBuf, 4096, pHash->getKey(), is_private);
    struct stat st;
    TempUmask tumsk(0007);

    lstrncpy(&achBuf[n], ".tmp", sizeof(achBuf) - n);
    if (nio_stat(achBuf, &st) == 0)
    {
        //in progress
        if (DateTime::s_curTime - st.st_mtime > 120)
            unlink(achBuf);
        else
            return -1;
    }

    pPathEnd = &achBuf[n - 2 * HASH_KEY_LEN - 1];
    *pPathEnd = 0;

    if ((nio_stat(achBuf, &st) == -1) && (errno == ENOENT))
    {
        if (createMissingPath(achBuf, pPathEnd, is_private) == -1)
            return -1;

    }
    *pPathEnd = '/';

    int fd = ::open(achBuf, O_RDWR | O_CREAT | O_TRUNC | O_EXCL, 0660);
    if (fd != -1)
        ::fcntl(fd, F_SETFD, FD_CLOEXEC);
    //LS_INFO( "createCacheEntry(), open fd: %d", fd ));
    return fd;
}


CacheEntry *DirHashCacheStore::createCacheEntry(
    const CacheHash &hash, CacheKey *pKey)
{
    bool is_private = pKey->isPrivate();
    int fd = createCacheFile(&hash, is_private);
    if (fd == -1)
        return NULL;

    CacheEntry *pEntry = new DirHashCacheEntry();
    pEntry->setFdStore(fd);
    pEntry->setKey(hash, pKey);
    return initCacheEntry(pEntry, is_private);
}



CacheEntry *DirHashCacheStore::initCacheEntry(CacheEntry *pEntry, bool is_private)
{
    static pid_t s_pid = 0;
    pEntry->setBuilding(1);
    if (is_private)
        pEntry->getHeader().m_flag |= CeHeader::CEH_PRIVATE;
    int32_t lastmode = pEntry->getHeader().m_tmLastMod;
    if (s_pid == 0)
        s_pid = getpid();
    pEntry->getHeader().m_pidCreator = s_pid;
    pEntry->saveCeHeader();
    pEntry->getHeader().m_tmLastMod = lastmode;

    //update current entry
    CacheStore::iterator iter = find(pEntry->getHashKey().getKey());
    if (iter != end())
        iter.second()->setUpdating(1);
    else
        addToHash(pEntry);
    return pEntry;
}


// remove:  0  do not remove temp file
//          1  remove temp file without checking
//          -1 check temp file inode then remove
void DirHashCacheStore::cancelEntry(CacheEntry *pEntry, int remove)
{
    char achBuf[4096];
    CacheStore::iterator iter = find(pEntry->getHashKey().getKey());
    if (iter != end())
        iter.second()->setUpdating(0);
    if (remove)
    {
        int n = buildCacheLocation(achBuf, 4096, pEntry->getHashKey().getKey(),
                                   pEntry->isPrivate());
        lstrncpy(&achBuf[n], ".tmp", sizeof(achBuf) - n);
        if ((pEntry->getFdStore() != -1) && remove == -1)
        {
            struct stat stFd;
            struct stat stDir;
            if ((fstat(pEntry->getFdStore(), &stFd) != 0) ||
                (nio_stat(achBuf, &stDir) != 0) ||
                (stFd.st_ino != stDir.st_ino))    //tmp has been modified by someone else
                remove = 0;
        }
        if (remove)
            unlink(achBuf);
    }
    close(pEntry->getFdStore());
    pEntry->setFdStore(-1);
    cancelEntryInMem(pEntry);

}


void DirHashCacheStore::cancelEntryInMem(CacheEntry* pEntry)
{
    if (pEntry->getFdStore() != -1)
    {
        close(pEntry->getFdStore());
        pEntry->setFdStore(-1);
    }

    CacheStore::iterator iter = find(pEntry->getHashKey().getKey());
    if (iter != end())
    {
        if (iter.second() != pEntry)
            iter.second()->setUpdating(0);
        else
        {
            getManager()->removeTracking((const char *)(pEntry->getHashKey().getKey()),
                                        HASH_KEY_LEN, pEntry->isPrivate());
            erase(iter);
        }
    }
    if (pEntry->getRef() > 0)
    {
        addToDirtyList(pEntry);
    }
    else
        delete pEntry;

}

CacheEntry *DirHashCacheStore::getCacheEntry(const char *pKey,
        int keyLen)
{
    return NULL;
}

CacheEntry *DirHashCacheStore::getWriteEntry(const char *pKey,
        int keyLen, const char *pHash)
{
    return NULL;
}

int DirHashCacheStore::saveEntry(CacheEntry *pEntry)
{

    return 0;
}

void DirHashCacheStore::removePermEntry(CacheEntry *pEntry)
{
    char achBuf[4096];
    buildCacheLocation(achBuf, 4096, pEntry->getHashKey().getKey(),
                       pEntry->isPrivate());
    unlink(achBuf);
}

void DirHashCacheStore::getEntryFilePath(CacheEntry *pEntry, char *pPath, int &len)
{
    assert(len >= 4096);
    len = buildCacheLocation(pPath, 4096, pEntry->getHashKey().getKey(),
                             pEntry->isPrivate());
}
/*
void DirHashCacheStore::renameDiskEntry( CacheEntry * pEntry )
{
    char achBuf[4096];
    char achBufNew[4096];
    buildCacheLocation( achBuf, 4096, pEntry->getHashKey() );
    lstrncpy( achBufNew, achBuf, sizeof(achBufNew) );
    lstrncat( achBufNew, ".d", sizeof(achBufNew) );
    rename( achBuf, achBufNew );
    //unlink( achBuf );
}
*/

/*
int DirHashCacheStore::dirty( const char * pKey, int keyLen )
{

}
*/

int DirHashCacheStore::isChanged(CacheEntry *pEntry, const char *pPath,
                                 int len, size_t max_len)
{
    DirHashCacheEntry *pE = (DirHashCacheEntry *) pEntry;
    pE->m_lastCheck = DateTime::s_curTime;

    struct stat st;
    int ret = nio_stat(pPath, &st);
    if (ret == -1)
    {
        lstrncpy((char *)pPath + len, ".S", max_len - len);
        ret = nio_stat(pPath, &st);
        *((char *)pPath + len) = 0;
        if (ret == -1)
            return 1;
        pEntry->setStale(1);
        lstrncpy((char *)pPath + len, ".tmp", max_len - len);
        ret = nio_stat(pPath, &st);
        *((char *)pPath + len) = 0;
        pEntry->setUpdating(ret == 0);

    }
    if ((st.st_mtime != pE->m_lastMod) ||
        (st.st_ino  != pE->m_inode) ||
        (st.st_size != pE->m_lSize))
        return 1;
    return 0;
}

int DirHashCacheStore::renameDiskEntry(CacheEntry *pEntry, char *pFrom,
                                       size_t maxFrom, const char *pFromSuffix,
                                       const char *pToSuffix, int validate)
{
    struct stat stFromFd;
    struct stat stFromDir;
    struct stat stTo;
    char achFrom[4096];
    char achTo[4096];
    int fd = pEntry->getFdStore();
    if (!pFrom)
    {
        pFrom = achFrom;
        maxFrom = sizeof(achFrom);
    }
    int n = buildCacheLocation(pFrom, 4090, pEntry->getHashKey().getKey(),
                               pEntry->isPrivate());
    if (n == -1)
        return -1;
    memmove(achTo, pFrom, n + 1);
    if (pFromSuffix)
        lstrncpy(&pFrom[n], pFromSuffix, maxFrom - n);
    if (pToSuffix)
        lstrncpy(&achTo[n], pToSuffix, sizeof(achTo) - n);
    if (validate & DHCS_SOURCE_MATCH)
    {
        if (fstat(fd, &stFromFd) == -1)
            return -2;
        if (nio_stat(pFrom, &stFromDir) == -1)
            return -2;
        if (stFromFd.st_ino !=
            stFromDir.st_ino)    //tmp has been modified by someone else
            return -2;
    }
    if ((validate & DHCS_DEST_CHECK)
        && (stat(achTo, &stTo) != -1))        // old
    {
        if (stFromFd.st_mtime >= stTo.st_mtime)
            unlink(achTo);
        else
            return -3;
    }

    if (rename(pFrom, achTo) == -1)
        return -1;
    return 0;

}


int DirHashCacheStore::updateEntryExpire(CacheEntry* pEntry)
{
    int fd = pEntry->getFdStore();
    if (fd == -1)
    {
        errno = EBADF;
        return -1;
    }
    
    pEntry->getHeader().m_tmExpire += DateTime::s_curTime -
                                      pEntry->getHeader().m_tmCreated;
    if (nio_lseek(fd, pEntry->getStartOffset() + CACHE_ENTRY_MAGIC_LEN,
                  SEEK_SET) == -1)
        return -1;
    if (nio_write(fd, &pEntry->getHeader(), sizeof(CeHeader)) <
        (int)sizeof(CeHeader))
        return -1;
    return 0;
}


int DirHashCacheStore::updateHashEntry(CacheEntry* pEntry)
{
    updateEntryState((DirHashCacheEntry *)pEntry);
    pEntry->setLastPurgrCheck(DateTime::s_curTime);
    CacheStore::iterator iter = find(pEntry->getHashKey().getKey());
    getManager()->incStats(pEntry->isPrivate(), offsetof(cachestats_t,
                           created));
    if (iter != end())
    {
        if (iter.second() == pEntry)
            return 0;
        dispose(iter, 0);
    }
    addToHash(pEntry);
//     if (!pEntry->isPrivate() && pEntry->isEsi())
//         getManager()->getCacheInfo()->addLitemageCached(1);
    return 0;
}

int DirHashCacheStore::publish(CacheEntry *pEntry)
{
    char achTmp[4096];
    int fd = pEntry->getFdStore();
    if (fd == -1)
    {
        errno = EBADF;
        return -1;
    }
    pEntry->getHeader().m_tmExpire += DateTime::s_curTime -
                                      pEntry->getHeader().m_tmCreated;
    if (nio_lseek(fd, pEntry->getStartOffset() + 4, SEEK_SET) == -1)
        return -1;
    if (nio_write(fd, &pEntry->getHeader(), sizeof(CeHeader)) <
        (int)sizeof(CeHeader))
        return -1;

    pEntry->setBuilding(0);
    if (updateEntryExpire(pEntry) != 0)
        return -1;
    int ret = renameDiskEntry(pEntry, achTmp, sizeof(achTmp), ".tmp", NULL,
                              DHCS_SOURCE_MATCH | DHCS_DEST_CHECK);
    if (ret)
        return ret;

    int len = strlen(achTmp);
    achTmp[len - 3] = 'S';
    achTmp[len - 2] = 0;
    unlink(achTmp);
    achTmp[len - 4] = 0;
    if (pEntry->isDirty())
    {
        g_api->log(NULL, LSI_LOG_DEBUG, 
                   "[CACHE] [%s] is marked dirty, do not add to hash.", achTmp);
        return 0;
    }
    
    return updateHashEntry(pEntry);
}


void DirHashCacheStore::removeEntryByHash(const unsigned char *pKey, int isPrivate)
{
    char achBuf[4096];
    char *pathEnd; 
    pathEnd = &achBuf[0] + buildCacheLocation(achBuf, 4096, pKey, isPrivate);

    g_api->log(NULL, LSI_LOG_DEBUG, "[CACHE] remove cache object [%s].\n", achBuf);
    unlink(achBuf);
    
    pathEnd -= 2 * HASH_KEY_LEN + 1;
    assert(*pathEnd == '/');
    
    *pathEnd = 0;
    if (rmdir(achBuf) == 0)
    {
        pathEnd -= 2;
        assert(*pathEnd == '/');
        *pathEnd = 0;
        rmdir(achBuf);
    }
}


void DirHashCacheStore::removeDeadEntry(CacheEntry *pEntry, 
                                        const CacheHash &hash,
                                        char *achBuf)
{
    getManager()->removeTracking((const char *)hash.getKey(), 
                                    HASH_KEY_LEN, pEntry->isPrivate());
    if (!achBuf[0])
        buildCacheLocation(achBuf, 4096, hash.getKey(), pEntry->isPrivate());
    delete pEntry;
    unlink(achBuf);
}
