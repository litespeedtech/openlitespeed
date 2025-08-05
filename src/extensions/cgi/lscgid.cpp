/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
#include <signal.h>

#ifndef _XPG4_2
# define _XPG4_2
#endif

#include "lscgid.h"

#include <lsdef.h>
#include <util/fdpass.h>
#include <util/stringtool.h>
#include <http/httpserverconfig.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#include "cgroupconn.h"
#include "cgroupuse.h"
#include "ns.h"
#include "nsopts.h" // For DEBUG_MESSAGE
#include "use_bwrap.h"
#include <sys/prctl.h>
#include <linux/capability.h>
#endif

void ls_stderr(const char * fmt, ...)
{
    char buf[1024];
    char *p = buf;
    struct timeval  tv;
    struct tm       tm;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    p += snprintf(p, 1024, "%04d-%02d-%02d %02d:%02d:%02d.%06d ",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec, (int)tv.tv_usec);
    fprintf(stderr, "%.*s", (int)(p - buf), buf);

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fprintf(stderr, "%s", buf);
}

static uid_t        s_uid;

#define HAS_CLOUD_LINUX
#ifdef HAS_CLOUD_LINUX
#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

static int s_enable_lve = 0;
static struct liblve *s_lve = NULL;

static void *s_liblve;
typedef int (*lve_is_available)(void);
typedef int (*lve_instance_init)(struct liblve *);
typedef int (*lve_destroy)(struct liblve *);
typedef int (*lve_enter)(struct liblve *, uint32_t, int32_t, int32_t,
                           uint32_t *);
typedef int (*lve_leave)(struct liblve *, uint32_t *);
typedef int (*lve_jail)(struct passwd *, char *);

static int (*fp_lve_is_available)(void) = NULL;
static int (*fp_lve_instance_init)(struct liblve *) = NULL;
static int (*fp_lve_destroy)(struct liblve *) = NULL;
static int (*fp_lve_enter)(struct liblve *, uint32_t, int32_t, int32_t,
                           uint32_t *) = NULL;
static int (*fp_lve_leave)(struct liblve *, uint32_t *) = NULL;
static int (*fp_lve_jail)(struct passwd *, char *) = NULL;
static int load_lve_lib()
{
    s_liblve = dlopen("liblve.so.0", RTLD_NOW | RTLD_GLOBAL);
    if (s_liblve)
    {
        fp_lve_is_available = (lve_is_available)dlsym(s_liblve, "lve_is_available");
        if (dlerror() == NULL)
        {
            if (!(*fp_lve_is_available)())
            {
                int uid = getuid();
                if (uid)
                {
                    setreuid(s_uid, uid);
                    if (!(*fp_lve_is_available)())
                    {
                        s_enable_lve = 0;
                        ls_stderr("LVE disabled, lve_is_available() failure.\n");
                    }
                    setreuid(uid, s_uid);
                }
            }
        }
    }
    else
    {
        s_enable_lve = 0;
        ls_stderr("LVE disabled, dlopen() failed: %s.\n", dlerror());
    }
    return (s_liblve) ? 0 : -1;
}


static int init_lve()
{
    int rc;
    if (!s_liblve)
        return LS_FAIL;
    fp_lve_instance_init = (lve_instance_init)dlsym(s_liblve, "lve_instance_init");
    fp_lve_destroy = (lve_destroy)dlsym(s_liblve, "lve_destroy");
    fp_lve_enter = (lve_enter)dlsym(s_liblve, "lve_enter");
    fp_lve_leave = (lve_leave)dlsym(s_liblve, "lve_leave");
    if (s_enable_lve >= 2)
        fp_lve_jail = (lve_jail)dlsym(s_liblve, "jail");

    if (s_lve == NULL)
    {
        rc = (*fp_lve_instance_init)(NULL);
        s_lve = (struct liblve *)malloc(rc);
    }
    rc = (*fp_lve_instance_init)(s_lve);
    if (rc != 0)
    {
        perror("lscgid: Unable to initialize LVE");
        free(s_lve);
        s_lve = NULL;
        return LS_FAIL;
    }
    //ls_stderr("lscgid (%d) LVE initialized !\n", s_pid );

    //ls_stderr("lscgid (%d) LVE initialized: %d, %p !\n", s_pid,
    //        s_enable_lve, fp_lve_jail );
    return 0;

}

