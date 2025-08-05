/*
 * Copyright 2002-2020 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */
#ifndef _NS_PERSIST_H
#define _NS_PERSIST_H

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#ifdef __cplusplus
extern "C"
{
#endif

#include <fcntl.h>
// For systems where it's not supported in compile
#ifndef O_PATH
#define O_PATH              010000000
#endif
#include <sched.h>
#ifndef CLONE_NEWCGROUP
#define CLONE_NEWCGROUP 0x02000000
#endif
#ifndef CLONE_NEWUSER
#define CLONE_NEWUSER   0x10000000
#endif
#ifndef CLONE_NEWPID
#define CLONE_NEWPID    0x20000000
#endif
/**
 * @file nspersist.h
 * @brief Only used by ns.c and nspersist.c
 */

typedef struct mount_tab_s
{
    char *mountpoint;
    unsigned long options;
} mount_tab_t;

typedef struct mount_flags_data_s
{
    int   flag;
    char *name;
} mount_flags_data_t;

#define PERSIST_PREFIX          "/var/lsns"
#define PERSIST_PREFIX_LEN      9
#define PERSIST_NO_VH_PREFIX    ""
#define PERSIST_NO_VH_PREFIX_LEN 0
#define PERSIST_DIR_SIZE        256

/**
 * @file persist
 * @brief The character number indicating the active persist directory.
 * A read lock is created by all readers and a write lock by the process
 * creating the persisted directory.  See the namespaces doc.
 **/

#define PERSIST_FILE    "persist"

/* Define PERSIST_CLEANLY if you wish an unpersist operation to leave things
 * mostly as it found them: deleting any files created, removing directories
 * and dismounting everything.  The alternative is to simply dismount the
 * namespace mounts, leave the files and the private mounted directory.  This
 * is a bit faster, but not as cleanly.  */
//#define PERSIST_CLEANLY


/**
 * @fn nspersist_init
 * @brief Should only be called by nsinit, initializes persistence.
 * @return 0 if no error; -1 if an error.
 **/
int nspersist_init();

/**
 * @fn is_persisted
 * @brief Determines if there are persistant namespaces here.
 * @param[in] pCGI the global parameters
 * @param[in] must_persist 1 if a non-persisted environment is unacceptable.
 * @param[out] persisted Non-zero if there is a persisted pid to attach to.
 * @note This function blocks until it can get a write lock (at least for
 * a short while) which it then converts to a read lock if we're using a
 * persisted namespace; if there's nothing persisted then the write lock
 * STAYS!
 * @warning This is a POSIX record lock so the file handle survives a fork,
 * but the lock does not.  A process with the lock (parent or child) can be 
 * the one to unlock it.
 * @return 0 if no error, or an error code.
 **/
int is_persisted(lscgid_t *pCGI, int must_persist, int *persisted);

/**
 * @fn persisted_use
 * @brief Attaches to a peristant set of namespaces.
 * @param[in] pCGI the global parameters
 * @param[out] done whether it did the exec.
 * @return 0 if no error, or an error code.
 **/
int persisted_use(lscgid_t *pCGI, int *done);

#define NSPERSIST_LOCK_READ         0
#define NSPERSIST_LOCK_WRITE        1
#define NSPERSIST_LOCK_TRY_WRITE    2
#define NSPERSIST_LOCK_WRITE_NB     3
#define NSPERSIST_LOCK_NONE         -1

/**
 * @fn lock_persist_vh_file
 * @brief Locks read or write a lock file created by is_persisted.
 * @param[in] report 1 if you wish to report errors to stderr; 0 to suppress
 * @param[in] lock_type.  One of the NSPERSIST_LOCK_ constants.
 * @param[out] none
 * @return 0 if no error, or an error code.
 **/
int lock_persist_vh_file(int report, int lock_type);

/**
 * @fn unlock_close_persist_vh_file
 * @brief Unlocks and closes a lock file created by is_persisted.
 * @param[in] delete.  Whether to delete the file (closed by the last)
 * @param[out] none
 * @return 0 if no error, or an error code.
 **/
int unlock_close_persist_vh_file(int delete);

/**
 * @fn persist_do_ns
 * @brief In a case where you are rebuilding persistence from scratch, this 
 * function will do the unshare and persist the namespaces.
 * @param[in] pCGI the global parameters
 * @param[in] persisted If the non-VH namespaces are persisted.
 * @param[out] parent_pid The pid to be reported back to the caller.
 * @param[out] persist_sibling_child A set of pipes ONLY USED BY
 * persist_ns_done.
 * @param[out] persist_sibling_rc A set of pipes ONLY USED BY
 * persist_ns_done.
 * @return 0 if no error, or an error code.  The persist
 **/
int persist_ns_start(lscgid_t *pCGI, int persisted,
                     pid_t *parent_pid, int persist_sibling_child[],
                     int persist_sibling_rc[]);

/**
 * @fn persist_ns_done
 * @brief In a case where you are rebuilding persistence from scratch, call
 * after you're completely done with the persisted_handle from the ns_start
 * and the final return code to allow the child to complete.
 * @param[in] pCGI the global parameters
 * @param[in] persisted 1 if there are existing persisted namespaces to use and
 * we're only doing the VHost ones.
 * @param[in] rc The return code to be passed to the child to know if it should
 * really persist everyging.
 * @param[in] persist_pid The pid to write to the peristence file.
 * @param[in] persist_sibling_child Created by persist_ns_start.
 * @param[in] persist_sibling_rc Created by persist_ns_start.
 * @return 0 if no error, or an error code.
 **/
int persist_ns_done(lscgid_t *pCGI, int persisted, int rc,
                    int persist_pid, int persist_sibling_child[],
                    int persist_sibling_rc[]);

/**
 * @fn nspersist_setuser
 * @brief Should only be called by ns_setuser, initializes the persistant user.
 * @param[in] uid The user to set.
 * @return None.
 **/
void nspersist_setuser(uid_t uid);

/**
 * @fn nspersist_done
 * @brief Should only be called by ns_done, finalizes persistence.
 * @return 0 if no error; -1 if an error.
 **/
int nspersist_done();

/**
* @fn unpersist_uid
* @brief Lets you unpersist all one or all vhosts for a given uid.
* @warning Again an expensive call, but useful.
* @param[in] report 1 if you wish to report errors to stderr; 0 to suppress
* @param[in] uid The uid to unpersist.
* @param[in] vhost The vhost you wish to unpersist or NULL for all
* @return 0 if there were no errors.
**/
int unpersist_uid(int report, uid_t uid, char *vhost);

/**
* @fn unpersist_all
* @brief Lets you unpersist all directories (not just the ones persisted here).
* @warning Again an expensive call, but useful.
* @return 0 if there were no errors.
**/
int unpersist_all();

/**
 * @fn parse_mount_tab
 * @brief Given a root_mount returns all of the mounts below it in a mount_tab_t
 * array (ending with a NULL mount point).
 * @param[in] root_mount The starting point to search.
 * @return A pointer to mount_tab_t if no error or NULL if an error was reported.
 **/
mount_tab_t *parse_mount_tab(const char *root_mount);

/**
 * @fn free_mount_tab
 * @brief Cleans up the allocations created by parse_mount_tab.
 * @param[in] mount_tab The value returned by parse_mount_tab.
 * @return none.
 **/
void free_mount_tab(mount_tab_t *mount_tab);

/**
 * @fn persist_namespace_dir_vh
 * @brief Gets the namespace specific dir with VH for the type.
 * @param[in] uid The uid to get the dir for.
 * @param[out] dirname The dirname to get the namespace dir for.
 * @param[in] dirname_len The size of dirname.
 * @return A pointer to dirname.
 **/
char *persist_namespace_dir_vh(uid_t uid, char *dirname, int dirname_len);

/**
 * @fn persist_change_stderr_log
 * @brief  called after uid/gid/chroot change, sets stderr as specified.
 * @param[in] pCGI parsed parameters
 * @return 0 if no errors.
**/
int persist_change_stderr_log(lscgid_t *pCGI);

/**
 * @fn persist_report_pid
 * @brief Gets the final pid back to the caller if it wants it.
 * @param[in] pid The pid to report.
 * @param[in] rc If there is an error rather than a pid to report.
 * @return Whether it reported it or not (which is usually ignored).
 **/
int persist_report_pid(pid_t pid, int rc);

/**
 * @fn persist_exit_child
 * @brief Call when you have a task you want to exit without leaving zombies.
 * @param[in] desc The description.
 * @param[in] rc The final exit return code.
 * @return None - it does an exit().
 **/
void persist_exit_child(char *desc, int rc);


int nspersist_setvhost(char *vhenv);

extern mount_flags_data_t s_mount_flags_data[];

#ifdef __cplusplus
}
#endif

#endif // Linux
#endif // _NS_OPTS_H

