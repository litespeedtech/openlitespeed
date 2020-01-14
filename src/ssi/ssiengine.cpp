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
#include "ssiengine.h"

#include <ls.h>
#include "ssiscript.h"
#include "ssiruntime.h"
#include "ssiconfig.h"

#include <http/handlertype.h>
#include <http/httpcgitool.h>
#include <http/httpmethod.h>
#include <http/httpsession.h>
#include <http/httpstatuscode.h>
#include <http/requestvars.h>
#include <log4cxx/logger.h>
#include <lsr/ls_fileio.h>
#include <util/httputil.h>
#include <util/ienv.h>

#include <ctype.h>
#include <stdio.h>

SsiEngine::SsiEngine()
    : HttpHandler(HandlerType::HT_SSI)
{
}


SsiEngine::~SsiEngine()
{
}


const char *SsiEngine::getName() const
{
    return "ssi";
}


void SsiEngine::printError(HttpSession *pSession, const char *pError)
{
    SsiRuntime *pRuntime = pSession->getSsiRuntime();
    int len;
    if (!pError)
    {
        pError = pRuntime->getConfig()->getErrMsg()->c_str();
        len = pRuntime->getConfig()->getErrMsg()->len();
    }
    else
        len = strlen(pError);

    pSession->appendDynBody(pError, len);
}


int SsiEngine::beginExecute(HttpSession *pSession,
                            const SsiScript *pScript)
{
    if (!pScript)
        return SC_500;
    SsiRuntime *pRuntime = pSession->getSsiRuntime();
    if (!pRuntime)
    {
        if (pSession->setupSsiRuntime() == -1)
            return SC_500;

        SsiRuntime *pRuntime = pSession->getSsiRuntime();
        HttpReq *pReq = pSession->getReq();
        char ct[] = "text/html";
        pSession->getResp()->reset();
        //pSession->getResp()->prepareHeaders( pReq );
        //pSession->setupChunkOS( 0 );
        if (!pSession->getResp()->getRespHeaders()
            .isHeaderSet(HttpRespHeaders::H_CONTENT_TYPE))
            HttpCgiTool::processContentType( pSession, ct , 9);
        //pSession->setupRespCache();
        if (pReq->isXbitHackFull() || pRuntime->getConfig()->isLastModOn())
            pSession->getResp()->appendLastMod(pReq->getLastMod());
        int status = pReq->getStatusCode();
        if ((status >= SC_300) && (status < SC_400))
        {
            if (pReq->getLocation() != NULL)
                pSession->addLocationHeader();
        }
        if (!pSession->isNoRespBody())
        {
            if (pSession->setupRespBodyBuf() != -1)
                pSession->setupGzipFilter();
        }
        pReq->andGzip(~GZIP_ENABLED);    //disable GZIP
    }
    else
        pSession->setupRespBodyBuf();
    pSession->prepareSsiStack(pScript);

    return execute(pSession);






//     if (!pRuntime)
//     {
//         char ct[] = "text/html";
//         HttpReq *pReq = pSession->getReq();
//         pRuntime = new SsiRuntime();
//         if (!pRuntime)
//             return SC_500;
//         pRuntime->init();
//         pRuntime->initConfig(pReq->getSsiConfig());
//         pReq->setSsiRuntime(pRuntime);
//         pSession->getResp()->reset();
//         //pSession->getResp()->prepareHeaders( pReq );
//         //pSession->setupChunkOS( 0 );
//         HttpCgiTool::processContentType(pReq, pSession->getResp(),
//                                         ct , 9);
// //        pSession->setupRespCache();
//         if (pReq->isXbitHackFull())
//             pSession->getResp()->appendLastMod(pReq->getLastMod());
//         int status = pReq->getStatusCode();
//         if ((status >= SC_300) && (status < SC_400))
//         {
//             if (pReq->getLocation() != NULL)
//                 pSession->addLocationHeader();
//         }
//         pSession->setupGzipFilter();
//         pReq->andGzip(~GZIP_ENABLED);    //disable GZIP
//     }
//     if (pRuntime->push(pScript) == -1)
//         return SC_500;
//     pSession->getReq()->backupPathInfo();
//     pScript->resetRuntime();
//     return resumeExecute(pSession);
}


