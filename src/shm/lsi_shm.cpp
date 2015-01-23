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
/*
*   LiteSpeed SHM interface 
*/
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>

#include <shm/lsshm.h>
#include <shm/lsshmpool.h>
#include <shm/lsshmhash.h>
#include <shm/lsi_shm.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  LiteSpeed SHM CONTAINER 
 */
lsi_shm_t *lsi_shm_open(const char * shmname, 
                       size_t initialsize)
{
    return LsShm::get(shmname, initialsize);
}

int lsi_shm_close(lsi_shm_t *shmhandle)
{
    ((LsShm*)shmhandle)->unget();
    return 0;
}

int lsi_shm_destroy(lsi_shm_t *shmhandle)
{
    ::unlink( ((LsShm*)shmhandle)->fileName() );
    ((LsShm*)shmhandle)->unget();
    return 0;
}

/*
 *  SHM memory allocator
 */
lsi_shmpool_t *lsi_shmpool_open(lsi_shm_t *shmhandle, const char * poolname)
{
    return LsShmPool::get((LsShm*)shmhandle, poolname);
}

lsi_shmpool_t *lsi_shmpool_openbyname(const char * shmname, size_t initialsize)
{
    return LsShmPool::get(LsShm::get(shmname, initialsize), NULL);
}

int lsi_shmpool_close(lsi_shmpool_t *poolhandle)
{
    ((LsShmPool*)poolhandle)->unget();
    return 0;
}

int lsi_shmpool_destroy(lsi_shmpool_t *poolhandle)
{
    delete poolhandle;
    return 0;
}

lsi_shm_off_t lsi_shmpool_alloc2(lsi_shmpool_t *poolhandle, size_t size)
{
    int remap = 0;
    return ((LsShmPool*)poolhandle)->alloc2(size, remap);
}

void lsi_shmpool_release2(lsi_shmpool_t *poolhandle, lsi_shm_off_t key, size_t size)
{
    ((LsShmPool*)poolhandle)->release2(key, size);
}

uint8_t * lsi_shmpool_key2ptr(lsi_shmpool_t *poolhandle , lsi_shm_off_t key)
{
    return (uint8_t*)((LsShmPool*)poolhandle)->offset2ptr(key);
}

/*
 *  LiteSpeed SHM HASH
 */
lsi_shmhash_t *lsi_shmhash_open(lsi_shmpool_t *poolhandle
                                , const char * hash_table_name
                                , size_t initialsize
                                , lsi_hash_fn hf
                                , lsi_val_comp vc
                              )
{
    if ( initialsize == 0 )
        initialsize = LSSHM_HASHINITSIZE;
    if ( hf == NULL )
        hf = (lsi_hash_fn)LsShmHash::hash_string;
    if ( vc == NULL )
        vc = (lsi_val_comp)LsShmHash::comp_string;
    
    return LsShmHash::get((LsShmPool*)poolhandle
                                , hash_table_name
                                , initialsize
                                , hf
                                , vc);
}

int lsi_shmhash_close(lsi_shmhash_t *hashhandle)
{
    ((LsShmHash*)hashhandle)->unget();
    return 0;
}

int lsi_shmhash_destroy(lsi_shmhash_t *hashhandle)
{
    delete hashhandle;
    return 0;
}

lsi_shm_key_t lsi_shmhash_alloc2(lsi_shmhash_t *hashhandle, size_t size)
{
    int remap = 0;
    return ((LsShmHash*)hashhandle)->alloc2(size, remap);
}

void lsi_shmhash_release2(lsi_shmhash_t *hashhandle
                        , lsi_shm_key_t key
                        , size_t size)
{
    ((LsShmHash*)hashhandle)->release2(key, size);
}

uint8_t * lsi_shmhash_key2ptr(lsi_shmhash_t *hashhandle , lsi_shm_key_t key)
{
    return (uint8_t *)((LsShmHash*)hashhandle)->offset2ptr( key);
}

uint8_t * lsi_shmhash_datakey2ptr(lsi_shmhash_t *hashhandle , lsi_shm_off_t key)
{
    return (uint8_t *)((LsShmHash*)hashhandle)->offset2ptr( (lsi_shmhash_datakey_t)key);
}

lsi_shmhash_datakey_t lsi_shmhash_find(lsi_shmhash_t *hashhandle , 
                               const uint8_t * key, int keylen,
                               int * retsize)
{
    return ((LsShmHash*)hashhandle)->find((const void *)key, keylen, retsize);
}

lsi_shmhash_datakey_t lsi_shmhash_get(lsi_shmhash_t *hashhandle , 
                               const uint8_t * key, int keylen,
                               int * retsize, int * pFlag)
{
    return ((LsShmHash*)hashhandle)->get((const void *)key, keylen, retsize, pFlag);
}

lsi_shmhash_datakey_t lsi_shmhash_set(lsi_shmhash_t *hashhandle , 
                                 const uint8_t * key, int keylen, 
                                 const uint8_t * value, int valuelen)
{
    return ((LsShmHash*)hashhandle)->set((const void *)key, keylen, (const void *)value, valuelen);
}

lsi_shmhash_datakey_t lsi_shmhash_insert(lsi_shmhash_t *hashhandle , 
                                 const uint8_t * key, int keylen, 
                                 const uint8_t * value, int valuelen)
{
    return ((LsShmHash*)hashhandle)->insert((const void *)key, keylen, (const void *)value, valuelen);
}

lsi_shmhash_datakey_t lsi_shmhash_update(lsi_shmhash_t *hashhandle , 
                                 const uint8_t * key, int keylen, 
                                 const uint8_t * value, int valuelen)
{
    return ((LsShmHash*)hashhandle)->update((const void *)key, keylen, (const void *)value, valuelen);
}