#endif
#endif

static char         s_pSecret[24];
static pid_t        s_parent;
static int          s_run = 1;
static char         s_sDataBuf[16384];
static int          s_fdControl = -1;
static int          s_ns_enabled = 0;


static void log_cgi_error(const char *func, const char *arg,
                                                    const char *explanation)
{
    char err[512];
    int n = ls_snprintf(err, sizeof(err) - 1, "%s:%s%.*s %s\n", func, (arg) ? arg : "",
                    !!arg, ":", explanation ? explanation : strerror(errno));
    write(STDERR_FILENO, err, n);
}


#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
static void set_cgi_error(char *func, char *arg)
{
    log_cgi_error(func, arg, NULL);
}
#endif

static int timed_read(int fd, char *pBuf, int len, int timeout)
{
    struct pollfd   pfd;
    time_t          begin;
    time_t          cur;
    int             left = len;
    int             ret;

    begin = cur = time(NULL);
    pfd.fd      = fd;
    pfd.events  = POLLIN;

    while (left > 0)
    {
        ret = poll(&pfd, 1, 1000);
        if (ret == 1)
        {
            if (pfd.revents & POLLIN)
            {
                ret = read(fd, pBuf, left);
                if (ret > 0)
                {
                    left -= ret;
                    pBuf += ret;
                }
                else if (ret <= 0)
                {
                    if (ret)
                        log_cgi_error("scgid: read()", NULL, NULL);
                    else
                        log_cgi_error("scgid", NULL, "pre-mature request");
                    return LS_FAIL;
                }
            }
        }
        if (ret == -1)
        {
            if (errno != EINTR)
            {
                log_cgi_error("scgid: poll()", NULL, NULL);
                return LS_FAIL;
            }
        }

        cur = time(NULL);
        if (cur - begin >= timeout)
        {
            log_cgi_error("scgid", NULL, "request timed out!");
            return LS_FAIL;
        }
    }
    return len;
}


static int writeall(int fd, const char *pBuf, int len)
{
    int left = len;
    int ret;
    while (left > 0)
    {
        ret = write(fd, pBuf, left);
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;
            return LS_FAIL;
        }
        if (ret > 0)
        {
            left -= ret;
            pBuf += ret;
        }
    }
    return len;
}


static int cgiError(int fd, int status)
{
    char achBuf[ 256 ];
    int ret = ls_snprintf(achBuf, sizeof(achBuf) - 1,
                          "Status:%d\n\nInternal error.\n", status);
    writeall(fd, achBuf, ret);
    close(fd);
    return 0;
}


static int set_uid_chroot(uid_t uid, gid_t gid, char *pChroot)
{
    int rv;

    //if ( !uid || !gid )  //do not allow root
    //{
    //    return LS_FAIL;
    //}
    struct passwd *pw = getpwuid(uid);
    rv = setgid(gid);
    if (rv == -1)
    {
        log_cgi_error("lscgid: setgid()", NULL, NULL);
        return LS_FAIL;
    }
    if (pw && (pw->pw_gid == gid))
    {
        rv = initgroups(pw->pw_name, gid);
        if (rv == -1)
        {
            log_cgi_error("lscgid: initgroups()", NULL, NULL);
            return LS_FAIL;
        }
    }
    else
    {
        rv = setgroups(1, &gid);
        if (rv == -1)
            log_cgi_error("lscgid: setgroups()", NULL, NULL);
    }
    if (pChroot)
    {
        rv = chroot(pChroot);
        if (rv == -1)
        {
            log_cgi_error("lscgid: chroot()", NULL, NULL);
            return LS_FAIL;
        }
    }
    rv = setuid(uid);
    if (rv == -1)
    {
        log_cgi_error("lscgid: setuid()", NULL, NULL);
        return LS_FAIL;
    }
    return 0;
}


