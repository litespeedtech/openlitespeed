/*
 * Copyright 2020 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
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
#include "nsopts.h"
#include "nspersist.h"
#include "nsutils.h"
#include "lscgid.h"
#include "use_bwrap.h"

#define BLOCK_SIZE  4096

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#define WHITESPACE " \t\n\r\\,"

void ls_stderr(const char * fmt, ...)
#if __GNUC__
        __attribute__((format(printf, 1, 2)))
#endif
;


void nsopts_free_member(SetupOp *op)
{
    if (op->flags & OP_FLAG_ALLOCATED_SOURCE)
    {
        free(op->source);
        op->source = NULL;
        op->flags &= ~OP_FLAG_ALLOCATED_SOURCE;
    }
    if (op->flags & OP_FLAG_ALLOCATED_DEST)
    {
        free(op->dest);
        op->dest = NULL;
        op->flags &= ~OP_FLAG_ALLOCATED_DEST;
    }
    if (op->fd > 0)
    {
        close(op->fd);
        op->fd = -1;
    }
    //op->flags = 0;
    //op->type = 0;
}


void nsopts_free_members(SetupOp *setupOps)
{
    SetupOp *op;
    //if (setupOps == s_SetupOp_default)
    //{
    //    DEBUG_MESSAGE("Attempt to free default in nsopts_free_members!\n");
    //    return;
    //}
    if (setupOps)
    {
        DEBUG_MESSAGE("nsopts_free_members %p\n", setupOps);
        for (op = &setupOps[0]; !(op->flags & OP_FLAG_LAST); ++op)
        {
            nsopts_free_member(op);
        }
    }
}


void nsopts_free(SetupOp **pSetupOps)
{
    if (pSetupOps)
    {
        if (*pSetupOps == s_SetupOp_default)
        {
            DEBUG_MESSAGE("Attempt to free default in nsopts_free!\n");
            *pSetupOps = NULL;
            return;
        }
        if (*pSetupOps)
        {
            nsopts_free_members(*pSetupOps);
            free(*pSetupOps);
            *pSetupOps = NULL;
        }
    }
}


void nsopts_free_conf(char **nsconf, char **nsconf2)
{
    if (nsconf && *nsconf)
    {
        free(*nsconf);
        *nsconf = NULL;
    }
    if (nsconf2 && *nsconf2)
    {
        free(*nsconf2);
        *nsconf2 = NULL;
    }
}


void nsopts_free_all(int *allocated, SetupOp **setupOps, size_t *setupOps_size, 
                     char **nsconf, char **nsconf2)
{
    nsopts_free(setupOps);
    nsopts_free_conf(nsconf, nsconf2);
    *allocated = 0;
    *setupOps_size = 0;
}


int nsopts_rc_from_errno(int err)
{
    int rc = DEFAULT_ERR_RC;
    if (err == ENOENT)
        rc = 404;
    else if (err == EACCES)
        rc = 403;
    return rc;
}


static char *skip_whitespace(char *pos)
{
    int len;
    len = strspn(pos, WHITESPACE);
    return pos + len;
}


static char *skip_to_whitespace(char *pos)
{
    return strpbrk(pos, WHITESPACE);
}


static int hasarg(char *pos)
{
    pos = skip_whitespace(pos);
    return *pos != 0 && *pos != '#';
}


static int getarg(char **pos, char **arg, int *flags)
{
    char c, *begin_param = NULL, *pos_start = *pos;
    int in_single_quote = 0, in_double_quote = 0;
    
    *pos = skip_whitespace(*pos);
    if (!**pos)
    {
        ls_stderr("Namespace expected argument\n");
        return DEFAULT_ERR_RC;
    }
    while ((c = **pos))
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
        else if (isspace(c) || c == ',')
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
                begin_param = *pos;
        }
        if (end_param)
            **pos = 0;
        ++(*pos);
        if (end_param)
            break;
    }
    if (in_double_quote || in_single_quote)
    {
        ls_stderr("Namespace missing end quotes in parameter: %s\n", pos_start);
        return DEFAULT_ERR_RC;
    }
    (*arg) = begin_param;
    DEBUG_MESSAGE("Arg: %s\n", *arg);
    if ((*arg && (strstr(*arg, BWRAP_VAR_GID) || strstr(*arg, BWRAP_VAR_UID) ||
                  strstr(*arg, BWRAP_VAR_USER) || strstr(*arg, BWRAP_VAR_HOMEDIR))))
        *flags |= OP_FLAG_BWRAP_SYMBOL;
    return 0;
}


static int set_op(SetupOp *op, SetupOpType type, char *source, char *dest,
                  int flags)
{
    op->type = type;
    if (source && (flags & OP_FLAG_ALLOCATED_SOURCE))
    {
        op->source = strdup(source);
        if (!op->source)
        {
            int err = errno;
            ls_stderr("Namespace error allocating op source: %s\n", strerror(err));
            return nsopts_rc_from_errno(err);
        }
    }
    if (dest && (flags & OP_FLAG_ALLOCATED_DEST))
    {
        op->dest = strdup(dest);
        if (!op->dest)
        {
            int err = errno;
            ls_stderr("Namespace error allocating op dest: %s\n", strerror(err));
            return nsopts_rc_from_errno(err);
        }
    }
    if (type == SETUP_NO_UNSHARE_USER)
        nspersist_disable_user_namespace();
    op->flags = flags;
    DEBUG_MESSAGE("Added op, type: %d, source: %s, dest: %s, flags: %d\n",
                  op->type, op->source, op->dest, op->flags);
    return 0;
}


static int read_opts(const char *nsopts, char **opts_raw)
{
    char *data = NULL;
    int rc, count = 0;
    DEBUG_MESSAGE("read_opts: %s\n", nsopts);
    int fd = open(nsopts, O_RDONLY);
    if (fd == -1)
    {
        int err = errno;
        ls_stderr("Namespace error opening file %s: %s\n", nsopts, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    *opts_raw = NULL;
    do
    {
        DEBUG_MESSAGE("read_opts: realloc: %p, %d bytes\n", data, BLOCK_SIZE * (count + 1));
        data = realloc(data, BLOCK_SIZE * (count + 1));
        if (!data)
        {
            int err = errno;
            ls_stderr("Namespace error reallocating file memory\n");
            close(fd);
            return nsopts_rc_from_errno(err);
        }
        rc = read(fd, data + BLOCK_SIZE * count, BLOCK_SIZE);
        if (rc < 0)
        {
            int err = errno;
            ls_stderr("Namespace error reading file %s: %s\n", nsopts, 
                      strerror(err));
            close(fd);
            free(data);
            return nsopts_rc_from_errno(err);
        }
        if (rc == BLOCK_SIZE)
            ++count;
    } while (rc == BLOCK_SIZE);
    close(fd);
    DEBUG_MESSAGE("read_opts: count: %d, rc: %d\n", count, rc);
    if (count == 0 && rc == 0)
    {
        ls_stderr("Namespace option file: %s is empty and this is invalid\n",
                  nsopts);
        free(data);
        return DEFAULT_ERR_RC;
    }
    *(data + BLOCK_SIZE * count + rc) = 0;
    *opts_raw = data;
    DEBUG_MESSAGE("Final options: %s\n", *opts_raw);
    return 0;
}


static char *get_line(char *opts_pos, char **line)
{
    *line = NULL;
    while (*opts_pos)
    {
        char *nl;
        opts_pos = skip_whitespace(opts_pos);
        if (!*opts_pos)
            break;
        if (*opts_pos == '\n')
        {
            ++opts_pos;
            continue;
        }
        nl = strchr(opts_pos, '\n');
        if (*opts_pos == '#')
        {
            if (nl)
            {
                opts_pos = nl + 1;
                continue;
            }
            opts_pos += strlen(opts_pos);
            break;
        }
        *line = opts_pos;
        if (nl)
        {
            *nl = 0;
            opts_pos = (nl + 1);
        }
        else
            opts_pos += strlen(opts_pos);
        break;
    }
    if (*line)
    {
        DEBUG_MESSAGE("get_line%s: %s\n", *opts_pos ? "" : " (LAST LINE)", *line);
    }
    else
    {
        DEBUG_MESSAGE("get_line empty line, %s last line\n", 
                      *opts_pos ? "NOT" : "IS");
    }
    return opts_pos;
}


static int reallocate_opts(int count, int *allocated, SetupOp **setupOps,
                           size_t *setupOps_size)
{
    SetupOp *setupOps_l = *setupOps;
    *setupOps_size = sizeof(SetupOp) * (count + 2);
    DEBUG_MESSAGE("%d options, total size: %d bytes %p-%p, each: %d\n", count, 
                  (int)*setupOps_size, setupOps_l, 
                  (char *)setupOps_l + *setupOps_size, (int)sizeof(SetupOp));
    setupOps_l = realloc(setupOps_l, *setupOps_size);
    *setupOps = setupOps_l;
    if (!setupOps_l)
    {
        int err = errno;
        ls_stderr("Namespace unable to allocate ops: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    memset(&setupOps_l[count], 0, sizeof(SetupOp) * 2); // Just the last two;
    setupOps_l[count + 1].flags = OP_FLAG_LAST;
    *allocated = 1;
    return 0;
}


static int find_op(SetupOpType type, char *source, char *dest, int flags,
                   int count, int allocated, SetupOp *setupOps)
{
    SetupOp *op;
    int i = 0;
    if (!setupOps)
        return -1;
    DEBUG_MESSAGE("find_op\n");
    if (flags & OP_FLAG_RAW_OP)
    {
        DEBUG_MESSAGE("Raw op, add\n");
        return -1;
    }
    for (op = &setupOps[0]; !(op->flags & OP_FLAG_LAST); ++op)
    {
        if (op->type == type && 
            (type == SETUP_NOOP || type == SETUP_SET_HOSTNAME || 
             type == SETUP_MAKE_GROUP || type == SETUP_MAKE_PASSWD))
        {
            DEBUG_MESSAGE("FOUND OP type: %d\n", type);
            return i;
        }
        if (!strcmp(dest, op->dest))
        {
            DEBUG_MESSAGE("FOUND OP type: %d->%d, %s (%s->%s)\n", op->type, type, 
                          dest, source, op->source);
            return i;
        }
        ++i;
    }
    DEBUG_MESSAGE("find_op not already defined\n");
    return -1;
}


static int add_op(SetupOpType type, char *source, char *dest, int flags,
                  int *count, int *found_op, int *allocated, SetupOp **setupOps, 
                  size_t *setupOps_size)
{
    int op, rc = 0;
    DEBUG_MESSAGE("add_op #%d type: %d, source: %s, dest: %s, flags: 0x%x\n",
                  *count, type, source, dest, flags);
    op = find_op(type, source, dest, flags, *count, *allocated, *setupOps);
    if (op >= 0)
    {
        SetupOp *setupOps_l = *setupOps;
        nsopts_free_member(&setupOps_l[op]);
    }
    else 
    {
        op = *count;
        rc = reallocate_opts(op, allocated, setupOps, setupOps_size);
        if (!rc)
            ++(*count);
    }
    if (!rc)
    {
        SetupOp *setupOps_l = *setupOps;
        rc = set_op(&setupOps_l[op], type, source, dest, flags);
    }
    if (!rc)
        *found_op = 1;
    return rc;
}


static int process_command_symbol(char *line, int *count, int *found_op, 
                                  int *allocated, SetupOp **setupOps, 
                                  size_t *setupOps_size)
{
    int passwd = 0, len, values_len = 0, rc = 0;
    char *values = NULL;
    if (!strncmp(line, BWRAP_VAR_PASSWD, len = strlen(BWRAP_VAR_PASSWD)))
        passwd = 1;
    else if (strncmp(line, BWRAP_VAR_GROUP, len = strlen(BWRAP_VAR_GROUP)))
    {
        ls_stderr("Namespace invalid command symbol: %s\n", line);
        return DEFAULT_ERR_RC;
    }
    line += len;
    line = skip_whitespace(line);
    values = strdup(passwd ? BWRAP_VAR_PASSWD : BWRAP_VAR_GROUP);
    if (!values)
    {
        ls_stderr("Namespace can't allocate initial value for %s\n", 
                  passwd ? BWRAP_VAR_PASSWD : BWRAP_VAR_GROUP);
        return DEFAULT_ERR_RC;
    }
    values_len = strlen(values);
    while (*line)
    {
        char *ws, *name, id_str[STRNUM_SIZE + 2];
        int id = -1, id_len, last_values_len = values_len;
        ws = skip_to_whitespace(line);
        if (ws)
        {
            *ws = 0;
            name = line;
            line = ws + 1;
        }
        else
        {
            name = line;
            line += strlen(line);
        }
        line = skip_whitespace(line);
        if (passwd)
        {
            struct passwd *pwd = getpwnam(name);
            if (pwd)
                id = pwd->pw_uid;
        }
        else
        {
            struct group *grp = getgrnam(name);
            if (grp)
                id = grp->gr_gid;
        }
        if (id == -1)
        {
            DEBUG_MESSAGE("Namespace %s name is not found: %s\n",
                          passwd ? BWRAP_VAR_PASSWD : BWRAP_VAR_GROUP, name);
            ls_stderr("Namespace %s name is not found: %s\n",
                      passwd ? BWRAP_VAR_PASSWD : BWRAP_VAR_GROUP, name);
            rc = DEFAULT_ERR_RC;
            break;
        }
        snprintf(id_str, sizeof(id_str), " %u", id);
        id_len = strlen(id_str);
        values_len += id_len;
        values = realloc(values, values_len + 1);
        if (!values)
        {
            DEBUG_MESSAGE("Namespace can't allocate values for %s\n", 
                          passwd ? BWRAP_VAR_PASSWD : BWRAP_VAR_GROUP);
            ls_stderr("Namespace can't allocate values for %s\n", 
                      passwd ? BWRAP_VAR_PASSWD : BWRAP_VAR_GROUP);
            rc = DEFAULT_ERR_RC;
            break;
        }
        memcpy(values + last_values_len, id_str, id_len + 1);
    }
    DEBUG_MESSAGE("Command symbol: %s, values: %s, rc: %d\n", 
                  passwd ? BWRAP_VAR_PASSWD : BWRAP_VAR_GROUP, values, rc);
    if (!rc)
        rc = add_op(passwd ? SETUP_MAKE_PASSWD : SETUP_MAKE_GROUP, NULL,
                    values, OP_FLAG_BWRAP_SYMBOL | OP_FLAG_NO_CREATE_DEST |
                    (values_len ? OP_FLAG_ALLOCATED_DEST : 0), count, 
                    found_op, allocated, setupOps, setupOps_size);
    if (values)
        free(values);
    return rc;
}

    
static int process_negate_path(char *line, int *count, int *found_op, 
                               int *allocated, SetupOp **setupOps, 
                               size_t *setupOps_size)
{
    char *arg, *arg_beg = line + 1;
    int flags = 0;
    int rc = getarg(&arg_beg, &arg, &flags);
    if (!rc)
        rc = add_op(SETUP_MOUNT_TMPFS, NULL, arg, flags | OP_FLAG_ALLOCATED_DEST, 
                    count, found_op, allocated, setupOps, setupOps_size);
    return rc;
}


static int is_option(char *option, SetupOpType *type, int *flags)
{
    if (!strncmp(option, "unshare", 7) || !strncmp(option, "cap-", 4) || 
        !strcmp(option, "share-net") || !strcmp(option, "die-with-parent"))
    {
        DEBUG_MESSAGE("'unshare', 'cap', 'share-net' and 'dir-with-parent'"
                      " options are all implied (%s)\n", option);
        *type = SETUP_NOOP;
    }
    else if (!strcmp(option, "uid") || !strcmp(option, "gid") ||
             !strcmp(option, "chdir") || !strcmp(option, "as-pid-1") ||
             !strcmp(option, "setenv") || !strcmp(option, "unsetenv"))
    {
        ls_stderr("Namespace option: %s is invalid in this context, as LiteSpeed "
                  "defines the values\n", option);
        return DEFAULT_ERR_RC;
    }
    else if (!strcmp(option, "bind"))
    {
        *type = SETUP_BIND_MOUNT;
        *flags |= (OP_FLAG_ALLOCATED_SOURCE | OP_FLAG_ALLOCATED_DEST);
    }
    else if (!strcmp(option, "bind-try"))
    {
        *type = SETUP_BIND_MOUNT;
        *flags |= (OP_FLAG_ALLOCATED_SOURCE | OP_FLAG_ALLOCATED_DEST | OP_FLAG_ALLOW_NOTEXIST);
    }
    else if (!strcmp(option, "dev"))
    {
        *type = SETUP_MOUNT_DEV;
        *flags |= OP_FLAG_ALLOCATED_DEST;
    }
    else if (!strcmp(option, "dev-bind"))
    {
        *type = SETUP_DEV_BIND_MOUNT;
        *flags |= (OP_FLAG_ALLOCATED_SOURCE | OP_FLAG_ALLOCATED_DEST);
    }
    else if (!strcmp(option, "dev-bind-try"))
    {
        *type = SETUP_DEV_BIND_MOUNT;
        *flags |= (OP_FLAG_ALLOCATED_SOURCE | OP_FLAG_ALLOCATED_DEST | OP_FLAG_ALLOW_NOTEXIST);
    }
    else if (!strcmp(option, "dir"))
    {
        *type = SETUP_MAKE_DIR;
        *flags |= OP_FLAG_ALLOCATED_DEST;
    }
    else if (!strcmp(option, "hostname"))
    {
        *type = SETUP_SET_HOSTNAME;
        *flags |= OP_FLAG_ALLOCATED_SOURCE;
    }
    else if (!strcmp(option, "mqueue"))
    {
        *type = SETUP_MOUNT_MQUEUE;
        *flags |= OP_FLAG_ALLOCATED_DEST;
    }
    else if (!strcmp(option, "proc"))
    {
        *type = SETUP_MOUNT_PROC;
        *flags |= OP_FLAG_ALLOCATED_DEST;
    }
    else if (!strcmp(option, "remount-ro"))
    {
        *type = SETUP_REMOUNT_RO_NO_RECURSIVE;
        *flags = OP_FLAG_ALLOCATED_DEST;
    }
    else if (!strcmp(option, "ro-bind"))
    {
        *type = SETUP_RO_BIND_MOUNT;
        *flags |= (OP_FLAG_ALLOCATED_SOURCE | OP_FLAG_ALLOCATED_DEST);
    }
    else if (!strcmp(option, "ro-bind-try"))
    {
        *type = SETUP_RO_BIND_MOUNT;
        *flags |= (OP_FLAG_ALLOCATED_SOURCE | OP_FLAG_ALLOCATED_DEST | OP_FLAG_ALLOW_NOTEXIST);
    }
    else if (!strcmp(option, "symlink"))
    {
        *type = SETUP_MAKE_SYMLINK;
        *flags |= (OP_FLAG_ALLOCATED_SOURCE | OP_FLAG_ALLOCATED_DEST);
    }
    else if (!strcmp(option, "tmpfs"))
    {
        *type = SETUP_MOUNT_TMPFS;
        *flags = OP_FLAG_ALLOCATED_DEST;
    }
    else if (!strcmp(option, "tmp"))
    {
        *type = SETUP_BIND_TMP;
        *flags |= (OP_FLAG_ALLOCATED_DEST | OP_FLAG_BWRAP_SYMBOL);
    }
    else if (!strcmp(option, "copy"))
    {
        *type = SETUP_COPY;
        *flags |= (OP_FLAG_ALLOCATED_SOURCE | OP_FLAG_ALLOCATED_DEST);
    }
    else if (!strcmp(option, "copy-try"))
    {
        *type = SETUP_COPY;
        *flags |= (OP_FLAG_ALLOCATED_SOURCE | OP_FLAG_ALLOCATED_DEST | OP_FLAG_ALLOW_NOTEXIST);
    }
    else if (!strcmp(option, "no-unshare-user"))
    {
        DEBUG_MESSAGE("Add no-unshare-user\n");
        *type = SETUP_NO_UNSHARE_USER;
        *flags = OP_FLAG_RAW_OP;
    }
    else
    {
        DEBUG_MESSAGE("%s is not a known option\n", option);
        *type = SETUP_NOOP;
    }
    return 0;
}


static int process_line(char *line, int *count, int *found_op, int *allocated, 
                        SetupOp **setupOps, size_t *setupOps_size)
{
    int rc, flags = 0;
    char *source = NULL, *dest = NULL, *pos = line, *arg, *arg2 = NULL, *option = NULL;
    SetupOpType type = SETUP_NOOP;
    
    DEBUG_MESSAGE("process_line: %s\n", line);
    if (line[0] == '$' && 
        strncmp(line, BWRAP_VAR_HOMEDIR, strlen(BWRAP_VAR_HOMEDIR)))
        return process_command_symbol(line, count, found_op, allocated, setupOps, 
                                      setupOps_size);
    if (line[0] == '!')
        return process_negate_path(line, count, found_op, allocated, setupOps, 
                                   setupOps_size);
    if (!hasarg(line))
        return 0; // Ignore line
    rc = getarg(&pos, &arg, &flags);
    if (rc)
        return rc;
    if (hasarg(pos))
    {
        rc = getarg(&pos, &arg2, &flags);
        if (rc)
            return rc;
        if (hasarg(pos))
        {
            rc = getarg(&pos, &option, &flags);
            if (rc)
                return rc;
            rc = is_option(option, &type, &flags);
            if (rc)
                return rc;
            if (type != SETUP_NOOP)
            {
                source = arg;
                dest = arg2;
            }
            else
            {
                ls_stderr("Namespace invalid option: %s\n", option);
                return DEFAULT_ERR_RC;
            }
        }
        else
        {
            rc = is_option(arg2, &type, &flags);
            if (rc)
                return rc;
            if (type == SETUP_NOOP)
            {
                source = arg;
                dest = arg2;
                type = SETUP_RO_BIND_MOUNT;
                flags |= (OP_FLAG_ALLOCATED_SOURCE | OP_FLAG_ALLOCATED_DEST | OP_FLAG_ALLOW_NOTEXIST);
            }
            else
            {
                if (flags & OP_FLAG_ALLOCATED_SOURCE)
                    source = arg;
                if (flags & OP_FLAG_ALLOCATED_DEST)
                    dest = arg;
            }
                
        }
    }
    else if (!is_option(arg, &type, &flags) && (flags & OP_FLAG_RAW_OP))
    {
        DEBUG_MESSAGE("Raw option, type: %d\n", type);
    }
    else 
    {
        type = SETUP_RO_BIND_MOUNT;
        source = arg;
        dest = arg;
        flags |= (OP_FLAG_ALLOCATED_SOURCE | OP_FLAG_ALLOCATED_DEST | OP_FLAG_ALLOW_NOTEXIST);
    }
    if (type == SETUP_NOOP)
    {
        DEBUG_MESSAGE("Unknown option, ignore\n");
        return 0;
    }
    rc = add_op(type, source, dest, flags, count, found_op, allocated, setupOps, 
                setupOps_size);
    return rc;
}


static int process_opts_raw(char *opts_raw, int *count, int *found_op, 
                            int *allocated, SetupOp **setupOps, 
                            size_t *setupOps_size)
{
    int rc;
    char *opts_pos = opts_raw;
    
    DEBUG_MESSAGE("process_opts_raw\n");
    while (*opts_pos)
    {
        char *line = NULL;
        opts_pos = get_line(opts_pos, &line);
        if (!line)
            break;
        rc = process_line(line, count, found_op, allocated, setupOps, 
                          setupOps_size);
        if (rc)
            return rc;
    }
    DEBUG_MESSAGE("process_opts_raw done\n");
    return 0;    
}


static int nsopts_parse(const char *nsopts, int *count, int *found_op, 
                        int *allocated, SetupOp **setupOps, 
                        size_t *setupOps_size)
{
    int rc = 0;
    char *opts_raw = NULL;
    rc = read_opts(nsopts, &opts_raw);
    if (rc)
        return rc;
    rc = process_opts_raw(opts_raw, count, found_op, allocated, setupOps, 
                          setupOps_size);
    free(opts_raw);
    return rc;
}


int nsopts_init(lscgid_t *pCGI, char **nsconf, char **nsconf2)
{
    int i;
    char *nsconfl = NULL, *nsconf2l = NULL;
    DEBUG_MESSAGE("nsopts_init()\n");
    for (i = 0; i < 2; ++i)
    {
        const char *envname = i ? LS_NS_CONF2 : LS_NS_CONF;
        char **res = i ? &nsconf2l : &nsconfl;
        *res = ns_getenv(pCGI, envname);
        //unsetenv(envname);
        DEBUG_MESSAGE("%s set to %s\n", envname, *res);
    }
    if (((!nsconfl && !*nsconf) || (nsconfl && *nsconf && !strcmp(nsconfl, *nsconf))) &&
        ((!nsconf2l && !*nsconf2) || (nsconf2l && *nsconf2 && !strcmp(nsconf2l, *nsconf2))))
    {
        DEBUG_MESSAGE("Allocated opts match requested opts, use them\n");
        return 0;
    }
    nsopts_free_conf(nsconf, nsconf2);
    for (i = 0; i < 2; ++i)
    {
        char *val = i ? nsconf2l : nsconfl;
        char **res = i ? nsconf2 : nsconf;
        if (val)
        {
            *res = strdup(val);
            if (!*res)
            {
                ls_stderr("Namespace unable to allocate option name memory: %s\n", val);
                return DEFAULT_ERR_RC;
            }
        }
    }
    return 0;
}


int nsopts_get(lscgid_t *pCGI, int *allocated, SetupOp **setupOps, 
               size_t *setupOps_size, char **nsconf, char **nsconf2)
{
    int i, rc = 0, count = 0;
    DEBUG_MESSAGE("nsopts_get(%sallocated)\n", *allocated ? "" : "NOT ");
    nsopts_free_all(allocated, setupOps, setupOps_size, NULL, NULL);
    for (i = 0; i < 2; ++i)
    {
        int found_op = 0;
        const char *env = i ? *nsconf2 : *nsconf;
        if (!env)
            continue;
        rc = nsopts_parse(env, &count, &found_op, allocated, setupOps, 
                          setupOps_size);
        if (rc)
        {
            nsopts_free_all(allocated, setupOps, setupOps_size, nsconf, nsconf2);
            return rc;
        }
        if (!found_op)
        {
            ls_stderr("Namespace expects a specified file %s to have at least "
                      "one valid line\n", env);
            nsopts_free_all(allocated, setupOps, setupOps_size, nsconf, nsconf2);
            return DEFAULT_ERR_RC;
        }
        DEBUG_MESSAGE("nsopts_get rc: %d\n", rc);
        if (rc)
            return rc;
    }
    return rc;
}
#endif
