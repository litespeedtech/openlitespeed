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
