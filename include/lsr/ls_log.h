/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2019  LiteSpeed Technologies, Inc.                 *
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
#ifndef LS_LOG_H
#define LS_LOG_H

#include <lsr/ls_types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum LS_LOG_LEVEL
{
    LS_LOG_FATAL = 0,
    LS_LOG_ALERT = 1000,
    LS_LOG_CRIT  = 2000,
    LS_LOG_ERROR  = 3000,
    LS_LOG_WARN   = 4000,
    LS_LOG_NOTICE = 5000,
    LS_LOG_INFO   = 6000,
    LS_LOG_DEBUG  = 7000,
    LS_LOG_DBG_LOW  = 7020,
    LS_LOG_DBG_MED = 7050,
    LS_LOG_DBG_HIGH = 7080,
    LS_LOG_TRACE  = 8000,
    LS_LOG_NOTSET = 9000,
    LS_LOG_UNKNOWN = 10000
};


#define LSR_DBG_H( ... ) \
    do { \
        c_log( LS_LOG_DBG_HIGH, __VA_ARGS__); \
    }while(0)

#define LSR_DBG_M( ... ) \
    do { \
        c_log( LS_LOG_DBG_MED, __VA_ARGS__); \
    }while(0)

#define LSR_DBG_L( ... ) \
    do { \
        c_log( LS_LOG_DBG_LOW, __VA_ARGS__); \
    }while(0)

#define LSR_DBG( ... ) \
    do { \
        c_log( LS_LOG_DEBUG, __VA_ARGS__); \
    }while(0)


#define LSR_INFO( ... ) \
    do { \
        c_log( LS_LOG_INFO, __VA_ARGS__); \
    }while(0)

#define LSR_NOTICE( ... ) \
    do { \
        c_log( LS_LOG_NOTICE, __VA_ARGS__); \
    }while(0)

#define LSR_WARN( ... ) \
    do { \
        c_log( LSI_LOG_WARN, __VA_ARGS__); \
    }while(0)

#define LSR_ERROR( ... ) \
    do { \
        c_log( LS_LOG_ERROR, __VA_ARGS__); \
    }while(0)

void c_log(int level, const char *format, ...)
#if __GNUC__
    __attribute__((format(printf, 2, 3)))
#endif
    ;

#ifdef __cplusplus
}
#endif

#endif //LS_LOG_H
