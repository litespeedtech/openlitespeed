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
#ifndef LSR_STRLIST_H
#define LSR_STRLIST_H


#include <lsr/lsr_ptrlist.h>
#include <lsr/lsr_str.h>
#include <lsr/lsr_pool.h>
#include <lsr/lsr_types.h>


/**
 * @file
 * @note
 * String List - 
 *   The string list object manages a list of lsr_str_t objects through
 *   a list of pointers to them.
 *   Functions are available to allocate, add, and remove elements
 *   to/from the list in a variety of ways, get information about the list,
 *   and also perform functions such as sorting the list.
 *
 * @warning Note the difference between
 *   lsr_strlist_{push,pop,insert,erase} and lsr_strlist_{add,remove,clear,_d}.\n
 *   {push,pop,...} expect user allocated (constructed) lsr_str_t objects.\n
 *   {add,remove,...} allocate lsr_str_t objects given char \*s,
 *   and deallocate the objects when called to.\n
 *   Do \*NOT\* mix these mechanisms.
 */


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef lsr_strlist_iter
 * @brief Iterator for the string list object.
 */
typedef lsr_str_t **  lsr_strlist_iter;

/**
 * @typedef lsr_const_strlist_iter
 * @brief Const iterator for the string list object.
 */
typedef lsr_str_t *const*  lsr_const_strlist_iter;


/**
 * @lsr_strlist
 * @brief Initializes a string list object.
 * @details This object manages pointers to lsr_str_t objects.
 *   An initial size is specified, but the size may change with other
 *   lsr_strlist_* routines.
 * 
 * @param[in] pThis - A pointer to an allocated string list object.
 * @param[in] initSize - The initial size to allocate for the list elements,
 *    which may be 0 specifying an empty initialized string list.
 * @return Void.
 *
 * @see lsr_strlist_new, lsr_strlist_d, lsr_str
 */
lsr_inline void lsr_strlist( lsr_strlist_t *pThis, size_t initSize )
{   lsr_ptrlist( pThis, initSize );   }

/**
 * @lsr_strlist_copy
 * @brief Copies a string list object.
 * 
 * @param[in] pThis - A pointer to an allocated string list object (destination).
 * @param[in] pRhs - A pointer to the string list object to be copied (source).
 * @return Void.
 */
void lsr_strlist_copy( lsr_strlist_t *pThis, const lsr_strlist_t *pRhs );

/**
 * @lsr_strlist_new
 * @brief Creates a new string list object.
 * @details The routine allocates and initializes an object
 *   which manages pointers to lsr_str_t objects.
 *   An initial size is specified, but the size may change with other
 *   lsr_strlist_* routines.
 * 
 * @param[in] initSize - The initial size to allocate for the list elements,
 *    which may be 0 specifying an empty initialized string list.
 * @return A pointer to an initialized string list object.
 *
 * @see lsr_strlist, lsr_strlist_delete
 */
lsr_inline lsr_strlist_t * lsr_strlist_new( size_t initSize )
{
    lsr_strlist_t * pThis;
    if ( ( pThis = (lsr_strlist_t *)
      lsr_palloc( sizeof(*pThis) ) ) != NULL )
    {
        lsr_strlist( pThis, initSize );
    }
    return pThis;
}

/**
 * @lsr_strlist_d
 * @brief Deletes the elements and destroys the contents of a string list object.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return Void.
 *
 * @see lsr_strlist
 */
void lsr_strlist_d( lsr_strlist_t *pThis );

/**
 * @lsr_strlist_delete
 * @brief Deletes the elements and destroys the contents of a string list object,
 *   then deletes the object.
 * @details The object should have been created with a previous
 *   successful call to lsr_strlist_new.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return Void.
 *
 * @see lsr_strlist_new
 */
lsr_inline void lsr_strlist_delete( lsr_strlist_t *pThis )
{   lsr_strlist_d( pThis );  lsr_pfree( pThis );   }

/* derived from lsr_ptrlist */

/**
 * @lsr_strlist_resize
 * @brief Changes the storage or data size in a string list object.
 * @details If the new size is greater than the current capacity,
 *   the storage is extended with the existing data remaining intact.
 *   If the new size is less, the data size is truncated.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] sz - The new size in terms of number of elements.
 * @return 0 on success, else -1 on error.
 */
lsr_inline int lsr_strlist_resize( lsr_strlist_t *pThis, size_t sz )
{   return lsr_ptrlist_resize( pThis, sz );   }

