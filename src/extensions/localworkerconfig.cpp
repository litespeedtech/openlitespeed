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

#include <util/rlimits.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>



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


