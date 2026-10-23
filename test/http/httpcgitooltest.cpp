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

#include <http/httpextconnector.h>
#include <http/httpcgitool.h>
#include <http/httpdefs.h>
#include <http/handlertype.h>
#include <http/hiostream.h>
#include <http/httprespheaders.h>
#include <http/httpserverconfig.h>
#include <http/httpsession.h>
#include <http/httpvhost.h>
#include "unittest-cpp/UnitTest++.h"

#include <errno.h>
#include <string.h>
#include <string>


class CgiToolTestStream : public HioStream
{
public:
    int read(char *, int)                         { return 0; }
    int write(const char *, int size)              { return size; }
    int writev(const struct iovec *, int)           { return 0; }
    int flush()                                     { return 0; }
    int close()                                     { return 0; }
    void suspendRead()                              {}
    void continueRead()                             {}
    void suspendWrite()                             {}
    void continueWrite()                            {}
    int sendRespHeaders(HttpRespHeaders *, send_hdr_flag) { return 0; }
    void switchWriteToRead()                        {}

protected:
    const char *buildLogId()                        { return "cgi-tool-test"; }
};


static int processTestHeader(HttpExtConnector *connector,
                             const std::string &value)
{
    return HttpCgiTool::processHeaderLine(
        connector, HttpRespHeaders::H_HEADER_END, "X-Large", 7,
        value.data(), value.size());
}


TEST(HttpCgiToolTest_dynamicResponseFieldHardLimit)
{
    HttpServerConfig &config = HttpServerConfig::getInstance();
    uint32_t savedLimit = config.getMaxDynRespHeaderLen();
    const int rejected = HttpCgiTool::RESP_HEADER_ERR_SET;
    config.setMaxDynRespHeaderLen(MAX_DYN_RESP_HEADER_LEN);
    std::string accepted(HRH_MAX_LEN - 7 - 4, 'a');

    HttpSession acceptedSession;
    HttpExtConnector acceptedConnector;
    acceptedConnector.setHttpSession(&acceptedSession);
    CHECK_EQUAL(LS_OK, processTestHeader(&acceptedConnector, accepted));

    CgiToolTestStream rejectedStream;
    HttpSession rejectedSession;
    rejectedSession.attachStream(&rejectedStream);
    HttpExtConnector rejectedConnector;
    rejectedConnector.setHttpSession(&rejectedSession);
    accepted.push_back('a');
    CHECK_EQUAL(rejected, processTestHeader(&rejectedConnector, accepted));
    CHECK_EQUAL(SC_500, rejectedSession.getReq()->getStatusCode());
    CHECK((rejectedConnector.getState()
           & (HEC_ABORT_REQUEST | HEC_ERROR))
          == (HEC_ABORT_REQUEST | HEC_ERROR));
    CHECK(!rejectedConnector.isRecoverable());
    config.setMaxDynRespHeaderLen(savedLimit);
}


TEST(HttpCgiToolTest_dynamicResponseFieldUsesConfiguredLimit)
{
    HttpServerConfig &config = HttpServerConfig::getInstance();
    uint32_t savedLimit = config.getMaxDynRespHeaderLen();
    const int rejected = HttpCgiTool::RESP_HEADER_ERR_SET;
    const uint32_t configuredLimit = 4096;
    config.setMaxDynRespHeaderLen(configuredLimit);
    std::string accepted(configuredLimit - 7 - 4, 'a');

    HttpSession acceptedSession;
    HttpExtConnector acceptedConnector;
    acceptedConnector.setHttpSession(&acceptedSession);
    CHECK_EQUAL(LS_OK, processTestHeader(&acceptedConnector, accepted));

    CgiToolTestStream rejectedStream;
    HttpSession rejectedSession;
    rejectedSession.attachStream(&rejectedStream);
    HttpExtConnector rejectedConnector;
    rejectedConnector.setHttpSession(&rejectedSession);
    accepted.push_back('a');
    CHECK_EQUAL(rejected, processTestHeader(&rejectedConnector, accepted));
    CHECK_EQUAL(SC_500, rejectedSession.getReq()->getStatusCode());
    CHECK(!rejectedConnector.isRecoverable());
    config.setMaxDynRespHeaderLen(savedLimit);
}


