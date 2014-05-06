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

#include <stdio.h>

BEGIN_LOG4CXX_NS

static int s_inited = 0;
::GFactory * s_pFactory = NULL;

Logger::Logger( const char * pName)
    : Duplicable( pName )
    , m_iLevel( Level::DEBUG )
    , m_pAppender( NULL )
    , m_iAdditive( 1 )
    , m_pLayout( NULL )
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

static int logSanitorize( char * pBuf, int len )
{
    char * pEnd = pBuf + len - 2;
    while( pBuf < pEnd )
    {
        if ( *pBuf < 0x20 )
            *pBuf = '.';
        ++pBuf;
    }
    return len;
}


void Logger::vlog( int level, const char * format, va_list args, int no_linefeed )
{
    char achBuf[8192];
    int messageLen;
    messageLen = vsnprintf( achBuf, sizeof( achBuf ) - 1,  format, args );
    if ( (size_t)messageLen > sizeof( achBuf ) - 1 )
    {
        messageLen = sizeof( achBuf ) - 1;
        achBuf[messageLen] = 0;
    }
    messageLen = logSanitorize( achBuf, messageLen );
    LoggingEvent event( level, getName(), achBuf, messageLen );

    if ( no_linefeed )
        event.m_flag |= LOGEVENT_NO_LINEFEED;
    
    gettimeofday( &event.m_timestamp, NULL );
    Logger * pLogger = this;
    while( 1 )
    {
        if ( !event.m_pLayout )
            event.m_pLayout = pLogger->m_pLayout;
        if ( pLogger->m_pAppender && level <= pLogger->getLevel() )
        {
            if ( pLogger->m_pAppender->append( &event ) == -1 )
                break;
        }
        if ( !pLogger->m_pParent || !pLogger->m_iAdditive )
            break;
        pLogger = m_pParent;
    }
}

void Logger::lograw( const char * pBuf, int len )
{
    if ( m_pAppender )
        if ( m_pAppender->append( pBuf, len ) == -1 )
            return;
    if ( m_pParent && m_iAdditive )
        m_pParent->lograw( pBuf, len );
}


END_LOG4CXX_NS

