cmake_minimum_required(VERSION 2.8)

add_definitions(-DXXH_HEADER_NAME="lsr/xxhash.h")

SET(h2_STAT_SRCS
h2connection.cpp
h2protocol.cpp
h2stream.cpp
h2streampool.cpp
unpackedheaders.cpp
h2connbase.cpp
h2streambase.cpp
../lshpack/lshpack.c
)

add_library(h2 STATIC ${h2_STAT_SRCS})

