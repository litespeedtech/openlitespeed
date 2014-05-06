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

#include <util/gmap.h>

GMap::GMap( val_comp vc )
    :m_size( 0 ),
    m_root( NULL ),
    m_vc( vc )
{
    m_insert = insertNode;
    m_update = updateNode;
    m_find = findNode;
}

GMap::~GMap()
{
    clear();
}

void GMap::clear()
{
    releaseNodes( m_root );
    m_root = NULL;
    m_size = 0;
}

void GMap::swap( GMap& rhs )
{
    char temp[ sizeof( GMap ) ];
    memmove( temp, this, sizeof( GMap ) );
    memmove( this, &rhs, sizeof( GMap ) );
    memmove( &rhs, temp, sizeof( GMap ) );
}

void GMap::releaseNodes( iterator node )
{
    if ( node == NULL )
        return;
    if ( node->m_left != NULL )
        releaseNodes( node->m_left );
    if ( node->m_right != NULL )
        releaseNodes( node->m_right );
    free( node );
}

GMap::iterator GMap::begin()
{
    iterator ptr = this->m_root;
    while( ptr->m_left != NULL )
        ptr = ptr->m_left;
    return ptr;
}

GMap::iterator GMap::end()
{
    iterator ptr = this->m_root;
    while( ptr->m_right != NULL )
        ptr = ptr->m_right;
    return ptr;
}

GMap::iterator GMap::next( iterator iter )
{
    if ( iter == NULL )
        return NULL;
    if ( iter->m_right == NULL )
    {
        if ( iter->m_parent != NULL )
        {
            if ( m_vc( iter->m_pKey, iter->m_parent->m_pKey ) > 0 )
                return NULL;
            else
                return iter->m_parent;
        }
        else
            return NULL;
    }
    iter = iter->m_right;
    while( iter->m_left != NULL )
        iter = iter->m_left;
    return iter;
}

int GMap::for_each( iterator beg, iterator end, for_each_fn fun )
{
    int iCount = 0;
    iterator iterNext = beg;
    iterator iter ;
    
    if ( !fun )
    {
        errno = EINVAL;
        return -1;
    }
    if ( !beg )
        return 0;
    
    if ( !end )
        return 0;
    
    while( iterNext && (m_vc( iterNext->getKey(), end->getKey() ) <= 0) )
    {
        iter = iterNext;
        iterNext = next( iterNext );
        if ( fun( iter ) )
            break;
        ++iCount;
    }
    return iCount;
}

int GMap::for_each2( iterator beg, iterator end, for_each_fn2 fun, void* pUData )
{
    int iCount = 0;
    iterator iterNext = beg;
    iterator iter ;
    if ( !fun )
    {
        errno = EINVAL;
        return -1;
    }
    if ( !beg )
        return 0;
    
    if ( !end )
        return 0;
    
    while( iterNext && (m_vc( iterNext->getKey(), end->getKey() ) <= 0) )
    {
        iter = iterNext;
        iterNext = next( iterNext );
        if ( fun( iter, pUData ) )
            break;
        ++iCount;
    }
    return iCount;
}

GMap::iterator GMap::findNode( GMap* pThis, const void* pKey )
{
    register iterator ptr = pThis->m_root;
    register int iComp;
    if ( pThis == NULL )
        return NULL;
    if ( pThis->m_vc == NULL )
        return NULL;
    while( ptr != NULL && (iComp = pThis->m_vc( ptr->m_pKey, pKey )) != 0)
        ptr = (iComp > 0 ? ptr->m_left : ptr->m_right);
    return ptr;
}

void* GMap::updateNode( GMap* pThis, const void* pKey, void* pValue, iterator node )
{
    void *pOldValue;
    if ( node != NULL )
    {
        if ( pThis->m_vc( node->m_pKey, pKey ) != 0 )
            return NULL;
    }
    else if ( (node = findNode( pThis, pKey )) == NULL )
        return NULL;
    
    node->m_pKey = pKey;
    pOldValue = node->m_pValue;
    node->m_pValue = pValue;
    return pOldValue;
}

