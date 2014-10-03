modulesdir=$(prefix)/modules

modules_LTLIBRARIES=cache.la
cache_la_LDFLAGS= -module -avoid-version -shared

INCLUDES= -I$(top_srcdir)/include -I$(top_srcdir)/src

cache_la_METASOURCES= AUTO

cache_la_SOURCES=cache.cpp cacheentry.cpp cachehash.cpp cachestore.cpp ceheader.cpp dirhashcacheentry.cpp dirhashcachestore.cpp \
        cacheconfig.cpp cachectrl.cpp

#noinst_HEADERS = 

