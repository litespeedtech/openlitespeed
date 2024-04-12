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
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/capability.h>
#include <sys/fsuid.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "ns.h"
#include "nsopts.h"
#include "nspersist.h"
#include "nsipc.h"
#include "nsutils.h"
#include "lscgid.h"
#include "use_bwrap.h"

#define CAP_TO_MASK_0(x) (1L << ((x) & 31))
#define CAP_TO_MASK_1(x) CAP_TO_MASK_0(x - 32)

#define N_ELEMENTS(arr) (sizeof (arr) / sizeof ((arr)[0]))

NsInfo s_ns_infos[NS_TYPE_COUNT + 1] = {
#ifdef CLONE_NEWCGROUP
  {"cgroup", 1, 1, CLONE_NEWCGROUP},
#endif
  {"ipc",    1, 1, CLONE_NEWIPC},
  {"uts",    1, 1, CLONE_NEWUTS},
  {"net",    0, 1, CLONE_NEWNET},
  {"pid",    1, 1, CLONE_NEWPID}, 
  {"mnt",    1, 1, CLONE_NEWNS},
  {"user",   0, 1, CLONE_NEWUSER},
  {NULL,     0, 0, 0}
};


#define REQUIRED_CAPS_0 (CAP_TO_MASK_0 (CAP_SYS_ADMIN) | \
                         CAP_TO_MASK_0 (CAP_SYS_CHROOT) | \
                         CAP_TO_MASK_0 (CAP_NET_ADMIN) | \
                         CAP_TO_MASK_0 (CAP_SETUID) | \
                         CAP_TO_MASK_0 (CAP_SETGID) | \
                         CAP_TO_MASK_0 (CAP_SYS_PTRACE))
/* high 32bit caps needed */
#define REQUIRED_CAPS_1 0

#ifndef BLOCK_SIZE
#define BLOCK_SIZE  4096
#endif

int         s_ns_debug = 0;
FILE       *s_ns_debug_file = NULL;
int         s_ns_supported_checked = 0;
int         s_ns_supported = 0;
int         s_didinit = 0;
int         s_persisted = 0;
char       *s_persisted_VH = NULL;
uid_t       s_overflowuid;
gid_t       s_overflowgid;
int         s_proc_fd = -1;
int         s_clone_flags;
uint32_t    s_requested_caps[2] = {0, 0};
const char *s_host_tty_dev = NULL;
verbose_callback_t s_verbose_callback = NULL;
SetupOp     s_setupOp_all[] =
{
    { SETUP_BIND_TMP, NULL, "/tmp", OP_FLAG_BWRAP_SYMBOL, -1 },
    // Does not need a last as it's size is important
};

