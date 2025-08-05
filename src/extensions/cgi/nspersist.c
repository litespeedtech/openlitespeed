/*
 * Copyright 2020 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/fsuid.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

#include "ns.h"
#include "nsipc.h"
#include "nsopts.h"
#include "nsutils.h"
#include "lscgid.h"
#include "use_bwrap.h"
#include "nsnosandbox.h"
#include "nspersist.h"

#define PERSIST_DIR_SIZE        256
#define PERSIST_FILE_SIZE       512

#define PERSIST_VH_FILE         "VHLock"

typedef struct mount_tab_line_s
{
    const char *mountpoint;
    const char *options;
    int         covered;
    int         id;
    int         parent_id;
    struct mount_tab_line_s *first_child;
    struct mount_tab_line_s *next_sibling;
} mount_tab_line_t;

int   s_persist_created_dir = 0;
char  s_persist_num[12] = { 0 };
uid_t s_persist_uid = -1;
int   s_persist_fd = -1;
int   s_persist_vh_fd = -1;
int   s_persist_vh_locked_write = 0;
extern int s_not_lscgid;
static int is_persisted_fn(int uid, int must_persist, int *persisted);
static int open_persist_vh_file(int report, uid_t uid, int lock_type);
static char *persist_vh_file_name(uid_t uid, char *filename, int filename_size);

static int lock_file(uid_t uid, int fd, char *desc, int report, int lock_type, int *locked_write)
{
    DEBUG_MESSAGE("Lock the %s file %d %s\n", desc, fd, 
                  (lock_type == NSPERSIST_LOCK_TRY_WRITE || lock_type == NSPERSIST_LOCK_WRITE_NB) ? 
                  "TRY_WRITE" : 
                  (lock_type ? "WRITE" : "READ"));
    int op = (lock_type == NSPERSIST_LOCK_TRY_WRITE || lock_type == NSPERSIST_LOCK_WRITE_NB) ? (LOCK_EX | LOCK_NB) : 
             (lock_type == NSPERSIST_LOCK_WRITE) ? LOCK_EX : LOCK_SH;
    if (flock(fd, op))
    {
        int err = errno;
        if (errno == EWOULDBLOCK)
        {
            if (lock_type == NSPERSIST_LOCK_WRITE_NB)
            {
                DEBUG_MESSAGE("Unable to get write lock %s for uid %d\n", desc, uid);
                //if (report)
                //    ls_stderr("Unable to get write lock %s for uid %d\n", desc, uid);
                return nsopts_rc_from_errno(err);
            }
            DEBUG_MESSAGE("Converting an attempted write to a read lock!\n");
            return lock_file(uid, fd, desc, report, NSPERSIST_LOCK_READ, locked_write);
        }
        DEBUG_MESSAGE("Unexpected error locking %s file: %s\n", desc, strerror(err));
        if (report)
            ls_stderr("Namespace error locking %s file: %s\n", desc, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    if (locked_write)
        *locked_write = lock_type;
    DEBUG_MESSAGE("Locked %s %s\n", desc, lock_type ? "WRITE" : "READ");
    return 0;
}


static char *persist_persist_file_dir(char *persist_dir, int persist_dir_len)
{
    char persist_num[sizeof(s_persist_num)];
    strncpy(persist_num, s_persist_num, sizeof(persist_num));
    s_persist_num[0] = 0;
    persist_namespace_dir_vh(s_persist_uid, persist_dir, persist_dir_len);
    strncpy(s_persist_num, persist_num, sizeof(s_persist_num));
    return persist_dir;
}


static char *persist_persist_file_name(char *persist_name, int persist_name_len)
{
    char dir[PERSIST_DIR_SIZE];
    persist_persist_file_dir(dir, sizeof(dir));
    snprintf(persist_name, persist_name_len, "%s/" PERSIST_FILE, dir);
    DEBUG_MESSAGE("Use persist_file_name: %s\n", persist_name);
    return persist_name;
}


static int new_persist(uid_t uid, int fd, int relock_read)
{
    if (relock_read &&
        lock_file(uid, fd, "Persist File", 1, NSPERSIST_LOCK_WRITE, NULL))
        return -1;
    int persist_num = 0;
    char persist_dir[PERSIST_FILE_SIZE], dir[PERSIST_DIR_SIZE];
    persist_persist_file_dir(dir, sizeof(dir));
    do
        snprintf(persist_dir, sizeof(persist_dir), "%s/%d", dir, ++persist_num);
    while (!access(persist_dir, 0));
    snprintf(s_persist_num, sizeof(s_persist_num), "%d", persist_num);
    DEBUG_MESSAGE("new_persist: uid: %d: %s\n", uid, s_persist_num);
    lseek(fd, 0, SEEK_SET);
    if (write(fd, s_persist_num, strlen(s_persist_num)) == -1)
    {
        int err = errno;
        ls_stderr("Namespace error writing persist file %s: %s\n", 
                  s_persist_num, strerror(err));
        DEBUG_MESSAGE("Namespace error writing persist file %s: %s\n", 
                      s_persist_num, strerror(err));
        return -1;
    }   
    if (!mkdir(persist_dir, S_IRWXU))
        s_persist_created_dir = 1;
    if (relock_read && 
        lock_file(uid, fd, "Relock read Persist File", 1, NSPERSIST_LOCK_READ, 
                  NULL))
        return -1;        
    DEBUG_MESSAGE("Created %s: %s\n", persist_dir, s_persist_created_dir ? "YES" : "NO");
    return 0;
}


static int mkdirs(char *dir)
{
    mkdir(PERSIST_PREFIX, S_IRWXU);
    int pos = strlen(PERSIST_PREFIX + 1);
    char *slash = dir + pos, *new_slash, do_mkdir[PERSIST_DIR_SIZE + 1];
    do
    {
        new_slash = strchr(slash, '/');
        int len;
        if (new_slash)
            len = new_slash - dir;
        else
            len = strlen(dir);
        memcpy(do_mkdir, dir, len);
        do_mkdir[len] = 0;
        if (!mkdir(do_mkdir, S_IRWXU))
            s_persist_created_dir = 1;
        DEBUG_MESSAGE("Namespace directory create of %s, created: %d, errno: %s\n",
                      do_mkdir, s_persist_created_dir, strerror(errno));
        if (new_slash)
            slash = new_slash + 1;
    } while (new_slash);
    return 0;
}


static void close_unlock_persist()
{
    if (s_persist_fd != -1)
    {
        DEBUG_MESSAGE("close_unlock_persist: %d\n", s_persist_fd);
        flock(s_persist_fd, LOCK_UN);   
        close(s_persist_fd);
        s_persist_fd = -1;
    }
}


static int get_persist_num(int unmount, uid_t uid, int lock_write, int close_unlock)
{
    char persist_name[PERSIST_FILE_SIZE];
    persist_persist_file_name(persist_name, sizeof(persist_name));
    s_persist_fd = open(persist_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (s_persist_fd == -1 && errno == ENOENT) 
    {
        if (unmount)
        {
            DEBUG_MESSAGE("No persist file, unmount, use none!\n");
            s_persist_num[0] = 0;
            return 0;
        }
        char dir[PERSIST_DIR_SIZE];
        persist_namespace_dir_vh(uid, dir, sizeof(dir));
        mkdirs(dir);
        s_persist_fd = open(persist_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    }
    if (s_persist_fd == -1) 
    {
        int err = errno;
        ls_stderr("Namespace error opening persist file %s: %s\n", 
                  persist_name, strerror(err));
        DEBUG_MESSAGE("Namespace error opening persist file %s: %s\n", 
                      persist_name, strerror(err));
        return -1;
    }
    if (lock_file(uid, s_persist_fd, "Persist File", 1, 
                  lock_write ? NSPERSIST_LOCK_WRITE : NSPERSIST_LOCK_READ, NULL))
    {
        close(s_persist_fd);
        s_persist_fd = -1;
        return -1;
    }
    ssize_t bytes = read(s_persist_fd, s_persist_num, sizeof(s_persist_num) - 1);
    if (bytes == -1) 
    {
        int err = errno;
        ls_stderr("Namespace error reading persist file %s: %s\n", 
                  persist_name, strerror(err));
        DEBUG_MESSAGE("Namespace error reading persist file %s: %s\n", 
                      persist_name, strerror(err));
        close_unlock_persist();
        return -1;
    }
    if (bytes > 0) 
    {
        s_persist_num[bytes] = 0;
        char *nl = strchr(s_persist_num, '\n');
        if (nl)
        {
            *nl = 0;
            bytes = nl - s_persist_num;
        }
    }
    if (bytes == 0 && !lock_write) 
    {
        DEBUG_MESSAGE("No persisted file/dir, we need to create one\n");
        if (new_persist(uid, s_persist_fd, 0))
        {
            close_unlock_persist();
            return -1;
        }
    }
    if (close_unlock)
        close_unlock_persist();
    DEBUG_MESSAGE("Use persist_num: %s\n", s_persist_num);
    return 0;
}


/* Note we may leave this function with persist file open locked. */
int is_persisted(lscgid_t *pCGI, int must_persist, int *persisted)
{
    s_persist_uid = pCGI->m_data.m_uid;
    if (get_persist_num(0, pCGI->m_data.m_uid, 0, 1))
    {
        if (must_persist)
        {
            DEBUG_MESSAGE("Persist number issue\n");
            ls_stderr("Persist number issue\n");
        }
        return -1;
    }
    return is_persisted_fn(pCGI->m_data.m_uid, must_persist, persisted);
}

