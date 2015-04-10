
cmake_minimum_required(VERSION 2.8)
INCLUDE( ${PROJECT_SOURCE_DIR}/CMakeModules/common.cmake)

cmake_policy(SET CMP0002 OLD)


########### next target ###############

SET(shm_STAT_SRCS 
    lsshmcache.cpp 
    lsshmlock.cpp 
    lsshmlru.c 
    lsshmhash.cpp 
    lsshmlruhash.cpp 
    lsshmpool.cpp 
    lsshm.cpp
    lsi_shm.cpp
    ls_fallocate.c
)

add_library(lsshm STATIC ${shm_STAT_SRCS} )

#add_library(lsshm SHARED ${shm_STAT_SRCS} )

