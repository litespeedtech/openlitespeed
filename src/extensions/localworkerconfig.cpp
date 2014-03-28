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
#include "localworkerconfig.h"
#include <http/httpglobals.h>
#include <http/httpserverconfig.h>
#include <util/rlimits.h>
#include "util/configctx.h"
#include <util/xmlnode.h>
#include <util/daemonize.h>
#include <extensions/localworker.h>
#include <extensions/registry/extappregistry.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>

LocalWorkerConfig::LocalWorkerConfig( const char * pName )
    : ExtWorkerConfig( pName )
    , m_pCommand( NULL )
    , m_iBackLog( 10 )
    , m_iInstances( 1 )
    , m_iPriority( 0 )
    , m_iRunOnStartUp( 0 )
{
}

LocalWorkerConfig::LocalWorkerConfig()
    : m_pCommand( NULL )
    , m_iBackLog( 10 )
    , m_iInstances( 1 )
    , m_iPriority( 0 )
    , m_iRunOnStartUp( 0 )
{
}

LocalWorkerConfig::LocalWorkerConfig( const LocalWorkerConfig& rhs )
    : ExtWorkerConfig( rhs )
{
    if ( rhs.m_pCommand )
        m_pCommand = strdup( rhs.m_pCommand );
    else
        m_pCommand = NULL;
    m_iBackLog = rhs.m_iBackLog;
    m_iInstances = rhs.m_iInstances;
    m_iPriority = rhs.m_iPriority;
    m_rlimits = rhs.m_rlimits;
    m_iRunOnStartUp = rhs.m_iRunOnStartUp;

}


LocalWorkerConfig::~LocalWorkerConfig()
{
    if ( m_pCommand )
        free( m_pCommand );
}

void LocalWorkerConfig::setAppPath( const char * pPath )
{
    if (( pPath != NULL )&&( strlen( pPath ) > 0 ))
    {
        if ( m_pCommand )
            free( m_pCommand );
        m_pCommand = strdup( pPath );
    }
}

void LocalWorkerConfig::beginConfig()
{
    clearEnv();
}

void LocalWorkerConfig::endConfig()
{
    
}

void LocalWorkerConfig::setRLimits( const RLimits * pRLimits )
{
    if ( !pRLimits )
        return;
    m_rlimits = *pRLimits;
    
}
int LocalWorkerConfig::checkExtAppSelfManagedAndFixEnv( )
{
    static const char *instanceEnv[] =
    {
        "PHP_FCGI_CHILDREN",  "PHP_LSAPI_CHILDREN",
        "LSAPI_CHILDREN"
    };
    int selfManaged = 0;
    size_t i;
    Env *pEnv = getEnv();
    const char *pEnvValue = NULL;

    for( i = 0; i < sizeof( instanceEnv ) / sizeof( char * ); ++i )
    {
        pEnvValue = pEnv->find( instanceEnv[i] );

        if ( pEnvValue != NULL )
            break;
    }


    if ( pEnvValue )
    {
        int children = atol( pEnvValue );
        
        
        if ( ( children > 0 ) && ( children * getInstances() < getMaxConns() ) )
        {
            ConfigCtx::getCurConfigCtx()->log_warn( "Improper configuration: the value of "
                                "%s should not be less than 'Max "
                                "connections', 'Max connections' is reduced to %d."
                                , instanceEnv[i], children * getInstances() );
            setMaxConns( children * getInstances() );
        }

        selfManaged = 1;
    }

    pEnv->add( 0, 0, 0, 0 );
    return selfManaged;    
}
int LocalWorkerConfig::config( const XmlNode *pNode )
{
    int selfManaged;
    int instances;
    int backlog = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "backlog", 1, 100, 10 );
    int priority = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "priority", -20, 20, HttpGlobals::s_priority + 1 );

    if ( priority > 20 )
        priority = 20;

    if ( priority < HttpGlobals::s_priority )
        priority = HttpGlobals::s_priority;

    long l = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "extMaxIdleTime", -1, INT_MAX, INT_MAX );

    if ( l == -1 )
        l = INT_MAX;

    setPriority( priority );
    setBackLog( backlog );
    setMaxIdleTime( l );

    setRunOnStartUp( ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "runOnStartUp", 0, 1, 0 ) );

    RLimits limits;
    limits = *(ExtAppRegistry::getRLimits());
    limits.setCPULimit( RLIM_INFINITY, RLIM_INFINITY );
    LocalWorker::configRlimit( &limits, pNode );
    setRLimits( &limits );
    Env *pEnv = getEnv();

    if ( pEnv->find( "PATH" ) == NULL )
    {
        pEnv->add( "PATH=/bin:/usr/bin" );
    }
    instances = ConfigCtx::getCurConfigCtx()->getLongValue( pNode, "instances", 1, INT_MAX, 1 );

    if ( instances > 2000 )
        instances = 2000;

    if ( instances >
            HttpServerConfig::getInstance().getMaxFcgiInstances() )
    {
        instances = HttpServerConfig::getInstance().getMaxFcgiInstances();
        ConfigCtx::getCurConfigCtx()->log_warn( "<instances> is too large, use default max:%d", instances );
    }
    setInstances( instances );
    selfManaged = checkExtAppSelfManagedAndFixEnv( );
    setSelfManaged( selfManaged );
    if ( ( instances != 1 ) &&
            ( getMaxConns() > instances ) )
    {
        ConfigCtx::getCurConfigCtx()->log_notice( "Possible mis-configuration: 'Instances'=%d, "
                                "'Max connections'=%d, unless one Fast CGI process is "
                                "capable of handling multiple connections, "
                                "you should set 'Instances' greater or equal to "
                                "'Max connections'.", instances, getMaxConns() );
        setMaxConns( instances );
    }
  
    RLimits *pLimits = getRLimits();
#if defined(RLIMIT_NPROC)
    int mini_nproc = ( 3 * getMaxConns() + 50 ) * HttpGlobals::s_children;
    struct rlimit   *pNProc = pLimits->getProcLimit();

    if ( ( ( pNProc->rlim_cur > 0 ) && ( ( int ) pNProc->rlim_cur < mini_nproc ) )
            || ( ( pNProc->rlim_max > 0 ) && ( ( int ) pNProc->rlim_max < mini_nproc ) ) )
    {
        ConfigCtx::getCurConfigCtx()->log_notice( "'Process Limit' probably is too low, "
                                "adjust the limit to: %d.", mini_nproc );
        pLimits->setProcLimit( mini_nproc, mini_nproc );
    }

#endif
  
    return 0;
}
void LocalWorkerConfig::configExtAppUserGroup(  const XmlNode *pNode, int iType )
{

    const char *pUser = pNode->getChildValue( "extUser" );
    const char *pGroup = pNode->getChildValue( "extGroup" );
    gid_t gid = -1;
    struct passwd *pw = Daemonize::configUserGroup( pUser, pGroup, gid );

    if ( pw )
    {
        if ( ( int ) gid == -1 )
            gid = pw->pw_gid;

        if ( ( iType != EA_LOGGER ) && ( ( pw->pw_uid < HttpGlobals::s_uidMin ) ||
                                        ( gid < HttpGlobals::s_gidMin ) ) )
        {
            ConfigCtx::getCurConfigCtx()->log_notice( "ExtApp suExec access denied,"
                                    " UID or GID of VHost document root is smaller "
                                    "than minimum UID, GID configured. " );
        }
        else
        {
            setUGid( pw->pw_uid, gid );
        }
    }
}