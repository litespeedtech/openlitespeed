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
#ifndef LS_LOCK_H
#define LS_LOCK_H

#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <lsr/ls_types.h>

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

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#define USE_ATOMIC_SPIN
#else
#define USE_MUTEX_LOCK
#endif

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__) \
    || defined(__FreeBSD__ ) || defined(__NetBSD__) || defined(__OpenBSD__)
#define USE_MUTEX_ADAPTIVE
#endif /* defined(linux) || defined(__FreeBSD__ ) */


#define MAX_SPIN_COUNT      10

/* LiteSpeed general purpose lock/unlock/trylock
 */
#ifdef USE_F_MUTEX
typedef int                 ls_mutex_t;
#else
typedef pthread_mutex_t     ls_mutex_t;
#endif

#if defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#include <libkern/OSAtomic.h>
typedef OSSpinLock ls_pspinlock_t;
#define ls_pspinlock_lock          OSSpinLockLock
#define ls_pspinlock_trylock       OSSpinLockTry
#define ls_pspinlock_unlock        OSSpinLockUnlock
#else
typedef pthread_spinlock_t ls_pspinlock_t;
#define ls_pspinlock_lock          pthread_spin_lock
#define ls_pspinlock_trylock       pthread_spin_trylock
#define ls_pspinlock_unlock        pthread_spin_unlock
#endif

#if defined(__i386__) || defined(__ia64__)
#define cpu_relax()         asm volatile("pause\n": : :"memory")
#else
#define cpu_relax()         ;
#endif
typedef int             ls_atom_spinlock_t;

#ifdef USE_ATOMIC_SPIN
typedef ls_atom_spinlock_t   ls_spinlock_t;
#else
typedef ls_pspinlock_t  ls_spinlock_t;
#endif

#ifdef USE_MUTEX_LOCK
typedef ls_mutex_t         ls_lock_t;
#else
typedef ls_spinlock_t      ls_lock_t;
#endif


#ifdef USE_F_MUTEX
#define ls_mutex_setup             ls_futex_setup
#define ls_mutex_lock              ls_futex_lock
#define ls_mutex_trylock           ls_futex_trylock
#define ls_mutex_unlock            ls_futex_unlock
#else
#define ls_mutex_setup             ls_pthread_mutex_setup
#define ls_mutex_lock              pthread_mutex_lock
#define ls_mutex_trylock           pthread_mutex_trylock
#define ls_mutex_unlock            pthread_mutex_unlock
#endif

#ifdef USE_ATOMIC_SPIN
#define ls_spinlock_setup         ls_atomic_spin_setup
#define ls_spinlock_lock          ls_atomic_spin_lock
#define ls_spinlock_trylock       ls_atomic_spin_trylock
#define ls_spinlock_unlock        ls_atomic_spin_unlock
#else
#define ls_spinlock_setup         ls_pspinlock_setup
#define ls_spinlock_lock          ls_pspinlock_lock
#define ls_spinlock_trylock       ls_pspinlock_trylock
#define ls_spinlock_unlock        ls_pspinlock_unlock
#endif

#ifdef USE_MUTEX_LOCK
#define ls_lock_setup             ls_mutex_setup
#define ls_lock_lock              ls_mutex_lock
#define ls_lock_trylock           ls_mutex_trylock
#define ls_lock_unlock            ls_mutex_unlock
#else
#define ls_lock_setup             ls_spinlock_setup
#define ls_lock_lock              ls_spinlock_lock
#define ls_lock_trylock           ls_spinlock_trylock
#define ls_lock_unlock            ls_spinlock_unlock
#endif

#define lock_Avail 0
#define lock_Inuse 456

#if defined(__FreeBSD__ ) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <errno.h>
#include <machine/atomic.h>
#include <sys/umtx.h>
ls_inline int ls_futex_wake(int *futex) 
{
    return _umtx_op(futex, UMTX_OP_WAKE, 1, 0, 0);
}

ls_inline int ls_futex_wait(int *futex, int val, struct timespec *timeout) 
{
    int err = _umtx_op(futex, UMTX_OP_WAIT_UINT, val, 0, (void *)timeout);
    return err;
}

#endif


#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

ls_inline int ls_futex_wake(int *futex) 
{
    return syscall(SYS_futex, futex, FUTEX_WAKE, 1, NULL, NULL, 0);
}


ls_inline int ls_futex_wait(int *futex, int val, struct timespec *timeout) 
{
    return syscall(SYS_futex, futex, FUTEX_WAIT, val, timeout, NULL, 0);
}

#endif


#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#ifdef USE_F_MUTEX
/**
 * @ls_futex_lock
 * @brief Locks
 *   a lock set up using futexes.
 *
 * @param[in] p - A pointer to the lock.
 * @return 0 when able to acquire the lock.
 *
 * @see ls_futex_setup, ls_futex_trylock, ls_futex_unlock
 */
