/*
 * Copyright 2020 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#include "lscgid.h"
#include "use_bwrap.h"
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
set_cgi_error_t set_cgi_error = NULL;
int s_bwrap_extra_bytes = BWRAP_ALLOCATE_EXTRA_DEFAULT;
static char *s_bwrap_bin = NULL;

#ifdef DO_BWRAP_DEBUG
int s_bwrap_debug = 1;
void debug_message (const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[0x1000];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    FILE *f;
    int ctr = 0;
    do {
        f = fopen("/tmp/bwrap.dbg","a");
        if (!f) {
            ctr++;
            usleep(0);
        }
    } while ((!f) && (ctr < 1000));
    if (f) {
        struct timeb tb;
        struct tm lt;
        ftime(&tb);
        localtime_r(&tb.time, &lt);
        fprintf(f,"%02d/%02d/%04d %02d:%02d:%02d:%03d %s", lt.tm_mon + 1,
                lt.tm_mday, lt.tm_year + 1900, lt.tm_hour, lt.tm_min, lt.tm_sec,
                tb.millitm, buf);
        fclose(f);
    }
}
#endif


static int count_args(char **args)
{
    int count = 0;
    while (*args)
    {
        ++count;
        ++args;
    }
    return count;
}


static int bwrap_allocate_extra(bwrap_mem_t **mem, int extra)
{
    int total = extra + sizeof(bwrap_mem_t) + BWRAP_ALLOCATE_EXTRA;
    bwrap_mem_t *new = malloc(total);
    DEBUG_MESSAGE("Allocate extra: %d total bytes, extra: %d\n", total, extra);
    if (!new)
    {
        set_cgi_error("Error allocating additional bwrap required memory", NULL);
        return -1;
    }
    memset(new, 0, sizeof(bwrap_mem_t));
    new->m_next = *mem;
    new->m_extra = extra + BWRAP_ALLOCATE_EXTRA;
    new->m_total = total;
    new->m_statics = (*mem)->m_statics;
    *mem = new;
    return 0;
}


static int malloc_ids(char *title, int *nids, char ***ids)
{
#define ALLOC_IDS_MIN 10
    if (!(*nids % ALLOC_IDS_MIN))
    {
        DEBUG_MESSAGE("realloc ids %d bytes orig: %p\n",
                      (*nids + ALLOC_IDS_MIN) * sizeof(char *), *ids);
        (*ids) = realloc(*ids, (*nids + ALLOC_IDS_MIN) * sizeof(char *));
        if (!*ids)
        {
            set_cgi_error("Error allocating memory for", title);
            return -1;
        }
    }
    return 0;
}


static int bwrap_file_param(int passwd, char *begin_param, char *title,
                            int *nids, char ***ids, bwrap_mem_t **mem)
{
    char *in_param = passwd ? (*mem)->m_statics->m_uid_str :
                              (*mem)->m_statics->m_gid_str;
    if (!*in_param)
        snprintf(in_param, sizeof((*mem)->m_statics->m_uid_str), "%u",
                 passwd ? geteuid() : getegid());
    if (malloc_ids(title, nids, ids))
        return -1;

    while (*begin_param)
    {
        if (isspace(*begin_param))
        {
            if (in_param)
            {
                (*ids)[(*nids)] = in_param;
                ++(*nids);
            }
            *begin_param = 0;
            in_param = NULL;
        }
        else
        {
            if (!isdigit(*begin_param))
            {
                DEBUG_MESSAGE("Bad char is: %c\n", *begin_param);
                set_cgi_error("All wildcard modifiers must be numeric", title);
                if (*ids)
                    free(*ids);
                return -1;
            }
            if (!in_param)
                in_param = begin_param;
            if (malloc_ids(title, nids, ids))
                return -1;
        }
        ++begin_param;
    }
    if (in_param)
    {
        (*ids)[(*nids)] = in_param;
        ++(*nids);
    }
    DEBUG_MESSAGE("Exit bwrap_file_param, param: %s, nids: %d, ids: %p\n",
                  begin_param, *nids, *ids);
    int i;
    for (i = 0; i < *nids; ++i)
        DEBUG_MESSAGE("IDs[%d]: %s\n", i, (*ids)[i]);

    return 0;
}


static int bwrap_file(int passwd, char *begin_param, char *wildcard, int *argc,
                      char ***oargv, bwrap_mem_t **mem)
{
    int *fds = passwd ? &(*mem)->m_statics->m_uid_fds[0] :
                        &(*mem)->m_statics->m_gid_fds[0];
    char *title = passwd ? BWRAP_VAR_PASSWD : BWRAP_VAR_GROUP;
    char *filename = passwd ? "/etc/passwd" : "/etc/group";
    int nids = 0;
    char **ids = NULL;
    FILE *fh;
    char line[256];

    if (bwrap_file_param(passwd, wildcard, title, &nids, &ids, mem))
        return -1;
    if (fds[0])
    {
        DEBUG_MESSAGE("Pipe already created for %s: %d %d, statics: %p\n",
                      title, fds[0],
                      passwd ? (*mem)->m_statics->m_uid_fds[0] :
                               (*mem)->m_statics->m_gid_fds[0],
                      (*mem)->m_statics);
        set_cgi_error("Can not specify wildcard parameter more than once",
                      title);
        if (ids)
            free(ids);
        return -1;
    }
    if (pipe(fds))
    {
        set_cgi_error("Error creating pipe for", title);
        if (ids)
            free(ids);
        return -1;
    }

    if (!(fh = fopen(filename, "r")))
    {
        set_cgi_error("Error opening", filename);
        if (ids)
            free(ids);
        return -1;
    }
    while (fgets(line, sizeof(line), fh))
    {
        char *colon;
        colon = strchr(line, ':');
        if (colon)
        {
            ++colon;
            colon = strchr(colon, ':');
            if (colon)
            {
                ++colon;
                char *begin_id = colon;
                colon = strchr(colon, ':');
                if (colon)
                {
                    int id;
                    int found = 0;
                    for (id = 0; id < nids; ++id)
                    {
                        if (colon - begin_id == (int)strlen(ids[id]) &&
                            !strncmp(ids[id], begin_id, colon - begin_id))
                        {
                            found = 1;
                            break;
                        }
                    }
                    if (found && (write(fds[1], line, strlen(line)) == -1))
                    {
                        set_cgi_error("Error writing to pipe for", title);
                        if (ids)
                            free(ids);
                        fclose(fh);
                        return -1;
                    }
                    if (found)
                        DEBUG_MESSAGE("Writing line: %s", line);
                }
            }
        }
    }
    close(fds[1]);
    fds[1] = 0;
    fclose(fh);
    (*oargv)[*argc] = "--file";
    ++(*argc);
    snprintf(begin_param, strlen(title) + 1, "%u", fds[0]);
    (*oargv)[*argc] = begin_param;
    ++(*argc);
    (*oargv)[*argc] = filename;
    ++(*argc);
    if (ids)
        free(ids);
    return 0;
}


static int bwrap_variable(char *begin_param, char *begin_wildcard, int *argc,
                          char ***oargv, bwrap_mem_t **mem)
{
    char *wildcard, *key, **argv = *oargv;
    int key_len, wildcard_len;
    if (!strncmp(begin_wildcard, key = BWRAP_VAR_USER, key_len = strlen(BWRAP_VAR_USER)) ||
        !strncmp(begin_wildcard, key = BWRAP_VAR_HOMEDIR, key_len = strlen(BWRAP_VAR_HOMEDIR)))
    {
        if (!(*mem)->m_statics->m_user_str)
        {
            struct passwd *pwd;
            if (!(pwd = getpwuid(geteuid())))
            {
                set_cgi_error("Error getting passwd entry for effective user", strerror(errno));
                return -1;
            }
            else
            {
                int extra_len;
                if ((key == BWRAP_VAR_USER && !pwd->pw_name) ||
                    (key == BWRAP_VAR_HOMEDIR && !pwd->pw_dir))
                {
                    set_cgi_error("Symbolic $USER or $HOMEDIR wildcard specified"
                                  " with NULL value in /etc/passwd", NULL);
                    return -1;
                }
                char *field = (key == BWRAP_VAR_HOMEDIR) ? pwd->pw_dir : pwd->pw_name;
                extra_len = strlen(field) + 1;
                if (((*mem)->m_extra < extra_len) &&
                    (bwrap_allocate_extra(mem, extra_len)))
                    return -1;
                (*mem)->m_statics->m_user_str = (char *)(*mem) + (*mem)->m_total - (*mem)->m_extra;
                (*mem)->m_extra -= extra_len;
                strncpy((*mem)->m_statics->m_user_str, field, extra_len);
                DEBUG_MESSAGE("Copied in user, statics: %p, uid[0]: %d\n",
                              (*mem)->m_statics, (*mem)->m_statics->m_uid_fds[0]);
            }
        }
        wildcard = (*mem)->m_statics->m_user_str;
    }
    else if (!strncmp(begin_wildcard, key = BWRAP_VAR_UID, key_len = 4))
    {
        if (!(*mem)->m_statics->m_uid_str[0])
            snprintf((*mem)->m_statics->m_uid_str,
                     sizeof((*mem)->m_statics->m_uid_str), "%u", geteuid());
        wildcard = (*mem)->m_statics->m_uid_str;
    }
    else if (!strncmp(begin_wildcard, key = BWRAP_VAR_GID, key_len = 4))
    {
        if (!(*mem)->m_statics->m_gid_str[0])
            snprintf((*mem)->m_statics->m_gid_str,
                     sizeof((*mem)->m_statics->m_gid_str), "%u", getegid());
        wildcard = (*mem)->m_statics->m_gid_str;
    }
    else if ((!strncmp(begin_wildcard, key = BWRAP_VAR_PASSWD, key_len = 7)) ||
             (!strncmp(begin_wildcard, key = BWRAP_VAR_GROUP, key_len = 6)))
        return bwrap_file(key_len == 7, begin_param, &begin_wildcard[key_len],
                          argc, oargv, mem);
    else
    {
        set_cgi_error("Unknown variable name", begin_wildcard);
        return -1;
    }
    wildcard_len = strlen(wildcard);
    if (wildcard_len <= key_len)
    {
        argv[*argc] = begin_param;
        memcpy(begin_wildcard, wildcard, wildcard_len + 1);
        if (wildcard_len < key_len)
            memmove(begin_wildcard + wildcard_len, begin_wildcard + key_len,
                    strlen(begin_wildcard + key_len) + 1);
    }
    else
    {
        int before_wildcard_len = begin_wildcard - begin_param;
        int extra_needed = strlen(begin_param) + wildcard_len - key_len + 1;
        char *arg;
        if (extra_needed > (*mem)->m_extra &&
            bwrap_allocate_extra(mem, extra_needed))
            return -1;
        arg = (char *)(*mem) + (*mem)->m_total - (*mem)->m_extra;
        (*mem)->m_extra -= extra_needed;
        memcpy(arg, begin_param, before_wildcard_len);
        memcpy(arg + before_wildcard_len, wildcard, wildcard_len);
        memcpy(arg + before_wildcard_len + wildcard_len,
               begin_param + before_wildcard_len + key_len,
               strlen(begin_param + before_wildcard_len + key_len) + 1);
        argv[*argc] = arg;
    }
    ++*argc;
    return 0;
}


static int add_argv(char *begin_param, int *argc, char ***oargv,
                    bwrap_mem_t **mem)
{
    char **argv = *oargv;
    char *begin_wildcard;

    if (!*argc)
    {
        int len = strlen(begin_param);
        if (len < 5 || strncmp(begin_param + len - 5, "bwrap", 5))
        {
            set_cgi_error("First parameter to bwrap must NOT be a program "
                          "other than bwrap", begin_param);
            return -1;
        }
    }
    if (strstr(begin_param, "$("))
    {
        set_cgi_error("Programs can not be run from within the bwrap command",
                      NULL);
        return -1;
    }
    else if ((begin_wildcard = strchr(begin_param, '$')))
    {
        if (bwrap_variable(begin_param, begin_wildcard, argc, oargv, mem))
            return -1;
    }
    else if (!strcmp(begin_param, "bash") || !strcmp(begin_param, "/bin/bash") ||
             !strcmp(begin_param, "sh") || !strcmp(begin_param, "/bin/sh") ||
             !strcmp(begin_param, "csh") || !strcmp(begin_param, "/bin/csh"))
    {
        set_cgi_error("It is invalid to run a shell from bwrap", NULL);
        return -1;
    }
    else
    {
        argv[*argc] = begin_param;
        DEBUG_MESSAGE("AddArgv argv[%d]: %s\n", *argc, argv[*argc]);
        ++*argc;
    }
    return 0;
}


char *cgi_getenv(lscgid_t *pCGI, const char *title)
{
    int title_len = strlen(title);
    char **env = pCGI->m_env;
    DEBUG_MESSAGE("getenv(%s): %s\n", title, getenv(title));
    while (*env)
    {
        if (!strncmp(title, *env, title_len) && ((*env)[title_len] == '=') &&
            (*env)[title_len + 1])
        {
            DEBUG_MESSAGE("ENV %s == %s\n", title, &((*env)[title_len + 1]));
            return &((*env)[title_len + 1]);
        }
        else
            DEBUG_MESSAGE("ENV %s != %s\n", title, *env);
        ++env;
    }
    return NULL;
}


int get_bwrap_bin()
{
    struct stat st;
    if (stat(BWRAP_DEFAULT_BIN, &st) == -1)
    {
        if (stat(BWRAP_DEFAULT_BIN2, &st) == 0)
            s_bwrap_bin = BWRAP_DEFAULT_BIN2;
        else
        {
            DEBUG_MESSAGE("Cannot find bwrap command.\n");
            set_cgi_error("lscgid failed to locate bwrap in expected locations", NULL);
            return 500;
        }
    }
    else
        s_bwrap_bin = BWRAP_DEFAULT_BIN;
    DEBUG_MESSAGE("bwrap found at: %s\n", s_bwrap_bin);
    return 0;
}


int build_bwrap_exec(lscgid_t *pCGI, set_cgi_error_t cgi_error, int *argc,
                     char ***oargv, int *done, bwrap_mem_t **mem)
{
    char *bwrap_cmdline = cgi_getenv(pCGI, "LS_BWRAP_CMDLINE");
    int cmdline_len;
    char **argv, *bwrap_args, *ch, *begin_param = NULL, c;
    int in_single_quote = 0;
    int in_double_quote = 0;
    int i, total, argv_max, cgi_args, rc = 0;

    if (!s_bwrap_bin && get_bwrap_bin())
    {
        *done = 1;
        return 500;
    }
    set_cgi_error = cgi_error;
    if (!bwrap_cmdline)
    {
        DEBUG_MESSAGE("LS_BWRAP_CMDLINE not set, use default\n");
        bwrap_cmdline = (s_bwrap_bin == BWRAP_DEFAULT_BIN) ? BWRAP_DEFAULT : BWRAP_DEFAULT2;
    }
    else
        DEBUG_MESSAGE("LS_BWRAP_CMDLINE set, use: %s\n", bwrap_cmdline);

    DEBUG_MESSAGE("Using bwrap_cmdline: %s\n", bwrap_cmdline);
    cmdline_len = strlen(bwrap_cmdline);
    cgi_args = count_args(pCGI->m_argv);
    argv_max = sizeof(char *) * (cmdline_len + cgi_args + 1);
    /* Special cases! */
    if (strstr(bwrap_cmdline, BWRAP_VAR_PASSWD))
        argv_max += 2; // Can only occur once so this is ok
    if (strstr(bwrap_cmdline, BWRAP_VAR_GROUP))
        argv_max += 2; // Can only occur once so this is ok
    total = sizeof(bwrap_mem_t) + argv_max + cmdline_len + 1 +
            sizeof(bwrap_statics_t) + BWRAP_ALLOCATE_EXTRA;
    *mem = malloc(total);
    if (!mem)
    {
        set_cgi_error("lscgid failed to allocate bwrap initial memory", NULL);
        *done = 1;
        return 500;
    }
    memset(*mem, 0, sizeof(bwrap_mem_t));
    (*mem)->m_extra = BWRAP_ALLOCATE_EXTRA;
    (*mem)->m_total = total;
    *oargv = (char **)&(*mem)->m_data[0];
    argv = *oargv;
    bwrap_args = (char *)*oargv + argv_max;
    strncpy(bwrap_args, bwrap_cmdline, cmdline_len + 1);
    (*mem)->m_statics = (bwrap_statics_t *)(bwrap_args + cmdline_len + 1);
    memset((*mem)->m_statics, 0, sizeof(bwrap_statics_t));
    ch = bwrap_args;
    DEBUG_MESSAGE("bwrap_args: %s\n", bwrap_args);
    *argc = 0;
    if (!strncmp(ch, "bwrap", 5))
    {
        argv[*argc] = s_bwrap_bin;
        ++*argc;
        ch += 5;
    }
    else if (*ch != '/')
    {
        argv[*argc] = s_bwrap_bin;
        ++*argc;
    }
    while ((c = *ch))
    {
        int end_param = 0;
        if (c == '\'')
        {
            in_single_quote = !in_single_quote;
            if (!in_single_quote && begin_param)
                end_param = 1;
        }
        else if (c == '"')
        {
            in_double_quote = !in_double_quote;
            if (!in_double_quote && begin_param)
                end_param = 1;
        }
        else if (isspace(c))
        {
            if (begin_param)
            {
                if (!in_single_quote && !in_double_quote)
                    end_param = 1;
            }
        }
        else
        {
            if (!begin_param)
                begin_param = ch;
        }
        if (end_param)
        {
            *ch = 0;
            if (add_argv(begin_param, argc, &argv, mem))
            {
                rc = -1;
                break;
            }
            begin_param = NULL;
        }
        ++ch;
    }
    if (!rc && (in_double_quote || in_single_quote))
    {
        set_cgi_error("Missing end quotes in bwrap command line", NULL);
        rc = -1;
    }
    if (!rc && begin_param)
        rc = add_argv(begin_param, argc, &argv, mem);
    if (rc)
    {
        bwrap_free(mem);
        *oargv = NULL;
        *done = 1;
        return 500;
    }
    for (i = 0; i < cgi_args; ++i)
    {
        DEBUG_MESSAGE("Add CGI args, argv[%d]: %s\n", *argc, pCGI->m_argv[i]);
        argv[*argc] = pCGI->m_argv[i];
        ++*argc;
    }
    argv[*argc] = NULL;
    return 0;
}


