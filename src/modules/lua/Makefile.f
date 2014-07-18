CC=g++
LFSFLAGS= $(shell getconf LFS_CFLAGS)
CFLAGS= -fPIC -g  -Wall -c -D_REENTRANT -I../../util/ -I./ -I../ -I../../  $(LFSFLAGS)

OS := $(shell uname)
ifeq ($(OS), Darwin)
        LDFLAGS= -fPIC -g -undefined dynamic_lookup  -Wall $(LFSFLAGS) -shared
else
        LDFLAGS= -fPIC -g -Wall $(LFSFLAGS) -shared
endif

SOURCES =lsluascript.cpp lsluaengine.cpp edluastream.cpp lsluaapi.cpp ls_lua_util.c \
    lsluasession.cpp lsluaheader.cpp lsluashared.cpp modlua.cpp

$(shell rm *.o ../../util/*.o)

OBJECTS=$(SOURCES:.cpp=.o)
TARGET  = mod_lua.so

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC)  $(OBJECTS) -o $@  $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
        
clean:
	rm *.o ../../util/*.o