SetupOp     s_SetupOp_default[] = 
{
    { SETUP_RO_BIND_MOUNT, "/usr", "/usr", 0, -1 },
    { SETUP_RO_BIND_MOUNT, "/lib", "/lib", 0, -1 },
    { SETUP_RO_BIND_MOUNT, "/lib64", "/lib64", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/bin", "/bin", 0, -1 },
    { SETUP_RO_BIND_MOUNT, "/sbin", "/sbin", 0, -1 },
    { SETUP_MAKE_DIR, NULL, "/var", 0, -1 },
    { SETUP_RO_BIND_MOUNT, "/var/www", "/var/www", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_MOUNT_PROC, NULL, "/proc", 0, -1 },
    { SETUP_MAKE_SYMLINK, "../tmp", "var/tmp", 0, -1 },
    { SETUP_MOUNT_DEV, NULL, "/dev", 0, -1 },
    { SETUP_RO_BIND_MOUNT, "/etc/localtime", "/etc/localtime",  OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/etc/ld.so.cache", "/etc/ld.so.cache", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/etc/resolv.conf", "/etc/resolv.conf", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/etc/ssl", "/etc/ssl", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/etc/pki", "/etc/pki", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/etc/man_db.conf", "/etc/man_db.conf", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/usr/local/bin/msmtp", "/etc/alternatives/mta", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/usr/local/bin/msmtp", "/usr/sbin/exim", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_BIND_MOUNT, BWRAP_VAR_HOMEDIR, BWRAP_VAR_HOMEDIR, OP_FLAG_ALLOW_NOTEXIST | OP_FLAG_BWRAP_SYMBOL, -1 },
    { SETUP_BIND_MOUNT, "/var/lib/mysql/mysql.sock", "/var/lib/mysql/mysql.sock", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_BIND_MOUNT, "/home/mysql/mysql.sock", "/home/mysql/mysql.sock", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_BIND_MOUNT, "/tmp/mysql.sock", "/tmp/mysql.sock", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_BIND_MOUNT, "/run/mysqld/mysql.sock", "/run/mysqld/mysqld.sock", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_BIND_MOUNT, "/var/run/mysqld/mysqld.sock", "/var/run/mysqld/mysqld.sock", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_MAKE_DIR, NULL, "/run/user/" BWRAP_VAR_UID, OP_FLAG_BWRAP_SYMBOL, -1 },
    { SETUP_MAKE_PASSWD, NULL, BWRAP_VAR_PASSWD, OP_FLAG_BWRAP_SYMBOL | OP_FLAG_NO_CREATE_DEST, -1 },
    { SETUP_MAKE_GROUP, NULL, BWRAP_VAR_GROUP, OP_FLAG_BWRAP_SYMBOL | OP_FLAG_NO_CREATE_DEST, -1 },
    { SETUP_COPY, "/etc/exim.jail/$USER.conf", "$HOMEDIR/.msmtprc", OP_FLAG_ALLOW_NOTEXIST | OP_FLAG_BWRAP_SYMBOL, -1 },
    { SETUP_RO_BIND_MOUNT, "/etc/php.ini", "/etc/php.ini", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/etc/php-fpm.conf", "/etc/php-fpm.conf", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/etc/php-fpm.d", "/etc/php-fpm.d", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/var/run", "/var/run", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/var/lib", "/var/lib", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/etc/imunify360/user_config/", "/etc/imunify360/user_config/", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/etc/sysconfig/imunify360/", "/etc/sysconfig/imunify360/", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_RO_BIND_MOUNT, "/opt/plesk/php", "/opt/plesk/php", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_BIND_MOUNT, "/opt/alt", "/opt/alt", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_BIND_MOUNT, "/opt/psa", "/opt/psa", OP_FLAG_ALLOW_NOTEXIST, -1 },
    { SETUP_BIND_MOUNT, "/var/lib/php/sessions", "/var/lib/php/sessions", OP_FLAG_ALLOW_NOTEXIST, -1},
    { SETUP_NOOP, NULL, NULL, OP_FLAG_LAST, -1 }
};
SetupOp    *s_SetupOp = NULL;
size_t      s_SetupOp_size = 0;
int         s_setupOp_allocated = 0;
char       *s_ns_conf = NULL;
char       *s_ns_conf2 = NULL;
mount_flags_data_t s_mount_flags_data[] = 
{
    { 0, "rw" },
    { MS_RDONLY, "ro" },
    { MS_NOSUID, "nosuid" },
    { MS_NODEV, "nodev" },
    { MS_NOEXEC, "noexec" },
    { MS_NOATIME, "noatime" },
    { MS_NODIRATIME, "nodiratime" },
    { MS_RELATIME, "relatime" },
    { 0, NULL }
};

enum 
{
    PRIV_SEP_OP_DONE,
    PRIV_SEP_OP_BIND_MOUNT,
    PRIV_SEP_OP_PROC_MOUNT,
    PRIV_SEP_OP_TMPFS_MOUNT,
    PRIV_SEP_OP_DEVPTS_MOUNT,
    PRIV_SEP_OP_MQUEUE_MOUNT,
    PRIV_SEP_OP_REMOUNT_RO_NO_RECURSIVE,
    PRIV_SEP_OP_SET_HOSTNAME,
    PRIV_SEP_OP_COPY,
};

typedef struct
{
    uint32_t op;
    uint32_t flags;
    uint32_t arg1_offset;
    uint32_t arg2_offset;
} PrivSepOp;


typedef enum {
  BIND_READONLY = (1 << 0),
  BIND_DEVICES = (1 << 2),
  BIND_RECURSIVE = (1 << 3),
} bind_option_t;

static int read_strfile(const char *filename, int *found, size_t str_len, 
                        char *str)
{
    int fd = open(filename, O_RDONLY);
    ssize_t len;
    if (fd == -1)
    {
        int err = errno;
        if (found && !*found && err == ENOENT)
            return 0;
        ls_stderr("Namespace error opening %s: %s\n", filename, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    if (found)
        *found = 1;
    len = read(fd, str, str_len - 1);
    if (len < 0)
    {
        int err = errno;
        ls_stderr("Namespace error reading %s: %s\n", filename, strerror(err));
        close(fd);
        return nsopts_rc_from_errno(err);
    }
    if (len > 0 && str[len - 1] == '\n')
        --len;
    str[len] = 0;
    close(fd);
    return 0;
}


int ns_supported()
{
    if (s_ns_supported_checked)
        return s_ns_supported;
    
    s_ns_supported_checked = 1;
    struct utsname name;
    int major, minor, count;
    if (uname(&name))
    {
        DEBUG_MESSAGE("ERROR: ns_supported can't get uname: %s\n", strerror(errno));
        //ls_stderr("Namespace can't determine if cgroups are supported: %s\n",
        //          strerror(err));
        return 0;
    }
    if ((count = sscanf(name.release,"%u.%u", &major, &minor)) != 2)
    {
        int err = errno;
        if (err >= 0)
        {
            DEBUG_MESSAGE("ERROR: ns_supported can't pull out version\n");
        //    ls_stderr("Namespace can't get version for cgroup support %d\n",
        //              count);
            return 0;
        }
        DEBUG_MESSAGE("ERROR: ns_supported can't get Linux version: %s\n", 
                      strerror(err));
        //ls_stderr("Namespace can't get Linux version: %s for cgroup support\n",
        //          strerror(err));
        return 0;
    }
    // See the unshare man page for the levels
    if (major < 3)
    {
        DEBUG_MESSAGE("NOT ns_supported for old OS: %d.%d\n", major, minor);
        s_ns_supported = 0;
        return 0;
    }
    struct stat sbuf;
    int unshare_user = 1;
    
    if (stat ("/proc/self/ns/user", &sbuf) == 0)
    {
        char str_num[STRNUM_SIZE];
        
        /* RHEL7 has a kernel module parameter that lets you enable user namespaces */
        if (stat ("/sys/module/user_namespace/parameters/enable", &sbuf) == 0)
        {
            int found = 0;
            if (read_strfile("/sys/module/user_namespace/parameters/enable", 
                             &found, sizeof(str_num), str_num) == 0)
                if (found && str_num[0] == 'N')
                    unshare_user = 0;
        }

        /* Check for max_user_namespaces */
        if (stat ("/proc/sys/user/max_user_namespaces", &sbuf) == 0)
        {
            int found = 0;
            if (read_strfile("/proc/sys/user/max_user_namespaces", &found,
                             sizeof(str_num), str_num) == 0)
                if (found && str_num[0] == '0' && str_num[1] == 0)
                    unshare_user = 0;
        }

    }
    if (!unshare_user)
        nspersist_disable_user_namespace();
    
    s_ns_supported = 1;
    if (major < 4 || (major == 4 && minor < 6))
    {
        DEBUG_MESSAGE("CGroups NOT supported, v%d.%d\n", major, minor);
        s_ns_infos[NS_TYPE_CGROUP].enabled = 0;
    }
    return 1;
}


int get_caps()
{
    /* If our uid is 0, default to inheriting all caps; the caller
     * can drop them via --cap-drop.  This is used by at least rpm-ostree.
     * Note this needs to happen before the argument parsing of --cap-drop.
     */
    struct __user_cap_header_struct hdr = { _LINUX_CAPABILITY_VERSION_3, 0 };
    struct __user_cap_data_struct data[2] = { { 0 } };

    if (capget (&hdr, data) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error getting capabilities: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    s_requested_caps[0] = data[0].effective;
    s_requested_caps[1] = data[1].effective;
    DEBUG_MESSAGE("s_requested_caps: 0x%x, 0x%x\n", s_requested_caps[0], 
                  s_requested_caps[1]);
    return 0;
}


static int read_overflowids (void)
{
    int rc;
    char str_num[STRNUM_SIZE];
    rc = read_strfile("/proc/sys/kernel/overflowuid", NULL, sizeof(str_num), 
                      str_num);
    if (rc)
        return rc;
    s_overflowuid = atoi(str_num);
    if (s_overflowuid == 0)
    {
        ls_stderr("Unexpected overflowuid in: %s\n", str_num);
        return DEFAULT_ERR_RC;
    }
    rc = read_strfile("/proc/sys/kernel/overflowgid", NULL, sizeof(str_num),
                      str_num);
    if (rc)
        return rc;
    s_overflowgid = atoi(str_num);
    if (s_overflowgid == 0)
    {
        ls_stderr("Unexpected overflowgid in: %s\n", str_num);
        return DEFAULT_ERR_RC;
    }
    return 0;
}


static int check_cgroup_ns()
{
    struct stat sbuf;
    
    if (!s_ns_infos[NS_TYPE_CGROUP].enabled ||
        stat ("/proc/self/ns/cgroup", &sbuf))
        s_ns_infos[NS_TYPE_CGROUP].enabled = 0;
    
    DEBUG_MESSAGE("unshare cgroup: %d\n", s_ns_infos[NS_TYPE_CGROUP].enabled);
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


static void free_setupOp(SetupOp *setupOp, int parent_write[], int is_error)
{
    if (setupOp)
    {
        DEBUG_MESSAGE("Freeing setupOp in free_setupOp\n");
        nsopts_free(&setupOp);
    }
    nsopts_free_conf(&s_ns_conf, &s_ns_conf2);
    close_pipe(parent_write);
}


static int preclone_build_user_SetupOp(lscgid_t *pCGI, SetupOp **psetupOp)
{
    DEBUG_MESSAGE("preclone_build_user_SetupOp, s_SetupOp_size: %ld\n", s_SetupOp_size);
    s_SetupOp_size += sizeof(s_setupOp_all);
    *psetupOp = malloc(s_SetupOp_size);
    if (!*psetupOp)
    {
        int err = errno;
        ls_stderr("Namespace insufficient memory allocating setupOp\n");
        return nsopts_rc_from_errno(err);
    }
    memcpy(*psetupOp, s_setupOp_all, sizeof(s_setupOp_all));
    memcpy(((char *)*psetupOp) + sizeof(s_setupOp_all), s_SetupOp, 
           s_SetupOp_size - sizeof(s_setupOp_all));
    /* We need to do a deep copy so that the frees work out */
    SetupOp *setupOp, *op;
    struct stat st;
    setupOp = *psetupOp;
    int rc = 0;
    for (op = &setupOp[0]; !(op->flags & OP_FLAG_LAST); ++op)
    {
        if (rc)
        {
            // All of this is to avoid a leak if there's an error
            op->type = SETUP_NOOP;
            op->dest = NULL;
            op->source = NULL;
            op->flags &= ~(OP_FLAG_ALLOCATED_DEST | OP_FLAG_ALLOCATED_SOURCE);
            continue;
        }
        if (!(op->flags & OP_FLAG_BWRAP_SYMBOL) &&
            (op->type == SETUP_BIND_MOUNT ||
             op->type == SETUP_RO_BIND_MOUNT ||
             op->type == SETUP_DEV_BIND_MOUNT ||
             op->type == SETUP_MOUNT_PROC ||
             op->type == SETUP_MOUNT_DEV ||
             op->type == SETUP_MOUNT_TMPFS ||
             op->type == SETUP_MOUNT_MQUEUE) &&
             op->source && stat(op->source, &st))
        {
            int err = errno;
            if (err == ENOENT && (op->flags & OP_FLAG_ALLOW_NOTEXIST))
            {
                DEBUG_MESSAGE("Getting rid of mount %s on %s\n", op->source, op->dest);
                op->type = SETUP_NOOP;
                op->dest = NULL;
                op->source = NULL;
                op->flags &= ~(OP_FLAG_ALLOCATED_DEST | OP_FLAG_ALLOCATED_SOURCE);
            }
            else
            {
                rc = nsopts_rc_from_errno(err);
                ls_stderr("Namespace Error accessing required entity: %s: %s\n", 
                          op->source, strerror(err));
                op->type = SETUP_NOOP;
                op->dest = NULL;
                op->source = NULL;
                op->flags &= ~(OP_FLAG_ALLOCATED_DEST | OP_FLAG_ALLOCATED_SOURCE);
                continue; // Keep going to avoid leaks in this structure
            }
        }
        if (op->source)
        {
            op->source = strdup(op->source);
            if (!op->source)
            {
                int err = errno;
                ls_stderr("Namespace error on deep copy of source: %s\n", strerror(err));
                rc = nsopts_rc_from_errno(err);
                op->flags &= ~OP_FLAG_ALLOCATED_SOURCE;
            }
            else
                op->flags |= OP_FLAG_ALLOCATED_SOURCE;
        }
        if (op->dest)
        {
            op->dest = strdup(op->dest);
            if (!op->dest)
            {
                int err = errno;
                ls_stderr("Namespace error on deep copy of dest: %s\n", strerror(err));
                rc = nsopts_rc_from_errno(err);
                op->flags &= ~OP_FLAG_ALLOCATED_DEST;
            }
            else
                op->flags |= OP_FLAG_ALLOCATED_DEST;
        }
        /* Keep going on error so that the structure is either deep copied or not */
    }
        
    return rc;
}


static int handle_die_with_parent (void)
{
    // Child only
    if (prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0) != 0)
    {
        int err = errno;
        ls_stderr("Namespace error setting death signal: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    return 0;
}


static int drop_all_caps(int keep_requested_caps)
{
    struct __user_cap_header_struct hdr = { _LINUX_CAPABILITY_VERSION_3, 0 };
    struct __user_cap_data_struct data[2] = { { 0 } };

    DEBUG_MESSAGE("drop_all_caps\n");
    if (keep_requested_caps)
    {
        /* Avoid calling capset() unless we need to; currently
         * systemd-nspawn at least is known to install a seccomp
         * policy denying capset() for dubious reasons.
         * <https://github.com/projectatomic/bubblewrap/pull/122>
         */
        /*if (!opt_cap_add_or_drop_used && real_uid == 0) */
        {
            assert (!IS_PRIVILEGED);
            return 0;
        }
        /*
        data[0].effective = s_requested_caps[0];
        data[0].permitted = s_requested_caps[0];
        data[0].inheritable = s_requested_caps[0];
        data[1].effective = s_requested_caps[1];
        data[1].permitted = s_requested_caps[1];
        data[1].inheritable = s_requested_caps[1];
        */
    }
    if (capset (&hdr, data) < 0)
    {
        int err = errno;
        /* While the above logic ensures we don't call capset() for the primary
         * process unless configured to do so, we still try to drop privileges for
         * the init process unconditionally. Since due to the systemd seccomp
         * filter that will fail, let's just ignore it.
         */
        if (err == EPERM /*&& real_uid == 0 */&& !IS_PRIVILEGED)
            return 0;
        ls_stderr("Namespace error in drop_all_caps: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    return 0;
}


static int drop_privs (uid_t uid, int keep_requested_caps, 
                       int already_changed_uid, int persisted)
{
    int rc;
    
    if (persisted)
        return 0;
    DEBUG_MESSAGE("drop_privs\n");
    /* Drop root uid */
    if (!already_changed_uid)
        DEBUG_MESSAGE("drop_privs, setuid from %d to %d\n", getuid(), uid);
    if (!already_changed_uid && setuid (uid) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error switch to uid: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }

    rc = drop_all_caps(keep_requested_caps);
    if (rc)
        return rc;

    /* We don't have any privs now, so mark us dumpable which makes /proc/self 
     * be owned by the user instead of root */
    if (prctl (PR_SET_DUMPABLE, 1, 0, 0, 0) != 0)
    {
        int err = errno;
        ls_stderr("Namespace error setting dumpable: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    return 0;
}


static int ns_parent(lscgid_t *pCGI, int persisted, int *done, pid_t pid, 
                     int parent_write)
{
    int rc;
    nsipc_parent_done_t parent_done;

    DEBUG_MESSAGE("ns_parent\n");
    
    if (IS_PRIVILEGED && !persisted && s_ns_infos[NS_TYPE_USER].enabled)
    {
        rc = nsutils_write_uid_gid_map(pCGI->m_data.m_uid, getuid(),
                                       pCGI->m_data.m_gid, getgid(), 
                                       pid, 1, 1, 0);
        if (rc)
            return rc;
    }
    
    /*
    rc = drop_privs(pCGI->m_data.m_uid, 0, 0, persisted);
    if (rc)
        return rc;
    rc = handle_die_with_parent();
    if (rc)
        return rc;
    */
    /* Let child run now that the uid maps are set up */
    parent_done.m_type = NSIPC_PARENT_DONE;
    rc = write(parent_write, &parent_done, sizeof(parent_done));
    close(parent_write);
    if (rc <= 0)
    {
        int err = errno;
        ls_stderr("Namespace error releasing child, may have died earlier: %s\n", 
                  strerror(err));
        return nsopts_rc_from_errno(err);
    }
    if (!persisted)
    {
        DEBUG_MESSAGE("ns_parent terminate normally\n");
        persist_exit_child("ns_parent", 0);
    }
    return 0;
}


static int prctl_caps(uint32_t *caps, int do_cap_bounding, int do_set_ambient)
{
    unsigned long cap;

    /* We ignore both EINVAL and EPERM, as we are actually relying
     * on PR_SET_NO_NEW_PRIVS to ensure the right capabilities are
     * available.  EPERM in particular can happen with old, buggy
     * kernels.  See:
     *  https://github.com/projectatomic/bubblewrap/pull/175#issuecomment-278051373
     *  https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/security/commoncap.c?id=160da84dbb39443fdade7151bc63a88f8e953077
     */
    for (cap = 0; cap <= CAP_LAST_CAP; cap++)
    {
        int keep = 0;
        if (cap < 32)
        {
            if (CAP_TO_MASK_0 (cap) & caps[0])
                keep = 1;
        }
        else
        {
            if (CAP_TO_MASK_1 (cap) & caps[1])
                keep = 1;
        }

        if (keep && do_set_ambient)
        {
#ifdef PR_CAP_AMBIENT
            int res = prctl (PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, cap, 0, 0);
            if (res == -1 && !(errno == EINVAL || errno == EPERM))
            {
                int err = errno;
                ls_stderr("Namespace error setting ambient capability %ld: %s\n", 
                          cap, strerror(err));
                return nsopts_rc_from_errno(err);
            }
#else
            /* We ignore the EINVAL that results from not having PR_CAP_AMBIENT
             * in the current kernel at runtime, so also ignore not having it
             * in the current kernel headers at compile-time */
#endif
        }

        if (!keep && do_cap_bounding)
        {
            DEBUG_MESSAGE("Dropping cap: %ld\n", cap);
            int res = prctl (PR_CAPBSET_DROP, cap, 0, 0, 0);
            if (res == -1 && !(errno == EINVAL || errno == EPERM))
            {
                int err = errno;
                ls_stderr("Namespace error dropping capability %ld from bounds: %s\n",
                          cap, strerror(err));
                return nsopts_rc_from_errno(err);
            }
        }
    }
    return 0;
}


static int set_ambient_capabilities(void)
{
    if (IS_PRIVILEGED)
        return 0;
    return prctl_caps (s_requested_caps, 0, 1);
}

static int drop_cap_bounding_set (int drop_all)
{
    int rc;
    if (!drop_all)
        rc = prctl_caps (s_requested_caps, 1, 0);
    else
    {
        uint32_t no_caps[2] = {0, 0};
        rc = prctl_caps (no_caps, 1, 0);
    }
    return rc;
}


static int set_required_caps (void)
{
    struct __user_cap_header_struct hdr = { _LINUX_CAPABILITY_VERSION_3, 0 };
    struct __user_cap_data_struct data[2] = { { 0 } };

    /* Drop all non-require capabilities */
    data[0].effective = REQUIRED_CAPS_0;
    data[0].permitted = REQUIRED_CAPS_0;
    data[0].inheritable = 0;
    data[1].effective = REQUIRED_CAPS_1;
    data[1].permitted = REQUIRED_CAPS_1;
    data[1].inheritable = 0;
    DEBUG_MESSAGE("set_required_caps\n");
    if (capset (&hdr, data) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error in capset for required caps: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    return 0;
}


static int pivot_root(const char * new_root, const char * put_old)
{
#ifdef __NR_pivot_root
    return syscall (__NR_pivot_root, new_root, put_old);
#else
    errno = ENOSYS;
    return -1;
#endif
}


static int switch_to_user_with_privs(lscgid_t *pCGI, int persisted)
{
    int rc = 0;
    /* If we're in a new user namespace, we got back the bounding set, clear it 
     * again */
    DEBUG_MESSAGE("switch_to_user_with_privs chroot: %s, user enabled: %d, persisted %d, user vh: %d\n", 
                  pCGI->m_pChroot, s_ns_infos[NS_TYPE_USER].enabled, persisted, s_ns_infos[NS_TYPE_USER].vh);
    if (s_ns_infos[NS_TYPE_USER].enabled && !persisted && 
        !s_ns_infos[NS_TYPE_USER].vh)
    {
        rc = drop_cap_bounding_set(0);
        if (rc)
            return rc;
    }

    if (!IS_PRIVILEGED)
        return rc;
    
    if (prctl (PR_SET_KEEPCAPS, 1, 0, 0, 0) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error keeping capabilities: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    
    struct passwd *pw = getpwuid(pCGI->m_data.m_uid);
    if (setgid(pCGI->m_data.m_gid) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error changing to gid: %d: %s\n", pCGI->m_data.m_gid,
                  strerror(err));
        return nsopts_rc_from_errno(err);
    }
    if (pw && pw->pw_gid == pCGI->m_data.m_gid && 
        initgroups(pw->pw_name, pCGI->m_data.m_gid) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error in initgroups: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    if (setgroups(1, &pCGI->m_data.m_gid) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error in setgroups: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    DEBUG_MESSAGE("About to chroot(%s)\n", pCGI->m_pChroot);
    if (pCGI->m_pChroot && chroot(pCGI->m_pChroot) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error in chroot to %s: %s\n", pCGI->m_pChroot, 
                  strerror(err));
        return nsopts_rc_from_errno(err);
    }
    
    DEBUG_MESSAGE("Doing setuid to %d\n", pCGI->m_data.m_uid);
    if (setuid(pCGI->m_data.m_uid) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error changing to uid: %d: %s\n", pCGI->m_data.m_uid, 
                  strerror(err));
        return nsopts_rc_from_errno(err);
    }

    /* Tell kernel not clear capabilities when later dropping root uid */
    if (prctl (PR_SET_KEEPCAPS, 1, 0, 0, 0) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error setting keep capabilities: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }

    /* Regain effective required capabilities from permitted */
    rc = set_required_caps ();
    return rc;
}


static int build_mount_namespace(lscgid_t *pCGI)
{
    //const char *base_path = "/tmp";
    char *base_path;
    
    /* Instead of a tmpfs, use a real disk that won't kill memory.  Begin:*/
    char root_dir[PERSIST_DIR_SIZE];
    char root[PERSIST_DIR_SIZE * 2];
    snprintf(root, sizeof(root), "%s/"PERSIST_NO_VH_PREFIX"root", 
             persist_namespace_dir_vh(pCGI->m_data.m_uid, NS_TYPE_MNT, root_dir, 
                                      sizeof(root_dir)));
    /* Mark everything as slave, so that we still
     * receive mounts from the real root, but don't
     * propagate mounts to the real root. */
    DEBUG_MESSAGE("build_mount_namespace, root: %s\n", root);
    if (mount (NULL, "/", NULL, MS_SILENT | MS_SLAVE | MS_REC, NULL) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error making / slave: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }

    ///* Create a tmpfs which we will use as / in the namespace */
    //if (mount ("tmpfs", base_path, "tmpfs", MS_NODEV | MS_NOSUID, NULL) != 0)
    //{
    //    int err = errno;
    //    ls_stderr("Namespace error mounting tmpfs: %s\n", strerror(err));
    //    return nsopts_rc_from_errno(err);
    //}

    if (mkdir(root, 0777))
    {
        if (errno != EEXIST)
        {
            int err = errno;
            ls_stderr("Namespace error creating rootfs %s: %s\n", root, strerror(err));
            return nsopts_rc_from_errno(err);
        }
    }
    
    base_path = root;
    if (mount(root, root, NULL, MS_SILENT | MS_MGC_VAL | MS_BIND | MS_REC, NULL))
    {
        int err = errno;
        ls_stderr("Namespace error mounting root: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    /* End real disk code (no tmpfs) */
    
    /* Chdir to the new root tmpfs mount. This will be the CWD during
       the entire setup. Access old or new root via "oldroot" and "newroot". */
    if (chdir (base_path) != 0)
    {
        int err = errno;
        ls_stderr("Namespace error changing to the base path: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }

    /* We create a subdir "$base_path/newroot" for the new root, that
     * way we can pivot_root to base_path, and put the old root at
     * "$base_path/oldroot". This avoids problems accessing the oldroot
     * dir if the user requested to bind mount something over / (or
     * over /tmp, now that we use that for base_path). */
    if (mkdir ("newroot", /*0755*/0777) && errno != EEXIST)
    {
        int err = errno;
        ls_stderr("Namespace error creating newroot failed: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }

    if (mount ("newroot", "newroot", NULL, 
               MS_SILENT | MS_MGC_VAL | MS_BIND | MS_REC, NULL) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error mounting new root failed: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }

    if (mkdir ("oldroot", /*0755*/0777) && errno != EEXIST)
    {
        int err = errno;
        ls_stderr("Namespace error creating oldroot failed: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    
    if (pivot_root (base_path, "oldroot"))
    {
        int err = errno;
        ls_stderr("Namespace error setting oldroot as pivot root: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    
    if (access("oldroot/etc", 0))
    {
        ls_stderr("Namespace pivot root is not working!\n");
        return DEFAULT_ERR_RC;
    }
    
    if (chdir ("/") != 0)
    {
        int err = errno;
        ls_stderr("Namespace error setting root as current dir: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    return 0;
}


static int bwrap_symbol_in_path(lscgid_t *pCGI, char *parm, char **alloc_path)
{
    char *symbol, *pos;
    char str_int[STRNUM_SIZE];
    char *new_text = str_int;
    *alloc_path = NULL;
    pos = strstr(parm, symbol = BWRAP_VAR_GID);
    if (pos)
        snprintf(new_text, STRNUM_SIZE, "%d", pCGI->m_data.m_gid);
    else 
    {
        pos = strstr(parm, symbol = BWRAP_VAR_UID);
        if (pos)
            snprintf(new_text, STRNUM_SIZE, "%d", pCGI->m_data.m_uid);
        else
        {
            int user = 1;
            if (!(pos = strstr(parm, symbol = BWRAP_VAR_USER)))
            {
                user = 0;
                pos = strstr(parm, symbol = BWRAP_VAR_HOMEDIR);
            }
            if (pos)
            {
                struct passwd *pwd;
                if (!(pwd = getpwuid(pCGI->m_data.m_uid)))
                {
                    int err = errno;
                    if (!err)
                    {
                        ls_stderr("Namespace error: passwd entry for euid %d: "
                                  "not found\n", pCGI->m_data.m_uid);
                        err = ENOENT;
                    }
                    else
                        ls_stderr("Namespace error geting passwd entry for euid"
                                  " %d: %s\n", pCGI->m_data.m_uid, 
                                  strerror(errno));
                    return nsopts_rc_from_errno(err);
                }
                if (user)
                    new_text = pwd->pw_name;
                else
                {
                    new_text = pwd->pw_dir;
                    if (new_text[0] == '/' && new_text[1] == 0)
                    {
                        DEBUG_MESSAGE("Do not allow root homedir\n");
                        pos = 0;
                    }
                }
            }
        }
    }
    if (pos)
    {
        if (!new_text)
        {
            ls_stderr("Namespace unexpected NULL replacement text for %s\n",
                      parm);
            return DEFAULT_ERR_RC;
        }
        int parm_len = strlen(parm);
        int malloc_len = parm_len + strlen(new_text);
        char *new_parm = malloc(malloc_len + 1);
        char *cur = new_parm;
        int remaining, symbol_len = strlen(symbol);
        
        if (!new_parm)
        {
            int err = errno;
            ls_stderr("Namespace error allocating op memory: %s\n", strerror(errno));
            return nsopts_rc_from_errno(err);
        }
        memcpy(cur, parm, pos - parm);
        cur += (pos - parm);
        memcpy(cur, new_text, strlen(new_text));
        cur += strlen(new_text);
        remaining = parm_len - ((pos - parm) + symbol_len);
        memcpy(cur, pos + symbol_len, remaining);
        cur += remaining;
        *cur = 0;
        *alloc_path = new_parm;
        DEBUG_MESSAGE("bwrap_symbol_in_path %s->%s\n", parm, new_parm);
    }
    return 0;
}


static int create_strnums(lscgid_t *pCGI, SetupOp *op, char *work_str, 
                          char **strnums)
{
    char *prefix = (op->type == SETUP_MAKE_GROUP) ? BWRAP_VAR_GROUP : BWRAP_VAR_PASSWD;
    uid_t prefix_num = (op->type == SETUP_MAKE_GROUP) ? pCGI->m_data.m_gid : 
                                                        pCGI->m_data.m_uid;
    int prefix_len = strlen(prefix);
    char *pos = work_str;
    int len;
    int strnums_pos = 0;
    DEBUG_MESSAGE("create_strnums: %s, work_str: %s\n", 
                  (op->type == SETUP_MAKE_GROUP) ? "/etc/group" : "/etc/passwd",
                  work_str);
    if (op->type == SETUP_MAKE_GROUP)
    {
        /* Because we don't do a conventional setgid(), we need to validate the
         * gid somewhere, how about here?  */
        DEBUG_MESSAGE("Test getgrgid of %d\n", pCGI->m_data.m_gid);
        if (!getgrgid(pCGI->m_data.m_gid))
        {
            int err = errno;
            if (!err)
            {
                ls_stderr("Namespace error: group entry for gid %d: not found\n",
                          pCGI->m_data.m_gid);
                err = ENOENT;
            }
            else
                ls_stderr("Namespace error geting passwd entry for euid %d: %s\n",
                          pCGI->m_data.m_uid, strerror(errno));
            return nsopts_rc_from_errno(err);
        }
    }
    while (*pos)
    {
        switch (*pos)
        {
            case '$':
                if (!strncmp(pos, prefix, prefix_len))
                {
                    char str_prefix_num[STRNUM_SIZE];
                    snprintf(str_prefix_num, sizeof(str_prefix_num), "%u", 
                             prefix_num);
                    int str_prefix_len = strlen(str_prefix_num);
                    *strnums = realloc(*strnums, strnums_pos + str_prefix_len + 2);
                    if (!*strnums)
                    {
                        int err = errno;
                        ls_stderr("Namespace can't allocate wildcard string number in %s: %s\n",
                                  prefix, strerror(err));
                        return nsopts_rc_from_errno(err);
                    }
                    DEBUG_MESSAGE("create_strnums, add my id: %s\n", str_prefix_num);
                    memcpy(&(*strnums)[strnums_pos], str_prefix_num, str_prefix_len);
                    strnums_pos += str_prefix_len;
                    (*strnums)[strnums_pos] = 0;
                    strnums_pos++;
                    (*strnums)[strnums_pos] = 0;
                    pos += prefix_len;
                    break;
                }
                ls_stderr("Namespace unexpected title in bwrap symbolic: %s\n", pos);
                free(*strnums);
                return DEFAULT_ERR_RC;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                len = strspn(pos, "0123456789");
                *strnums = realloc(*strnums, strnums_pos + len + 2);
                if (!*strnums)
                {
                    int err = errno;
                    ls_stderr("Namespace can't allocate string number in %s: %s\n",
                              prefix, strerror(err));
                    return nsopts_rc_from_errno(err);
                }
                DEBUG_MESSAGE("create_strnums, add specified id: %.*s\n", len,
                              pos);
                memcpy(&(*strnums)[strnums_pos], pos, len);
                strnums_pos += len;
                (*strnums)[strnums_pos] = 0;
                strnums_pos++;
                (*strnums)[strnums_pos] = 0;
                pos += len;
                break;
            case '\'':
            case '"':
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                ++pos; // Skip these
                break;
            default:
                ls_stderr("Namespace unexpected character in %s\n", work_str);
                free(*strnums);
                return DEFAULT_ERR_RC;
        }
    }
    pos = *strnums;
    while (*pos)
    {
        DEBUG_MESSAGE("create_strnums -> %s\n", pos);
        pos += (strlen(pos) + 1);
    }
    
    return 0;
}


static int extract_filedata(SetupOp *op, char *strnums)
{
    char *filename = (op->type == SETUP_MAKE_GROUP) ? "/etc/group":"/etc/passwd";
    FILE *fh = fopen(filename, "r");
    char line[256];    
    int op_dest_len = 0;
    
    if (op->flags & OP_FLAG_ALLOCATED_DEST)
        free(op->dest);
    op->dest = NULL;
    op->flags &= ~OP_FLAG_ALLOCATED_DEST;
    if (!fh)
    {
        int err = errno;
        ls_stderr("Namespace error opening %s: %s\n", filename, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    DEBUG_MESSAGE("extract_filedata: %s\n", filename);
    while (fgets(line, sizeof(line) - 1, fh))
    {
        char *colon1 = NULL, *colon2 = NULL, *colon3 = NULL;
        colon1 = strchr(line, ':');
        if (colon1)
        {
            colon2 = strchr(colon1 + 1, ':');
            if (colon2)
            {
                char *strnum = colon2 + 1;
                char *comp = strnums;
                colon3 = strchr(colon2 + 1, ':');
                if (colon3)
                {
                    int id_len = colon3 - colon2 - 1;
                    while (*comp)
                    {
                        int comp_len = strlen(comp);
                        if (id_len == comp_len &&
                            !memcmp(strnum, comp, id_len))
                        {
                            int line_len = strlen(line);
                            op->dest = realloc(op->dest, op_dest_len + line_len + 1);
                            if (!op->dest)
                            {
                                int err = errno;
                                ls_stderr("Namespace unable to allocate %s buffer: %s\n",
                                          filename, strerror(err));
                                fclose(fh);
                                return nsopts_rc_from_errno(err);
                            }
                            memcpy(&op->dest[op_dest_len], line, line_len + 1);
                            op_dest_len += (line_len);
                            op->flags |= OP_FLAG_ALLOCATED_DEST;
                            DEBUG_MESSAGE("extract_filedata, add line: %s -> (%p, len: %d) %s", 
                                          line, op->dest, (int)strlen(op->dest), op->dest);
                            break;
                        }
                        comp += (comp_len) + 1;
                    }
                }
            }
        }
        if (!colon1 || !colon2 || !colon3)
            DEBUG_MESSAGE("extract_filedata: Bad line in file: %s", line);
    }
    fclose(fh);
    op->flags &= ~OP_FLAG_BWRAP_SYMBOL;
    return 0;
}


static int bwrap_file_to_data(lscgid_t *pCGI, SetupOp *op)
{
    /* Convert passwd or group to just the data requested and store it in the
     * destination.  */
    char *work_str = strdup(op->dest);
    char *strnums = NULL;
    int rc = 0;
    if (!work_str)
    {
        int err = errno;
        ls_stderr("Namespace error duping a string: %s: %s\n", op->dest, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    rc = create_strnums(pCGI, op, work_str, &strnums);
    if (rc)
    {
        free(work_str);
        return rc;
    }
    free(work_str);
    rc = extract_filedata(op, strnums);
    if (rc)
    {
        free(strnums);
        return rc;
    }
    free(strnums);
    return 0;
}


static char *get_newroot_path(const char *path)
{
    char *result_path;
    int  len;
    while (*path == '/')
        path++;
    
    len = strlen(path) + 10;
    result_path = malloc(len);
    if (!result_path)
    {
        ls_stderr("Namespace Insufficent memory allocating newroot path\n");
        return NULL;
    }
    snprintf(result_path, len, "/newroot/%s", path);
    DEBUG_MESSAGE("get_newroot_path: %s, len: %d\n", result_path, 
                  (int)strlen(result_path));
    return result_path;
}


static char *get_oldroot_path(const char *path)
{
    char *result_path;
    int  len;
    while (*path == '/')
        path++;
    
    len = strlen(path) + 10;
    result_path = malloc(len);
    if (!result_path)
    {
        ls_stderr("Namespace Insufficent memory allocating oldroot path\n");
        return NULL;
    }
    snprintf(result_path, len, "/oldroot/%s", path);
    DEBUG_MESSAGE("get_oldroot_path %s -> %s\n", --path, result_path);
    return result_path;
}


static int cleanup_source(SetupOp *op, char **ppch)
{
    struct stat st;
    if (!op->source)
    {
        ls_stderr("Namespace option bind-tmp requires source\n");
        return DEFAULT_ERR_RC;
    }
    if (!op->dest)
    {
        ls_stderr("Namespace option bind-tmp requires dest\n");
        return DEFAULT_ERR_RC;
    }
    if (!stat(op->source, &st) && (st.st_mode & S_IFMT) == S_IFDIR)
    {
        DEBUG_MESSAGE("nspersist_setup_bind_tmp already exists %s\n", op->source);
        op->type = SETUP_BIND_MOUNT;
        return 0;
    }
    int source_len = strlen(op->source);
    char *pch = strrchr(op->source, '/');
    while (pch && pch == op->source + source_len)
    {
        *(op->source + source_len) = 0;
        --source_len;
        pch = strrchr(op->source, '/');
    }
    if (!pch || pch == op->source)
    {
        ls_stderr("Namespace source missing higher level directory: %s\n", 
                  op->source);
        return DEFAULT_ERR_RC;
    }
    *ppch = pch;
    return 0;
}


static int tmp_finish_ok(lscgid_t *pCGI, SetupOp *op)
{
    umask(0);
    if (mkdir(op->source, 0777/*0700*/)) // Must be 777 or can't be written to.
    {
        int err = errno;
        ls_stderr("Namespace error making requested bind-tmp dir %s: %s\n",
                  op->source, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    op->type = SETUP_BIND_MOUNT;
    DEBUG_MESSAGE("tmp created directory just fine: %s\n", op->source);
    return 0;
}


static int setup_bind_tmp(lscgid_t *pCGI, SetupOp *op)
{
    struct stat st;
    char *pch = NULL;
    DEBUG_MESSAGE("nspersist_setup_bind_tmp %s %s\n", op->source, op->dest);
    int rc = cleanup_source(op, &pch);
    if (!pch || rc)
        return rc;
    char *dironly = malloc((pch - op->source) + 1);
    if (!dironly)
    {
        ls_stderr("Namespace insufficient memory creating tmp dironly\n");
        return DEFAULT_ERR_RC;
    }
    memcpy(dironly, op->source, pch - op->source);
    *(dironly + (pch - op->source)) = 0;
    if (!stat(dironly, &st))
    {
        DEBUG_MESSAGE("Found successfully dironly: %s\n", dironly);
        free(dironly);
        return tmp_finish_ok(pCGI, op);
    }
    free(dironly);
    if (strchr(pch + 1, '/'))
    {
        ls_stderr("Namespace error bind-tmp must only have one directory "
                  "level in the source\n");
        return DEFAULT_ERR_RC;
    }
    char vhdironly[PERSIST_DIR_SIZE];
    persist_namespace_dir_vh(pCGI->m_data.m_uid, NS_TYPE_MNT, vhdironly,
                             sizeof(vhdironly));
    if (access(vhdironly, 0) && mkdir(vhdironly, 0777/*0700*/))
    {
        int err = errno;
        ls_stderr("Namespace error creating bind-tmp dir: %s: %s\n", vhdironly,
                  strerror(err));
        return nsopts_rc_from_errno(err);
    }
    int newsource_len = strlen(vhdironly) + 1 + strlen(pch + 1) + 1;
    char *newsource = malloc(newsource_len);
    if (!newsource)
    {
        ls_stderr("Namespace insufficient memory creating derived tmp\n");
        return DEFAULT_ERR_RC;
    }
    snprintf(newsource, newsource_len, "%s/%s", vhdironly, pch + 1);
    DEBUG_MESSAGE("Final newsource: %s\n", newsource);
    if (op->flags & OP_FLAG_ALLOCATED_SOURCE)
        free(op->source);
    op->source = newsource;
    op->flags |= OP_FLAG_ALLOCATED_SOURCE;
    return tmp_finish_ok(pCGI, op);
}


static int setup_copy(lscgid_t *pCGI, SetupOp *op, int index)
{
    int fds[2];
    DEBUG_MESSAGE("setup_copy\n");
    if (!op->source || !op->dest)
    {
        ls_stderr("Namespace missing source or dest to copy\n");
        return DEFAULT_ERR_RC;
    }
    int fd = open(op->source, O_RDONLY);
    if (fd == -1)
    {
        int err = errno;
        if (err == ENOENT && op->flags & OP_FLAG_ALLOW_NOTEXIST)
        {
            DEBUG_MESSAGE("Does not exist, set op to NOOP: %s\n", op->source);
            op->type = SETUP_NOOP;
            return 0;
        }
        ls_stderr("Namespace error opening source in copy %s: %s\n", op->source,
                  strerror(err));
        return nsopts_rc_from_errno(err);
    }
    if (pipe(fds))
    {
        int err = errno;
        ls_stderr("Namespace error creating pipe to copy: %s\n", strerror(err));
        close(fd);
        return nsopts_rc_from_errno(err);
    }
    char block[BLOCK_SIZE];
    int bytes;
    fcntl(fds[1], F_SETFL, (fcntl(fds[1], F_GETFL, 0) | O_NONBLOCK));
    while ((bytes = read(fd, block, BLOCK_SIZE)) > 0)
    {
        if (write(fds[1], block, bytes) == -1)
        {
            int err = errno;
            close(fd);
            close_pipe(fds);
            if (!(op->flags & OP_FLAG_ALLOW_NOTEXIST))
            {
                ls_stderr("Namespace error writing to pipe in copy: %s", 
                          strerror(err));
                return nsopts_rc_from_errno(err);
            }
            DEBUG_MESSAGE("Can't write to file in copy, set op to NOOP: %s\n", strerror(err));
            op->type = SETUP_NOOP;
            return 0;
        }
    }
    if (bytes < 0)
    {
        int err = errno;
        ls_stderr("Namespace error reading from %s in copy: %s", op->source,
                  strerror(err));
        close(fd);
        close_pipe(fds);
        return nsopts_rc_from_errno(err);
    }
    close(fd);
    close(fds[1]);
    op->fd = fds[0];
    return 0;
}


static int presetup_bind_tmp(lscgid_t *pCGI, SetupOp *op, int index)
{
    char persist_dir[PERSIST_DIR_SIZE];
                                                
    if (op->flags & OP_FLAG_ALLOCATED_SOURCE)
    {
        free(op->source);
        op->flags &= ~OP_FLAG_ALLOCATED_SOURCE;
    }
    int bind_tmp_len = strlen(persist_namespace_dir_vh(pCGI->m_data.m_uid, 
                                                       NS_TYPE_MNT, 
                                                       persist_dir, 
                                                       sizeof(persist_dir))) +
                       PERSIST_NO_VH_PREFIX_LEN + STRNUM_SIZE + 5;
    char *bind_tmp = malloc(bind_tmp_len);
    if (!bind_tmp)
    {
        ls_stderr("Namespace insufficient memory in allocating bind_tmp\n");
        return DEFAULT_ERR_RC;
    }
    op->source = bind_tmp;
    op->flags |= OP_FLAG_ALLOCATED_SOURCE;
    snprintf(bind_tmp, bind_tmp_len, "%s/"PERSIST_NO_VH_PREFIX"tmp%d",
             persist_dir, index);
    DEBUG_MESSAGE("presetup_bind_tmp set to: %s\n", bind_tmp);
    return 0;
}


static int op_as_user(lscgid_t *pCGI, SetupOp *setupOp)
{
    SetupOp *op;
    int rc, index = 0;

    for (op = &setupOp[0]; !(op->flags & OP_FLAG_LAST); ++op)
    {
        DEBUG_MESSAGE("op_as_user[%d]: type: %d, flags: 0x%x, src: %s, dest: %s\n",
                      index, op->type, op->flags, op->source, op->dest);
        ++index;
        if (op->type == SETUP_BIND_TMP && (rc = presetup_bind_tmp(pCGI, op, index)))
            return rc;
        if ((op->type == SETUP_BIND_MOUNT || op->type == SETUP_RO_BIND_MOUNT ||
             op->type == SETUP_MAKE_DIR || op->type == SETUP_BIND_TMP ||
             op->type == SETUP_COPY) &&
            op->flags & OP_FLAG_BWRAP_SYMBOL)
        {
            int i;
            for (i = 0; i < 2; ++i)
            {
                char *parm; 

                if (i == 0)
                    parm = op->source;
                else
                    parm = op->dest;
                if (parm)
                {
                    char *new_parm = NULL;
                    rc = bwrap_symbol_in_path(pCGI, parm, &new_parm);
                    if (rc)
                        return rc;
                    if (!new_parm)
                        continue;
                    if (parm == op->source)
                    {
                        if (op->flags & OP_FLAG_ALLOCATED_SOURCE)
                            free(op->source);
                        op->source = new_parm;
                        op->flags |= OP_FLAG_ALLOCATED_SOURCE;
                    }
                    else
                    {
                        if (op->flags & OP_FLAG_ALLOCATED_DEST)
                            free(op->dest);
                        op->dest = new_parm;
                        op->flags |= OP_FLAG_ALLOCATED_DEST;
                    }
                    op->flags &= ~OP_FLAG_BWRAP_SYMBOL;
                }
            }
        }
        else if (op->type == SETUP_MAKE_GROUP || op->type == SETUP_MAKE_PASSWD)
        {
            rc = bwrap_file_to_data(pCGI, op);
            if (rc)
                return rc;
        }
        if (op->type == SETUP_BIND_TMP && (rc = setup_bind_tmp(pCGI, op)))
            return rc;
        if (op->type == SETUP_COPY && (rc = setup_copy(pCGI, op, index)))
            return rc;
    }
    return 0;
}


static int bind_mount(const char *src, const char *dest, bind_option_t options)
{
    int readonly = (options & BIND_READONLY) != 0;
    int devices = (options & BIND_DEVICES) != 0;
    int recursive = (options & BIND_RECURSIVE) != 0;
    unsigned long current_flags, new_flags;
    mount_tab_t *mount_tab = NULL;
    char *resolved_dest = NULL;
    int i;

    DEBUG_MESSAGE("bind_mount, src: %s, dest: %s READONLY: %s, DEVICES: %s, RECURSIVE: %s, uid: %d\n", 
                  src, dest, readonly ? "YES":"NO", devices ? "YES":"NO",
                  recursive ? "YES":"NO", getuid());
    if (src)
    {
        if (mount(src, dest, NULL, 
                  MS_SILENT | MS_BIND | (recursive ? MS_REC : 0), NULL) != 0)
        {
            int err = errno;
            ls_stderr("Namespace mount of source/dest failed: %s\n", strerror(err));
            return 1;
        }
        DEBUG_MESSAGE("Did bind_mount of: %s on [%s]\n", src, dest);
    }

    /* The mount operation will resolve any symlinks in the destination
       path, so to find it in the mount table we need to do that too. */
    DEBUG_MESSAGE("bind_mount, dest: %s\n", dest);
    resolved_dest = realpath(dest, NULL);
    if (resolved_dest == NULL)
    {
        int err = errno;
        ls_stderr("Namespace realpath %s failed: %s\n", dest, strerror(err));
        return 2;
    }
    mount_tab = parse_mount_tab(resolved_dest);
    if (!mount_tab || mount_tab[0].mountpoint == NULL)
    {
        ls_stderr("Namespace No mountpoint at resolved_dest: %s\n", resolved_dest);
        errno = EINVAL;
        free(resolved_dest);
        free_mount_tab(mount_tab);
        return 2; /* No mountpoint at dest */
    }

    //assert (path_equal(mount_tab[0].mountpoint, resolved_dest));
    current_flags = mount_tab[0].options;
    new_flags = current_flags | (devices ? 0 : MS_NODEV) | MS_NOSUID | 
                (readonly ? MS_RDONLY : 0);
    if (new_flags != current_flags &&
        mount("none", resolved_dest,
              NULL, MS_SILENT | MS_BIND | MS_REMOUNT | new_flags, NULL) != 0)
    {
        int err = errno;
        ls_stderr("Namespace mount of none to %s failed: %s\n", resolved_dest, 
                  strerror(err));
        free(resolved_dest);
        free_mount_tab(mount_tab);
        return 3;
    }
    free(resolved_dest);

    /* We need to work around the fact that a bind mount does not apply the 
     * flags, so we need to manually apply the flags to all submounts in the 
     * recursive case.
     * Note: This does not apply the flags to mounts which are later propagated 
     * into this namespace.
     */
    if (recursive)
    {
        for (i = 1; mount_tab[i].mountpoint != NULL; i++)
        {
            current_flags = mount_tab[i].options;
            new_flags = current_flags | (devices ? 0 : MS_NODEV) | MS_NOSUID | 
                        (readonly ? MS_RDONLY : 0);
            if (new_flags != current_flags &&
                mount("none", mount_tab[i].mountpoint,
                      NULL, MS_SILENT | MS_BIND | MS_REMOUNT | new_flags, NULL) != 0)
            {
                /* If we can't read the mountpoint we can't remount it, but that should
                   be safe to ignore because its not something the user can access. */
                if (errno != EACCES)
                {
                    int err = errno;
                    ls_stderr("Namespace (re)mount of none to %s failed: %s\n", 
                              mount_tab[i].mountpoint, strerror(err));
                    free_mount_tab(mount_tab);
                    return 5;
                }
            }
        }
    }
    free_mount_tab(mount_tab);

    return 0;
}


static int do_copy(uint32_t flags, const char *source, const char *dest, int *fd)
{
    struct stat st;
    DEBUG_MESSAGE("do_copy: %s -> %s\n", source, dest);
    if (!fd)
    {
        ls_stderr("Namespace copy not supported in privileged environment\n");
        return DEFAULT_ERR_RC;
    }
    if (stat(source, &st))
    {
        if (flags & OP_FLAG_ALLOW_NOTEXIST)
        {
            DEBUG_MESSAGE("do_copy source error: %s\n", strerror(errno));
            return 0;
        }
        else
        {
            ls_stderr("Namespace error in copy of %s: %s\n", source, strerror(errno));
            return DEFAULT_ERR_RC;
        }
    }
    char *last_slash = strrchr(dest, '/');
    if (!last_slash)
    {
        ls_stderr("Namespace copy of unqualified target %s\n", dest);
        return -1;
    }
    char *dest_dir = strndup(dest, last_slash - dest);
    if (!dest_dir)
    {
        ls_stderr("Namespace can't allocate destination in copy\n");
        return -1;
    }
    int rc = access(dest_dir, 0);
    int err = errno;
    free(dest_dir);
    if (rc) 
    {
        if (flags & OP_FLAG_ALLOW_NOTEXIST)
        {
            DEBUG_MESSAGE("do_copy dest dir doesn't exist: %s\n", strerror(err));
            return 0;
        }
        ls_stderr("Namespace error in copy to dest: %s\n", strerror(err));
        return -1;
    }
    char temp_filename[PERSIST_DIR_SIZE];
    snprintf(temp_filename, sizeof(temp_filename), 
             "/newroot/tmp/"PERSIST_NO_VH_PREFIX"copy%d", *fd);
    DEBUG_MESSAGE("Try to create %s with mode: 0x%x\n", temp_filename, st.st_mode);
    int fd_dest = creat(temp_filename, st.st_mode);
    if (fd_dest == -1)
    {
        err = errno;
        ls_stderr("Namespace error in opening temp target in copy %s: %s\n", 
                  temp_filename, strerror(err));
        close(*fd);
        *fd = -1;
        return nsopts_rc_from_errno(err);
    }
    char block[BLOCK_SIZE];
    int bytes;
    while ((bytes = read(*fd, block, BLOCK_SIZE)) > 0)
    {
        if (write(fd_dest, block, bytes) == -1)
        {
            ls_stderr("Namespace error writing to target in copy: %s: %s", dest,
                      strerror(errno));
            close(fd_dest);
            close(*fd);
            *fd = -1;
            return -1;
        }
    }
    err = errno;
    close(*fd);
    *fd = -1;
    
    //if (bytes != -1)
    //{
    //    DEBUG_MESSAGE("Set mode to %o\n", st.st_mode & 0777);
    //    if (fchmod(fd_dest, st.st_mode & 0777))
    //        DEBUG_MESSAGE("Ignore error setting mode: %s\n", strerror(errno));
    //    DEBUG_MESSAGE("Set uid: %d, gid: %d\n", st.st_uid, st.st_gid);
    //    if (fchown(fd_dest, st.st_uid, st.st_gid))
    //        DEBUG_MESSAGE("Ignore error setting user/group: %s\n", 
    //                      strerror(errno));
    //}
    close(fd_dest);
    if (bytes == -1)
    {
        ls_stderr("Namespace error writing to target in copy: %s: %s", dest,
                  strerror(err));
        return -1;
    }
    DEBUG_MESSAGE("copy complete: %s -> %s\n", source, dest);
    rc = bind_mount(temp_filename, dest, 0);
    return 0;
}


static int privileged_op(int privileged_op_socket, uint32_t op, 
                         uint32_t flags, const char *arg1, const char *arg2, int *fd)
{
    if (privileged_op_socket != -1)
    {
        uint32_t buffer[2048];  /* 8k, but is int32 to guarantee nice alignment */
        PrivSepOp *op_buffer = (PrivSepOp *) buffer;
        size_t buffer_size = sizeof (PrivSepOp);
        uint32_t arg1_offset = 0, arg2_offset = 0;

        /* We're unprivileged, send this request to the privileged part */
        if (arg1 != NULL)
        {
            arg1_offset = buffer_size;
            buffer_size += strlen (arg1) + 1;
        }
        if (arg2 != NULL)
        {
            arg2_offset = buffer_size;
            buffer_size += strlen (arg2) + 1;
        }

        if (buffer_size >= sizeof (buffer))
        {
            ls_stderr("Namespace error in priveleged_op, buffer too big\n");
            return DEFAULT_ERR_RC;
        }

        DEBUG_MESSAGE("privileged_op send request to privileged part: %d\n", op);
        op_buffer->op = op;
        op_buffer->flags = flags;
        op_buffer->arg1_offset = arg1_offset;
        op_buffer->arg2_offset = arg2_offset;
        if (arg1 != NULL)
            strcpy ((char *) buffer + arg1_offset, arg1);
        if (arg2 != NULL)
            strcpy ((char *) buffer + arg2_offset, arg2);

        if (write(privileged_op_socket, buffer, buffer_size) != (ssize_t)buffer_size)
        {
            int err = errno;
            ls_stderr("Namespace error writing to privileged_op_socket: %s\n", 
                      strerror(err));
            return nsopts_rc_from_errno(err);
        }

        if (read(privileged_op_socket, buffer, 1) != 1)
        {
            int err = errno;
            ls_stderr("Namespace error reading op code from privileged op socket: %s\n",
                      strerror(err));
            return nsopts_rc_from_errno(err);
        }

        return 0;
    }

    /*
     * This runs a privileged request for the unprivileged setup
     * code. Note that since the setup code is unprivileged it is not as
     * trusted, so we need to verify that all requests only affect the
     * child namespace as set up by the privileged parts of the setup,
     * and that all the code is very careful about handling input.
     *
     * This means:
     *  * Bind mounts are safe, since we always use filesystem namespace. They
     *     must be recursive though, as otherwise you can use a non-recursive bind
     *     mount to access an otherwise over-mounted mountpoint.
     *  * Mounting proc, tmpfs, mqueue, devpts in the child namespace is assumed to
     *    be safe.
     *  * Remounting RO (even non-recursive) is safe because it decreases privileges.
     *  * sethostname() is safe only if we set up a UTS namespace
     */
    DEBUG_MESSAGE("privileged_op run request here: %d\n", op);
    switch (op)
    {
        case PRIV_SEP_OP_DONE:
            break;

        case PRIV_SEP_OP_REMOUNT_RO_NO_RECURSIVE:
            if (bind_mount(NULL, arg2, BIND_READONLY) != 0)
            {
                ls_stderr("Namespace error in bind mount of %s: %s\n", arg2, 
                          strerror(errno));
                return -1;
            }
            break;

        case PRIV_SEP_OP_BIND_MOUNT:
            /* We always bind directories recursively, otherwise this would let us
               access files that are otherwise covered on the host */
            if (bind_mount (arg1, arg2, BIND_RECURSIVE | flags) != 0)
            {
                ls_stderr("Namespace error in bind mount of %s to %s: %s\n", arg1, 
                          arg2, strerror(errno));
                return DEFAULT_ERR_RC;
            }
            break;

        case PRIV_SEP_OP_PROC_MOUNT:
            if (mount("proc", arg1, "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, 
                      NULL) != 0)
            {
                int err = errno;
                ls_stderr("Namespace error mounting proc (%s): %s\n", arg1,
                          strerror(err));
                return nsopts_rc_from_errno(err);
            }
            break;

        case PRIV_SEP_OP_TMPFS_MOUNT:
            {
                char *opt = "mode=0755"; // Ignore --file-label for now
                if (mount ("tmpfs", arg1, "tmpfs", MS_NOSUID | MS_NODEV, opt) != 0)
                {
                    int err = errno;
                    ls_stderr("Namespace error mounting tmpfs: %s: %s\n", arg1, 
                              strerror(err));
                    return nsopts_rc_from_errno(err);
                }
                break;
            }

        case PRIV_SEP_OP_DEVPTS_MOUNT:
            if (mount ("devpts", arg1, "devpts", MS_NOSUID | MS_NOEXEC,
                       "newinstance,ptmxmode=0666,mode=620") != 0)
            {
                int err = errno;
                ls_stderr("Namespace error mounting device: %s: %s\n", arg1, 
                          strerror(err));
                return nsopts_rc_from_errno(err);
            }
            break;

        case PRIV_SEP_OP_MQUEUE_MOUNT:
            if (mount ("mqueue", arg1, "mqueue", 0, NULL) != 0)
            {
                int err = errno;
                ls_stderr("Namespace error mounting mqueue: %s: %s\n", arg1,
                          strerror(err));
                return nsopts_rc_from_errno(err);
            }
            break;

        case PRIV_SEP_OP_SET_HOSTNAME:
            /* This is checked at the start, but lets verify it here in case
               something manages to send hacked priv-sep operation requests. */
            {
                char name[256];
                char *slash = strrchr(arg1, '/');
                if (slash)
                    strncpy(name, slash + 1, sizeof(name) - 1);
                else
                    strncpy(name, arg1, sizeof(name) - 1);
                    
                if (sethostname(name, strlen(name)) != 0)
                {
                    int err = errno;
                    struct __user_cap_header_struct hdr = { _LINUX_CAPABILITY_VERSION_3, 0 };
                    struct __user_cap_data_struct data[2] = { { 0 } };

                    if (capget (&hdr, data) < 0)
                    {
                        ls_stderr("ERROR GETTING CAPABILITIES: %s\n", strerror(errno));
                    }
                    ls_stderr("Namespace error setting hostname: %s: %s, uid: %d, euid: %d "
                              "effective caps: 0x%x (%s), permitted: 0x%x (%s)\n", 
                              name, strerror(err), getuid(), geteuid(), data[0].effective, 
                              (data[0].effective & CAP_TO_MASK_0 (CAP_SYS_ADMIN)) ? "YES" : "NO",
                              data[0].permitted,
                              (data[0].permitted & CAP_TO_MASK_0 (CAP_SYS_ADMIN)) ? "YES" : "NO");                          
                    return nsopts_rc_from_errno(err);
                }
            }
            break;

        case PRIV_SEP_OP_COPY:
            return do_copy(flags, arg1, arg2, fd);
            
        default:
            ls_stderr("Namespace unexpected privileged op #%d\n", op);
            return DEFAULT_ERR_RC;
    }
    return 0;
}


static int create_file_stat(lscgid_t *pCGI, const char *path, struct stat *st)
{
    int fd;

    fd = creat (path, st ? (st->st_mode & 0777) : 0644);
    if (fd == -1)
    {
        int err = errno;
        if (err == EEXIST)
            return 0;
        DEBUG_MESSAGE("ERROR CREATING FILE: %s: %s, mode: %o\n", path, strerror(err),
                      st ? (st->st_mode & 0777) : 0644);
        return 0;
        //ls_stderr("Namespace error creating file: %s: %s\n", path, strerror(err));
        //return nsopts_rc_from_errno(err);
    }
    if ((!st || (st && fchown(fd, st->st_uid, st->st_gid))) && 
        fchown(fd, pCGI->m_data.m_uid, pCGI->m_data.m_gid))
    {
        DEBUG_MESSAGE("Can't set owner/group for %s: %s\n", path, strerror(errno));
        /*
        int err = errno;
        ls_stderr("Namespace error setting owner/group for file: %s: %s\n", path, 
                  strerror(err));
        close(fd);
        return nsopts_rc_from_errno(err);
        */
    }
    close(fd);
    return 0;
}


static int ensure_file (lscgid_t *pCGI, const char *source, const char *path)
{
    struct stat buf;

    if (stat (path, &buf) ==  0 && S_ISREG (buf.st_mode))
        return 0;

    if (source && stat(source, &buf))
    {
        int err = errno;
        ls_stderr("Namespace error getting source info of %s: %s\n", source,
                  strerror(err));
        return nsopts_rc_from_errno(err);
    }
    return create_file_stat(pCGI, path, source ? &buf : NULL);
}


static int ensure_dir_stat (lscgid_t *pCGI, const char *path, struct stat *st)
{
    if (!path)
    {
        ls_stderr("Namespace missing path creating directory\n");
        return DEFAULT_ERR_RC;
    }
    if (mkdir(path, 0777/*st ? (st->st_mode & 0777) : 0755*/) && errno != EEXIST)
    {
        int err = errno;
        ls_stderr("Namespace can't create directory %s: %s\n", path, strerror(err));
        return nsopts_rc_from_errno(err);
    }
    if (strchr(path + 1, '/') && // Don't do highest level
        ((st && lchown(path, st->st_uid, st->st_gid)) ||
         lchown(path, pCGI->m_data.m_uid, pCGI->m_data.m_gid)))
    {
        DEBUG_MESSAGE("Ignore error setting owner/group for dir %s: %s using %s uid: %d, gid: %d\n", 
                      path, strerror(errno), st ? "original" : "program",
                      st ? st->st_uid : pCGI->m_data.m_uid, 
                      st ? st->st_gid : pCGI->m_data.m_gid);
    }
    DEBUG_MESSAGE("%s mkdir ok\n", path);
    return 0;
}


static int ensure_dir (lscgid_t *pCGI, const char *source, const char *path)
{
    struct stat buf;
    int use_stat = 0;

    if (source && stat (source, &buf) == 0)
    {
        use_stat = 1;
        if (!S_ISDIR (buf.st_mode))
        {
            ls_stderr("Namespace Can't create directory: %s, source not a dir!\n",
                      source);
            return DEFAULT_ERR_RC;
        }
    }

    return ensure_dir_stat(pCGI, path, use_stat ? &buf : NULL);
}


static int mkdir_with_parents(lscgid_t *pCGI, const char *source, const char *pathname)
{
    int create_last = 0;
    char *fn = NULL, *sfn = NULL;
    char *p, *sp = NULL;
    struct stat st, *pst = NULL;

    DEBUG_MESSAGE("mkdir_with_parents: %s -> %s\n", source, pathname);
    if (pathname == NULL || *pathname == '\0')
    {
        ls_stderr("Namespace mkdir_with_parents invalid pathname: %s\n", pathname);
        return DEFAULT_ERR_RC;
    }

    if (source)
    {
        sfn = strdup(source);
        if (!sfn)
        {
            int err = errno;
            ls_stderr("Namespace can't dup source directory: %s: %s\n", source,
                      strerror(err));
            return nsopts_rc_from_errno(err);
        }
    }
    fn = strdup(pathname);
    if (!fn)
    {
        int err = errno;
        if (sfn)
            free(sfn);
        ls_stderr("Namespace can't allocate temporary directory: %s: %s\n", pathname,
                  strerror(err));
        return nsopts_rc_from_errno(err);
    }

    p = fn;
    while (*p == '/')
        p++;
    if (source)
    {
        sp = sfn;
        while (*sp == '/')
            sp++;
        pst = &st;
    }
    do
    {
        while (*p && *p != '/')
            p++;
        while (sp && *sp && *sp != '/')
            sp++;

        if (!*p)
            p = NULL;
        else
            *p = '\0';

        if (sp && !*sp)
            sp = NULL;
        else if (sp)
            *sp = 0;
        
        if (!create_last && p == NULL)
            break;

        if (sfn && !sp)
        {
            DEBUG_MESSAGE("mkdir_with_parents, source ran out before dest %s->%s\n",
                          source, pathname);
            free(sfn);
            sfn = NULL;
            pst = NULL;
        }
        if (sp && stat(sfn, &st))
        {
            int err = errno;
            ls_stderr("Namespace mkdir_with_parents, error finding source %s: %s\n",
                      sfn, strerror(err));
            free(fn);
            free(sfn);
            return nsopts_rc_from_errno(err);
        }
        else if (sp)
        {
            DEBUG_MESSAGE("mkdir_with_parents, stat of %s got uid: %d\n", sfn, st.st_uid)
            pst = &st;
        }
        else
        {
            DEBUG_MESSAGE("mkdir_with_parents, No stat\n");
            pst = NULL;
        }
        int rc = ensure_dir_stat(pCGI, fn, pst);
        if (rc)
        {
            free(fn);
            if (sfn)
                free(sfn);
            return rc;
        }

        if (p)
        {
            *p++ = '/';
            while (*p && *p == '/')
                p++;
            if (sp)
            {
                *sp++ = '/';
                while (*sp && *sp == '/')
                    sp++;
            }
        }
    }
    while (p);
    free(fn);
    if (sfn)
        free(sfn);
    DEBUG_MESSAGE("mkdir_with_parents done\n");
    return 0;
}


static int get_file_mode(const char *pathname, struct stat *buf)
{
    if (stat(pathname, buf) != 0)
        return -1; // Must return -1 and set errno.

    return buf->st_mode & S_IFMT;
}


/* stralloc2 - don't forget to free the returned value */
static char *stralloc2(char *one, char *two)
{
    int sz = strlen(one) + strlen(two) + 1;
    char *out;
    out = malloc(sz);
    if (!out)
    {
        ls_stderr("Namespace unable to build double of %s and %s: %s\n", one, 
                  two, strerror(errno));
        return NULL;
    }
    snprintf(out, sz, "%s%s", one, two);
    return out;
}


/* stralloc3 - don't forget to free the returned value */
static char *stralloc3(char *one, char *two, char *three)
{
    int sz = strlen(one) + strlen(two) + strlen(three) + 1;
    char *out;
    out = malloc(sz);
    if (!out)
    {
        ls_stderr("Namespace unable to build triple of %s, %s and %s: %s\n", one, 
                  two, three, strerror(errno));
        return NULL;
    }
    snprintf(out, sz, "%s%s%s", one, two, three);
    return out;
}


/* This is run unprivileged in the child namespace but can request
 * some privileged operations (also in the child namespace) via the
 * privileged_op_socket.
 */
static int setup_newroot(lscgid_t *pCGI, SetupOp *setupOp, 
                         int privileged_op_socket)
{
    SetupOp *op;
    int rc = 0;
    DEBUG_MESSAGE("setup_newroot entry!\n");
    for (op = setupOp; op->flags != OP_FLAG_LAST; ++op)
        DEBUG_MESSAGE("source: %s, dest: %s\n", op->source, op->dest);
    
    DEBUG_MESSAGE("setup_newroot\n");
    for (op = setupOp; op->flags != OP_FLAG_LAST; ++op)
    {
        char *source = op->source;
        char *dest = op->dest;
        int source_mode = 0, i;
        int created_source = 0, created_dest = 0, skip = 0;
        struct stat stat_source, *pstat = NULL;
        
        DEBUG_MESSAGE("setup_newroot loop: source: %s, dest: %s\n", source, dest);
        if (!rc && !skip && source && op->type != SETUP_MAKE_SYMLINK)
        {
            source = get_oldroot_path(source);
            if (!source)
                rc = -1;
            if (!rc && !skip)
            {
                created_source = 1;
                source_mode = get_file_mode (source, &stat_source);
                if (source_mode < 0)
                {
                    if ((op->flags & OP_FLAG_ALLOW_NOTEXIST) && errno == ENOENT)
                        skip = 1;
                    else
                    {
                        ls_stderr("Namespace error getting type of source: %s: %s\n", 
                                  source,  strerror(errno));
                        rc = -1;
                    }
                }
                else
                {
                    DEBUG_MESSAGE("stat: %s, uid: %d, gid: %d, mode: 0x%x\n", 
                                  source, stat_source.st_uid, stat_source.st_gid, stat_source.st_mode)
                    pstat = &stat_source;
                }
            }
        }

        if (!rc && !skip && dest && (op->flags & OP_FLAG_NO_CREATE_DEST) == 0)
        {
            dest = get_newroot_path(dest);
            if (!dest)
                rc = -1;
            if (!rc && !skip)
            {
                created_dest = 1;
                if (mkdir_with_parents (pCGI, source, dest) != 0)
                    rc = -1;
            }
        }

        if (!rc && !skip) switch (op->type)
        {
            case SETUP_RO_BIND_MOUNT:
            case SETUP_DEV_BIND_MOUNT:
            case SETUP_BIND_MOUNT:
                if (!dest)
                {
                    ls_stderr("Namespace error you must specify a destination\n");
                    rc = -1;
                }
                else if (source_mode == S_IFDIR && ensure_dir_stat(pCGI, dest, pstat))
                    rc = -1;
                else if (source_mode != S_IFDIR && pstat && 
                         create_file_stat(pCGI, dest, pstat))
                    rc = -1;
                else if (source_mode != S_IFDIR && ensure_file(pCGI, source, dest))
                    rc = -1;
                if (!rc)
                    if (privileged_op(privileged_op_socket, PRIV_SEP_OP_BIND_MOUNT,
                                      (op->type == SETUP_RO_BIND_MOUNT ? BIND_READONLY : 0) |
                                          (op->type == SETUP_DEV_BIND_MOUNT ? BIND_DEVICES : 0),
                                      source, dest, &op->fd))
                        rc = -1;
                break;

            case SETUP_REMOUNT_RO_NO_RECURSIVE:
                if (privileged_op(privileged_op_socket, 
                                  PRIV_SEP_OP_REMOUNT_RO_NO_RECURSIVE, 0, NULL, 
                                  dest, &op->fd))
                    rc = -1;
                break;

            case SETUP_MOUNT_PROC:
                if (ensure_dir (pCGI, "/oldroot/proc", dest) != 0)
                    rc = -1;

                if (!rc && privileged_op(privileged_op_socket, 
                                         PRIV_SEP_OP_PROC_MOUNT, 0, dest, NULL, 
                                         &op->fd))
                    rc = -1;

                /* There are a bunch of weird old subdirs of /proc that could 
                 * potentially be problematic (for instance /proc/sysrq-trigger 
                 * lets you shut down the machine if you have write access). 
                 * We should not have access to these as a non-privileged user, 
                 * but lets cover them anyway just to make sure */
                if (!rc)
                {
                    char *cover_proc_dirs[] = { "sys", "sysrq-trigger", "irq", "bus" };
                    for (i = 0; i < (int)N_ELEMENTS(cover_proc_dirs); i++)
                    {
                        char *subdir = stralloc3(dest, "/", cover_proc_dirs[i]);
                        if (!subdir)
                            rc = -1;
                        if (!rc && access (subdir, W_OK) < 0)
                        {
                            /* The file is already read-only or doesn't exist.  */
                            if (errno == EACCES || errno == ENOENT)
                            {
                                free(subdir);
                                continue;
                            }
                            ls_stderr("Namespace error accessing %s: %s\n", subdir, 
                                      strerror(errno));
                            rc = -1;
                        }
                        if (!rc && privileged_op(privileged_op_socket, 
                                                 PRIV_SEP_OP_BIND_MOUNT, 
                                                 BIND_READONLY, subdir, subdir, 
                                                 &op->fd))
                            rc = -1;
                        free(subdir);
                        if (rc)
                            break;
                    }
                }
                break;

            case SETUP_MOUNT_DEV:
                if (ensure_dir (pCGI, source, dest) != 0)
                    rc = -1;

                if (!rc && privileged_op(privileged_op_socket, 
                                         PRIV_SEP_OP_TMPFS_MOUNT, 
                                         0, dest, NULL, &op->fd))
                    rc = -1;
                
                if (!rc)
                {
                    char *const devnodes[] = { "null", "zero", "full", "random", "urandom", "tty" };
                    for (i = 0; i < (int)N_ELEMENTS(devnodes); i++)
                    {
                        char *node_dest = stralloc3(dest, "/", devnodes[i]);
                        char *node_src = stralloc2("/oldroot/dev/", devnodes[i]);
                        if (!node_dest || !node_src)
                            rc = -1;
                        if (!rc && ensure_file(pCGI, node_src, node_dest) != 0)
                            rc = -1;
                        if (!rc && privileged_op(privileged_op_socket, 
                                                 PRIV_SEP_OP_BIND_MOUNT, 
                                                 BIND_DEVICES, node_src, 
                                                 node_dest, &op->fd))
                            rc = -1;
                        free(node_dest);
                        free(node_src);
                        if (rc)
                            break;
                    }
                }

                if (!rc)
                {
                    char *const stdionodes[] = { "stdin", "stdout", "stderr" };
                    for (i = 0; i < (int)N_ELEMENTS (stdionodes); i++)
                    {
                        char target[25];
                        snprintf(target, sizeof(target), "/proc/self/fd/%d", i);
                        char *node_dest = stralloc3 (dest, "/", stdionodes[i]);
                        if (!node_dest)
                            rc = -1;
                        if (!rc && symlink (target, node_dest) < 0)
                        {
                            ls_stderr("Namespace can't create symlink %s/%s: %s", 
                                      dest, stdionodes[i], strerror(errno));
                            free(node_dest);
                            node_dest = NULL;
                            rc = -1;
                        }
                        if (node_dest)
                            free(node_dest);
                        if (rc)
                            break;
                    }
                }

                /* /dev/fd and /dev/core - legacy, but both nspawn and docker do these */
                if (!rc)
                { 
                    char *dev_fd = stralloc2(dest, "/fd");
                    if (!dev_fd)
                        rc = -1;
                    if (!rc && symlink ("/proc/self/fd", dev_fd) < 0)
                    {
                        ls_stderr("Namespace can't create symlink %s: %s", dev_fd,
                                  strerror(errno));
                        rc = -1;
                    }
                    free(dev_fd);
                }
                
                if (!rc) 
                { 
                    char *dev_core = stralloc2(dest, "/core");
                    if (!dev_core)
                        rc = -1;
                    
                    if (!rc && symlink ("/proc/kcore", dev_core) < 0)
                    {
                        ls_stderr("Namespace can't create symlink %s: %s", dev_core,
                                  strerror(errno));
                        rc = -1;
                    }
                    free(dev_core);
                }

                if (!rc) 
                {
                    char *pts = stralloc2(dest, "/pts");
                    char *ptmx = stralloc2(dest, "/ptmx");
                    char *shm = stralloc2(dest, "/shm");
                    if (!pts || !ptmx || !shm)
                        rc = -1;
                    if (!rc && mkdir (shm, 0777/*0755*/) == -1)
                    {
                        ls_stderr("Namespace can't create %s: %s\n", shm, 
                                  strerror(errno));
                        rc = -1;
                    }
                    if (!rc && mkdir (pts, 0777/*0755*/) == -1)
                    {
                        ls_stderr("Can can't create %s: %s\n", pts, 
                                  strerror(errno));
                        rc = -1;
                    }
                    if (!rc && privileged_op(privileged_op_socket,
                                             PRIV_SEP_OP_DEVPTS_MOUNT, 0, pts, 
                                             NULL, &op->fd))
                        rc = -1;
                    if (!rc && symlink ("pts/ptmx", ptmx) != 0)
                    {
                        ls_stderr("Can can't symlink %s: %s\n", ptmx, 
                                  strerror(errno));
                        rc = -1;
                    }
                    free(pts);
                    free(ptmx);
                    free(shm);
                }
                
                /* If stdout is a tty, that means the sandbox can write to the
                   outside-sandbox tty. In that case we also create a /dev/console
                   that points to this tty device. This should not cause any more
                   access than we already have, and it makes ttyname() work in the
                   sandbox. */
                if (!rc && s_host_tty_dev != NULL && *s_host_tty_dev != 0)
                {
                    char *src_tty_dev = stralloc2("/oldroot", (char *)s_host_tty_dev);
                    char *dest_console = stralloc2(dest, "/console");

                    DEBUG_MESSAGE("Create console\n");
                    if (!src_tty_dev || !dest_console)
                        rc = -1;
                    
                    if (!rc && ensure_file(pCGI, src_tty_dev, dest_console) != 0)
                        rc = -1;

                    if (!rc && privileged_op(privileged_op_socket,
                                             PRIV_SEP_OP_BIND_MOUNT, 
                                             BIND_DEVICES, src_tty_dev, 
                                             dest_console, &op->fd))
                        rc = -1;
                    free(src_tty_dev);
                    free(dest_console);
                }
                
                break;

            case SETUP_MOUNT_TMPFS:
                if (ensure_dir (pCGI, NULL, dest) != 0)
                    rc = -1;

                if (!rc && privileged_op(privileged_op_socket,
                                         PRIV_SEP_OP_TMPFS_MOUNT, 0, dest, NULL, 
                                         &op->fd))
                    rc = -1;
                break;

            case SETUP_MOUNT_MQUEUE:
                if (ensure_dir (pCGI, NULL, dest) != 0)
                    rc = -1;

                if (!rc && privileged_op(privileged_op_socket,
                                         PRIV_SEP_OP_MQUEUE_MOUNT, 0, dest, 
                                         NULL, &op->fd))
                    rc = -1;
                break;

            case SETUP_MAKE_DIR:
                if (ensure_dir (pCGI, NULL, dest) != 0)
                    rc = -1;
                break;

            //case SETUP_MAKE_FILE: not supported
            //case SETUP_MAKE_BIND_FILE: not supported
            //case SETUP_MAKE_RO_BIND_FILE: not supported

            case SETUP_MAKE_GROUP:
            case SETUP_MAKE_PASSWD:
                if (ensure_dir(pCGI, "/oldroot/etc", "/newroot/etc"))
                    rc = -1;
                else
                {
                    int dest_fd = -1;
                    char *filename = (op->type == SETUP_MAKE_GROUP) ? 
                                     "/newroot/etc/group" : "/newroot/etc/passwd";
                    DEBUG_MESSAGE("Write to %s: %s (%p), len: %d, flags: 0x%x\n", 
                                  filename, dest, dest, (int)strlen(dest), 
                                  op->flags);
                    dest_fd = creat (filename, 0644);
                    if (dest_fd == -1)
                    {
                        ls_stderr("Namespace error creating %s: %s\n", filename, 
                                  strerror(errno));
                        rc = -1;
                    }
                    if (!rc && write(dest_fd, op->dest, strlen(op->dest)) <= 0)
                    {
                        ls_stderr("Namespace error writing %s: %s\n", filename, 
                                  strerror(errno));
                        rc = -1;
                    }
                    if (dest_fd != -1)
                        close (dest_fd);
                }
                break;
               
            case SETUP_MAKE_SYMLINK:
                assert (source != NULL);  
                if (symlink (source, dest) != 0 && errno != EEXIST)
                {
                    ls_stderr("Namespace can't make symlink at %s: %s\n", dest, 
                              strerror(errno));
                    rc = -1;
                }
                break;

            case SETUP_SET_HOSTNAME:
                assert (dest != NULL);
                if (privileged_op(privileged_op_socket,
                                  PRIV_SEP_OP_SET_HOSTNAME, 0, dest, NULL, 
                                  &op->fd))
                    rc = -1;
                break;

            case SETUP_COPY:
                assert (dest != NULL);
                if (privileged_op(privileged_op_socket,
                                  PRIV_SEP_OP_COPY, op->flags, source, dest, 
                                  &op->fd))
                    rc = -1;
                break;
              
            case SETUP_NO_UNSHARE_USER:
                nspersist_disable_user_namespace();
                break;
                
            case SETUP_NOOP:
                break;
                
            default:
                ls_stderr("Namespace unexpected type %d", op->type);
                rc = -1;
        }
        if (created_source)
            free(source);
        if (created_dest)
            free(dest);
        if (rc)
            break;
    }
    DEBUG_MESSAGE("setup_newroot, completed big loop, rc: %d\n", rc);
    
    if (!rc && privileged_op(privileged_op_socket,
                             PRIV_SEP_OP_DONE, 0, NULL, NULL, &op->fd))
    {
        ls_stderr("Namespace error completing operations\n");
        rc = -1;
    }
    return rc;
}


static const char *resolve_string_offset (void *buffer, size_t buffer_size,
                                          uint32_t offset)
{
    if (offset == 0)
        return NULL;

    if (offset > buffer_size)
    {
        ls_stderr("Namespace invalid string offset %d (buffer size %zd)", offset, 
                  buffer_size);
        return NULL;
    }

    return (const char *) buffer + offset;
}


static uint32_t read_priv_sec_op (int read_socket, void *buffer, 
                                  size_t buffer_size, uint32_t *flags,
                                  const char **arg1, const char **arg2)
{
    const PrivSepOp *op = (const PrivSepOp *) buffer;
    ssize_t rec_len;

    do
        rec_len = read (read_socket, buffer, buffer_size - 1);
    while (rec_len == -1 && errno == EINTR);

    if (rec_len < 0)
    {
        int err = errno;
        ls_stderr("Namespace error reading from privileged socket: %s\n", 
                  strerror(err));
        return nsopts_rc_from_errno(err);
    }

    if (rec_len == 0)
    {
        ls_stderr("Namespace child closed socket, see prior error\n");
        return DEFAULT_ERR_RC;
    }

    if (rec_len < (ssize_t)sizeof(PrivSepOp))
    {
        ls_stderr("Namespace invalid size from helper: %d\n", (int)rec_len);
        return DEFAULT_ERR_RC;
    }

    /* Guarantee zero termination of any strings */
    ((char *) buffer)[rec_len] = 0;

    *flags = op->flags;
    *arg1 = resolve_string_offset (buffer, rec_len, op->arg1_offset);
    *arg2 = resolve_string_offset (buffer, rec_len, op->arg2_offset);

    return op->op;
}


static int fork_priv_ops(lscgid_t *pCGI, SetupOp *setupOp, int persisted)
{
    pid_t child;
    int privsep_sockets[2];
    int rc;

    DEBUG_MESSAGE("fork_priv_ops\n");
    if (IS_PRIVILEGED && !persisted && s_ns_infos[NS_TYPE_USER].enabled && 
        !s_ns_infos[NS_TYPE_USER].vh)
    {
        if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, privsep_sockets))
        {
            int err = errno;
            ls_stderr("Namespace error creating privsep socket: %s\n", strerror(err));
            return nsopts_rc_from_errno(err);
        }
        child = fork();
        if (child == -1)
        {
            int err = errno;
            ls_stderr("Namespace error forking unprivileged helper: %s\n", strerror(err));
            return nsopts_rc_from_errno(err);
        }
        if (child == 0)
        {
            /* Unprivileged setup process */
            drop_privs(pCGI->m_data.m_uid, !IS_PRIVILEGED, 1, persisted);
            close (privsep_sockets[0]);
            setup_newroot (pCGI, setupOp, privsep_sockets[1]);
            persist_exit_child("fork_priv_ops", 0);
        }
        else
        {
            uint32_t buffer[2048];  /* 8k, but is int32 to guarantee nice alignment */
            uint32_t op, flags;
            const char *arg1, *arg2;
            int unpriv_socket = -1;

            unpriv_socket = privsep_sockets[0];
            close (privsep_sockets[1]);
            DEBUG_MESSAGE("fork_priv_ops, child: %d\n", child);
            do
            {
                op = read_priv_sec_op (unpriv_socket, buffer, sizeof (buffer),
                                       &flags, &arg1, &arg2);
                int rc = privileged_op (-1, op, flags, arg1, arg2, NULL); // Copy not supported here
                if (rc)
                    return rc;
                if (write (unpriv_socket, buffer, 1) != 1)
                {
                    int err = errno;
                    ls_stderr("Namespace error writing to unpriv socket: %s\n", 
                              strerror(err));
                    return nsopts_rc_from_errno(err);
                }
            }
            while (op != PRIV_SEP_OP_DONE);

            /* Continue post setup */
        }
    }
    else if ((rc = setup_newroot(pCGI, setupOp, -1)))
        return rc;
    return 0;
}


static int final_exec(lscgid_t *pCGI)
{
    DEBUG_MESSAGE("About to do exec(%s): uid: %d, euid: %d, pid: %d\n",
                  pCGI->m_pCGIDir, getuid(), geteuid(), getpid());
    if (execve(pCGI->m_pCGIDir, pCGI->m_argv, pCGI->m_env) == -1)
    {
        ls_stderr("lscgid: execve(%s) (ns): %s\n", pCGI->m_pCGIDir, 
                  strerror(errno));
        DEBUG_MESSAGE("final_exec failed\n");
        DEBUG_MESSAGE("Should not get here!\n");
    }
    return DEFAULT_ERR_RC;
}

        
static int resolve_symlinks_in_ops (SetupOp *setupOp)
{
    SetupOp *op;

    for (op = &setupOp[0]; !(op->flags & OP_FLAG_LAST); ++op)
    {
        char *old_source;

        switch (op->type)
        {
            case SETUP_RO_BIND_MOUNT:
            case SETUP_DEV_BIND_MOUNT:
            case SETUP_BIND_MOUNT:
                old_source = op->source;
                op->source = realpath (old_source, NULL);
                if (op->source == NULL)
                {
                    if (op->flags & OP_FLAG_ALLOW_NOTEXIST && errno == ENOENT)
                        op->source = old_source;
                    else
                    {
                        int err = errno;
                        ls_stderr("Namespace error in resolving path: %s: %s\n", 
                                  old_source, strerror(errno));
                        return nsopts_rc_from_errno(err);
                    }
                }
                else
                {
                    DEBUG_MESSAGE("Symlink resolved %s to %s\n", old_source, 
                                  op->source);
                    if (op->flags & OP_FLAG_ALLOCATED_SOURCE)
                        free(old_source);
                    op->flags |= OP_FLAG_ALLOCATED_SOURCE;
                }
                break;
            default:
                break;
        }
    }
    return 0;
}


static int cleanup_oldroot(lscgid_t *pCGI, int persisted)
{
    int rc;
    /* The old root better be rprivate or we will send unmount events to the parent namespace */
    if (mount ("oldroot", "oldroot", NULL, MS_SILENT | MS_REC | MS_PRIVATE, NULL) != 0)
    {
        int err = errno;
        ls_stderr("Namespace error making old root rprivate: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    if (umount2 ("oldroot", MNT_DETACH))
    {
        int err = errno;
        ls_stderr("Namespace error unmounting old root: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }

    /* This is our second pivot.  We're aiming to make /newroot the real root, 
     * and get rid of /oldroot. To do that we need a temporary place to store 
     * it before we can unmount it.   */
    { 
        int oldrootfd = open ("/", O_DIRECTORY | O_RDONLY);
        if (oldrootfd < 0)
        {
            int err = errno;
            ls_stderr("Namespace error opening root: %s\n", strerror(err));
            return nsopts_rc_from_errno(err);
        }
        if (chdir ("/newroot") != 0)
        {
            int err = errno;
            ls_stderr("Namespace error changing to /newroot: %s\n", strerror(err));
            close(oldrootfd);
            return nsopts_rc_from_errno(err);
        }
        /* While the documentation claims that put_old must be underneath
         * new_root, it is perfectly fine to use the same directory as the
         * kernel checks only if old_root is accessible from new_root.  */
        if (pivot_root (".", ".") != 0)
        {
            int err = errno;
            ls_stderr("Namespace error in second pivot: %s\n", strerror(err));
            close(oldrootfd);
            return nsopts_rc_from_errno(err);
        }
        if (fchdir (oldrootfd) < 0)
        {
            int err = errno;
            ls_stderr("Namespace error in fchdir to oldroot: %s\n", strerror(err));
            close(oldrootfd);
            return nsopts_rc_from_errno(err);
        }
        close(oldrootfd);
        if (umount2 (".", MNT_DETACH) < 0)
        {
            int err = errno;
            ls_stderr("Namespace error umount oldroot: %s\n", strerror(err));
            return nsopts_rc_from_errno(err);
        }
        if (chdir ("/") != 0)
        {
            int err = errno;
            ls_stderr("Namespace error changing to root after umount: %s\n", strerror(err));
            return nsopts_rc_from_errno(err);
        }
    }

    //if (0 != pCGI->m_data.m_uid || 0 != pCGI->m_data.m_gid) 
    if (!persisted && s_ns_infos[NS_TYPE_USER].enabled)
    {
        /* Now that devpts is mounted and we've no need for mount
           permissions we can create a new userspace and map our uid
           1:1 */
        if (unshare (CLONE_NEWUSER))
        {
            int err = errno;
            DEBUG_MESSAGE("ERROR in unshare of CLONE_NEWUSER: %s\n", strerror(err));
            ls_stderr("Namespace error unsharing user ns: %s\n", strerror(err));
            return nsopts_rc_from_errno(err);
        }
        /* We're in a new user namespace, we got back the bounding set, clear it again */
        rc = drop_cap_bounding_set (0);
        if (rc)
            return rc;
        DEBUG_MESSAGE("write_uid_gid_map to real values\n");
        rc = nsutils_write_uid_gid_map (pCGI->m_data.m_uid, 0, 
                                        pCGI->m_data.m_gid, 0,
                                        -1, 0, 0, 0);
        if (rc)
            return rc;
        DEBUG_MESSAGE("Did write_uid_gid_map to real values ok\n");
    }
    else if (!persisted && !s_ns_infos[NS_TYPE_USER].enabled)
        DEBUG_MESSAGE("USER_NAMESPACE not enabled, NOT unsharing!\n");
    /* All privileged ops are done now, so drop caps we don't need */
    rc = drop_privs(pCGI->m_data.m_uid, !IS_PRIVILEGED, s_ns_infos[NS_TYPE_USER].enabled, persisted);
    return rc;
}


/* Closes all fd:s except 0,1,2 and the passed in array of extra fds */
static int close_extra_fds (void *data, int fd)
{
    int *extra_fds = (int *) data;
    int i;

    for (i = 0; extra_fds[i] != -1; i++)
        if (fd == extra_fds[i])
            return 0;

    if (fd <= 2)
        return 0;

    close (fd);
    return 0;
}


static int fdwalk (int proc_fd, int (*cb)(void *data, int   fd), void *data)
{
    int open_max;
    int fd;
    int dfd;
    int res = 0;
    DIR *d;

    dfd = openat (proc_fd, "self/fd", O_DIRECTORY | O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOCTTY);
    if (dfd == -1)
        return res;

    if ((d = fdopendir (dfd)))
    {
        struct dirent *de;

        while ((de = readdir (d)))
        {
            long l;
            char *e = NULL;

            if (de->d_name[0] == '.')
                continue;

            errno = 0;
            l = strtol (de->d_name, &e, 10);
            if (errno != 0 || !e || *e)
                continue;

            fd = (int) l;

            if ((long) fd != l)
                continue;

            if (fd == dirfd (d))
                continue;

            if ((res = cb (data, fd)) != 0)
                break;
        }

        closedir (d);
        return res;
    }

    open_max = sysconf (_SC_OPEN_MAX);

    for (fd = 0; fd < open_max; fd++)
        if ((res = cb (data, fd)) != 0)
            break;

    return res;
}


static int propagate_exit_status (int status)
{
    if (WIFEXITED (status))
        return WEXITSTATUS (status);

    /* The process died of a signal, we can't really report that, but we
     * can at least be bash-compatible. The bash manpage says:
     *   The return value of a simple command is its
     *   exit status, or 128+n if the command is
     *   terminated by signal n.
     */
    if (WIFSIGNALED (status))
        return 128 + WTERMSIG (status);

    /* Weird? */
    return 255;
}


/* This is pid 1 in the app sandbox. It is needed because we're using
 * pid namespaces, and someone has to reap zombies in it. We also detect
 * when the initial process (pid 2) dies and report its exit status to
 * the monitor so that it can return it to the original spawner.
 *
 * When there are no other processes in the sandbox the wait will return
 * ECHILD, and we then exit pid 1 to clean up the sandbox. */
static int do_init (pid_t initial_pid)
{
    int initial_exit_status = 1;
    /* Optionally bind our lifecycle to that of the caller */
    handle_die_with_parent ();

    while (1)
    {
        pid_t child;
        int status;

        child = wait (&status);
        if (child == initial_pid)
        {
            initial_exit_status = propagate_exit_status (status);
        }

        if (child == -1 && errno != EINTR)
        {
            if (errno != ECHILD)
            {
                int err = errno;
                ls_stderr("Namespace error in init wait: %s\n", strerror(err));
                return nsopts_rc_from_errno(err);
            }
            DEBUG_MESSAGE("pid_1 error: %s\n", strerror(errno));
            break;
        }
    }
    DEBUG_MESSAGE("pid_1 terminating: %d\n", initial_exit_status);

    exit(initial_exit_status);
}


static int pid_1()
{
    pid_t pid;
    /* We have to have a pid 1 in the pid namespace, because
     * otherwise we'll get a bunch of zombies as nothing reaps
     * them. Alternatively if we're using sync_fd or lock_files we
     * need some process to own these.
     */

    if (!s_ns_infos[NS_TYPE_PID].enabled)
        return 0;
    
    DEBUG_MESSAGE("Start pid_1\n");
    pid = fork ();
    if (pid == -1)
    {
        int err = errno;
        ls_stderr("Namespace can't fork for pid 1: %s\n", strerror(errno));
        return nsopts_rc_from_errno(err);
    }
    if (pid != 0)
    {
        drop_all_caps (0); // ignore rc for now

        /* Close fds in pid 1, except stdio and optionally event_fd
           (for syncing pid 2 lifetime with monitor_child) and
           opt_sync_fd (for syncing sandbox lifetime with outside
           process).
           Any other fds will been passed on to the child though. */
        {
            int dont_close[3];
            int j = 0;
            dont_close[j++] = -1;
            fdwalk (s_proc_fd, close_extra_fds, dont_close);
        }

        return do_init (pid);
    }
    return 0;
}


static int pre_exec(lscgid_t *pCGI)
{
    int rc;
    rc = handle_die_with_parent ();

    if (!rc && !IS_PRIVILEGED)
        rc = set_ambient_capabilities ();

    if (!rc && pCGI->m_pChroot)
    {
        DEBUG_MESSAGE("About to chroot(%s)\n", pCGI->m_pChroot);
        if (chroot(pCGI->m_pChroot) < 0)
        {
            int err = errno;
            ls_stderr("Namespace error in chroot to %s: %s\n", pCGI->m_pChroot, 
                      strerror(err));
            return nsopts_rc_from_errno(err);
        }
    }
        
    return rc;
}


static int ns_child(lscgid_t *pCGI, SetupOp *setupOp, int persisted, 
                    int parent_write)
{
    nsipc_parent_done_t parent_done;
    int rc = 0;
    int err;
    int persist_sibling_child[2] = { -1, -1 }, persist_sibling_rc[2] = { -1, -1 };
    pid_t parent_pid;
    mode_t old_umask = -1;
    char *old_cwd = NULL;
    
    DEBUG_MESSAGE("Entering child, uid: %d, ppid: %d\n", getuid(), getppid());

    if (!rc)
        rc = op_as_user(pCGI, setupOp);

    if (!rc)
        rc = persist_child_start(pCGI, persisted, &parent_pid,
                                 persist_sibling_child, persist_sibling_rc);
    if (rc)
        return rc;
    rc = read(parent_write, &parent_done, sizeof(parent_done));
    close(parent_write);
    err = errno;
    if (rc <= 0)
    {
        ls_stderr("Namespace child error reading from parent: %s\n", strerror(err));
        rc = nsopts_rc_from_errno(err);
        persist_child_done(persisted, rc, -1, 
                           persist_sibling_child, persist_sibling_rc);
        return rc;
    }
    else 
        rc = 0;
    DEBUG_MESSAGE("Child ok to go\n");
    if (!rc)
        rc = switch_to_user_with_privs(pCGI, persisted);
    
    if (!rc && !IS_PRIVILEGED && !persisted && !s_ns_infos[NS_TYPE_USER].vh)
    {
        /* In the unprivileged case we have to write the uid/gid maps in
         * the child, because we have no caps in the parent */
        rc = nsutils_write_uid_gid_map (0, 0, 
                                        0, 0,
                                        -1, 1, 0, 0);
    }

    old_umask = umask (0);

    if (!rc)
        rc = resolve_symlinks_in_ops(setupOp);
    
    old_cwd = get_current_dir_name ();

    if (!rc)
        rc = build_mount_namespace(pCGI);

    if (!rc)
        rc = fork_priv_ops(pCGI, setupOp, persisted);

    if (!rc)
        rc = cleanup_oldroot(pCGI, persisted);
    
    if (old_umask != (mode_t)-1)
        umask(old_umask);
    if (!rc && !persisted)
    {
        if (pCGI->m_data.m_umask)
            umask(pCGI->m_data.m_umask);
        if (pCGI->m_cwdPath)
        {
            if (chdir(pCGI->m_cwdPath)) // Best effort
                DEBUG_MESSAGE("Error in chdir(%s): %s\n", pCGI->m_cwdPath, 
                              strerror(errno));
        }
        else if (old_cwd)
            if (chdir(old_cwd))
                DEBUG_MESSAGE("Error in chdir(%s): %s\n", old_cwd, 
                              strerror(errno));
                
    }    
    if (old_cwd)
        free(old_cwd);
    
    if (!rc && !persisted)
        rc = pre_exec(pCGI);
    
    if (!rc && !persisted)
        rc = pid_1();

    if (!rc && !persisted && access(pCGI->m_pCGIDir, 0))
    {
        int err = errno;
        ls_stderr("Namespace error in locating %s: %s, pid: %d\n", pCGI->m_pCGIDir, 
                  strerror(err), getpid());
        rc = nsopts_rc_from_errno(errno);
    }
    
    rc = persist_child_done(persisted, rc, parent_pid, 
                            persist_sibling_child, persist_sibling_rc);

    if (!rc && !persisted)
    {
        rc = final_exec(pCGI);
    }
    return rc;
}


#ifdef UNMOUNT_NS
char *ns_getenv(lscgid_t *pCGI, const char *title)
{
    DEBUG_MESSAGE("getenv(%s): %s\n", title, getenv(title));
    return getenv(title);
}
#else
char *ns_getenv(lscgid_t *pCGI, const char *title)
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
#endif


static int init(lscgid_t *pCGI)
{
    int rc, ns_type;
    if (getuid())
    {
        ls_stderr("To run namespace support, you must run as root.\n");
        return DEFAULT_ERR_RC;
    }
    rc = nsopts_init(pCGI, &s_ns_conf, &s_ns_conf2);
    if (rc)
        return rc;
    rc = nspersist_init(pCGI);
    if (rc)
        return rc;
    
    if (s_didinit)
        return 0;
    
    if (isatty (1))
        s_host_tty_dev = ttyname (1);

    DEBUG_MESSAGE("tty device: %s\n", s_host_tty_dev);
    
    /* Never gain any more privs during exec */
    if (prctl (PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0)
    {
        int err = errno;
        ls_stderr("Namespace error setting NO_NEW_PRIVS: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    rc = get_caps();
    if (rc)
        return rc;
    rc = read_overflowids();
    if (rc)
        return rc;
    rc = check_cgroup_ns();
    if (rc)
        return rc;
    s_proc_fd = open("/proc", O_PATH);
    if (s_proc_fd == -1)
    {
        int err = errno;
        ls_stderr("Error opening /proc: %s\n", strerror(err));
        return nsopts_rc_from_errno(err);
    }
    s_clone_flags = SIGCHLD;
    for (ns_type = 0; ns_type < NS_TYPE_COUNT; ++ns_type)
        if (s_ns_infos[ns_type].enabled)
            s_clone_flags |= s_ns_infos[ns_type].clone_flag;
        
    s_didinit = 1;
 
    return 0;
}


static int notpersist_init(lscgid_t *pCGI)
{
    int rc;
    DEBUG_MESSAGE("notpersist_init\n");
    rc = nsopts_get(pCGI, &s_setupOp_allocated, &s_SetupOp, &s_SetupOp_size,
                    &s_ns_conf, &s_ns_conf2);
    if (rc)
        return rc;
    if (!s_SetupOp)
    {
        s_SetupOp = s_SetupOp_default;
        s_SetupOp_size = sizeof(s_SetupOp_default);
    }
    return rc;
}


void ns_init_debug()
{
    s_ns_debug = !access("/lsnsdebug", 0);
    DEBUG_MESSAGE("Activating debugging on existence of /lsnsdebug\n");
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


int ns_init(lscgid_t *pCGI)
{
    if (getuid())
    {
        DEBUG_MESSAGE("Test for NS failed, not root\n");
        return 0;
    }
    char *lsns = ns_getenv(pCGI, LS_NS);
    
    DEBUG_MESSAGE("LS_NS set to %s\n", lsns);
    if (!lsns)
        return 0;
    if (!ns_supported())
    {
        ls_stderr("Namespace specified but not supported - disabled\n");
        return 0;
    }
    int enabled = atol(lsns);
    //unsetenv(LS_NS);
    if (enabled)
    {
        ns_init_debug();
        enabled = init(pCGI) ? -1 : 1;
    }
    return enabled;
}


int ns_exec(lscgid_t *pCGI, int *done)
{
    SetupOp *setupOp = NULL;
    int rc, pid, persisted;
    int parent_write[2] = { -1, -1 };
    
    DEBUG_MESSAGE("Entering ns_exec\n");
    rc = ns_init(pCGI);
    if (rc != 1)
        return rc;
    
    DEBUG_MESSAGE("After ns_init initial user: %d CGI user: %d\n", getuid(), pCGI->m_data.m_uid);
    rc = is_persisted(pCGI, &persisted);
    if (rc)
        return rc;
    if (persisted) 
    {
        rc = persisted_use(pCGI, done);
        return rc;
    }
    rc = notpersist_init(pCGI);
    if (rc)
        return rc;
    // If we already have the namespaces setup...
    if (pCGI->m_pChroot)
        DEBUG_MESSAGE("Enter ns_exec, chroot set to: %s\n", pCGI->m_pChroot);
    *done = 0;
    rc = preclone_build_user_SetupOp(pCGI, &setupOp);
    if (rc)
    {
        free_setupOp(setupOp, parent_write, 1);
        persist_exit_child("ns_exec (before fork 1)", rc);
    }
    if (pipe(parent_write))
    {
        int err = errno;
        ls_stderr("Namespace error creating pipe: %s\n", strerror(err));
        free_setupOp(setupOp, parent_write, 1);
        rc = nsopts_rc_from_errno(errno);
        persist_exit_child("ns_exec (before fork 2)", rc);
    }
    pid = fork();
    //pid = syscall (__NR_clone, s_clone_flags, NULL);
    if (pid == -1)
    {
        int err = errno;
        ls_stderr("Namespace error in clone of subtask: %s\n", strerror(err));
        free_setupOp(setupOp, parent_write, 1);
        rc = nsopts_rc_from_errno(err);
        persist_exit_child("ns_exec fork error", rc);
    }
    if (pid == 0)
    {
        close(parent_write[1]);
        parent_write[1] = -1;
        rc = ns_child(pCGI, setupOp, persisted, parent_write[0]);
        
        /* We only get here if there's an error! */
        free_setupOp(setupOp, parent_write, 0);
        persist_exit_child("ns_exec child", rc);
    }
    close(parent_write[0]);
    parent_write[0] = -1;
    rc = ns_parent(pCGI, persisted, done, pid, parent_write[1]);
    *done = 1;
    lock_persist_vh_file(NSPERSIST_LOCK_READ);
    if (!rc && persisted)
        rc = persisted_use(pCGI, done);
    free_setupOp(setupOp, parent_write, rc);
    persist_exit_child("ns_exec (parent)", rc);
    return rc; // Not used - keep the compiler happy
}



void ns_setuser(uid_t uid)
{
    DEBUG_MESSAGE("ns_setuser: %d\n", uid);
    nspersist_setuser(uid);
}


void ns_setverbose_callback(verbose_callback_t callback)
{
    s_verbose_callback = callback;
}


void ns_done(int unpersist)
{
    DEBUG_MESSAGE("ns_done\n");
    nspersist_done(unpersist);
    if (s_proc_fd != -1)
    {
        close(s_proc_fd);
        s_proc_fd = -1;
    }
    if (s_setupOp_allocated)
    {
        DEBUG_MESSAGE("ns_done free_setupOp (allocated)\n");
        free_setupOp(s_SetupOp, NULL, unpersist);
    }
    else 
    {
        nsopts_free_members(s_SetupOp);
        nsopts_free_conf(&s_ns_conf, &s_ns_conf2);
    }
    s_SetupOp = NULL;
    s_setupOp_allocated = 0;
    s_SetupOp_size = 0;
}


int ns_unpersist_all()
{
    return unpersist_all();
}

#endif