int bwrap_exec(lscgid_t *pCGI, int argc, char *argv[], int *done)
{
    *done = 0;
    {
        int i;
        DEBUG_MESSAGE("About to bwrap_exec %d params\n", argc);
        for (i = 0; i < argc; ++i)
        {
            DEBUG_MESSAGE("argv[%d] = %s\n", i, argv[i]);
        }
    }
    if (execve(argv[0], argv, pCGI->m_env) == -1)
    {
        set_cgi_error("lscgid: execve()", pCGI->m_pCGIDir);
        return 500;
    }
    *done = 1;
    return 0;
}


void bwrap_free(bwrap_mem_t **mem)
{
    if (mem && *mem)
    {
        if ((*mem)->m_statics->m_uid_fds[0])
            close((*mem)->m_statics->m_uid_fds[0]);
        if ((*mem)->m_statics->m_uid_fds[1])
            close((*mem)->m_statics->m_uid_fds[1]);
        if ((*mem)->m_statics->m_gid_fds[0])
            close((*mem)->m_statics->m_gid_fds[0]);
        if ((*mem)->m_statics->m_gid_fds[1])
            close((*mem)->m_statics->m_gid_fds[1]);
    }
    while (mem && *mem)
    {
        bwrap_mem_t *next = (*mem)->m_next;
        free(*mem);
        *mem = next;
    }
}


int exec_using_bwrap(lscgid_t *pCGI, set_cgi_error_t cgi_error, int *done)
{
    int rc;
    int argc;
    char **argv;
    bwrap_mem_t *mem = NULL;
    char *lsbwrap = cgi_getenv(pCGI, "LS_BWRAP");
    DEBUG_MESSAGE("LS_BWRAP set to %s\n", lsbwrap);
    if (!lsbwrap)
        return 0;

    rc = build_bwrap_exec(pCGI, cgi_error, &argc, &argv, done, &mem);
    if (rc || *done)
        return rc;
    rc = bwrap_exec(pCGI, argc, argv, done);
    bwrap_free(&mem);
    return rc;
}



#endif


