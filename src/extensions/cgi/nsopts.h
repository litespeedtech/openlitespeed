/*
 * Copyright 2002-2020 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */
#ifndef _NS_OPTS_H
#define _NS_OPTS_H

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file nsopts.h
 * @brief Only used by ns.c and nsopts.c
 */

typedef enum {
  SETUP_NOOP,
  SETUP_BIND_MOUNT,
  SETUP_RO_BIND_MOUNT,
  SETUP_DEV_BIND_MOUNT,
  SETUP_MOUNT_PROC,
  SETUP_MOUNT_DEV,
  SETUP_MOUNT_TMPFS,
  SETUP_MOUNT_MQUEUE,
  SETUP_MAKE_DIR,
  //SETUP_MAKE_FILE,
  //SETUP_MAKE_BIND_FILE,
  //SETUP_MAKE_RO_BIND_FILE,
  SETUP_MAKE_SYMLINK,
  SETUP_REMOUNT_RO_NO_RECURSIVE,
  SETUP_SET_HOSTNAME,
  SETUP_MAKE_PASSWD,
  SETUP_MAKE_GROUP,
  SETUP_BIND_TMP,
  SETUP_COPY,
  SETUP_NO_UNSHARE_USER,
} SetupOpType;

#define OP_FLAG_NO_CREATE_DEST      1
#define OP_FLAG_ALLOW_NOTEXIST      2
#define OP_FLAG_ALLOCATED_SOURCE    4
#define OP_FLAG_ALLOCATED_DEST      8
//#define OP_FLAG_ALLOCATED_OP        16
#define OP_FLAG_BWRAP_SYMBOL        32
#define OP_FLAG_RAW_OP              64
#define OP_FLAG_LAST                128

typedef struct _SetupOp SetupOp;

struct _SetupOp
{
  SetupOpType type;
  char       *source;
  char       *dest;
  int         flags;
  int         fd;
};

#define DEFAULT_ERR_RC  500


int nsopts_rc_from_errno(int err);

int nsopts_init(lscgid_t *pCGI, char **nsconf, char **nsconf2);

int nsopts_get(lscgid_t *pCGI, int *allocated, SetupOp **setupOps,
               size_t *setupOps_size, char **nsconf, char **nsconf2);

void nsopts_free(SetupOp **setupOps);
void nsopts_free_members(SetupOp *setupOps);
void nsopts_free_conf(char **nsconf, char **nsconf2);
void nsopts_free_all(int *allocated, SetupOp **setupOps, size_t *setupOps_size,
                     char **nsconf, char **nsconf2);
extern int s_ns_debug;
extern SetupOp     s_SetupOp_default[];

#if 1
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
extern FILE *s_ns_debug_file;

static void debug_message (const char *fmt, ...)
{
    if (!s_ns_debug)
        return;
    va_list ap;
    va_start(ap, fmt);
    FILE *f = stderr;
    if (s_ns_debug_file)
        f = s_ns_debug_file;
    /*
    int ctr = 0;
    do {
        f = fopen("/usr/local/lsws/logs/ns.dbg","a");
        if (!f) {
            ctr++;
            usleep(0);
        }
    } while ((!f) && (ctr < 1000));
    */
    if (f)
    {
        struct timespec ts;
        struct tm lt;
        clock_gettime(CLOCK_REALTIME, &ts);
        localtime_r(&ts.tv_sec, &lt);
        fprintf(f,"%02d/%02d/%04d %02d:%02d:%02d:%06ld ",
                lt.tm_mon + 1, lt.tm_mday, lt.tm_year + 1900, lt.tm_hour,
                lt.tm_min, lt.tm_sec, ts.tv_nsec / 1000);
        vfprintf(f, fmt, ap);
        fflush(f);
        /*
        fclose(f);
        */
    }
    va_end(ap);
}
#ifdef DEBUG_MESSAGE
#undef DEBUG_MESSAGE
#endif

#define DEBUG_MESSAGE(fmt, ...) {     \
    debug_message("%s Line #%d pid: %d " fmt, __FILE__, __LINE__, getpid(), ##__VA_ARGS__); \
    }
#else
#define DEBUG_MESSAGE(fmt, ...) {}
#endif


#ifdef __cplusplus
}
#endif

#endif // Linux
#endif // _NS_OPTS_H

