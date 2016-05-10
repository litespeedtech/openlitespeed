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
#include "adns.h"
#ifdef  USE_UDNS

#include <http/clientinfo.h>
#include <log4cxx/logger.h>

#include <udns.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <lsr/ls_strtool.h>

int Adns::s_inited = 0;


Adns::Adns()
    : m_pCtx(NULL)
{
}


Adns::~Adns()
{
    shutdown();
}


int Adns::init()
{
    if (!s_inited)
    {
        if (dns_init(1) < 0)
            return LS_FAIL;
        s_inited = 1;
    }
//    if ( m_pCtx )
//        return 0;
//    m_pCtx = dns_new( NULL );
//    if ( !m_pCtx )
//        return LS_FAIL;
//    if ( dns_open( m_pCtx ) < 0 )
//        return LS_FAIL;
    setfd(dns_sock(m_pCtx));
    return 0;
}


int Adns::shutdown()
{
    dns_close(m_pCtx);
//    if ( m_pCtx )
//    {
//        dns_free( m_pCtx );
//        m_pCtx = NULL;
//    }
    return 0;
}


static void addrlookup_callback(struct dns_ctx *ctx, struct dns_rr_ptr *rr,
                                void *data)
{
    ClientInfo *pInfo = (ClientInfo *)data;
    if (rr)
    {
        //Just use the first result for now
        pInfo->setHostName(rr->dnsptr_ptr[0]);
        if (LS_LOG_ENABLED(LOG4CXX_NS::Level::DBG_MORE))
        {
            char achBuf[4096];
            int n = 0;
            char *p;
            int i;

            if (rr->dnsptr_nrr == 1)
                p = rr->dnsptr_ptr[0];
            else
            {
                p = achBuf;
                for (i = 0; i < rr->dnsptr_nrr; ++i)
                    n += ls_snprintf(achBuf, 4096 - n, "%s,", rr->dnsptr_ptr[i]);
                achBuf[--n] = 0;
            }

            LS_DBG_H("DNS reverse lookup: [%s]: %s",
                     pInfo->getAddrString(), p);
        }
        free(rr);
    }
    else
    {
        LS_DBG_H("Failed to lookup [%s]: %s\n", pInfo->getAddrString(),
                 dns_strerror(dns_status(ctx)));
        pInfo->setHostName(NULL);
    }
}


static void namelookupA4_callback(struct dns_ctx *ctx,
                                  struct dns_rr_a4 *rr, void *data)
{
    //TODO: to be done
    free(rr);
    return;
}


static void namelookupA6_callback(struct dns_ctx *ctx,
                                  struct dns_rr_a6 *rr, void *data)
{
    //TODO: to be done
    free(rr);
    return;
}


void Adns::getHostByName(const char *pName, void *arg)
{
    dns_submit_a4(m_pCtx, pName, DNS_NOSRCH,
                  namelookupA4_callback, arg);

}


void Adns::getHostByNameV6(const char *pName, void *arg)
{
    dns_submit_a6(m_pCtx, pName, DNS_NOSRCH,
                  namelookupA6_callback, arg);

}


void Adns::getHostByAddr(ClientInfo *pInfo, const struct sockaddr *pAddr)
{
    switch (pAddr->sa_family)
    {
    case AF_INET:
        dns_submit_a4ptr(m_pCtx, &(((sockaddr_in *)pAddr)->sin_addr),
                         addrlookup_callback, pInfo);
        return;
    case AF_INET6:
        dns_submit_a6ptr(m_pCtx, &(((sockaddr_in6 *)pAddr)->sin6_addr),
                         addrlookup_callback, pInfo);
    }
}


int Adns::handleEvents(short events)
{
    time_t now;
    if (events & POLLIN)
    {
        now = time(NULL);
        dns_ioevent(m_pCtx, now);
    }
    return 0;
}


void Adns::onTimer()
{
    dns_timeouts(m_pCtx, -1, time(NULL));
}


#endif


#ifdef  USE_CARES

#include "adns.h"
#include <http/clientinfo.h>
#include <http/httplog.h>

extern "C"
{
#include <ares.h>
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

Adns::Adns()
{
}


Adns::~Adns()
{
}


static  ares_channel    s_channel = 0;


int Adns::init()
{
    if (s_channel)
        return 0;
    struct ares_options opt;
    int mask = ARES_OPT_FLAGS;
    opt.flags = ARES_FLAG_STAYOPEN | ARES_FLAG_USEVC;
    int ret = ares_init_options(&s_channel, &opt, mask);
    if (ret != ARES_SUCCESS)
    {
        LS_ERROR("ares_init: %s\n", ares_strerror(ret)));
        return LS_FAIL;
    }
    return 0;
}


int Adns::shutdown()
{
    if (s_channel)
    {
        ares_destroy(s_channel);
        s_channel = 0;
    }
    return 0;
}


static void addrlookup_callback(void *arg, int status,
                                struct hostent *host)
{
    ClientInfo *pInfo = (ClientInfo *)arg;
    if (!pInfo)
        return;
    switch (status)
    {
    case ARES_SUCCESS:
        pInfo->setHostName(host->h_name);
        break;
    case ARES_ENOTFOUND:
        pInfo->setHostName(NULL);
        break;
    case ARES_ENOMEM:
    case ARES_EDESTRUCTION:
        LS_NOTICE("ADNS: while resolving %s: %s\n", pInfo->getAddrString(),
                  ares_strerror(status)));
        break;
    }
}


static void namelookup_callback(void *arg, int status,
                                struct hostent *hostent)
{
    return;
}


void Adns::getHostByName(const char *pName, void *arg)
{
    ares_gethostbyname(s_channel, pName, AF_INET, namelookup_callback, arg);
}


void Adns::getHostByAddr(ClientInfo *pInfo, const struct sockaddr *pAddr)
{
    switch (pAddr->sa_family)
    {
    case AF_INET:
        ares_gethostbyaddr(s_channel, &(((sockaddr_in *)pAddr)->sin_addr.s_addr),
                           sizeof(in_addr), pAddr->sa_family, addrlookup_callback, pInfo);
        return;
    case AF_INET6:
        ares_gethostbyaddr(s_channel, &(((sockaddr_in6 *)pAddr)->sin6_addr),
                           sizeof(in6_addr), pAddr->sa_family, addrlookup_callback, pInfo);
    }
}


void Adns::process()
{
    struct timeval timeout;
    fd_set read_fds, write_fds;
    int nfds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    nfds = ares_fds(s_channel, &read_fds, &write_fds);
    if (nfds == 0)
        return;
    memset(&timeout, 0, sizeof(struct timeval));
    //tvp = ares_timeout(channel, NULL, &tv);
    if (select(nfds, &read_fds, &write_fds, NULL, &timeout) > 0)
        ares_process(s_channel, &read_fds, &write_fds);
}

#endif





