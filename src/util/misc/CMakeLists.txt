cmake_minimum_required(VERSION 2.8)


########### next target ###############

SET(misc_STAT_SRCS
   profiletime.cpp
)

add_library(misc STATIC ${misc_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

######## kdevelop will overwrite this part!!! (begin)##########
#noinst_LIBRARIES = libmisc.a
#
#INCLUDES = $(all_includes)
#
#libmisc_a_METASOURCES = AUTO
#
#libmisc_a_SOURCES = profiletime.cpp 
#
#
#EXTRA_DIST = denydup.h profiletime.cpp profiletime.h 
#
######## kdevelop will overwrite this part!!! (end)############
