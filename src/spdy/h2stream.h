/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2020  LiteSpeed Technologies, Inc.                 *
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
#ifndef H2STREAM_H
#define H2STREAM_H

#include <lsdef.h>
#include <http/hiostream.h>
#include <spdy/unpackedheaders.h>
#include <spdy/h2protocol.h>
#include <util/linkedobj.h>
#include <util/loopbuf.h>
#include <util/datetime.h>
#include <lstl/thash.h>

#include <inttypes.h>

struct Priority_st;
class H2Connection;
class NtwkIOLink;

class H2Stream : public DLinkedObj, public HioStream
               , public HentryInt<H2Stream, uint32_t, H2StreamHasher>
{

public:
    H2Stream();
    ~H2Stream();

    void reset();

    int init(H2Connection *pH2Conn,
             Priority_st *pPriority = NULL);
    int onInitConnected(HioHandler *pHandler, bool bUpgraded = false);

    int appendReqData(char *pData, int len, uint8_t H2_Flags);

    int read(char *buf, int len);

    uint32_t getStreamID() const
    {   return get_hash();    }

    int write(const char *buf, int len);
    int writev(const struct iovec *vec, int count);
    int writev(IOVec &vector, int total);
    int write(const char *buf, int len, int flag);

    int sendfile(int fdSrc, off_t off, size_t size, int flag);

    void switchWriteToRead() {};

    int flush();
    int sendRespHeaders(HttpRespHeaders *pHeaders, int isNoBody);

    UnpackedHeaders * getReqHeaders()
    {   return &m_headers;   }

    int push(ls_str_t *pUrl, ls_str_t *pHost, 
             ls_strpair_t *pExtraHeaders);


    void suspendRead()
    {   setFlag(HIO_FLAG_WANT_READ, 0);     }

    void suspendWrite()
    {   setFlag(HIO_FLAG_WANT_WRITE, 0);    }

    void continueRead();
    void continueWrite();

    int onTimer();

    int isStuckOnRead()
    {
        return (isWantRead() && DateTime::s_curTime - getActiveTime() >= 5);
    }

    int shutdown();

    void closeEx();
    int close();

    int onWrite();

    int isFlowCtrl() const          {   return getFlag(HIO_FLAG_FLOWCTRL);    }

    int32_t getWindowOut() const    {   return m_iWindowOut;    }
//    void setWindowOut(int32_t v)    {   m_iWindowOut = v;    }
    int adjWindowOut(int32_t n);

    int32_t getWindowIn() const     {   return m_iWindowIn;     }
    void adjWindowIn(int32_t n)     {   m_iWindowIn += n;       }

    void clearBufIn()               {   m_bufIn.clear();        }
    LoopBuf *getBufIn()             {   return &m_bufIn;        }
    int appendInputData(const char *pData, int len)
    {
        return m_bufIn.append(pData, len);
    }

    int getDataFrameSize(int wanted);

    void apply_priority(Priority_st *priority);

    void appendInputData(char ch)
    {
        return m_bufIn.append(ch);
    }

private:
    bool operator==(const H2Stream &other) const;

    int dataSent(int ret);
    void shutdownEx();
    void markShutdown();

protected:
    virtual const char *buildLogId();

private:
    LoopBuf         m_bufIn;
    UnpackedHeaders m_headers;
    H2Connection   *m_pH2Conn;
    int32_t         m_iWindowOut;
    int32_t         m_iWindowIn;

    LS_NO_COPY_ASSIGN(H2Stream);
};


#endif // H2STREAM_H
