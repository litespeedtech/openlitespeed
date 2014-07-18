cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CSS_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

include_directories(./;../;../../;../../util/)

execute_process( COMMAND getconf LFS_CFLAGS
            OUTPUT_VARIABLE  LFS_CFLAGS )

add_definitions( ${LFS_CFLAGS} )

add_library(cache SHARED  cacheentry.cpp cachehash.cpp 
            cachestore.cpp ceheader.cpp dirhashcacheentry.cpp 
            dirhashcachestore.cpp cache.cpp cacheconfig.cpp cachectrl.cpp 
            ../../util/crc64.cpp
            ../../util/datetime.cpp
            ../../util/stringtool.cpp
            ../../util/autostr.cpp
            )
set_target_properties(cache PROPERTIES PREFIX "")
