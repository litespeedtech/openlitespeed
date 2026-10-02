cmake_minimum_required(VERSION 3.0...3.23)

add_definitions(-DNOT_LSCGID)

SET(cmd_ns_SRCS
    cmd_ns.c
    nsnosandbox.c
    nsopts.c
    nspersist.c
    nsutils.c
    ns.c
    ../../src/extensions/cgi/rootcheck.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/extensions/cgi)

add_executable(cmd_ns ${cmd_ns_SRCS} )

target_link_libraries(cmd_ns
    rt )

