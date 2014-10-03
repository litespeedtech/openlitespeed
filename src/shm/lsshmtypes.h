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
#ifndef LSSHMTYPES_H
#define LSSHMTYPES_H
/*
*   LsShm - LiteSpeed Shared memory Types...
*/
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include "lslock.h"


#ifdef __cplusplus
extern "C"
{
#endif

// #define DEBUG_RUN         /* dump a lot of msg and sleep */
// #define DEBUG_TEST_SYS      /* to exercise the system  */
// #define DEBUG_SHOW_MORE
#define LSSHM_DEBUG_ENABLE
// #define LSSHM_USE_SPINLOCK  /* enable for spinlock otherwise mutex */

/* C to C++ wrapper */
struct ls_shm_s {};
struct ls_shmpool_s {};
struct ls_shmhash_s {};
struct ls_shmobject_s {};
typedef struct ls_shm_s        lsi_shm_t;
typedef struct ls_shmpool_s    lsi_shmpool_t;
typedef struct ls_shmhash_s    lsi_shmhash_t;
typedef struct ls_shmobject_s  lsi_shmobject_t;

#ifdef LSSHM_USE_SPINLOCK
typedef lsr_spinlock_t          lsi_shmlock_t;
#define lsi_shmlock_setup       lsr_spinlock_setup         
#define lsi_shmlock_lock        lsr_spinlock_lock         
#define lsi_shmlock_trylock     lsr_spinlock_trylock       
#define lsi_shmlock_unlock      lsr_spinlock_unlock      
#else
typedef lsr_mutex_t             lsi_shmlock_t;
#define lsi_shmlock_setup       lsr_mutex_setup             
#define lsi_shmlock_lock        lsr_mutex_lock              
#define lsi_shmlock_trylock     lsr_mutex_trylock           
#define lsi_shmlock_unlock      lsr_mutex_unlock            
#endif


/* shared memory types */
typedef uint32_t                LsShm_offset_t ;
typedef uint32_t                LsShm_size_t ;

#define LSSHM_MAGIC             0x20140115   // 32 bits
#define LSSHM_LOCK_MAGIC        0x20140116   // 32 bits
#define LSSHM_HASH_MAGIC        0x20140117   // 32 bits
#define LSSHM_POOL_MAGIC        0x20140118   // 32 bits

#define LSSHM_VER_MAJOR         0x0     // 16 bits
#define LSSHM_VER_MINOR         0x0     // 8 bits
#define LSSHM_VER_REL           0x1     // 8 bits

#define LSSHM_PAGESIZE          0x2000  // min pagesize 8k
#define LSSHM_PAGEMASK          0xFFFFE000
#define LSSHM_MAXNAMELEN        12      // only 11 characters.

#define LSSHM_SYSSHM            "LsShm"     // default SHM name
#define LSSHM_SYSPOOL           "LsPool"    // default SHM POOL name
#define LSSHM_SYSHASH           "LsHash"    // default SHM HASH name

#define LSSHM_SYSSHM_DIR        "/dev/shm/LiteSpeed"
#define LSSHM_SYSSHM_FILENAME   LSSHM_SYSSHM
#define LSSHM_SYSSHM_FILE_EXT   "shm"
#define LSSHM_SYSLOCK_FILE_EXT  "lock"

#define LSSHM_INITSIZE          LSSHM_PAGESIZE   // default SHM SIZE

#define LSSHM_MINUNIT           0x400
#define LSSHM_MINHASH           0x400
#define LSSHM_MINLOCK           0x400
#define LSSHM_HASHINITSIZE      97

#define LSSHM_MINSPACE          0x2000          // minimum SHM SIZE

#define LSSHM_MAXSIZE           2000000000      // max size in memory allocation

/* bit mash from ls.h */
#define LSSHM_FLAG_CREATED      0x0001          /* return flag if SHM created */
#define LSSHM_FLAG_INIT         0x0001          /* used to indicate for memset */
#define LSSHM_FLAG_NONE         0x0000          /* used to indicate for memset */

#define LSSHM_CHECKSIZE(x) { if ((x&0x80000000) || (x>LSSHM_MAXSIZE)) return 0;}

enum LsShm_status_t {
                        LSSHM_ERROR = -100,
                        LSSHM_BADNOSPACE = -7,
                        LSSHM_BADMAXSPACE = -6,
                        LSSHM_NAMEINUSED = -5,
                        LSSHM_BADNAMELEN = -4,
                        LSSHM_BADMAPFILE = -3,
                        LSSHM_BADVERSION = -2,
                        LSSHM_BADPARAM = -1,
                        LSSHM_NOTREADY = 0,  
                        LSSHM_OK = 0, // Tricky - for internal function to use
                        LSSHM_READY = 1
                    };

enum LsShm_mode_t { LSSHM_MODE_RDONLY = 1, LSSHM_MODE_RW = 2 };

/* This will help to explain the key efficiency */
struct LsHash_stat_s
{
    int maxLink;        /* max number in same key */
    int numIdx;         /* num of index key */
    int numIdxOccupied; /* num of key populated */
    int num;            /* total elements */
    int numExpired;     /* user supplied */
    int numDup;         /* num of duplicated keys */
    int top[10];        /* 1,2,3,4,5,10,20,40,80,160*/
};
typedef struct LsHash_stat_s LsHash_stat_t;

#ifdef __cplusplus
}
#endif


#endif

