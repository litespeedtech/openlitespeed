cmake_minimum_required(VERSION 2.8)


add_subdirectory(fcgi)
add_subdirectory(cgi)
add_subdirectory(jk)
add_subdirectory(proxy)
add_subdirectory(registry)
add_subdirectory(lsapi)


SET(extensions_STAT_SRCS
   loadbalancer.cpp
   localworkerconfig.cpp
   localworker.cpp
   pidlist.cpp
   iprocessortimer.cpp
   httpextprocessor.cpp
   extrequest.cpp
   extworker.cpp
   extconn.cpp
   extworkerconfig.cpp
   l4conn.cpp
)

add_library(extensions STATIC ${extensions_STAT_SRCS})

