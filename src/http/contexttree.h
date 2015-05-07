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
#ifndef CONTEXTTREE_H
#define CONTEXTTREE_H

#include <cstddef>


class HttpContext;
class HttpVHost;
class RadixNode;
class RadixTree;

class ContextTree
{
    RadixTree          *m_pURITree;
    RadixTree          *m_pLocTree;
    const HttpContext  *m_pRootContext;

    static int updateChildren(void *pObj, void *pUData, const char *pKey,
                              size_t iKeyLen);
    static int inherit(void *pObj, void *pUData, const char *pKey,
                       size_t iKeyLen);

    HttpContext *getParentContext(RadixNode *pCurNode);
    void updateTreeAfterAdd(RadixNode *pRadixNode, HttpContext *pContext);

    ContextTree(const ContextTree &rhs);
    void operator=(const ContextTree &rhs);
public:
    ContextTree();
    ~ContextTree();

    const HttpContext *getRootContext() const  {   return m_pRootContext;  }
    void setRootContext(const HttpContext *pContext);
    void setRootLocation(const char *pLocation, size_t iLocLen);

    int add(HttpContext *pContext);
    const HttpContext *bestMatch(const char *pURI, size_t iUriLen) const;
    const HttpContext *matchLocation(const char *pLoc, size_t iLocLen) const;
    HttpContext *getContext(const char *pURI, size_t iUriLen) const;
    void contextInherit();
};


#endif
