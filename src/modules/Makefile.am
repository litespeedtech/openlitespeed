####### kdevelop will overwrite this part!!! (begin)##########

####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = libmodules.a

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src

libmodules_a_METASOURCES = AUTO

libmodules_a_SOURCES = modgzip/modgzip.cpp

if HAVE_LIBLUA
SUBDIRS = cache uploadprogress lua
else
SUBDIRS = cache uploadprogress
endif

#noinst_HEADERS = 

