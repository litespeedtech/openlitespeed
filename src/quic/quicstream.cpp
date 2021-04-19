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

#include "quicstream.h"
#include "quiclog.h"
#include <lsquic.h>
#include <lsr/ls_strtool.h>
#include <lsr/ls_str.h>
#include <util/datetime.h>
#include <http/clientinfo.h>
#include <http/hiohandlerfactory.h>
#include <http/httpstatuscode.h>
#include <http/httprespheaders.h>
#include <log4cxx/logger.h>
#include <lsr/ls_swap.h>

#include <inttypes.h>
#include <errno.h>
#include <stdio.h>


QuicStream::~QuicStream()
{
    lsquic_stream_ctx_t::m_pStream = NULL;
}


int QuicStream::init(lsquic_stream_t *s)
{
    m_pStream = s;
    setActiveTime(DateTime::s_curTime);
    clearLogId();
    //if (lsquic_conn_quic_version(lsquic_stream_conn(s)) >= LSQVER_ID24)
    if ((1u <<lsquic_conn_quic_version(lsquic_stream_conn(s)))
          & LSQUIC_IETF_VERSIONS)
        setProtocol(HIOS_PROTO_HTTP3);
    else
        setProtocol(HIOS_PROTO_QUIC);

    enum stream_flag flag = HIO_FLAG_FLOWCTRL;
    /* Turn on the push capable flag: check it when push() is called and
     * unset it if necessary.
     */
    if (lsquic_stream_is_pushed(m_pStream))
        flag = flag | HIO_FLAG_IS_PUSH;
    else
        flag = flag | HIO_FLAG_PUSH_CAPABLE;
    setFlag(flag, 1);

    setState(HIOS_CONNECTED);

    int pri = HIO_PRIORITY_HTML;
//     if (pPriority)
//     {
//         if (pPriority->m_weight <= 32)
//             pri = (32 - pPriority->m_weight) >> 2;
//         else
//             pri = (256 - pPriority->m_weight) >> 5;
//     }
    setPriority(pri);

    LS_DBG_L(this, "QuicStream::init(), id: %" PRIu64 ", priority: %d, flag: %d. ",
             lsquic_stream_id(s), pri, (int)getFlag());
    return 0;

}


int QuicStream::processHdrSet(void *hdr_set)
{
    UnpackedHeaders *hdrs;
    assert(hdr_set);
    if ((long)hdr_set & 0x1L)
        hdrs = (UnpackedHeaders *)((long)hdr_set & ~0x1L);
    else
    {
        UpkdHdrBuilder *builder = (UpkdHdrBuilder *)hdr_set;
        hdrs = builder->retrieveHeaders();
        delete builder;
    }
    assert(hdrs);
    if (processUpkdHdrs(hdrs) == LS_FAIL)
    {
        LS_DBG_L(this, "QuicStream::processUpkdHdrs() failed, shutdown.");
        shutdown();
        return LS_FAIL;
    }
    return LS_OK;
}


int QuicStream::processUpkdHdrs(UnpackedHeaders *hdrs)
{
    HioHandler *pHandler = HioHandlerFactory::getHandler(HIOS_PROTO_HTTP);
    if (pHandler)
    {
        setActiveTime(DateTime::s_curTime);
        pHandler->attachStream(this);
        setReqHeaders(hdrs);
        pHandler->onInitConnected();
        return LS_OK;
    }
    delete hdrs;
    return LS_FAIL;
}


int QuicStream::shutdown()
{
    LS_DBG_L(this, "QuicStream::shutdown()");
    if (getState() >= HIOS_SHUTDOWN)
        return 0;
    setState(HIOS_SHUTDOWN);
    if (!m_pStream)
        return 0;
    setActiveTime(DateTime::s_curTime);
    //lsquic_stream_wantwrite(m_pStream, 1);
    //return lsquic_stream_shutdown(m_pStream, 1);
    return lsquic_stream_close(m_pStream);
}




int QuicStream::onTimer()
{
    if (getState() == HIOS_CONNECTED && getHandler())
        return getHandler()->onTimerEx();
    return 0;
}


void QuicStream::switchWriteToRead()
{
    return;
    //lsquic_stream_wantwrite(0);
    //lsquic_stream_wantread(1);
}


void QuicStream::continueWrite()
{
    LS_DBG_L(this, "QuicStream::continueWrite()");
    if (!m_pStream)
        return;
    setFlag(HIO_FLAG_WANT_WRITE, 1);
    lsquic_stream_wantwrite(m_pStream, 1);
}


void QuicStream::suspendWrite()
{
    LS_DBG_L(this, "QuicStream::suspendWrite()");
    if (!m_pStream)
        return;
    setFlag(HIO_FLAG_WANT_WRITE, 0);
    lsquic_stream_wantwrite(m_pStream, 0);
}


