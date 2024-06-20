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

uid_t s_persist_uid = -1;
int   s_persist_vh_fd = -1;
int   s_persist_vh_locked_write = 0;
static int is_persisted_fn(int uid, int *persisted);
static int open_persist_vh_file(int report, uid_t uid, int lock_type);
static char *persist_vh_file_name(uid_t uid, char *filename, int filename_size);
static int persist_vh_lock_type()   { return s_persist_vh_fd == -1 ? -1 : s_persist_vh_locked_write; }

int is_persisted(lscgid_t *pCGI, int *persisted)
{
    return is_persisted_fn(pCGI->m_data.m_uid, persisted);
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
    int fd = openat(s_proc_fd, filename, O_RDONLY);
    if (fd == -1)
    {
        int err = errno;
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
            ls_stderr("Namespace error reading /proc/%s: %s\n", filename, 
                      strerror(errno));
            free(block);
            close(fd);
            return NULL;
        }
        pos += len;
        block[pos] = 0;
    } while (len);
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
    if (s_persisted_VH)
    {
        char dirname_only[PERSIST_DIR_SIZE];
        persist_namespace_dir(uid, dirname_only, sizeof(dirname_only));
        if (s_persisted_VH && s_persisted_VH[0] == 0)
            snprintf(dirname, dirname_len, "%s", dirname_only);
        else
            snprintf(dirname, dirname_len, "%s/%s", dirname_only, s_persisted_VH);
    }
    else
        persist_namespace_dir(uid, dirname, dirname_len);
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


static void user_unpersist(uid_t uid, int *have_unpersist)
{
    int rc = 0, user_unpersist = 0;
    struct passwd *pwd = getpwuid(uid);
    if (!pwd)
    {
        DEBUG_MESSAGE("Could not getpwuid for %d (deleted?): %s\n", uid,
                      strerror(errno));
        rc = DEFAULT_ERR_RC;
    }
    else
    {
        if (pwd->pw_dir && pwd->pw_dir[0])
        {
            char unpersist_file[strlen(pwd->pw_dir) + PERSIST_USER_FILE_LEN + 1];
            snprintf(unpersist_file, sizeof(unpersist_file), "%s%s", pwd->pw_dir,
                     PERSIST_USER_FILE);
            if (!unlink(unpersist_file))
            {
                ls_stderr("User requested namespace unpersist, uid: %d, file: %s\n", 
                          uid, unpersist_file);
                DEBUG_MESSAGE("User requested namespace unpersist\n");
                user_unpersist = 1;
            }
            else if (errno != ENOENT)
            {
                int err = errno;
                ls_stderr("Unable to delete unpersist file %s: %s\n", 
                          unpersist_file, strerror(err));
                rc = nsopts_rc_from_errno(err);
            }
        }
    }
    if ((rc || user_unpersist) && have_unpersist)
        *have_unpersist = 1;
}


