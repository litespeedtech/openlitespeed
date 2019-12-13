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
#ifndef _LSDEF_H_
#define _LSDEF_H_

#define __STDC_FORMAT_MACROS

#include <stddef.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


#define LS_OK       0
#define LS_FAIL     (-1)

#define LS_AGAIN    1
#define LS_DONE     0

#define LS_TRUE     1
#define LS_FALSE    0

#define LS_NO_COPY_ASSIGN(T) \
    private: \
    T(const T&);               \
    void operator=(const T&);

#define LS_ZERO_FILL(x, y)     memset(&x, 0, (char *)(&y+1) - (char *)&x)

#define ls_inline           static inline
#define ls_always_inline    static inline __attribute__((always_inline))
#define ls_attr_inline      __attribute__((always_inline))

ls_inline char *lstrncpy(char *dest, const char *src, size_t n)
{
    char *end = (char *)memccpy(dest, src, '\0', n);
    if (end)
        return end - 1;
    if (n)
    {
        end = dest + n - 1;
        *end = '\0';
        return end;
    }
    return dest;
}

ls_inline char *lstrncat(char *dest, const char *src, size_t n)
{
    char *end = (char *)memchr(dest, '\0', n);
    if (end)
        return lstrncpy(end, src, dest + n - end);
    return dest + n;
}

ls_inline char *lstrncat2(char *dest, size_t n, const char *src1, const char *src2)
{
    char *end = lstrncpy(dest, src1, n);
    return lstrncpy(end, src2, dest + n - end);
}

ls_inline int lsnprintf(char *dest, size_t size, const char *format, ...)
#if __GNUC__
        __attribute__((format(printf, 3, 4)))
#endif
        ;
ls_inline int lsnprintf(char *dest, size_t size, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(dest, size, format, ap);
    va_end(ap);

    if (ret < 0 || (unsigned)ret < size)
        return ret;
    if (size)
    {
        *(dest + size - 1) = '\0';
        return size - 1;
    }
    return size;
}

#endif //_LSDEF_H_
