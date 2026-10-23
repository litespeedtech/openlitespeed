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

#include <http/httpcontext.h>
#include <http/httpheader.h>
#include <http/httpmethod.h>
#include <http/httpserverconfig.h>
#include <http/httpvhost.h>
#include <http/httpver.h>
#include <http/httprespheaders.h>
#include <http/httpsession.h>
#include <http/ntwkiolink.h>
#include <util/iovec.h>
#include <lsqpack.h>
#include "unittest-cpp/UnitTest++.h"

#include <string>
#include <string.h>

static std::string flatten(const IOVec &vec)
{
    std::string result;
    const struct iovec *p = vec.get();
    const struct iovec *end = p + vec.len();
    for (; p < end; ++p)
        result.append((const char *)p->iov_base, p->iov_len);
    return result;
}

static void addLink(HttpRespHeaders &headers, const char *value)
{
    CHECK_EQUAL(LS_OK, headers.add(HttpRespHeaders::H_LINK, value,
                                  strlen(value), LSI_HEADER_ADD));
}

class HintTestStream : public HioStream
{
public:
    int sent;
    int fail;
    int code;
    send_hdr_flag flags;
    std::string wire;
    HintTestStream() : sent(0), fail(0), code(0), flags(SHF_NONE)
    { setProtocol(HIOS_PROTO_HTTP2); }
    int read(char *, int) { return 0; }
    int write(const char *, int size) { return size; }
    int writev(const struct iovec *, int) { return 0; }
    int flush() { return 0; }
    int close() { return 0; }
    void suspendRead() {}
    void continueRead() {}
    void suspendWrite() {}
    void continueWrite() {}
    void switchWriteToRead() {}
    int sendRespHeaders(HttpRespHeaders *headers, send_hdr_flag flag)
    {
        if (fail)
            return LS_FAIL;
        ++sent;
        flags = flag;
        code = headers->getHttpCode();
        IOVec vec;
        headers->outputNonSpdyHeaders(&vec);
        wire += flatten(vec);
        return LS_OK;
    }
protected:
    const char *buildLogId() { return "early-hints-test"; }
};

struct HintConfigGuard
{
    int saved;
    HintConfigGuard() : saved(HttpServerConfig::getInstance().getEarlyHints()) {}
    ~HintConfigGuard() { HttpServerConfig::getInstance().setEarlyHints(saved); }
};

TEST(EarlyHints_protocolModesAndVhostInheritance)
{
    HintConfigGuard guard;
    HttpServerConfig &config = HttpServerConfig::getInstance();
    HttpVHost vhost("hints-test");
    HintTestStream stream;
    HttpSession session;
    session.attachStream(&stream);
    session.getReq()->setVHost(&vhost);
    for (int mode = 0; mode <= 2; ++mode)
    {
        config.setEarlyHints(mode);
        CHECK_EQUAL(mode, vhost.getEarlyHints());
        stream.setProtocol(HIOS_PROTO_HTTP);
        CHECK_EQUAL(mode == 2, session.earlyHintsEnabled());
        stream.setProtocol(HIOS_PROTO_HTTP2);
        CHECK_EQUAL(mode != 0, session.earlyHintsEnabled());
        stream.setProtocol(HIOS_PROTO_HTTP3);
        CHECK_EQUAL(mode != 0, session.earlyHintsEnabled());
    }
    vhost.setEarlyHints(0);
    CHECK(!session.earlyHintsEnabled());
    config.setEarlyHints(0);
    vhost.setEarlyHints(2);
    stream.setProtocol(HIOS_PROTO_HTTP);
    CHECK(session.earlyHintsEnabled());
    vhost.setEarlyHints(-1);
    CHECK(!session.earlyHintsEnabled());
    session.getReq()->setVHost(NULL);
}

