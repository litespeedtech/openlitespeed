cmake_minimum_required(VERSION 2.8)


########### next target ###############

SET(socket_STAT_SRCS
   hostinfotest.cpp
   tcpsockettest.cpp
   coresockettest.cpp
)

add_library(testsocket STATIC ${socket_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

######## kdevelop will overwrite this part!!! (begin)##########
#noinst_LIBRARIES = libsocket.a
#
#INCLUDES = $(all_includes)
#
#libsocket_a_METASOURCES = AUTO
#
#libsocket_a_SOURCES = hostinfotest.cpp tcpsockettest.cpp coresockettest.cpp 
#
#
#EXTRA_DIST = coresockettest.cpp coresockettest.h tcpsockettest.cpp tcpsockettest.h hostinfotest.cpp hostinfotest.h 
#
######## kdevelop will overwrite this part!!! (end)############
