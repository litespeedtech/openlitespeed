
cmake_minimum_required(VERSION 3.0...3.23)


SET(adns_STAT_SRCS 
    adns.cpp
)

add_library(adns STATIC ${adns_STAT_SRCS} )
