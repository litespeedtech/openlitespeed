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
#include <shm/lsshmlru.h>

#include <lsdef.h>


int shmlru_init(shmlru_t *pShmLru,
                const char *pPoolName, size_t iPoolSize,
                const char *pHashName, size_t iHashSize, lsi_hash_fn hf, lsi_val_comp vc,
                int mode)
{
    lsi_shmpool_t *pShmpool;
    lsi_shmhash_t *pShmhash;
    if ((pShmpool = lsi_shmpool_openbyname(pPoolName, iPoolSize)) == NULL)
        return LS_FAIL;
    if ((pShmhash = lsi_shmlruhash_open(
                        pShmpool, pHashName, iHashSize, hf, vc, mode)) == NULL)
        return LS_FAIL;
    pShmLru->phash = pShmhash;
    pShmLru->offhdr = lsi_shmhash_hdroff(pShmhash);

    return 0;
}


int shmlru_set(shmlru_t *pShmLru,
               const uint8_t *pKey, int keyLen, const uint8_t *pVal, int valLen)
{
    lsi_shmhash_t *pHash;
    if ((pShmLru == NULL) || ((pHash = pShmLru->phash) == NULL))
        return LS_FAIL;

    int ret;
    int mylen = valLen;
    int flag = LSSHM_FLAG_NONE;
    lsi_shm_off_t offVal;
    lsi_shmhash_lock(pHash);
    if ((offVal = lsi_shmhash_get(pHash, pKey, keyLen, &mylen, &flag)) == 0)
        ret = -1;
    else
        ret = lsi_shmhash_setdata(pHash, offVal, pVal, valLen);
    lsi_shmhash_unlock(pHash);
    return ret;
}


int shmlru_get(shmlru_t *pShmLru,
               const uint8_t *pKey, int keyLen, lsi_shm_off_t *pData, int cnt)
{
    lsi_shmhash_t *pHash;
    if ((pShmLru == NULL) || ((pHash = pShmLru->phash) == NULL))
        return LS_FAIL;

    int ret;
    int mylen;
    lsi_shm_off_t offVal;
    lsi_shmhash_lock(pHash);
    if ((offVal = lsi_shmhash_find(pHash, pKey, keyLen, &mylen)) == 0)
        ret = -1;
    else
        ret = lsi_shmhash_getdata(pHash, offVal, pData, cnt);
    lsi_shmhash_unlock(pHash);
    return ret;
}


int shmlru_getptrs(shmlru_t *pShmLru,
                   const uint8_t *pKey, int keyLen, int (*func)(void *pData))
{
    lsi_shmhash_t *pHash;
    if ((pShmLru == NULL) || ((pHash = pShmLru->phash) == NULL))
        return LS_FAIL;

    int ret;
    int mylen;
    lsi_shm_off_t offVal;
    lsi_shmhash_lock(pHash);
    if ((offVal = lsi_shmhash_find(pHash, pKey, keyLen, &mylen)) == 0)
        ret = -1;
    else
        ret = lsi_shmhash_getdataptrs(pHash, offVal, func);
    lsi_shmhash_unlock(pHash);
    return ret;
}


int shmlru_check(shmlru_t *pShmLru)
{
    lsi_shmhash_t *pHash;
    if ((pShmLru == NULL) || ((pHash = pShmLru->phash) == NULL))
        return SHMLRU_BADINIT;

    lsi_shmhash_lock(pHash);
    int ret = lsi_shmhash_check(pHash);
    lsi_shmhash_unlock(pHash);
    return ret;
}


int shmlru_trim(shmlru_t *pShmLru, time_t tmCutoff, int (*func)(iterator iter, void *arg), void *arg)
{
    lsi_shmhash_t *pHash;
    if ((pShmLru == NULL) || ((pHash = pShmLru->phash) == NULL))
        return LS_FAIL;

    lsi_shmhash_lock(pHash);
    int ret = lsi_shmhash_trim(pHash, tmCutoff, func, arg);
    lsi_shmhash_unlock(pHash);
    return ret;
}
