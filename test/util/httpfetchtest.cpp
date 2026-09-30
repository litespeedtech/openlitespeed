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
#ifdef RUN_TEST

#include <util/httpfetch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <adns/adns.h>
#include "socket/gsockaddr.h"
#include "unittest-cpp/UnitTest++.h"
#include <edio/multiplexer.h>
#include <edio/multiplexerfactory.h>
#include <edio/evtcbque.h>
#include <lsr/ls_base64.h>
#include <sslpp/sslutil.h>
#include <util/vmembuf.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>


class HttpFetchTestPeer
{
public:
    static int buildReq(HttpFetch *pFetch, const char *pUrl)
    {   return pFetch->buildReq("GET", pUrl);   }

    static const char *getAddress(const HttpFetch *pFetch)
    {   return pFetch->m_achHost;   }

    static const char *getHost(const HttpFetch *pFetch)
    {   return pFetch->m_achHost + pFetch->m_iHostOffset;   }

    static int getHostLen(const HttpFetch *pFetch)
    {   return pFetch->m_iHostLen;   }

    static int getAuthorityLen(const HttpFetch *pFetch)
    {   return pFetch->m_iAuthorityLen;   }

    static int getHostOffset(const HttpFetch *pFetch)
    {   return pFetch->m_iHostOffset;   }

    static int isVerifyCert(const HttpFetch *pFetch)
    {   return pFetch->m_iVerifyCert;   }

    static int driveTlsHandshakeTimeout(HttpFetch *pFetch)
    {
        int fd[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == -1)
            return -2;
        pFetch->m_fdHttp = fd[0];
        pFetch->m_connTimeout = 0;
        pFetch->m_pollEvents = POLLIN;
        pFetch->m_reqState = HttpFetch::STATE_CONNECTED;
        int ret = pFetch->processEvents(POLLOUT | POLLERR);
        close(fd[1]);
        return ret;
    }
};


static int countFetchCallback(void *pArg, HttpFetch *)
{
    ++*(int *)pArg;
    return 0;
}


TEST(httpfetchAuthorityTest)
{
    struct authority_test
    {
        const char *url;
        const char *address;
        const char *authority;
        const char *host;
        int useSsl;
        int hostOffset;
    } tests[] =
    {
        { "http://example.com/a", "example.com:80", "example.com",
          "example.com", 0, 0 },
        { "https://example.com/a", "example.com:443", "example.com",
          "example.com", 1, 0 },
        { "https://example.com:8443/a", "example.com:8443",
          "example.com:8443", "example.com", 1, 0 },
        { "https://192.0.2.10:9443/a", "192.0.2.10:9443",
          "192.0.2.10:9443", "192.0.2.10", 1, 0 },
        { "https://[2001:db8::1]/a", "[2001:db8::1]:443",
          "[2001:db8::1]", "2001:db8::1", 1, 1 },
        { "https://[2001:db8::1]:8443/a", "[2001:db8::1]:8443",
          "[2001:db8::1]:8443", "2001:db8::1", 1, 1 },
    };

    for (unsigned int i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i)
    {
        HttpFetch fetch;
        CHECK_EQUAL(0, HttpFetchTestPeer::buildReq(&fetch, tests[i].url));
        CHECK_EQUAL(tests[i].useSsl, fetch.isUseSsl());
        CHECK(strcmp(tests[i].address,
                     HttpFetchTestPeer::getAddress(&fetch)) == 0);
        CHECK_EQUAL((int)strlen(tests[i].authority),
                    HttpFetchTestPeer::getAuthorityLen(&fetch));
        CHECK(memcmp(tests[i].authority,
                     HttpFetchTestPeer::getAddress(&fetch),
                     strlen(tests[i].authority)) == 0);
        CHECK_EQUAL(tests[i].hostOffset,
                    HttpFetchTestPeer::getHostOffset(&fetch));
        CHECK_EQUAL((int)strlen(tests[i].host),
                    HttpFetchTestPeer::getHostLen(&fetch));
        CHECK(memcmp(tests[i].host, HttpFetchTestPeer::getHost(&fetch),
                     strlen(tests[i].host)) == 0);
    }

    const char *badUrls[] =
    {
        "https:///a",
        "https://:443/a",
        "https://[2001:db8::1/a",
        "https://2001:db8::1/a",
    };
    for (unsigned int i = 0; i < sizeof(badUrls) / sizeof(badUrls[0]); ++i)
    {
        HttpFetch fetch;
        CHECK_EQUAL(-1, HttpFetchTestPeer::buildReq(&fetch, badUrls[i]));
    }

    HttpFetch fetch;
    CHECK_EQUAL(1, HttpFetchTestPeer::isVerifyCert(&fetch));
    fetch.setVerifyCert(0);
    fetch.reset();
    CHECK_EQUAL(0, HttpFetchTestPeer::isVerifyCert(&fetch));
    CHECK_EQUAL(0, HttpFetchTestPeer::buildReq(
                          &fetch, "https://example.com/a"));
    CHECK_EQUAL(1, fetch.isUseSsl());
    fetch.reset();
    CHECK_EQUAL(0, HttpFetchTestPeer::buildReq(
                          &fetch, "http://example.com/a"));
    CHECK_EQUAL(0, fetch.isUseSsl());
}


