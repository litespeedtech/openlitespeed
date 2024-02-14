
cmake_minimum_required(VERSION 3.0)
INCLUDE( ${PROJECT_SOURCE_DIR}/CMakeModules/common.cmake)

set(LUAJITDIR "${PROJECT_SOURCE_DIR}/../third-party/include/luajit-2.1/")
include_directories(${LUAJITDIR})

set(CMAKE_VERBOSE_MAKEFILE ON)
set(MY_CMAKE_WARNING_FLAGS  " -Wall -Wextra -Wno-unused-parameter" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MY_CMAKE_WARNING_FLAGS}" )
#set(CMAKE_C_FLAGS " -Wall " )
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${MY_CMAKE_WARNING_FLAGS}" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${MY_CMAKE_WARNING_FLAGS}" )
set(LUAJITHEADERS ${LUAJITDIR}/luajit.h)

SET(lua_STAT_SRCS
    lsluaengine.cpp
    edluastream.cpp
    lsluaapi.cpp
    lsluasession.cpp
    lsluaheader.cpp
    lsluashared.cpp
    lsluaregex.cpp
    modlua.cpp
)


add_library(mod_lua MODULE ${lua_STAT_SRCS} ${LUAJITHEADERS})
target_link_libraries(mod_lua libluajit.a
    -nodefaultlibs libstdc++.a
    pthread rt
    ${LIBSAN} crypt m gcc_eh c c_nonshared gcc)
set_target_properties(mod_lua PROPERTIES PREFIX "")
