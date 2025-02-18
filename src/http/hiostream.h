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
#ifndef HIOSTREAM_H
#define HIOSTREAM_H


#include <sys/types.h>
#include <edio/ediostream.h>
#include <edio/streamstat.h>
#include <log4cxx/logsession.h>
#include <lsdef.h>
#include <lsr/ls_types.h>

class IOVec;
class Aiosfcb;
class HioHandler;
class HttpRespHeaders;
class HioChainStream;
class NtwkIOLink;
class ClientInfo;
class HioCrypto;
class ServerAddrInfo;
class UnpackedHeaders;

#define HIOS_DISCONNECTED   SS_DISCONNECTED
#define HIOS_CONNECTING     SS_CONNECTING
#define HIOS_CONNECTED      SS_CONNECTED
#define HIOS_CLOSING        SS_CLOSING
#define HIOS_SHUTDOWN       SS_SHUTDOWN
#define HIOS_RESET          SS_RESET

#define HiosProtocol        stream_protocol

#define HIO_FLAG_PEER_SHUTDOWN      SS_FLAG_PEER_SHUTDOWN
#define HIO_FLAG_LOCAL_SHUTDOWN     SS_FLAG_LOCAL_SHUTDOWN
#define HIO_FLAG_WANT_READ          SS_FLAG_WANT_READ
#define HIO_FLAG_WANT_WRITE         SS_FLAG_WANT_WRITE
#define HIO_FLAG_ABORT              SS_FLAG_ABORT
#define HIO_FLAG_PEER_RESET         SS_FLAG_PEER_RESET
#define HIO_FLAG_HANDLER_RELEASE    SS_FLAG_HANDLER_RELEASE
#define HIO_FLAG_PAUSE_WRITE        SS_FLAG_PAUSE_WRITE
#define HIO_EVENT_PROCESSING        SS_EVENT_PROCESSING
#define HIO_FLAG_DELAY_FLUSH        SS_FLAG_DELAY_FLUSH
#define HIO_FLAG_WRITE_BUFFER       SS_FLAG_WRITE_BUFFER
#define HIO_FLAG_BLACK_HOLE         SS_FLAG_BLACK_HOLE
#define HIO_FLAG_FROM_LOCAL         SS_FLAG_FROM_LOCAL
#define HIO_FLAG_PUSH_CAPABLE       SS_FLAG_PUSH_CAPABLE
#define HIO_FLAG_INIT_SESS          SS_FLAG_INIT_SESS
#define HIO_FLAG_IS_PUSH            SS_FLAG_IS_PUSH
#define HIO_FLAG_PASS_THROUGH       SS_FLAG_PASS_THROUGH
#define HIO_FLAG_SENDFILE           SS_FLAG_SENDFILE
#define HIO_FLAG_FLOWCTRL           SS_FLAG_FLOWCTRL
#define HIO_FLAG_PRI_SET            SS_FLAG_PRI_SET
#define HIO_FLAG_ALTSVC_SENT        SS_FLAG_ALTSVC_SENT
#define HIO_FLAG_PASS_SETCOOKIE     SS_FLAG_PASS_SETCOOKIE
#define HIO_FLAG_RESP_HEADER_SENT   SS_FLAG_RESP_HEADER_SENT


#define HIO_EOR                     1

#define HIO_PRIORITY_HIGHEST        (0)
#define HIO_PRIORITY_LOWEST         (7)
#define HIO_PRIORITY_HTML           (2)
#define HIO_PRIORITY_CSS            (HIO_PRIORITY_HTML + 1)
#define HIO_PRIORITY_JS             (HIO_PRIORITY_CSS + 1)
#define HIO_PRIORITY_IMAGE          (HIO_PRIORITY_JS + 1)
#define HIO_PRIORITY_DOWNLOAD       (HIO_PRIORITY_IMAGE + 1)
#define HIO_PRIORITY_PUSH           (HIO_PRIORITY_DOWNLOAD + 1)
#define HIO_PRIORITY_LARGEFILE      (HIO_PRIORITY_LOWEST)


struct ConnInfo
{
    ClientInfo             *m_pClientInfo;
    union
    {
        HioCrypto          *m_pCrypto;
        struct ssl_st      *m_pSsl;
    };
    const ServerAddrInfo   *m_pServerAddrInfo;
    unsigned int            m_remotePort;
};


enum send_hdr_flag
{
    SHF_NONE = 0,
    SHF_EOS = 1,
    SHF_MORE_HDRS = 2,
    SHF_TRAILER = 4
};


