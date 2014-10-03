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
#ifndef LSR_LOCK_H
#define LSR_LOCK_H

#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <lsr/lsr_types.h>

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#include <linux/futex.h>
#include <sys/syscall.h>
#endif


/**
 * @file
 * There are a number of locking mechanisms available to the user,
 * selectable through conditional compilation.
 *
 * \li USE_F_MUTEX - Support futex (fast user-space mutex) instead of pthread_mutex.
 * \li USE_ATOMIC_SPIN - Support atomic spin instead of pthread_spin_lock.
 * \li USE_MUTEX_LOCK - Use mutex/futex locks rather than spin locks.
 *
 * Note that certain hardware and operating systems handle implementation differently.
 */


#ifdef __cplusplus
extern "C" {
#endif

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#define USE_F_MUTEX
#else
#undef USE_F_MUTEX
#endif

#define USE_ATOMIC_SPIN
//#define USE_MUTEX_LOCK

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__) \
    || defined(__FreeBSD__ ) || defined(__NetBSD__) || defined(__OpenBSD__)
#define USE_MUTEX_ADAPTIVE
#endif /* defined(linux) || defined(__FreeBSD__ ) */


#define MAX_SPIN_COUNT      10

/* LiteSpeed general purpose lock/unlock/trylock
 */
#ifdef USE_F_MUTEX
typedef int                 lsr_mutex_t;
#else
typedef pthread_mutex_t     lsr_mutex_t;
#endif

#if defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#include <libkern/OSAtomic.h>
typedef OSSpinLock lsr_pspinlock_t;
#define lsr_pspinlock_lock          OSSpinLockLock
#define lsr_pspinlock_trylock       OSSpinLockTry
#define lsr_pspinlock_unlock        OSSpinLockUnlock
#else
typedef pthread_spinlock_t lsr_pspinlock_t;
#define lsr_pspinlock_lock          pthread_spin_lock
#define lsr_pspinlock_trylock       pthread_spin_trylock
#define lsr_pspinlock_unlock        pthread_spin_unlock
#endif

#ifdef USE_ATOMIC_SPIN
#if defined(__i386__) || defined(__ia64__)
#define cpu_relax()         asm volatile("pause\n": : :"memory")
#else
#define cpu_relax()         ;
#endif
typedef int                 lsr_spinlock_t;
#else
typedef lsr_pspinlock_t  lsr_spinlock_t;
#endif

#ifdef USE_MUTEX_LOCK
typedef lsr_mutex_t         lsr_lock_t;
#else
typedef lsr_spinlock_t      lsr_lock_t;
#endif 


#ifdef USE_F_MUTEX
#define lsr_mutex_setup             lsr_futex_setup
#define lsr_mutex_lock              lsr_futex_lock
#define lsr_mutex_trylock           lsr_futex_trylock
#define lsr_mutex_unlock            lsr_futex_unlock
#else
#define lsr_mutex_setup             lsr_pthread_mutex_setup
#define lsr_mutex_lock              pthread_mutex_lock    
#define lsr_mutex_trylock           pthread_mutex_trylock 
#define lsr_mutex_unlock            pthread_mutex_unlock  
#endif

#ifdef USE_ATOMIC_SPIN
#define lsr_spinlock_setup         lsr_atomic_spin_setup
#define lsr_spinlock_lock          lsr_atomic_spin_lock
#define lsr_spinlock_trylock       lsr_atomic_spin_trylock
#define lsr_spinlock_unlock        lsr_atomic_spin_unlock
#else
#define lsr_spinlock_setup         lsr_pspinlock_setup
#define lsr_spinlock_lock          lsr_pspinlock_lock
#define lsr_spinlock_trylock       lsr_pspinlock_trylock
#define lsr_spinlock_unlock        lsr_pspinlock_unlock
#endif

#ifdef USE_MUTEX_LOCK
#define lsr_lock_setup             lsr_mutex_setup             
#define lsr_lock_lock              lsr_mutex_lock              
#define lsr_lock_trylock           lsr_mutex_trylock           
#define lsr_lock_unlock            lsr_mutex_unlock            
#else
#define lsr_lock_setup             lsr_spinlock_setup         
#define lsr_lock_lock              lsr_spinlock_lock         
#define lsr_lock_trylock           lsr_spinlock_trylock       
#define lsr_lock_unlock            lsr_spinlock_unlock      
#endif 

#define lock_Avail 0
#define lock_Inuse 456

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#ifdef USE_F_MUTEX
/**
 * @lsr_futex_lock
 * @brief Locks
 *   a lock set up using futexes.
 * 
 * @param[in] p - A pointer to the lock.
 * @return 0 when able to acquire the lock.
 *
 * @see lsr_futex_setup, lsr_futex_trylock, lsr_futex_unlock
 */
