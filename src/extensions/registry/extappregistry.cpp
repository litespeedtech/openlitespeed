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
#include "extappregistry.h"
#include <extensions/extworker.h>
#include <http/handlertype.h>
#include <http/httpglobals.h>

#include <util/hashstringmap.h>

static ExtWorker * newWorker( int type, const char * pName );

class ExtAppMap : public HashStringMap< ExtWorker* >
{
public:
    ~ExtAppMap()
    {   removeAll();    }
    void removeAll();
};


void ExtAppMap::removeAll()
{
    release_objects();
}

ExtAppSubRegistry::ExtAppSubRegistry()
    : m_pRegistry( new ExtAppMap() )
    , m_pOldWorkers( new ExtAppMap() )
{
}

ExtAppSubRegistry::~ExtAppSubRegistry()
{
    if ( m_pRegistry )
        delete m_pRegistry;
    if ( m_pOldWorkers )
        delete m_pOldWorkers;
    s_toBeStoped.release_objects();
}

ExtWorker * ExtAppSubRegistry::addWorker( int type, const char * pName )
{
    if ( pName == NULL )
        return NULL;
    ExtAppMap::iterator iter = m_pRegistry->find( pName );
    if ( iter != m_pRegistry->end() )
    {
        return iter.second();
    }
    else
    {
        ExtWorker * pApp = NULL;
        iter = m_pOldWorkers->find( pName );
        if ( iter != m_pOldWorkers->end() )
        {
            pApp = iter.second();
            m_pOldWorkers->remove( pApp->getName() );
        }
        else
            pApp = newWorker( type, pName );
        if ( pApp )
        {
            m_pRegistry->insert( pApp->getName(), pApp );
        }
        return pApp;
    }
}

ExtWorker * ExtAppSubRegistry::getWorker( const char * pName )
{
    if (( pName == NULL )||( strlen( pName ) == 0 ))
        return NULL;
    ExtAppMap::iterator iter = m_pRegistry->find( pName );
    if ( iter == m_pRegistry->end() )
        return NULL;
//    if ( iter.second()->notStarted() )
//    {
//        if ( iter.second()->start() == -1 )
//            return NULL;
//    }
    return iter.second();

}


int ExtAppSubRegistry::stopWorker( ExtWorker * pApp )
{
    if ( pApp )
        return pApp->stop();
    return 0;
}

int ExtAppSubRegistry::stopAllWorkers()
{
    ExtAppMap::iterator iter;
    for( iter = m_pRegistry->begin();
        iter != m_pRegistry->end();
        iter = m_pRegistry->next( iter ) )
    {
        iter.second()->stop();
    }
    return 0;
}


void ExtAppSubRegistry::runOnStartUp()
{
    ExtAppMap::iterator iter;
    for( iter = m_pRegistry->begin();
        iter != m_pRegistry->end();
        iter = m_pRegistry->next( iter ) )
    {
        iter.second()->runOnStartUp();
    }
}


void ExtAppSubRegistry::beginConfig()
{
    assert( m_pOldWorkers->size() == 0 );
    m_pRegistry->swap( *m_pOldWorkers );

}

void ExtAppSubRegistry::endConfig()
{
    ExtAppMap::iterator iter;
    for( iter = m_pOldWorkers->begin();
        iter != m_pOldWorkers->end();
        iter = m_pRegistry->next( iter ) )
    {
        if ( iter.second()->canStop() )
        {
            delete iter.second();
        }
        else
            s_toBeStoped.push_back( iter.second() );
    }
    m_pOldWorkers->clear();
}

void ExtAppSubRegistry::onTimer()
{
    ExtAppMap::iterator iter;
    for( iter = m_pRegistry->begin();
        iter != m_pRegistry->end();
        iter = m_pRegistry->next( iter ) )
    {
        iter.second()->onTimer();
    }
}

void ExtAppSubRegistry::clear()
{
    m_pRegistry->release_objects();
    m_pOldWorkers->release_objects();
}

int ExtAppSubRegistry::generateRTReport( int fd, int type )
{
    static const char * s_pTypeName[] =
    {
        "CGI",
        "FastCGI",
        "Proxy",
        "Servlet",
        "LSAPI",
        "Logger"
    };
        
    ExtAppMap::iterator iter;
    for( iter = m_pRegistry->begin();
        iter != m_pRegistry->end();
        iter = m_pRegistry->next( iter ) )
    {
        iter.second()->generateRTReport( fd, s_pTypeName[ type ] );
    }
    return 0;
}


#include <extensions/cgi/cgidworker.h>
#include <extensions/jk/jworker.h>
#include <extensions/fcgi/fcgiapp.h>
#include <extensions/proxy/proxyworker.h>
#include <extensions/lsapi/lsapiworker.h>
#include <extensions/loadbalancer.h>

