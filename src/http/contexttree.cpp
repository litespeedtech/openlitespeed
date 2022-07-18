/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
#include "contexttree.h"
#include <http/contextnode.h>
#include <http/httpcontext.h>
#include <http/contextlist.h>
#include <util/pool.h>
#include <util/stringlist.h>

#include <errno.h>
#include <string.h>


ContextTree::ContextTree()
    : m_pRootContext(NULL)
    , m_iLocRootPathLen(1)
{
    m_pRootNode = new ContextNode("", NULL);
    m_pLocRootNode = new ContextNode("", NULL);
    m_pLocRootPath = ls_pdupstr("/");
}


ContextTree::~ContextTree()
{
    delete m_pRootNode;
    delete m_pLocRootNode;
    ls_pfree(m_pLocRootPath);
}


void ContextTree::setRootLocation(const char *pLocation)
{
    ls_pfree(m_pLocRootPath);
    m_pLocRootPath = ls_pdupstr(pLocation);
    m_iLocRootPathLen =  strlen(pLocation);
}


const HttpContext *ContextTree::bestMatch(const char *pURI, int len) const
{
    if (*pURI != '/')
        return NULL;
    return m_pRootNode->match(pURI + 1)->getContext();
}


const HttpContext *ContextTree::matchLocation(const char *pLocation,
                                              int len) const
{
    if (strncmp(pLocation, m_pLocRootPath, m_iLocRootPathLen) != 0)
        return NULL;
    return m_pLocRootNode->match(pLocation + m_iLocRootPathLen)->getContext();
}


ContextNode *ContextNode::match(const char *pStart)
{
    char *pEnd = (char *)pStart;
    ContextNode *pCurNode = this;
    ContextNode *pChild;
    ContextNode *pLastMatch = this;
    while (pEnd && *pStart)
    {
        pEnd = (char *)strchr(pStart, '/');
        if (pEnd == NULL)
            pChild = pCurNode->findChild(pStart);
        else
        {
            *pEnd = 0;
            pChild = pCurNode->findChild(pStart);
            *pEnd = '/';
            pStart = pEnd + 1;
        }
        if (!pChild)
            return pLastMatch;
        if (pChild->getContext() != NULL)
            pLastMatch = pChild;
        pCurNode = pChild;

    }
    return pLastMatch;

}


ContextNode *ContextNode::find(const char *pStart)
{
    char *pEnd = (char *)pStart;
    ContextNode *pCurNode = this;
    ContextNode *pChild;
    while (pEnd && *pStart)
    {
        pEnd = (char *)strchr(pStart, '/');
        if (pEnd == NULL)
            pChild = pCurNode->findChild(pStart);
        else
        {
            *pEnd = 0;
            pChild = pCurNode->findChild(pStart);
            *pEnd = '/';
            pStart = pEnd + 1;
        }
        if (!pChild)
            return NULL;
        pCurNode = pChild;

    }
    return pCurNode;

}


HttpContext *ContextTree::removeMatchContext(const char *pURI)
{
    ContextNode *pNode = m_pRootNode->match(pURI + 1);
    HttpContext *pMatched = pNode->getContext();
    if (pMatched && strcmp(pMatched->getURI(), pURI) == 0)
    {
        pNode->setContext(NULL);
        return pMatched;
    }
    return NULL;
}


HttpContext *ContextTree::getContext(const char *pURI) const
{
    HttpContext *pMatched = m_pRootNode->match(pURI + 1)->getContext();
    if (pMatched)
    {
        int keyLen = strlen(pURI);
        if ((((pMatched->getURILen() - 1) >= keyLen) &&
             (*(pMatched->getURI() +
                pMatched->getURILen() - 1) == '/')) ||
            (strcmp(pMatched->getURI(), pURI) == 0))
            return pMatched;
    }
    return NULL;
}


HttpContext *ContextTree::getContext(const char *pURI, int regex) const
{
    if (!regex)
        return getContext(pURI);
    const HttpContext *pContext = getRootContext();
    pContext = pContext->findMatchContext(pURI);
    return (HttpContext *)pContext;
}


void ContextTree::contextInherit()
{
    m_pRootNode->contextInherit(m_pRootContext);
    ((HttpContext *)m_pRootContext)->inherit(m_pRootContext);
}


const char *ContextTree::getPrefix(int &iPrefixLen)
{
    const char *pPrefix;
    if (m_pRootContext)
    {
        pPrefix = getRootContext()->getURI();
        iPrefixLen = getRootContext()->getURILen();
    }
    else
    {
        pPrefix = "/";
        iPrefixLen = 1;
    }
    return pPrefix;
}


int ContextTree::add(HttpContext *pContext)
{
    if (pContext == NULL)
        return EINVAL;
    const char *pURI = pContext->getURI();
    const char *pPrefix;
    int          iPrefixLen;
    pPrefix = getPrefix(iPrefixLen);
    ContextNode *pCurNode = addNode(pPrefix, iPrefixLen,
                                    m_pRootNode, (char *)pURI);
    if (!pCurNode)
        return EINVAL;
    if (pCurNode->getContext() != NULL)
        return EINVAL;
    pCurNode->setContextUpdateParent(pContext, 0);
    if (!pContext->getParent())
        pContext->setParent(m_pRootContext);
    pURI = pContext->getLocation();
    if (pURI)
    {
        pCurNode = addNode(m_pLocRootPath, m_iLocRootPathLen,
                           m_pLocRootNode, (char *)pURI);
        if (pCurNode)
        {
            pCurNode->setContext(pContext);
            pCurNode->setRelease(0);
        }
    }
    return 0;
}


ContextNode *ContextTree::findNode(const char *pPath)
{
    return m_pLocRootNode->find(pPath + m_iLocRootPathLen);
}


ContextNode *ContextTree::addNode(const char *pPrefix, int iPrefixLen,
                                  ContextNode *pCurNode, char *pURI,
                                  long lastCheck)
{
    if (strncmp(pPrefix, pURI, iPrefixLen) != 0)
        return NULL;
    const char *pStart = pURI + iPrefixLen;
    char *pEnd ;
    while (*pStart)
    {
        pEnd = (char *)strchr(pStart, '/');
        if (pEnd)
            *pEnd = 0;
        pCurNode = pCurNode->getChildNode(pStart, lastCheck);
        if (pEnd)
        {
            *pEnd++ = '/';
            pStart = pEnd;
        }
        else
            break;
    }
    return pCurNode;
}


HttpContext *ContextTree::addContextByUri(const char *pBegin, int tmLastCheck)
{
    HttpContext *pContext;
    ContextNode *pNode;
    const char *pPrefix;
    int len;
    pPrefix = getPrefix(len);
    pNode = addNode(pPrefix, len, getRootNode(),
                    (char *)pBegin, tmLastCheck);
    if (!pNode)
        return NULL;
    pContext = pNode->getContext();
    if (!pContext)
    {
        pContext = new HttpContext();
        pNode->setContext(pContext);
    }
    pNode->setLastCheck(tmLastCheck);
    return pContext;
}


