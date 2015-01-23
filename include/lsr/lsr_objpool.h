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
#ifndef LSR_OBJPOOL_H
#define LSR_OBJPOOL_H

#include <lsr/lsr_ptrlist.h>
#include <lsr/lsr_types.h>

/**
 * @file
 */

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct lsr_objpool_s lsr_objpool_t;
/**
 * @typedef lsr_objpool_objfn
 * @brief A function that the user wants to apply to every pointer in the object pool.
 * Also allows a user provided data for the function.
 * 
 * @param[in] pObj - A pointer to the object to work on.
 * @param[in] param - A pointer to the user data passed into the function.
 * @return Should return 0 on success, not 0 on failure.
 */
typedef int (*lsr_objpool_objfn)( void *pObj, void *param );

/** 
 * @typedef lsr_objpool_newObjFn
 * @brief \b USER \b IMPLEMENTED!
 * @details The user must implement a new object function for the object pool
 * to create pointers.
 * 
 * @return A pointer to the new object.
 */
typedef void *(*lsr_objpool_getNewFn)();

/** 
 * @typedef lsr_objpool_releaseObjFn
 * @brief \b USER \b IMPLEMENTED!
 * @details The user must implement a release object function for the object pool
 * to release objects.
 * 
 * @param[in] pObj - A pointer to release.
 * @return Void.
 */
typedef void (*lsr_objpool_releaseObjFn)( void *pObj );



/**
 * @typedef lsr_objpool_t
 * @brief An object pool object.
 */
struct lsr_objpool_s
{
    int m_chunkSize;
    int m_poolSize;
    lsr_ptrlist_t m_freeList;
    lsr_objpool_getNewFn m_getNew;
    lsr_objpool_releaseObjFn m_releaseObj;
};

/** @lsr_objpool
 * @brief Initializes an object pool object.  
 * @details Chunk size determines how many pointers should be allocated 
 * each time an allocation is needed.  If chunkSize is 0, default is set
 * to 10.
 * 
 * @param[in] pThis - A pointer to an allocated object pool object.
 * @param[in] chunkSize - The number of pointers to allocate.
 * @param[in] getNewFn - A user implemented get new function.
 * @param[in] releaseFn - A user implemented release object function.
 * @return Void.
 * 
 * @see lsr_objpool_d
 */
void lsr_objpool( lsr_objpool_t *pThis, int chunkSize, 
                  lsr_objpool_getNewFn getNewFn, lsr_objpool_releaseObjFn releaseFn );

/** @lsr_objpool
 * @brief Destroys the contents of an initialized object pool object.
 * 
 * @param[in] pThis - A pointer to an initialized object pool object.
 * @return Void.
 * 
 * @see lsr_objpool
 */
void lsr_objpool_d( lsr_objpool_t *pThis );

/** @lsr_objpool_get
 * @brief Gets a pointer from the object pool.
 * @details Will allocate more if there are none left.
 * 
 * @param[in] pThis - A pointer to an initialized object pool object.
 * @return The requested pointer if successful, NULL if not.
 */
void *lsr_objpool_get( lsr_objpool_t *pThis );

/** @lsr_objpool_getMulti
 * @brief Gets an array of pointers from the object pool and stores
 * them in the provided structure.
 * 
 * @param[in] pThis - A pointer to an initialized object pool object.
 * @param[out] pObj - The output array.
 * @param[in] n - The number of pointers to get.
 * @return The number of pointers gotten if successful, 0 if not.
 * 
 */
int lsr_objpool_getMulti( lsr_objpool_t *pThis, void **pObj, int n );

/** @lsr_objpool_recycle
 * @brief Returns the pointer to the pool.
 * 
 * @param[in] pThis - A pointer to an initialized object pool object.
 * @param[in] pObj - A pointer to the pointer to recycle.
 * @return Void.
 */
lsr_inline void lsr_objpool_recycle( lsr_objpool_t *pThis, void * pObj )
{
    if ( pObj )
        lsr_ptrlist_unsafe_push_back( &pThis->m_freeList, pObj );
}

