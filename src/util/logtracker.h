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
#ifndef LOGTRACKER_H
#define LOGTRACKER_H



#include <log4cxx/ilog.h>
#include <util/autostr.h>

#define MAX_LOGID_LEN   127
  
class LogTracker : public LOG4CXX_NS::ILog
{
    AutoStr2            m_logID;
    LOG4CXX_NS::Logger* m_pLogger;
public: 
    LogTracker();
    ~LogTracker();
    
    AutoStr2& getIDBuf()             {   return m_logID;     }
    const char * getLogId() const    {   return m_logID.c_str();  }
    const char * getLogId()          {   return m_logID.c_str();  }
    
    LOG4CXX_NS::Logger* getLogger() const   {   return m_pLogger;   }
    void setLogger( LOG4CXX_NS::Logger* pLogger)
    {   m_pLogger = pLogger;    }
};

#endif