/**
 * @lsr_strlist_size
 * @brief Gets the current size of a string list object.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return The size in terms of number of elements.
 */
lsr_inline ssize_t lsr_strlist_size( const lsr_strlist_t *pThis )
{   return lsr_ptrlist_size( pThis );   }

/**
 * @lsr_strlist_empty
 * @brief Specifies whether or not a string list object contains any elements.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return True if empty, else false.
 */
lsr_inline bool lsr_strlist_empty( const lsr_strlist_t *pThis )
{   return lsr_ptrlist_empty( pThis );   }

/**
 * @lsr_strlist_full
 * @brief Specifies whether or not a string list object is at full capacity.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return True if full, else false.
 */
lsr_inline bool lsr_strlist_full( const lsr_strlist_t *pThis )
{   return lsr_ptrlist_full( pThis );   }

/**
 * @lsr_strlist_capacity
 * @brief Gets the storage capacity of a string list object;
 *   \e i.e., how many elements it may hold.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return The capacity in terms of number of elements.
 */
lsr_inline size_t lsr_strlist_capacity( const lsr_strlist_t *pThis )
{   return lsr_ptrlist_capacity( pThis );   }

/**
 * @lsr_strlist_begin
 * @brief Gets the iterator beginning of a string list object.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return The iterator beginning of a string list object.
 */
lsr_inline lsr_strlist_iter lsr_strlist_begin( lsr_strlist_t *pThis )
{   return (lsr_strlist_iter)lsr_ptrlist_begin( pThis );   }

/**
 * @lsr_strlist_end
 * @brief Gets the iterator end of a string list object.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return The iterator end of a string list object.
 */
lsr_inline lsr_strlist_iter lsr_strlist_end( lsr_strlist_t *pThis )
{   return (lsr_strlist_iter)lsr_ptrlist_end( pThis );   }

/**
 * @lsr_strlist_back
 * @brief Gets the last element in a string list object.
 * @details The routine does \e not change anything in the object.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return The last element in a string list object.
 */
lsr_inline lsr_str_t * lsr_strlist_back( const lsr_strlist_t *pThis )
{   return (lsr_str_t *)lsr_ptrlist_back( pThis );   }

/**
 * @lsr_strlist_reserve
 * @brief Reserves (allocates) storage in a string list object.
 * @details If the object currently contains data elements,
 *   the storage is extended with the existing data remaining intact.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] sz - The new size in terms of number of elements.
 * @return 0 on success, else -1 on error.
 */
lsr_inline int lsr_strlist_reserve( lsr_strlist_t *pThis, size_t sz )
{   return lsr_ptrlist_reserve( pThis, sz );   }

/**
 * @lsr_strlist_grow
 * @brief Extends (increases) the storage capacity of a string list object.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] sz - The size increment in terms of number of elements.
 * @return 0 on success, else -1 on error.
 */
lsr_inline int lsr_strlist_grow( lsr_strlist_t *pThis, size_t sz )
{   return lsr_ptrlist_grow( pThis, sz );   }
 
/**
 * @lsr_strlist_erase
 * @brief Erases (deletes) an element from a string list object.
 * @details This decreases the size of the string list object by one.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in,out] iter - Iterator specifying the element to erase.
 * @return The same iterator which now specifies the previous last element in
 *   the string list object.
 * @note The order of the elements in the string list object
 *   may have been changed.
 *   This may be significant if the elements were expected to be in a
 *   specific order.
 * @note Note also that when erasing the last (\e i.e., end) element
 *   in the list, upon return the iterator specifies the newly erased
 *   element, which is thus invalid.
 */
lsr_inline lsr_strlist_iter lsr_strlist_erase(
  lsr_strlist_t *pThis, lsr_strlist_iter iter )
{
    return (lsr_strlist_iter)lsr_ptrlist_erase( pThis, (lsr_ptrlist_iter)iter );
}

/**
 * @lsr_strlist_unsafe_push_back
 * @brief Adds (pushes) an element to the end of a string list object.
 * @details This function is unsafe in that it will not check if there is enough space
 * allocated.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] pPointer - The element to add.
 * @return Void.
 * @warning It is the responsibility of the user to ensure that
 *   there is sufficient allocated storage for the request.
 */
lsr_inline void lsr_strlist_unsafe_push_back(
  lsr_strlist_t *pThis, lsr_str_t *pPointer )
{   lsr_ptrlist_unsafe_push_back( pThis, (void *)pPointer );   }

