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
#ifndef HTTPLISTENER_H
#define HTTPLISTENER_H

#include <edio/eventreactor.h>
#include <lsiapi/lsiapihooks.h>
#include <lsiapi/modulemanager.h>
#include <util/autostr.h>
#include <log4cxx/logsession.h>

#include <sys/types.h>
#include <socket/reuseport.h>
#include <socket/gsockaddr.h>


class HttpVHost;
class SslContext;
class VHostMap;
class ClientInfo;
struct ConnInfo;
class SubIpMap;
class HttpServerImpl;
class AutoBuf;
class UdpListener;
struct ssl_st;

class HttpListener : public EventReactor, public LogSession
{
    friend class HttpServerImpl;
    static int32_t      m_iSockSendBufSize;
    static int32_t      m_iSockRecvBufSize;

    AutoStr             m_sName;
    VHostMap           *m_pMapVHost;
    SubIpMap           *m_pSubIpMap;
    UdpListener        *m_pUdpListener;
    unsigned long long  m_iBinding;
    char                m_iAdmin;
    char                m_isSSL;
    char                m_iSendZconf;
    int                 m_flag;
    AutoStr            *m_pAdcPortList;

    ModuleConfig        m_moduleConfig;
    IolinkSessionHooks  m_iolinkSessionHooks;

    ReusePortFds        m_reusePortFds;

    GSockAddr           m_sockAddr;

    HttpListener(const HttpListener &rhs);
    void operator=(const HttpListener &rhs);
    int addConnection(struct conn_data *pCur, int *iCount);
    //int addConnection( int fd, const struct sockaddr * pPeer );
    int batchAddConn(struct conn_data *pBegin,
                     struct conn_data *pEnd, int *iCount);
    int checkAccess(struct conn_data *pData);
    int setSockAttr(int fd);
    VHostMap *getSubMap(int fd);


protected:
    virtual const char *buildLogId();

public:
    explicit HttpListener(const char *pName, const char *pAddr);

    HttpListener();

    virtual ~HttpListener();

    void beginConfig();
    void endConfig();

    char isSSL() const                 {   return m_isSSL;     }

    const char *getName() const        {   return m_sName.c_str();     }
    void setName(const char *pName)  {   m_sName = pName;    }

    const char *getAddrStr() const;

    int getPort() const;

    char isAdmin() const               {   return m_iAdmin;    }
    void setAdmin(char admin)         {   m_iAdmin = admin;   }


    void finializeSslCtx();
    int beginServe();

    unsigned long long getBinding() const     {   return m_iBinding;  }
    void setBinding(unsigned long long b)   {   m_iBinding = b;     }

    int assign(int fd, struct sockaddr *pAddr);

    const VHostMap *getVHostMap() const
    {   return m_pMapVHost;     }
    VHostMap *getVHostMap()
    {   return m_pMapVHost;     }

    const VHostMap *findVhostMap(const struct sockaddr * pAddr) const;

    virtual int start();

    virtual int handleEvents(short event);

    //virtual int start( Multiplexer* pMulti );
    virtual int suspend();
    virtual int resume();
    virtual int stop();

    static void setSockSendBufSize(int32_t size)
    {   m_iSockSendBufSize = size;              }
    static void setSockRecvBufSize(int32_t size)
    {   m_iSockRecvBufSize = size;              }

    VHostMap *addIpMap(const char *pIP);
    int addDefaultVHost(HttpVHost *pVHost);

    int writeStatusReport(int fd);
    int writeStatusJsonReport(int fd, int first, int last);
    int mapDomainList(HttpVHost *pVHost, const char *pDomains);

    IolinkSessionHooks  *getSessionHooks() {  return &m_iolinkSessionHooks;    }
    ModuleConfig *getModuleConfig()         { return &m_moduleConfig;   }

    void setSendZConf(char send)            {   m_iSendZconf = send;    }
    char isSendZConf() const                {   return m_iSendZconf;    }
    void setAdcPortList(const char *pList);
    AutoStr *getAdcPortList() const         { return m_pAdcPortList;    }
    int zconfAppendVHostList(AutoBuf *pBuf);

    int addConnection();
    void batchAddConn();

    ssl_st *newSsl(SslContext *pSslContext);
    int setConnInfo(ConnInfo *pInfo, struct conn_data *pCur);

    int enableQuic();

    bool isSameAddr(const struct sockaddr* addr);
    void addReusePortSocket(int fd);
    int  activeReusePort(int seq);
    int closeUnActiveReusePort();
    int adjustReusePortCount(int total);

    int getFdCount(int* maxfd);
    int passFds(int arg1);
    int passFds2(int target_fd);
    bool isReusePort() const    {   return m_reusePortFds.size() > 0;    }

    UdpListener *getQuicListener() { return m_pUdpListener;     }

    void releaseReusePortSocket();
    void enableReusePort();
    void adjustFds(int iNumChildren);
    void addUdpSocket(int fd);
    int bindUdpPort();
    int startReusePortSocket(int count);
    int startReusePortSocket(int start, int total);

};

#endif
