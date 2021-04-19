/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2021  LiteSpeed Technologies, Inc.                 *
*                                                                            *
*    This program is free software: you can redistribute it and/or modify    *
*    it under the terms of the GNU General Public License as published by    *
*    the Free Software Foundation, either version 3 of the License, or       *
*    (at your option) any later version.                                     *
*                                                                            *
*    This program is distributed in the hope that it will be useful,         *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of          *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
*    GNU General Public License for more details.                            *
*                                                                            *
*    You should have received a copy of the GNU General Public License       *
*    along with this program. If not, see http://www.gnu.org/licenses/.      *
*****************************************************************************/

#include "coresocket.h"

#include "gsockaddr.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <string.h>
#include <unistd.h>
#ifdef _USE_F_STACK
#include <sys/ioctl.h>
#endif

#ifndef TCP_FASTOPEN

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#define TCP_FASTOPEN 23

#elif defined(__FreeBSD__)

#define TCP_FASTOPEN 1025

#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)

#define TCP_FASTOPEN 0x105

#endif

#endif // !defined(TCP_FASTOPEN)


static int s_ipv6only = 0;


CoreSocket::CoreSocket(int domain, int type, int protocol, int user)
{
    int fd;
    if (user)
        fd = ls_socket(domain, type, protocol);
    else
        fd = ::socket(domain, type, protocol);

    setfd(fd);
}


CoreSocket::~CoreSocket()
{
    if (getfd() != INVALID_FD)
        close();
}


/**
  * @return 0, succeed
  *         -1,  invalid URL.
  *         EINPROGRESS, non blocking socket connecting are in progress
  *         Other error code occured when call socket(), connect().
*/

int  CoreSocket::connect(const char *pURL, int iFLTag, int *fd,
                         int dnslookup,
                         int flag, const GSockAddr *pLocalAddr)
{
    int ret;
    GSockAddr server;
    *fd = -1;
    int tag = NO_ANY;
    if (dnslookup)
        tag |= DO_NSLOOKUP;
    ret = server.set(pURL, tag);
    if (ret != 0)
        return -1;
    return connect(server, iFLTag, fd, flag, pLocalAddr);
}


int  CoreSocket::connect(const GSockAddr &server, int iFLTag, int *fd,
                         int flag, const GSockAddr *pLocalAddr)
{
    int type = SOCK_STREAM;
    int ret;
    int user = (flag & LS_SOCK_USERSOCKET) != 0;
    if ((user) && (server.family() != AF_INET) && (server.family() != AF_INET6))
    {
        errno = EINVAL;
        return -1;
    }    
#if !defined(_USE_F_STACK) || !(defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__))
    if (user)
    {
        errno = EINVAL;
        return -1;
    }
#endif

    /**
     * If serverAddr not initialized, the m_len is not set to right value
     */
    if (server.len() < 16)
        return -1;

#ifdef _USE_F_STACK    
    if (user)
        *fd = ls_socket_fstack(server.family(), type, 0);
    else
#endif        
        *fd = ::socket(server.family(), type, 0);
    
    if (*fd == -1)
        return -1;
    if ((iFLTag) && !user)
        ::fcntl(*fd, F_SETFL, iFLTag);

#ifdef _USE_F_STACK    
    if (user)
    {
        int on = 1;
        ls_ioctl(*fd, FIONBIO, &on);
    }
#endif
    int val = 1;
    if ((flag & LS_SOCK_NODELAY) &&
        ((server.family() == AF_INET) || (server.family() == AF_INET6)))
        ls_setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(int));
    if (pLocalAddr)
    {
        if (ls_setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR,
                      (char *)(&val), sizeof(val)) == 0)
            ls_bind(*fd, pLocalAddr->get(), pLocalAddr->len());
    }
    ret = ls_connect(*fd, server.get(), server.len());
    if (ret != 0)
    {
        if (!(((user) || (iFLTag & O_NONBLOCK)) && (errno == EINPROGRESS)))
        {
            ls_close(*fd);
            *fd = -1;
        }
        return -1;
    }
    return ret;
}

#ifdef _USE_USER_SOCK
// Only for user mode sockets with return code -1 and errno == EINPROGRESS.
int  CoreSocket::connect_finish(int fd)
{
    struct pollfd pfd;
    int rc;
    pfd.fd = fd;
    pfd.events = POLLOUT;
    pfd.revents = 0;
    rc = ls_poll1(&pfd, 1, -1);
    if (rc == -1)
        return -1;
    if (!(pfd.revents & POLLOUT))
    {
        errno = EINPROGRESS;
        return -1; // Not ready yet.
    }
    // NOW check the return code of the connect
    socklen_t errlen = sizeof(int);
    int err = 0;
    if (ls_getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1)
        return -1;
    if (err > 0)
    {
        errno = err;
        return -1;
    }
    return 0;
}
#endif


int CoreSocket::listen(const char *pURL, int backlog, int *fd, int flag,
                       int sndBuf, int rcvBuf)
{
    int ret;
    GSockAddr server;
    ret = server.set(pURL, 0);
    if (ret != 0)
        return -1;
    return listen(server, backlog, fd, flag, sndBuf, rcvBuf);
}


