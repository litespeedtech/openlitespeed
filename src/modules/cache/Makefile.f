CC=g++
LFSFLAGS= $(shell getconf LFS_CFLAGS) -D_GLIBCXX_USE_CXX11_ABI=0
INCLUDEPATH= -I../../util/ -I../../../ssl/include  -I./ -I../../../include  -I../ -I../../
CFLAGS= -fPIC -g -fvisibility=hidden  -Wall -c -D_REENTRANT $(INCLUDEPATH)  $(LFSFLAGS)

ifeq ($(BUILDSTATIC), 1)
    ALLLIB := -nodefaultlibs $(shell g++ -print-file-name='libstdc++.a') -lm -lc -lgcc_eh  -lc_nonshared -lgcc
endif

OS := $(shell uname)
ifeq ($(OS), Darwin)
        LDFLAGS= $(ALLLIB) -fPIC -g -undefined dynamic_lookup  -Wall $(LFSFLAGS) -shared
else
        LDFLAGS= $(ALLLIB) -fPIC -pg -O2  -g -Wall $(LFSFLAGS) -shared
endif

SOURCES =cache.cpp cacheentry.cpp cachehash.cpp cachestore.cpp \
        ceheader.cpp dirhashcacheentry.cpp dirhashcachestore.cpp \
        cacheconfig.cpp cachectrl.cpp  \
        cachemanager.cpp shmcachemanager.cpp

$(shell rm *.o )

OBJECTS=$(SOURCES:.cpp=.o)
TARGET  = cache.so

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(INCLUDEPATH) $(OBJECTS)  -o $@  $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS)  $< -o $@
        
clean:
	rm *.o
