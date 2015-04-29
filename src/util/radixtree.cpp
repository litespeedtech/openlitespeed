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
#include "radixtree.h"

#include <lsdef.h>
#include <lsr/ls_str.h>
#include <util/ghash.h>

#include <assert.h>
#include <string.h>

#include <new>

#define RNSTATE_NOCHILD     0
#define RNSTATE_CNODE       1
#define RNSTATE_PNODE       2
#define RNSTATE_CARRAY      3
#define RNSTATE_PARRAY      4
#define RNSTATE_HASH        5
#define RN_USEHASHCNT      10

struct rnheader_s
{
    RadixNode  *body;
    size_t      len;
    char        label[0];
};

struct rnparams_s
{
    ls_xpool_t *pool;
    const char *label;
    size_t      labellen;
    int         mode;
    void       *obj;
};

#define rnh_size(iLen) (sizeof(void *) + sizeof(size_t) + iLen)

#define rnh_roundup(size) ((size + 4 - 1) & ~(4 - 1))

#define rnh_roundedsize(iLen) (rnh_roundup(rnh_size(iLen)))


/**
 * This function currently only works for contexts.
 * Cases checked:
 * - No more children.
 * - '/' is the last character.
 * Length of the current label is stored in iChildLen.
 * Return value is if there are any children left.
 */
static int snNextOffset(const char *pLabel, size_t iLabelLen,
                        size_t &iChildLen)
{
    const char *ptr = (const char *)memchr(pLabel, '/', iLabelLen);
    if (ptr == NULL)
        iChildLen = iLabelLen;
    else if ((iChildLen = ptr - pLabel) < iLabelLen - 1)
        return 1;
    return 0;
}


int RadixTree::setRootLabel(const char *pLabel, size_t iLabelLen)
{
    if (pLabel == NULL || iLabelLen == 0)
        return LS_FAIL;
    if (m_pRoot == NULL)
        m_pRoot = (rnheader_t *)ls_xpool_alloc(&m_pool, rnh_size(iLabelLen));
    else
        m_pRoot = (rnheader_t *)ls_xpool_realloc(&m_pool, m_pRoot,
                                                 rnh_size(iLabelLen));
    if (m_pRoot == NULL)
        return LS_FAIL;
    m_pRoot->body = NULL;
    m_pRoot->len = iLabelLen;
    memmove(m_pRoot->label, pLabel, iLabelLen);
    m_pRoot->label[iLabelLen] = '\0';
    return LS_OK;
}


int RadixTree::checkPrefix(const char *pLabel, size_t iLabelLen) const
{
    if (m_pRoot == NULL)
        return LS_FAIL;
    if (m_pRoot->len > iLabelLen)
        return LS_FAIL;
    return memcmp(m_pRoot->label, pLabel, m_pRoot->len);
}


RadixNode *RadixTree::insert(const char *pLabel, size_t iLabelLen, void *pObj)
{
    RadixNode *pDest = NULL;
    rnparams_t myParams;
    if (m_pRoot == NULL)
        return NULL;
    if (memcmp(pLabel, m_pRoot->label, m_pRoot->len) != 0)
        return NULL;
    myParams.pool = &m_pool;
    myParams.label = pLabel + m_pRoot->len;
    myParams.labellen = iLabelLen - m_pRoot->len;
    myParams.mode = m_iMode;
    myParams.obj = pObj;
    if (myParams.labellen == 0)
    {
        if (m_pRoot->body == NULL)
            m_pRoot->body = RadixNode::newNode(&m_pool, NULL, pObj);
        else if (m_pRoot->body->getObj() == NULL)
            m_pRoot->body->setObj(pObj);
        else
            return NULL;
        pDest = m_pRoot->body;
    }
    else if (m_pRoot->body != NULL)
        return m_pRoot->body->insert(&myParams);
    else
        m_pRoot->body = RadixNode::newBranch(&myParams, NULL, pDest);
    return pDest;
}