TEST(EarlyHints_statusAndSerialization)
{
    HttpStatusCode &status = HttpStatusCode::getInstance();
    CHECK_EQUAL(SC_103, status.codeToIndex(103));
    CHECK_EQUAL(SC_103, status.codeToIndex("103"));
    CHECK_EQUAL(103, status.indexToCode(SC_103));
    CHECK_EQUAL(200, status.indexToCode(SC_200));
    HttpRespHeaders headers;
    addLink(headers, "</style.css>; rel=preload; as=style");
    headers.addStatusLine(HTTP_1_1, SC_103, 1);
    IOVec vec;
    headers.outputNonSpdyHeaders(&vec);
    CHECK_EQUAL("HTTP/1.1 103 Early Hints\r\n"
                "link: </style.css>; rel=preload; as=style\r\n\r\n",
                flatten(vec).c_str());
    headers.prepareSendQpack();
    CHECK_EQUAL((unsigned)LSQPACK_TNV_STATUS_103,
                (unsigned)headers.begin()->qpack_index);
    CHECK(headers.begin()->flags & LSXPACK_VAL_MATCHED);
    CHECK_EQUAL(0, memcmp("103", lsxpack_header_get_value(headers.begin()), 3));
    headers.prepareSendHpack();
    CHECK(!(headers.begin()->flags & LSXPACK_HPACK_VAL_MATCHED));
    CHECK_EQUAL(0, memcmp("103", lsxpack_header_get_value(headers.begin()), 3));
}

TEST(EarlyHints_parseLinkMembers)
{
    HttpRespHeaders headers;
    addLink(headers, "</one,a.css>; title=\"a, b; preload\"; rel=\"alternate preload\"; as=style, "
                     "</skip>; rel=preloader, </two.js>; rel=modulepreload; nopush");
    addLink(headers, "<https://cdn.example/>; REL=preconnect");
    addLink(headers, "</skip>; title=\"preload\"; rel=alternate");
    addLink(headers, "</skip>; rel=preload; anchor=\"/other\"");
    addLink(headers, "</skip>; rel=alternate; rel=preload");
    addLink(headers, "</skip>; rel=preload; title=\"unterminated");
    HttpRespHeaders hints;
    CHECK_EQUAL(3, headers.buildEarlyHints(hints));
    struct iovec links[8];
    CHECK_EQUAL(3, hints.getHeader(HttpRespHeaders::H_LINK, links, 8));
    CHECK_EQUAL("</one,a.css>; title=\"a, b; preload\"; rel=\"alternate preload\"; as=style",
                std::string((char *)links[0].iov_base, links[0].iov_len).c_str());
    CHECK_EQUAL("</two.js>; rel=modulepreload; nopush",
                std::string((char *)links[1].iov_base, links[1].iov_len).c_str());
    CHECK_EQUAL("<https://cdn.example/>; REL=preconnect",
                std::string((char *)links[2].iov_base, links[2].iov_len).c_str());
}

TEST(EarlyHints_unsetLinkDoesNotGenerateHints)
{
    HttpRespHeaders headers;
    addLink(headers, "</style.css>; rel=preload");
    headers.del(HttpRespHeaders::H_LINK);
    HttpRespHeaders hints;
    CHECK_EQUAL(0, headers.buildEarlyHints(hints));
}

TEST(EarlyHints_preserveCommasInUrisAndQuotedParameters)
{
    const char first[] = "</f?a=1,2>; rel=preload; as=font";
    const char second[] = "</g?a=1,2>; rel=preload; as=font";
    const char third[] = "</h>; rel=preload; title=\"a,b\"";
    const char fourth[] = "</i>; rel=preload; title=\"a,b\"";
    HttpRespHeaders headers;
    addLink(headers, (std::string(first) + ", " + second).c_str());
    addLink(headers, third);
    addLink(headers, fourth);
    HttpRespHeaders hints;
    CHECK_EQUAL(4, headers.buildEarlyHints(hints));
    struct iovec links[4];
    CHECK_EQUAL(4, hints.getHeader(HttpRespHeaders::H_LINK, links, 4));
    const char *expected[] = {first, second, third, fourth};
    for (int i = 0; i < 4; ++i)
        CHECK_EQUAL(expected[i],
                    std::string((char *)links[i].iov_base, links[i].iov_len));
}

