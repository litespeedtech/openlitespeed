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
#ifndef HTTPCGITOOL_H
#define HTTPCGITOOL_H


class HttpResp;
class HttpReq;
class HttpSession;
class HttpExtConnector;
class FcgiEnv;
class Env;
class IEnv;

class HttpCgiTool
{
    HttpCgiTool()   {};
    ~HttpCgiTool()  {};
    static int addHttpHeaderEnv(IEnv *pEnv, HttpReq *pReq);
    static int addSpecialEnv(IEnv *pEnv, HttpReq *pReq);
public:
    static int processContentType(HttpSession *pSession,
                                  const char *pValue, int valLen);
    static int processExpires(HttpReq *pReq, HttpResp *pResp, const char *pValue);

    static int processHeaderLine(HttpExtConnector *pExtConn,
                                 const char *pName, int nameLen,
                                 const char *pValue, int valLen);
    static int processHeaderLine(HttpExtConnector *pExtConn, int index,
                                 const char *pLineBegin, int nameLen,
                                 const char *pValue, int valLen);
    static int processHeaderLine2(HttpExtConnector *pExtConn, int index,
                                 const char *pLineBegin, int nameLen,
                                 const char *pValue, int valLen);
    static int processHeaderLine(HttpExtConnector *pExtConn,
                                 const char *pValue, const char *pLineEnd);
    static void processStatusCode(HttpExtConnector* connector, int code);
    static int parseRespHeader(HttpExtConnector *pExtConn,
                               const char *pBuf, int size);
    static int buildEnv(IEnv *pEnv, HttpSession *pSession);
    static int buildFcgiEnv(FcgiEnv *pEnv, HttpSession *pSession);
    static void buildServerEnv();
    static int buildCommonEnv(IEnv *pEnv, HttpSession *pSession);

};



#endif
