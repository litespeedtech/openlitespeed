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
#ifndef ADNS_H
#define ADNS_H

#ifdef  USE_CARES
struct sockaddr;
class ClientInfo;

class Adns
{

public:

    Adns();
    ~Adns();

    static int init();
    static int shutdown();
    static void getHostByName(const char *pName, void *arg);
    static void getHostByAddr(ClientInfo *pInfo, const struct sockaddr *pAddr);
    static void process();
};

#endif

#ifdef  USE_UDNS
struct sockaddr;
class ClientInfo;

#include <lsdef.h>
#include <edio/eventreactor.h>
#include <util/tsingleton.h>

class Adns : public EventReactor, public TSingleton<Adns>
{
    static int          s_inited;
    struct dns_ctx *m_pCtx;

    Adns();
public:

    ~Adns();

    int  init();
    int  shutdown();
    void getHostByName(const char *pName, void *arg);
    void getHostByAddr(ClientInfo *pInfo, const struct sockaddr *pAddr);
    void getHostByNameV6(const char *pName, void *arg);
    int  handleEvents(short events);
    void onTimer();

    LS_NO_COPY_ASSIGN(Adns);
};

#endif


#endif
