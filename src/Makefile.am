bin_PROGRAMS = openlitespeed
openlitespeed_SOURCES = modules/prelinkedmods.cpp httpdtest.cpp main.cpp \
   util/httpfetchdriver.cpp \
   util/httpfetch.cpp \
   util/ni_fio.c \
   util/linkobjpool.cpp \
   util/pidfile.cpp \
   util/hashdatacache.cpp \
   util/keydata.cpp \
   util/tlinklist.cpp \
   util/mysleep.c \
   util/rlimits.cpp \
   util/duplicable.cpp \
   util/gfactory.cpp \
   util/fdpass.cpp \
   util/pcregex.cpp \
   util/autostr.cpp \
   util/staticobj.cpp \
   util/pool.cpp \
   util/hashstringmap.cpp \
   util/ghash.cpp \
   util/emailsender.cpp \
   util/guardedapp.cpp \
   util/crashguard.cpp \
   util/iconnection.cpp \
   util/dlinkqueue.cpp \
   util/connpool.cpp \
   util/gzipbuf.cpp \
   util/vmembuf.cpp \
   util/blockbuf.cpp \
   util/stringlist.cpp \
   util/semaphore.cpp \
   util/refcounter.cpp \
   util/gpointerlist.cpp \
   util/linkedobj.cpp \
   util/objpool.cpp \
   util/gpath.cpp \
   util/logtracker.cpp \
   util/poolalloc.cpp \
   util/env.cpp \
   util/xmlnode.cpp \
   util/iovec.cpp \
   util/accesscontrol.cpp \
   util/signalutil.cpp \
   util/loopbuf.cpp \
   util/stringtool.cpp \
   util/tsingleton.cpp \
   util/pcutil.cpp \
   util/daemonize.cpp \
   util/configentry.cpp \
   util/configctx.cpp \
   util/logidtracker.cpp \
   util/datetime.cpp \
   util/misc/profiletime.cpp \
   util/sysinfo/partitioninfo.cpp \
   util/sysinfo/nicdetect.cpp \
   util/sysinfo/systeminfo.cpp \
   lsr/lsr_aho.c \
   lsr/lsr_base64.c \
   lsr/lsr_buf.c \
   lsr/lsr_confparser.c \
   lsr/lsr_crc64.c \
   lsr/lsr_dlinkq.c \
   lsr/lsr_hash.c \
   lsr/lsr_link.c \
   lsr/lsr_lock.c \
   lsr/lsr_loopbuf.c \
   lsr/lsr_map.c \
   lsr/lsr_md5.c \
   lsr/lsr_objarray.c \
   lsr/lsr_pcreg.c \
   lsr/lsr_pool.c \
   lsr/lsr_ptrlist.c \
   lsr/lsr_sha1.c \
   lsr/lsr_str.c \
   lsr/lsr_strlist.c \
   lsr/lsr_strtool.c \
   lsr/lsr_xpool.c


openlitespeed_LDADD =  ./main/libmain.a ./http/libhttp.a ./spdy/libspdy.a \
        ./extensions/libextensions.a ./log4cxx/liblog4cxx.a ./socket/libsocket.a \
        ./sslpp/libsslpp.a  ./ssi/libssi.a ./edio/libedio.a ./lsiapi/liblsiapi.a \
        ./modules/libmodules.a ./shm/liblsshm.a  \
        $(EXPAT_LIBS) $(OPENSSL_LIBS) $(PCRE_LIBS) -lGeoIP -lz -lexpat -lpthread $(DL_LIB_OPTION)


AM_CPPFLAGS = $(PCRE_CFLAGS) $(OPENSSL_INCLUDES) $(EXPAT_CFLAGS)
AM_LDFLAGS = -L/usr/lib $(OPENSSL_LDFLAGS)
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src 
SUBDIRS =  socket http spdy sslpp ssi extensions log4cxx main edio lsiapi  modules shm 


# the library search path.
openlitespeed_LDFLAGS = $(all_libraries)