void QuicStream::continueRead()
{
    LS_DBG_L(this, "QuicStream::continueRead()");
    if (!m_pStream)
        return;
    setFlag(HIO_FLAG_WANT_READ, 1);
    lsquic_stream_wantread(m_pStream, 1);
}


void QuicStream::suspendRead()
{
    LS_DBG_L(this, "QuicStream::suspendRead()");
    if (!m_pStream)
        return;
    setFlag(HIO_FLAG_WANT_READ, 0);
    lsquic_stream_wantread(m_pStream, 0);
}


int QuicStream::sendRespHeaders(HttpRespHeaders *pRespHeaders, int isNoBody)
{
    lsquic_http_headers_t headers;

    if (!m_pStream)
        return -1;
    pRespHeaders->prepareSendXpack(getProtocol() == HIOS_PROTO_HTTP3);
    headers.headers = pRespHeaders->begin();
    headers.count = pRespHeaders->end() - pRespHeaders->begin();

    return lsquic_stream_send_headers(m_pStream, &headers, isNoBody);
}


int QuicStream::sendfile(int fdSrc, off_t off, size_t size, int flag)
{
    return 0;
}


int QuicStream::checkReadRet(int ret)
{
    switch(ret)
    {
    case 0:
        if (getState() != HIOS_SHUTDOWN)
        {
            LS_DBG_L(this, "End of stream detected, CLOSING!");
#ifdef _ENTERPRISE_   //have the connection closed quickly
            setFlag(HIO_FLAG_PEER_SHUTDOWN, 1);
#endif
            setState(HIOS_CLOSING);
        }
        return -1;
    case -1:
        switch(errno)
        {
        case EAGAIN:
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK:
#endif
            return 0;
        default:
            tobeClosed();
            LS_DBG_L(this, "read error: %s\n", strerror(errno));
        }
        break;
    default:
        bytesRecv(ret);
        setActiveTime(DateTime::s_curTime);

    }
    return ret;

}


int QuicStream::readv(iovec *vector, int count)
{
    if (!m_pStream)
        return -1;
    ssize_t ret = lsquic_stream_readv(m_pStream, vector, count);
    LS_DBG_L(this, "QuicStream::readv(), ret: %zd", ret);
    return checkReadRet(ret);
}


int QuicStream::read(char *pBuf, int size)
{
    if (!m_pStream)
        return -1;
    ssize_t ret = lsquic_stream_read(m_pStream, pBuf, size);
    LS_DBG_L(this, "QuicStream::read(), to read: %d, ret: %zd", size, ret);
    return checkReadRet(ret);

}


int QuicStream::push(UnpackedHeaders *hdrs)
{
    if (!m_pStream)
        return -1;

    lsquic_conn_t *pConn = lsquic_stream_conn(m_pStream);
    lsquic_http_headers_t headers;
    int pushed;
    hdrs->prepareSendXpack(getProtocol() == HIOS_PROTO_HTTP3);
    headers.headers = (lsxpack_header *)hdrs->begin();
    headers.count = hdrs->end() - hdrs->begin();
    pushed = lsquic_conn_push_stream(pConn, (void *)((long)hdrs | 0x1L),
                                     m_pStream, &headers);
    if (pushed == 0)
        return 0;

    LS_DBG_L("push_stream() returned %d, unset flag", pushed);
    setFlag(HIO_FLAG_PUSH_CAPABLE, 0);
    return -1;
}


int QuicStream::close()
{
    LS_DBG_L(this, "QuicStream::close()");
    if (!m_pStream)
        return 0;
    return lsquic_stream_close(m_pStream);
}


int QuicStream::flush()
{
    LS_DBG_L(this, "QuicStream::flush()");
    if (!m_pStream)
        return -1;
    return lsquic_stream_flush(m_pStream);
}


int QuicStream::writev(const iovec *vector, int count)
{
    if (!m_pStream)
        return -1;
    ssize_t ret = lsquic_stream_writev(m_pStream, vector, count);
    LS_DBG_L(this, "QuicStream::writev(), ret: %zd, errno: %d", ret, errno);
    if (ret > 0)
        setActiveTime(DateTime::s_curTime);
    else if (ret == -1)
    {
        tobeClosed();
        LS_DBG_L(this, "close stream, writev error: %s\n", strerror(errno));
    }
    return ret;
}


int QuicStream::write(const char *pBuf, int size)
{
    if (!m_pStream)
        return -1;
    ssize_t ret = lsquic_stream_write(m_pStream, pBuf, size);
    LS_DBG_L(this, "QuicStream::writev(), to write: %d, ret: %zd, errno: %d",
             size, ret, errno);
    if (ret > 0)
        setActiveTime(DateTime::s_curTime);
    else if (ret == -1)
    {
        tobeClosed();
        LS_DBG_L(this, "close stream, write error: %s\n", strerror(errno));
    }

    return ret;
}


