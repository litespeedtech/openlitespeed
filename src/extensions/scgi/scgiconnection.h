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
#ifndef SCGICONNECTION_H
#define SCGICONNECTION_H

#include "scgidef.h"
#include "scgienv.h"

#include <lsdef.h>
#include <edio/bufferedos.h>
#include <extensions/extconn.h>
#include <extensions/httpextprocessor.h>
#include <util/autobuf.h>

//#define SCGI_MPLX

#define SCGI_MAX_PACKET_SIZE    65535
#define SCGI_MAX_STDIN_PENDING  8192
#define SCGI_MAX_SCGI_HEADER    40       // long length, :, NULL

class ScgiApp;
class Multiplexer;
class HttpReq;

class ScgiConnection : public ExtConn
    , public HttpExtProcessor
{
private:
    int             m_fd;
    bool            m_sentHeader;
    IOVec           m_iovec;
    ScgiEnv         m_scgiEnv;
    int             m_iTotalPending;
    off_t           m_reqBodyLen;
    bool            m_waitingBodyLength;
    char            m_header[SCGI_MAX_SCGI_HEADER];

protected:
    virtual int doRead();
    virtual int doError(int err);
    int addRequest(ExtRequest *pReq);
    void retryProcessor();
    ExtRequest *getReq() const;
    int processScgiData();

public:
    static const char s_padding[8];
    ScgiConnection();
    ~ScgiConnection();
    void init(int fd, Multiplexer *pMplx);
    int writeBody(const char *blk, int len);
    int sendNetString(const char *data, off_t len);

    int endOfStream();
    bool wantRead();
    bool wantWrite();

    int removeRequest(ExtRequest *pReq);

    int close()    {   ExtConn::close(); return 0;    }
    int  readResp(char *pBuf, int size);
    void finishRecvBuf();
    int readStdOut(char *pBuf, int size);

    void suspendWrite();
    void continueWrite();
    virtual int doWrite();

    virtual int  begin();
    int  sendSpecial(const char *pBuf, int size);
    int  sendReqBody(const char *pBuf, int size);
    int  sendReqHeader();
    int  beginReqBody();
    int  endOfReqBody();
    void abort();
    void cleanUp();
    int  flush();

    int  onStdOut();
    int  processStdOut(char *pBuf, int size);
    int  processStdErr(char *pBuf, int size);
    int  endOfRequest(int code, int status);
    int  sendEndOfStream(int type);

    int  connUnavail();

    void dump();


    LS_NO_COPY_ASSIGN(ScgiConnection);
};

#endif