#ifndef __NR_setns
#define __NR_setns 308
int setns(int fd, int nstype)
{
    DEBUG_MESSAGE("setns, making syscall\n");
    return syscall(__NR_setns, fd, nstype);
}
#endif

int nspersist_setvhost(char *vhenv)
{
    if (!vhenv)
    {
        vhenv = "";
        DEBUG_MESSAGE("Namespace requires a virtual host - use %s\n", vhenv);
    }
    if (s_persisted_VH)
        if (strcmp(s_persisted_VH, vhenv))
        {
            free(s_persisted_VH);
            s_persisted_VH = NULL;
        }
    if (!s_persisted_VH)
    {
        s_persisted_VH = (char *)strdup(vhenv);
        if (!s_persisted_VH)
        {
            int err = errno;
            ls_stderr("Namespace VH insufficient memory for name: %s\n", strerror(err));
            return nsopts_rc_from_errno(err);
        }
    }
    DEBUG_MESSAGE("VH specified as %s\n", s_persisted_VH);
    return 0;
}


int nspersist_init(lscgid_t *pCGI)
{
    char *vhenv = ns_getenv(pCGI, LS_NS_VHOST);
    int rc = nspersist_setvhost(vhenv);
    return rc;
}


/* Return value must be freed!  */
#define PROC_BLOCK_SIZE 16384
static char *read_big_proc_file(const char *filename)
{
    ssize_t len, pos = 0, buf_size = 0;
    char *block = NULL;
    if (s_proc_fd == -1)
    {
        s_proc_fd = open("/proc", O_PATH);
        if (s_proc_fd == -1)
        {
            int err = errno;
            DEBUG_MESSAGE("Error opening /proc: %s\n", strerror(err));
            ls_stderr("Error opening /proc: %s\n", strerror(err));
            return NULL;
        }
    }
    fcntl(s_proc_fd, F_SETFD, FD_CLOEXEC);
    int fd = openat(s_proc_fd, filename, O_RDONLY);
    if (fd == -1)
    {
        int err = errno;
        DEBUG_MESSAGE("Namespace error opening /proc/%s: %s\n", filename, strerror(err));
        ls_stderr("Namespace error opening /proc/%s: %s\n", filename, strerror(err));
        return NULL;
    }
    do
    {
        if (buf_size - pos < PROC_BLOCK_SIZE)
        {
            buf_size += PROC_BLOCK_SIZE;
            DEBUG_MESSAGE("doing realloc, block: %p, size: %d\n", block, (int)(buf_size + 2));
            block = realloc(block, buf_size + 2);
            DEBUG_MESSAGE("did realloc, block: %p, size: %d\n", block, (int)(buf_size + 2));
        }
        if (!block)
        {
            ls_stderr("Namespace error reallocating /proc/%s at %ld: %s\n", filename,
                      pos, strerror(errno));
            close(fd);
            return NULL;
        }
        len = read(fd, &block[pos], PROC_BLOCK_SIZE);
        if (len < 0)
        {
            int err = errno;
            DEBUG_MESSAGE("Namespace error reading /proc/%s: %s\n", filename, 
                          strerror(err));
            ls_stderr("Namespace error reading /proc/%s: %s\n", filename, 
                      strerror(err));
            free(block);
            close(fd);
            return NULL;
        }
        pos += len;
        block[pos] = 0;
    } while (len);
    DEBUG_MESSAGE("close /proc/%s fd: %d\n", filename, fd);
    close(fd);
    return block;
}


static unsigned int count_lines(const char *data)
{
    unsigned int count = 0;
    const char *p = data;

    while (*p != 0)
    {
        if (*p == '\n')
            count++;
        p++;
    }

    /* If missing final newline, add one */
    if (p > data && *(p-1) != '\n')
        count++;

    return count;
}


static int count_mounts (mount_tab_line_t *line)
{
    mount_tab_line_t *child;
    int res = 0;

    if (!line->covered)
        res += 1;

    child = line->first_child;
    while (child != NULL)
    {
        res += count_mounts (child);
        child = child->next_sibling;
    }

    return res;
}


static char *skip_token (char *line, int eat_whitespace)
{
    while (*line != ' ' && *line != '\n')
        line++;

    if (eat_whitespace && *line == ' ')
        line++;

    return line;
}


static char *unescape_inline (char *escaped)
{
    char *unescaped, *res;
    const char *end;

    res = escaped;
    end = escaped + strlen (escaped);

    unescaped = escaped;
    while (escaped < end)
    {
        if (*escaped == '\\')
        {
            *unescaped++ = ((escaped[1] - '0') << 6) | 
                           ((escaped[2] - '0') << 3) |
                           ((escaped[3] - '0') << 0);
            escaped += 4;
        }
        else
        {
            *unescaped++ = *escaped++;
        }
    }
    *unescaped = 0;
    return res;
}


static int has_path_prefix (const char *str, const char *prefix)
{
    while (1)
    {
        /* Skip consecutive slashes to reach next path
           element */
        while (*str == '/')
            str++;
        while (*prefix == '/')
            prefix++;

        /* No more prefix path elements? Done! */
        if (*prefix == 0)
            return 1;

        /* Compare path element */
        while (*prefix != 0 && *prefix != '/')
        {
            if (*str != *prefix)
                return 0;
            str++;
            prefix++;
        }

        /* Matched prefix path element,
           must be entire str path element */
        if (*str != '/' && *str != 0)
            return 0;
    }
}


static int path_equal (const char *path1, const char *path2)
{
    while (1)
    {
        /* Skip consecutive slashes to reach next path
           element */
        while (*path1 == '/')
            path1++;
        while (*path2 == '/')
            path2++;

        /* No more prefix path elements? Done! */
        if (*path1 == 0 || *path2 == 0)
            return *path1 == 0 && *path2 == 0;

        /* Compare path element */
        while (*path1 != 0 && *path1 != '/')
        {
            if (*path1 != *path2)
                return 0;
            path1++;
            path2++;
        }

        /* Matched path1 path element, must be entire path element */
        if (*path2 != '/' && *path2 != 0)
            return 0;
    }
}


