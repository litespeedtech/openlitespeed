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
#ifndef LSR_TYPES_H
#define LSR_TYPES_H

/**
 * @file
 */

#ifdef __cplusplus
extern "C" {
#endif 

#define lsr_inline      static inline

typedef unsigned long           lsr_hash_key_t;
typedef struct lsr_ptrlist_s    lsr_ptrlist_t;
typedef lsr_ptrlist_t           lsr_strlist_t;
/**
 * @addtogroup LSR_STR_GROUP
 * @{
 */
typedef struct lsr_str_s        lsr_str_t;
typedef struct lsr_str_pair_s   lsr_str_pair_t;
/**
 * @}
 */
typedef struct lsr_xpool_s      lsr_xpool_t;


#ifdef __cplusplus
}
#endif 

#endif //LSR_TYPES_H
