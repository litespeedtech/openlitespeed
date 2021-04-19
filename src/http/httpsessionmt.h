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
#ifndef HTTPSESSIONMT_H
#define HTTPSESSIONMT_H
#define HSF_MT_NOTIFIED             (1<<1)
#define HSF_MT_CANCEL               (1<<2)
#define HSF_MT_FLUSH                (1<<3)
#define HSF_MT_END_RESP             (1<<4)
#define HSF_MT_RECYCLE              (1<<5)
#define HSF_MT_READING              (1<<6)
#define HSF_MT_WRITING              (1<<7)
#define HSF_MT_INIT_RESP_BUF        (1<<8)
#define HSF_MT_END                  (1<<9)
#define HSF_MT_PARSE_REQ_ARGS       (1<<10)
#define HSF_MT_SND_RSP_HDRS         (1<<11)
#define HSF_MT_SENDFILE             (1<<12)
#define HSF_MT_SET_URI_QS           (1<<13)
#define HSF_MT_RESP_HDR_SENT        (1<<14)
#define HSF_MT_FLUSH_RBDY_LBUF      (1<<15)
#define HSF_MT_FLUSH_RBDY_LBUF_Q    (1<<16)

#define HSF_MT_MASK                 (HSF_MT_FLUSH | HSF_MT_END_RESP \
                                    | HSF_MT_INIT_RESP_BUF)

#endif /* HTTPSESSIONMT_H */
