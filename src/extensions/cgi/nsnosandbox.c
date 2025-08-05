/*
 * Copyright 2024 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "ns.h"
#include "nsopts.h"
#include "nsnosandbox.h"
#include "nspersist.h"
#include "lscgid.h"
#include "use_bwrap.h"

char       *s_nosandbox = NULL;
char      **s_nosandbox_arr = NULL; /* At the end of no_sandbox and sorted. */
int         s_nosandbox_count = 0;
char       *s_nosandbox_pgm = NULL;
int         s_nosandbox_socket = -1;
int         s_nosandbox_pid = 0;
char       *s_nosandbox_link = NULL;
char      **s_nosandbox_arr_link = NULL; /* At the end of no_sandbox and sorted. */
int         s_nosandbox_force_symlink = 0;
char       *s_nosandbox_name = NULL;
int         s_nosandbox_added = 0;

char *s_bwrap_var[] = {
    BWRAP_VAR_COPY,
    BWRAP_VAR_COPY_TRY,
    BWRAP_VAR_GID,
    BWRAP_VAR_GROUP,
    BWRAP_VAR_HOMEDIR,
    BWRAP_VAR_PASSWD,
    BWRAP_VAR_UID,
    BWRAP_VAR_USER
};

static int readall(int fd, char *pBuf, int len, int EOFOk)
{
    struct pollfd   pfd;
    int             left = len;
    int             ret;

    pfd.fd      = fd;
    pfd.events  = POLLIN;

    DEBUG_MESSAGE("readall, %d bytes, EOFOk: %d\n", len, EOFOk);
    while (left > 0)
    {
        ret = poll(&pfd, 1, 1000);
        if (ret == 1)
        {
            if (pfd.revents & POLLIN)
            {
                ret = read(fd, pBuf, left);
                if (ret > 0)
                {
                    left -= ret;
                    pBuf += ret;
                }
                else if (ret < 0)
                {
                    if (errno == EINTR)
                        continue;
                    ls_stderr("Child process error reading data: %s\n", strerror(errno));
                    return -1;
                }
                else if (EOFOk)
                {
                    DEBUG_MESSAGE("readall, EOF (and it's ok)\n");
                    return 0;
                }
                else
                {
                    ls_stderr("Child process unexpected end of communications\n");
                    return -1;
                }
            }
        }
    }
    DEBUG_MESSAGE("readall, return %d\n", len);
    return len;
}

static int compareStrings(const void *a, const void *b) 
{
    // Cast the void pointers to char pointers
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;

    // Use strcmp for string comparison
    return strcmp(str1, str2);
}

