cmake_minimum_required(VERSION 3.0)

add_subdirectory(modgzip)
add_subdirectory(cache)

if (MOD_REQPARSER)
    message("Make mod_reqparser")
    add_subdirectory(modreqparser)
endif()

if (MOD_PAGESPEED)
    message("Make mod_pagespeed")
    add_subdirectory(pagespeed)
endif()

if (MOD_LUA)
    message("Make mod_lua")
    add_subdirectory(lua)
endif()

if (MOD_INSPECTOR)
    message("Make mod_inspector")
    add_subdirectory(modinspector)
endif()

if (MOD_UPLOADPROGRESS)
    message("Make UploadProgress")
    add_subdirectory(uploadprogress)
endif()

if (MOD_LSPHP)
    message("Make mod_lsphp")
    add_subdirectory(mod_lsphp)
endif()

if (MOD_SECURITY)
    message("Make mod_security")
    add_subdirectory(modsecurity-ls)
endif()

