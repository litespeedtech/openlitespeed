cmake_minimum_required(VERSION 2.8)

cmake_policy(SET CMP0002 OLD)

add_subdirectory(stl)
add_subdirectory(misc)
add_subdirectory(sysinfo)


########### next target ###############

SET(util_STAT_SRCS
   httpfetchdriver.cpp
   httpfetch.cpp
   ni_fio.c
   linkobjpool.cpp
   pidfile.cpp
   hashdatacache.cpp
   keydata.cpp
   tlinklist.cpp
   mysleep.c
   rlimits.cpp
   duplicable.cpp
   gfactory.cpp
   fdpass.cpp
   pcregex.cpp
   autostr.cpp
   staticobj.cpp
   pool.cpp
   hashstringmap.cpp
   ghash.cpp
   emailsender.cpp
   guardedapp.cpp
   crashguard.cpp
   iconnection.cpp
   dlinkqueue.cpp
   connpool.cpp
   gzipbuf.cpp
   vmembuf.cpp
   blockbuf.cpp
   stringlist.cpp
   semaphore.cpp
   refcounter.cpp
   gpointerlist.cpp
   linkedobj.cpp
   objpool.cpp
   gpath.cpp
   logtracker.cpp
   gstring.cpp
   poolalloc.cpp
   env.cpp
   xmlnode.cpp
   iovec.cpp
   accesscontrol.cpp
   signalutil.cpp
   loopbuf.cpp
   stringtool.cpp
   tsingleton.cpp
   base64.cpp
   pcutil.cpp
   daemonize.cpp
   autobuf.cpp
   configentry.cpp 
   configctx.cpp 
   logidtracker.cpp
   datetime.cpp
   gmap.cpp
)

add_library(util STATIC ${util_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

######## kdevelop will overwrite this part!!! (begin)##########
#noinst_LIBRARIES = libutil.a
#
#INCLUDES = $(all_includes)
#
#libutil_a_METASOURCES = AUTO
#
#libutil_a_SOURCES = httpfetch.cpp ni_fio.c linkobjpool.cpp pidfile.cpp hashdatacache.cpp keydata.cpp tlinklist.cpp mysleep.c rlimits.cpp duplicable.cpp gfactory.cpp fdpass.cpp pcregex.cpp autostr.cpp staticobj.cpp pool.cpp hashstringmap.cpp ghash.cpp emailsender.cpp guardedapp.cpp crashguard.cpp iconnection.cpp dlinkqueue.cpp connpool.cpp gzipbuf.cpp vmembuf.cpp blockbuf.cpp stringlist.cpp semaphore.cpp refcounter.cpp gpointerlist.cpp linkedobj.cpp objpool.cpp gpath.cpp logtracker.cpp gstring.cpp poolalloc.cpp env.cpp xmlnode.cpp iovec.cpp accesscontrol.cpp signalutil.cpp loopbuf.cpp stringtool.cpp tsingleton.cpp base64.cpp pcutil.cpp daemonize.cpp 
#
#SUBDIRS = stl misc sysinfo 
#
#EXTRA_DIST = daemonize.cpp daemonize.h pcutil.cpp pcutil.h base64.cpp base64.h tsingleton.cpp tsingleton.h stringtool.cpp stringtool.h loopbuf.cpp loopbuf.h signalutil.cpp signalutil.h accesscontrol.cpp accesscontrol.h iovec.cpp iovec.h xmlnode.cpp xmlnode.h env.cpp env.h poolalloc.cpp poolalloc.h gstring.cpp gstring.h logtracker.cpp logtracker.h gpath.h gpath.cpp objpool.cpp objpool.h linkedobj.cpp linkedobj.h ienv.h gpointerlist.cpp gpointerlist.h refcounter.cpp refcounter.h semaphore.cpp semaphore.h stringlist.cpp stringlist.h blockbuf.cpp blockbuf.h vmembuf.cpp vmembuf.h gzipbuf.cpp gzipbuf.h connpool.cpp connpool.h dlinkqueue.cpp dlinkqueue.h iconnection.cpp iconnection.h crashguard.cpp crashguard.h guardedapp.cpp guardedapp.h emailsender.cpp emailsender.h ghash.cpp ghash.h hashstringmap.cpp hashstringmap.h pool.cpp pool.h swap.h staticobj.cpp staticobj.h autostr.cpp autostr.h pcregex.cpp pcregex.h fdpass.cpp fdpass.h gfactory.cpp gfactory.h duplicable.cpp duplicable.h rlimits.cpp rlimits.h mysleep.h
#     mysleep.c tlinklist.cpp tlinklist.h keydata.cpp keydata.h hashdatacache.cpp hashdatacache.h pidfile.cpp pidfile.h linkobjpool.cpp linkobjpool.h accessdef.h ni_fio.h ni_fio.c httpfetch.cpp httpfetch.h 
#
######## kdevelop will overwrite this part!!! (end)############