int SsiEngine::updateSsiConfig(HttpSession *pSession,
                               const SsiComponent *pComponent,
                               SsiRuntime *pRuntime)
{
    SubstItem *pItem = (SubstItem *)pComponent->getFirstAttr();
    char achBuf[4096];
    char *p;
    int len;
    int attr;
    while (pItem)
    {
        attr = pItem->getSubType();
        p = achBuf;
        len = 4096;
        switch (attr)
        {
        case SSI_ATTR_ECHOMSG:
        case SSI_ATTR_ERRMSG:
        case SSI_ATTR_TIMEFMT:
            RequestVars::appendSubst(pItem, pSession, p, len, 0, NULL);
            if (attr == SSI_ATTR_ERRMSG)
                pRuntime->getConfig()->setErrMsg(achBuf, p - achBuf);
            else if (attr == SSI_ATTR_ECHOMSG)
                pRuntime->getConfig()->setEchoMsg(achBuf, p - achBuf);
            else
                pRuntime->getConfig()->setTimeFmt(achBuf, p - achBuf);
            break;
        case SSI_ATTR_SIZEFMT:
            if (pItem->getType() == REF_STRING)
            {
                const AutoStr2 *pStr = pItem->getStr();
                if ((pStr) && (pStr->c_str()))
                    pRuntime->getConfig()->setSizeFmt(pStr->c_str(), pStr->len());
            }
            break;

        }

        pItem = (SubstItem *) pItem->next();
    }
    return 0;
}


int SsiEngine::processEcho(HttpSession *pSession,
                           const SsiComponent *pComponent)
{
    SubstItem *pItem = (SubstItem *)pComponent->getFirstAttr();
    char achBuf1[8192];
    char achBuf[40960];
    char *p;
    int len;
    int attr;
    int encode = SSI_ATTR_ENC_ENTITY;
    while (pItem)
    {
        attr = pItem->getSubType();
        p = achBuf1;
        len = 8192;
        switch (attr)
        {
        case SSI_ATTR_ECHO_VAR:
            if ((pItem->getType() == REF_DATE_LOCAL) ||
                (pItem->getType() == REF_LAST_MODIFIED) ||
                (pItem->getType() == REF_DATE_GMT))
                memccpy(p, pSession->getSsiRuntime()
                        ->getConfig()->getTimeFmt()->c_str(), 0, 4096);
            RequestVars::appendSubst(pItem, pSession, p, len,
                                     0, pSession->getSsiRuntime()->getRegexResult());
            if (encode == SSI_ATTR_ENC_URL)
            {
                len = HttpUtil::escape(achBuf1, p - achBuf1,
                                       achBuf, 40960);
                p = achBuf;
            }
            else if (encode == SSI_ATTR_ENC_ENTITY)
            {
                len = HttpUtil::escapeHtml(achBuf1, p, achBuf, 40960);
                p = achBuf;
            }
            else
            {
                len = p - achBuf1;
                p = achBuf1;
            }
            if (len > 0)
                pSession->appendDynBody(p, len);
            break;
        case SSI_ATTR_ENC_NONE:
        case SSI_ATTR_ENC_URL:
        case SSI_ATTR_ENC_ENTITY:
            encode = attr;
            break;
        }

        pItem = (SubstItem *) pItem->next();
    }
    return 0;
}


int SsiEngine::processExec(HttpSession *pSession,
                           const SsiComponent *pComponent)
{
    if (pSession->getReq()->isIncludesNoExec())
    {
        //Notice: Exec from server side include is disabled.
        return 0;
    }
    SubstItem *pItem = (SubstItem *)pComponent->getFirstAttr();
    return processSubReq(pSession, pItem);

}


