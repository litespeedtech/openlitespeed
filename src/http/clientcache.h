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
#ifndef CLIENTCACHE_H
#define CLIENTCACHE_H


#include <assert.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <lsdef.h>
#include <util/ghash.h>
#include <util/gpointerlist.h>
#include <util/tsingleton.h>
#include <http/ip2geo.h>

class AutoBuf;
class ClientInfo;

class ClientCache : public TSingleton<ClientCache>
{
    friend class TSingleton<ClientCache>;
public:
    typedef THash<ClientInfo *>      Cache;
    typedef Cache::const_iterator    const_iterator;

private:
    typedef TPointerList<ClientInfo> ClientList;

    Cache                   m_v4;
    Cache                   m_v6;
    ClientList              m_toBeRemoved;
    static ClientCache     *s_pClients;
    static Ip2Geo          *s_ip2geo;

    static int appendDirtyList(const void *pKey, void *pData, void *pList);
    void       clean(Cache *pCache);
    int     writeBlockedIP(AutoBuf *pBuf, Cache *pCache);
    void    recycle(ClientInfo *pInfo);

    ClientCache(int initSize);
public:

    ~ClientCache();
    ClientInfo *newClient(const struct sockaddr *pAddr);
    const_iterator find(const struct sockaddr *pAddr) const;
    void add(ClientInfo *pInfo);
    void del(ClientInfo *pInfo);
    ClientInfo *del(const struct sockaddr *pAddr);
    //void clear();
    void clean();
    void dirtyAll();
    void onTimer();
    void onTimer30Secs()
    {   clean();    }
    void resetThrottleLimit();

    static void initObjPool();
    static void clearObjPool();

    ClientInfo *getClientInfo(struct sockaddr *pPeer);

    int generateBlockedIPReport(int fd);

    static void initClientCache(int iInitSize)
    {   s_pClients = new ClientCache(iInitSize);    }
    static ClientCache *getClientCache()
    {   return s_pClients;          }
    static void setIp2Geo(Ip2Geo *ip2geo)
    {   s_ip2geo = ip2geo;          }
    static Ip2Geo *getIp2Geo()
    {   return s_ip2geo;            }
    LS_NO_COPY_ASSIGN(ClientCache);
};

LS_SINGLETON_DECL(ClientCache);
#endif