void GMap::fixTree( GMap* pThis, iterator node )
{
    iterator pGrandparent, pUncle;
    if ( node->m_parent == NULL )
    {
        node->m_color = GMapNode::black;
        return;
    }
    else if ( node->m_parent->m_color == GMapNode::black )
        return;
    else if ( ((pUncle = getUncle( node ) ) != NULL) 
        && (pUncle->m_color == GMapNode::red) 
    )
    {
        node->m_parent->m_color = GMapNode::black;
        pUncle->m_color = GMapNode::black;
        pGrandparent = getGrandparent( node );
        pGrandparent->m_color = GMapNode::red;
        fixTree( pThis, pGrandparent );
    }
    else
    {
        pGrandparent = getGrandparent( node );
        if ( (node == node->m_parent->m_right)
            && (node->m_parent == pGrandparent->m_left)
        )
        {
            rotateLeft( pThis, node->m_parent );
            node = node->m_left;
        }
        else if ( (node == node->m_parent->m_left)
            && (node->m_parent == pGrandparent->m_right)
        )
        {
            rotateRight( pThis, node->m_parent );
            node = node->m_right;
        }
        pGrandparent = getGrandparent( node );
        node->m_parent->m_color = GMapNode::black;
        pGrandparent->m_color = GMapNode::red;
        
        if ( node == node->m_parent->m_left )
            rotateRight( pThis, pGrandparent );
        else
            rotateLeft( pThis, pGrandparent );
    }
}

int GMap::insertNode( GMap* pThis, const void* pKey, void* pValue )
{
    iterator pNode;
    pNode = (iterator)malloc( sizeof(GMapNode) );
    if ( !pNode )
        return -1;
    
    pNode->m_pKey = pKey;
    pNode->m_pValue = pValue;
    pNode->m_left = NULL;
    pNode->m_right = NULL;
    pNode->m_parent = NULL;
    if ( pThis->m_root == NULL )
    {
        pNode->m_color = GMapNode::black;
        pThis->m_root = pNode;
        ++pThis->m_size;
        return 0;
    }
    
    pNode->m_color = GMapNode::red;
    if ( insertIntoTree( pThis->m_root, pNode, pThis->m_vc ) < 0 )
    {
        free( pNode );
        return -1;
    }
    
    fixTree( pThis, pNode );
    pThis->m_root->m_color = GMapNode::black;
    ++pThis->m_size;
    return 0;
}

int GMap::insertIntoTree( iterator pCurrent, iterator pNew, val_comp vc )
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
            return insertIntoTree( pCurrent->m_right, pNew, vc );
    else if ( iComp > 0 )
        if ( pCurrent->m_left == NULL )
        {
            pCurrent->m_left = pNew;
            pNew->m_parent = pCurrent;
            return 0;
        }
        else
            return insertIntoTree( pCurrent->m_left, pNew, vc );
    else
        return -1;
}

void* GMap::deleteNode( iterator node )
{
    void *val;
    iterator ptr;
    if ( node == NULL )
        return NULL;
    val = node->m_pValue;
    
    ptr = removeNodeFromTree( this, node );
    --this->m_size;
    free( ptr );
    return val;
}

GMap::iterator GMap::removeNodeFromTree( GMap* pThis, iterator node )
{
    iterator pPtrParent, pChild, ptr = node;
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
    if ( ptr->m_color == GMapNode::red )
    {
        if ( ptr == pPtrParent->m_left )
            pPtrParent->m_left = NULL;
        else
            pPtrParent->m_right = NULL;
        return ptr;
    }
    
    pChild = ( ptr->m_left == NULL ? ptr->m_right : ptr->m_left );
    
    if ( (pChild != NULL) 
        && (pChild->m_color == GMapNode::red) 
    )
    {
        if ( pPtrParent == NULL )
            pThis->m_root = pChild;
        else
            if ( ptr == pPtrParent->m_left )
                pPtrParent->m_left = pChild;
            else
                pPtrParent->m_right = pChild;
        
        pChild->m_parent = pPtrParent;
        pChild->m_color = GMapNode::black;
        return ptr;
    }
    
    //At this point, we know ptr has no children, otherwise there will 
    //be some invalid properties in the tree.
    return removeEndNode( pThis, ptr, 1 );
}