int SsiEngine::processFileAttr(HttpSession *pSession,
                               const SsiComponent *pComponent)
{
    SubstItem *pItem = (SubstItem *)pComponent->getFirstAttr();
    if (!pItem)
        return 0;
    int len;
    int attr = pItem->getSubType();
    char achBuf[4096];
    char *p = achBuf;
    len = 4096;
    if ((attr != SSI_ATTR_INC_FILE) && (attr != SSI_ATTR_INC_VIRTUAL))
        return 0;
    SsiRuntime *pRuntime = pSession->getSsiRuntime();
    RequestVars::appendSubst(pItem, pSession, p, len,
                             0, pRuntime->getRegexResult());
    {
        if (achBuf[0] == 0)
        {
            pSession->appendDynBody(pRuntime->getConfig()->getErrMsg()->c_str(),
                                    pRuntime->getConfig()->getErrMsg()->len());
//             len = snprintf(achBuf, 4096,
//                            "[an error occurred while processing this directive]\n");
//             pSession->appendDynBody(achBuf, len);
            return 0;
        }
        HttpReq *pReq = pSession->getReq();
        const char *pURI ;
        const char *p1;
        if ((attr == SSI_ATTR_INC_FILE) || (achBuf[0] != '/'))
        {
            pURI = pReq->getRealPath()->c_str();
            p1 =  pURI + pReq->getRealPath()->len();
            while ((p1 > pReq->getRealPath()->c_str()) && p1[-1] != '/')
                --p1;
        }
        else
        {
            pURI = pReq->getDocRoot()->c_str();
            p1 = pURI + pReq->getDocRoot()->len();
        }
        int prefix_len = p1 - pURI;
        memmove(&achBuf[prefix_len], achBuf, p - achBuf);
        memmove(achBuf, pURI, prefix_len);
        p += prefix_len;
    }
    *p = 0;
    p = achBuf;
    struct stat st;
    if (ls_fio_stat(achBuf, &st) == -1)
    {
        pSession->appendDynBody("[error: stat() failed!\n", 23);
        return 0;
    }
    if (pComponent->getType() == SsiComponent::SSI_FSize)
    {
        long long size = st.st_size;
        len = snprintf(achBuf, 1024, "%lld", size);
    }
    else    //SSIComponent::SSI_Flastmod
    {
        struct tm *tm;
        tm = localtime(&st.st_mtime);
        len = strftime(achBuf, 1024, pRuntime
                       ->getConfig()->getTimeFmt()->c_str(), tm);
    }
    pSession->appendDynBody(achBuf, len);
    return 0;
}


int SsiEngine::toLocalAbsUrl(HttpSession *pSession, const char *pOrgUrl,
                             int urlLen, char *pAbsUrl, int absLen)
{
    HttpReq *pReq = pSession->getReq();
    const char *pURI = pReq->getURI();
    const char *p1 = pOrgUrl;
    if ((strncasecmp(p1, "http", 4) == 0) &&
        ((*(p1 + 4) == ':') ||
         (((*(p1 + 4) | 0x20) == 's') && (*(p1 + 5) == ':'))))
    {
        p1 += 5;
        if (*p1 == ':')
            ++p1;
        p1 += 2;
        if (strncasecmp(p1, pSession->getReq()->getHeader(HttpHeader::H_HOST),
                        pSession->getReq()->getHeaderLen(HttpHeader::H_HOST)) == 0)
        {
            p1 += pSession->getReq()->getHeaderLen(HttpHeader::H_HOST);
            if (*p1 == ':')
            {
                const char *p = p1 + 1;
                while (isdigit(*p))
                    ++p;
                if (*p == '/')
                    p1 = p;
            }
        }
        else
            return urlLen;
        memmove(pAbsUrl, p1, pOrgUrl + urlLen - p1 + 1);
        return pOrgUrl + urlLen - p1;
    }
    p1 = pURI + pReq->getURILen() - pReq->getPathInfoLen();
    while ((p1 > pURI) && p1[-1] != '/')
        --p1;
    int prefix_len = p1 - pURI;
    if (absLen <= urlLen + prefix_len)
        return -1;
    memmove(pAbsUrl + prefix_len, pOrgUrl, urlLen);
    memmove(pAbsUrl, pURI, prefix_len);
    pAbsUrl[urlLen + prefix_len] = 0;
    return urlLen + prefix_len;
}