//LSAPI-style call: a complete header line passed directly to the parser.
static int processTestLine(HttpExtConnector *connector, const char *line)
{
    return HttpCgiTool::processHeaderLine(connector, line,
                                          line + strlen(line));
}


TEST(HttpCgiToolTest_directHeaderFailureIsNotRecoverable)
{
    CgiToolTestStream stream;
    HttpSession session;
    session.attachStream(&stream);
    session.getResp()->setContentLen(LSI_BODY_SIZE_UNKNOWN);
    HttpExtConnector connector;
    connector.setHttpSession(&session);

    int ret = processTestLine(&connector, "Content-Length: abc");
    CHECK_EQUAL(LS_FAIL, ret);
    CHECK(connector.isRecoverable());
    errno = 0;
    CHECK_EQUAL(LS_FAIL, connector.respHeaderError(ret));
    CHECK_EQUAL(EPROTO, errno);
    CHECK_EQUAL(SC_500, session.getReq()->getStatusCode());
    CHECK((connector.getState() & (HEC_ABORT_REQUEST | HEC_ERROR))
          == (HEC_ABORT_REQUEST | HEC_ERROR));
    CHECK(!connector.isRecoverable());
}


TEST(HttpCgiToolTest_directHeaderStatus444KeepsDrop)
{
    CgiToolTestStream stream;
    HttpSession session;
    session.attachStream(&stream);
    HttpExtConnector connector;
    connector.setHttpSession(&session);

    int ret = processTestLine(&connector, "Status: 444");
    CHECK_EQUAL(LS_FAIL, ret);
    CHECK_EQUAL(HSS_DROP, session.getState());
    errno = 0;
    CHECK_EQUAL(LS_FAIL, connector.respHeaderError(ret));
    CHECK_EQUAL(EPROTO, errno);
    CHECK_EQUAL(HSS_DROP, session.getState());
    CHECK(connector.getState() & HEC_ABORT_REQUEST);
    CHECK(!(connector.getState() & HEC_ERROR));
    CHECK(!connector.isRecoverable());
}


TEST(HttpCgiToolTest_streamHeaderStatus444KeepsDrop)
{
    CgiToolTestStream stream;
    HttpSession session;
    session.attachStream(&stream);
    HttpExtConnector connector;
    connector.setHttpSession(&session);
    const char response[] = "Status: 444\r\n\r\n";
    const char *p = response;
    int len = sizeof(response) - 1;

    errno = 0;
    CHECK_EQUAL(LS_FAIL, connector.parseHeader(p, len));
    CHECK_EQUAL(EPROTO, errno);
    CHECK_EQUAL(HSS_DROP, session.getState());
    CHECK(connector.getState() & HEC_ABORT_REQUEST);
    CHECK(!(connector.getState() & HEC_ERROR));
    CHECK(!connector.isRecoverable());
}


TEST(HttpCgiToolTest_redirectIsNotRecoverable)
{
    HttpExtConnector connector;
    connector.setState(HEC_REDIRECT);
    CHECK(!connector.isRecoverable());
}