static ExtWorker * newWorker( int type, const char * pName )
{
    ExtWorker * pWorker;
    switch( type )
    {
    case EA_CGID:
        pWorker = new CgidWorker( pName );
        break;
    case EA_FCGI:
        pWorker = new FcgiApp( pName );
        break;
    case EA_PROXY:
        pWorker = new ProxyWorker( pName );
        break;
    case EA_JENGINE:
        pWorker = new JWorker( pName );
        break;
    case EA_LSAPI:
        pWorker = new LsapiWorker( pName );
        break;
    case EA_LOGGER:
        pWorker = new FcgiApp( pName );
        break;
    case EA_LOADBALANCER:
        pWorker = new LoadBalancer( pName );
        break;
    default:
        return NULL;
    }
    if ( pWorker )
        pWorker->setHandlerType( type + HandlerType::HT_CGI );
    return pWorker;
}

#include <util/staticobj.h>
static StaticObj< ExtAppSubRegistry > s_registry[EA_NUM_APP];

#include <extensions/pidlist.h>
static StaticObj< PidList > s_pidList;
static PidSimpleList * s_pSimpleList = NULL;


ExtWorker * ExtAppRegistry::addApp( int type, const char * pName )
{
    assert( type >= 0 && type < EA_NUM_APP );
    return s_registry[type]()->addWorker( type, pName );
}

ExtWorker * ExtAppRegistry::getApp( int type, const char * pName )
{
    assert( type >= 0 && type < EA_NUM_APP );
    return s_registry[type]()->getWorker( pName );
}

int ExtAppRegistry::stopApp( ExtWorker * pApp )
{
    if ( pApp )
        return pApp->stop();
    return 0;
}

int ExtAppRegistry::stopAll()
{
    for( int i = 0; i < EA_NUM_APP; ++i )
    {
        s_registry[i]()->stopAllWorkers();
    }
    return 0;
}


void ExtAppRegistry::beginConfig()
{
    for( int i = 0; i < EA_NUM_APP; ++i )
    {
        s_registry[i]()->beginConfig();
    }
}

void ExtAppRegistry::endConfig()
{
    for( int i = 0; i < EA_NUM_APP; ++i )
    {
        s_registry[i]()->endConfig();
    }
}

void ExtAppRegistry::clear()
{
    for( int i = 0; i < EA_NUM_APP; ++i )
    {
        s_registry[i]()->clear();
    }
}

void ExtAppRegistry::onTimer()
{
    for( int i = 0; i < EA_NUM_APP; ++i )
    {
        s_registry[i]()->onTimer();
    }
}

void ExtAppRegistry::init()
{
    for( int i = 0; i < EA_NUM_APP; ++i )
    {
        s_registry[i].construct();
    }
    s_pidList.construct();
}

void ExtAppRegistry::shutdown()
{
    s_pidList.destruct();
    s_pSimpleList = NULL;
    for( int i = 0; i < EA_NUM_APP; ++i )
    {
        s_registry[i]()->clear();
        s_registry[i].destruct();
    }
}

void ExtAppRegistry::runOnStartUp()
{
    for( int i = 0; i < EA_NUM_APP; ++i )
    {
        if ( i != EA_LOGGER )
            s_registry[i]()->runOnStartUp();
    }
    
}


int ExtAppRegistry::generateRTReport( int fd )
{
    for( int i = 0; i < EA_NUM_APP; ++i )
    {
        if ( i != EA_LOGGER )
            s_registry[i]()->generateRTReport( fd, i );
    }
    return 0;
}

#include <unistd.h> 
PidRegistry::PidRegistry()
{
}

PidRegistry::~PidRegistry()
{
}

void PidRegistry::add( pid_t pid, ExtWorker * pApp, long tm )
{
    s_pidList()->add( pid, tm );
    if ( pApp )
        pApp->addPid( pid );

    if ( s_pSimpleList )
        s_pSimpleList->add( pid, getpid(), pApp );
}

ExtWorker * PidRegistry::remove( pid_t pid)
{
    ExtWorker * pWorker = NULL;
    if ( s_pSimpleList )
    {
        pWorker = s_pSimpleList->remove( pid );
        s_pidList()->remove( pid );
    }
    return pWorker;
    
}

    

void PidRegistry::setSimpleList( PidSimpleList * pList )
{
    s_pSimpleList = pList;    
}

void PidRegistry::markToStop( pid_t pid, int kill_type )
{
    if ( s_pSimpleList )
    {
        s_pSimpleList->markToStop( pid, kill_type );
    }
}


