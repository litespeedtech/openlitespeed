/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2020  LiteSpeed Technologies, Inc.                 *
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
#ifndef _LS_HTTP_HEADER_H_
#define _LS_HTTP_HEADER_H_

#include <lsr/ls_str.h>
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct ls_http_header
{
    union
    {
        struct ls_str_s name;
        struct iovec    name_iov;
    };
    union
    {
        struct ls_str_s value;
        struct iovec    value_iov;
    };
} ls_http_header_t;

static inline void ls_http_header_set(ls_http_header_t *hdr,
                                      const char *name, int name_len,
                                      const char *value, int value_len)
{
    hdr->name.ptr = (char *)name;
    hdr->name.len = name_len;
    hdr->value.ptr = (char *)value;
    hdr->value.len = value_len;
}


static inline void ls_http_header_set2(ls_http_header_t *hdr,
                                      const char *name, const char *value)
{
    hdr->name.ptr = (char *)name;
    hdr->name.len = strlen(name);
    hdr->value.ptr = (char *)value;
    hdr->value.len = strlen(value);
}


#ifdef __cplusplus
}
#endif

#endif // _LS_HTTP_HEADER_H_

