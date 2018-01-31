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
#ifndef LSIAPI_CONST_H
#define LSIAPI_CONST_H

#include <ls.h>
#include <lsdef.h>

class LsiApiConst
{
public:
    static int get_cgi_header_count(void);
    
    static const char *get_cgi_header(int index);
    static const char **get_cgi_header_array(void);
    
    static int get_cgi_header_len(int index);
    static const int *get_cgi_header_len_array(void);
};

#endif
