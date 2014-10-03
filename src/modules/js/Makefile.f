CC=g++
LFSFLAGS= $(shell getconf LFS_CFLAGS)
CFLAGS= -fPIC -g  -Wall -c -D_REENTRANT -I../../../include/ -I./ -I../ -I../../  $(LFSFLAGS)

OS := $(shell uname)
ifeq ($(OS), Darwin)
        LDFLAGS= -fPIC -g -undefined dynamic_lookup  -Wall $(LFSFLAGS) -shared
else
        LDFLAGS= -fPIC -g -Wall $(LFSFLAGS) -shared
endif

SOURCES = lsjsengine.cpp modjs.cpp

$(shell rm *.o)

OBJECTS=$(SOURCES:.cpp=.o)
TARGET  = mod_js.so

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC)  $(OBJECTS) -o $@  $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
        
clean:
	rm *.o
