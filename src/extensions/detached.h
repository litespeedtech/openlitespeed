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
#ifndef DETACHED_H
#define DETACHED_H



struct DetachedPidInfo
{
    ino_t inode;
    long  last_modify;
    pid_t pid;
    unsigned start_time_diff;
    uint32_t bin_last_mod;
    uint32_t reserved;

    DetachedPidInfo()
        : inode(0)
        , last_modify(0)
        , pid(-1)
        , start_time_diff(0)
        , bin_last_mod(0)
        , reserved(0)
        {}
};
typedef struct DetachedPidInfo DetachedPidInfo_t;


struct DetachedProcess
{
    DetachedPidInfo_t  pid_info;
    uint32_t           last_check_time;
    uint32_t           last_stop_time;
    int                fd_pid_file;

    DetachedProcess()
        : last_check_time(0)
        , last_stop_time(0)
        , fd_pid_file(-1)
        {}
};
typedef struct DetachedProcess DetachedProcess_t;



#endif //DETACHED_H



