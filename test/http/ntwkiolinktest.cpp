/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2026  LiteSpeed Technologies, Inc.                 *
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
#ifdef RUN_TEST

#include <edio/multiplexer.h>
#include <edio/multiplexerfactory.h>
#include <http/clientcache.h>
#include <http/clientinfo.h>
#include <http/connlimitctrl.h>
#include <http/httpdefs.h>
#include <http/ntwkiolink.h>
#include <http/httpserverconfig.h>
#include <util/accessdef.h>
#include <util/datetime.h>

#include "unittest-cpp/UnitTest++.h"

#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>


static ClientInfo *getProxyTestClient(const char *address)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    CHECK_EQUAL(1, inet_pton(AF_INET, address, &addr.sin_addr));
    return ClientCache::getInstance().getClientInfo((sockaddr *)&addr);
}


static void initProxyTestCache()
{
    static bool initialized = false;
    if (!initialized)
    {
        ClientCache::initObjPool();
        initialized = true;
    }
}


TEST(NtwkIOLinkTest_proxyProtocolChecksAssertedClientAccess)
{
    initProxyTestCache();
    HttpServerConfig &config = HttpServerConfig::getInstance();
    const int oldDnsLookup = config.getDnsLookup();
    config.setDnsLookup(0);
    ClientInfo *proxy = getProxyTestClient("192.0.2.201");
    ClientInfo *asserted = getProxyTestClient("198.51.100.201");
    config.setDnsLookup(oldDnsLookup);
    CHECK(proxy != NULL);
    CHECK(asserted != NULL);
    if (!proxy || !asserted)
        return;

    const int oldProxyAccess = proxy->getAccess();
    const int oldAssertedAccess = asserted->getAccess();
    proxy->setAccess(AC_ALLOW);
    asserted->setAccess(AC_DENY);

    ConnInfo connInfo;
    memset(&connInfo, 0, sizeof(connInfo));
    connInfo.m_pClientInfo = proxy;
    NtwkIOLink link;
    link.setConnInfo(&connInfo);
    proxy->incConn();

    CHECK_EQUAL(LS_FAIL,
                link.UpdateClientInfoByProxyProto(asserted->getAddr(), 12345));
    CHECK_EQUAL(proxy, link.getClientInfo());
    CHECK_EQUAL(1, proxy->getConns());
    CHECK_EQUAL(0, asserted->getConns());

    asserted->setAccess(AC_ALLOW);
    CHECK_EQUAL(0,
                link.UpdateClientInfoByProxyProto(asserted->getAddr(), 12345));
    CHECK_EQUAL(asserted, link.getClientInfo());
    CHECK_EQUAL(0, proxy->getConns());
    CHECK_EQUAL(1, asserted->getConns());

    asserted->decConn();
    proxy->setAccess(oldProxyAccess);
    asserted->setAccess(oldAssertedAccess);
}


TEST(NtwkIOLinkTest_proxyProtocolRejectsInvalidPortDelimiters)
{
    initProxyTestCache();
    HttpServerConfig &config = HttpServerConfig::getInstance();
    const int oldDnsLookup = config.getDnsLookup();
    config.setDnsLookup(0);
    ClientInfo *proxy = getProxyTestClient("192.0.2.202");
    config.setDnsLookup(oldDnsLookup);
    CHECK(proxy != NULL);
    if (!proxy)
        return;

    ConnInfo connInfo;
    memset(&connInfo, 0, sizeof(connInfo));
    connInfo.m_pClientInfo = proxy;
    NtwkIOLink link;
    link.setConnInfo(&connInfo);

    struct sockaddr_storage from;
    struct sockaddr_storage to;
    char badSeparator[] = "TCP4 198.51.100.202 203.0.113.202 123x 443";
    CHECK_EQUAL(-1, link.process_proxy_v1(badSeparator,
                badSeparator + sizeof(badSeparator) - 1, &from, &to));

    char trailingGarbage[] =
        "TCP4 198.51.100.202 203.0.113.202 123 443junk";
    CHECK_EQUAL(-1, link.process_proxy_v1(trailingGarbage,
                trailingGarbage + sizeof(trailingGarbage) - 1, &from, &to));
}


