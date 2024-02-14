/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#include <edio/iouring.h>
#if IOURING
#include <edio/lsiouringreq.h>
#include <edio/multiplexer.h>
#include "liburing/include/liburing/io_uring.h"
#include "liburing/include/liburing.h"
#ifdef OLS_IOURING_DEBUG
#include "log4cxx/logger.h"
#endif

LS_SINGLETON(Iouring);

bool Iouring::s_tested_supported = false;
bool Iouring::s_supported = false;

// By default, only errors are debugged
// You can turn on detailed debugging with DETAILED_DEBUGGING here
#define DETAILED_DEBUGGING(...)  message(false, __VA_ARGS__)
//#define DETAILED_DEBUGGING(...)

void Iouring::message(bool error, const char *fmt, ...)
{
    if (!s_debug && !error)
        return;
    va_list ap;
    va_start(ap, fmt);
    FILE *f = stderr;
    /*
    int ctr = 0;
    do {
        f = fopen("/tmp/iouring.dbg","a");
        if (!f) {
            ctr++;
            usleep(0);
        }
    } while ((!f) && (ctr < 1000));
    */
    if (f) {
#ifdef OLS_IOURING_DEBUG
        if (error)
        {
            if (log4cxx::Level::isEnabled( log4cxx::Level::NOTICE ) )
                log4cxx::Logger::s_vlog(log4cxx::Level::NOTICE, NULL, fmt, ap, 1);
        }
        else
            if ( log4cxx::Level::isEnabled( log4cxx::Level::DEBUG ) )
                log4cxx::Logger::s_vlog(log4cxx::Level::DEBUG, NULL, fmt, ap, 1);
#else
        struct timespec ts;
        struct tm lt;
        clock_gettime(CLOCK_REALTIME, &ts);
        localtime_r(&ts.tv_sec, &lt);

        char prefix[256];
        char suffix[8192];
        snprintf(prefix, sizeof(prefix),
                 "%02d/%02d/%04d %02d:%02d:%02d:%06ld (%d) %s", lt.tm_mon + 1,
                 lt.tm_mday, lt.tm_year + 1900, lt.tm_hour, lt.tm_min, lt.tm_sec,
                 ts.tv_nsec / 1000, getpid(), error ? "ERROR: " : "");
        vsnprintf(suffix, sizeof(suffix), fmt, ap);
        fprintf(f, "%s%s", prefix, suffix); // Makes it a single write
        //fprintf(f,"%02d/%02d/%04d %02d:%02d:%02d:%06ld (%d) %s", lt.tm_mon + 1,
        //        lt.tm_mday, lt.tm_year + 1900, lt.tm_hour, lt.tm_min, lt.tm_sec,
        //        ts.tv_nsec / 1000, getpid(), error ? "ERROR: " : "");
        //vfprintf(f, fmt, ap);
        fflush(f);
        /*
        fclose(f);
        */
#endif
    }
    va_end(ap);
}


int Iouring::supported(bool errorIfNotSupported)
{
    if (s_tested_supported)
    {
        if (!s_supported && errorIfNotSupported)
            message(errorIfNotSupported, "ERROR: Iouring is not supported and it's required");
        return s_supported;
    }
    s_tested_supported = true;
    struct utsname name;
    int major, minor, count;
    if (uname(&name))
    {
        int err = errno;
        message(true, "ERROR: Iouring::supported Couldn't get uname!  %s\n",
                strerror(err));
        return -1;
    }
    if ((count = sscanf(name.release,"%u.%u", &major, &minor)) != 2)
    {
        int err = errno;
        if (err >= 0)
        {
            message(true, "ERROR: Iouring::supported sscanf crazy return %d\n",
                    count);
            err = EINVAL;
        }
        message(true, "ERROR: Iouring::supported failed to extract Linux version: %s\n",
                strerror(err));
        return -1;
    }
    // We require 5.8 as earlier versions leave unkillable tasks!
    if (major > 5 || (major == 5 && minor >= 8))
    {
        message(false, "Iouring::supported, in this version v%d.%d\n", major, minor);
        struct io_uring io_uring_l;
        memset(&io_uring_l, 0, sizeof(struct io_uring));
        int err = io_uring_queue_init(s_queue_depth_default, &io_uring_l, 0);
        if (err)
            message(errorIfNotSupported, "ERROR: Iouring io_uring_queue_init failed: %s.  Not supported\n", strerror(-err));
        else
        {
            io_uring_queue_exit(&io_uring_l);
            s_supported = true;
        }
    }
    else
        message(errorIfNotSupported, "Iouring::NOT supported! v%d.%d < 5.8\n", major, minor);

    return s_supported;
}


