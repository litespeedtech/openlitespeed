/*
 * Copyright 2002-2024 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */
#ifndef _NS_NOSANDBOX_H
#define _NS_NOSANDBOX_H

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file nsnosandbox.h
 * @brief Only used by ns.c, nsopts.c and nssandbox.c
 */

/* IPC protocol.  Begins with sandbox_req.  */
typedef struct sandbox_init_req_s
{
    pid_t   m_pid;
    pid_t   m_ppid;
    uid_t   m_uid;
    gid_t   m_gid;
    struct rlimit   m_data;
    struct rlimit   m_nproc;
    struct rlimit   m_cpu;
    int     m_argc;
    int     m_argv_len;
    int     m_nenv;
    int     m_env_len;
} sandbox_init_req_t;
/* Followed by the argv array as a single m_argv_len string, binary 0 argument separators
   Followed by the env array as a single m_env_len string, binary 0 argument separators.  */

typedef enum {
    SANDBOX_STDIN,
    SANDBOX_STDOUT,
    SANDBOX_STDERR,
    SANDBOX_DONE,
} sandbox_data_type_t;

typedef struct sandbox_data_s
{
    sandbox_data_type_t m_sandbox_data_type;
    int                 m_datalen_rc; // Depending on the type
} sandbox_data_t;
// Followed by datalen data as a string

/* Non-files */
#define NOSANDBOX_BUF_SZ    2048

#define NOSANDBOX_MAX_FILE_LEN  512

/* Variables */
extern char       *s_nosandbox;
extern char      **s_nosandbox_arr;
extern int         s_nosandbox_count;
extern char       *s_nosandbox_pgm;
extern char       *s_nosandbox_link;
extern char      **s_nosandbox_link_arr;
extern char       *s_nosandbox_name;
extern int         s_nosandbox_added;

int nsnosandbox_init();
int nsnosandbox_socket_file_name(char *file_name, int max);
void nsnosandbox_done();
int nsnosandbox_symlink();

#ifdef __cplusplus
}
#endif // c++
#endif // linux
#endif // guard