void lsi_shmhash_remove(lsi_shmhash_t *hashhandle, 
                                 const uint8_t * key, int keylen)
{
    ((LsShmHash*)hashhandle)->remove((const void *)key, keylen);
}

void lsi_shmhash_clear(lsi_shmhash_t *hashhandle)
{ 
    ((LsShmHash*)hashhandle)->clear();
}

/*
 *  Hash Table Scan functions
 */
/*
 * scanner helper data block
 */
typedef struct {
    /* homing sequence */
    uint32_t            m_magic;    /* magic to scan for    */
    int                 m_len;      /* len of the data      */
    
    /* call back data */
    lsi_shmhash_t *      m_handle;   /* hash handle          */
    void *              m_pUdata;   /* user callback data   */
    shmhash_rawdata_fn2 m_fn2;      /* callback with pUdata */
    
    int                 m_maxCheck;  /* 0 - no max          */
    int                 m_numChecked;/* updated by checker  */
} lsi_shmhash_scandata_t ;
    
/* 
 * @brief lsi_shmhash_scanhelper
 * @brief (1) hash table scan functions 
 * @brief (2) the homing sequence is magic and size
 * @brief (3) raw access for quick respones
 * 
 * @brief NOTE- build this for LsLuaShm interface
 */
static int lsi_shmhash_scanhelper_iterator( LsShmHash::iterator iter
                                    , void * pUdata)
{
    lsi_shmhash_scandata_t * pData = (lsi_shmhash_scandata_t*)pUdata;
    register uint32_t * lp = (uint32_t*)iter->p_value();
    
    /* check homing sequence */
    if ( ( iter->x_valueLen == pData->m_len )
            && ( lp[0] == pData->m_magic ) )
    {
        return pData->m_fn2( pData->m_handle, 
                              iter->p_value(), pData->m_pUdata);
    }
    return 0; /* move on to next */
}

/*
 *  remove the iterator is check return 1
 *
 */
static int lsi_shmhash_scanhelperx_iterator( LsShmHash::iterator iter
                                    , void * pUdata)
{
    lsi_shmhash_scandata_t * pData = (lsi_shmhash_scandata_t*)pUdata;
    register uint32_t * lp = (uint32_t*)iter->p_value();
    
    /* check homing sequence */
    if ( ( iter->x_valueLen == pData->m_len )
            && ( lp[0] == pData->m_magic ) )
    {
        if (pData->m_fn2( pData->m_handle, 
                              iter->p_value(), pData->m_pUdata))
        {
            ((LsShmHash*)(pData->m_handle))->erase_iterator(iter);
            pData->m_numChecked++;
            if ( (pData->m_maxCheck)
                        && (pData->m_numChecked >= pData->m_maxCheck) )
                return 1; /* done */
        }
    }
    return 0; /* move on to next */
}

int lsi_shmhash_scanraw(lsi_shmhash_t *hashhandle
                    , uint32_t magic
                    , int len
                    , shmhash_rawdata_fn2 fn2
                    , void * pUdata)
{
    lsi_shmhash_scandata_t cb_data;
    
    cb_data.m_magic = magic;
    cb_data.m_len = len;
    cb_data.m_handle = hashhandle;
    cb_data.m_fn2 = fn2;
    cb_data.m_pUdata = pUdata;
    cb_data.m_numChecked = 0;
    cb_data.m_maxCheck = 0;
    
    LsShmHash* pLsShmHash = (LsShmHash*)hashhandle;
    return pLsShmHash->for_each2(pLsShmHash->begin()
                        , pLsShmHash->end()
                        , lsi_shmhash_scanhelper_iterator
                        , &cb_data);
}

/*
 *  @brief lsi_shmhash_scanraw_checkremove
 *  @brief perform the hash table scan, 
 *  @brief     if the magic and size matched (good homing sequence)
 *  @brief         then check function will get called  
 *  @brief             check function return 1 indicated to perform remove
 *  @brief             check function return 0 indicated do nothing
 *  @brief
 *  
*/
int lsi_shmhash_scanraw_checkremove( lsi_shmhash_t *hashhandle
                    , uint32_t magic
                    , int len
                    , shmhash_rawdata_fn2 fn2
                    , void* pUdata
                    )
{
    lsi_shmhash_scandata_t cb_data;
    
    cb_data.m_magic = magic;
    cb_data.m_len = len;
    cb_data.m_handle = hashhandle;
    cb_data.m_fn2 = fn2;
    cb_data.m_pUdata = pUdata;
    cb_data.m_numChecked = 0;
    cb_data.m_maxCheck = (int)*((int*)pUdata);
    
    LsShmHash* pLsShmHash = (LsShmHash*)hashhandle;
    pLsShmHash->for_each2(pLsShmHash->begin()
                        , pLsShmHash->end()
                        , lsi_shmhash_scanhelperx_iterator
                        , &cb_data);
    return cb_data.m_numChecked;
}

/* Hash table statistic */
int                     lsi_shmhash_stat(lsi_shmhash_t *hashhandle
                        , LsHash_stat_t * p_hashstat
                        )
{
    LsShmHash* pLsShmHash = (LsShmHash*)hashhandle;
    return pLsShmHash->stat(p_hashstat, NULL);
}

/*
 *  LiteSpeed SHM Lock
 */
lsi_shmlock_t *lsi_shmlock_get(lsi_shm_t *shmhandle)
{
    return ((LsShm*)shmhandle)->allocLock();
}

int lsi_shmlock_remove(lsi_shm_t *shmhandle, lsi_shmlock_t *lock)
{
    return ((LsShm*)shmhandle)->freeLock((lsi_shmlock_t*)lock);
}


#ifdef __cplusplus
};
#endif