//called after uid/gid/chroot change
static int changeStderrLog(lscgid_t *pCGI)
{
    const char *path = pCGI->m_stderrPath;
    if (pCGI->m_pChroot)
    {
        if (pCGI->m_data.m_chrootPathLen > 1
            && strncmp(path, pCGI->m_pChroot, pCGI->m_data.m_chrootPathLen) == 0)
        {
            path += pCGI->m_data.m_chrootPathLen;
            if (*path != '/')
                --path;
        }
    }
    int newfd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (newfd == -1)
    {
        ls_stderr("lscgid (%d): failed to change stderr log to: %s\n",
                  getpid(), path);
        return -1;
    }
    if (newfd != 2)
    {
        dup2(newfd, 2);
        close(newfd);
    }
    return 0;
}


int apply_rlimits_uid_chroot_stderr(lscgid_t *cgid)
{
    lscgid_req *req = &cgid->m_data;
    //ls_stderr("Proc: %ld, data: %ld\n", pCGI->m_nproc.rlim_cur,
    //                        pCGI->m_data.rlim_cur );

#if defined(RLIMIT_NPROC)
    if (req->m_nproc.rlim_cur)
        setrlimit(RLIMIT_NPROC, &req->m_nproc);
#endif

    if ((!s_uid) && (req->m_uid || req->m_gid))
    {
        if (set_uid_chroot(req->m_uid, req->m_gid, cgid->m_pChroot) == -1)
            return 403;
    }

    if (cgid->m_stderrPath)
        changeStderrLog(cgid);

#if defined(RLIMIT_AS) || defined(RLIMIT_DATA) || defined(RLIMIT_VMEM)
    if (req->m_data.rlim_cur)
    {
#if defined(RLIMIT_AS)
        setrlimit(RLIMIT_AS, &req->m_data);
#elif defined(RLIMIT_DATA)
        setrlimit(RLIMIT_DATA, &req->m_data);
#elif defined(RLIMIT_VMEM)
        setrlimit(RLIMIT_VMEM, &req->m_data);
#endif
    }
#endif

    return 0;
}


#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)


static const char *s_user_slice_dir = NULL;

static int detect_cgroup_user_slice_dir()
{
    struct stat st;
    if (stat("/sys/fs/cgroup/user.slice", &st) == 0)
        s_user_slice_dir = "/sys/fs/cgroup/user.slice";
    else if (stat("/sys/fs/cgroup/systemd/user.slice", &st) == 0)
        s_user_slice_dir = "/sys/fs/cgroup/systemd/user.slice";
    else
    {
        s_user_slice_dir = (const char *)-1LL;
        ls_stderr("CGROUPS: No user slice dir - you must configure for cgroups v2\n");
        return -1;
    }
    return 0;
}


bool is_cgroup_v2_available()
{
    if (s_user_slice_dir == NULL)
        detect_cgroup_user_slice_dir();
    if (s_user_slice_dir == (const char *)-1LL)
        return false;
    return true;
}