TEST(EarlyHints_firstRelationWins)
{
    HttpRespHeaders headers;
    const char first[] = "</app.css>; rel=preload; as=style; rel=stylesheet";
    addLink(headers, first);
    addLink(headers, "</skip.css>; rel=stylesheet; rel=preload");
    HttpRespHeaders hints;
    CHECK_EQUAL(1, headers.buildEarlyHints(hints));
    int len;
    const char *value = hints.getHeader(HttpRespHeaders::H_LINK, &len);
    CHECK_EQUAL(first, std::string(value, len));
}

TEST(EarlyHints_linkOpFlagTracksInheritance)
{
    HttpHeaderOps parent, child, replacement;
    const char security[] = "set X-Frame-Options DENY";
    CHECK_EQUAL(0, child.parseOp(security, security + sizeof(security) - 1, 0));
    CHECK(!child.hasLinkOp());
    const char link[] = "unset Link";
    CHECK_EQUAL(0, parent.parseOp(link, link + sizeof(link) - 1, 0));
    CHECK(parent.hasLinkOp());
    child.inherit(parent);
    CHECK(child.hasLinkOp());
    child.inherit(replacement);
    CHECK(!child.hasLinkOp());
    CHECK_EQUAL(0, child.parseOp(link, link + sizeof(link) - 1, 1));
    CHECK(!child.hasLinkOp());
}

TEST(EarlyHints_requestLinkOpsSkipUnrelatedAndNullContexts)
{
    HttpSession session;
    CHECK(!session.getReq()->hasLinkHeaderOps());
    session.getReq()->orContextState(GLOBAL_VH_CTX);
    CHECK(!session.getReq()->hasLinkHeaderOps());
    HttpContext context;
    const char security[] = "set Strict-Transport-Security max-age=86400";
    CHECK_EQUAL(0, context.setHeaderOps("test", security, sizeof(security) - 1));
    session.getReq()->setContext(&context);
    CHECK(!session.getReq()->hasLinkHeaderOps());
    const char link[] = "unset Link";
    CHECK_EQUAL(0, context.setHeaderOps("test", link, sizeof(link) - 1));
    CHECK(session.getReq()->hasLinkHeaderOps());
}

TEST(EarlyHints_allLinkFieldsAreIncluded)
{
    HttpRespHeaders headers;
    for (int i = 0; i < 120; ++i)
    {
        std::string link = "</asset-" + std::to_string(i) + ".css>; rel=preload";
        addLink(headers, link.c_str());
    }
    HttpRespHeaders hints;
    CHECK_EQUAL(120, headers.buildEarlyHints(hints));
    struct iovec links[120];
    CHECK_EQUAL(120, hints.getHeader(HttpRespHeaders::H_LINK, links, 120));
}


TEST(EarlyHints_http11KeepsContinueBeforeHints)
{
    NtwkIOLink stream;
    {
        HttpRespHeaders headers;
        headers.addStatusLine(HTTP_1_1, SC_100, 1);
        CHECK_EQUAL(LS_OK, stream.sendRespHeaders(&headers, SHF_MORE_HDRS));
    }
    {
        HttpRespHeaders hints;
        addLink(hints, "</style.css>; rel=preload");
        hints.addStatusLine(HTTP_1_1, SC_103, 1);
        CHECK_EQUAL(LS_OK, stream.sendRespHeaders(&hints, SHF_MORE_HDRS));
    }
    const std::string expected =
        "HTTP/1.1 100 Continue\r\n\r\n"
        "HTTP/1.1 103 Early Hints\r\n"
        "link: </style.css>; rel=preload\r\n\r\n";
    CHECK_EQUAL(expected, flatten(stream.testHeaderIov()));
    const AutoBuf &owned = stream.testPendingHeaders();
    CHECK_EQUAL(expected, std::string(owned.begin(), owned.size()));
    CHECK_EQUAL((const void *)owned.begin(), stream.testHeaderIov().get()[0].iov_base);
}


