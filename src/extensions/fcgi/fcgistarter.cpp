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
#include "fcgistarter.h"
#include "socket/coresocket.h"
#include "fcgidef.h"
#include "fcgiapp.h"
#include "fcgiappconfig.h"
#include <extensions/registry/extappregistry.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <socket/gsockaddr.h>

#include <main/serverinfo.h>

#include <util/ni_fio.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>


FcgiStarter::FcgiStarter()
{
}
FcgiStarter::~FcgiStarter()
{
}


int FcgiStarter::start(FcgiApp &app)
{
    int fd = app.getfd();
    FcgiAppConfig &config = app.getConfig();
    struct stat st;
//    if (( stat( config.getCommand(), &st ) == -1 )||
//        ( access(config.getCommand(), X_OK) == -1 ))
//    {
//        LOG_ERR(("Start FCGI [%s]: invalid path to executable - %s,"
//                 " not exist or not executable ",
//                config.getName(),config.getCommand() ));
//        return -1;
//    }
//    if ( st.st_mode & S_ISUID )
//    {
//        if ( D_ENABLED( DL_LESS ))
//            LOG_D(( "Fast CGI [%s]: Setuid bit is not allowed : %s\n",
//                config.getName(), config.getCommand() ));
//        return -1;
//    }
    if (app.getfd() < 0)
    {
        fd = ExtWorker::startServerSock(&config, config.getBackLog());
        if (fd != -1)
        {
            app.setfd(fd);
            if (config.getServerAddr().family() == PF_UNIX)
            {
                nio_stat(config.getServerAddr().getUnix(), &st);
                HttpGlobals::getServerInfo()->addUnixSocket(
                    config.getServerAddr().getUnix(), &st);
            }
        }
        else
            return -1;
    }
    int instances = config.getInstances();
    int cur_instances = app.getCurInstances();
    int new_instances = app.getConnPool().getTotalConns() + 2 - cur_instances;
    if (new_instances <= 0)
        new_instances = 1;
    if (instances < new_instances + cur_instances)
        new_instances = instances - cur_instances;
    if (new_instances <= 0)
        return 0;
    int i;
    for (i = 0; i < new_instances; ++i)
    {
        int pid;
        pid = LocalWorker::workerExec(config, fd);
        if (pid > 0)
        {
            if (D_ENABLED(DL_LESS))
                LOG_D(("[%s] add child process pid: %d", app.getName(), pid));
            PidRegistry::add(pid, &app, 0);
        }
        else
        {
            LOG_ERR(("Start FCGI [%s]: failed to start the %d of %d instances.",
                     config.getName(), i + 1, instances));
            break;
        }
    }
    return (i == 0) ? -1 : 0;
}


