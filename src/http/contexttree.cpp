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
#include "contexttree.h"
#include <http/contextnode.h>
#include <http/httpcontext.h>
#include <util/pool.h>

#include <errno.h>
#include <string.h>



ContextTree::ContextTree()
    : m_pRootContext( NULL )
    , m_iLocRootPathLen( 1 )
{
    m_pRootNode = new ContextNode("", NULL);
    m_pLocRootNode = new ContextNode( "", NULL );
    m_pLocRootPath = Pool::dupstr( "/" );
}

ContextTree::~ContextTree()
{
    delete m_pRootNode;
    delete m_pLocRootNode;
    Pool::deallocate2( m_pLocRootPath );
}

void ContextTree::setRootLocation( const char * pLocation )
{
    Pool::deallocate2( m_pLocRootPath );
    m_pLocRootPath = Pool::dupstr( pLocation );
    m_iLocRootPathLen =  strlen( pLocation );
}


const HttpContext* ContextTree::bestMatch( const char * pURI ) const
{
    if ( *pURI != '/' )
        return NULL;
    return m_pRootNode->match( pURI + 1 )->getContext();
}

const HttpContext* ContextTree::matchLocation( const char * pLocation ) const
{
    if ( strncmp( pLocation, m_pLocRootPath, m_iLocRootPathLen ) != 0 )
        return NULL;
    return m_pLocRootNode->match( pLocation + m_iLocRootPathLen )->getContext();
}

ContextNode * ContextNode::match( const char * pStart )
{
    char * pEnd = (char * )pStart;
    ContextNode * pCurNode = this;
    iterator iter ;
    ContextNode *pLastMatch = this;
    while ( pEnd )
    {
        pEnd = (char *) strchr( pStart, '/' );
        if ( pEnd == NULL )
        {
            iter = pCurNode->find(pStart);
        }
        else
        {
            *pEnd = 0;
            iter = pCurNode->find(pStart);
            *pEnd = '/';
            pStart = pEnd + 1;
        }
        if ( iter == pCurNode->end() )
            break;
        if ( iter.second()->getContext() != NULL )
        {
            pLastMatch = iter.second();
        }
        pCurNode = iter.second() ;

    }
    return pLastMatch;

}


HttpContext* ContextTree::getContext( const char * pURI ) const
{
    HttpContext* pMatched = m_pRootNode->match( pURI + 1 )->getContext();
    if ( pMatched )
    {
        if ( strcmp( pMatched->getURI(), pURI ) == 0 )
            return pMatched;
    }
    return NULL;
}

void ContextTree::contextInherit()
{
    m_pRootNode->contextInherit( m_pRootContext );
    ((HttpContext *)m_pRootContext)->inherit( m_pRootContext );
}


const char * ContextTree::getPrefix( int &iPrefixLen )
{
    const char * pPrefix;
    if ( m_pRootContext )
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


int ContextTree::add( HttpContext* pContext )
{
    if ( pContext == NULL )
        return EINVAL;
    const char * pURI = pContext->getURI();
    const char * pPrefix;
    int          iPrefixLen;
    pPrefix = getPrefix( iPrefixLen );
    ContextNode * pCurNode = addNode( pPrefix, iPrefixLen,
                                    m_pRootNode, (char *)pURI );    
    if ( !pCurNode )
        return EINVAL;
    if ( pCurNode->getContext() != NULL )
        return EINVAL;
    pCurNode->setContextUpdateParent(pContext, 0);
    if ( !pContext->getParent() )
    {
        pContext->setParent( m_pRootContext );
    }
    pURI = pContext->getLocation();
    if ( pURI )
    {
        pCurNode = addNode( m_pLocRootPath, m_iLocRootPathLen,
                        m_pLocRootNode, (char *)pURI );
        if ( pCurNode )
        {
            pCurNode->setContext( pContext );
            pCurNode->setRelease( 0 );
        }
    }
    return 0;
}


ContextNode * ContextTree::addNode( const char * pPrefix, int iPrefixLen,
                                ContextNode * pCurNode, char * pURI,
                                long lastCheck )
{
    if ( strncmp( pPrefix, pURI, iPrefixLen ) != 0 )
        return NULL;
    const char * pStart = pURI + iPrefixLen;
    register char * pEnd ;
    while ( *pStart )
    {
        pEnd = (char *) strchr( pStart, '/' );
        if ( pEnd )
        {
            *pEnd = 0;
        }
        pCurNode = pCurNode->getChildNode(pStart, lastCheck);
        if ( pEnd )
        {
            *pEnd++ = '/';
            pStart = pEnd;
        }
        else
            break;
    }
    return pCurNode;
}
/*
void ContextTree::merge( const ContextTree & rhs )
{
    ContextList list;
    rhs.m_pRootNode->getAllContexts( &list );
    ContextList::iter;

    for( iter = list.begin(); iter != list.end(); ++iter )
    {
        const char * pURI = (*iter)->getURI();
        if ( strcmp( pURI, '/' ) == 0 )
        {
            ContextList * pMatch = getMatchList();
            if (( pMatch )&&( m_pRootContext ))
            {
                ContextList::iter1;
                for( iter1 = pMatch->begin(); iter1 != pMatch->end(); ++iter1 )
                {
                    if ( m_pRootContext->findMatchContext(
                                (*iter1)->getURI() ) == NULL )
                    {
                        HttpContext * pContext = (*iter1)->dup();
                        m_pRootContext->addMatchContext( pContext );
                    }
                }
            }
        }
        else
        {
            if ( getContext( pURI ) == NULL )
            {
                HttpContext * pContext = (*iter)->dup();
                addContext( pContext );
            }
        }
    }
    list.clear();
}
*/
