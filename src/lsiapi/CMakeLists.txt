cmake_minimum_required(VERSION 3.0)

SET(lsiapi_STAT_SRCS 
    ediohandler.cpp
    envmanager.cpp
    lsimoduledata.cpp
    lsiapihooks.cpp
    modulehandler.cpp
    lsiapi.cpp
    lsiapi_const.cpp
    lsiapilib.cpp
    lsiapigd.cpp
    modulemanager.cpp
    moduletimer.cpp
)

add_library(lsiapi STATIC ${lsiapi_STAT_SRCS})

