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
#ifndef LSR_LINK_H
#define LSR_LINK_H


#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <lsr/lsr_pool.h>
#include <lsr/lsr_types.h>


/**
 * @file
 */


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef lsr_link_t
 */
typedef struct lsr_link_s
{
    struct lsr_link_s * m_pNext;
    void * m_pObj;
} lsr_link_t;

/**
 * @typedef lsr_dlink_t
 */
typedef struct lsr_dlink_s
{
    struct lsr_dlink_s * m_pNext;
    struct lsr_dlink_s * m_pPrev;
    void * m_pObj;
} lsr_dlink_t;


/**
 * @lsr_link
 * @brief Initializes a singly linked list object.
 * 
 * @param[in] pThis - A pointer to an allocated linked list object.
 * @param[in] next - A pointer to the next linked object, which could be NULL.
 * @return Void.
 *
 * @see lsr_link_new, lsr_link_d
 */
lsr_inline void lsr_link( lsr_link_t *pThis, lsr_link_t *next )
{   pThis->m_pNext = next;  pThis->m_pObj = NULL;  }

/**
 * @lsr_link_new
 * @brief Creates (allocates and initializes)
 *   a new singly linked list object.
 * 
 * @param[in] next - A pointer to the next linked object, which could be NULL.
 * @return A pointer to an initialized linked list object, else NULL on error.
 *
 * @see lsr_link, lsr_link_delete
 */
lsr_inline lsr_link_t * lsr_link_new( lsr_link_t *next )
{
    lsr_link_t * pThis;
    if ( ( pThis = (lsr_link_t *)
      lsr_palloc( sizeof(*pThis) ) ) != NULL )
    {
        lsr_link( pThis, next );
    }
    return pThis;
}

/**
 * @lsr_link_d
 * @brief Destroys a singly linked list object.
 * 
 * @param[in] pThis - A pointer to an initialized linked list object.
 * @return Void.
 *
 * @see lsr_link
 */
lsr_inline void lsr_link_d( lsr_link_t *pThis )
{}

/**
 * @lsr_link_delete
 * @brief Destroys a singly linked list object, then deletes the object.
 * @details The object should have been created with a previous
 *   successful call to lsr_link_new.
 * 
 * @param[in] pThis - A pointer to an initialized linked list object.
 * @return Void.
 *
 * @see lsr_link_new
 */
lsr_inline void lsr_link_delete( lsr_link_t *pThis )
{   lsr_link_d( pThis );  lsr_pfree( pThis );   }

/**
 * @lsr_link_getobj
 * @brief Gets the object referenced by a singly linked list object.
 * 
 * @param[in] pThis - A pointer to an initialized linked list object.
 * @return A pointer to the object referenced by the link object.
 */
lsr_inline void * lsr_link_getobj( const lsr_link_t *pThis )
{   return pThis->m_pObj;  }

/**
 * @lsr_link_setobj
 * @brief Sets the object referenced by a singly linked list object.
 * 
 * @param[in] pThis - A pointer to an initialized linked list object.
 * @param[in] pObj - A pointer to the object to be referenced.
 * @return Void.
 */
lsr_inline void lsr_link_setobj( lsr_link_t *pThis, void *pObj )
{   pThis->m_pObj = pObj;  }

/**
 * @lsr_link_next
 * @brief Gets the next object in a singly linked list relative to pThis.
 * 
 * @param[in] pThis - A pointer to an initialized linked list object.
 * @return A pointer to the next object in the list.
 *
 * @see lsr_link_setnext
 */
lsr_inline lsr_link_t * lsr_link_next( const lsr_link_t *pThis )
{   return pThis->m_pNext;  }

/**
 * @lsr_link_setnext
 * @brief Sets the next link object in a singly linked list relative to pThis.
 * 
 * @param[in] pThis - A pointer to an initialized linked list object.
 * @param[in] pNext - A pointer to the object to set as \e next.
 * @return Void.
 *
 * @see lsr_link_next
 */
lsr_inline void lsr_link_setnext( lsr_link_t *pThis, lsr_link_t *pNext )
{   pThis->m_pNext = pNext;  }

/**
 * @lsr_link_addnext
 * @brief Adds the next object to a singly linked list relative to pThis.
 * 
 * @param[in] pThis - A pointer to an initialized linked list object.
 * @param[in] pNext - A pointer to the object to add.
 * @return Void.
 *
 * @see lsr_link_removenext
 */
lsr_inline void lsr_link_addnext( lsr_link_t *pThis, lsr_link_t *pNext )
{
    lsr_link_t * pTemp = pThis->m_pNext;
    lsr_link_setnext( pThis, pNext );
    lsr_link_setnext( pNext, pTemp );
}

