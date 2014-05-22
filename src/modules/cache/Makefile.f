CC=g++
LFSFLAGS= $(shell getconf LFS_CFLAGS)
CFLAGS= -fPIC -g  -Wall -c -D_REENTRANT -I../../util/ -I./ -I../ -I../../  $(LFSFLAGS)

OS := $(shell uname)
ifeq ($(OS), Darwin)
        LDFLAGS= -fPIC -g -undefined dynamic_lookup  -Wall $(LFSFLAGS) -shared
else
        LDFLAGS= -fPIC -g -Wall $(LFSFLAGS) -shared
endif

SOURCES =cache.cpp cacheentry.cpp cachehash.cpp cachestore.cpp ceheader.cpp dirhashcacheentry.cpp dirhashcachestore.cpp \
        cacheconfig.cpp cachectrl.cpp \
        ../../util/crc64.cpp ../../util/autostr.cpp ../../util/datetime.cpp ../../util/stringtool.cpp \
        ../../util/pool.cpp ../../util/stringlist.cpp ../../util/gpointerlist.cpp \
        ../../util/ghash.cpp ../../util/ni_fio.c

$(shell rm *.o ../../util/*.o)

OBJECTS=$(SOURCES:.cpp=.o)
TARGET  = cache.so

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC)  $(OBJECTS) -o $@  $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
        
clean:
	rm *.o ../../util/*.o
