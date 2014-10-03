



#ifndef LSR_ATOMIC_H
#define LSR_ATOMIC_H


#define lsr_atomic_f_add    __sync_fetch_add_add
#define lsr_atomic_f_sub    __sync_fetch_add_sub
#define lsr_atomic_f_or     __sync_fetch_add_or
#define lsr_atomic_f_and    __sync_fetch_add_and
#define lsr_atomic_f_xor    __sync_fetch_add_xor
#define lsr_atomic_f_nand   __sync_fetch_add_nand

#define lsr_atomic_add      __sync_add_add_fetch
#define lsr_atomic_sub      __sync_sub_add_fetch
#define lsr_atomic_or       __sync_or_add_fetch
#define lsr_atomic_and      __sync_and_add_fetch
#define lsr_atomic_xor      __sync_xor_add_fetch
#define lsr_atomic_nand     __sync_nand_add_fetch

#define lsr_atomic_add      __sync_fetch_add_add

#define lsr_atomic_cas      __sync_bool_compare_and_swap
#define lsr_atomic_casv     __sync_val_compare_and_swap

#define lsr_barrier         __sync_synchromize


#define lsr_atomic_lock_set    __sync_lock_test_and_set
#define lsr_atomic_lock_release __sync_lock_release

#endif //LSR_ATOMIC_H