int SsiEngine::processSubReq(HttpSession *pSession, SubstItem *pItem)
{
    char achBuf[40960];
    char *p;
    int len;
    int attr;
    const char *pQs = NULL;
    int iQsLen = 0;
    if (!pItem)
        return 0;
    SsiRuntime *pRuntime = pSession->getSsiRuntime();
    SsiStack   *pStack = pSession->getSsiStack();
    attr = pItem->getSubType();
    p = achBuf;
    len = 40960;
    switch (attr)
    {
    case SSI_ATTR_INC_FILE:
        {
            HttpReq *pReq = pSession->getReq();
            memmove(p, pReq->getURI(), pReq->getURILen());
            p = p + pReq->getURILen() - pReq->getPathInfoLen();
            while (p[-1] != '/')
                --p;
            *p = 0;
            len -= p - achBuf;
            break;
        }
    case SSI_ATTR_EXEC_CGI:
        pQs = pSession->getReq()->getQueryString();
        iQsLen = pSession->getReq()->getQueryStringLen();
        pStack->requireCGI();
        break;
    case SSI_ATTR_EXEC_CMD:
        pStack->requireCmd();
        p += snprintf(achBuf, 40960, "%s",
                      pStack->getScript()->getPath()->c_str());
        while (p[-1] != '/')
            --p;
        *p++ = '&';
        *p++ = ' ';
        *p++ = '-';
        *p++ = 'c';
        *p++ = ' ';
        len -= p - achBuf;
        // make the command looks like "/script/path/& command"
        // '&' tell cgid to execute it as shell command
        break;
    }
    RequestVars::appendSubst(pItem, pSession, p, len,
                             0, pRuntime->getRegexResult());
    *p = 0;
    if (attr == SSI_ATTR_INC_FILE)
    {
        if (strstr(achBuf, "/../") != NULL)

            return 0;
    }
    else if (attr == SSI_ATTR_EXEC_CMD)
    {

        if (pSession->execExtCmd(achBuf, p - achBuf) == 0)
            return -2;
        else
            return 0;
        //len = snprintf( achBuf, 40960, "'exec cmd' is not available, "
        //            "use 'include virutal' instead.\n" );
        //pSession->appendDynBody( achBuf, len );

        //return 0;
    }
    if ((achBuf[0] != '/')) // && (attr != SSI_ATTR_EXEC_CMD))
    {
        if (achBuf[0] == 0)
            len = -1;
        else
        {
            len = toLocalAbsUrl(pSession, achBuf, p - achBuf, achBuf, sizeof(achBuf));
            if (len >= 0)
            {
                p = &achBuf[len];
                *p = 0;
            }
        }
        if (len == -1)
        {
            pSession->appendDynBody(pRuntime->getConfig()->getErrMsg()->c_str(),
                                    pRuntime->getConfig()->getErrMsg()->len());
            return 0;
        }
    }
    if (achBuf[0] == '/')
        return startSubSession(pSession, achBuf, p - achBuf, pQs, iQsLen);
    else
    {
        LS_INFO(pSession->getLogSession(),
                "Cannot include None-local URL or configured proxy backend: %s",
                achBuf);
        printError(pSession, NULL);
    }
    return 0;
}


int SsiEngine::startSubSession(HttpSession *pSession,
                               const char *pUri, int uriLen,
                               const char *pQs, int qsLen)
{
    lsi_subreq_t subSessionInfo;
    memset(&subSessionInfo, 0, sizeof(lsi_subreq_t));
    subSessionInfo.m_pUri   = pUri;
    subSessionInfo.m_uriLen = uriLen;
    subSessionInfo.m_pQs    = pQs;
    subSessionInfo.m_qsLen  = qsLen;
    subSessionInfo.m_method = HttpMethod::HTTP_GET;
    subSessionInfo.m_flag |= SUB_REQ_SETREFERER;

    return startSubSession(pSession, &subSessionInfo);
}