void *RadixTree::update(const char *pLabel, size_t iLabelLen, void *pObj) const
{
    if ((checkPrefix(pLabel, iLabelLen) != 0) || (m_pRoot->body == NULL))
        return NULL;
    return m_pRoot->body->update(pLabel + m_pRoot->len,
                                 iLabelLen - m_pRoot->len, pObj);
}

void *RadixTree::find(const char *pLabel, size_t iLabelLen) const
{
    if ((checkPrefix(pLabel, iLabelLen) != 0) || (m_pRoot->body == NULL))
        return NULL;
    return m_pRoot->body->find(pLabel + m_pRoot->len,
                               iLabelLen - m_pRoot->len);
}

void *RadixTree::bestMatch(const char *pLabel, size_t iLabelLen) const
{
    if ((checkPrefix(pLabel, iLabelLen) != 0) || (m_pRoot->body == NULL))
        return NULL;
    return m_pRoot->body->bestMatch(pLabel + m_pRoot->len,
                                    iLabelLen - m_pRoot->len);
}

int RadixTree::for_each(rn_foreach fun)
{
    if (m_pRoot != NULL && m_pRoot->body != NULL)
        return m_pRoot->body->for_each(fun);
    return 0;
}

int RadixTree::for_each2(rn_foreach2 fun, void *pUData)
{
    if (m_pRoot != NULL && m_pRoot->body != NULL)
        return m_pRoot->body->for_each2(fun, pUData);
    return 0;
}

void RadixTree::printTree()
{
    if (m_pRoot == NULL)
        return;
    if (m_pRoot->body == NULL)
    {
        printf("Only root prefix: %.*s\n", m_pRoot->len, m_pRoot->label);
        return;
    }
    printf("%.*s  ->%p\n", m_pRoot->len, m_pRoot->label,
           m_pRoot->body->getObj());
    m_pRoot->body->printChildren(2);
}


RadixNode::RadixNode(RadixNode *pParent, void *pObj)
    : m_iNumChildren(0)
    , m_iState(RNSTATE_NOCHILD)
    , m_pObj(pObj)
    , m_pParent(pParent)
    , m_pCHeaders(NULL)
{}


void *RadixNode::getParentObj()
{
    if (m_pParent == NULL)
        return NULL;
    else if (m_pParent->m_pObj != NULL)
        return m_pParent->m_pObj;
    return m_pParent->getParentObj();
}


/**
 * Insert will check to see what type of children setup is being used,
 * and will parse the existing children to see if the new one matches.
 * If the new one does match, it will call checkLevel to handle the situation.
 * If it doesn't match, then it will create a new node.  Based on the number
 * of children, it may also change the setup type.
 */