bool Iouring::did_queue_init()
{
    if (!m_io_uring)
        return false;
    return m_io_uring->cq.khead != NULL;
}

Iouring::~Iouring()
{
    DETAILED_DEBUGGING("Iouring destructor\n");
    if (getPollfd()->fd != -1)
    {
        Multiplexer *pMpl = getMultiplexer();
        if (pMpl)
            pMpl->remove(this);
    }
    if (did_queue_init())
    {
        DETAILED_DEBUGGING("Iouring destructor Doing queue_exit\n");
        io_uring_queue_exit(m_io_uring);
    }
    if (m_io_uring)
        free(m_io_uring);
    if (m_eventfd > 0)
        close(m_eventfd);
    DETAILED_DEBUGGING("Iouring destructor exiting\n");
}


int Iouring::load()
{
    int err;
    if (!supported(true))
        return -1;
    if (queue_init())
        return -1;
    if ((m_eventfd = eventfd(0, EFD_NONBLOCK)) < 0)
    {
        err = errno;
        message(true, "ERROR: Iouring Unable to setup eventfd: %s\n",
                strerror(err));
        errno = err;
        return -1;
    }
    setfd(m_eventfd);

    DETAILED_DEBUGGING("Iouring register eventfd\n");
    err = io_uring_register_eventfd(m_io_uring, m_eventfd);
    if (err < 0)
    {
        message(true, "ERROR: Iouring Unable to register eventfd: %s\n",
                strerror(-err));
        errno = -err;
        return -1;
    }
    DETAILED_DEBUGGING("Iouring init, fd: %d\n", m_eventfd);
    setPollfd();
    Multiplexer *pMpl = getMultiplexer();
    DETAILED_DEBUGGING("Adding %p with %d to Multiplexer\n", this, getfd());
    pMpl->add(this, POLLIN);
    return 0;
}


int Iouring::read_eventfd(int &pending)
{
    int ret = 0;
    eventfd_t v = 0;
    ret = eventfd_read(m_eventfd, &v);
    if (ret)
    {
        int err = errno;
        if (err == EAGAIN)
        {
            pending = 0;
            DETAILED_DEBUGGING("Iouring idle eventfd\n");
            return 0;
        }
        message(true, "ERROR: Iouring read_eventfd: %s\n", strerror(err));
        errno = err;
        return -1;
    }
    pending = (int)v;
    return ret;
}


int Iouring::check_pending()
{
    int pending = 0;
    int ret = read_eventfd(pending);
    if (ret < 0)
        return ret;
    while (true)
    {
        struct io_uring_cqe *cqe = NULL;
        ret = io_uring_peek_cqe(m_io_uring, &cqe);
        if (!cqe)
        {
            message(false, "Did a io_uring_peek_cqe with no pending event (%s)!\n",
                    strerror(-ret));
            return 0;
        }
        LsIouringReq *lsIouringReq = (LsIouringReq *)io_uring_cqe_get_data(cqe);
        if (!lsIouringReq)
        {
            message(true, "io_uring cqe with no data\n");
            return -1;
        }
        lsIouringReq->setRet(cqe->res);
        io_uring_cqe_seen(m_io_uring, cqe);
        if (lsIouringReq->getCancel())
        {
            DETAILED_DEBUGGING("Canceled fd: %d at %ld ret %d\n",
                               lsIouringReq->getfd(), lsIouringReq->getPos(), 
                               lsIouringReq->getRet());
            lsIouringReq->setAsyncState(ASYNC_STATE_IDLE);
        }
        else
        {
            lsIouringReq->setAsyncState(ASYNC_STATE_READ);
            ret = lsIouringReq->io_complete(POLLIN);
            if (ret < 0 && !lsIouringReq->getCancel())
            {
                message(false, "iouring setError for process failure\n");
                lsIouringReq->setError();
                return ret;
            }
        }
        pending--;
    }
    return 0;
}


int Iouring::handleEvents(short int revent)
{
    if (!(revent & POLLIN))
    {
        message(true, "Iouring, unexpected event %d\n", revent);
        return -1;
    }
    return check_pending();
 }


int Iouring::queue_init()
{
    int err;
    if (did_queue_init())
        return 0;
    m_io_uring = (struct io_uring *)malloc(sizeof(*m_io_uring));
    if (!m_io_uring)
    {
        message(true, "ERROR: Iouring queue_init Can't allocate m_io_uring memory\n");
        return -1;
    }
    memset(m_io_uring, 0, sizeof(*m_io_uring));
    err = io_uring_queue_init(s_queue_depth_default, m_io_uring, 0);
    if (err)
    {
        message(true, "ERROR: Iouring io_uring_queue_init failed: %s\n", strerror(-err));
        errno = -err;
        return -1;
    }
    return 0;
}


#endif // Linux only