class HioStream : virtual public LogSession
                , virtual public EdIoStream
                , virtual public StreamStat
{

public:
    HioStream()
    {
        LS_ZERO_FILL(m_pReqHeaders, m_connInfo);
    }
    virtual ~HioStream();

    virtual int aiosendfile(Aiosfcb *cb)
    {   return 0;   }
    virtual int aiosendfiledone(Aiosfcb *cb)
    {   return 0;   }
    virtual int readv(struct iovec *vector, int count)
    {       return -1;      }

    virtual int sendRespHeaders(HttpRespHeaders *pHeaders, send_hdr_flag flag) = 0;

    virtual void switchWriteToRead() = 0;
    virtual int onTimer()             {    return 0;   }

    virtual int detectClose()       {   return 0;   }
    virtual void enableSocketKeepAlive() {}

    virtual int doneWrite()         {   return -1;  }

    virtual void applyPriority()    {};

    void setPriority(int pri)
    {
        if (pri > HIO_PRIORITY_LOWEST)
            pri = HIO_PRIORITY_LOWEST;
        else if (pri < HIO_PRIORITY_HIGHEST)
            pri = HIO_PRIORITY_HIGHEST;
        if (pri != getPriority())
        {
            StreamStat::setPriority(pri);
            applyPriority();
        }
    }

    void reset(int32_t timeStamp);

    void setClientInfo(ClientInfo *p)   {   m_connInfo.m_pClientInfo = p;      }
    ClientInfo *getClientInfo() const   {   return m_connInfo.m_pClientInfo;   }

    void setConnInfo(const ConnInfo *p)
    {   memmove(&m_connInfo, p, sizeof(m_connInfo));        }
    const ConnInfo *getConnInfo() const {   return &m_connInfo;     }


    HioHandler *getHandler() const  {   return m_pHandler;  }
    void setHandler(HioHandler *p)  {   m_pHandler = p;     }
    void switchHandler(HioHandler *pCurrent, HioHandler *pNew);

    void wantRead(int want)
    {
        if (!(getFlag(HIO_FLAG_WANT_READ)) == !want)
            return;
        if (want)
        {
            setFlag(HIO_FLAG_WANT_READ, 1);
            continueRead();
        }
        else
        {
            setFlag(HIO_FLAG_WANT_READ, 0);
            suspendRead();
        }
    }

    void wantWrite(int want)
    {
        if (!(getFlag(HIO_FLAG_WANT_WRITE)) == !want)
            return;
        if (want)
        {
            setFlag(HIO_FLAG_WANT_WRITE, 1);
            continueWrite();
        }
        else
        {
            setFlag(HIO_FLAG_WANT_WRITE, 0);
            suspendWrite();
        }
    }

    virtual int push(UnpackedHeaders *hdrs)
    {   return -1;      }

    int onPeerClose()
    {
        setFlag(HIO_FLAG_PEER_SHUTDOWN, 1);
        if (m_pHandler)
            handlerOnClose();
        return close();
    }

    const ls_str_t *getProtocolName() const
    {   return getProtocolName(getProtocol());   }

    static const ls_str_t *getProtocolName(HiosProtocol proto);

    UnpackedHeaders *getReqHeaders()
    {   return m_pReqHeaders;    }

    void setReqHeaders(UnpackedHeaders *hdrs)
    {   m_pReqHeaders = hdrs;     }

    virtual ssize_t bufferedWrite(const char *data, size_t size)
    {   return 0;   }
protected:
    void releaseHandler();

private:

    UnpackedHeaders    *m_pReqHeaders;
    HioHandler         *m_pHandler;
    ConnInfo            m_connInfo;


    HioStream(const HioStream &other);
    HioStream &operator=(const HioStream &other);
    bool operator==(const HioStream &other) const;

    void handlerOnClose();

};

class HioHandler
{
    HioStream *m_pStream;

public:
    HioHandler()
        : m_pStream(NULL)
    {}
    virtual ~HioHandler();

    HioStream *getStream() const           {   return m_pStream;   }
    void attachStream(HioStream *p)
    {
        m_pStream  = p;
        p->setHandler(this);
    }

    HioStream *detachStream()
    {
        HioStream *pStream = m_pStream;
        if (pStream)
        {
            m_pStream = NULL;
            if (pStream->getHandler() == this)
                pStream->setHandler(NULL);
        }
        return pStream;
    }

    LogSession *getLogSession() const
    {   return m_pStream;   }

    LogSession *log_s() const
    {   return m_pStream;   }

    virtual int onInitConnected() = 0;
    virtual int onReadEx()  = 0;
    virtual int onWriteEx() = 0;
    virtual int onCloseEx() = 0;
    virtual int onTimerEx() = 0;

    virtual void recycle() = 0;

    virtual int h2cUpgrade(HioHandler *pOld, const char * pBuf, int size);
    virtual int detectContentLenMismatch(int buffered)  {   return 0;  }

private:
    HioHandler(const HioHandler &other);
    HioHandler &operator=(const HioHandler &other);
    bool operator==(const HioHandler &other) const;
};


inline void HioStream::releaseHandler()
{
    if (m_pHandler)
    {
        m_pHandler->recycle();
        m_pHandler = NULL;
    }
}


#endif // HIOSTREAM_H
