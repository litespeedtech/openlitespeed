/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2026  LiteSpeed Technologies, Inc.                 *
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

#ifndef LSCGID_ROOTCHECK_H_
#define LSCGID_ROOTCHECK_H_

#include "lscgid.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*rootcheck_error_cb)(char *operation, char *path);

void rootcheck_set_error_callback(rootcheck_error_cb callback);
int rootcheck_env_is_dangerous(const char *env);
int check_root_executable(char *path);
int check_root_exec_path(lscgid_t *cgi);
int check_root_protected_file(char *path);
int check_root_copy_source(char *path);
int check_root_protected_directory(char *path);

#ifdef __cplusplus
}
#endif

#endif
