cmake_minimum_required(VERSION 3.0)


#include_directories ("${PROJECT_SOURCE_DIR}/../include")
#include_directories ("${PROJECT_SOURCE_DIR}/../src")
#include_directories ("${PROJECT_SOURCE_DIR}/../../third-party/include")
#link_directories ("${PROJECT_SOURCE_DIR}/../build/src/modules/modgzip")

enable_testing()

# The unit-test binaries link every module statically (prelinkedmods) and never
# dlopen anything, so they do not need the default -rdynamic. Dropping it lets
# -Wl,--gc-sections actually collect unreferenced code instead of treating every
# exported symbol as a root. Scoped to this directory, so the server build that
# does rely on -rdynamic for dlopen modules is unaffected.
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Shared sources for the FULL tier: the test harness, the prelinked module
# table and the config context. These drag in the whole server (http, h2, quic,
# modules, lsiapi), so only tests that exercise that stack should use them.
set(OLS_FULL_COMMON_SRCS
   ../src/httpdtest.cpp
   ../src/modules/prelinkedmods.cpp
   ../src/main/configctx.cpp
   unittest_helpers.cpp
   unittest_main.cpp
)

# Shared sources for the LIGHT tier: just the test-root helper and a minimal
# main() (no HttpLog / LsiApi init), so low-level tests stay small.
set(OLS_LIGHT_COMMON_SRCS
   unittest_helpers.cpp
   unittest_main_light.cpp
)

# LIGHT tier: low-level tests (lsr / util / edio / thread / socket) that only
# need the utility/runtime libraries. With -Wl,--gc-sections each binary keeps
# only the objects it actually references, so these shrink from ~35M to <5M.
set(OLS_UNITTEST_LIGHT_SRCS
   edio/bufferedostest.cpp
   edio/multiplexertest.cpp
   socket/hostinfotest.cpp
   socket/tcpsockettest.cpp
   socket/coresockettest.cpp
   util/pcregextest.cpp
   util/ghashtest.cpp
   util/linkedobjtest.cpp
   util/dlinkqueuetest.cpp
   util/gzipbuftest.cpp
   util/brotlibuftest.cpp
   util/vmembuftest.cpp
   util/filtermatchtest.cpp
   util/gpathtest.cpp
   util/poolalloctest.cpp
   util/xmlnodetest.cpp
   util/accesscontroltest.cpp
   util/loopbuftest.cpp
   util/logfiletest.cpp
   util/stringmaptest.cpp
   util/partitioninfotest.cpp
   util/gmaptest.cpp
   util/ahotest.cpp
   util/autostrtest.cpp
   util/stringtooltest.cpp
   util/gpointerlisttest.cpp
   util/httpfetchtest.cpp
   util/objarraytest.cpp
   util/objpooltest.cpp
   util/radixtreetest.cpp
   lsr/ls_ahotest.cpp
   lsr/ls_confparsertest.cpp
   lsr/ls_base64test.cpp
   lsr/ls_buftest.cpp
   lsr/ls_dlinkqtest.cpp
   lsr/ls_hashtest.cpp
   lsr/ls_linktest.cpp
   lsr/ls_loopbuftest.cpp
   lsr/ls_maptest.cpp
   lsr/ls_md5test.cpp
   lsr/ls_objarraytest.cpp
   lsr/ls_objpooltest.cpp
   lsr/ls_pcregtest.cpp
   lsr/ls_pooltest.cpp
   lsr/ls_ptrlisttest.cpp
   lsr/ls_sha1test.cpp
   lsr/ls_strtest.cpp
   lsr/ls_strlisttest.cpp
   lsr/ls_strtooltest.cpp
   lsr/ls_xpooltest.cpp
   shm/shmbaselrutest.cpp
   shm/shmxtest.cpp
   thread/pthreadworkqueuetest.cpp
   thread/threadtest.cpp
   thread/mtnotifiertest.cpp
   thread/workcrewtest.cpp
)

