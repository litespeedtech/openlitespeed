/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#include <util/linkedobj.h>
#include <util/loopbuf.h>

#include <inttypes.h>

struct Priority_st;
class H2Connection;
class NtwkIOLink;

class H2Stream: public DLinkedObj, public HioStream
{

public:
    H2Stream();
    ~H2Stream();

    int init(uint32_t StreamID, H2Connection *pH2Conn, uint8_t H2_Flags,
             HioHandler *pHandler, Priority_st *pPriority = NULL);
    int onInitConnected(bool bUpgraded = false);

    int appendReqData(char *pData, int len, uint8_t H2_Flags);

    int read(char *buf, int len);

    uint32_t getStreamID()
    {   return m_uiStreamID;    }

    int write(const char *buf, int len);
    int writev(const struct iovec *vec, int count);
    int writev(IOVec &vector, int total);

    int sendfile(int fdSrc, off_t off, size_t size)
    {
        return 0;
    };
    void switchWriteToRead() {};

    int flush();
    int sendRespHeaders(HttpRespHeaders *pHeaders);

    void suspendRead()
    {   setFlag(HIO_FLAG_WANT_READ, 0);     }

    void suspendWrite()
    {   setFlag(HIO_FLAG_WANT_WRITE, 0);    }

    void continueRead();
    void continueWrite();

    void onTimer();

    virtual NtwkIOLink *getNtwkIoLink();

    int shutdown();

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


    void appendInputData(char ch)
    {
        return m_bufIn.append(ch);
    }

    uint8_t getReqHeaderEnd() {   return m_reqHeaderEnd;  }
    void    setReqHeaderEnd(uint8_t v)  {   m_reqHeaderEnd = v;  }

    void setContentlen( int32_t len )   {   m_iContentLen = len;    }

private:
    bool operator==(const H2Stream &other) const;

    int sendData(IOVec *pIov, int total);

protected:
    virtual const char *buildLogId();

private:
    uint32_t    m_uiStreamID;
    int32_t     m_iWindowOut;
    int32_t     m_iWindowIn;
    H2Connection *m_pH2Conn;
    LoopBuf     m_bufIn;
    int32_t     m_iContentLen;
    int32_t     m_iContentRead;

    uint8_t     m_reqHeaderEnd;

    LS_NO_COPY_ASSIGN(H2Stream);
};


#endif // H2STREAM_H
