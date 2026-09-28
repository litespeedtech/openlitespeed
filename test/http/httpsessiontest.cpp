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

#include <http/handlertype.h>
#include <http/httpcontext.h>
#include <http/htauth.h>
#include <http/httpreq.h>
#include <http/httpsession.h>
#include <http/httpvhost.h>
#include <lsiapi/modulemanager.h>

#include "unittest-cpp/UnitTest++.h"

#include <string.h>

TEST(HttpSessionTest_authenticateConfiguredWebSocketContext)
{
    HttpVHost vhost("websocket-auth-test");
    HttpContext *context = vhost.addContext(
                               "/socket/", HandlerType::HT_NULL,
                               "/srv/socket/", NULL, 1);
    CHECK(context != NULL);
    context->setWebSockAddr("127.0.0.1:9000", NULL, false);

    HttpSession session;
    HttpReq *req = session.getReq();
    req->setILog(&session);
    req->setVHost((HttpVHost *)1);
    const char request[] =
        "GET /socket/chat HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "\r\n";
    req->getHeaderBuf().append(request, sizeof(request) - 1);
    CHECK_EQUAL(0, req->processHeader());
    req->setVHost(&vhost);

    CHECK_EQUAL(0, session.testProcessContextMap());
    CHECK_EQUAL(context, req->getContext());
    CHECK_EQUAL(HSPS_CHECK_AUTH_ACCESS, session.testGetProcessState());
}


TEST(HttpSessionTest_deferRewrittenWebSocketUntilAuthentication)
{
    HttpSession session;
    const char target[] = "ws://127.0.0.1:9000/socket";

    CHECK(session.testGetWebSocketBackend() == NULL);
    CHECK_EQUAL(0, session.testGetDeferredWebSocket().len());
    CHECK_EQUAL(0, session.testDeferWebSocketUpgrade(
                       target, sizeof(target) - 1));
    CHECK_EQUAL((int)sizeof(target) - 1,
                session.testGetDeferredWebSocket().len());
    CHECK_EQUAL(target, session.testGetDeferredWebSocket().c_str());
    CHECK_EQUAL(SC_500, session.testDeferWebSocketUpgrade(target, 0));
    session.testClearDeferredWebSocket();
    CHECK_EQUAL(0, session.testGetDeferredWebSocket().len());
    CHECK(session.testGetWebSocketBackend() == NULL);
}


TEST(HttpSessionTest_authWebSocketUpgradeSkipsRequestMapping)
{
    HttpVHost vhost("websocket-direct-auth-test");
    HttpContext *context = vhost.addContext(
                               "/socket/", HandlerType::HT_NULL,
                               "/srv/socket/", NULL, 1);
    CHECK(context != NULL);

    HttpSession session;
    HttpReq *req = session.getReq();
    req->orContextState(CONTEXT_AUTH_CHECKED);
    CHECK_EQUAL(0, session.testAuthWebSocketUpgrade(context));
    CHECK_EQUAL(context, req->getContext());
    CHECK_EQUAL(0, req->getContextState(CONTEXT_AUTH_CHECKED));
    CHECK(session.getFlag(HSF_URI_PROCESSED));
    CHECK_EQUAL(HSPS_CHECK_AUTH_ACCESS, session.testGetProcessState());

    //same context keeps the authentication state of the current request
    req->orContextState(CONTEXT_AUTH_CHECKED);
    CHECK_EQUAL(0, session.testAuthWebSocketUpgrade(context));
    CHECK(req->getContextState(CONTEXT_AUTH_CHECKED));
}


TEST(HttpSessionTest_webSocketAuthHonorsRegexContext)
{
    HttpVHost vhost("websocket-regex-auth-test");
    HttpContext *backend = vhost.addContext(
                               "/socket/", HandlerType::HT_NULL,
                               "/srv/socket/", NULL, 1);
    CHECK(backend != NULL);
    backend->setWebSockAddr("127.0.0.1:9000", NULL, false);
    HttpContext *protectedContext = vhost.addContext(
                                        1, "^/socket/.*", HandlerType::HT_NULL,
                                        "/srv/socket/", NULL, 1);
    CHECK(protectedContext != NULL);
    protectedContext->setHTAuth(new HTAuth());
    protectedContext->setAuthRequired("valid-user");
    ModuleConfig moduleConfig;
    protectedContext->setModuleConfig(&moduleConfig, 0);

    HttpSession session;
    HttpReq *req = session.getReq();
    req->setILog(&session);
    req->setVHost((HttpVHost *)1);
    const char request[] =
        "GET /socket/chat HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "\r\n";
    req->getHeaderBuf().append(request, sizeof(request) - 1);
    CHECK_EQUAL(0, req->processHeader());
    req->setVHost(&vhost);

    CHECK_EQUAL(0, session.testMapAndAuthWebSocketUpgrade(backend));
    CHECK_EQUAL(protectedContext, req->getContext());
    CHECK_EQUAL(backend, session.testGetWebSocketBackend());
    CHECK_EQUAL(&moduleConfig, session.getModuleConfig());
    CHECK_EQUAL(SC_401, session.testProcessContextAuth());
}


