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


#include <lsr/lsr_map.h>
#include <lsr/lsr_internal.h>
#include <lsr/lsr_pool.h>


const void *lsr_map_get_node_key( lsr_map_iter node )  
{    return node->m_pKey;    }
void *lsr_map_get_node_value( lsr_map_iter node )  
{    return node->m_pValue;  }



static lsr_map_iter lsr_map_get_grandparent( lsr_map_iter node );
static lsr_map_iter lsr_map_get_uncle( lsr_map_iter node );
static void lsr_map_rotate_left( lsr_map_t *pThis, lsr_map_iter node );
static void lsr_map_rotate_right( lsr_map_t *pThis, lsr_map_iter node );
static void lsr_map_fix_tree( lsr_map_t *pThis, lsr_map_iter node );
static int lsr_map_insert_into_tree( lsr_map_iter pCurrent, lsr_map_iter pNew, lsr_map_val_comp vc );
static lsr_map_iter lsr_map_remove_end_node( lsr_map_t *pThis, lsr_map_iter node, char nullify );
static lsr_map_iter lsr_map_remove_node_from_tree( lsr_map_t *pThis, lsr_map_iter node );

lsr_inline void *lsr_map_do_alloc( lsr_xpool_t *pool, size_t size )
{
    return ( pool? lsr_xpool_alloc( pool, size ): lsr_palloc( size ) );
}

lsr_inline void lsr_map_do_free( lsr_xpool_t *pool, void *ptr )
{
    if ( pool )
        lsr_xpool_free( pool, ptr );
    else
        lsr_pfree( ptr );
    return;
}

lsr_map_t *lsr_map_new( lsr_map_val_comp vc, lsr_xpool_t *pool )
{
    lsr_map_t *pThis;
    pThis = (lsr_map_t *)lsr_map_do_alloc( pool, sizeof( lsr_map_t ));
    if ( pThis == NULL )
        return NULL;
    if ( !lsr_map( pThis, vc, pool ))
    {
        lsr_map_do_free( pool, pThis );
        return NULL;
    }
    return pThis;
}

lsr_map_t *lsr_map( lsr_map_t *pThis, lsr_map_val_comp vc, lsr_xpool_t *pool )
{
    pThis->m_size = 0;
    pThis->m_root = NULL;
    pThis->m_vc = vc;
    pThis->m_xpool = pool;
    pThis->m_insert = lsr_map_insert;
    pThis->m_update = lsr_map_update;
    pThis->m_find = lsr_map_find;
    
    assert( pThis->m_vc );
    return pThis;
}

void lsr_map_d( lsr_map_t *pThis )
{
    lsr_map_clear( pThis );
    memset( pThis, 0, sizeof( lsr_map_t ) );
}

void lsr_map_delete( lsr_map_t *pThis )
{
    lsr_map_clear( pThis );
    lsr_map_do_free( pThis->m_xpool, pThis );
}

void lsr_map_clear( lsr_map_t *pThis )
{
    lsr_map_release_nodes( pThis, pThis->m_root );
    pThis->m_root = NULL;
    pThis->m_size = 0;
}

void lsr_map_release_nodes( lsr_map_t *pThis, lsr_map_iter node )
{
    if ( node == NULL )
        return;
    if ( node->m_left != NULL )
        lsr_map_release_nodes( pThis, node->m_left );
    if ( node->m_right != NULL )
        lsr_map_release_nodes( pThis, node->m_right );
    lsr_map_do_free( pThis->m_xpool, node );
}

void lsr_map_swap( lsr_map_t *lhs, lsr_map_t *rhs )
{
    char temp[ sizeof( lsr_map_t ) ];
    assert( lhs != NULL && rhs != NULL );
    memmove( temp, lhs, sizeof( lsr_map_t ) );
    memmove( lhs, rhs, sizeof( lsr_map_t ) );
    memmove( rhs, temp, sizeof( lsr_map_t ) );
}

