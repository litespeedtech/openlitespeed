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
#include "httpresp.h"

#include <http/expiresctrl.h>

// #include <http/httpheader.h> //setheader commented out.
#include <http/httpreq.h>
#include <lsr/ls_strtool.h>
#include <util/autostr.h>
#include <util/datetime.h>
#include <util/gzipbuf.h>
#include <util/stringtool.h>
#include <util/vmembuf.h>

#include <string.h>

static const char *s_protocol[] =
{
    "http://",
    "https://"
};

HttpResp::HttpResp(ls_xpool_t *pool)
    : m_respHeaders(pool)
    , m_pRespBodyBuf(NULL)
    , m_pGzipBuf(NULL)
{
    m_lEntityLength = 0;
    m_lEntityFinished = 0;
}


HttpResp::~HttpResp()
{
}


void HttpResp::reset(int delCookies)
{
    if (delCookies)
        m_respHeaders.reset();
    else
        m_respHeaders.reset2();

    m_lEntityLength = LSI_RSP_BODY_SIZE_UNKNOWN;
    m_lEntityFinished = 0;
    resetRespBody();
}

void HttpResp::resetRespBody()
{
    if (m_pGzipBuf)
        m_pGzipBuf->reinit();
    if (m_pRespBodyBuf)
    {
        m_pRespBodyBuf->rewindReadBuf();
        m_pRespBodyBuf->rewindWriteBuf();
    }
}


void HttpResp::appendContentLenHeader()
{
    if (m_lEntityLength >= 0)
    {
        char achLength[44] = {0};
        int n = StringTool::offsetToStr(achLength, 43, m_lEntityLength);
        m_respHeaders.add(HttpRespHeaders::H_CONTENT_LENGTH, achLength, n);
    }
}


int HttpResp::addExpiresHeader(time_t lastMod, const ExpiresCtrl *pExpires)
{
    char achTmp[128];
    time_t expire;
    int    age;
    if (m_respHeaders.isHeaderSet(HttpRespHeaders::H_CACHE_CTRL)
        || m_respHeaders.isHeaderSet(HttpRespHeaders::H_EXPIRES))
        return 0;
    switch (pExpires->getBase())
    {
    case EXPIRES_ACCESS:
        age = pExpires->getAge();
        expire = DateTime::s_curTime + age;
        break;
    case EXPIRES_MODIFY:
        expire = lastMod + pExpires->getAge();
        age = (int)expire - (int)DateTime::s_curTime;
        break;
    default:
        return 0;
    }
    int n = snprintf(achTmp, 128,
                     "public, max-age=%d", age);
    m_respHeaders.add(HttpRespHeaders::H_CACHE_CTRL, achTmp, n);
    DateTime::getRFCTime(expire, achTmp);
    m_respHeaders.add(HttpRespHeaders::H_EXPIRES, achTmp, RFC_1123_TIME_LEN);

    return 0;
}


int HttpResp::setContentTypeHeader(const char *pType, int typeLen,
                                   const AutoStr2 *pCharset)
{
    int ret = m_respHeaders.add(HttpRespHeaders::H_CONTENT_TYPE,
                                pType, typeLen);
    if (ret == 0 && pCharset)
        m_respHeaders.appendLastVal(pCharset->c_str(), pCharset->len());
    return ret;
}


int HttpResp::appendHeader(const char *pName, int nameLen,
                           const char *pValue, int valLen)
{
    m_respHeaders.add(pName, nameLen, pValue, valLen, LSI_HEADEROP_ADD);
    return 0;
}


//void HttpResp::setHeader( int headerCode, long lVal )
//{
//    char buf[80];
//    int len = sprintf( buf, "%s%ld\r\n", HttpHeader::getHeader( headerCode ), lVal );
//    m_outputBuf.append( buf, len );
//}


int HttpResp::appendLastMod(long tmMod)
{
    static char sTimeBuf[RFC_1123_TIME_LEN + 1] = {0};
    DateTime::getRFCTime(tmMod, sTimeBuf);
    m_respHeaders.add(HttpRespHeaders::H_LAST_MODIFIED, sTimeBuf,
                      RFC_1123_TIME_LEN);
    return 0;
}


int HttpResp::addCookie(const char *pName, const char *pVal,
                        const char *path, const char *domain, int expires,
                        int secure, int httponly)
{
    char achBuf[8192] = "";
    char *p = achBuf;

    if (!pName || !pVal || !domain)
        return LS_FAIL;

    if (path == NULL)
        path = "/";
    p += ls_snprintf(achBuf, 8091, "%s=%s; path=%s; domain=%s",
                     pName, pVal, path, domain);
    if (expires)
    {
        memcpy(p, "; expires=", 10);
        p += 10;
        long t = DateTime::s_curTime + expires * 60;
        DateTime::getRFCTime(t, p);
        p += RFC_1123_TIME_LEN;
    }
    if (secure)
    {
        memcpy(p, "; secure", 8);
        p += 8;
    }
    if (httponly)
    {
        memcpy(p, "; HttpOnly", 10);
        p += 10;
    }
    m_respHeaders.add(HttpRespHeaders::H_SET_COOKIE, achBuf, p - achBuf,
                      LSI_HEADEROP_ADD);
    return 0;
}


int HttpResp::appendDynBody(VMemBuf *pvBuf, int offset, int len)
{
    char *pBuf;
    size_t iBufLen;
    int ret, total = 0;
    if (pvBuf->setROffset(offset) == -1)
        return -1;
    while (len > 0)
    {
        pBuf = pvBuf->getReadBuffer(iBufLen);
        if ((int)iBufLen > len)
            iBufLen = len;
        ret = appendDynBody(pBuf, iBufLen);
        if (ret > 0)
        {
            total += ret;
            pvBuf->readUsed(ret);
        }
        if (ret < (int)iBufLen)
            break;
        len -= iBufLen;
    }
    return total;
}


int HttpResp::appendDynBody(const char *pBuf, int len)
{
    int ret = 0;
    if ((getGzipBuf()) && (getGzipBuf()->getType() == GzipBuf::COMPRESSOR_COMPRESS))
    {
        ret = getGzipBuf()->write(pBuf, len);
    }
    else
    {
        ret = appendDynBodyEx(pBuf, len);
    }

    return ret;
}


int HttpResp::appendDynBodyEx(const char *pBuf, int len)
{
    char *pCache;
    size_t iCacheSize;
    const char *pBufOrg = pBuf;

    while (len > 0)
    {
        pCache = getRespBodyBuf()->getWriteBuffer(iCacheSize);
        if (pCache)
        {
            int ret = len;
            if (iCacheSize < (size_t)ret)
                ret = iCacheSize;
            if (pCache != pBuf)
                memmove(pCache, pBuf, ret);
            pBuf += ret;
            len -= ret;
            getRespBodyBuf()->writeUsed(ret);
        }
        else
        {
            //LS_ERROR( getLogger(), "[%s] out of swapping space while appending to response buffer!", getLogId() ));
            return -1;
        }
    }
    return pBuf - pBufOrg;
}
