cmake_minimum_required(VERSION 2.8)
INCLUDE( ${PROJECT_SOURCE_DIR}/CMakeModules/common.cmake)


SET(socket_STAT_SRCS
   reuseport.cpp
   gsockaddr.cpp
   hostinfo.cpp
   tcpsockopt.cpp
   tcpserversocket.cpp
   tcpconnection.cpp
   streamsocket.cpp
   serversocket.cpp
   coresocket.cpp
)

add_library(socket STATIC ${socket_STAT_SRCS})

