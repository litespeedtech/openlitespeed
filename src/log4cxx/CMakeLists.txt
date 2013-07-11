cmake_minimum_required(VERSION 2.8)


########### next target ###############

SET(log4cxx_STAT_SRCS
   appendermanager.cpp
   fileappender.cpp
   logrotate.cpp
   patternlayout.cpp
   loggingevent.cpp
   ilog.cpp
   bench.cpp
   layout.cpp
   appender.cpp
   logger.cpp
   level.cpp
)

add_library(log4cxx STATIC ${log4cxx_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

######## kdevelop will overwrite this part!!! (begin)##########
#noinst_LIBRARIES = liblog4cxx.a
#
#INCLUDES = $(all_includes)
#
#liblog4cxx_a_METASOURCES = AUTO
#
#liblog4cxx_a_SOURCES = appendermanager.cpp fileappender.cpp logrotate.cpp patternlayout.cpp loggingevent.cpp ilog.cpp bench.cpp layout.cpp appender.cpp logger.cpp level.cpp 
#
#
#EXTRA_DIST = level.cpp level.h logger.cpp logger.h appender.cpp appender.h nsdefs.h layout.cpp layout.h bench.cpp ilog.cpp ilog.h loggingevent.cpp loggingevent.h patternlayout.cpp patternlayout.h logrotate.cpp logrotate.h fileappender.cpp fileappender.h appendermanager.cpp appendermanager.h 
#
######## kdevelop will overwrite this part!!! (end)############
#noinst_HEADERS = appendermanager.h
