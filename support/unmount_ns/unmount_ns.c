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

#include "lscgid.h"
#include "ns.h"
#include "nsopts.h" // For DEBUG_MESSAGE
#include "nspersist.h" // For PERSIST_PREFIX
#include "lscgid.h"
#include "use_bwrap.h"

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
    int ch;
    int is_all = 0, is_user = 0, rc = 0, user = getuid();
    char *vhost = NULL;
    lscgid_t CGI;
    
    memset(&CGI, 0, sizeof(CGI));
    
    if (getuid())
        return usage("Must be run as root");
    
    if (access(PERSIST_PREFIX, 0))
        return usage("Persistance directory: " PERSIST_PREFIX " not found");
    
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
                    return usage("Specified user directory not found");
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
    if (is_user)
        CGI.m_data.m_uid = user;
    
    putenv(LS_NS "=1");
    if (vhost)
        setenv(LS_NS_VHOST, vhost, 1);
    else
        setenv(LS_NS_VHOST, "NOVHOST", 1);
    if (ns_init(&CGI) != 1)
    {
        printf("Error in initializing dismount environment\n");
        return -1;
    }
    if (!is_all && is_user)
        ns_setuser(user);
    if (!is_all)
        rc = unpersist_uid(user, !vhost);
    else
        rc = ns_unpersist_all();
    ns_done(!is_all);
    if (rc)
        return 1;
    printf("Unmount done\n");
    return 0;
}
