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
#include <http/httpsession.h>
#include <http/hiochainstream.h>
#include <http/httpdefs.h>
#include <http/httpmethod.h>
#include <http/httpresourcemanager.h>
#include <http/httpvhost.h>
#include <http/httpstatuscode.h>
#include <ssi/ssiruntime.h>

#include <log4cxx/logger.h>
#include <util/datetime.h>


//subrequest mode
//  1. background mode, we do not care the result, result is discarded
//  2. capture mode, result is captured, save in its buffer,
//     we can use API to retrieve it. for example, LUA issue subrequest
//     need to access response headers.
//  3. inline mode, result is directly incorperated into current response
//     SSI, ESI does that

//Multiple parallel requests
//  1. in background mode, do not care, just let individual request run
//  2. capture mode, at the end of each request, need to notify the owner of subrequest
//     through callback functions, the request should not be garbage collected
//     unless the owner finished with it
//  3. inline mode is implemented as special capture mode, callback functions
//     to combine them into the main response.


int HttpSession::detectLoopSubSession(lsi_subreq_t *pSubSessInfo)
{
    //check if parent subsession has the same URL,
    //check if parent subsession has same redirect URL.
    if (m_request.detectLoopRedirect(1, pSubSessInfo->m_pUri,
                    pSubSessInfo->m_uriLen, pSubSessInfo->m_pQs,
                    pSubSessInfo->m_qsLen))
        return 1;
    if (m_pParent)
        return m_pParent->detectLoopSubSession(pSubSessInfo);

    return 0;
}


HttpSession *HttpSession::newSubSession(lsi_subreq_t *pSubSessInfo)
{
    //for subsession, we intentionally turn off
    //   keepalive
    //   chunk
    //   gzip compression
    //   security

    //detect loop subsessions
    int depth = 1;
    if (m_pParent)
    {
        depth = 1 + ((HioChainStream *)getStream())->getDepth();
        if (depth > 9)
            return NULL;
    }

    if (detectLoopSubSession(pSubSessInfo) == 1)
        return NULL;

    HttpSession *pSession = HttpResourceManager::getInstance().getConnection();
    if (! pSession)
        return NULL;

    pSession->reset();
    pSession->m_lReqTime = DateTime::s_curTime;
    pSession->m_iReqTimeUs = DateTime::s_curTimeUs;
    pSession->m_pClientInfo = m_pClientInfo;
    pSession->m_iRemotePort = m_iRemotePort;
    pSession->getReq()->setCrypto(m_request.getCrypto());

    //pSession->setSsiRuntime( m_pSsiRuntime );
    HioChainStream *pStream = new HioChainStream();
    pSession->attachStream(pStream);
    pSession->getReq()->setILog(pStream);

    pStream->setDepth(depth);

    pSession->getReq()->setMethod(pSubSessInfo->m_method);
    if (pSession->getReq()->clone(getReq(), pSubSessInfo))
    {
        pStream->setHandler(NULL);
        delete pStream;
        HttpResourceManager::getInstance().recycle(pSession);
        return NULL;
    }

    pSession->setFlag(HSF_SUB_SESSION);
    pSession->getResp()->reset();

    const char *pSrcQs = pSubSessInfo->m_pQs;
    LS_DBG_M(getLogSession(), "Create SUB SESSION: %d, flag: %d, "
             "method: %s, URI: %s, len: %d, QS: %s, len: %d\n",
             m_iSubReqSeq, pSubSessInfo->m_flag,
             HttpMethod::get(pSubSessInfo->m_method),
             pSubSessInfo->m_pUri,
             pSubSessInfo->m_uriLen,
             pSrcQs ? pSrcQs : "",
             pSubSessInfo->m_qsLen);
    HttpSession *parent = NULL;
    if (pSubSessInfo->m_flag & SUB_REQ_DETACHED)
    {
        pStream->setParentSession(NULL);
        //set Stream to black hole mode,
        pStream->setFlag(HIO_FLAG_BLACK_HOLE, 1);
        //parent = getBackGroundSession();

    }
    else
    {
        pStream->setSequence(m_iSubReqSeq++);

        //Attach stream to parant session may cause trouble?
        pStream->setParentSession(this);
        parent = this;
    }

    if (pSubSessInfo->m_flag & SUB_REQ_NOABORT)
        pSession->setFlag(HSF_NO_ABORT, 1);

    pSession->m_pParent = parent;

    return pSession;
}

