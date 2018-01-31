/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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

#include <http/httpheader.h>
#include <lsiapi/lsiapi_const.h>

static const char *CGI_HEADERS[HttpHeader::H_TRANSFER_ENCODING ] =
{
    "HTTP_ACCEPT", "HTTP_ACCEPT_CHARSET", "HTTP_ACCEPT_ENCODING",
    "HTTP_ACCEPT_LANGUAGE", "HTTP_AUTHORIZATION", "HTTP_CONNECTION",
    "CONTENT_TYPE", "CONTENT_LENGTH", "HTTP_COOKIE", "HTTP_COOKIE2",
    "HTTP_HOST", "HTTP_PRAGMA", "HTTP_REFERER", "HTTP_USER_AGENT",
    "HTTP_CACHE_CONTROL",
    "HTTP_IF_MODIFIED_SINCE", "HTTP_IF_MATCH",
    "HTTP_IF_NONE_MATCH",
    "HTTP_IF_RANGE",
    "HTTP_IF_UNMODIFIED_SINCE",
    "HTTP_KEEPALIVE",
    "HTTP_RANGE",
    "HTTP_X_FORWARDED_FOR",
    "HTTP_VIA",

    //"HTTP_TRANSFER_ENCODING"
};
static const int CGI_HEADER_LEN[HttpHeader::H_TRANSFER_ENCODING ] =
{
    11, 19, 20, 20, 18, 15, 12, 14, 11, 12, 9, 11, 12, 15, 18,
    22, 13, 18, 13, 24, 14, 10, 20, 8
};


int LsiApiConst::get_cgi_header_count(void)
{
    return (int)HttpHeader::H_TRANSFER_ENCODING;
}


const char *LsiApiConst::get_cgi_header(int index) 
{
    return CGI_HEADERS[index];
}


const char **LsiApiConst::get_cgi_header_array(void)
{
    return CGI_HEADERS;
}

int LsiApiConst::get_cgi_header_len(int index)
{
    return CGI_HEADER_LEN[index];
}

const int *LsiApiConst::get_cgi_header_len_array(void)
{
    return CGI_HEADER_LEN;
}