lsr_inline int lsr_futex_lock( lsr_mutex_t * p )
{
    struct timespec x_time = { 0, 100 }; // 100 nsec wait
    int val;
    
#if defined( USE_MUTEX_ADAPTIVE )
    register int i;
    for( i =0; i < MAX_SPIN_COUNT; ++i )
        if ( __sync_bool_compare_and_swap( p, 0, 1 ) )
            return 0;
#endif
 
    while ( 1 )
    {
        if ( (val = __sync_add_and_fetch( p, 1 )) == 1 )
            return 0;
        if ( (val & 0x80000001) == 0x80000001 )
            *p &= ~0x80000000;  /* do not let count wrap to zero */

        syscall( SYS_futex, p, FUTEX_WAIT, *p, &x_time, NULL, 0 );
    }
}

/**
 * @lsr_futex_trylock
 * @brief Tries to lock
 *   a lock set up using futexes.
 * @details The routine actually uses the built-in functions for
 *   atomic memory access.
 * 
 * @param[in] p - A pointer to the lock.
 * @return 0 on success, else -1 if unable to acquire the lock.
 *
 * @see lsr_futex_setup, lsr_futex_lock, lsr_futex_unlock
 */
lsr_inline int lsr_futex_trylock( lsr_mutex_t * p )
{
    return ( __sync_bool_compare_and_swap( p, 0, 1 ) ) ? 0 : -1;
}

/**
 * @lsr_futex_unlock
 * @brief Unlocks
 *   a lock set up using futexes.
 * 
 * @param[in] p - A pointer to the lock.
 * @return 0, or 1 if there was a process waiting, else -1 on error.
 *
 * @see lsr_futex_setup, lsr_futex_lock, lsr_futex_trylock
 */
lsr_inline int lsr_futex_unlock( lsr_mutex_t * p )
{
    register int * lp = p;

    assert (*lp != 0);
    if ( __sync_lock_test_and_set( p, 0 ) == 1 )
        return 0;
    return syscall ( SYS_futex, p, FUTEX_WAKE, 1, NULL, NULL, 0 );
}

/**
 * @lsr_futex_setup
 * @brief Initializes a locking mechanism
 *   using futexes (fast user-space mutexes).
 * 
 * @param[in] p - A pointer to the lock.
 * @return 0.
 *
 * @see lsr_futex_lock, lsr_futex_unlock, lsr_futex_trylock
 */
int lsr_futex_setup( lsr_mutex_t * p );

#endif //USE_F_MUTEX
#endif

/**
 * @lsr_atomic_spin_lock
 * @brief Locks
 *   a spinlock set up with built-in functions for atomic memory access.
 * @details The routine \e spins until it is able to acquire the lock.
 * 
 * @param[in] p - A pointer to the lock.
 * @return 0 when able to acquire the lock.
 *
 * @see lsr_atomic_spin_setup, lsr_atomic_spin_trylock, lsr_atomic_spin_unlock
 */
lsr_inline int lsr_atomic_spin_lock( lsr_spinlock_t * p )
{
    while (1)
    {
        if ( ( *p == lock_Avail )
          && __sync_bool_compare_and_swap( p, lock_Avail, lock_Inuse ) )
            return 0;
        cpu_relax();
    }
}

/**
 * @lsr_atomic_spin_trylock
 * @brief Tries to lock
 *   a spinlock set up with built-in functions for atomic memory access.
 * 
 * @param[in] p - A pointer to the lock.
 * @return 0 on success, else -1 if unable to acquire the lock.
 *
 * @see lsr_atomic_spin_setup, lsr_atomic_spin_lock, lsr_atomic_spin_unlock
 */
lsr_inline int lsr_atomic_spin_trylock( lsr_spinlock_t * p )
{
    return ( __sync_bool_compare_and_swap( p, lock_Avail, lock_Inuse ) )? 0: -1;
}

/**
 * @lsr_atomic_spin_unlock
 * @brief Unlocks
 *   a spinlock set up with built-in functions for atomic memory access.
 * 
 * @param[in] p - A pointer to the lock.
 * @return 0.
 *
 * @see lsr_atomic_spin_setup, lsr_atomic_spin_lock, lsr_atomic_spin_trylock
 */
lsr_inline int lsr_atomic_spin_unlock( lsr_spinlock_t * p )
{
    __sync_lock_release( p );
    return 0;
}

/**
 * @lsr_atomic_spin_setup
 * @brief Initializes a locking mechanism
 *   using spinlocks with built-in functions for atomic memory access.
 * 
 * @param[in] p - A pointer to the lock.
 * @return 0.
 *
 * @see lsr_atomic_spin_lock, lsr_atomic_spin_unlock, lsr_atomic_spin_trylock
 */
int lsr_atomic_spin_setup( lsr_spinlock_t * p );

int lsr_pthread_mutex_setup( pthread_mutex_t * );

int lsr_pspinlock_setup( lsr_pspinlock_t * p );
    
#ifdef __cplusplus
}
#endif

#endif //LSR_LOCK_H