static int unpersist_fn(int report, uid_t uid, int vh_only, int *NOW_PERSISTED)
{
    char dest[PERSIST_FILE_SIZE];
    int rc = 0, write_lock = s_persist_vh_locked_write;
    DEBUG_MESSAGE("unpersist_fn, uid: %d\n", uid);
    if (s_persist_vh_fd == -1)
    {
        DEBUG_MESSAGE("Running unpersist_fn without being locked\n");
        ls_stderr("Running unpersist_fn without being locked - internal error\n");
        return -1;
    }
    if (!write_lock && lock_persist_vh_file(report, NSPERSIST_LOCK_WRITE))
        return -1;
    int user_unpersisted = 0;
    user_unpersist(uid, &user_unpersisted);
    if (!vh_only && NOW_PERSISTED && !write_lock && user_unpersisted &&
        (rc = is_persisted_fn(uid, NOW_PERSISTED)))
    {
        if (rc)
            return rc;
        if (*NOW_PERSISTED)
        {
            DEBUG_MESSAGE("unpersist_fn, a prior unperist condition has been resolved by another task\n");
            lock_persist_vh_file(report, NSPERSIST_LOCK_READ);
            return 0;
        }
    }
    char filename[PERSIST_FILE_SIZE];
    persist_vh_file_name(uid, filename, sizeof(filename));
    int unmounted;
    get_ns_filename(uid, dest, sizeof(dest));
    do
    {
        unmounted = 0;
        DEBUG_MESSAGE("umount %s\n", dest);
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
            unmounted = 1;
        }
    } while (unmounted && remove(dest));
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
            if (remove(fullname))
                DEBUG_MESSAGE("Error deleting %s: %s\n", fullname,
                              strerror(errno));
        }
    }
    if (dir)
        closedir(dir);
    return rc;
}
            
    
static int mount_private(int created, char *dir)
{
    mount_tab_t *mount_tab = NULL;
    if (!created)
        mount_tab = parse_mount_tab(dir);
    if (created || !mount_tab || !mount_tab[0].mountpoint)
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
    char dir[PERSIST_DIR_SIZE], vhdir[PERSIST_FILE_SIZE], *dirp;
    char source[PERSIST_FILE_SIZE], dest[PERSIST_FILE_SIZE];
    int created, rc;
    DEBUG_MESSAGE("persist_namespaces\n");
    mkdir(PERSIST_PREFIX, S_IRWXU);
    persist_namespace_dir(pCGI->m_data.m_uid, dir, sizeof(dir));
    created = mkdir(dir, S_IRWXU) == 0;
    DEBUG_MESSAGE("Namespace directory create of %s, created: %d, errno: %s\n",
            dir, created, strerror(errno));

    if (s_persisted_VH)
        snprintf(vhdir, sizeof(vhdir), "%s/%s", dir, s_persisted_VH);
    rc = mount_private(created, dir);
    if (rc)
        return rc;
    DEBUG_MESSAGE("persist_namespaces\n");
    dirp = dir;
    if (s_persisted_VH)
    {
        dirp = vhdir;
        if (mkdir(dirp, S_IRWXU) && errno != EEXIST)
        {
            int err = errno;
            ls_stderr("Namespace directory create of %s failed: %s\n",
                      dirp, strerror(err));
            unpersist_fn(0, pCGI->m_data.m_uid, 0, NULL);
            return -1;
        }
        snprintf(dest, sizeof(dest), "%s/mnt", dirp);
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
        if (mount(source, dest, NULL, MS_SILENT | MS_BIND, NULL) != 0)
        {
            int err = errno;
            ls_stderr("Namespace mount of %s on %s failed: %s\n", source, 
                      dest, strerror(err));
            DEBUG_MESSAGE("Source exists: %s\n", access(source,0) ? "NO":"YES");
            DEBUG_MESSAGE("Dest exists: %s\n", access(dest,0) ? "NO":"YES");
            unpersist_fn(0, pCGI->m_data.m_uid, 0, NULL);
            return -1;
        }
        DEBUG_MESSAGE("Did bind_mount of: %s on [%s]\n", source, dest);
    }
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
            unpersist_fn(0, pCGI->m_data.m_uid, 0, NULL);
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
    char dirname[PERSIST_FILE_SIZE];
    char dirname_only[PERSIST_DIR_SIZE];
    persist_namespace_dir(uid, dirname_only, sizeof(dirname_only));
    snprintf(dirname, sizeof(dirname), "%s/%s", dirname_only, 
             s_persisted_VH ? s_persisted_VH : "");
    snprintf(filename, filename_size, "%s/"PERSIST_NO_VH_PREFIX PERSIST_VH_FILE, 
             dirname);
    return filename;
}


int unlock_close_persist_vh_file(int delete)
{
    if (s_persist_vh_fd == -1)
        return 0;
    DEBUG_MESSAGE("Close and unlock the persist vh file\n");
    flock(s_persist_vh_fd, LOCK_UN);
    close(s_persist_vh_fd);
    s_persist_vh_fd = -1;
#ifdef PERSIST_CLEANLY
    if (delete && s_persist_uid != -1)
    {
        char filename[PERSIST_FILE_SIZE];
        int rc;
        rc = remove(persist_vh_file_name(s_persist_uid, filename, sizeof(filename)));
        DEBUG_MESSAGE("Removed persistence vh file: %s: %s\n", filename, 
                      rc == 0 ? "YES" : "NO");
    }
#endif
    DEBUG_MESSAGE("unlock_close_persist_vh_file done\n");
    return 0;
}


