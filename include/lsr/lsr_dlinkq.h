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
#ifndef LSR_DLINKQ_H
#define LSR_DLINKQ_H


#include <stdbool.h>
#include <lsr/lsr_link.h>
#include <lsr/lsr_pool.h>
#include <lsr/lsr_types.h>


/**
 * @file
 * \warning ABANDONED... for now.
 */


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef lsr_linkq_t
 */
typedef struct lsr_linkq_s
{
    lsr_link_t m_head;
    int m_iTotal;
} lsr_linkq_t;

/**
 * @typedef lsr_dlinkq_t
 */
typedef struct lsr_dlinkq_s
{
    lsr_dlink_t m_head;
    int m_iTotal;
} lsr_dlinkq_t;


/**
 * @lsr_linkq
 * @brief Initializes a queue of singly linked list objects.
 * 
 * @param[in] pThis - A pointer to an allocated linked list queue object.
 * @return Void.
 *
 * @see lsr_linkq_new, lsr_linkq_d
 */
lsr_inline void lsr_linkq( lsr_linkq_t *pThis )
{
    lsr_link( &pThis->m_head, NULL );
    pThis->m_iTotal = 0;
}

/**
 * @lsr_linkq_new
 * @brief Creates (allocates and initializes)
 *   a new queue of singly linked list objects.
 * 
 * @return A pointer to an initialized linked list queue object,
 *   else NULL on error.
 *
 * @see lsr_linkq, lsr_linkq_delete
 */
lsr_inline lsr_linkq_t * lsr_linkq_new()
{
    lsr_linkq_t * pThis;
    if ( ( pThis = (lsr_linkq_t *)
      lsr_palloc( sizeof(*pThis) ) ) != NULL )
    {
        lsr_linkq( pThis );
    }
    return pThis;
}

/**
 * @lsr_linkq_d
 * @brief Destroys the singly linked list <b> queue object</b>.
 * @note It is the user's responsibility to destroy or delete
 *   the linked list objects, depending on how the objects were created.
 * 
 * @param[in] pThis - A pointer to an initialized linked list queue object.
 * @return Void.
 *
 * @see lsr_linkq
 */
lsr_inline void lsr_linkq_d( lsr_linkq_t *pThis )
{}

/**
 * @lsr_linkq_delete
 * @brief Destroys the singly linked list <b> queue object</b>,
 *   then deletes the object.
 * @details The object should have been created with a previous
 *   successful call to lsr_linkq_new.
 * @note It is the user's responsibility to destroy or delete
 *   the linked list objects, depending on how the objects were created.
 * 
 * @param[in] pThis - A pointer to an initialized linked list queue object.
 * @return Void.
 *
 * @see lsr_linkq_new
 */
lsr_inline void lsr_linkq_delete( lsr_linkq_t *pThis )
{   lsr_linkq_d( pThis );  lsr_pfree( pThis );   }

/**
 * @lsr_linkq_size
 * @brief Gets the size of a singly linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized linked list queue object.
 * @return The size in terms of number of elements.
 */
lsr_inline int lsr_linkq_size( const lsr_linkq_t *pThis )
{   return pThis->m_iTotal;  }

/**
 * @lsr_linkq_push
 * @brief Pushes an element to a linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized linked list queue object.
 * @param[in] pObj - A pointer to a singly linked list object.
 * @return Void.
 */
lsr_inline void lsr_linkq_push( lsr_linkq_t *pThis,  lsr_link_t *pObj )
{   lsr_link_addnext( &pThis->m_head, pObj );  ++pThis->m_iTotal;  }

/**
 * @lsr_linkq_pop
 * @brief Pops an element from a linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized linked list queue object.
 * @return A pointer to the linked list object if the queue is not empty,
 *   else NULL.
 */
lsr_inline lsr_link_t * lsr_linkq_pop( lsr_linkq_t *pThis )
{
    if ( pThis->m_iTotal )
    {
        --pThis->m_iTotal;
        return lsr_link_removenext( &pThis->m_head );
    }
    return NULL;
}

