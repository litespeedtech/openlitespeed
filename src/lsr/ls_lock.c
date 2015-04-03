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
#include <lsr/ls_lock.h>

#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <lsdef.h>

/*
 *   futex
 */

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

int ls_futex_setup(ls_mutex_t *p)
{
#if 0
    int *lp = (int *)p;
    assert(*lp != LS_LOCK_INUSE);
#endif

    *((int *)p) = LS_LOCK_AVAIL;
    return 0;
}

#endif


/*
 *   atomic spin
 */

#ifdef USE_ATOMIC_SPIN
int ls_atomic_spin_setup(ls_atom_spinlock_t *p)
{
    *((int *)p) = LS_LOCK_AVAIL;
    return 0;
}


int ls_spin_pid = 0;        /* process id used with ls_atomic_pidspin */
static void child_func()    /* clear pid in child after fork() */
{
    ls_spin_pid = 0;
}


void ls_atomic_pidspin_init()
{
    ls_spin_pid = getpid();
    pthread_atfork(NULL, NULL, child_func);
}
#endif

/*
 *   pthread
 */
int ls_pthread_mutex_setup(pthread_mutex_t *p)
{
    pthread_mutexattr_t myAttr;
    pthread_mutexattr_init(&myAttr);
#if defined(USE_MUTEX_ADAPTIVE)
    pthread_mutexattr_settype(&myAttr, PTHREAD_MUTEX_ADAPTIVE_NP);
#else  /* defined(USE_MUTEX_ADAPTIVE) */
    /* pthread_mutexattr_settype(&myAttr, PTHREAD_MUTEX_NORMAL); */
    pthread_mutexattr_settype(&myAttr,
#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
                              PTHREAD_MUTEX_ERRORCHECK_NP
#else  /* defined(linux) */
                              PTHREAD_MUTEX_ERRORCHECK
#endif  /* defined(linux) */
                             );
#endif  /* defined(USE_MUTEX_ADAPTIVE) */
    /* pthread_mutexattr_settype(&myAttr, PTHREAD_MUTEX_RECURSIVE); */
    /* pthread_mutexattr_settype(&myAttr, PTHREAD_MUTEX_RECURSIVE_NP); */

    pthread_mutexattr_setpshared(&myAttr, PTHREAD_PROCESS_SHARED);

    int code = pthread_mutex_init((pthread_mutex_t *)p, &myAttr);
    if ((!code) || (code == EBUSY))
    {
        // already inited... ok..
        return 0;
    }
    return LS_FAIL;
}


/*
 * pthread spinlock
 */
int ls_pspinlock_setup(ls_pspinlock_t   *p)
{
    int code;
#if defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    *p = OS_SPINLOCK_INIT;
    code = 0;
#else
    code = pthread_spin_init((pthread_spinlock_t *)p,
                             PTHREAD_PROCESS_SHARED
                             /*  PTHREAD_PROCESS_PRIVATE */
                            );
#endif
    if ((!code) || (code == EBUSY))
    {
        // already inited... ok..
        return 0;
    }
    return LS_FAIL;
}

