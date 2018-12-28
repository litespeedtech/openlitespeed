cmake_minimum_required(VERSION 2.8)


########### next target ###############

SET(sslpp_STAT_SRCS
   sslcontextconfig.cpp
   sslengine.cpp
   sslcert.cpp
   sslerror.cpp
   sslconnection.cpp
   sslcontext.cpp
   sslocspstapling.cpp
   sslsesscache.cpp
   sslticket.cpp
   sslutil.cpp
   ocsp/ocsp.c
   ls_fdbuf_bio.c
)

add_library(sslpp STATIC ${sslpp_STAT_SRCS})


