/*
 * Copyright 2002-2020 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */
#ifndef _NS_H
#define _NS_H

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#ifdef __cplusplus
extern "C"
{
#endif

#include "lscgid.h"

/**
 * @file ns.h
 */

#define LS_NS             "LS_NS"
#define LS_NS_LEN         5

#define LS_NS_CONF        "LS_NS_CONF"
#define LS_NS_CONF2       "LS_NS_CONF2"

#define LS_NS_VHOST       "LS_NS_VHOST" // "${VHOST} ${NAMESPACE_NAME_IN_VH} ...
#define LS_NS_VHOST_LEN   11

#define PERSIST_USER_FILE       "/.lsns_reload" // Relative to the user's home
#define PERSIST_USER_FILE_LEN   13

/**
 * @fn ns_supported
 * @brief Returns if the OS supports namespaces.
 * @return 1 if supported; 0 if not.
 **/
int ns_supported();

/**
 * @fn ns_exec
 * @brief Run the program specified in a ns
 * @param[in] pCGI the global parameters
 * @param[out] done whether it did the exec.
 * @return The return code of the exec or -1 if an error was logged.
 **/
int ns_exec(lscgid_t *pCGI, int *done);


/**
* @fn ns_setuser
* @brief If you're simply unpersisting or some other function where you are
* not calling the exec function, this lets you specify a user.
* @param[in] user The user to set.
* @return None.
**/
void ns_setuser(uid_t uid);

/**
* @fn ns_setverbose_callback
* @brief If you're doing a command line or other function, lets you specify
* a callback for detailed output.
* @param[in] user The user to set.
* @return None.
**/
typedef int (* verbose_callback_t)(const char *format, ...);
void ns_setverbose_callback(verbose_callback_t callback);

/**
* @fn ns_done
* @brief Call to complete the ns processing and free everything we can.
* @note If ns_exec is done successfully, a namespace is persisted and will
* last until the next reboot until, and unless, you specify unpersist here.
* @param[in] unpersist 1 if you wish to dismount the persisted namespaces.  0
* (the default) is to leave them running.
* @warning Setting unpersist to 1 will get the write lock on the mounts and
* do them all.  This is a somewhat expensive call and should only be used when
* you're sure it's a good time.
* @warning This function blocks for unpersist == 1.
* @note This function only unpersists the last one just persisted (if any).
* @return None.
**/
void ns_done(int unpersist);

/**
* @fn ns_unpersist_all
* @brief Lets you unpersist all directories (not just the ones persisted here).
* @warning Again an expensive call, but useful.
* @warning This function blocks.
* @return 0 if there were no errors.
**/
int ns_unpersist_all();

/**
* @fn ns_getenv
* @brief Unique to OLS, gets the environment variables as passed on the wire.
* @param[in] pCGI the CGI used by lscgid.
* @param[in] title the value to search for.
* @return A pointer the the value that matches the title found if any.
**/
char *ns_getenv(lscgid_t *pCGI, const char *title);

/**
 * @fn ns_init
 * @brief Only used by unmount_ns, initializes when you don't do ns_exec.
 * @return 0 if not specified or initialized, 1 if specified and initialized
 * -1 for an error.
 **/
int ns_init(lscgid_t *pCGI);

/**
 * @fn ns_init_debug
 * @brief Only used internally and by bubblewrap lets you to some advanced
 * debugging.
 * @return None.
 **/
void ns_init_debug();

#ifdef __cplusplus
}
#endif

#endif // Linux
#endif // _CGROUP_USE_H

