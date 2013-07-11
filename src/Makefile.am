bin_PROGRAMS = openlitespeed
openlitespeed_SOURCES = httpdtest.cpp main.cpp
openlitespeed_LDADD =  ./main/libmain.a ./http/libhttp.a \
        ./extensions/libextensions.a ./log4cxx/liblog4cxx.a \
        ./socket/libsocket.a ./sslpp/libsslpp.a  ./ssi/libssi.a ./edio/libedio.a ./util/libutil.a \
        -lexpat -lssl -lz -lcrypto -lpcre -lGeoIP



AM_LDFLAGS = -L/usr/lib
INCLUDES = -I$(top_srcdir)/src 
SUBDIRS =  socket util http sslpp ssi extensions log4cxx main edio


# the library search path.
openlitespeed_LDFLAGS = $(all_libraries)
