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
#include <lsr/ls_pool.h>
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
#define RN_HASHSZ         256

typedef int (*rtcmpfn)(const char *p1, const char *p2, size_t len);

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
    int         flags;
    rtcmpfn     cmp;
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
static int rnNextOffset(const char *pLabel, size_t iLabelLen,
                        size_t &iChildLen)
{
    const char *ptr = (const char *)memchr(pLabel, '/', iLabelLen);
    if (ptr == NULL)
        iChildLen = iLabelLen;
    else if ((iChildLen = ptr - pLabel) < iLabelLen - 1)
        return 1;
    return 0;
}


static void *rnDoAlloc(int iFlags, ls_xpool_t *pool, int iSize)
{
    if (((iFlags & RTFLAG_GLOBALPOOL) == 0)
        && (pool != NULL))
        return ls_xpool_alloc(pool, iSize);
    return ls_palloc(iSize);
}


static void *rnDoRealloc(int iFlags, ls_xpool_t *pool, void *pOld, int iSize)
{
    if ((iFlags & RTFLAG_GLOBALPOOL) == 0)
        return ls_xpool_realloc(pool, pOld, iSize);
    return ls_prealloc(pOld, iSize);
}


static void rnDoFree(int iFlags, ls_xpool_t *pool, void *ptr)
{
    if ((iFlags & RTFLAG_GLOBALPOOL) == 0)
        ls_xpool_free(pool, ptr);
    else
        ls_pfree(ptr);
}


int RadixTree::setRootLabel(const char *pLabel, size_t iLabelLen)
{
    if (pLabel == NULL || iLabelLen == 0)
        return LS_FAIL;
    if (m_pRoot == NULL)
        m_pRoot = (rnheader_t *)rnDoAlloc(m_iFlags, &m_pool,
                                          rnh_size(iLabelLen));
    else
        m_pRoot = (rnheader_t *)rnDoRealloc(m_iFlags, &m_pool, m_pRoot,
                                            rnh_size(iLabelLen));
    if (m_pRoot == NULL)
        return LS_FAIL;
    m_pRoot->body = NULL;
    m_pRoot->len = iLabelLen;
    memmove(m_pRoot->label, pLabel, iLabelLen);
    m_pRoot->label[iLabelLen] = '\0';
    return LS_OK;
}


void RadixTree::setNoContext()
{
    if (m_pRoot == NULL)
    {
        m_pRoot = (rnheader_t *)rnDoAlloc(m_iFlags, &m_pool,
                                          sizeof(rnheader_t));
        m_pRoot->body = NULL;
    }
    m_pRoot->label[0] = '\0';
    m_pRoot->len = 0;
    m_iFlags |= RTFLAG_NOCONTEXT;
}


int RadixTree::checkPrefix(const char *pLabel, size_t iLabelLen) const
{
    if (m_pRoot == NULL)
        return LS_FAIL;
    if (m_pRoot->len > iLabelLen)
        return LS_FAIL;
    if ((m_iFlags & RTFLAG_REGEXCMP) != 0)
        return strncmp(m_pRoot->label, pLabel, m_pRoot->len);
    else if ((m_iFlags & RTFLAG_CICMP) != 0)
        return strncasecmp(m_pRoot->label, pLabel, m_pRoot->len);
    else
        return strncmp(m_pRoot->label, pLabel, m_pRoot->len);
}


