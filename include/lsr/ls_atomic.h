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

#ifndef LS_ATOMIC_H
#define LS_ATOMIC_H

#include <inttypes.h>

/**
 * @file
 */


typedef volatile int32_t ls_atom_32_t;
typedef volatile int64_t ls_atom_64_t;
typedef volatile int     ls_atom_int_t;
typedef volatile long    ls_atom_long_t;

typedef volatile uint32_t ls_atom_u32_t;
typedef volatile uint64_t ls_atom_u64_t;
typedef volatile unsigned int    ls_atom_uint_t;
typedef volatile unsigned long   ls_atom_ulong_t;

typedef union
{
    struct
    {
        void *m_ptr;
        long   m_seq;
    };
#if defined( __i386__ )
    uint64_t   m_whole;
#elif defined( __x86_64 )||defined( __x86_64__ )
#if 0
    unsigned __int128 m_whole;
#else
    uint8_t    m_whole[16];
#endif
#endif
} __attribute__((__aligned__(
#if defined( __i386__ )
                     8
#elif defined( __x86_64 )||defined( __x86_64__ )
                     16
#endif
                 ))) ls_atom_ptr_t;


static inline int32_t ls_atomic_add(ls_atom_32_t *ptr, int32_t val)
{   return __sync_add_and_fetch(ptr, val);  }

static inline int32_t ls_atomic_sub(ls_atom_32_t *ptr, int32_t val)
{   return __sync_sub_and_fetch(ptr, val);  }

static inline int32_t ls_atomic_set(ls_atom_32_t *ptr, int32_t val)
{   return __sync_lock_test_and_set(ptr, val);  }

static inline void ls_atomic_clr(ls_atom_32_t *p)
{   __sync_lock_release(p);  }

static inline uint8_t ls_atomic_cas(ls_atom_32_t *ptr, int32_t oldval, int32_t newval)
{   return __sync_bool_compare_and_swap(ptr, oldval, newval);  }

static inline int32_t ls_atomic_casv(ls_atom_32_t *ptr, int32_t oldval, int32_t newval)
{   return __sync_val_compare_and_swap(ptr, oldval, newval);  }

static inline void *ls_atomic_setptr(void **ptr, void *val)
{   return __sync_lock_test_and_set(ptr, val);  }

static inline void ls_atomic_clrptr(void **p)
{   __sync_lock_release(p);  }

static inline uint8_t ls_atomic_casptr(void **ptr, void *oldval, void *newval)
{   return __sync_bool_compare_and_swap(ptr, oldval, newval);  }

static inline void *ls_atomic_casvptr(void **ptr, void *oldval, void *newval)
{   return __sync_val_compare_and_swap(ptr, oldval, newval);  }

static inline void ls_barrier()
{   __sync_synchronize();  }


#define LSR_ATOMIC_LOAD( v, px )  {   v = *px; ls_barrier();     }
#define LSR_ATOMIC_STORE( pv, x ) {   ls_barrier(); *pv = x;     }


#if defined( __i386__ )
static inline char ls_atomic_dcas(ls_atom_ptr_t *ptr,
                                  ls_atom_ptr_t *cmpptr, ls_atom_ptr_t *newptr)
{
    return __sync_bool_compare_and_swap(
               &ptr->m_whole, cmpptr->m_whole, newptr->m_whole);
}


static inline void ls_atomic_dcasv(ls_atom_ptr_t *ptr,
                                   ls_atom_ptr_t *cmpptr, ls_atom_ptr_t *newptr, ls_atom_ptr_t *oldptr)
{
    oldptr->m_whole = __sync_val_compare_and_swap(
                          &ptr->m_whole, cmpptr->m_whole, newptr->m_whole);
    return;
}


#elif defined( __x86_64 )||defined( __x86_64__ )
static inline char ls_atomic_dcas(volatile ls_atom_ptr_t *ptr,
                                  ls_atom_ptr_t *cmpptr, ls_atom_ptr_t *newptr)
{
    char result;
    __asm__ __volatile__(
        "lock cmpxchg16b %1\n\t"
        "setz %0\n"
        : "=q"(result)
        , "+m"(*ptr)
        : "a"(cmpptr->m_ptr), "d"(cmpptr->m_seq)
        , "b"(newptr->m_ptr), "c"(newptr->m_seq)
        : "cc"
    );
    return result;

}


static inline void ls_atomic_dcasv(volatile ls_atom_ptr_t *ptr,
                                   ls_atom_ptr_t *cmpptr, ls_atom_ptr_t *newptr, ls_atom_ptr_t *oldptr)
{
    __asm__ __volatile__(
        "lock cmpxchg16b %0\n\t"
        : "+m"(*ptr)
        , "=a"(oldptr->m_ptr)
        , "=d"(oldptr->m_seq)
        : "a"(cmpptr->m_ptr), "d"(cmpptr->m_seq)
        , "b"(newptr->m_ptr), "c"(newptr->m_seq)
        : "cc"
    );
    return;

}
#endif

#endif //LS_ATOMIC_H

