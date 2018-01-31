
cmake_minimum_required(VERSION 2.8)


########### next target ###############

SET(thread_STAT_SRCS
   mtnotifier.cpp
   pthreadcond.cpp
   pthreadmutex.cpp
   pthreadworkqueue.cpp
   thread.cpp
   workcrew.cpp
   worker.cpp
   crewworker.cpp
)

add_library(thread STATIC ${thread_STAT_SRCS} )