# FULL tier: tests that need the HTTP server, h2/quic, modules or shm and so
# link the complete library set with the full harness.
set(OLS_UNITTEST_FULL_SRCS
#   extensions/fcgistartertest.cpp
   h2/unpackedheaderstest.cpp
   http/httpiptogeo2test.cpp
   http/expirestest.cpp
   http/rewritetest.cpp
   http/httprequestlinetest.cpp
   http/httprangetest.cpp
   http/denieddirtest.cpp
   http/statusurlmaptest.cpp
   http/contexttreetest.cpp
   http/httpmimetest.cpp
   http/httpcgitooltest.cpp
   http/chunkostest.cpp
   http/chunkistest.cpp
   http/httplistenerstest.cpp
   http/httpvhostlisttest.cpp
   http/htauthtest.cpp
   http/httpreqtest.cpp
   http/httpreqheaderstest.cpp
   http/httpbuftest.cpp
   http/httpheadertest.cpp
   http/datetimetest.cpp
   http/reqparsertest.cpp
   lsiapi/moduledata.cpp
   lsiapi/moduletimertest.cpp
   lsiapi/lsiapihookstest.cpp
   lsiapi/envhandler.cpp
   lsiapi/moduleconf.cpp
   security/parserregression.cpp
)

link_directories(${PROJECT_SOURCE_DIR}/../third-party/lib/ /usr/local/lib /usr/lib64)

# add_executable(ctbench
#     modules/prelinkedmods.cpp
#     http/contexttreebench.cpp
#     httpdtest.cpp
# )

#add_executable(luatest
#modules/prelinkedmods.cpp
#lua/luatest.cpp
#httpdtest.cpp
#)

# add_executable(shmlru_test
# shm/shmlru_test.c
# modules/prelinkedmods.cpp
# httpdtest.cpp
# ${util_SRCS}
# ${lsr_SRCS}
# ${thread_SRCS}
# ${http_STAT_SRCS}
# )

# add_executable(shmtest
# shm/testshmapi.cpp
# shm/shmtest.cpp
# shm/lsshmdebug.cpp
# shm/testshm.cpp
# shm/testshmreg.cpp
# shm/testshmhash.cpp
# modules/prelinkedmods.cpp
# httpdtest.cpp
# ${lsr_SRCS}
# ${thread_SRCS}
# ${util_SRCS}
# )








set ( libUnitTest UnitTest++ )
# NOTE: When creating a new directory, the order it is placed in this list
# matters.  Everything listed depends on things listed AFTER it, though
# it won't depend on everything after it.  Inter-dependency may be fixed
# by listing one directory twice, though this isn't recommended.
# So for example, if edio depends on your new directory, your directory
# should be listed AFTER edio.  PLEASE TRY TO KEEP THIS NEAT!
SET( unittestlib
    modgzip cache lsiapi main http ssi http lsiapi
    registry cgi fcgi scgi uwsgi jk extensions lsapi proxy
    socket sslpp lsshm thread log4cxx adns
    quic h2 lsquic bcrypt
    -Wl,--whole-archive util lsr -Wl,--no-whole-archive
    edio udns pthread rt ${CMAKE_DL_LIBS} ${libUnitTest} ${BSSL_ADD_LIB}
    ${LINUX_AIO_LIB} libz.a libpcre2-8.a libexpat.a libxml2.a
    ${BROTLI_ADD_LIB} ${IP2LOC_ADD_LIB} ${MMDB_LIB} atomic
    crypt libssl.a libcrypto.a)

# Library set for the LIGHT tier. util/lsr are listed again after the libs that
# depend on them (socket/adns) so static resolution works without
# --whole-archive. --gc-sections then discards whatever a given test does not
# reference, so naming a lib here costs nothing for tests that do not use it.
SET( unittestlib_light
    util lsr socket sslpp adns lsshm thread log4cxx edio util lsr
    udns pthread rt ${CMAKE_DL_LIBS} ${libUnitTest} ${BSSL_ADD_LIB}
    ${LINUX_AIO_LIB} bcrypt libz.a libpcre2-8.a libexpat.a libxml2.a
    ${BROTLI_ADD_LIB} ${IP2LOC_ADD_LIB} ${MMDB_LIB} atomic
    crypt libssl.a libcrypto.a)

# By default build a single combined runner: it is much smaller on disk and
# faster to compile/link than ~80 separate binaries. When a test fails, rebuild
# with -DOLS_SPLIT_UNIT_TESTS=ON to get one binary per source file so the
# failing case can be run and debugged in isolation.
option(OLS_SPLIT_UNIT_TESTS
    "Build one unit-test binary per source file (to isolate a failing test) instead of a single combined runner."
    OFF)

