cmake_minimum_required(VERSION 3.0)


########### next target ###############

SET(uwsgi_STAT_SRCS
   uwsgiapp.cpp
   uwsgiappconfig.cpp
   uwsgiconnection.cpp
   uwsgienv.cpp
)

add_library(uwsgi STATIC ${uwsgi_STAT_SRCS})


########### install files ###############