const char *QuicStream::buildLogId()
{
    const lsquic_cid_t *cid;
    lsquic_stream_id_t streamid;
    char cidstr[MAX_CID_LEN * 2 + 1];

    if (!m_pStream)
    {
        cidstr[0] = '\0';
        streamid = ~0ull;
    }
    else
    {
        lsquic_conn_t *pConn = lsquic_stream_conn(m_pStream);
        cid = lsquic_conn_id(pConn);
        lsquic_cid2str(cid, cidstr);
        streamid = lsquic_stream_id(m_pStream);
    }
    m_logId.len = ls_snprintf(m_logId.ptr, MAX_LOGID_LEN, "%s:%d-Q:%s-%" PRIu64,
                      getConnInfo()->m_pClientInfo->getAddrString(),
                      getConnInfo()->m_remotePort, cidstr, streamid);
    return m_logId.ptr;
}


int QuicStream::onRead()
{
    LS_DBG_L(this, "QuicStream::onRead()");
    if (getHandler())
    {
        getHandler()->onReadEx();
    }
    else if (getState() == HIOS_CONNECTED)
    {
        void *hset = lsquic_stream_get_hset(m_pStream);
        if (hset)
            processHdrSet(hset);
        else
        {
            LS_DBG_L(this, "lsquic_stream_get_hset() failed, shutdown.");
            shutdown();
            return LS_FAIL;
        }
    }

    if (getState() == HIOS_CLOSING)
    {
        onPeerClose();
        return LS_FAIL;
    }
    return LS_OK;
}


int QuicStream::onWrite()
{
    LS_DBG_L(this, "QuicStream::onWrite()");
    if (getState() != HIOS_CONNECTED)
    {
        close();
        return LS_FAIL;
    }
    else
    {
        if (getHandler())
            getHandler()->onWriteEx();
        if (getState() == HIOS_CLOSING)
        {
            onPeerClose();
            return LS_FAIL;
        }
    }
    return LS_OK;
}


int QuicStream::onClose()
{
    LS_DBG_L(this, "QuicStream::onClose()");
    if (getFlag() & HIO_FLAG_IS_PUSH)
    {
        UnpackedHeaders *hdrs;
        hdrs = (UnpackedHeaders *)lsquic_stream_get_hset(m_pStream);
        if (hdrs)
            delete hdrs;
    }
    if (getHandler())
        getHandler()->onCloseEx();
    m_pStream = NULL;
    return LS_OK;
}


int QuicStream::getEnv(HioCrypto::ENV id, char *&val, int maxValLen)
{
    if (!m_pStream)
        return 0;
    lsquic_conn_t *pConn = lsquic_stream_conn(m_pStream);
    const char *str;
    int len, size;
    enum lsquic_version ver;

    if (maxValLen < 0)
        return 0;

    /* In the code below, we always NUL-terminate the string */

    switch(id)
    {
    case CRYPTO_VERSION:
        switch (lsquic_conn_crypto_ver(pConn))
        {
        case LSQ_CRY_QUIC:
            str = "QUIC";
            len = 4;
            break;
        default:
            str = "TLSv13";
            len = 6;
            break;
        }
        if (len + 1 > maxValLen)
            return 0;
        memcpy(val, str, len + 1);
        return len;

    case SESSION_ID:
        return 0;
//         if (sizeof(cid) * 2 + 1 > (unsigned) maxValLen)
//             return -1;
//         cid = lsquic_conn_id(pConn);
//         snprintf(val, maxValLen - 1, "%016"PRIx64, cid);
//         return sizeof(cid) * 2;

    case CLIENT_CERT:
        return 0;

    case CIPHER:
        str = lsquic_conn_crypto_cipher(pConn);
        if (!str)
            return 0;
        len = strlen(str);
        if (len + 1 > maxValLen)
            return 0;
        memcpy(val, str, len + 1);
        return len;

    case CIPHER_USEKEYSIZE:
        size = lsquic_conn_crypto_keysize(pConn);
        if (size < 0)
            return 0;
        len = snprintf(val, maxValLen - 1, "%d", size << 3);
        if (len > maxValLen - 1)
            return 0;
        return len;

    case CIPHER_ALGKEYSIZE:
        size = lsquic_conn_crypto_alg_keysize(pConn);
        if (size < 0)
            return 0;
        len = snprintf(val, maxValLen - 1, "%d", size << 3);
        if (len > maxValLen - 1)
            return 0;
        return len;

    case TRANS_PROTOCOL_VERSION:
        ver = lsquic_conn_quic_version(pConn);
        if (ver < N_LSQVER)
        {
            str = lsquic_ver2str[ver];
            len = strlen(str);
            if (len + 1 <= maxValLen)
            {
                memcpy(val, str, len + 1);
                return len;
            }
        }
        return 0;

    default:
        return 0;
    }
}

