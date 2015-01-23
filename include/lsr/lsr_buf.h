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
#ifndef LSR_BUF_H
#define LSR_BUF_H

#include <assert.h>
#include <string.h>
#include <lsr/lsr_types.h>

/**
 * @file
 * @note
 * Auto Buffer - An auto buffer is a mechanism which manages buffer allocation as long as
 *      the user ensures there is enough space (reserve, guarantee, grow, append).\n\n
 * From hereafter, lsr buf refers to the lsr implementation of auto buffer.\n\n
 * lsr_buf_t defines an auto buffer object.\n
 *      Functions prefixed with lsr_buf_* use a lsr_buf_t \* parameter.\n\n
 * lsr_xbuf_t is a wrapper around lsr_buf_t, adding a pointer to the session pool (xpool).\n
 *      Functions prefixed with lsr_xbuf_* use a lsr_xbuf_t \* parameter.\n\n
 * If a user wants to use the session pool with lsr buf, there are two options:
 *      @li Use lsr_buf_x* functions with lsr_buf_t.
 *      This option should be used if the user can pass the xpool pointer every time a
 *      growing/shrinking function is called.
 * 
 *      @li Use lsr_xbuf_x* functions with lsr_xbuf_t.
 *      This structure takes in the session pool as a parameter during initialization and
 *      stores the pointer.  This should be used if the user cannot guarantee
 *      having the xpool pointer every time a function needs it.
 */


