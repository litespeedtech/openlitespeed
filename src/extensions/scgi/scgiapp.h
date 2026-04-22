/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2025  LiteSpeed Technologies, Inc.                 *
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
#ifndef SCGIAPP_H
#define SCGIAPP_H


#include <lsdef.h>
#include <extensions/localworker.h>
class ScgiAppConfig;

class ScgiApp : public LocalWorker
{
    int             m_iMaxConns;
    int             m_iMaxReqs;

    ExtConn        *newConn();

public:
    explicit ScgiApp(const char *pName);
    ~ScgiApp();

    int startEx();

    ScgiAppConfig &getConfig() const
    {   return *(ScgiAppConfig *)getConfigPointer();    }

    void setScgiMaxConns(int max)     {   m_iMaxConns = max;          }
    void setScgiMaxReqs(int max)      {   m_iMaxReqs = max;           }

    virtual int setURL(const char *pURL);

    LS_NO_COPY_ASSIGN(ScgiApp);
};

#endif
