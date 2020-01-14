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

#include "cachestore.h"
#include "cachehash.h"
#include "cacheentry.h"
#include <util/datetime.h>

#include "shmcachemanager.h"


CacheStore::CacheStore()
    : HashStringMap<CacheEntry * >(29,
                                   CacheHash::to_ghash_key,
                                   CacheHash::compare)
    , m_iTotalEntries(0)
    , m_iTotalHit(0)
    , m_iTotalMiss(0)
    , m_pManager(NULL)
{
}


CacheStore::~CacheStore()
{
    m_dirtyList.release_objects();
    if (m_pManager)
        delete m_pManager;
}


void CacheStore::setStorageRoot(const char *pRoot)
{
    if ((m_sRoot.c_str() != NULL)
        && (strcmp(pRoot, m_sRoot.c_str()) == 0))
        return;
    m_sRoot.setStr(pRoot);

}

int CacheStore::addToHash(CacheEntry *pEntry)
{
    assert(pEntry->isDirty() == 0);
    iterator iter = insert((char *)pEntry->getHashKey().getKey(), pEntry);
    assert(iter != end());
    return 0;
}

void CacheStore::addToDirtyList(CacheEntry *pEntry)
{
    g_api->log(NULL, LSI_LOG_DEBUG, 
               "[CACHE] addTodirtyList(): %p [%s], ref: %d, flag: %hx",
               pEntry, pEntry->getHashKey().to_str(NULL), pEntry->getRef(),
               pEntry->getHeader().m_flag);

    pEntry->setDirty();
    m_dirtyList.push_back(pEntry);
}

int CacheStore::initManager()
{
    if (!m_sRoot.c_str())
        return LS_FAIL;
    if (m_pManager)
        return LS_OK;
    m_pManager = new ShmCacheManager();
    if (m_pManager->init(m_sRoot.c_str()) == -1)
    {
        delete m_pManager;
        m_pManager = NULL;
        return LS_FAIL;
    }
    return LS_OK;
}




int CacheStore::stale(CacheEntry *pEntry)
{
    pEntry->setStale(1);
    if (renameDiskEntry(pEntry, NULL, 0, NULL, ".S", 1) == -1)
    {
        iterator iter = find(pEntry->getHashKey().getKey());
        if (iter != end())
            dispose(iter, 0);
    }
    return 0;
}


int CacheStore::dispose(CacheStore::iterator iter, int isRemovePermEntry)
{
    CacheEntry *pEntry = iter.second();
    erase(iter);
    if (isRemovePermEntry)
        removePermEntry(pEntry);
    if (pEntry->getRef() <= 0)
        delete pEntry;
    else
        m_dirtyList.push_back(pEntry);
    return 0;
}

int CacheStore::purge(CacheEntry  *pEntry)
{
    iterator iter = find(pEntry->getHashKey().getKey());
    if (iter != end())
    {
        dispose(iter, 1);
        return 1;
    }
    return 0;
}

int CacheStore::refresh(CacheEntry  *pEntry)
{
    return stale(pEntry);
    /*
    iterator iter = find( pEntry->getHashKey().getKey() );
    if ( iter != end() )
    {
        iter.second()->setStale( 1 );
        return 1;
    }
    return 0;
    */
}


void CacheStore::houseKeeping()
{
    CacheEntry *pEntry;
    iterator iterEnd = end();
    iterator iterNext;
    for (iterator iter = begin(); iter != iterEnd; iter = iterNext)
    {
        pEntry = (CacheEntry *)iter.second();
        iterNext = GHash::next(iter);
        if (pEntry->getRef() == 0 && !pEntry->isUnderConstruct() && !pEntry->isBuilding())
        {
            if (DateTime::s_curTime > pEntry->getExpireTime() + pEntry->getMaxStale())
            {
                dispose(iter, 1);
                continue;
            }
            else
            {
                int idle = DateTime::s_curTime - pEntry->getLastAccess();
                if (idle > 120)
                {
                    erase(iter);
                    delete pEntry;
                    continue;
                }
                if (idle > 10)
                    pEntry->releaseTmpResource();
            }
        }
        //else if (DateTime::s_curTime - pEntry->getLastAccess() > 300 )
        //{
        //    LS_INFO( "Idle Cache, fd: %d, ref: %d, force release.",
        //                pEntry->getFdStore(), pEntry->getRef() ));
        //    erase( iter );
        //    delete pEntry;
        //}
    }
    for (TPointerList< CacheEntry >::iterator it = m_dirtyList.begin();
         it != m_dirtyList.end();)
    {
        if ((*it)->getRef() == 0)
        {
            delete *it;
            it = m_dirtyList.erase(it);
        }
        //else if (DateTime::s_curTime - (*it)->getLastAccess() > 300 )
        //{
        //    LS_INFO( "Unreleased Cache in dirty list, fd: %d, ref: %d, force release.",
        //                (*it)->getFdStore(), (*it)->getRef() ));
        //    delete *it;
        //    it = m_dirtyList.erase( it );
        //}
        else
            ++it;
    }
}

int CacheStore::getCacheDirPath(char *pBuf, int len,
        const unsigned char *pHashKey, int isPrivate)
{
    return snprintf(pBuf, len, "%s%s%x/%x/%x/", getRoot().c_str(),
                    isPrivate ? "priv/" : "",
                    (pHashKey[0]) >> 4, pHashKey[0] & 0xf, (pHashKey[1]) >> 4);
}


void CacheStore::debug_dump(CacheEntry *pEntry, const char *msg)
{
    g_api->log(NULL, LSI_LOG_DEBUG, "[CACHE] %s: %p [%s], ref: %d, flag: 0x%02x, expire: %ld, stale: %d, cur_time: %ld\n",
           msg, pEntry, pEntry->getHashKey().to_str(NULL), pEntry->getRef(),
           pEntry->getHeader().m_flag, pEntry->getExpireTime(),
           pEntry->getMaxStale(), DateTime::s_curTime);
}

#include <shm/lsshmhash.h>
int CacheStore::cleanByTrackingCb(void * pIter, void *pParam)
{
    LsShmHash::iterator iter = (LsShmHash::iterator)pIter;
    shm_objtrack_t *pData = (shm_objtrack_t *)iter->getVal();
    
    if (pData->x_tmExpire < DateTime::s_curTime)
    {
        CacheStore *pThis = (CacheStore *)pParam;
        pThis->getManager()->updateStatsExpireByTracking(pData);
        pThis->removeEntryByHash(iter->getKey(), pData->x_flag & CM_TRACK_PRIVATE);
        return 1;
    }
    return 0;
}


int CacheStore::cleanByTracking(int public_max, int private_max)
{
    getManager()->trimExpiredByTracking(0, public_max, CacheStore::cleanByTrackingCb, this);
    getManager()->trimExpiredByTracking(1, private_max, CacheStore::cleanByTrackingCb, this);
    return 0;
}

