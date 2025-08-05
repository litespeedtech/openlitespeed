/*
 * Copyright 2020 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fsuid.h>
#include <sys/timeb.h>
#include <time.h>
#include <unistd.h>

#include "ns.h"
#include "nsopts.h"
#include "nspersist.h"
#include "nsipc.h"
#include "nsutils.h"
#include "lscgid.h"
#include "use_bwrap.h"

//extern "C"
//{
void ls_stderr(const char * fmt, ...)
#if __GNUC__
        __attribute__((format(printf, 1, 2)))
#endif
;

//} /* C */


static int write_file_at(int dirfd, const char *path, const char *content, 
                         int ignore_errors)
{
    int fd;
    int res;

    fd = openat (dirfd, path, O_RDWR | O_CLOEXEC, 0);
    if (fd == -1)
    {
        int err = errno;
        ls_stderr("Namespace error opening partial path: %s: %s\n", path, 
                  strerror(err));
        return nsopts_rc_from_errno(err);
    }
    
    if (content)
    {
        res = write(fd, content, strlen (content));
        if (res < 0 && !ignore_errors)
        {
            int err = errno;
            close (fd);
            ls_stderr("Namespace error writing partial path: %s: %s, len: %d\n", 
                      path, strerror(err), (int)strlen(content));
            return nsopts_rc_from_errno(err);
        }
        else if (res < 0)
            DEBUG_MESSAGE("Ignore error %s writing to %s\n", strerror(errno), 
                          path);
    }
    close (fd);
    return 0;
}


int nsutils_write_uid_gid_map(uid_t sandbox_uid,
                              uid_t parent_uid,
                              uid_t sandbox_gid,
                              uid_t parent_gid,
                              pid_t pid,
                              int   deny_groups,
                              int   map_root,
                              int   ignore_errors)
{
    char *dir = NULL;
    int dir_fd = -1, rc;
    uid_t old_fsuid = -1;
    char pid_str[STRNUM_SIZE * 6];

    //if (!s_ns_infos[NS_TYPE_USER].enabled)
    //{
    //    DEBUG_MESSAGE("Skip uid/gid map because user namespace is disabled\n");
    //    return 0;
    //}
    DEBUG_MESSAGE("write_uid_gid_map uid: %d->%d, gid: %d->%d, pid: %d, "
                  "deny_groups: %d, map_root: %d\n",
                  parent_uid, sandbox_uid, parent_gid, sandbox_gid, pid,
                  deny_groups, map_root);
    if (pid == -1)
        dir = "self";
    else
    {
        snprintf(pid_str, sizeof(pid_str), "%u", pid);
        dir = pid_str;
    }

    dir_fd = openat (s_proc_fd, dir, O_PATH);
    if (dir_fd < 0)
    {
        int err = errno;
        close(dir_fd);
        ls_stderr("Namespace error opening %s: %s\n", dir, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    if (map_root && parent_uid != 0 && sandbox_uid != 0)
        snprintf(pid_str, sizeof(pid_str), "0 %d 1\n%d %d 1\n", 
                 /*s_overflowuid*/0, sandbox_uid, parent_uid);
    else
        snprintf(pid_str, sizeof(pid_str), "%d %d 1\n", sandbox_uid, parent_uid);

    /* We have to be root to be allowed to write to the uid map
     * for setuid apps, so temporary set fsuid to 0 */
    if (IS_PRIVILEGED)
        old_fsuid = setfsuid(0);

    DEBUG_MESSAGE("Writing to uid_map: %s:", pid_str);
    rc = write_file_at(dir_fd, "uid_map", pid_str, ignore_errors);
    if (rc)
    {
        close(dir_fd);
        return rc;
    }

    
    if (map_root && parent_gid != 0 && sandbox_gid != 0)
        snprintf(pid_str, sizeof(pid_str), "0 %d 1\n%d %d 1\n", 
                 s_overflowgid, sandbox_gid, parent_gid);
    else
        snprintf(pid_str, sizeof(pid_str), "%d %d 1\n", sandbox_gid, parent_gid);

    /* We have to be root to be allowed to write to the uid map
     * for setuid apps, so temporary set fsuid to 0 */
    if (deny_groups &&
        (rc = write_file_at (dir_fd, "setgroups", "deny\n", ignore_errors)) != 0)
    {
        /* If /proc/[pid]/setgroups does not exist, assume we are
         * running a linux kernel < 3.19, i.e. we live with the
         * vulnerability known as CVE-2014-8989 in older kernels
         * where setgroups does not exist.
         */
        //if (errno != ENOENT)
        close(dir_fd);
        return rc;
    }

    /* We have to be root to be allowed to write to the gid map
     * for setuid apps, so temporary set fsuid to 0 */
    if (IS_PRIVILEGED)
        old_fsuid = setfsuid(0);
    DEBUG_MESSAGE("Writing to gid_map: %s:", pid_str);
    rc = write_file_at (dir_fd, "gid_map", pid_str, ignore_errors);
    if (rc)
    {
        close(dir_fd);
        return rc;
    }
    close(dir_fd);
    if (IS_PRIVILEGED)
    {
        setfsuid (old_fsuid);
        if (setfsuid (-1) != 0)
        {
            ls_stderr("Namespace fsuid set to %d, expected 0\n", setfsuid(-1));
            return DEFAULT_ERR_RC;
        }
    }
    return 0;
}


int clean_dir(char *dir)
{
    struct dirent *ent;
    DIR *dp = opendir(dir);
    DEBUG_MESSAGE("clean_dir: %s\n", dir);
    if (!dp)
    {
        int err = errno;
        ls_stderr("Namespace error opening directory %s to clean it: %s\n",
                  dir, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    int dir_len = strlen(dir);
    while ((ent = readdir(dp)))
    {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
            continue;
        int file_len = dir_len + strlen(ent->d_name) + 2;
        char *file = malloc(file_len);
        if (!file)
        {
            ls_stderr("Namespace can't allocate memory to cleanup dir %s\n",
                      dir);
            free(file);
            closedir(dp);
            return DEFAULT_ERR_RC;
        }
        snprintf(file, file_len, "%s/%s", dir, ent->d_name);
        if (ent->d_type == DT_DIR)
        {
            int rc = clean_dir(file);
            if (rc)
            {
                free(file);
                closedir(dp);
                return rc;
            }
        }
        DEBUG_MESSAGE("clean_dir remove: %s\n", file);
        if (remove(file))
        {
            int err = errno;
            ls_stderr("Namespace can't delete %s cleaning %s: %s\n", file, dir,
                      strerror(err));
            free(file);
            closedir(dp);
            return nsopts_rc_from_errno(err);
        }
        free(file);
    }
    closedir(dp);
    return 0;
}



#endif // Linux
