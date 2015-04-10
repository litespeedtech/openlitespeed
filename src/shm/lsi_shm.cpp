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
#include <shm/lsi_shm.h>

#include <shm/lsshmlruhash.h>
#include <shm/lsshmpool.h>

#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 *  LiteSpeed SHM CONTAINER
 */


lsi_shm_t *lsi_shm_open(const char *shmname, size_t initialsize)
{
    return LsShm::open(shmname, initialsize);
}


int lsi_shm_close(lsi_shm_t *shmhandle)
{
    ((LsShm *)shmhandle)->close();
    return 0;
}


int lsi_shm_destroy(lsi_shm_t *shmhandle)
{
    ::unlink(((LsShm *)shmhandle)->fileName());
    ((LsShm *)shmhandle)->close();
    return 0;
}


/*
 *  SHM memory allocator
 */


lsi_shmpool_t *lsi_shmpool_open(lsi_shm_t *shmhandle, const char *poolname)
{
    return ((LsShm *)shmhandle)->getNamedPool(poolname);
}


lsi_shmpool_t *lsi_shmpool_openbyname(const char *shmname,
                                      size_t initialsize)
{
    LsShm *pshm;
    if ((pshm = LsShm::open(shmname, initialsize)) == NULL)
        return NULL;
    return pshm->getGlobalPool();
}


int lsi_shmpool_close(lsi_shmpool_t *poolhandle)
{
    ((LsShmPool *)poolhandle)->close();
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
    return ((LsShmPool *)poolhandle)->alloc2(size, remap);
}


void lsi_shmpool_release2(
    lsi_shmpool_t *poolhandle, lsi_shm_off_t key, size_t size)
{
    ((LsShmPool *)poolhandle)->release2(key, size);
}


uint8_t *lsi_shmpool_key2ptr(lsi_shmpool_t *poolhandle, lsi_shm_off_t key)
{
    return (uint8_t *)((LsShmPool *)poolhandle)->offset2ptr(key);
}


lsi_shm_off_t lsi_shmpool_getreg(lsi_shmpool_t *poolhandle,
                                 const char *name)
{
    LsShmReg *p_reg;
    return ((p_reg = ((LsShmPool *)poolhandle)->findReg(name)) == NULL) ?
           0 : p_reg->x_iValue;
}


int lsi_shmpool_setreg(
    lsi_shmpool_t *poolhandle, const char *name, lsi_shm_off_t off)
{
    LsShmReg *p_reg = ((LsShmPool *)poolhandle)->addReg(name);
    if ((p_reg == NULL) || (off <= 0))
        return LS_FAIL;
    p_reg->x_iValue = off;
    return 0;
}


/*
 *  LiteSpeed SHM HASH
 */


static void check_defaults(
    size_t *pinitialsize, lsi_hash_fn *phf, lsi_val_comp *pvc)
{
    if (*pinitialsize == 0)
        *pinitialsize = LSSHM_HASHINITSIZE;
    if (*phf == NULL)
        *phf = (lsi_hash_fn)LsShmHash::hashString;
    if (*pvc == NULL)
        *pvc = (lsi_val_comp)LsShmHash::compString;
}


lsi_shmhash_t *lsi_shmhash_open(lsi_shmpool_t *poolhandle,
                                const char *hash_table_name,
                                size_t initialsize,
                                lsi_hash_fn hf,
                                lsi_val_comp vc)
{
    check_defaults(&initialsize, &hf, &vc);
    return ((LsShmPool *)poolhandle)->getNamedHash(hash_table_name, initialsize, hf, vc, LSSHM_LRU_NONE);
}


lsi_shmhash_t *lsi_shmlruhash_open(lsi_shmpool_t *poolhandle,
                                   const char *hash_table_name,
                                   size_t initialsize,
                                   lsi_hash_fn hf,
                                   lsi_val_comp vc,
                                   int mode)
{
    check_defaults(&initialsize, &hf, &vc);
    switch (mode)
    {
    case LSSHM_LRU_MODE2:
    case LSSHM_LRU_MODE3:
        return ((LsShmPool *)poolhandle)->getNamedHash(
                   hash_table_name, initialsize, hf, vc, mode);
    default:
        return NULL;
    }
}


int lsi_shmhash_close(lsi_shmhash_t *hashhandle)
{
    ((LsShmHash *)hashhandle)->close();
    return 0;
}


int lsi_shmhash_destroy(lsi_shmhash_t *hashhandle)
{
    delete hashhandle;
    return 0;
}


lsi_shm_off_t lsi_shmhash_hdroff(lsi_shmhash_t *hashhandle)
{
    return ((LsShmHash *)hashhandle)->lruHdrOff();
}


lsi_shm_key_t lsi_shmhash_alloc2(lsi_shmhash_t *hashhandle, size_t size)
{
    int remap = 0;
    return ((LsShmHash *)hashhandle)->alloc2(size, remap);
}


