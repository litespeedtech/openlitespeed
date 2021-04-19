/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2021  LiteSpeed Technologies, Inc.                 *
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
#ifndef QUICLOG_H
#define QUICLOG_H

#include <liblsquic/lsquic_logger.h>

#define LS_DBG_LC(...) do {                             \
    char cidbuf_[MAX_CID_LEN * 2 + 1];                  \
    LS_DBG_L(__VA_ARGS__);                              \
} while (0)

#define LS_DBG_MC(...) do {                             \
    char cidbuf_[MAX_CID_LEN * 2 + 1];                  \
    LS_DBG_M(__VA_ARGS__);                              \
} while (0)

#endif