static int match_token(const char *token, const char *token_end, const char *str)
{
    while (token != token_end && *token == *str)
    {
        token++;
        str++;
    }
    if (token == token_end)
        return *str == 0;

    return 0;
}


static unsigned long decode_mountoptions(const char *options)
{
    const char *token, *end_token;
    int i;
    unsigned long flags = 0;
    token = options;
    do
    {
        end_token = strchr (token, ',');
        if (end_token == NULL)
            end_token = token + strlen (token);

        for (i = 0; s_mount_flags_data[i].name != NULL; i++)
        {
            if (match_token (token, end_token, s_mount_flags_data[i].name))
            {
                flags |= s_mount_flags_data[i].flag;
                break;
            }
        }

        if (*end_token != 0)
            token = end_token + 1;
        else
            token = NULL;
    }
    while (token != NULL);

    return flags;
}


static mount_tab_t *collect_mounts(mount_tab_t *info, mount_tab_line_t *line)
{
    mount_tab_line_t *child;

    if (!line->covered)
    {
        info->mountpoint = strdup(line->mountpoint);
        if (line->mountpoint && !info->mountpoint)
        {
            ls_stderr("Namespace insufficient memory collecting mounts\n");
            return NULL;
        }
        info->options = decode_mountoptions (line->options);
        info ++;
    }

    child = line->first_child;
    while (child != NULL)
    {
        info = collect_mounts (info, child);
        child = child->next_sibling;
    }
    return info;
}


int persist_mounted(const char *persist_dir, int *persisted)
{
    char *mount_tab_data = NULL;
    int dir_len = strlen(persist_dir);

    DEBUG_MESSAGE("Enter parse_mounted, persist_dir: %s\n", persist_dir);
    mount_tab_data = read_big_proc_file("self/mountinfo");
    if (mount_tab_data == NULL)
    {
        int err = errno;
        ls_stderr("read_big_proc_file failed: %s\n", strerror(err));
        DEBUG_MESSAGE("read_big_proc_file failed: %s\n", strerror(err));
        return -1;
    }
    char *pos = mount_tab_data;
    while ((pos = strstr(pos, persist_dir)))
    {
        if(*(pos + dir_len) == ' ' && pos > mount_tab_data && *(pos - 1) == ' ')
        {
            *persisted = 1;
            break;
        }
    }
    free(mount_tab_data);
    return 0;
}


mount_tab_t *parse_mount_tab(const char *root_mount)
{
    char *mount_tab_data = NULL;
    mount_tab_line_t *lines = NULL;
    mount_tab_line_t **by_id = NULL;
    mount_tab_t *mount_tab = NULL;
    mount_tab_t *end_tab;
    int n_mounts;
    char *line;
    int i;
    int max_id;
    int n_lines;
    int root;

    DEBUG_MESSAGE("Enter parse_mount_tab, root_mount: %s\n", root_mount);
    mount_tab_data = read_big_proc_file("self/mountinfo");
    if (mount_tab_data == NULL)
    {
        DEBUG_MESSAGE("read_big_proc_file failed: %s\n", strerror(errno));
        return NULL;
    }
    n_lines = (int)count_lines (mount_tab_data);
    DEBUG_MESSAGE("Counted mountinfo lines: %d\n", n_lines);
    lines = calloc(n_lines, sizeof (mount_tab_line_t));
    if (!lines)
    {
        DEBUG_MESSAGE("calloc failed %s\n", strerror(errno));
        free(mount_tab_data);
        return NULL;
    }

    max_id = 0;
    line = mount_tab_data;
    i = 0;
    root = -1;
    while (*line != 0)
    {
        int rc, consumed = 0;
        unsigned int maj, min;
        char *end;
        char *rest;
        char *mountpoint;
        char *mountpoint_end;
        char *options;
        char *options_end;
        char *next_line;

        assert(i < (int)n_lines);

        end = strchr (line, '\n');
        if (end != NULL)
        {
            *end = 0;
            next_line = end + 1;
        }
        else
            next_line = line + strlen (line);

        rc = sscanf (line, "%d %d %u:%u %n", &lines[i].id, &lines[i].parent_id, 
                     &maj, &min, &consumed);
        if (rc != 4)
        {
            ls_stderr("Namespace unexpected line parsing mountinfo: %s\n", line);
            free(lines);
            free(mount_tab_data);
            return NULL;
        }
        rest = line + consumed;

        rest = skip_token(rest, 1); /* mountroot */
        mountpoint = rest;
        rest = skip_token(rest, 0); /* mountpoint */
        mountpoint_end = rest++;
        options = rest;
        rest = skip_token(rest, 0); /* vfs options */
        options_end = rest;

        *mountpoint_end = 0;
        lines[i].mountpoint = unescape_inline (mountpoint);

        *options_end = 0;
        lines[i].options = options;

        if (lines[i].id > max_id)
            max_id = lines[i].id;
        if (lines[i].parent_id > max_id)
            max_id = lines[i].parent_id;

        if (path_equal (lines[i].mountpoint, root_mount))
            root = i;

        i++;
        line = next_line;
    }
    assert (i == n_lines);

    if (root == -1)
    {
        DEBUG_MESSAGE("No root\n");
        mount_tab = calloc(sizeof(mount_tab_t), 1);
        if (!mount_tab)
            ls_stderr("Namespace error in calloc mount_tab: %s\n", strerror(errno));
        free(lines);
        free(mount_tab_data);
        return mount_tab;
    }

    by_id = calloc((max_id + 1), sizeof (mount_tab_line_t *));
    if (!by_id)
    {
        ls_stderr("Namespace error in call of by_id: %s\n", strerror(errno));
        free(lines);
        free(mount_tab_data);
        return NULL;
    }
    for (i = 0; i < n_lines; i++)
        by_id[lines[i].id] = &lines[i];

    for (i = 0; i < n_lines; i++)
    {
        mount_tab_line_t *this = &lines[i];
        mount_tab_line_t *parent = by_id[this->parent_id];
        mount_tab_line_t **to_sibling;
        mount_tab_line_t *sibling;
        int covered = 0;

        if (!has_path_prefix (this->mountpoint, root_mount))
            continue;

        if (parent == NULL)
            continue;

        if (strcmp (parent->mountpoint, this->mountpoint) == 0)
            parent->covered = 1;

        to_sibling = &parent->first_child;
        sibling = parent->first_child;
        while (sibling != NULL)
        {
            /* If this mountpoint is a path prefix of the sibling,
             * say this->mp=/foo/bar and sibling->mp=/foo, then it is
             * covered by the sibling, and we drop it. */
            if (has_path_prefix (this->mountpoint, sibling->mountpoint))
            {
                covered = 1;
                break;
            }

            /* If the sibling is a path prefix of this mount point,
             * say this->mp=/foo and sibling->mp=/foo/bar, then the sibling
             * is covered, and we drop it.
             */
            if (has_path_prefix (sibling->mountpoint, this->mountpoint))
                *to_sibling = sibling->next_sibling;
            else
                to_sibling = &sibling->next_sibling;
            sibling = sibling->next_sibling;
        }

        if (covered)
            continue;

        *to_sibling = this;
    }

    n_mounts = count_mounts(&lines[root]);
    DEBUG_MESSAGE("Final n_mounts: %d\n", n_mounts);
    mount_tab = calloc(sizeof(mount_tab_t), (n_mounts + 1));
    if (!mount_tab)
        ls_stderr("Namespace error in final call of mount_tab: %s\n", strerror(errno));
    else
    {
        end_tab = collect_mounts (&mount_tab[0], &lines[root]);
        if (end_tab != &mount_tab[n_mounts])
            ls_stderr("Namespace expected end_tab to be equal to last mount\n");
    }
    free(by_id);
    free(lines);
    free(mount_tab_data);
    return mount_tab;
}


