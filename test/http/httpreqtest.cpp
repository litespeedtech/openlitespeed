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

#include "httpreqtest.h"

#include <http/httpdefs.h>
#include <http/handlertype.h>
#include <http/httphandler.h>
#include <http/httpreq.h>
#include <http/httpmethod.h>
#include <http/httpserverconfig.h>
#include <http/httpvhost.h>
#include <h2/unpackedheaders.h>
#include <lshpack/lshpack.h>
#include <log4cxx/logsession.h>
#include <stdio.h>
#include "unittest-cpp/UnitTest++.h"

#include <http/httpstatuscode.h>

class HttpReqTst : public HttpReq, public LogSession
{
public:
    HttpReqTst()
        : HttpReq()
    {  getLogId();  }
    const char *getLogId()     {   return LogSession::getLogId();  }
    virtual const char *buildLogId() {
        appendLogId("HttpReqTest", true);
        return m_logId.ptr;
    }

    int append(const char *pBuf, int size)
    {   return HttpReq::appendTestHeaderData(pBuf, size); }
};


class ProxyContextTestHandler : public HttpHandler
{
public:
    ProxyContextTestHandler()
        : HttpHandler(HandlerType::HT_PROXY)
    {}

    const char *getName() const   {   return "proxy-context-test";   }
};


