####### kdevelop will overwrite this part!!! (begin)##########

####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = libmodules.a

INCLUDES = -I$(top_srcdir)/src

libmodules_a_METASOURCES = AUTO

libmodules_a_SOURCES = modgzip/modgzip.cpp \
  ../../addon/example/loopbuff.c 

if HAVE_LIBLUA
SUBDIRS = cache lua
else
SUBDIRS = cache
endif

#noinst_HEADERS = 