ls_inline int ls_futex_lock(ls_mutex_t *p)
{
    struct timespec x_time = { 0, 100 }; // 100 nsec wait
    int val;

#if defined( USE_MUTEX_ADAPTIVE )
    int i;
    for (i = 0; i < MAX_SPIN_COUNT; ++i)
        if (__sync_bool_compare_and_swap(p, 0, 1))
            return 0;
#endif

    while (1)
    {
        if ((val = __sync_add_and_fetch(p, 1)) == 1)
            return 0;
        if ((val & 0x80000001) == 0x80000001)
            *p &= ~0x80000000;  /* do not let count wrap to zero */

        //syscall(SYS_futex, p, FUTEX_WAIT, *p, &x_time, NULL, 0);
        ls_futex_wait(p, *p, &x_time);

    }
}

/**
 * @ls_futex_trylock
 * @brief Tries to lock
 *   a lock set up using futexes.
 * @details The routine actually uses the built-in functions for
 *   atomic memory access.
 *
 * @param[in] p - A pointer to the lock.
 * @return 0 on success, else 1 if unable to acquire the lock.
 *
 * @see ls_futex_setup, ls_futex_lock, ls_futex_unlock
 */
ls_inline int ls_futex_trylock(ls_mutex_t *p)
{
    return !__sync_bool_compare_and_swap(p, 0, 1);
}

/**
 * @ls_futex_unlock
 * @brief Unlocks
 *   a lock set up using futexes.
 *
 * @param[in] p - A pointer to the lock.
 * @return 0, or 1 if there was a process waiting, else -1 on error.
 *
 * @see ls_futex_setup, ls_futex_lock, ls_futex_trylock
 */
ls_inline int ls_futex_unlock(ls_mutex_t *p)
{
    int *lp = p;

    assert(*lp != 0);
    if (__sync_lock_test_and_set(p, 0) == 1)
        return 0;
    
    //return syscall(SYS_futex, p, FUTEX_WAKE, 1, NULL, NULL, 0);
    return ls_futex_wake( p );
}

/**
 * @ls_futex_setup
 * @brief Initializes a locking mechanism
 *   using futexes (fast user-space mutexes).
 *
 * @param[in] p - A pointer to the lock.
 * @return 0.
 *
 * @see ls_futex_lock, ls_futex_unlock, ls_futex_trylock
 */
int ls_futex_setup(ls_mutex_t *p);

#endif //USE_F_MUTEX
#endif

#ifdef USE_ATOMIC_SPIN

/**
 * @ls_atomic_spin_lock
 * @brief Locks
 *   a spinlock set up with built-in functions for atomic memory access.
 * @details The routine \e spins until it is able to acquire the lock.
 *
 * @param[in] p - A pointer to the lock.
 * @return 0 when able to acquire the lock.
 *
 * @see ls_atomic_spin_setup, ls_atomic_spin_trylock, ls_atomic_spin_unlock
 */
ls_inline int ls_atomic_spin_lock(ls_atom_spinlock_t *p)
{
    while (1)
    {
        if ((*p == lock_Avail)
            && __sync_bool_compare_and_swap(p, lock_Avail, lock_Inuse))
            return 0;
        cpu_relax();
    }
}

/**
 * @ls_atomic_spin_trylock
 * @brief Tries to lock
 *   a spinlock set up with built-in functions for atomic memory access.
 *
 * @param[in] p - A pointer to the lock.
 * @return 0 on success, else 1 if unable to acquire the lock.
 *
 * @see ls_atomic_spin_setup, ls_atomic_spin_lock, ls_atomic_spin_unlock
 */
ls_inline int ls_atomic_spin_trylock(ls_atom_spinlock_t *p)
{
    return !__sync_bool_compare_and_swap(p, lock_Avail, lock_Inuse);
}

/**
 * @ls_atomic_spin_unlock
 * @brief Unlocks
 *   a spinlock set up with built-in functions for atomic memory access.
 *
 * @param[in] p - A pointer to the lock.
 * @return 0.
 *
 * @see ls_atomic_spin_setup, ls_atomic_spin_lock, ls_atomic_spin_trylock
 */
ls_inline int ls_atomic_spin_unlock(ls_atom_spinlock_t *p)
{
    __sync_lock_release(p);
    return 0;
}

/**
 * @ls_atomic_spin_setup
 * @brief Initializes a locking mechanism
 *   using spinlocks with built-in functions for atomic memory access.
 *
 * @param[in] p - A pointer to the lock.
 * @return 0.
 *
 * @see ls_atomic_spin_lock, ls_atomic_spin_unlock, ls_atomic_spin_trylock
 */
int ls_atomic_spin_setup(ls_atom_spinlock_t *p);

#endif

int ls_pthread_mutex_setup(pthread_mutex_t *);

int ls_pspinlock_setup(ls_pspinlock_t *p);

#ifdef __cplusplus
}
#endif

#endif //LS_LOCK_H