RadixNode *RadixNode::insert(rnparams_t *pParams)
{
    int iHasChildren = 1;
    size_t iOffset, iChildLen;
    rnheader_t *pHeader, **pArray;
    rnparams_t myParams;
    RadixNode *pDest = NULL;
    ls_xpool_t *pool = pParams->pool;
    iHasChildren = snNextOffset(pParams->label, pParams->labellen, iChildLen);
    myParams.pool = pool;
    myParams.label = pParams->label + iChildLen + 1;
    myParams.labellen = pParams->labellen - iChildLen - 1;
    myParams.mode = pParams->mode;
    myParams.obj = pParams->obj;

    switch (getState())
    {
    case RNSTATE_NOCHILD:
        pHeader = (rnheader_t *)ls_xpool_alloc(pool, rnh_size(iChildLen + 1));
        if (pHeader == NULL)
            return NULL;
        if ((pDest = fillNode(pParams, pHeader, iHasChildren,
                              iChildLen)) == NULL)
        {
            ls_xpool_free(pool, pHeader);
            return NULL;
        }
        m_pCHeaders = pHeader;
        if (pParams->mode == RTMODE_CONTIGUOUS)
            setState(RNSTATE_CNODE);
        else
            setState(RNSTATE_PNODE);
        incrNumChildren();
        return pDest;
    case RNSTATE_CNODE:
        pHeader = (rnheader_t *)m_pCHeaders;
        if ((m_pCHeaders->len == iChildLen)
            && (memcmp(m_pCHeaders->label, pParams->label,
                       m_pCHeaders->len) == 0))
        {
            return checkLevel(&myParams, m_pCHeaders->body, iHasChildren);
        }
        iOffset = rnh_roundedsize(m_pCHeaders->len + 1);
        pHeader = (rnheader_t *)ls_xpool_realloc(pool, m_pCHeaders, iOffset
                                            + rnh_roundedsize(iChildLen + 1));
        if (pHeader == NULL)
            return NULL;
        m_pCHeaders = pHeader;
        pHeader = (rnheader_t *)((char *)m_pCHeaders + iOffset);

        if ((pDest = fillNode(pParams, pHeader, iHasChildren,
                              iChildLen)) == NULL)
            return NULL;
        setState(RNSTATE_CARRAY);
        incrNumChildren();
        return pDest;
    case RNSTATE_PNODE:
        pHeader = (rnheader_t *)m_pCHeaders;
        if ((pHeader->len == iChildLen)
            && (memcmp(pHeader->label, pParams->label, pHeader->len) == 0))
        {
            return checkLevel(&myParams, pHeader->body, iHasChildren);
        }

        pHeader = (rnheader_t *)ls_xpool_alloc(pool, rnh_size(iChildLen + 1));
        if (pHeader == NULL)
            return NULL;
        pArray = (rnheader_t **)ls_xpool_alloc(pool, sizeof(rnheader_t *)
                                               * (RN_USEHASHCNT >> 1));
        if (pArray == NULL)
        {
            ls_xpool_free(pool, pHeader);
            return NULL;
        }
        if ((pDest = fillNode(pParams, pHeader, iHasChildren,
                              iChildLen)) == NULL)
        {
            ls_xpool_free(pool, pHeader);
            ls_xpool_free(pool, pArray);
            return NULL;
        }
        pArray[0] = m_pCHeaders;
        pArray[1] = pHeader;
        m_pPHeaders = pArray;
        setState(RNSTATE_PARRAY);
        incrNumChildren();
        return pDest;
    case RNSTATE_CARRAY:
        return insertCArray(pParams, &myParams, iHasChildren, iChildLen);
    case RNSTATE_PARRAY:
        return insertPArray(pParams, &myParams, iHasChildren, iChildLen);
    case RNSTATE_HASH:
        return insertHash(pParams, &myParams, iHasChildren, iChildLen);
    default:
        return NULL;
    }
    return NULL;
}


void *RadixNode::update(const char *pLabel, size_t iLabelLen, void *pObj)
{
    void *pTmp;
    RadixNode *pNode;
    assert(pObj != NULL);
    if (iLabelLen == 0)
    {
        if (m_pObj == NULL)
            return NULL;
        pTmp = m_pObj;
        m_pObj = pObj;
        return pTmp;
    }
    if ((pNode = findChild(pLabel, iLabelLen, 0)) == NULL)
        return NULL;
    pTmp = pNode->getObj();
    pNode->setObj(pObj);
    return pTmp;
}


void *RadixNode::find(const char *pLabel, size_t iLabelLen)
{
    RadixNode *pNode;
    if (iLabelLen == 0)
        return m_pObj;
    if ((pNode = findChild(pLabel, iLabelLen, 0)) == NULL)
        return NULL;
    return pNode->getObj();
}


void *RadixNode::bestMatch(const char *pLabel, size_t iLabelLen)
{
    RadixNode *pNode;
    if (iLabelLen == 0)
        return m_pObj;
    if ((pNode = findChild(pLabel, iLabelLen, 1)) == NULL)
        return m_pObj;
    return pNode->getObj();
}