if(OLS_SPLIT_UNIT_TESTS)

    # common_srcs and libs are passed by variable name and dereferenced here so
    # the two tiers can share one registration helper.
    function(add_ols_unit_test target_name common_srcs libs test_src)
        add_executable(${target_name}
            ${${common_srcs}}
            ${test_src}
        )
        # --gc-sections drops every section not reachable from this test's
        # main(), so each binary only carries the code it actually exercises.
        target_link_libraries(${target_name} ${${libs}} -Wl,--gc-sections)
        add_test(NAME ${target_name} COMMAND ${target_name})
        set_tests_properties(${target_name} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0")
    endfunction()

    foreach(test_src ${OLS_UNITTEST_LIGHT_SRCS})
        get_filename_component(test_name "${test_src}" NAME_WE)
        get_filename_component(test_dir "${test_src}" DIRECTORY)
        string(REPLACE "/" "_" test_dir_name "${test_dir}")
        add_ols_unit_test("${test_dir_name}_${test_name}"
            OLS_LIGHT_COMMON_SRCS unittestlib_light "${test_src}")
    endforeach()

    foreach(test_src ${OLS_UNITTEST_FULL_SRCS})
        get_filename_component(test_name "${test_src}" NAME_WE)
        get_filename_component(test_dir "${test_src}" DIRECTORY)
        string(REPLACE "/" "_" test_dir_name "${test_dir}")
        add_ols_unit_test("${test_dir_name}_${test_name}"
            OLS_FULL_COMMON_SRCS unittestlib "${test_src}")
    endforeach()

    set_tests_properties(
        thread_workcrewtest
        thread_mtnotifiertest
        PROPERTIES RUN_SERIAL TRUE)

else()

    # Single combined runner with every test source linked together.
    add_executable(ols_unittest
        ${OLS_FULL_COMMON_SRCS}
        ${OLS_UNITTEST_LIGHT_SRCS}
        ${OLS_UNITTEST_FULL_SRCS}
    )
    target_link_libraries(ols_unittest ${unittestlib} -Wl,--gc-sections)
    add_test(NAME ols_unittest COMMAND ols_unittest)
    set_tests_properties(ols_unittest PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0")

endif()

# target_link_libraries(ctbench ${litespeedlib} )

# target_link_libraries(shmtest ${litespeedlib} )

# target_link_libraries(shmlru_test ${litespeedlib} )

add_executable(POSTtest2
mthandler/POSTtest2.cpp
)
target_link_libraries(POSTtest2 pthread )

add_executable(POSTtest
mthandler/POSTtest.cpp
)

add_executable(ls_lfqueuetest
lsr/ls_lfqueuetest.c
${lsr_SRCS}
)

target_link_libraries(ls_lfqueuetest lsr thread pthread pcre2-8 )

add_executable(ls_lfstacktest
lsr/ls_lfstacktest.cpp
../src/util/misc/profiletime.cpp
${lsr_SRCS}
${test_SRCS}
)
target_link_libraries(ls_lfstacktest lsr thread pthread pcre2-8 rt )

add_executable(ls_llmqtest
lsr/ls_llmqtest.c
${lsr_SRCS}
)
target_link_libraries(ls_llmqtest lsr thread pthread pcre2-8 )

add_executable(ls_locktest
lsr/ls_locktest.cpp
${lsr_SRCS}
)
target_link_libraries(ls_locktest lsr thread pthread pcre2-8 )

add_executable(ls_stacktest
lsr/ls_stacktest.cpp
../src/util/misc/profiletime.cpp
${lsr_SRCS}
${test_SRCS}
)
target_link_libraries(ls_stacktest lsr thread pthread pcre2-8 rt )

add_executable(ls_thrsafetest
lsr/ls_thrsafetest.cpp
${lsr_SRCS}
)
target_link_libraries(ls_thrsafetest lsr thread pthread pcre2-8 )

add_executable(ls_valgrindtest
lsr/ls_valgrindtest.c
${lsr_SRCS}
)
target_link_libraries(ls_valgrindtest lsr thread pthread pcre2-8 )