int SsiEngine::startSubSession(HttpSession *pSession,
                               lsi_subreq_t *pSubSessionInfo)
{
    HttpSession *pSubSession = pSession->newSubSession(pSubSessionInfo);
    if (!pSubSession)
    {
        printError(pSession, NULL);
        return -1;
    }

    pSubSession->getStream()->setFlag(HIO_FLAG_PASS_SETCOOKIE, 1);
    pSubSession->setSsiRuntime(pSession->getSsiRuntime());
    pSubSession->setFlag(HSF_NO_ERROR_PAGE);
    int ret = pSubSession->execSubSession();
    return processSubSessionRet(ret, pSession, pSubSession);
}


int SsiEngine::processInclude(HttpSession *pSession,
                              const SsiComponent *pComponent)
{
    SubstItem *pItem = (SubstItem *)pComponent->getFirstAttr();
    if (pSession->getSsiStack()->getDepth() > 9)
    {
        LS_DBG_H(pSession->getLogSession(),
                 "maximum include depth 10 reached, skip."
                  );
        printError(pSession, NULL);
        return 0;
    }
    return processSubReq(pSession, pItem);
}


class SsiEnv : public IEnv
{
    HttpSession *m_pSession;


    SsiEnv(const SsiEnv &rhs);
    void operator=(const SsiEnv &rhs);
public:
    SsiEnv(HttpSession *pSession) : m_pSession(pSession)    {};
    ~SsiEnv()   {};
    int add(const char *name, const char *value)
    {   return IEnv::add(name, value);    }

    int add(const char *name, size_t nameLen,
            const char *value, size_t valLen);
    int add(const char *buf, size_t len);
    void clear()    { }
    int addVar(int var_id);
};


int SsiEnv::add(const char *name, size_t nameLen,
                const char *value, size_t valLen)
{
    char achBuf[40960];
    char *p = achBuf;
    if (!name)
        return 0;
    int len = HttpUtil::escapeHtml(name, name + nameLen, p, 40960);
    p += len;
    *p++ = '=';
    len = HttpUtil::escapeHtml(value, value + valLen, p, &achBuf[40960] - p);
    p += len;
    *p++ = '\n';
    m_pSession->appendDynBody(achBuf, p - achBuf);
    return 0;
}


int SsiEnv::add(const char *buf, size_t len)
{
    char achBuf[40960];
    int ret = HttpUtil::escapeHtml(buf, buf + len, achBuf, 40960);
    achBuf[ret] = '\n';
    m_pSession->appendDynBody(achBuf, ret + 1);
    return 0;
}


int SsiEnv::addVar(int var_id)
{
    char achBuf[4096];
    const char *pName;
    int nameLen;
    pName = RequestVars::getVarNameStr(var_id, nameLen);
    if (!pName)
        return 0;
    char *pValue = achBuf;
    memccpy(pValue, m_pSession->getSsiRuntime()
            ->getConfig()->getTimeFmt()->c_str(), 0, 4096);
    int valLen = RequestVars::getReqVar(m_pSession, var_id, pValue, 4096);
    return add(pName, nameLen, pValue, valLen);
}


int SsiEngine::processPrintEnv(HttpSession *pSession)
{
    SsiEnv env(pSession);

    HttpCgiTool::buildEnv(&env, pSession);
    env.addVar(REF_DATE_GMT);
    env.addVar(REF_DATE_LOCAL);
    env.addVar(REF_DOCUMENT_NAME);
    env.addVar(REF_DOCUMENT_URI);
    env.addVar(REF_LAST_MODIFIED);
    env.addVar(REF_QS_UNESCAPED);
    return 0;
}