void free_mount_tab(mount_tab_t *mount_tab)
{
    int i;
    if (!mount_tab)
        return;
    for (i = 0; mount_tab[i].mountpoint != NULL; i++)
        free(mount_tab[i].mountpoint);
    free(mount_tab);
}


static char *persist_namespace_dir(uid_t uid, char *dirname, int dirname_len)
{
    snprintf(dirname, dirname_len, PERSIST_PREFIX"/%u", uid);
    return dirname;
}


char *persist_namespace_dir_vh(uid_t uid, char *dirname, int dirname_len)
{
    char dirname_only[PERSIST_DIR_SIZE];
    persist_namespace_dir(uid, dirname_only, sizeof(dirname_only));
    if (s_persist_num[0])
    {
        if (s_persisted_VH && s_persisted_VH[0] != 0)
            snprintf(dirname, dirname_len, "%s/%s/%s", dirname_only, 
                     s_persisted_VH, s_persist_num);
        else
            snprintf(dirname, dirname_len, "%s/%s", dirname_only,  
                     s_persist_num);
    }
    else 
    {
        if (s_persisted_VH && s_persisted_VH[0] != 0)
            snprintf(dirname, dirname_len, "%s/%s", dirname_only, 
                     s_persisted_VH);
        else
            snprintf(dirname, dirname_len, "%s", dirname_only);
    } 
    return dirname;
}


static char *get_ns_filename(uid_t uid, char *filename, int filename_len)
{
    char dirname[PERSIST_DIR_SIZE];
    persist_namespace_dir_vh(uid, dirname, sizeof(dirname));
    snprintf(filename, filename_len, "%s/mnt", dirname);
    DEBUG_MESSAGE("get_ns_filename uid: %d: %s\n", uid, filename);
    return filename;
}


static int unpersist_fn(int report, uid_t uid)
{
    char dest[PERSIST_FILE_SIZE];
    int rc = 0, write_lock = s_persist_vh_locked_write;
    DEBUG_MESSAGE("unpersist_fn, uid: %d, num: %s\n", uid, s_persist_num);
    if (s_persist_vh_fd == -1)
    {
        DEBUG_MESSAGE("Running unpersist_fn without being locked\n");
        ls_stderr("Running unpersist_fn without being locked - internal error\n");
        return -1;
    }
    if (!write_lock && lock_persist_vh_file(report, NSPERSIST_LOCK_WRITE))
        return -1;
    char filename[PERSIST_FILE_SIZE];
    persist_vh_file_name(uid, filename, sizeof(filename));
    get_ns_filename(uid, dest, sizeof(dest));
    if (!access(dest, 0))
    {
        int mounted = 1;
        DEBUG_MESSAGE("umount %s\n", dest);
        if (report && persist_mounted(dest, &mounted))
            return -1; // Fatal
        if (mounted)
        {
            if (umount(dest) != 0)
            {
                int err = errno;
                if (report)
                    ls_stderr("Namespace umount of %s failed: %s\n", dest, strerror(err));
                rc = -1;
                DEBUG_MESSAGE("umount of %s failed: %s\n", dest, strerror(err));
            }
            else
            {
                DEBUG_MESSAGE("Did umount of: %s\n", dest);
            }
        }
    }
    char dirname[PERSIST_DIR_SIZE];
    DIR *dir;
    struct dirent *ent;
                
    if (!(dir = opendir(persist_namespace_dir_vh(uid, dirname, 
                                                 sizeof(dirname)))))
    {
        DEBUG_MESSAGE("Error doing opendir of %s: %s\n", dirname,
                      strerror(errno));
    }
    else while ((ent = readdir(dir)))
    {
        char fullname[PERSIST_DIR_SIZE * 2];
        snprintf(fullname, sizeof(fullname), "%s/%s", dirname, ent->d_name);
        if (ent->d_type == DT_DIR)
        {
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") ||
                strncmp(ent->d_name, PERSIST_NO_VH_PREFIX, PERSIST_NO_VH_PREFIX_LEN))
                continue;
            clean_dir(fullname);
        }
        if (!strcmp(fullname, filename))
            DEBUG_MESSAGE("Do not remove persist vh file: %s\n", filename)
        else
        {
            DEBUG_MESSAGE("remove: %s\n", fullname);
            if (ent->d_type != DT_DIR)
            {
                if (umount(fullname))
                {
                    DEBUG_MESSAGE("Error umount: %s: %s\n", fullname, strerror(errno));
                }
                else
                {
                    DEBUG_MESSAGE("Did umount: %s\n", fullname);
                }
            }
            if (remove(fullname))
            {
                DEBUG_MESSAGE("Error deleting %s: %s\n", fullname,
                              strerror(errno));
            }
        }
    }
    if (dir)
        closedir(dir);
    unlock_close_persist_vh_file(dir != NULL);
    if (dir)
    {
        DEBUG_MESSAGE("umount %s\n", dirname);
        if (umount(dirname) != 0)
        {
            int err = errno;
            DEBUG_MESSAGE("Error doing umount of %s: %s\n", dirname,
                          strerror(err));
        }
    }
    if (!rc)
    {
        int ret = rmdir(dirname);
        DEBUG_MESSAGE("Remove of %s, rc: %d, %s\n", dirname, ret, strerror(errno));
    }
    return rc;
}
            
    
static int mount_private(char *dir)
{
    mount_tab_t *mount_tab = NULL;
    if (!s_persist_created_dir)
        mount_tab = parse_mount_tab(dir);
    if (s_persist_created_dir || !mount_tab || !mount_tab[0].mountpoint)
    {
        DEBUG_MESSAGE("%s not mounted - bind mount it\n", dir);
        if (mount(dir, dir, NULL, MS_SILENT | MS_BIND, NULL) != 0)
        {
            int err = errno;
            ls_stderr("Error bind mounting %s which is required for "
                      "persisting a namespace: %s\n", dir, strerror(err));
            if (mount_tab)
                free_mount_tab(mount_tab);
            return -1;
        }
        DEBUG_MESSAGE("Now make it private\n");
        if (mount(NULL, dir, NULL, MS_PRIVATE, NULL))
        {
            int err = errno;
            ls_stderr("Error bind mounting private %s which is required for "
                      "persisting a namespace: %s\n", dir, strerror(err));
            if (mount_tab)
                free_mount_tab(mount_tab);
            return -1;
        }
    }
    else if (mount_tab && mount_tab[0].mountpoint)
        DEBUG_MESSAGE("%s already mounted, flags: %ld\n", dir, (long)mount_tab->options);
    if (mount_tab)
        free_mount_tab(mount_tab);
    return 0;
}


/* This function does the opposite of what all other functions do: it mounts to
 * the permanent namespace stuff that's local.  Yikes!  */
static int persist_namespaces(lscgid_t *pCGI, int persisted, pid_t pid)
{
    char dir[PERSIST_DIR_SIZE];
    char source[PERSIST_FILE_SIZE], dest[PERSIST_FILE_SIZE + 5];
    int rc;
    DEBUG_MESSAGE("persist_namespaces\n");
    persist_namespace_dir_vh(pCGI->m_data.m_uid, dir, sizeof(dir));
    rc = mount_private(dir);
    if (rc)
        return rc;
    snprintf(dest, sizeof(dest), "%s/mnt", dir);
    int fd = creat(dest, S_IRUSR | S_IWUSR);
    if (fd != -1)
    {
        DEBUG_MESSAGE("Created %s\n", dest);
        close(fd);
    }
    else
        DEBUG_MESSAGE("Namespace directory create of %s failed: %s (may be ok)\n",
                      dest, strerror(errno));

    snprintf(source, sizeof(source), "/proc/%u/ns/mnt", pid);
    DEBUG_MESSAGE("mount %s on %s\n", source, dest);
    if (mount(source, dest, NULL, MS_SILENT | MS_BIND | MS_PRIVATE, NULL) != 0)
    {
        int err = errno;
        ls_stderr("Namespace mount of %s on %s failed: %s\n", source, 
                  dest, strerror(err));
        DEBUG_MESSAGE("Source exists: %s\n", access(source,0) ? "NO":"YES");
        DEBUG_MESSAGE("Dest exists: %s\n", access(dest,0) ? "NO":"YES");
        unpersist_fn(0, pCGI->m_data.m_uid);
        return -1;
    }
    DEBUG_MESSAGE("Did bind_mount of: %s on [%s]\n", source, dest);
    s_persisted = 1;
    return 0;
}
            
    
static void close_pipe(int pipefd[])
{
    if (pipefd)
    {
        if (pipefd[0] != -1)
            close(pipefd[0]);
        if (pipefd[1] != -1)
            close(pipefd[1]);
    }
}


