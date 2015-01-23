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
#ifndef LSR_STR_H
#define LSR_STR_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <lsr/lsr_types.h>

/**
 * @file
 * This structure is essentially a string with the length included.
 * It will do any allocation for the user as necessary.  If the user
 * wishes to use the session pool,  the *_xp functions must be called
 * when it is available.  Any other function may use the regular lsr_str_*
 * functions because they do not allocate.
 * 
 * The user must remain consistent with the xp or non xp version, as any
 * mixing will likely cause errors.
 */

#ifdef __cplusplus
extern "C" {
#endif 
    
/**
 * @defgroup LSR_STR_GROUP Str Group
 * @{
 * 
 * This module defines the lsr_str structures.
 * 
 * @see lsr_str.h
 * 
 */
 
/**
 * @typedef lsr_str_t
 * @brief A structure like a char *, except it has the string length as well.
 */
struct lsr_str_s
{
    char *m_pStr;
    int   m_iStrLen;
};

/**
 * @typedef lsr_str_pair_t
 * @brief A str pair structure that can be used for key/value pairs.
 */
struct lsr_str_pair_s
{
    lsr_str_t m_key;
    lsr_str_t m_value;
};

/**
 * @}
 */

/** @lsr_str_new
 * @brief Creates a new lsr str object.  Initializes the
 * buffer and length to a deep copy of \e pStr and \e len.
 * 
 * @param[in] pStr - The string to duplicate.  May be NULL.
 * @param[in] len - The length of the string.
 * @return A pointer to the lsr str object if successful, NULL if not.
 * 
 * @see lsr_str_delete
 */
lsr_str_t *lsr_str_new( const char *pStr, int len );

/** @lsr_str
 * @brief Initializes a given lsr str.  Initializes the
 * buffer and length to a deep copy of \e pStr and \e len.
 * 
 * @param[in] pThis - A pointer to an allocated lsr str object.
 * @param[in] pStr - The string to duplicate.  May be NULL.
 * @param[in] len - The length of the string.
 * @return The lsr str object if successful, NULL if not.
 * 
 * @see lsr_str_d
 */
lsr_str_t *lsr_str( lsr_str_t *pThis, const char *pStr, int len );

/** @lsr_str_copy
 * @brief Copies the source lsr str to the dest lsr str.  Makes a
 * deep copy of the string.  Must be destroyed with lsr_str_d
 * 
 * @param[in] dest - A pointer to an initialized lsr str object.
 * @param[in] src - A pointer to another initialized lsr str object.
 * @return Dest if successful, NULL if not.
 * 
 * @see lsr_str, lsr_str_d
 */
lsr_str_t *lsr_str_copy( lsr_str_t *dest, const lsr_str_t *src );

/** @lsr_str_d
 * @brief Destroys the internals of the lsr str object.  DOES NOT FREE pThis!
 * The opposite of lsr_str.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @return Void.
 * 
 * @see lsr_str
 */
void       lsr_str_d( lsr_str_t *pThis );

/** @lsr_str_delete
 * @brief Destroys and deletes the lsr str object. The opposite of lsr_str_new.
 * @details The object should have been created with a previous
 *   successful call to lsr_str_new.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @return Void.
 * 
 * @see lsr_str_new
 */
void       lsr_str_delete( lsr_str_t *pThis );

/** @lsr_str_blank
 * @brief Clears the internals of the lsr str object.  DOES NOT FREE THE BUF!
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @return Void.
 */
lsr_inline void lsr_str_blank( lsr_str_t * pThis )
{   pThis->m_pStr = NULL; pThis->m_iStrLen = 0;       }

/** @lsr_str_len
 * @brief Gets the length of the lsr str object.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @return The length.
 */
lsr_inline int           lsr_str_len( const lsr_str_t *pThis )
{   return pThis->m_iStrLen;    }

/** @lsr_str_set_len
 * @brief Sets the length of the string.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @param[in] len - The length to set the lsr str object to.
 * @return Void.
 */
lsr_inline void          lsr_str_set_len( lsr_str_t *pThis, int len )
{   pThis->m_iStrLen = len;    }

/** @lsr_str_buf
 * @brief Gets a char * of the string.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @return A char * of the string.
 */
lsr_inline char         *lsr_str_buf( const lsr_str_t *pThis )
{   return pThis->m_pStr;   }

/** @lsr_str_c_str
 * @brief Gets a \e const char * of the string.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @return A \e const char * of the string.
 */
lsr_inline const char   *lsr_str_c_str( const lsr_str_t *pThis )
{   return pThis->m_pStr;   }

/** @lsr_str_prealloc
 * @brief Preallocates space in the lsr str object.
 * @details Useful for when you want a buffer created for later use.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @param[in] size - The size to allocate.
 * @return The allocated space, NULL if error.
 */
char   *lsr_str_prealloc( lsr_str_t *pThis, int size );

/** @lsr_str_set_str
 * @brief Sets the lsr str object to \e pStr.  Will create a deep copy.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @param[in] pStr - The string to set the lsr str object to.
 * @param[in] len - The length of the string.
 * @return The number of bytes set.
 */
int     lsr_str_set_str( lsr_str_t *pThis, const char *pStr, int len );

/** @lsr_str_append
 * @brief Appends the current lsr str object with \e pStr.  Will allocate a deep copy.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @param[in] pStr - The string to append the lsr str object to.
 * @param[in] len - The length of the string.
 * @return Void.
 */
void    lsr_str_append( lsr_str_t *pThis, const char *pStr, const int len );

/** @lsr_str_cmp
 * @brief A comparison function for lsr str. Case Sensitive.
 * @details This may be used for lsr hash or map comparison.
 * 
 * @param[in] pVal1 - The first lsr str object to compare.
 * @param[in] pVal2 - The second lsr str object to compare.
 * @return Result according to #lsr_hash_val_comp.
 */
int             lsr_str_cmp( const void *pVal1, const void *pVal2 );

/** @lsr_str_hf
 * @brief A hash function for lsr str structure. Case Sensitive.
 * @details This may be used for lsr hash.
 * 
 * @param[in] pKey - The key to calculate.
 * @return The hash key.
 */
lsr_hash_key_t  lsr_str_hf( const void *pKey );

/** @lsr_str_ci_cmp
 * @brief A comparison function for lsr str. Case Insensitive.
 * @details This may be used for lsr hash or map comparison.
 * 
 * @param[in] pVal1 - The first lsr str object to compare.
 * @param[in] pVal2 - The second lsr str object to compare.
 * @return Result according to #lsr_hash_val_comp.
 */
int             lsr_str_ci_cmp( const void *pVal1, const void *pVal2 );

/** @lsr_str_ci_hf
 * @brief A hash function for lsr str. Case Insensitive.
 * @details This may be used for lsr hash.
 * 
 * @param[in] pKey - The key to calculate.
 * @return The hash key.
 */
lsr_hash_key_t  lsr_str_ci_hf( const void *pKey );

/** @lsr_str_unsafe_set
 * @brief Sets the lsr str object to the \e pStr.  WILL NOT MAKE A DEEP COPY!
 * @warning This may be used if the user does not want to create a deep
 * copy of the string.  However, with that said, the user is responsible
 * with deallocating the string and may not destroy the lsr str unless
 * it is \link #lsr_str_blank blanked. \endlink
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @param[in] pStr - The string to set to.
 * @param[in] len - The length of the string.
 * @return Void.
 */
lsr_inline void lsr_str_unsafe_set( lsr_str_t *pThis, char *pStr, int len )
{
    pThis->m_pStr = pStr;
    pThis->m_iStrLen = len;
}

/** @lsr_str_new_xp
 * @brief Creates a new lsr str object.
 * @details Initializes the buffer and length to a deep copy 
 * of \e pStr and \e len from the given session pool.  Must delete
 * with \link #lsr_str_delete_xp delete xp. \endlink
 * 
 * @param[in] pStr - The string to duplicate.  May be NULL.
 * @param[in] len - The length of the string.
 * @param[in] pool - A pointer to the pool to allocate from.
 * @return The lsr str object if successful, NULL if not.
 * 
 * @see lsr_str_delete_xp
 */
lsr_str_t  *lsr_str_new_xp( const char *pStr, int len, lsr_xpool_t *pool );

/** @lsr_str_xp
 * @brief Initializes a given lsr str from the session pool.  Initializes the
 * buffer and length to a deep copy of \e pStr and \e len.
 * @details Must destroy with \link #lsr_str_d_xp destroy xp. \endlink
 * 
 * @param[in] pThis - A pointer to an allocated str to initialize.
 * @param[in] pStr - The string to duplicate.  May be NULL.
 * @param[in] len - The length of the string.
 * @param[in] pool - A pointer to the pool to allocate from.
 * @return The lsr str object if successful, NULL if not.
 * 
 * @see lsr_str_d_xp
 */
lsr_str_t  *lsr_str_xp( lsr_str_t *pThis, const char *pStr, int len, lsr_xpool_t *pool );

/** @lsr_str_copy_xp
 * @brief Copies the source lsr str object to the destination lsr str object.  Makes a
 * deep copy of the string.  Must destroy with \link #lsr_str_d_xp destroy xp. \endlink
 * 
 * @param[in] dest - A pointer to an allocated lsr str object.
 * @param[in] src - A pointer to an initialized lsr str object.
 * @param[in] pool - A pointer to the pool to allocate from.
 * @return Dest if successful, NULL if not.
 * 
 * @see lsr_str_d_xp
 */
lsr_str_t  *lsr_str_copy_xp( lsr_str_t *dest, const lsr_str_t *src, lsr_xpool_t *pool );

/** @lsr_str_d_xp
 * @brief Destroys the internals of the lsr str object.  DOES NOT FREE pThis!
 * The opposite of lsr_str_xp.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @param[in] pool - The pool to deallocate from.
 * @return Void.
 * 
 * @see lsr_str_xp
 */
void        lsr_str_d_xp( lsr_str_t *pThis, lsr_xpool_t *pool );

/** @lsr_str_delete_xp
 * @brief Destroys and deletes the lsr str object. The opposite of lsr_str_new_xp.
 * @details The object should have been created with a previous
 *   successful call to lsr_str_new_xp.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @param[in] pool - A pointer to the pool to deallocate from.
 * @return Void.
 * 
 * @see lsr_str_new_xp
 */
void        lsr_str_delete_xp( lsr_str_t *pThis, lsr_xpool_t *pool );

/** @lsr_str_prealloc_xp
 * @brief Preallocates space in the lsr str object from the session pool.
 * @details Useful for when you want a buffer created for later use.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @param[in] size - The size to allocate.
 * @param[in] pool - A pointer to the pool to allocate from.
 * @return The allocated space, NULL if error.
 */
char   *lsr_str_prealloc_xp( lsr_str_t *pThis, int size, lsr_xpool_t *pool );

/** @lsr_str_set_str_xp
 * @brief Sets the lsr str object to \e pStr.  Will create a deep copy.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @param[in] pStr - The string to set the lsr str object to.
 * @param[in] len - The length of the string.
 * @param[in] pool - A pointer to the pool to allocate from.
 * @return The number of bytes set.
 */
int     lsr_str_set_str_xp( lsr_str_t *pThis, const char *pStr, int len, lsr_xpool_t *pool );

/** @lsr_str_append_xp
 * @brief Appends the current lsr str object with \e pStr.  Will allocate a deep copy
 * from the session pool.
 * 
 * @param[in] pThis - A pointer to an initialized lsr str object.
 * @param[in] pStr - The string to append to the struct.
 * @param[in] len - The length of the string.
 * @param[in] pool - A pointer to the pool to allocate from.
 * @return Void.
 */
void    lsr_str_append_xp( lsr_str_t *pThis, const char *pStr, const int len, lsr_xpool_t *pool );



#ifdef __cplusplus
}
#endif 


#endif //LSR_STR_H
