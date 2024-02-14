cmake_minimum_required(VERSION 3.0)

#include_directories("/usr/lib64/glib-2.0/include")
#include_directories("/usr/include/glib-2.0")

########### next target ###############

SET(cgi_STAT_SRCS
   suexec.cpp
   cgidreq.cpp
   cgidconfig.cpp
   cgidworker.cpp
   cgidconn.cpp
   lscgid.cpp
   cgroupconn.cpp
   cgroupuse.cpp
   ns.c
   nsopts.c
   nspersist.c
   nsutils.c
   use_bwrap.c
)

add_library(cgi STATIC ${cgi_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

######## kdevelop will overwrite this part!!! (begin)##########
#noinst_LIBRARIES = libcgi.a
#
#INCLUDES = $(all_includes)
#
#libcgi_a_METASOURCES = AUTO
#
#libcgi_a_SOURCES = suexec.cpp cgidreq.cpp cgidconfig.cpp cgidworker.cpp cgidconn.cpp 
#
#
#EXTRA_DIST = cgidconn.cpp cgidconn.h cgidworker.cpp cgidworker.h cgidconfig.cpp cgidconfig.h cgidreq.cpp cgidreq.h lscgiddef.h suexec.cpp suexec.h 
#
######## kdevelop will overwrite this part!!! (end)############
