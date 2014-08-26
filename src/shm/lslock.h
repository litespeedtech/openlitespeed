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
#ifndef LSLOCK_H
#define LSLOCK_H
/*
*   LsShmLock - LiteSpeed Shared memory Lock
*/
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#include <linux/futex.h>
#include <sys/syscall.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  USE_F_MUTEX - support futex instead of pthread_mutex
 *  USE_ATOMIC_SPIN -  support atomic Spin instead of pthread_spin_lock
 */
#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#define USE_F_MUTEX
#else
#undef USE_F_MUTEX
#endif

#define USE_ATOMIC_SPIN

#ifdef PTHREAD_MUTEX_ADAPTIVE_NP
#define USE_MUTEX_ADAPTIVE
#endif


#define MAX_SPIN_COUNT      10

/* LiteSpeed general purpose lock/unlock/trylock
*/
#ifdef USE_F_MUTEX
typedef int                 lsi_mutex_t;
#else
typedef pthread_mutex_t     lsi_mutex_t;
#endif

#ifdef USE_ATOMIC_SPIN
typedef int                 lsi_spinlock_t;
#else
typedef pthread_spinlock_t  lsi_spinlock_t;
#endif

#ifdef USE_MUTEX_LOCK
typedef lsi_mutex_t         lsi_lock_t;
#else
typedef lsi_spinlock_t      lsi_lock_t;
#endif 


#ifdef USE_F_MUTEX
#define lsi_mutex_setup             lsi_futex_setup
#define lsi_mutex_lock              lsi_futex_lock
#define lsi_mutex_trylock           lsi_futex_trylock
#define lsi_mutex_unlock            lsi_futex_unlock
#else
#define lsi_mutex_setup             lsi_pthread_mutex_setup
#define lsi_mutex_lock              pthread_mutex_lock    
#define lsi_mutex_trylock           pthread_mutex_trylock 
#define lsi_mutex_unlock            pthread_mutex_unlock  
#endif

#ifdef USE_ATOMIC_SPIN
#define lsi_spinlock_setup         lsi_atomic_spin_setup
#define lsi_spinlock_lock          lsi_atomic_spin_lock
#define lsi_spinlock_trylock       lsi_atomic_spin_trylock
#define lsi_spinlock_unlock        lsi_atomic_spin_unlock
#else
#define lsi_spinlock_setup         lsi_pthread_spin_setup
#define lsi_spinlock_lock          pthread_spin_lock
#define lsi_spinlock_trylock       pthread_spin_trylock
#define lsi_spinlock_unlock        pthread_spin_unlock
#endif

#ifdef USE_MUTEX_LOCK
#define lsi_lock_setup             lsi_mutex_setup             
#define lsi_lock_lock              lsi_mutex_lock              
#define lsi_lock_trylock           lsi_mutex_trylock           
#define lsi_lock_unlock            lsi_mutex_unlock            
#else
#define lsi_lock_setup             lsi_spinlock_setup         
#define lsi_lock_lock              lsi_spinlock_lock         
#define lsi_lock_trylock           lsi_spinlock_trylock       
#define lsi_lock_unlock            lsi_spinlock_unlock      
#endif 

#define lock_Avail 123
#define lock_Inuse 456

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#ifdef USE_F_MUTEX
static inline int lsi_futex_lock( lsi_mutex_t * p )
{
    struct timespec x_time = { 0, 100}; // 100 nsec wait
    
    while (1)
    {
#if defined( USE_MUTEX_ADAPTIVE )
        register int i;
        for( i =0; i < MAX_SPIN_COUNT; ++i )
            if ( __sync_bool_compare_and_swap( p, lock_Avail, lock_Inuse ) )
                return 0;
        
#else
        if ( __sync_bool_compare_and_swap( p, lock_Avail, lock_Inuse ) )
            return 0;
#endif
        syscall ( SYS_futex, p, FUTEX_WAIT, lock_Inuse, &x_time, NULL, 0 );
    }
}

static inline int lsi_futex_trylock( lsi_mutex_t * p )
{
    return ( __sync_bool_compare_and_swap( p, lock_Avail, lock_Inuse ) ) ? 0 : -1;
}

static inline int lsi_futex_unlock( lsi_mutex_t * p )
{
    // register int * lp = p;
    assert (*p == lock_Inuse);
        
    *p = lock_Avail;
    return syscall ( SYS_futex, p, FUTEX_WAKE, 1, NULL, NULL, 0);
}

int lsi_futex_setup(lsi_mutex_t *);
#endif //USE_F_MUTEX
#endif

int lsi_pthread_mutex_setup(lsi_mutex_t *);

static inline int lsi_atomic_spin_lock( lsi_spinlock_t * p )
{
    while (1)
    {
        if ( __sync_bool_compare_and_swap( p, lock_Avail, lock_Inuse ) )
            return 0;
    }
}

// SAME AS THE lsi_futex_trylock
static inline int lsi_atomic_spin_trylock( lsi_spinlock_t * p )
{
    return ( __sync_bool_compare_and_swap( p, lock_Avail, lock_Inuse ) )?0:-1;
}

static inline int lsi_atomic_spin_unlock( lsi_spinlock_t * p )
{
    *p = lock_Avail;
    return 0;
}

int lsi_atomic_spin_setup(lsi_spinlock_t *);
int lsi_pthread_spin_setup(lsi_spinlock_t *);
    
#ifdef __cplusplus
}
#endif

#endif