int HttpSession::setReqBody(const char *pBodyBuf, int len)
{
    //reset request body buffer
    //append
    //adjust content length
    //should bypass the request body related hookpoint, recv_req_body filter.

    return -1;
}


int HttpSession::processSubReq()
{
    int ret;
    m_lReqTime = DateTime::s_curTime;
    m_iReqTimeUs = DateTime::s_curTimeUs;
    m_lDynBodySent = 0;

    const HttpVHost *pVHost = m_request.getVHost();
    if (!pVHost)
        m_request.matchVHost();
    else
        ((HttpVHost *)pVHost)->incRef();
    if (!pVHost)
    {
        LS_DBG_L(getLogSession(), "can not find a matching VHost for [%.*s]",
                 m_request.getHostStrLen(), m_request.getHostStr());
        return SC_404;
    }
    getStream()->setLogger(pVHost->getLogger());
    if (getStream()->isLogIdBuilt())
    {
        getStream()->lockAddOrReplaceFrom('#', pVHost->getName());
    }

    ret = m_request.processNewReqData(getPeerAddr());
    if (ret)
        return ret;

    m_request.setStatusCode(SC_200);
    LS_DBG_M(getLogSession(),
             "processSubReq(): set state to HSPS_HKPT_HTTP_BEGIN");
    m_processState = HSPS_HKPT_HTTP_BEGIN;
    return 0;
}


int HttpSession::execSubSession()
{
    //pause current session
    //assign current stream to the subsession stream

    LS_DBG_M(getLogSession(), "Execute SUB SESSION: %d",
             ((HioChainStream *)getStream())->getSequence());

    //init sub session data
    int ret = processSubReq();
    if (!ret)
        ret = smProcessReq();
    else
        ret = httpError(ret);
    return ret;
}

int HttpSession::attachSubSession(HttpSession *pSubSess)
{
    HioChainStream *pStream = (HioChainStream *)pSubSess->getStream();
    pStream->setParentSession(this);
    pSubSess->getResp()->setContentLen(LSI_BODY_SIZE_UNKNOWN);
    pStream->setFlag(HIO_FLAG_PASS_THROUGH, 1);
    m_pCurSubSession = pSubSess;
    if (pStream->isWantWrite())
        wantWrite(1);

    LS_DBG_M(getLogSession(), "Attach SUB SESSION: %d",
             pStream->getSequence());

    return processSubSessionResponse(pSubSess);
}

int HttpSession::detachSubSession(HttpSession *pSubSess)
{
    HioChainStream *pStream = (HioChainStream *)pSubSess->getStream();
    LS_DBG_M(getLogSession(), "Detach SUB SESSION: %d",
             pStream->getSequence());
    pStream->setParentSession(NULL);
    //set Stream to black hole mode,
    pStream->setFlag(HIO_FLAG_BLACK_HOLE, 1);

    // remove from current sub session list;
    pSubSess->m_pParent = NULL;
    // add this session to global orphan session list


    return 0;
}

int HttpSession::includeSubSession()
{
    int ret;
    if (m_pCurSubSession)
    {
        LS_DBG_M(getLogSession(), "Include response from SUB SESSION: %d",
                 ((HioChainStream *)m_pCurSubSession->getStream())->getSequence());
        ret = m_pCurSubSession->onWriteEx();
        if (ret != 0)
            return ret;

        if (m_iFlag & HSF_AIO_READING)
        {
            wantWrite(0);
            return 0;
        }

//         if (m_pCurSubSession->m_iFlag & HSF_RESP_DONE)
//             onSubSessionRespIncluded(m_pCurSubSession);
    }
    return 0;
}

int HttpSession::processSubSessionResponse(HttpSession *pSubSess)
{
    if (includeSubSession())
    {
    }
    //doWrite();
    return 0;
}

int HttpSession::onSubSessionEndResp(HttpSession *pSubSess)
{
    LS_DBG_M(getLogSession(), "onSubSessionEndResp() SUB SESSION: %d",
             ((HioChainStream *)pSubSess->getStream())->getSequence());
    //Sub Session finished handler response processing
    // the response body buffer should have full response body
    // before any outgoing transformation applied.

    //should we apply the outgoing transformation in order to get the final result?

    //For hookpoint, need to locate the callback, continue the hook chain.
    if (pSubSess->m_pSubSessionCb)
        (*pSubSess->m_pSubSessionCb)(pSubSess, pSubSess->m_pSubSessionCbParam, 0);
    //make sure subsession result has been incorperated into current session,
    // current session could be a subsession as well, so, need to

    if (m_iFlag & HSF_WAIT_SUBSESSION)
    {
        //Look up sub session list, if current session is the
        attachSubSession(pSubSess);
    }

    if (pSubSess != m_pCurSubSession)
    {
        //not current sub session, need to wait till its turn to
        //be processed,
        // Or, we call associated callback to process the result
        // subrequest started by LUA or other module.
    }

    return 0;

}


