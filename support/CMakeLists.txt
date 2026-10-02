cmake_minimum_required(VERSION 3.0...3.23)


if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    add_subdirectory(unmount_ns)
    add_subdirectory(cmd_ns)
endif()



