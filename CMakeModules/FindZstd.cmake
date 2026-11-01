# - Try to find the Zstandard (zstd) compression library
# Once done this will define
#
#  ZSTD_FOUND        - system has the zstd library (and it is new enough)
#  ZSTD_INCLUDE_DIR   - the zstd include directory
#  ZSTD_LIBRARIES     - the libraries needed to use zstd
#  ZSTD_VERSION       - the version string reported by zstd.h, if found
#
# Detection order:
#   1) pkg-config (libzstd.pc), when available
#   2) standard find_path()/find_library() search, honoring
#      ZSTD_ROOT/ZSTD_INCLUDE_DIR/ZSTD_LIBRARY hints and the paths
#      reported by pkg-config above
#
# Minimum required version: zstd 1.4.0, since this project relies on the
# advanced streaming API (ZSTD_CCtx_setParameter/ZSTD_compressStream2)
# introduced in that release.

if (ZSTD_INCLUDE_DIR AND ZSTD_LIBRARY)
    # Already in cache, be silent
    set(ZSTD_FIND_QUIETLY TRUE)
endif ()

if (NOT WIN32)
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(PC_ZSTD QUIET libzstd)
    endif ()
    set(ZSTD_DEFINITIONS ${PC_ZSTD_CFLAGS_OTHER})
endif ()

find_path(ZSTD_INCLUDE_DIR
    NAMES zstd.h
    HINTS ${ZSTD_ROOT} ${PC_ZSTD_INCLUDEDIR} ${PC_ZSTD_INCLUDE_DIRS}
    PATH_SUFFIXES include
)

find_library(ZSTD_LIBRARY
    NAMES zstd libzstd zstd_static
    HINTS ${ZSTD_ROOT} ${PC_ZSTD_LIBDIR} ${PC_ZSTD_LIBRARY_DIRS}
    PATH_SUFFIXES lib lib64
)

if (ZSTD_INCLUDE_DIR AND EXISTS "${ZSTD_INCLUDE_DIR}/zstd.h")
    file(STRINGS "${ZSTD_INCLUDE_DIR}/zstd.h" _zstd_major_line
         REGEX "^#define[ \t]+ZSTD_VERSION_MAJOR[ \t]+[0-9]+")
    file(STRINGS "${ZSTD_INCLUDE_DIR}/zstd.h" _zstd_minor_line
         REGEX "^#define[ \t]+ZSTD_VERSION_MINOR[ \t]+[0-9]+")
    file(STRINGS "${ZSTD_INCLUDE_DIR}/zstd.h" _zstd_release_line
         REGEX "^#define[ \t]+ZSTD_VERSION_RELEASE[ \t]+[0-9]+")
    string(REGEX REPLACE ".*ZSTD_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1"
           ZSTD_VERSION_MAJOR "${_zstd_major_line}")
    string(REGEX REPLACE ".*ZSTD_VERSION_MINOR[ \t]+([0-9]+).*" "\\1"
           ZSTD_VERSION_MINOR "${_zstd_minor_line}")
    string(REGEX REPLACE ".*ZSTD_VERSION_RELEASE[ \t]+([0-9]+).*" "\\1"
           ZSTD_VERSION_RELEASE "${_zstd_release_line}")
    if (ZSTD_VERSION_MAJOR)
        set(ZSTD_VERSION "${ZSTD_VERSION_MAJOR}.${ZSTD_VERSION_MINOR}.${ZSTD_VERSION_RELEASE}")
    endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Zstd
    REQUIRED_VARS ZSTD_LIBRARY ZSTD_INCLUDE_DIR
    VERSION_VAR ZSTD_VERSION
)

if (ZSTD_FOUND AND ZSTD_VERSION AND ZSTD_VERSION VERSION_LESS "1.4.0")
    message(STATUS "Found zstd ${ZSTD_VERSION}, but version 1.4.0+ is "
                    "required for streaming Accept-Encoding: zstd support; "
                    "disabling zstd support.")
    set(ZSTD_FOUND FALSE)
endif ()

if (ZSTD_FOUND)
    set(ZSTD_LIBRARIES ${ZSTD_LIBRARY})
endif ()

mark_as_advanced(ZSTD_INCLUDE_DIR ZSTD_LIBRARY ZSTD_LIBRARIES)