static void close_pipes(int pipe1[], int pipe2[])
{
    close_pipe(pipe1);
    close_pipe(pipe2);
}


static void persist_child(lscgid_t *pCGI, int persisted, 
                          int persist_sibling_child[], int persist_sibling_rc[])
{
    nsipc_child_started_t child_started;
    int do_unpersist = 0, rc = 0;
    
    close(persist_sibling_child[1]);
    persist_sibling_child[1] = -1;
    close(persist_sibling_rc[0]);
    persist_sibling_rc[0] = -1;
    DEBUG_MESSAGE("Wait for persist parent\n");
    rc = read(persist_sibling_child[0], &child_started, sizeof(child_started));
    if (rc == -1)
        ls_stderr("Namespace error reading from sibling pipe: %s\n", strerror(errno));
    else if (rc == 0)
    {
        ls_stderr("Namespace sibling pipe closed early\n");
        rc = -1;
    }
    else
        rc = 0;
    if (!rc)
        rc = persist_namespaces(pCGI, persisted, child_started.m_pid);
    {
        nsipc_error_t error;
        DEBUG_MESSAGE("persist_child release parent, read done, rc: %d\n", rc);
        memset(&error, 0, sizeof(error));
        error.m_type = NSIPC_ERROR;
        error.m_rc = rc;
        if (write(persist_sibling_rc[1], &error, sizeof(error)) == -1)
            DEBUG_MESSAGE("Write error writing to persist_sibling_rc pipe: %s\n", 
                          strerror(errno)); // But otherwise keep doing.
    }
    if (!rc)
    {
        do_unpersist = 1;
        close(persist_sibling_rc[1]);
        persist_sibling_rc[1] = -1;
        //DEBUG_MESSAGE("persist_child wait for parent to release me\n");
        //rc = read(persist_sibling_child[0], &child_started, sizeof(child_started));
        //DEBUG_MESSAGE("persist_child released, read rc: %d\n", rc);
        //if (rc <= 0)
        //    rc = -1;
        //else
        //    rc = 0;
    }
    if (rc)
    {
        if (do_unpersist)
            unpersist_fn(0, pCGI->m_data.m_uid);
    }
    close_pipes(persist_sibling_child, persist_sibling_rc);
    persist_exit_child("persist_child", rc);
}


static int persist_parent(int persisted, int persist_sibling_child[], 
                          int persist_sibling_rc[], pid_t *parent_pid)
{
    int rc = 0, err = 0, clone_flags = CLONE_NEWNS;
    close(persist_sibling_child[0]);
    persist_sibling_child[0] = -1;
    close(persist_sibling_rc[1]);
    persist_sibling_rc[1] = -1;
        
    DEBUG_MESSAGE("Doing unshare: flags: 0x%x\n", clone_flags);
    rc = unshare(clone_flags);
    if (rc)
    {
        int err = errno;
        ls_stderr("Namespace error in unshare: 0x%x: %s\n", s_clone_flags & ~SIGCHLD, 
                  strerror(err));
        DEBUG_MESSAGE("Closing persist sibling_child due to Namespace error in "
                      "unshare: 0x%x: %s\n", s_clone_flags & ~SIGCHLD, 
                      strerror(err));
        close_pipes(persist_sibling_child, persist_sibling_rc);

        return nsopts_rc_from_errno(err);
    }
    *parent_pid = getpid();
    DEBUG_MESSAGE("Final pid to wait on: %d (my pid: %d)\n", *parent_pid, 
                  getpid());
    
    if (rc)
    {
        DEBUG_MESSAGE("Closed persist_sibling_child, rc: %d\n", rc);
        close_pipes(persist_sibling_child, persist_sibling_rc);
        if (err)
            return nsopts_rc_from_errno(err);
        return -1;
    }
    return rc;
}


static char *persist_vh_file_name(uid_t uid, char *filename, int filename_size)
{
    char dirname[PERSIST_DIR_SIZE];
    persist_namespace_dir_vh(uid, dirname, sizeof(dirname));
    snprintf(filename, filename_size, "%s/"PERSIST_NO_VH_PREFIX PERSIST_VH_FILE, 
             dirname);
    return filename;
}


static int try_delete_persist_fn()
{
    char persist_name[PERSIST_FILE_SIZE];
    int opened = 0;
    persist_persist_file_name(persist_name, sizeof(persist_name));
    DEBUG_MESSAGE("try_delete_persist: %s\n", persist_name);
    if (s_persist_fd == -1)
    {
        s_persist_fd = open(persist_name, O_RDONLY);
        if (s_persist_fd == -1) 
        {
            DEBUG_MESSAGE("Error operning to delete persist file: %s\n", strerror(errno));
            return -1;
        }
        opened = 1;
    }
    if (lock_file(s_persist_uid, s_persist_fd, "try_delete_persist", 0, 
                  NSPERSIST_LOCK_WRITE, NULL))
    {
        if (opened)
        {
            close(s_persist_uid);
            s_persist_uid = -1;
        }
        return -1;
    }
    char persist_dir[PERSIST_FILE_SIZE];
    int unmount_delete_dir = 0;
    persist_persist_file_dir(persist_dir, sizeof(persist_dir));
    /* Make sure directory is empty except for persist file*/
    DIR *dir = opendir(persist_dir);
    if (!dir)
    {
        int err = errno;
        DEBUG_MESSAGE("In unpersist of %s: %s\n", persist_dir, strerror(err));
    }
    else {
        struct dirent *ent;
        while ((ent = readdir(dir)))
        {
            if (ent->d_type == DT_DIR && 
                strspn(ent->d_name, "0123456789") == strlen(ent->d_name))
                DEBUG_MESSAGE("Existing dir %s\n", ent->d_name);
                break;
            if (ent->d_type == DT_DIR && ent->d_name[0] == '.')
                continue;
            if (!strcmp(ent->d_name, PERSIST_FILE))
                continue;
            DEBUG_MESSAGE("Unexpected: %s\n", ent->d_name);
        }
        if (ent == NULL)        
            unmount_delete_dir = 1;
    }
    closedir(dir);
    if (unmount_delete_dir)
    {
        DEBUG_MESSAGE("Delete %s and %s\n", persist_name, persist_dir);
        if (remove(persist_name))
        {
            DEBUG_MESSAGE("Delete %s failed: %s\n", persist_name, strerror(errno));
        }
        close_unlock_persist();
        if (remove(persist_dir))
        {
            DEBUG_MESSAGE("Delete %s failed: %s\n", persist_dir, strerror(errno));
        }
        else
        {
            DEBUG_MESSAGE("try_delete_persist worked!\n");
        }
    }
    return 0;
}


