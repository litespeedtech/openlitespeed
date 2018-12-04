/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
#ifndef RAILSAPPCONFIG_H
#define RAILSAPPCONFIG_H

#include <lsdef.h>
#include <sys/types.h>
#include <stddef.h>
#include <util/autostr.h>

class ConfigCtx;
class LocalWorkerConfig;
class LocalWorker;
class Env;
class HttpVHost;
class RLimits;
class XmlNode;



class AppConfig
{
public:
    AutoStr      s_binPath;

private:
    int          s_iAppEnv;
    LocalWorkerConfig *s_pAppDefault;

    AppConfig()
    {
        s_iAppEnv = 1;
        s_pAppDefault = NULL;
    }

    ~AppConfig() {}
public:
    int getAppEnv()  { return s_iAppEnv;}
    const LocalWorkerConfig *getpAppDefault()  { return s_pAppDefault; }
    int loadAppDefault(const XmlNode *pNode);

    static AppConfig s_rubyAppConfig;
    static AppConfig s_wsgiAppConfig;
    static AppConfig s_nodeAppConfig;

    LS_NO_COPY_ASSIGN(AppConfig);
};

#endif // RAILSAPPCONFIG_H
