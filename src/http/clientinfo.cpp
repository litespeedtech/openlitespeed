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
#include "clientinfo.h"
#include <http/iptogeo.h>
#include <util/datetime.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <util/pool.h>
#include <util/accessdef.h>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

ClientInfo::ClientInfo()
    : m_pGeoInfo( NULL )
{}


ClientInfo::~ClientInfo()
{
    if ( m_pGeoInfo )
        delete m_pGeoInfo;    
}


void ClientInfo::setAddr( const struct sockaddr * pAddr )
{
    int len, strLen;
    if ( AF_INET == pAddr->sa_family )
    {
        len = 16; strLen = 17;
    }
    else
    {
        len = 24; strLen = 41;
    }
    memmove( m_achSockAddr, pAddr, len );
    m_sAddr.resizeBuf( strLen );
    if ( m_sAddr.buf() )
    {
        inet_ntop( pAddr->sa_family, ((char * )pAddr) + ((len >> 1) - 4),
                m_sAddr.buf(), strLen );
        m_sAddr.setLen( strlen( m_sAddr.c_str()) );
    }
    memset( &m_iConns, 0, (char *)(&m_lastConnect + 1) - (char *)&m_iConns );
    m_iAccess = 1;
}

int ClientInfo::checkAccess()
{
    switch ( m_iAccess )
    {
    case AC_BLOCK:
    case AC_DENY:
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( "[%s] Access is denied!", getAddrString() ));
        return 1;
    case AC_ALLOW:
        if ( getOverLimitTime() )
        {
            if ( DateTime::s_curTime - getOverLimitTime()
                    >= HttpGlobals::s_iOverLimitGracePeriod )
            {
                LOG_NOTICE(( "[%s] is over per client soft connection limit: %d for %d seconds,"
                             " close connection!",
                    getAddrString(), HttpGlobals::s_iConnsPerClientSoftLimit,
                    DateTime::s_curTime - getOverLimitTime() ));
                setOverLimitTime( DateTime::s_curTime );
                setAccess( AC_BLOCK );
                return 1;
            }
            else
            {
                if ( D_ENABLED( DL_LESS ))
                    LOG_D(( "[%s] %d connections established, limit: %d.",
                        getAddrString(), getConns(),
                        HttpGlobals::s_iConnsPerClientSoftLimit ));
            }
        }
        else if ( (int)getConns() >= HttpGlobals::s_iConnsPerClientSoftLimit )
            setOverLimitTime( DateTime::s_curTime );
        if ( (int)getConns() >= HttpGlobals::s_iConnsPerClientHardLimit )
        {
            LOG_NOTICE(( "[%s] Reached per client connection hard limit: %d, close connection!",
                    getAddrString(), HttpGlobals::s_iConnsPerClientHardLimit ));
            setOverLimitTime( DateTime::s_curTime );
            setAccess( AC_BLOCK );
            return 1;
        }
        //fall through
    case AC_TRUST:
    default:
        break;
    }
    return 0;
}
GeoInfo * ClientInfo::allocateGeoInfo()
{
    if ( !m_pGeoInfo )
    {
        m_pGeoInfo = new GeoInfo();
    }
    return m_pGeoInfo;
}
