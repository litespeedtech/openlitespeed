cmake_minimum_required(VERSION 2.8)


########### next target ###############

SET(sysinfo_STAT_SRCS partitioninfo.cpp
   nicdetect.cpp
   systeminfo.cpp
)

add_library(sysinfo STATIC ${sysinfo_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

######## kdevelop will overwrite this part!!! (begin)##########
#noinst_LIBRARIES = libsysinfo.a
#
#INCLUDES = $(all_includes)
#
#libsysinfo_a_METASOURCES = AUTO
#
#libsysinfo_a_SOURCES = nicdetect.cpp systeminfo.cpp 
#
#
#EXTRA_DIST = systeminfo.cpp systeminfo.h nicdetect.cpp nicdetect.h 
#
######## kdevelop will overwrite this part!!! (end)############
