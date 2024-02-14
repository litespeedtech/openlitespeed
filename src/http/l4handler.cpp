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

#include "l4handler.h"
#include <log4cxx/logger.h>
#include <socket/gsockaddr.h>

L4Handler::L4Handler()
{
    m_pL4conn = new Ssl4conn(this);
    m_buf = new LoopBuf(MAX_OUTGOING_BUF_ZISE);
}


L4Handler::~L4Handler()
{
    if (m_buf)
        delete m_buf;
    if (m_pL4conn)
        delete m_pL4conn;
}


void L4Handler::recycle()
{
    delete this;
}


int L4Handler::onReadEx()
{
    bool empty = m_pL4conn->getBuf()->empty();
    int space;

    while (true)
    {
        space = m_pL4conn->getBuf()->contiguous();
        if (space <= 0)
        {
            if (m_pL4conn->doWrite() == LS_FAIL)
                goto ERROR;
            if (!m_pL4conn->getBuf()->empty() && empty)
                m_pL4conn->continueWrite();

            if ((space = m_pL4conn->getBuf()->available()) <= 0)
            {
                suspendRead();
                LS_DBG_L(getLogSession(), "L4Handler: suspendRead");
                return 0;
            }
        }

        int n = getStream()->read(m_pL4conn->getBuf()->end(), space);
        LS_DBG_L(getLogSession(), "L4Handler: read [%d]", n);

        if (n > 0)
            m_pL4conn->getBuf()->used(n);
        else if (n == 0)
            break;
        else // if (n < 0)
            goto ERROR;
    }

    if (!m_pL4conn->getBuf()->empty())
    {
        if (m_pL4conn->doWrite() == LS_FAIL)
            goto ERROR;
        if (!m_pL4conn->getBuf()->empty() && empty)
            m_pL4conn->continueWrite();

        if (m_pL4conn->getBuf()->available() <= 0)
        {
            suspendRead();
            LS_DBG_L(getLogSession(), "L4Handler: suspendRead");
        }
    }

    return 0;
ERROR:
    closeBothConnection();
    return LS_FAIL;
}


int L4Handler::onWriteEx()
{
    bool full = ((getBuf()->available() == 0) ? true : false);
    int length;

    while ((length = getBuf()->blockSize()) > 0)
    {
        int n = getStream()->write(getBuf()->begin(), length);
        LS_DBG_L(getLogSession(), "L4Handler: write [%d of %d]", n,
                 length);

        if (n > 0)
            getBuf()->pop_front(n);
        else if (n == 0)
            break;
        else // if (n < 0)
        {
            closeBothConnection();
            return -1;
        }
    }
    getStream()->flush();

    if (getBuf()->available() != 0)
    {
        if (full)
            m_pL4conn->continueRead();

        if (getBuf()->empty())
        {
            suspendWrite();
            LS_DBG_L(getLogSession(), "[L4conn] m_pL4conn->continueRead");
        }
    }

    return 0;
}


void L4Handler::buildWsReq(HttpReq &req, const char *pIP, int iIpLen,
                           const char *url, int url_len)
{
    int hasSlashR = 1; //"\r\n"" or "\n"
    LoopBuf *pBuff = m_pL4conn->getBuf();
    if (url == NULL)
    {
        pBuff->append(req.getOrgReqLine(), req.getHttpHeaderLen());
    }
    else
    {
        pBuff->append(HttpMethod::get(req.getMethod()),
                      HttpMethod::getLen(req.getMethod()));
        pBuff->append(' ');
        pBuff->append(url, url_len);
        pBuff->append(" HTTP/1.1", 9);
        pBuff->append(req.getOrgReqLine() + req.getOrgReqLineLen(),
                      req.getHttpHeaderLen() - req.getOrgReqLineLen());
    }
    char *pBuffEnd = pBuff->end();
    assert(pBuffEnd[-1] == '\n');
    if (pBuffEnd[-2] == 'n')
        hasSlashR = 0;
    else
        assert(pBuffEnd[-2] == '\r');

    pBuff->used(-1 * hasSlashR - 1);
    if (req.isHttps())
        pBuff->append("X-Forwarded-Proto: https\r\n", 26);
    pBuff->append("X-Forwarded-For: ", 17);
    pBuff->append(pIP, iIpLen);
    if (hasSlashR)
        pBuff->append("\r\n\r\n", 4);
    else
        pBuff->append("\n\n", 2);

    LS_DBG_L(getLogSession(),
             "L4Handler: init WebSocket, reqheader [%.*s]",
             req.getHttpHeaderLen(), req.getOrgReqLine() );
}


int L4Handler::init(HttpReq &req, const char *pAddrStr,
                    const char *pIP, int iIpLen, bool ssl,
                    const char *url, int url_len)
{
    buildWsReq(req, pIP, iIpLen, url, url_len);

    int ret = m_pL4conn->init(pAddrStr, ssl);
    if (ret != 0)
        return ret;

    return 0;
}


int L4Handler::init(HttpReq &req, const GSockAddr *pGSockAddr,
                    const char *pIP, int iIpLen, bool ssl,
                    const char *url, int url_len)
{
    buildWsReq(req, pIP, iIpLen, url, url_len);

    int ret = m_pL4conn->init(pGSockAddr, ssl);
    if (ret != 0)
        return ret;

    return 0;
}


int L4Handler::doWrite()
{
    return onWriteEx();
}


void L4Handler::closeBothConnection()
{
    if (m_buf)
    {
        delete m_buf;
        m_buf = NULL;
        if (m_pL4conn)
            m_pL4conn->close();
        if (getStream())
            getStream()->close();
    }
}


int L4Handler::onCloseEx()
{
    if (m_pL4conn)
        m_pL4conn->close();
    //getStream()->handlerReadyToRelease();
    return 0;
}

