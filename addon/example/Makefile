LSIAPIDIR = /home/ron/Documents/litespeedtech/openlitespeed

CXX = clang++
#CXX = g++-4.8
CC = clang
#CXXFLAGS = -std=c++11 -Wall -O0 -g
#CXXFLAGS = -Wall -O0 -m32 -g -fno-inline -fno-omit-frame-pointer

# TODO: CXXFLAGS = -I $(LSIAPIDIR)/include -Wall -g -O0 -fPIC

#MY_CMAKE_TSAN_FLAG = -DUSE_THREADCHECK -DDYNAMIC_ANNOTATIONS_ENABLED -fsanitize=thread
#MY_CMAKE_ASAN_FLAG = -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer

CXXFLAGS = $(MY_CMAKE_ASAN_FLAG) $(MY_CMAKE_TSAN_FLAG) -I $(LSIAPIDIR)/src -I $(LSIAPIDIR)/include -Wall -g -fPIC -D_REENTRANT -Werror=return-type -Wno-comment -Wmissing-field-initializers

LDFLAGS = -lgd


ifeq ($(UNAME_S),Darwin)
    CXXFLAGS += "-undefined dynamic_lookup"
endif

CFLAGS = $(CXXFLAGS)

MODEXMPL_SRCS := $(wildcard *.c)

MODEXMPL_OBJS = $(MODEXMPL_SRCS:.c=.o)
MODEXMPL_SOS = $(MODEXMPL_SRCS:.c=.so)

SRCS = $(MODEXMPL_SRCS);
OBJS = $(MODEXMPL_OBJS);
SOS = $(MODEXMPL_SOS);

all: $(SOS)


-include make.Autodepend