TEST(EarlyHints_sessionKeepsFinalResponseAndSendsOnce)
{
    HintTestStream stream;
    HttpSession session;
    session.attachStream(&stream);
    session.getReq()->setMethod(HttpMethod::HTTP_GET);
    session.getReq()->setStatusCode(SC_200);
    HttpRespHeaders &headers = session.getResp()->getRespHeaders();
    addLink(headers, "</style.css>; rel=preload");
    headers.add(HttpRespHeaders::H_SET_COOKIE, "private=secret", 14);
    CHECK_EQUAL(LS_OK, session.sendEarlyHints(headers));
    CHECK_EQUAL(1, stream.sent);
    CHECK_EQUAL(SC_103, stream.code);
    CHECK_EQUAL((int)SHF_MORE_HDRS, (int)stream.flags);
    CHECK(!session.isRespHeaderSent());
    CHECK_EQUAL(SC_200, session.getReq()->getStatusCode());
    CHECK(headers.isHeaderSet(HttpRespHeaders::H_LINK));
    CHECK(stream.wire.find("secret") == std::string::npos);
    CHECK(stream.wire.find("Content-Length") == std::string::npos);
    CHECK_EQUAL(LS_OK, session.sendEarlyHints(headers));
    CHECK_EQUAL(1, stream.sent);
    session.reset();
    session.getReq()->setStatusCode(SC_200);
    CHECK_EQUAL(LS_OK, session.sendEarlyHints(headers));
    CHECK_EQUAL(2, stream.sent);
}

TEST(EarlyHints_skipIneligibleRequests)
{
    HintTestStream stream;
    HttpSession session;
    session.attachStream(&stream);
    HttpRespHeaders headers;
    addLink(headers, "</style.css>; rel=preload");
    session.getReq()->setStatusCode(SC_200);
    session.getReq()->setMethod(HttpMethod::HTTP_POST);
    CHECK_EQUAL(LS_OK, session.sendEarlyHints(headers));
    session.getReq()->setMethod(HttpMethod::HTTP_GET);
    session.getReq()->setStatusCode(SC_404);
    CHECK_EQUAL(LS_OK, session.sendEarlyHints(headers));
    session.getReq()->setStatusCode(SC_200);
    session.setFlag(HSF_SUB_SESSION);
    CHECK_EQUAL(LS_OK, session.sendEarlyHints(headers));
    session.clearFlag(HSF_SUB_SESSION);
    session.setFlag(HSF_RESP_HEADER_SENT);
    CHECK_EQUAL(LS_OK, session.sendEarlyHints(headers));
    session.clearFlag(HSF_RESP_HEADER_SENT);
    stream.setProtocol(HIOS_PROTO_QUIC);
    CHECK_EQUAL(LS_OK, session.sendEarlyHints(headers));
    CHECK_EQUAL(0, stream.sent);
}

TEST(EarlyHints_http10DoesNotReceiveHints)
{
    HintConfigGuard guard;
    HttpServerConfig::getInstance().setEarlyHints(2);
    HintTestStream stream;
    stream.setProtocol(HIOS_PROTO_HTTP);
    HttpSession session;
    session.attachStream(&stream);
    const char request[] = "GET / HTTP/1.0\r\nHost: example.com\r\n\r\n";
    session.getReq()->setVHost((HttpVHost *)1);
    session.getReq()->getHeaderBuf().append(request, sizeof(request) - 1);
    CHECK_EQUAL(0, session.getReq()->processHeader());
    session.getReq()->setVHost(NULL);
    session.getReq()->setStatusCode(SC_200);
    HttpRespHeaders headers;
    addLink(headers, "</style.css>; rel=preload");
    CHECK_EQUAL(LS_OK, session.sendEarlyHints(headers));
    CHECK_EQUAL(0, stream.sent);
}

