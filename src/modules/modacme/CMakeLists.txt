cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

add_library(mod_acme SHARED  mod_acme.c)

# FreeBSD's C++ runtime is libc++; libstdc++ does not exist there.
if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "FreeBSD")
  set(STDCXX libc++.a)
else()
  set(STDCXX libstdc++.a)
endif()

target_link_libraries(mod_acme
    -nodefaultlibs ${STDCXX}
    pthread rt
    ${LIBSAN} crypt m gcc_eh c c_nonshared gcc)

set_target_properties(mod_acme PROPERTIES PREFIX "")
