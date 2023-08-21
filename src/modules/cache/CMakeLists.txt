cmake_minimum_required(VERSION 3.0)

include_directories(./;../;../../;)

add_library(cache STATIC 
    cachemanager.cpp
    shmcachemanager.cpp
    cacheentry.cpp
    cachehash.cpp 
    cachestore.cpp
    ceheader.cpp
    dirhashcacheentry.cpp 
    dirhashcachestore.cpp
    cacheconfig.cpp
    cachectrl.cpp
    cache.cpp
)
