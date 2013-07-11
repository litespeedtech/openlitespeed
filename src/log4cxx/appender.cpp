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
#include <log4cxx/appender.h>
#include <log4cxx/layout.h>
#include <log4cxx/loggingevent.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <util/gfactory.h>
#include <util/ni_fio.h>


BEGIN_LOG4CXX_NS

int Appender::isopen()
{
    return m_fd != -1;
}

Appender* Appender::getAppender( const char * pName )
{
    return getAppender( pName, "appender.ps" );
}

Appender* Appender::getAppender( const char * pName, const char *pType )
{
    return (Appender *)s_pFactory->getObj( pName, pType );
}

int Appender::append( LoggingEvent * pEvent, va_list args )
{
    if ( !pEvent )
        return -1;
    if ( getLayout() )
    {
        if ( getLayout() != pEvent->m_pLayout)
        {
            pEvent->m_pLayout = getLayout();
            getLayout()->format( pEvent, args );
        }
    }
    else
    {
        Layout::defaultFormat( pEvent, args );
    }
    assert( pEvent->m_iRMessageLen < 8192 );
    return append( pEvent->m_pMessageBuf, pEvent->m_iRMessageLen );
}


END_LOG4CXX_NS


