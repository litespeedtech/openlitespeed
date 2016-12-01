cmake_minimum_required(VERSION 2.8)

Project(openlitespeed)
INCLUDE( ${PROJECT_SOURCE_DIR}/CMakeModules/common.cmake)

set(openlitespeed_MAJOR_VERSION 1)
set(openlitespeed_MINOR_VERSION 4)
set(openlitespeed_PATCH_VERSION 0)
set(openlitespeed_VERSION
  ${FOOBAR_MAJOR_VERSION}.${FOOBAR_MINOR_VERSION}.${FOOBAR_PATCH_VERSION})

SET(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories( BEFORE ssl/include )
include_directories( include )

SET (CMAKE_C_COMPILER         "/usr/bin/clang")
SET (CMAKE_CXX_COMPILER       "/usr/bin/clang++")

set(MY_CMAKE_WARNING_FLAG  " -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  ${MY_CMAKE_WARNING_FLAG}")
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${MY_CMAKE_WARNING_FLAG}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${MY_CMAKE_WARNING_FLAG}")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${MY_CMAKE_WARNING_FLAG}")


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
find_package(ZLIB REQUIRED)
##find_package(LibLdap REQUIRED)
find_package(PCRE REQUIRED)
find_package(EXPAT REQUIRED)
#find_package(OpenSSL REQUIRED)


execute_process( COMMAND ${PROJECT_SOURCE_DIR}/dlossl.sh )

add_subdirectory(src)



