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
#ifndef LSR_MAP_H
#define LSR_MAP_H

#include <lsr/lsr_types.h>
#include <lsr/lsr_xpool.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file
 */


#ifdef __cplusplus
extern "C" {
#endif 
    
//#define LSR_MAP_DEBUG
    

typedef struct lsr_mapnode_s lsr_mapnode_t;
typedef struct lsr_map_s lsr_map_t;


typedef lsr_mapnode_t *lsr_map_iter;

/**
 * @typedef lsr_map_val_comp
 * @brief The user must provide a comparison function for the key structure.
 * It will be used whenever comparisons need to be made.
 * 
 * @param[in] pVal1 - The first key to compare.
 * @param[in] pVal2 - The key to compare pVal1 against.
 * @return < 0 for pVal1 before pVal2, 0 for equal,
 * \> 0 for pVal1 after pVal2.
 */
typedef int (*lsr_map_val_comp)    ( const void *pVal1, const void *pVal2 );

/**
 * @typedef lsr_map_for_each_fn
 * @brief A function that the user wants to apply to every node in the map.
 * 
 * @param[in] pKey - The key from the current node.
 * @param[in] pValue - The value from the same node.
 * @return Should return 0 on success, not 0 on failure.
 */
typedef int (*lsr_map_for_each_fn) ( const void *pKey, void *pValue );

/**
 * @typedef lsr_map_for_each2_fn
 * @brief A function that the user wants to apply to every node in the map.
 * Also allows a user provided data for the function.
 * 
 * @param[in] pKey - The key from the current node.
 * @param[in] pValue - The value from the same node.
 * @param[in] pUData - The user data passed into the function.
 * @return Should return 0 on success, not 0 on failure.
 */
typedef int (*lsr_map_for_each2_fn)( const void *pKey, void *pValue, void *pUData );

typedef int (*lsr_map_insert_fn)   ( lsr_map_t *pThis, const void *pKey, void *pValue );
typedef void *(*lsr_map_update_fn) ( lsr_map_t *pThis, const void *pKey, void *pValue, lsr_map_iter node );
typedef lsr_map_iter (*lsr_map_find_fn)( lsr_map_t *pThis, const void *pKey );

/**
 * @typedef lsr_map_t
 * @brief A tree structure that follows the Red Black Tree structure.
 */
struct lsr_map_s
{
    size_t              m_size;
    lsr_mapnode_t      *m_root;
    lsr_map_val_comp    m_vc;
    lsr_xpool_t        *m_xpool;
    lsr_map_insert_fn   m_insert;
    // Use either key/value or node, node can be null if key/value put in.
    lsr_map_update_fn   m_update;
    lsr_map_find_fn     m_find;
};


/** @lsr_map_new
 * @brief Creates a new map.  Allocates from the global pool unless the pool parameter specifies a session pool.
 * Initializes the map according to the parameters.
 * @details The user may create his/her own val comp associated with his/her key structure,
 * but some sample ones for char * and ipv6 values are provided in lsr_hash.h.
 * @note If the user knows the map will only last for a session, a pointer to a
 * session pool may be passed in the pool parameter.
 * 
 * @param[in] vc - A pointer to the comparison function to use for the keys.
 * @param[in] pool - A session pool pointer, else NULL to specify the global pool.
 * @return A pointer to a new initialized map on success, NULL on failure.
 * 
 * @see lsr_map_delete, lsr_hash_cmp_string, lsr_hash_cmp_ci_string, 
 * lsr_hash_cmp_ipv6, lsr_map_val_comp
 */
lsr_map_t  *lsr_map_new( lsr_map_val_comp vc, lsr_xpool_t *pool );

/** @lsr_map
 * @brief Initializes the map.  Allocates from the global pool unless the pool parameter specifies a session pool.
 * @details The user may create his/her own val comp associated with his/her key structure,
 * but some sample ones for char * and ipv6 values are provided in lsr_hash.h.
 * @note If the user knows the map will only last for a session, a pointer to a
 * session pool may be passed in the pool parameter.
 * 
 * @param[in] pThis - A pointer to an allocated map.
 * @param[in] vc - A pointer to the comparison function to use for the keys.
 * @param[in] pool - A session pool pointer, else NULL to specify the global pool.
 * @return A pointer to the initialized map on success, NULL on failure.
 * 
 * @see lsr_map_d, lsr_hash_cmp_string, lsr_hash_cmp_ci_string, lsr_hash_cmp_ipv6, lsr_map_val_comp
 */
lsr_map_t  *lsr_map( lsr_map_t *pThis, lsr_map_val_comp vc, lsr_xpool_t *pool );

/** @lsr_map_d
 * @brief Destroys the map.  Does not free the map structure itself, only the internals.
 * @note This function should be used in conjunction with lsr_map.  
 * The user is responsible for freeing the data itself.  This function only frees structures
 * it allocated.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @return Void.
 * 
 * @see lsr_map
 */
void        lsr_map_d( lsr_map_t *pThis );

/** @lsr_map_delete
 * @brief Deletes the map.  Frees the map internals and the map structure itself.
 * @note This function should be used in conjunction with lsr_map_new.
 * The user is responsible for freeing the data itself.  This function only frees structures
 * it allocated.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @return Void.
 * 
 * @see lsr_map_new
 */
void        lsr_map_delete( lsr_map_t *pThis );

/** @lsr_map_get_node_key
 * @brief Gets the node's key.
 * 
 * @param[in] node - The node to extract the key from.
 * @return The key.
 */
const void *lsr_map_get_node_key( lsr_map_iter node );

/** @lsr_map_get_node_value
 * @brief Gets the node's value.
 * 
 * @param[in] node - The node to extract the value from.
 * @return The value.
 */
void       *lsr_map_get_node_value( lsr_map_iter node );  

/** @lsr_map_clear
 * @brief Empties the map of and frees the nodes and resets the size.
 * @note The values will not be freed in this function.  It is the user's
 * responsibility to free them.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @return Void.
 */
void        lsr_map_clear( lsr_map_t *pThis );

/** @lsr_map_release_nodes
 * @brief Releases node and any children it may have.
 * @note The values will not be freed in this function.  It is the user's
 * responsibility to free them.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @param[in] node - A pointer to the source node to release.
 * @return Void.
 */
void        lsr_map_release_nodes( lsr_map_t *pThis, lsr_map_iter node );

/** @lsr_map_swap
 * @brief Swaps the lhs and rhs maps.
 * 
 * @param[in,out] lhs - A pointer to an initialized map.
 * @param[in,out] rhs - A pointer to another initialized map.
 * @return Void.
 */
void        lsr_map_swap( lsr_map_t *lhs, lsr_map_t *rhs );

/** @lsr_map_insert
 * @brief Inserts a node with the given key and value in the map.
 * @note \b IMPORTANT: The key \b MUST \b BE a part of the value structure.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @param[in] pKey - A pointer to the key of the new node.
 * @param[in] pValue - A pointer to the value of the new node.
 * @return 0 if added or -1 if not.
 */
int             lsr_map_insert( lsr_map_t *pThis, const void *pKey, void *pValue );

/** @lsr_map_find
 * @brief Finds the node with the given key in the map.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @param[in] pKey - The key to search for.
 * @return A pointer to the node if found or NULL if not found.
 */
lsr_map_iter    lsr_map_find( lsr_map_t *pThis, const void *pKey );

/** @lsr_map_update
 * @brief Updates a node with the given key in the map.  Can use
 * the key or the node parameter to search with.
 * @note \b IMPORTANT: The key \b MUST \b BE a part of the value structure.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @param[in] pKey - The key of the node.
 * @param[in] pValue - The new value of the node.
 * @param[in] node - The node with the key/value to update.  The key must match
 * pKey.
 * @return The old value if updated or NULL if something went wrong.
 */
void           *lsr_map_update( lsr_map_t *pThis, const void *pKey, void *pValue,
                       lsr_map_iter node );

/** @lsr_map_delete_node
 * @brief Deletes the node from the map.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @param[in] node - A pointer to the node to delete.
 * @return The value from the node.
 */
void           *lsr_map_delete_node(  lsr_map_t *pThis, lsr_map_iter node );

/** @lsr_map_begin
 * @brief Gets the first node of the map.
 * 
 * @param[in] pThis - A pointer to an initialized source map.
 * @return The first node of the map.
 */
lsr_map_iter    lsr_map_begin( lsr_map_t *pThis );

/** @lsr_map_end
 * @brief Gets the end node of the map.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @return The end node of the map.
 */
lsr_map_iter    lsr_map_end( lsr_map_t *pThis );

/** @lsr_map_next
 * @brief Gets the next node of the map.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @param[in] node - A pointer to the current node.
 * @return A pointer to the next node of the map, NULL if it's the end.
 */
lsr_map_iter    lsr_map_next( lsr_map_t *pThis, lsr_map_iter node );

/** @lsr_map_for_each
 * @brief Runs a function for each node in the map.  The function must 
 * follow the #lsr_map_for_each_fn format.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @param[in] beg - A pointer to the first node to apply the function to.
 * @param[in] end - A pointer to the last node to apply the function to.
 * @param[in] fun - A pointer to the \link #lsr_map_for_each_fn function\endlink 
 * to apply to nodes.
 * @return The number of nodes that the function applied to.
 */
int lsr_map_for_each( lsr_map_t *pThis, lsr_map_iter beg, 
                      lsr_map_iter end, lsr_map_for_each_fn fun );

/** @lsr_map_for_each2
 * @brief Runs a function for each node in the map.  The function must 
 * follow the #lsr_map_for_each2_fn format.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @param[in] beg - A pointer to the first node to apply the function to.
 * @param[in] end - A pointer to the last node to apply the function to.
 * @param[in] fun - A pointer to the \link #lsr_map_for_each2_fn function\endlink 
 * to apply to nodes.
 * @param[in] pUData - A pointer to the user data to pass into the function.
 * @return The number of nodes that the function applied to.
 */
int lsr_map_for_each2( lsr_map_t *pThis, lsr_map_iter beg, 
                       lsr_map_iter end, lsr_map_for_each2_fn fun, void *pUData );

/** @lsr_map_empty
 * @brief Specifies whether or not the map is empty.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @return Non-zero if empty, 0 if not.
 */
lsr_inline int lsr_map_empty( const lsr_map_t *pThis )         
{    return pThis->m_size == 0;  }

/** @lsr_map_size
 * @brief Gets the current size of the map.
 * 
 * @param[in] pThis - A pointer to an initialized map.
 * @return The size of the map.
 */
lsr_inline size_t lsr_map_size( const lsr_map_t *pThis )        
{    return pThis->m_size;       }

/** @lsr_map_get_val_comp
 * @brief Gets the comparison function of the map.
 * 
 * @param[in] pThis - A pointer to an initialized source map.
 * @return A pointer to the comparison function.
 */
lsr_inline lsr_map_val_comp lsr_map_get_val_comp( lsr_map_t *pThis ) 
{    return pThis->m_vc;         }


#ifdef LSR_MAP_DEBUG
void lsr_map_print( lsr_map_iter node, int layer );
lsr_inline void lsr_map_printTree( lsr_map_t *pThis )    
{    lsr_map_print( pThis->m_root, 0 );    }
#endif

#ifdef __cplusplus
}
#endif 


#endif //LSR_MAP_H
