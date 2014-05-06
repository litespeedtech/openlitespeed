bin_PROGRAMS = openlitespeed
openlitespeed_SOURCES = modules/prelinkedmods.cpp httpdtest.cpp main.cpp
openlitespeed_LDADD =  ./main/libmain.a ./http/libhttp.a ./spdy/libspdy.a \
        ./extensions/libextensions.a ./log4cxx/liblog4cxx.a ./socket/libsocket.a \
        ./sslpp/libsslpp.a  ./ssi/libssi.a ./edio/libedio.a ./util/libutil.a ./lsiapi/liblsiapi.a \
        ./modules/libmodules.a \
        $(EXPAT_LIBS) $(OPENSSL_LIBS) $(PCRE_LIBS) -lGeoIP -lz -lexpat -lpthread $(DL_LIB_OPTION)


AM_CPPFLAGS = $(PCRE_CFLAGS) $(OPENSSL_INCLUDES) $(EXPAT_CFLAGS)
AM_LDFLAGS = -L/usr/lib $(OPENSSL_LDFLAGS)
INCLUDES = -I$(top_srcdir)/src 
SUBDIRS =  socket util http spdy sslpp ssi extensions log4cxx main edio lsiapi  modules


# the library search path.
openlitespeed_LDFLAGS = $(all_libraries)
