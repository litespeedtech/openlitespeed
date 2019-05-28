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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lsgo.h"
#include "lsapidef.h"


struct lsapi_http_header_index *
lsgo_get_hdr_idx_ptr (uintptr_t p)
{
    return (struct lsapi_http_header_index *)p;
}


struct lsapi_header_offset *
lsgo_get_unkn_hdr_list_ptr (uintptr_t p)
{
    return (struct lsapi_header_offset *)p;
}

int
lsgo_is_big_endian (void)
{
    return LSAPI_ENDIAN == LSAPI_ENDIAN_BIG;
}

int
lsgo_max_hdr_cnt (void)
{
    return LSAPI_MAX_RESP_HEADERS;
}

int
lsgo_max_hdr_len (void)
{
    return LSAPI_RESP_HTTP_HEADER_MAX;
}