int RadixNode::for_each(rn_foreach fun)
{
    int incr = 0;
    if (getObj() != NULL)
    {
        if (fun(m_pObj) != 0)
            return 0;
        incr = 1;
    }
    return for_each_child(fun) + incr;
}


int RadixNode::for_each_child(rn_foreach fun)
{
    int i, count = 0;
    rnheader_t *pHeader;
    GHash::iterator iter;
    switch (getState())
    {
    case RNSTATE_CNODE:
    case RNSTATE_PNODE:
        return count + m_pCHeaders->body->for_each(fun);
    case RNSTATE_CARRAY:
        pHeader = m_pCHeaders;
        for (i = 0; i < m_iNumChildren; ++i)
        {
            count += pHeader->body->for_each(fun);
            pHeader = (rnheader_t *)(&pHeader->label[0]
                                    + rnh_roundup(pHeader->len + 1));
        }
        break;
    case RNSTATE_PARRAY:
        for (i = 0; i < m_iNumChildren; ++i)
        {
            pHeader = m_pPHeaders[i];
            count += pHeader->body->for_each(fun);
        }
        break;
    case RNSTATE_HASH:
        iter = m_pHash->begin();
        while (iter != m_pHash->end())
        {
            pHeader = (rnheader_t *)iter->second();
            count += pHeader->body->for_each(fun);
            iter = m_pHash->next(iter);
        }
        break;
    default:
        break;
    }
    return count;
}


int RadixNode::for_each2(rn_foreach2 fun, void *pUData)
{
    int incr = 0;
    if (getObj() != NULL)
    {
        if (fun(m_pObj, pUData) != 0)
            return 0;
        incr = 1;
    }
    return for_each_child2(fun, pUData) + incr;
}


int RadixNode::for_each_child2(rn_foreach2 fun, void *pUData)
{
    int i, count = 0;
    rnheader_t *pHeader;
    GHash::iterator iter;
    switch (m_iState)
    {
    case RNSTATE_CNODE:
    case RNSTATE_PNODE:
        return count + m_pCHeaders->body->for_each2(fun, pUData);
    case RNSTATE_CARRAY:
        pHeader = m_pCHeaders;
        for (i = 0; i < m_iNumChildren; ++i)
        {
            count += pHeader->body->for_each2(fun, pUData);
            pHeader = (rnheader_t *)(&pHeader->label[0]
                                    + rnh_roundup(pHeader->len + 1));
        }
        break;
    case RNSTATE_PARRAY:
        for (i = 0; i < m_iNumChildren; ++i)
        {
            pHeader = m_pPHeaders[i];
            count += pHeader->body->for_each2(fun, pUData);
        }
        break;
    case RNSTATE_HASH:
        iter = m_pHash->begin();
        while (iter != m_pHash->end())
        {
            pHeader = (rnheader_t *)iter->second();
            count += pHeader->body->for_each2(fun, pUData);
            iter = m_pHash->next(iter);
        }
        break;
    default:
        break;
    }
    return count;
}


RadixNode *RadixNode::newNode(ls_xpool_t *pool, RadixNode *pParent, void *pObj)
{
    void *ptr = ls_xpool_alloc(pool, sizeof(RadixNode));
    if (ptr == NULL)
        return NULL;
    return new (ptr) RadixNode(pParent, pObj);
}


