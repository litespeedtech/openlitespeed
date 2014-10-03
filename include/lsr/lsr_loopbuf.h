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
#ifndef LSR_LOOPBUF_H
#define LSR_LOOPBUF_H

#include <lsr/lsr_types.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

/**
 * @file
 * @note
 * Loopbuf - A loopbuf is a mechanism which manages buffer allocation as long as
 *      the user ensures there is enough space (reserve, guarantee, append).\n
 *      It also loops around to the beginning of the allocation if there is space available
 *      in the front (pop front, pop front to).\n\n
 * The remainder of this document refers to the lsr implementation of loopbuf.\n\n
 * lsr_loopbuf_t defines a loopbuf object.\n
 *      Functions prefixed with lsr_loopbuf_* use a lsr_loopbuf_t \* parameter.\n\n
 * lsr_xloopbuf_t is a wrapper around lsr_loopbuf_t, adding a pointer to the session pool (xpool).\n
 *      Functions prefixed with lsr_xloopbuf_* use a lsr_xloopbuf_t \* parameter.\n\n
 * If a user wants to use the session pool with loopbuf, there are two options:
 *      @li Use lsr_loopbuf_x* functions with lsr_loopbuf_t.
 *      This option should be used if the user can pass the xpool pointer every time a
 *      growing/shrinking function is called.
 * 
 *      @li Use lsr_xloopbuf_* functions with lsr_xloopbuf_t.
 *      This structure takes in the session pool as a parameter during initialization and
 *      stores the pointer.  This should be used if the user cannot guarantee
 *      having the xpool pointer every time a function needs it.
 */

#define LSR_LOOPBUFUNIT 64

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct lsr_loopbuf_s lsr_loopbuf_t;
typedef struct lsr_xloopbuf_s lsr_xloopbuf_t;

/**
 * @typedef lsr_loopbuf_t
 * @brief An auto allocation buffer that allows looping if the front is popped off.
 */
struct lsr_loopbuf_s
{
    char           *m_pBuf;
    char           *m_pBufEnd;
    char           *m_pHead;
    char           *m_pEnd;
    int             m_iCapacity;
};

/**
 * @typedef lsr_xloopbuf_t
 * @brief A struct that contains a session pool pointer as well as lsr_loopbuf_t
 * @details This structure can be useful for when the user knows that the
 * structure will only last an HTTP Session and the user cannot guarantee that
 * he/she will have a pointer to the session pool every time it is needed.
 */
struct lsr_xloopbuf_s
{
    lsr_loopbuf_t   m_loopbuf;
    lsr_xpool_t    *m_pPool;
};

/** @lsr_loopbuf_xnew
 * @brief Create a new loopbuf structure with an initial capacity from
 * a session pool.
 * 
 * @param[in] size - The initial capacity for the loopbuf (bytes).
 * @param[in] pool - A pointer to the session pool to allocate from.
 * @return A loopbuf structure if successful, NULL if not.
 * 
 * @see lsr_loopbuf_xdelete
 */
lsr_loopbuf_t *lsr_loopbuf_xnew( int size, lsr_xpool_t *pool );

/** @lsr_loopbuf_x
 * @brief Initializes a given loopbuf to an initial capacity from
 * a session pool.
 * 
 * @param[in] pThis - A pointer to an allocated loopbuf object.
 * @param[in] size - The initial capacity for the loopbuf (bytes).
 * @param[in] pool - A pointer to the session pool to allocate from.
 * @return The loopbuf structure if successful, NULL if not.
 * 
 * @see lsr_loopbuf_xd
 */
lsr_loopbuf_t *lsr_loopbuf_x( lsr_loopbuf_t *pThis, int size, lsr_xpool_t *pool );

/** @lsr_loopbuf_xd
 * @brief Destroys the loopbuf.  DOES NOT FREE pThis!
 * @details Use with lsr_loopbuf_x.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] pool - A pointer to the session pool.
 * @return Void.
 * 
 * @see lsr_loopbuf_x
 */
void lsr_loopbuf_xd( lsr_loopbuf_t *pThis, lsr_xpool_t *pool );