static int validate(sandbox_init_req_t *req, char *argv, char ***argva, char *env, char ***enva)
{
    int pos = 0;
    DEBUG_MESSAGE("My pid: %d, my ppid: %d, recv pid: %d, recv ppid: %d, argc: %d, argv_len: %d, env_len: %d, nenv: %d\n",
                  getpid(), getppid(), req->m_pid, req->m_ppid, req->m_argc, req->m_argv_len, req->m_env_len, req->m_nenv);
    if (req->m_argc <= 0)
    {
        DEBUG_MESSAGE("Sandbox program invalid number of parameters %d\n", req->m_argc);
        ls_stderr("Child process invalid number of parameters %d\n", req->m_argc);
        return -1;
    }
    argv[req->m_argv_len] = 0;
    DEBUG_MESSAGE("For bsearch, argv: %s\n", argv);
    for (int i = 0; i < s_nosandbox_count; i++)
    {
        DEBUG_MESSAGE("s_sandbox_arr[%d]: %s\n", i, s_nosandbox_arr[i]);
    }
    if (!bsearch(&argv, s_nosandbox_arr, s_nosandbox_count, sizeof(char *), compareStrings))
    {
        DEBUG_MESSAGE("Sandbox program not in list: %s, breaking connection\n", argv);
        ls_stderr("Sandbox program not in list: %s, breaking connection\n", argv);
        return -1;
    }
    DEBUG_MESSAGE("bsearch, found: %s\n", argv);
    if (access(argv, X_OK) != 0)
    {
        int err = errno;
        DEBUG_MESSAGE("Sandbox program %s has issues: %s\n", argv, strerror(err));
        ls_stderr("Sandbox program %s has issues: %s\n", argv, strerror(err));
        return -1;
    }
    *argva = malloc(sizeof(char *) * (req->m_argc + 1));
    if (!*argva)
    {
        DEBUG_MESSAGE("Sandbox program memory shortage argca\n");
        ls_stderr("Sandbox program memory shortage argca\n");
        return -1;
    }
    DEBUG_MESSAGE("Build argva\n")
    for (int i = 0; i < req->m_argc; i++)
    {
        int len = strlen(&argv[pos]);
        if (len + pos > req->m_argv_len)
        {
            DEBUG_MESSAGE("Sandbox process unexpected overrun in argv #%d of %d, pos: %d, len: %d, full len: %d\n",
                          i, req->m_argc, pos, len, req->m_argv_len);
            ls_stderr("Sandbox process unexpected overrun in argv #%d of %d, pos: %d, len: %d, full len: %d\n",
                      i, req->m_argc, pos, len, req->m_argv_len);
            return -1;
        }
        (*argva)[i] = &argv[pos];
        DEBUG_MESSAGE(" argv[%d]: %s\n", i, &argv[pos])
        pos += (len + 1);
    }
    (*argva)[req->m_argc] = NULL;
    env[req->m_env_len] = 0;
    *enva = malloc(sizeof(char *) * (req->m_nenv + 1));
    if (!*enva)
    {
        DEBUG_MESSAGE("Sandbox program memory shortage enva\n");
        ls_stderr("Sandbox program memory shortage enva\n");
        return -1;
    }
    DEBUG_MESSAGE("Build enva\n")
    pos = 0;
    for (int i = 0; i < req->m_nenv; i++)
    {
        int len = strlen(&env[pos]);
        if (len + pos > req->m_env_len)
        {
            ls_stderr("Sandbox unexpected overrun in enva #%d of %d, pos: %d, len: %d, full len: %d\n",
                      i, req->m_nenv, pos, len, req->m_env_len);
            DEBUG_MESSAGE("Sandbox unexpected overrun in enva #%d of %d, pos: %d, len: %d, full len: %d\n",
                          i, req->m_nenv, pos, len, req->m_env_len);
            return -1;
        }
        (*enva)[i] = &env[pos];
        DEBUG_MESSAGE(" env[%d]: %s\n", i, &env[pos])
        pos += (len + 1);
    }
    (*enva)[req->m_nenv] = NULL;
    DEBUG_MESSAGE("validate ok\n")
    return 0;
}