/**
 * @lsr_strlist_unsafe_push_backn
 * @brief Adds (pushes) a number of elements to the end of a string list object.
 * @details This function is unsafe in that it will not check if there is enough space
 * allocated.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] pPointer - A pointer to the elements to add.
 * @param[in] n - The number of elements to add.
 * @return Void.
 * @warning It is the responsibility of the user to ensure that
 *   there is sufficient allocated storage for the request.
 */
lsr_inline void lsr_strlist_unsafe_push_backn(
  lsr_strlist_t *pThis, lsr_str_t **pPointer, int n )
{   lsr_ptrlist_unsafe_push_backn( pThis, (void **)pPointer, n );   }

/**
 * @lsr_strlist_unsafe_pop_backn
 * @brief Pops a number of elements from the end of a string list object.
 * @details This function is unsafe in that it will not check if there is enough space
 * allocated.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[out] pPointer - A pointer where the elements are returned.
 * @param[in] n - The number of elements to return.
 * @return Void.
 * @warning It is the responsibility of the user to ensure that
 *   there is sufficient allocated storage for the request.
 */
lsr_inline void lsr_strlist_unsafe_pop_backn(
  lsr_strlist_t *pThis, lsr_str_t **pPointer, int n )
{   lsr_ptrlist_unsafe_pop_backn( pThis, (void **)pPointer, n );   }

/**
 * @lsr_strlist_push_back
 * @brief Adds an element to the end of a string list object.
 * @details If the string list object is at capacity, the storage is extended.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] pPointer - The element to add.
 * @return 0 on success, else -1 on error.
 */
lsr_inline int lsr_strlist_push_back(
  lsr_strlist_t *pThis, lsr_str_t *pPointer )
{   return lsr_ptrlist_push_back( pThis, (void *)pPointer );   }

/**
 * @lsr_strlist_push_back2
 * @brief Adds a list of elements to the end of a string list object.
 * @details If the string list object is at capacity, the storage is extended.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] plist - A pointer to the string list object
 *   containing the elements to add.
 * @return 0 on success, else -1 on error.
 */
lsr_inline int lsr_strlist_push_back2(
  lsr_strlist_t *pThis, const lsr_strlist_t *plist )
{   return lsr_ptrlist_push_back2( pThis, plist );   }

/**
 * @lsr_strlist_pop_back
 * @brief Pops an element from the end of a string list object.
 * @details This decreases the size of the string list object by one.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return The element.
 * @warning It is expected that the string list object is \e not empty.
 */
lsr_inline lsr_str_t * lsr_strlist_pop_back( lsr_strlist_t *pThis )
{   return (lsr_str_t *)lsr_ptrlist_pop_back( pThis );   }

/**
 * @lsr_strlist_pop_front
 * @brief Pops a number of elements from the beginning (front)
 *   of a string list object.
 * @details The number of elements returned cannot be greater
 *   than the current size of the string list object.
 *   After popping,
 *   the remaining elements, if any, are moved to the front.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[out] pPointer - A pointer where the elements are returned.
 * @param[in] n - The number of elements to return.
 * @return The number of elements returned.
 */
lsr_inline int lsr_strlist_pop_front(
  lsr_strlist_t *pThis, lsr_str_t **pPointer, int n )
{   return lsr_ptrlist_pop_front( pThis, (void **)pPointer, n );   }

/* specialized lsr_strlist */

/**
 * @lsr_strlist_release_objects
 * @brief Deletes all elements from a string list object, and empties the object.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return Void.
 * @warning The elements of the string list object must have been allocated
 *   and added through previous successful calls to lsr_strlist_add.
 *
 * @see lsr_strlist_add, lsr_strlist_remove, lsr_strlist_clear
 */
lsr_inline void lsr_strlist_release_objects( lsr_strlist_t *pThis )
{
    lsr_strlist_iter iter = lsr_strlist_begin( pThis );
    for( ; iter < lsr_strlist_end( pThis ); ++iter )
    {
        if ( *iter )
            lsr_str_delete( *iter );
    }
    lsr_ptrlist_clear( pThis );
}

/**
 * @lsr_strlist_for_each
 * @brief Iterates through a list of elements
 *   calling the specified function for each.
 * 
 * @param[in] beg - The beginning string list object iterator.
 * @param[in] end - The end iterator.
 * @param[in] fn - The function to be called for each element.
 * @return The number of elements processed.
 */
lsr_inline int lsr_strlist_for_each(
  lsr_strlist_iter beg, lsr_strlist_iter end, gpl_for_each_fn fn )
{   return lsr_ptrlist_for_each( (void **)beg, (void **)end, fn );   }

