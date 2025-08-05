cmake_minimum_required(VERSION 2.8)

SET(lshostexec_SRCS
    lshostexec.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/extensions/cgi)
#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../thirdparty/lib/)

add_executable(lshostexec ${lshostexec_SRCS} )

target_link_libraries(lshostexec
    rt )