/** @lsr_loopbuf_xdelete
 * @brief Deletes the loopbuf, opposite of \link #lsr_loopbuf_xnew loopbuf xnew \endlink
 * @details If a loopbuf was created with \link #lsr_loopbuf_xnew loopbuf xnew, \endlink
 * it must be deleted with \link #lsr_loopbuf_xdelete loopbuf xdelete. \endlink
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] pool - A pointer to the session pool to delete from.
 * @return Void.
 * 
 * @see lsr_loopbuf_xnew
 */
void lsr_loopbuf_xdelete( lsr_loopbuf_t *pThis, lsr_xpool_t *pool );

/** @lsr_loopbuf_new
 * @brief Create a new loopbuf structure with an initial capacity.
 * 
 * @param[in] size - The initial capacity for the loopbuf (bytes).
 * @return A loopbuf structure if successful, NULL if not.
 * 
 * @see lsr_loopbuf_delete
 */
lsr_loopbuf_t *lsr_loopbuf_new( int size );

/** @lsr_loopbuf
 * @brief Initializes a given loopbuf to an initial capacity.
 * 
 * @param[in] pThis - A pointer to an allocated loopbuf object.
 * @param[in] size - The initial capacity for the loopbuf (bytes).
 * @return The loopbuf object if successful, NULL if not.
 * 
 * @see lsr_loopbuf_d
 */
lsr_loopbuf_t *lsr_loopbuf( lsr_loopbuf_t *pThis, int size );

/** @lsr_loopbuf_d
 * @brief Destroys the loopbuf.  DOES NOT FREE pThis!
 * @details Use with lsr_loopbuf.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return Void.
 * 
 * @see lsr_loopbuf
 */
lsr_inline void lsr_loopbuf_d( lsr_loopbuf_t *pThis )
{   lsr_loopbuf_xd( pThis, NULL );  }

/** @lsr_loopbuf_delete
 * @brief Deletes the loopbuf, opposite of \link #lsr_loopbuf_new loopbuf new \endlink
 * @details If a loopbuf was created with \link #lsr_loopbuf_new loopbuf new. \endlink
 * it must be deleted with \link #lsr_loopbuf_delete loopbuf delete; \endlink
 * otherwise, there will be a memory leak.  This function does garbage collection 
 * for the internal constructs and will free pThis.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return Void.
 * 
 * @see lsr_loopbuf_new
 */
void lsr_loopbuf_delete( lsr_loopbuf_t *pThis );

/** @lsr_loopbuf_xreserve
 * @brief Reserves size total bytes for the loopbuf from the session pool.
 * If size is lower than the current size, it will not shrink.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] size - The total number of bytes to reserve.
 * @param[in] pool - A pointer to the session pool to allocate from.
 * @return 0 on success, -1 on failure.
 */
int lsr_loopbuf_xreserve( lsr_loopbuf_t *pThis, int size, lsr_xpool_t *pool );

/** @lsr_loopbuf_xappend
 * @brief Appends the loopbuf with the specified buffer.
 * @details This function appends the loopbuf with the specified buffer.  It will check
 * if there is enough space in the loopbuf and will grow the loopbuf from the
 * session pool if there is not enough space.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] pBuf - A pointer to the buffer to append.
 * @param[in] size - The size of pBuf.
 * @param[in] pool - A pointer to the session pool to allocate from.
 * @return The size appended or -1 if it failed.
 */
int lsr_loopbuf_xappend( 
            lsr_loopbuf_t *pThis, const char *pBuf, int size, lsr_xpool_t *pool );

/** @lsr_loopbuf_xguarantee
 * @brief Guarantees size more bytes for the loopbuf.  If size is lower than the
 * available amount, it will not allocate any more.  Allocates from the given session pool.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] size - The number of bytes to guarantee.
 * @param[in] pool - A pointer to the session pool to allocate from.
 * @return 0 on success, -1 on failure.
 */
int lsr_loopbuf_xguarantee( lsr_loopbuf_t *pThis, int size, lsr_xpool_t *pool );