int HttpSession::onSubSessionRespIncluded(HttpSession *pSubSess)
{
    LS_DBG_M(getLogSession(), "onSubSessionRespIncluded() SUB SESSION: %d",
             ((HioChainStream *)pSubSess->getStream())->getSequence());
    //resume current session,
    //for ESI and SSI, need to find next component, resume
    //For hookpoint, need to locate the callback, continue the hook chain.
    //if ( pSubSess->m_pSubSessionCb )
    //{
    //   (*pSubSess->m_pSubSessionCb)( pSubSess, this, pSubSess->m_pSubSessCbParam);
    //
    //}
    //make sure subsession result has been incorperated into current session,
    // current session could be a subsession as well, so, need to


    if (pSubSess == m_pCurSubSession)
    {
        curSubSessionCleanUp();
//         m_iFlag |= HSF_CUR_SUB_SESSION_DONE;
//         wantWrite(1);

    }
    else
    {
        //not current sub session, need to wait till its turn to
        //be processed,
        // Or, we call associated callback to process the result
        // subrequest started by LUA or other module.
    }

    return 0;

}

int HttpSession::curSubSessionCleanUp()
{
    m_iFlag &= ~HSF_CUR_SUB_SESSION_DONE;

    if (m_pCurSubSession)
    {
        closeSubSession(m_pCurSubSession);
        // if current session has remaining data, resume processing, like SSI ESI.
        // or multiple subrequests has been added.
        // start send out next sub request response.
        // If AIO pending, just wait AIO to finished
        if (getSsiRuntime())
        {
            if (m_sendFileInfo.getRemain() > 0)
            {
                setFlag(HSF_RESUME_SSI);
                getStream()->wantWrite(1);
                return 1;
            }
            resumeSSI();
        }
        else
        {
            //If we have multiple sub sessions, activate another one.
            endResponse(1);
        }
    }
    return 0;
}

int HttpSession::closeSubSession(HttpSession *pSubSess)
{
    LS_DBG_M(getLogSession(), "Close SUB SESSION: %d",
             ((HioChainStream *)pSubSess->getStream())->getSequence());

    pSubSess->setSsiRuntime(NULL);
    pSubSess->closeSession();
    if (pSubSess == m_pCurSubSession)
    {
        m_iFlag &= ~HSF_CUR_SUB_SESSION_DONE;
        m_pCurSubSession = NULL;
    }
    HioStream *pStream = pSubSess->detachStream();
    if (pStream)
        delete pStream;
    pSubSess->recycle();
    return 0;
}

int HttpSession::cancelSubSession(HttpSession *pSubSess)
{
    LS_DBG_M(getLogSession(), "Cancel SUB SESSION: %d",
             ((HioChainStream *)pSubSess->getStream())->getSequence());
    if (pSubSess->getFlag(HSF_NO_ABORT) & HSF_NO_ABORT)
    {
        // if sub session can not be interrupted, detach it, add it to global lingering session list
        detachSubSession(pSubSess);
        //its own sub-session wont be cleanUp until it is finished.
    }
    else
    {
        pSubSess->cleanUpSubSessions();
        //mark subsession to be stopped, cancel current handler.
        pSubSess->setState(HSS_COMPLETE);
    }
    if (pSubSess->getState() == HSS_COMPLETE)
        closeSubSession(pSubSess);
    //request sub session to stop
    //   just treat it as has been cancelled sucessfully
    //   trigger onSubsessionEnd().
    return -1;

}


void HttpSession::cleanUpSubSessions()
{
    //TODO: go through the sub session list, cleanup them all.

    if (!m_pCurSubSession)
        return;
    cancelSubSession(m_pCurSubSession);
    m_pCurSubSession = NULL;

}

void HttpSession::mergeRespHeaders(HttpRespHeaders::INDEX headerIndex,
                                   const char *pName, int nameLen,
                                   struct iovec *pIov, int count)
{
    struct iovec *pEnd = pIov + count;
    while (pIov < pEnd)
    {
        if (headerIndex == HttpRespHeaders::H_SET_COOKIE)
        {
            if (m_request.processSetCookieHeader((const char *)pIov->iov_base,
                                                 pIov->iov_len) == 0)
            {
                ++pIov;
                continue;
            }

        }
        m_response.getRespHeaders().add(
            headerIndex, pName, nameLen,
            (const char *)pIov->iov_base, pIov->iov_len,
            LSI_HEADEROP_APPEND);
        ++pIov;
    }
}


