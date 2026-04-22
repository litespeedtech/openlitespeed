cmake_minimum_required(VERSION 3.0)


########### next target ###############

SET(scgi_STAT_SRCS
   scgiapp.cpp
   scgiappconfig.cpp
   scgiconnection.cpp
   scgienv.cpp
)

add_library(scgi STATIC ${scgi_STAT_SRCS})


########### install files ###############


