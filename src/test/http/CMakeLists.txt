cmake_minimum_required(VERSION 2.8)


########### next target ###############

SET(http_STAT_SRCS
   expirestest.cpp
   rewritetest.cpp
   httprequestlinetest.cpp
   httprangetest.cpp
   denieddirtest.cpp
   statusurlmaptest.cpp
   contexttreetest.cpp
   httpmimetest.cpp
   httpcgitooltest.cpp
   chunkostest.cpp
   chunkistest.cpp
   httplistenerstest.cpp
   httpvhostlisttest.cpp
   httpreqtest.cpp
   httpreqheaderstest.cpp
   httpbuftest.cpp
   httpheadertest.cpp
   datetimetest.cpp
)

add_library(testhttp STATIC ${http_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

######## kdevelop will overwrite this part!!! (begin)##########
#noinst_LIBRARIES = libhttp.a
#
#INCLUDES = $(all_includes)
#
#libhttp_a_METASOURCES = AUTO
#
#libhttp_a_SOURCES = expirestest.cpp rewritetest.cpp httprequestlinetest.cpp httprangetest.cpp denieddirtest.cpp statusurlmaptest.cpp contexttreetest.cpp httpmimetest.cpp httpcgitooltest.cpp chunkostest.cpp chunkistest.cpp httplistenerstest.cpp httpvhostlisttest.cpp httpreqtest.cpp httpreqheaderstest.cpp httpbuftest.cpp httpheadertest.cpp datetimetest.cpp 
#
#
#EXTRA_DIST = datetimetest.cpp datetimetest.h httpheadertest.cpp httpheadertest.h httpbuftest.cpp httpbuftest.h httpreqheaderstest.cpp httpreqheaderstest.h httpreqtest.cpp httpreqtest.h httpvhostlisttest.cpp httpvhostlisttest.h httplistenerstest.cpp httplistenerstest.h chunkistest.cpp chunkistest.h chunkostest.cpp chunkostest.h httpcgitooltest.cpp httpcgitooltest.h httpmimetest.cpp httpmimetest.h contexttreetest.cpp contexttreetest.h statusurlmaptest.cpp statusurlmaptest.h denieddirtest.h denieddirtest.cpp httprangetest.cpp httprangetest.h httprequestlinetest.cpp httprequestlinetest.h rewritetest.cpp rewritetest.h expirestest.cpp expirestest.h 
#
######## kdevelop will overwrite this part!!! (end)############
