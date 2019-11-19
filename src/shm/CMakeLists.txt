
cmake_minimum_required(VERSION 2.8)

SET(shm_STAT_SRCS addrmap.cpp
    lsshm.cpp
    lsshmhash.cpp
    lsshmlock.cpp
    lsshmpool.cpp
    lsshmtidmgr.cpp
    lsshmhashobserver.cpp
)

add_library(lsshm STATIC ${shm_STAT_SRCS} )

#add_library(lsshm SHARED ${shm_STAT_SRCS} )

