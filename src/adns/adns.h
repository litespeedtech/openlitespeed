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

#ifndef ADNS_H
#define ADNS_H

#include <lsdef.h>
#include <lsr/ls_evtcb.h>

#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>


/**
  *@author Gang Wang
  */
struct sockaddr;
class LsShmHash;
class Adns;

#include <edio/eventreactor.h>
#include <lsr/ls_atomic.h>
#include <lsr/ls_lock.h>
#include <socket/gsockaddr.h>
#include <util/tsingleton.h>

typedef void (* addrLookupCbV4)(struct dns_ctx *, struct dns_rr_a4 *, void *);
typedef void (* addrLookupCbV6)(struct dns_ctx *, struct dns_rr_a6 *, void *);
typedef int (* lookup_pf)(void *arg, long lParam, void *pParam);
typedef void (* unknownCb)();

class AdnsReq
{
    friend class Adns;
public:
    AdnsReq()
       : cb(NULL)
       , name(NULL)
       , arg(NULL)
       , start_time(0)
       , ref_count(1)
       , type(0)
       {}


    void **getArgAddr()                     {   return &arg;        }
    time_t getStartTime()                   {   return start_time;  }

    void setCallback(lookup_pf callback)    {   cb = callback;      }
    unsigned short getRefCount() const      {   return ref_count;   }
    void incRefCount()                      {   ++ref_count;        }
    const char *getName() const             {   return name;        }

private:

    ~AdnsReq()
    {
        if (name)
            free(name);
    }

    lookup_pf        cb;
    char            *name;
    void            *arg;
    long             start_time;
    unsigned short   ref_count;
    unsigned short   type;//PF_INET, PF_INET6
};


class AdnsFetch
{
public:
    AdnsFetch();
    ~AdnsFetch();

    bool isInProgress() const   {   return m_pAdnsReq != NULL;  }
    int tryResolve(const char *name, char *name_only,
                   int family, int flag);
    int asyncLookup(int family, const char *name, lookup_pf callback, void *ctx);

    int setResolvedIp(const void *pIP, int ip_len);
    const GSockAddr &getResolvedAddr() const {  return m_resolvedAddr;  }
    void setResolvedAddr(const sockaddr *addr);

    void cancel();
    void release();

private:
    AdnsReq  *m_pAdnsReq;
    GSockAddr m_resolvedAddr;
};


class Adns : public EventReactor, public evtcbhead_s, public TSingleton<Adns>
{
    friend class TSingleton<Adns>;

public:

    int  init();
    int  shutdown();
    int  initShm(int uid, int gid);


    static int  deleteCache();
    void        trimCache();

    char *getCacheName(const char *pName, int type);
    const char *getCacheValue(const char * pName, int nameLen, int &valLen,
                               int max_ttl = 0);

    //Sync mode, return the ip
    const char *getHostByNameInCache(const char * pName, int &length,
                                     int type, int max_ttl = 0);
    //Async mode, when done will call the cb
    AdnsReq *getHostByName(const char * pName, int type, lookup_pf cb, void *arg);

    const char *getHostByAddrInCache(const struct sockaddr * pAddr,
                                     int &length, int max_ttl = 0);
    AdnsReq *getHostByAddr(const struct sockaddr * pAddr, void *arg, lookup_pf cb);

    static int setResult(const struct sockaddr *result, const void *ip, int len);
    static void release(AdnsReq *pReq);


    int  handleEvents(short events);
    int  onTimer();
    void setTimeOut(int tmSec);

    static int getHostByNameSync(const char *pName, in_addr_t *addr);
    static int getHostByNameV6Sync(const char *pName, in6_addr *addr);

    void       checkCacheTTL();

    void       processPendingEvt()
    {}

private:
    Adns();
    ~Adns();

    static void getHostByNameCb(struct dns_ctx *ctx, void *rr_unknown, void *param);
    static void getHostByAddrCb(struct dns_ctx *ctx, struct dns_rr_ptr *rr, void *param);
    static void printLookupError(struct dns_ctx *ctx, AdnsReq *pAdnsReq);

    void checkDnsEvents();
    static int checkDnsEventsCb(evtcbhead_t *, const long , void *);
    void wantCheckDnsEvents();
    LsShmHash  *getShmHash()    {   return m_pShmHash;  }


    struct dns_ctx     *m_pCtx;
    LsShmHash          *m_pShmHash;
    pthread_t           m_lockedBy;
    time_t              m_tmLastTrim;
    ls_mutex_t          m_mutex;
    ls_mutex_t          m_udns_mutex;
    short               m_iCounter;
    short               m_iPendingEvt;

    LS_NO_COPY_ASSIGN(Adns);
};

LS_SINGLETON_DECL(Adns);

#endif

