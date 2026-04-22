/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2025  LiteSpeed Technologies, Inc.                 *
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
#include "uwsgienv.h"
#include <log4cxx/logger.h>

#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

int UwsgiEnv::add(const char *name, size_t nameLen,
                 const char *value, size_t valLen)
{
    if (!name)
        return 0;
    LS_DBG("UwsgiEnv::add(%s, %.*s)", name, (int)valLen, value);
    #define MAX_OVERHEAD    4
    if ((nameLen > 1024) || (valLen > 65535))
        return 0;
    int bufLen = m_buf.available();
    if (bufLen < (int)(nameLen + valLen + MAX_OVERHEAD))
    {
        int grow = nameLen + valLen + MAX_OVERHEAD - bufLen;
        int ret = m_buf.grow(grow);
        if (ret == -1)
            return ret;
    }
    uint8_t *pBuf = (uint8_t *)m_buf.end();
    *pBuf++ = (uint8_t)(nameLen & 0xff);
    *pBuf++ = (uint8_t)((nameLen >> 8) & 0xff);
    memcpy(pBuf, name, nameLen);
    pBuf += nameLen;

    *pBuf++ = (uint8_t)(valLen & 0xff);
    *pBuf++ = (uint8_t)((valLen >> 8) & 0xff);
    memcpy(pBuf, value, valLen);
    pBuf += valLen;
    m_buf.used(nameLen + valLen + MAX_OVERHEAD);
    return 0;
}


