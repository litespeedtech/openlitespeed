/*
 * Copyright 2024 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#include <errno.h>
#include <getopt.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ns.h"
#include "nsopts.h" // For DEBUG_MESSAGE
#include "nspersist.h" // For PERSIST_PREFIX
#include "lscgid.h"
#include "use_bwrap.h"

static uid_t        s_uid;
static pid_t        s_pid;
char               *s_argv0 = NULL;

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
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}


int usage(const char *message)
{
    if (message)
        printf("%s\n", message);
    printf("cmd_ns - Enter a shell using a method like LiteSpeed\n");
    printf("Usage:\n");
    printf("  -u <user or ID>   : A specific user's name or ID to be used (nobody by default)\n");
    printf("  -s <vhost>        : Lets you specify a specific vhost for the specific user\n");
    printf("  -m <cmd>          : The command to run. /bin/bash by default\n");
    printf("  -d <dir>          : Default directory.  Defaults to home of UID\n");
    printf("  -c                : Whether cgroups should be enforced.\n");
    printf("  -f <config file>  : The config file to be used (system defaults if not specified)\n");
    printf("  -2 <config file>  : Additional vhost config file\n");
    printf("  -n                : If specified hostexec will NOT be enforced\n");
    printf("  -v                : Verbose output\n");
    printf("  -h                : Display this help message\n");
    return 1;
}


int verbose_callback(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    FILE *f = stdout;
    int rc = 0;
    if (f) {
        //struct timespec ts;
        //struct tm lt;
        //clock_gettime(CLOCK_REALTIME, &ts);
        //localtime_r(&ts.tv_sec, &lt);
        //fprintf(f,"%02d/%02d/%04d %02d:%02d:%02d:%06ld ", lt.tm_mon + 1,
        //        lt.tm_mday, lt.tm_year + 1900, lt.tm_hour, lt.tm_min, lt.tm_sec,
        //        ts.tv_nsec / 1000);
        rc = vfprintf(f, fmt, ap);
        //fflush(f);
    }
    va_end(ap);
    return rc;
}


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
        return -1;
    }
    return 0;
}


static int cgroup_v2(int uid, int pid)
{
    char user_slice[1024];
    if (s_user_slice_dir == NULL)
        detect_cgroup_user_slice_dir();
    if (s_user_slice_dir == (const char *)-1LL)
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
                ls_stderr("Error opening %s for writing: %s", user_slice,
                        strerror(errno));
                return -1;
            }
            else
            {
                fprintf(fp, "%d\n", (int) pid);
                fclose(fp);

                char systemd_start[128];
                snprintf(systemd_start, sizeof(systemd_start), "/bin/systemctl start user-%d.slice", uid);
                system(systemd_start);

                return 0;
            }
        }
    }
    return -1;
}

int main(int argc, char *argv[])
{
    int ch; 
    int cgroup = 0, rc = 0, must_exist = 1;
    char *vhost = NULL, *cfgfile = NULL, *cfg2file = NULL, *cmd = "/bin/bash", *dir = NULL;
    char env[256];
    struct passwd *pw = NULL;
    
    s_argv0 = argv[0];
    DEBUG_MESSAGE("Entering shell_ns program\n");
    if (getuid())
        return usage("Must be run as root");
    
    if (access(PERSIST_PREFIX, 0))
        return usage("Persistance directory: " PERSIST_PREFIX " not found");
    
    while ((ch = getopt(argc, argv, "u:s:m:d:cf:2:v?hn")) != -1)
    {
        switch (ch)
        {
            case 'u':
            {
                if (!(pw = getpwnam(optarg)) &&
                    (strspn(optarg, "0123456789") != strlen(optarg) ||
                     !(pw = getpwuid(atoi(optarg)))))
                    return usage("You must specify a valid user or ID for -u");
            }
            break;
                
            case 's':
                vhost = strdup(optarg);
                snprintf(env, sizeof(env), "LS_NS_VHOST=%s", cfg2file);
                putenv(env);
                break;
            
            case 'm':
                cmd = strdup(optarg);
                break;

            case 'd':
                dir = strdup(optarg);
                break;

            case 'c':
                cgroup = 1;
                break;

            case 'f':
                cfgfile = strdup(optarg);
                snprintf(env, sizeof(env), "LS_NS_CONF=%s", cfgfile);
                putenv(env);
                break;

            case '2':
                cfg2file = strdup(optarg);
                snprintf(env, sizeof(env), "LS_NS_CONF2=%s", cfg2file);
                putenv(env);
                break;

            case 'n':
                must_exist = 0;
                break;

            case 'v':
                printf("Verbose output specified\n");
                ns_setverbose_callback(verbose_callback);
                break;
                
            case ':':
                return usage("Missing parameter option value");
            
            default:
                return usage(NULL);
        }
    }
    if (!pw)
    {
        pw = getpwnam("nobody");
        if (!pw)
        {
            ls_stderr("Error getting details for the user nobody: %s\n", strerror(errno));
            return 1;
        }
    }
    printf("Using UID: %d\n", pw->pw_uid);
    if (!dir)
        dir = pw->pw_dir;
    if (access(dir, 0) != 0)
    {
        ls_stderr("Default directory for UID: %d, is %s and it doesn't exist\n", pw->pw_uid, pw->pw_dir);
        return 1;
    }
    s_uid = pw->pw_uid;
    s_pid = getpid();
    ns_not_lscgid();
    if (!ns_init_engine(cfgfile, 1))
    {
        ls_stderr("ns_init_engine disabled namespaces\n");
        return 1;
    }
    if (vhost)
        nspersist_setvhost(vhost);
    lscgid_t cgi;
    memset(&cgi, 0, sizeof(cgi));
    char systemd_start[128];
    snprintf(systemd_start, sizeof(systemd_start), "/bin/systemctl start user-%d.slice", s_uid);
    system(systemd_start);

    char *a[2];
    a[0] = cmd;
    a[1] = NULL;
    cgi.m_argv = a;
    cgi.m_cwdPath = dir;
    cgi.m_pCGIDir = a[0];
    cgi.m_cgroup = cgroup;
    //cgi.m_ns = 1;
    cgi.m_data.m_gid = pw->pw_gid;
    cgi.m_data.m_uid = pw->pw_uid;
    //cgi.m_ns_conf2 = cfg2file;
    if (cgi.m_cgroup && cgroup_v2(s_uid, s_pid))
    {
        ls_stderr("CGroup test failed\n");
        return 1;
    }
    int done = 0;
    rc = ns_exec(&cgi, must_exist, &done);
    ns_done(0);
    if (rc)
    {
        ls_stderr("NS error\n");
        return 1;
    }
    return 0;
}
