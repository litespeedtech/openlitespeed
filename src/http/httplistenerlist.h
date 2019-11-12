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
#ifndef HTTPLISTENERLIST_H
#define HTTPLISTENERLIST_H


#include <util/hashstringmap.h>
#include <util/gpointerlist.h>
#include <util/tsingleton.h>

class HttpListener;
class HttpVHost;
class AutoBuf;
//class SocketListener;
class HttpVHost;
class GSockAddr;
class VHostMap;

class HttpListenerList : public TPointerList< HttpListener >
{
    HttpListenerList(const HttpListenerList &rhs);
    void operator=(const HttpListenerList &rhs);
public:
    HttpListenerList();
    ~HttpListenerList();
    int add(HttpListener *pListener);
    int remove(HttpListener *pListener);
    //HttpListener* get( const char * pName ) const;
    HttpListener *get(const char *pName, const char *pAddr);
    void stopAll();
    void suspendAll();
    void suspendSSL();
    void resumeAll();
    void resumeSSL();
    void resumeAllButSSL();
    void endConfig();
    void clear();
    void moveNonExist(HttpListenerList &rhs);
    void removeVHostMappings(HttpVHost *pVHost);
    int  writeStatusReport(int fd);
    int  writeRTReport(int fd);
    void releaseUnused();
    int  saveInUseListnersTo(HttpListenerList &rhs);
    void passListeners();
    void recvListeners();
};



class ServerAddrInfo
{
public:
    ServerAddrInfo()
        :m_pVHostMap(NULL)
        {}
    ~ServerAddrInfo()       {}

    int init(const struct sockaddr *pAddr);
    const struct sockaddr *getAddr() const
    {   return (const struct sockaddr *)m_serverAddr;   }
    const AutoStr2 *getAddrStr() const      {   return &m_serverAddrStr;    }
    const VHostMap *getVHostMap() const     {   return m_pVHostMap;         }

    void setVHostMap(const VHostMap *pMap)  {   m_pVHostMap = pMap;     }

    static hash_key_t hasher(const void *pAddr);
    static int value_cmp(const void *v1, const void *v2);

private:
    char                m_serverAddr[28];
    AutoStr2            m_serverAddrStr;
    //const HttpListener *m_pListener;
    const VHostMap     *m_pVHostMap;
};


class ServerAddrRegistry : public TSingleton<ServerAddrRegistry>
{
    friend class TSingleton<ServerAddrRegistry>;
public:

    void init(HttpListenerList *pList);

    const ServerAddrInfo *get(const struct sockaddr *pAddr,
                              const void *pProcessor);

private:
    ServerAddrRegistry();
    ~ServerAddrRegistry()   {   m_registry.release_objects();  }

    THash<ServerAddrInfo*>  m_registry;
    HttpListenerList       *m_pListenerList;

};

LS_SINGLETON_DECL(ServerAddrRegistry);

#endif
