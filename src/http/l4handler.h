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

#ifndef L4TUNNEL_H
#define L4TUNNEL_H

#include <http/hiostream.h>
#include <http/httpreq.h>
#include <extensions/ssl4conn.h>
#include <util/loopbuf.h>
#include "socket/gsockaddr.h"

class L4Handler : public HioHandler
{

public:
    L4Handler();
    ~L4Handler();

    int  init(HttpReq &req, const GSockAddr *pGSockAddr, const char *pIP,
              int iIpLen, bool ssl, const char *url, int url_len);

    int  init(HttpReq &req, const char *pAddrStr, const char *pIP,
              int iIpLen, bool ssl, const char *url, int url_len);

    LoopBuf    *getBuf()            {   return m_buf;  }
    void        continueRead()      {   getStream()->continueRead();    }
    void        suspendRead()       {   getStream()->suspendRead();     }
    void        suspendWrite()      {   getStream()->suspendWrite();    }
    void        continueWrite()     {   getStream()->continueWrite();   }
    bool        isWantRead() const  {   return getStream()->isWantRead();   }

    int         onReadEx();
    int         doWrite();
    void        closeBothConnection();

private:
    Ssl4conn       *m_pL4conn;
    LoopBuf        *m_buf;


    void buildWsReq(HttpReq &req, const char *pIP, int iIpLen,
                    const char *url, int url_len);
    void recycle();
    int onTimerEx()         {   return 0;   }
    int onCloseEx();
    int onWriteEx();
    int onInitConnected()   {   return 0;   };

};

#endif // L4TUNNEL_H
