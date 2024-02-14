/*
 * Copyright 2002-2020 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */
#ifndef _NSUTILS_H
#define _NSUTILS_H

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file nsutils.h
 * @brief Only used by ns.c and it's modules internally
 */

#define STRNUM_SIZE     12 // 0 terminated number string size
#define IS_PRIVILEGED   0

void ls_stderr(const char * fmt, ...)
#if __GNUC__
        __attribute__((format(printf, 1, 2)))
#endif
;

int nsutils_write_uid_gid_map(uid_t sandbox_uid,
                              uid_t parent_uid,
                              uid_t sandbox_gid,
                              uid_t parent_gid,
                              pid_t pid,
                              int   deny_groups,
                              int   map_root,
                              int   ignore_errors);
int clean_dir(char *dir);

extern int    s_clone_flags;
extern int    s_persisted;
extern char  *s_persisted_VH;
extern int    s_proc_fd;
extern gid_t  s_overflowgid;
extern uid_t  s_overflowuid;
extern verbose_callback_t s_verbose_callback;
extern char  *s_ns_conf;
extern char  *s_ns_conf2;

#ifdef __cplusplus
}
#endif


#endif // linux

#endif