/** @lsr_objpool_recycleMulti
 * @brief Recycles an array of pointers to the object pool from the
 * provided pointer.
 * 
 * @param[in] pThis - A pointer to an initialized object pool object.
 * @param[in] pObj - An array of pointers to return.
 * @param[in] n - The number of pointers to return.
 * @return Void.
 */
lsr_inline void lsr_objpool_recycleMulti( lsr_objpool_t *pThis, void **pObj, int n )
{
    if ( pObj )
        lsr_ptrlist_unsafe_push_backn( &pThis->m_freeList, pObj, n );
}

/** @lsr_objpool_size
 * @brief Gets the current number of pointers \e available in the object pool.
 * 
 * @param[in] pThis - A pointer to an initialized object pool object.
 * @return The size available.
 */
lsr_inline int lsr_objpool_size( const lsr_objpool_t *pThis )
{   return lsr_ptrlist_size( &pThis->m_freeList );  }

/** @lsr_objpool_shrinkTo
 * @brief Shrinks the pool to contain sz pointers.
 * @details If the current \link #lsr_objpool_size size\endlink
 * is less than the size given, this function will do nothing.
 * 
 * @param[in] pThis - A pointer to an initialized object pool object.
 * @param[in] sz - The size to shrink the pool to.
 * @return Void.
 */
lsr_inline void lsr_objpool_shrinkTo( lsr_objpool_t *pThis, int sz )
{
    int curSize = lsr_ptrlist_size( &pThis->m_freeList );
    int i;
    for( i = 0; i < curSize - sz; ++i )
    {
        void *pObj = lsr_ptrlist_pop_back( &pThis->m_freeList );
        pThis->m_releaseObj( pObj );
        --pThis->m_poolSize;
    }
}

/** @lsr_objpool_begin
 * @brief Gets the iterator beginning of a object pool object.
 * 
 * @param[in] pThis - A pointer to an initialized object pool object.
 * @return The iterator beginning of a object pool object.
 */
lsr_inline lsr_ptrlist_iter lsr_objpool_begin( lsr_objpool_t *pThis )
{   return lsr_ptrlist_begin( &pThis->m_freeList ); }

/** @lsr_objpool_end
 * @brief Gets the iterator end of a object pool object.
 * 
 * @param[in] pThis - A pointer to an initialized object pool object.
 * @return The iterator end of a object pool object.
 */
lsr_inline lsr_ptrlist_iter lsr_objpool_end( lsr_objpool_t *pThis )
{   return lsr_ptrlist_end( &pThis->m_freeList ); }

/** @lsr_objpool_applyAll
 * @brief Applies fn to all the pointers in the object pool object.
 * @details Param may be used as a user data to give to the function if needed.
 * 
 * @param[in] pThis - A pointer to an initialized object pool object.
 * @param[in] fn - A pointer to the \link lsr_objpool_objfn function\endlink 
 * to apply to the pointers.
 * @param[in] param - A pointer to the user data to pass into the function.
 * @return Void.
 * 
 */
lsr_inline void lsr_objpool_applyAll( lsr_objpool_t *pThis, lsr_objpool_objfn fn, void *param )
{
    lsr_ptrlist_iter iter;
    for( iter = lsr_objpool_begin( pThis ); iter != lsr_objpool_end( pThis ); ++iter )
        (*fn)( *iter, param );
}

/** @lsr_objpool_poolSize
 * @brief Gets the \e total number of pointers created by the object pool object.
 * @details This includes all pointers that the user may have gotten.
 * 
 * @param[in] pThis - A pointer to an initialized object pool object.
 * @return The total number of pointers created.
 */
lsr_inline int lsr_objpool_poolSize( const lsr_objpool_t *pThis )
{   return pThis->m_poolSize;   }

/** @lsr_objpool_capacity
 * @brief Gets the storage capacity of an object pool object;
 *   \e i.e., how many pointers it may hold.
 * 
 * @param[in] pThis - A pointer to an initialized object pool object.
 * @return The capacity in terms of number of pointers.
 */
lsr_inline int lsr_objpool_capacity( const lsr_objpool_t *pThis )
{   return lsr_ptrlist_capacity( &pThis->m_freeList );  }

#ifdef __cplusplus
}
#endif

#endif
