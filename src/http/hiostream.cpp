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

#include "hiostream.h"
#include <assert.h>
#include <lsr/ls_str.h>
#include <spdy/unpackedheaders.h>

static const ls_str_t s_sProtoName[] =
{
    LS_STR_CONST("HTTP"),
    LS_STR_CONST("SPDY2"),
    LS_STR_CONST("SPDY3"),
    LS_STR_CONST("SPDY3.1"),
    LS_STR_CONST("HTTP2"),
    LS_STR_CONST("QUIC"),
    LS_STR_CONST("HTTP3"),
    LS_STR_CONST("UNKNOWN"),
};


const ls_str_t *HioStream::getProtocolName(HiosProtocol proto)
{
    if (proto > HIOS_PROTO_MAX)
        proto = HIOS_PROTO_MAX;
    return &s_sProtoName[proto];
}


HioStream::~HioStream()
{
}


void HioStream::switchHandler(HioHandler *pCurrent, HioHandler *pNew)
{
    assert(m_pHandler == pCurrent);
    pCurrent->detachStream();
    pNew->attachStream(this);
}


void HioStream::handlerOnClose()
{
    if (!isReadyToRelease())
        m_pHandler->onCloseEx();
    if (m_pHandler && isReadyToRelease())
    {
        m_pHandler->recycle();
        m_pHandler = NULL;
    }
}


HioHandler::~HioHandler()
{

}

int HioHandler::h2cUpgrade(HioHandler *pOld, const char * pBuf, int size)
{
    return LS_FAIL;
}
