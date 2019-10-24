
cmake_minimum_required(VERSION 2.8)

SET(adns_STAT_SRCS 
    adns.cpp
)

add_library(adns STATIC ${adns_STAT_SRCS} )