/**
 * @lsr_linkq_begin
 * @brief Gets a pointer to the linked list object at the head (beginning)
 *   of a linked list queue object.
 * @details The routine does \e not pop anything from the object.
 * 
 * @param[in] pThis - A pointer to an initialized linked list queue object.
 * @return A pointer to the linked list object at the beginning of the queue.
 */
lsr_inline lsr_link_t * lsr_linkq_begin( const lsr_linkq_t *pThis )
{   return lsr_link_next( &pThis->m_head );  }

/**
 * @lsr_linkq_end
 * @brief Gets the end of a linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized linked list queue object.
 * @return A pointer to the end of the queue (NULL).
 */
lsr_inline lsr_link_t * lsr_linkq_end( const lsr_linkq_t *pThis )
{   return NULL;  }

/**
 * @lsr_linkq_head
 * @brief Gets the head of a linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized linked list queue object.
 * @return A pointer to the head of the queue.
 */
lsr_inline lsr_link_t * lsr_linkq_head( lsr_linkq_t *pThis )
{   return &pThis->m_head;  }

/**
 * @lsr_linkq_removenext
 * @brief Removes a linked list element from a linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized linked list queue object.
 * @param[in] pObj - A pointer to a linked list object
 *   whose \e next is to be removed.
 * @return A pointer to the removed linked list object.
 * @warning It is expected that the queue object is \e not empty.
 */
lsr_inline lsr_link_t * lsr_linkq_removenext(
  lsr_linkq_t *pThis, lsr_link_t *pObj )
{    --pThis->m_iTotal;  return lsr_link_removenext( pObj );  }

/**
 * @lsr_linkq_addnext
 * @brief Adds a linked list element to a linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized linked list queue object.
 * @param[in] pObj - A pointer to a linked list object
 *   where the new element is to be added.
 * @param[in] pNext - A pointer to the singly linked list object to add.
 * @return Void.
 */
lsr_inline void lsr_linkq_addnext(
  lsr_linkq_t *pThis, lsr_link_t *pObj, lsr_link_t *pNext )
{   ++pThis->m_iTotal;  lsr_link_addnext( pObj, pNext );  }



/**
 * @lsr_dlinkq
 * @brief Initializes a queue of doubly linked list objects.
 * 
 * @param[in] pThis - A pointer to an allocated doubly linked list queue object.
 * @return Void.
 *
 * @see lsr_dlinkq_new, lsr_dlinkq_d
 */
lsr_inline void lsr_dlinkq( lsr_dlinkq_t *pThis )
{
    lsr_dlink( &pThis->m_head, &pThis->m_head, &pThis->m_head );
    pThis->m_iTotal = 0;
}

/**
 * @lsr_dlinkq_new
 * @brief Creates (allocates and initializes)
 *   a new queue of doubly linked list objects.
 * 
 * @return A pointer to an initialized linked list queue object,
 *   else NULL on error.
 *
 * @see lsr_dlinkq, lsr_dlinkq_delete
 */
lsr_inline lsr_dlinkq_t * lsr_dlinkq_new()
{
    lsr_dlinkq_t * pThis;
    if ( ( pThis = (lsr_dlinkq_t *)
      lsr_palloc( sizeof(*pThis) ) ) != NULL )
    {
        lsr_dlinkq( pThis );
    }
    return pThis;
}

/**
 * @lsr_dlinkq_d
 * @brief Destroys the doubly linked list <b> queue object</b>.
 * @note It is the user's responsibility to destroy or delete
 *   the linked list objects, depending on how the objects were created.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list queue object.
 * @return Void.
 *
 * @see lsr_dlinkq
 */
lsr_inline void lsr_dlinkq_d( lsr_dlinkq_t *pThis )
{}

/**
 * @lsr_dlinkq_delete
 * @brief Destroys the doubly linked list <b> queue object</b>,
 *   then deletes the object.
 * @details The object should have been created with a previous
 *   successful call to lsr_dlinkq_new.
 * @note It is the user's responsibility to destroy or delete
 *   the linked list objects, depending on how the objects were created.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list queue object.
 * @return Void.
 *
 * @see lsr_dlinkq_new
 */