int unlock_close_persist_vh_file(int delete)
{
    char dir[PERSIST_DIR_SIZE] = { 0 };
    int try_delete_persist = 0;
    DEBUG_MESSAGE("Close and unlock the persist vh file vh fd: %d\n", s_persist_vh_fd);
    if (s_persist_vh_fd == -1)
        return 0;
    if (delete)
    {
        lock_file(s_persist_uid, s_persist_vh_fd, "Delete VH file", 0, 
                  NSPERSIST_LOCK_TRY_WRITE, &s_persist_vh_locked_write);
        if (!s_persist_vh_locked_write)
            delete = 0;
        else
        {
            int rc = clean_dir(persist_namespace_dir_vh(s_persist_uid, dir, sizeof(dir)));
            DEBUG_MESSAGE("Removed persistence vh dir: %s: %s (%s)\n", 
                          dir, rc == 0 ? "YES" : "NO", strerror(errno));
            if (rc)
                delete = 0;
            else
                try_delete_persist = 1;
        }
    }
    flock(s_persist_vh_fd, LOCK_UN);
    close(s_persist_vh_fd);
    s_persist_vh_fd = -1;
    if (delete && try_delete_persist)
    {
        try_delete_persist_fn(NULL);
    }
    DEBUG_MESSAGE("unlock_close_persist_vh_file done\n");
    return 0;
}


int lock_persist_vh_file(int report, int lock_type)
{
    int ret = lock_file(s_persist_uid, s_persist_vh_fd, "VH persist", report, 
                        lock_type, &s_persist_vh_locked_write);
    //DEBUG_MESSAGE("Close persist file\n");
    //close_unlock_persist();
    return ret;
}


static int open_persist_vh_file(int report, uid_t uid, int lock_type)
{
    char filename[PERSIST_FILE_SIZE];
    int rc;
    if (s_persist_vh_fd > 0)
    {
        if (uid != s_persist_uid)
            unlock_close_persist_vh_file(1);
        else
        {
            DEBUG_MESSAGE("file already open, just locking\n");
            return lock_persist_vh_file(report, lock_type);
        }
    }
    persist_vh_file_name(uid, filename, sizeof(filename));
    DEBUG_MESSAGE("Open/lock persist vh file %s for %s lock\n", filename,
                  lock_type == NSPERSIST_LOCK_TRY_WRITE ? "TRY_WRITE" :
                  (lock_type == NSPERSIST_LOCK_WRITE ? "WRITE" : "READ"));
    s_persist_vh_fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int err = errno;
    if (s_persist_vh_fd == -1)
    {
        DEBUG_MESSAGE("Namespace error opening lock file %s: %s\n",
                      filename, strerror(err));
        if (err != ENOENT)
            ls_stderr("Namespace error opening lock file %s: %s\n",
                      filename, strerror(err));
        else
            return 0; // Missing component, just fine!
        return nsopts_rc_from_errno(err);
    }
    //fcntl(s_persist_vh_fd, F_SETFD, FD_CLOEXEC); MUST LEAVE THIS OPEN FOR PERSISTENCE!
    /* Use a Posix record lock and make sure that the parent is always the one
     * with the lock.  */
    s_persist_uid = uid;
    rc = lock_persist_vh_file(report, lock_type);
    if (rc)
        unlock_close_persist_vh_file(0);
    return rc;
}


int persist_ns_start(lscgid_t *pCGI, int persisted, pid_t *parent_pid, int persist_sibling_child[],
                     int persist_sibling_rc[])
{
    int sibling_pid, rc;

    persist_sibling_child[0] = -1;
    persist_sibling_child[1] = -1;
    persist_sibling_rc[0] = -1;
    persist_sibling_rc[1] = -1;
    if (pipe(persist_sibling_child) || pipe(persist_sibling_rc))
    {
        int err = errno;
        ls_stderr("Namespace error creating child sibling pipes: %s\n", strerror(err));
        close_pipes(persist_sibling_child, persist_sibling_rc);
        return nsopts_rc_from_errno(err);
    }
    sibling_pid = fork();
    if (sibling_pid == -1)
    {
        int err = errno;
        close_pipes(persist_sibling_child, persist_sibling_rc);
        ls_stderr("Namespace error creating child sibling: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    else if (sibling_pid == 0)
    {
        persist_child(pCGI, persisted, persist_sibling_child, persist_sibling_rc);
    }
    DEBUG_MESSAGE("child_start pid: %d\n", sibling_pid);
    
    rc = persist_parent(persisted, persist_sibling_child, persist_sibling_rc, 
                        parent_pid);
    return rc;
}


static int persist_do(int persist_pid, int persisted, 
                      int persist_sibling_child[], int persist_sibling_rc[])
{
    nsipc_child_started_t child_started;
    int rc;
    
    child_started.m_type = NSIPC_CHILD_STARTED;
    child_started.m_pid = persist_pid;
    DEBUG_MESSAGE("persist_do start child\n");
    if (write(persist_sibling_child[1], &child_started, 
              sizeof(child_started)) == -1)
    {
        int err = errno;
        ls_stderr("Write error writing to persist_sibling_child: %s\n",
                  strerror(err)); 
        return nsopts_rc_from_errno(err);
    }
    nsipc_error_t error;
    DEBUG_MESSAGE("persist_do get rc\n");
    rc = read(persist_sibling_rc[0], &error, sizeof(error));
    if (rc == 0)
    {
        ls_stderr("Namespace parent detected child closed pipe (see above)\n");
        rc = -1;
    }
    else if (rc < 0)
    {
        int err = errno;
        ls_stderr("Namespace error in waitpid in sibling parent: %s\n", 
                  strerror(err));
        rc = nsopts_rc_from_errno(err);
    }
    else if (error.m_rc)
    {
        ls_stderr("Namespace child reported error %d\n", error.m_rc);
        rc = error.m_rc;
    }
    else
        rc = 0;
    DEBUG_MESSAGE("persist_do, final rc: %d\n", rc);
    return rc;
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
            return -1;
        }
        if (ret > 0)
        {
            left -= ret;
            pBuf += ret;
        }
    }
    return len;
}


static int report_pid_to_parent(lscgid_t *pCGI, uint32_t pid)
{
    if (s_not_lscgid)
    {
        DEBUG_MESSAGE("report_pid_to_parent SKIP for not lscgid\n");
        return 0;
    }
    DEBUG_MESSAGE("report_pid_to_parent: %d\n", pid);
    if (**pCGI->m_argv == '&')
    {
        static const char sHeader[] = "Status: 200\r\n\r\n";
        writeall(STDOUT_FILENO, sHeader, sizeof(sHeader) - 1);
        pCGI->m_pCGIDir = "/bin/sh";
        pCGI->m_argv[0] = "/bin/sh";
    }
    else
    {
        //pCGI->m_argv[0] = strdup( pCGI->m_pCGIDir );
        pCGI->m_argv[0] = pCGI->m_pCGIDir;
    }

    {
        uint32_t pid = (uint32_t)getpid();
        if (write(STDOUT_FILENO, &pid, 4) < 0)
            DEBUG_MESSAGE("Can't write pid to parent: %s, may be ok\n", 
                          strerror(errno));
    }
    return 0;
}

int persist_ns_done(lscgid_t *pCGI, int persisted, int rc, 
                    int persist_pid, int persist_sibling_child[], 
                    int persist_sibling_rc[])
{
    if (persist_sibling_child[1] != -1)
    {
        if (rc)
        {
            /*
            DEBUG_MESSAGE("Error in persistence, so unpersist all\n");
            unpersist(s_persist_uid, NS_TYPE_COUNT);
            char persist_file[PERSIST_FILE_SIZE];
            persist_file_name(s_persist_uid, persist_file, sizeof(persist_file));
            DEBUG_MESSAGE("Close and delete persistence file %s\n", persist_file);
            remove(persist_file);
            */
        }
        else
            rc = persist_do(persist_pid, persisted, persist_sibling_child, 
                            persist_sibling_rc);
        if (!rc)
        {
            DEBUG_MESSAGE("persist_child_done, pid: %d, persist_pid: %d\n", 
                          getpid(), persist_pid);
            persist_report_pid(persist_pid, rc);
            report_pid_to_parent(pCGI, persist_pid);
        }
    }
    if (rc)
        persist_report_pid(0, rc);
        
    DEBUG_MESSAGE("Closed persist_sibling_child\n");
    close_pipes(persist_sibling_child, persist_sibling_rc);
    lock_persist_vh_file(0, NSPERSIST_LOCK_READ); // Doesn't release the lock, but it allows other tasks to go
    return rc;
}