int SsiEngine::processSet(HttpSession *pSession,
                          const SsiComponent *pComponent)
{
    SubstItem *pItem = (SubstItem *)pComponent->getFirstAttr();
    const AutoStr2 *pVarName;
    char achBuf1[8192];
    char *p;
    int len;
    int attr;
    while (pItem)
    {

        attr = pItem->getSubType();
        if (attr == SSI_ATTR_SET_VAR)
        {
            pVarName = pItem->getStr();
            pItem = (SubstItem *) pItem->next();
            if (!pItem)
                break;
            if (pItem->getSubType() != SSI_ATTR_SET_VALUE)
                continue;
            p = achBuf1;
            len = 8192;
            len = RequestVars::appendSubst(pItem, pSession, p, len, 0,
                                           pSession->getSsiRuntime()->getRegexResult(),
                                           pSession->getSsiRuntime()
                                           ->getConfig()->getTimeFmt()->c_str());
            pSession->getSsiRuntime()->setVar(pVarName->c_str(), pVarName->len(),
                                              achBuf1, p - achBuf1);
//             RequestVars::setEnv(pSession, pVarName->c_str(), pVarName->len(), achBuf1,
//                                 p - achBuf1);
        }

        pItem = (SubstItem *) pItem->next();
    }
    return 0;
}


int SsiEngine::appendLocation(HttpSession *pSession, const char *pLocation,
                              int len)
{
    char achBuf[40960];
    char *p = &achBuf[9];
    memcpy(achBuf, "<A HREF=\"", 9);
    memmove(p, pLocation, len);
    p += len;
    *p++ = '"';
    *p++ = '>';
    memmove(p, pLocation, len);
    p += len;
    memmove(p, "</A>", 4);
    p += 4;
    pSession->appendDynBody(achBuf, p - achBuf);
    return 0;
}


static int  shortCurcuit(ExprToken *&pTok)
{
    if (!pTok)
        return 0;
    int type = pTok->getType();
    pTok = pTok->next();
    if (!pTok)
        return 0;
    switch (type)
    {
    case ExprToken::EXP_STRING:
    case ExprToken::EXP_FMT:
    case ExprToken::EXP_REGEX:
        break;
    case ExprToken::EXP_AND:
    case ExprToken::EXP_OR:
        shortCurcuit(pTok);
    //Fall through
    case ExprToken::EXP_NOT:
        shortCurcuit(pTok);
        break;
    case ExprToken::EXP_EQ:
    case ExprToken::EXP_NE:
    case ExprToken::EXP_LE:
    case ExprToken::EXP_LESS:
    case ExprToken::EXP_GE:
    case ExprToken::EXP_GREAT:
        pTok = pTok->next();
        if (pTok)
            pTok = pTok->next();
        break;
    }
    return 1;
}


static int compString(HttpSession *pSession, int type, ExprToken *&pTok)
{
    int     ret;
    char achBuf2[40960] = "";
    char achBuf1[40960] = "";
    int     len = sizeof(achBuf1);
    char   *p1 = achBuf1;
    char   *p2 = achBuf2;
    if (pTok->getType() == ExprToken::EXP_STRING)
    {
        p1 = (char *)pTok->getStr()->c_str();
        len = pTok->getStr()->len();
    }
    else if (pTok->getType() == ExprToken::EXP_FMT)
    {
        p1 = achBuf1;
        len = 40960;
        RequestVars::buildString(pTok->getFormat(), pSession, p1, len, 0,
                                 pSession->getSsiRuntime()->getRegexResult());
        p1 = achBuf1;
    }
    pTok = pTok->next();
    if (!pTok)
        return 0;
    if (pTok->getType() == ExprToken::EXP_REGEX)
    {
        ret = pSession->getSsiRuntime()->execRegex(pTok->getRegex(), p1,
                len);
        if (ret == 0)
            ret = 10;
        if (ret == -1)
            ret = 0;
        if (type == ExprToken::EXP_NE)
            ret = !ret;
        pTok = pTok->next();
        return ret;
    }

    if (pTok->getType() == ExprToken::EXP_STRING)
        p2 = (char *)pTok->getStr()->c_str();
    else if (pTok->getType() == ExprToken::EXP_FMT)
    {
        p2 = achBuf2;
        len = 40960;
        RequestVars::buildString(pTok->getFormat(), pSession, p2, len, 0,
                                 pSession->getSsiRuntime()->getRegexResult());
        p2 = achBuf2;
    }
    pTok = pTok->next();
    ret = strcmp(p1, p2);
    switch (type)
    {
    case ExprToken::EXP_EQ:
        return (ret == 0);
    case ExprToken::EXP_NE:
        return (ret != 0);
    case ExprToken::EXP_LE:
        return (ret <= 0);
    case ExprToken::EXP_LESS:
        return (ret < 0);
    case ExprToken::EXP_GE:
        return (ret >= 0);
    case ExprToken::EXP_GREAT:
        return (ret > 0);
    }
    return 0;
}