/** @lsr_loopbuf_xstraight
 * @brief Straighten the loopbuf.
 * @details If the loopbuf is split in two parts due to looping, this function
 * makes it one whole again.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] pool - A pointer to the pool to allocate from.
 * @return Void.
 */
void lsr_loopbuf_xstraight( lsr_loopbuf_t *pThis, lsr_xpool_t *pool );

/** @lsr_loopbuf_available
 * @brief Gets the number of bytes left in the loopbuf.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return The number of bytes left.
 */
int lsr_loopbuf_available( const lsr_loopbuf_t *pThis );

/** @lsr_loopbuf_contiguous
 * @brief Gets the number of contiguous bytes left in the loopbuf.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return The number of contiguous bytes left.
 */
int lsr_loopbuf_contiguous( const lsr_loopbuf_t *pThis );

/** @lsr_loopbuf_used
 * @brief Tells the loopbuf that the user used size more bytes from
 * the end of the loopbuf.
 * @details Do not call this after the calls 
 * \link #lsr_loopbuf_append loopbuf append,\endlink
 * \link #lsr_loopbuf_xappend loopbuf xappend.\endlink
 * It will be updated within those functions.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] size - The size to add.
 * @return Void.
 */
void lsr_loopbuf_used( lsr_loopbuf_t *pThis, int size );

/** @lsr_loopbuf_reserve
 * @brief Reserves size total bytes for the loopbuf.  If size is lower than the current
 * size, it will not shrink.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] size - The total number of bytes to reserve.
 * @return 0 on success, -1 on failure.
 */
lsr_inline int lsr_loopbuf_reserve( lsr_loopbuf_t *pThis, int size )
{   return lsr_loopbuf_xreserve( pThis, size, NULL );   }

/** @lsr_loopbuf_append
 * @brief Appends the loopbuf with the specified buffer.
 * @details This function appends the loopbuf with the specified buffer.  It will check
 * if there is enough space in the loopbuf and will grow the loopbuf if there
 * is not enough space.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] pBuf - A pointer to the given buffer to append.
 * @param[in] size - The size of pBuf.
 * @return The size appended or -1 if it failed.
 */
lsr_inline int lsr_loopbuf_append( lsr_loopbuf_t *pThis, const char *pBuf, int size )
{   return lsr_loopbuf_xappend( pThis, pBuf, size, NULL );  }

/** @lsr_loopbuf_guarantee
 * @brief Guarantees size more bytes for the loopbuf.  If size is lower than the
 * available amount, it will not allocate any more.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] size - The number of bytes to guarantee.
 * @return 0 on success, -1 on failure.
 */
lsr_inline int lsr_loopbuf_guarantee( lsr_loopbuf_t *pThis, int size )
{   return lsr_loopbuf_xguarantee( pThis, size, NULL ); }

/** @lsr_loopbuf_straight
 * @brief Straighten the loopbuf.
 * @details If the loopbuf is split in two parts due to looping, this function
 * makes it one whole again.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return Void.
 */
lsr_inline void lsr_loopbuf_straight( lsr_loopbuf_t *pThis )
{   return lsr_loopbuf_xstraight( pThis, NULL );    }

/** @lsr_loopbuf_move_to
 * @brief Moves the front size bytes from pThis to pBuf.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[out] pBuf - A pointer to the allocated buffer to copy to.
 * @param[in] size - The number of bytes to attempt to copy.
 * @return The number of bytes copied.
 */
int lsr_loopbuf_move_to( lsr_loopbuf_t *pThis, char * pBuf, int size );

/** @lsr_loopbuf_pop_front
 * @brief Pops the front size bytes from pThis.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] size - The number of bytes to pop.
 * @return The number of bytes popped.
 */
int lsr_loopbuf_pop_front( lsr_loopbuf_t *pThis, int size );

/** @lsr_loopbuf_pop_back
 * @brief Pops the ending size bytes from pThis.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] size - The number of bytes to pop.
 * @return The number of bytes popped.
 */
int lsr_loopbuf_pop_back( lsr_loopbuf_t *pThis, int size );

