cmake_minimum_required(VERSION 3.0)
set( CMAKE_EXPORT_COMPILE_COMMANDS ON)

Project(openlitespeed VERSION 1.8.4)
INCLUDE( ${PROJECT_SOURCE_DIR}/CMakeModules/common.cmake)
INCLUDE(GNUInstallDirs)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_SKIP_BUILD_RPATH TRUE)

SET(LSQUIC_SERVER_MODE 1)

if (CMAKE_HOST_SYSTEM_NAME MATCHES "Linux" AND 
    CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    if (${ALPINE})
        message("Alpine X64")
    else()
        message("Set LINUX_X64")
        set (LINUX_X64 ON)
    endif()
endif()
if (CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    set (NOT_MACOS OFF)
else()
    set (NOT_MACOS ON)
endif()
OPTION(MOD_PAGESPEED "Whether pagespeed should be built" ${LINUX_X64})
OPTION(MOD_SECURITY "Whether mod_security should be built" ${NOT_MACOS})
OPTION(MOD_LUA "Whether Lua should be built" ${LINUX_X64})
OPTION(MOD_REQPARSER "Whether modreqparser should be built" OFF)
OPTION(MOD_INSPECTOR "Whether modinspector should be built" OFF)
OPTION(MOD_UPLOADPROGRESS "Whether uploadprogress should be built" OFF)
OPTION(MOD_LSPHP "Whether mod_lsphp should be built" OFF)

include_directories( BEFORE  include src/lshpack src/liblsquic/ls-qpack ${PROJECT_SOURCE_DIR}/../third-party/include/)
link_directories( ${PROJECT_SOURCE_DIR}/../third-party/lib/)

#SET (CMAKE_C_COMPILER         "/usr/bin/clang")
#SET (CMAKE_CXX_COMPILER       "/usr/bin/clang++")

IF (CMAKE_BUILD_TYPE MATCHES "Debug")
if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
## thread sanitizer
    #set (TSAN "1")
    set (ASAN "1")
    set (BUILD_TEST "1")
    add_definitions(-DRUN_TEST)
    set(libUnitTest  libUnitTest++.a)
###############################################################################
endif()
ENDIF()


IF (CMAKE_BUILD_TYPE MATCHES "tsan")
    set (TSAN "1")
ENDIF()

IF (CMAKE_BUILD_TYPE MATCHES "asan")
    set (ASAN "1")
ENDIF()

IF (CMAKE_BUILD_TYPE MATCHES "heaptrack")
    set(MY_CMAKE_FLAGS  "-g -O0 ")
    add_definitions(-DASAN_DBG)
    add_definitions(-DDEBUG_POOL)
ENDIF()

IF (CMAKE_BUILD_TYPE MATCHES "prof")
    set(MY_CMAKE_FLAGS "-g -pg -O2 -D_PROFILE_ -DNDEBUG")
ENDIF()

IF (CMAKE_BUILD_TYPE MATCHES "pkgdbg")
    set(MY_CMAKE_FLAGS "-g -O0")
ENDIF()



IF(TSAN)
    set(MY_CMAKE_FLAGS  "-g -O0 -DUSE_THREADCHECK -DDYNAMIC_ANNOTATIONS_ENABLED -fsanitize=thread -fsanitize-blacklist=${PROJECT_SOURCE_DIR}/thread_sanitizer_blacklist")
    add_definitions(-DASAN_DBG)
    set(LIBSAN  libtsan.a)
    #-fsanitize-blacklist=${PROJECT_SOURCE_DIR}/thread_sanitizer_blacklist")
ELSE(TSAN)
    ## address sanitizer
    IF(ASAN)
        set(MY_CMAKE_FLAGS  "-g -O0 -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer")
        add_definitions(-DLS_VG_DEBUG)
        add_definitions(-DASAN_DBG)
        add_definitions(-DDEBUG_POOL)
        set(LIBSAN  libasan.a)
        #-fsanitize-address-use-after-scope
    ENDIF(ASAN)
ENDIF(TSAN)

if (CMAKE_C_COMPILER MATCHES "clang")
    unset(LIBSAN)
endif()

set(MY_CMAKE_FLAGS  "${MY_CMAKE_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-unused-function")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Suppress gcc warnings about BoringSSL's openssl/stack.h
    set(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} -Wno-ignored-qualifiers")
    # TODO Suppress some of our own warnings (we should clean these up):
    set(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} -Wno-unused-result")
    set(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} -Wno-format-truncation")
endif()

if(OPT_SSE4)
set(MY_CMAKE_FLAGS  "${MY_CMAKE_FLAGS} -march=corei7 -maes -mpclmul -mpopcnt")
endif(OPT_SSE4)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MY_CMAKE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MY_CMAKE_FLAGS}")


add_definitions(-DDISABLE_OBJ_POOL)
#set(MY_CMAKE_POOL_FLAG  " -DPOOL_TESTING -DUSE_VALGRIND")
#add_definitions(-DPOOL_TESTING)
#add_definitions(-DLS_WORKCREW_DEBUG)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

#Add definition for QUIC
add_definitions(-DH3_ALPN="\\x02h3\\x05h3-29")

if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    # You must set COMPILE_IO_URING to true and NOT comment out -DIOURING OR
    # set COMPILE_IO_URING to false and comment out -DIOURING
    set(COMPILE_IO_URING
        true)
    add_definitions(-DIOURING)
    # You must set COMPILE_LINUX_AIO to true and NOT comment out -DLS_AIO_USE_LINUX_AIO OR
    # set COMPILE_LINUX_AIO to false and comment out -DLS_AIO_USE_LINUX_AIO
    set(COMPILE_LINUX_AIO
        true)
    add_definitions(-DLS_AIO_USE_LINUX_AIO)
    set(LINUX_AIO_LIB
        aio)
endif()

# Update submodules as needed
find_package(Git QUIET)
if (GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
  option(GIT_SUBMODULE "Check submodules during build" ON)
  if (GIT_SUBMODULE)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --force
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_SUBMOD_RESULT)
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
      message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
    endif()
  endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
#find_package(ZLIB REQUIRED)
##find_package(LibLdap REQUIRED)
#find_package(PCRE REQUIRED)
#find_package(EXPAT REQUIRED)
#find_package(OpenSSL REQUIRED)


#execute_process( COMMAND ${PROJECT_SOURCE_DIR}/dlossl.sh )

##########################################################################################
#If you want to use BoringSSL, just un-comment out the following commands
#AND YOU NEED TO HAVE THE files ready in openlitespeed/ssl
#You can run the dlbssl.sh to make them ready
set(BSSL_ADD_LIB  libdecrepit.a)
##########################################################################################
#If you want to use Brotli Compression, just un-comment out the following commands
set(BROTLI_ADD_LIB  libbrotlidec-static.a libbrotlienc-static.a libbrotlicommon-static.a)
add_definitions(-DUSE_BROTLI)
##########################################################################################
#If you want to use IP2Location, just un-comment out the following commands
set(IP2LOC_ADD_LIB  libIP2Location.a)
add_definitions(-DUSE_IP2LOCATION)
##########################################################################################
#If you want to use IPTOGEO2, just un-comment out the following commands
set(MMDB_LIB  maxminddb)
add_definitions(-DENABLE_IPTOGEO2)
##########################################################################################


add_definitions(-DTEST_OUTPUT_PLAIN_CONF)
#add_definitions(-DLS_VG_DEBUG)
add_definitions(-DDEBUG_POOL)
add_definitions(-DOPENSSL_NO_CXX)

execute_process( COMMAND getconf LFS_CFLAGS
    OUTPUT_VARIABLE  LFS_CFLAGS )

add_definitions( ${LFS_CFLAGS} )
add_definitions(-DUSE_UDNS)

add_subdirectory(src)
add_subdirectory(support)
add_subdirectory(lshostexec)

if (BUILD_TEST)
add_subdirectory(test)
endif()
