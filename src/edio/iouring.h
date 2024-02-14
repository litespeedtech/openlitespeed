/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#ifndef IOURING_H
#define IOURING_H

#if defined(IOURING)
#include "edio/eventreactor.h"
#include "util/tsingleton.h"

#include <sys/uio.h>
#include <linux/fs.h>

class LsIouringReq;

class Iouring : public EventReactor
              , public TSingleton< Iouring >
{
private:
    friend class TSingleton< Iouring >;
    static const bool   s_debug = true;
    static const int    s_queue_depth_default = 128;
    static bool         s_tested_supported;
    static bool         s_supported;

    int                 m_eventfd;
    bool                m_eventfd_nowait;
    struct io_uring    *m_io_uring;

    bool did_queue_init();
    int  queue_init();
    int read_eventfd(int &pending);
    int check_pending();

public:
    static void message(bool error, const char *fmt, ...);
    static int supported(bool errorIfNotSupported);
    int load();
    int get_eventfd()   {   return m_eventfd;   }
    Iouring()
        : m_eventfd(-1)
        , m_eventfd_nowait(false)
        , m_io_uring(NULL)
        {  }
    ~Iouring();

    virtual int handleEvents(short int revent);
    
    void stop();

    struct io_uring *getIo_uring()  {   return m_io_uring;  }

};

LS_SINGLETON_DECL(Iouring);

#endif // Linux only
#endif