/** @lsr_loopbuf_swap
 * @brief Swaps the contents of the two loopbufs.
 * @param[in,out] lhs - A pointer to an initialized loopbuf object.
 * @param[in,out] rhs - A pointer to another initialized loopbuf object.
 * @return Void.
 */
void lsr_loopbuf_swap( lsr_loopbuf_t *lhs, lsr_loopbuf_t *rhs );

/** @lsr_loopbuf_update
 * @brief Update the loopbuf at the offset to the contents of pBuf.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] offset - The starting offset to update in the loopbuf.
 * @param[in] pBuf - A pointer to the buffer.
 * @param[in] size - The amount of bytes to copy.
 * @return Void.
 */
void lsr_loopbuf_update( 
            lsr_loopbuf_t *pThis, int offset, const char * pBuf, int size );

/** @lsr_loopbuf_search
 * @brief Searches the loopbuf starting at the offset for the pattern.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] offset - The offset to start the search at.
 * @param[in] accept - A pointer to the pattern to match.
 * @param[in] acceptLen - The length of the pattern to match.
 * @return The pointer to the pattern in the loopbuf if found, NULL if not.
 */
char *lsr_loopbuf_search( 
            lsr_loopbuf_t *pThis, int offset, const char *accept, int acceptLen );

/** @lsr_loopbuf_block_size
 * @brief Gets the front block size.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return The front block size.
 */
lsr_inline int lsr_loopbuf_block_size( const lsr_loopbuf_t *pThis )
{   
    if ( pThis->m_pHead > pThis->m_pEnd )
        return pThis->m_pBufEnd - pThis->m_pHead;
    return pThis->m_pEnd - pThis->m_pHead;
}

/** @lsr_loopbuf_size
 * @brief Gets the size used in the loopbuf so far.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return The size used.
 */
lsr_inline int lsr_loopbuf_size( const lsr_loopbuf_t *pThis )
{ 
    register int ret = pThis->m_pEnd - pThis->m_pHead;
    if ( ret >= 0 )
        return ret;
    return ret + pThis->m_iCapacity; 
}

/** @lsr_loopbuf_capacity
 * @brief Gets the total capacity of the loopbuf.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return The total capacity in terms of bytes.
 */
lsr_inline int lsr_loopbuf_capacity( const lsr_loopbuf_t *pThis )
{   return pThis->m_iCapacity;  }

/** @lsr_loopbuf_empty
 * @brief Specifies whether or not the loopbuf is empty.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return Non-zero if empty, 0 if not.
 */
lsr_inline int lsr_loopbuf_empty( const lsr_loopbuf_t *pThis )
{   return ( pThis->m_pHead == pThis->m_pEnd );  }

/** @lsr_loopbuf_full
 * @brief Specifies whether or not the loopbuf is full.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return Non-zero if full, 0 if not.
 */
lsr_inline int lsr_loopbuf_full( const lsr_loopbuf_t *pThis )
{   return  lsr_loopbuf_size( pThis ) == pThis->m_iCapacity - 1; }

/** @lsr_loopbuf_begin
 * @brief Gets a pointer to the beginning of the used data.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return A pointer to the beginning.
 */
lsr_inline char *lsr_loopbuf_begin( const lsr_loopbuf_t *pThis )
{   return pThis->m_pHead;  }

/** @lsr_loopbuf_end
 * @brief Gets a pointer to the end of the used data.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return A pointer to the end.
 */
lsr_inline char *lsr_loopbuf_end( const lsr_loopbuf_t *pThis )
{   return pThis->m_pEnd;  }

/** @lsr_loopbuf_get_pointer
 * @brief Gets a pointer of a given offset from the loopbuf.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] offset - The offset from the beginning of the used data.
 * @return The pointer associated with the offset.
 */
lsr_inline char *lsr_loopbuf_get_pointer( const lsr_loopbuf_t *pThis, int offset )
{   
    return pThis->m_pBuf 
            + (pThis->m_pHead - pThis->m_pBuf + offset) 
            % pThis->m_iCapacity;
}

/** @lsr_loopbuf_get_offset
 * @brief Given a pointer, calculates the offset relative to the start of
 * the used data.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] p - The pointer to compare.
 * @return The offset.
 */
