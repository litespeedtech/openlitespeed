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
#ifndef LSR_XPOOL_H
#define LSR_XPOOL_H

/* define to handle all allocs/frees as separate `big' blocks */
//#define LSR_VG_DEBUG

/* define for miscellaneous debug status */
//#define LSR_XPOOL_INTERNAL_DEBUG

#include <inttypes.h>
#include <lsr/lsr_types.h>
#include <lsr/lsr_pool.h>
#ifdef _REENTRANT
#include <lsr/lsr_lock.h>
#endif


/**
 * @file
 */


#ifdef __cplusplus
extern "C" {
#endif

typedef struct lsr_xpool_sb_s       lsr_xpool_sb_t;
typedef struct lsr_xpool_header_s   lsr_xpool_header_t;
typedef struct xpool_alloclink_s    xpool_alloclink_t;
typedef struct xpool_qlist_s        xpool_qlist_t;

/**
 * @struct lsr_qlist_s
 * @brief Session memory pool list management.
 */
struct xpool_qlist_s
{
    xpool_alloclink_t *m_pPtr;
#ifdef _REENTRANT
    lsr_spinlock_t     m_lock;
#endif
};

/**
 * @struct lsr_xpool_s
 * @brief Session memory pool top level management.
 */
struct lsr_xpool_s
{
    lsr_pool_blk_t    *m_pSB;
    xpool_qlist_t      m_pSmBlk;
    xpool_qlist_t      m_pLgBlk;
    lsr_xpool_bblk_t  *m_pBblk;
    xpool_qlist_t     *m_pFreeLists;
    int                m_flag; 
#ifdef _REENTRANT
    lsr_spinlock_t     m_lock;
    lsr_spinlock_t     m_initlock;
#endif
};

/**
 * @lsr_xpool_new
 * @brief Creates a new session memory pool object.
 * @details The routine allocates and initializes an object
 *   to manage a memory pool which is expected
 *   to exist only for the life of a session, at which time it
 *   may be efficiently cleaned up.
 * 
 * @return A pointer to an initialized session memory pool object.
 *
 * @see lsr_xpool_delete
 */
lsr_xpool_t * lsr_xpool_new();

/**
 * @lsr_xpool_init
 * @brief Initializes a session memory pool object.
 * @details This object manages a memory pool which is expected
 *   to exist only for the life of a session, at which time it
 *   may be efficiently cleaned up.
 * 
 * @param[in] pool - A pointer to an allocated session memory pool object.
 * @return Void.
 *
 * @see lsr_xpool_destroy
 */
void    lsr_xpool_init( lsr_xpool_t *pool );

/**
 * @lsr_xpool_destroy
 * @brief Destroys the contents of a session memory pool object and
 *   frees (releases) its data.
 * 
 * @param[in] pool - A pointer to an initialized session pool object.
 * @return Void.
 *
 * @see lsr_xpool_init
 */
void    lsr_xpool_destroy( lsr_xpool_t *pool );

/**
 * @lsr_xpool_reset
 * @brief Destroys then re-initializes a session memory pool object.
 * @details The previously allocated memory and data are released,
 *   and the pool is set empty to be used again.
 * 
 * @param[in] pool - A pointer to an initialized session pool object.
 * @return Void.
 */
void    lsr_xpool_reset( lsr_xpool_t *pool );

/**
 * @lsr_xpool_alloc
 * @brief Allocates memory from the session memory pool.
 * @details The allocated memory is not initialized.
 * 
 * @param[in] pool - A pointer to an initialized session pool object.
 * @param[in] size - The size (number of bytes) to allocate.
 * @return A pointer to the allocated memory, else NULL on error.
 *
 * @see lsr_xpool_free
 */
void   *lsr_xpool_alloc( lsr_xpool_t *pool, uint32_t size );

/**
 * @lsr_xpool_calloc
 * @brief Allocates memory from the session memory pool
 *   for an array of fixed size items.
 * @details The allocated memory is set to zero (unlike lsr_xpool_alloc).
 * 
 * @param[in] pool - A pointer to an initialized session pool object.
 * @param[in] items - The number of items to allocate.
 * @param[in] size - The size in bytes of each item.
 * @return A pointer to the allocated memory, else NULL on error.
 *
 * @see lsr_xpool_alloc
 */
void   *lsr_xpool_calloc( lsr_xpool_t *pool, uint32_t items, uint32_t size );

/**
 * @lsr_xpool_realloc
 * @brief Changes the size of a block of memory allocated
 *   from the session memory pool.
 * @details The new memory contents will be unchanged
 *   for the minimum of the old and new sizes.
 *   If the memory size is increasing, the additional memory is uninitialized.
 * 
 * @param[in] pool - A pointer to an initialized session pool object.
 * @param[in] pOld - A pointer to the current allocated memory.
 * @param[in] new_sz - The new size in bytes.
 * @return A pointer to the new allocated memory, else NULL on error.
 * @note If the current pointer \e pOld argument is NULL,
 *   lsr_xpool_realloc effectively becomes lsr_xpool_alloc to allocate new memory.
 *
 * @see lsr_xpool_alloc
 */
void   *lsr_xpool_realloc( lsr_xpool_t *pool, void *pOld, uint32_t new_sz );

/**
 * @lsr_xpool_free
 * @brief Frees (releases) memory back to the session memory pool.
 * @details The memory pointer must be one returned from a previous
 *   successful call to lsr_xpool_alloc.
 * 
 * @param[in] pool - A pointer to an initialized session pool object.
 * @param[in] data - A pointer to the allocated memory.
 * @return Void.
 *
 * @see lsr_xpool_alloc
 */
void    lsr_xpool_free( lsr_xpool_t *pool, void *data );

/**
 * @lsr_xpool_is_empty
 * @brief Specifies whether or not a session memory pool object is empty.
 * 
 * @param[in] pool - A pointer to an initialized session pool object.
 * @return Non-zero if empty, else 0 if not.
 */
int     lsr_xpool_is_empty( lsr_xpool_t *pool );

/**
 * @lsr_xpool_skip_free
 * @brief Sets the mode of the session memory pool to \e not free memory
 *   blocks individually at this time.
 * @details This call may be used as an optimization when the pool
 *   is going to be reset or destroyed.
 * 
 * @param[in] pool - A pointer to an initialized session pool object.
 * @return Void.
 */
void    lsr_xpool_skip_free( lsr_xpool_t *pool );

/**
 * @lsr_xpool_delete
 * @brief Destroys then deletes a session memory pool object.
 * @details The object should have been created with a previous
 *   successful call to lsr_xpool_new.
 * 
 * @param[in] pool - A pointer to an initialized session pool object.
 * @return Void.
 *
 * @see lsr_xpool_new
 */
void    lsr_xpool_delete( lsr_xpool_t *pool );

#ifdef __cplusplus
}
#endif

#endif //LSR_XPOOL_H