/**
* This function handles the creation of RadixNodes.  This should only
* be necessary if the branch is not found in the current tree.
* pDest will have the RadixNode that contains the object.
* Return value is new child.
*/
RadixNode *RadixNode::newBranch(rnparams_t *pParams, RadixNode *pParent,
                                RadixNode *&pDest)
{
    RadixNode *pMyNode;
    rnheader_t *pMyChildHeader;
    int iHasChildren;
    size_t iChildLen;
    ls_xpool_t *pool = pParams->pool;
    pMyNode = newNode(pool, pParent, NULL);

    iHasChildren = snNextOffset(pParams->label, pParams->labellen, iChildLen);
    pMyChildHeader = (rnheader_t *)ls_xpool_alloc(pool,
                                                  rnh_size(iChildLen + 1));
    if (pMyChildHeader == NULL)
    {
        ls_xpool_free(pool, pMyNode);
        return NULL;
    }

    if ((pDest = pMyNode->fillNode(pParams, pMyChildHeader, iHasChildren,
                                   iChildLen)) == NULL)
    {
        ls_xpool_free(pool, pMyChildHeader);
        ls_xpool_free(pool, pMyNode);
        return NULL;
    }
    pMyNode->m_pCHeaders = pMyChildHeader;
    if (pParams->mode == RTMODE_CONTIGUOUS)
        pMyNode->setState(RNSTATE_CNODE);
    else
        pMyNode->setState(RNSTATE_PNODE);
    pMyNode->incrNumChildren();
    return pMyNode;
}


/**
 * This function checks the rnheader after a match.  It will check if it needs
 * to continue on or if it should just stop and insert the object where it is.
 * If it needs to continue on, it will also check if the node already has
 * children, and if it does, insert the new one amongst them,
 * else create a new branch.
 */
RadixNode *RadixNode::checkLevel(rnparams_t *pParams, RadixNode *pNode,
                                 int iHasChildren)
{
    if (iHasChildren == 0)
    {
        if (pNode->getObj() != NULL)
            return NULL;
        pNode->setObj(pParams->obj);
        return pNode;
    }
    return pNode->insert(pParams);
}


/**
 * This function is just to reduce repetition.  It fills in the node
 * with the appropriate values passed in.
 * If it has children, it will also create the new branch.
 * Return value is the RadixNode that contains the Object.
 */
RadixNode *RadixNode::fillNode(rnparams_t *pParams, rnheader_t *pHeader,
                               int iHasChildren,  size_t iChildLen)
{
    rnparams_t myParams;
    RadixNode *pDest = NULL;
    pHeader->len = iChildLen;
    memmove(pHeader->label, pParams->label, iChildLen);
    pHeader->label[iChildLen] = '\0';
    if (iHasChildren != 0)
    {
        myParams.label = pParams->label + iChildLen + 1;
        myParams.labellen = pParams->labellen - iChildLen - 1;
        myParams.mode = pParams->mode;
        myParams.obj = pParams->obj;
        myParams.pool = pParams->pool;

        pHeader->body = newBranch(&myParams, this, pDest);
        if (pHeader->body == NULL)
            return NULL;
    }
    else
    {
        pDest = newNode(pParams->pool, this, pParams->obj);
        if (pDest == NULL)
            return NULL;

        pHeader->body = pDest;
    }
    return pDest;
}