lsr_inline int lsr_loopbuf_get_offset( const lsr_loopbuf_t *pThis, const char *p )
{
    if ( p < pThis->m_pBuf || p >= pThis->m_pBufEnd )
        return -1;
    return ( p - pThis->m_pHead + pThis->m_iCapacity ) % pThis->m_iCapacity;
}

/** @lsr_loopbuf_clear
 * @brief Clears the loopbuf to 0 bytes used.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return Void.
 */
lsr_inline void lsr_loopbuf_clear( lsr_loopbuf_t *pThis )
{   pThis->m_pHead = pThis->m_pEnd = pThis->m_pBuf;    }

/** @lsr_loopbuf_append_unsafe
 * @brief Appends the loopbuf with a character.
 * @details This function is unsafe in that it will not check if there is enough space
 * allocated.  This function exists to speed up the append process if the user can
 * guarantee that the loopbuf is large enough.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] ch - The character to append to the loopbuf.
 * @return Void.
 */
lsr_inline void lsr_loopbuf_append_unsafe( lsr_loopbuf_t *pThis, char ch ) 
{
    *((pThis->m_pEnd)++) = ch;
    if ( pThis->m_pEnd == pThis->m_pBufEnd )
        pThis->m_pEnd = pThis->m_pBuf;
}

/** @lsr_loopbuf_inc
 * @brief Increments the pointer.  If it is currently pointing to the end of the loopbuf,
 * then the increment will point to the beginning of the loopbuf.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in,out] pPos - A pointer to the pointer to increment.
 * @return The incremented pointer.
 */
lsr_inline char *lsr_loopbuf_inc( lsr_loopbuf_t *pThis, char **pPos )
{
    assert( pPos );
    if ( ++*pPos == pThis->m_pBufEnd  )
        *pPos = pThis->m_pBuf ;
    return *pPos ;
}

//NOTICE: The number of segments must match the count of iovec structs in order to use iov_insert.
/** @lsr_loopbuf_get_num_segments
 * @brief Gets the number of segments the data is stored as.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return The number of segments the data is stored as.
 */
lsr_inline int lsr_loopbuf_get_num_segments( lsr_loopbuf_t *pThis )
{   return pThis->m_pEnd > pThis->m_pHead ? 1 : 2;  }

/** @lsr_loopbuf_iov_insert
 * @brief Inserts the contents of the loopbuf into the vector.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @param[in] vect - The vector to copy the content pointer to.
 * @param[in] count - The number of segments the data is stored as.
 * @return 0 on success, -1 on failure.
 */
int lsr_loopbuf_iov_insert( lsr_loopbuf_t *pThis, struct iovec *vect, int count );





/** @lsr_xloopbuf_new
 * @brief Create a new xloopbuf structure with an initial capacity from
 * a session pool.
 * 
 * @param[in] size - The initial capacity for the xloopbuf (bytes).
 * @param[in] pool - A pointer to the session pool to allocate from.
 * @return An xloopbuf structure if successful, NULL if not.
 * 
 * @see lsr_xloopbuf_delete
 */
lsr_xloopbuf_t *lsr_xloopbuf_new( int size, lsr_xpool_t *pool );

/** @lsr_xloopbuf
 * @brief Initializes a given xloopbuf to an initial capacity from
 * a session pool.
 * 
 * @param[in] pThis - A pointer to an allocated xloopbuf object.
 * @param[in] size - The initial capacity for the xloopbuf (bytes).
 * @param[in] pool - A pointer to the session pool to allocate from.
 * @return The xloopbuf structure if successful, NULL if not.
 * 
 * @see lsr_xloopbuf_d
 */
lsr_xloopbuf_t *lsr_xloopbuf( lsr_xloopbuf_t *pThis, int size, lsr_xpool_t *pool );

/** @lsr_xloopbuf_d
 * @brief Destroys the xloopbuf.  DOES NOT FREE pThis!
 * @details Use with lsr_xloopbuf.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return Void.
 * 
 * @see lsr_xloopbuf
 */
void lsr_xloopbuf_d( lsr_xloopbuf_t *pThis );

