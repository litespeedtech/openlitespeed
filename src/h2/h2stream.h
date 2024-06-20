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
#ifndef H2STREAM_H
#define H2STREAM_H

#include <lsdef.h>
#include <http/hiostream.h>
#include <h2/unpackedheaders.h>
#include <h2/h2protocol.h>
#include <h2/h2streambase.h>
#include <util/linkedobj.h>
#include <util/loopbuf.h>
#include <util/datetime.h>
#include <lstl/thash.h>

#include <inttypes.h>

class H2Connection;

class H2Stream : public H2StreamBase, public HioStream
{

public:
    H2Stream();
    ~H2Stream();

    void reset();

    void switchWriteToRead() {};

    int sendRespHeaders(HttpRespHeaders *pHeaders, int isNoBody);

    int push(UnpackedHeaders *hdrs);

    int onInitConnected();
    int onTimer();
    int onClose();
    int onRead();
    int onWrite();
    int onPeerShutdown();
    virtual int doneWrite()
    {   shutdownWrite(); flush(); return 1;   }


private:
    bool operator==(const H2Stream &other) const;

protected:
    virtual const char *buildLogId();

private:

    LS_NO_COPY_ASSIGN(H2Stream);
};


#endif // H2STREAM_H