/* Note: For now we use all or none of persisted mounts.  There are still known
 * issues with attempting to use some (like you can't mount /proc without a PID 
 * space).  */
int is_persisted_fn(int uid, int must_persist, int *persisted)
{
    int rc = 0, was_vh_fd = s_persist_vh_fd;
    char dir[PERSIST_DIR_SIZE];
    persist_namespace_dir_vh(uid, dir, sizeof(dir));
    DEBUG_MESSAGE("is_persisted uid: %d\n", uid);
    if (was_vh_fd == -1 && 
        open_persist_vh_file(1, uid, NSPERSIST_LOCK_TRY_WRITE))
        return DEFAULT_ERR_RC;
    char ns_filename[PERSIST_FILE_SIZE];
    get_ns_filename(uid, ns_filename, sizeof(ns_filename));
    if (persist_mounted(ns_filename, persisted))
        return DEFAULT_ERR_RC;
    if (!*persisted)
    {
        DEBUG_MESSAGE("NOT persisted user\n");
        if (must_persist)
        {
            ls_stderr("User is not persisted\n");
            return -1;
        }
        return 0;
    }
    if (was_vh_fd != -1 && s_persist_vh_locked_write)        
        return 0;
    if (was_vh_fd == -1 && s_persist_vh_locked_write)
    {
        DEBUG_MESSAGE("Got write lock but don't need it\n")
        lock_persist_vh_file(1, NSPERSIST_LOCK_READ);
    }
    DEBUG_MESSAGE("IS persisted user, rc: %d\n", rc);
    return rc;
}


