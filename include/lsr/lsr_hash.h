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
#ifndef LSR_HASH_H
#define LSR_HASH_H

#include <lsr/lsr_xpool.h>

#include <ctype.h>
#include <stddef.h>
#include <lsr/lsr_types.h>

/**
 * @file
 * @note \b IMPORTANT: For every hash element, the key 
 * \b MUST \b BE a part of the value structure.
 */

#ifdef __cplusplus
extern "C" {
#endif 
    

typedef struct lsr_hash_s lsr_hash_t;
typedef struct lsr_hashelem_s lsr_hashelem_t;


typedef lsr_hashelem_t *lsr_hash_iter;

/**
 * @typedef lsr_hash_hash_fn
 * @brief The user must provide a hash function for the key structure.
 * It will be used to calculate where the hash element belongs.
 * 
 * @param[in] p - A pointer to the key.
 * @return A lsr_hash_key_t value.
 */
typedef lsr_hash_key_t (*lsr_hash_hash_fn) ( const void * p );

/**
 * @typedef lsr_hash_val_comp
 * @brief The user must provide a comparison function for the key structure.
 * It will be used whenever comparisons need to be made.
 * 
 * @param[in] pVal1 - A pointer to the first key to compare.
 * @param[in] pVal2 - A pointer to the second key to compare.
 * @return < 0 for pVal1 before pVal2, 0 for equal,
 * \> 0 for pVal1 after pVal2.
 */
typedef int (*lsr_hash_val_comp) ( const void *pVal1, const void *pVal2 );

/**
 * @typedef lsr_hash_for_each_fn
 * @brief A function that the user wants to apply to every element in the hash table.
 * 
 * @param[in] pKey - A pointer to the key from the current hash element.
 * @param[in] pData - A pointer to the value from the same hash element.
 * @return Should return 0 on success, not 0 on failure.
 */
typedef int (*lsr_hash_for_each_fn)( const void *pKey, void *pData );

/**
 * @typedef lsr_hash_for_each2_fn
 * @brief A function that the user wants to apply to every element in the hash table.
 * Also allows a user provided data for the function.
 * 
 * @param[in] pKey - A pointer to the key from the current hash element.
 * @param[in] pData - A pointer to the value from the same hash element.
 * @param[in] pUData - A pointer to the user data passed into the function.
 * @return Should return 0 on success, not 0 on failure.
 */
typedef int (*lsr_hash_for_each2_fn)( const void *pKey, void *pData, void *pUData );

typedef lsr_hash_iter (*lsr_hash_insert_fn)(lsr_hash_t *pThis, const void *pKey, void *pValue);
typedef lsr_hash_iter (*lsr_hash_update_fn)(lsr_hash_t *pThis, const void *pKey, void *pValue);
typedef lsr_hash_iter (*lsr_hash_find_fn)  (lsr_hash_t *pThis, const void *pKey );



/**
 * @typedef lsr_hash_t
 * @brief A hash table structure.
 */
struct lsr_hash_s
{
    lsr_hashelem_t    **m_table;
    lsr_hashelem_t    **m_tableEnd;
    size_t              m_capacity;
    size_t              m_size;
    int                 m_full_factor;
    lsr_hash_hash_fn    m_hf;
    lsr_hash_val_comp   m_vc;
    int                 m_grow_factor;
    lsr_xpool_t        *m_xpool;
    
    lsr_hash_insert_fn  m_insert;
    lsr_hash_update_fn  m_update;
    lsr_hash_find_fn    m_find;
};

/** @lsr_hash_new
 * @brief Creates a new hash table.  Allocates from the global pool unless the pool parameter specifies a session pool.
 * Initializes the table according to the parameters.
 * @details The user may create his/her own hash function and val comp associated with his/her key structure,
 * but some sample ones for char * and ipv6 values are provided below.
 * @note If the user knows the table will only last for a session, a pointer to a
 * session pool may be passed in the pool parameter.
 * 
 * @param[in] init_size - Initial capacity to allocate.
 * @param[in] hf - A pointer to the hash function to use for the keys.
 * @param[in] vc - A pointer to the comparison function to use for the keys.
 * @param[in] pool - A session pool pointer, else NULL to specify the global pool.
 * @return A pointer to a new initialized hash table on success, NULL on failure.
 * 
 * @see lsr_hash_delete, lsr_hash_hf_string, lsr_hash_hf_ci_string, lsr_hash_hf_ipv6, lsr_hash_hash_fn
 * lsr_hash_cmp_string, lsr_hash_cmp_ci_string, lsr_hash_cmp_ipv6, lsr_hash_val_comp
 */
lsr_hash_t     *lsr_hash_new( size_t init_size, lsr_hash_hash_fn hf, lsr_hash_val_comp vc, lsr_xpool_t *pool );

/** @lsr_hash
 * @brief Initializes the hash table.  Allocates from the global pool unless the pool parameter specifies a session pool.
 * @details The user may create his/her own hash function and val comp associated with his/her key structure,
 * but some sample ones for char * and ipv6 values are provided below.
 * @note If the user knows the table will only last for a session, a pointer to a
 * session pool may be passed in the pool parameter.
 * 
 * @param[in] pThis - A pointer to an allocated hash table object.
 * @param[in] init_size - Initial capacity to allocate.
 * @param[in] hf - A pointer to the hash function to use for the keys.
 * @param[in] vc - A pointer to the comparison function to use for the keys.
 * @param[in] pool - A session pool pointer, else NULL to specify the global pool.
 * @return A pointer to the initialized hash table on success, NULL on failure.
 * 
 * @see lsr_hash_d, lsr_hash_hf_string, lsr_hash_hf_ci_string, lsr_hash_hf_ipv6, lsr_hash_hash_fn
 * lsr_hash_cmp_string, lsr_hash_cmp_ci_string, lsr_hash_cmp_ipv6, lsr_hash_val_comp
 */
lsr_hash_t     *lsr_hash( lsr_hash_t *pThis, size_t init_size, lsr_hash_hash_fn hf, lsr_hash_val_comp vc, lsr_xpool_t *pool );

/** @lsr_hash_d
 * @brief Destroys the table.  Does not free the hash structure itself, only the internals.
 * @note This function should be used in conjunction with lsr_hash.  
 * The user is responsible for freeing the data itself.  This function only frees structures
 * allocated by lsr hash functions.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @return Void.
 * 
 * @see lsr_hash
 */
void            lsr_hash_d( lsr_hash_t *pThis );

/** @lsr_hash_delete
 * @brief Deletes the table.  Frees the table and the hash structure itself.
 * @note This function should be used in conjunction with lsr_hash_new.
 * The user is responsible for freeing the data itself.  This function only frees structures
 * allocated by lsr hash functions.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @return Void.
 * 
 * @see lsr_hash_new
 */
void            lsr_hash_delete( lsr_hash_t *pThis );

/** @lsr_hash_get_key
 * @brief Given a hash element, returns the key.
 * 
 * @param[in] pElem - A pointer to the hash element to get the key from.
 * @return The key.
 */
const void     *lsr_hash_get_key( lsr_hashelem_t *pElem );

/** @lsr_hash_get_data
 * @brief Given a hash element, returns the data/value.
 * 
 * @param[in] pElem - A pointer to the hash element to get the data from.
 * @return The data.
 */
void           *lsr_hash_get_data( lsr_hashelem_t *pElem );

/** @lsr_hash_get_hkey
 * @brief Given a hash element, returns the hash key.
 * 
 * @param[in] pElem - A pointer to the hash element to get the hash key from.
 * @return The hash key.
 */
lsr_hash_key_t  lsr_hash_get_hkey( lsr_hashelem_t *pElem );

/** @lsr_hash_get_next
 * @brief Given a hash element, returns the next hash element in the same
 * array slot.
 * 
 * @param[in] pElem - A pointer to the current hash element.
 * @return The next hash element, or NULL if there are no more in the slot.
 */
lsr_hashelem_t *lsr_hash_get_next( lsr_hashelem_t *pElem );

/** @lsr_hash_hf_string
 * @brief A provided hash function for strings.  Case sensitive.
 * 
 * @param[in] __s - The string to calculate.
 * @return The hash key.
 */
lsr_hash_key_t  lsr_hash_hf_string( const void *__s );

/** @lsr_hash_cmp_string
 * @brief A provided comparison function for strings.  Case sensitive.
 * 
 * @param[in] pVal1 - The first string to compare.
 * @param[in] pVal2 - The second string to compare.
 * @return Result according to #lsr_hash_val_comp.
 */
int             lsr_hash_cmp_string( const void *pVal1, const void *pVal2 );

/** @lsr_hash_hf_ci_string
 * @brief A provided hash function for strings.  Case insensitive.
 * 
 * @param[in] __s - The string to calculate.
 * @return The hash key.
 */
lsr_hash_key_t  lsr_hash_hf_ci_string(const void *__s);

/** @lsr_hash_cmp_ci_string
 * @brief A provided comparison function for strings.  Case insensitive.
 * 
 * @param[in] pVal1 - The first string to compare.
 * @param[in] pVal2 - The second string to compare.
 * @return Result according to #lsr_hash_val_comp.
 */
int             lsr_hash_cmp_ci_string( const void *pVal1, const void *pVal2 );

/** @lsr_hash_hf_ipv6
 * @brief A provided hash function for ipv6 values (unsigned long).
 * 
 * @param[in] pKey - A pointer to the value to calculate.
 * @return The hash key.
 */
lsr_hash_key_t  lsr_hash_hf_ipv6( const void *pKey );

/** @lsr_hash_cmp_ipv6
 * @brief A provided comparison function for ipv6 values (unsigned long).
 * 
 * @param[in] pVal1 - The first value to compare.
 * @param[in] pVal2 - The second value to compare.
 * @return Result according to #lsr_hash_val_comp.
 */
int             lsr_hash_cmp_ipv6( const void *pVal1, const void *pVal2 );

/** @lsr_hash_clear
 * @brief Empties the table of and frees the elements and resets the size.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @return Void.
 */
void            lsr_hash_clear( lsr_hash_t *pThis );

/** @lsr_hash_erase
 * @brief Erases the element from the table.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @param[in] iter - A pointer to the element to remove from the table.
 * This will be freed within the function.
 * @return Void.
 */
void            lsr_hash_erase( lsr_hash_t *pThis, lsr_hash_iter iter );

/** @lsr_hash_swap
 * @brief Swaps the lhs and rhs hash tables.
 * 
 * @param[in,out] lhs - A pointer to an initialized hash table object.
 * @param[in,out] rhs - A pointer to another initialized hash table object.
 * @return Void.
 */
void            lsr_hash_swap( lsr_hash_t *lhs, lsr_hash_t *rhs );

/** @lsr_hash_find
 * @brief Finds the element with the given key in the hash table.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @param[in] pKey - The key to search for.
 * @return A pointer to the hash element if found or NULL if not found.
 */
lsr_inline lsr_hash_iter lsr_hash_find( lsr_hash_t *pThis, const void *pKey )
{    return (pThis->m_find)( pThis, pKey );             }

/** @lsr_hash_insert
 * @brief Inserts an element with the given key and value in the hash table.
 * @note \b IMPORTANT: The key \b MUST \b BE a part of the value structure.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @param[in] pKey - The key of the new hash element.
 * @param[in] pValue - The value of the new hash element.
 * @return A pointer to the hash element if added or NULL if not.
 */
lsr_inline lsr_hash_iter lsr_hash_insert( lsr_hash_t *pThis, const void *pKey, void *pValue )
{    return (pThis->m_insert)( pThis, pKey, pValue );   }

/** @lsr_hash_update
 * @brief Updates an element with the given key in the hash table.  Inserts
 * if it doesn't exist.
 * @note \b IMPORTANT: The key \b MUST \b BE a part of the value structure.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @param[in] pKey - The key of the hash element.
 * @param[in] pValue - The value of the hash element.
 * @return A pointer to the hash element if updated or NULL if something went wrong.
 */
lsr_inline lsr_hash_iter lsr_hash_update( lsr_hash_t *pThis, const void *pKey, void *pValue )
{    return (pThis->m_update)( pThis, pKey, pValue );   }

/** @lsr_hash_get_hash_fn
 * @brief Gets the hash function of the hash table.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @return A pointer to the hash function.
 */
lsr_inline lsr_hash_hash_fn  lsr_hash_get_hash_fn( lsr_hash_t *pThis )
{    return pThis->m_hf;    }

/** @lsr_hash_get_val_comp
 * @brief Gets the comparison function of the hash table.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @return A pointer to the comparison function.
 */
lsr_inline lsr_hash_val_comp lsr_hash_get_val_comp( lsr_hash_t *pThis )
{    return pThis->m_vc;    }

/** @lsr_hash_set_full_factor
 * @brief Sets the full factor of the hash table.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @param[in] f - The full factor to update the table to.
 * @return Void.
 */
lsr_inline void      lsr_hash_set_full_factor( lsr_hash_t *pThis, int f )
{    if ( f > 0 )    pThis->m_full_factor = f;  }

/** @lsr_hash_set_grow_factor
 * @brief Sets the grow factor of the hash table.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @param[in] f - The grow factor to update the table to.
 * @return Void.
 */
lsr_inline void      lsr_hash_set_grow_factor( lsr_hash_t *pThis, int f )
{    if ( f > 0 )    pThis->m_grow_factor = f;  }

/** @lsr_hash_empty
 * @brief Specifies whether or not the hash table is empty.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @return Non-zero if empty, 0 if not.
 */
lsr_inline int       lsr_hash_empty( const lsr_hash_t *pThis )
{    return pThis->m_size == 0; }

/** @lsr_hash_size
 * @brief Gets the current size of the hash table.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @return The size of the table.
 */
lsr_inline size_t    lsr_hash_size( const lsr_hash_t *pThis )
{    return pThis->m_size;      }

/** @lsr_hash_capacity
 * @brief Gets the current capacity of the hash table.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @return The capacity of the table.
 */
lsr_inline size_t           lsr_hash_capacity( const lsr_hash_t *pThis )
{    return pThis->m_capacity;  }

/** @lsr_hash_begin
 * @brief Gets the first element of the hash table.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @return A pointer to the first element of the table.
 */
lsr_hash_iter               lsr_hash_begin( lsr_hash_t *pThis );

/** @lsr_hash_end
 * @brief Gets the end element of the hash table.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @return A pointer to the end element of the table.
 */
lsr_inline lsr_hash_iter    lsr_hash_end( lsr_hash_t *pThis )
{    return NULL;               }
    
/** @lsr_hash_next
 * @brief Gets the next element of the hash table.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @param[in] iter - A pointer to the current hash element.
 * @return A pointer to the next element of the table, NULL if it's the end.
 */
lsr_hash_iter   lsr_hash_next( lsr_hash_t *pThis, lsr_hash_iter iter );

/** @lsr_hash_for_each
 * @brief Runs a function for each element in the hash table.  The function must 
 * follow the #lsr_hash_for_each_fn format.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @param[in] beg - A pointer to the first hash element to apply the function to.
 * @param[in] end - A pointer to the last hash element to apply the function to.
 * @param[in] fun - A pointer to the \link #lsr_hash_for_each_fn function\endlink 
 * to apply to hash elements.
 * @return The number of hash elements that the function applied to.
 */
int             lsr_hash_for_each( lsr_hash_t *pThis, lsr_hash_iter beg, 
                            lsr_hash_iter end, lsr_hash_for_each_fn fun );

/** @lsr_hash_for_each2
 * @brief Runs a function for each element in the hash table.  The function must 
 * follow the #lsr_hash_for_each2_fn format.
 * 
 * @param[in] pThis - A pointer to an initialized hash table object.
 * @param[in] beg - A pointer to the first hash element to apply the function to.
 * @param[in] end - A pointer to the last hash element to apply the function to.
 * @param[in] fun - A pointer to the \link #lsr_hash_for_each2_fn function\endlink 
 * to apply to hash elements.
 * @param[in] pUData - A pointer to the user data to pass into the function.
 * @return The number of hash elements that the function applied to.
 */
int             lsr_hash_for_each2( lsr_hash_t *pThis, lsr_hash_iter beg, lsr_hash_iter end,
                            lsr_hash_for_each2_fn fun, void * pUData );


#ifdef __cplusplus
}
#endif 


#endif //LSR_HASH_H
