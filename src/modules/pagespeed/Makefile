CC=g++
LFSFLAGS= $(shell getconf LFS_CFLAGS)

CFLAGS= -fPIC -g  -Wall -c -D_REENTRANT $(LFSFLAGS)

OS := $(shell uname)
MACHINE_TYPE := $(shell uname -m)

INCLUDE_ARCH=
PSOLPATH=

ifeq ($(OS), Darwin)
        LDFLAGS= -fPIC -g -undefined dynamic_lookup  -Wall $(LFSFLAGS) -shared
	INCLUDE_ARCH=arch/mac/ia32
	$(shell echo Sorry, pagespeed does not support MAC OS)
	
else
    ifeq ($(MACHINE_TYPE), x86_64)
	    INCLUDE_ARCH = arch/linux/x64
	    PSOLPATH = $(shell pwd)/psol/lib/Release/linux/x64/
	else
	    INCLUDE_ARCH = arch/linux/ia32
	    PSOLPATH = $(shell pwd)/psol/lib/Release/linux/ia32/
	endif
	LDFLAGS= -fPIC -g -Wall $(LFSFLAGS) -shared
endif

INCLUDEFILES =-I. -I../.. -I.. -I../../util -I../../../include \
          -I./psol/include \
          -I./psol/include/third_party/chromium/src \
	      -I./psol/include/third_party/protobuf/src \
	      -I./psol/include/third_party/google-sparsehash/src/ \
	      -I./psol/include/third_party/re2/src/ \
	      -I./psol/include/out/Debug/obj/gen/protoc_out/instaweb/ \
	      -I./psol/include/third_party/apr/src/include/ \
	      -I./psol/include/third_party/google-sparsehash/gen/$(INCLUDE_ARCH)/include/ \
	      -I./psol/include/third_party/apr/gen/$(INCLUDE_ARCH)/include/ \
	      -I./psol/include/third_party/css_parser/src/ 

	      
SOURCES =pagespeed.cc log_message_handler.cc ls_message_handler.cc \
        pipenotifier.cc \
        ls_server_context.cc \
        ls_rewrite_driver_factory.cc \
        ls_caching_headers.cc \
        ls_rewrite_options.cc  \
        ls_base_fetch.cc
        

$(shell rm *.o ../../util/*.o)

OBJECTS=$(SOURCES:.cc=.o)
TARGET  = modpagespeed.so

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC)  $(OBJECTS) $(PSOLPATH)pagespeed_automatic.a $(INCLUDEFILES) -o $@  $(LDFLAGS)

.cc.o:
	$(CC)  $(INCLUDEFILES) $(CFLAGS)  $< -o $@
        
clean:
	rm *.o ../../util/*.o