static int cgroup_v2(int uid, int pid)
{
    char user_slice[1024];

    if (!is_cgroup_v2_available())
        return -1;

    snprintf(user_slice, sizeof(user_slice), "%s/user-%d.slice",
             s_user_slice_dir, uid);
    if (mkdir(user_slice, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0
        || errno == EEXIST)
    {
        strcat(user_slice, "/litespeed-exec.scope");
        if (mkdir(user_slice, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
                    == 0 || errno == EEXIST)
        {
            strcat(user_slice, "/cgroup.procs");
            FILE *fp = fopen(user_slice, "a");
            //if there is no cgroup, we may hit permission denied. Wait moment.
            if (fp == NULL)
            {
                struct timespec ns;
                ns.tv_sec = 0;
                ns.tv_nsec = 1 * 1000000; //10ms
                int count = 200;
                while (fp == NULL && count--)
                {
                    nanosleep(&ns, NULL);
                    fp = fopen(user_slice, "a");
                }
            }
            if (fp == NULL)
            {
                ls_stderr("Cgroups Error opening %s for writing: %s", user_slice,
                          strerror(errno));
                return -1;
            }
            else
            {
                if (fprintf(fp, "%d\n", (int) pid) < 0)
                    ls_stderr("Cgroups Error writing to %s: %s", user_slice,
                              strerror(errno));
                if (fclose(fp) < 0)
                    ls_stderr("Cgroups Error closing %s: %s", user_slice,
                              strerror(errno));
                ls_stderr("Cgroups returning success file: %s, pid: %d\n",
                          user_slice, pid);

                char systemd_start[128];
                snprintf(systemd_start, sizeof(systemd_start), "/bin/systemctl start user-%d.slice", uid);
                system(systemd_start);

                return 0;
            }
        }
        else
            ls_stderr("Cgroups Error creating user slice directory: %s\n", strerror(errno));
    }
    return -1;
}


static int cgroup_env(lscgid_t *pCGI)
{
    if (getuid())
        return -1;

    char **env_arr = pCGI->m_env;
    char  *prefix = (char *)"LS_CGROUP=";
    int    prefix_len = 10;
    while (*env_arr)
    {
        char *env = *env_arr;
        if (!(strncmp(env, prefix, prefix_len)))
        {
            char *val = env + prefix_len;
            if ((*val == 'y') || (*val == 'Y'))
            {
                return 0;
            }
            return -1; // not a valid value then
        }
        ++env_arr;
    }
    return -1;
}

static int cgroup_activate(lscgid_t *pCGI)
{
    int rc = -1;
    CGroupUse *use = NULL;
    CGroupConn *conn = NULL;

    int uid = geteuid();
    seteuid(0);
    conn = new CGroupConn();
    if (!conn)
        log_cgi_error("execute cgi create cgroup conn", NULL, NULL);
    else if (conn->create() == -1)
        log_cgi_error("execute cgi create cgroup conn", NULL, conn->getErrorText());
    else
    {
        seteuid(0);
        use = new CGroupUse(conn);
        if (!use)
            log_cgi_error("execute_cgi create cgroup use", NULL, conn->getErrorText());
        else if (use->apply(pCGI->m_data.m_uid) == -1)
            log_cgi_error("execute_cgi apply uid", NULL, conn->getErrorText());
        else
            rc = 0;
    }
    seteuid(uid);
    if (use)
        delete use;
    if (conn)
        delete conn;
    return rc;
}


static int cgroup_process(lscgid_t *pCGI)
{
    if (!cgroup_env(pCGI))
        return cgroup_activate(pCGI);
    return 0;
}
#endif


//called before uid/gid/chroot change.
static int fixStderrLogPermission(lscgid_t *pCGI)
{
    struct stat st;
    const char *path = pCGI->m_stderrPath;

    if (lstat(path, &st) == -1)
        return 0;
    if (S_ISLNK(st.st_mode))
        return -1;
    if (st.st_uid != pCGI->m_data.m_uid && pCGI->m_data.m_uid > 0)
    {
        pCGI->m_stderrPath = NULL;
        int newfd = open(path, O_WRONLY | O_APPEND, 0644);
        if (newfd == -1)
        {
            ls_stderr("lscgid (%d): failed to open stderr: %s\n",
                      getpid(), path);
            return -1;
        }
        if (newfd != 2)
        {
            dup2(newfd, 2);
            close(newfd);
        }
    }
    return 0;
}


static int execute_cgi(lscgid_t *pCGI)
{
    char ch;
    uint32_t pid = (uint32_t)getpid();

    setsid();
    //applyLimits(&pCGI->m_data);

    if (pCGI->m_stderrPath)
        fixStderrLogPermission(pCGI);

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    if (pCGI->m_cgroup && pCGI->m_data.m_uid != 0)
    {
        cgroup_v2(pCGI->m_data.m_uid, pid);
    }
#endif

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    if (s_ns_enabled && pCGI->m_ns)
    {
        int rc = 0, done = 0;
        rc = ns_exec_ols(pCGI, &done);
        if (rc || done)
            return rc;
    }
#endif

    if (setpriority(PRIO_PROCESS, 0, pCGI->m_data.m_priority))
        perror("lscgid: setpriority()");

    if (pCGI->m_stderrPath)
        fixStderrLogPermission(pCGI);

#ifdef HAS_CLOUD_LINUX

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    if (s_lve && pCGI->m_data.m_uid)   //root user should not do that
    {
        uint32_t cookie;
        int ret = -1;
        //int count;
        //for( count = 0; count < 10; ++count )
        {
            ret = (*fp_lve_enter)(s_lve, pCGI->m_data.m_uid, -1, -1, &cookie);
            //if ( !ret )
            //    break;
            //usleep( 10000 );
        }
        if (ret < 0)
        {
            ls_stderr("lscgid (%d): enter LVE (%d) : result: %d !\n", pid,
                      pCGI->m_data.m_uid, ret);
            log_cgi_error("lscgid", NULL, "lve_enter() failure, reached resource limit");
            return 500;
        }
    }
#endif

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    if (!s_uid && pCGI->m_data.m_uid && fp_lve_jail)
    {
        char  error_msg[1024] = "";
        int ret;
        struct passwd *pw = getpwuid(pCGI->m_data.m_uid);
        if (pw)
        {
            ret = (*fp_lve_jail)(pw, error_msg);
            if (ret < 0)
            {
                ls_stderr("lscgid (%d): LVE jail(%d) result: %d, error: %s !\n",
                          pid, pCGI->m_data.m_uid, ret, error_msg);
                //set_cgi_error( "lscgid: CloudLinux jail() failure.", NULL );
                return 508;
            }
        }
    }
#endif

#endif

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    if (pCGI->m_data.m_flags & LSCGID_FLAG_DROP_CAPS)
    {
        if (0 != prctl(PR_CAPBSET_DROP, CAP_SETUID, 0, 0, 0))
            ls_stderr("dropping capabilities failed: %s", strerror(errno));
    }
#endif

    ch = *(pCGI->m_argv[0]);
    * pCGI->m_argv[0] = 0;
    const char *dir = pCGI->m_cwdPath;
    if (!dir)
        dir = pCGI->m_pCGIDir;
    if (chdir(dir) == -1)
    {
        int error = errno;
        log_cgi_error("lscgid: chdir()", pCGI->m_pCGIDir, NULL);
        switch (error)
        {
        case ENOENT:
            return 404;
        case EACCES:
            return 403;
        default:
            return 500;
        }
    }
    *(pCGI->m_argv[0]) = ch;
    if (ch == '&')
    {
        static const char sHeader[] = "Status: 200\r\n\r\n";
        writeall(STDOUT_FILENO, sHeader, sizeof(sHeader) - 1);
        pCGI->m_pCGIDir = (char *)"/bin/sh";
        pCGI->m_argv[0] = (char *)"/bin/sh";
    }
    else
    {
        //pCGI->m_argv[0] = strdup( pCGI->m_pCGIDir );
        pCGI->m_argv[0] = pCGI->m_pCGIDir;
    }

    umask(pCGI->m_data.m_umask);
#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    if (pCGI->m_bwrap)
    {
        int rc = 0, done = 0;
        rc = exec_using_bwrap(pCGI, set_cgi_error, &done);
        if (rc || done)
            return rc;
    }
#endif

    if (apply_rlimits_uid_chroot_stderr(pCGI) == 403)
        return 403;

    //ls_stderr( "execute_cgi m_umask=%03o\n", pCGI->m_data.m_umask );
    if (execve(pCGI->m_pCGIDir, pCGI->m_argv, pCGI->m_env) == -1)
    {
        log_cgi_error("lscgid: execve()", pCGI->m_pCGIDir, NULL);
        return 500;
    }

    return 0;
}


static int process_req_data(lscgid_t *cgi_req)
{
    char *p = cgi_req->m_pBuf;
    char *pEnd = p + cgi_req->m_data.m_szData;
    int i;
    unsigned short len;
    if (cgi_req->m_data.m_chrootPathLen > 0)
    {
        cgi_req->m_pChroot = p;
        p += cgi_req->m_data.m_chrootPathLen;
    }
    else
        cgi_req->m_pChroot = NULL;
    cgi_req->m_pCGIDir = p;
    p += cgi_req->m_data.m_exePathLen;
    if (p > pEnd)
    {
        ls_stderr("exePathLen=%d\n", cgi_req->m_data.m_exePathLen);
        return 500;
    }
    cgi_req->m_argv[0] = p;
    p += cgi_req->m_data.m_exeNameLen + 1;

//    ls_stderr("exePath=%s\n", cgi_req->m_pCGIDir );
//    ls_stderr("nargv=%d\n", cgi_req->m_data.m_nargv );
//    ls_stderr("exeName=%s\n", cgi_req->m_argv[0] );
//    ls_stderr("exePath ends with %s\n",
//            cgi_req->m_pCGIDir + cgi_req->m_data.m_exePathLen );

    for (i = 1; i < cgi_req->m_data.m_nargv - 1; ++i)
    {
#if defined( sparc )
        *(unsigned char *)&len = *p++;
        *(((unsigned char *)&len) + 1) = *p++;
#else
        len = *((unsigned short *)p);
        p += sizeof(short);
#endif
        cgi_req->m_argv[i] = p;
        p += len;
        if (p > pEnd)
            return 500;
//        ls_stderr("arg %d, len=%d, str=%s\n", i, len, cgi_req->m_argv[i] );
    }
    if (*p != 0 || *(p + 1) != 0)
    {
        ls_stderr("argv is not terminated with \\0\\0\n");
        return 500;
    }
    p += sizeof(short);
    if (p > pEnd)
        return 500;
    cgi_req->m_argv[i] = NULL;

//    ls_stderr("nenv=%d\n", cgi_req->m_data.m_nenv );
    for (i = 0; i < cgi_req->m_data.m_nenv - 1; ++i)
    {
#if defined( sparc )
        *(unsigned char *)&len = *p++;
        *(((unsigned char *)&len) + 1) = *p++;
#else
        len = *((unsigned short *)p);
        p += sizeof(short);
#endif
        cgi_req->m_env[i] = p;
        //ls_stderr("env %d, len=%d, str=%s\n", i, len, cgi_req->m_env[i]);

        if (*p == 'L')
        {
            if (strncasecmp(p, "LS_STDERR_LOG=", 14) == 0)
            {
                --i;
                --cgi_req->m_data.m_nenv;
                cgi_req->m_stderrPath = p + 14;
            }
            if (strncasecmp(p, "LS_CWD=", 7) == 0)
            {
                --i;
                --cgi_req->m_data.m_nenv;
                cgi_req->m_cwdPath = p + 7;
            }
            else if (strncasecmp(p, "LS_BWRAP=", 9) == 0)
            {
                --i;
                --cgi_req->m_data.m_nenv;
                cgi_req->m_bwrap = strtol(p + 9, NULL, 10);
            }
            else if (strncasecmp(p, "LS_CGROUP=", 10) == 0)
            {
                --i;
                --cgi_req->m_data.m_nenv;
                cgi_req->m_cgroup = strtol(p + 10, NULL, 10);
            }
            else if (strncasecmp(p, "LS_NS=", LS_NS_LEN + 1) == 0)
            {
                --i;
                --cgi_req->m_data.m_nenv;
                cgi_req->m_ns = strtol(p + LS_NS_LEN + 1, NULL, 10);
            }
        }
        p += len;
        if (p > pEnd)
            return 500;
    }
    if (*p != 0 || *(p + 1) != 0)
    {
        ls_stderr("env is not terminated with \\0\\0\n");
        return 500;
    }
    p += sizeof(short);
    if (p != pEnd)
    {
        ls_stderr("header is too big\n");
        return 500;
    }
    cgi_req->m_env[i] = NULL;
    return 0;

}


#define MAX_CGI_DATA_LEN 65536
static int process_req_header(lscgid_t *cgi_req)
{
    char achMD5[16];
    int totalBufLen;
    memmove(achMD5, cgi_req->m_data.m_md5, 16);
    memmove(cgi_req->m_data.m_md5, s_pSecret, 16);
    StringTool::getMd5((const char *)&cgi_req->m_data,
                       sizeof(lscgid_req), cgi_req->m_data.m_md5);
   if (memcmp(cgi_req->m_data.m_md5, achMD5, 16) != 0)
    {
        log_cgi_error("lscgid", NULL, "request validation failed!");
        return 500;
    }
    totalBufLen = cgi_req->m_data.m_szData + sizeof(char *) *
                  (cgi_req->m_data.m_nargv + cgi_req->m_data.m_nenv);
    if ((unsigned int)totalBufLen < sizeof(s_sDataBuf))
        cgi_req->m_pBuf = s_sDataBuf;
    else
    {
        if (totalBufLen > MAX_CGI_DATA_LEN)
        {
            log_cgi_error("lscgid", NULL, "cgi header data is too big");
            return 500;

        }
        cgi_req->m_pBuf = (char *)malloc(totalBufLen);
        if (!cgi_req->m_pBuf)
        {
            log_cgi_error("lscgid: malloc()", NULL, NULL);
            return 500;
        }
    }
    cgi_req->m_argv = (char **)(cgi_req->m_pBuf + ((cgi_req->m_data.m_szData +
                                7) & ~7L));
    cgi_req->m_env = (char **)(cgi_req->m_argv + sizeof(char *) *
                               cgi_req->m_data.m_nargv);
    return 0;
}



static int recv_req(int fd, lscgid_t *cgi_req, int timeout)
{

    time_t          begin;
    time_t          cur;
    int             ret;

    begin = time(NULL);
    cgi_req->m_fdReceived = -1;
    ret = timed_read(fd, (char *)&cgi_req->m_data,
                     sizeof(lscgid_req), timeout - 1);
    if (ret == -1)
        return 500;
    ret = process_req_header(cgi_req);
    if (ret)
        return ret;
    //ls_stderr("1 Proc: %ld, data: %ld\n", cgi_req->m_data.m_nproc.rlim_cur,
    //                        cgi_req->m_data.m_data.rlim_cur );

    if (cgi_req->m_data.m_type == LSCGID_TYPE_SUEXEC)
    {
        uint32_t pid = (uint32_t)getpid();
        write(STDOUT_FILENO, (const void *)&pid, sizeof(pid));
    }

    cur = time(NULL);
    timeout -= cur - begin;
    ret = timed_read(fd, cgi_req->m_pBuf,
                     cgi_req->m_data.m_szData, timeout);
    if (ret == -1)
        return 500;

    ret = process_req_data(cgi_req);
    if (ret)
    {
        log_cgi_error("lscgid", NULL, "data error!");
        return ret;
    }
    if (cgi_req->m_data.m_type == LSCGID_TYPE_SUEXEC)
    {
        //cgi_req->m_fdReceived = recv_fd( fd );
        char nothing;
        if ((FDPass::readFd(fd, &nothing, 1, &cgi_req->m_fdReceived) == -1) ||
            (cgi_req->m_fdReceived == -1))
        {
            ls_stderr("lscgid: read_fd() failed: %s\n",
                    strerror(errno));
            return 500;
        }
        if (cgi_req->m_fdReceived != STDIN_FILENO)
        {
            dup2(cgi_req->m_fdReceived, STDIN_FILENO);
            close(cgi_req->m_fdReceived);
            cgi_req->m_fdReceived = -1;
        }
    }
    //ls_stderr("2 Proc: %ld, data: %ld\n", cgi_req->m_data.m_nproc.rlim_cur,
    //                        cgi_req->m_data.m_data.rlim_cur );
    return 0;

}


static int processreq(int fd)
{
    lscgid_t cgi_req;
    int ret;

    memset(&cgi_req, 0, sizeof(cgi_req));
    ret = recv_req(fd, &cgi_req, 10);
    if (ret)
        cgiError(fd, ret);
    else
    {
        ret = execute_cgi(&cgi_req);
        if (ret)
            cgiError(fd, ret);
    }
    return ret;
}


static void child_main(int fd)
{
    int closeit = 1;
    //close( LSCGID_LISTENSOCK_FD );
    if (s_fdControl != -1)
        close(s_fdControl);
    if (fd != STDIN_FILENO)
        dup2(fd, STDIN_FILENO);
    else
        closeit = 0;
    if (fd != STDOUT_FILENO)
        dup2(fd, STDOUT_FILENO);
    else
        closeit = 0;
    if (closeit && fd != STDOUT_FILENO)
        close(fd);
    processreq(STDOUT_FILENO);
    exit(0);
}


static int new_conn(int fd)
{
    pid_t pid;
    pid = fork();
    if (!pid)
        child_main(fd);
    close(fd);
    if (pid > 0)
        pid = 0;
    else
        perror("lscgid: fork() failed");
    return pid;
}

static int s_got_sigchild = 0;


static void processSigchild();


static int run(int fdServerSock)
{
    int ret;
    struct pollfd pfd;
    pfd.fd = fdServerSock;
    pfd.events = POLLIN;
    while (s_run)
    {
        ret = poll(&pfd, 1, 1000);
        if (ret == 1)
        {
            int fd = accept(fdServerSock, NULL, NULL);
            if (fd != -1)
                new_conn(fd);
            else
                perror("lscgid: accept() failed");
        }
        else
        {
            if (getppid() != s_parent)
                return 1;
        }
        if (s_got_sigchild)
            processSigchild();
    }
    return 0;
}


static void sigterm(int sig)
{
    s_run = 0;
}


static void sigusr1(int sig)
{
    pid_t pid = getppid();
    if (pid == s_parent)
        kill(pid, SIGHUP);
}


static void sigchild(int sig)
{
    s_got_sigchild = 1;
}


static void processSigchild()
{
    s_got_sigchild = 0;
    int status[2];
    while (1)
    {
        status[0] = waitpid(-1, &status[1], WNOHANG);
        if (status[0] <= 0)
        {
            //if ((pid < 1)&&( errno == EINTR ))
            //    continue;
            break;
        }
        if (s_fdControl != -1)
            write(s_fdControl, status, sizeof(status));
        if (WIFSIGNALED(status[1]))
        {
            int sig_num = WTERMSIG(status[1]);
            if (sig_num != 15)
            {
                ls_stderr("Cgid: Child process with pid: %d was killed by signal: %d, core dump: %d\n",
                          status[0], sig_num,
#ifdef WCOREDUMP
                          WCOREDUMP(status[1])
#else
                          - 1
#endif

                       );
            }
        }
        //ls_stderr( "reape child %d: status: %d\n", pid, status );
    }

}


//#define LOCAL_TEST

#ifndef sighandler_t
typedef void (*sighandler_t)(int);
#endif
sighandler_t my_signal(int sig, sighandler_t f)
{
    struct sigaction act, oact;

    act.sa_handler = f;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sig == SIGALRM)
    {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT; /* SunOS */
#endif
    }
    else
    {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
    }
    if (sigaction(sig, &act, &oact) < 0)
        return (SIG_ERR);
    return (oact.sa_handler);
}


int lscgid_main(int fd, char *argv0, const char *secret, char *pSock)
{
    int ret;
    char *sEnv = NULL;

    s_parent = getppid();
    my_signal(SIGCHLD, sigchild);
    my_signal(SIGINT, sigterm);
    my_signal(SIGTERM, sigterm);
    my_signal(SIGHUP, sigusr1);
    my_signal(SIGUSR1, sigusr1);
    signal(SIGPIPE, SIG_IGN);
    s_uid = geteuid();

    memcpy(s_pSecret, secret, 16);

#ifdef HAS_CLOUD_LINUX

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    if ((sEnv = getenv("LVE_ENABLE")) != NULL)
    {
        s_enable_lve = atol(sEnv);
        unsetenv("LVE_ENABLE");
        sEnv = NULL;
    }
    if (s_enable_lve && !s_uid)
    {
        load_lve_lib();
        if (s_enable_lve)
            init_lve();

    }
    s_ns_enabled = HttpServerConfig::getInstance().getNS();
#endif

#endif

#if defined(__FreeBSD__)
    //setproctitle( "%s", "httpd" );
#else
    memset(argv0, 0, strlen(argv0));

#ifdef IS_LSCPD
    strcpy(argv0, "lscpd (lscgid)");
#else
    strcpy(argv0, "openlitespeed (lscgid)");
#endif
#endif

    ret = run(fd);
    if ((ret) && (pSock) && (strncasecmp(pSock, "uds:/", 5) == 0))
    {
        pSock += 5;
        close(STDIN_FILENO);
        unlink(pSock);
    }
    return ret;
}