/** @lsr_xloopbuf_delete
 * @brief Deletes the xloopbuf, opposite of \link #lsr_xloopbuf_new xloopbuf new \endlink
 * @details If a xloopbuf was created with \link #lsr_xloopbuf_new xloopbuf new, \endlink
 * it must be deleted with \link #lsr_xloopbuf_delete xloopbuf delete. \endlink
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return Void.
 * 
 * @see lsr_xloopbuf_new
 */
void lsr_xloopbuf_delete( lsr_xloopbuf_t *pThis );

/** @lsr_xloopbuf_reserve
 * @brief Reserves size total bytes for the xloopbuf.  If size is lower than the current
 * size, it will not shrink.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in] size - The total number of bytes to reserve.
 * @return 0 on success, -1 on failure.
 */
lsr_inline int lsr_xloopbuf_reserve( lsr_xloopbuf_t *pThis, int size )
{   return lsr_loopbuf_xreserve( &pThis->m_loopbuf, size, pThis->m_pPool ); }

/** @lsr_xloopbuf_append
 * @brief Appends the xloopbuf with the specified buffer.
 * @details This function appends the xloopbuf with the specified buffer.  It will check
 * if there is enough space in the xloopbuf and will grow the xloopbuf if there
 * is not enough space.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in] pBuf - A pointer to the given buffer to append.
 * @param[in] size - The size of pBuf.
 * @return The size appended or -1 if it failed.
 */
lsr_inline int lsr_xloopbuf_append( lsr_xloopbuf_t *pThis, const char *pBuf, int size )
{   return lsr_loopbuf_xappend( &pThis->m_loopbuf, pBuf, size, pThis->m_pPool );    }

/** @lsr_xloopbuf_guarantee
 * @brief Guarantees size more bytes for the xloopbuf.  If size is lower than the
 * available amount, it will not allocate any more.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in] size - The number of bytes to guarantee.
 * @return 0 on success, -1 on failure.
 */
lsr_inline int lsr_xloopbuf_guarantee( lsr_xloopbuf_t *pThis, int size )
{   return lsr_loopbuf_xguarantee( &pThis->m_loopbuf, size, pThis->m_pPool );   }

/** @lsr_xloopbuf_swap
 * @brief Swaps the contents of the two xloopbufs.
 * 
 * @param[in,out] lhs - A pointer to an initialized xloopbuf object.
 * @param[in,out] rhs - A pointer to another initialized xloopbuf object.
 * @return Void.
 */
void lsr_xloopbuf_swap( lsr_xloopbuf_t *lhs, lsr_xloopbuf_t *rhs );

/** @lsr_xloopbuf_straight
 * @brief Straighten the xloopbuf.
 * @details If the xloopbuf is split in two parts due to looping, this function
 * makes it one whole again.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return Void.
 */
lsr_inline void lsr_xloopbuf_straight( lsr_xloopbuf_t *pThis )
{   lsr_loopbuf_xstraight( &pThis->m_loopbuf, pThis->m_pPool ); }

/** @lsr_xloopbuf_available
 * @brief Gets the number of bytes left in the xloopbuf.
 * 
 * @param[in] pThis - A pointer to an initialized loopbuf object.
 * @return The number of bytes left.
 */
lsr_inline int lsr_xloopbuf_available( const lsr_xloopbuf_t *pThis )
{   return lsr_loopbuf_available( &pThis->m_loopbuf );  }

/** @lsr_xloopbuf_contiguous
 * @brief Gets the number of contiguous bytes left in the xloopbuf.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return The number of contiguous bytes left.
 */
lsr_inline int lsr_xloopbuf_contiguous( const lsr_xloopbuf_t *pThis )
{   return lsr_loopbuf_contiguous( &pThis->m_loopbuf ); }

/** @lsr_xloopbuf_used
 * @brief Tells the xloopbuf that the user used size more bytes from
 * the end of the xloopbuf.
 * @details Do not call this after the call 
 * \link #lsr_xloopbuf_append xloopbuf append.\endlink
 * It will be updated within that function.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in] size - The size to add.
 * @return Void.
 */
