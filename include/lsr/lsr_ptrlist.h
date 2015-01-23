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
#ifndef LSR_PTRLIST_H
#define LSR_PTRLIST_H

 
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <lsr/lsr_pool.h>
#include <lsr/lsr_types.h>


/**
 * @file
 * @note
 * Pointer List - 
 *   The pointer list object manages a list of objects through
 *   a list of pointers (void *) to them.
 *   The objects pointed to may be of any defined type.
 *   Functions are available to allocate, add, and remove elements
 *   to/from the list in a variety of ways, get information about the list,
 *   and also perform functions such as sorting the list.
 */


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef gpl_for_each_fn
 * @brief Function pointer when iterating through a pointer list object.
 */
typedef int (*gpl_for_each_fn)( void * );
/**
 * @typedef lsr_ptrlist_iter
 * @brief Iterator for the pointer list object.
 */
typedef void ** lsr_ptrlist_iter;
/**
 * @typedef lsr_const_ptrlist_iter
 * @brief Const iterator for the pointer list object.
 */
typedef void *const* lsr_const_ptrlist_iter;

/**
 * @typedef lsr_ptrlist_t
 */
struct lsr_ptrlist_s
{
    /**
     * @brief The beginning of storage.
     */
    void ** m_pStore;
    /**
     * @brief The end of storage.
     */
    void ** m_pStoreEnd;
    /**
     * @brief The current end of data.
     */
    void ** m_pEnd;
};
    
/**
 * @lsr_ptrlist
 * @brief Initializes a pointer list object.
 * @details This object manages pointers to generic (void *) objects.
 *   An initial size is specified, but the size may change with other
 *   lsr_ptrlist_* routines.
 * 
 * @param[in] pThis - A pointer to an allocated pointer list object.
 * @param[in] initSize - The initial size to allocate for the list elements,
 *    which may be 0 specifying an empty initialized pointer list.
 * @return Void.
 *
 * @see lsr_ptrlist_new, lsr_ptrlist_d
 */
void lsr_ptrlist( lsr_ptrlist_t *pThis, size_t initSize );

/**
 * @lsr_ptrlist_copy
 * @brief Copies a pointer list object.
 * 
 * @param[in] pThis - A pointer to an allocated pointer list object (destination).
 * @param[in] pRhs - A pointer to the pointer list object to be copied (source).
 * @return Void.
 */
void lsr_ptrlist_copy( lsr_ptrlist_t *pThis, const lsr_ptrlist_t *pRhs );

/**
 * @lsr_ptrlist_new
 * @brief Creates a new pointer list object.
 * @details The routine allocates and initializes an object
 *   which manages pointers to generic (void *) objects.
 *   An initial size is specified, but the size may change with other
 *   lsr_ptrlist_* routines.
 * 
 * @param[in] initSize - The initial size to allocate for the list elements,
 *    which may be 0 specifying an empty initialized pointer list.
 * @return A pointer to an initialized pointer list object.
 *
 * @see lsr_ptrlist, lsr_ptrlist_delete
 */
lsr_inline lsr_ptrlist_t * lsr_ptrlist_new( size_t initSize )
{
    lsr_ptrlist_t * pThis;
    if ( ( pThis = (lsr_ptrlist_t *)
      lsr_palloc( sizeof(*pThis) ) ) != NULL )
    {
        lsr_ptrlist( pThis, initSize );
    }
    return pThis;
}

/**
 * @lsr_ptrlist_d
 * @brief Destroys the contents of a pointer list object.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @return Void.
 *
 * @see lsr_ptrlist
 */
void lsr_ptrlist_d( lsr_ptrlist_t *pThis );

/**
 * @lsr_ptrlist_delete
 * @brief Destroys then deletes a pointer list object.
 * @details The object should have been created with a previous
 *   successful call to lsr_ptrlist_new.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @return Void.
 *
 * @see lsr_ptrlist_new
 */
lsr_inline void lsr_ptrlist_delete( lsr_ptrlist_t *pThis )
{   lsr_ptrlist_d( pThis );  lsr_pfree( pThis );   }

/**
 * @lsr_ptrlist_resize
 * @brief Changes the total storage or list size in a pointer list object.
 * @details If the new size is greater than the current capacity,
 *   the storage is extended with the existing data remaining intact.
 *   If the new size is less, the list size is truncated.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[in] sz - The new size in terms of number of elements (pointers).
 * @return 0 on success, else -1 on error.
 */
int lsr_ptrlist_resize( lsr_ptrlist_t *pThis, size_t sz );

/**
 * @lsr_ptrlist_size
 * @brief Gets the current size of a pointer list object.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @return The size in terms of number of elements (pointers).
 */
lsr_inline ssize_t lsr_ptrlist_size( const lsr_ptrlist_t *pThis )
{   return pThis->m_pEnd - pThis->m_pStore;   }

/**
 * @lsr_ptrlist_empty
 * @brief Specifies whether or not a pointer list object contains any elements.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @return True if empty, else false.
 */
lsr_inline bool lsr_ptrlist_empty( const lsr_ptrlist_t *pThis )
{   return pThis->m_pEnd == pThis->m_pStore;   }