static int setUIDs(uid_t uid, gid_t gid, char *pChroot)
{
    int rv;
    struct passwd pwd, *pw;
    char buffer[1024];
    //if ( !uid || !gid )  //do not allow root
    //{
    //    return -1;
    //}

    if (getpwuid_r(uid, &pwd, buffer, sizeof(buffer), &pw) == -1)
    {
        int err = errno;
        ls_stderr("Namespace error lscgid: getpwuid(%d): %s\n", uid, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    DEBUG_MESSAGE("Set gid for %d\n", gid);
    rv = setgid(gid);
    if (rv == -1)
    {
        int err = errno;
        ls_stderr("Namespace error lscgid: setgid(%d): %s\n", gid, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    if (pw && (pw->pw_gid == gid))
    {
        DEBUG_MESSAGE("initgroups, name: %s\n", pw->pw_name);
        rv = initgroups(pw->pw_name, gid);
        if (rv == -1)
            DEBUG_MESSAGE("Can't initgroups(): %s\n", strerror(errno));
    }
    else
    {
        rv = setgroups(1, &gid);
        if (rv == -1)
            DEBUG_MESSAGE("Namespace error lscgid: setgroups(): %s\n", 
                          strerror(errno));
    }
    DEBUG_MESSAGE("chroot: %s\n", pChroot);
    if (pChroot)
    {
        rv = chroot(pChroot);
        if (rv == -1)
        {
            int err = errno;
            ls_stderr("Namespace error lscgid: chroot(): %s\n", strerror(err));
            return nsopts_rc_from_errno(err);
        }
    }
    DEBUG_MESSAGE("Set uid for %d\n", uid);
    rv = setuid(uid);
    if (rv == -1)
    {
        int err = errno;
        ls_stderr("Namespace error lscgid: setuid(%d): %s", uid, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    return 0;
}


int persist_change_stderr_log(lscgid_t *pCGI)
{
    if (!pCGI->m_stderrPath)
        return 0;

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
    DEBUG_MESSAGE("persist_change_stderr_log path: %s\n", path);
    int newfd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (newfd == -1)
    {
        DEBUG_MESSAGE("Namespace error unable to open log: %s for write: %s\n",
                      path, strerror(errno));
        char altpath[PATH_MAX];
        if (pCGI->m_cwdPath)
            snprintf(altpath, sizeof(altpath), "%s/stderr.log", pCGI->m_cwdPath);
        else
            snprintf(altpath, sizeof(altpath), "/tmp/stderr.log");
        DEBUG_MESSAGE("Try: %s\n", altpath);
        newfd = open(altpath, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (newfd == -1)
        {
            DEBUG_MESSAGE("Namespace error unable to open alt log: %s for write: %s\n",
                          altpath, strerror(errno));
            //ls_stderr("Namespace error unable to open alt log: %s for write: %s, pid: %d\n",
            //          altpath, strerror(errno), getpid());
            return -1;
        }
    }
    if (newfd != 2)
    {
        dup2(newfd, 2);
        close(newfd);
    }
    return 0;
}


int persist_report_pid(pid_t pid, int rc)
{
    if (s_pid_pipe > 0)
    {
        nsipc_t ipc;
        memset(&ipc, 0, sizeof(ipc));
        if (rc)
        {
            DEBUG_MESSAGE("persist_report_pid error: %d\n", rc);
            ipc.m_error.m_type = NSIPC_ERROR;
            ipc.m_error.m_rc = rc;
        }
        else
        {
            DEBUG_MESSAGE("persist_report_pid: %d\n", pid);
            ipc.m_started.m_type = NSIPC_CHILD_STARTED;
            ipc.m_started.m_pid = pid;
        }
        rc = write(s_pid_pipe, &ipc, sizeof(ipc));
        if (rc == -1)
            DEBUG_MESSAGE("persist_report_pid write returned %d: %s\n", rc, strerror(errno));
        return rc > 0 ? 0 : -1;
    }
    return 0;
}

static int persist_use_child(lscgid_t *pCGI)
{
    int fd = -1, rc = 0;
    
    DEBUG_MESSAGE("persist_use_child, pid: %d\n", getpid());
    if (setgroups(0, NULL))
    {
        DEBUG_MESSAGE("setgroups failed first time: %s\n", strerror(errno));
    }
    else
    {
        DEBUG_MESSAGE("setgroups worked first time\n");
    }
    
    char ns_filename[PERSIST_FILE_SIZE];
    get_ns_filename(pCGI->m_data.m_uid, ns_filename, sizeof(ns_filename));
    fd = open(ns_filename, O_RDONLY);
    if (fd == -1)
    {
        rc = -1;
        ls_stderr("Namespace error opening namespace for %s: %s\n", 
                  ns_filename, strerror(errno));
    }
    else if (setns(fd, 0))
    {
        rc = -1;
        ls_stderr("Namespace error setting namespace: %s\n", strerror(errno));
    }
    if (fd != -1)
    {
        /* Close the handles (we're set to use them, don't need the open handles anymore */
        close(fd);
    }
    if (!rc && setgroups(0, NULL))
    {
        DEBUG_MESSAGE("setgroups failed second time: %s\n", strerror(errno));
    }
    else
    {
        DEBUG_MESSAGE("setgroups worked second time\n");
    }
    
    if (rc == 0)
    {
        if (pCGI->m_data.m_uid || pCGI->m_data.m_gid || pCGI->m_pChroot)
            rc = setUIDs(pCGI->m_data.m_uid, pCGI->m_data.m_gid, pCGI->m_pChroot);
    }
    DEBUG_MESSAGE("persist_use_child release parent which will unlock, rc: %d\n", rc);
    
    if (!rc)
    {
        char *dir = pCGI->m_cwdPath;
        char *alloc_dir = NULL;
        if (!dir && !(alloc_dir = strdup(pCGI->m_pCGIDir)))
        {
            ls_stderr("Namespace insufficient memory to alloc dir\n");
            rc = 500;
        }
        else if (alloc_dir)
        {
            char *last_slash = strrchr(alloc_dir, '/');
            if (last_slash)
                *last_slash = 0;
            dir = alloc_dir;
        }
        if (chdir(dir) == -1)
        {
            int err = errno;
            ls_stderr("Namespace change directory error %s: %s\n", dir, 
                      strerror(err));
            if (err == ENOENT)
                rc = 404;
            else if (err == EACCES)
                rc = 403;
            else
                rc = 500;
        }
        if (alloc_dir)
            free(alloc_dir);
    }
    if (!rc)
    {
        report_pid_to_parent(pCGI, getpid());
        if (pCGI->m_data.m_umask)
            umask(pCGI->m_data.m_umask);
    }
    if (rc == 0)
    {
        persist_change_stderr_log(pCGI);
        persist_report_pid(getpid(), 0);
        DEBUG_MESSAGE("Doing execve: %s\n", pCGI->m_pCGIDir);
        if (execve(pCGI->m_pCGIDir, pCGI->m_argv, pCGI->m_env) == -1)
        {
            ls_stderr("lscgid: execve(%s) (nspersist): %s\n", pCGI->m_pCGIDir, 
                      strerror(errno));
            DEBUG_MESSAGE("final_exec failed\n");
        }
        rc = -1;
        DEBUG_MESSAGE("Should not get here!\n");
    }
    //ns_type_ended = ns_type;
    //for (ns_type = 0; ns_type <= ns_type_ended; ++ns_type)
    //{
    //    if (fds[ns_type] != -1)
    //        close(fds[ns_type]);
    //}
    if (rc == -1)
        rc = DEFAULT_ERR_RC;
    persist_report_pid(getpid(), rc);
    persist_exit_child("persist_child", rc);
    return rc; // Not used - keep the compiler happy
}


int persisted_use(lscgid_t *pCGI, int *done)
{
    int rc = 0;
    
    DEBUG_MESSAGE("persisted_use\n");
    *done = 1;
    rc = persist_use_child(pCGI);
    DEBUG_MESSAGE("persisted_use_child returned!\n");
    return rc;
}


static int unmount_vhost(int report, int uid, char *vhost)
{
    int rc = 0;
    DEBUG_MESSAGE("unmount_vhost, uid: %d, num: %s, vhost: %s\n", uid, s_persist_num, vhost);
    if (vhost && vhost != s_persisted_VH)
        nspersist_setvhost(vhost);
    if (open_persist_vh_file(report, uid, NSPERSIST_LOCK_WRITE_NB))
    {
        DEBUG_MESSAGE("Can't lock\n");
        return -1;
    }
    if (s_persist_vh_fd != -1)
    {
        rc = unpersist_fn(report, uid);
        unlock_close_persist_vh_file(rc == 0 ? 1 : 0);
    }
    return rc;
}


int unpersist_uid(int report, uid_t uid, char *vhost)
{
    s_persist_uid = uid;
    s_persist_num[0] = 0;
    nspersist_setvhost(vhost);
    int rc = get_persist_num(1, uid, 1, 0);
    DEBUG_MESSAGE("unpersist_uid: %u, active vhost: %s, active persist: %s\n", uid, s_persisted_VH, s_persist_num);
    char persist_dir[PERSIST_DIR_SIZE];
    persist_persist_file_dir(persist_dir, sizeof(persist_dir));
    DIR *dir_fd = opendir(persist_dir);
    if (!dir_fd)
    {
        int err = errno;
        DEBUG_MESSAGE("Unable to open: %s: %s\n", persist_dir, strerror(err));
        return 0; // Not anything to worry about.
    }
    struct dirent *ent;
    s_persist_num[0] = 0;
    int need_newpersist = 0;
    while ((ent = readdir(dir_fd)))
    {
        /* All that should be there are perist files, persist dirs and VH names */
        if (ent->d_type == DT_DIR)
        {
            if (ent->d_name[0] == '.')
                continue;
            if (strspn(ent->d_name, "0123456789") == strlen(ent->d_name))
            {
                // Assume a persist number
                strncpy(s_persist_num, ent->d_name, sizeof(s_persist_num));
                if (unmount_vhost(report, uid, vhost))
                    need_newpersist = 1;
            }
            else if (!strcmp(ent->d_name, "root"))
            {
                struct stat st;
                char root[PERSIST_FILE_SIZE];
                snprintf(root, sizeof(root), "%s/root", persist_dir);
                if (!lstat(root, &st))
                {
                    if (st.st_mode & 0777)
                    {
                        DEBUG_MESSAGE("Remove old directory: %s\n", root);
                        clean_dir(root);
                        if (remove(root))
                            DEBUG_MESSAGE("Error removing old root directory: %s: %s\n", root, strerror(errno));
                        continue;
                    }
                    else
                       DEBUG_MESSAGE("st_mode of root is: %o\n", st.st_mode);
                }
                else
                    DEBUG_MESSAGE("Error in lstat of %s: %s\n", root, strerror(errno));
            }
            else {
                // Assume a VH
                if (!vhost)
                {
                    close_unlock_persist();
                    unpersist_uid(report, uid, ent->d_name);
                    nspersist_setvhost(vhost);
                    get_persist_num(1, uid, 1, 0);
                }
            }
        }
        else
        {
            if (!strcmp(ent->d_name, PERSIST_FILE))
                continue;
            DEBUG_MESSAGE("Unexpected file: %s, delete it\n", ent->d_name);
            char random_file[PERSIST_FILE_SIZE];
            snprintf(random_file, sizeof(random_file), "%s/%s", persist_dir, ent->d_name);
            remove(random_file);
        }
    }
    if (need_newpersist)
    {
        DEBUG_MESSAGE("unpersist_uid failed, bump persist number, graceful unmount - do new_persist on first use\n");
        rc = new_persist(uid, s_persist_fd, 0);
    }
    else
    {
        char persist[PERSIST_FILE_SIZE];
        persist_persist_file_name(persist, sizeof(persist));
        int ret = remove(persist);
        DEBUG_MESSAGE("Attempt to remove %s, %d, %s\n", persist, ret, strerror(errno));
        char dirname_only[PERSIST_DIR_SIZE];
        s_persist_num[0] = 0;
        persist_namespace_dir_vh(uid, dirname_only, sizeof(dirname_only));
        ret = remove(dirname_only);
        DEBUG_MESSAGE("Attempt to remove dir %s, %d, %s\n", dirname_only, ret, strerror(errno));
    }
    closedir(dir_fd);
    close_unlock_persist();
    return rc;
}   


void nspersist_setuser(uid_t uid)
{
    s_persist_uid = uid;
}


void persist_exit_child(char *desc, int rc)
{
    DEBUG_MESSAGE("Exiting %s, rc: %d\n", desc, rc);
    if (rc)
        persist_report_pid(0, rc); // Just so it really gets done for an error!
    exit(rc);
}


int nspersist_done()
{
    return 0;
}


int unpersist_all()
{
    DIR *dir;
    struct dirent *ent;
    int rc = 0;
    
    DEBUG_MESSAGE("unpersist_all\n");
    dir = opendir(PERSIST_PREFIX);
    if (!dir)
    {
        int err = errno;
        ls_stderr("Namespaces unable to open %s: %s\n", PERSIST_PREFIX, 
                  strerror(err));
        return -1;
    }
    while (/*!rc && */(ent = readdir(dir)))
    {
        if (ent->d_type == DT_DIR && 
            strspn(ent->d_name, "0123456789") == strlen(ent->d_name))
            rc = unpersist_uid(0, atoi(ent->d_name), NULL);
    }
    closedir(dir);
    return rc;
}

#endif // Linux only