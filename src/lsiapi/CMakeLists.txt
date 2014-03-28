cmake_minimum_required(VERSION 2.8)

SET(lsiapi_STAT_SRCS 
    envmanager.cpp 
    lsimoduledata.cpp 
    lsiapihooks.cpp 
    modulehandler.cpp
    linklist.c
    lsiapi.cpp
    lsiapilib.cpp
    lsiapigd.cpp
    modulemanager.cpp
)

add_library(lsiapi STATIC ${lsiapi_STAT_SRCS})

