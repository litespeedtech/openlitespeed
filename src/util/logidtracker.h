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
#ifndef LOGIDTRACKER_H
#define LOGIDTRACKER_H

#include <util/autostr.h>

class LogIdTracker
{
    static char  s_sLogId[128];
    static int   s_iIdLen;
    
    AutoStr m_sOldId;
public:
    LogIdTracker( const char * pNewId )
    {
        m_sOldId = getLogId();
        setLogId( pNewId );
    }
    LogIdTracker()
    {
        m_sOldId = getLogId();
    }
    ~LogIdTracker()
    {
        setLogId( m_sOldId.c_str() );
    }
    static const char * getLogId()
    {   return s_sLogId;    }
    
    static void setLogId( const char * pId )
    {
        strncpy( s_sLogId, pId, sizeof( s_sLogId ) - 1 );
        s_sLogId[ sizeof( s_sLogId ) - 1 ] = 0;
        s_iIdLen = strlen( s_sLogId ); 
    }
    
    static void appendLogId( const char * pId )
    {
        strncpy( s_sLogId + s_iIdLen, pId, sizeof( s_sLogId ) -1 - s_iIdLen );
        s_sLogId[ sizeof( s_sLogId ) - 1 ] = 0;
        s_iIdLen += strlen( s_sLogId + s_iIdLen );
    }
};


#endif // LOGIDTRACKER_H