RadixNode *RadixNode::insertPArray(rnparams_t *pParams, rnparams_t *myParams,
                                   int iHasChildren, size_t iChildLen)
{
    int i;
    void *ptr;
    GHash *pHash;
    rnheader_t *pHeader, **pArray = NULL;
    RadixNode *pDest = NULL;
    ls_xpool_t *pool = pParams->pool;

    for (i = 0; i < m_iNumChildren; ++i)
    {
        pHeader = m_pPHeaders[i];
        if ((iChildLen == pHeader->len)
            && (memcmp(pHeader->label, pParams->label, pHeader->len) == 0))
        {
            return checkLevel(myParams, pHeader->body, iHasChildren);
        }
    }
    if (getNumChildren() < RN_USEHASHCNT)
    {
        if (getNumChildren() == RN_USEHASHCNT >> 1)
        {
            pArray = (rnheader_t **)ls_xpool_realloc(pool, m_pPHeaders,
                                sizeof(rnheader_t *) * RN_USEHASHCNT);
            if (pArray == NULL)
                return NULL;
            m_pPHeaders = pArray;
        }

        pHeader = (rnheader_t *)ls_xpool_alloc(pool, rnh_size(iChildLen + 1));
        if (pHeader == NULL)
            return NULL;
        if ((pDest = fillNode(pParams, pHeader, iHasChildren,
                              iChildLen)) == NULL)
        {
            ls_xpool_free(pool, pHeader);
            return NULL;
        }
        m_pPHeaders[m_iNumChildren++] = pHeader;
        return pDest;
    }
    ptr = ls_xpool_alloc(pool, sizeof(GHash));
    pHash = new (ptr) GHash(256, GHash::hfString, GHash::cmpString, pool);
    for (i = 0; i < m_iNumChildren; ++i)
    {
        pHeader = m_pPHeaders[i];
        if (pHash->insert(pHeader->label, pHeader) == NULL)
        {
            pHash->~GHash();
            ls_xpool_free(pool, pHash);
            return NULL;
        }
    }

    pHeader = (rnheader_t *)ls_xpool_alloc(pool, rnh_size(iChildLen + 1));
    if (pHeader == NULL)
    {
        pHash->~GHash();
        ls_xpool_free(pool, pHash);
        return NULL;
    }
    if (((pDest = fillNode(pParams, pHeader, iHasChildren, iChildLen)) == NULL)
        || (pHash->insert(pHeader->label, pHeader) == NULL))
    {
        pHash->~GHash();
        ls_xpool_free(pool, pHash);
        ls_xpool_free(pool, pHeader);
        return NULL;
    }
    ls_xpool_free(pool, m_pPHeaders);
    m_pHash = pHash;
    setState(RNSTATE_HASH);
    incrNumChildren();
    return pDest;
}


RadixNode *RadixNode::insertCArray(rnparams_t *pParams, rnparams_t *myParams,
                                   int iHasChildren, size_t iChildLen)
{
    int i;
    size_t iOffset;
    void *ptr;
    GHash *pHash;
    RadixNode *pDest = NULL;
    ls_xpool_t *pool = pParams->pool;
    rnheader_t *pHeader = (rnheader_t *)m_pCHeaders;

    for (i = 0; i < m_iNumChildren; ++i)
    {
        if ((iChildLen == pHeader->len)
            && (memcmp(pHeader->label, pParams->label, pHeader->len) == 0))
        {
            return checkLevel(myParams, pHeader->body, iHasChildren);
        }

        pHeader = (rnheader_t *)(&pHeader->label[0]
                                 + rnh_roundup(pHeader->len + 1));
    }
    iOffset = (char *)pHeader - (char *)m_pCHeaders;
    if (getNumChildren() < RN_USEHASHCNT)
    {

        pHeader = (rnheader_t *)ls_xpool_realloc(pool, m_pCHeaders, iOffset
                                            + rnh_roundedsize(iChildLen + 1));
        if (pHeader == NULL)
            return NULL;
        m_pCHeaders = pHeader;
        pHeader = (rnheader_t *)((char *)m_pCHeaders + iOffset);
        if ((pDest = fillNode(pParams, pHeader, iHasChildren,
                              iChildLen)) == NULL)
            return NULL;
        incrNumChildren();
        return pDest;
    }
    ptr = ls_xpool_alloc(pool, sizeof(GHash));
    pHash = new (ptr) GHash(256, GHash::hfString, GHash::cmpString, pool);

    pHeader = (rnheader_t *)m_pCHeaders;
    for (i = 0; i < m_iNumChildren; ++i)
    {
        if (pHash->insert(pHeader->label, pHeader) == NULL)
        {
            pHash->~GHash();
            ls_xpool_free(pool, pHash);
            return NULL;
        }

        pHeader = (rnheader_t *)(&pHeader->label[0]
                                 + rnh_roundup(pHeader->len + 1));
    }

    pHeader = (rnheader_t *)ls_xpool_alloc(pool, rnh_size(iChildLen + 1));
    if (pHeader == NULL)
    {
        pHash->~GHash();
        ls_xpool_free(pool, pHash);
        return NULL;
    }
    if (((pDest = fillNode(pParams, pHeader, iHasChildren, iChildLen)) == NULL)
        || (pHash->insert(pHeader->label, pHeader) == NULL))
    {
        pHash->~GHash();
        ls_xpool_free(pool, pHash);
        ls_xpool_free(pool, pHeader);
        return NULL;
    }
    m_pHash = pHash;
    setState(RNSTATE_HASH);
    incrNumChildren();
    return pDest;
}