void *lsr_map_delete_node( lsr_map_t *pThis, lsr_map_iter node )
{
    void *val;
    lsr_map_iter ptr;
    if ( node == NULL )
        return NULL;
    val = node->m_pValue;
    ptr = lsr_map_remove_node_from_tree( pThis, node );
    --(pThis->m_size);
    lsr_map_do_free( pThis->m_xpool, ptr );
    return val;
}

lsr_map_iter lsr_map_begin( lsr_map_t *pThis )
{
    lsr_map_iter ptr = pThis->m_root;
    while( ptr->m_left != NULL )
        ptr = ptr->m_left;
    return ptr;
}

lsr_map_iter lsr_map_end( lsr_map_t *pThis )
{
    lsr_map_iter ptr = pThis->m_root;
    while( ptr->m_right != NULL )
        ptr = ptr->m_right;
    return ptr;
}

lsr_map_iter lsr_map_next( lsr_map_t *pThis, lsr_map_iter node )
{
    lsr_map_iter pUp;
    if ( node == NULL )
        return NULL;
    if ( node->m_right == NULL )
    {
        if ( node->m_parent != NULL )
        {
            if ( pThis->m_vc( node->m_pKey, node->m_parent->m_pKey ) > 0 )
            {
                pUp = node->m_parent;
                while( pUp->m_parent != NULL 
                    && pThis->m_vc( pUp->m_pKey, pUp->m_parent->m_pKey ) > 0 )
                    pUp = pUp->m_parent;
                if ( pUp->m_parent == NULL )
                    return NULL;
                return pUp->m_parent;
            }
            else
                return node->m_parent;
        }
        else
            return NULL;
    }
    
    node = node->m_right;
    while( node->m_left != NULL )
        node = node->m_left;
    return node;
}

int lsr_map_for_each( lsr_map_t *pThis, lsr_map_iter beg, lsr_map_iter end, lsr_map_for_each_fn fun )
{
    int iCount = 0;
    lsr_map_iter iterNext = beg;
    lsr_map_iter iter;
    
    if ( !fun )
    {
        errno = EINVAL;
        return -1;
    }
    if ( !beg )
        return 0;
    
    if ( !end )
        return 0;
    
    while( iterNext && (pThis->m_vc( iterNext->m_pKey, end->m_pKey ) <= 0))
    {
        iter = iterNext;
        iterNext = lsr_map_next( pThis, iterNext );
        if ( fun( iter->m_pKey, iter->m_pValue ))
            break;
        ++iCount;
    }
    return iCount;
}

int lsr_map_for_each2( lsr_map_t *pThis, lsr_map_iter beg, 
                        lsr_map_iter end, lsr_map_for_each2_fn fun, void *pUData )
{
    int iCount = 0;
    lsr_map_iter iterNext = beg;
    lsr_map_iter iter;
    
    if ( !fun )
    {
        errno = EINVAL;
        return -1;
    }
    if ( !beg )
        return 0;
    
    if ( !end )
        return 0;
    
    while( iterNext && (pThis->m_vc( iterNext->m_pKey, end->m_pKey ) <= 0))
    {
        iter = iterNext;
        iterNext = lsr_map_next( pThis, iterNext );
        if ( fun( iter->m_pKey, iter->m_pValue, pUData ))
            break;
        ++iCount;
    }
    return iCount;
}

