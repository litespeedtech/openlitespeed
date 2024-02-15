cmake_minimum_required(VERSION 3.0)

add_definitions(-DUNMOUNT_NS)

SET(unmount_ns_SRCS
    unmount_ns.c
    nsopts.c
    nspersist.c
    nsutils.c
    ns.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/extensions/cgi)
#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/lib/)

add_executable(unmount_ns ${unmount_ns_SRCS} )

target_link_libraries(unmount_ns rt)



