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
#ifndef QUICSTREAM_H
#define QUICSTREAM_H

#include <lsdef.h>
#include <http/hiostream.h>
#include <sslpp/hiocrypto.h>
#include <lsquic_types.h>
#include <util/linkedobj.h>
#include <h2/unpackedheaders.h>

struct lsquic_stream_ctx {
    lsquic_stream_t *m_pStream;
};


class QuicStream : private lsquic_stream_ctx_t
                 , public HioStream
                 , public HioCrypto
                 , public DLinkedObj
{
public:
    QuicStream()
        {}
    ~QuicStream();
    int init(lsquic_stream_t *s);
    void setQuicStream(lsquic_stream_t *s)  {   m_pStream = s;  }
    int processHdrSet(void *hdr_set);
    int processUpkdHdrs(UnpackedHeaders *hdrs);
    lsquic_stream_ctx_t *toHandler()    {   return this;    }

    virtual int shutdown();
    virtual int onTimer();
    virtual void switchWriteToRead();
    virtual void continueWrite();
    virtual void suspendWrite();
    virtual void continueRead();
    virtual void suspendRead();
    virtual int sendRespHeaders(HttpRespHeaders *pHeaders, int isNoBody);

    virtual int sendfile(int fdSrc, off_t off, size_t size, int flag);
    virtual int readv(iovec *vector, int count);
    virtual int read(char *pBuf, int size);
    virtual int close();
    virtual int flush();
    virtual int writev(const iovec *vector, int count);
    virtual int write(const char *pBuf, int size);
    virtual const char *buildLogId();
    virtual int push(UnpackedHeaders *hdrs);

    virtual int getEnv(HioCrypto::ENV id, char *&val, int maxValLen);

    virtual int sendfile(int fdSrc, off_t off, size_t size) { return 0; }

    int onRead();
    int onWrite();
    int onClose();

private:
    int checkReadRet(int ret);

    LS_NO_COPY_ASSIGN(QuicStream);
};

#endif // QUICSTREAM_H
