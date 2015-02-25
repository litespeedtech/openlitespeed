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
#include <log4cxx/fileappender.h>
#include <log4cxx/layout.h>
#include <log4cxx/loggingevent.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <util/gfactory.h>
#include <util/ni_fio.h>



BEGIN_LOG4CXX_NS

int FileAppender::init()
{
    return s_pFactory->registType(new FileAppender("appender.ps"));
}

Duplicable *FileAppender::dup(const char *pName)
{
    Appender *pAppender = new FileAppender(pName);
    if (strcasecmp(pName, "stdout") == 0)
        pAppender->setfd(::dup(1));
    else if (strcasecmp(pName, "stderr") == 0)
        pAppender->setfd(::dup(2));
    else
        return pAppender;
    fcntl(pAppender->getfd(), F_SETFD, FD_CLOEXEC);
    return pAppender;
}


int FileAppender::reopenExist()
{
    const char *pName = getName();
    struct stat st;
    if (nio_stat(pName, &st) == -1)
        return 0;
    if (m_ino != st.st_ino)
    {
        close();
        return open();
    }
    return 0;
}


int FileAppender::open()
{
    if (getfd() != -1)
        return 0;
    const char *pName = getName();
    if (!pName)
    {
        errno = EINVAL;
        return -1;
    }
    int flag = O_WRONLY | O_CREAT;
    if (!getAppendMode())
        flag |= O_TRUNC ;
    else
        flag |= O_APPEND;
    setfd(nio_open(pName, flag, 0644));
    if (getfd() != -1)
    {
        struct stat st;
        if (::fstat(getfd(), &st) != -1)
            m_ino = st.st_ino;
        fcntl(getfd(), F_SETFD, FD_CLOEXEC);
        return 0;
    }
    return -1;
}

int FileAppender::close()
{
    nio_close(getfd());
    setfd(-1);
    return 0;
}


int FileAppender::append(const char *pBuf, int len)
{
    if (getfd() == -1)
        if (open() == -1)
            return -1;
    return nio_write(getfd(), pBuf, len);
}


END_LOG4CXX_NS