static int set_uid_gid(uid_t uid, gid_t gid)
{
    int rv;

    DEBUG_MESSAGE("set_uid_gid: %d/%d\n", uid, gid);
    struct passwd *pw = getpwuid(uid);
    rv = setgid(gid);
    if (rv == -1)
    {
        ls_stderr("Cannot set gid for child: %s\n", strerror(errno));
        return -1;
    }
    if (pw && (pw->pw_gid == gid))
    {
        rv = initgroups(pw->pw_name, gid);
        if (rv == -1)
        {
            ls_stderr("Cannot do initgroups: %s\n", strerror(errno));
            return -1;
        }
    }
    else
    {
        rv = setgroups(1, &gid);
        if (rv == -1)
            ls_stderr("Cannot do setgroups (nonfatal): %s\n", strerror(errno));
    }
    rv = setuid(uid);
    if (rv == -1)
    {
        ls_stderr("Cannot set uid for child: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


static void apply_rlimits(sandbox_init_req_t *req)
{
#if defined(RLIMIT_AS) || defined(RLIMIT_DATA) || defined(RLIMIT_VMEM)
    setrlimit(RLIMIT_AS, &req->m_data);
#elif defined(RLIMIT_DATA)
    setrlimit(RLIMIT_DATA, &req->m_data);
#elif defined(RLIMIT_VMEM)
    setrlimit(RLIMIT_VMEM, &req->m_data);
#endif

#if defined(RLIMIT_NPROC)
    setrlimit(RLIMIT_NPROC, &req->m_nproc);
#endif

#if defined(RLIMIT_CPU)
    setrlimit(RLIMIT_CPU, &req->m_cpu);
#endif
}


int apply_rlimits_uid(sandbox_init_req_t *req)
{
    apply_rlimits(req);
    return set_uid_gid(req->m_uid, req->m_gid);
}


static int run_pgm(int fd, sandbox_init_req_t *req, int fdin[], int fdout[], 
                   int fderr[], char *argv[], char *env[], pid_t *pid)
{
    pipe(fdin);
    pipe(fdout);
    pipe(fderr);
    *pid = fork();
    if (*pid == 0)
    {
        // child
        close(fd);
        dup2(fdin[0], STDIN_FILENO);
        close(fdin[0]); close(fdin[1]);
        dup2(fdout[1], STDOUT_FILENO);
        close(fdout[0]); close(fdout[1]);
        dup2(fderr[1], STDERR_FILENO);
        close(fderr[0]); close(fderr[1]);
        if (apply_rlimits_uid(req))
            return -1;
        DEBUG_MESSAGE("running: %s\n", argv[0]);
        return execvpe(argv[0], argv, env);
    }
    else if (*pid == -1)
    {
        int err = errno;
        DEBUG_MESSAGE("Error starting sandbox program: %s\n", strerror(err));
        ls_stderr("Error starting sandbox program: %s\n", strerror(err));
        return -1;
    }
    s_listenPid = *pid;
    close(fdin[0]);
    close(fdout[1]);
    close(fderr[1]);
    DEBUG_MESSAGE("sandbox_pgm pid #%d\n", *pid);
    return 0;
}


static void pgm_done(int fd, pid_t pid, int fatal)
{
    int status;
    if (waitpid(pid, &status, 0) == -1)
    {
        DEBUG_MESSAGE("waitpid returned error %s\n", strerror(errno));
    }
    else
    {
        DEBUG_MESSAGE("waitpid returned status 0x%x, exited: %d, rc: %d\n", status, WIFEXITED(status), WEXITSTATUS(status));
    }
    sandbox_data_t sandbox_done;
    sandbox_done.m_sandbox_data_type = SANDBOX_DONE;
    sandbox_done.m_datalen_rc = status;
    DEBUG_MESSAGE("Final run_sandbox_pgm finishing up, fatal: %d, status: 0x%x\n", fatal, status);
    send(fd, &sandbox_done, sizeof(sandbox_done), 0);
}


static int read_sandbox_data(int fd, int fdin[], int fdout[], int fderr[], 
                             struct pollfd fds[], struct pollfd *pollout, 
                             int i, char *buffer, char *data, int buffersize, 
                             int *done)
{
    sandbox_data_t *sandbox_data = (sandbox_data_t *)buffer;
    int send_err = 0, pollcount;
    const char *err_loc;
    int bytes = read(fds[i].fd, i == 0 ? buffer : data, 
                     i == 0 ? (int)sizeof(sandbox_data_t) : buffersize);
    int err = errno;
    DEBUG_MESSAGE("recv %d bytes from index: %d\n", bytes, i);
    if (i == 0)
    {
        if (bytes <= 0)
        {
            DEBUG_MESSAGE("Sandbox lost connection with partner: %s\n", strerror(err));
            ls_stderr("Sandbox lost connection with partner: %s\n", strerror(err));
            return -1;           
        }
        DEBUG_MESSAGE("Sandbox recv type: %d, len: %d\n", sandbox_data->m_sandbox_data_type, sandbox_data->m_datalen_rc);
        bytes = sandbox_data->m_datalen_rc;
        if (!sandbox_data->m_datalen_rc)
        {
            DEBUG_MESSAGE("STDIN is done\n");
            close(fdin[1]);
            fdin[1] = -1;
        }
        else 
        {
            bytes = readall(fds[i].fd, data, bytes, 0);
            if (bytes <= 0)
            {
                DEBUG_MESSAGE("Sandbox lost connection with partner (data): %s\n", strerror(err));
                ls_stderr("Sandbox lost connection with partner (data): %s\n", strerror(err));
                return -1;
            }
            DEBUG_MESSAGE("Sandbox send fd %d stdin: %.*s\n", pollout->fd, bytes, data);
            while (!send_err && (pollcount = poll(pollout, 1, -1)) == -1 && errno != EINTR) 
            {
                send_err = 1;
                err_loc = "poll write stdin";
            }
            DEBUG_MESSAGE("STDIN data : %.*s\n", bytes, data)
            while (!send_err && write(pollout->fd, data, bytes) == -1 && errno != EINTR)
            {
                send_err = 1;
                err_loc = "write stdin";
            }
        }
    }
    else
    {
        if (bytes == 0)
        {
            *done = 1;
            return 0;
        }
        else if (bytes < 0) 
        {
            DEBUG_MESSAGE("Sandbox fd #%d error: %s\n", i, strerror(err));
            if (i == 1)
                fds[1] = fds[2];
        }
        else 
        {
            if (fds[i].fd == fdout[0])
                sandbox_data->m_sandbox_data_type = SANDBOX_STDOUT;
            else
                sandbox_data->m_sandbox_data_type = SANDBOX_STDERR;
            sandbox_data->m_datalen_rc = bytes;
            pollout->fd = fd;
            while (!send_err && (pollcount = poll(pollout, 1, -1)) == -1 && errno != EINTR)
            {
                send_err = 1;
                err_loc = (fds[i].fd == fdout[0]) ? "poll write stdout" : "poll write stderr";
            }
            DEBUG_MESSAGE("Sending on %s: %.*s\n", fds[i].fd == fdout[0] ? "STDOUT" : "STDERR", bytes, data)
            while (!send_err && send(fd, sandbox_data, sizeof(sandbox_data_t) + bytes, 0) == -1 && errno != EINTR)
            {
                send_err = 1;
                err_loc = (fds[i].fd == fdout[0]) ? "write stdout" : "write stderr";
            }
        }
    }
    if (send_err)
    {
        int err = errno;
        DEBUG_MESSAGE("Sandbox error in %s with child process: %s\n", err_loc, strerror(err));
        ls_stderr("Sandbox error in %s with child process: %s\n", err_loc, strerror(err));
        return -1;
    }
    return 0;
}


static int run_sandbox_pgm(int fd, sandbox_init_req_t *req, char *argv[], char *env[])
{
    DEBUG_MESSAGE("run_sandbox_pgm %s\n", argv[0]);
    int fdin[2], fdout[2], fderr[2];
    pid_t pid;
    if (run_pgm(fd, req, fdin, fdout, fderr, argv, env, &pid) == -1)
        return -1;
    int numfds = 3;
    struct pollfd fds[numfds], pollout;
    memset(fds, 0, sizeof(struct pollfd) * numfds);
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    fds[1].fd = fdout[0];
    fds[1].events = POLLIN;
    fds[2].fd = fderr[0];
    fds[2].events = POLLIN;
    pollout.fd = fdin[1];
    pollout.events = POLLOUT;
    int fatal = 0, done = 0;
    while (numfds > 1 && !done && !fatal)
    {
        int count = poll(fds, numfds, -1);
        if (count < 0)
        {
            if (errno != EINTR)
            {
                int err = errno;
                DEBUG_MESSAGE("Sandbox process got error in poll: %s\n", strerror(err));
                ls_stderr("Sandbox process got error in poll: %s\n", strerror(err));
                fatal = 1;
                break;
            }
            continue;
        }
        DEBUG_MESSAGE("poll returned count: %d\n", count);
        int buffersize = NOSANDBOX_BUF_SZ;
        int recv_count = 0;
        for (int i = 0; i < numfds; i++)
        {
            DEBUG_MESSAGE("[%d].revents: 0x%x\n", i, fds[i].revents);
            if (!count)
                break;
            if (fds[i].revents & (POLLIN | POLLHUP))
            {
                recv_count++;
                char buffer[buffersize + sizeof(sandbox_data_t)];
                char *data = &buffer[sizeof(sandbox_data_t)];
                count--;
                if (read_sandbox_data(fd, fdin, fdout, fderr, fds, &pollout, i, 
                                      buffer, data, buffersize, &done))
                {
                    fatal = 1;
                    break;
                }
            }
        }
        DEBUG_MESSAGE("recv_count: %d\n", recv_count);
        if (!recv_count)
        {
            DEBUG_MESSAGE("Received nothing!  BUG!\n");
            break;
        }
    }
    DEBUG_MESSAGE("run_sandbox_pgm finishing up, fatal: %d\n", fatal);
    if (done)
    {
        pgm_done(fd, pid, fatal);
    }
    close(fd);
    if (fdin[1] != -1)
        close(fdin[1]);
    close(fdout[0]);
    close(fderr[0]);
    return fatal ? -1 : 0;
}


static int sandbox_req(int fd)
{
    DEBUG_MESSAGE("sandbox_req\n");
    sandbox_init_req_t req;
    int rc = 0;
    if (readall(fd, (char *)&req, sizeof(req), 0) < 0)
        return -1;
    char *argv = malloc(req.m_argv_len + 1);
    char *env = malloc(req.m_env_len + 1);
    char **argva = NULL;
    char **enva = NULL;
    if (!argv || !env) {
        ls_stderr("Child process unable to allocate required data\n");
        if (argv)
            free(argv);
        if (env)
            free(env);
        return -1;
    }
    if (readall(fd, argv, req.m_argv_len, 0) < 0 || 
        readall(fd, env, req.m_env_len, 0) < 0 ||
        validate(&req, argv, &argva, env, &enva) ||
        run_sandbox_pgm(fd, &req, argva, enva))
        rc = -1;
    free(argva);
    free(enva);
    free(env);
    free(argv);
    return rc;
}


static int sandbox_conn(int socket_in)
{
    /* Normally close most things here, but I'm going to use much of them.*/
    ns_init_debug();
    DEBUG_MESSAGE("sandbox_conn: pid: %d\n", getpid());
    close(s_nosandbox_socket);
    s_nosandbox_socket = -1;
    return sandbox_req(socket_in);
}

static int new_sandbox_conn(int socket_in)
{
    pid_t pid;

    pid = fork();
    if (!pid)
        exit(sandbox_conn(socket_in));
    if (pid == -1)
    {
        ls_stderr("Error starting sandbox task: %s\n", strerror(errno));
        return -1;
    }
    close(socket_in);
    return 0;
}

static int nosandbox_main()
{
    int waiting = 1, rc = 0;
    struct pollfd pfd;
    pfd.fd = s_nosandbox_socket;
    pfd.events = POLLIN;
    pid_t entryppid = getppid();
    DEBUG_MESSAGE("nosandbox_main at pid: %d\n", getpid());
    while (waiting && !rc) 
    {
        int ret = poll(&pfd, 1, 1000);
        if (ret == 1)
        {
            int socket_in;
            if ((socket_in = accept(s_nosandbox_socket, NULL, 0)) == -1)
            {
                DEBUG_MESSAGE("accept error: %s\n", strerror(errno));
                break;
            }
            DEBUG_MESSAGE("Connected by remote within lscgid\n");
            if (new_sandbox_conn(socket_in))
                rc = -1;
        } else if (ret == -1)
        {
            int err = errno;
            DEBUG_MESSAGE("nosandbox_main poll error: %s\n", strerror(err));
            if (err == EINTR)
            {
                DEBUG_MESSAGE("nosandbox_main interrupted system call - continue:\n");
                continue;
            }
            rc = -1;
        }
        int status;
        int pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        {
            DEBUG_MESSAGE("Child pid %d terminated\n", pid);
        }
        if (getppid() != entryppid)
        {
            DEBUG_MESSAGE("Parent changed from %d to %d\n", entryppid, getppid());
            waiting = 0;
        }
    }
    DEBUG_MESSAGE("nosandbox_main stopped listening\n");
    return rc;
}


static void cleanup_old_sockets()
{
    char path[NOSANDBOX_MAX_FILE_LEN];
    char lsws_path[PERSIST_DIR_SIZE];
    snprintf(path, sizeof(path), NOSANDBOX_DIR_FMT, ns_lsws_home(lsws_path, sizeof(lsws_path)));
    DEBUG_MESSAGE("cleanup_old_sockets in %s\n", path)
    DIR *dir = opendir(path);
    if (!dir)
    {
        int err = errno;
        DEBUG_MESSAGE("Can't open old socket directory: %s: %s\n", path, strerror(err));
        return;
    }
    struct dirent *ent;
    errno = 0;
    while ((ent = readdir(dir)))
    {
        int pid;
        int scancount = sscanf(ent->d_name, NOSANDBOX_FILE_FMT, &pid);
        if (scancount == 1)
        {
            DEBUG_MESSAGE("Check pid: %d in %s\n", pid, ent->d_name);
            char proc_name[NOSANDBOX_MAX_FILE_LEN];
            snprintf(proc_name, sizeof(proc_name), "/proc/%d", pid);
            if (access(proc_name, 0))
            {
                char full_name[NOSANDBOX_MAX_FILE_LEN];
                snprintf(full_name, sizeof(full_name), NOSANDBOX_FORMAT, lsws_path, pid);
                DEBUG_MESSAGE("Deleting %s\n", full_name);
                unlink(full_name);
            }
        }
    }
    closedir(dir);
}


static int write_nosandbox_socket_file()
{
    char file_name[NOSANDBOX_MAX_FILE_LEN];
    int fd = open(nosandbox_socket_file(file_name, sizeof(file_name)), O_RDWR | O_CREAT, 0666);
    if (fd == -1)
    {
        int err = errno;
        DEBUG_MESSAGE("Can not open nosandbox file for write: %s: %s\n", file_name, strerror(err));
        ls_stderr("Can not open nosandbox file for write: %s: %s\n", file_name, strerror(err));
        return -1;
    }
    ftruncate(fd, 0);
    write(fd, s_nosandbox_name, strlen(s_nosandbox_name));
    close(fd);
    return 0;
}


static int open_nosandbox_socket() 
{
    DEBUG_MESSAGE("open_nosandbox_socket\n");
    int err = 0;
    cleanup_old_sockets();
    s_nosandbox_socket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (s_nosandbox_socket == -1)
    {
        err = errno;
        DEBUG_MESSAGE("Can not initialize no sandbox socket: %s\n", strerror(err));
        ls_stderr("Can not initialize no sandbox socket: %s\n", strerror(err));
        ns_done(0);
        return -1;
    }
    struct sockaddr_un loc;
    loc.sun_family = AF_UNIX;
    char pathname[sizeof(loc.sun_path) - 10];
    snprintf(loc.sun_path, sizeof(loc.sun_path), 
             NOSANDBOX_FORMAT, ns_lsws_home(pathname, sizeof(pathname)), getpid());
    DEBUG_MESSAGE("Use socket: %s (%s)\n", loc.sun_path, strerror(errno));
    unlink(loc.sun_path);
    if (bind(s_nosandbox_socket, (struct sockaddr *)&loc, 
             strlen(loc.sun_path) + sizeof(loc.sun_family)) != 0)
    {
        err = errno;
        DEBUG_MESSAGE("Can not bind on %s: %s\n", loc.sun_path, strerror(err));
        ls_stderr("Can not bind on %s: %s\n", loc.sun_path, strerror(err));
    }
    s_nosandbox_name = realpath(loc.sun_path, NULL);
    DEBUG_MESSAGE("Final socket name: %s\n", s_nosandbox_name);
    chmod(loc.sun_path, 0666);
    if (listen(s_nosandbox_socket, 5) != 0)
    {
        int err = errno;
        DEBUG_MESSAGE("Can not listen on %s: %s\n", loc.sun_path, strerror(err));
        ls_stderr("Can not listen on %s: %s\n", loc.sun_path, strerror(err));
        ns_done(0);
        return -1;
    }
    int pid = fork();
    if (!pid)
    {
        //strcpy(s_argv0, "lscgid (ns)");
        nosandbox_main();
        ns_done(0);
        exit(0);
    }
    close(s_nosandbox_socket);
    s_nosandbox_socket = -1;
    if (pid < 0)
    {
        err = errno;
        DEBUG_MESSAGE("Can not start listening process for %s: %s\n", loc.sun_path, strerror(err));
        ls_stderr("Can not start listening process for %s: %s\n", loc.sun_path, strerror(err));
        ns_done(0);
        return -1;
    }
    if (write_nosandbox_socket_file())
    {
        ns_done(0);
        return -1;
    }
    s_nosandbox_pid = pid;
    return 0;
}


static int save_links()
{
    if (s_nosandbox_count && nsnosandbox_symlink())
    {
        DEBUG_MESSAGE("save_links\n");
        s_nosandbox_arr_link = malloc(sizeof(char *) * s_nosandbox_count);
        int nosandbox_size = 0;
        if (!s_nosandbox_arr_link)
        {
            ls_stderr("Insufficient memory for link array");
            ns_done(0);
            return -1;
        }
        for (int i = 0; i < s_nosandbox_count; i++)
        {
            char f[NOSANDBOX_MAX_FILE_LEN] = { 0 };
            char *pf = f;
            struct stat statbuf;
            if (lstat(s_nosandbox_arr[i], &statbuf))
            {
                DEBUG_MESSAGE("%s lstat error: %s, don't sweat it for now\n", s_nosandbox_arr[i], strerror(errno));
            }
            else if ((statbuf.st_mode & S_IFMT) != S_IFLNK)
            {
                DEBUG_MESSAGE("%s not a link, put in a placeholder\n", s_nosandbox_arr[i]);
            }
            else if (!(pf = realpath(s_nosandbox_arr[i], f)))
            {
                DEBUG_MESSAGE("%s realpath error: %s\n", s_nosandbox_arr[i], strerror(errno));
            }
            else 
            {
                DEBUG_MESSAGE("%s realpath %s\n", s_nosandbox_arr[i], f);
                int lastpos = nosandbox_size;
                nosandbox_size += (strlen(f) + 1);
                s_nosandbox_link = realloc(s_nosandbox_link, nosandbox_size);
                if (!s_nosandbox_link)
                {
                    ls_stderr("Insufficient memory for links");
                    ns_done(0);
                    return -1;
                }
                strcpy(&s_nosandbox_link[lastpos], f);
            }
        }
    }
    return 0;
}


static int read_hostexec_files(char *filename, FILE *fh, int *no_sandbox_len)
{
    char line[NOSANDBOX_MAX_FILE_LEN];  
    DEBUG_MESSAGE("Reading hostexec files in %s\n", filename);
  
    /* Read in everything into 0 len strings and put the array of ptrs at the end (a final realloc) */
    while (fgets(line, sizeof(line) - 1, fh))
    {
        if (line[0] != '/')
            continue;
        int line_len = strlen(line);
        if (line[line_len - 1] == '\n')
        {
            line_len--;
            line[line_len] = 0;
        }
        if (line[line_len - 1] == '/')
        {
            ls_stderr("hostexec file (%s) contains invalid file: %s\n", filename, line);
            fclose(fh);
            ns_done(0);
            return -1;
        }
        if (access(line, X_OK) != 0)
        {
            ls_stderr("hostexec file (%s) contains problem file: %s: %s\n", filename, line, strerror(errno));
            fclose(fh);
            ns_done(0);
            return -1;
        }
        for (unsigned long i = 0; i < sizeof(s_bwrap_var) / sizeof(char *); i++)
        {
            if (strstr(line, s_bwrap_var[i])) {
                ls_stderr("hostexec file (%s) contains a file with a bubblewrap symbol: %s\n", filename, line);
                fclose(fh);
                ns_done(0);
                return -1;
            }
        }
        int last_len = *no_sandbox_len;
        (*no_sandbox_len) += (line_len + 1);  // To allow the string to be updated later
        s_nosandbox = realloc(s_nosandbox, *no_sandbox_len);
        if (!s_nosandbox) {
            ls_stderr("Insufficient memory creating hostexec table\n");
            fclose(fh);
            return -1;
        }
        memcpy(&s_nosandbox[last_len], line, line_len + 1);
        s_nosandbox_count++;
    }
    fclose(fh);
    DEBUG_MESSAGE("There are %d hostexec files\n", s_nosandbox_count);
    return 0;
}


int nsnosandbox_init()
{
    if (s_nosandbox)
        return 0;
    char pathname[NOSANDBOX_MAX_FILE_LEN / 2], filename[NOSANDBOX_MAX_FILE_LEN], 
         nosandbox_pgm[NOSANDBOX_MAX_FILE_LEN], nosandbox_force_symlink_file[NOSANDBOX_MAX_FILE_LEN];
    ns_lsws_home(pathname, sizeof(pathname));
    snprintf(nosandbox_force_symlink_file, sizeof(nosandbox_force_symlink_file),
             NOSANDBOX_FORCE_SYMLINK, pathname);
    if (!access(nosandbox_force_symlink_file, 0))
    {
        DEBUG_MESSAGE("Force symlink enabled\n");
        s_nosandbox_force_symlink = 1;
    }
    snprintf(filename, sizeof(filename), NOSANDBOX_CONF, pathname);
    FILE *fh = fopen(filename, "r");
    if (!fh)
    {
        DEBUG_MESSAGE("Error reading hostexec file: %s: %s, ok to not be there\n", 
                      filename, strerror(errno));
        snprintf(filename, sizeof(filename), NOSANDBOX2_CONF, pathname);
        fh = fopen(filename, "r");
        if (!fh)
        {
            DEBUG_MESSAGE("Error reading hostexec file: %s: %s, ok to not be there\n", 
                          filename, strerror(errno));
            return 0;
        }
    }
    int no_sandbox_len = 0;
    if (read_hostexec_files(filename, fh, &no_sandbox_len))
        return -1;
    if (s_nosandbox_count) 
    {
        snprintf(nosandbox_pgm, sizeof(nosandbox_pgm), NOSANDBOX_PGM, pathname);
        if (access(nosandbox_pgm, X_OK) != 0) 
        {
            ls_stderr("hostexec program (%s) is not an executable program: %s\n", 
                      nosandbox_pgm, strerror(errno));
            return -1;
        }
        if (!(s_nosandbox_pgm = strdup(nosandbox_pgm)))
        {
            ls_stderr("Insufficient memory to allocate program space\n");
            return -1;
        }
        int last_len = no_sandbox_len;
        no_sandbox_len += (s_nosandbox_count * sizeof(char *));
        s_nosandbox = realloc(s_nosandbox, no_sandbox_len);
        s_nosandbox_arr = (char **)&s_nosandbox[last_len];
        int pos = 0;
        for (int i = 0; i < s_nosandbox_count; i++)
        {
            s_nosandbox_arr[i] = &s_nosandbox[pos];
            pos += (strlen(s_nosandbox_arr[i]) + 1);
            DEBUG_MESSAGE("s_sandbox_arr[%d]: %s\n", i, s_nosandbox_arr[i]);
        }
        qsort(s_nosandbox_arr, s_nosandbox_count, sizeof(char *), compareStrings);
        DEBUG_MESSAGE("After sort There are %d hostexec files\n", s_nosandbox_count);
        for (int i = 0; i < s_nosandbox_count; i++)
        {
            DEBUG_MESSAGE("s_sandbox_arr[%d]: %s\n", i, s_nosandbox_arr[i]);
        }
        if (save_links())
            return -1;
        if (open_nosandbox_socket())
            return -1;
    }
    //ls_stderr("There are %d sandbox files excluded from the sandbox\n", s_nosandbox_count);
    return 0;
}


void nsnosandbox_resort()
{
    qsort(s_nosandbox_arr, s_nosandbox_count, sizeof(char *), compareStrings);
}


void nsnosandbox_done()
{
    DEBUG_MESSAGE("nsnosandbox_done\n");
    if (s_nosandbox_socket != -1)
    {
        close(s_nosandbox_socket);
        s_nosandbox_socket = -1;
    }
    free(s_nosandbox);
    s_nosandbox = NULL;
    free(s_nosandbox_pgm);
    s_nosandbox_pgm = NULL;
    if (s_nosandbox_pid)
    {
        DEBUG_MESSAGE("nsnosandbox_done, kill child: %d\n", s_nosandbox_pid);
        kill(s_nosandbox_pid, SIGKILL);
        s_nosandbox_pid = 0;
    }
    free(s_nosandbox_name);
    s_nosandbox_name = NULL;
}


int nsnosandbox_symlink()
{
    return s_nosandbox_force_symlink || (s_ns_osmajor < 5 || (s_ns_osmajor == 5 && s_ns_osminor < 12));
}

