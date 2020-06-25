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
#ifndef H2CONNECTION_H
#define H2CONNECTION_H

#include <edio/bufferedos.h>
#include <http/hiostream.h>
#include <h2/h2protocol.h>
#include <h2/h2connbase.h>
#include <util/autobuf.h>
#include <util/dlinkqueue.h>
#include <lstl/thash.h>

#include <lsdef.h>

#include <sys/time.h>
#include <limits.h>

#include "lshpack.h"




class H2StreamBase;
class H2Stream;
class UpkdHdrBuilder;

class H2Connection: public HioHandler, public H2ConnBase
{
public:
    H2Connection();
    virtual ~H2Connection();
    virtual LogSession* getLogSession() const
    {   return getStream();    }
    virtual InputStream* getInStream()
    {   return getStream();     }

    virtual int assignStreamHandler(H2StreamBase *stream);
    int verifyStreamId(uint32_t id);

    int onReadEx();
    int onWriteEx();

    int flush();

    int onCloseEx();

    void recycle();
    bool isPauseWrite() const
    {   return getStream()->isPauseWrite(); }

    //Following functions are just placeholder

    //Placeholder
    int onInitConnected();

    int onTimerEx();

    void continueWrite()
    {   if (!getStream()->isWantWrite())
            getStream()->continueWrite();   }
    void suspendRead()
    {   getStream()->suspendRead(); }


    int sendRespHeaders(H2Stream *stream, HttpRespHeaders *hdrs, uint8_t flag);

    int h2cUpgrade(HioHandler *pSession, const char * pBuf, int size);

    void recycleStream(H2StreamBase *stream);

    static HioHandler *get();

    int pushPromise(uint32_t streamId, UnpackedHeaders *hdrs);

private:
    int onWriteEx2();

    H2StreamBase *getNewStream(uint8_t ubH2_Flags);

    int decodeHeaders(uint32_t id, unsigned char *src, int length,
                      unsigned char iHeaderFlag);

    int doGoAway(H2ErrorCode status);

    ssize_t directBuffer(const char *data, size_t size);
    int appendSendfileOutput(int fd, off_t off, int size);

    int decodeReqHdrData(H2Stream *pStream, const unsigned char *pSrc,
                         const unsigned char *bufEnd, UpkdHdrBuilder *builder);
    int encodeRespHeaders(H2Stream *stream, HttpRespHeaders *pRespHeaders,
                          unsigned char *buf, int maxSize);

    H2StreamBase* createPushStream(uint32_t pushStreamId,
                                   UnpackedHeaders *headers);

private:

    LS_NO_COPY_ASSIGN(H2Connection);
};

#endif // H2CONNECTION_H