#ifdef __cplusplus
extern "C" {
#endif 

/**
 * @typedef lsr_buf_t
 * @brief A buffer that will grow as needed.
 */
typedef struct lsr_buf_s
{
    char  * m_pBuf;
    char  * m_pEnd;
    char  * m_pBufEnd;
} lsr_buf_t;

/**
 * @typedef lsr_xbuf_t
 * @brief A struct that contains a session pool pointer as well as lsr_buf_t
 * @details This structure can be useful for when the user knows that the
 * structure will only last an HTTP Session and the user cannot guarantee that
 * he/she will have a pointer to the session pool every time it is needed.
 */
typedef struct lsr_xbuf_s
{
    lsr_buf_t       m_buf;
    lsr_xpool_t    *m_pPool;
} lsr_xbuf_t;

/** @lsr_buf_xnew
 * @brief Creates and initializes a new buffer from the session pool.
 * 
 * @param[in] size - The initial size to set the buffer to.
 * @param[in] pool - A pointer to the session pool to allocate from.
 * @return A pointer to the created buffer if successful, NULL if not.
 * 
 * @see lsr_buf_xdelete
 */
lsr_buf_t * lsr_buf_xnew( int size, lsr_xpool_t * pool );

/** @lsr_buf_x
 * @brief Initializes the buffer to a given size from a given
 * session pool.
 * 
 * @param[in] pThis - A pointer to an allocated lsr buf object.
 * @param[in] size - The initial size to set the buffer to.
 * @param[in] pool - A pointer to the session pool to allocate from.
 * @return 0 on success, -1 on failure.
 * 
 * @see lsr_buf_xd
 */
int lsr_buf_x( lsr_buf_t * pThis, int size, lsr_xpool_t * pool );

/** @lsr_buf_xd
 * @brief Destroys the buffer.  DOES NOT FREE \e pThis!
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] pool - A pointer to the session pool to deallocate from.
 * @return Void.
 * 
 * @see lsr_buf_x
 */
void lsr_buf_xd( lsr_buf_t * pThis, lsr_xpool_t * pool );

/** @lsr_buf_xdelete
 * @brief Deletes the buffer, opposite of \link #lsr_buf_xnew buf xnew \endlink
 * @details If a buf was created with \link #lsr_buf_xnew buf xnew, \endlink
 * it must be deleted with \link #lsr_buf_xdelete buf xdelete. \endlink
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] pool - A pointer to the session pool to deallocate from.
 * @return Void.
 * 
 * @see lsr_buf_xnew
 */
void lsr_buf_xdelete( lsr_buf_t * pThis, lsr_xpool_t * pool );

/** @lsr_buf_new
 * @brief Creates and initializes a new buffer.
 * 
 * @param[in] size - The initial size to set the buffer to.
 * @return A pointer to the created buffer if successful, NULL if not.
 * 
 * @see lsr_buf_delete
 */
lsr_inline lsr_buf_t * lsr_buf_new( int size )
{   return lsr_buf_xnew( size, NULL );  }

/** @lsr_buf
 * @brief Initializes the buffer to a given size.
 * 
 * @param[in] pThis - A pointer to an allocated lsr buf object.
 * @param[in] size - The initial size to set the buffer to.
 * @return 0 on success, -1 on failure.
 * 
 * @see lsr_buf_d
 */
lsr_inline int lsr_buf( lsr_buf_t * pThis, int size )
{   return lsr_buf_x( pThis, size, NULL );  }

/** @lsr_buf_d
 * @brief Destroys the buffer.  DOES NOT FREE \e pThis!
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @return Void.
 * 
 * @see lsr_buf
 */
lsr_inline void lsr_buf_d( lsr_buf_t * pThis )
{   lsr_buf_xd( pThis, NULL );  }

/** @lsr_buf_delete
 * @brief Deletes the buffer, opposite of \link #lsr_buf_new buf new \endlink
 * @details If a buf was created with \link #lsr_buf_new buf new, \endlink
 * it must be deleted with \link #lsr_buf_delete buf delete. \endlink
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @return Void.
 * 
 * @see lsr_buf_new
 */
lsr_inline void lsr_buf_delete( lsr_buf_t * pThis )
{   lsr_buf_xdelete( pThis, NULL ); }

/** @lsr_buf_xreserve
 * @brief Reserves \e size total bytes for the buffer from the session pool.  
 * If \e size is lower than the current size, it will shrink.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] size - The total number of bytes to reserve.
 * @param[in] pool - A pointer to the session pool to allocate from.
 * @return 0 on success, -1 on failure.
 */
int lsr_buf_xreserve( lsr_buf_t * pThis, int size, lsr_xpool_t * pool );

/** @lsr_buf_xgrow
 * @brief Extends the buffer by \e size bytes.
 * @details Allocates from the given session pool.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] size - The size to grow the buffer by.
 * @param[in] pool - A pointer to the session pool to allocate from.
 * @return 0 on success, -1 on failure.
 */
int lsr_buf_xgrow( lsr_buf_t * pThis, int size, lsr_xpool_t * pool );

/** @lsr_buf_xappend2
 * @brief Appends the buffer with a given buffer.
 * @details It will check if there is enough space in the buffer and will grow 
 * the buffer from the session pool if there is not enough space.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] pBuf - A pointer to the source buffer.
 * @param[in] size - The size of pBuf.
 * @param[in] pool - A pointer to the pool to allocate from.
 * @return The size appended or -1 if it failed.
 */
int lsr_buf_xappend2( 
            lsr_buf_t * pThis, const char * pBuf, int size, lsr_xpool_t * pool );

/** @lsr_buf_xappend
 * @brief Appends the buffer with a given buffer.
 * @details Since the size is not passed in, it will have to calculate the length.  
 * It will check if there is enough space in the buffer and will grow the buffer from the
 * session pool if there is not enough space.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] pBuf - A pointer to the source buffer.
 * @param[in] pool - A pointer to the pool to allocate from.
 * @return The size appended or -1 if it failed.
 */
lsr_inline int lsr_buf_xappend( 
            lsr_buf_t * pThis, const char * pBuf, lsr_xpool_t * pool )
{   return lsr_buf_xappend2( pThis, pBuf, strlen( pBuf ), pool ); }

/** @lsr_buf_reserve
 * @brief Reserves \e size total bytes for the buffer.  If \e size is lower than the current
 * size, it will shrink.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] size - The total number of bytes to reserve.
 * @return 0 on success, -1 on failure.
 */
lsr_inline int lsr_buf_reserve( lsr_buf_t * pThis, int size )
{   return lsr_buf_xreserve( pThis, size, NULL );   }

/** @lsr_buf_available
 * @brief Gets the number of bytes left in the buffer.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @return The number of bytes left.
 */
lsr_inline int lsr_buf_available( const lsr_buf_t * pThis )
{   return pThis->m_pBufEnd - pThis->m_pEnd;    }

/** @lsr_buf_begin
 * @brief Gets the beginning of the buffer.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @return A pointer to the beginning.
 */
lsr_inline char * lsr_buf_begin( const lsr_buf_t * pThis ) 
{   return pThis->m_pBuf;   }

/** @lsr_buf_end
 * @brief Gets the end of the buffer.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @return A pointer to the end of the buffer.
 */
lsr_inline char * lsr_buf_end( const lsr_buf_t * pThis )
{   return pThis->m_pEnd;   }

/** @lsr_buf_getp
 * @brief Gets a pointer of a given offset from the start of the buffer.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] offset - The offset from the beginning of the buffer.
 * @return The pointer associated with the offset.
 */
lsr_inline char * lsr_buf_getp( const lsr_buf_t * pThis, int offset ) 
{   return pThis->m_pBuf + offset; }

/** @lsr_buf_used
 * @brief Tells the buffer that the user appended \e size more bytes.
 * @details
 * Do not call this after the calls \link #lsr_buf_append buf append,\endlink
 * \link #lsr_buf_append2 buf append2,\endlink 
 * \link #lsr_buf_append_unsafe buf append unsafe,\endlink
 * \link #lsr_buf_xappend buf xappend,\endlink
 * and \link #lsr_buf_xappend2 buf xappend2.\endlink
 * It will be updated within those functions.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] size - The size to add.
 * @return Void.
 */
lsr_inline void lsr_buf_used( lsr_buf_t * pThis, int size )
{   pThis->m_pEnd += size;     }

/** @lsr_buf_clear
 * @brief Clears the buffer to 0 bytes used.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @return Void.
 */
lsr_inline void lsr_buf_clear( lsr_buf_t * pThis )            
{   pThis->m_pEnd = pThis->m_pBuf;    }

/** @lsr_buf_capacity
 * @brief Gets the total capacity of the buffer
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @return The total capacity.
 */
lsr_inline int lsr_buf_capacity( const lsr_buf_t * pThis )     
{   return pThis->m_pBufEnd - pThis->m_pBuf;  }

/** @lsr_buf_size
 * @brief Gets the size used in the buffer so far.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @return The size used.
 */
lsr_inline int lsr_buf_size( const lsr_buf_t * pThis )      
{   return pThis->m_pEnd - pThis->m_pBuf;   }

/** @lsr_buf_resize
 * @brief Resizes the used amount to \e size bytes.
 * @details The size must be less than or equal to capacity.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] size - The size to resize the buffer to.
 * @return Void.
 */
lsr_inline void lsr_buf_resize( lsr_buf_t * pThis, int size )  
{   
    assert( size <= lsr_buf_capacity( pThis ));
    pThis->m_pEnd = pThis->m_pBuf + size;
}

/** @lsr_buf_grow
 * @brief Extends the buffer by \e size bytes.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] size - The size to grow the buffer by.
 * @return 0 on success, -1 on failure.
 */
lsr_inline int lsr_buf_grow( lsr_buf_t * pThis, int size )
{   return lsr_buf_xgrow( pThis, size, NULL );  }

/** @lsr_buf_append_unsafe
 * @brief Appends the buffer with a given buffer.
 * @details This function is unsafe in that it will not check if there is enough space
 * allocated.  This function exists to speed up the append process if the user can
 * guarantee that the buffer is large enough.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] pBuf - A pointer to the source buffer.
 * @param[in] size - The size of pBuf.
 * @return The size appended.
 */
lsr_inline int lsr_buf_append_unsafe( lsr_buf_t * pThis, const char * pBuf, int size )
{
    memmove( lsr_buf_end(pThis), pBuf, size );
    lsr_buf_used( pThis, size );
    return size;
}

/** @lsr_buf_append_char_unsafe
 * @brief Appends the buffer with a character.
 * @details This function is unsafe in that it will not check if there is enough space
 * allocated.  This function exists to speed up the append process if the user can
 * guarantee that the buffer is large enough.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] ch - The character to append to the buffer.
 * @return Void.
 */
lsr_inline void lsr_buf_append_char_unsafe( lsr_buf_t * pThis, char ch )
{   *pThis->m_pEnd++ = ch;   }

/** @lsr_buf_append2
 * @brief Appends the buffer with a given buffer.
 * @details It will check if there is enough space in the buffer and will 
 * grow the buffer if there is not enough space.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] pBuf - A pointer to the source buffer.
 * @param[in] size - The size of pBuf.
 * @return The size appended or -1 if it failed.
 */
lsr_inline int lsr_buf_append2( lsr_buf_t * pThis, const char * pBuf, int size )
{   return lsr_buf_xappend2( pThis, pBuf, size, NULL );     }

/** @lsr_buf_append
 * @brief Appends the buffer with a given buffer.
 * @details Since the size is not passed in, it will have to calculate the length.  
 * It will check if there is enough space in the buffer and will grow the buffer if there 
 * is not enough space.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] pBuf - A pointer to the source buffer.
 * @return The size appended or -1 if it failed.
 */
lsr_inline int lsr_buf_append( lsr_buf_t * pThis, const char * pBuf )
{   return lsr_buf_xappend2( pThis, pBuf, strlen( pBuf ), NULL ); }

/** @lsr_buf_empty
 * @brief Specifies whether or not a lsr buf object is empty.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @return Non-zero if empty, 0 if not.
 */
lsr_inline int lsr_buf_empty( const lsr_buf_t * pThis )
{   return ( pThis->m_pBuf == pThis->m_pEnd );  }

/** @lsr_buf_full
 * @brief Specifies whether or not a lsr buf object is full.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @return Non-zero if full, 0 if not.
 */
lsr_inline int lsr_buf_full( const lsr_buf_t * pThis )
{   return ( pThis->m_pEnd == pThis->m_pBufEnd ); }

/** @lsr_buf_get_offset
 * @brief Given a pointer, calculates the offset relative to the start of
 * the buffer.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] p - The pointer to get the offset of.
 * @return The offset.
 */
lsr_inline int lsr_buf_get_offset( const lsr_buf_t * pThis, const char * p )
{   return p - pThis->m_pBuf;    }

/** @lsr_buf_pop_front_to
 * @brief Pops the front \e size bytes from \e pThis and stores in \e pBuf.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[out] pBuf - A pointer to the allocated buffer to copy to.
 * @param[in] size - The number of bytes to attempt to copy.
 * @return The number of bytes copied.
 */
int  lsr_buf_pop_front_to( lsr_buf_t * pThis, char * pBuf, int size );

/** @lsr_buf_pop_front
 * @brief Pops the front \e size bytes from \e pThis.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] size - The number of bytes to pop.
 * @return The number of bytes popped.
 */
int  lsr_buf_pop_front( lsr_buf_t * pThis, int size );

/** @lsr_buf_pop_end
 * @brief Pops the ending \e size bytes from \e pThis.
 * 
 * @param[in] pThis - A pointer to an initialized lsr buf object.
 * @param[in] size - The number of bytes to pop.
 * @return The number of bytes popped.
 */
int  lsr_buf_pop_end( lsr_buf_t * pThis, int size );

/** @lsr_buf_swap
 * @brief Swaps the contents of the two buffers.
 * 
 * @param[in,out] pThis - A pointer to an initialized lsr buf object.
 * @param[in,out] pRhs - A pointer to another initialized lsr buf object.
 * @return Void.
 */
void lsr_buf_swap( lsr_buf_t * pThis, lsr_buf_t * pRhs );

/** @lsr_xbuf_new
 * @brief Creates and initializes a new buffer from the session pool.
 * 
 * @param[in] size - The initial size to set the buffer to.
 * @param[in] pool - A pointer to the session pool to allocate from.
 * @return A pointer to the created buffer if successful, NULL if not.
 * 
 * @see lsr_xbuf_delete
 */
lsr_xbuf_t * lsr_xbuf_new( int size, lsr_xpool_t * pool );

/** @lsr_xbuf
 * @brief Initializes the buffer to a given size.
 * 
 * @param[in] pThis - A pointer to an allocated lsr xbuf object.
 * @param[in] size - The initial size to set the buffer to.
 * @param[in] pool - A pointer to the pool the buffer should allocate from.
 * @return 0 on success, -1 on failure.
 */
int lsr_xbuf( lsr_xbuf_t * pThis, int size, lsr_xpool_t * pool );

/** @lsr_xbuf_d
 * @brief Destroys the buffer.  DOES NOT FREE \e pThis!
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @return Void.
 * 
 * @see lsr_xbuf_d
 */
void lsr_xbuf_d( lsr_xbuf_t * pThis );

/** @lsr_xbuf_delete
 * @brief Deletes the buffer, opposite of \link #lsr_xbuf_new xbuf new \endlink
 * @details If a buf was created with \link #lsr_xbuf_new xbuf new, \endlink
 * it must be deleted with \link #lsr_xbuf_delete xbuf delete. \endlink
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @return Void.
 * 
 * @see lsr_xbuf_new
 */
void lsr_xbuf_delete( lsr_xbuf_t * pThis );

/** @lsr_xbuf_reserve
 * @brief Reserves \e size total bytes for the buffer.  If \e size is lower than the current
 * size, it will shrink.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in] size - The total number of bytes to reserve.
 * @return 0 on success, -1 on failure.
 * 
 * @see lsr_xbuf
 */
lsr_inline int lsr_xbuf_reserve( lsr_xbuf_t * pThis, int size )
{   return lsr_buf_xreserve( &pThis->m_buf, size, pThis->m_pPool ); }

/** @lsr_xbuf_available
 * @brief Gets the number of bytes left in the buffer.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @return The number of bytes left
 */
lsr_inline int lsr_xbuf_available( const lsr_xbuf_t * pThis )
{   return pThis->m_buf.m_pBufEnd - pThis->m_buf.m_pEnd;    }

/** @lsr_xbuf_begin
 * @brief Gets the beginning of the buffer.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @return A pointer to the beginning.
 */
lsr_inline char * lsr_xbuf_begin( const lsr_xbuf_t * pThis ) 
{   return pThis->m_buf.m_pBuf;     }

/** @lsr_xbuf_end
 * @brief Gets the end of the buffer.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @return A pointer to the end of the buffer.
 */
lsr_inline char * lsr_xbuf_end( const lsr_xbuf_t * pThis )
{   return pThis->m_buf.m_pEnd;    }

/** @lsr_xbuf_getp
 * @brief Gets a pointer of a given offset from the start of the buffer.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in] offset - The offset from the beginning of the buffer.
 * @return The pointer associated with the offset.
 */
lsr_inline char * lsr_xbuf_getp( const lsr_xbuf_t * pThis, int offset ) 
{   return pThis->m_buf.m_pBuf + offset; }

/** @lsr_xbuf_used
 * @brief Tells the buffer that the user appended \e size more bytes.
 * @details
 * Do not call this after the calls \link #lsr_xbuf_append xbuf append,\endlink
 * \link #lsr_xbuf_append2 xbuf append2,\endlink 
 * and \link #lsr_xbuf_append_unsafe xbuf append unsafe.\endlink
 * It will be updated within those functions.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in] size - The size to add.
 * @return Void.
 */
lsr_inline void lsr_xbuf_used( lsr_xbuf_t * pThis, int size )
{   pThis->m_buf.m_pEnd += size;     }

/** @lsr_xbuf_clear
 * @brief Clears the buffer to 0 bytes used.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @return Void.
 */
lsr_inline void lsr_xbuf_clear( lsr_xbuf_t * pThis )            
{   pThis->m_buf.m_pEnd = pThis->m_buf.m_pBuf;    }

/** @lsr_xbuf_capacity
 * @brief Gets the total capacity of the buffer
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @return The total capacity.
 */
lsr_inline int lsr_xbuf_capacity( const lsr_xbuf_t * pThis )     
{   return pThis->m_buf.m_pBufEnd - pThis->m_buf.m_pBuf;  }

/** @lsr_xbuf_size
 * @brief Gets the size used in the buffer so far.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @return The size used.
 */
lsr_inline int lsr_xbuf_size( const lsr_xbuf_t * pThis )      
{   return pThis->m_buf.m_pEnd - pThis->m_buf.m_pBuf;   }

/** @lsr_xbuf_resize
 * @brief Resizes the used amount to \e size bytes.
 * @details The size must be less than or equal to capacity.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in] size - The size to resize the buffer to.
 * @return Void.
 */
lsr_inline void lsr_xbuf_resize( lsr_xbuf_t * pThis, int size )  
{   lsr_buf_resize( &pThis->m_buf, size );  }

/** @lsr_xbuf_grow
 * @brief Extends the buffer by \e size bytes.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in] size - The size to grow the buffer by.
 * @return 0 on success, -1 on failure.
 */
lsr_inline int lsr_xbuf_grow( lsr_xbuf_t * pThis, int size )
{   return lsr_buf_xgrow( &pThis->m_buf, size, pThis->m_pPool );    }

/** @lsr_xbuf_append_unsafe
 * @brief Appends the buffer with a given buffer.
 * @details This function is unsafe in that it will not check if there is enough space
 * allocated.  This function exists to speed up the append process if the user can
 * guarantee that the buffer is large enough.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in] pBuf - A pointer to the source buffer.
 * @param[in] size - The size of pBuf.
 * @return The size appended.
 */
lsr_inline int lsr_xbuf_append_unsafe( lsr_xbuf_t * pThis, const char * pBuf, int size )
{   return lsr_buf_append_unsafe( &pThis->m_buf, pBuf, size );  }

/** @lsr_xbuf_append_char_unsafe
 * @brief Appends the buffer with a character.
 * @details This function is unsafe in that it will not check if there is enough space
 * allocated.  This function exists to speed up the append process if the user can
 * guarantee that the buffer is large enough.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in] ch - The character to append to the buffer.
 * @return Void.
 */
lsr_inline void lsr_xbuf_append_char_unsafe( lsr_xbuf_t * pThis, char ch )
{   *pThis->m_buf.m_pEnd++ = ch;   }

/** @lsr_xbuf_append2
 * @brief Appends the buffer with a given buffer.
 * @details It will check if there is enough space in the buffer and will grow 
 * the buffer if there is not enough space.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in] pBuf - A pointer to the source buffer.
 * @param[in] size - The size of pBuf.
 * @return The size appended or -1 if it failed.
 */
lsr_inline int lsr_xbuf_append2( lsr_xbuf_t * pThis, const char * pBuf, int size )
{   return lsr_buf_xappend2( &pThis->m_buf, pBuf, size, pThis->m_pPool );   }

/** @lsr_xbuf_append
 * @brief Appends the buffer with a given buffer.
 * @details Since the size is not passed in, it will have to calculate the length.  
 * It will check if there is enough space in the buffer and will grow the buffer if there 
 * is not enough space.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in] pBuf - A pointer to the source buffer.
 * @return The size appended or -1 if it failed.
 */
lsr_inline int lsr_xbuf_append( lsr_xbuf_t * pThis, const char * pBuf )
{   return lsr_xbuf_append2( pThis, pBuf, strlen( pBuf ) ); }

/** @lsr_xbuf_empty
 * @brief Specifies whether or not a lsr xbuf object is empty.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @return 0 for false, true otherwise.
 */
lsr_inline int lsr_xbuf_empty( const lsr_xbuf_t * pThis )
{   return ( pThis->m_buf.m_pBuf == pThis->m_buf.m_pEnd );  }

/** @lsr_xbuf_full
 * @brief Specifies whether or not a lsr xbuf object is full.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @return 0 for false, true otherwise.
 */
lsr_inline int lsr_xbuf_full( const lsr_xbuf_t * pThis )
{   return ( pThis->m_buf.m_pEnd == pThis->m_buf.m_pBufEnd ); }

/** @lsr_xbuf_get_offset
 * @brief Given a pointer, calculates the offset relative to the start of
 * the buffer.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in] p - The pointer to get the offset of.
 * @return The offset.
 */
lsr_inline int lsr_xbuf_get_offset( const lsr_xbuf_t * pThis, const char * p )
{   return p - pThis->m_buf.m_pBuf;    }

/** @lsr_xbuf_pop_front_to
 * @brief Pops the front \e size bytes from \e pThis and stores in \e pBuf.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[out] pBuf - A pointer to the allocated buffer to copy to.
 * @param[in] size - The number of bytes to attempt to copy.
 * @return The number of bytes copied.
 */
lsr_inline int lsr_xbuf_pop_front_to( lsr_xbuf_t * pThis, char * pBuf, int size )
{   return lsr_buf_pop_front_to( &pThis->m_buf, pBuf, size );    }

/** @lsr_xbuf_pop_front
 * @brief Pops the front \e size bytes from \e pThis.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in] size - The number of bytes to pop.
 * @return The number of bytes popped.
 */
lsr_inline int lsr_xbuf_pop_front( lsr_xbuf_t * pThis, int size )
{   return lsr_buf_pop_front( &pThis->m_buf, size );    }

/** @lsr_xbuf_pop_end
 * @brief Pops the ending \e size bytes from \e pThis.
 * 
 * @param[in] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in] size - The number of bytes to pop.
 * @return The number of bytes popped.
 */
lsr_inline int lsr_xbuf_pop_end( lsr_xbuf_t * pThis, int size )
{   return lsr_buf_pop_end( &pThis->m_buf, size );    }

/** @lsr_xbuf_swap
 * @brief Swaps the contents of the two buffers.
 * 
 * @param[in,out] pThis - A pointer to an initialized lsr xbuf object.
 * @param[in,out] pRhs - A pointer to another initialized lsr xbuf object.
 * @return Void.
 */
void lsr_xbuf_swap( lsr_xbuf_t * pThis, lsr_xbuf_t * pRhs );


#ifdef __cplusplus
}
#endif 


#endif // LSR_BUF_H
