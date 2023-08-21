cmake_minimum_required(VERSION 3.0)
INCLUDE( ${PROJECT_SOURCE_DIR}/CMakeModules/common.cmake)


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
)

add_library(edio STATIC ${edio_STAT_SRCS})

