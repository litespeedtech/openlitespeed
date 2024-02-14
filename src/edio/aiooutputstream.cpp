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

#include <edio/aiooutputstream.h>
#include <edio/sigeventdispatcher.h>
#include <log4cxx/logger.h>
#include <lsr/ls_lock.h>
#include <util/autobuf.h>
#include <util/ni_fio.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>

//#define AIOSTREAM_DEBUG
#ifdef AIOSTREAM_DEBUG
#include <stdio.h>
#endif

#ifdef LS_AIO_USE_KQ
#include <edio/multiplexer.h>
#include <edio/kqueuer.h>
#include <sys/param.h>
#include <sys/linker.h>

/* aio.ko no longer used 
static short s_iAiokoLoaded = -1;
*/
void AioOutputStream::setAiokoLoaded()
{
    //s_iAiokoLoaded = (kldfind("aio.ko") != -1);
}

short AioOutputStream::aiokoIsLoaded()
{
    //return s_iAiokoLoaded;
    return 1;
}
#endif

void AioReq::setcb(int fildes, void *buf, size_t nbytes, int offset,
                   void *pHandler)
{
    m_aiocb.aio_fildes = fildes;
    m_aiocb.aio_buf = (volatile void *)buf;
    m_aiocb.aio_nbytes = nbytes;
    m_aiocb.aio_offset = offset;
    m_aiocb.aio_sigevent.sigev_value.sival_ptr = pHandler;
#if defined(LS_AIO_USE_KQ)
    m_aiocb.aio_sigevent.sigev_notify_kqueue = KQueuer::getFdKQ();
#elif defined(LS_AIO_USE_SIGNAL)
    assert( s_rtsigNo != -1);
    m_aiocb.aio_sigevent.sigev_signo = s_rtsigNo;
#endif
}

int AioReq::s_rtsigNo = -1;

AioReq::AioReq()
{
    memset(&m_aiocb, 0, sizeof(struct aiocb));
    m_aiocb.aio_reqprio = 0;
#if defined(LS_AIO_USE_KQ)
    m_aiocb.aio_sigevent.sigev_notify = SIGEV_KEVENT;
    m_aiocb.aio_sigevent.sigev_notify_kevent_flags = EV_ONESHOT;
#elif defined(LS_AIO_USE_SIGNAL)
    m_aiocb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
#endif
}


int AioOutputStream::open(const char *pathname, int flags, mode_t mode)
{
    if (m_closeRequested)
        m_closeRequested = 0;
    if (getfd() == -1)
        setfd(nio_open(pathname, flags, mode));
    int ret = getfd();
    return ret;
}


int AioOutputStream::close()
{
    if (getfd() != -1)
    {
        if (m_pRecv)
            flushEx();
        if (m_pSend)
            m_closeRequested = 1;
        else
        {
            nio_close(getfd());
            setfd(-1);
            m_closeRequested = 0;
            m_flushRequested = 0;
        }
    }
    return LS_OK;
}


int AioOutputStream::syncWrite(const char *pBuf, int len)
{
    struct flock lock;
    int ret;
    if (m_iFlock)
    {
        lock.l_type = F_WRLCK;
        lock.l_start = 0;
        lock.l_whence = SEEK_SET;
        lock.l_len = 1;
        fcntl(getfd(), F_SETLK, &lock);
    }
    ret = nio_write(getfd(), pBuf, len);
    if (m_iFlock)
    {
        lock.l_type = F_UNLCK;
        fcntl(getfd(), F_SETLK, &lock);
    }
    return ret;
}


#define MAX_AIO_BUFFER_SIZE (256 * 1024)
int AioOutputStream::append(const char *pBuf, int len)
{
    int ret;
    if (getfd() == -1)
    {
        return LS_FAIL;
    }
    if (!m_async)
    {
        ret = syncWrite(pBuf, len);
        return ret;
    }
    if (!m_pRecv)
        m_pRecv = new AutoBuf();
    ret = m_pRecv->append(pBuf, len);
    if (m_pRecv->size() >= MAX_AIO_BUFFER_SIZE)
    {
        if (m_pSend == NULL)
        {
            m_pSend = m_pRecv;
            m_pRecv = NULL;

            if (write(getfd(), m_pSend->begin(), m_pSend->size(),
                    0, (EventHandler *)this))
                ret = LS_FAIL;
            else
                m_flushRequested = 0;
        }
        else
        {
            len = m_pRecv->size();
            ret = syncWrite(m_pRecv->begin(), len);
            m_pRecv->clear();
            if (ret < len)
                ret = -1;
        }
    }
    return ret;
}


#define LS_AIO_MAXBUF 4096
int AioOutputStream::onEvent()
{
    LS_DBG("AioOutputStream::onEvent");
    MutexLocker lock(m_mutex);
    if (m_pSend)
    {
        int err = getError();
        if (err != EINPROGRESS)
        {
            assert(m_pSend->begin() == getBuf());
            delete m_pSend;
            m_pSend = NULL;
        }
        else
        {
            return LS_OK;
        }
    }
    if (m_flushRequested)
    {
        int ret = flushEx();
        return ret;
    }
    if (m_closeRequested)
        return close();
    return LS_OK;
}


int AioOutputStream::flush()
{
#if defined(LS_AIO_USE_AIO)
    ls_mutex_lock(&m_mutex);
    int ret = flushEx();
    ls_mutex_unlock(&m_mutex);
#endif // defined(LS_AIO_USE_AIO)
    return ret;
}


int AioOutputStream::flushEx()
{
#if defined(LS_AIO_USE_AIO)
    if (!m_pRecv)
        return LS_OK;
    else if (m_pSend)
    {
        int err = getError();
        if (err == EINPROGRESS)
        {
            m_flushRequested = 1;
            return LS_OK;
        }
        else //succeed or failed with error
        {
            assert(m_pSend->begin() == getBuf());
            delete m_pSend;
            m_pSend = NULL;
        }
    }
    else if (getfd() == -1)
        return LS_FAIL;

    m_pSend = m_pRecv;
    m_pRecv = NULL;
    if (write(getfd(), m_pSend->begin(), m_pSend->size(),
              0, (EventHandler *)this))
        return LS_FAIL;
    m_flushRequested = 0;
#endif // defined(LS_AIO_USE_AIO)
    return LS_OK;
}


