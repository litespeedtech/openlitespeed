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
#include <lsr/ls_radix.h>

#include <lsdef.h>
#include <lsr/ls_pool.h>
#include <lsr/ls_str.h>


struct ls_radix_node_s
{
    ls_str_t prefix;
    ls_radix_node_t *next;
    ls_radix_node_t *children;
    void *obj;
};

#ifdef LS_RADIX_DEBUG
#include <stdio.h>
void ls_radix_printnode(ls_radix_node_t *pNode, int iNumSpaces)
{
    if (pNode == NULL)
        return;

    int counter;
    char hasobject = pNode->obj != NULL ? 'y' : 'o';
    ls_radix_node_t *ptr = pNode->children;
    for (counter = 0; counter < iNumSpaces; ++counter)
        printf("|");
    printf("%.*s [%c]\n", ls_str_len(&pNode->prefix), ls_str_cstr(&pNode->prefix), hasobject);
    ls_radix_printnode(ptr, iNumSpaces + 2);
    ls_radix_printnode(pNode->next, iNumSpaces);
}


void ls_radix_printtree(ls_radix_t *pThis)
{
    if (pThis == NULL || pThis->children == NULL)
    {
        printf("Empty Tree!\n");
        return;
    }
    printf("Printing possibilities:\n");
    ls_radix_printnode(pThis->children, 0);
}
#endif


static int ls_radix_insertnode(ls_radix_node_t *pNode, const char *pStr,
                               int iStrLen, void *pObj);

static int ls_radix_insertchild(ls_radix_node_t **pChildren, const char *pStr,
                                int iStrLen, void *pObj);

static int ls_radix_splitnode(ls_radix_node_t *pNode, int iSplit,
                              const char *pStr, int iStrLen, void *pObj);

static int ls_radix_extendnode(ls_radix_node_t *pNode, int iSplit, void *pObj);

static ls_radix_node_t *ls_radix_findnode(ls_radix_t *pThis,
                                          const char *pStr, int iStrLen);



static ls_radix_node_t *ls_radix_createnode(const char *pStr, int iStrLen,
                                            void *pObj)
{
    ls_radix_node_t *pNode =
            (ls_radix_node_t *)ls_palloc(sizeof(ls_radix_node_t));
    if (pNode == NULL)
        return NULL;
    if (ls_str(&pNode->prefix, pStr, iStrLen) == NULL)
    {
        ls_pfree(pNode);
        return NULL;
    }
    pNode->next = NULL;
    pNode->children = NULL;
    pNode->obj = pObj;
    return pNode;
}


static void ls_radix_freenode(ls_radix_node_t *pNode)
{
    if (pNode == NULL)
        return;
    ls_radix_freenode(pNode->next);
    ls_radix_freenode(pNode->children);
    ls_str_d(&pNode->prefix);
    ls_pfree(pNode);
}


ls_radix_t *ls_radix_new()
{
    ls_radix_t *pThis = (ls_radix_t *)ls_palloc(sizeof(ls_radix_t));
    if (pThis == NULL)
        return NULL;
    ls_radix(pThis);
    return pThis;
}


void ls_radix(ls_radix_t *pThis)
{
    pThis->children = NULL;
}


void ls_radix_d(ls_radix_t *pThis)
{
    ls_radix_freenode(pThis->children);
}


void ls_radix_delete(ls_radix_t *pThis)
{
    ls_radix_d(pThis);
    ls_pfree(pThis);
}


int ls_radix_insert(ls_radix_t *pThis, const char *pStr, int iStrLen,
                    void *pObj)
{
    ls_radix_node_t *pNode;
    if (pThis == NULL)
        return LS_FAIL;
    if (pThis->children == NULL)
    {
        pNode = ls_radix_createnode(pStr, iStrLen, pObj);
        if (pNode == NULL)
            return LS_FAIL;
        pThis->children = pNode;
        return LS_OK;
    }
    return ls_radix_insertchild(&pThis->children, pStr, iStrLen, pObj);
}


void *ls_radix_update(ls_radix_t *pThis, const char *pStr, int iStrLen,
                      void *pObj)
{
    ls_radix_node_t *pNode;
    void *pOldObj;
    if (pObj == NULL)
        return NULL;
    pNode = ls_radix_findnode(pThis, pStr, iStrLen);
    if (pNode == NULL)
        return NULL;
    pOldObj = pNode->obj;
    pNode->obj = pObj;
    return pOldObj;
}


void *ls_radix_find(ls_radix_t *pThis, const char *pStr, int iStrLen)
{
    ls_radix_node_t *pNode = ls_radix_findnode(pThis, pStr, iStrLen);
    if (pNode == NULL)
        return NULL;
    return pNode->obj;
}