TEST(EarlyHints_failedSendDoesNotMarkResponseSent)
{
    HintTestStream stream;
    HttpSession session;
    session.attachStream(&stream);
    session.getReq()->setMethod(HttpMethod::HTTP_GET);
    session.getReq()->setStatusCode(SC_200);
    HttpRespHeaders headers;
    addLink(headers, "</style.css>; rel=preload");
    stream.fail = 1;
    CHECK_EQUAL(LS_FAIL, session.sendEarlyHints(headers));
    CHECK(!session.getFlag2(HSF2_EARLY_HINTS_SENT));
    CHECK(!session.isRespHeaderSent());
    stream.fail = 0;
    CHECK_EQUAL(LS_OK, session.sendEarlyHints(headers));
    CHECK_EQUAL(1, stream.sent);
}

TEST(EarlyHints_http11OwnsPendingInformationalHeaders)
{
    NtwkIOLink stream;
    {
        HttpRespHeaders hints;
        addLink(hints, "</style.css>; rel=preload");
        hints.addStatusLine(HTTP_1_1, SC_103, 1);
        CHECK_EQUAL(LS_OK, stream.sendRespHeaders(&hints, SHF_MORE_HDRS));
    }
    std::string pending = flatten(stream.testHeaderIov());
    CHECK(pending.find("103 Early Hints") != std::string::npos);
    HttpRespHeaders finalHeaders;
    finalHeaders.addStatusLine(HTTP_1_1, SC_200, 1);
    finalHeaders.add(HttpRespHeaders::H_CONTENT_LENGTH, "0", 1);
    CHECK_EQUAL(LS_OK, stream.sendRespHeaders(&finalHeaders, SHF_EOS));
    std::string combined = flatten(stream.testHeaderIov());
    const AutoBuf &owned = stream.testPendingHeaders();
    CHECK_EQUAL(combined, std::string(owned.begin(), owned.size()));
    CHECK_EQUAL((const void *)owned.begin(), stream.testHeaderIov().get()[0].iov_base);
    CHECK_EQUAL(pending, combined.substr(0, pending.size()));
    CHECK(combined.find("HTTP/1.1 200 OK", pending.size()) == pending.size());
    CHECK(combined.find("content-length: 0", pending.size()) != std::string::npos);
}

TEST(EarlyHints_configuredLinkOpsHonorSetAndUnset)
{
    HttpSession session;
    HttpContext context;
    const char ops[] = "add Link </one.css>; rel=preload\n"
                       "set Link </two.css>; rel=preload\n"
                       "add X-Private secret\n";
    CHECK_EQUAL(0, context.setHeaderOps("test", ops, sizeof(ops) - 1));
    session.getReq()->setContext(&context);
    HttpRespHeaders headers;
    session.getReq()->applyHeaderOps(&session, &headers, true);
    HttpRespHeaders hints;
    CHECK_EQUAL(1, headers.buildEarlyHints(hints));
    int len;
    const char *value = hints.getHeader(HttpRespHeaders::H_LINK, &len);
    CHECK_EQUAL("</two.css>; rel=preload", std::string(value, len).c_str());
    CHECK_EQUAL(1, headers.getUniqueCnt());
    const char unset[] = "unset Link";
    CHECK_EQUAL(0, context.setHeaderOps("test", unset, sizeof(unset) - 1));
    session.getReq()->applyHeaderOps(&session, &headers, true);
    hints.reset();
    CHECK_EQUAL(0, headers.buildEarlyHints(hints));
}

TEST(EarlyHints_deferConfiguredBatchWithDynamicOverride)
{
    HttpSession session;
    HttpContext context;
    const char ops[] = "add Link </old.css>; rel=preload\n"
                       "set Link </%{ENV:ASSET}>; rel=preload\n";
    CHECK_EQUAL(0, context.setHeaderOps("test", ops, sizeof(ops) - 1));
    session.getReq()->setContext(&context);
    HttpRespHeaders headers;
    CHECK_EQUAL(LS_AGAIN,
                session.getReq()->applyHeaderOps(&session, &headers, true));
}

