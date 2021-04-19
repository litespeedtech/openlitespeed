/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2021  LiteSpeed Technologies, Inc.                 *
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
#include "contextnode.h"
#include <util/pool.h>

#include <http/contextlist.h>
#include <http/handlertype.h>
#include <http/httpcontext.h>
#include <http/httphandler.h>

#include <string.h>


ContextNode::ContextNode(const char *pchLabel, ContextNode *pParentNode)
    : m_pChildren(NULL)
    , m_pParentNode(pParentNode)
    , m_pContext(NULL)
    , m_pLabel(NULL)
    , m_lHTALastCheck(0)
    , m_iHTAState(HTA_UNKNOWN)
    , m_isDir(0)
    , m_iRelease(1)
{
    m_pLabel = ls_pdupstr(pchLabel);
}


ContextNode::~ContextNode()
{
    if (m_pChildren)
        delete m_pChildren;
    if (m_iRelease && m_pContext)
        delete m_pContext;
    if (m_pLabel)
        ls_pfree(m_pLabel);
}


void ContextNode::setLabel(const char *l)
{
    if (m_pLabel)
        ls_pfree(m_pLabel);
    m_pLabel = ls_pdupstr(l);
}


void ContextNode::setContextUpdateParent(HttpContext *pContext,
        int noRedirect)
{
    if (pContext == m_pContext)
        return;
    HttpContext *pNewParent = pContext;
    HttpContext *pOldParent = m_pContext;
    if (!pNewParent)
        pNewParent = getParentContext();
    if (!pOldParent)
        pOldParent = getParentContext();
    setChildrenParentContext(pOldParent, pNewParent, noRedirect);
    if (pContext)
    {
        if (m_pContext)
            pContext->setParent(m_pContext->getParent());
        else
            pContext->setParent(getParentContext());
    }
    m_pContext = pContext;

}


void ContextNode::setChildrenParentContext(const HttpContext *pOldParent,
        const HttpContext *pNewParent, int noRedirect)
{
    if (!m_pChildren)
        return;
    ChildNodeList::iterator iter = m_pChildren->begin();
    while (iter != m_pChildren->end())
    {
        HttpContext *pContext = iter.second()->getContext();
        if ((pContext) && ((!noRedirect)
                           || (pContext->getHandler()->getType() != HandlerType::HT_REDIRECT)))
        {
            if (pContext->getParent() == pOldParent)
                pContext->setParent(pNewParent);
        }
        else
        {
            iter.second()->setChildrenParentContext(pOldParent, pNewParent,
                                                    noRedirect);
        }
        iter = m_pChildren->next(iter);
    }
}


void ContextNode::contextInherit(const HttpContext *pRootContext)
{
    if (m_pContext)
        m_pContext->inherit(pRootContext);
    if (!m_pChildren)
        return;
    ChildNodeList::iterator iter = m_pChildren->begin();
    while (iter != m_pChildren->end())
    {
        iter.second()->contextInherit(pRootContext);
        iter = m_pChildren->next(iter);
    }
}


HttpContext *ContextNode::getParentContext()
{
    if (m_pParentNode != NULL)
    {
        if (m_pParentNode->getContext())
        {
            if ((!m_pParentNode->getContext()->getHandler()) ||
                (m_pParentNode->getContext()->getHandler()->getType()
                 != HandlerType::HT_REDIRECT))
                return m_pParentNode->getContext();
        }
        return m_pParentNode->getParentContext();
    }
    return NULL;

}


ContextNode *ContextNode::insertChild(const char *pchLabel)
{
    if (!m_pChildren)
    {
        m_pChildren = new ChildNodeList();
        if (!m_pChildren)
            return NULL;
    }
    ContextNode *pNewChild = new ContextNode(pchLabel, this);
    if (pNewChild)
        m_pChildren->insert(pNewChild->getLabel(), pNewChild);
    return pNewChild;
}


void ContextNode::removeChild(ContextNode *pChild)
{
    if (!m_pChildren)
        return;
    ChildNodeList::iterator iter = m_pChildren->find(pChild->getLabel());
    if (iter != m_pChildren->end())
        m_pChildren->erase(iter);
}

