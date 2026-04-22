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
#include "uwsgiapp.h"
#include "uwsgiappconfig.h"
#include "uwsgiconnection.h"
#include <http/handlertype.h>
#include <log4cxx/logger.h>
#include <lsr/ls_time.h>
#include <util/gpointerlist.h>

#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>


UwsgiApp::UwsgiApp(const char *pName)
    : LocalWorker(HandlerType::HT_UWSGI)
    , m_iMaxConns(10)
    , m_iMaxReqs(10)
{
    LS_DBG("UwsgiApp::UwsgiApp()\n");
    setConfigPointer(new UwsgiAppConfig(pName));
}


UwsgiApp::~UwsgiApp()
{
    LS_DBG("UwsgiApp::~UwsgiApp()\n");
    //stop();
}

int UwsgiApp::startEx()
{
    LS_DBG("UwsgiApp::startEx()\n");
    int ret = 1;
    //if ((getConfig().getURL()) && (getConfig().getCommand()))
    //    NO RELIABLE WAY TO HAVE WORKER GET LISTEN FD CREATED BY startWorker()
    //    ret = startWorker();
    return ret;
}


ExtConn *UwsgiApp::newConn()
{
    LS_DBG("UwsgiApp::newConn()\n");
    return new UwsgiConnection();
}


int UwsgiApp::setURL(const char *pURL)
{
//    return ExtWorker::setURL( pURL );
    LS_DBG("UwsgiApp::setURL(%s)", pURL);
    getConfig().setURL(pURL);
    if (getfd() == -1)
        return getConfig().updateServerAddr(pURL);
    return 0;
}

