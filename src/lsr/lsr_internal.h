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
#ifndef LSR_INTERNAL_H
#define LSR_INTERNAL_H

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <lsr/lsr_types.h>


/**
 * @file
 */


#ifdef __cplusplus
extern "C" {
#endif 


typedef enum { RED, BLACK } lsr_map_color;

struct lsr_mapnode_s
{
    lsr_map_color   m_color;
    const void     *m_pKey;
    void           *m_pValue;
    struct lsr_mapnode_s  *m_left;
    struct lsr_mapnode_s  *m_right;
    struct lsr_mapnode_s  *m_parent;
};

struct lsr_hashelem_s
{
    struct lsr_hashelem_s   *m_pNext;
    const void              *m_pKey;
    void                    *m_pData;
    lsr_hash_key_t           m_hkey;
};

/**
 * @struct lsr_pool_header_s
 * @brief Global memory pool header.
 */
struct lsr_pool_header_s
{
    uint32_t m_size;
    uint32_t m_magic;
};

#define LSR_POOL_DATA_ALIGN              8

/**
 * @struct lsr_pool_blk_s
 * @brief Global memory pool block definition.
 */
struct lsr_pool_blk_s
{
    union
    {
        struct lsr_pool_blk_s     * m_pNext;
        struct lsr_pool_header_s    m_header;
        char m_align[LSR_POOL_DATA_ALIGN];
    };
};

/**
 * @struct lsr_xpool_header_s
 * @brief Session memory pool header.
 */
struct lsr_xpool_header_s
{
    uint32_t m_size;
    uint32_t m_magic;
    
};

/**
 * @struct lsr_xpool_bblk_s
 * @brief Session memory pool `big' block definition.
 */
struct lsr_xpool_bblk_s
{
    struct lsr_xpool_bblk_s   *m_pPrev;
    struct lsr_xpool_bblk_s   *m_pNext;
    struct lsr_xpool_header_s  m_header;
    char     m_cData[];
};

/**
 * @lsr_plistfree
 * @brief Frees (releases) a linked list of allocated memory blocks
 *   back to the global memory pool.
 * @details The list of memory blocks must have the same size,
 *   and must be linked as provided by the lsr_pool_blk_t structure.
 * 
 * @param[in] plist - A pointer to the linked list of allocated memory blocks.
 * @param[in] size - The size in bytes of each block.
 * @return Void.
 * @note This routine provides an efficient way to free a list of allocated
 *   blocks back to the global memory pool, such as in the case of releasing
 *   the \e superblocks from the session memory pool.
 */
void   lsr_plistfree( lsr_pool_blk_t *plist, size_t size );

/**
 * @lsr_psavepending
 * @brief Saves a linked list of allocated memory blocks
 *   to be freed to the global memory pool at a later time.
 * @details The list of memory blocks may have various sizes.
 *   They must be linked as provided by the lsr_xpool_bblk_t structure.
 * 
 * @param[in] plist - A pointer to the linked list of allocated memory blocks.
 * @return Void.
 * @note This routine provides a way to delay the freeing of allocated memory
 *   until a more appropriate or convenient time.
 *
 * @see lsr_pfreepending
 */
void   lsr_psavepending( lsr_xpool_bblk_t *plist );

/**
 * @lsr_pfreepending
 * @brief Frees (releases) the list of previously \e saved allocated
 *   memory blocks back to the global memory pool.
 * @details The linked list must have been created with previous
 *   successful call(s) to lsr_psavepending.

 * @return Void.
 *
 * @see lsr_psavepending
 */
void   lsr_pfreepending();

#ifdef __cplusplus
}
#endif 


#endif //LSR_INTERNAL_H
