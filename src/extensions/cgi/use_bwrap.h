/*
 * Copyright 2020 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#ifndef USE_BWRAP_H_
#define USE_BWRAP_H_

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#define BWRAP_ALLOCATE_EXTRA_DEFAULT    100
extern int s_bwrap_extra_bytes;
#define BWRAP_ALLOCATE_EXTRA s_bwrap_extra_bytes

#define BWRAP_VAR_GID       "$GID"
#define BWRAP_VAR_GROUP     "$GROUP"
#define BWRAP_VAR_PASSWD    "$PASSWD"
#define BWRAP_VAR_UID       "$UID"
#define BWRAP_VAR_USER      "$USER"
#define BWRAP_VAR_HOMEDIR   "$HOMEDIR"
#define BWRAP_VAR_COPY      "$COPY"
#define BWRAP_VAR_COPY_TRY  "$COPY-TRY"

#define BWRAP_DEFAULT_BIN   "/bin/bwrap"
#define BWRAP_DEFAULT_BIN2  "/usr/bin/bwrap"

#define BWRAP_DEFAULT_PARAM " --ro-bind /usr /usr"\
                            " --ro-bind /lib /lib"\
                            " --ro-bind-try /lib64 /lib64"\
                            " --ro-bind /bin /bin"\
                            " --ro-bind /sbin /sbin"\
                            " --dir /var"\
                            " --ro-bind-try /var/www /var/www"\
                            " --dir /tmp"\
                            " --proc /proc"\
                            " --symlink ../tmp var/tmp"\
                            " --dev /dev"\
                            " --ro-bind-try /etc/localtime /etc/localtime"\
                            " --ro-bind-try /etc/ld.so.cache /etc/ld.so.cache"\
                            " --ro-bind-try /etc/resolv.conf /etc/resolv.conf"\
                            " --ro-bind-try /etc/ssl /etc/ssl"\
                            " --ro-bind-try /etc/pki /etc/pki"\
                            " --ro-bind-try /etc/man_db.conf /etc/man_db.conf"\
                            " --ro-bind-try /usr/local/bin/msmtp /etc/alternatives/mta"\
                            " --ro-bind-try /usr/local/bin/msmtp /usr/sbin/exim"\
                            " --bind-try " BWRAP_VAR_HOMEDIR " " BWRAP_VAR_HOMEDIR\
                            " --bind-try /var/lib/mysql/mysql.sock /var/lib/mysql/mysql.sock"\
                            " --bind-try /home/mysql/mysql.sock /home/mysql/mysql.sock"\
                            " --bind-try /tmp/mysql.sock /tmp/mysql.sock"\
                            " --bind-try /run/mysqld/mysqld.sock /run/mysqld/mysqld.sock"\
                            " --bind-try /var/run/mysqld/mysqld.sock /var/run/mysqld/mysqld.sock"\
                            " '$COPY-TRY /etc/exim.jail/$USER.conf $HOMEDIR/.msmtprc'"\
                            " --unshare-all"\
                            " --share-net"\
                            " --die-with-parent"\
                            " --dir /run/user/" BWRAP_VAR_UID\
                            " '" BWRAP_VAR_PASSWD " 65534'"\
                            " '" BWRAP_VAR_GROUP " 65534'"

#define BWRAP_DEFAULT       BWRAP_DEFAULT_BIN BWRAP_DEFAULT_PARAM
#define BWRAP_DEFAULT2      BWRAP_DEFAULT_BIN2 BWRAP_DEFAULT_PARAM


typedef struct bwrap_statics_s
{
    char  m_uid_str[11];
    char  m_gid_str[11];
    char *m_username_str;
    char *m_userdir_str;
    int   m_uid_fds[2];
    int   m_gid_fds[2];
    int   m_copy_num;   // The number of copied fds
    int  *m_copy_fds;   // A reallocated array of fds used for copies.
} bwrap_statics_t;

typedef struct bwrap_mem_s
{
    int                 m_total;
    int                 m_extra;
    bwrap_statics_t    *m_statics; /* Pointer copied to all new memory */
    struct bwrap_mem_s *m_next; /* order doesn't matter */
    char                m_data[1];
} bwrap_mem_t;

typedef void (*log_cgi_error_t)(const char *fn, const char *arg,
                                const char *explanation);
typedef void (*set_cgi_error_t)(char *p, char *p2);

/* build_bwrap_exec and bwrap_free are exported for unit testing */
int build_bwrap_exec(lscgid_t *pCGI, set_cgi_error_t cgi_error, int *argc,
                     char ***oargv, int *done, bwrap_mem_t **mem);
void bwrap_free(bwrap_mem_t **mem);
int exec_using_bwrap(lscgid_t *pCGI, set_cgi_error_t cgi_error, int *done);

//#define DO_BWRAP_DEBUG
#ifdef DO_BWRAP_DEBUG
extern int s_bwrap_debug;
void debug_message (const char *fmt, ...);
#ifndef DEBUG_MESSAGE
#define DEBUG_MESSAGE(fmt, ...) if (s_bwrap_debug) {     \
    debug_message("File: %s Line #%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
    }
#endif
#else
#define DEBUG_MESSAGE(fmt, ...) {}
#endif

#else
#define DEBUG_MESSAGE(fmt, ...) {}
#endif

#ifdef __cplusplus
}
#endif

#endif