int lock_persist_vh_file(int report, int lock_type)
{
    DEBUG_MESSAGE("Lock the persist vh file %d %s\n", s_persist_vh_fd, 
                  (lock_type == NSPERSIST_LOCK_TRY_WRITE || lock_type == NSPERSIST_LOCK_WRITE_NB) ? 
                  "TRY_WRITE" : 
                  (lock_type ? "WRITE" : "READ"));
    if (flock(s_persist_vh_fd, 
              (lock_type == NSPERSIST_LOCK_TRY_WRITE || lock_type == NSPERSIST_LOCK_WRITE_NB) ? (LOCK_EX | LOCK_NB) : 
              (lock_type == NSPERSIST_LOCK_WRITE) ? LOCK_EX : LOCK_SH))
    {
        int err = errno;
        if (errno == EWOULDBLOCK)
        {
            if (lock_type == NSPERSIST_LOCK_WRITE_NB)
            {
                DEBUG_MESSAGE("Unable to get write lock for uid %d\n", s_persist_uid);
                if (report)
                    ls_stderr("Unable to get write lock for uid %d\n", s_persist_uid);
                return nsopts_rc_from_errno(err);
            }
            DEBUG_MESSAGE("Converting an attempted write to a read lock!\n");
            return lock_persist_vh_file(report, NSPERSIST_LOCK_READ);
        }
        DEBUG_MESSAGE("Unexpected error locking VH persist file: %s\n", strerror(err));
        if (report)
            ls_stderr("Namespace error locking VH persist file: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    s_persist_vh_locked_write = lock_type != NSPERSIST_LOCK_READ;
    DEBUG_MESSAGE("Locked %s\n", s_persist_vh_locked_write ? "WRITE" : "READ");
    return 0;
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
    if (s_persist_vh_fd == -1 && errno == ENOENT)
    {
        DEBUG_MESSAGE("Try creating directories above\n");
        if (!mkdir(PERSIST_PREFIX, S_IRWXU) || errno == EEXIST)
        {
            char dir[PERSIST_DIR_SIZE];
            persist_namespace_dir(uid, dir, sizeof(dir));
            if (!mkdir(dir, S_IRWXU) || errno == EEXIST)
            {
                int try_open = 0;
                if (s_persisted_VH)
                {
                    char dir_vh[PERSIST_FILE_SIZE];
                    snprintf(dir_vh, sizeof(dir_vh), "%s/%s", dir, s_persisted_VH);
                    if (!mkdir(dir_vh, S_IRWXU))
                        try_open = 1;
                    else
                    {
                        DEBUG_MESSAGE("Error creating directory: %s: %s\n", 
                                      dir_vh, strerror(errno));
                        try_open = 1;
                    }
                }
                else
                    try_open = 1;
                if (try_open)
                {
                    s_persist_vh_fd = open(filename, O_RDWR | O_CREAT, 
                                           S_IRUSR | S_IWUSR);
                    err = errno;
                }
            }
            else
            {
                DEBUG_MESSAGE("Error creating directory %s: %s\n", dir, 
                              strerror(errno));
            }
        }
        else
        {
            DEBUG_MESSAGE("Error creating directory: %s: %s\n", PERSIST_PREFIX,
                          strerror(errno));
        }
    }
    if (s_persist_vh_fd == -1)
    {
        ls_stderr("Namespace error opening lock file %s: %s\n",
                  filename, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    //fcntl(s_persist_vh_fd, F_SETFD, FD_CLOEXEC); MUST LEAVE THIS OPEN FOR PERSISTENCE!
    /* Use a Posix record lock and make sure that the parent is always the one
     * with the lock.  */
    s_persist_uid = uid;
    rc = lock_persist_vh_file(report, lock_type);
    if (rc)
        unlock_close_persist_vh_file(1);
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
int is_persisted_fn(int uid, int *persisted)
{
    int i, rc = 0;
    char dir[PERSIST_DIR_SIZE];
    persist_namespace_dir(uid, dir, sizeof(dir));
    DEBUG_MESSAGE("is_persisted uid: %d\n", uid);
    int original_lock_type = persist_vh_lock_type();
    if (original_lock_type == -1 && 
        open_persist_vh_file(1, uid, NSPERSIST_LOCK_TRY_WRITE))
        return DEFAULT_ERR_RC;
    mount_tab_t *mount_tab = parse_mount_tab(dir);
    if (!mount_tab)
        return DEFAULT_ERR_RC;
    char ns_filename[PERSIST_FILE_SIZE];
    int found = 0;
    get_ns_filename(uid, ns_filename, sizeof(ns_filename));
    for (i = 0; mount_tab[i].mountpoint != NULL; i++)
    {
        if (!strcmp(ns_filename, mount_tab[i].mountpoint))
        {
            found = 1;
            break;
        }
    }
    free_mount_tab(mount_tab);
    if (!found)
    {
        DEBUG_MESSAGE("NOT persisted ns: %s\n", ns_filename);
        if (s_persisted_VH)
        {
            DEBUG_MESSAGE("   (VH NOT persisted)\n");
        }
        if (persisted)
            *persisted = 0;
        int now_persisted = 0;
        unpersist_fn(0, uid, 0, original_lock_type == -1 ? &now_persisted : NULL);
        if (now_persisted)
            found = 1;
        else
        {
            DEBUG_MESSAGE("NOT persisted user\n");
            return 0;
        }
    }
    if (persisted)
        *persisted = 1;
    if (original_lock_type == 1)        
        return 0;
    int have_user_unpersist = 0;
    user_unpersist(uid, &have_user_unpersist);
    if (have_user_unpersist)
    {
        DEBUG_MESSAGE("have_user_unpersist\n");
        rc = unpersist_fn(0, uid, 0, NULL);
        if (persisted)
            *persisted = 0;
        return rc;
    }
    if (original_lock_type == -1 && persist_vh_lock_type() == 1)
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

    //if ( !uid || !gid )  //do not allow root
    //{
    //    return -1;
    //}
    struct passwd *pw = getpwuid(uid);
    if (!pw)
        DEBUG_MESSAGE("Can't get pwuid for %d: %s\n", uid, strerror(errno));
    
    rv = setgid(gid);
    if (rv == -1)
    {
        int err = errno;
        ls_stderr("Namespace error lscgid: setgid(%d): %s\n", gid, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    if (pw && (pw->pw_gid == gid))
    {
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
    rv = setuid(uid);
    if (rv == -1)
    {
        int err = errno;
        ls_stderr("Namespace error lscgid: setuid(%d): %s", uid, strerror(err));
        return nsopts_rc_from_errno(err);
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
        ls_stderr("Namespace lscgid (): failed to change stderr log to: %s: %s\n",
                  path, strerror(errno));
        return -1;
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
        if (!rc && pCGI->m_stderrPath)
            rc = changeStderrLog(pCGI);

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


static int getvhost(FILE *mountfile, char *dir, int dirlen, int *did_novhost, 
                    int *uid_found, char *vhost)
{
    if (!mountfile)
    {
        mountfile = fopen("/proc/self/mountinfo", "r");
        if (!mountfile)
        {
            ls_stderr("Unable to open /proc/self/mountinfo: %s\n", strerror(errno));
            return -1;
        }
    }
    char line[4096];
    while (fgets(line, sizeof(line), mountfile)) 
    {
        int parm_no = 0;
        char *parm = line, *next_parm = line;
        while ((next_parm = strchr(parm + 1, ' ')) && parm_no < 4)
        {
            parm_no++;
            parm = next_parm;
        }
        if (!next_parm) 
        {
            DEBUG_MESSAGE("Not a correctly formatted string: %s", line);
            continue;
        }
        parm++; // Skip past space
        int parmlen = next_parm - parm;
        *next_parm = 0;
        if (parmlen < dirlen || memcmp(parm, dir, dirlen))
        {
            DEBUG_MESSAGE("Skip mountpoint %s != %s (%d bytes, test: %d, memcmp: %d)\n", 
                          parm, dir, parmlen, dirlen, memcmp(parm, dir, dirlen));
            continue;
        }
        if (!parm[dirlen])
        {
            DEBUG_MESSAGE("Unmount uid\n");
            *uid_found = 1;
            continue;
        }
        if (parm[dirlen] != '/')
        {
            DEBUG_MESSAGE("Bad UID %s != %s\n", parm, dir);                
            continue;
        }
        char *vhost_pos = &parm[dirlen+1], *endvhost;
        if (!(endvhost = strchr(vhost_pos, '/')))
        {
            if (*did_novhost)
            {
                DEBUG_MESSAGE("Already released no_vh\n");
                continue;
            }
            DEBUG_MESSAGE("vhost = no_vh\n");
            vhost[0] = 0;
            *did_novhost = 1;
            return 1;
        }
        int vhost_len = endvhost - vhost_pos;
        memcpy(vhost, vhost_pos, vhost_len);
        vhost[vhost_len] = 0;
        DEBUG_MESSAGE("vhost = %s\n", vhost);
        return 1;
    }
    fclose(mountfile);
    return 0;
}


static int unmount_vhost(int report, int uid, char *vhost, char *dir, int dirlen)
{
    DEBUG_MESSAGE("unmount_vhost, uid: %d, vhost: %s\n", uid, vhost);
    if (vhost && vhost != s_persisted_VH)
        nspersist_setvhost(vhost);
    if (open_persist_vh_file(report, uid, NSPERSIST_LOCK_WRITE_NB))
    {
        DEBUG_MESSAGE("Can't lock\n");
        return -1;
    }
    if (vhost)
    {
        unpersist_fn(report, uid, 1, NULL);
        unlock_close_persist_vh_file(1);
    } 
    else
    {
        unlock_close_persist_vh_file(1);
        int rc = umount(dir);
        int err = errno;
        DEBUG_MESSAGE("umount %s rc: %d, errno: %d\n", dir, rc, err);
        if (rc)
        {
            if (report)
                ls_stderr("Namespaces unable to unmount %s: %s\n", dir, 
                          strerror(err));
            return rc;
        }
    }
    return 0;
}


int unpersist_uid(int report, uid_t uid, int all_vhosts)
{
    int rc = 0;
    char dir[PERSIST_DIR_SIZE];
    DEBUG_MESSAGE("unpersist_uid: %u\n", uid);
    persist_namespace_dir(uid, dir, sizeof(dir));
    int dirlen = strlen(dir), did_novhost = 0;
    int uid_found = 0;
    if (all_vhosts)
    {
        char vhost[PERSIST_DIR_SIZE];
        FILE *mountfile = NULL;
        while ((rc = getvhost(mountfile, dir, dirlen, &did_novhost, 
                              &uid_found, vhost)) > 0)
            if ((rc = unmount_vhost(report, uid, vhost, dir, dirlen)))
                break;
    }
    else
    {
        uid_found = 1;
        if (s_persisted_VH)
            unmount_vhost(report, uid, s_persisted_VH, dir, dirlen);
    }
    if (!rc && uid_found)
        rc = unmount_vhost(report, uid, NULL, dir, dirlen);
    s_persist_uid = (uid_t)-1;
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


int nspersist_done(int report, int unpersist)
{
    if (unpersist && s_persist_uid != (uid_t)-1)
        return unpersist_uid(report, s_persist_uid, 0);
    return 0;
}


int unpersist_all()
{
    DIR *dir;
    struct dirent *ent;
    int rc = 0;
    
    dir = opendir(PERSIST_PREFIX);
    if (!dir)
    {
        int err = errno;
        ls_stderr("Namespaces unable to open %s: %s\n", PERSIST_PREFIX, 
                  strerror(err));
        return -1;
    }
    while (!rc && (ent = readdir(dir)))
    {
        if (ent->d_type == DT_DIR && 
            strspn(ent->d_name, "0123456789") == strlen(ent->d_name))
            rc = unpersist_uid(0, atoi(ent->d_name), 1);
    }
    closedir(dir);
    return rc;
}

#endif // Linux only