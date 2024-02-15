bin_PROGRAMS = unmount_ns

unmount_ns_SOURCES = unmount_ns.c \
		nsopts.c \
		nspersist.c \
		nsutils.c \
		ns.c


AM_LDFLAGS = $(OPENSSL_LDFLAGS) -L/usr/lib -L/usr/local/lib -L/usr/lib64  
AM_CPPFLAGS = -std=c++0x -I$(top_srcdir)/ssl/include/ -I$(top_srcdir)/src/extensions/cgi -I$(top_srcdir)/include -I$(top_srcdir)/src  $(PCRE_CFLAGS) $(EXPAT_CFLAGS)
AM_CXXFLAGS = -std=c++0x   