int lsr_map_insert( lsr_map_t *pThis, const void *pKey, void *pValue )
{
    lsr_map_iter pNode;
    pNode = (lsr_map_iter)lsr_map_do_alloc( pThis->m_xpool, sizeof( lsr_mapnode_t ));
    if ( !pNode )
        return -1;
    
    pNode->m_pKey = pKey;
    pNode->m_pValue = pValue;
    pNode->m_left = NULL;
    pNode->m_right = NULL;
    pNode->m_parent = NULL;
    if ( pThis->m_root == NULL )
    {
        pNode->m_color = BLACK;
        pThis->m_root = pNode;
        ++pThis->m_size;
        return 0;
    }
    
    pNode->m_color = RED;
    if ( lsr_map_insert_into_tree( pThis->m_root, pNode, pThis->m_vc ) < 0 )
    {
        lsr_map_do_free( pThis->m_xpool, pNode );
        return -1;
    }
    
    lsr_map_fix_tree( pThis, pNode );
    pThis->m_root->m_color = BLACK;
    ++pThis->m_size;
    return 0;
}

lsr_map_iter lsr_map_find( lsr_map_t *pThis, const void *pKey )
{
    register lsr_map_iter ptr = pThis->m_root;
    register int iComp;
    if ( pThis == NULL )
        return NULL;
    if ( pThis->m_vc == NULL )
        return NULL;
    while( ptr != NULL && (iComp = pThis->m_vc( ptr->m_pKey, pKey )) != 0 )
        ptr = (iComp > 0 ? ptr->m_left : ptr->m_right);
    return ptr;
}

void *lsr_map_update( lsr_map_t *pThis, const void *pKey, void *pValue,
                       lsr_map_iter node )
{
    void *pOldValue;
    if ( node != NULL )
    {
        if ( pThis->m_vc( node->m_pKey, pKey ) != 0 )
            return NULL;
    }
    else if ((node = lsr_map_find( pThis, pKey )) == NULL )
        return NULL;
    
    node->m_pKey = pKey;
    pOldValue = node->m_pValue;
    node->m_pValue = pValue;
    return pOldValue;
}

// Internal Functions

static lsr_map_iter lsr_map_get_grandparent( lsr_map_iter node )
{
    if ( node != NULL && node->m_parent != NULL )
        return node->m_parent->m_parent;
    return NULL;
}

static lsr_map_iter lsr_map_get_uncle( lsr_map_iter node )
{
    lsr_map_iter pGrandparent = lsr_map_get_grandparent( node );
    if ( pGrandparent == NULL )
        return NULL;
    if ( node->m_parent == pGrandparent->m_left )
        return pGrandparent->m_right;
    return pGrandparent->m_left;
}

static void lsr_map_rotate_left( lsr_map_t *pThis, lsr_map_iter node )
{
    lsr_map_iter pSwap = node->m_right;
    node->m_right = pSwap->m_left;
    if ( pSwap->m_left != NULL )
        pSwap->m_left->m_parent = node;
    
    pSwap->m_parent = node->m_parent;
    if ( node->m_parent == NULL )
        pThis->m_root = pSwap;
    else
        if ( node == node->m_parent->m_left )
            node->m_parent->m_left = pSwap;
        else
            node->m_parent->m_right = pSwap;
    
    pSwap->m_left = node;
    node->m_parent = pSwap;
}

static void lsr_map_rotate_right( lsr_map_t *pThis, lsr_map_iter node )
{
    lsr_map_iter pSwap = node->m_left;
    node->m_left = pSwap->m_right;
    if ( pSwap->m_right != NULL )
        pSwap->m_right->m_parent = node;
    
    pSwap->m_parent = node->m_parent;
    if ( node->m_parent == NULL )
        pThis->m_root = pSwap;
    else
        if ( node == node->m_parent->m_left )
            node->m_parent->m_left = pSwap;
        else
            node->m_parent->m_right = pSwap;
    
    pSwap->m_right = node;
    node->m_parent = pSwap;
}

