CC=g++
LFSFLAGS= $(shell getconf LFS_CFLAGS) -D_GLIBCXX_USE_CXX11_ABI=0 -std=gnu++11
INCLUDEPATH= -I../../util/ -I./ -I../../../include  -I../../../../../../thirdparty/include -I../ -I../../ -I./ModSecurity/headers/
CFLAGS= -fPIC -fvisibility=hidden -g -O2 -Wall -c -D_REENTRANT $(INCLUDEPATH)  $(LFSFLAGS)

LIBFLAGS := $(shell pwd)/ModSecurity/src/.libs/libmodsecurity.a -lxml2 -lcurl
ifeq ($(BUILDSTATIC), 1)
	ALLLIB := -nodefaultlibs $(shell g++ -print-file-name='libstdc++.a') -lm -lc -lssl -lcrypto -lpthread -lGeoIP -lz -lpcre -lyajl -lgcc_eh  -lc_nonshared -lgcc
	LIBFLAGS := -L$(shell pwd)/../../../../../../thirdparty/lib -lmodsecurity  -lxml2  -lcurl
endif


OS := $(shell uname)
ifeq ($(OS), Darwin)
	LDFLAGS := $(ALLLIB)  $(LDFLAGS) -fPIC -g -undefined dynamic_lookup  -Wall  $(LFSFLAGS) -shared
else
	LDFLAGS := $(ALLLIB)  $(LDFLAGS) -fPIC -g -Wall   $(LFSFLAGS) -shared
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

