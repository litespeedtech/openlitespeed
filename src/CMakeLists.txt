cmake_minimum_required(VERSION 2.8)


##########################################################################################
#If you want to link libUnitTest++, just un-comment out the following commands
#AND YOU NEED TO INSTALL it to the directory /src/test/unittest-cpp/UnitTest++/
add_definitions(-DRUN_TEST)
add_custom_target(Name ALL COMMAND make WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/test/unittest-cpp/UnitTest++)
set_directory_properties(PROPERTIES  ADDITIONAL_MAKE_CLEAN_FILES "${PROJECT_SOURCE_DIR}/src/test/unittest-cpp/UnitTest++/libUnitTest++.a")
set(libUnitTest  libUnitTest++.a)
##########################################################################################

add_definitions(-DTEST_OUTPUT_PLAIN_CONF)

execute_process( COMMAND getconf LFS_CFLAGS
            OUTPUT_VARIABLE  LFS_CFLAGS )

add_definitions( ${LFS_CFLAGS} )
#FIND_LIBRARY(CPPUNIT_LIBRARY NAMES cppunit
#                          PATHS /usr/lib /lib /usr/local/lib /usr/share/lib /opt/lib /opt/share/lib /var/lib /usr/lib64 /lib64 /usr/local/lib64 /usr/share/lib64 /opt/lib64 /opt/share/lib64 /var/lib64
#                          DOC "Try and find libcppunit")

#INCLUDE( ${PROJECT_SOURCE_DIR}/CMakeModules/common.cmake)

#add_subdirectory(test)
add_subdirectory(spdy)
add_subdirectory(edio)
add_subdirectory(socket)
add_subdirectory(util)
add_subdirectory(http)
add_subdirectory(shm)
add_subdirectory(sslpp)
add_subdirectory(extensions)
add_subdirectory(log4cxx)
add_subdirectory(main)
add_subdirectory(ssi)
add_subdirectory(lsiapi)
add_subdirectory(modules)

########### next target ###############

SET(openlitespeed_SRCS
   modules/prelinkedmods.cpp
   httpdtest.cpp
   main.cpp
)

SET(edio_STAT_SRCS
   test/edio/bufferedostest.cpp
   test/edio/multiplexertest.cpp
)

SET(extensions_STAT_SRCS
   test/extensions/fcgistartertest.cpp
)

SET(http_STAT_SRCS
   test/http/expirestest.cpp
   test/http/rewritetest.cpp
   test/http/httprequestlinetest.cpp
   test/http/httprangetest.cpp
   test/http/denieddirtest.cpp
   test/http/statusurlmaptest.cpp
   test/http/contexttreetest.cpp
   test/http/httpmimetest.cpp
   test/http/httpcgitooltest.cpp
   test/http/chunkostest.cpp
   test/http/chunkistest.cpp
   test/http/httplistenerstest.cpp
   test/http/httpvhostlisttest.cpp
   test/http/httpreqtest.cpp
   test/http/httpreqheaderstest.cpp
   test/http/httpbuftest.cpp
   test/http/httpheadertest.cpp
   test/http/datetimetest.cpp
)
SET(socket_STAT_SRCS
   test/socket/hostinfotest.cpp
   test/socket/tcpsockettest.cpp
   test/socket/coresockettest.cpp
)

SET(util_STAT_SRCS
   test/util/pcregextest.cpp
   test/util/ghashtest.cpp
   test/util/linkedobjtest.cpp
   test/util/gzipbuftest.cpp
   test/util/vmembuftest.cpp
   test/util/gpathtest.cpp
   test/util/gstringtest.cpp
   test/util/poolalloctest.cpp
   test/util/xmlnodetest.cpp
   test/util/accesscontroltest.cpp
   test/util/loopbuftest.cpp
   test/util/base64test.cpp
   test/util/logfiletest.cpp
   test/util/stringmaptest.cpp
   test/util/httpfetchtest.cpp
   test/util/partitioninfotest.cpp
   test/util/gmaptest.cpp
)

SET(spdy_STAT_SRCS
   test/spdy/spdyzlibfiltertest.cpp
   test/spdy/spdyconnectiontest.cpp
   test/spdy/dummiostream.cpp
)

SET(MODULE_DATAS_SRCS
   test/lsiapi/moduledata.cpp
   test/lsiapi/moduletimer.cpp
   test/lsiapi/lsiapihookstest.cpp
   test/lsiapi/envhandler.cpp
)

link_directories(/usr/local/lib ${PROJECT_SOURCE_DIR}/src/test/unittest-cpp/UnitTest++)

add_executable(openlitespeed ${openlitespeed_SRCS} ${edio_STAT_SRCS} 
    ${extensions_STAT_SRCS} ${http_STAT_SRCS} ${socket_STAT_SRCS} 
    ${util_STAT_SRCS} ${spdy_STAT_SRCS} ${MODULE_DATAS_SRCS} )

#add_executable(luatest 
#modules/prelinkedmods.cpp
#test/lua/luatest.cpp 
#httpdtest.cpp
#)

add_executable(shmtest
test/shm/testshmapi.cpp
test/shm/shmtest.cpp
test/shm/lsshmdebug.cpp
test/shm/testshm.cpp
test/shm/testshmreg.cpp
test/shm/testshmhash.cpp
modules/prelinkedmods.cpp
httpdtest.cpp
)

SET( litespeedlib 
    lsshm main http main modgzip spdy ssi 
    lsiapi registry cgi fcgi jk extensions 
    lsapi proxy edio  log4cxx socket 
    sslpp util misc sysinfo http util GeoIP pthread dl ${libUnitTest} )

target_link_libraries(openlitespeed ${litespeedlib} )

target_link_libraries(shmtest ${litespeedlib} )

install(TARGETS openlitespeed DESTINATION bin)
