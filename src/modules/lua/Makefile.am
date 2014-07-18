modulesdir=$(prefix)/modules

modules_LTLIBRARIES=mod_lua.la
mod_lua_la_LDFLAGS= -module -avoid-version -shared

INCLUDES= -I$(top_srcdir)/src -I$(LUA_INCLUDES)

mod_lua_la_METASOURCES= AUTO

mod_lua_la_SOURCES=lsluascript.cpp lsluaengine.cpp edluastream.cpp lsluaapi.cpp ls_lua_util.c \
    lsluasession.cpp lsluaheader.cpp lsluashared.cpp modlua.cpp

#noinst_HEADERS = 