/**
 * @lsr_ptrlist_full
 * @brief Specifies whether or not a pointer list object is at full capacity.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @return True if full, else false.
 */
lsr_inline bool lsr_ptrlist_full( const lsr_ptrlist_t *pThis )
{   return pThis->m_pEnd == pThis->m_pStoreEnd;   }

/**
 * @lsr_ptrlist_capacity
 * @brief Gets the storage capacity of a pointer list object;
 *   \e i.e., how many elements it may hold.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @return The capacity in terms of number of elements (pointers).
 */
lsr_inline size_t lsr_ptrlist_capacity( const lsr_ptrlist_t *pThis )
{   return pThis->m_pStoreEnd - pThis->m_pStore;   }

/**
 * @lsr_ptrlist_clear
 * @brief Clears (empties, deletes) a pointer list object of all its elements.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @return Void.
 */
lsr_inline void lsr_ptrlist_clear( lsr_ptrlist_t *pThis )
{   pThis->m_pEnd = pThis->m_pStore;   }

/**
 * @lsr_ptrlist_begin
 * @brief Gets the iterator beginning of a pointer list object.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @return The iterator beginning of a pointer list object.
 */
lsr_inline lsr_ptrlist_iter lsr_ptrlist_begin( lsr_ptrlist_t *pThis )
{   return pThis->m_pStore;   }

/**
 * @lsr_ptrlist_end
 * @brief Gets the iterator end of a pointer list object.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @return The iterator end of a pointer list object.
 */
lsr_inline lsr_ptrlist_iter lsr_ptrlist_end( lsr_ptrlist_t *pThis )
{   return pThis->m_pEnd;   }

/**
 * @lsr_ptrlist_back
 * @brief Gets the last element (pointer) in a pointer list object.
 * @details The routine does \e not change anything in the object.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @return The last element (pointer) in a pointer list object.
 */
lsr_inline void * lsr_ptrlist_back( const lsr_ptrlist_t *pThis )
{   return *(pThis->m_pEnd - 1);   }

/**
 * @lsr_ptrlist_reserve
 * @brief Reserves storage in a pointer list object.
 * @details If the object currently contains data elements,
 *   the storage is extended with the existing data remaining intact.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[in] sz - The new size in terms of number of elements (pointers).
 * @return 0 on success, else -1 on error.
 */
int lsr_ptrlist_reserve( lsr_ptrlist_t *pThis, size_t sz );

/**
 * @lsr_ptrlist_grow
 * @brief Extends (increases) the storage capacity of a pointer list object.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[in] sz - The size increment in terms of number of elements (pointers).
 * @return 0 on success, else -1 on error.
 */
int lsr_ptrlist_grow( lsr_ptrlist_t *pThis, size_t sz );
 
/**
 * @lsr_ptrlist_erase
 * @brief Erases (deletes) an element from a pointer list object.
 * @details This decreases the size of the pointer list object by one.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[in,out] iter - Iterator specifying the element to erase.
 * @return The same iterator which now specifies the previous last element in
 *   the pointer list object.
 * @note The order of the elements in the pointer list object
 *   may have been changed.
 *   This may be significant if the elements were expected to be in a
 *   specific order.
 * @note Note also that when erasing the last (\e i.e., end) element
 *   in the list, upon return the iterator specifies the newly erased
 *   element, which is thus invalid.
 */
lsr_inline lsr_ptrlist_iter lsr_ptrlist_erase(
  lsr_ptrlist_t *pThis, lsr_ptrlist_iter iter )
{   *iter = *(--pThis->m_pEnd );  return iter;   }

/**
 * @lsr_ptrlist_unsafe_push_back
 * @brief Adds (pushes) an element to the end of a pointer list object.
 * @details This function is unsafe in that it will not check if there 
 * is enough space allocated.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[in] pPointer - The element (pointer) to add.
 * @return Void.
 * @warning It is the responsibility of the user to ensure that
 *   there is sufficient allocated storage for the request.
 */
lsr_inline void lsr_ptrlist_unsafe_push_back(
  lsr_ptrlist_t *pThis, void *pPointer )
{   *pThis->m_pEnd++ = pPointer;   }

/**
 * @lsr_ptrlist_unsafe_push_backn
 * @brief Adds (pushes) a number of elements to the end
 *   of a pointer list object.
 * @details This function is unsafe in that it will not check if there 
 * is enough space allocated.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[in] pPointer - A pointer to the elements (pointers) to add.
 * @param[in] n - The number of elements to add.
 * @return Void.
 * @warning It is the responsibility of the user to ensure that
 *   there is sufficient allocated storage for the request.
 */
void lsr_ptrlist_unsafe_push_backn(
  lsr_ptrlist_t *pThis, void **pPointer, int n );

/**
 * @lsr_ptrlist_unsafe_pop_backn
 * @brief Pops a number of elements from the end
 *   of a pointer list object.
 * @details This function is unsafe in that it will not check if there 
 * is enough space allocated.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[out] pPointer - A pointer where the elements (pointers) are returned.
 * @param[in] n - The number of elements to return.
 * @return Void.
 * @warning It is the responsibility of the user to ensure that
 *   there is sufficient allocated storage for the request.
 */