lsr_inline void lsr_xloopbuf_used( lsr_xloopbuf_t *pThis, int size )
{   lsr_loopbuf_used( &pThis->m_loopbuf, size );    }

/** @lsr_xloopbuf_update
 * @brief Update the xloopbuf at the offset to the contents of pBuf.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in] offset - The starting offset to update in the loopbuf.
 * @param[in] pBuf - A pointer to the buffer to copy.
 * @param[in] size - The amount of bytes to copy.
 * @return Void.
 */
lsr_inline void lsr_xloopbuf_update( 
            lsr_xloopbuf_t *pThis, int offset, const char * pBuf, int size )
{   lsr_loopbuf_update( &pThis->m_loopbuf, offset, pBuf, size );    }

/** @lsr_xloopbuf_search
 * @brief Searches the xloopbuf starting at the offset for the pattern.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in] offset - The offset to start the search at.
 * @param[in] accept - A pointer to the pattern to match.
 * @param[in] acceptLen - The length of the pattern to match.
 * @return The pointer to the pattern in the xloopbuf if found, NULL if not.
 */
lsr_inline char *lsr_xloopbuf_search( 
            lsr_xloopbuf_t *pThis, int offset, const char *accept, int acceptLen )
{   return lsr_loopbuf_search( &pThis->m_loopbuf, offset, accept, acceptLen );  }

/** @lsr_xloopbuf_move_to
 * @brief Moves the front size bytes from pThis to pBuf.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[out] pBuf - A pointer to the allocated buffer to copy to.
 * @param[in] size - The number of bytes to attempt to copy.
 * @return The number of bytes copied.
 */
lsr_inline int lsr_xloopbuf_move_to( lsr_xloopbuf_t *pThis, char * pBuf, int size )
{   return lsr_loopbuf_move_to( &pThis->m_loopbuf, pBuf, size );    }

/** @lsr_xloopbuf_pop_front
 * @brief Pops the front size bytes from pThis.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in] size - The number of bytes to pop.
 * @return The number of bytes popped.
 */
lsr_inline int lsr_xloopbuf_pop_front( lsr_xloopbuf_t *pThis, int size )
{   return lsr_loopbuf_pop_front( &pThis->m_loopbuf, size );    }

/** @lsr_xloopbuf_pop_back
 * @brief Pops the ending size bytes from pThis.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in] size - The number of bytes to pop.
 * @return The number of bytes popped.
 */
lsr_inline int lsr_xloopbuf_pop_back( lsr_xloopbuf_t *pThis, int size )
{   return lsr_loopbuf_pop_back( &pThis->m_loopbuf, size ); }

/** @lsr_xloopbuf_block_size
 * @brief Gets the front block size.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return The front block size.
 */
lsr_inline int lsr_xloopbuf_block_size( const lsr_xloopbuf_t *pThis )
{   return lsr_loopbuf_block_size( &pThis->m_loopbuf ); }

/** @lsr_xloopbuf_size
 * @brief Gets the size used in the xloopbuf so far.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return The size used.
 */
lsr_inline int lsr_xloopbuf_size( const lsr_xloopbuf_t *pThis )
{   return lsr_loopbuf_size( &pThis->m_loopbuf );   }

/** @lsr_xloopbuf_capacity
 * @brief Gets the total capacity of the xloopbuf.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return The total capacity in terms of bytes.
 */
lsr_inline int lsr_xloopbuf_capacity( const lsr_xloopbuf_t *pThis )
{   return pThis->m_loopbuf.m_iCapacity;    }

/** @lsr_xloopbuf_empty
 * @brief Specifies whether or not the xloopbuf is empty.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return 0 for false, true otherwise.
 */
lsr_inline int lsr_xloopbuf_empty( const lsr_xloopbuf_t *pThis )
{   return ( pThis->m_loopbuf.m_pHead == pThis->m_loopbuf.m_pEnd ); }

/** @lsr_xloopbuf_full
 * @brief Specifies whether or not the xloopbuf is full.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return 0 for false, true otherwise.
 */
lsr_inline int lsr_xloopbuf_full( const lsr_xloopbuf_t *pThis )
{   return lsr_xloopbuf_size( pThis ) == pThis->m_loopbuf.m_iCapacity - 1;  }