RadixNode *RadixTree::insert(const char *pLabel, size_t iLabelLen, void *pObj)
{
    RadixNode *pDest = NULL;
    rnparams_t myParams;
    if (m_pRoot == NULL)
        return NULL;
    if ((m_iFlags & RTFLAG_NOCONTEXT) == 0)
    {
        if (checkPrefix(pLabel, iLabelLen) != 0)
            return NULL;
        myParams.label = pLabel + m_pRoot->len;
        myParams.labellen = iLabelLen - m_pRoot->len;
    }
    else
    {
        myParams.label = pLabel;
        myParams.labellen = iLabelLen;
    }
    if ((m_iFlags & RTFLAG_GLOBALPOOL) == 0)
        myParams.pool = &m_pool;
    else
        myParams.pool = NULL;
    if ((m_iFlags & RTFLAG_REGEXCMP) != 0)
        myParams.cmp = strncmp;
    else if ((m_iFlags & RTFLAG_CICMP) != 0)
        myParams.cmp = strncasecmp;
    else
        myParams.cmp = strncmp;
    myParams.mode = m_iMode;
    myParams.flags = m_iFlags;
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


void *RadixTree::erase(const char *pLabel, size_t iLabelLen) const
{
    if ((((m_iFlags & RTFLAG_NOCONTEXT) == 0)
            && (checkPrefix(pLabel, iLabelLen) != 0))
        || (m_pRoot->body == NULL))
        return NULL;
    return m_pRoot->body->erase(pLabel + m_pRoot->len,
                                iLabelLen - m_pRoot->len, m_iFlags);
}


void *RadixTree::update(const char *pLabel, size_t iLabelLen, void *pObj) const
{
    if ((((m_iFlags & RTFLAG_NOCONTEXT) == 0)
            && (checkPrefix(pLabel, iLabelLen) != 0))
        || (m_pRoot->body == NULL))
        return NULL;
    return m_pRoot->body->update(pLabel + m_pRoot->len,
                                 iLabelLen - m_pRoot->len, pObj, m_iFlags);
}


void *RadixTree::find(const char *pLabel, size_t iLabelLen) const
{
    if ((((m_iFlags & RTFLAG_NOCONTEXT) == 0)
            && (checkPrefix(pLabel, iLabelLen) != 0))
        || (m_pRoot->body == NULL))
        return NULL;
    return m_pRoot->body->find(pLabel + m_pRoot->len, iLabelLen - m_pRoot->len,
                               m_iFlags);
}


void *RadixTree::bestMatch(const char *pLabel, size_t iLabelLen) const
{
    if ((((m_iFlags & RTFLAG_NOCONTEXT) == 0)
            && (checkPrefix(pLabel, iLabelLen) != 0))
        || (m_pRoot->body == NULL))
        return NULL;
    return m_pRoot->body->find(pLabel + m_pRoot->len, iLabelLen - m_pRoot->len,
                               m_iFlags | RTFLAG_BESTMATCH);
}


int RadixTree::for_each(rn_foreach fun)
{
    if (m_pRoot != NULL && m_pRoot->body != NULL)
        return m_pRoot->body->for_each(fun, m_pRoot->label, m_pRoot->len);
    return 0;
}


int RadixTree::for_each2(rn_foreach2 fun, void *pUData)
{
    if (m_pRoot != NULL && m_pRoot->body != NULL)
        return m_pRoot->body->for_each2(fun, pUData, m_pRoot->label,
                                        m_pRoot->len);
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
    , m_pParent(pParent)
    , m_pObj(pObj)
    , m_pCHeaders(NULL)
{
    ls_str(&m_label, NULL, 0);
}


void *RadixNode::getParentObj()
{
    if (m_pParent == NULL)
        return NULL;
    else if (m_pParent->m_pObj != NULL)
        return m_pParent->m_pObj;
    return m_pParent->getParentObj();
}


RadixNode *RadixNode::insert(ls_xpool_t *pool, const char *pLabel,
                             size_t iLabelLen, void *pObj, int iFlags)
{
    rnparams_t myParams;
    if (iLabelLen == 0)
    {
        if (m_pObj == NULL)
            m_pObj = pObj;
        return NULL;
    }
    myParams.pool = pool;
    myParams.label = pLabel;
    myParams.labellen = iLabelLen;
    myParams.mode = RTMODE_CONTIGUOUS;
    myParams.flags = iFlags;
    myParams.obj = pObj;
    if ((iFlags & RTFLAG_REGEXCMP) != 0)
        myParams.cmp = strncmp;
    else if ((iFlags & RTFLAG_CICMP) != 0)
        myParams.cmp = strncasecmp;
    else
        myParams.cmp = strncmp;
    return insert(&myParams);
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
    int iHasChildren = 0;
    size_t iOffset, iChildLen;
    rnheader_t *pHeader, **pArray;
    rnparams_t myParams;
    RadixNode *pDest = NULL;
    ls_xpool_t *pool = pParams->pool;
    if ((pParams->flags & RTFLAG_NOCONTEXT) == 0)
    {
        iHasChildren = rnNextOffset(pParams->label, pParams->labellen,
                                    iChildLen);
        myParams.pool = pool;
        myParams.label = pParams->label + iChildLen + 1;
        myParams.labellen = pParams->labellen - iChildLen - 1;
        myParams.mode = pParams->mode;
        myParams.flags = pParams->flags;
        myParams.cmp = pParams->cmp;
    }
    else
    {
        if (pParams->label[pParams->labellen - 1] == '/')
            iChildLen = pParams->labellen - 1;
        else
            iChildLen = pParams->labellen;
    }
    myParams.obj = pParams->obj;

    switch (getState())
    {
    case RNSTATE_NOCHILD:
        pHeader = (rnheader_t *)rnDoAlloc(pParams->flags, pool,
                                          rnh_size(iChildLen + 1));
        if (pHeader == NULL)
            return NULL;
        if ((pDest = fillNode(pParams, pHeader, iHasChildren,
                              iChildLen)) == NULL)
        {
            rnDoFree(pParams->flags, pool, pHeader);
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
            && (pParams->cmp(m_pCHeaders->label, pParams->label,
                             m_pCHeaders->len) == 0))
        {
            return checkLevel(&myParams, m_pCHeaders->body, iHasChildren);
        }
        iOffset = rnh_roundedsize(m_pCHeaders->len + 1);
        pHeader = (rnheader_t *)rnDoRealloc(pParams->flags, pool, m_pCHeaders,
                                    iOffset + rnh_roundedsize(iChildLen + 1));
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
            && (pParams->cmp(pHeader->label, pParams->label,
                             pHeader->len) == 0))
        {
            return checkLevel(&myParams, pHeader->body, iHasChildren);
        }

        pHeader = (rnheader_t *)rnDoAlloc(pParams->flags, pool,
                                          rnh_size(iChildLen + 1));
        if (pHeader == NULL)
            return NULL;
        pArray = (rnheader_t **)rnDoAlloc(pParams->flags, pool,
                                sizeof(rnheader_t *) * (RN_USEHASHCNT >> 1));
        if (pArray == NULL)
        {
            rnDoFree(pParams->flags, pool, pHeader);
            return NULL;
        }
        if ((pDest = fillNode(pParams, pHeader, iHasChildren,
                              iChildLen)) == NULL)
        {
            rnDoFree(pParams->flags, pool, pHeader);
            rnDoFree(pParams->flags, pool, pArray);
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


void *RadixNode::erase(const char *pLabel, size_t iLabelLen, int iFlags)
{
    void *pObj;
    RadixNode *pNode;
    if (iLabelLen == 0)
    {
        pObj = m_pObj;
        m_pObj = NULL;
        return pObj;
    }
    if ((pNode = findChild(pLabel, iLabelLen, iFlags)) == NULL)
        return NULL;
    pObj = pNode->getObj();
    pNode->setObj(NULL);
    return pObj;
}


void *RadixNode::update(const char *pLabel, size_t iLabelLen, void *pObj,
                        int iFlags)
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
    if ((pNode = findChild(pLabel, iLabelLen, iFlags)) == NULL)
        return NULL;
    pTmp = pNode->getObj();
    pNode->setObj(pObj);
    return pTmp;
}


void *RadixNode::find(const char *pLabel, size_t iLabelLen, int iFlags)
{
    RadixNode *pNode;
    if (iLabelLen == 0)
        return m_pObj;
    if ((pNode = findChild(pLabel, iLabelLen, iFlags)) == NULL)
        return ((iFlags & RTFLAG_BESTMATCH) == 0 ? NULL : m_pObj);
    return pNode->getObj();
}


void *RadixNode::bestMatch(const char *pLabel, size_t iLabelLen, int iFlags)
{
    return find(pLabel, iLabelLen, iFlags | RTFLAG_BESTMATCH);
}


int RadixNode::for_each(rn_foreach fun, const char *pKey, size_t iKeyLen)
{
    int incr = 0;
    if (getObj() != NULL)
    {
        if (fun(m_pObj, pKey, iKeyLen) != 0)
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
        return count + m_pCHeaders->body->for_each(fun, m_pCHeaders->label,
                                                   m_pCHeaders->len);
    case RNSTATE_CARRAY:
        pHeader = m_pCHeaders;
        for (i = 0; i < m_iNumChildren; ++i)
        {
            count += pHeader->body->for_each(fun, pHeader->label,
                                             pHeader->len);
            pHeader = (rnheader_t *)(&pHeader->label[0]
                                    + rnh_roundup(pHeader->len + 1));
        }
        break;
    case RNSTATE_PARRAY:
        for (i = 0; i < m_iNumChildren; ++i)
        {
            pHeader = m_pPHeaders[i];
            count += pHeader->body->for_each(fun, pHeader->label,
                                             pHeader->len);
        }
        break;
    case RNSTATE_HASH:
        iter = m_pHash->begin();
        while (iter != m_pHash->end())
        {
            pHeader = (rnheader_t *)iter->second();
            count += pHeader->body->for_each(fun, pHeader->label,
                                             pHeader->len);
            iter = m_pHash->next(iter);
        }
        break;
    default:
        break;
    }
    return count;
}


int RadixNode::for_each2(rn_foreach2 fun, void *pUData, const char *pKey,
                         size_t iKeyLen)
{
    int incr = 0;
    if (getObj() != NULL)
    {
        if (fun(m_pObj, pUData, pKey, iKeyLen) != 0)
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
        return count + m_pCHeaders->body->for_each2(fun, pUData,
                                                    m_pCHeaders->label,
                                                    m_pCHeaders->len);
    case RNSTATE_CARRAY:
        pHeader = m_pCHeaders;
        for (i = 0; i < m_iNumChildren; ++i)
        {
            count += pHeader->body->for_each2(fun, pUData, pHeader->label,
                                              pHeader->len);
            pHeader = (rnheader_t *)(&pHeader->label[0]
                                    + rnh_roundup(pHeader->len + 1));
        }
        break;
    case RNSTATE_PARRAY:
        for (i = 0; i < m_iNumChildren; ++i)
        {
            pHeader = m_pPHeaders[i];
            count += pHeader->body->for_each2(fun, pUData, pHeader->label,
                                              pHeader->len);
        }
        break;
    case RNSTATE_HASH:
        iter = m_pHash->begin();
        while (iter != m_pHash->end())
        {
            pHeader = (rnheader_t *)iter->second();
            count += pHeader->body->for_each2(fun, pUData, pHeader->label,
                                              pHeader->len);
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
    void *ptr = rnDoAlloc(0, pool, sizeof(RadixNode));
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
    int iHasChildren = 0;
    size_t iChildLen;
    ls_xpool_t *pool = pParams->pool;
    pMyNode = newNode(pool, pParent, NULL);

    if ((pParams->flags & RTFLAG_NOCONTEXT) == 0)
        iHasChildren = rnNextOffset(pParams->label, pParams->labellen,
                                    iChildLen);
    else
        iChildLen = pParams->labellen;
    pMyChildHeader = (rnheader_t *)rnDoAlloc(pParams->flags, pool,
                                             rnh_size(iChildLen + 1));
    if (pMyChildHeader == NULL)
    {
        rnDoFree(pParams->flags, pool, pMyNode);
        return NULL;
    }

    if ((pDest = pMyNode->fillNode(pParams, pMyChildHeader, iHasChildren,
                                   iChildLen)) == NULL)
    {
        rnDoFree(pParams->flags, pool, pMyChildHeader);
        rnDoFree(pParams->flags, pool, pMyNode);
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
        myParams.flags = pParams->flags;
        myParams.cmp = pParams->cmp;
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
            && (pParams->cmp(pHeader->label, pParams->label,
                             pHeader->len) == 0))
        {
            return checkLevel(myParams, pHeader->body, iHasChildren);
        }

        pHeader = (rnheader_t *)(&pHeader->label[0]
                                 + rnh_roundup(pHeader->len + 1));
    }
    iOffset = (char *)pHeader - (char *)m_pCHeaders;
    if (getNumChildren() < RN_USEHASHCNT)
    {

        pHeader = (rnheader_t *)rnDoRealloc(pParams->flags, pool, m_pCHeaders,
                                    iOffset + rnh_roundedsize(iChildLen + 1));
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
    ptr = rnDoAlloc(pParams->flags, pool, sizeof(GHash));
    if ((pParams->flags & RTFLAG_REGEXCMP) != 0)
        pHash = new (ptr) GHash(RN_HASHSZ, ls_str_hfxx, ls_str_cmp, pool);
    else if ((pParams->flags & RTFLAG_CICMP) != 0)
        pHash = new (ptr) GHash(RN_HASHSZ, ls_str_hfci, ls_str_cmpci, pool);
    else
        pHash = new (ptr) GHash(RN_HASHSZ, ls_str_hfxx, ls_str_cmp, pool);

    pHeader = (rnheader_t *)m_pCHeaders;
    for (i = 0; i < m_iNumChildren; ++i)
    {
        pHeader->body->setLabel(pHeader->label, pHeader->len);
        if (pHash->insert(pHeader->body->getLabel(), pHeader) == NULL)
        {
            pHash->~GHash();
            rnDoFree(pParams->flags, pool, pHash);
            return NULL;
        }

        pHeader = (rnheader_t *)(&pHeader->label[0]
                                 + rnh_roundup(pHeader->len + 1));
    }

    pHeader = (rnheader_t *)rnDoAlloc(pParams->flags, pool,
                                     rnh_size(iChildLen + 1));
    if (pHeader == NULL)
    {
        pHash->~GHash();
        rnDoFree(pParams->flags, pool, pHash);
        return NULL;
    }
    if ((pDest = fillNode(pParams, pHeader, iHasChildren, iChildLen)) == NULL)
    {
        pHash->~GHash();
        rnDoFree(pParams->flags, pool, pHash);
        rnDoFree(pParams->flags, pool, pHeader);
        return NULL;
    }
    pHeader->body->setLabel(pHeader->label, pHeader->len);
    if (pHash->insert(pHeader->body->getLabel(), pHeader) == NULL)
    {
        pHash->~GHash();
        rnDoFree(pParams->flags, pool, pHash);
        rnDoFree(pParams->flags, pool, pHeader);
        return NULL;
    }
    m_pHash = pHash;
    setState(RNSTATE_HASH);
    incrNumChildren();
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
            && (pParams->cmp(pHeader->label, pParams->label,
                             pHeader->len) == 0))
        {
            return checkLevel(myParams, pHeader->body, iHasChildren);
        }
    }
    if (getNumChildren() < RN_USEHASHCNT)
    {
        if (getNumChildren() == RN_USEHASHCNT >> 1)
        {
            pArray = (rnheader_t **)rnDoRealloc(pParams->flags, pool,
                                            m_pPHeaders, sizeof(rnheader_t *)
                                                        * RN_USEHASHCNT);
            if (pArray == NULL)
                return NULL;
            m_pPHeaders = pArray;
        }

        pHeader = (rnheader_t *)rnDoAlloc(pParams->flags, pool,
                                          rnh_size(iChildLen + 1));
        if (pHeader == NULL)
            return NULL;
        if ((pDest = fillNode(pParams, pHeader, iHasChildren,
                              iChildLen)) == NULL)
        {
            rnDoFree(pParams->flags, pool, pHeader);
            return NULL;
        }
        m_pPHeaders[m_iNumChildren++] = pHeader;
        return pDest;
    }
    ptr = rnDoAlloc(pParams->flags, pool, sizeof(GHash));
    if ((pParams->flags & RTFLAG_REGEXCMP) != 0)
        pHash = new (ptr) GHash(RN_HASHSZ, ls_str_hfxx, ls_str_cmp, pool);
    else if ((pParams->flags & RTFLAG_CICMP) != 0)
        pHash = new (ptr) GHash(RN_HASHSZ, ls_str_hfci, ls_str_cmpci, pool);
    else
        pHash = new (ptr) GHash(RN_HASHSZ, ls_str_hfxx, ls_str_cmp, pool);

    for (i = 0; i < m_iNumChildren; ++i)
    {
        pHeader = m_pPHeaders[i];
        pHeader->body->setLabel(pHeader->label, pHeader->len);
        if (pHash->insert(pHeader->body->getLabel(), pHeader) == NULL)
        {
            pHash->~GHash();
            rnDoFree(pParams->flags, pool, pHash);
            return NULL;
        }
    }

    pHeader = (rnheader_t *)rnDoAlloc(pParams->flags, pool, rnh_size(iChildLen + 1));
    if (pHeader == NULL)
    {
        pHash->~GHash();
        rnDoFree(pParams->flags, pool, pHash);
        return NULL;
    }
    if ((pDest = fillNode(pParams, pHeader, iHasChildren, iChildLen)) == NULL)
    {
        pHash->~GHash();
        rnDoFree(pParams->flags, pool, pHash);
        rnDoFree(pParams->flags, pool, pHeader);
        return NULL;
    }
    pHeader->body->setLabel(pHeader->label, pHeader->len);
    if (pHash->insert(pHeader->body->getLabel(), pHeader) == NULL)
    {
        pHash->~GHash();
        rnDoFree(pParams->flags, pool, pHash);
        rnDoFree(pParams->flags, pool, pHeader);
        return NULL;
    }
    rnDoFree(pParams->flags, pool, m_pPHeaders);
    m_pHash = pHash;
    setState(RNSTATE_HASH);
    incrNumChildren();
    return pDest;
}


RadixNode *RadixNode::insertHash(rnparams_t *pParams, rnparams_t *myParams,
                                 int iHasChildren, size_t iChildLen)
{
    GHash::iterator pHashNode;
    ls_str_t hashMatch;
    rnheader_t *pHeader;
    RadixNode *pDest;
    ls_xpool_t *pool = pParams->pool;

    ls_str_set(&hashMatch, (char *)pParams->label, pParams->labellen);
    pHashNode = m_pHash->find(&hashMatch);
    if (pHashNode != NULL)
    {
        pHeader = (rnheader_t *)pHashNode->getData();
        return checkLevel(myParams, pHeader->body, iHasChildren);
    }

    pHeader = (rnheader_t *)rnDoAlloc(pParams->flags, pool,
                                      rnh_size(iChildLen));
    if (pHeader == NULL)
        return NULL;
    if ((pDest = fillNode(pParams, pHeader, iHasChildren, iChildLen)) == NULL)
    {
        rnDoFree(pParams->flags, pool, pHeader);
        return NULL;
    }
    pHeader->body->setLabel(pHeader->label, pHeader->len);
    if (m_pHash->insert(pHeader->body->getLabel(), pHeader) == NULL)
    {
        rnDoFree(pParams->flags, pool, pHeader);
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
                                size_t iChildLen, int iHasChildren, int iFlags)
{
    int i;
    rtcmpfn cmp;
    rnheader_t *pHeader = m_pCHeaders;
    if ((iFlags & RTFLAG_REGEXCMP) != 0)
        cmp = strncmp;
    else if ((iFlags & RTFLAG_CICMP) != 0)
        cmp = strncasecmp;
    else
        cmp = strncmp;
    for (i = 0; i < m_iNumChildren; ++i)
    {
        if ((iChildLen == pHeader->len)
            && (cmp(pHeader->label, pLabel, pHeader->len) == 0))
            return findChildData(pLabel + iChildLen + 1,
                                 iLabelLen - iChildLen - 1, pHeader->body,
                                 iHasChildren, iFlags);

        pHeader = (rnheader_t *)(&pHeader->label[0]
                                 + rnh_roundup(pHeader->len + 1));
    }
    if (((iFlags & RTFLAG_BESTMATCH) == 0) || (getObj() == NULL))
        return NULL;
    return this;
}


/**
 * This function compares labels of the search target and current children.
 * If found, it will call findChildData, which may recurse back to findChild.
 */
RadixNode *RadixNode::findChild(const char *pLabel, size_t iLabelLen,
                                int iFlags)
{
    int i, iHasChildren = 0;
    size_t iChildLen, iGCLen;
    GHash::iterator pHashNode;
    ls_str_t hashMatch;
    rnheader_t *pHeader;
    const char *pGC;
    RadixNode *pOut;
    rtcmpfn cmp;
    if ((iFlags & RTFLAG_REGEXCMP) != 0)
        cmp = strncmp;
    else if ((iFlags & RTFLAG_CICMP) != 0)
        cmp = strncasecmp;
    else
        cmp = strncmp;

    if (((iFlags & RTFLAG_BESTMATCH) == 0) || (m_pObj == NULL))
        pOut = NULL;
    else
        pOut = this;

    if ((iFlags & RTFLAG_NOCONTEXT) != 0)
    {
        if (pLabel[iLabelLen - 1] == '/')
            iChildLen = iLabelLen - 1;
        else
            iChildLen = iLabelLen;
    }
    else
        iHasChildren = rnNextOffset(pLabel, iLabelLen, iChildLen);
    pGC = pLabel + iChildLen + 1;
    iGCLen = iLabelLen - iChildLen - 1;
    switch (getState())
    {
    case RNSTATE_CNODE:
    case RNSTATE_PNODE:
        if (iChildLen < m_pCHeaders->len)
            return pOut;
        if (cmp(m_pCHeaders->label, pLabel, iChildLen) != 0)
            return pOut;

        pHeader = m_pCHeaders;
        break;
    case RNSTATE_CARRAY:
        return findArray(pLabel, iLabelLen, iChildLen, iHasChildren, iFlags);
    case RNSTATE_PARRAY:
        for (i = 0; i < m_iNumChildren; ++i)
        {
            pHeader = m_pPHeaders[i];
            if ((iChildLen == pHeader->len)
                && (cmp(pHeader->label, pLabel, pHeader->len) == 0))
                return findChildData(pGC, iGCLen, pHeader->body, iHasChildren,
                                     iFlags);
        }

        return pOut;
    case RNSTATE_HASH:
        ls_str_set(&hashMatch, (char *)pLabel, iChildLen);
        pHashNode = m_pHash->find(&hashMatch);
        if (pHashNode == NULL)
            return pOut;
        pHeader = (rnheader_t *)pHashNode->getData();
        break;
    default:
        return pOut;
    }
    return findChildData(pGC, iGCLen, pHeader->body, iHasChildren, iFlags);
}


/**
 * This function is called when a child matches.  It will check if
 * it needs to recurse deeper, and if it doesn't, returns the pointer to
 * the object.
 */
RadixNode *RadixNode::findChildData(const char *pLabel, size_t iLabelLen,
                                RadixNode *pNode, int iHasChildren, int iFlags)
{
    RadixNode *pOut;
    if (iHasChildren == 0)
        pOut = pNode;
    else if (pNode->hasChildren())
        pOut = pNode->findChild(pLabel, iLabelLen, iFlags);
    else if ((iFlags & RTFLAG_BESTMATCH) == 0)
        return NULL;
    else
        pOut = pNode;

    if (((iFlags & RTFLAG_BESTMATCH) == 0)
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
    ls_str_t *pStr = pHeader->body->getLabel();
    unsigned long offset = (unsigned long)extra;
    unsigned int i;
    for (i = 0; i < offset; ++i)
        printf("|");
    printf("%.*s  ->", pHeader->len, pHeader->label);
    printf("%p, (%.*s)\n", pHeader->body->getObj(),
           ls_str_len(pStr), ls_str_cstr(pStr));
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