/**
 * @lsr_link_removenext
 * @brief Removes the \e next object from a singly linked list relative to pThis.
 * 
 * @param[in] pThis - A pointer to an initialized linked list object.
 * @return A pointer to the object removed (could be NULL).
 *
 * @see lsr_link_addnext
 */
lsr_inline lsr_link_t * lsr_link_removenext( lsr_link_t *pThis )
{
    lsr_link_t * pNext = pThis->m_pNext;
    if ( pNext )
    {
        lsr_link_setnext( pThis, pNext->m_pNext );
        lsr_link_setnext( pNext, NULL );
    }
    return pNext;
}


/**
 * @lsr_dlink
 * @brief Initializes a doubly linked list object.
 * 
 * @param[in] pThis - A pointer to an allocated doubly linked list object.
 * @param[in] prev - A pointer to the previous linked object, which could be NULL.
 * @param[in] next - A pointer to the next linked object, which could be NULL.
 * @return Void.
 *
 * @see lsr_dlink_new, lsr_dlink_d
 */
lsr_inline void lsr_dlink(
  lsr_dlink_t *pThis, lsr_dlink_t *prev, lsr_dlink_t *next )
{
    pThis->m_pNext = next;
    pThis->m_pPrev = prev;
    pThis->m_pObj = NULL;
}

/**
 * @lsr_dlink_new
 * @brief Creates (allocates and initializes)
 *   a new doubly linked list object.
 * 
 * @param[in] prev - A pointer to the previous linked object, which could be NULL.
 * @param[in] next - A pointer to the next linked object, which could be NULL.
 * @return A pointer to an initialized linked list object, else NULL on error.
 *
 * @see lsr_dlink, lsr_dlink_delete
 */
lsr_inline lsr_dlink_t * lsr_dlink_new( lsr_dlink_t *prev, lsr_dlink_t *next )
{
    lsr_dlink_t * pThis;
    if ( ( pThis = (lsr_dlink_t *)
      lsr_palloc( sizeof(*pThis) ) ) != NULL )
    {
        lsr_dlink( pThis, prev, next );
    }
    return pThis;
}

/**
 * @lsr_dlink_d
 * @brief Destroys a doubly linked list object.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list object.
 * @return Void.
 *
 * @see lsr_dlink
 */
lsr_inline void lsr_dlink_d( lsr_dlink_t *pThis )
{}

/**
 * @lsr_dlink_delete
 * @brief Destroys a doubly linked list object, then deletes the object.
 * @details The object should have been created with a previous
 *   successful call to lsr_dlink_new.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list object.
 * @return Void.
 *
 * @see lsr_dlink_new
 */
lsr_inline void lsr_dlink_delete( lsr_dlink_t *pThis )
{   lsr_dlink_d( pThis );  lsr_pfree( pThis );   }

/**
 * @lsr_dlink_getobj
 * @brief Gets the object referenced by a doubly linked list object.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list object.
 * @return A pointer to the object referenced by the link object.
 */
lsr_inline void * lsr_dlink_getobj( const lsr_dlink_t *pThis )
{   return pThis->m_pObj;  }

/**
 * @lsr_dlink_setobj
 * @brief Sets the object referenced by a doubly linked list object.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list object.
 * @param[in] pObj - A pointer to the object to be referenced.
 * @return Void.
 */
lsr_inline void lsr_dlink_setobj( lsr_dlink_t *pThis, void *pObj )
{   pThis->m_pObj = pObj;  }

/**
 * @lsr_dlink_prev
 * @brief Gets the previous object in a doubly linked list relative to pThis.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list object.
 * @return A pointer to the previous object in the list.
 *
 * @see lsr_dlink_setprev
 */
lsr_inline lsr_dlink_t * lsr_dlink_prev( const lsr_dlink_t *pThis )
{   return pThis->m_pPrev;  }

/**
 * @lsr_dlink_setprev
 * @brief Sets the previous object in a doubly linked list relative to pThis.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list object.
 * @param[in] pPrev - A pointer to the object to set as \e previous.
 * @return Void.
 *
 * @see lsr_dlink_prev
 */
lsr_inline void lsr_dlink_setprev( lsr_dlink_t *pThis, lsr_dlink_t *pPrev )
{   pThis->m_pPrev = pPrev;  }

/**
 * @lsr_dlink_next
 * @brief Gets the next object in a doubly linked list relative to pThis.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list object.
 * @return A pointer to the next object in the list.
 *
 * @see lsr_dlink_setnext
 */
