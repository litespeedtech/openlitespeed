#! /bin/bash
#
# This script is to download PSOL and extract it to right location
#
#

cd `dirname "$0"`

PSOLVERSION=1.11.33.4

if [ ! -f psol/include/out/Release/obj/gen/net/instaweb/public/version.h ] ; then

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

    TARGET=$PSOLVERSION.tar.gz

    if [ ! -f ../../../../$TARGET ]; then
        curl -O -k  https://dl.google.com/dl/page-speed/psol/$TARGET
        mv $TARGET ../../../..
    fi
    tar -xzvf ../../../../$TARGET # expands to psol/


    if [ "x$USEOLDLIB" = "xyes" ] ; then

    #fix a file which stop the compiling of pagespeed module
    echo .
    cat << EOF > psol/include/pagespeed/kernel/base/scoped_ptr.h
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
cd -

