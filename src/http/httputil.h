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
#ifndef HTTPUTIL_H
#define HTTPUTIL_H


#include <limits.h>

class HttpUtil
{
    HttpUtil() {};
    ~HttpUtil() {};
public:
    static int unescape_inplace(char *pDest, int &uriLen,
                                const char *&pOrgSrc);
    static int unescape(char *pDest, int &uriLen,
                        const char *&pOrgSrc);
    //static int unescape(const char *pSrc, char *pDest);
    static int unescape_n(const char *pSrc, char *pDest, int n);
    static int escape(const char *pSrc, char *pDest, int n);
    static int unescape_n(const char *pSrc, int srcLen, char *pDest,
                          int destLen);
    static int escapeHtml(const char *pSrc, const char *pSrcEnd, char *pDest,
                          int n);
    static int escape(const char *pSrc, int srcLen, char *pDest, int n);
};

#endif
