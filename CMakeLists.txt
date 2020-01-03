cmake_minimum_required(VERSION 2.8)
set( CMAKE_EXPORT_COMPILE_COMMANDS ON)

Project(openlitespeed)
INCLUDE( ${PROJECT_SOURCE_DIR}/CMakeModules/common.cmake)

set(openlitespeed_MAJOR_VERSION 1)
set(openlitespeed_MINOR_VERSION 4)
set(openlitespeed_PATCH_VERSION 0)
set(openlitespeed_VERSION
    ${FOOBAR_MAJOR_VERSION}.${FOOBAR_MINOR_VERSION}.${FOOBAR_PATCH_VERSION})

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(LSQUIC_SERVER_MODE 1)

include_directories( BEFORE  include ${PROJECT_SOURCE_DIR}/../thirdparty/include/)
link_directories( ${PROJECT_SOURCE_DIR}/../thirdparty/lib/)

SET (CMAKE_C_COMPILER         "/usr/bin/clang")
SET (CMAKE_CXX_COMPILER       "/usr/bin/clang++")

IF (CMAKE_BUILD_TYPE MATCHES "Debug")
    set (ASAN "1")
    set(MY_CMAKE_POOL_FLAG  " -DPOOL_TESTING -DUSE_VALGRIND")
    add_definitions(-DRUN_TEST)
    set(libUnitTest  libUnitTest++.a)

ELSE ()
    set(MY_CMAKE_WARNING_FLAG  " -Wall -Wextra -Wno-unused-parameter ")
ENDIF()

## thread sanitizer
#set (TSAN "1")
IF(TSAN)
    set(MY_CMAKE_TSAN_FLAG  " -DUSE_THREADCHECK -DDYNAMIC_ANNOTATIONS_ENABLED -fsanitize=thread -fsanitize-blacklist=${PROJECT_SOURCE_DIR}/thread_sanitizer_blacklist")
ELSE(TSAN)
    #set (ASAN "1")
    set(MY_CMAKE_TSAN_FLAG  " ")
ENDIF(TSAN)


## address sanitizer
IF(ASAN)
    set(MY_CMAKE_ASAN_FLAG  " -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer")
    #-fsanitize-address-use-after-scope
ELSE(ASAN)
    set(MY_CMAKE_ASAN_FLAG  " ")
ENDIF(ASAN)

set(MY_CMAKE_WARNING_FLAG  " -Wall -Wextra -Wno-unused-parameter")

#set(MY_CMAKE_POOL_FLAG  " -DPOOL_TESTING -DUSE_VALGRIND")
add_definitions(-DPOOL_TESTING)
#add_definitions(-DLS_WORKCREW_DEBUG)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

#Add definition for QUIC 
add_definitions(-DH3_ALPN="\\x05h3-23")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MY_CMAKE_WARNING_FLAG} ${MY_CMAKE_TSAN_FLAG} ${MY_CMAKE_ASAN_FLAG}")
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS} ${MY_CMAKE_WARNING_FLAG} ${MY_CMAKE_TSAN_FLAG} ${MY_CMAKE_ASAN_FLAG}")


# Update submodules as needed
find_package(Git QUIET)
if (GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
  option(GIT_SUBMODULE "Check submodules during build" ON)
  if (GIT_SUBMODULE)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_SUBMOD_RESULT)
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
      message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
    endif()
  endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
find_package(ZLIB REQUIRED)
##find_package(LibLdap REQUIRED)
find_package(PCRE REQUIRED)
find_package(EXPAT REQUIRED)
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
add_subdirectory(test)