GMap::iterator GMap::removeEndNode( GMap* pThis, iterator node, char nullify )
{
    iterator pSibling, pParent = node->m_parent;
    if ( pParent == NULL )
    {
        pThis->m_root = NULL;
        return node;
    }
    
    /* A sibling must exist because there can't be two black nodes 
     * in a row without two on the other side to balance it.
     */
    pSibling = ( node == pParent->m_left ? pParent->m_right : pParent->m_left );
    
    if ( pSibling->m_color == GMapNode::red )
    {
        pParent->m_color = GMapNode::red;
        pSibling->m_color = GMapNode::black;
        if ( node == pParent->m_left )
            rotateLeft( pThis, pParent );
        else
            rotateRight( pThis, pParent );
        pSibling = ( node == pParent->m_left ? pParent->m_right : pParent->m_left );
    }
    
    // From this point on, Sibling must be black.
    if ( ((pSibling->m_left == NULL) || (pSibling->m_left->m_color == GMapNode::black))
        && ((pSibling->m_right == NULL) || (pSibling->m_right->m_color == GMapNode::black))
    ) 
    {
        pSibling->m_color = GMapNode::red;
        if ( nullify == 1 )
        {
            if ( node == pParent->m_left )
                pParent->m_left = NULL;
            else
                pParent->m_right = NULL;
        }
        
        if ( pParent->m_color == GMapNode::black ) 
            removeEndNode( pThis, pParent, 0 );
        else
            pParent->m_color = GMapNode::black;
        
        return node;
    }
    
    // From this point on, at least one of Sibling's children is red.
    if ( (node == pParent->m_left)
        && (pSibling->m_left != NULL)
        && (pSibling->m_left->m_color == GMapNode::red)
    ) 
    {
        pSibling->m_color = GMapNode::red;
        pSibling->m_left->m_color = GMapNode::black;
        rotateRight( pThis, pSibling );
    }
    else if ( (node == pParent->m_right)
        && (pSibling->m_right != NULL)
        && (pSibling->m_right->m_color == GMapNode::red)
    ) 
    {
        pSibling->m_color = GMapNode::red;
        pSibling->m_right->m_color = GMapNode::black;
        rotateLeft( pThis, pSibling );
    }
    
    pSibling = ( node == pParent->m_left ? pParent->m_right : pParent->m_left );
    
    /* From earlier, sibling must be black and we have two cases here:
     * - Node is Parent's left child and Sibling's right child is red.
     * - Node is Parent's right child and Sibling's left child is red.
     */
    
    pSibling->m_color = pParent->m_color;
    pParent->m_color = GMapNode::black;
    
    if ( node == pParent->m_left ) 
    {
        pSibling->m_right->m_color = GMapNode::black;
        rotateLeft( pThis, pParent );
        if ( nullify == 1 )
            pParent->m_left = NULL;
    }
    else 
    {
        pSibling->m_left->m_color = GMapNode::black;
        rotateRight( pThis, pParent );
        if ( nullify == 1 )
            pParent->m_right = NULL;
    }
    return node;
}

GMap::iterator GMap::getGrandparent( iterator node )
{
    if ( node != NULL && node->m_parent != NULL )
        return node->m_parent->m_parent;
    return NULL;
}

GMap::iterator GMap::getUncle( iterator node )
{
    iterator pGrandparent = getGrandparent( node );
    if ( pGrandparent == NULL )
        return NULL;
    if ( node->m_parent == pGrandparent->m_left )
        return pGrandparent->m_right;
    return pGrandparent->m_left;
}

void GMap::rotateLeft( GMap* pThis, iterator node )
{
    iterator pSwap = node->m_right;
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

void GMap::rotateRight( GMap* pThis, iterator node )
{
    iterator pSwap = node->m_left;
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

#ifdef GMAP_DEBUG
void GMap::print( iterator node, int layer )
{
    int iLeft, iRight, iMyLayer = layer;
    const char *sColor, *sLColor, *sRColor;
    if ( node == NULL )
        return;
    sColor = node->m_color == GMapNode::black ? "B" : "R";
    if ( node->m_left == NULL )
    {
        iLeft = 0;
        sLColor = "";
    }
    else
    {
        iLeft = (int)node->m_left->m_pKey;
        sLColor = node->m_left->m_color == GMapNode::black ? "B" : "R";
    }
    
    if ( node->m_right == NULL )
    {
        iRight = 0;
        sRColor = "";
    }
    else
    {
        iRight = (int)node->m_right->m_pKey;
        sRColor = node->m_right->m_color == GMapNode::black ? "B" : "R";
    }
    printf( "Layer: %d, %d%s [ left = %d%s; right = %d%s ]\n", 
           iMyLayer, (int)node->getKey(), sColor, iLeft, sLColor, iRight, sRColor );
    print( node->m_left, ++layer );
    print( node->m_right, layer );
}
#endif