TEST(HttpSessionTest_rewrittenWebSocketIgnoresContextRedirect)
{
    HttpVHost vhost("websocket-redirect-test");
    HttpContext *backend = vhost.addContext(
                               "/ws/", HandlerType::HT_NULL,
                               "/srv/ws/", NULL, 1);
    CHECK(backend != NULL);

    HttpSession session;
    HttpReq *req = session.getReq();
    req->setILog(&session);
    req->setVHost((HttpVHost *)1);
    const char request[] =
        "GET /ws HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "\r\n";
    req->getHeaderBuf().append(request, sizeof(request) - 1);
    CHECK_EQUAL(0, req->processHeader());
    req->setVHost(&vhost);
    const char target[] = "ws://127.0.0.1:9000/ws";
    CHECK_EQUAL(0, session.testDeferWebSocketUpgrade(
                       target, sizeof(target) - 1));

    //auth checked by an earlier pass before an internal redirect restart
    req->orContextState(CONTEXT_AUTH_CHECKED);

    CHECK_EQUAL(0, session.testMapAndAuthWebSocketUpgrade(backend));
    CHECK_EQUAL(backend, req->getContext());
    CHECK(req->getLocation() == NULL);
    CHECK_EQUAL(0, req->getContextState(CONTEXT_AUTH_CHECKED));
    CHECK_EQUAL(HSPS_CHECK_AUTH_ACCESS, session.testGetProcessState());
}


TEST(HttpSessionTest_httpErrorCancelsDeferredWebSocketUpgrade)
{
    HttpVHost vhost("websocket-error-test");
    HttpContext *backend = vhost.addContext(
                               "/ws/", HandlerType::HT_NULL,
                               "/srv/ws/", NULL, 1);
    CHECK(backend != NULL);

    HttpSession session;
    HttpReq *req = session.getReq();
    req->setILog(&session);
    req->setVHost((HttpVHost *)1);
    const char request[] =
        "GET /ws/ HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "\r\n";
    req->getHeaderBuf().append(request, sizeof(request) - 1);
    CHECK_EQUAL(0, req->processHeader());
    req->setVHost(&vhost);

    const char target[] = "ws://127.0.0.1:9000/ws/";
    CHECK_EQUAL(0, session.testDeferWebSocketUpgrade(
                       target, sizeof(target) - 1));
    CHECK_EQUAL(0, session.testMapAndAuthWebSocketUpgrade(backend));
    CHECK(session.testGetDeferredWebSocket().len() > 0);
    CHECK_EQUAL(backend, session.testGetWebSocketBackend());

    //Avoid rendering a response so the real error path needs no network stream.
    session.setFlag(HSF_NO_ERROR_PAGE);
    CHECK_EQUAL(0, session.httpError(SC_401));

    CHECK_EQUAL(0, session.testGetDeferredWebSocket().len());
    CHECK(session.testGetWebSocketBackend() == NULL);
}


TEST(HttpSessionTest_errorPageDoesNotOpenWebSocket)
{
    HttpVHost vhost("websocket-error-page-test");
    HttpContext *app = vhost.addContext(
                           "/app/", HandlerType::HT_NULL,
                           "/srv/app/", NULL, 1);
    CHECK(app != NULL);
    app->setWebSockAddr("127.0.0.1:9000", NULL, false);
    HttpContext *protectedContext = vhost.addContext(
                                        "/app/private/", HandlerType::HT_NULL,
                                        "/srv/app/private/", NULL, 1);
    CHECK(protectedContext != NULL);
    protectedContext->setHTAuth(new HTAuth());
    protectedContext->setAuthRequired("valid-user");

    HttpSession session;
    HttpReq *req = session.getReq();
    req->setILog(&session);
    req->setVHost((HttpVHost *)1);
    const char request[] =
        "GET /app/private/ws HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "\r\n";
    req->getHeaderBuf().append(request, sizeof(request) - 1);
    CHECK_EQUAL(0, req->processHeader());
    req->setVHost(&vhost);

    //the protected context has no WebSocket backend and rejects the request
    CHECK_EQUAL(0, session.testProcessContextMap());
    CHECK_EQUAL(protectedContext, req->getContext());
    CHECK_EQUAL(HSPS_CONTEXT_REWRITE, session.testGetProcessState());
    CHECK_EQUAL(SC_401, session.testProcessContextAuth());
    CHECK(session.testShouldUpgradeWebSocket(app));

    //sendHttpError() redirects to a custom page under the WebSocket context
    req->setErrorPage();
    const char errorPage[] = "/app/401.html";
    CHECK_EQUAL(0, req->redirect(errorPage, sizeof(errorPage) - 1, 1));
    req->setContext(NULL);

    CHECK_EQUAL(0, session.testProcessContextMap());
    CHECK_EQUAL(app, req->getContext());
    CHECK_EQUAL(HSPS_CONTEXT_REWRITE, session.testGetProcessState());
    CHECK(!session.testShouldUpgradeWebSocket(req->getContext()));
}

#endif