void lsi_shmhash_release2(lsi_shmhash_t *hashhandle,
                          lsi_shm_key_t key,
                          size_t size)
{
    ((LsShmHash *)hashhandle)->release2(key, size);
}


uint8_t *lsi_shmhash_key2ptr(lsi_shmhash_t *hashhandle, lsi_shm_key_t key)
{
    return (uint8_t *)((LsShmHash *)hashhandle)->offset2ptr(key);
}


uint8_t *lsi_shmhash_datakey2ptr(lsi_shmhash_t *hashhandle,
                                 lsi_shm_off_t key)
{
    return (uint8_t *)
           ((LsShmHash *)hashhandle)->offset2ptr((lsi_shmhash_datakey_t)key);
}


lsi_shmhash_datakey_t lsi_shmhash_find(lsi_shmhash_t *hashhandle,
                                       const uint8_t *key, int keylen, int *retsize)
{
    return ((LsShmHash *)hashhandle)->find((const void *)key, keylen, retsize);
}


lsi_shmhash_datakey_t lsi_shmhash_get(lsi_shmhash_t *hashhandle,
                                      const uint8_t *key, int keylen, int *retsize, int *pFlag)
{
    return ((LsShmHash *)hashhandle)->get(
               (const void *)key, keylen, retsize, pFlag);
}


lsi_shmhash_datakey_t lsi_shmhash_set(lsi_shmhash_t *hashhandle,
                                      const uint8_t *key, int keylen,
                                      const uint8_t *value, int valuelen)
{
    return ((LsShmHash *)hashhandle)->set(
               (const void *)key, keylen, (const void *)value, valuelen);
}


lsi_shmhash_datakey_t lsi_shmhash_insert(lsi_shmhash_t *hashhandle,
        const uint8_t *key, int keylen,
        const uint8_t *value, int valuelen)
{
    return ((LsShmHash *)hashhandle)->insert(
               (const void *)key, keylen, (const void *)value, valuelen);
}


lsi_shmhash_datakey_t lsi_shmhash_update(lsi_shmhash_t *hashhandle,
        const uint8_t *key, int keylen,
        const uint8_t *value, int valuelen)
{
    return ((LsShmHash *)hashhandle)->update(
               (const void *)key, keylen, (const void *)value, valuelen);
}


void lsi_shmhash_remove(lsi_shmhash_t *hashhandle,
                        const uint8_t *key, int keylen)
{
    ((LsShmHash *)hashhandle)->remove((const void *)key, keylen);
}


void lsi_shmhash_clear(lsi_shmhash_t *hashhandle)
{
    ((LsShmHash *)hashhandle)->clear();
}


int lsi_shmhash_setdata(lsi_shmhash_t *hashhandle,
                        LsShmOffset_t offVal, const uint8_t *value, int valuelen)
{
    return ((LsShmHash *)hashhandle)->setLruData(offVal, value, valuelen);
}


int lsi_shmhash_getdata(lsi_shmhash_t *hashhandle,
                        LsShmOffset_t offVal, LsShmOffset_t *pvalue, int cnt)
{
    return ((LsShmHash *)hashhandle)->getLruData(offVal, pvalue, cnt);
}


int lsi_shmhash_getdataptrs(lsi_shmhash_t *hashhandle,
                            LsShmOffset_t offVal, int (*func)(void *pData))
{
    return ((LsShmHash *)hashhandle)->getLruDataPtrs(offVal, func);
}


int lsi_shmhash_trim(lsi_shmhash_t *hashhandle,
                     time_t tmcutoff, int (*func)(iterator iter, void *arg), void *arg)
{
    return ((LsShmHash *)hashhandle)->trim(tmcutoff, func, arg);
}


int lsi_shmhash_check(lsi_shmhash_t *hashhandle)
{
    return ((LsShmHash *)hashhandle)->check();
}


int lsi_shmhash_lock(lsi_shmhash_t *hashhandle)
{
    return ((LsShmHash *)hashhandle)->lock();
}


int lsi_shmhash_unlock(lsi_shmhash_t *hashhandle)
{
    return ((LsShmHash *)hashhandle)->unlock();
}


/* Hash table statistic */
int lsi_shmhash_stat(lsi_shmhash_t *hashhandle, LsHashStat *phashstat)
{
    LsShmHash *pLsShmHash = (LsShmHash *)hashhandle;
    return pLsShmHash->stat(phashstat, NULL, 0);
}


/*
 *  LiteSpeed SHM Lock
 */
lsi_shmlock_t *lsi_shmlock_get(lsi_shm_t *shmhandle)
{
    return ((LsShm *)shmhandle)->allocLock();
}


int lsi_shmlock_remove(lsi_shm_t *shmhandle, lsi_shmlock_t *lock)
{
    return ((LsShm *)shmhandle)->freeLock((lsi_shmlock_t *)lock);
}


#ifdef __cplusplus
};
#endif

