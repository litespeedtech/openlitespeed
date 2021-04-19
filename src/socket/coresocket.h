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

#ifndef CORESOCKET_H
#define CORESOCKET_H

#include <sys/types.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <socket/sockdef.h>
#include <socket/ls_sock.h>

/**
  *It is a wrapper for socket related system function call.
  *@author George Wang
  */

#define INVALID_FD  -1

/* These two flags can be specified in the 'nodelay' option of listen or 
 * connect.  They can be combined.  Note that if LS_SOCK_USERSOCKET is
 * specified, it is automatically non-blocking, so be ready for that.  */
/* User socket "connect" note: if the connect fails with an errno of 
 * EINPROGRESS you will need to return control to the event loop and 
 * complete the function by calling connect_finish().  */
#define LS_SOCK_NODELAY         1
#define LS_SOCK_USERSOCKET      2
#define LS_SOCK_REUSEPORT       4

class GSockAddr;
class CoreSocket
{
private:    //class members
    int m_ifd;      //file descriptor
protected:

    CoreSocket(int fd = INVALID_FD)
        : m_ifd(fd)
    {}
    CoreSocket(int domain, int type, int protocol = 0, int user = 0);
    void    setfd(int ifd) { m_ifd = ifd;  }

public:
    enum
    {
        FAIL = -1,
        SUCCESS = 0
    };

    virtual ~CoreSocket();

    int     getfd() const   { return m_ifd;     }

    int     close();

    int     getSockName(SockAddr *name, socklen_t *namelen)
    {   return ls_getsockname(m_ifd, name, namelen);   }

    int     getSockOpt(int level, int optname, void *optval, socklen_t *optlen)
    {   return ls_getsockopt(m_ifd, level, optname, optval, optlen);   }

    int     setSockOpt(int level, int optname, const void *optval,
                       socklen_t optlen)
    {   return ls_setsockopt(m_ifd, level, optname, optval, optlen);   }

    int     setSockOpt(int optname, const void *optval, socklen_t optlen)
    {   return setSockOpt(SOL_SOCKET, optname, optval, optlen);         }

    int     setSockOptInt(int optname, int val)
    {   return setSockOpt(optname, (void *)&val, sizeof(val));  }

    int     setTcpSockOptInt(int optname, int val)
    {   return ls_setsockopt(m_ifd, IPPROTO_TCP, optname, &val, sizeof(int)); }

    int     setReuseAddr(int reuse)
    {   return setSockOptInt(SO_REUSEADDR, reuse);  }

    int     setRcvBuf(int size)
    {   return setSockOptInt(SO_RCVBUF, size);   }

    int     setSndBuf(int size)
    {   return setSockOptInt(SO_SNDBUF, size);   }

    static int  connect(const char *pURL, int iFLTag, int *fd,
                        int dnslookup = 0,
                        int flag = LS_SOCK_NODELAY,
                        const GSockAddr *pLocalAddr = NULL);
    static int  connect(const GSockAddr &server, int iFLTag, int *fd,
                        int flag = LS_SOCK_NODELAY,
                        const GSockAddr *pLocalAddr = NULL);
    /* connect_finish is only used for user-land sockets and is only required
     * if the connect returns -1 with errno == EINPROGRESS */
    static int  connect_finish(int fd);
    static int  bind(const GSockAddr &server, int type, int *fd, 
                     int flag = 0);
    static int  listen(const char *pURL, int backlog, int *fd,
                       int flag = LS_SOCK_NODELAY,
                       int sndBuf = -1, int rcvBuf = -1);
    static int  listen(const GSockAddr &addr, int backlog, int *fd,
                       int flag = LS_SOCK_NODELAY,
                       int sndBuf = -1, int rcvBuf = -1);
    static int  cork(int fd)
    {
#if !defined(USE_FSTACK) && (defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__))
        int cork = 1;
        return ::setsockopt(fd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(int));

#elif defined(__FreeBSD__)
        int nopush = 1;
        return ::setsockopt(fd, IPPROTO_TCP, TCP_NOPUSH, &nopush, sizeof(int));
#else
        int nodelay = 0;
        return ls_setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int));
#endif

    }
    static int  uncork(int fd)
    {
#if defined(__FreeBSD__)
        int nopush = 0;
        return ::setsockopt(fd, IPPROTO_TCP, TCP_NOPUSH, &nopush, sizeof(int));
        //return ::setsockopt( fd, IPPROTO_TCP, TCP_NOPUSH, &nopush, sizeof( int ) );
#endif
//#else
        int nodelay = 1;
        return ls_setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int));
//#endif
    }
    
    static int enableFastOpen(int fd, int queLen);

    static void setIpv6Only(int only);
    static int  getIpv6Only();
};

#endif
