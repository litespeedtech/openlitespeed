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
#include "httpserverversion.h"

const char HttpServerVersion::s_pVersion[] = "LiteSpeed Open";

int        HttpServerVersion::s_iVersionLen = 9; //sizeof( s_pVersion ) - 1;

void HttpServerVersion::hideDetail( int hide )
{
    if ( !hide )
        s_iVersionLen = 9;
    else if ( 2 == hide )
       s_iVersionLen = 0;
    else
       s_iVersionLen = sizeof( s_pVersion ) - 1;
}

