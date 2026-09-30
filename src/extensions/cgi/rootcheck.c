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

#include "rootcheck.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static rootcheck_error_cb s_error_callback;


void rootcheck_set_error_callback(rootcheck_error_cb callback)
{
    s_error_callback = callback;
}


int rootcheck_env_is_dangerous(const char *env)
{
    static const char *const exact_names[] = {
        "BASH_ENV=", "BASHOPTS=", "ENV=", "GCONV_PATH=",
        "GLIBC_TUNABLES=", "HOSTALIASES=", "IFS=", "JAVA_TOOL_OPTIONS=",
        "JDK_JAVA_OPTIONS=", "LOCPATH=", "MALLOC_TRACE=", "NLSPATH=",
        "NODE_OPTIONS=", "NODE_PATH=", "PERL5DB=", "PERL5LIB=",
        "PERL5OPT=", "PERLLIB=", "PHP_INI_SCAN_DIR=", "PHPRC=", "PS4=",
        "PYTHONHOME=", "PYTHONINSPECT=", "PYTHONPATH=", "PYTHONSTARTUP=",
        "PYTHONUSERBASE=", "PYTHONWARNINGS=", "RESOLV_HOST_CONF=",
        "RUBYLIB=", "RUBYOPT=", "SHELLOPTS=", "_JAVA_OPTIONS="
    };
    size_t i;

    /* The dynamic linker recognizes several LD_* controls, so keep the
     * prefix rule rather than maintaining an incomplete exact-name list. */
    if (strncmp(env, "LD_", 3) == 0)
        return 1;
    for (i = 0; i < sizeof(exact_names) / sizeof(exact_names[0]); ++i)
    {
        size_t len = strlen(exact_names[i]);
        if (strncmp(env, exact_names[i], len) == 0)
            return 1;
    }
    return 0;
}


static int root_path_error(char *operation, char *path)
{
    int error = errno;
    if (s_error_callback)
        s_error_callback(operation, path);
    else
        fprintf(stderr, "%s: %s: %s\n", operation, path, strerror(error));
    if (error == ENOENT)
        return 404;
    if (error == EACCES)
        return 403;
    return 500;
}


static int root_path_denied(char *reason, char *path)
{
    errno = EACCES;
    if (s_error_callback)
        s_error_callback(reason, path);
    else
        fprintf(stderr, "%s: %s: %s\n", reason, path, strerror(errno));
    return 403;
}


static int check_root_parent(char *path, char *stat_operation,
                             char *denied_reason)
{
    struct stat st;
    char *parent = ".";
    char *restore = NULL;
    char saved = 0;
    char *slash = strrchr(path, '/');

    if (slash)
    {
        parent = path;
        restore = (slash == path) ? slash + 1 : slash;
        saved = *restore;
        *restore = 0;
    }

    int rc = 0;
    if (stat(parent, &st) == -1)
        rc = root_path_error(stat_operation, parent);
    else if (!S_ISDIR(st.st_mode) || st.st_uid != 0 ||
             (st.st_mode & (S_IWGRP | S_IWOTH)))
        rc = root_path_denied(denied_reason, parent);

    if (restore)
        *restore = saved;
    return rc;
}


static int check_root_path(char *path, int protected_file)
{
    struct stat st;
    int rc;
    char *target;

    if (lstat(path, &st) == -1)
        return root_path_error(
            protected_file ? "lscgid: lstat() root-protected file"
                           : "lscgid: lstat() executable",
            path);

    if (st.st_uid != 0)
        return root_path_denied(
            protected_file
                ? (S_ISLNK(st.st_mode)
                    ? "lscgid: protected file link is not owned by root"
                    : "lscgid: protected file is not owned by root")
                : (S_ISLNK(st.st_mode)
                    ? "lscgid: root executable link is not owned by root"
                    : "lscgid: root executable is not owned by root"),
            path);
    if (!S_ISLNK(st.st_mode) && (st.st_mode & (S_IWGRP | S_IWOTH)))
        return root_path_denied(
            protected_file
                ? "lscgid: protected file is writable by non-root"
                : "lscgid: root executable is writable by non-root",
            path);
    if (protected_file && !S_ISLNK(st.st_mode) && !S_ISREG(st.st_mode))
        return root_path_denied(
            "lscgid: protected file is not a regular file", path);

    rc = check_root_parent(
        path,
        protected_file ? "lscgid: stat() protected file directory"
                       : "lscgid: stat() executable directory",
        protected_file ? "lscgid: protected file directory is not protected"
                       : "lscgid: root executable directory is not protected");
    if (rc || !S_ISLNK(st.st_mode))
        return rc;

    target = realpath(path, NULL);
    if (!target)
        return root_path_error(
            protected_file ? "lscgid: realpath() protected file link"
                           : "lscgid: realpath() executable link",
            path);

    if (stat(target, &st) == -1)
        rc = root_path_error(
            protected_file ? "lscgid: stat() protected file link target"
                           : "lscgid: stat() executable link target",
            target);
    else if (st.st_uid != 0)
        rc = root_path_denied(
            protected_file
                ? "lscgid: protected file link target is not owned by root"
                : "lscgid: root executable link target is not owned by root",
            target);
    else if (st.st_mode & (S_IWGRP | S_IWOTH))
        rc = root_path_denied(
            protected_file
                ? "lscgid: protected file link target is writable by non-root"
                : "lscgid: root executable link target is writable by non-root",
            target);
    else if (protected_file && !S_ISREG(st.st_mode))
        rc = root_path_denied(
            "lscgid: protected file link target is not a regular file",
            target);
    else
        rc = check_root_parent(
            target,
            protected_file ? "lscgid: stat() protected target directory"
                           : "lscgid: stat() executable directory",
            protected_file
                ? "lscgid: protected target directory is not protected"
                : "lscgid: root executable directory is not protected");
    free(target);
    return rc;
}


int check_root_exec_path(lscgid_t *cgi)
{
    if (geteuid() != 0 || cgi->m_data.m_uid != 0)
        return 0;
    return check_root_executable(cgi->m_pCGIDir);
}


int check_root_executable(char *path)
{
    return check_root_path(path, 0);
}


int check_root_protected_file(char *path)
{
    return check_root_path(path, 1);
}


int check_root_protected_directory(char *path)
{
    struct stat st;

    if (lstat(path, &st) == -1)
        return root_path_error(
            "lscgid: lstat() root-protected directory", path);
    if (!S_ISDIR(st.st_mode) || st.st_uid != 0 ||
        (st.st_mode & (S_IWGRP | S_IWOTH)))
        return root_path_denied(
            "lscgid: root-protected directory is not protected", path);
    return check_root_parent(
        path, "lscgid: stat() protected directory parent",
        "lscgid: protected directory parent is not protected");
}
