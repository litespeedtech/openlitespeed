
cmake_minimum_required(VERSION 3.0)
INCLUDE( ${PROJECT_SOURCE_DIR}/CMakeModules/common.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(MY_CMAKE_WARNING_FLAGS  " -Wall -Wextra -Wno-unused-parameter" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MY_CMAKE_WARNING_FLAGS}" )
#set(CMAKE_C_FLAGS " -Wall " )
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${MY_CMAKE_WARNING_FLAGS}" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${MY_CMAKE_WARNING_FLAGS}" )

SET(js_STAT_SRCS 
    lsjsengine.cpp
    modjs.cpp
)


add_library(mod_js MODULE ${js_STAT_SRCS} )
set_target_properties(mod_js PROPERTIES PREFIX "")