RadixNode *RadixNode::insertHash(rnparams_t *pParams, rnparams_t *myParams,
                                 int iHasChildren, size_t iChildLen)
{
    GHash::iterator pHashNode;
    char match, *pMatch;
    rnheader_t *pHeader;
    RadixNode *pDest;
    ls_xpool_t *pool = pParams->pool;

    pMatch = (char *)pParams->label;
    match = pMatch[iChildLen];
    pMatch[iChildLen] = '\0';
    pHashNode = m_pHash->find(pMatch);
    pMatch[iChildLen] = match;
    if (pHashNode != NULL)
    {
        pHeader = (rnheader_t *)pHashNode->getData();
        return checkLevel(myParams, pHeader->body, iHasChildren);
    }

    pHeader = (rnheader_t *)ls_xpool_alloc(pool, rnh_size(iChildLen));
    if (pHeader == NULL)
        return NULL;
    if (((pDest = fillNode(pParams, pHeader, iHasChildren, iChildLen)) == NULL)
        || (m_pHash->insert(pHeader->label, pHeader) == NULL))
    {
        ls_xpool_free(pool, pHeader);
        return NULL;
    }
    incrNumChildren();
    return pDest;
}


/**
 * Does the findChild function for arrays.  This version is for the array
 * that is stored contiguously in memory.
 */
RadixNode *RadixNode::findArray(const char *pLabel, size_t iLabelLen,
                                size_t iChildLen, int iHasChildren, int iMatch)
{
    int i;
    rnheader_t *pHeader = m_pCHeaders;
    for (i = 0; i < m_iNumChildren; ++i)
    {
        if ((iChildLen == pHeader->len)
            && (memcmp(pHeader->label, pLabel, pHeader->len) == 0))
        {
            return findChildData(pLabel + iChildLen + 1,
                                 iLabelLen - iChildLen - 1, pHeader->body,
                                 iHasChildren, iMatch);
        }

        pHeader = (rnheader_t *)(&pHeader->label[0]
                                 + rnh_roundup(pHeader->len + 1));
    }
    if (iMatch == 0 || getObj() == NULL)
        return NULL;
    return this;
}


/**
 * This function compares labels of the search target and current children.
 * If found, it will call findChildData, which may recurse back to findChild.
 */
RadixNode *RadixNode::findChild(const char *pLabel, size_t iLabelLen,
                                int iMatch)
{
    int i, iHasChildren;
    size_t iChildLen, iGCLen;
    GHash::iterator pHashNode;
    rnheader_t *pHeader;
    char match, *pMatch;
    const char *pGC;
    RadixNode *pOut;

    if (iMatch == 0 || m_pObj == NULL)
        pOut = NULL;
    else
        pOut = this;

    iHasChildren = snNextOffset(pLabel, iLabelLen, iChildLen);
    pGC = pLabel + iChildLen + 1;
    iGCLen = iLabelLen - iChildLen - 1;
    switch (getState())
    {
    case RNSTATE_CNODE:
    case RNSTATE_PNODE:
        if (iChildLen < m_pCHeaders->len)
            return pOut;
        if (memcmp(m_pCHeaders->label, pLabel, iChildLen) != 0)
            return pOut;

        pHeader = m_pCHeaders;
        break;
    case RNSTATE_CARRAY:
        return findArray(pLabel, iLabelLen, iChildLen, iHasChildren, iMatch);
    case RNSTATE_PARRAY:
        for (i = 0; i < m_iNumChildren; ++i)
        {
            pHeader = m_pPHeaders[i];
            if ((iChildLen == pHeader->len)
                && (memcmp(pHeader->label, pLabel, pHeader->len) == 0))
            {
                return findChildData(pGC, iGCLen, pHeader->body, iHasChildren,
                                     iMatch);
            }
        }
        return pOut;
    case RNSTATE_HASH:
        pMatch = (char *)pLabel;
        match = pMatch[iChildLen];
        pMatch[iChildLen] = '\0';
        pHashNode = m_pHash->find(pMatch);
        pMatch[iChildLen] = match;
        if (pHashNode == NULL)
            return pOut;

        pHeader = (rnheader_t *)pHashNode->getData();
        break;
    default:
        return pOut;
    }
    return findChildData(pGC, iGCLen, pHeader->body, iHasChildren, iMatch);
}


