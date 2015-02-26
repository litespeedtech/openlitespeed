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
   util/linkobjpool.cpp
   util/pidfile.cpp
   util/hashdatacache.cpp
   util/keydata.cpp
   util/tlinklist.cpp
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
   util/resourcepool.cpp
   util/linkedqueue.cpp
   util/httputil.cpp
   util/misc/profiletime.cpp
   util/sysinfo/partitioninfo.cpp
   util/sysinfo/nicdetect.cpp
   util/sysinfo/systeminfo.cpp
)

SET(lsr_SRCS
   lsr/ls_aho.c
   lsr/ls_base64.c
   lsr/ls_buf.c
   lsr/ls_confparser.c
   lsr/ls_crc64.c
   lsr/ls_dlinkq.c
   lsr/ls_fileio.c
   lsr/ls_hash.c
   lsr/ls_lfqueue.c
   lsr/ls_lfstack.c
   lsr/ls_link.c
   lsr/ls_llmq.c
   lsr/ls_llxq.c
   lsr/ls_lock.c
   lsr/ls_loopbuf.c
   lsr/ls_map.c
   lsr/ls_md5.c
   lsr/ls_mpscq.c
   lsr/ls_objarray.c
   lsr/ls_objpool.c
   lsr/ls_pcreg.c
   lsr/ls_pool.c
   lsr/ls_ptrlist.c
   lsr/ls_sha1.c
   lsr/ls_str.c
   lsr/ls_strlist.c
   lsr/ls_strtool.c
   lsr/ls_time.c
   lsr/ls_xpool.c
   lsr/xxhash.c
)

SET(thread_SRCS
   thread/pthreadcond.cpp
   thread/pthreadmutex.cpp
   thread/pthreadworkqueue.cpp
   thread/thread.cpp
   thread/workcrew.cpp
   thread/worker.cpp
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
   test/spdy/huffmantest.cpp
   test/spdy/hpacktest.cpp
   test/spdy/spdyzlibfiltertest.cpp
   test/spdy/spdyconnectiontest.cpp
   test/spdy/dummiostream.cpp
)

SET(MODULE_DATAS_SRCS
   test/lsiapi/moduledata.cpp
   test/lsiapi/moduletimertest.cpp
   test/lsiapi/lsiapihookstest.cpp
   test/lsiapi/envhandler.cpp
)

SET(lsr_STAT_SRCS
   test/lsr/ls_ahotest.cpp
   test/lsr/ls_confparsertest.cpp
   test/lsr/ls_base64test.cpp
   test/lsr/ls_buftest.cpp
   test/lsr/ls_dlinkqtest.cpp
   test/lsr/ls_hashtest.cpp
   test/lsr/ls_linktest.cpp
   test/lsr/ls_loopbuftest.cpp
   test/lsr/ls_maptest.cpp
   test/lsr/ls_md5test.cpp
   test/lsr/ls_objarraytest.cpp
   test/lsr/ls_objpooltest.cpp
   test/lsr/ls_pcregtest.cpp
   test/lsr/ls_pooltest.cpp
   test/lsr/ls_ptrlisttest.cpp
   test/lsr/ls_sha1test.cpp
   test/lsr/ls_strtest.cpp
   test/lsr/ls_strlisttest.cpp
   test/lsr/ls_strtooltest.cpp
   test/lsr/ls_xpooltest.cpp
)

SET(thread_STAT_SRCS
   test/thread/pthreadworkqueuetest.cpp
   test/thread/threadtest.cpp
   test/thread/workcrewtest.cpp
)

SET(shm_STAT_SRCS
   test/shm/shmlrutest.cpp
   test/shm/shmxtest.cpp
)

link_directories(/usr/local/lib ${PROJECT_SOURCE_DIR}/src/test/unittest-cpp/UnitTest++)

add_executable(openlitespeed ${openlitespeed_SRCS} ${util_SRCS} ${lsr_SRCS}
    ${thread_SRCS} ${edio_STAT_SRCS}
    ${extensions_STAT_SRCS} ${http_STAT_SRCS} ${socket_STAT_SRCS}
    ${util_STAT_SRCS} ${spdy_STAT_SRCS} ${MODULE_DATAS_SRCS}
    ${lsr_STAT_SRCS} ${thread_STAT_SRCS} ${shm_STAT_SRCS})

#add_executable(luatest
#modules/prelinkedmods.cpp
#test/lua/luatest.cpp
#httpdtest.cpp
#)

add_executable(shmlru_test
test/shm/shmlru_test.c
modules/prelinkedmods.cpp
httpdtest.cpp
${util_SRCS}
${lsr_SRCS}
${thread_SRCS}
)

# add_executable(shmtest
# test/shm/testshmapi.cpp
# test/shm/shmtest.cpp
# test/shm/lsshmdebug.cpp
# test/shm/testshm.cpp
# test/shm/testshmreg.cpp
# test/shm/testshmhash.cpp
# modules/prelinkedmods.cpp
# httpdtest.cpp
# ${lsr_SRCS}
# ${thread_SRCS}
# ${util_SRCS}
# )

add_executable(ls_lfqueuetest
test/lsr/ls_lfqueuetest.c
${lsr_SRCS}
)

add_executable(ls_lfstacktest
test/lsr/ls_lfstacktest.cpp
util/misc/profiletime.cpp
${lsr_SRCS}
${test_SRCS}
)

add_executable(ls_llmqtest
test/lsr/ls_llmqtest.c
${lsr_SRCS}
)

add_executable(ls_locktest
test/lsr/ls_locktest.cpp
${lsr_SRCS}
)

add_executable(ls_stacktest
test/lsr/ls_stacktest.cpp
util/misc/profiletime.cpp
${lsr_SRCS}
${test_SRCS}
)

add_executable(ls_thrsafetest
test/lsr/ls_thrsafetest.cpp
${lsr_SRCS}
)

add_executable(ls_valgrindtest
test/lsr/ls_valgrindtest.c
${lsr_SRCS}
)



SET( litespeedlib
    lsshm main http main modgzip spdy ssi
    lsiapi registry cgi fcgi jk extensions
    lsapi proxy edio log4cxx socket
    sslpp  http edio GeoIP pthread rt
    cache
    ${CMAKE_DL_LIBS} ${libUnitTest} )

target_link_libraries(openlitespeed ${litespeedlib} )

# target_link_libraries(shmtest ${litespeedlib} )

target_link_libraries(shmlru_test ${litespeedlib} )

target_link_libraries(ls_lfqueuetest pthread ssl crypto pcre )

target_link_libraries(ls_lfstacktest pthread ssl crypto pcre )

target_link_libraries(ls_llmqtest pthread ssl crypto pcre )

target_link_libraries(ls_locktest pthread ssl crypto pcre )

target_link_libraries(ls_stacktest pthread ssl crypto pcre )

target_link_libraries(ls_thrsafetest pthread ssl crypto pcre )

target_link_libraries(ls_valgrindtest pthread ssl crypto pcre )

install(TARGETS openlitespeed DESTINATION bin)

