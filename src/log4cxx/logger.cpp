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
#include "logger.h"

#include <util/gfactory.h>
#include <log4cxx/fileappender.h>
#include "patternlayout.h"
#include "loggingevent.h"

BEGIN_LOG4CXX_NS

static int s_inited = 0;
::GFactory * s_pFactory = NULL;

Logger::Logger( const char * pName)
    : Duplicable( pName )
    , m_iLevel( Level::DEBUG )
    , m_pAppender( NULL )
    , m_iAdditive( 1 )
    , m_pParent( NULL )
{
}


void Logger::init()
{
    if ( !s_inited )
    {
        s_inited = 1;
        s_pFactory = new GFactory();
        if ( s_pFactory )
        {
            FileAppender::init();
            Layout::init();
            PatternLayout::init();
            s_pFactory->registType( new Logger("Logger") );
            s_inited = 1;
        }
    }
}

Logger * Logger::getLogger( const char * pName )
{
    init();
    if ( !*pName )
        pName = ROOT_LOGGER_NAME;
    return (Logger *)s_pFactory->getObj( pName, "Logger" );
}

Duplicable * Logger::dup( const char * pName )
{
    return new Logger( pName );
}


void Logger::vlog( int level, const char * format, va_list args )
{
    char achBuf[8192];
    LoggingEvent event( level, getName(), format, &achBuf[0], (int)8192 );
    gettimeofday( &event.m_timestamp, NULL );
    Logger * pLogger = this;
    while( 1 )
    {
        if ( pLogger->m_pAppender )
        {
            if ( pLogger->m_pAppender->append( &event, args ) == -1 )
                break;
        }
        if ( !pLogger->m_pParent || !pLogger->m_iAdditive )
            break;
        pLogger = m_pParent;
    }
}

void Logger::log( const char * pBuf, int len )
{
    if ( m_pAppender )
        if ( m_pAppender->append( pBuf, len ) == -1 )
            return;
    if ( m_pParent && m_iAdditive )
        m_pParent->log( pBuf, len );
}


END_LOG4CXX_NS

