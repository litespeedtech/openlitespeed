/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
#ifndef AIOOUTPUTSTREAM_H
#define AIOOUTPUTSTREAM_H

#include <lsdef.h>
#include <edio/eventhandler.h>
#include <lsr/ls_atomic.h>
#include <lsr/ls_lock.h>

#include <aio.h>
#include <sys/types.h>

#include <signal.h>


#define LS_AIO_USE_AIO

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
//#include <linux/version.h>
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
//#define LS_AIO_USE_SIGFD
//#else
#define LS_AIO_USE_SIGNAL
//#endif // LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
#elif defined(__FreeBSD__ ) || defined(__OpenBSD__)
#define LS_AIO_USE_KQ
#else
#undef LS_AIO_USE_AIO
#endif // defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

#ifdef LS_AIO_USE_KQ
#include <sys/event.h>
#endif

class AutoBuf;

class AioReq
{
    static int      s_rtsigNo;
    struct aiocb    m_aiocb;

private:
    void setcb(int fildes, void *buf, size_t nbytes, int off, void *handler);

public:
    AioReq();

    void *getBuf() const
    {   return (void *)m_aiocb.aio_buf;     }

    off_t getOffset() const
    {   return m_aiocb.aio_offset;  }

    int getError() const
    {   return aio_error(&m_aiocb);   }

    //NOTICE: If the error is EINPROGRESS, the result of this is undefined.
    int getReturn()
    {   return aio_return(&m_aiocb);  }

    int read(int fildes, void *buf, int nbytes, int offset,
                    EventHandler *pHandler)
    {
        setcb(fildes, buf, nbytes, offset, pHandler);
        return aio_read(&m_aiocb);
    }

    int write(int fildes, void *buf, int nbytes, int offset,
                     EventHandler *pHandler)
    {
        setcb(fildes, buf, nbytes, offset, pHandler);
        return aio_write(&m_aiocb);
    }
    static void setSigNo(int signo)     {   s_rtsigNo = signo;  }
    static int getSigNo()               {   return s_rtsigNo;   }

    LS_NO_COPY_ASSIGN(AioReq);
};

class AioOutputStream : public EventHandler, private AioReq
{
public:

    AioOutputStream()
        : AioReq()
        , m_fd(-1)
        , m_async(0)
        , m_flushRequested(0)
        , m_closeRequested(0)
        , m_iFlock(0)
        , m_pRecv(NULL)
        , m_pSend(NULL)
    {
        ls_mutex_setup(&m_mutex);
    }
    virtual ~AioOutputStream() {};

    int getfd() const               {   return m_fd;            }
    void setfd(int fd)              {   m_fd = fd;              }
    int open(const char *pathname, int flags, mode_t mode);
    int close();
    bool isAsync() const            {   return m_async == 1;    }

    void setFlock(int l)
    {
        m_iFlock = l;
    }

    void setAsync()
    {
#ifdef LS_AIO_USE_AIO
        m_async = 1;
#endif
    }

    int append(const char *pBuf, int len);
    virtual int onEvent();
    int flush();

#ifdef LS_AIO_USE_KQ
    static void setAiokoLoaded();
    static short aiokoIsLoaded();
#endif

    ls_mutex_t &get_mutex()  {   return m_mutex; }

private:
    int syncWrite(const char *pBuf, int len);
    int flushEx();

private:
    int             m_fd;
    char            m_async;
    char            m_flushRequested;
    char            m_closeRequested;
    char            m_iFlock;

    ls_mutex_t      m_mutex;
    AutoBuf        *m_pRecv;
    AutoBuf        *m_pSend;

    LS_NO_COPY_ASSIGN(AioOutputStream);
};


#endif //AIOOUTPUTSTREAM_H
