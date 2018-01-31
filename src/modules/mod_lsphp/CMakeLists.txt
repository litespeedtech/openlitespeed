add_custom_target(php ALL COMMAND ${CMAKE_COMMAND} -E env ASAN=${ASAN} TSAN=${TSAN} CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} ./build.sh
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/modules/mod_lsphp)
#set_directory_properties(PROPERTIES  ADDITIONAL_MAKE_CLEAN_FILES "${PROJECT_SOURCE_DIR}/src/test/unittest-cpp/UnitTest++/libUnitTest++.a")
