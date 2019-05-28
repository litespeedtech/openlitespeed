CC=g++
LFSFLAGS= $(shell getconf LFS_CFLAGS) -D_GLIBCXX_USE_CXX11_ABI=0
INCLUDEPATH= -I../../util/ -I../../../ssl/include  -I./ -I../../../include  -I../ -I../../


ifeq ($(BUILDSTATIC), 1)
    ALLLIB := -nodefaultlibs $(shell g++ -print-file-name='libstdc++.a') -lm -lc -lgcc_eh  -lc_nonshared -lgcc
endif

OS := $(shell uname)
ifeq ($(OS), Darwin)
        LDFLAGS := $(ALLLIB) -fPIC  -undefined dynamic_lookup  
else
        LDFLAGS := $(ALLLIB) -fPIC
endif


#make -f Makefile.f CFG=debug will create a debug version module
ifeq ($(CFG), debug)
        CFLAGS := -fPIC -g -fvisibility=hidden  -Wall -c -D_REENTRANT $(INCLUDEPATH)  $(LFSFLAGS)
        LDFLAGS := $(LDFLAGS) -g3 -O0 -Wall  $(LFSFLAGS) -shared
else
        CFLAGS := -fPIC -g -O2 -fvisibility=hidden  -Wall -c -D_REENTRANT $(INCLUDEPATH)  $(LFSFLAGS)
        LDFLAGS := $(LDFLAGS) -g -O2 -Wall  $(LFSFLAGS) -shared
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
