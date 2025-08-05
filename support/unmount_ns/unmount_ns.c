/*
 * Copyright 2020 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#include <getopt.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ns.h"
#include "nsopts.h" // For DEBUG_MESSAGE
#include "nspersist.h" // For PERSIST_PREFIX
#include "lscgid.h"
#include "use_bwrap.h"

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
    printf("unmount_ns - Dismount persisted mounts created by LiteSpeed\n");
    printf("Usage:\n");
    printf("  -u <user or ID> : A specific user's name or ID to be dismounted\n");
    printf("  -s <vhost>      : Lets you specify a specific vhost for the specific user\n");
    printf("  -a              : Dismount all persisted mounts\n");
    printf("  -v              : Verbose output\n");
    printf("  -h              : Display this help message\n");
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


int main(int argc, char *argv[])
{
    char dir[512]; 
    int ch, is_all = 0, is_user = 0, rc = 0, user = 0;
    char *vhost = NULL;
    
    s_argv0 = argv[0];
    DEBUG_MESSAGE("Entering unmount_ns program\n");
    if (getuid())
        return usage("Must be run as root");
    
    if (access(PERSIST_PREFIX, 0))
        //return usage("Persistance directory: " PERSIST_PREFIX " not found");
        return 0;
    
    while ((ch = getopt(argc, argv, "u:as:v?h")) != -1)
    {
        switch (ch)
        {
            case 'u':
            {
                struct passwd *pw;
                is_user = 1;
                if (!(pw = getpwnam(optarg)) &&
                    (strspn(optarg, "0123456789") != strlen(optarg) ||
                     !(pw = getpwuid(atoi(optarg)))))
                    return usage("You must specify a valid user or ID for -u");
                user = pw->pw_uid;
                snprintf(dir, sizeof(dir), PERSIST_PREFIX"/%u", user);
                if (access(dir, 0))
                {
                    fprintf(stderr, "User is not mounted\n");
                    exit(1);
                }
            }
            break;
                
            case 's':
                vhost = strdup(optarg);
                break;
            
            case 'v':
                printf("Verbose output specified\n");
                ns_setverbose_callback(verbose_callback);
                break;
                
            case 'a':
                is_all = 1;
                break;

            case ':':
                return usage("Missing parameter option value");
            
            default:
                return usage(NULL);
        }
    }
    if (!is_user && !is_all)
        return usage("You must specify -u or -a");
    if (vhost && !is_user)
        return usage("You can only specify a vhost for a single user");
    ns_init_debug();
    if (!is_all && is_user)
        ns_setuser(user);
    if (!is_all)
        rc = unpersist_uid(1, user, vhost);
    else
        rc = ns_unpersist_all();
    ns_done();
    if (rc)
        return 1;
    printf("Unmount done\n");
    return 0;
}