TEST(HttpCgiToolTest_directHeaderKeepsSelectedResponse)
{
    HttpServerConfig &config = HttpServerConfig::getInstance();
    int64_t savedLimit = config.getMaxDynRespLen();
    const int rejected = HttpCgiTool::RESP_HEADER_ERR_SET;
    config.setMaxDynRespLen(4096);
    CgiToolTestStream stream;
    HttpSession session;
    session.attachStream(&stream);
    session.getResp()->setContentLen(LSI_BODY_SIZE_UNKNOWN);
    HttpExtConnector connector;
    connector.setHttpSession(&session);

    int ret = processTestLine(&connector, "Content-Length: 4097");
    CHECK_EQUAL(rejected, ret);
    CHECK_EQUAL(LS_FAIL, connector.respHeaderError(ret));
    CHECK_EQUAL(SC_413, session.getReq()->getStatusCode());
    CHECK(!connector.isRecoverable());
    config.setMaxDynRespLen(savedLimit);
}


TEST(HttpCgiToolTest_doNotHonorProxyInternalLocation)
{
    HttpVHost vhost("proxy-internal-location-test");
    CHECK(vhost.addContext("/private/", HandlerType::HT_NULL,
                           "/htdocs_private/", NULL, 1) != NULL);
    HttpSession session;
    session.getReq()->setVHost(&vhost);
    session.getReq()->setStatusCode(SC_200);
    HttpExtConnector connector;
    connector.setHttpSession(&session);
    connector.setRespState(HEC_RESP_PROXY);
    const char location[] = "/htdocs_private/secret";

    CHECK_EQUAL(0, HttpCgiTool::processHeaderLine(
                    &connector, HttpRespHeaders::H_LITESPEED_LOCATION,
                    "X-LiteSpeed-Location", 20, location,
                    sizeof(location) - 1));
    CHECK(session.getReq()->getLocation() == NULL);
    const char *value;
    int valueLen;
    CHECK_EQUAL(-1, session.getResp()->getRespHeaders().getHeader(
                    "X-LiteSpeed-Location", 20, &value, valueLen));
}

TEST(HttpCgiToolTest_doNotHonorProxyStatusControls)
{
    HttpSession session;
    HttpExtConnector connector;
    connector.setHttpSession(&session);
    connector.setRespState(HEC_RESP_PROXY);
    const char response[] =
        "HTTP/1.1 200 OK\r\n"
        "Status: 444 BLOCK\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    CHECK_EQUAL((int)strlen(response), HttpCgiTool::parseRespHeader(
                    &connector, response, strlen(response)));
    CHECK_EQUAL(SC_200, session.getReq()->getStatusCode());

    HttpSession statusSession;
    HttpExtConnector statusConnector;
    statusConnector.setHttpSession(&statusSession);
    statusConnector.setRespState(HEC_RESP_PROXY);
    const char statusResponse[] = "HTTP/1.1 444 BLOCK\r\n\r\n";
    CHECK_EQUAL(LS_FAIL, HttpCgiTool::parseRespHeader(
                    &statusConnector, statusResponse,
                    strlen(statusResponse)));
}

TEST(HttpCgiToolTest_doNotHonorProxyRecaptchaControl)
{
    HttpSession session;
    HttpExtConnector connector;
    connector.setHttpSession(&session);
    connector.setRespState(HEC_RESP_PROXY);
    const char response[] =
        "HTTP/1.1 200 OK\r\n"
        "Lsrecaptcha: 1\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    CHECK_EQUAL((int)strlen(response), HttpCgiTool::parseRespHeader(
                    &connector, response, strlen(response)));
    const char *value;
    int valueLen;
    CHECK_EQUAL(-1, session.getResp()->getRespHeaders().getHeader(
                    "Lsrecaptcha", 11, &value, valueLen));
}

TEST(HttpCgiToolTest_rejectUnsafeInternalLocation)
{
    HttpVHost vhost("internal-location-test");
    CHECK(vhost.addContext("/errors/", HandlerType::HT_NULL,
                           "/htdocs_error/", NULL, 1) != NULL);
    HttpSession session;
    session.getReq()->setVHost(&vhost);
    HttpExtConnector connector;
    connector.setHttpSession(&session);
    const char location[] =
        "/htdocs_error/../../../../../home/phpmyadmin/public_html/../../hacking/url";

    CHECK_EQUAL(LS_FAIL, HttpCgiTool::processHeaderLine2(
                    &connector, HttpRespHeaders::H_LITESPEED_LOCATION,
                    "X-LiteSpeed-Location", 20, location,
                    sizeof(location) - 1));
}