int ls_radix_splitnode(ls_radix_node_t *pNode, int iSplit, const char *pStr,
                       int iStrLen, void *pObj)
{
    ls_radix_node_t *pOrigEnd, *pSplit;
    char *pNodeStr = ls_str_buf(&pNode->prefix);
    int iNodeLen = ls_str_len(&pNode->prefix);

    pOrigEnd = ls_radix_createnode(&pNodeStr[iSplit], iNodeLen - iSplit,
                                   pNode->obj);
    if (pOrigEnd == NULL)
        return LS_FAIL;
    pSplit = ls_radix_createnode(pStr, iStrLen, pObj);
    if (pSplit == NULL)
    {
        ls_radix_freenode(pOrigEnd);
        return LS_FAIL;
    }

    pNode->obj = NULL;
    pOrigEnd->children = pNode->children;
    if (*pStr < pNodeStr[iSplit])
    {
        pSplit->next = pOrigEnd;
        pNode->children = pSplit;
    }
    else
    {
        pOrigEnd->next = pSplit;
        pNode->children = pOrigEnd;
    }
    pNodeStr[iSplit] = '\0';
    ls_str_setlen(&pNode->prefix, iSplit);
    return LS_OK;
}


int ls_radix_extendnode(ls_radix_node_t *pNode, int iSplit, void *pObj)
{
    int iOrigLen = ls_str_len(&pNode->prefix);
    char *pOrigStr = ls_str_buf(&pNode->prefix);
    ls_radix_node_t *pEnd = ls_radix_createnode(&pOrigStr[iSplit],
                                                iOrigLen - iSplit,
                                                pNode->obj);
    if (pEnd == NULL)
        return LS_FAIL;
    pNode->children = pEnd;
    pNode->obj = pObj;
    pOrigStr[iSplit] = '\0';
    ls_str_setlen(&pNode->prefix, iSplit);
    return LS_OK;
}


int ls_radix_insertnode(ls_radix_node_t *pNode, const char *pStr, int iStrLen,
                        void *pObj)
{
    int i, iLen, iOrigLen = ls_str_len(&pNode->prefix);
    const char *pOrigStr = ls_str_cstr(&pNode->prefix);
    if (iStrLen < iOrigLen)
        iLen = iStrLen;
    else
        iLen = iOrigLen;

    for (i = 1; i < iLen; ++i)
    {
        if (pOrigStr[i] != pStr[i])
            return ls_radix_splitnode(pNode, i, &pStr[i], iStrLen - i, pObj);
    }
    if (iStrLen < iOrigLen)
        return ls_radix_extendnode(pNode, iStrLen, pObj);
    else if (iStrLen == iOrigLen)
    {
        if (pNode->obj != NULL)
            return LS_FAIL; // Cannot add duplicate.
        pNode->obj = pObj;
        return LS_OK;
    }
    return ls_radix_insertchild(&pNode->children, pStr + i,
                                iStrLen - i, pObj);
}


int ls_radix_insertchild(ls_radix_node_t **pChildren, const char *pStr,
                         int iStrLen, void *pObj)
{
    assert(pChildren != NULL);
    const char *pCurStr;
    ls_radix_node_t *pTmp, *pPrev, *pNode = *pChildren;
    pPrev = pNode;
    while (pNode != NULL)
    {
        pCurStr = ls_str_cstr(&pNode->prefix);
        if (*pCurStr < *pStr)
        {
            pPrev = pNode;
            pNode = pNode->next;
            continue;
        }
        else if (*pCurStr > *pStr)
            break;
        return ls_radix_insertnode(pNode, pStr, iStrLen, pObj);
    }
    pTmp = ls_radix_createnode(pStr, iStrLen, pObj);
    if (pTmp == NULL)
        return LS_FAIL;
    pTmp->next = pNode;
    if (pNode == pPrev)
        *pChildren = pTmp;
    else
        pPrev->next = pTmp;
    return LS_OK;
}


ls_radix_node_t *ls_radix_findchild(ls_radix_node_t *pNode,
                                    const char *pStr, int iStrLen)
{
    int iNodeLen, iCmp;
    const char *pNodeStr;
    if (pNode == NULL)
        return NULL;
    while(pNode != NULL)
    {
        pNodeStr = ls_str_cstr(&pNode->prefix);
        iNodeLen = ls_str_len(&pNode->prefix);
        iCmp = memcmp(pNodeStr, pStr,
                      (iNodeLen < iStrLen ? iNodeLen : iStrLen));
        if (iCmp == 0)
            break;
        else if (iCmp > 0)
            return NULL;

        pNode = pNode->next;
    }
    if (iNodeLen == iStrLen)
        return pNode;
    else if (iNodeLen > iStrLen || pNode->children == NULL)
        return NULL;
    return ls_radix_findchild(pNode->children, &pStr[iNodeLen],
                                iStrLen - iNodeLen);
}


ls_radix_node_t *ls_radix_findnode(ls_radix_t *pThis, const char *pStr,
                                   int iStrLen)
{
    if (pThis == NULL || pThis->children == NULL)
        return NULL;
    return ls_radix_findchild(pThis->children, pStr, iStrLen);
}