int SsiEngine::evalOperator(HttpSession *pSession, ExprToken *&pTok)
{
    char   *p1 = NULL;
    int     len;
    int     ret;
    if (!pTok)
        return 0;
    int type = pTok->getType();
    ExprToken *pCur = pTok;
    pTok = pTok->next();
    switch (type)
    {
    case ExprToken::EXP_STRING:
        return (pCur->getStr()->len() > 0);
    case ExprToken::EXP_FMT:
        {
            char achBuf1[40960];
            p1 = achBuf1;
            len = 40960;
            RequestVars::buildString(pCur->getFormat(), pSession, p1, len, 0,
                                     pSession->getSsiRuntime()->getRegexResult());
            return len > 0;
        }
    case ExprToken::EXP_REGEX:
        return 0;
    case ExprToken::EXP_NOT:
        if (pTok)
            return !evalOperator(pSession, pTok);
        else
            return 1;
    default:
        break;
    }

    if ((type == ExprToken::EXP_AND) ||
        (type == ExprToken::EXP_OR))
    {
        ret = evalOperator(pSession, pTok);
        if (!pTok)
            return 0;
        if (((ret) && (type == ExprToken::EXP_OR)) ||
            ((!ret) && (type == ExprToken::EXP_AND)))
            shortCurcuit(pTok);
        else
            ret = evalOperator(pSession, pTok);
        return ret;
    }
    if (!pTok)
        return 0;
    return compString(pSession, type, pTok);
}


int SsiEngine::evalExpr(HttpSession *pSession, SubstItem *pItem)
{
    int ret = 0;
    if ((!pItem) || (pItem->getType() != REF_EXPR))
        return 0;
    Expression *pExpr = (Expression *)pItem->getAny();
    ExprToken *pTok = pExpr->begin();
    ret = evalOperator(pSession, pTok);
    return ret;
}


int SsiEngine::pushBlock(HttpSession *pSession,
                         const SsiComponent *pComponent)
{
    const SsiBlock *pBlock = (const SsiBlock *)pComponent;
    pSession->getSsiStack()->setCurrentBlock(pBlock);
    return 0;
}


int SsiEngine::processConditional(HttpSession *pSession,
                                  const SsiComponent *pComponent)
{
    SubstItem *pItem = (SubstItem *)pComponent->getFirstAttr();
    int ret = evalExpr(pSession, pItem);
    if (ret)
        pushBlock(pSession, pComponent);
    return 0;
}


int SsiEngine::processSubSessionRet(int ret, HttpSession *pSession,
                                    HttpSession *pSubSession)
{
    if (ret)
    {
        //Got error in processing code
        //generate error response
        printError(pSession, NULL);
        pSession->closeSubSession(pSubSession);
        return 0;
    }
    else
    {
        pSession->attachSubSession(pSubSession);
        if (pSubSession->getState() == HSS_COMPLETE)
        {
            pSession->closeSubSession(pSubSession);
            return 0;
        }
    }
    return -2;

}


