/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
#include "stderrlogger.h"

#include <edio/multiplexer.h>
#include <http/httplog.h>
#include <log4cxx/appender.h>
#include <log4cxx/logger.h>

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>


LS_SINGLETON(StdErrLogger);


StdErrLogger::StdErrLogger()
    : m_iEnabled(0)
    , m_fdStdErr(0)
    , m_pAppender(NULL)
{
}


StdErrLogger::~StdErrLogger()
{
}


static int s_newline = 1;
int StdErrLogger::handleEvents(short event)
{
    int ret = 0;
    if (event & POLLIN)
    {
        int len = 1;
        int count = 0;
        char achBuf[4096];
        while (len > 0 && count++ < 100)
        {
            char *pBuf = &achBuf[33];
            len = ::read(EventReactor::getfd(), pBuf, &achBuf[4096] - pBuf);
            if (m_iEnabled && (len > 0) && (m_pAppender))
            {
                if (s_newline)
                {
                    struct timeval curTime;
                    struct tm   tm;
                    time_t      t;
                    gettimeofday(&curTime, NULL);
                    t = curTime.tv_sec;
                    localtime_r(&t, &tm);
                    snprintf(achBuf, 33,
                             "%04d-%02d-%02d %02d:%02d:%02d.%03d [STDERR]",
                             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                             tm.tm_hour, tm.tm_min, tm.tm_sec,
                             (int)(curTime.tv_usec / 1000));
                    achBuf[32] = ' ';
                    len += 33;
                    pBuf = achBuf;
                }
                s_newline = (pBuf[ len - 1 ] == '\n');

                m_pAppender->append(pBuf, len);
            }
        }
        //m_pAppender->flush();
    }
    else if (event & POLLHUP)
        LS_ERROR("POLLHUP");
    if ((ret != -1) && (event & POLLOUT))
        LS_ERROR("POLLOUT");
    if ((ret != -1) && (event & POLLERR))
        LS_ERROR("POLLERR");
    return 0;

}


int StdErrLogger::setLogFileName(const char *pName)
{
    if (!pName)
        m_pAppender = HttpLog::getErrorLogger()->getAppender();
    else
    {
        m_iEnabled = 1;
        if (m_pAppender)
        {
            if (strcmp(m_pAppender->getName(), pName) == 0)
                return 0;
            else
                m_pAppender->close();
        }
        m_pAppender = LOG4CXX_NS::Appender::getAppender(pName);
        dupAppenderFdToStdErr();
    }
    return 0;
}


const char *StdErrLogger::getLogFileName()
{   
    if (!m_pAppender)
        m_pAppender = HttpLog::getErrorLogger()->getAppender();
    return m_pAppender->getName();     
}


int StdErrLogger::initLogger(Multiplexer *pMultiplexer)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1)
    {
        LS_ERROR("Failed to setup StdErrLogger!");
        return LS_FAIL;
    }
    ::fcntl(fds[0], F_SETFD, FD_CLOEXEC);
    int fl = 0;
    ::fcntl(fds[0], F_GETFL, &fl);
    ::fcntl(fds[0], F_SETFL, fl | pMultiplexer->getFLTag());
    EventReactor::setfd(fds[0]);
    pMultiplexer->add(this, POLLIN | POLLHUP | POLLERR);
    if (fds[1] == STDERR_FILENO)
    {
        m_fdStdErr = dup(fds[1]);
    }
    else
    {
        m_fdStdErr = fds[1];
    }
    return 0;
}


void StdErrLogger::dupAppenderFdToStdErr()
{
    if (m_pAppender->getfd() == -1)
        m_pAppender->open();
    if (m_pAppender->getfd() != -1)
    {
        dup2(m_pAppender->getfd(), STDERR_FILENO);
    }
}


void StdErrLogger::dupPipeFdToStdErr()
{
#ifndef RUN_TEST
    if (m_fdStdErr != STDERR_FILENO)
        dup2(m_fdStdErr, STDERR_FILENO);
#endif
}


int StdErrLogger::movePipeFdToStdErr()
{
#ifndef RUN_TEST
    if (m_fdStdErr != STDERR_FILENO && dup2(m_fdStdErr, STDERR_FILENO) != -1)
    {
        close(m_fdStdErr);
        m_fdStdErr = STDERR_FILENO;
    }
#endif
    return 0;
}

