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
#include <stdlib.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shm/lsshmpool.h>
#include <shm/lsshmhash.h>
#include <shm/lsshmcache.h>
#include <http/httplog.h>


//
//  @brief LsShmCache 
//  @brief Shared Memory Hash Cache Container
//
LsShmCache::LsShmCache (
                  uint32_t              magic
                , const char *          cacheName
                , const char *          shmHashName
                , size_t                initHashSize
                , LsShm_size_t          uDataSize
                , LsShmHash::hash_fn    hf
                , LsShmHash::val_comp   vc
                , udata_init_fn         udataInitCallback
                , udata_remove_fn       udataRemoveCallback
                )
                : m_status(LSSHM_NOTREADY)
                , m_pShmHash(NULL)
                , m_hdrOffset(0)
                , m_magic(magic)
                , m_uDataSize(uDataSize)
                , m_dataInit_cb(udataInitCallback)
                , m_dataRemove_cb(udataRemoveCallback)
{
    if ( (m_pShmHash = LsShmHash::get(NULL
                    , shmHashName
                    , initHashSize
                    , hf
                    , vc
                    ) ) )
    { 
        lsShmReg_t * pReg;
        if (! (pReg = m_pShmHash->findReg (cacheName)) ) 
        {
            pReg = m_pShmHash->addReg(cacheName);
        }
        if (pReg)
        {
            if ( ! (m_hdrOffset = pReg->x_value) )
            {
                int remapped = 0;
                m_hdrOffset = m_pShmHash->alloc2(sizeof(lsShm_hCacheHdr_t), remapped );
                if ( ( !m_hdrOffset ) )
                {
                    // trouble no memory
                    m_status = LSSHM_ERROR;
                }
                else
                {
                    if (remapped)
                        remap();
                    pReg->x_value = m_hdrOffset;
                }
            }
        }
        if (m_status == LSSHM_NOTREADY)
        {
            m_status = LSSHM_READY;
        }
        m_pShmHash->enableManualLock(); // we will be responsible for the lock ourselve.
    }   
}

void LsShmCache::push( lsShm_hCacheData_t* pObj )
{
    if (pObj->x_tracked)
        return;

    register lsShm_hCacheHdr_t * pHdr = getCacheHeader();
    register LsShm_offset_t offset = m_pShmHash->ptr2offset(pObj);
    if (pHdr->x_front)
    {
        lsShm_hCacheData_t * pFront = (lsShm_hCacheData_t*) m_pShmHash->offset2ptr(pHdr->x_front);
        pFront->x_next = offset;
        pObj->x_back = pHdr->x_front;
    }
    else
    {
        // lonely child
        pHdr->x_back = offset;
        pObj->x_back = 0;
    }
    pHdr->x_front = offset;
    pHdr->x_num++;
    
    pObj->x_next = 0;
    pObj->x_tracked = 1;
}

void LsShmCache::pop()
{
    register lsShm_hCacheHdr_t * pHdr = getCacheHeader();
    register LsShm_offset_t offset = pHdr->x_back;
    
    if (offset)
    {
        lsShm_hCacheData_t * pBack = (lsShm_hCacheData_t*) 
                                            m_pShmHash->offset2ptr(offset);
        pHdr->x_back = pBack->x_next;
        if (pHdr->x_back == 0)
        {
            // queue is empty after pop...
            pHdr->x_front = 0;
        }
        else
        {
            lsShm_hCacheData_t * pNext = (lsShm_hCacheData_t*) 
                                            m_pShmHash->offset2ptr(pHdr->x_back);
            pNext->x_back = 0;
        }
        pHdr->x_num--;
        pBack->x_next = 0;
        pBack->x_tracked = 0;
    }
}

void LsShmCache::push_back( lsShm_hCacheData_t* pObj )
{ 
    if (pObj->x_tracked)
        return;
        
    register lsShm_hCacheHdr_t * pHdr = getCacheHeader();
    register LsShm_offset_t offset = m_pShmHash->ptr2offset(pObj);
    if (pHdr->x_back)
    {
        lsShm_hCacheData_t * pBack = (lsShm_hCacheData_t*) m_pShmHash->offset2ptr(pHdr->x_back);
        pBack->x_back = offset;
        pObj->x_next = pHdr->x_back;
    }
    else 
    {
        // lonely child
        pHdr->x_front = offset;
        pObj->x_next = 0;
    }
    pHdr->x_back = offset;
    pHdr->x_num++;
    pObj->x_back = 0; //ensure I am the last one
    pObj->x_tracked = 1;
}

void LsShmCache::setFirst( lsShm_hCacheData_t* pObj )
{
    register lsShm_hCacheHdr_t * pHdr = getCacheHeader();
    lsShm_hCacheData_t * pFirst = (lsShm_hCacheData_t*)m_pShmHash->offset2ptr(pHdr->x_front);
    if (pObj == pFirst)
        return;
    disconnectObj( pObj );
    push(pObj);
}

void LsShmCache::setLast( lsShm_hCacheData_t* pObj )
{
    register lsShm_hCacheHdr_t * pHdr = getCacheHeader();
    lsShm_hCacheData_t * pLast = (lsShm_hCacheData_t*)m_pShmHash->offset2ptr(pHdr->x_back);
    if (pObj == pLast)
        return;
    disconnectObj( pObj );
    push_back( pObj );
}