// #include "httpcgitooltest.h"
// #include <http/httpsession.h>
// #include <http/httpextconnector.h>
// #include <http/httpcgitool.h>
// #include "unittest-cpp/UnitTest++.h"
//
// TEST( HttpCgiToolTest_test)
// {
//     HttpSession session;
//     HttpExtConnector connector;
//     connector.setHttpSession( &session );
//     session.getReq()->reset();
//     char test1[] =
//     "staTus: 404 not found   \t \r \n"
//     "Content-Type: \tapplication/x-testcgi \n"
//     "Script-Control: no-abort \n"
//     "Http-eXtended:my-extension\n"
//     "MultiLineHeader: line1\n"
//     " line2\r\n"
//     "\n"
//     "this is the resp body";
//     char * pEnd = test1 + strlen( test1 );
//     char * pCur = test1;
//     char * pBuf = test1;
//     int status = 0;
//     while( pCur < pEnd )
//     {
//         pCur++;
//         int ret = HttpCgiTool::parseRespHeader( &connector, pBuf, pCur - pBuf, status );
//         if ( ret > 0 )
//             pBuf += ret;
//         if ( status == HttpReq::HEADER_OK )
//             break;
//     }
//
//     CHECK( strncmp( pBuf, "this is the resp body", 21 ) == 0 );
//     CHECK( session.getReq()->getStatusCode() == SC_404 );
//     session.getResp()->getOutputBuf().append( '\0' );
//     const char * pOutput = session.getResp()->getOutputBuf().begin();
//     CHECK( strstr( pOutput,
//                     "Content-Type: \tapplication/x-testcgi\r\n" ) != NULL );
//     CHECK( strstr( pOutput,
//                     "Http-eXtended:my-extension\r\n" ) != NULL );
//     CHECK( strstr( pOutput,     "MultiLineHeader: line1\r\n"
//                                          " line2\r\n" ) != NULL );
//     CHECK( *session.getReq()->getLocation() == 0 );
//
//     char test2[] = "Location: http://www.somewhere.com/\n\n";
//     status = 0;
//     session.getResp()->reset();
//     session.getReq()->setStatusCode( SC_200 );
//     int ret = HttpCgiTool::parseRespHeader( &connector, test2, strlen( test2 ), status );
//     CHECK( ret == (int)strlen( test2 ) );
//     CHECK( status == HttpReq::HEADER_OK );
//     CHECK( session.getReq()->getStatusCode() == SC_302 );
//     session.getResp()->getOutputBuf().append( '\0' );
//     pOutput = session.getResp()->getOutputBuf().begin();
//
//     CHECK( strstr( pOutput,
//                     "Location: http://www.somewhere.com/\r\n" ) != NULL );
//     CHECK( *session.getReq()->getLocation() == 0 );
//
//     char test3[] = "Location: /internal/redirect/url\n\n";
//     status = 0;
//     session.getResp()->reset();
//     session.getReq()->setStatusCode( SC_200 );
//     ret = HttpCgiTool::parseRespHeader( &connector, test3, strlen( test3 ), status );
//     CHECK( ret == (int)strlen( test3 ) );
//     CHECK( status == HttpReq::HEADER_OK );
//     CHECK( session.getReq()->getStatusCode() == SC_200 );
//     session.getResp()->getOutputBuf().append( '\0' );
//     pOutput = session.getResp()->getOutputBuf().begin();
//
//     CHECK( strstr( pOutput,
//                     "/internal/redirect/url" ) == NULL );
//     CHECK( strcmp( "/internal/redirect/url",
//                             session.getReq()->getLocation()) == 0 );
//
//
// }

#endif