static void lsr_map_fix_tree( lsr_map_t *pThis, lsr_map_iter node )
{
    lsr_map_iter pGrandparent, pUncle;
    if ( node->m_parent == NULL )
    {
        node->m_color = BLACK;
        return;
    }
    else if ( node->m_parent->m_color == BLACK )
        return;
    else if (((pUncle = lsr_map_get_uncle( node )) != NULL) 
        && (pUncle->m_color == RED))
    {
        node->m_parent->m_color = BLACK;
        pUncle->m_color = BLACK;
        pGrandparent = lsr_map_get_grandparent( node );
        pGrandparent->m_color = RED;
        lsr_map_fix_tree( pThis, pGrandparent );
    }
    else
    {
        pGrandparent = lsr_map_get_grandparent( node );
        if ((node == node->m_parent->m_right)
            && (node->m_parent == pGrandparent->m_left))
        {
            lsr_map_rotate_left( pThis, node->m_parent );
            node = node->m_left;
        }
        else if ((node == node->m_parent->m_left)
            && (node->m_parent == pGrandparent->m_right))
        {
            lsr_map_rotate_right( pThis, node->m_parent );
            node = node->m_right;
        }
        pGrandparent = lsr_map_get_grandparent( node );
        node->m_parent->m_color = BLACK;
        pGrandparent->m_color = RED;
        
        if ( node == node->m_parent->m_left )
            lsr_map_rotate_right( pThis, pGrandparent );
        else
            lsr_map_rotate_left( pThis, pGrandparent );
    }
}

static int lsr_map_insert_into_tree( lsr_map_iter pCurrent, lsr_map_iter pNew, lsr_map_val_comp vc )
{
    int iComp = vc( pCurrent->m_pKey, pNew->m_pKey );
    if ( iComp < 0 )
        if ( pCurrent->m_right == NULL )
        {
            pCurrent->m_right = pNew;
            pNew->m_parent = pCurrent;
            return 0;
        }
        else
            return lsr_map_insert_into_tree( pCurrent->m_right, pNew, vc );
    else if ( iComp > 0 )
        if ( pCurrent->m_left == NULL )
        {
            pCurrent->m_left = pNew;
            pNew->m_parent = pCurrent;
            return 0;
        }
        else
            return lsr_map_insert_into_tree( pCurrent->m_left, pNew, vc );
    else
        return -1;
}

static lsr_map_iter lsr_map_remove_end_node( lsr_map_t *pThis, lsr_map_iter node, char nullify )
{
    lsr_map_iter pSibling, pParent = node->m_parent;
    if ( pParent == NULL )
    {
        pThis->m_root = NULL;
        return node;
    }
    
    /* A sibling must exist because there can't be two black nodes 
     * in a row without two on the other side to balance it.
     */
    pSibling = ( node == pParent->m_left ? pParent->m_right : pParent->m_left );
    
    if ( pSibling->m_color == RED )
    {
        pParent->m_color = RED;
        pSibling->m_color = BLACK;
        if ( node == pParent->m_left )
            lsr_map_rotate_left( pThis, pParent );
        else
            lsr_map_rotate_right( pThis, pParent );
        pSibling = ( node == pParent->m_left ? pParent->m_right : pParent->m_left );
    }
    
    // From this point on, Sibling must be black.
    if (((pSibling->m_left == NULL) || (pSibling->m_left->m_color == BLACK))
        && ((pSibling->m_right == NULL) || (pSibling->m_right->m_color == BLACK))) 
    {
        pSibling->m_color = RED;
        if ( nullify == 1 )
        {
            if ( node == pParent->m_left )
                pParent->m_left = NULL;
            else
                pParent->m_right = NULL;
        }
        
        if ( pParent->m_color == BLACK ) 
            lsr_map_remove_end_node( pThis, pParent, 0 );
        else
            pParent->m_color = BLACK;
        
        return node;
    }
    
    // From this point on, at least one of Sibling's children is red.
    if ((node == pParent->m_left)
        && (pSibling->m_left != NULL)
        && (pSibling->m_left->m_color == RED)) 
    {
        pSibling->m_color = RED;
        pSibling->m_left->m_color = BLACK;
        lsr_map_rotate_right( pThis, pSibling );
    }
    else if ((node == pParent->m_right)
        && (pSibling->m_right != NULL)
        && (pSibling->m_right->m_color == RED)) 
    {
        pSibling->m_color = RED;
        pSibling->m_right->m_color = BLACK;
            lsr_map_rotate_left( pThis, pSibling );
    }
    
    pSibling = ( node == pParent->m_left ? pParent->m_right : pParent->m_left );
    
    /* From earlier, sibling must be black and we have two cases here:
     * - Node is Parent's left child and Sibling's right child is red.
     * - Node is Parent's right child and Sibling's left child is red.
     */
    
    pSibling->m_color = pParent->m_color;
    pParent->m_color = BLACK;
    
    if ( node == pParent->m_left ) 
    {
        pSibling->m_right->m_color = BLACK;
        lsr_map_rotate_left( pThis, pParent );
        if ( nullify == 1 )
            pParent->m_left = NULL;
    }
    else 
    {
        pSibling->m_left->m_color = BLACK;
        lsr_map_rotate_right( pThis, pParent );
        if ( nullify == 1 )
            pParent->m_right = NULL;
    }
    return node;
}

