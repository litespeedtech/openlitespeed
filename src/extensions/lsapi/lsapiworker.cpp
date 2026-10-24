/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
#include "lsapiworker.h"
#include "lsapiconfig.h"
#include "lsapiconn.h"
#include <http/handlertype.h>
#include <http/httpserverconfig.h>
#include <main/configctx.h>
#include <main/mainserverconfig.h>
#include <stdio.h>
#include <stdlib.h>


LsapiWorker::LsapiWorker(const char *pName)
    : LocalWorker(HandlerType::HT_LSAPI)
{
    setConfigPointer(new LsapiConfig(pName));
}


LsapiWorker::~LsapiWorker()
{
}

ExtConn *LsapiWorker::newConn()
{
    return new LsapiConn();
}


//An idle connection holds an lsphp child another httpd worker may need
int LsapiWorker::keepIdleConn() const
{
    if (!getConfig().getSelfManaged()
        || (HttpServerConfig::getInstance().getChildren() <= 1))
        return 1;
    //Without LSAPI_AVOID_FORK, lsphp frees idle children itself and forks one
    //per new connection, so closing would cost more than it saves.
    const char *pAvoidFork = getConfig().getEnv()->find("LSAPI_AVOID_FORK");
    return !(pAvoidFork && (atoi(pAvoidFork) > 0));
}


int LsapiWorker::startEx()
{
    int ret = 1;
    LsapiConfig &config = getConfig();
    if (config.getSelfManaged() && (config.getURL()) && (config.getCommand()))
        ret = startWorker();
    return ret;
}