//
//  @brief disconnectObj
//  @brief disconnect the forward and backward links to the queue
//
void LsShmCache::disconnectObj( lsShm_hCacheData_t* pObj )
{
    if (!pObj->x_tracked)
        return ;
    
    register lsShm_hCacheHdr_t * pHdr = getCacheHeader();
    if (pObj->x_next)
    {
        lsShm_hCacheData_t * pNext = (lsShm_hCacheData_t*)m_pShmHash->offset2ptr(pObj->x_next);
        if (pObj->x_back)
        {
            lsShm_hCacheData_t * pBack = (lsShm_hCacheData_t*)m_pShmHash->offset2ptr(pObj->x_back);
            pBack->x_next = pObj->x_next;
            pNext->x_back = pObj->x_back;
        }
        else
        {
            pNext->x_back = 0;
            pHdr->x_back = pObj->x_next;
        }
    }
    else
    {
        if (pObj->x_back)
        {
            lsShm_hCacheData_t * pBack = (lsShm_hCacheData_t*)m_pShmHash->offset2ptr(pObj->x_back);
            pBack->x_next = 0;
            pHdr->x_front = pObj->x_back;
        }
        else
        {
            pHdr->x_back = 0;
            pHdr->x_front = 0;
        }
    }
    pHdr->x_num--;
    pObj->x_next = 0;
    pObj->x_back = 0;
    pObj->x_tracked = 0; // not in the queue
}

lsShm_hCacheData_t* LsShmCache::getObj( const void* pKey
                                        , int keyLen
                                        , const void* pValue
                                        , int valueLen
                                        , void* pUParam )
{
    lsShm_hCacheData_t * pObj;
    
#if 0
    pObj = findObj(pKey, keyLen);
    if ( pObj )
    {
        return pObj;
    }
#endif
    LsShmHash::iterator iter;
    if (!( iter = m_pShmHash->insert_iterator(pKey
                                        , keyLen
                                        , NULL
                                        , valueLen)) )
    {
        return NULL;
    }
    pObj = ( lsShm_hCacheData_t * ) iter->p_value();
    // ::memset(pObj, 0, sizeof(lsShm_hCacheData_t));
    ::memset(pObj, 0, valueLen);
    pObj->x_magic = m_magic;
    pObj->x_expireTime = DateTime::s_curTime;
    pObj->x_expireTimeMs = DateTime::s_curTimeUs / 1000;
    pObj->x_iteratorOffset = m_pShmHash->ptr2offset(iter);
#if 0
    // memset take care of these already...
    pObj->x_next = 0;
    pObj->x_back = 0;
    pObj->x_inited = 0;   /* init indication */
    pObj->x_tracked = 0;  /* push indication */
#endif
        
#ifdef DEBUG_RUN
    HttpLog::notice("LsShmCache::getObj %6d iter <%p> obj <%p> size %d"
            , ::getpid(), iter, pObj, (long)pObj-(long)iter);
#endif
    
    if (m_dataInit_cb)
        (*m_dataInit_cb)(pObj, pUParam);
    return pObj;
}

int LsShmCache::remove2NonExpired( void* pUParam )
{
    register int numRemoved = 0;    // number of cache items removed
    register lsShm_hCacheHdr_t * pHdr = getCacheHeader();
    LsShm_offset_t offset;

    while ( (offset = pHdr->x_back) )
    {
        register lsShm_hCacheData_t * pObj;
        pObj = (lsShm_hCacheData_t*)m_pShmHash->offset2ptr(offset);
        if ( isElementExpired(pObj) )
        {
            pop();
            // remove
            removeObjData(pObj, pUParam);
            numRemoved++;
        }
        else
        {
            // stop on first non expired
            break;
        }
    }
    return numRemoved;
}

int LsShmCache::removeAllExpired( void* pUParam )
{
    register int numRemoved = 0;   // number of cache items removed
    register lsShm_hCacheHdr_t * pHdr = getCacheHeader();
    LsShm_offset_t offset = pHdr->x_back;
        
    numRemoved = remove2NonExpired( pUParam );
    if ( (offset = pHdr->x_back) )
    {
        // skip the first one!
        register lsShm_hCacheData_t * pObj;
        pObj = (lsShm_hCacheData_t*)m_pShmHash->offset2ptr(offset);
        while ( (offset = pObj->x_next) )
        {
            pObj = (lsShm_hCacheData_t*)m_pShmHash->offset2ptr(offset);
            if ( isElementExpired(pObj) )
            {
                disconnectObj(pObj);
                removeObjData(pObj, pUParam);
                numRemoved++;
            }
        }
    }
    return numRemoved;
}

void LsShmCache::stat( LsHash_stat_t& stat, udata_callback_fn _cb )
{
    ::memset(&stat, 0, sizeof(LsHash_stat_t));
    register lsShm_hCacheHdr_t * pHdr = getCacheHeader();
    LsShm_offset_t offset = pHdr->x_back;
    while (offset)
    {
        lsShm_hCacheData_t * pObj;
        pObj = (lsShm_hCacheData_t *) m_pShmHash->offset2ptr(offset);
        if (isElementExpired(pObj))
            stat.numExpired++;
        stat.num++;
        
        if (_cb)
            (*_cb)(pObj, (void *)&stat);
        offset = pObj->x_next;
    }
}

int LsShmCache::loop( LsShmCache::udata_callback_fn _cb, void* pUParam )
{
    register int num = 0;
    register lsShm_hCacheHdr_t * pHdr = getCacheHeader();
    LsShm_offset_t offset = pHdr->x_back;
        
    while (offset)
    {
        register lsShm_hCacheData_t * pObj;
        pObj = (lsShm_hCacheData_t *) m_pShmHash->offset2ptr(offset);
                        
        int retValue = (*_cb)(pObj, pUParam);
        if (retValue)
            return retValue;
        
        offset = pObj->x_next;
        num++;
    }
    return num;
} 