const char *HttpSession::getOrgReqUrl(int *n)
{
    HttpReq *pReq = &m_request;
    if (m_iFlag & HSF_SUB_SESSION)
    {
        if (m_pParent && m_pParent->m_pSsiStack
            && m_pParent->m_pSsiStack->getScript())
            pReq = m_pSsiRuntime->getMainReq();
    }
    *n = pReq->getOrgReqURLLen();
    return pReq->getOrgReqURL();
}


void HttpReq::setNewOrgUrl(const char *pUrl, int len, const char *pQS,
                           int qsLen)
{
    int total = len;
    if (qsLen > 0)
        total += qsLen + 1;

    if (m_headerBuf.available() < total + 2)
        m_headerBuf.grow(total + 2 - m_headerBuf.available());
    //TODO: consider URL encoding.
    char *p = m_headerBuf.end();
    *p++ = '\0';

    m_reqURLOff = p - m_headerBuf.begin();
    memmove(p, pUrl, len);
    p += len;
    if (qsLen)
    {
        if (memchr(pUrl, '?', len))
            *p++ = '&';
        else
            *p++ = '?';
        memmove(p, pQS, qsLen);
        p += qsLen;
    }
    *p = '\0';
    m_reqURLLen = total;
    m_headerBuf.used(total + 2);
    clearContextState(CACHE_KEY | CACHE_PRIVATE_KEY);
}


int HttpReq::cloneReqBody(const char *pBuf, int32_t len)
{
    //int iBodyBegin = m_headerBuf.size();
    m_lEntityLength = len;
    m_headerBuf.append(pBuf, m_lEntityLength);
    return 0;
    //return memBlockToReqBodyBuf(m_headerBuf.begin() + iBodyBegin, pBuf->size());
}


