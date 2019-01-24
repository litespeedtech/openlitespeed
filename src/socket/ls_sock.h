/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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

#ifndef __LS_SOCK_H__
#define __LS_SOCK_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef __cplusplus 
extern "C" {
#endif
/**
 * @file ls_sock.h
 * @brief Include file for library which allows you to use both native and 
 * user-land sockets with the same basic API (some minor changes, see below).
 */

/**
 * @mainpage
 * liblssock
 * User reference for compatibility library for native and user-land sockets.
 * @version 1.0
 * @date 2013-2018
 * @author LiteSpeed Development Team
 * @copyright LiteSpeed Technologies, Inc. and the GNU Public License.
 */

/**
 * @def _USE_USER_SOCK       
 * @brief If defined, then user-land sockets are enabled.  
 */

#ifdef _USE_USER_SOCK

#include "ls_usocket.h"

#else //!_USE_USER_SOCK
#define ls_sock_init(a,b,c)
#define ls_run(a,b)         while (1) a(b)

#ifdef __cplusplus 
#define ls_bind(a,b,c)      ::bind(a,b,c)
#define ls_connect(a,b,c)   ::connect(a,b,c)
#define is_usersock(sockfd) 0
#define real_sockfd(sockfd) sockfd
#define log_sockfd(sockfd)  sockfd
#define ls_socket           ::socket
#define ls_socketpair       ::socketpair
#define ls_fcntl            ::fcntl
#define ls_ioctl            ::ioctl
#define ls_getsockopt       ::getsockopt
#define ls_setsockopt       ::setsockopt
#define ls_listen           ::listen
#define ls_accept           ::accept
#define ls_close            ::close
#define ls_shutdown         ::shutdown
#define ls_getpeername      ::getpeername
#define ls_getsockname      ::getsockname
#define ls_read             ::read
#define ls_readv            ::readv
#define ls_write            ::write
#define ls_writev           ::writev
#define ls_send             ::send
#define ls_sendto           ::sendto
#define ls_sendmsg          ::sendmsg
#define ls_recv             ::recv
#define ls_recvfrom         ::recvfrom
#define ls_recvmsg          ::recvmsg
#define ls_select           ::select
#define ls_poll1            ::poll
#define ls_kpoll            ::poll
#define ls_upoll            ::poll
#define ls_epoll_create     ::epoll_create
#define ls_epoll_ctl        ::epoll_ctl
#else
#define ls_bind(a,b,c)      bind(a,b,c)
#define ls_connect(a,b,c)   connect(a,b,c)
#define is_usersock(sockfd) 0
#define real_sockfd(sockfd) sockfd
#define log_sockfd(sockfd)  sockfd
#define ls_socket           socket
#define ls_socketpair       socketpair
#define ls_fcntl            fcntl
#define ls_ioctl            ioctl
#define ls_getsockopt       getsockopt
#define ls_setsockopt       setsockopt
#define ls_listen           listen
#define ls_accept           accept
#define ls_close            close
#define ls_shutdown         shutdown
#define ls_getpeername      getpeername
#define ls_getsockname      getsockname
#define ls_read             read
#define ls_readv            readv
#define ls_write            write
#define ls_writev           writev
#define ls_send             send
#define ls_sendto           sendto
#define ls_sendmsg          sendmsg
#define ls_recv             recv
#define ls_recvfrom         recvfrom
#define ls_recvmsg          recvmsg
#define ls_select           select
#define ls_poll1            poll
#define ls_kpoll            poll
#define ls_upoll            poll
#define ls_epoll_create     epoll_create
#define ls_epoll_ctl        epoll_ctl
#endif // cplusplus
#define ls_kqueue           #error kqueue doesnt exist for kernel functions
#define ls_kevent           #error kevent doesnt exist for kernel functions
#endif //_USE_USER_SOCK
#ifdef __cplusplus
}
#endif
#endif

