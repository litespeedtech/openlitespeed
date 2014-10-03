/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

/***************************************************************************
    $Id: pool.h,v 1.1.1.1 2013/12/22 23:42:51 gwang Exp $
                         -------------------
    begin                : Wed Apr 30 2003
    author               : Gang Wang
    email                : gwang@litespeedtech.com
 ***************************************************************************/
#ifndef LSR_POOL_H
#define LSR_POOL_H

/* define for thread safe memory pool */
//#define _REENTRANT

/* define for *no* valgrind tracking of custom lsr memory pool */
#define NVALGRIND

/* define to bypass lsr memory management and always use malloc/free */
#define DEBUG_POOL

#ifdef DEBUG_POOL
#undef _REENTRANT
#define NVALGRIND
#endif

#include <stddef.h>


/**
 * @file
 * The global (and session) memory pools may be configured in a number of ways
 * selectable through conditional compilation.
 *
 * \li _REENTRANT - Define for thread safe memory pool.
 * \li NVALGRIND - Define for \e no valgrind tracking of custom lsr memory pool.
 * \li DEBUG_POOL - Define to bypass lsr memory management and always use malloc/free.
 *
 * Note that defining DEBUG_POOL will undefine _REENTRANT and define NVALGRIND.
 */


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef lsr_pool_header_t
 * @brief Global memory pool header.
 */
typedef struct lsr_pool_header_s    lsr_pool_header_t;

/**
 * @typedef lsr_pool_blk_t
 * @brief Global memory pool block definition.
 */
typedef struct lsr_pool_blk_s       lsr_pool_blk_t;

/**
 * @typedef lsr_xpool_bblk_t
 * @brief Session memory pool `big' block definition.
 */
typedef struct lsr_xpool_bblk_s     lsr_xpool_bblk_t;
    
/**
 * @lsr_palloc
 * @brief Allocates memory from the global memory pool.
 * @details The allocated memory is not initialized.
 * 
 * @param[in] size - The size (number of bytes) to allocate.
 * @return A pointer to the allocated memory, else NULL on error.
 *
 * @see lsr_pfree
 */
void * lsr_palloc( size_t size );

/**
 * @lsr_pfree
 * @brief Frees (releases) memory back to the global memory pool.
 * @details The memory pointer must be one returned from a previous
 *   successful call to lsr_palloc.
 * 
 * @param[in] p - A pointer to the allocated memory.
 * @return Void.
 *
 * @see lsr_palloc
 */
void   lsr_pfree( void *p );

/**
 * @lsr_prealloc
 * @brief Changes the size of a block of memory allocated
 *   from the global memory pool.
 * @details The new memory contents will be unchanged
 *   for the minimum of the old and new sizes.
 *   If the memory size is increasing, the additional memory is uninitialized.
 *   If the current pointer \e p argument is NULL, lsr_prealloc effectively 
 *   becomes lsr_palloc to allocate new memory.
 * 
 * @param[in] p - A pointer to the current allocated memory.
 * @param[in] new_sz - The new size in bytes.
 * @return A pointer to the new allocated memory, else NULL on error.
 *
 * @see lsr_palloc
 */
void * lsr_prealloc( void *p, size_t new_sz );

/**
 * @lsr_dupstr2
 * @brief Duplicates a buffer (or string).
 * @details The new memory for the duplication is allocated from the
 *   global memory pool.
 * 
 * @param[in] p - A pointer to the buffer to duplicate.
 * @param[in] len - The size (length) in bytes of the buffer.
 * @return A pointer to the duplicated buffer, else NULL on error.
 */
char * lsr_pdupstr2( const char *p, int len );

/**
 * @lsr_dupstr
 * @brief Duplicates a null-terminated string.
 * @details The new memory for the duplication is allocated from the
 *   global memory pool.
 * 
 * @param[in] p - A pointer to the string to duplicate.
 * @return A pointer to the duplicated string (including null-termination),
 *   else NULL on error.
 */
char * lsr_pdupstr( const char *p );

#ifdef __cplusplus
}
#endif

#endif  /* LSR_POOL_H */

