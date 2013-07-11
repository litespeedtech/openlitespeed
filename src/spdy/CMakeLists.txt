cmake_minimum_required(VERSION 2.8)


########### next target ###############

SET(spdy_STAT_SRCS 
spdyprotocol.cpp 
spdyconnection.cpp 
spdystream.cpp
spdyzlibfilter.cpp
spdystreampool.cpp
spdydebug.cpp
)

add_library(spdy STATIC ${spdy_STAT_SRCS})

