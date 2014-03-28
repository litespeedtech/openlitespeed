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
#ifndef CGIDWORKER_H
#define CGIDWORKER_H

#include <extensions/extworker.h>
#include <util/autostr.h>
#include <sys/types.h>

  
class CgidConfig;
class CgidWorker : public ExtWorker
{
    int     m_pid;
    int     m_fdCgid;
    int     m_lve;

    int spawnCgid( int fd, char * pData, const char *secret );
    int watchDog( const char * pServerRoot, const char * pChroot,
                        int priority, int switchToLscgid );
    
protected:
    virtual ExtConn * newConn();

public:
    explicit CgidWorker( const char * pname );
    ~CgidWorker();
    CgidConfig& getConfig()
    {   return *((CgidConfig *)getConfigPointer());  }

    int start( const char * pServerRoot, const char * pChroot,
                        uid_t uid, gid_t gid, int priority );
    void setLVE( int lve )  {   m_lve = lve;    }
    int  getLVE() const     {   return m_lve;   }
    void closeFdCgid();

    static int checkRestartCgid( const char * pServerRoot, const char * pChroot,
                        int priority, int switchToLscgid = 0 );
    static int getCgidPid();
    int config( const XmlNode *pNode1 );
};

#endif
