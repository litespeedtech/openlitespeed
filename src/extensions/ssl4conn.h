/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2021  LiteSpeed Technologies, Inc.                 *
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

#ifndef SSL4CONN_H
#define SSL4CONN_H

#include <adns/adns.h>
#include <http/httplog.h>
#include <edio/ssledstream.h>
#define MAX_OUTGOING_BUF_ZISE    8192
#include <util/loopbuf.h>
#include "socket/gsockaddr.h"

class L4Handler;

class Ssl4conn : public SslEdStream
{
public:
    Ssl4conn(L4Handler  *pL4Handler);
    virtual ~Ssl4conn();

    const char *buildLogId();

    int onEventDone(short event);
    int onError();
    int onWrite();
    int onRead();
    int close();
    int readv(iovec *vector, int count)  {    return 0;  }

    int onInitConnected();

    int doRead();
    int doWrite();


    //Return 0 is OK
    int init(const GSockAddr *pGSockAddr, bool ssl);
    int connectEx(const GSockAddr *pGSockAddr);

    int init(const char *pAddrStr, bool ssl);
    int connectEx(const char *pAddrStr);

    LoopBuf  *getBuf()             {   return m_buf;  }

private:
    AdnsFetch       m_adns;
    L4Handler      *m_pL4Handler;
    LoopBuf        *m_buf;

    enum
    {
        DISCONNECTED,
        ASYNC_NSLOOKUP,
        CONNECTING,
        PROCESSING,
        CLOSING,
    };

    int doAddrLookup(const char *pAddrStr);
    static int adnsLookupCb(void *arg, const long lParam, void *pParam);
    int adnsDone(const void *ip, int ip_len);


};

#endif // SSL4CONN_H