static lsxpack_err_code addUnpackedReqHeader(UpkdHdrBuilder &builder,
        const char *name, unsigned nameLen, const char *value,
        unsigned valueLen, int hpackIndex = LSHPACK_HDR_UNKNOWN)
{
    lsxpack_header *header = builder.prepareDecode(NULL,
                              nameLen + valueLen + 4);
    if (header == NULL)
        return LSXPACK_ERR_NOMEM;
    char *buf = header->buf;
    unsigned offset = header->name_offset;
    memcpy(buf + offset, name, nameLen);
    memcpy(buf + offset + nameLen, ": ", 2);
    memcpy(buf + offset + nameLen + 2, value, valueLen);
    memcpy(buf + offset + nameLen + 2 + valueLen, "\r\n", 2);
    lsxpack_header_set_offset2(header, buf, offset, nameLen,
                               offset + nameLen + 2, valueLen);
    header->hpack_index = hpackIndex;
    return builder.process(header);
}
SUITE(HttpReqTest)
{
    TEST(HttpReqTest_testFragment)
    {
        const char pFrag1[] =
            "POST /serv_admin/wizControl.php HTTP/1.1\r\n"
            "Host: 192.168.0.10:8083\r\n"
            "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.0.1) Gecko/20020830\r\n"
            "Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,video/x-mng,image/png,image/jpeg,image/gif;q=0.2,text/css,*/*;q=0.1\r\n"
            "Accept-Language: en-us, en;q=0.50\r\n"
            "Accept-Encoding: gzip, deflate, compress;q=0.9\r\n"
            "Accept-Charset: ISO-8859-1, utf-8;q=0.66, *;q=0.66\r\n"
            "Keep-Alive: 300\r\n"
            "Connection: keep-alive\r\n"
            "Referer: http://192.168.0.10:8083/serv_admin/servGeneral.php?id=1\r\n"
            "Cookie: PHPSESSID=386791a581cda4e5f5c6b4f960f19c33\r\n";

        const char pFrag2[] =
            "Content-Type: application/x-www-form-urlencoded\r\n"
            "Content-Length: 104\r\n"
            "\r\n"
            "parentWin=YB_conf&objName=ESgeneral&objId=&finishPage=servGeneral.php%3Fid%3D1&nextPage=servGeneral1.php\r\n";
        HttpReqTst req;
        req.appendLogId("testFragment");
        req.reset(0);
        req.setVHost((HttpVHost *)
                     1);    //just skip vhost lookup while parsing header
        CHECK(1 == req.append(pFrag1, strlen(pFrag1)));
        CHECK(0 == req.append(pFrag2, strlen(pFrag2)));
        CHECK(req.getOrgReqURLLen() == 26);
    }

    TEST(HttpReqTest_testParseHeader)
    {

        const char *pSample[] =
        {
            "host", "www.example.com:3080",
            "user-agent", "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:0.9.2.1) Gecko/20010901",
            "accept", "text/xml, application/xml, application/xhtml+xml, text/html;q=0.9, image/png, image/jpeg, image/gif;q=0.2, text/plain;q=0.8, text/css",
            "accept-language", "en-us",
            "accept-encoding", "deflate,gzip,compress,identity",
            "accept-charset", "ISO-8859-1, utf-8;q=0.66, *;q=0.66",
            "content-type", "text/html",
            "keep-alive", "300",
            "connection", "keep-alive",
            "range", "bytes=10-20"
        };
        const char *pURI = "/path/path1/path3 path4";
        const char *pArg = "a=b%20c";
        const char *pInput =
            "GET http://WWW.eXample.cOm:3080///path//./path1/path2//..////path3%20path4?a=b%20c HTTP/1.0\r\n"
            "Host: www.example.com:3080\r\n"
            "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:0.9.2.1) Gecko/20010901\r\n"
            "Accept: text/xml, application/xml, application/xhtml+xml, text/html;q=0.9, image/png, image/jpeg, image/gif;q=0.2, text/plain;q=0.8, text/css\r\n"
            "Accept-Language: en-us\r\n"
            "Accept-Encoding: deflate,gzip,compress,identity\r\n"
            "Accept-Charset: ISO-8859-1, utf-8;q=0.66, *;q=0.66\r\n"
            "Content-Type: text/html\t \r\n"
            "Keep-Alive: 300 \r\n"
            "Connection: keep-alive\r\n"
            "Range: bytes=10-20\r\n"
            "Non-Standard-Header: header1\r\n"
            "Non-Standard-Header2: line2\r\n"
            "Non-Standard-Header3: line3\r\n"
            "\r\n";
        HttpReqTst req;
        req.appendLogId("testParseHeader");
        req.reset(0);
        req.setVHost((HttpVHost *)
                     1);    //just skip vhost look while parsing header

        int len = strlen(pInput);
        int i;
        for (i = 0; i < len; ++i)
        {
            int ret = req.append(pInput + i, 1);
            if ((ret != 1) && (ret != 0))
                printf("test error: ret=%d, i=%d, bufleft=%s\n", ret, i, pInput + i);
            CHECK((ret == 1) || (ret == 0));
        }
        int status = req.getStatus();
        CHECK(HttpReq::HEADER_OK == status);
        CHECK(HttpMethod::HTTP_GET == req.getMethod());
        CHECK(strcmp(req.getURI(), pURI) == 0);
        CHECK(strncmp(req.getQueryString(), pArg, strlen(pArg)) == 0);
        CHECK(req.getOrgReqURLLen() == 51);
        for (int i = 0; i < (int)(sizeof(pSample) / sizeof(char *)); i += 2)
        {
            int index = HttpHeader::getIndex(pSample[i], strlen(pSample[i]));
            const char *pHeader = req.getHeader(index);

            if (!*pHeader)
                printf("i=%d\n", i);
            CHECK(*pHeader);
            CHECK(0 ==
                  strncmp(pHeader,
                          pSample[i + 1],
                          strlen(pSample[i + 1])));
        }

        const char *pHost = "www.example.com";
        int hl = req.getHostStrLen();
        CHECK(hl == (int)strlen(pHost));

        CHECK(strncmp(req.getHostStr(), pHost, strlen(pHost)) == 0);
        req.setVHost(NULL);
        CHECK(req.gzipAcceptable());
        CHECK(req.isKeepAlive());
        const char *pNSHKey = "non-standard-header";
        const char *pNSHValue = "header1";
        const char *pParsed = req.getHeader(pNSHKey, 19, len);
        CHECK(NULL != pParsed);
        CHECK(0 ==
              strncmp(pParsed,
                      pNSHValue,
                      strlen(pNSHValue)));
    }

    TEST(HttpReqTest_testParseHeader1)
    {

        const char *pSample[] =
        {
            "host", "www.example.com:3080",
            "user-agent", "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:0.9.2.1) Gecko/20010901",
            "accept", "text/xml, application/xml, application/xhtml+xml, text/html;q=0.9, image/png, image/jpeg, image/gif;q=0.2, text/plain;q=0.8, text/css",
            "accept-language", "en-us",
            "accept-encoding", "deflate,compress,identity",
            "accept-charset", "ISO-8859-1, utf-8;q=0.66, *;q=0.66",
            "content-type", "text/html",
            "content-length", "1234",
            "keep-alive", "300",
            "connection", "close"
        };

        //const char * pURI = "/path/path1/path3 path4";
        const char *pInput =
            "POST ///path//path1////path3%20path4/../../../ HTTP/1.1\r\n"
            "Host: www.example.com:3080\r\n"
            "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:0.9.2.1) Gecko/20010901\r\n"
            "Accept: text/xml, application/xml, application/xhtml+xml, text/html;q=0.9, image/png, image/jpeg, image/gif;q=0.2, text/plain;q=0.8, text/css\r\n"
            "Accept-Language: en-us\r\n"
            "Accept-Encoding: deflate,compress,identity\r\n"
            "Accept-Charset: ISO-8859-1, utf-8;q=0.66, *;q=0.66\r\n"
            "Content-Type: text/html\t \r\n"
            "Content-length: 1234 \r\n"
            "Keep-Alive: 300 \r\n"
            "Connection: close\r\n"
            "\r\n";
        HttpReqTst req;
        req.appendLogId("testParseHeader1");
        req.reset(0);
        req.setVHost((HttpVHost *)
                     1);    //just skip vhost look while parsing header

        int len = strlen(pInput);
        int i;
        for (i = 0; i < len; ++i)
        {
            int ret = req.append(pInput + i, 1);
            if ((ret != 1) && (ret != 0))
                printf("test error: ret=%d, i=%d, bufleft=%s\n", ret, i, pInput + i);
            CHECK((ret == 1) || (ret == 0));
        }
        int status = req.getStatus();
        CHECK(HttpReq::HEADER_OK == status);
        CHECK(HttpMethod::HTTP_POST == req.getMethod());
        CHECK(strcmp(req.getURI(), "/") == 0);
        CHECK(req.getQueryStringLen() == 0);
        CHECK(req.getOrgReqURLLen() == 41);
        for (int i = 0; i < (int)(sizeof(pSample) / sizeof(char *)); i += 2)
        {
            int index = HttpHeader::getIndex(pSample[i], strlen(pSample[i]));
            const char *pHeader = req.getHeader(index);

            if (!*pHeader)
                printf("i=%d\n", i);
            CHECK(*pHeader);
            int cmp = strncmp(pHeader,
                              pSample[i + 1],
                              strlen(pSample[i + 1]));
            if (cmp)
                printf("i=%d\n", i);
            CHECK(0 == cmp);
        }

        const char *pHost = "www.example.com";
        int hl = req.getHostStrLen();
        CHECK(hl == (int)strlen(pHost));
        CHECK(strncmp(req.getHostStr(), pHost, strlen(pHost)) == 0);
        req.setVHost(NULL);
        CHECK(req.getContentLength() == 1234);
        CHECK(!req.gzipAcceptable());
        CHECK(!req.isKeepAlive());
        //const char * pNSHKey = "non-standard-header";
        //const char * pNSHValue = "header1    line2  \t \tline3";
        //const char * pParsed = req.getHeader( pNSHKey );
        //CHECK( NULL != pParsed );
        //printf( "%s\n" ,  pParsed );
        //CHECK( 0 ==
        //        strncmp( pParsed,
        //                pNSHValue,
        //                strlen( pNSHValue ) ) );
    }

    TEST(HttpReqTest_testParseHeader2)
    {
        const char pInput[] =
        {
            "GET / HTTP/1.1\r\n"
            "Host: www.epochtimes.com\r\n"
            "User-Agent: productfinderbot\r\n"
            "From: \r\nReferer: \r\nAccept-Encoding: \r\n"
            "Customize: Customized\r\n"
            "Connection: close\r\n\r\n"
        };
        HttpReqTst req;
        req.appendLogId("testParseHeader1");
        req.reset(0);
        req.setVHost((HttpVHost *)
                     1);    //just skip vhost look while parsing header

        int len = strlen(pInput);
        int i;
        for (i = 0; i < len; ++i)
        {
            int ret = req.append(pInput + i, 1);
            if ((ret != 1) && (ret != 0))
                printf("test error: ret=%d, i=%d, bufleft=%s\n", ret, i, pInput + i);
            CHECK((ret == 1) || (ret == 0));
        }
        int status = req.getStatus();
        CHECK(HttpReq::HEADER_OK == status);
        CHECK(HttpMethod::HTTP_GET == req.getMethod());
        int index = HttpHeader::getIndex("Referer", strlen("Referer"));
        const char *pHeader = req.getHeader(index);
        CHECK(*pHeader);
        CHECK(req.getHeaderLen(index) == 0);
        CHECK(req.getOrgReqURLLen() == 1);
        index = HttpHeader::getIndex("Accept-Encoding", strlen("Accept-Encoding"));
        pHeader = req.getHeader(index);
        CHECK(*pHeader);
        CHECK(req.getHeaderLen(index) == 0);
        int unknown = req.getUnknownHeaderCount();
        CHECK(unknown == 2);
        const char *pKey;
        const char *pVal;
        int keyLen;
        int valLen;
        pKey = req.getUnknownHeaderByIndex(0, keyLen, pVal, valLen);
        CHECK(keyLen == 4);
        CHECK(valLen == 0);
        CHECK(strncasecmp(pKey, "from", 4) == 0);
        CHECK(pVal > req.getHeaderBuf().begin());
        CHECK(pVal < req.getHeaderBuf().end());

        pKey = req.getUnknownHeaderByIndex(1, keyLen, pVal, valLen);
        CHECK(keyLen == 9);
        CHECK(valLen == 10);
        CHECK(strncasecmp(pKey, "customize", 9) == 0);
        CHECK(strncmp(pVal, "Customized", 9) == 0);
        CHECK(pVal > req.getHeaderBuf().begin());
        CHECK(pVal < req.getHeaderBuf().end());

    }

    TEST(HttpReqTest_rejectWhitespaceBeforeHeaderColon)
    {
        const char pInput[] =
        {
            "GET / HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "X-Bad : value\r\n"
            "\r\n"
        };
        HttpReqTst req;
        req.appendLogId("rejectWhitespaceBeforeHeaderColon");
        req.reset(0);
        req.setVHost((HttpVHost *)1);

        int len = strlen(pInput);
        int ret = 1;
        for (int i = 0; i < len; ++i)
        {
            ret = req.append(pInput + i, 1);
            if (ret == SC_400)
                break;
        }
        CHECK(ret == SC_400);
    }

    TEST(HttpReqTest_rejectNulByteInURI)
    {
        const char pInput[] =
        {
            "GET /a\0b HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "\r\n"
        };
        HttpReqTst req;
        req.appendLogId("rejectNulByteInURI");
        req.reset(0);
        req.setVHost((HttpVHost *)1);

        int len = sizeof(pInput) - 1;
        int ret = 1;
        for (int i = 0; i < len; ++i)
        {
            ret = req.append(pInput + i, 1);
            if (ret == SC_400)
                break;
        }
        CHECK(ret == SC_400);
    }

    TEST(HttpReqTest_rejectBareLfLineEndings)
    {
        const char *pInputs[] =
        {
            "GET / HTTP/1.1\nHost: www.example.com\r\n\r\n",
            "GET / HTTP/1.1\r\nHost: www.example.com\n\r\n",
            "GET / HTTP/1.1\r\nHost: www.example.com\r\n\n"
        };

        for (unsigned int i = 0; i < sizeof(pInputs) / sizeof(pInputs[0]); ++i)
        {
            HttpReqTst req;
            req.appendLogId("rejectBareLfLineEndings");
            req.reset(0);
            req.setVHost((HttpVHost *)1);
            CHECK(SC_400 == req.append(pInputs[i], strlen(pInputs[i])));
        }
    }

    TEST(HttpReqTest_rejectPercentEncodedNulByteInURI)
    {
        const char *pInput =
            "GET /a%00b HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "\r\n";
        HttpReqTst req;
        req.appendLogId("rejectPercentEncodedNulByteInURI");
        req.reset(0);
        req.setVHost((HttpVHost *)1);

        int len = strlen(pInput);
        int ret = 1;
        for (int i = 0; i < len; ++i)
        {
            ret = req.append(pInput + i, 1);
            if (ret == SC_400)
                break;
        }
        CHECK(ret == SC_400);
    }

    TEST(HttpReqTest_testParseWhitespaceCookie)
    {
        const char pInput[] =
        {
            "GET / HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "Cookie: empty=   ; good=value\r\n"
            "Connection: close\r\n\r\n"
        };
        HttpReqTst req;
        req.appendLogId("testParseWhitespaceCookie");
        req.reset(0);
        req.setVHost((HttpVHost *)1);

        CHECK(0 == req.append(pInput, strlen(pInput)));
        CHECK(HttpReq::HEADER_OK == req.getStatus());

        cookieval_t *pCookie = req.getCookie("empty", 5);
        CHECK(pCookie != NULL);
        if (pCookie)
            CHECK(pCookie->valLen == 0);

        pCookie = req.getCookie("good", 4);
        CHECK(pCookie != NULL);
        if (pCookie)
        {
            CHECK(pCookie->valLen == 5);
            CHECK(strncmp(req.getHeaderBuf().getp(pCookie->valOff),
                          "value", 5) == 0);
        }
    }

    TEST(HttpReqTest_testParseHeader3)
    {
        const char pInput[] =
        {
            "GET /asf/../../../ HTTP/1.1\r\n"
            "Host: www.epochtimes.com\r\n"
            "User-Agent: productfinderbot\r\n"
            "From: \r\nReferer: \r\nAccept-Encoding: \r\n"
            "Customize : \r\n"
            "   Customized\r\n"
            "Connection: close\r\n\r\n"
        };
        HttpReqTst req;
        req.appendLogId("testParseHeader1");
        req.reset(0);
        req.setVHost((HttpVHost *)
                     1);    //just skip vhost look while parsing header

        int len = strlen(pInput);
        int i;
        int ret;
        for (i = 0; i < len; ++i)
        {
            ret = req.append(pInput + i, 1);
            if (ret == SC_400)
                break;
            if ((ret != 1) && (ret != 0))
                printf("test error: ret=%d, i=%d, bufleft=%s\n", ret, i, pInput + i);
            CHECK((ret == 1) || (ret == 0));
        }
        CHECK(ret == SC_400);

    }

    TEST(HttpReqTest_setRewriteURITruncatesLong)
    {
        const int longLen = MAX_URL_LEN + 64;
        char *pURI = new char[longLen + 1];
        memset(pURI, 'a', longLen);
        pURI[0] = '/';
        pURI[longLen] = 0;

        HttpReqTst req;
        req.reset(0);
        CHECK(0 == req.setRewriteURI(pURI, longLen));
        CHECK(MAX_URL_LEN == req.getURILen());
        CHECK('/' == req.getURI()[0]);
        CHECK(0 == req.getURI()[MAX_URL_LEN]);

        delete []pURI;
    }

    TEST(HttpReqTest_setRewriteLocationTruncatesLongNoEscape)
    {
        const int longLen = MAX_URL_LEN + 64;
        char *pURI = new char[longLen + 1];
        memset(pURI, 'a', longLen);
        pURI[0] = '/';
        pURI[longLen] = 0;

        HttpReqTst req;
        req.reset(0);
        CHECK(0 == req.setRewriteLocation(pURI, longLen, "a=b", 3, 0));
        CHECK(req.getLocationLen() <= MAX_URL_LEN + 2);
        CHECK('/' == req.getLocation()[0]);
        CHECK(0 == req.getLocation()[req.getLocationLen()]);

        delete []pURI;
    }

    static int countHeaderName(HttpReq &req, const char *pName, int nameLen)
    {
        const char *p = req.getOrgReqLine();
        const char *pEnd = p + req.getHttpHeaderLen();
        int count = 0;
        while ((p = (const char *)memchr(p, '\n', pEnd - p)) != NULL)
        {
            ++p;
            if (pEnd - p > nameLen && *(p + nameLen) == ':'
                && strncasecmp(p, pName, nameLen) == 0)
                ++count;
        }
        return count;
    }

    TEST(HttpReqTest_setUnsetUnknownReqHeader)
    {
        const char *pInput =
            "GET /api/ HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "Origin: https://client.example\r\n"
            "Connection: close\r\n"
            "\r\n";
        HttpReqTst req;
        req.appendLogId("setUnsetUnknownReqHeader");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK(0 == req.append(pInput, strlen(pInput)));
        CHECK(1 == countHeaderName(req, "Origin", 6));

        HttpHeaderOps ops;
        const char *pVal = "https://proxy.example";
        HeaderOp *pSet = ops.append(HttpHeader::getIndex("Origin", 6),
                                    "Origin", 6, pVal, strlen(pVal),
                                    LSI_HEADER_SET, 1);
        req.applyOp(NULL, pSet);
        //"set" must replace the client's header, not add a second one
        CHECK(1 == countHeaderName(req, "Origin", 6));
        req.applyOp(NULL, pSet);
        CHECK(1 == countHeaderName(req, "Origin", 6));
        int valLen = 0;
        const char *pParsed = req.getHeader("origin", 6, valLen);
        CHECK(NULL != pParsed);
        CHECK(valLen == (int)strlen(pVal));
        CHECK(0 == strncmp(pParsed, pVal, valLen));

        HeaderOp *pUnset = ops.append(HttpHeader::getIndex("Origin", 6),
                                      "Origin", 6, "", 0,
                                      LSI_HEADER_UNSET, 1);
        req.applyOp(NULL, pUnset);
        CHECK(0 == countHeaderName(req, "Origin", 6));
        CHECK(NULL == req.getHeader("origin", 6, valLen));
    }

    TEST(HttpReqTest_rejectTransferEncodingWithContentLengthInEitherOrder)
    {
        const char *requests[] =
        {
            "POST / HTTP/1.1\r\nHost: example.com\r\n"
            "Content-Length: 0\r\nTransfer-Encoding: chunked\r\n\r\n",
            "POST / HTTP/1.1\r\nHost: example.com\r\n"
            "Transfer-Encoding: chunked\r\nContent-Length: 0\r\n\r\n"
        };
        for (unsigned i = 0; i < sizeof(requests) / sizeof(requests[0]); ++i)
        {
            HttpReqTst req;
            req.appendLogId("rejectAmbiguousRequestFraming");
            req.reset(0);
            req.setVHost((HttpVHost *)1);
            CHECK_EQUAL(SC_400, req.append(requests[i], strlen(requests[i])));
        }
    }

    TEST(HttpReqTest_rejectDuplicateTransferEncoding)
    {
        const char request[] =
            "POST / HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Transfer-Encoding: chunked\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n";
        HttpReqTst req;
        req.appendLogId("rejectDuplicateTransferEncoding");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(SC_400, req.append(request, sizeof(request) - 1));
    }

    TEST(HttpReqTest_rejectDuplicateHost)
    {
        const char *requests[] =
        {
            "GET / HTTP/1.1\r\nHost: first.example\r\n"
            "Host: second.example\r\n\r\n",
            "GET / HTTP/1.1\r\nHost: same.example\r\n"
            "Host: same.example\r\n\r\n"
        };
        for (unsigned i = 0; i < sizeof(requests) / sizeof(requests[0]); ++i)
        {
            HttpReqTst req;
            req.appendLogId("rejectDuplicateHost");
            req.reset(0);
            req.setVHost((HttpVHost *)1);
            CHECK_EQUAL(SC_400, req.append(requests[i], strlen(requests[i])));
        }
    }

    TEST(HttpReqTest_requireHostForHttp11)
    {
        const char *invalid[] =
        {
            "GET / HTTP/1.1\r\n\r\n",
            "GET http://example.com/path HTTP/1.1\r\n\r\n"
        };
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        {
            HttpReqTst req;
            req.appendLogId("requireHostForHttp11");
            req.reset(0);
            req.setVHost((HttpVHost *)1);
            CHECK_EQUAL(SC_400, req.append(invalid[i], strlen(invalid[i])));
        }

        const char valid[] = "GET / HTTP/1.0\r\n\r\n";
        HttpReqTst req;
        req.appendLogId("allowMissingHostForHttp10");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(valid, sizeof(valid) - 1));
    }

    TEST(HttpReqTest_rejectHostMismatchWithAbsoluteTarget)
    {
        const char *requests[] =
        {
            "GET http://target.example:8080/path HTTP/1.1\r\n"
            "Host: other.example:8080\r\n\r\n",
            "GET http://target.example:8080/path HTTP/1.1\r\n"
            "Host: target.example:8081\r\n\r\n"
        };
        for (unsigned i = 0; i < sizeof(requests) / sizeof(requests[0]); ++i)
        {
            HttpReqTst req;
            req.appendLogId("rejectHostMismatchWithAbsoluteTarget");
            req.reset(0);
            req.setVHost((HttpVHost *)1);
            CHECK_EQUAL(SC_400, req.append(requests[i], strlen(requests[i])));
        }
    }

    TEST(HttpReqTest_encodeOriginFormForProxyBackend)
    {
        const char absolute[] =
            "GET http://example.com/a%2Fb?q=%23 HTTP/1.1\r\n"
            "Host: example.com\r\n\r\n";
        HttpReqTst req;
        req.appendLogId("encodeOriginFormForProxyBackend");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(absolute, sizeof(absolute) - 1));

        int len = 0;
        const char *line = req.encodeProxyReqLine(len);
        const char expected[] = "GET /a%2Fb?q=%23";
        CHECK(line != NULL);
        CHECK_EQUAL((int)sizeof(expected) - 1, len);
        CHECK_EQUAL(0, memcmp(expected, line, len));

        const char origin[] =
            "GET /a%2Fb?q=%23 HTTP/1.1\r\nHost: example.com\r\n\r\n";
        HttpReqTst originReq;
        originReq.appendLogId("leaveOriginFormForProxyBackend");
        originReq.reset(0);
        originReq.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, originReq.append(origin, sizeof(origin) - 1));
        CHECK(originReq.encodeProxyReqLine(len) == NULL);
        CHECK_EQUAL(0, len);
    }

    TEST(HttpReqTest_escapeDecodedRewritePathForProxyBackend)
    {
        const char request[] =
            "GET /start?next=%2F HTTP/1.1\r\n"
            "Host: example.com\r\n\r\n";
        HttpReqTst req;
        req.appendLogId("escapeDecodedRewritePathForProxyBackend");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(request, sizeof(request) - 1));

        const char rewritten[] = "/base/%2e%2e/a b?c#d";
        CHECK_EQUAL(0, req.internalRedirectURI(rewritten,
                                                sizeof(rewritten) - 1));
        int len = 0;
        const char *line = req.encodeProxyReqLine(len);
        const char expected[] =
            "GET /base/%252e%252e/a%20b%3Fc%23d?next=%2F";
        CHECK(line != NULL);
        CHECK_EQUAL((int)sizeof(expected) - 1, len);
        CHECK_EQUAL(0, memcmp(expected, line, len));
    }

    TEST(HttpReqTest_escapeNormalProxyRewritePathOnce)
    {
        const char request[] =
            "GET /start HTTP/1.1\r\n"
            "Host: example.com\r\n\r\n";
        HttpReqTst req;
        req.appendLogId("escapeNormalProxyRewritePathOnce");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(request, sizeof(request) - 1));

        const char rewritten[] = "/a b/%value/caf\xc3\xa9?name";
        CHECK_EQUAL(0, req.internalRedirectURI(rewritten,
                                                sizeof(rewritten) - 1,
                                                0, 0));
        CHECK_EQUAL(rewritten, req.getURI());

        int len = 0;
        const char *line = req.encodeProxyReqLine(len);
        const char expected[] =
            "GET /a%20b/%25value/caf%C3%A9%3Fname";
        CHECK(line != NULL);
        CHECK_EQUAL((int)sizeof(expected) - 1, len);
        CHECK_EQUAL(0, memcmp(expected, line, len));
    }

    TEST(HttpReqTest_keepNoEscapeProxyRewriteEscapes)
    {
        const char request[] =
            "GET /start HTTP/1.1\r\n"
            "Host: example.com\r\n\r\n";
        HttpReqTst req;
        req.appendLogId("keepNoEscapeProxyRewriteEscapes");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(request, sizeof(request) - 1));

        //[P,NE] passes an encoded target; only unsafe bytes get escaped
        const char rewritten[] = "/a%20b/g%2Fp/%zz/%4 x%41";
        CHECK_EQUAL(0, req.internalRedirectURI(rewritten,
                                                sizeof(rewritten) - 1,
                                                0, 1));
        int len = 0;
        const char *line = req.encodeProxyReqLine(len);
        const char expected[] = "GET /a%20b/g%2Fp/%25zz/%254%20x%41";
        CHECK(line != NULL);
        CHECK_EQUAL((int)sizeof(expected) - 1, len);
        CHECK_EQUAL(0, memcmp(expected, line, len));

        //a later decoded rewrite escapes '%' again
        const char decoded[] = "/a%20b";
        CHECK_EQUAL(0, req.setRewriteURI(decoded, sizeof(decoded) - 1));
        line = req.encodeProxyReqLine(len);
        const char expected2[] = "GET /a%2520b";
        CHECK(line != NULL);
        CHECK_EQUAL((int)sizeof(expected2) - 1, len);
        CHECK_EQUAL(0, memcmp(expected2, line, len));
    }

    TEST(HttpReqTest_keepPathSubDelimsForProxyBackend)
    {
        const char request[] =
            "GET /start HTTP/1.1\r\n"
            "Host: example.com\r\n\r\n";
        HttpReqTst req;
        req.appendLogId("keepPathSubDelimsForProxyBackend");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(request, sizeof(request) - 1));

        const char rewritten[] = "/@user/a+b,c;d=e&f:g!$'()*~-._ x";
        CHECK_EQUAL(0, req.internalRedirectURI(rewritten,
                                                sizeof(rewritten) - 1,
                                                0));
        int len = 0;
        const char *line = req.encodeProxyReqLine(len);
        const char expected[] = "GET /@user/a+b,c;d=e&f:g!$'()*~-._%20x";
        CHECK(line != NULL);
        CHECK_EQUAL((int)sizeof(expected) - 1, len);
        CHECK_EQUAL(0, memcmp(expected, line, len));
    }

    TEST(HttpReqTest_encodeLongMethodRewriteLine)
    {
        const char request[] =
            "BASELINE-CONTROL /start HTTP/1.1\r\n"
            "Host: example.com\r\n\r\n";
        HttpReqTst req;
        req.appendLogId("encodeLongMethodRewriteLine");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(request, sizeof(request) - 1));

        const char rewritten[] = "/a ";
        CHECK_EQUAL(0, req.internalRedirectURI(rewritten,
                                                sizeof(rewritten) - 1,
                                                0));
        int len = 0;
        const char *line = req.encodeProxyReqLine(len);
        const char expected[] = "BASELINE-CONTROL /a%20";
        CHECK(line != NULL);
        CHECK_EQUAL((int)sizeof(expected) - 1, len);
        CHECK_EQUAL(0, memcmp(expected, line, len));
    }

    TEST(HttpReqTest_escapeUnsafeRewriteQueryForProxyBackend)
    {
        const char request[] =
            "GET /start HTTP/1.1\r\n"
            "Host: example.com\r\n\r\n";
        HttpReqTst req;
        req.appendLogId("escapeUnsafeRewriteQueryForProxyBackend");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(request, sizeof(request) - 1));

        const char rewritten[] = "/backend";
        CHECK_EQUAL(0, req.internalRedirectURI(rewritten,
                                                sizeof(rewritten) - 1));
        const char query[] =
            "a=one two\tthree\r\nX-Test: yes#frag&ok=%2F+z?x";
        CHECK_EQUAL(0, req.setRewriteQueryString(query,
                                                 sizeof(query) - 1));

        int len = 0;
        const char *line = req.encodeProxyReqLine(len);
        const char expected[] =
            "GET /backend?a=one%20two%09three%0D%0AX-Test:%20yes%23frag"
            "&ok=%2F+z?x";
        CHECK(line != NULL);
        CHECK_EQUAL((int)sizeof(expected) - 1, len);
        CHECK_EQUAL(0, memcmp(expected, line, len));
    }

    TEST(HttpReqTest_parseAbsoluteTargetPathAfterShortAuthority)
    {
        const char request[] =
            "GET http://a.co/private/path HTTP/1.1\r\n"
            "Host: a.co\r\n"
            "\r\n";
        HttpReqTst req;
        req.appendLogId("parseAbsoluteTargetPathAfterShortAuthority");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(request, sizeof(request) - 1));
        CHECK_EQUAL(13, req.getURILen());
        CHECK_EQUAL(0, memcmp("/private/path", req.getURI(), 13));

        int len = 0;
        const char *line = req.encodeProxyReqLine(len);
        const char expected[] = "GET /private/path";
        CHECK(line != NULL);
        CHECK_EQUAL((int)sizeof(expected) - 1, len);
        CHECK_EQUAL(0, memcmp(expected, line, len));
    }

    TEST(HttpReqTest_validateAbsoluteTargetScheme)
    {
        const char *invalid[] =
        {
            "GET 1http://example.com/path HTTP/1.1\r\n"
            "Host: example.com\r\n\r\n",
            "GET ht_tp://example.com/path HTTP/1.1\r\n"
            "Host: example.com\r\n\r\n"
        };
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        {
            HttpReqTst req;
            req.appendLogId("validateAbsoluteTargetScheme");
            req.reset(0);
            req.setVHost((HttpVHost *)1);
            CHECK_EQUAL(SC_400, req.append(invalid[i], strlen(invalid[i])));
        }
    }

    TEST(HttpReqTest_validateRequestAuthority)
    {
        const char *invalid[] =
        {
            "Host: user@example.com",
            "Host: example.com:80:90",
            "Host: example.com:http",
            "Host: example.com:65536",
            "Host: example.com:",
            "Host: bad example.com",
            "Host: example.com/path",
            "Host: example.com,other.example",
            "Host: example.com]"
        };
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        {
            char request[256];
            int len = snprintf(request, sizeof(request),
                               "GET / HTTP/1.1\r\n%s\r\n\r\n", invalid[i]);
            HttpReqTst req;
            req.appendLogId("validateRequestAuthority");
            req.reset(0);
            req.setVHost((HttpVHost *)1);
            CHECK_EQUAL(SC_400, req.append(request, len));
        }

        const char valid[] =
            "GET / HTTP/1.1\r\nHost: [2001:db8::1]:8443\r\n\r\n";
        HttpReqTst req;
        req.appendLogId("validateRequestAuthority");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(valid, sizeof(valid) - 1));
        CHECK_EQUAL(13, req.getHostStrLen());
        CHECK_EQUAL(0, memcmp("[2001:db8::1]", req.getHostStr(), 13));
    }

    TEST(HttpReqTest_validateRewrittenProxyAuthority)
    {
        HttpReqTst req;
        req.appendLogId("validateRewrittenProxyAuthority");
        req.reset(0);

        const char *invalid[] =
        {
            "bad host.example", "user@example.com", "example.com/path",
            "example.com,other.example", "example.com\r\nX-Test: injected"
        };
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        {
            req.setNewHost(invalid[i], strlen(invalid[i]));
            CHECK(!req.isNewHostValid());
        }

        const char valid[] = "[2001:db8::1]:8443";
        req.setNewHost(valid, sizeof(valid) - 1);
        CHECK(req.isNewHostValid());
        CHECK_EQUAL(13, req.getNewHostNameLen());

        const char dnsWithPort[] = "example.com:8443";
        req.setNewHost(dnsWithPort, sizeof(dnsWithPort) - 1);
        CHECK(req.isNewHostValid());
        CHECK_EQUAL(11, req.getNewHostNameLen());
    }

    TEST(HttpReqTest_validateUnpackedRequestAuthority)
    {
        HttpReqTst req;
        req.appendLogId("validateUnpackedRequestAuthority");
        req.reset(0);
        UnpackedHeaders *headers = new UnpackedHeaders();
        UpkdHdrBuilder builder(headers, false);
        CHECK(LSXPACK_OK == addUnpackedReqHeader(builder, ":method", 7,
                                                 "GET", 3));
        CHECK(LSXPACK_OK == addUnpackedReqHeader(builder, ":scheme", 7,
                                                 "https", 5));
        CHECK(LSXPACK_OK == addUnpackedReqHeader(builder, ":authority", 10,
                                                 "user@example.com", 16));
        CHECK(LSXPACK_OK == addUnpackedReqHeader(builder, ":path", 5,
                                                 "/", 1));
        CHECK(LSXPACK_OK == builder.end());
        headers = builder.retrieveHeaders();

        CHECK_EQUAL(SC_400, req.processUnpackedHeaders(headers));
        delete headers;
    }

    TEST(HttpReqTest_acceptUnpackedOptionsAsteriskForm)
    {
        HttpReqTst req;
        req.appendLogId("acceptUnpackedOptionsAsteriskForm");
        req.reset(0);
        UnpackedHeaders *headers = new UnpackedHeaders();
        UpkdHdrBuilder builder(headers, false);
        CHECK(LSXPACK_OK == addUnpackedReqHeader(builder, ":method", 7,
                                                 "OPTIONS", 7));
        CHECK(LSXPACK_OK == addUnpackedReqHeader(builder, ":scheme", 7,
                                                 "https", 5));
        CHECK(LSXPACK_OK == addUnpackedReqHeader(builder, ":authority", 10,
                                                 "example.com", 11));
        CHECK(LSXPACK_OK == addUnpackedReqHeader(builder, ":path", 5,
                                                 "*", 1));
        CHECK(LSXPACK_OK == builder.end());
        headers = builder.retrieveHeaders();

        CHECK_EQUAL(0, req.processUnpackedHeaders(headers));
        CHECK_EQUAL("*", req.getURI());
        CHECK(req.isOptionsAsterisk());
    }

    TEST(HttpReqTest_acceptHttp1OptionsAsteriskForm)
    {
        const char request[] =
            "OPTIONS * HTTP/1.1\r\n"
            "Host: example.com\r\n\r\n";
        HttpReqTst req;
        req.appendLogId("acceptHttp1OptionsAsteriskForm");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(request, sizeof(request) - 1));
        CHECK_EQUAL("*", req.getURI());
        CHECK(req.isOptionsAsterisk());
    }

    TEST(HttpReqTest_rejectInvalidHttp1AsteriskForm)
    {
        const char *invalid[] =
        {
            "GET * HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "OPTIONS *x HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "OPTIONS *?a HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "OPTIONS */ HTTP/1.1\r\nHost: example.com\r\n\r\n",
        };
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        {
            HttpReqTst req;
            req.appendLogId("rejectInvalidHttp1AsteriskForm");
            req.reset(0);
            req.setVHost((HttpVHost *)1);
            CHECK_EQUAL(SC_400, req.append(invalid[i], strlen(invalid[i])));
        }
    }

    TEST(HttpReqTest_recheckAuthAfterPhysicalRewriteContextChange)
    {
        HttpVHost vhost("rewrite-auth-context-test");
        HttpContext *target = vhost.addContext(
                                  "/protected/", HandlerType::HT_NULL,
                                  "/srv/protected/", NULL, 1);
        CHECK(target != NULL);

        HttpReqTst req;
        req.appendLogId("recheckAuthAfterPhysicalRewriteContextChange");
        req.reset(0);
        req.setVHost(&vhost);
        req.setContext(&vhost.getRootContext());
        req.orContextState(CONTEXT_AUTH_CHECKED);

        const char rewritten[] = "/srv/protected/resource";
        CHECK_EQUAL(0, req.postRewriteProcess(rewritten,
                                              sizeof(rewritten) - 1));
        CHECK_EQUAL(target, req.getContext());
        CHECK_EQUAL(0, req.getContextState(CONTEXT_AUTH_CHECKED));
    }

    TEST(HttpReqTest_recheckAuthBeforeProxyContextReturn)
    {
        ProxyContextTestHandler proxyHandler;
        HttpVHost vhost("rewrite-proxy-auth-test");
        HttpContext *target = vhost.addContext(
                                  "/proxy/", HandlerType::HT_NULL,
                                  "/srv/proxy/", NULL, 1);
        CHECK(target != NULL);
        target->setHandler(&proxyHandler);

        HttpReqTst req;
        req.appendLogId("recheckAuthBeforeProxyContextReturn");
        req.reset(0);
        req.setVHost(&vhost);
        req.setContext(&vhost.getRootContext());
        req.orContextState(CONTEXT_AUTH_CHECKED);
        CHECK_EQUAL(0, req.setRewriteURI("/proxy/resource", 15));

        CHECK_EQUAL(-2, req.processContext());
        CHECK_EQUAL(target, req.getContext());
        CHECK_EQUAL(0, req.getContextState(CONTEXT_AUTH_CHECKED));
    }

    TEST(HttpReqTest_rejectDuplicateAuthorization)
    {
        const char request[] =
            "GET / HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Authorization: Basic YWxpY2U6cGFzcw==\r\n"
            "Authorization: Bearer attacker\r\n"
            "\r\n";
        HttpReqTst req;
        req.appendLogId("rejectDuplicateAuthorization");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(SC_400, req.append(request, sizeof(request) - 1));
    }

    TEST(HttpReqTest_rejectDuplicateUnpackedAuthorization)
    {
        const char first[] = "Basic YWxpY2U6cGFzcw==";
        const char second[] = "Bearer attacker";
        HttpReqTst req;
        req.appendLogId("rejectDuplicateUnpackedAuthorization");
        req.reset(0);
        UnpackedHeaders *headers = new UnpackedHeaders();
        UpkdHdrBuilder builder(headers, false);
        CHECK(LSXPACK_OK == addUnpackedReqHeader(builder, ":method", 7,
                                                 "GET", 3));
        CHECK(LSXPACK_OK == addUnpackedReqHeader(builder, ":scheme", 7,
                                                 "https", 5));
        CHECK(LSXPACK_OK == addUnpackedReqHeader(builder, ":authority", 10,
                                                 "example.com", 11));
        CHECK(LSXPACK_OK == addUnpackedReqHeader(builder, ":path", 5,
                                                 "/", 1));
        CHECK(LSXPACK_OK == addUnpackedReqHeader(
                            builder, "authorization", 13,
                            first, sizeof(first) - 1,
                            LSHPACK_HDR_AUTHORIZATION));
        CHECK(LSXPACK_OK == addUnpackedReqHeader(
                            builder, "authorization", 13,
                            second, sizeof(second) - 1,
                            LSHPACK_HDR_AUTHORIZATION));
        CHECK(LSXPACK_OK == builder.end());
        headers = builder.retrieveHeaders();

        CHECK_EQUAL(SC_400, req.processUnpackedHeaders(headers));
        delete headers;
    }

    TEST(HttpReqTest_trustCacheFrontendHeadersOnlyFromProxy)
    {
        const char request[] =
            "GET / HTTP/1.1\r\n"
            "Host: example.test\r\n"
            "X-LSCACHE: 1\r\n"
            "\r\n";
        HttpReqTst req;
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(request, sizeof(request) - 1));

        int valueLen;
        CHECK_EQUAL(0, req.getContextState(LSCACHE_FRONTEND));
        CHECK(req.getEnv("LSCACHE_FRONTEND", 16, valueLen) == NULL);

        req.processCacheFrontendHeader(false);
        CHECK_EQUAL(0, req.getContextState(LSCACHE_FRONTEND));
        CHECK(req.getEnv("LSCACHE_FRONTEND", 16, valueLen) == NULL);

        req.processCacheFrontendHeader(true);
        CHECK(req.getContextState(LSCACHE_FRONTEND));
        const char *value = req.getEnv("LSCACHE_FRONTEND", 16, valueLen);
        CHECK(value != NULL);
        CHECK_EQUAL(1, valueLen);
        CHECK_EQUAL('1', *value);
    }

    TEST(HttpReqTest_dropRepeatedUnknownHeadersInOnePass)
    {
        std::string request = "GET / HTTP/1.1\r\nHost: example.test\r\n";
        for (int i = 0; i < 256; ++i)
        {
            request.append("X-LSCACHE: 1\r\n");
            request.append("X-Keep: value\r\n");
        }
        request.append("X-Real-IP: 192.0.2.1\r\n\r\n");

        HttpReqTst req;
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(request.data(), request.size()));

        req.dropUnknownReqHeader("X-LSCACHE", 9);
        int valueLen = 0;
        CHECK(req.getHeader("X-LSCACHE", 9, valueLen) == NULL);
        CHECK(req.getHeader("X-Keep", 6, valueLen) != NULL);
        CHECK(req.getHeader("X-Real-IP", 9, valueLen) != NULL);
        char realIpName[20];
        const char *realIp = req.getCfRealIpHeader(realIpName, valueLen);
        CHECK(realIp != NULL);
        CHECK_EQUAL("X-Real-IP", realIpName);
        CHECK_EQUAL("192.0.2.1", std::string(realIp, valueLen));
        CHECK_EQUAL(0, countHeaderName(req, "X-LSCACHE", 9));
        CHECK_EQUAL(256, countHeaderName(req, "X-Keep", 6));
    }

    TEST(HttpReqTest_rejectRequestTargetControls)
    {
        const unsigned char invalid[] = { 1, 0x0b, 0x0c, 0x1f, 0x7f };
        for (unsigned i = 0; i < sizeof(invalid); ++i)
        {
            char input[] =
                "GET /a?b HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "\r\n";
            *strchr(input, '?') = invalid[i];
            HttpReqTst req;
            req.appendLogId("rejectRequestTargetControls");
            req.reset(0);
            req.setVHost((HttpVHost *)1);
            CHECK_EQUAL(SC_400, req.append(input, sizeof(input) - 1));
        }
    }

    TEST(HttpReqTest_rejectFragmentInRequestTarget)
    {
        const char invalid[] =
            "GET /public#private HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "\r\n";
        HttpReqTst req;
        req.appendLogId("rejectFragmentInRequestTarget");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(SC_400, req.append(invalid, sizeof(invalid) - 1));
    }

    TEST(HttpReqTest_rejectBackslashInRequestPath)
    {
        const char invalid[] =
            "GET /safe\\../admin HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "\r\n";
        HttpReqTst invalidReq;
        invalidReq.appendLogId("rejectBackslashInRequestPath");
        invalidReq.reset(0);
        invalidReq.setVHost((HttpVHost *)1);
        CHECK_EQUAL(SC_400,
                    invalidReq.append(invalid, sizeof(invalid) - 1));

        const char valid[] =
            "GET /path?value=a\\b HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "\r\n";
        HttpReqTst validReq;
        validReq.appendLogId("allowBackslashInRequestQuery");
        validReq.reset(0);
        validReq.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, validReq.append(valid, sizeof(valid) - 1));
    }

    TEST(HttpReqTest_rejectEncodedInvalidPathBytes)
    {
        const char *invalid[] =
        {
            "GET /safe%5c../admin HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "GET /safe%0dpath HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "GET /safe%1fpath HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "GET /safe%7fpath HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "GET /safe% HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "GET /safe%2 HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "GET /safe%GG HTTP/1.1\r\nHost: example.com\r\n\r\n"
        };
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        {
            HttpReqTst req;
            req.appendLogId("rejectEncodedInvalidPathBytes");
            req.reset(0);
            req.setVHost((HttpVHost *)1);
            CHECK_EQUAL(SC_400, req.append(invalid[i], strlen(invalid[i])));
        }
    }

    TEST(HttpReqTest_rejectEncodedInvalidUnpackedPathBytes)
    {
        const char *invalid[] =
        {
            "/safe%5c../admin", "/safe%0dpath", "/safe%7fpath",
            "/safe%", "/safe%2", "/safe%GG"
        };
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        {
            HttpReqTst req;
            req.appendLogId("rejectEncodedInvalidUnpackedPathBytes");
            req.reset(0);
            UnpackedHeaders *headers = new UnpackedHeaders();
            UpkdHdrBuilder builder(headers, false);
            CHECK_EQUAL(LSXPACK_OK, addUnpackedReqHeader(
                            builder, ":method", 7, "GET", 3));
            CHECK_EQUAL(LSXPACK_OK, addUnpackedReqHeader(
                            builder, ":scheme", 7, "https", 5));
            CHECK_EQUAL(LSXPACK_OK, addUnpackedReqHeader(
                            builder, ":authority", 10, "example.com", 11));
            CHECK_EQUAL(LSXPACK_OK, addUnpackedReqHeader(
                            builder, ":path", 5, invalid[i],
                            strlen(invalid[i])));
            CHECK_EQUAL(LSXPACK_OK, builder.end());
            headers = builder.retrieveHeaders();

            CHECK_EQUAL(SC_400, req.processUnpackedHeaders(headers));
            delete headers;
        }
    }

    TEST(HttpReqTest_limitUnpackedRequestPathLength)
    {
        int maxReqLineLen =
            HttpServerConfig::getInstance().getMaxURLLen();
        int pathLen = maxReqLineLen - 3 - 12 + 1;
        char *path = new char[pathLen];
        memset(path, 'a', pathLen);
        path[0] = '/';

        HttpReqTst req;
        req.appendLogId("limitUnpackedRequestPathLength");
        req.reset(0);
        UnpackedHeaders *headers = new UnpackedHeaders();
        UpkdHdrBuilder builder(headers, false);
        CHECK_EQUAL(LSXPACK_OK, addUnpackedReqHeader(
                        builder, ":method", 7, "GET", 3));
        CHECK_EQUAL(LSXPACK_OK, addUnpackedReqHeader(
                        builder, ":scheme", 7, "https", 5));
        CHECK_EQUAL(LSXPACK_OK, addUnpackedReqHeader(
                        builder, ":authority", 10, "example.com", 11));
        CHECK_EQUAL(LSXPACK_OK, addUnpackedReqHeader(
                        builder, ":path", 5, path, pathLen));
        CHECK_EQUAL(LSXPACK_OK, builder.end());
        headers = builder.retrieveHeaders();

        CHECK_EQUAL(SC_414, req.processUnpackedHeaders(headers));
        delete headers;
        delete [] path;
    }

    TEST(HttpReqTest_sanitizeGeneralInternalRedirect)
    {
        HttpReqTst req;
        req.appendLogId("sanitizeGeneralInternalRedirect");
        req.reset(0);

        const char valid[] = "/safe/../target?next=../admin&raw=%GG";
        CHECK_EQUAL((int)sizeof(valid) - 1,
                    req.setLocation(valid, sizeof(valid) - 1));
        const char *pLocation = req.getLocation();
        int locationLen = req.getLocationLen();
        CHECK_EQUAL(0, req.setCurrentURL(pLocation, locationLen));
        CHECK_EQUAL("/target", req.getURI());
        CHECK_EQUAL("next=../admin&raw=%GG", req.getQueryString());

        const char *invalid[] =
        {
            "/../../secret",
            "/safe/%2e%2e/%2e%2e/secret",
            "/safe%5c../secret",
            "/safe%0dpath",
            "/safe#fragment",
            "/safe/%GG"
        };
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
            CHECK_EQUAL(SC_400,
                        req.setCurrentURL(invalid[i], strlen(invalid[i]), 1));
    }

    TEST(HttpReqTest_sanitizeRewritePaths)
    {
        HttpReqTst req;
        req.appendLogId("sanitizeRewritePaths");
        req.reset(0);

        const char valid[] = "/public/a/../file";
        CHECK_EQUAL(0, req.setRewriteURI(valid, sizeof(valid) - 1));
        CHECK_EQUAL("/public/file", req.getURI());

        const char *invalid[] =
        {
            "relative/path", "/../../secret", "/safe\\../secret",
            "/safe\rpath"
        };
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
            CHECK_EQUAL(LS_FAIL,
                        req.setRewriteURI(invalid[i], strlen(invalid[i])));

        HttpVHost vhost("rewrite-path-test");
        req.reset(0);
        req.setVHost(&vhost);
        CHECK_EQUAL(0, req.postRewriteProcess(valid, sizeof(valid) - 1));
        CHECK_EQUAL("/public/file", req.getURI());
        CHECK_EQUAL(LS_FAIL,
                    req.postRewriteProcess("/../../secret", 13));
    }

    TEST(HttpReqTest_cleanTrailingRequestDotSegments)
    {
        const char *invalid[] =
        {
            "GET /.. HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "GET /%2e%2e HTTP/1.1\r\nHost: example.com\r\n\r\n"
        };
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        {
            HttpReqTst req;
            req.appendLogId("rejectTrailingRootTraversal");
            req.reset(0);
            req.setVHost((HttpVHost *)1);
            CHECK_EQUAL(SC_400, req.append(invalid[i], strlen(invalid[i])));
        }

        const char valid[] =
            "GET /safe/.. HTTP/1.1\r\nHost: example.com\r\n\r\n";
        HttpReqTst req;
        req.appendLogId("cleanTrailingParentSegment");
        req.reset(0);
        req.setVHost((HttpVHost *)1);
        CHECK_EQUAL(0, req.append(valid, sizeof(valid) - 1));
        CHECK_EQUAL("/", req.getURI());
    }

    TEST(HttpReqTest_requireCanonicalRequestLineDelimiters)
    {
        const char *invalid[] =
        {
            "GET\t/ HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "GET  / HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "GET /\tHTTP/1.1\r\nHost: example.com\r\n\r\n",
            "GET /  HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "GET / http/1.1\r\nHost: example.com\r\n\r\n",
            " GET / HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "\tGET / HTTP/1.1\r\nHost: example.com\r\n\r\n",
            "\vGET / HTTP/1.1\r\nHost: example.com\r\n\r\n"
        };
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        {
            HttpReqTst req;
            req.appendLogId("requireCanonicalRequestLineDelimiters");
            req.reset(0);
            req.setVHost((HttpVHost *)1);
            CHECK_EQUAL(SC_400, req.append(invalid[i], strlen(invalid[i])));
        }
    }

    TEST(HttpReqTest_sanitizeInternalResponseLocation)
    {
        HttpVHost vhost("internal-location-test");
        CHECK(vhost.addContext("/errors/", HandlerType::HT_NULL,
                               "/custom_error/", NULL, 1) != NULL);
        HttpReqTst req;
        req.appendLogId("sanitizeInternalResponseLocation");
        req.reset(0);
        req.setVHost(&vhost);

        const char plain[] = "/custom_error/plain/file.html?name=value";
        CHECK_EQUAL(0, req.locationToUrl(plain, sizeof(plain) - 1));
        CHECK_EQUAL("/errors/plain/file.html?name=value", req.getLocation());

        const char safe[] =
            "/custom_error/a/./b//not%20found.html?next=../admin&bad=%GG";
        CHECK_EQUAL(0, req.locationToUrl(safe, sizeof(safe) - 1));
        CHECK_EQUAL("/errors/a/b/not%20found.html?next=../admin&bad=%GG",
                    req.getLocation());

        const char encodedQuestion[] =
            "/custom_error/name%3Fpart?name=unchanged%2";
        CHECK_EQUAL(0, req.locationToUrl(encodedQuestion,
                                         sizeof(encodedQuestion) - 1));
        CHECK_EQUAL("/errors/name%3Fpart?name=unchanged%2",
                    req.getLocation());

        const char *invalid[] =
        {
            "/custom_error/../../../home/shared_app/html/../../url",
            "/custom_error/%2e%2e/secret",
            "/custom_error/%2E%2E%2fsecret",
            "/custom_error/%5c..%5csecret",
            "/custom_error/%00secret",
            "/custom_error/%2",
            "/custom_error/%GG"
        };
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        {
            req.clearLocation();
            CHECK_EQUAL(LS_FAIL,
                        req.locationToUrl(invalid[i], strlen(invalid[i])));
            CHECK(req.getLocation() == NULL);
        }

        const char doubleEncoded[] = "/custom_error/%252e%252e/secret";
        CHECK_EQUAL(0, req.locationToUrl(doubleEncoded,
                                         sizeof(doubleEncoded) - 1));
        CHECK_EQUAL("/errors/%252e%252e/secret", req.getLocation());
    }
}


#endif
