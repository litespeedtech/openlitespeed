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
#define OP_FLAG_SOURCE_CREATE       128
#define OP_FLAG_NO_SANDBOX          256
#define OP_FLAG_DO_BIND             512
#define OP_FLAG_LAST                1024

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

/* For lscgid modules.  */
/* Files */
#define NOSANDBOX_PGM_ONLY  "lshostexec"
#define NOSANDBOX_TOP_FMT   "%s/lsns"
#define NOSANDBOX_PGM       NOSANDBOX_TOP_FMT "/bin/" NOSANDBOX_PGM_ONLY
#define NOSANDBOX_DIR_FMT   NOSANDBOX_TOP_FMT "/conf"
#define NOSANDBOX_CONF      NOSANDBOX_DIR_FMT "/hostexec.conf"
#define NOSANDBOX2_CONF     NOSANDBOX_DIR_FMT "/no-sandbox.conf"
#define NOSANDBOX_FILE_FMT  "hostexec.%d.sock"
#define NOSANDBOX_FORMAT    NOSANDBOX_DIR_FMT "/" NOSANDBOX_FILE_FMT
#define NOSANDBOX_SOCKET_FILE_FMT NOSANDBOX_DIR_FMT "/current-hostexec-sock"
#define NOSANDBOX_FORCE_SYMLINK NOSANDBOX_DIR_FMT "/hostexec.symlink"

void ls_stderr(const char * fmt, ...);

char *nsopts_lsws_home(char *path, size_t path_max);
int nsopts_rc_from_errno(int err);

int nsopts_get(lscgid_t *pCGI, int *allocated, SetupOp **setupOps,
               size_t *setupOps_size, char **nsconf, char **nsconf2);

void nsopts_free(SetupOp **setupOps);
void nsopts_free_conf(char **nsconf2);
void nsopts_free_all(int *allocated, SetupOp **setupOps, size_t *setupOps_size,
                     char **nsconf, char **nsconf2);
void nsopts_free_members(SetupOp *setupOps);

extern int s_ns_debug;

extern SetupOp     s_SetupOp_default[];
extern int         s_setupOp_default_size;
extern SetupOp     s_setupOp_all[];
extern int         s_setupOp_all_size;
extern int         s_listenPid;
extern int         s_listenPidStatus;
extern int         s_ns_osmajor;
extern int         s_ns_osminor;


#if 1
char *ns_lsws_home(char *path, size_t path_max);
char *nosandbox_socket_file(char *file, int max);
int nsnosandbox_socket_file_name(char *file_name, int max);
void debug_message(const char *fmt, ...);
#define DEBUG_MESSAGE(fmt, ...)  debug_message("%s Line #%d pid: %d " fmt, __FILE__, __LINE__, getpid(), ##__VA_ARGS__);

#if defined(NS_MAIN)
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
extern FILE *s_ns_debug_file;

char *ns_lsws_home(char *path, size_t path_max)
{
    char validate[path_max];
    ssize_t len = readlink("/proc/self/exe", path, path_max);
    if (len < 0)
    {
        DEBUG_MESSAGE("Unexpected error getting link: %s\n", strerror(errno));
        strncpy(path, "/usr/local/lsws/", path_max);
        return path;
    }
    path[len] = 0;
    char *slash = strrchr(path, '/');
    if (slash)
    {
        *(slash) = 0;
        slash = strrchr(slash, '/');
        if (slash)
            *(slash + 1) = 0;
    }
    snprintf(validate, path_max, "%sVERSION", path);
    if (access(validate, 0))
    {
        DEBUG_MESSAGE("Use default directory instead of resolved path (%s) for LSWS_HOME\n", path);
        strncpy(path, "/usr/local/lsws/", path_max);
    }
    return path;
}

char *nosandbox_socket_file(char *file, int max)
{
    char lsws_path[256];
    snprintf(file, max, NOSANDBOX_SOCKET_FILE_FMT, ns_lsws_home(lsws_path, sizeof(lsws_path)));
    return file;
}


int nsnosandbox_socket_file_name(char *file_name, int max)
{
    char file[512];
    int fd = open(nosandbox_socket_file(file, sizeof(file)), O_RDONLY);
    if (fd == -1)
    {
        int err = errno;
        DEBUG_MESSAGE("Can not open nosandbox file: %s: %s\n", file, strerror(err));
        ls_stderr("Can not open nosandbox file: %s: %s\n", file, strerror(err));
        return -1;
    }    
    int sz = read(fd, file_name, max - 1);
    close(fd);
    if (sz < 0)
    {
        int err = errno;
        DEBUG_MESSAGE("Can not read nosandbox file: %s: %s\n", file, strerror(err));
        ls_stderr("Can not read nosandbox file: %s: %s\n", file, strerror(err));
        return -1;
    }
    file_name[sz] = 0;
    return 0;
}

void ns_init_debug()
{
    if (s_ns_debug)
    {
        if (access("/lsnsdebug", 0) == 0)
            return;
        DEBUG_MESSAGE("Removed trace file - turn off tracing\n")
        s_ns_debug = 0;
        fclose(s_ns_debug_file);
        s_ns_debug_file = NULL;
        return;
    }
    char *penv = getenv("LSNSDEBUG");
    if (penv)
    {
        if ((s_ns_debug_file = fopen(penv, "a")))
        {
            s_ns_debug = 1;
            fcntl(fileno(s_ns_debug_file), F_SETFD, FD_CLOEXEC);
            DEBUG_MESSAGE("Enabled debugging via environment variable\n");
            return;
        }
    }
    s_ns_debug = !access("/lsnsdebug", 0);
    //DEBUG_MESSAGE("Activating debugging on existence of /lsnsdebug\n");
    if (s_ns_debug && !s_ns_debug_file)
    {
        FILE *files_name = fopen("/lsnsdebug", "r");
        char filename[256];
        if (files_name && fgets(filename, sizeof(filename), files_name))
        {
            char *nl = strchr(filename, '\n');
            if (nl)
                *nl = 0;
            DEBUG_MESSAGE("Trying to use debug filename: %s\n", filename);
            FILE *debug_file = fopen(filename, "a");
            if (debug_file)
            {
                s_ns_debug_file = debug_file;
                DEBUG_MESSAGE("Switched to debug file: %s\n", filename);
                fcntl(fileno(s_ns_debug_file), F_SETFD, FD_CLOEXEC);
                fchmod(fileno(s_ns_debug_file), 0666); // To allow children to write to it.
                setenv("LSNSDEBUG", filename, 1);
            }
            else
            {
                DEBUG_MESSAGE("Can't open debug file: leave alone: %s\n",
                              strerror(errno));
            }
        }
        else
        {
            DEBUG_MESSAGE("Can't open or get filename: %s, using stderr\n",
                          strerror(errno));
        }
        if (files_name)
            fclose(files_name);
    }
}


void debug_message (const char *fmt, ...)
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
#endif // NS_MAIN
#else // DEBUGGING
#define DEBUG_MESSAGE(...)
#endif


#ifdef __cplusplus
}
#endif

#endif // Linux
#endif // _NS_OPTS_H