TEST(EarlyHints_applicationLinksHonorConfiguredUnset)
{
    HintTestStream stream;
    HttpSession session;
    HttpContext context;
    const char ops[] = "unset Link";
    CHECK_EQUAL(0, context.setHeaderOps("test", ops, sizeof(ops) - 1));
    session.attachStream(&stream);
    session.getReq()->setContext(&context);
    session.getReq()->setMethod(HttpMethod::HTTP_HEAD);
    session.getReq()->setStatusCode(SC_200);
    addLink(session.getResp()->getRespHeaders(), "</private.css>; rel=preload");
    CHECK_EQUAL(LS_OK, session.respHeaderDone());
    CHECK_EQUAL(0, stream.sent);
    CHECK_EQUAL(LS_OK, session.sendRespHeaders());
    CHECK_EQUAL(1, stream.sent);
    CHECK(stream.wire.find("103 Early Hints") == std::string::npos);
    CHECK(stream.wire.find("private.css") == std::string::npos);
}

TEST(EarlyHints_responseHeadersDoNotGenerateLateHints)
{
    HintTestStream stream;
    HttpSession session;
    HttpContext context;
    const char ops[] = "add Link </configured.css>; rel=preload";
    CHECK_EQUAL(0, context.setHeaderOps("test", ops, sizeof(ops) - 1));
    session.attachStream(&stream);
    session.getReq()->setContext(&context);
    session.getReq()->setMethod(HttpMethod::HTTP_HEAD);
    session.getReq()->setStatusCode(SC_200);
    HttpRespHeaders &headers = session.getResp()->getRespHeaders();
    addLink(headers, "</app.css>; rel=preload");
    CHECK_EQUAL(LS_OK, session.respHeaderDone());
    CHECK_EQUAL(0, stream.sent);
    struct iovec links[3];
    CHECK_EQUAL(1, headers.getHeader(HttpRespHeaders::H_LINK, links, 3));
    CHECK_EQUAL(LS_OK, session.sendRespHeaders());
    CHECK_EQUAL(1, stream.sent);
    CHECK_EQUAL(SC_200, stream.code);
    CHECK(stream.wire.find("103 Early Hints") == std::string::npos);
    CHECK_EQUAL(2, headers.getHeader(HttpRespHeaders::H_LINK, links, 3));
}

static int addLateHint(lsi_param_t *param)
{
    HttpSession *session = (HttpSession *)param->session;
    addLink(session->getResp()->getRespHeaders(), "</late.css>; rel=preload");
    return LS_OK;
}

TEST(EarlyHints_lateHookLinksRemainInFinalResponse)
{
    lsi_module_t module = {};
    LsiApiHooks &hooks = LsiApiHooks::s_hooks[LSI_HKPT_SEND_RESP_HEADER];
    CHECK(hooks.add(&module, addLateHint, 0, LSI_FLAG_ENABLED) >= 0);
    {
        HintTestStream stream;
        HttpSession session;
        session.attachStream(&stream);
        session.getReq()->setMethod(HttpMethod::HTTP_HEAD);
        session.getReq()->setStatusCode(SC_200);
        session.getSessionHooks()->inherit(NULL, 1);
        CHECK_EQUAL(LS_OK, session.sendRespHeaders());
        CHECK_EQUAL(1, stream.sent);
        CHECK_EQUAL(SC_200, stream.code);
        CHECK(stream.wire.find("103 Early Hints") == std::string::npos);
        CHECK(stream.wire.find("</late.css>; rel=preload") != std::string::npos);
        CHECK(session.isRespHeaderSent());
    }
    hooks.remove(&module);
}

#ifdef OLS_TEST_QUIC_HEADERS
static bool s_failHintBuild;
extern "C" int __real__ZN15HttpRespHeaders15buildEarlyHintsERS_(
    HttpRespHeaders *, HttpRespHeaders *);
extern "C" int __wrap__ZN15HttpRespHeaders15buildEarlyHintsERS_(
    HttpRespHeaders *headers, HttpRespHeaders *hints)
{
    return s_failHintBuild ? LS_FAIL
        : __real__ZN15HttpRespHeaders15buildEarlyHintsERS_(headers, hints);
}