/** @lsr_xloopbuf_begin
 * @brief Gets a pointer to the beginning of the used data.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return A pointer to the beginning.
 */
lsr_inline char *lsr_xloopbuf_begin( const lsr_xloopbuf_t *pThis )
{   return pThis->m_loopbuf.m_pHead;    }

/** @lsr_xloopbuf_end
 * @brief Gets a pointer to the end of the used data.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return A pointer to the end.
 */
lsr_inline char *lsr_xloopbuf_end( const lsr_xloopbuf_t *pThis )
{   return pThis->m_loopbuf.m_pEnd; }

/** @lsr_xloopbuf_get_pointer
 * @brief Gets a pointer of a given offset from the xloopbuf.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in] size - The offset from the beginning of the used data.
 * @return The pointer associated with the offset.
 */
lsr_inline char *lsr_xloopbuf_get_pointer( const lsr_xloopbuf_t *pThis, int size )
{   return lsr_loopbuf_get_pointer( &pThis->m_loopbuf, size );  }

/** @lsr_xloopbuf_get_offset
 * @brief Given a pointer, calculates the offset relative to the start of
 * the used data.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in] p - The pointer to compare.
 * @return The offset.
 */
lsr_inline int lsr_xloopbuf_get_offset( const lsr_xloopbuf_t *pThis, const char *p )
{   return lsr_loopbuf_get_offset( &pThis->m_loopbuf, p );  }

/** @lsr_xloopbuf_clear
 * @brief Clears the xloopbuf to 0 bytes used.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return Void.
 */
lsr_inline void lsr_xloopbuf_clear( lsr_xloopbuf_t *pThis )
{   pThis->m_loopbuf.m_pHead = pThis->m_loopbuf.m_pEnd = pThis->m_loopbuf.m_pBuf;    }

/** @lsr_xloopbuf_append_unsafe
 * @brief Appends the xloopbuf with a character.
 * @details This function is unsafe in that it will not check if there is enough space
 * allocated.  This function exists to speed up the append process if the user can
 * guarantee that the xloopbuf is large enough.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in] ch - The character to append to the xloopbuf.
 * @return Void.
 */
lsr_inline void lsr_xloopbuf_append_unsafe( lsr_xloopbuf_t *pThis, char ch ) 
{   lsr_loopbuf_append_unsafe( &pThis->m_loopbuf, ch ); }

/** @lsr_xloopbuf_inc
 * @brief Increments the pointer.  If it is currently pointing to the end of the xloopbuf,
 * then the increment will point to the beginning of the xloopbuf.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in,out] pPos - A pointer to the pointer to increment.
 * @return The incremented pointer.
 */
lsr_inline char *lsr_xloopbuf_inc( lsr_xloopbuf_t *pThis, char **pPos )
{   return lsr_loopbuf_inc( &pThis->m_loopbuf, pPos);   }

//NOTICE: The number of segments must match the count of iovec structs in order to use iov_insert.
/** @lsr_xloopbuf_get_num_segments
 * @brief Gets the number of segments the data is stored as.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @return The number of segments the data is stored as.
 */
lsr_inline int lsr_xloopbuf_get_num_segments( lsr_xloopbuf_t *pThis )
{   return pThis->m_loopbuf.m_pEnd > pThis->m_loopbuf.m_pHead ? 1 : 2;  }

/** @lsr_xloopbuf_iov_insert
 * @brief Inserts the contents of the xloopbuf into the vector.
 * 
 * @param[in] pThis - A pointer to an initialized xloopbuf object.
 * @param[in] vect - The vector to copy the content pointer to.
 * @param[in] count - The number of segments the data is stored as.
 * @return 0 on success, -1 on failure.
 */
lsr_inline int lsr_xloopbuf_iov_insert( 
            lsr_xloopbuf_t *pThis, struct iovec *vect, int count )
{   return lsr_loopbuf_iov_insert( &pThis->m_loopbuf, vect, count );    }



#ifdef __cplusplus
}
#endif 
#endif //LSR_LOOPBUF_H
