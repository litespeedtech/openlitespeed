cmake_minimum_required(VERSION 3.0)
INCLUDE( ${PROJECT_SOURCE_DIR}/CMakeModules/common.cmake)

if(${COMPILE_IO_URING})
    add_definitions(-DOLS_IOURING_DEBUG)
    include_directories(BEFORE SYSTEM liburing liburing/include)
endif()

SET(edio_STAT_SRCS
#    eventnotifier.cpp
   reactorindex.cpp
   fdindex.cpp
   kqueuer.cpp
   epoll.cpp
   rtsigio.cpp
   ediostream.cpp
   outputbuf.cpp
   cacheos.cpp
   inputstream.cpp
   bufferedos.cpp
   outputstream.cpp
   flowcontrol.cpp
   iochain.cpp
   multiplexerfactory.cpp
   eventreactor.cpp
   poller.cpp
   multiplexer.cpp
   pollfdreactor.cpp
   lookupfd.cpp
   devpoller.cpp
   sigeventdispatcher.cpp
   aiooutputstream.cpp
   aiosendfile.cpp
   eventnotifier.cpp
   eventprocessor.cpp
   evtcbque.cpp
   ssledstream.cpp
   lsaioreq.cpp
   lsposixaioreq.cpp
)

if(${COMPILE_IO_URING})
LIST(APPEND
   edio_STAT_SRCS
   iouring.cpp
   liburing/queue.c
   liburing/register.c
   liburing/setup.c
   liburing/syscall.c
   lsiouringreq.cpp
)
endif()
if(${COMPILE_IO_URING} OR ${COMPILE_LINUX_AIO})
LIST(APPEND
   edio_STAT_SRCS
   linuxaio.cpp
   lslinuxaioreq.cpp
)
endif()

add_library(edio STATIC ${edio_STAT_SRCS})

