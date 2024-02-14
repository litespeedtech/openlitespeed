CC=g++
LFSFLAGS= $(shell getconf LFS_CFLAGS) -D_GLIBCXX_USE_CXX11_ABI=0 -std=gnu++11
INCLUDEPATH= -I../../util/ -I./ -I../../../include  -I../../../../../../thirdparty/include -I../ -I../../ -I./ModSecurity/headers/

LIBFLAGS := $(shell pwd)/ModSecurity/src/.libs/libmodsecurity.a -lxml2 -lcurl -lyajl 
ifeq ($(BUILDSTATIC), 1)
	ALLLIB := -nodefaultlibs $(shell g++ -print-file-name='libstdc++.a') -lm -lc -lssl -lcrypto -lpthread -lz -lpcre -lyajl -lgcc_eh  -lc_nonshared -lgcc
	LIBFLAGS := -L$(shell pwd)/../../../../../../thirdparty/lib -lmodsecurity  -lxml2  -lcurl -lyajl 
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



SOURCES = mod_security.cpp
$(shell ./dllibmodsecurity.sh >&2)
$(shell rm *.o )

OBJECTS=$(SOURCES:.cpp=.o)
TARGET  = mod_security.so

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(INCLUDEPATH) $(OBJECTS) $(LIBFLAGS) -o $@  $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS)  $< -o $@
        
clean:
	rm *.o

