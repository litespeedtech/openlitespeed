cmake_minimum_required(VERSION 3.0)


########### next target ###############

SET(registry_STAT_SRCS 
   appconfig.cpp
   extappregistry.cpp
)

add_library(registry STATIC ${registry_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

######## kdevelop will overwrite this part!!! (begin)##########
#noinst_LIBRARIES = libregistry.a
#
#INCLUDES = $(all_includes)
#
#libregistry_a_METASOURCES = AUTO
#
#libregistry_a_SOURCES = extappregistry.cpp 
#
#
#EXTRA_DIST = extappregistry.cpp extappregistry.h 
#
######## kdevelop will overwrite this part!!! (end)############
