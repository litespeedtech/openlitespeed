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
#include "appconfig.h"
#include "extappregistry.h"

#include <http/serverprocessconfig.h>
#include <log4cxx/logger.h>
#include <main/configctx.h>
#include <main/mainserverconfig.h>
#include <util/xmlnode.h>

#include <extensions/localworker.h>
#include <extensions/localworkerconfig.h>
#include <unistd.h>
#include <limits.h>
#include <config.h>

AppConfig AppConfig::s_rubyAppConfig;
AppConfig AppConfig::s_wsgiAppConfig;
AppConfig AppConfig::s_nodeAppConfig;

int AppConfig::loadAppDefault(const XmlNode *pNode)
{
    s_pAppDefault = new LocalWorkerConfig();

    s_pAppDefault->setPersistConn(1);
    s_pAppDefault->setKeepAliveTimeout(30);
    s_pAppDefault->setMaxConns(1);
    s_pAppDefault->setTimeout(120);
    s_pAppDefault->setRetryTimeout(0);
    s_pAppDefault->setBuffering(0);
    s_pAppDefault->setPriority(
        ServerProcessConfig::getInstance().getPriority() + 1);
    s_pAppDefault->setBackLog(100);
    s_pAppDefault->setMaxIdleTime(300);
    s_pAppDefault->getRLimits()->setDataLimit(RLIM_INFINITY, RLIM_INFINITY);
    s_pAppDefault->getRLimits()->setProcLimit(1400, 1500);
    s_pAppDefault->getRLimits()->setCPULimit(RLIM_INFINITY, RLIM_INFINITY);
    s_binPath = "";

    s_iAppEnv = ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "appserverEnv",
                  0, 2, 1);
    const char *path = NULL;
    if (pNode)
    {
        path = pNode->getChildValue("binPath");
        if (path && *path)
            s_binPath = path;
    }

    ((ExtWorkerConfig *)s_pAppDefault)->config(pNode);
    s_pAppDefault->config(pNode);

    return 0;
}