int CoreSocket::listen(const GSockAddr &server, int backLog, int *fd,
                       int flag, int sndBuf, int rcvBuf)
{
    int ret;
#if !defined(_USE_F_STACK) || !(defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__))
    if (flag & LS_SOCK_USERSOCKET)
    {
        errno = EINVAL;
        return -1;
    }
#endif    
    ret = bind(server, SOCK_STREAM, fd, flag);
    if (ret)
        return ret;

    if (sndBuf > 4096)
        ls_setsockopt(*fd, SOL_SOCKET, SO_SNDBUF, &sndBuf, sizeof(int));
    if (rcvBuf > 4096)
        ls_setsockopt(*fd, SOL_SOCKET, SO_RCVBUF, &rcvBuf, sizeof(int));
    if ((server.family() == AF_INET)
        || (server.family() == AF_INET6))
    {
        int val = 1;
        if (flag & LS_SOCK_NODELAY)
            ls_setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(int));

#ifdef TCP_FASTOPEN
        if (!(flag & LS_SOCK_USERSOCKET))
        {
            val = backLog / 2;
            if (val > 256)
                val = 256;
            ls_setsockopt(*fd, IPPROTO_TCP, TCP_FASTOPEN, &val, sizeof(int));
        }
#endif
        
    }

    ret = ls_listen(*fd, backLog);

    if (ret == 0)
        return 0;
    ret = errno;
    ls_close(*fd);
    *fd = -1;
    return ret;

}


int CoreSocket::bind(const GSockAddr &server, int type, int *fd, 
                     int flag)
{
    int ret;
    if (!server.get())
        return EINVAL;
#ifdef _USE_USER_SOCK
    if (flag & LS_SOCK_USERSOCKET)
        *fd = ls_socket_fstack(server.family(), type, 0);
    else
        *fd = ::socket(server.family(), type, 0);
    if (*fd == -1)
        return errno;
    if (flag & LS_SOCK_USERSOCKET)
    {
        int on = 1;
        ls_ioctl(*fd, FIONBIO, &on);
    }
#else
    *fd = ::socket(server.family(), type, 0);
    if (*fd == -1)
        return errno;
#endif
    int val = 1;
    if (ls_setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR,
                      (char *)(&val), sizeof(val)) != 0)
        goto ERROR;
    if ((flag & LS_SOCK_REUSEPORT)
        && ls_setsockopt(*fd, SOL_SOCKET, SO_REUSEPORT,
                      (char *)(&val), sizeof(val)) != 0)
        goto ERROR;
#if defined(IPV6_V6ONLY) 
    if (!(flag & LS_SOCK_USERSOCKET) &&
        server.family() == AF_INET6
        && IN6_IS_ADDR_UNSPECIFIED(&(server.getV6()->sin6_addr)))
    {
        setsockopt(*fd, IPPROTO_IPV6, IPV6_V6ONLY,
                    (char *)(&s_ipv6only), sizeof(s_ipv6only));
    }
#endif
    ret = ls_bind(*fd, server.get(), server.len());
    if (!ret)
        return ret;
#if defined(IPV6_V6ONLY)
    if (!(flag & LS_SOCK_USERSOCKET) &&
        EADDRINUSE == errno && !s_ipv6only && server.family() == AF_INET6
        && IN6_IS_ADDR_UNSPECIFIED(&(server.getV6()->sin6_addr)))
    {
        if (ls_setsockopt(*fd, IPPROTO_IPV6, IPV6_V6ONLY,
                            (char *)(&val), sizeof(val)) == 0)
        {
            ret = ls_bind(*fd, server.get(), server.len());
            if (!ret)
                return ret;
        }
    }
#endif

ERROR:
    ret = errno;
    ls_close(*fd);
    *fd = -1;
    return ret;

}


int CoreSocket::close()
{
    int iRet;
    int fd = getfd();
    for (int i = 0; i < 3; i++)
    {
        iRet = ls_close(fd);
        if (iRet != EINTR)   // not interupted
        {
            setfd(INVALID_FD);
            return iRet;
        }
    }
    return iRet;
}


int CoreSocket::enableFastOpen(int fd, int queLen)
{
    int ret = 1;
#ifdef _USE_F_STACK    
    if (is_usersock(fd))
        return -1;
#endif    
#if defined(linux) || defined(__linux) || defined(__linux__)
#ifndef TCP_FASTOPEN
#define TCP_FASTOPEN 23
#endif
    static int isTfoAvail = 1;
    if (!isTfoAvail)
        return -1;
    
    ret = ls_setsockopt(fd, SOL_TCP, TCP_FASTOPEN, &queLen, sizeof(queLen));
    if ((ret == -1)&&(errno == ENOPROTOOPT))
        isTfoAvail = 0;
#endif
    return ret;
}


void CoreSocket::setIpv6Only(int only)
{
    s_ipv6only = (only != 0);
}


int CoreSocket::getIpv6Only()
{
    return s_ipv6only;
}


