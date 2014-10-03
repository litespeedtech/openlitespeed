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

SET(util_SRCS
   util/httpfetchdriver.cpp
   util/httpfetch.cpp
   util/ni_fio.c
   util/linkobjpool.cpp
   util/pidfile.cpp
   util/hashdatacache.cpp
   util/keydata.cpp
   util/tlinklist.cpp
   util/mysleep.c
   util/rlimits.cpp
   util/duplicable.cpp
   util/gfactory.cpp
   util/fdpass.cpp
   util/pcregex.cpp
   util/autostr.cpp
   util/staticobj.cpp
   util/pool.cpp
   util/hashstringmap.cpp
   util/ghash.cpp
   util/emailsender.cpp
   util/guardedapp.cpp
   util/crashguard.cpp
   util/iconnection.cpp
   util/dlinkqueue.cpp
   util/connpool.cpp
   util/gzipbuf.cpp
   util/vmembuf.cpp
   util/blockbuf.cpp
   util/stringlist.cpp
   util/semaphore.cpp
   util/refcounter.cpp
   util/gpointerlist.cpp
   util/linkedobj.cpp
   util/objpool.cpp
   util/gpath.cpp
   util/logtracker.cpp
   util/poolalloc.cpp
   util/env.cpp
   util/xmlnode.cpp
   util/iovec.cpp
   util/accesscontrol.cpp
   util/signalutil.cpp
   util/loopbuf.cpp
   util/stringtool.cpp
   util/tsingleton.cpp
   util/pcutil.cpp
   util/daemonize.cpp
   util/configentry.cpp
   util/configctx.cpp
   util/logidtracker.cpp
   util/datetime.cpp
   util/misc/profiletime.cpp
   util/sysinfo/partitioninfo.cpp
   util/sysinfo/nicdetect.cpp
   util/sysinfo/systeminfo.cpp
)

SET(lsr_SRCS
   lsr/lsr_aho.c
   lsr/lsr_base64.c
   lsr/lsr_buf.c
   lsr/lsr_confparser.c
   lsr/lsr_crc64.c
   lsr/lsr_dlinkq.c
   lsr/lsr_hash.c
   lsr/lsr_link.c
   lsr/lsr_lock.c
   lsr/lsr_loopbuf.c
   lsr/lsr_map.c
   lsr/lsr_md5.c
   lsr/lsr_objarray.c
   lsr/lsr_objpool.c
   lsr/lsr_pcreg.c
   lsr/lsr_pool.c
   lsr/lsr_ptrlist.c
   lsr/lsr_sha1.c
   lsr/lsr_str.c
   lsr/lsr_strlist.c
   lsr/lsr_strtool.c
   lsr/lsr_xpool.c
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
   test/util/poolalloctest.cpp
   test/util/xmlnodetest.cpp
   test/util/accesscontroltest.cpp
   test/util/loopbuftest.cpp
   test/util/logfiletest.cpp
   test/util/stringmaptest.cpp
   test/util/httpfetchtest.cpp
   test/util/partitioninfotest.cpp
   test/util/gmaptest.cpp
   test/util/ahotest.cpp
   test/util/stringtooltest.cpp
   test/util/gpointerlisttest.cpp
   test/util/objarraytest.cpp
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

SET(lsr_STAT_SRCS
   test/lsr/lsr_ahotest.cpp
   test/lsr/lsr_confparsertest.cpp
   test/lsr/lsr_base64test.cpp
   test/lsr/lsr_buftest.cpp
   test/lsr/lsr_dlinkqtest.cpp
   test/lsr/lsr_hashtest.cpp
   test/lsr/lsr_linktest.cpp
   test/lsr/lsr_loopbuftest.cpp
   test/lsr/lsr_maptest.cpp
   test/lsr/lsr_md5test.cpp
   test/lsr/lsr_objarraytest.cpp
   test/lsr/lsr_objpooltest.cpp
   test/lsr/lsr_pcregtest.cpp
   test/lsr/lsr_pooltest.cpp
   test/lsr/lsr_ptrlisttest.cpp
   test/lsr/lsr_sha1test.cpp
   test/lsr/lsr_strtest.cpp
   test/lsr/lsr_strlisttest.cpp
   test/lsr/lsr_strtooltest.cpp
   test/lsr/lsr_xpooltest.cpp
)

link_directories(/usr/local/lib ${PROJECT_SOURCE_DIR}/src/test/unittest-cpp/UnitTest++)

add_executable(openlitespeed ${openlitespeed_SRCS} ${util_SRCS} ${lsr_SRCS} ${edio_STAT_SRCS} 
    ${extensions_STAT_SRCS} ${http_STAT_SRCS} ${socket_STAT_SRCS} 
    ${util_STAT_SRCS} ${spdy_STAT_SRCS} ${MODULE_DATAS_SRCS} 
    ${lsr_STAT_SRCS})

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
${lsr_SRCS}
${util_SRCS}
)

add_executable(lsr_locktest
test/lsr/lsr_locktest.cpp ${lsr_SRCS}
)

add_executable(lsr_thrsafetest
test/lsr/lsr_thrsafetest.cpp ${lsr_SRCS}
)

add_executable(lsr_valgrindtest
test/lsr/lsr_valgrindtest.c ${lsr_SRCS}
)



SET( litespeedlib 
    lsshm main http main modgzip spdy ssi 
    lsiapi registry cgi fcgi jk extensions 
    lsapi proxy edio  log4cxx socket 
    sslpp  http edio GeoIP pthread ${CMAKE_DL_LIBS} ${libUnitTest} )

target_link_libraries(openlitespeed ${litespeedlib} )

target_link_libraries(shmtest ${litespeedlib} )

target_link_libraries(lsr_locktest pthread ssl crypto pcre )

target_link_libraries(lsr_thrsafetest pthread ssl crypto pcre )

target_link_libraries(lsr_valgrindtest pthread ssl crypto pcre )

install(TARGETS openlitespeed DESTINATION bin)

