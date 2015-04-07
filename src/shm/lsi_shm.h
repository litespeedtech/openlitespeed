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
#ifndef LSI_SHM_H
#define LSI_SHM_H
#include <stdint.h>

#include <shm/lsshmtypes.h>

/**
 * @file
 * LiteSpeed Shared Memory Allocation, Hash and Lock
 */


#ifdef __cplusplus
extern "C" {
#endif

/*
 *   Shared memory handle
 */
typedef LsShmOffset_t      lsi_shm_key_t;        /* SHM offset    */
typedef LsShmOffset_t      lsi_shmhash_datakey_t;
typedef LsShmOffset_t      lsi_shm_off_t;

typedef struct lsShm_hElem_s    *iterator;

/* USER Hash key generator and compare functions */
typedef uint32_t (*lsi_hash_fn)(const void *p, int len);
typedef int (*lsi_val_comp)(const void *p1, const void *p2, int len);

/*
 *    SHM CONTAINER -----------> SHM LOCK
 *  +-----------------------+   +---------+
 *  | SHM Pool1 (allocator) |   | Lock1   |
 *  |    SHM USER Memory1   |   | Lock2   |
 *  |    SHM Hash Table1    |   |  ...    |
 *  |-----------------------+   +---------+
 *  | SHM Pool2 (allocator) |
 *  |    SHM HASH Table1    |
 *  |    SHM HASH Table2    |
 *  |    SHM USER Memory2   |
 *  |       ...             |
 *  |-----------------------+
 *  | SHM Pool3 (allocator) |
 *  |    SHM HASH Table3    |
 *  |    SHM HASH Table4    |
 *  |    SHM HASH Table5    |
 *  |       ...             |
 *  +-----------------------+
 */

/*
 *   LiteSpeed SHM memory container
 */
lsi_shm_t      *lsi_shm_open(const char *shmname, size_t initialsize);
int             lsi_shm_close(lsi_shm_t
                              *shmhandle);    /* close connection */
int             lsi_shm_destroy(lsi_shm_t
                                *shmhandle);  /* remove hash map */

/*
 *   SHM memory allocator
 */
lsi_shmpool_t *lsi_shmpool_open(lsi_shm_t *shmhandle,
                                const char *poolname);
lsi_shmpool_t *lsi_shmpool_openbyname(const char *shmname,
                                      size_t initialsize);
int             lsi_shmpool_close(lsi_shmpool_t *poolhandle);
int             lsi_shmpool_destroy(lsi_shmpool_t *poolhandle);

lsi_shm_off_t   lsi_shmpool_alloc2(
    lsi_shmpool_t *poolhandle, size_t size);
void            lsi_shmpool_release2(
    lsi_shmpool_t *poolhandle, lsi_shm_off_t key, size_t size);
uint8_t        *lsi_shmpool_key2ptr(
    lsi_shmpool_t *poolhandle, lsi_shm_off_t key);
lsi_shm_off_t   lsi_shmpool_getreg(
    lsi_shmpool_t *poolhandle, const char *name);
int             lsi_shmpool_setreg(
    lsi_shmpool_t *poolhandle, const char *name, lsi_shm_off_t off);

/*
 *    SHM HASH
 * insert  = Insert without check
 * find    = Search for it
 * update  = Update the hash element
 * remove  = Remove the hash element
 *
 * Hash open/close lsi_shmhash_data_t
 */
lsi_shmhash_t *lsi_shmhash_open(lsi_shmpool_t *poolhandle,
                                const char *hash_table_name,
                                size_t initialsize,
                                lsi_hash_fn hf,
                                lsi_val_comp vc);
lsi_shmhash_t *lsi_shmlruhash_open(lsi_shmpool_t *poolhandle,
                                   const char *hash_table_name,
                                   size_t initialsize,
                                   lsi_hash_fn hf,
                                   lsi_val_comp vc,
                                   int mode);
int             lsi_shmhash_close(lsi_shmhash_t *hashhandle);
int             lsi_shmhash_destroy(lsi_shmhash_t *hashhandle);

/* Hash Shared memory access */
lsi_shm_off_t   lsi_shmhash_hdroff(lsi_shmhash_t *hashhandle);
lsi_shm_key_t   lsi_shmhash_alloc2(lsi_shmhash_t *hashhandle,
                                   size_t size);
void            lsi_shmhash_release2(lsi_shmhash_t *hashhandle,
                                     lsi_shm_key_t key,
                                     size_t size);
uint8_t        *lsi_shmhash_key2ptr(lsi_shmhash_t *hashhandle,
                                    lsi_shm_key_t);

/* Hash Element memory access */
uint8_t        *lsi_shmhash_datakey2ptr(lsi_shmhash_t *hashhandle,
                                        lsi_shm_off_t);
lsi_shm_off_t   lsi_shmhash_find(lsi_shmhash_t *hashhandle,
                                 const uint8_t *key, int keylen, int *valuesize);
lsi_shm_off_t   lsi_shmhash_get(lsi_shmhash_t *hashhandle,
                                const uint8_t *key, int keylen, int *valuesize, int *pflag);
lsi_shm_off_t   lsi_shmhash_set(lsi_shmhash_t *hashhandle,
                                const uint8_t *key, int len,
                                const uint8_t *value, int valuelen);
lsi_shm_off_t   lsi_shmhash_insert(lsi_shmhash_t *hashhandle,
                                   const uint8_t *key, int len,
                                   const uint8_t *value, int valuelen);
lsi_shm_off_t   lsi_shmhash_update(lsi_shmhash_t *hashhandle,
                                   const uint8_t *key, int len,
                                   const uint8_t *value, int valuelen);
void            lsi_shmhash_remove(lsi_shmhash_t *hashhandle,
                                   const uint8_t *key, int len);
void            lsi_shmhash_clear(lsi_shmhash_t *hashhandle);
int             lsi_shmhash_setdata(lsi_shmhash_t *hashhandle,
                                    LsShmOffset_t offVal, const uint8_t *value, int valuelen);
int             lsi_shmhash_getdata(lsi_shmhash_t *hashhandle,
                                    LsShmOffset_t offVal, LsShmOffset_t *pvalue, int cnt);
int             lsi_shmhash_getdataptrs(lsi_shmhash_t *hashhandle,
                                        LsShmOffset_t offVal, int (*func)(void *pData));
int             lsi_shmhash_trim(lsi_shmhash_t *hashhandle,
                                 time_t tmcutoff, int (*func)(iterator iter, void *arg), void *arg);
int             lsi_shmhash_check(lsi_shmhash_t *hashhandle);
int             lsi_shmhash_lock(lsi_shmhash_t *hashhandle);
int             lsi_shmhash_unlock(lsi_shmhash_t *hashhandle);


/*
 * Hash table statistics
 */
int             lsi_shmhash_stat(lsi_shmhash_t *hashhandle,
                                 LsHashStat *phashstat);

/*  LOCK related
 *  All shared memory pool come with lock.
 *  get    = return first available lock
 *  remove = return the given lock
 */
lsi_shmlock_t *lsi_shmlock_get(lsi_shm_t *handle);
int             lsi_shmlock_remove(lsi_shm_t *handle, lsi_shmlock_t *lock);


#ifdef __cplusplus
}
#endif

#endif