TEST(EarlyHints_buildFailureContinuesToFinalResponse)
{
    HintTestStream stream;
    HttpSession session;
    session.attachStream(&stream);
    session.setState(HSS_PROCESSING);
    session.getReq()->setMethod(HttpMethod::HTTP_HEAD);
    session.getReq()->setStatusCode(SC_200);
    HttpRespHeaders &headers = session.getResp()->getRespHeaders();
    addLink(headers, "</style.css>; rel=preload");
    s_failHintBuild = true;
    int result = session.sendEarlyHints(headers);
    s_failHintBuild = false;
    CHECK_EQUAL(LS_OK, result);
    CHECK_EQUAL(HSS_PROCESSING, session.getState());
    CHECK_EQUAL(0, stream.sent);
    CHECK(!session.getFlag2(HSF2_EARLY_HINTS_SENT));
    CHECK_EQUAL(LS_OK, session.respHeaderDone());
    CHECK_EQUAL(LS_OK, session.sendRespHeaders());
    CHECK_EQUAL(1, stream.sent);
    CHECK_EQUAL(SC_200, stream.code);
}

#include <quic/quicstream.h>
#include <lsquic.h>
#include <errno.h>
#include <vector>

static bool s_quicBlocked;
static bool s_quicHeaderError;
static int s_quicWantWrite;
static int s_quicFin;
static std::vector<std::string> s_quicStatus;

extern "C" int __wrap_lsquic_stream_send_headers(
    lsquic_stream_t *, const lsquic_http_headers_t *headers, int)
{
    if (s_quicHeaderError)
        return LS_FAIL;
    if (s_quicBlocked)
    {
        errno = EAGAIN;
        return LS_FAIL;
    }
    const lsxpack_header *status = headers->headers;
    s_quicStatus.push_back(std::string(lsxpack_header_get_value(status),
                                       status->val_len));
    //Simulate a header block still waiting for transport/encoder credit.
    s_quicBlocked = true;
    return LS_OK;
}
extern "C" int __wrap_lsquic_stream_wantwrite(lsquic_stream_t *, int want)
{
    s_quicWantWrite = want;
    return 0;
}
extern "C" int __wrap_lsquic_stream_flush(lsquic_stream_t *) { return 0; }
extern "C" int __wrap_lsquic_stream_shutdown(lsquic_stream_t *, int)
{
    ++s_quicFin;
    return 0;
}

class HintTestHandler : public HioHandler
{
public:
    int writes;
    HintTestHandler() : writes(0) {}
    int onInitConnected() { return 0; }
    int onReadEx() { return 0; }
    int onWriteEx() { ++writes; return 0; }
    int onCloseEx() { return 0; }
    int onTimerEx() { return 0; }
    void recycle() {}
};

TEST(EarlyHints_http3DoesNotRetryPermanentHeaderErrors)
{
    QuicStream stream;
    stream.setQuicStream((lsquic_stream_t *)1);
    stream.setProtocol(HIOS_PROTO_HTTP3);
    HttpRespHeaders hints;
    hints.addStatusLine(HTTP_1_1, SC_103, 1);
    s_quicHeaderError = true;
    //Some LSQUIC encoding errors leave errno unchanged.
    errno = EAGAIN;
    CHECK_EQUAL(LS_FAIL, stream.sendRespHeaders(&hints, SHF_MORE_HDRS));
    s_quicHeaderError = false;
    stream.setQuicStream(NULL);
}