/**
 * This function is called when a child matches.  It will check if
 * it needs to recurse deeper, and if it doesn't, returns the pointer to
 * the object.
 */
RadixNode *RadixNode::findChildData(const char *pLabel, size_t iLabelLen,
                                RadixNode *pNode, int iHasChildren, int iMatch)
{
    RadixNode *pOut;
    if (iHasChildren == 0)
        pOut = pNode;
    else if (pNode->hasChildren())
        pOut = pNode->findChild(pLabel, iLabelLen, iMatch);
    else if (iMatch == 0)
        return NULL;
    else
        pOut = pNode;

    if ((iMatch == 0)
        || (pOut != NULL && pOut->m_pObj != NULL))
        return pOut;
    else if (m_pObj != NULL)
        return this;
    return NULL;
}


/**
 * For debugging, the for_each function for the hash table.
 */
int RadixNode::printHash(const void *key, void *data, void *extra)
{
    rnheader_t *pHeader = (rnheader_t *)data;
    unsigned long offset = (unsigned long)extra;
    unsigned int i;
    for (i = 0; i < offset; ++i)
        printf("|");
    printf("%.*s  ->", pHeader->len, pHeader->label);
    printf("%p\n", pHeader->body->getObj());
    if (pHeader->body->hasChildren())
        pHeader->body->printChildren(offset + 2);

    return 0;
}


/**
 * For debugging.
 */
void RadixNode::printChildren(long unsigned int offset)
{
    unsigned int i;
    int j;
    rnheader_t *pHeader;
    if (!hasChildren())
        return;
    switch (getState())
    {
        case RNSTATE_CNODE:
        case RNSTATE_PNODE:
            for (i = 0; i < offset; ++i)
                printf("|");
            printf("%.*s  ->", m_pCHeaders->len, m_pCHeaders->label);
            printf("%p\n", m_pCHeaders->body->getObj());
            if (m_pCHeaders->body->hasChildren())
                m_pCHeaders->body->printChildren(offset + 2);
            break;
        case RNSTATE_CARRAY:
            pHeader = m_pCHeaders;
            for (j = 0; j < m_iNumChildren; ++j)
            {
                for (i = 0; i < offset; ++i)
                    printf("|");
                printf("%.*s  ->", pHeader->len, pHeader->label);
                printf("%p\n", pHeader->body->getObj());
                if (pHeader->body->hasChildren())
                    pHeader->body->printChildren(offset + 2);

            pHeader = (rnheader_t *)(&pHeader->label[0]
                                     + rnh_roundup(pHeader->len + 1));
            }
            break;
        case RNSTATE_PARRAY:
            for (j = 0; j < m_iNumChildren; ++j)
            {
                pHeader = m_pPHeaders[j];
                for (i = 0; i < offset; ++i)
                    printf("|");
                printf("%.*s  ->", pHeader->len, pHeader->label);
                printf("%p\n", pHeader->body->getObj());
                if (pHeader->body->hasChildren())
                    pHeader->body->printChildren(offset + 2);
            }
            break;
        case RNSTATE_HASH:
            m_pHash->for_each2(m_pHash->begin(), m_pHash->end(), printHash,
                               (void *)offset);
            break;
        default:
            return;
    }
}





