cmake_minimum_required(VERSION 2.8)


########### next target ###############

SET(edio_STAT_SRCS
   bufferedostest.cpp
   multiplexertest.cpp
)

add_library(testedio STATIC ${edio_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

######## kdevelop will overwrite this part!!! (begin)##########
#noinst_LIBRARIES = libedio.a
#
#INCLUDES = $(all_includes)
#
#libedio_a_METASOURCES = AUTO
#
#libedio_a_SOURCES = bufferedostest.cpp multiplexertest.cpp 
#
#
#EXTRA_DIST = multiplexertest.cpp multiplexertest.h bufferedostest.cpp bufferedostest.h 
#
######## kdevelop will overwrite this part!!! (end)############