lsr_inline void lsr_dlinkq_delete( lsr_dlinkq_t *pThis )
{   lsr_dlinkq_d( pThis );  lsr_pfree( pThis );   }

/**
 * @lsr_dlinkq_size
 * @brief Gets the size of a doubly linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list queue object.
 * @return The size in terms of number of elements.
 */
lsr_inline int lsr_dlinkq_size( const lsr_dlinkq_t *pThis )
{   return pThis->m_iTotal;  }

/**
 * @lsr_dlinkq_empty
 * @brief Specifies whether or not a doubly linked list queue object
 *   contains any elements.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list queue object.
 * @return True if empty, else false.
 */
lsr_inline bool lsr_dlinkq_empty( const lsr_dlinkq_t *pThis )
{   return lsr_dlink_next( &pThis->m_head ) == &pThis->m_head;  }
 
/**
 * @lsr_dlinkq_append
 * @brief Pushes an element to the end of a doubly linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list queue object.
 * @param[in] pReq - A pointer to the linked list object to append.
 * @return Void.
 */
lsr_inline void lsr_dlinkq_append( lsr_dlinkq_t *pThis, lsr_dlink_t *pReq )
{
    lsr_dlink_addnext( lsr_dlink_prev( &pThis->m_head ), pReq );
    ++pThis->m_iTotal;
}

/**
 * @lsr_dlinkq_push_front
 * @brief Pushes an element to the beginning (front)
 *   of a doubly linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list queue object.
 * @param[in] pReq - A pointer to the linked list object to add.
 * @return Void.
 */
lsr_inline void lsr_dlinkq_push_front( lsr_dlinkq_t *pThis, lsr_dlink_t *pReq )
{
    lsr_dlink_addprev( lsr_dlink_next( &pThis->m_head ), pReq );
    ++pThis->m_iTotal;
}
    
/**
 * @lsr_dlinkq_remove
 * @brief Removes a linked list element from a doubly linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list queue object.
 * @param[in] pReq - A pointer to the linked list object to remove.
 * @return Void.
 */
lsr_inline void lsr_dlinkq_remove( lsr_dlinkq_t *pThis, lsr_dlink_t *pReq )
{
    assert( pReq != &pThis->m_head );
    if ( lsr_dlink_next( pReq ) )
    {
        lsr_dlink_remove( pReq );
        --pThis->m_iTotal;
    }
}

/**
 * @lsr_dlinkq_pop_front
 * @brief Pops an element from the beginning (front)
 *   of a doubly linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list queue object.
 * @return A pointer to the linked list object if the queue is not empty,
 *   else NULL.
 */
lsr_inline lsr_dlink_t * lsr_dlinkq_pop_front( lsr_dlinkq_t *pThis )
{
    if ( lsr_dlink_next( &pThis->m_head ) != &pThis->m_head )
    {
        --pThis->m_iTotal;
        return lsr_dlink_removenext( &pThis->m_head );
    }
    assert( pThis->m_iTotal == 0 );
    return NULL;
}

/**
 * @lsr_dlinkq_begin
 * @brief Gets a pointer to the linked list object at the head (beginning)
 *   of a doubly linked list queue object.
 * @details The routine does \e not pop anything from the object.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list queue object.
 * @return A pointer to the linked list object at the beginning of the queue.
 */
lsr_inline lsr_dlink_t * lsr_dlinkq_begin( lsr_dlinkq_t *pThis )
{   return lsr_dlink_next( &pThis->m_head );  }

/**
 * @lsr_dlinkq_end
 * @brief Gets the end of a doubly linked list queue object.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list queue object.
 * @return A pointer to the end of the queue.
 */
lsr_inline lsr_dlink_t * lsr_dlinkq_end( lsr_dlinkq_t *pThis ) 
{   return &pThis->m_head;  }


#ifdef __cplusplus
}
#endif

#endif  /* LSR_DLINKQ_H */

