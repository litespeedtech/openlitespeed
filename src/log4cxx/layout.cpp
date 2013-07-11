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
#include "layout.h"
#include "loggingevent.h"
#include <stdio.h>
#include <util/gfactory.h>

BEGIN_LOG4CXX_NS

int Layout::init()
{
    return s_pFactory->registType( new Layout( "layout.plain" ) );
}

Layout* Layout::getLayout( const char * pName, const char * pType )
{
    return (Layout *)s_pFactory->getObj( pName, pType );
}


Duplicable * Layout::dup( const char * pName )
{
    return new Layout( pName );
}

int Layout::format( LoggingEvent * pEvent, va_list args )
{
    return defaultFormat( pEvent, args );
}
int Layout::defaultFormat( LoggingEvent * pEvent, va_list args )
{
    return pEvent->m_iRMessageLen = vsnprintf(
            pEvent->m_pMessageBuf, pEvent->m_bufLen,
            pEvent->m_format, args );
}


END_LOG4CXX_NS

