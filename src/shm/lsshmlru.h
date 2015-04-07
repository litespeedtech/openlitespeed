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
#ifndef LSSHMLRU_H
#define LSSHMLRU_H

#include <shm/lsi_shm.h>

#include <unistd.h>
#include <time.h>

/**
 * @file
 */


#ifdef __cplusplus
extern "C" {
#endif

#define LSSHM_LRU_MAGIC     0x20141201
#define SHMLRU_MINSAVECNT   5           /* minimum count to save history */

typedef uint32_t LsShmHKey;

typedef struct shmlru_val_s
{
    LsShmOffset_t    offdata;
} shmlru_val_t;

typedef struct shmlru_data_s
{
    uint32_t          magic;
    time_t            strttime;
    time_t            lasttime;
    int32_t           count;
    uint16_t          size;
    uint16_t          maxsize;
    union
    {
        LsShmOffset_t    offprev;       /* history */
        LsShmOffset_t    offiter;       /* or if no history, my iterator */
    };
    uint8_t           data[0];
} shmlru_data_t;

typedef struct shmlru_s
{
    lsi_shmhash_t    *phash;
    LsShmOffset_t     offhdr;
} shmlru_t;

int shmlru_init(shmlru_t *pShmLru,
                const char *pPoolName, size_t iPoolSize,
                const char *pHashName, size_t iHashSize, lsi_hash_fn fn, lsi_val_comp vc,
                int mode);
int shmlru_set(shmlru_t *pShmLru,
               const uint8_t *pKey, int keyLen, const uint8_t *pVal, int valLen);
int shmlru_get(shmlru_t *pShmLru,
               const uint8_t *pKey, int keyLen, lsi_shm_off_t *pData, int cnt);
int shmlru_getptrs(shmlru_t *pShmLru,
                   const uint8_t *pKey, int keyLen, int (*func)(void *pData));
int shmlru_check(shmlru_t *pShmLru);
int shmlru_trim(shmlru_t *pShmLru,
                time_t tmCutoff, int (*func)(iterator iter, void *arg), void *arg);

#ifdef __cplusplus
}
#endif

#endif /* LSSHMLRU_H */

