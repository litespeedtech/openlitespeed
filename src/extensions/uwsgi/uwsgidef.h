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
#ifndef _UWSGIDEF_H_
#define _UWSGIDEF_H_

/************************************************************************
    FastCGI types and definitions.
 ************************************************************************/


/*
 * Listening socket file number
 */
#define UWSGI_LISTENSOCK_FILENO  0
#define UWSGI_MAX_PREFIX_LEN     8 // a 6 digit length and a colon (and a spare bytes)


#endif //_UWSGIDEF_H_


