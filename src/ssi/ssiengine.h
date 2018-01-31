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
#ifndef SSIENGINE_H
#define SSIENGINE_H


#include <http/httphandler.h>

struct lsi_subreq_s;
class HttpSession;
class ExprToken;
class Expression;
class SsiScript;
class SsiComponent;
class SsiRuntime;
class SubstItem;
class Ssi_If;


class SsiEngine : public HttpHandler
{
public:
    SsiEngine();

    ~SsiEngine();

    virtual const char *getName() const;


    static int beginExecute(HttpSession *pSession,
                            const SsiScript *pScript);
    static int resumeExecute(HttpSession *pSession);

    static int appendLocation(HttpSession *pSession, const char *pLocation,
                              int len);

    static void printError(HttpSession *pSession, const char *pError);

private:
    static int updateSsiConfig(HttpSession *pSession,
                               const SsiComponent *pComponent,
                               SsiRuntime *pRuntime);

    static int processEcho(HttpSession *pSession,
                           const SsiComponent *pComponent);

    static int processExec(HttpSession *pSession,
                           const SsiComponent *pComponent);

    static int processFileAttr(HttpSession *pSession,
                               const SsiComponent *pComponent);

    static int processInclude(HttpSession *pSession,
                              const SsiComponent *pComponent);

    static int processPrintEnv(HttpSession *pSession);

    static int processSet(HttpSession *pSession,
                          const SsiComponent *pComponent);

    static int processSubReq(HttpSession *pSession, SubstItem *pItem);

    static int executeComponent(HttpSession *pSession,
                                const SsiComponent *pComponent);

    static int execute(HttpSession *pSession);

    static int endExecute(HttpSession *pSession);

    static int evalOperator(HttpSession *pSession, ExprToken *&pTok);
    static int evalExpr(HttpSession *pSession, SubstItem *pItem);

    static int processConditional(HttpSession *pSession,
                                  const SsiComponent *pComponent);
    static int pushBlock(HttpSession *pSession,
                         const SsiComponent *pComponent);

    static int toLocalAbsUrl(HttpSession *pSession, const char *pRelUrl,
                             int urlLen, char *pAbsUrl, int absLen);
    static int startSubSession(HttpSession *pSession, const char *pURI,
                               int uriLen, const char *pQS, int qsLen);
    static int processSubSessionRet(int ret, HttpSession *pSession,
                                    HttpSession *pSubSession);
    static int startSubSession(HttpSession *pSession,
                               struct lsi_subreq_s *pSubSessionInfo);

};

#endif
