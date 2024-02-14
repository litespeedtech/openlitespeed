#! /bin/bash
#
# This script is to download PSOL and extract it to right location
#


DLCMD=
source ../../../dist/functions.sh 2>/dev/null
if [ $? != 0 ] ; then
    . ../../../dist/functions.sh
    if [ $? != 0 ] ; then
        echo [ERROR] Can not include 'functions.sh'.
        exit 1
    fi
fi


pushd .
cd `dirname "$0"`

if [ ! -d ../../../../third-party ] ; then
   mkdir ../../../../thirdp-arty
fi

PSOLVERSION=1.11.33.4

USEOLDLIB=no

GCCVER=`gcc -dumpfullversion -dumpversion`
IFS='.';
parts=( $GCCVER )
unset IFS;
if [ x${parts[2]} = 'x' ] ; then
    verval=$(( 1000000 * ${parts[0]} + 1000 * ${parts[1]} ))
else
    verval=$(( 1000000 * ${parts[0]} + 1000 * ${parts[1]} + ${parts[2]} ))
fi

if [ $verval -lt 4008000 ] ; then
    echo "Warning: your gcc $GCCVER is too low(less than 4.8.0) to use PSOL $PSOLVERSION"
    PSOLVERSION=1.11.33.3
    echo "         Will use PSOL $PSOLVERSION instead."
    USEOLDLIB=yes
else
    echo
fi

if [ "x$1" != "x" ]; then
   ln -sf ../../../../third-party/psol-$PSOLVERSION psol
fi


cd ../../../../third-party

if [ ! -f psol-$PSOLVERSION/include/out/Release/obj/gen/net/instaweb/public/version.h ] ; then

    TARGET=$PSOLVERSION.tar.gz

    if [ ! -f $TARGET ] ; then
        detectdlcmd
        $DLCMD $TARGET https://dl.google.com/dl/page-speed/psol/$TARGET
    fi
    tar -xzvf $TARGET # expands to psol/
    mv psol psol-$PSOLVERSION

    
    if [ "x$USEOLDLIB" = "xyes" ] ; then
   
    #fix a file which stop the compiling of pagespeed module
    echo .
    cat << EOF > psol-$PSOLVERSION/include/pagespeed/kernel/base/scoped_ptr.h
/**
* Due the compiling issue, this file was updated from the original file.
*/
#ifndef PAGESPEED_KERNEL_BASE_SCOPED_PTR_H_
#define PAGESPEED_KERNEL_BASE_SCOPED_PTR_H_
#include "base/memory/scoped_ptr.h"

namespace net_instaweb {
template<typename T> class scoped_array : public scoped_ptr<T[]> {
public:
    scoped_array() : scoped_ptr<T[]>() {}
    explicit scoped_array(T* t) : scoped_ptr<T[]>(t) {}
};
}
#endif

EOF
    fi
fi
popd


