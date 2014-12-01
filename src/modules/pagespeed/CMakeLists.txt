cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CSS_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

execute_process( COMMAND uname -m
                OUTPUT_VARIABLE  MACHINE_TYPE )
string(STRIP ${MACHINE_TYPE} MACHINE_TYPE)

if( "${MACHINE_TYPE}" STREQUAL "x86_64" )
    set(OSVERSION "x64")
else()
    set(OSVERSION "ia32")
endif()


include_directories(./;../;../../;../../util/;
                ./psol/include/;
                ./psol/include/third_party/apr/src/include/;
                ./psol/include/third_party/chromium/src/;
                ./psol/include/third_party/protobuf/src/;
                ./psol/include/third_party/google-sparsehash/src/;
                ./psol/include/third_party/re2/src/;
                ./psol/include/out/Debug/obj/gen/protoc_out/instaweb/;
                ./psol/include/third_party/apr/gen/arch/linux/${OSVERSION}/include/;
                ./psol/include/third_party/css_parser/src;
                ./psol/include/third_party/google-sparsehash/gen/arch/linux/${OSVERSION}/include/)

execute_process( COMMAND getconf LFS_CFLAGS
            OUTPUT_VARIABLE  LFS_CFLAGS )

add_definitions( ${LFS_CFLAGS} )

add_library(modpagespeed SHARED  pagespeed.cc  log_message_handler.cc ls_message_handler.cc
            pipenotifier.cc
            ls_server_context.cc
            ls_rewrite_driver_factory.cc
            ls_caching_headers.cc
            ls_rewrite_options.cc 
            ls_base_fetch.cc
            )

target_link_libraries(modpagespeed ${PROJECT_SOURCE_DIR}/src/modules/pagespeed/psol/lib/Debug/linux/${OSVERSION}/pagespeed_automatic.a)
set_target_properties(modpagespeed PROPERTIES PREFIX "")

