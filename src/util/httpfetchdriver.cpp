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
#include "httpfetchdriver.h"
#include "httpfetch.h"
#include <http/httpglobals.h>
#include <edio/multiplexer.h>


HttpFetchDriver::HttpFetchDriver(HttpFetch * pHttpFetch)
{
    m_pHttpFetch = pHttpFetch; 
    m_tmStart = time(NULL); 
}

int HttpFetchDriver::handleEvents(short int event)
{
    return m_pHttpFetch->processEvents( event );
}

void HttpFetchDriver::onTimer()
{
    if ( m_pHttpFetch->getTimeout() < 0 )
        return ;
    
    if ( time(NULL) - m_tmStart >= m_pHttpFetch->getTimeout() )
    {
        m_pHttpFetch->setTimeout( -1 );
        m_pHttpFetch->closeConnection();
    }
}

void HttpFetchDriver::start()
{
    setPollfd();
    Multiplexer *pMpl =  HttpGlobals::getMultiplexer();
    if (pMpl)
       pMpl->add( this,  POLLIN|POLLOUT|POLLHUP|POLLERR );
}

void HttpFetchDriver::stop()
{
    Multiplexer *pMpl =  HttpGlobals::getMultiplexer();
    if (pMpl)
        pMpl->remove( this );
}