TEST(EarlyHints_http3RestoresApplicationWriteInterest)
{
    s_quicBlocked = false;
    s_quicFin = s_quicWantWrite = 0;
    s_quicStatus.clear();
    QuicStream stream;
    stream.setQuicStream((lsquic_stream_t *)1);
    stream.setProtocol(HIOS_PROTO_HTTP3);
    stream.setState(HIOS_CONNECTED);
    HintTestHandler handler;
    handler.attachStream(&stream);
    HttpRespHeaders hints;
    addLink(hints, "</style.css>; rel=preload");
    hints.addStatusLine(HTTP_1_1, SC_103, 1);
    CHECK_EQUAL(LS_OK, stream.sendRespHeaders(&hints, SHF_MORE_HDRS));
    stream.suspendWrite();
    CHECK_EQUAL(1, s_quicWantWrite);
    s_quicBlocked = false;
    CHECK_EQUAL(LS_OK, stream.onWrite());
    CHECK_EQUAL(0, s_quicWantWrite);
    CHECK_EQUAL(0, handler.writes);
    stream.continueWrite();
    CHECK_EQUAL(LS_OK, stream.onWrite());
    CHECK_EQUAL(1, s_quicWantWrite);
    CHECK_EQUAL(1, handler.writes);
    handler.detachStream();
    stream.setQuicStream(NULL);
}

TEST(EarlyHints_http3QueuesFinalHeadersBeforeFin)
{
    s_quicBlocked = false;
    s_quicFin = s_quicWantWrite = 0;
    s_quicStatus.clear();
    QuicStream stream;
    stream.setQuicStream((lsquic_stream_t *)1);
    stream.setProtocol(HIOS_PROTO_HTTP3);
    stream.setState(HIOS_CONNECTED);
    {
        HttpRespHeaders hints;
        addLink(hints, "</style.css>; rel=preload");
        hints.addStatusLine(HTTP_1_1, SC_103, 1);
        CHECK_EQUAL(LS_OK, stream.sendRespHeaders(&hints, SHF_MORE_HDRS));
        HttpRespHeaders finalHeaders;
        finalHeaders.addStatusLine(HTTP_1_1, SC_200, 1);
        CHECK_EQUAL(LS_OK, stream.sendRespHeaders(&finalHeaders, SHF_EOS));
    }
    CHECK_EQUAL(1U, s_quicStatus.size());
    CHECK_EQUAL("103", s_quicStatus[0].c_str());
    CHECK_EQUAL(0, stream.write("body", 4));
    CHECK_EQUAL(LS_OK, stream.doneWrite());
    stream.suspendWrite();
    CHECK_EQUAL(1, s_quicWantWrite);
    CHECK_EQUAL(0, s_quicFin);
    CHECK_EQUAL(LS_OK, stream.onWrite());
    CHECK_EQUAL(1U, s_quicStatus.size());
    s_quicBlocked = false;
    CHECK_EQUAL(LS_OK, stream.onWrite());
    CHECK_EQUAL(2U, s_quicStatus.size());
    CHECK_EQUAL("200", s_quicStatus[1].c_str());
    CHECK_EQUAL(0, s_quicFin);
    CHECK_EQUAL(0, stream.write("body", 4));
    s_quicBlocked = false;
    CHECK_EQUAL(LS_OK, stream.onWrite());
    CHECK_EQUAL(1, s_quicFin);
    stream.setQuicStream(NULL);
}

TEST(EarlyHints_http3OwnsQueuedHints)
{
    s_quicBlocked = true;
    s_quicFin = s_quicWantWrite = 0;
    s_quicStatus.clear();
    QuicStream stream;
    stream.setQuicStream((lsquic_stream_t *)1);
    stream.setProtocol(HIOS_PROTO_HTTP3);
    stream.setState(HIOS_CONNECTED);
    {
        HttpRespHeaders hints;
        addLink(hints, "</style.css>; rel=preload");
        hints.addStatusLine(HTTP_1_1, SC_103, 1);
        CHECK_EQUAL(LS_OK, stream.sendRespHeaders(&hints, SHF_MORE_HDRS));
    }
    CHECK_EQUAL(LS_AGAIN, stream.flush());
    CHECK(s_quicStatus.empty());
    s_quicBlocked = false;
    CHECK_EQUAL(LS_OK, stream.onWrite());
    CHECK_EQUAL(1U, s_quicStatus.size());
    CHECK_EQUAL("103", s_quicStatus[0].c_str());
    stream.setQuicStream(NULL);
}
#endif

#endif
