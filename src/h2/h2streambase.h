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
#ifndef H2STREAMBASE_H
#define H2STREAMBASE_H

#include <lsdef.h>
#include <h2/h2protocol.h>
#include <log4cxx/logsession.h>
#include <util/linkedobj.h>
#include <util/loopbuf.h>
#include <util/datetime.h>
#include <lstl/thash.h>

#include <http/hiostream.h>
#include <inttypes.h>

struct Priority_st;
class H2ConnBase;

class H2StreamBase : public DLinkedObj
                   , public HentryInt<H2StreamBase, uint32_t, H2StreamHasher>
                   , virtual public LogSession
                   , virtual public EdIoStream
                   , virtual public StreamStat
{

public:
    H2StreamBase();
    virtual ~H2StreamBase();

    void reset();
    int init(H2ConnBase *pH2Conn, Priority_st *pPriority);

    virtual int processRcvdData(char *pData, int len);
    virtual char *getDirectInBuf(size_t &size);
    virtual void directInBufUsed(size_t len);
    virtual char *getDirectOutBuf(size_t &size);
    virtual void directOutBufUsed(size_t len);


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

    void suspendRead()
    {   setFlag(HIO_FLAG_WANT_READ, 0);     }

    void suspendWrite()
    {   setFlag(HIO_FLAG_WANT_WRITE, 0);    }

    void continueRead();
    void continueWrite();
    void resumeNotify() {   continueRead(); }

    int isStuckOnRead()
    {
        return (isWantRead() && DateTime::s_curTime - getActiveTime() >= 5);
    }
    int onPeerShutdown();

    int shutdown();

    void abort();
    void closeEx();
    int close();

    int32_t getWindowOut() const    {   return m_iWindowOut;    }
//    void setWindowOut(int32_t v)    {   m_iWindowOut = v;    }
    int adjWindowOut(int32_t n);

    void clearBufIn()               {   m_bufRcvd.clear();        }
    LoopBuf *getBufIn()             {   return &m_bufRcvd;        }

    int getDataFrameSize(int wanted);

    void apply_priority(Priority_st *priority);

    void adjWindowToUpdate(int32_t n)   {   m_iWindowToUpdate += n;     }
    int32_t getWindowToUpdate() const   {   return m_iWindowToUpdate;   }
    void windowUpdate();


private:
    bool operator==(const H2StreamBase &other) const;


protected:
    int dataSent(int ret);
    void shutdownWrite();
    void markShutdown();

protected:
    LoopBuf         m_bufRcvd;
    H2ConnBase     *m_pH2Conn;
    int32_t         m_iWindowToUpdate;
    int32_t         m_iWindowOut;

    LS_NO_COPY_ASSIGN(H2StreamBase);
};


#endif // H2STREAMBASE_H