void lsr_ptrlist_unsafe_pop_backn(
  lsr_ptrlist_t *pThis, void **pPointer, int n );

/**
 * @lsr_ptrlist_push_back
 * @brief Adds an element to the end of a pointer list object.
 * @details If the pointer list object is at capacity, the storage is extended.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[in] pPointer - The element (pointer) to add.
 * @return 0 on success, else -1 on error.
 */
int  lsr_ptrlist_push_back( lsr_ptrlist_t *pThis, void *pPointer );

/**
 * @lsr_ptrlist_push_back2
 * @brief Adds a list of elements to the end of a pointer list object.
 * @details If the pointer list object is at capacity, the storage is extended.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[in] plist - A pointer to the pointer list object
 *   containing the elements (pointers) to add.
 * @return 0 on success, else -1 on error.
 */
int  lsr_ptrlist_push_back2( lsr_ptrlist_t *pThis, const lsr_ptrlist_t *plist );

/**
 * @lsr_ptrlist_pop_back
 * @brief Pops an element from the end of a pointer list object.
 * @details This decreases the size of the pointer list object by one.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @return The element (pointer).
 * @warning It is expected that the pointer list object is \e not empty.
 */
lsr_inline void * lsr_ptrlist_pop_back( lsr_ptrlist_t *pThis )
{   return *(--pThis->m_pEnd);   }

/**
 * @lsr_ptrlist_pop_front
 * @brief Pops a number of elements from the beginning (front)
 *   of a pointer list object.
 * @details The number of elements returned cannot be greater
 *   than the current size of the pointer list object.
 *   The remaining elements, if any, are then moved to the front.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[out] pPointer - A pointer where the elements (pointers) are returned.
 * @param[in] n - The number of elements to return.
 * @return The number of elements returned.
 */
lsr_inline int lsr_ptrlist_pop_front(
  lsr_ptrlist_t *pThis, void **pPointer, int n )
{
    if ( n > lsr_ptrlist_size( pThis ) ) 
    {
        n = lsr_ptrlist_size( pThis );
    }
    memmove( pPointer, pThis->m_pStore, n * sizeof(void *) );
    if ( n >= lsr_ptrlist_size( pThis ) )
        lsr_ptrlist_clear( pThis );
    else
    {
        memmove( pThis->m_pStore, pThis->m_pStore + n,
          sizeof(void *) * ( pThis->m_pEnd - pThis->m_pStore - n ) );
        pThis->m_pEnd -= n;
    }
    return n;
}

/**
 * @lsr_ptrlist_sort
 * @brief Sorts the elements in a pointer list object.
 * @details The routine uses qsort(3).
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[in] compare - The compare function used to sort.
 * @return Void.
 */
void lsr_ptrlist_sort(
  lsr_ptrlist_t *pThis, int (*compare)(const void *, const void *) );

/**
 * @lsr_ptrlist_swap
 * @brief Exchanges (swaps) the contents of two pointer list objects.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[in] pRhs - A pointer to the initialized object to swap with.
 * @return Void.
 */
void lsr_ptrlist_swap(
  lsr_ptrlist_t *pThis, lsr_ptrlist_t *pRhs );
    
/**
 * @lsr_ptrlist_lower_bound
 * @brief Gets an element or the lower bound in a sorted pointer list object.
 * @details The pointer list object elements must be sorted,
 *   as the routine uses a binary search algorithm.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[in] pKey - A pointer to the key to match.
 * @param[in] compare - The compare function used to match.
 * @return The iterator to the element if found,
 *   else the lower bound.
 *
 * @see lsr_ptrlist_sort
 */
lsr_const_ptrlist_iter lsr_ptrlist_lower_bound(
  const lsr_ptrlist_t *pThis,
  const void *pKey, int (*compare)(const void *, const void *) );

/**
 * @lsr_ptrlist_bfind
 * @brief Finds an element in a sorted pointer list object.
 * @details The pointer list object elements must be sorted,
 *   as the routine uses a binary search algorithm.
 * 
 * @param[in] pThis - A pointer to an initialized pointer list object.
 * @param[in] pKey - A pointer to the key to match.
 * @param[in] compare - The compare function used to match.
 * @return The iterator to the element if found,
 *   else lsr_ptrlist_end if not found.
 *
 * @see lsr_ptrlist_sort
 */
lsr_const_ptrlist_iter lsr_ptrlist_bfind(
  const lsr_ptrlist_t *pThis,
  const void *pKey, int (*compare)(const void *, const void *) );

/**
 * @lsr_ptrlist_for_each
 * @brief Iterates through a list of elements
 *   calling the specified function for each.
 * 
 * @param[in] beg - The beginning pointer list object iterator.
 * @param[in] end - The end iterator.
 * @param[in] fn - The function to be called for each element.
 * @return The number of elements processed.
 */
int lsr_ptrlist_for_each(
  lsr_ptrlist_iter beg, lsr_ptrlist_iter end, gpl_for_each_fn fn );


#ifdef __cplusplus
}
#endif

#endif  /* LSR_PTRLIST_H */