TEST(NtwkIOLinkTest_proxyProtocolWaitsForCompleteHeader)
{
    const char v1[] = "PROXY TCP4 198.51.100.1";
    const unsigned char v2[] =
    {
        0x0d, 0x0a, 0x0d, 0x0a, 0x00, 0x0d,
        0x0a, 0x51, 0x55, 0x49, 0x54, 0x0a,
        0x21, 0x11, 0x00, 0x0c
    };
    const struct
    {
        const void *data;
        size_t len;
    } partial[] =
    {
        { v1, sizeof(v1) - 1 },
        { v2, 12 },
        { v2, sizeof(v2) }
    };

    for (unsigned i = 0; i < sizeof(partial) / sizeof(partial[0]); ++i)
    {
        int sockets[2];
        CHECK_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sockets));
        CHECK_EQUAL((ssize_t)partial[i].len,
                    write(sockets[0], partial[i].data, partial[i].len));

        NtwkIOLink link;
        link.setfd(sockets[1]);
        CHECK_EQUAL(0, link.tryProtocolProxy());

        close(sockets[0]);
        close(sockets[1]);
        link.setfd(-1);
    }
}


TEST(NtwkIOLinkTest_proxyProtocolPartialHeaderStopsPolling)
{
    initProxyTestCache();
    HttpServerConfig &config = HttpServerConfig::getInstance();
    const int oldDnsLookup = config.getDnsLookup();
    config.setDnsLookup(0);
    ClientInfo *proxy = getProxyTestClient("192.0.2.203");
    config.setDnsLookup(oldDnsLookup);
    CHECK(proxy != NULL);
    if (!proxy)
        return;

    Multiplexer *pOld = MultiplexerFactory::getMultiplexer();
    Multiplexer *pMultiplexer = MultiplexerFactory::getNew(
                                    MultiplexerFactory::getType("poll"));
    CHECK(pMultiplexer != NULL);
    if (!pMultiplexer)
        return;
    pMultiplexer->init(1024);
    MultiplexerFactory::setMultiplexer(pMultiplexer);
    DateTime::s_curTime = time(NULL);
    NtwkIOLink::setPrevToken(TIMER_PRECISION - 1);
    NtwkIOLink::setToken(0);

    int sockets[2];
    CHECK_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sockets));
    const char partial[] = "PROXY TCP4 198.51.100.1";
    CHECK_EQUAL((ssize_t)sizeof(partial) - 1,
                write(sockets[0], partial, sizeof(partial) - 1));

    ConnInfo connInfo;
    memset(&connInfo, 0, sizeof(connInfo));
    connInfo.m_pClientInfo = proxy;
    //closeSocket() recycles the link into the resource pool
    NtwkIOLink *link = new NtwkIOLink();
    link->setConnInfo(&connInfo);
    link->setfd(sockets[1]);
    link->setNoSSL();
    link->setActiveTime(DateTime::s_curTime);
    link->testTryProxyProtocol();
    proxy->incConn();
    ConnLimitCtrl::getInstance().incConn();
    CHECK_EQUAL(0, pMultiplexer->add(link, POLLIN | POLLHUP | POLLERR));

    //the header is still queued, so stop polling instead of spinning
    link->handleEvents(POLLIN);
    CHECK_EQUAL(0, link->getEvents() & POLLIN);

    //the next timer tick peeks again
    link->onTimer();
    CHECK(link->getEvents() & POLLIN);
    CHECK_EQUAL(sockets[1], link->getfd());
    link->handleEvents(POLLIN);
    CHECK_EQUAL(0, link->getEvents() & POLLIN);

    //a header that never completes closes the connection
    link->setActiveTime(DateTime::s_curTime - 10);
    link->onTimer();
    //closed with the partial header unread, so the peer sees a reset
    char ch;
    CHECK_EQUAL(-1, (int)recv(sockets[0], &ch, 1, MSG_DONTWAIT));
    CHECK_EQUAL(ECONNRESET, errno);
    CHECK_EQUAL(0, proxy->getConns());

    close(sockets[0]);
    MultiplexerFactory::setMultiplexer(pOld);
    MultiplexerFactory::recycle(pMultiplexer);
}

#endif