static lsr_map_iter lsr_map_remove_node_from_tree( lsr_map_t *pThis, lsr_map_iter node )
{
    lsr_map_iter pPtrParent, pChild, ptr = node;
    //Replace with successor if there are 2 children.
    if ( node->m_left != NULL && node->m_right != NULL )
    {
        pPtrParent = ptr;
        ptr = ptr->m_right;
        while( ptr->m_left != NULL )
        {
            pPtrParent = ptr;
            ptr = ptr->m_left;
        }
        node->m_pKey = ptr->m_pKey;
        node->m_pValue = ptr->m_pValue;
    }
    else
        pPtrParent = node->m_parent;
    
    //Now we want to delete Ptr from tree
    if ( ptr->m_color == RED )
    {
        if ( ptr == pPtrParent->m_left )
            pPtrParent->m_left = NULL;
        else
            pPtrParent->m_right = NULL;
        return ptr;
    }
    
    pChild = ( ptr->m_left == NULL ? ptr->m_right : ptr->m_left );
    
    if ((pChild != NULL) 
        && (pChild->m_color == RED))
    {
        if ( pPtrParent == NULL )
            pThis->m_root = pChild;
        else if ( ptr == pPtrParent->m_left )
            pPtrParent->m_left = pChild;
        else
            pPtrParent->m_right = pChild;
        
        pChild->m_parent = pPtrParent;
        pChild->m_color = BLACK;
        return ptr;
    }
    
    //At this point, we know ptr has no children, otherwise there will 
    //be some invalid properties in the tree.
    return lsr_map_remove_end_node( pThis, ptr, 1 );
}






#ifdef LSR_MAP_DEBUG
void lsr_map_print( lsr_map_iter node, int layer )
{
    int iLeft, iRight, iMyLayer = layer;
    const char *sColor, *sLColor, *sRColor;
    if ( node == NULL )
        return;
    sColor = node->m_color == BLACK ? "B" : "R";
    if ( node->m_left == NULL )
    {
        iLeft = 0;
        sLColor = "";
    }
    else
    {
        iLeft = (int)node->m_left->m_pKey;
        sLColor = node->m_left->m_color == BLACK ? "B" : "R";
    }
    
    if ( node->m_right == NULL )
    {
        iRight = 0;
        sRColor = "";
    }
    else
    {
        iRight = (int)node->m_right->m_pKey;
        sRColor = node->m_right->m_color == BLACK ? "B" : "R";
    }
    printf( "Layer: %d, %d%s [ left = %d%s; right = %d%s ]\n", 
           iMyLayer, (int)node->m_pKey, sColor, iLeft, sLColor, iRight, sRColor );
    lsr_map_print( node->m_left, ++layer );
    lsr_map_print( node->m_right, layer );
}
#endif





