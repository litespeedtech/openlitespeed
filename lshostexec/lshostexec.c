/*
 * Copyright 2024 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
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

#define LSNOSANDBOX
#define NS_MAIN // To get in debugging
#include "lshostexec.h"
#include "ns.h"
#include "nsopts.h"
#include "nsnosandbox.h"
#include "use_bwrap.h"

int         s_ns_debug = 0;
FILE       *s_ns_debug_file = NULL;
char       *s_realpath = NULL;
extern char **environ;


void ls_stderr(const char * fmt, ...)
{
    char buf[1024];
    char *p = buf;
    struct timeval  tv;
    struct tm       tm;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    p += snprintf(p, 1024, "%04d-%02d-%02d %02d:%02d:%02d.%06d ",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec, (int)tv.tv_usec);
    fprintf(stderr, "%.*s", (int)(p - buf), buf);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

static int rebuild_argv0(char *argv[])
{
    if (*argv[0] == '/')
        return 0;
    if (realpath(argv[0], s_realpath))  
    {
        DEBUG_MESSAGE("realpath %s -> %s\n", argv[0], s_realpath);
        argv[0] = s_realpath;
        return 0;
    } 
    DEBUG_MESSAGE("realpath failed for %s: %s, PATH: %s\n", argv[0], 
                  strerror(errno), getenv("PATH"));
    char *PATH = getenv("PATH");
    if (!PATH)
    {
        ls_stderr("Program is not found and no PATH to follow\n");
        return -1;
    }
    char *path = strdup(PATH);
    if (!path)
    {
        ls_stderr("Insufficient memory for even a path: %s\n", PATH);
        return -1;
    }
    char *allocpath = path;
    int argvlen = strlen(argv[0]);
    while (*path)
    {
        char *colon = strchr(path, ':');
        if (colon)
            *colon = 0;
        int path_len = strlen(path);
        char fullpath[path_len + 1 + argvlen + 1];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, argv[0]);
        DEBUG_MESSAGE("Try %s\n", fullpath);
        if (!access(fullpath, 0)) 
        {
            s_realpath = strdup(fullpath);
            if (!s_realpath)
            {
                ls_stderr("Insufficient memory for even a resolved path: %s\n", fullpath);
                free(path);
                return -1;
            }
            break;
        }
        if (colon)
            path = colon + 1;
        else
            break;
    }
    free(allocpath);
    DEBUG_MESSAGE("Resolved path: %s\n", s_realpath);
    argv[0] = s_realpath;
    return 0;
}

static int build_req(int argc, char *argv[], sandbox_init_req_t *req)
{
    DEBUG_MESSAGE("build_req, pgm: %s\n", argv[0]);
    memset(req, 0, sizeof(*req));
    if (rebuild_argv0(argv))
        return -1;
    req->m_pid = getpid();
    req->m_ppid = getppid();
    req->m_uid = getuid();
    req->m_gid = getgid();
    getrlimit(RLIMIT_AS, &req->m_data);
    getrlimit(RLIMIT_NPROC, &req->m_nproc);
    getrlimit(RLIMIT_CPU, &req->m_cpu);
    req->m_argc = argc;
    for (int i = 0; i < argc; i++)
        req->m_argv_len += (strlen(argv[i]) + 1);
    for (req->m_nenv = 0; environ[req->m_nenv]; req->m_nenv++)
        req->m_env_len += (strlen(environ[req->m_nenv]) + 1);
    return 0;
}

static int send_req(sandbox_init_req_t *req, char *argv[], int *psock)
{
    int sock = -1;
    DEBUG_MESSAGE("send_req\n");
    if ((sock = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
    {
        ls_stderr("Unable to create IPC socket for no-sandbox LiteSpeed: %s\n", strerror(errno));
        return -1;
    }
    struct sockaddr_un remote;
    if (nsnosandbox_socket_file_name(remote.sun_path, sizeof(remote.sun_path)))
    {
        close(sock);
        return -1;
    }
    int len = strlen(remote.sun_path);
    remote.sun_family = AF_UNIX;
    while (connect(sock, (struct sockaddr *)&remote, len + sizeof(remote.sun_family)) == -1 &&
           errno != EAGAIN)
    {
        int err = errno;
        DEBUG_MESSAGE("Error connecting to client socket: %s for no-sandbox LiteSpeed: %s\n", 
                      remote.sun_path, strerror(err));
        ls_stderr("Error connecting to client socket: %s for no-sandbox LiteSpeed: %s\n", 
                  remote.sun_path, strerror(err));
        close(sock);
        return -1;
    }
    DEBUG_MESSAGE("connect successful\n");
    int send_err = 0;
    struct pollfd sendpfd;
    sendpfd.fd = sock;
    sendpfd.events = POLLOUT;
    while (!send_err)
        if (poll(&sendpfd, 1, -1) == -1 && errno != EINTR)
            send_err = 1;
        else if (errno != EINTR &&
                 send(sock, req, sizeof(*req), 0) == -1 && 
                      errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            send_err = 1;
        else
            break;
    DEBUG_MESSAGE("sent req %d bytes\n", (int)sizeof(*req));
    if (!send_err)
    {
        int b = 0;
        for (int i = 0; i < req->m_argc; i++)
        {
            DEBUG_MESSAGE("argv[%d]: %s\n", i, argv[i]);
            b += (strlen(argv[i]) + 1);
            if (poll(&sendpfd, 1, -1) == -1 && errno != EINTR)
                send_err = 1;
            else if (errno != EINTR &&
                     send(sock, argv[i], strlen(argv[i]) + 1, 0) == -1 && 
                          errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            {
                send_err = 1;
                break;
            }
        }
        DEBUG_MESSAGE("Expected to send %d bytes, sent %d\n", req->m_argv_len, b);
    }
    if (!send_err)
    {
        int b = 0;
        for (int i = 0; environ[i]; i++)
        {
            DEBUG_MESSAGE("  send environ[%d]: %s\n", i, environ[i]);
            b += (strlen(environ[i]) + 1);
            if (poll(&sendpfd, 1, -1) == -1 && errno != EINTR)
                send_err = 1;
            else if (errno != EINTR &&
                     send(sock, environ[i], strlen(environ[i]) + 1, 0) == -1 &&
                          errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            {
                send_err = 1;
                break;
            }
        }
        DEBUG_MESSAGE("Expected to send env %d bytes, sent %d\n", req->m_env_len, b);
    }
    if (send_err)
    {
        ls_stderr("Error sending to client socket: %s for no-sandbox LiteSpeed: %s\n", 
                  remote.sun_path, strerror(errno));
        close(sock);
        return -1;
    }
    *psock = sock;
    return 0;
}

static int ready_fd(struct pollfd *fds, int numfds, int *foundfds)
{
    while ((*foundfds = poll(fds, numfds, -1)) == -1 && errno == EINTR)
        continue;
    if (*foundfds <= 0)
    {
        ls_stderr("Unexpected numfds %d checking %s errno: %s\n", 
                  *foundfds, fds->events == POLLIN ? "READ" : "WRITE", strerror(errno));
        return -1;
    }
    return 0;
}

static int recv_fd(struct pollfd *fds, char *d, int len, int any, int close_ok)
{
    int bytes, sofar = 0;

    while (any || sofar < len)
    {
        while ((bytes = read(fds->fd, &d[sofar], len - sofar)) == -1 && errno == EINTR)
        {
            DEBUG_MESSAGE("Retry recv\n");
            continue;
        }
        if (bytes == -1)
        {
            int err = errno;
            DEBUG_MESSAGE("Error receiving: %s, fd: %d\n", strerror(err), fds->fd);
            ls_stderr("Error receiving: %s, fd: %d\n", strerror(err), fds->fd);
            return -1;
        }
        else if (bytes == 0)
        {
            DEBUG_MESSAGE("Closed connection\n");
            if (close_ok)
                return 0;
            else
            {
                DEBUG_MESSAGE("Unexpected end of connection\n");
                ls_stderr("Unexpected end of connection\n");
                return -1;
            }
        }
        else 
        {
            DEBUG_MESSAGE("Read: %d bytes\n", bytes);
        }
        sofar += bytes;
        if (any)
            break;
    }
    return sofar;
}

static int forward_data(struct pollfd *fdout, char *data, int totalbytes)
{
    int fds;
    if (ready_fd(fdout, 1, &fds))
        return -1;
    if (send(fdout->fd, data, totalbytes, 0) == -1)
    {
        int err = errno;
        DEBUG_MESSAGE("Error forwarding STDIN data: %s\n", strerror(err));
        ls_stderr("Error forwarding STDIN data: %s\n", strerror(err));
        return -1;
    }
    return 0;
}

static int read_fds(sandbox_init_req_t * req, int sock, struct pollfd *fds, 
                    struct pollfd *fdout, int *numfds, int *done, int *rc)
{
    int buffersize = NOSANDBOX_BUF_SZ, foundfds;
    char buffer[buffersize + sizeof(sandbox_data_t)];
    if (ready_fd(fds, *numfds, &foundfds))
        return -1;
    for (int i = 0; i < *numfds; i++)
    {
        int bytes;
        sandbox_data_t *datastr = (sandbox_data_t *)buffer;
        if (fds[i].revents == 0)
            continue;
        if (i == 0)
        {
            DEBUG_MESSAGE("recv header, i: %d, revents: %d, fd: %d\n", i, fds[i].revents, fds[i].fd);
            if ((bytes = recv_fd(&fds[i], buffer, sizeof(*datastr), 0, 0)) < 0)
                return bytes;
            DEBUG_MESSAGE("Received header from remote, type: %d, len/rc: %d\n", datastr->m_sandbox_data_type, datastr->m_datalen_rc);
            if (datastr->m_sandbox_data_type == SANDBOX_DONE)
            {
                *done = 1;
                int rc = 0xff;
                if (WIFEXITED(datastr->m_datalen_rc))
                {
                    rc = WEXITSTATUS(datastr->m_datalen_rc);
                    DEBUG_MESSAGE("Final rc: %d\n", WEXITSTATUS(datastr->m_datalen_rc));
                } else if (WIFSIGNALED(datastr->m_datalen_rc))
                {
                    rc = WTERMSIG(datastr->m_datalen_rc);
                    DEBUG_MESSAGE("signal rc: %d\n", WEXITSTATUS(datastr->m_datalen_rc));
                } else 
                {
                    DEBUG_MESSAGE("Has not finished, but report done\n");
                    rc = 0;
                }
                return rc;
            }
        }
        char *data = &buffer[sizeof(sandbox_data_t)];

        DEBUG_MESSAGE("recv data, i: %d, revents: %d, fd: %d\n", i, fds[i].revents, fds[i].fd);
        if ((bytes = recv_fd(&fds[i], data, i == 0 ? datastr->m_datalen_rc : NOSANDBOX_BUF_SZ, 
                             i == 1, i == 1)) == -1)
            return bytes;

        if (i == 0)
        {
            if (write(datastr->m_sandbox_data_type == SANDBOX_STDOUT ? STDOUT_FILENO : STDERR_FILENO, data, bytes) == -1)
            {
                int err = errno;
                DEBUG_MESSAGE("Error writing %s: %s\n", datastr->m_sandbox_data_type == SANDBOX_STDOUT ? "stdout" : "stderr", strerror(err));
                ls_stderr("Error writing %s: %s\n", datastr->m_sandbox_data_type == SANDBOX_STDOUT ? "stdout" : "stderr", strerror(err));
                return -1;
            }
        }
        else
        {
            datastr->m_sandbox_data_type = SANDBOX_STDIN;
            datastr->m_datalen_rc = bytes;
            if (bytes == 0)
                (*numfds)--;
            DEBUG_MESSAGE("stdin read: %.*s\n", bytes, data);
            if (forward_data(fdout, (char *)datastr, bytes + sizeof(*datastr)) == -1)
                return -1;
        }
    }
    return 0;
}


static int process_req(sandbox_init_req_t *req, int sock)
{
    int numfds = 2, done = 0, fatal = 0, rc = 0;
    struct pollfd fds[numfds], fdout;
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;
    fdout.events = POLLOUT;
    fdout.fd = sock;
    while (!fatal && !done)
    {
        DEBUG_MESSAGE("process_req, read_fds\n");
        fatal = read_fds(req, sock, fds, &fdout, &numfds, &done, &rc);
    }
    if (done)
        return rc;
    return 1;
}

int main(int argc, char *argv[])
{
    sandbox_init_req_t req;
    int sock, rc = 0;
    ns_init_debug();
    DEBUG_MESSAGE("Enter lsnosandbox, argv[0]: %s\n", argv[0]);
    char *slash = strrchr(argv[0], '/');
    if ((slash && !strcmp(slash + 1, NOSANDBOX_PGM_ONLY)) || 
        (!slash && !strcmp(argv[0], NOSANDBOX_PGM_ONLY)))
    {
        DEBUG_MESSAGE("This program should only be started in a sandbox, by a no-sandbox program\n");
        ls_stderr("This program should only be started in a sandbox, by a no-sandbox program\n");
        rc = 1;
    }
    else if (build_req(argc, argv, &req) || send_req(&req, argv, &sock) || process_req(&req, sock))
        rc = 1;
    free(s_realpath);
    return rc;
}