int SsiEngine::executeComponent(HttpSession *pSession,
                                const SsiComponent *pComponent)
{
    int ret;

    LS_DBG_H(pSession->getLogSession(), "SSI Process component: %d",
             pComponent->getType());

    switch (pComponent->getType())
    {
    case SsiComponent::SSI_String:
        pSession->appendDynBody(pSession->getSsiStack()->getScript()->getContent(),
                pComponent->getContentOffset(), pComponent->getContentLen());
        break;
    case SsiComponent::SSI_Config:
        updateSsiConfig(pSession, pComponent, pSession->getSsiRuntime());
        break;
    case SsiComponent::SSI_Echo:
        processEcho(pSession, pComponent);
        break;
    case SsiComponent::SSI_Exec:
        ret = processExec(pSession, pComponent);
        return ret;
        break;
    case SsiComponent::SSI_FSize:
    case SsiComponent::SSI_Flastmod:
        processFileAttr(pSession, pComponent);
        break;
    case SsiComponent::SSI_Include:
        ret = processInclude(pSession, pComponent);
        return ret;
        break;
    case SsiComponent::SSI_Printenv:
        processPrintEnv(pSession);
        break;
    case SsiComponent::SSI_Set:
        processSet(pSession, pComponent);
        break;
    case SsiComponent::SSI_Switch:
        pushBlock(pSession, pComponent);
        break;
    case SsiComponent::SSI_If:
    case SsiComponent::SSI_Elif:
        processConditional(pSession, pComponent);
        break;
    case SsiComponent::SSI_Else:
        pushBlock(pSession, pComponent);
        break;

    }
    return 0;
}


int SsiEngine::endExecute(HttpSession *pSession)
{
    pSession->releaseSsiRuntime();
    return 0;
}


int SsiEngine::resumeExecute(HttpSession *pSession)
{
    SsiStack *pStack = pSession->getSsiStack();
    if (pSession->getSsiRuntime() && pSession->getSsiRuntime()->isInSsiEngine())
        return 0;
    pStack->nextComponentOfCurScript();
    return execute(pSession);
    //TODO: redo this when I get to it.  Left pRuntime so I have a warning here.
//     SsiRuntime *pRuntime = pSession->getSsiRuntime();
//     int ret = 0;
//     if (!pRuntime)
//         return LS_FAIL;
//     SsiComponent *pComponent;
//     while (!pRuntime->done())
//     {
//         SsiScript *pScript = pRuntime->getCurrentScript();
//         pRuntime->clearFlag();
//         pComponent = pScript->nextComponent();
//         if (!pComponent)
//         {
//             pRuntime->pop();
//             if (!pRuntime->done())
//                 pSession->getReq()->restorePathInfo();
//             continue;
//         }
//         ret = executeComponent(pSession, pComponent);
//         if (ret == -2)
//             return 0;
//     }
//     endExecute(pSession);
//     return pSession->endResponse(1);

}


int SsiEngine::execute(HttpSession *pSession)
{
    SsiStack *pStack = pSession->getSsiStack();
    int ret = 0;
    if (!pStack)
        return -1;
    if (!pSession->isNoRespBody())
    {
        pSession->getSsiRuntime()->incInSsiEngine();
        const SsiComponent *pComponent = pStack->getCurrentComponent();
        while (pComponent || pStack->getPrevious())
        {
            if (pComponent)
            {
                pStack->clearFlag();
                ret = executeComponent(pSession, pComponent);
                if (ret == -2 || pSession->getSsiRuntime() == NULL)
                {
                    if (pSession->getSsiRuntime())
                        pSession->getSsiRuntime()->decInSsiEngine();
                    return 0;
                }
                pStack = pSession->getSsiStack();
                pComponent = pStack->nextComponentOfCurScript();
            }
            while (!pComponent && pStack->getPrevious())
            {
                pStack = pSession->popSsiStack();
                if (pStack)
                    pComponent = pStack->nextComponentOfCurScript();
            }
        }
        if (pSession->getSsiRuntime())
            pSession->getSsiRuntime()->decInSsiEngine();
    }
    endExecute(pSession);
    pSession->endResponse(1);
    return 0;
}
