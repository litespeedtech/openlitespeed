/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#include "pipenotifier.h"
#include <../../http/httpsession.h>
#include <unistd.h>
#include <fcntl.h>
#include "../../edio/multiplexer.h"


PipeNotifier::~PipeNotifier()
{
    if (m_fdIn != -1)
        close(m_fdIn);

    if (getfd() != -1)
        close(getfd());
}

int PipeNotifier::handleEvents(short int event)
{
    char achBuf[50];
    int count = 0, len = 0;

    if (event & POLLIN)
    {
        while ((len = read(getfd(), achBuf, sizeof(achBuf) / sizeof(char))) > 0)
            count += len;

        g_api->log(NULL, LSI_LOG_DEBUG,
                   "[PipeNotifier] handleEvents called, fd = %d, read %d byte(s), session=%ld\n ",
                   getfd(), count, (long) session_);

        if (m_cb)
            m_cb(session_);
    }

    return 0;
}

int PipeNotifier::initNotifier(Multiplexer *pMultiplexer, void *session)
{
    session_ = session;
    int fds[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1)
        return LS_FAIL;

    ::fcntl(fds[0], F_SETFD, FD_CLOEXEC);
    ::fcntl(fds[1], F_SETFD, FD_CLOEXEC);
    int fl = 0;
    ::fcntl(fds[1], F_GETFL, &fl);
    ::fcntl(fds[1], F_SETFL, fl | pMultiplexer->getFLTag());
    EventReactor::setfd(fds[1]);
    m_fdIn = fds[0];
    int ret = pMultiplexer->add(this, POLLIN | POLLHUP | POLLERR);

    g_api->log(NULL, LSI_LOG_DEBUG,
               "[PipeNotifier] initNotifier called, fd = %d ret = %d.\n ", getfd(), ret);


    return 0;

}

void PipeNotifier::uninitNotifier(Multiplexer *pMultiplexer)
{
    pMultiplexer->remove(this);
}

void PipeNotifier::notify()
{
    write(m_fdIn, "a", 1);
}


