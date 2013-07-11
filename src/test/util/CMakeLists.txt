cmake_minimum_required(VERSION 2.8)


########### next target ###############

SET(util_STAT_SRCS
   pcregextest.cpp
   ghashtest.cpp
   linkedobjtest.cpp
   gzipbuftest.cpp
   vmembuftest.cpp
   gpathtest.cpp
   gstringtest.cpp
   poolalloctest.cpp
   xmlnodetest.cpp
   accesscontroltest.cpp
   loopbuftest.cpp
   base64test.cpp
   logfiletest.cpp
   stringmaptest.cpp
)

add_library(testutil STATIC ${util_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

######## kdevelop will overwrite this part!!! (begin)##########
#noinst_LIBRARIES = libutil.a
#
#INCLUDES = $(all_includes)
#
#libutil_a_METASOURCES = AUTO
#
#libutil_a_SOURCES = pcregextest.cpp ghashtest.cpp linkedobjtest.cpp gzipbuftest.cpp vmembuftest.cpp gpathtest.cpp gstringtest.cpp poolalloctest.cpp xmlnodetest.cpp accesscontroltest.cpp loopbuftest.cpp base64test.cpp logfiletest.cpp stringmaptest.cpp 
#
#
#EXTRA_DIST = stringmaptest.cpp stringmaptest.h logfiletest.cpp logfiletest.h base64test.cpp base64test.h loopbuftest.h loopbuftest.cpp accesscontroltest.h accesscontroltest.cpp xmlnodetest.cpp xmlnodetest.h poolalloctest.cpp poolalloctest.h gstringtest.cpp gstringtest.h gpathtest.h gpathtest.cpp vmembuftest.cpp vmembuftest.h gzipbuftest.cpp gzipbuftest.h linkedobjtest.cpp linkedobjtest.h ghashtest.cpp ghashtest.h pcregextest.cpp pcregextest.h 
#
######## kdevelop will overwrite this part!!! (end)############
