cmake_minimum_required(VERSION 3.0)


########### next target ###############

SET(quic_STAT_SRCS quicshm.cpp udplistener.cpp 
    quicengine.cpp
    quicstream.cpp
    pbset.cpp
    #pkt_capture.c
)

add_library(quic STATIC ${quic_STAT_SRCS})