lsr_inline lsr_dlink_t * lsr_dlink_next( const lsr_dlink_t *pThis )
{   return pThis->m_pNext;  }

/**
 * @lsr_dlink_setnext
 * @brief Sets the next object in a doubly linked list relative to pThis.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list object.
 * @param[in] pNext - A pointer to the object to set as \e next.
 * @return Void.
 *
 * @see lsr_dlink_next
 */
lsr_inline void lsr_dlink_setnext( lsr_dlink_t *pThis, lsr_dlink_t *pNext )
{   pThis->m_pNext = pNext;  }

/**
 * @lsr_dlink_addnext
 * @brief Adds the \e next object to a doubly linked list relative to pThis.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list object.
 * @param[in] pNext - A pointer to the object to add.
 * @return Void.
 *
 * @see lsr_dlink_removenext
 */
lsr_inline void lsr_dlink_addnext( lsr_dlink_t *pThis, lsr_dlink_t *pNext )
{
    assert( pNext );
    lsr_dlink_t * pTemp = lsr_dlink_next( pThis );
    lsr_dlink_setnext( pThis, pNext );
    lsr_dlink_setnext( pNext, pTemp );
    pNext->m_pPrev = pThis;
    if ( pTemp )
    {
        pTemp->m_pPrev = pNext;
    }
}
 
/**
 * @lsr_dlink_removenext
 * @brief Removes the \e next object from a doubly linked list relative to pThis.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list object.
 * @return A pointer to the object removed (could be NULL).
 *
 * @see lsr_dlink_addnext
 */
lsr_inline lsr_dlink_t * lsr_dlink_removenext( lsr_dlink_t *pThis )
{
    lsr_dlink_t * pTemp;
    lsr_dlink_t * pNext = lsr_dlink_next( pThis );
    if ( pNext )
    {
        pTemp = lsr_dlink_next( pNext );
        lsr_dlink_setnext( pThis, pTemp );
        if ( pTemp )
            pTemp->m_pPrev = pThis;
        memset( pNext, 0, sizeof( lsr_dlink_t ) );
    }
    return pNext;
}

/**
 * @lsr_dlink_addprev
 * @brief Adds the \e previous object to a doubly linked list relative to pThis.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list object.
 * @param[in] pPrev - A pointer to the object to add.
 * @return Void.
 *
 * @see lsr_dlink_removeprev
 */
lsr_inline void lsr_dlink_addprev( lsr_dlink_t *pThis, lsr_dlink_t *pPrev )
{
    assert( pPrev );
    lsr_dlink_t * pTemp;
    pTemp = pThis->m_pPrev;
    pThis->m_pPrev = pPrev;
    pPrev->m_pPrev = pTemp;
    lsr_dlink_setnext( pPrev, pThis );
    if ( pTemp )
    {
        lsr_dlink_setnext( pTemp, pPrev );
    }
}

/**
 * @lsr_dlink_removeprev
 * @brief Removes the \e previous object from a doubly linked list relative to pThis.
 * 
 * @param[in] pThis - A pointer to an initialized doubly linked list object.
 * @return A pointer to the object removed (could be NULL).
 *
 * @see lsr_dlink_addprev
 */
lsr_inline lsr_dlink_t * lsr_dlink_removeprev( lsr_dlink_t *pThis )
{
    lsr_dlink_t *pPrev = pThis->m_pPrev;
    if ( pThis->m_pPrev )
    {
        pThis->m_pPrev = pPrev->m_pPrev;
        if ( pThis->m_pPrev )
            lsr_dlink_setnext( pThis->m_pPrev, pThis );
        memset( pPrev, 0, sizeof( lsr_dlink_t ) );
    }
    return pPrev;
}

/**
 * @lsr_dlink_remove
 * @brief Removes an object from a doubly linked list.
 * 
 * @param[in] pThis - A pointer to the initialized doubly linked list object
 *   to remove.
 * @return A pointer to the \e next object in the list (could be NULL).
 *
 * @see lsr_dlink_removenext, lsr_dlink_removeprev
 */
lsr_inline lsr_dlink_t * lsr_dlink_remove( lsr_dlink_t *pThis )
{
    lsr_dlink_t * pNext = lsr_dlink_next( pThis );
    if ( pNext )
        pNext->m_pPrev = pThis->m_pPrev;
    if ( pThis->m_pPrev )
        lsr_dlink_setnext( pThis->m_pPrev, lsr_dlink_next( pThis ) );
    memset( pThis, 0, sizeof( lsr_dlink_t ) );
    return pNext;
}
                

#ifdef __cplusplus
}
#endif

#endif  /* LSR_LINK_H */

