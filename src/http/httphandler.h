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
#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H


class HttpHandler
{
    int                 m_iType;
public: 
    HttpHandler();
    HttpHandler( const HttpHandler & rhs );
    virtual ~HttpHandler();

    int getHandlerType() const          {   return m_iType;     }
    void setHandlerType( int type )     {   m_iType = type;     }
    virtual const char * getName() const = 0;
    
};

#endif
