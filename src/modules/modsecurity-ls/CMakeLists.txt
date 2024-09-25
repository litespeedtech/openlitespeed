cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")

include_directories(${PROJECT_SOURCE_DIR}/../third-party/ModSecurity/headers/)

execute_process( COMMAND getconf LFS_CFLAGS
            OUTPUT_VARIABLE  LFS_CFLAGS )

add_definitions( ${LFS_CFLAGS} )

include(CheckCCompilerFlag)
set(EXTRA_FLAGS "")
check_c_compiler_flag(-Wno-unused-function HAS_NO_UNUSED_FUNCTION)
if(HAS_NO_UNUSED_FUNCTION)
    # Silence warnings about unused modsecurity::intervention::clear() et al:
    set(EXTRA_FLAGS "${EXTRA_FLAGS} -Wno-unused-function")
endif()
check_c_compiler_flag(-Wno-sign-compare HAS_NO_SIGN_COMPARE)
if(HAS_NO_SIGN_COMPARE)
    # Silence warnings in rules.h:
    set(EXTRA_FLAGS "${EXTRA_FLAGS} -Wno-sign-compare")
endif()
set_source_files_properties(mod_security.cpp PROPERTIES COMPILE_FLAGS ${EXTRA_FLAGS})

add_library(mod_security SHARED
    mod_security.cpp
)


target_link_libraries(mod_security libmodsecurity.a
    -nodefaultlibs libstdc++.a
    pthread rt -llmdb -lxml2 -lcurl -lssl -lcrypto -lz -lpcre -lyajl
    ${LIBSAN} crypt m gcc_eh c c_nonshared gcc)

set_target_properties(mod_security PROPERTIES PREFIX "")
