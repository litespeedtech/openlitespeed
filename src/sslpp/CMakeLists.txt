cmake_minimum_required(VERSION 2.8)


########### next target ###############

SET(sslpp_STAT_SRCS
   sslengine.cpp
   sslcert.cpp
   sslerror.cpp
   sslconnection.cpp
   sslcontext.cpp
   ../http/requestvars.cpp
)

add_library(sslpp STATIC ${sslpp_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

######## kdevelop will overwrite this part!!! (begin)##########
#noinst_LIBRARIES = libsslpp.a
#
#INCLUDES = $(all_includes)
#
#libsslpp_a_METASOURCES = AUTO
#
#libsslpp_a_SOURCES = sslengine.cpp sslcert.cpp sslerror.cpp sslconnection.cpp sslcontext.cpp 
#
#
#EXTRA_DIST = sslcontext.cpp sslcontext.h sslconnection.cpp sslconnection.h sslerror.cpp sslerror.h sslcert.cpp sslcert.h sslengine.cpp sslengine.h 
#
######## kdevelop will overwrite this part!!! (end)############
