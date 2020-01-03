####### kdevelop will overwrite this part!!! (begin)##########

####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = libmodules.a

# AM_CPPFLAGS =  -I$(top_srcdir)/openssl/include/ -I$(top_srcdir)/include -I$(top_srcdir)/src
AM_CPPFLAGS = -I$(top_srcdir)/openssl/include/ -I$(top_srcdir)/include -I$(top_srcdir)/src  -I$(top_srcdir)/src/modules/cache

libmodules_a_METASOURCES = AUTO

libmodules_a_SOURCES = modgzip/modgzip.cpp \
	cache/cache.cpp cache/cacheentry.cpp cache/cachehash.cpp cache/cachestore.cpp \
	cache/ceheader.cpp cache/dirhashcacheentry.cpp cache/dirhashcachestore.cpp \
        cache/cacheconfig.cpp cache/cachectrl.cpp \
        cache/cachemanager.cpp cache/shmcachemanager.cpp



if HAVE_LIBLUA
SUBDIRS = uploadprogress lua modinspector modreqparser
else
SUBDIRS = uploadprogress modinspector modreqparser
endif

#noinst_HEADERS = 