int HttpReq::clone(HttpReq *pProto, lsi_subreq_t *pSubSessInfo)
{
    int ret = 0;
    //copy request headers
    m_pVHostMap = pProto->m_pVHostMap;
    m_pVHost = pProto->m_pVHost;

    m_iHttpHeaderEnd = m_iReqHeaderBufFinished = pProto->m_iHttpHeaderEnd;
    m_headerBuf.resize(HEADER_BUF_PAD);

    int begin = m_headerBuf.size();

    m_headerBuf.append(pProto->m_headerBuf.getp(begin),
                       m_iHttpHeaderEnd - begin);

    memmove(&m_reqLineOff, &pProto->m_reqLineOff,
            (char *)(&m_reqURLLen + 1) - (char *)&m_reqLineOff);

    memmove(m_commonHeaderLen, &pProto->m_commonHeaderLen,
            (char *)&m_headerIdxOff - (char *)m_commonHeaderLen);

    m_iLeadingWWW = pProto->m_iLeadingWWW;
    m_iBodyType = pProto->m_iBodyType;
    m_lEntityLength = pProto->m_lEntityLength;
    keepAlive(pProto->isKeepAlive());
    m_iAcceptGzip = 0; //pProto->m_iAcceptGzip &
    m_iAcceptBr = 0;
    m_iRedirects = 0;
    m_iHostOff = pProto->m_iHostOff;
    m_iHostLen = pProto->m_iHostLen;

    clearContextState(LOG_ACCESS_404);


    if (pProto->m_headerIdxOff)
    {
        //copy unkown header index
    }

    if (pProto->m_iContextState & COOKIE_PARSED)
    {
        m_iContextState |= COOKIE_PARSED;
        m_cookies.copy(pProto->m_cookies, m_pPool);
    }
    if (pProto->m_commonHeaderOffset[ HttpHeader::H_COOKIE ] >
        m_iHttpHeaderEnd)
    {
        copyCookieHeaderToBufEnd(
            pProto->m_commonHeaderOffset[ HttpHeader::H_COOKIE ],
            pProto->getHeader(HttpHeader::H_COOKIE),
            pProto->getHeaderLen(HttpHeader::H_COOKIE)
        );
    }
    else if (pProto->m_commonHeaderOffset[ HttpHeader::H_COOKIE ]
             + pProto->m_commonHeaderLen[ HttpHeader::H_COOKIE ] > m_iHttpHeaderEnd)
    {
        m_headerBuf.append(pProto->m_headerBuf.getp(m_iHttpHeaderEnd),
                           pProto->m_commonHeaderOffset[ HttpHeader::H_COOKIE ]
                           + pProto->m_commonHeaderLen[ HttpHeader::H_COOKIE ]
                           + 2 - m_iHttpHeaderEnd);
    }

    if (pSubSessInfo->m_flag & SUB_REQ_SETREFERER)
        setReferer(pProto->getOrgReqURL(), pProto->getOrgReqURLLen());

    if ((pSubSessInfo->m_method == HttpMethod::HTTP_POST)
        && (pSubSessInfo->m_pReqBody != NULL))
    {
        addContentLenHeader(pSubSessInfo->m_reqBodyLen);
        updateReqHeader(HttpHeader::H_CONTENT_TYPE,
                        "application/x-www-form-urlencoded", 33);

    }

    m_iHeaderStatus = HEADER_OK;

    int totalUrlLen = pSubSessInfo->m_uriLen;
    char *p, *pURL;
    const char *pSrcUrl = pSubSessInfo->m_pUri;
    const char *pSrcQs = pSubSessInfo->m_pQs;
    int iSrcUrlLen = totalUrlLen;
    int iSrcQsLen = pSubSessInfo->m_qsLen;
    if (iSrcQsLen > 0)
        totalUrlLen += iSrcQsLen + 1;
    if (totalUrlLen <= pProto->getOrgReqURLLen())
    {
        char *pOrgEnd;
        p = pURL = m_headerBuf.getp(pProto->m_reqURLOff);
        pOrgEnd = p + pProto->m_reqURLLen;

        memmove(p, pSrcUrl, iSrcUrlLen);
        p += iSrcUrlLen;
        if ((pSrcQs != NULL) && (iSrcQsLen > 0))
        {
            if (memchr(pSrcUrl, '?', iSrcUrlLen))
                *p++ = '&';
            else
                *p++ = '?';
            memmove(p, pSrcQs, iSrcQsLen);
            p += iSrcQsLen;
        }
        while (p < pOrgEnd)
            *p++ = ' ';
        m_reqURLLen = totalUrlLen;
    }
    else
    {
        setNewOrgUrl(pSrcUrl, iSrcUrlLen, pSrcQs, iSrcQsLen);

    }
    m_iHttpHeaderEnd = m_iReqHeaderBufFinished
                       = m_headerBuf.size();

    if ((pSubSessInfo->m_method == HttpMethod::HTTP_POST)
        && (pSubSessInfo->m_pReqBody != NULL))
        cloneReqBody(pSubSessInfo->m_pReqBody, pSubSessInfo->m_reqBodyLen);

    pURL = m_headerBuf.getp(m_reqURLOff);
    ret = setCurrentURL(pURL, totalUrlLen, 1);
    if (ret == 0)
    {
        m_pUrls[0].key = m_curUrl.key;
        m_pUrls[0].val = m_curUrl.val;
    }
    return ret;
}


void HttpReq::addContentLenHeader(size_t len)
{
    char sLen[40];
    int n = snprintf(sLen, sizeof(sLen), "%ld", (unsigned long)len);
    updateReqHeader(HttpHeader::H_CONTENT_LENGTH, sLen, n);
}


void HttpReq::updateReqHeader(int index, const char *pNewValue,
                              int newValueLen)
{
    char *pOld = (char *)getHeader(index);
    int oldLen = getHeaderLen(index);
    int diff = oldLen - newValueLen;
    if (*pOld && diff >= 0)
    {
        memmove(pOld, pNewValue, newValueLen);
        if (diff > 0)
            memset(pOld + newValueLen, ' ', diff);
    }
    else
    {
        const char *pName = HttpHeader::getHeaderName(index);
        int iNameLen = HttpHeader::getHeaderStringLen(index);
        appendReqHeader(pName, iNameLen, pNewValue, newValueLen);
        m_commonHeaderOffset[ index ] = m_headerBuf.size() - 4 - newValueLen;
    }
    m_commonHeaderLen[index] = newValueLen;
}

void HttpReq::setReferer(const char *pNewReferer, int newRefererLen)
{
    //updateReqHeader(HttpHeader::H_REFERER, pNewReferer, newRefererLen );

    //addEnv( "ESI_INCLUDED_BY", 15, pNewReferer, newRefererLen );
    addEnv("ESI_REFERER", 11, pNewReferer, newRefererLen);
}