TEST(httpfetchTlsTimeoutCallbackTest)
{
    HttpFetch fetch;
    int callbacks = 0;

    fetch.setVerifyCert(0);
    fetch.setCallBack(countFetchCallback, &callbacks);
    CHECK_EQUAL(0, HttpFetchTestPeer::buildReq(
                          &fetch, "https://example.com/a"));
    CHECK_EQUAL(-1, HttpFetchTestPeer::driveTlsHandshakeTimeout(&fetch));
    CHECK_EQUAL(HttpFetch::ERROR_CONN_TIMEOUT, fetch.getStatusCode());
    CHECK_EQUAL(1, callbacks);
    CHECK_EQUAL(HttpFetch::STATE_FINISH, fetch.getReqState());
}


TEST(httpfetchTest_Test)
//void VOID_TEST()//httpfetchTest_Test)
{
    const char *pRunNetwork = getenv("LS_HTTPFETCH_NETWORK_TESTS");
    if (!pRunNetwork || strcmp(pRunNetwork, "1") != 0)
        return;

    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("THIS TEST CAN ONLY TEST BY DEBUG with breakpoints\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    // Required before any anonymous-mapped VMemBuf is built (requests with a
    // NULL save file); the light-tier main() does not initialize it.
    VMemBuf::initAnonPool();

    MultiplexerFactory::initDefault();
    Multiplexer *mult = MultiplexerFactory::getMultiplexer();
    HttpFetch *pHttpFetch = new HttpFetch;
    //pHttpFetch->setResProcessor(NULL, p);
    //pHttpFetch->setTimeout(100);

    int nonblock = 0;
    const char *pBody = NULL;
    int bodyLen = 0;
    const char *pSaveFile = "./lswslogo3.png";


    Adns::getInstance().init();
    EvtcbQue::getInstance().initNotifier();

    pHttpFetch->setProxyServerAddr("127.0.0.1:8000");
    int ret = pHttpFetch->startReq("http://www.litespeedtech.com/templates/litespeed/images/lswslogo3.png",
                                   nonblock, 1,
                                   pBody,
                                   bodyLen,
                                   pSaveFile);
    delete pHttpFetch;
    pHttpFetch = NULL;

    pHttpFetch = new HttpFetch;
    ret = pHttpFetch->startReq("http://www.litespeedtech.com/templates/litespeed/images/lswslogo3.png",
                               nonblock, 1,
                               pBody,
                               bodyLen,
                               pSaveFile, NULL, "www.litespeedtech.com:80");
    mult->waitAndProcessEvents(5000);
    delete pHttpFetch;
    pHttpFetch = NULL;

    pHttpFetch = new HttpFetch;
    GSockAddr gsock;
    gsock.setHttpUrl("http://www.litespeedtech.com/",
                     strlen("http://www.litespeedtech.com/"));

    GSockAddr gsock1;
    gsock1.setHttpUrl("https://www.litespeedtech.com/",
                      strlen("https://www.litespeedtech.com/"));

    GSockAddr gsock2;
    gsock2.setHttpUrl("http://www.litespeedtech.com:888",
                      strlen("http://www.litespeedtech.com:998"));

    GSockAddr gsock3;
    gsock3.setHttpUrl("https://www.litespeedtech.com:444",
                      strlen("https://www.litespeedtech.com:998"));

    GSockAddr gsock4;
    gsock4.setHttpUrl("http://www.litespeedtech.com/dfgfdgfdgfdg",
                      strlen("http://www.litespeedtech.com/dfgfdgfdgfdg"));

    GSockAddr gsock5;
    gsock5.setHttpUrl("http://www.litespeedtech.com:888/dfgfdgfdgfdg",
                      strlen("http://www.litespeedtech.com:888/dfgfdgfdgfdg"));

    GSockAddr gsock6;
    gsock6.setHttpUrl("https://www.litespeedtech.com/dfgfdgfdgfdg",
                      strlen("https://www.litespeedtech.com/dfgfdgfdgfdg"));

    GSockAddr gsock7;
    gsock7.setHttpUrl("https://www.litespeedtech.com:444/dfgfdgfdgfdg",
                      strlen("https://www.litespeedtech.com:888/dfgfdgfdgfdg"));

    ret = pHttpFetch->startReq("http://www.litespeedtech.com/templates/litespeed/images/lswslogo3.png",
                               nonblock, 1,
                               pBody,
                               bodyLen,
                               pSaveFile, 0, gsock);
    mult->waitAndProcessEvents(5000);
    delete pHttpFetch;
    pHttpFetch = NULL;

    pHttpFetch = new HttpFetch;
    pHttpFetch->setProxyServerAddr("127.0.0.1:8000");
    ret = pHttpFetch->startReq("http://www.litespeedtech.com/templates/litespeed/images/lswslogo3.png",
                               nonblock, 1,
                               pBody,
                               bodyLen,
                               pSaveFile, 0, gsock);
    delete pHttpFetch;
    pHttpFetch = NULL;

    CHECK(ret == 0);

    pHttpFetch = new HttpFetch;
    ret = pHttpFetch->startReq("http://open.litespeedtech.com/packages/openlitespeed-1.4.25.tgz",
                               nonblock, 1,
                               pBody,
                               bodyLen,
                               pSaveFile, 0, gsock);
    mult->waitAndProcessEvents(5000);
// The following can be uncommented to test checking the response.
//     sleep(5);
//     ret = pHttpFetch->process();
    delete pHttpFetch;
    pHttpFetch = NULL;

    CHECK(ret == 0);



    pHttpFetch = new HttpFetch;
    SslUtil::initDefaultCA(NULL, NULL);
    /**
     * The following request is meant to test secure connections.
     *
     * The commented out logic afterwards is meant to check the
     * request processing as well as the results of the request.
     *
     * I(Kevin) found that the sleep/process -> waitAndProcessEvents -> sleep/process
     * combination was sufficient to complete the connections, process SSL,
     * and download the data. These steps may not all be necessary, but this
     * combination worked for me.
     *
     * NOTICE: Please test all combinations of
     *      nonblock(0/1),
     *      enableDriver(0/1),
     *      and isSecure(HF_SECURE/HF_UNKNOWN)
     *
     * As they may impact the output.
     */
    ret = pHttpFetch->startReq(
        "https://www.litespeedtech.com/images/litespeed/Subpage_misc/lsws-header.png",
        1, 1, // Check combinations here
        pBody,
        bodyLen,
        pSaveFile,
        NULL, NULL); //and here

//
//     sleep(5);
//     ret = pHttpFetch->process();
//
//     mult->waitAndProcessEvents(5000);
//     sleep(5);
//     ret = pHttpFetch->process();
//
//     CHECK(pHttpFetch->getStatusCode() == 200);
//
//     if (pHttpFetch->getStatusCode() == 200)
//     {
//         VMemBuf *pBuf = pHttpFetch->getResult();
//         int fd = open("/home/user/testinfo.png", O_RDWR | O_CREAT | O_TRUNC);
//         pBuf->copyToFile(0, pBuf->getCurWOffset(), fd, 0);
//         close(fd);
//     }

    delete pHttpFetch;
    pHttpFetch = NULL;

    CHECK(ret >= 0);

    // Retain public endpoint probes for explicit integration runs while the
    // environment gate above keeps normal CI independent of the Internet.
    pHttpFetch = new HttpFetch;
    ret = pHttpFetch->startReq("https://quic.cloud/ips?ln", 0, 0);
    if (ret == 0)
        pHttpFetch->process();
    CHECK_EQUAL(200, pHttpFetch->getStatusCode());
    delete pHttpFetch;
    pHttpFetch = NULL;

    pHttpFetch = new HttpFetch;
    ret = pHttpFetch->startReq("https://wrong.host.badssl.com/", 0, 0);
    if (ret == 0)
        pHttpFetch->process();
    CHECK_EQUAL(HttpFetch::ERROR_SSL_UNMATCH_DOMAIN,
                pHttpFetch->getStatusCode());
    delete pHttpFetch;
    pHttpFetch = NULL;

    pHttpFetch = new HttpFetch;
    ret = pHttpFetch->startReq("https://self-signed.badssl.com/", 0, 0);
    if (ret == 0)
        pHttpFetch->process();
    CHECK_EQUAL(HttpFetch::ERROR_SSL_CERT_VERIFY,
                pHttpFetch->getStatusCode());
    delete pHttpFetch;
    pHttpFetch = NULL;

    HttpFetch *pZConf = new HttpFetch();
    const char *pServerUp = "conf=\n"
        "{\n"
        "\"max_conn\" : 1000,\n"
        "\"vhost_list\" :\n"
        "    [\n"
        "        {\n"
        "        \"domain_list\" :\n"
        "            [ \"wiki.ls.lan\" ],\n"
        "        \"conf_list\":[\n"
        "            {\n"
        "            \"lb_port_list\" : [ 9090 ],\n" // Load balancer port(s) to listen on
        "            \"dport\" : 80,\n" // Destination port for backend.
        "            \"be_ssl\" : false,\n"
        "            \"ip_list\" :\n"
        "            [\n"
        "                { \"ip\" : \"192.168.0.111\" }\n" // Backend IP - may want to be the same as identifying IP?
        "            ]\n"
        "            }\n"
        "        ]\n"
        "        }\n"
        "    ]\n"
        "}\n";
    int iServerUpLen = strlen(pServerUp);
    char achEncoded[512], achAuthHdr[512];
    int iEncodedLen, iAuthHdrLen;

    iEncodedLen = ls_base64_encode("ron:ron", 7, achEncoded,
                                   sizeof(achEncoded));


    // For HttpFetch, cannot use url based auth. Add authorization header instead.
    iAuthHdrLen = snprintf(achAuthHdr, 512, "Authorization: Basic %.*s\r\n",
                           iEncodedLen, achEncoded);

    pZConf->setExtraHeaders(achAuthHdr, iAuthHdrLen);

    // serverUp request self identifying as hi_lswiki and ip 1.2.3.7
    ret = pZConf->startReq(
        "https://localhost:7443/ZCUP?name=hi_lswiki",
        1, 1, // Check combinations here
        pServerUp,
        iServerUpLen,
        NULL,
        NULL, NULL); //and here

//     sleep(5);
//     ret = pZConf->process();
//
//     mult->waitAndProcessEvents(5000);
//     sleep(5);
//     ret = pZConf->process();
//
//     CHECK(pZConf->getStatusCode() == 200);
//
//     if (pZConf->getStatusCode() == 200)
//     {
//         VMemBuf *pBuf = pZConf->getResult();
//         int fd = open("/home/user/testinfo.txt", O_RDWR | O_CREAT | O_TRUNC);
//         pBuf->copyToFile(0, pBuf->getCurWOffset(), fd, 0);
//         close(fd);
//     }






}

#endif
