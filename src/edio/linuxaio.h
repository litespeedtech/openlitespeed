/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#ifndef LINUXAIO_H
#define LINUXAIO_H

#if defined(LS_AIO_USE_LINUX_AIO)
#include <libaio.h>
//#include <linux/aio_abi.h>

#include "edio/eventreactor.h"
#include "util/tsingleton.h"

class LsLinuxAioReq;

class LinuxAio : public EventReactor
               , public TSingleton< LinuxAio >
{
private:
    friend class TSingleton< LinuxAio >;
    static const int        s_check_event_count = 128;
    int                     m_eventfd;
    io_context_t            m_context;

    int read_eventfd(int &pending);
    int check_pending();

    void start();
    void close_eventfd();
    LinuxAio(const LinuxAio &other);

public:
    LinuxAio()
        : m_eventfd(-1)
        , m_context((io_context_t)0)
    {}
    virtual ~LinuxAio();

    int load();
    int get_eventfd()           {   return m_eventfd;   };
    io_context_t get_context()  {   return m_context;   }
    virtual int handleEvents(short int event);

};

LS_SINGLETON_DECL(LinuxAio);

#endif // LS_AIO_USE_LINUX_AIO guard
#endif // LINUXAIO_H Guard