/**
 * @lsr_strlist_add
 * @brief Creates and adds an element to the end of a string list object.
 * @details If the string list object is at capacity, the storage is extended.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] pStr - A char \* used to create the lsr_str_t element.
 * @param[in] len - The length of characters at \e pStr.
 * @return A pointer to the created lsr_str_t element on success,
 *   else NULL on error.
 * @note lsr_strlist_add and lsr_strlist_remove should be used together.
 *
 * @see lsr_strlist_remove
 */
const lsr_str_t * lsr_strlist_add(
  lsr_strlist_t *pThis, const char *pStr, int len );

/**
 * @lsr_strlist_remove
 * @brief Removes and deletes an element from a string list object.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] pStr - A char \* used to find the lsr_str_t element.
 * @return Void.
 * @note lsr_strlist_add and lsr_strlist_remove should be used together.
 *
 * @see lsr_strlist_add
 */
void lsr_strlist_remove( lsr_strlist_t *pThis, const char *pStr );

/**
 * @lsr_strlist_clear
 * @brief Deletes all elements from a string list object, and empties the object.
 * @details It is identical to lsr_strlist_release_objects.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return Void.
 * @warning The elements of the string list object must have been allocated
 *   and added through previous successful calls to lsr_strlist_add.
 *
 * @see lsr_strlist_add, lsr_strlist_remove, lsr_strlist_release_objects
 */
void lsr_strlist_clear( lsr_strlist_t *pThis );

/**
 * @lsr_strlist_sort
 * @brief Sorts the elements in a string list object.
 * @details The routine uses qsort(3) with strcmp(3) as the compare function.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @return Void.
 */
void lsr_strlist_sort( lsr_strlist_t *pThis );

/**
 * @lsr_strlist_insert
 * @brief Adds an element to a string list object, then sorts the object.
 * @details If the string list object is at capacity, the storage is extended.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] pDir - The element to add.
 * @return Void.
 * @note Note that the \e pDir parameter to lsr_strlist_insert
 *   must be a previously allocated lsr_str_t element.
 *   In contrast, lsr_strlist_add takes a char \*, and creates a new
 *   lsr_str_t element to add.
 *
 * @see lsr_strlist_sort, lsr_strlist_add
 */
void lsr_strlist_insert( lsr_strlist_t *pThis, lsr_str_t *pDir );

/**
 * @lsr_strlist_find
 * @brief Finds an element in a string list object.
 * @details The routine uses a linear search algorithm and strcmp(3).
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] pStr - The key to match.
 * @return The element if found, else NULL if not found.
 *
 * @see lsr_strlist_bfind
 * @note If the string list object is sorted,
 *   lsr_strlist_bfind may be more efficient.
 */
const lsr_str_t * lsr_strlist_find(
  const lsr_strlist_t *pThis, const char *pStr );

/**
 * @lsr_strlist_lower_bound
 * @brief Gets an element or the lower bound in a sorted string list object.
 * @details The string list object elements must be sorted,
 *   as the routine uses a binary search algorithm.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] pStr - The key to match.
 * @return The element if found, else the lower bound.
 *
 * @see lsr_strlist_sort
 */
lsr_const_strlist_iter lsr_strlist_lower_bound(
  const lsr_strlist_t *pThis, const char *pStr );

/**
 * @lsr_strlist_bfind
 * @brief Finds an element in a sorted string list object.
 * @details The string list object elements must be sorted,
 *   as the routine uses a binary search algorithm.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] pStr - The key to match.
 * @return The element if found, else NULL if not found.
 *
 * @see lsr_strlist_find, lsr_strlist_sort
 * @note If the string list object is not sorted,
 *   lsr_strlist_find may be used instead.
 */
lsr_str_t * lsr_strlist_bfind(
  const lsr_strlist_t *pThis, const char *pStr );

/**
 * @lsr_strlist_split
 * @brief Parses (splits) a buffer into tokens which are added to the specified
 *   string list object.
 * 
 * @param[in] pThis - A pointer to an initialized string list object.
 * @param[in] pBegin - A pointer to the beginning of the string to parse.
 * @param[in] pEnd - A pointer to the end of the string.
 * @param[in] delim - The field delimiters (cannot be an empty string).
 * @return The new size of the string list object
 *   in terms of number of elements.
 */
int lsr_strlist_split(
  lsr_strlist_t *pThis, const char *pBegin, const char *pEnd, const char *delim );


#ifdef __cplusplus
}
#endif

#endif  /* LSR_STRLIST_H */
